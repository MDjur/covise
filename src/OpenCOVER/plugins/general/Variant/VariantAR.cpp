#include "VariantAR.h"

#include <OpenVRUI/coCheckboxMenuItem.h>
#include <OpenVRUI/coRowMenu.h>
#include <OpenVRUI/coSubMenuItem.h>
#include <OpenVRUI/osg/mathUtils.h>
#include <cover/ARToolKit.h>
#include <cover/RenderObject.h>
#include <cover/VRSceneGraph.h>
#include <cover/VRViewer.h>
#include <cover/coInteractor.h>
#include <cover/coVRAnimationManager.h>
#include <cover/coVRConfig.h>

#ifdef USE_COVISE
#include <appl/RenderInterface.h>
#endif

using namespace covise;

VariantAR::VariantAR(std::string varName, osg::Node *node, osg::Node::ParentList parents, ui::Menu *VariantMenu, coTUITab *VariantPluginTab, int numVar, QDomDocument *qdomDoc, QDomElement *qdomElem, coVRBoxOfInterest *boxOfInterest, bool defaultState)
    : Variant(varName, node, parents, VariantMenu, VariantPluginTab, numVar, qdomDoc, qdomElem, boxOfInterest, defaultState) {
    variantClass = this;
}

VariantAR::TraceModule::TraceModule(int ID, const std::string &name, int instanceID, const std::string &fbInfo, std::shared_ptr<coTUITab> tab, std::shared_ptr<coInteractor> inter)
    : m_id(ID),
      m_interactor(inter),
      m_tab(tab),
      m_moduleName(name),
      m_modInstanceID(instanceID),
      m_oldTime(0.0),
      m_positionThreshold(3000.0),
      m_positionChanged(false),
      m_oldVisibility(true),
      m_enabled(true),
      m_firstUpdate(true),
      m_doUpdate(false) {
    firstModuleInteractor() = ModuleInteractor(
        std::make_shared<ModuleInteractorPoint>(0.0f, 0.0f, 0.1f, tab->getID()),
        tab->getID(),
        osg::Vec3(0, 0, 0.1),
        osg::Vec3(1, 0, 0),
        osg::Vec3(0, 0, 0),
        osg::Vec3(11110, 0, 0),
        osg::Vec3(0, 0, 0));
    secondModuleInteractor() = ModuleInteractor(
        std::make_shared<ModuleInteractorPoint>(0.0f, 0.0f, 1.1f, tab->getID()),
        tab->getID(),
        osg::Vec3(0, 0, 100.1),
        osg::Vec3(0, 1, 0),
        osg::Vec3(0, 0, 0),
        osg::Vec3(111110, 0, 0),
        osg::Vec3(0, 0, 0));

    int tabID = m_tab->getID();
    m_updateOnVisibilityChange->setState(false);

    std::string markerName = name + std::to_string(instanceID);
    std::string label = markerName + ":";

    m_marker = std::make_shared<ARToolKitMarker>(markerName.c_str());
    m_arMenuEntry = std::make_shared<coSubMenuItem>(markerName.c_str());
    // m_plugin->arMenu->add(m_arMenuEntry);
    m_moduleMenu = std::make_shared<coRowMenu>(markerName.c_str());
    m_enabledToggle = std::make_shared<coCheckboxMenuItem>("enabled", m_enabled);
    m_TracerModuleLabel = std::make_shared<coTUILabel>(label, tabID);
    m_updateOnVisibilityChange = std::make_shared<coTUIToggleButton>("updateOnVisibilityChange", tabID);
    m_updateNow = std::make_shared<coTUIButton>("update", tabID);
    m_updateInterval = std::make_shared<coTUIEditFloatField>("updateInterval", tabID);
    m_arMenuEntry->setMenu(m_moduleMenu.get());
    m_moduleMenu->add(m_enabledToggle.get());

    m_enabledToggle->setMenuListener(this);
    m_TracerModuleLabel->setEventListener(this);
    m_updateOnVisibilityChange->setEventListener(this);
    m_updateInterval->setEventListener(this);
    m_updateNow->setEventListener(this);

    m_TracerModuleLabel->setPos(0, 1 + ID * 5);
    m_updateOnVisibilityChange->setPos(0, 1 + ID * 5 + 1);
    m_updateInterval->setPos(1, 1 + ID * 5 + 1);
    m_updateNow->setPos(0, 1 + ID * 5 + 2);

    int i = 3;
    for (auto &inter : m_ModInteractors) {
        auto &point = inter.interactorPoint();
        point->setEventListener(this);
        point->setUIPos(1 + ID * 5 + i++);
    }

    m_feedbackInfo = fbInfo.empty() ? "" : fbInfo;
}

void VariantAR::TraceModule::tabletPressEvent(coTUIElement *tUIItem) {
    if (tUIItem == m_updateNow.get())
        m_doUpdate = true;
}

void VariantAR::TraceModule::tabletEvent(coTUIElement *tUIItem) {
    for (auto &inter : m_ModInteractors)
        inter.interactorEvent(tUIItem);
}

void VariantAR::TraceModule::menuEvent(coMenuItem *menuItem) {
    if (menuItem == m_enabledToggle.get())
        m_enabled = m_enabledToggle->getState();
}

bool VariantAR::TraceModule::calcPositionChanged() {
    if (std::any_of(m_ModInteractors.begin(), m_ModInteractors.end(), [this](auto &inter) { return inter.getDiff().length() > m_positionThreshold; }))
        return false;

    resetInteractorsLastPos();
    return true;
}

void VariantAR::TraceModule::update() {
    if (m_marker)
        return;
    if (m_firstUpdate) {
        m_firstUpdate = false;
        return;
    }
    m_positionChanged = calcPositionChanged();
    bool visibilityChanged = m_oldVisibility != m_marker->isVisible();
    m_oldVisibility = m_marker->isVisible();

    // marker position in camera coordinate system
    const auto &MarkerPos = m_marker->getMarkerTrans();
    const auto vrviewer = VRViewer::instance();
    auto leftCameraTrans = vrviewer->getViewerMat();
    if (coVRConfig::instance()->stereoState())
        leftCameraTrans = osg::Matrix::translate(-(vrviewer->getSeparation() / 2.0), 0, 0) * vrviewer->getViewerMat();

    const auto &MarkerInWorld = MarkerPos * leftCameraTrans;
    const auto &MarkerInLocalCoords = MarkerInWorld * cover->getInvBaseMat();
    for (auto &inter : m_ModInteractors) {
        inter.updateCurPos(MarkerInLocalCoords);
        inter.updateNormal(MarkerInLocalCoords);
    }
    if (m_positionChanged) {
        resetInteractorsLastPos();
    } else {
        const auto &updateinterval = m_updateInterval->getValue();
        const auto &frameTimeDiff = cover->frameTime() - m_oldTime;
        const bool &visibilityStateUpdate = m_updateOnVisibilityChange->getState();
        const bool &validInterval = 0 < updateinterval < frameTimeDiff;
        if (((visibilityStateUpdate && visibilityChanged) || m_doUpdate || (!visibilityStateUpdate && validInterval)) && m_marker->isVisible()) {
            // send
            m_doUpdate = false;
            m_oldTime = cover->frameTime();

            auto &currentNormal = firstModuleInteractor().currentNormal();
            auto &currentNormal2 = secondModuleInteractor().currentNormal();
            auto &currentPosition1 = firstModuleInteractor().currentPosition();
            auto &currentPosition2 = secondModuleInteractor().currentPosition();
            currentNormal.normalize();
            float c = currentPosition1 * currentNormal;
            c /= currentNormal.length();
            currentNormal.normalize();
            currentNormal2.normalize();
            // char ch;
            // char buf[10000];
            // fprintf(stderr, "%s %f %f %f    %f %f %f\n", m_moduleName.c_str(), currentPosition1[0], currentPosition1[1], currentPosition1[2], currentNormal[0], currentNormal[1], currentNormal[2]);

            // if (m_interactor)
            // {
            //     if (strncmp(m_interactor->getPluginName(), "Tracer", 6) == 0)
            //     {
            //         m_interactor->setVectorParam("startpoint1", currentPosition1[0], currentPosition1[1], currentPosition1[2]);
            //         m_interactor->setVectorParam("startpoint2", currentPosition2[0], currentPosition2[1], currentPosition2[2]);
            //         m_interactor->executeModule();
            //     }
            // }
            //             else if (!m_feedbackInfo.empty())
            //             {
            // #ifdef USE_COVISE
            //                 CoviseRender::set_feedback_info(m_feedbackInfo.c_str());
            //                 auto sendFeedback = [&buf](const char *msgType)
            //                 { CoviseRender::send_feedback_message(msgType, buf); };
            //                 auto sendParam = [&sendFeedback]()
            //                 { sendFeedback("PARAM"); };
            //                 auto sendExec = [&sendFeedback]()
            //                 { sendFeedback("EXEC"); };
            //                 switch (ch = CoviseRender::get_feedback_type())
            //                 {
            //                 case 'C':
            //                     /* button just pressed */
            //                     {
            //                         fprintf(stdout, "\a");
            //                         fflush(stdout);
            //                         sprintf(buf, "vertex\nFloatVector\n%f %f %f\n", currentNormal[0], currentNormal[1], currentNormal[2]);
            //                         sendParam();
            //                         sendParam();
            //                         sprintf(buf, "scalar\nFloatScalar\n%f\n", c);
            //                         sendParam();
            //                         sendParam();
            //                         buf[0] = '\0';
            //                         sendExec();
            //                     }
            //                     break;
            //                 case 'G': // CutGeometry with new parameter names
            //                 {
            //                     fprintf(stdout, "\a");
            //                     fflush(stdout);
            //                     sprintf(buf, "normal\nFloatVector\n%f %f %f\n", currentNormal[0], currentNormal[1], currentNormal[2]);
            //                     sendParam();
            //                     sprintf(buf, "distance\nFloatScalar\n%f\n", c);
            //                     sendParam();
            //                     buf[0] = '\0';
            //                     sendExec();
            //                 }
            //                 break;
            //                 case 'Z':
            //                 {
            //                     fprintf(stdout, "\a");
            //                     fflush(stdout);
            //                     sprintf(buf, "vertex\nFloatVector\n%f %f %f\n", currentPosition1[0], currentPosition1[1], currentPosition1[2]);
            //                     sendParam();
            //                     sendExec();
            //                 }
            //                 break;
            //                 case 'A':
            //                 {
            //                     fprintf(stdout, "\a");
            //                     fflush(stdout);
            //                     sprintf(buf, "position\nFloatVector\n%f %f %f\n", currentPosition1[0], currentPosition1[1], currentPosition1[2]);
            //                     sendParam();
            //                     sprintf(buf, "normal\nFloatVector\n%f %f %f\n", currentNormal[0], currentNormal[1], currentNormal[2]);
            //                     sendParam();
            //                     sprintf(buf, "normal2\nFloatVector\n%f %f %f\n", currentNormal2[0], currentNormal2[1], currentNormal2[2]);
            //                     sendParam();
            //                     buf[0] = '\0';
            //                     sendExec();
            //                 }
            //                 break;
            //                 case 'T':
            //                 {
            //                     fprintf(stdout, "\a");
            //                     fflush(stdout);
            //                     sprintf(buf, "startpoint1\nFloatVector\n%f %f %f\n", currentPosition1[0], currentPosition1[1], currentPosition1[2]);
            //                     sendParam();
            //                     sprintf(buf, "startpoint2\nFloatVector\n%f %f %f\n", currentPosition2[0], currentPosition2[1], currentPosition2[2]);
            //                     sendParam();
            //                     buf[0] = '\0';
            //                     sendExec();
            //                 }
            //                 break;
            //                 case 'P':
            //                 {
            //                     fprintf(stdout, "\a");
            //                     fflush(stdout);
            //                     sprintf(buf, "startpoint1\nFloatVector\n%f %f %f\n", currentPosition1[0], currentPosition1[1], currentPosition1[2]);
            //                     sendParam();
            //                     sprintf(buf, "startpoint2\nFloatVector\n%f %f %f\n", currentPosition2[0], currentPosition2[1], currentPosition2[2]);
            //                     sendParam();
            //                     // TracerUsg has no normal parameter (nor Tracer)...
            //                     /* && strcmp(currentFeedbackInfo+1,"Tracer")!= 0 */
            //                     if (strncmp(m_feedbackInfo.c_str() + 1, "Tracer", strlen("Tracer")) != 0)
            //                     {
            //                         sprintf(buf, "normal\nFloatVector\n%f %f %f\n", currentNormal[0], currentNormal[1], currentNormal[2]);
            //                         sendParam();
            //                     }
            //                     // ... but has direction
            //                     sprintf(buf, "direction\nFloatVector\n%f %f %f\n", currentNormal2[0], currentNormal2[1], currentNormal2[2]);
            //                     sendParam();
            //                     buf[0] = '\0';
            //                     sendExec();
            //                 }
            //                 break;
            //                 case 'I':
            //                 {
            //                     fprintf(stdout, "\a");
            //                     fflush(stdout);
            //                     sprintf(buf, "isopoint\nFloatVector\n%f %f %f\n", currentPosition1[0], currentPosition1[1], currentPosition1[2]);
            //                     sendParam();
            //                     buf[0] = '\0';
            //                     sendExec();
            //                 }
            //                 break;
            //                 default:
            //                     printf("unknown feedback type %c\n", ch);
            //                 }
            // #else
            //                 printf("Old style COVISE feedback not support, recompile with -DUSE_COVISE\n");
            // #endif
            //             }
        }
    }
}

void VariantAR::TraceModule::ModuleInteractor::interactorEvent(coTUIElement *tuiItem) {
    if (m_interactorPoint->X().get() == tuiItem) {
        m_startpointOffset[0] = m_interactorPoint->X()->getValue();
    } else if (m_interactorPoint->Y().get() == tuiItem) {
        m_startpointOffset[1] = m_interactorPoint->Y()->getValue();
    } else if (m_interactorPoint->Z().get() == tuiItem) {
        m_startpointOffset[2] = m_interactorPoint->Z()->getValue();
    }
}

VariantAR::TraceModule::ModuleInteractorPoint::ModuleInteractorPoint(float x, float y, float z, int plugID) {
    int pluginID = plugID;
    createAxis("startPos X", pluginID, Axis::X);
    createAxis("startPos Y", pluginID, Axis::Y);
    createAxis("startPos Z", pluginID, Axis::Z);
    setAxisValue(x, Axis::X);
    setAxisValue(y, Axis::Y);
    setAxisValue(z, Axis::Z);
}