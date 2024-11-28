/* This file is part of COVISE.

   You can use it under the terms of the GNU Lesser General Public License
   version 2.1 or later, see lgpl-2.1.txt.

 * License: LGPL 2+ */

 /****************************************************************************\
  **                                                            (C)2024 HLRS  **
  **                                                                          **
  ** Description: GeoDataLoader Plugin                                        **
  **                                                                          **
  **                                                                          **
  ** Author: Uwe Woessner 		                                              **
  **                                                                          **
  ** History:  								                                  **
  **                                                                          **
  **                                                                          **
 \****************************************************************************/

#include "GeoDataLoader.h"

#include <cover/coVRPluginSupport.h>
#include <cover/RenderObject.h>
#include <cover/coVRShader.h>
#include <cover/coVRFileManager.h>
#include <cover/coVRTui.h>
#include <cover/coVRMSController.h>
#include <cover/coVRConfig.h>
#include <cover/VRViewer.h>

#include <config/CoviseConfig.h>

#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osg/Node>
#include <osg/Group>
#include <osg/Geode>
#include <osg/Geometry>
#include <osg/Texture2D>
#include <osg/StateSet>
#include <osg/Material>
#include <osg/LOD>
#include <osg/PolygonOffset>
#include <osgUtil/Optimizer>
#include <osgTerrain/Terrain>
#include <osgViewer/Renderer>
#include <iostream>
#include <string>
#include "HTTPClient/CURL/methods.h"
#include "HTTPClient/CURL/request.h"
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <rapidjson/prettywriter.h> 
#include <rapidjson/rapidjson.h>
#include "cover/coVRConfig.h"
#include <PluginUtil/PluginMessageTypes.h>

namespace opencover
{
    namespace ui
    {
        class Menu;
        class Label;
        class Group;
        class Button;
        class EditField;
    }
}


using namespace covise;
using namespace opencover;

std::string getCoordinates(const std::string& address) {
    using namespace opencover::httpclient::curl;
    std::string readBuffer;

    // // Nominatim request URL
    std::string url = "https://nominatim.openstreetmap.org/search?q=" + address + "&format=json&limit=1";

    GET getRequest(url);
    Request::Options options = {
        {CURLOPT_USERAGENT, "Mozilla/5.0"},
    };
    if (!Request().httpRequest(getRequest, readBuffer, options))
        readBuffer = "[ERROR] Failed to fetch data from Nominatim. With request: " + url;

    return readBuffer;
}

void GeoDataLoader::parseCoordinates(const std::string& jsonData) {
    rapidjson::Document document;

    if (document.Parse(jsonData.c_str()).HasParseError()) {
        std::cerr << "Failed to parse JSON data." << std::endl;
        return;
    }

    if (document.IsArray() && !document.Empty()) {
        const rapidjson::Value& location = document[0];
        if (location.HasMember("lat") && location.HasMember("lon")) {
            double latitude = std::stod(location["lat"].GetString());
            double longitude = std::stod(location["lon"].GetString());

            std::cout << "Latitude: " << latitude << std::endl;
            std::cout << "Longitude: " << longitude << std::endl;
            PJ_COORD input;
            input.lp.lam = longitude * DEG_TO_RAD; // Convert degrees to radians
            input.lp.phi = latitude * DEG_TO_RAD; // Convert degrees to radians

            // Perform the transformation
            PJ_COORD output;
            output = proj_trans(ProjInstance, PJ_FWD, input); // Forward transformation (WGS84 -> UTM)

            // Output UTM coordinates
            std::cout << "Easting (X): " << output.enu.e << " meters" << std::endl;
            std::cout << "Northing (Y): " << output.enu.n << " meters" << std::endl;
            float z = cover->getXformMat().getTrans().z();
            osg::Vec3 pos(output.enu.e, output.enu.n, z);
            osg::Matrix mat;
            pos += offset;
            mat.setTrans(pos);
            cover->setXformMat(mat);
        }
        else {
            std::cerr << "Error: Latitude and/or Longitude not found in the response." << std::endl;
        }
    }
    else {
        std::cerr << "Error: No results found." << std::endl;
    }
}




GeoDataLoader* GeoDataLoader::s_instance = nullptr;

GeoDataLoader::GeoDataLoader(): coVRPlugin(COVER_PLUGIN_NAME), ui::Owner("GeoData", cover->ui)
{
    assert(s_instance == nullptr);
    s_instance = this;

}
bool GeoDataLoader::init()
{
    ProjContext = proj_context_create();
    // Define the transformation from WGS84 to UTM Zone 32N (EPSG:32632)
    ProjInstance = proj_create_crs_to_crs(ProjContext, "EPSG:4326", "EPSG:32632", NULL); // EPSG:4326 is WGS84, EPSG:32632 is UTM Zone 32N

    geoDataMenu = new ui::Menu("GeoData", this);
    geoDataMenu->setText("GeoData");
    rootNode = new osg::MatrixTransform();
    skyRootNode = new osg::MatrixTransform();
    cover->getObjectsRoot()->addChild(rootNode);
    cover->getScene()->addChild(skyRootNode);
    //Restart Button
    skyButton = new ui::Button(geoDataMenu, "Sky");
    skyButton->setText("Sky");
    skyButton->setState(true);
    skyButton->setCallback([this](bool state) 
        {
            if (state && skyNode.get()!=nullptr &&  skyNode->getNumParents()==0)
                skyRootNode->addChild(skyNode.get());
            else if(!state && skyNode.get()!=nullptr) 
                skyRootNode->removeChild(skyNode.get());
        });
    skys = new ui::SelectionList(geoDataMenu, "Skys");
    skys->append("None");
    skyPath = configString("sky", "skyDir" ,"/data/Geodata/sky")->value();
    defaultSky = configInt("sky", "defaultSky", 6)->value();
    int skyNumber = 1;
    try {
        for (const auto& entry : fs::directory_iterator(skyPath))
        {
            if (entry.is_regular_file() && entry.path().extension() == ".wrl") {
                std::string name = entry.path().filename().string();
                if (skyNumber == defaultSky)
                {
                    skyNode = coVRFileManager::instance()->loadFile(entry.path().string().c_str(), nullptr, skyRootNode);
                }
                skyNumber++;
                skys->append(name.substr(0,name.length()-4));
            }
        }
    }
    catch (const fs::filesystem_error& err) {
        std::cerr << "Filesystem error: " << err.what() << std::endl;
    }
    catch (const std::exception& ex) {
        std::cerr << "General error: " << ex.what() << std::endl;
    }
    skys->setCallback([this](int selection)
        {
            setSky(selection);
        });
    float northAngle = 0;

    float farValue = coVRConfig::instance()->farClip();
    float scale = (farValue * 2) / 20000;
    skyRootNode->setMatrix(osg::Matrix::scale(scale, scale, scale)*osg::Matrix::rotate(northAngle,osg::Vec3(0,0,1)));
    
    location = new ui::EditField(geoDataMenu, "location");
    location->setText("location:");
    location->setCallback([this](std::string val) 
        {
            std::string coord = getCoordinates(val);
            parseCoordinates(coord);
        });

    terrainFile = configString("terrain","file", "D:/QSync/visnas/Data/Suedlink/out/vpb_DGM1m_FDOP20/vpb_DGM1m_FDOP20.ive")->value();
    loadTerrain(terrainFile,osg::Vec3d(0,0,0));
    return true;
}

// this is called if the plugin is removed at runtime
GeoDataLoader::~GeoDataLoader()
{
    s_instance = nullptr;
    proj_destroy(ProjInstance);
    proj_context_destroy(ProjContext);
}

void GeoDataLoader::message(int toWhom, int type, int length, const void* data)
{
    const char* messageData = (const char*)data;
    if(type == PluginMessageTypes::LoadTerrain)
        loadTerrain(messageData+20, osg::Vec3d(0, 0, 0)); // 20= opencover://terrain/
    else if (type == PluginMessageTypes::setSky)
    {
        int offset = 0;
        if(messageData[0]=='/' || messageData[0] == '\\')
            offset = 1;
        int num = atoi(messageData + offset);
        setSky(num);
    }
}
void GeoDataLoader::setSky(int selection)
{
    if (skyNode.get() != nullptr)
        skyRootNode->removeChild(skyNode.get());
    if (selection == 0)
    {
        skyNode = nullptr;
    }
    else
    {
        if (skyNode.get() != nullptr)
            skyRootNode->removeChild(skyNode.get());
        skyNode = coVRFileManager::instance()->loadFile((skyPath + "/" + skys->items()[selection] + ".wrl").c_str(), nullptr, skyRootNode);
        cover->getScene()->addChild(skyRootNode);

    }
}

bool GeoDataLoader::update()
{
    const osg::Matrix &m = cover->getObjectsXform()->getMatrix();
    //double x = m(0, 0);
    //double y = m(1, 0);
    //northAngle = -atan2(y, x);
    //skyRootNode->setMatrix(osg::Matrix::scale(1000, 1000, 1000) * osg::Matrix::rotate(northAngle, osg::Vec3(0, 0, 1)));
    float farValue = coVRConfig::instance()->farClip();
    float scale = ((farValue) /20000)+500; // scale the sphere so that it stays a bit closer than the far clipping plane.
    skyRootNode->setMatrix(osg::Matrix::scale(scale, scale, scale) * osg::Matrix::rotate(cover->getObjectsXform()->getMatrix().getRotate()));
    return false; // don't request that scene be re-rendered
}
GeoDataLoader *GeoDataLoader::instance()
{
    if (!s_instance)
        s_instance = new GeoDataLoader;
    return s_instance;
}


bool GeoDataLoader::loadTerrain(std::string filename, osg::Vec3d offset)
{
    VRViewer *viewer = VRViewer::instance();
    osgDB::DatabasePager *pager = viewer->getDatabasePager();
    std::cout << "DatabasePager:" << std::endl;
    std::cout << "\t num threads: " << pager->getNumDatabaseThreads() << std::endl;
    std::cout << "\t getDatabasePagerThreadPause: " << (pager->getDatabasePagerThreadPause() ? "yes" : "no") << std::endl;
    std::cout << "\t getDoPreCompile: " << (pager->getDoPreCompile() ? "yes" : "no") << std::endl;
    std::cout << "\t getApplyPBOToImages: " << (pager->getApplyPBOToImages() ? "yes" : "no") << std::endl;
    std::cout << "\t getTargetMaximumNumberOfPageLOD: " << pager->getTargetMaximumNumberOfPageLOD() << std::endl;
    viewer->setIncrementalCompileOperation(new osgUtil::IncrementalCompileOperation());
    //pager->setIncrementalCompileOperation(new osgUtil::IncrementalCompileOperation());
    osgUtil::IncrementalCompileOperation *coop = pager->getIncrementalCompileOperation();
    if (coop)
    {
        for (int screenIt = 0; screenIt < coVRConfig::instance()->numScreens(); ++screenIt)
        {
            osgViewer::Renderer *renderer = dynamic_cast<osgViewer::Renderer *>(coVRConfig::instance()->channels[screenIt].camera->getRenderer());
            if (renderer)
            {
                renderer->getSceneView(0)->setAutomaticFlush(false);
                renderer->getSceneView(1)->setAutomaticFlush(false);
            }
        }

        coop->setTargetFrameRate(60);
        coop->setMinimumTimeAvailableForGLCompileAndDeletePerFrame(0.001);
        coop->setMaximumNumOfObjectsToCompilePerFrame(2);
        coop->setConservativeTimeRatio(0.1);
        coop->setFlushTimeRatio(0.1);

        std::cout << "IncrementedCompileOperation:" << std::endl;
        std::cout << "\t is active: " << (coop->isActive() ? "yes" : "no") << std::endl;
        std::cout << "\t target frame rate: " << coop->getTargetFrameRate() << std::endl;
        std::cout << "\t getMinimumTimeAvailableForGLCompileAndDeletePerFrame: " << coop->getMinimumTimeAvailableForGLCompileAndDeletePerFrame() << std::endl;
        std::cout << "\t getMaximumNumOfObjectsToCompilePerFrame: " << coop->getMaximumNumOfObjectsToCompilePerFrame() << std::endl;
        std::cout << "\t flush time ratio: " << coop->getFlushTimeRatio() << std::endl;
        std::cout << "\t conservative time ratio: " << coop->getConservativeTimeRatio() << std::endl;
    }

    osg::Node *terrain = osgDB::readNodeFile(filename);

    if (terrain)
    {
        /*osgTerrain::Terrain* terrainNode = dynamic_cast<osgTerrain::Terrain*>(terrain);
      if(terrainNode) {
         std::cout << "Casted to terrainNode!" << std::endl;
         std::cout << "Num children: " << terrainNode->getNumChildren() << std::endl;
         if(terrainNode->getNumChildren()>0) {
            terrain = terrainNode->getChild(0);
         }
      }*/
        terrain->setDataVariance(osg::Object::DYNAMIC);

        osg::StateSet *terrainStateSet = terrain->getOrCreateStateSet();

        terrainStateSet->setMode(GL_LIGHTING, osg::StateAttribute::ON);
        terrainStateSet->setMode(GL_LIGHT0, osg::StateAttribute::ON);
        //terrainStateSet->setMode ( GL_LIGHT1, osg::StateAttribute::ON);

        /*osg::StateSet* terrainStateSet = terrain->getOrCreateStateSet();
      osg::PolygonOffset* offset = new osg::PolygonOffset(1.0, 1.0);
      osg::PolygonOffset* offset = new osg::PolygonOffset(0.0, 0.0);
      terrainStateSet->setAttributeAndModes(offset, osg::StateAttribute::OVERRIDE|osg::StateAttribute::ON
      */
        //osgUtil::Optimizer optimizer;
        //optimizer.optimize(terrain);

        terrain->setNodeMask(terrain->getNodeMask() & (~Isect::Intersection) & (~Isect::Pick));

        //std::vector<BoundingArea> voidBoundingAreaVector;
        //voidBoundingAreaVector.push_back(BoundingArea(osg::Vec2(506426.839,5398055.357),osg::Vec2(508461.865,5399852.0)));
        osgTerrain::TerrainTile::setTileLoadedCallback(new CutGeometry(this));

        osg::PositionAttitudeTransform *terrainTransform = new osg::PositionAttitudeTransform();
        terrainTransform->setName("Terrain");
        terrainTransform->addChild(terrain);
        terrainTransform->setPosition(-offset);

        rootNode->addChild(terrainTransform);

        const osg::BoundingSphere &terrainBS = terrain->getBound();
        std::cout << "Terrain BB: center: (" << terrainBS.center()[0] << ", " << terrainBS.center()[1] << ", " << terrainBS.center()[2] << "), radius: " << terrainBS.radius() << std::endl;

        return true;
    }
    else
    {
        return false;
    }
}

bool GeoDataLoader::addLayer(std::string filename)
{
#if GDAL_VERSION_MAJOR<2

#else
        if (GDALGetDriverCount() == 0)
            GDALAllRegister();

        // Try to open data source
        GDALDataset* poDS  = (GDALDataset*) GDALOpenEx( filename.c_str(), GDAL_OF_VECTOR, NULL, NULL, NULL );
#endif
        /*
    if (poDS == NULL)
    {
        std::cout << "GeoDataLoader: Could not open layer " << filename << "!" << std::endl;
        return false;
    }

    for (int layerIt = 0; layerIt < poDS->GetLayerCount(); ++layerIt)
    {
    }*/

    return true;
}

COVERPLUGIN(GeoDataLoader)
