/* This file is part of COVISE.

   You can use it under the terms of the GNU Lesser General Public License
   version 2.1 or later, see lgpl-2.1.txt.

 * License: LGPL 2+ */


#include "Flux2.h"
#include <util/UDPComm.h>
#include <config/CoviseConfig.h>
#include <util/byteswap.h>
#include <util/unixcompat.h>
#include <cover/coVRMSController.h>
#include <cover/VRSceneGraph.h>
#include <osg/LineSegment>
#include <osgUtil/IntersectionVisitor>
#include <osgUtil/LineSegmentIntersector>
#include "cover/coIntersection.h"
#include <OpenVRUI/osg/mathUtils.h> //for MAKE_EULER_MAT
#include <cover/input/input.h>
#include <cover/input/deviceDiscovery.h>
using namespace opencover;

//To be added
//static float zeroAngle = _;

Flux2::Flux2()
	: udp(NULL), coVRNavigationProvider("Flux2", this)
{
	fluxData.brake = 0.0;
	fluxData.steeringAngle = 0.0;
	fluxData.speed = 0.0;
	fluxData.button = 0;
	fluxControl.cmd = 0;
	fluxControl.value = 0;
	resistance = 50.0;
	stepSizeUp = 2000;
	stepSizeDown = 2000;
	speed = 0.0;
	prevSpeed = 9999.0;
	counter = 0;
	coVRNavigationManager::instance()->registerNavigationProvider(this);
}

Flux2::~Flux2()
{
	delete flux2;
	coVRNavigationManager::instance()->unregisterNavigationProvider(this);
	running = false;

	delete udp;
}

bool Flux2::init()
{
	delete flux2;
	float floorHeight = VRSceneGraph::instance()->floorHeight();

	float x = covise::coCoviseConfig::getFloat("x", "COVER.Plugin.Flux2.Position", 0);
	float y = covise::coCoviseConfig::getFloat("y", "COVER.Plugin.Flux2.Position", 0);
	float z = covise::coCoviseConfig::getFloat("z", "COVER.Plugin.Flux2.Position", floorHeight);
	float h = covise::coCoviseConfig::getFloat("h", "COVER.Plugin.Flux2.Position", 0);
	float p = covise::coCoviseConfig::getFloat("p", "COVER.Plugin.Flux2.Position", 0);
	float r = covise::coCoviseConfig::getFloat("r", "COVER.Plugin.Flux2.Position", 0);

	MAKE_EULER_MAT(Flux2Pos, h, p, r);
	Flux2Pos.postMultTranslate(osg::Vec3(x, y, z));
	flux2 = nullptr;
	if (coVRMSController::instance()->isMaster())
	{
		flux2 = new Flux2();
	}
	const std::string host = covise::coCoviseConfig::getEntry("value", "COVER.Plugin.Flux2.serverHost", "192.168.178.36");
	unsigned short serverPort = covise::coCoviseConfig::getInt("COVER.Plugin.Flux2.serverPort", 31319);
	unsigned short localPort = covise::coCoviseConfig::getInt("COVER.Plugin.Flux2.localPort", 31322);

	std::cerr << "Flux2 config: UDP: serverHost: " << host << ", localPort: " << localPort << ", serverPort: " << serverPort << std::endl;
	bool ret = false;

	if (coVRMSController::instance()->isMaster())
	{
		std::string host = "";
		for (const auto& i : opencover::Input::instance()->discovery()->getDevices())
		{
			if (i->pluginName == "Flux2")
			{
				host = i->address;
				udp = new UDPComm(host.c_str(), serverPort, localPort);
				if (!udp->isBad())
				{
					ret = true;
					start();
				}
				else
				{
					std::cerr << "Flux2: falided to open local UDP port" << localPort << std::endl;
					ret = false;
				}
				break;
			}
		}

		coVRMSController::instance()->sendSlaves(&ret, sizeof(ret));
	}
	else
	{
		coVRMSController::instance()->readMaster(&ret, sizeof(ret));
	}
	return ret;
}

bool Flux2::update()
{
	if (isEnabled())
	{
		if (coVRMSController::instance()->isMaster())
		{
			if (flux2 != nullptr)
			{
				flux2->updateThread();
				double dT = cover->frameDuration();

				TransformMat = VRSceneGraph::instance()->getTransform()->getMatrix();

				float a = getYAcceleration();
				if (a > 0)
				{
					a = 0;
				}
				// need to write setForce()
				//flux2->setForce(-a / 2.0);
				//fprintf(stderr, "force: %f\n", -a / 2.0);
				speed = getSpeed() - getBrakeForce()/BRAKE_SCALE_FACTOR;
				if (speed < 0)
				{
					speed = 0;
				}

				if (getBrakeForce() > BRAKE_THRESHOLD)
				{
					counter++;
					prevSpeed = getSpeed();
					fprintf(stderr, "Counter: %d\n", counter);
				}
				if (counter > COUNTER_THRESHOLD && (prevSpeed > getSpeed())) // hold brake for long enough that it should stop even if brake is released
				{
					speed = 0;
					fprintf(stderr, "Hold brake\n");
				}
				else if (prevSpeed < getSpeed()) // speeding up
				{
					counter = 0;
					prevSpeed = 9999.0;
					speed = getSpeed() - getBrakeForce() / BRAKE_SCALE_FACTOR; // reach actual speed
					fprintf(stderr, "Speeding up!\n");
				}
				float s = speed * dT;
				//fprintf(stderr, "Displacement: %lf   Speed: %lf    dt: %lf Y-acceleration: %lf\n", s, speed, dT, a);

				osg::Vec3 V(0, s, 0);
				float rotAngle = 0.0;
				if ((s < 0.0001 && s > -0.0001)) // straight
				{
					//fprintf(stderr,"bikeTrans: %f %f %f\n",bikeTrans(3, 0), bikeTrans(3, 1), bikeTrans(3, 2) );
					//fprintf(stderr,"V: %f %f %f\n",V[0], V[1], V[2] );
				}
				float wheelAngle = getAngle();
				float r = tan(M_PI_2 - wheelAngle * 0.2 / (((fabs(speed) * 0.2) + 1))) * wheelBase;
				float u = 2.0 * r * M_PI;
				rotAngle = (s / u) * 2.0 * M_PI;
				V[1] = r * sin(rotAngle);
				V[0] = (r - r * cos(rotAngle));


				osg::Matrix relTrans;
				osg::Matrix relRot;
				relRot.makeRotate(-rotAngle, 0, 0, 1);
				relTrans.makeTranslate(V * -1000); // m to mm (move world in the opposite direction

				auto mat = getMatrix();

				TransformMat = mat * relTrans * relRot;
			}

			coVRMSController::instance()->sendSlaves((char*)TransformMat.ptr(), sizeof(TransformMat));
		}
		else
		{
			coVRMSController::instance()->readMaster((char*)TransformMat.ptr(), sizeof(TransformMat));
		}
		VRSceneGraph::instance()->getTransform()->setMatrix(TransformMat);
	}
	return false;
}

float Flux2::getYAcceleration()
{
	osg::Vec3d x{ TransformMat(0, 0), TransformMat(0, 1), TransformMat(0, 2) };
	x.normalize();
	osg::Vec3d y{ TransformMat(1, 0), TransformMat(1, 1), TransformMat(1, 2) };
	y.normalize();
	osg::Vec3d z_yz{ 0, TransformMat(2, 1), TransformMat(2, 2) };
	z_yz.normalize();

	float cangle = 1.0 - z_yz * osg::Vec3(0, 0, 1);
	if (z_yz[1] > 0)
		cangle *= -1;
	//fprintf(stderr,"z_yz %f x0 %f sprod: %f\n",z_yz[1],x[0],cangle);
	float a = cangle * 9.81;
	return a;
}

void Flux2::setEnabled(bool flag)
{
	coVRNavigationProvider::setEnabled(flag);
	if (udp)
	{
		if (flag)
		{
			udp->send("start");
		}
		else
		{
			udp->send("stop");
		}
	}
	//WakeUp Flux2
	Initialize();
}

void Flux2::run()
{
	running = true;
	doStop = false;
	if (coVRMSController::instance()->isMaster())
	{
		while (running && !doStop)
		{
			usleep(5000);
			this->updateThread();
		}
	}
}

void Flux2::updateThread()
{
	if (udp)
	{
		char tmpBuf[10000];
		int status = udp->receive(&tmpBuf, 10000);

		if (status > 0 && status >= sizeof(FluxData))
		{
			OpenThreads::ScopedLock<OpenThreads::Mutex> lock(mutex);
			memcpy(&fluxData, tmpBuf, sizeof(FluxData));
			fprintf(stderr, "Brake: %lf Steering Angle: %lf Speed: %lf\n", fluxData.brake, fluxData.steeringAngle, fluxData.speed);
			updateResistance();
			sendResistance();
		}
		else if (status == -1)
		{
			if (isEnabled()) // otherwise we are not supposed to receive anything
			{
				std::cerr << "Flux2::update: error while reading data" << std::endl;
				if (udp) // try to wake up the flux (if the first start UDP message was lost)
					udp->send("start");
			}
			return;
		}
		else
		{
			std::cerr << "Flux2::update: received invalid no. of bytes: recv=" << status << ", got=" << status << std::endl;
			return;
		}
	}
}

void Flux2::Initialize()
{
	if (coVRMSController::instance()->isMaster())
	{
		fluxControl.cmd = 1;

		ret = udp->send(&fluxControl, sizeof(FluxCtrlData));
	}
}

float Flux2::getSpeed()
{
	return fluxData.speed/545.0; // speed in m/s
}

float Flux2::getAngle()
{
	return fluxData.steeringAngle; 
}

float Flux2::getBrakeForce()
{
	// prevents brake force to be negative
	if (fluxData.brake < 0) {
		return 0;
	}
	return fluxData.brake;
}

void Flux2::updateResistance()
{
	resistance = getBrakeForce();
}

void Flux2::sendResistance()
{
	ret = udp->send(&resistance, sizeof(float));
}

osgUtil::LineSegmentIntersector::Intersection getFirstIntersection(osg::ref_ptr<osg::LineSegment> ray, bool* haveISect) {

	//  just adjust height here

	osg::ref_ptr<osgUtil::IntersectorGroup> igroup = new osgUtil::IntersectorGroup;
	osg::ref_ptr<osgUtil::LineSegmentIntersector> intersector;
	intersector = coIntersection::instance()->newIntersector(ray->start(), ray->end());
	igroup->addIntersector(intersector);

	osgUtil::IntersectionVisitor visitor(igroup);
	visitor.setTraversalMask(Isect::Walk);
	VRSceneGraph::instance()->getTransform()->accept(visitor);

	*haveISect = intersector->containsIntersections();
	if (!*haveISect) {
		return {};
	}

	return intersector->getFirstIntersection();
}

osg::Matrix Flux2::getMatrix() {
	float wheelDis = wheelBase * 1000;
	osg::Vec3 pos = Flux2Pos.getTrans();
	osg::Vec3d y{ Flux2Pos(1, 0), Flux2Pos(1, 1), Flux2Pos(1, 2) };
	osg::Vec3 rearPos = pos + y * -wheelDis;


	osg::ref_ptr<osg::LineSegment> rayFront;
	{
		// down segment
		osg::Vec3 p0, q0;
		p0.set(pos[0], pos[1], pos[2] + stepSizeUp);
		q0.set(pos[0], pos[1], pos[2] - stepSizeDown);

		rayFront = new osg::LineSegment(p0, q0);
	}
	osg::ref_ptr<osg::LineSegment> rayBack;
	{
		// down segment
		osg::Vec3 p0, q0;

		p0.set(rearPos[0], rearPos[1], rearPos[2] + stepSizeUp);
		q0.set(rearPos[0], rearPos[1], rearPos[2] - stepSizeDown);

		rayBack = new osg::LineSegment(p0, q0);
	}
	bool intersects;
	auto front = getFirstIntersection(rayFront, &intersects);
	if (!intersects) {
		return TransformMat;
	}
	auto back = getFirstIntersection(rayBack, &intersects);
	if (!intersects) {
		return TransformMat;
	}

	auto frontNormal = front.getWorldIntersectNormal();
	frontNormal.normalize();
	auto backNormal = back.getWorldIntersectNormal();
	backNormal.normalize();
	//fprintf(stderr,"f %f \n",frontNormal*osg::Vec3(0,0,1));
	//fprintf(stderr,"b %f \n",backNormal*osg::Vec3(0,0,1) );
	if (frontNormal * osg::Vec3(0, 0, 1) < 0.2)
		return TransformMat;
	if (backNormal * osg::Vec3(0, 0, 1) < 0.2)
		return TransformMat;

	osg::Vec3d newY = front.getWorldIntersectPoint() - back.getWorldIntersectPoint();
	newY.normalize();
	osg::Vec3d newX = newY ^ frontNormal;
	newX.normalize();
	osg::Vec3d newZ = newX ^ newY;

	osg::Vec3d translation = front.getWorldIntersectPoint();

	osg::Matrix newMatrix;

	newMatrix(0, 0) = newX.x();
	newMatrix(0, 1) = newX.y();
	newMatrix(0, 2) = newX.z();

	newMatrix(1, 0) = newY.x();
	newMatrix(1, 1) = newY.y();
	newMatrix(1, 2) = newY.z();

	newMatrix(2, 0) = newZ.x();
	newMatrix(2, 1) = newZ.y();
	newMatrix(2, 2) = newZ.z();

	newMatrix = newMatrix * osg::Matrix::translate(translation);

	osg::Matrix Nn = newMatrix;
	osg::Matrix invNn;
	invNn.invert(Nn);

	osg::Matrix NewTransform = TransformMat * invNn * Flux2Pos;
	osg::Vec3d z{ NewTransform(2,0), NewTransform(2, 1), NewTransform(2, 2) };
	
	if (z * osg::Vec3(0, 0, 1) < 0.2)
		return TransformMat;

	return  NewTransform;
}


COVERPLUGIN(Flux2)