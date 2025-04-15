/* This file is part of COVISE.

   You can use it under the terms of the GNU Lesser General Public License
   version 2.1 or later, see lgpl-2.1.txt.

 * License: LGPL 2+ */

#ifndef QT_CLEAN_NAMESPACE
#define QT_CLEAN_NAMESPACE
#endif

#include <util/common.h>

#include "vvTui.h"
#include "vvSceneGraph.h"
#include "vvFileManager.h"
#include "vvNavigationManager.h"
#include "vvCollaboration.h"
#include "vvViewer.h"
#include "vvPluginSupport.h"
#include "vvConfig.h"
#include "vvPluginList.h"
#include "vvCommunication.h"
#include "vvMSController.h"
#include "vvIntersection.h"
#include "vvMarkerTracking.h"
#include "vvVIVE.h"
#include <config/CoviseConfig.h>
#include <util/coFileUtil.h>
#include <util/environment.h>
#include <grmsg/coGRKeyWordMsg.h>
#include <vsg/nodes/MatrixTransform.h>
#include <vsg/io/write.h>
#include "vvVIVE.h"
#include "vvHud.h"
#include <input/input.h>
#include <input/inputdevice.h>
#include <OpenVRUI/vsg/mathUtils.h> //for MAKE_EULER_MAT

//#define PRESENTATION

using namespace vive;
using namespace grmsg;
using namespace covise;

VVCORE_EXPORT vvTui *vvTui::vrtui = NULL;
vvTui *vvTui::instance()
{
    assert(vrtui);
    return vrtui;
}

vvTui::vvTui(vvTabletUI *tui)
: tui(tui), collision(false), navigationMode(vvNavigationManager::NavNone), driveSpeed(0.0), ScaleValue(1.0)
{
    if (!tui)
    {
        tui = vvTabletUI::instance();
        assert(!vrtui);
        vrtui = this;
    }

    lastUpdateTime = 0;
    //binList = new BinList(tui);
    mainFolder = new vvTUITabFolder(tui, "VIVEMainFolder");
    viveTab = new vvTUITab(tui, "VIVE", mainFolder->getID());
#ifdef PRESENTATION
    presentationTab = new vvTUITab(tui, "Presentation", mainFolder->getID());
#endif
    inputTUI = new coInputTUI(tui);
    topContainer = new vvTUIFrame(tui, "Buttons", viveTab->getID());
    bottomContainer = new vvTUIFrame(tui, "Nav", viveTab->getID());
    rightContainer = new vvTUIFrame(tui, "misc", bottomContainer->getID());

    Walk = new vvTUIToggleButton(tui, "Walk", topContainer->getID());
    Drive = new vvTUIToggleButton(tui, "Drive", topContainer->getID());
    Fly = new vvTUIToggleButton(tui, "Fly", topContainer->getID());
    XForm = new vvTUIToggleButton(tui, "Move world", topContainer->getID());
    DebugBins = new vvTUIToggleButton(tui, "DebugBins", topContainer->getID(), false);
    DebugBins->setEventListener(this);

    FlipStereo = new vvTUIToggleButton(tui, "Flip eyes", topContainer->getID(), false);
    FlipStereo->setEventListener(this);

    Scale = new vvTUIToggleButton(tui, "Scale", topContainer->getID());
    Collision = new vvTUIToggleButton(tui, "Detect collisions", topContainer->getID());
    DisableIntersection = new vvTUIToggleButton(tui, "Disable intersection", topContainer->getID());
    testImage = new vvTUIToggleButton(tui, "TestImage", topContainer->getID());
    testImage->setState(false);
    Quit = new vvTUIButton(tui, "Quit", topContainer->getID());
    Freeze = new vvTUIToggleButton(tui, "Stop headtracking", topContainer->getID());
    Wireframe = new vvTUIToggleButton(tui, "Wireframe", topContainer->getID());
    ViewAll = new vvTUIButton(tui, "View all", topContainer->getID());
    Menu = new vvTUIToggleButton(tui, "Hide menu", topContainer->getID());
    scaleLabel = new vvTUILabel(tui, "Scale factor", topContainer->getID());
    ScaleSlider = new vvTUIFloatSlider(tui, "ScaleFactor", topContainer->getID());
    SceneUnit = new vvTUIComboBox(tui, "SceneUni", topContainer->getID());
    Menu->setState(false);
    debugLabel = new vvTUILabel(tui, "Debug level", topContainer->getID());
    debugLevel = new vvTUIEditIntField(tui, "DebugLevel", topContainer->getID());
#ifndef NOFB
    FileBrowser = new vvTUIFileBrowserButton(tui, "Load file...", topContainer->getID());
    vvCommunication::instance()->setFBData(FileBrowser->getVRBData());
    SaveFileFB = new vvTUIFileBrowserButton(tui, "Save file...", topContainer->getID());
    SaveFileFB->setMode(vvTUIFileBrowserButton::SAVE);
    vvCommunication::instance()->setFBData(SaveFileFB->getVRBData());
    vvFileManager::instance()->SetDefaultFB(FileBrowser);

#endif

    speedLabel = new vvTUILabel(tui, "Navigation speed", bottomContainer->getID());
    NavSpeed = new vvTUIFloatSlider(tui, "NavSpeed", bottomContainer->getID());
    viewerLabel = new vvTUILabel(tui, "Viewer position", bottomContainer->getID());
    posX = new vvTUIEditFloatField(tui, "ViewerX", bottomContainer->getID());
    posY = new vvTUIEditFloatField(tui, "ViewerY", bottomContainer->getID());
    posZ = new vvTUIEditFloatField(tui, "ViewerZ", bottomContainer->getID());
    fovLabel = new vvTUILabel(tui, "FieldOfView in ", bottomContainer->getID());
    fovH = new vvTUIEditFloatField(tui, "ViewerZ", bottomContainer->getID());
    stereoSepLabel = new vvTUILabel(tui, "EyeDistance", bottomContainer->getID());
    stereoSep = new vvTUIEditFloatField(tui, "ViewerZ", bottomContainer->getID());
    FPSLabel = new vvTUILabel(tui, "Fps:", bottomContainer->getID());
    CFPSLabel = new vvTUILabel(tui, "Constant fps", bottomContainer->getID());
    CFPS = new vvTUIEditFloatField(tui, "cfps", bottomContainer->getID());
    backgroundLabel = new vvTUILabel(tui, "Background color", bottomContainer->getID());
    backgroundColor = new vvTUIColorButton(tui, "background", bottomContainer->getID());
    LODScaleLabel = new vvTUILabel(tui, "LODScale", bottomContainer->getID());
    LODScaleEdit = new vvTUIEditFloatField(tui, "LODScaleE", bottomContainer->getID());

    driveLabel = new vvTUILabel(tui, "Drive navigation", bottomContainer->getID());
    driveNav = new vvTUINav(tui, "driveNav", bottomContainer->getID());
    panLabel = new vvTUILabel(tui, "Pan navigation", bottomContainer->getID());
    panNav = new vvTUINav(tui, "panNav", bottomContainer->getID());

    nearEdit = new vvTUIEditFloatField(tui, "near", rightContainer->getID());
    farEdit = new vvTUIEditFloatField(tui, "far", rightContainer->getID());
    nearLabel = new vvTUILabel(tui, "near", rightContainer->getID());
    farLabel = new vvTUILabel(tui, "far", rightContainer->getID());

    NavSpeed->setEventListener(this);
    Walk->setEventListener(this);
    Drive->setEventListener(this);
    Fly->setEventListener(this);
    XForm->setEventListener(this);
    Scale->setEventListener(this);
    Collision->setEventListener(this);
    DisableIntersection->setEventListener(this);
    testImage->setEventListener(this);
    ScaleSlider->setEventListener(this);
    SceneUnit->setEventListener(this);
    Quit->setEventListener(this);
    Freeze->setEventListener(this);
    Wireframe->setEventListener(this);
    Menu->setEventListener(this);
    posX->setEventListener(this);
    posY->setEventListener(this);
    posZ->setEventListener(this);
    fovH->setEventListener(this);
    stereoSep->setEventListener(this);
    driveNav->setEventListener(this);
    panNav->setEventListener(this);
    ViewAll->setEventListener(this);
    CFPS->setEventListener(this);
    backgroundColor->setEventListener(this);
    LODScaleEdit->setEventListener(this);
    debugLevel->setEventListener(this);

    nearEdit->setEventListener(this);
    farEdit->setEventListener(this);

#ifndef NOFB
    FileBrowser->setEventListener(this);
    FileBrowser->setMode(vvTUIFileBrowserButton::OPEN);
    SaveFileFB->setEventListener(this);
    SaveFileFB->setMode(vvTUIFileBrowserButton::OPEN);
#endif

    ScaleSlider->setMin(1e-5f);
    ScaleSlider->setMax(1e+5f);
    ScaleSlider->setValue(1.0f);
    ScaleSlider->setLogarithmic(true);

    for(auto unit : LengthUnitNames)
        SceneUnit->addEntry(unit);
    SceneUnit->setSelectedEntry((int)vv->getSceneUnit());

    NavSpeed->setMin(0.03f);
    NavSpeed->setMax(30);
    NavSpeed->setValue(1);

    topContainer->setPos(0, 0);
    bottomContainer->setPos(0, 1);
    rightContainer->setPos(2, 5);

    Walk->setPos(0, 0);
    Drive->setPos(0, 1);
    Fly->setPos(0, 2);
    XForm->setPos(0, 3);
    Scale->setPos(0, 4);
    Collision->setPos(0, 5);
    DebugBins->setPos(3, 0);
    FlipStereo->setPos(3,1);
    DisableIntersection->setPos(1, 5);
    testImage->setPos(2, 5);

    debugLabel->setPos(3,2);
    debugLevel->setPos(3,3);

    speedLabel->setPos(0, 10);
    NavSpeed->setPos(1, 10);

    viewerLabel->setPos(0, 7);
    posX->setPos(0, 8);
    posY->setPos(1, 8);
    posZ->setPos(2, 8);
    fovLabel->setPos(3, 7);
    fovH->setPos(3, 8);
    stereoSepLabel->setPos(3, 9);
    stereoSep->setPos(3, 10);

    scaleLabel->setPos(1, 3);
    ScaleSlider->setPos(1, 4);
    SceneUnit->setPos(3, 4);

    FPSLabel->setPos(0, 11);
    CFPSLabel->setPos(1, 11);
    CFPS->setPos(2, 11);
    CFPS->setValue(0.0);

    backgroundLabel->setPos(0, 12);
    backgroundColor->setPos(1, 12);
    backgroundColor->setColor(0, 0, 0, 1);
    LODScaleLabel->setPos(2, 12);
    LODScaleEdit->setPos(3, 12);

    Quit->setPos(2, 0);
#ifndef NOFB
    FileBrowser->setPos(2, 1);
    SaveFileFB->setPos(2, 2);
#endif
    Menu->setPos(2, 3);

    ViewAll->setPos(1, 0);
    Freeze->setPos(1, 1);
    Wireframe->setPos(1, 2);

    driveNav->setPos(0, 5);
    driveLabel->setPos(0, 6);
    panNav->setPos(1, 5);
    panLabel->setPos(1, 6);

    nearEdit->setPos(0, 1);
    farEdit->setPos(0, 3);
    nearLabel->setPos(0, 0);
    farLabel->setPos(0, 2);

    //TODO coConfig
    float xp, yp, zp;
    xp = coCoviseConfig::getFloat("x", "VIVE.ViewerPosition", 0.0f);
    yp = coCoviseConfig::getFloat("y", "VIVE.ViewerPosition", -2000.0f);
    zp = coCoviseConfig::getFloat("z", "VIVE.ViewerPosition", 30.0f);
    viewPos.set(xp, yp, zp);
    posX->setValue(viewPos[0]);
    posY->setValue(viewPos[1]);
    posZ->setValue(viewPos[2]);

#ifdef PRESENTATION
    // Animation tab
    PresentationLabel = new vvTUILabel(tui, "Presentation", presentationTab->getID());
    PresentationForward = new vvTUIButton(tui, "Forward", presentationTab->getID());
    PresentationBack = new vvTUIButton(tui, "Back", presentationTab->getID());
    PresentationStep = new vvTUIEditIntField(tui, "PresStep", presentationTab->getID());
    PresentationForward->setEventListener(this);
    PresentationBack->setEventListener(this);
    PresentationStep->setEventListener(this);
    PresentationStep->setValue(0);
    PresentationStep->setMin(0);
    PresentationStep->setMax(1000);

    PresentationLabel->setPos(0, 10);
    PresentationBack->setPos(0, 11);
    PresentationStep->setPos(1, 11);
    PresentationForward->setPos(2, 11);
#endif

    nearEdit->setValue(vvConfig::instance()->nearClip());
    farEdit->setValue(vvConfig::instance()->farClip());
    LODScaleEdit->setValue(vvConfig::instance()->getLODScale());
}
void vvTui::config()
{
#ifndef NOFB
    FileBrowser->setFilterList(vvFileManager::instance()->getFilterList());
    SaveFileFB->setFilterList(vvFileManager::instance()->getWriteFilterList());
#endif
}

vvTui::~vvTui()
{
    delete PresentationLabel;
    delete PresentationForward;
    delete PresentationBack;
    delete PresentationStep;
    delete Walk;
    delete Drive;
    delete Fly;
    delete XForm;
    delete Scale;
    delete Collision;
    delete DebugBins;
    delete FlipStereo;
    delete DisableIntersection;
    delete testImage;
    delete speedLabel;
    delete NavSpeed;
    delete ScaleSlider;
    delete viveTab;
    delete mainFolder;
    delete Quit;
    delete Freeze;
    delete Wireframe;
    delete Menu;
    delete viewerLabel;
    delete posX;
    delete posY;
    delete posZ;
    delete fovLabel;
    delete fovH;
    delete stereoSepLabel;
    delete stereoSep;
    delete FPSLabel;
    delete CFPSLabel;
    delete CFPS;
    delete backgroundLabel;
    delete backgroundColor;
    delete LODScaleLabel;
    delete LODScaleEdit;
#ifndef NOFB
    delete FileBrowser;
    delete SaveFileFB;
#endif
    delete driveLabel;
    delete panLabel;
    delete nearLabel;
    delete farLabel;
    delete nearEdit;
    delete farEdit;
    delete topContainer;
    delete bottomContainer;
    delete rightContainer;
    //delete binList;
    delete inputTUI;

    if (tui == vvTabletUI::instance())
    {
        vrtui = NULL;
    }
}

coInputTUI::coInputTUI(vvTabletUI *tui): tui(tui)
{
    inputTab = new vvTUITab(tui, "Input", vvTui::instance()->mainFolder->getID());

    personContainer = new vvTUIFrame(tui, "pc", inputTab->getID());
    personContainer->setPos(0,0);
    personsLabel = new vvTUILabel(tui, "Person", personContainer->getID());
    personsLabel->setPos(0,0);
    personsChoice = new vvTUIComboBox(tui, "personsCombo", personContainer->getID());
    personsChoice->setPos(0,1);
    personsChoice->setEventListener(this);

    eyeDistanceLabel = new vvTUILabel(tui, "Eye distance", personContainer->getID());
    eyeDistanceLabel->setPos(0,3);
    eyeDistanceEdit = new vvTUIEditFloatField(tui, "EyeDistance", personContainer->getID());
    eyeDistanceEdit->setPos(0,4);
    eyeDistanceEdit->setEventListener(this);

    bodiesContainer = new vvTUIFrame(tui, "bc", inputTab->getID());
    bodiesContainer->setPos(1,0);
    bodiesLabel = new vvTUILabel(tui, "Body", bodiesContainer->getID());
    bodiesLabel->setPos(0,0);
    bodiesChoice = new vvTUIComboBox(tui, "bodiesCombo", bodiesContainer->getID());
    bodiesChoice->setPos(0,1);
    bodiesChoice->setEventListener(this);

    bodyTrans[0] = new vvTUIEditFloatField(tui, "xe", bodiesContainer->getID());
    bodyTransLabel[0] = new vvTUILabel(tui, "x", bodiesContainer->getID());
    bodyTrans[1] = new vvTUIEditFloatField(tui, "ye", bodiesContainer->getID());
    bodyTransLabel[1] = new vvTUILabel(tui, "y", bodiesContainer->getID());
    bodyTrans[2] = new vvTUIEditFloatField(tui, "ze", bodiesContainer->getID());
    bodyTransLabel[2] = new vvTUILabel(tui, "z", bodiesContainer->getID());
    bodyRot[0] = new vvTUIEditFloatField(tui, "he", bodiesContainer->getID());
    bodyRotLabel[0] = new vvTUILabel(tui, "h", bodiesContainer->getID());
    bodyRot[1] = new vvTUIEditFloatField(tui, "pe", bodiesContainer->getID());
    bodyRotLabel[1] = new vvTUILabel(tui, "p", bodiesContainer->getID());
    bodyRot[2] = new vvTUIEditFloatField(tui, "re", bodiesContainer->getID());
    bodyRotLabel[2] = new vvTUILabel(tui, "r", bodiesContainer->getID());
    for (int i = 0; i < 3; i++)
    {
        bodyTrans[i]->setPos(1+i * 2 + 1, 1);
        bodyTransLabel[i]->setPos(1+i * 2, 1);
        bodyTrans[i]->setEventListener(this);
        bodyRot[i]->setPos(1+i * 2 + 1, 2);
        bodyRotLabel[i]->setPos(1+i * 2, 2);
        bodyRot[i]->setEventListener(this);
    }

    devicesLabel = new vvTUILabel(tui, "Device", bodiesContainer->getID());
    devicesLabel->setPos(0,4);
    devicesChoice = new vvTUIComboBox(tui, "devicesCombo", bodiesContainer->getID());
    devicesChoice->setPos(0,5);
    devicesChoice->setEventListener(this);
    for (size_t i = 0; i < Input::instance()->getNumDevices(); i++)
    {
        devicesChoice->addEntry(Input::instance()->getDevice(i)->getName());
    }
    devicesChoice->setSelectedEntry((int)Input::instance()->getActivePerson());

    deviceTrans[0] = new vvTUIEditFloatField(tui, "xe", bodiesContainer->getID());
    deviceTransLabel[0] = new vvTUILabel(tui, "x", bodiesContainer->getID());
    deviceTrans[1] = new vvTUIEditFloatField(tui, "ye", bodiesContainer->getID());
    deviceTransLabel[1] = new vvTUILabel(tui, "y", bodiesContainer->getID());
    deviceTrans[2] = new vvTUIEditFloatField(tui, "ze", bodiesContainer->getID());
    deviceTransLabel[2] = new vvTUILabel(tui, "z", bodiesContainer->getID());
    deviceRot[0] = new vvTUIEditFloatField(tui, "he", bodiesContainer->getID());
    deviceRotLabel[0] = new vvTUILabel(tui, "h", bodiesContainer->getID());
    deviceRot[1] = new vvTUIEditFloatField(tui, "pe", bodiesContainer->getID());
    deviceRotLabel[1] = new vvTUILabel(tui, "p", bodiesContainer->getID());
    deviceRot[2] = new vvTUIEditFloatField(tui, "re", bodiesContainer->getID());
    deviceRotLabel[2] = new vvTUILabel(tui, "r", bodiesContainer->getID());
    for (int i = 0; i < 3; i++)
    {
        deviceTrans[i]->setPos(1+i * 2 + 1, 5);
        deviceTransLabel[i]->setPos(1+i * 2, 5);
        deviceTrans[i]->setEventListener(this);
        deviceRot[i]->setPos(1+i * 2 + 1, 6);
        deviceRotLabel[i]->setPos(1+i * 2, 6);
        deviceRot[i]->setEventListener(this);
    }

    calibrateTrackingsystem = new vvTUIToggleButton(tui, "Calibrate Device", bodiesContainer->getID());
    calibrateTrackingsystem->setPos(0, 7);
    calibrateTrackingsystem->setEventListener(this);

    calibrateToHand = new vvTUIToggleButton(tui, "CalibrateToHand", bodiesContainer->getID());
    calibrateToHand->setPos(2, 7);
	calibrateToHand->setEventListener(this);

    calibrationLabel = new vvTUILabel(tui, "Select device and press Calibrate Device button", inputTab->getID());
    calibrationLabel->setPos(1, 6);

    debugContainer = new vvTUIFrame(tui, "Debug", inputTab->getID());
    debugContainer->setPos(1,7);
    debugLabel = new vvTUILabel(tui, "Debug", debugContainer->getID());
    debugLabel->setPos(0,0);

    debugMatrices = new vvTUIToggleButton(tui, "Matrices", debugContainer->getID());
    debugMatrices->setPos(2,1);
    debugMatrices->setEventListener(this);
    debugOther = new vvTUIToggleButton(tui, "Buttons+Valuators", debugContainer->getID());
    debugOther->setPos(3,1);
    debugOther->setEventListener(this);

    debugMouseButton = new vvTUIToggleButton(tui, "Mouse", debugContainer->getID());
    debugMouseButton->setPos(0,2);
    debugMouseButton->setEventListener(this);
    debugDriverButton = new vvTUIToggleButton(tui, "Driver", debugContainer->getID());
    debugDriverButton->setPos(1,2);
    debugDriverButton->setEventListener(this);
    debugRawButton = new vvTUIToggleButton(tui, "Raw", debugContainer->getID());
    debugRawButton->setPos(2,2);
    debugRawButton->setEventListener(this);
    debugTransformedButton = new vvTUIToggleButton(tui, "Transformed", debugContainer->getID());
    debugTransformedButton->setPos(3,2);
    debugTransformedButton->setEventListener(this);

	calibrationStep = -2;

    updateTUI();
}

void coInputTUI::updateTUI()
{
    if (size_t(personsChoice->getNumEntries()) != Input::instance()->getNumPersons())
    {
        personsChoice->clear();
        for (size_t i = 0; i < Input::instance()->getNumPersons(); i++)
        {
            personsChoice->addEntry(Input::instance()->getPerson(i)->name());
        }
    }
    int activePerson = (int)Input::instance()->getActivePerson();
    if(activePerson != personsChoice->getSelectedEntry())
        personsChoice->setSelectedEntry(activePerson);

    if (eyeDistanceEdit->getValue() != Input::instance()->eyeDistance())
    {
        eyeDistanceEdit->setValue(Input::instance()->eyeDistance());
    }

    if (size_t(bodiesChoice->getNumEntries()) != Input::instance()->getNumBodies())
    {
        const std::string body = bodiesChoice->getSelectedText();
        bodiesChoice->clear();
        for (size_t i = 0; i < Input::instance()->getNumBodies(); i++)
        {
            bodiesChoice->addEntry(Input::instance()->getBody(i)->name());
        }
        bodiesChoice->setSelectedEntry(0);
        if(body !="")
        bodiesChoice->setSelectedText(body);
    }

    TrackingBody * tb = Input::instance()->getBody(bodiesChoice->getSelectedEntry());
    if(tb)
    {
        vsg::dmat4 m = tb->getOffsetMat();
        vsg::dvec3 v = getTrans(m);
        coCoord coord = m;
        for (int i = 0; i < 3; i++)
        {
            bodyTrans[i]->setValue(v[i]);
            bodyRot[i]->setValue(coord.hpr[i]);
        }
    }
    InputDevice *id = Input::instance()->getDevice(devicesChoice->getSelectedEntry());
    if(id)
    {
        vsg::dmat4 m = id->getOffsetMat();
        vsg::dvec3 v = getTrans(m);
        coCoord coord = m;
        for (int i = 0; i < 3; i++)
        {
            deviceTrans[i]->setValue(v[i]);
            deviceRot[i]->setValue(coord.hpr[i]);
        }
    }

    if (debugMouseButton->getState() != Input::debug(Input::Mouse))
        debugMouseButton->setState(Input::debug(Input::Mouse));
    if (debugDriverButton->getState() != Input::debug(Input::Driver))
        debugDriverButton->setState(Input::debug(Input::Driver));
    if (debugRawButton->getState() != Input::debug(Input::Raw))
        debugRawButton->setState(Input::debug(Input::Raw));
    if (debugTransformedButton->getState() != Input::debug(Input::Transformed))
        debugTransformedButton->setState(Input::debug(Input::Transformed));
    if (debugMatrices->getState() != Input::debug(Input::Matrices))
        debugMatrices->setState(Input::debug(Input::Matrices));
    if (debugOther->getState() != Input::debug(Input::Buttons)||Input::debug(Input::Valuators))
        debugOther->setState(Input::debug(Input::Buttons)||Input::debug(Input::Valuators));
}

coInputTUI::~coInputTUI()
{
    for (int i = 0; i < 3; i++)
    {
        delete bodyTrans[i];
        delete bodyTransLabel[i];
        delete bodyRot[i];
        delete bodyRotLabel[i];
        delete deviceTrans[i];
        delete deviceTransLabel[i];
        delete deviceRot[i];
        delete deviceRotLabel[i];
    }

	delete calibrateTrackingsystem;
	delete calibrationLabel;
	delete calibrateToHand;
    
    delete personContainer;
    delete personsLabel;
    delete personsChoice;

    delete bodiesContainer;
    delete bodiesLabel;
    delete bodiesChoice;
    delete devicesLabel;
    delete devicesChoice;
    delete inputTab;
    
}

void coInputTUI::update()
{
	if (calibrationStep >= -1 && calibrationStep < 3)
	{
		if (vv->getPointerButton()->wasReleased() || calibrationStep  == -1)
		{
			InputDevice *id = Input::instance()->getDevice(devicesChoice->getSelectedEntry());
			calibrationStep++;
			char buf[300];
			sprintf(buf, "step %d, please go to position %s and press any button.", calibrationStep,id->getCalibrationPointName(calibrationStep).c_str());
			calibrationLabel->setLabel(buf);
		}
		if (vv->getPointerButton()->wasPressed())
		{
			calibrationPositions[calibrationStep] = getTrans(vv->getPointerMat());
		}
		if (calibrationStep == 3) // calibration done, we have all three points
		{
			InputDevice *id = Input::instance()->getDevice(devicesChoice->getSelectedEntry());
			vsg::vec3 axisA = id->getCalibrationPoint(1) - id->getCalibrationPoint(0);
			vsg::vec3 axisB = id->getCalibrationPoint(2) - id->getCalibrationPoint(0);
			vsg::vec3 axisAm = calibrationPositions[1] - calibrationPositions[0];
			vsg::vec3 axisBm = calibrationPositions[2] - calibrationPositions[0];
			vsg::dmat4 m;
			m= vsg::translate(id->getCalibrationPoint(0) - calibrationPositions[0]);

            normalize(axisA);
            normalize(axisAm);
            normalize(axisB);
            normalize(axisBm);
			vsg::vec3 axisC = cross(axisA,axisB);
			vsg::vec3 axisCm = cross(axisAm,axisBm);
            normalize(axisC);
            normalize(axisCm);
			axisB = cross(axisA,axisC);
			axisBm = cross(axisAm,axisCm);
            normalize(axisB);
            normalize(axisBm);

			vsg::dmat4 calibCS;
			vsg::dmat4 measuredCS;
			vsg::dmat4 measuredCSI;
			calibCS(0, 0) = axisA[0]; calibCS(0, 1) = axisA[1]; calibCS(0, 2) = axisA[2];
			calibCS(1, 0) = axisB[0]; calibCS(1, 1) = axisB[1]; calibCS(1, 2) = axisB[2];
			calibCS(2, 0) = axisC[0]; calibCS(2, 1) = axisC[1]; calibCS(2, 2) = axisC[2];
			calibCS(3, 0) = id->getCalibrationPoint(0)[0]; calibCS(3, 1) = id->getCalibrationPoint(0)[1]; calibCS(3, 2) = id->getCalibrationPoint(0)[2];
			measuredCS(0, 0) = axisAm[0]; measuredCS(0, 1) = axisAm[1]; measuredCS(0, 2) = axisAm[2];
			measuredCS(1, 0) = axisBm[0]; measuredCS(1, 1) = axisBm[1]; measuredCS(1, 2) = axisBm[2];
			measuredCS(2, 0) = axisCm[0]; measuredCS(2, 1) = axisCm[1]; measuredCS(2, 2) = axisCm[2];
			measuredCS(3, 0) = calibrationPositions[0][0]; measuredCS(3, 1) = calibrationPositions[0][1]; measuredCS(3, 2) = calibrationPositions[0][2];
			measuredCSI = inverse(measuredCS);
			m = measuredCSI * calibCS;
			id->setOffsetMat(m);
			updateTUI();

			calibrationLabel->setLabel("Calibration done. Select device and press Calibrate Device button");
			calibrationStep = -2;
		}
	}
	if (calibrateToHand->getState())
	{
		InputDevice *id = Input::instance()->getDevice(devicesChoice->getSelectedEntry());
		vsg::dmat4 m;
		id->setOffsetMat(m);
		if (vv->getPointerButton()->wasPressed())
		{
			m = inverse(vv->getPointerMat());
			//m = vv->getPointerMat();
			id->setOffsetMat(m);
			updateTUI();
			calibrateToHand->setState(false);
		}
	}
}
void coInputTUI::tabletEvent(vvTUIElement *tUIItem)
{
    if (tUIItem == eyeDistanceEdit)
    {
        const int activePerson = (int)Input::instance()->getActivePerson();
        Input::instance()->getPerson(activePerson)->setEyeDistance(eyeDistanceEdit->getValue());
        vvViewer::instance()->setSeparation(Input::instance()->eyeDistance());
    }
	else if (tUIItem == calibrateTrackingsystem)
	{
		if (calibrateTrackingsystem->getState() == true)
		{
			calibrationStep = -1;

			InputDevice *id = Input::instance()->getDevice(devicesChoice->getSelectedEntry());
			for (int i = 0; i < 3; i++)
			{
				deviceTrans[i]->setValue(0);
				deviceRot[i]->setValue(0);
			}
            vsg::dmat4 identity;
			id->setOffsetMat(identity);

			calibrationLabel->setLabel("Select device and press Calibrate Device button");
		}
		else
		{
			calibrationStep = -2;
			calibrationLabel->setLabel("Select device and press Calibrate Device button");
		}
		delete calibrateTrackingsystem;
		delete calibrationLabel;
	}
    else if(tUIItem == personsChoice)
    {
        Input::instance()->setActivePerson(personsChoice->getSelectedEntry());
    }
    else if(tUIItem == bodiesChoice)
    {
        updateTUI();
    }
    else if(tUIItem == devicesChoice)
    {
        updateTUI();
    }
    else if(tUIItem == bodyTrans[0] || tUIItem == bodyTrans[1] || tUIItem == bodyTrans[2] ||
            tUIItem == bodyRot[0] || tUIItem == bodyRot[1] || tUIItem == bodyRot[2])
    {
        TrackingBody * tb = Input::instance()->getBody(bodiesChoice->getSelectedEntry());
        if (tb)
        {
            vsg::dmat4 m;
            m = makeEulerMat((double)bodyRot[0]->getValue(), (double)bodyRot[1]->getValue(), (double)bodyRot[2]->getValue());

            vsg::dmat4 translationMat;
            translationMat = vsg::translate(bodyTrans[0]->getValue(), bodyTrans[1]->getValue(), bodyTrans[2]->getValue());
            m = translationMat * m;
            tb->setOffsetMat(m);
        }
    }
    else if(tUIItem == deviceTrans[0] || tUIItem == deviceTrans[1] || tUIItem == deviceTrans[2] ||
            tUIItem == deviceRot[0] || tUIItem == deviceRot[1] || tUIItem == deviceRot[2])
    {
        InputDevice *id = Input::instance()->getDevice(devicesChoice->getSelectedEntry());
        if (id)
        {
            vsg::dmat4 m;
            m = makeEulerMat((double)deviceRot[0]->getValue(), (double)deviceRot[1]->getValue(), (double)deviceRot[2]->getValue());

            vsg::dmat4 translationMat;
            translationMat = vsg::translate(deviceTrans[0]->getValue(), deviceTrans[1]->getValue(), deviceTrans[2]->getValue());
            m = translationMat * m;
            id->setOffsetMat(m);
        }
    }
    else if (tUIItem == debugMatrices || tUIItem == debugOther ||
            tUIItem == debugRawButton || tUIItem == debugMouseButton || tUIItem == debugDriverButton || tUIItem == debugTransformedButton)
    {
        int debug = Input::instance()->debug(Input::Config);
        if (debugRawButton->getState())
            debug |= Input::Raw;
        if (debugMouseButton->getState())
            debug |= Input::Mouse;
        if (debugDriverButton->getState())
            debug |= Input::Driver;
        if (debugTransformedButton->getState())
            debug |= Input::Transformed;
        if (debugMatrices->getState())
            debug |= Input::Matrices;
        if (debugOther->getState())
            debug |= Input::Buttons|Input::Valuators;
        Input::instance()->setDebug(debug);
    }
}

void coInputTUI::tabletPressEvent(vvTUIElement *tUIItem)
{
}
void coInputTUI::tabletReleaseEvent(vvTUIElement *tUIItem)
{
}


void vvTui::updateState()
{
}

void vvTui::updateFPS(double fps)
{
    char buf[300];
    sprintf(buf, "Fps: %6.2f", fps);
    FPSLabel->setLabel(buf);
}

void vvTui::update()
{
	inputTUI->update();
    if (debugLevel->getValue() != vvConfig::instance()->getDebugLevel())
    {
        debugLevel->setValue(vvConfig::instance()->getDebugLevel());
    }
    if (navigationMode != vvNavigationManager::instance()->getMode())
    {
        navigationMode = vvNavigationManager::instance()->getMode();

		XForm->setState(false);
		Scale->setState(false);
		Fly->setState(false);
		Drive->setState(false);
		Walk->setState(false);
        switch (navigationMode)
        {
        case vvNavigationManager::XForm:
            XForm->setState(true);
            driveLabel->setLabel("Rotate");
            panLabel->setLabel("Panning");
            break;
        case vvNavigationManager::Scale:
            Scale->setState(true);
            driveLabel->setLabel("Scale (up/down)");
            panLabel->setLabel("Scale (right/left)");
            break;
        case vvNavigationManager::Fly:
            Fly->setState(true);
            driveLabel->setLabel("Pitch & heading");
            panLabel->setLabel("Forward: pitch & roll");
            break;
        case vvNavigationManager::Glide:
            Drive->setState(true);
            driveLabel->setLabel("Speed & heading");
            panLabel->setLabel("Panning");
            break;
        case vvNavigationManager::Walk:
            Walk->setState(true);
            driveLabel->setLabel("Speed & heading");
            panLabel->setLabel("Panning");
            break;
        default:
            ;
        }
    }
    if (collision != vvNavigationManager::instance()->getCollision())
    {
        collision = vvNavigationManager::instance()->getCollision();
        Collision->setState(collision);
    }
    if (DisableIntersection->getState() != (vvIntersection::instance()->numIsectAllNodes == 0))
    {
        DisableIntersection->setState(vvIntersection::instance()->numIsectAllNodes == 0);
    }
    if (testImage->getState() != MarkerTracking::instance()->testImage)
    {
        testImage->setState(MarkerTracking::instance()->testImage);
    }
    if (Freeze->getState() != vvConfig::instance()->frozen())
    {
        Freeze->setState(vvConfig::instance()->frozen());
    }
    if (driveSpeed != vvNavigationManager::instance()->getDriveSpeed())
    {
        driveSpeed = vvNavigationManager::instance()->getDriveSpeed();
        NavSpeed->setValue(driveSpeed);
    }
    if (ScaleValue != vv->getScale())
    {
        ScaleValue = vv->getScale();
        ScaleSlider->setValue(ScaleValue);
    }

    if ((driveNav->down || panNav->down))
    {

        if (navigationMode == vvNavigationManager::Walk || navigationMode == vvNavigationManager::Glide)
        {
            doTabWalk();
        }

        else if (navigationMode == vvNavigationManager::Fly)
        {
            doTabFly();
        }

        else if (navigationMode == vvNavigationManager::Scale)
        {
            doTabScale();
        }

        else if (navigationMode == vvNavigationManager::XForm)
        {
            doTabXform();
        }
    }

    if (inputTUI)
        inputTUI->updateTUI();
}

void vvTui::tabletEvent(vvTUIElement *tUIItem)
{

    if (tUIItem == debugLevel)
    {
        vvConfig::instance()->setDebugLevel(debugLevel->getValue());
    }

    if (tUIItem == driveNav)
    {
        mx = driveNav->x;
        my = driveNav->y;
    }
    else if (tUIItem == panNav)
    {
        mx = panNav->x;
        my = panNav->y;
    }
    else if (tUIItem == testImage)
    {
        MarkerTracking::instance()->testImage = testImage->getState();
    }
    else if (tUIItem == DebugBins)
    {
        if (DebugBins->getState())
        {
           // binList->refresh();
        }
        else
        {
           // binList->removeAll();
        }
    }
    else if (tUIItem == FlipStereo)
    {
        vvViewer::instance()->flipStereo();
    }
    else if (tUIItem == PresentationForward)
    {
        PresentationStep->setValue(PresentationStep->getValue() + 1);
        char buf[100];
        sprintf(buf, "%d\ngoToStep %d", coGRMsg::KEYWORD, PresentationStep->getValue());
        vv->guiToRenderMsg(buf);
    }
    else if (tUIItem == PresentationBack)
    {
        if (PresentationStep->getValue() > 0)
        {
            PresentationStep->setValue(PresentationStep->getValue() - 1);
        }
        char buf[100];
        sprintf(buf, "%d\ngoToStep %d", coGRMsg::KEYWORD, PresentationStep->getValue());
        vv->guiToRenderMsg(buf);
    }
    else if (tUIItem == PresentationStep)
    {
        char buf[100];
        sprintf(buf, "%d\ngoToStep %d", coGRMsg::KEYWORD, PresentationStep->getValue());
        vv->guiToRenderMsg(buf);
    }

    if (tUIItem == nearEdit || tUIItem == farEdit)
    {
        vvConfig::instance()->setNearFar(nearEdit->getValue(), farEdit->getValue());
    }
    if (tUIItem == posX)
    {
        viewPos[0] = posX->getValue();
        vvViewer::instance()->setInitialViewerPos(viewPos);
        vsg::dmat4 viewMat;
        setTrans(viewMat,viewPos);
        vvViewer::instance()->setViewerMat(viewMat);
    }
    else if (tUIItem == posY)
    {
        viewPos[1] = posY->getValue();
        vvViewer::instance()->setInitialViewerPos(viewPos);
        vsg::dmat4 viewMat;
        setTrans(viewMat, viewPos);
        vvViewer::instance()->setViewerMat(viewMat);
    }
    else if (tUIItem == posZ)
    {
        viewPos[2] = posZ->getValue();
        vvViewer::instance()->setInitialViewerPos(viewPos);
        vsg::dmat4 viewMat;
        setTrans(viewMat, viewPos);
        vvViewer::instance()->setViewerMat(viewMat);
    }
    else if (tUIItem == fovH)
    {
        float fov = (fovH->getValue() / 180.0) * M_PI;
        vvConfig::instance()->HMDViewingAngle = fovH->getValue();
        float dist = (vvConfig::instance()->screens[0].hsize / 2.0) / tan(fov / 2.0);
        viewPos[0] = 0.0;
        viewPos[1] = vvConfig::instance()->screens[0].xyz[1] - dist;
        fprintf(stderr, "dist = %f\n", viewPos[1]);
        viewPos[2] = 0.0;
        vvViewer::instance()->setInitialViewerPos(viewPos);
        vsg::dmat4 viewMat;
        setTrans(viewMat,viewPos);
        vvViewer::instance()->setViewerMat(viewMat);
    }
    else if (tUIItem == stereoSep)
    {
        vvViewer::instance()->setSeparation(stereoSep->getValue());
    }
    else if (tUIItem == CFPS)
    {
        vvConfig::instance()->setFrameRate(CFPS->getValue());
    }
    else if (tUIItem == backgroundColor)
    {
        vvViewer::instance()->setClearColor(vsg::vec4(backgroundColor->getRed(), backgroundColor->getGreen(), backgroundColor->getBlue(), backgroundColor->getAlpha()));
    }

    else if (tUIItem == NavSpeed)
    {
        driveSpeed = NavSpeed->getValue();
        vvNavigationManager::instance()->setDriveSpeed(driveSpeed);
    }
    else if (tUIItem == ScaleSlider)
    {
        ScaleValue = ScaleSlider->getValue();
        vv->setScale(ScaleValue);
    }
    else if(tUIItem == SceneUnit)
    {
        vv->setSceneUnit((LengthUnit)SceneUnit->getSelectedEntry());
    }
    else if (tUIItem == driveNav)
    {
        if (driveNav->down)
        {
            mx = driveNav->x;
            my = driveNav->y;
        }
    }
    else if (tUIItem == panNav)
    {
        if (panNav->down)
        {
            mx = panNav->x;
            my = panNav->y;
        }
    }
    else if (tUIItem == FileBrowser)
    {
        //Retrieve Data object
        std::string filename = FileBrowser->getSelectedPath();

        //Do what you want to do with the filename
        vvVIVE::instance()->hud->show();
        vvVIVE::instance()->hud->setText1("Loading File...");
        vvVIVE::instance()->hud->setText2(filename.c_str());
        vvFileManager::instance()->loadFile(filename.c_str(), FileBrowser);
    }
    else if (tUIItem == SaveFileFB)
    {
        //Retrieve Data object
        std::string filename = SaveFileFB->getSelectedPath();

        vvVIVE::instance()->hud->show();
        vvVIVE::instance()->hud->setText1("Saving File...");
        vvVIVE::instance()->hud->setText2(filename.c_str());

        cerr << "File-Path: " << SaveFileFB->getSelectedPath().c_str() << endl;
        //Do what you want to do with the filename

        vsg::write(vv->getObjectsRoot(), SaveFileFB->getFilename(SaveFileFB->getSelectedPath()), vvPluginSupport::instance()->options);
        {
            if (vv->debugLevel(3))
                std::cerr << "Data written to \"" << SaveFileFB->getFilename(SaveFileFB->getSelectedPath()) << "\"." << std::endl;
        }

        //vvFileManager::instance()->replaceFile(filename.c_str(), SaveFileFB);
    }
    else if (tUIItem == LODScaleEdit)
    {
        vvConfig::instance()->setLODScale(LODScaleEdit->getValue());
        //for (int i = 0; i < vvConfig::instance()->numChannels(); i++)
          //  vvConfig::instance()->channels[i].camera->setLODScale(LODScaleEdit->getValue());
    }
    vvVIVE::instance()->hud->hide();
}

void vvTui::tabletPressEvent(vvTUIElement *tUIItem)
{
    if (tUIItem == driveNav)
    {
        mx = driveNav->x;
        my = driveNav->y;
    }
    else if (tUIItem == panNav)
    {
        mx = panNav->x;
        my = panNav->y;
    }

    if (tUIItem == Quit)
    {
        vvVIVE::instance()->requestQuit();
    }
    else if (tUIItem == ViewAll)
    {
        vvSceneGraph::instance()->viewAll();
    }
    else if (tUIItem == Walk)
    {
        vvNavigationManager::instance()->setNavMode(vvNavigationManager::Walk);
    }
    else if (tUIItem == Fly)
    {
        vvNavigationManager::instance()->setNavMode(vvNavigationManager::Fly);
    }
    else if (tUIItem == Drive)
    {
        vvNavigationManager::instance()->setNavMode(vvNavigationManager::Glide);
    }
    else if (tUIItem == Scale)
    {
        vvNavigationManager::instance()->setNavMode(vvNavigationManager::Scale);
    }
    else if (tUIItem == XForm)
    {
        vvNavigationManager::instance()->setNavMode(vvNavigationManager::XForm);
    }
    else if (tUIItem == Collision)
    {
        vvNavigationManager::instance()->toggleCollide(true);
    }
    else if (tUIItem == Freeze)
    {
        vvSceneGraph::instance()->toggleHeadTracking(false);
    }
    else if (tUIItem == Wireframe)
    {
        vvSceneGraph::instance()->setWireframe(vvSceneGraph::Enabled);
    }
    else if (tUIItem == Menu)
    {
        vvSceneGraph::instance()->setMenu(vvSceneGraph::MenuHidden);
    }
    else if ((tUIItem == panNav) || (tUIItem == driveNav))
    {
        startTabNav();
    }
}

void vvTui::tabletReleaseEvent(vvTUIElement *tUIItem)
{
    if (tUIItem == driveNav)
    {
        mx = driveNav->x;
        my = driveNav->y;
    }
    else if (tUIItem == panNav)
    {
        mx = panNav->x;
        my = panNav->y;
    }

    if (tUIItem == viveTab)
    {
        //  cerr << "entry:" << cBox->getSelectedText() << endl;
    }
    else if ((tUIItem == panNav) || (tUIItem == driveNav))
    {
        stopTabNav();
    }
#if 0
    else if (tUIItem == Walk)
    {
        vv->disableNavigation("Walk");
    }
    else if (tUIItem == Fly)
    {
        vv->disableNavigation("Fly");
    }
    else if (tUIItem == Drive)
    {
        vv->disableNavigation("Drive");
    }
    else if (tUIItem == Scale)
    {
        vv->disableNavigation("Scale");
    }
    else if (tUIItem == XForm)
    {
        vv->disableNavigation("XForm");
    }
#endif
    else if (tUIItem == Collision)
    {
        vvNavigationManager::instance()->toggleCollide(false);
    }
    else if (tUIItem == Freeze)
    {
        vvSceneGraph::instance()->toggleHeadTracking(true);
    }
    else if (tUIItem == Wireframe)
    {
        vvSceneGraph::instance()->setWireframe(vvSceneGraph::Disabled);
    }
    else if (tUIItem == Menu)
    {
        vvSceneGraph::instance()->setMenu(vvSceneGraph::MenuAndObjects);
    }
}

void vvTui::doTabFly()
{
    vsg::dvec3 velDir;

    vsg::dmat4 dcs_mat = vv->getObjectsXform()->matrix;
    double heading = 0.0;
    if (driveNav->down)
        heading = (mx - x0) / -300;
    double pitch = (my - y0) / -300;
    double roll = (mx - x0) / -300;
    if (driveNav->down)
        roll = 0.;

    vsg::dmat4 rot;
    //rot.makeEuler(heading,pitch,roll);
    rot = makeEulerMat(vsg::dvec3( heading, pitch, roll));
    dcs_mat = dcs_mat * rot ;
    // XXX
    //dcs_mat.getRow(1,velDir);                // velocity in direction of viewer
    velDir[0] = 0.0;
    if (panNav->down)
        velDir[1] = -1.0;
    else
        velDir[1] = 0;
    velDir[2] = 0.0;
    vv->getObjectsXform()->matrix = dcs_mat * vsg::translate(velDir * (double)currentVelocity);
    //       navigating = true;
    vvCollaboration::instance()->SyncXform();
}

void vvTui::doTabXform()
{
    if (driveNav->down)
    {

        float newx, newy; //newz;//relz0
        float heading, pitch, roll;
        //rollVerti, rollHori;

        //    whichButton = 1;
        newx = mx - originX;
        newy = my - originY;

        //getPhi kann man noch aendern, wenn xy-Rotation nicht gewichtet werden soll
        heading = getPhi(newx - relx0, widthX);
        pitch = getPhi(newy - rely0, widthY);
        roll = getPhi(newy - rely0, widthY);
        roll = 0;

        //makeRot(heading, pitch, roll, headingBool, pitchBool, rollBool);

        makeRot(-heading, -pitch, -roll, 1, 1, 1);
        relx0 = mx - originX;
        rely0 = my - originY;
    }

    //Translation funktioniert
    if (panNav->down)
    {
        //irgendwas einbauen, damit die folgenden Anweisungen vor der
        //naechsten if-Schleife nicht unnoetigerweise dauernd ausgefuehrt werden

        float newxTrans = mx - originX;
        float newyTrans = my - originY;
        float xTrans, yTrans;
        xTrans = (newxTrans - relx0);
        yTrans = (newyTrans - rely0);
        cerr << yTrans << endl;
        vsg::dmat4 actTransState = mat0;
        vsg::dmat4 trans;
        vsg::dmat4 doTrans;
        trans= vsg::translate(1.1 * xTrans, 0.0, -1.1 * yTrans);
        //die 1.1 sind geschaetzt, um die Ungenauigkeit des errechneten Faktors auszugleichen
        //es sieht aber nicht so aus, als wuerde man es auf diese
        //Weise perfekt hinbekommen
        doTrans = actTransState * trans;
        vv->getObjectsXform()->matrix = (doTrans);
        vvCollaboration::instance()->SyncXform();
    }
}

void vvTui::doTabScale()
{
    float newx, newScaleFactor; // newy,
    float maxFactor = 10.0;
    float widthX = vvConfig::instance()->windows[0].sx;
    float ampl = widthX / 2;

    if (driveNav->down)
        newx = my - originY;
    else
        newx = mx - originX;

    //Skalierung mittels e-Funktion, kann nicht negativ werden
    newScaleFactor = exp(log(maxFactor) * ((newx - relx0) / ampl));
    vv->setScale(actScaleFactor * newScaleFactor);
    vvCollaboration::instance()->SyncXform();
}

void vvTui::doTabWalk()
{
    vsg::dvec3 velDir;
    vsg::dmat4 dcs_mat = vv->getObjectsXform()->matrix;
    if (driveNav->down)
    {
        float angle;

        angle = (float)((mx - x0) * vv->frameDuration()) / 300.0f;
        /*
      if((mx - x0) < 0)
      angle =  -((mx - x0)*(mx - x0) / 4000);
      else
      angle =  (mx - x0)*(mx - x0) / 4000;*/
        dcs_mat = vsg::rotate((double)angle, 0.0, 0.0, 1.0) * dcs_mat;
        velDir[0] = 0.0;
        velDir[1] = 1.0;
        velDir[2] = 0.0;
        currentVelocity = (my - y0 ) * driveSpeed * vv->frameDuration();
        dcs_mat = vsg::translate(velDir * (double)currentVelocity) * dcs_mat;
    }
    else if (panNav->down)
    {
        dcs_mat =  vsg::translate(( mx - x0) * driveSpeed * 0.1 * 1.0, 0.0, (my - y0) * driveSpeed * 0.1 * 1) * dcs_mat;
    }
    vv->getObjectsXform()->matrix = (dcs_mat);
    vvCollaboration::instance()->SyncXform();
}

void vvTui::stopTabNav()
{
    actScaleFactor = vv->getScale();

    x0 = mx;
    y0 = my;
    currentVelocity = 10;
    relx0 = x0 - originX; //relativ zum Ursprung des Koordinatensystems
    rely0 = y0 - originY; //dito
    cerr << "testit" << endl;
}

void vvTui::startTabNav()
{
    widthX = 200;
    widthY = 170;
    originX = widthX / 2.0;
    originY = widthY / 2.0;

    vsg::dmat4 dcs_mat = vv->getObjectsXform()->matrix;
    actScaleFactor = vv->getScale();
    x0 = mx;
    y0 = my;
    mat0 = dcs_mat;
    mat0_Scale = dcs_mat;
    //cerr << "mouseNav" << endl;
    currentVelocity = 10;

    relx0 = x0 - originX; //relativ zum Ursprung des Koordinatensystems
    rely0 = y0 - originY; //dito
}

float vvTui::getPhiZVerti(float y2, float y1, float x2, float widthX, float widthY)
{
    float phiZMax = 180.0;
    float y = (y2 - y1);
    float phiZ = 4 * pow(x2, 2) * phiZMax * y / (pow(widthX, 2) * widthY);
    if (x2 < 0.0)
        return (phiZ);
    else
        return (-phiZ);
}

float vvTui::getPhiZHori(float x2, float x1, float y2, float widthY, float widthX)
{
    float phiZMax = 180.0;
    float x = x2 - x1;
    float phiZ = 4 * pow(y2, 2) * phiZMax * x / (pow(widthY, 2) * widthX);

    if (y2 > 0.0)
        return (phiZ);
    else
        return (-phiZ);
}

float vvTui::getPhi(float relCoord1, float width1)
{
    float phi, phiMax = 180.0;
    phi = phiMax * relCoord1 / width1;
    return (phi);
}

void vvTui::makeRot(float heading, float pitch, float roll, int headingBool, int pitchBool, int rollBool)
{
    vsg::dmat4 actRotState = vv->getObjectsXform()->matrix;
    vsg::dmat4 rot;
    vsg::dmat4 doRot;
    //rot.makeEuler(headingBool * heading, pitchBool * pitch, rollBool * roll);
    rot = makeEulerMat(headingBool * heading, pitchBool * pitch, rollBool * roll);
    vv->getObjectsXform()->matrix = actRotState * rot;
    vvCollaboration::instance()->SyncXform();
}

vvTUIFileBrowserButton *vvTui::getExtFB()
{
    return SaveFileFB;
}
/*
BinListEntry::BinListEntry(vvTabletUI *tui, osgUtil::RenderBin *rb, int num)
{
    binNumber = rb->getBinNum();
    std::string name;
    std::stringstream ss;
    ss << binNumber;
    name = ss.str();
    tb = new vvTUIToggleButton(tui, name, vvTui::instance()->getTopContainer()->getID(), true);
    tb->setPos(4, num);
    tb->setEventListener(this);
}

void BinListEntry::updateBin()
{
    osgUtil::RenderBin *bin = renderBin();
    if (bin)
    {
        if (tb->getState())
        {
            bin->setDrawCallback(NULL);
        }
        else
        {
            bin->setDrawCallback(new DontDrawBin);
        }
    }
}

osgUtil::RenderBin *BinListEntry::renderBin()
{
    osgUtil::RenderStage *rs = dynamic_cast<osgViewer::Renderer *>(vvConfig::instance()->channels[0].camera->getRenderer())->getSceneView(0)->getRenderStage();
    if (rs == NULL)
        rs = dynamic_cast<osgViewer::Renderer *>(vvConfig::instance()->channels[0].camera->getRenderer())->getSceneView(0)->getRenderStageLeft();
    if (rs)
    {
        BinRenderStage bs = *rs;
        osgUtil::RenderBin *rb = bs.getPreRenderStage();
        if (rb)
        {
            for (osgUtil::RenderBin::RenderBinList::iterator it = rb->getRenderBinList().begin(); it != rb->getRenderBinList().end(); it++)
            {
                if (it->second->getBinNum() == binNumber)
                    return it->second;
            }
        }
        rb = rs;
        for (osgUtil::RenderBin::RenderBinList::iterator it = rb->getRenderBinList().begin(); it != rb->getRenderBinList().end(); it++)
        {
            if (it->second->getBinNum() == binNumber)
                return it->second;
        }
        rb = bs.getPostRenderStage();
        if (rb)
        {
            for (osgUtil::RenderBin::RenderBinList::iterator it = rb->getRenderBinList().begin(); it != rb->getRenderBinList().end(); it++)
            {
                if (it->second->getBinNum() == binNumber)
                    return it->second;
            }
        }
    }
    return NULL;
}
BinListEntry::~BinListEntry()
{
    delete tb;
    renderBin()->setDrawCallback(NULL);
}
void BinListEntry::tabletEvent(vvTUIElement *tUIItem)
{

  
}

BinList::BinList(vvTabletUI *tui): tui(tui)
{
}

BinList::~BinList()
{
    removeAll();
}
void BinList::refresh()
{
    for (BinList::iterator it = begin(); it != end(); it++)
    {
        delete *it;
    }
    clear();
    osgUtil::RenderStage *rs = dynamic_cast<osgViewer::Renderer *>(vvConfig::instance()->channels[0].camera->getRenderer())->getSceneView(0)->getRenderStage();
    if (rs == NULL)
        rs = dynamic_cast<osgViewer::Renderer *>(vvConfig::instance()->channels[0].camera->getRenderer())->getSceneView(0)->getRenderStageLeft();
    if (rs)
    {
        BinRenderStage bs = *rs;
        osgUtil::RenderBin *rb = bs.getPreRenderStage();
        if (rb)
        {
            for (osgUtil::RenderBin::RenderBinList::iterator it = rb->getRenderBinList().begin(); it != rb->getRenderBinList().end(); it++)
            {
                push_back(new BinListEntry(tui, it->second, size()));
            }
        }
        rb = rs;
        for (osgUtil::RenderBin::RenderBinList::iterator it = rb->getRenderBinList().begin(); it != rb->getRenderBinList().end(); it++)
        {
            push_back(new BinListEntry(tui, it->second, size()));
        }
        rb = bs.getPostRenderStage();
        if (rb)
        {
            for (osgUtil::RenderBin::RenderBinList::iterator it = rb->getRenderBinList().begin(); it != rb->getRenderBinList().end(); it++)
            {
                push_back(new BinListEntry(tui, it->second, size()));
            }
        }
    }
}

void BinList::updateBins()
{
    for (BinList::iterator it = begin(); it != end(); it++)
    {
        (*it)->updateBin();
    }
}
void BinList::removeAll()
{
    for (BinList::iterator it = begin(); it != end(); it++)
    {
        delete *it;
    }
    clear();
}
*/