#include "VariantAR.h"

#include <OpenVRUI/coCheckboxMenuItem.h>
#include <OpenVRUI/coRowMenu.h>
#include <OpenVRUI/coSubMenuItem.h>
#include <OpenVRUI/osg/mathUtils.h>
#include <config/CoviseConfig.h>
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

VariantAR::VariantAR(coInteractor *interactor, const Variant &variant)
    : Variant(variant) {
    variantClass = this;
    const std::string &varName = variant.getName();
    m_traceModule = std::make_unique<TraceModule>(varName, std::shared_ptr<coInteractor>(interactor));
    auto combobox = ui->getTUIARCombobox();

    // TODO: extract this to VariantPlugin and pass through list of markres
    for (const auto &entry : coCoviseConfig::getScopeEntries("COVER.Plugin.ARToolKit")) {
        auto entry_name = entry.first;
        auto entry_val = entry.second;
        if (entry_name.find("Marker") != std::string::npos) {
            size_t pos = entry_name.find(":");
            std::string name = entry_name.substr(pos + 1);
            if (name.find("ObjectMarker") != std::string::npos)
                continue;
            combobox->addEntry(name);
        }
    }
    combobox->setEventListener(this);
}

VariantAR::TraceModule::TraceModule(const std::string &markerName, std::shared_ptr<coInteractor> inter)
    : m_interactor(inter),
      m_oldTime(0.0),
      m_positionThreshold(3000.0),
      m_positionChanged(false),
      m_oldVisibility(true),
      m_enabled(true),
      m_firstUpdate(true),
      m_doUpdate(false),
      m_ModInteractors{
          ModuleInteractor(
              std::make_shared<ModuleInteractorPoint>(0.0f, 0.0f, 0.1f),
              osg::Vec3(0, 0, 0.1),
              osg::Vec3(1, 0, 0),
              osg::Vec3(0, 0, 0),
              osg::Vec3(11110, 0, 0),
              osg::Vec3(0, 0, 0)),
          ModuleInteractor(
              std::make_shared<ModuleInteractorPoint>(0.0f, 0.0f, 1.1f),
              osg::Vec3(0, 0, 100.1),
              osg::Vec3(0, 1, 0),
              osg::Vec3(0, 0, 0),
              osg::Vec3(111110, 0, 0),
              osg::Vec3(0, 0, 0))} {
    //TODO: compare name with existing markers in ARToolkit
    // bool valid = false;
    // for (auto marker : ARToolKit::instance()->markers)
    //     valid = markerName == marker->getName();

    // if (valid)
    if (!markerName.compare(""))
        m_marker = std::make_shared<ARToolKitMarker>(markerName.c_str());
}

bool VariantAR::TraceModule::calcPositionChanged() {
    if (std::any_of(m_ModInteractors.begin(), m_ModInteractors.end(), [this](auto &inter) { return inter.getDiff().length() > m_positionThreshold; }))
        return false;

    resetInteractorsLastPos();
    return true;
}

void VariantAR::TraceModule::handleCOVISEFeedback(const osg::Vec3 &currentNormal, const osg::Vec3 &currentNormal2, const osg::Vec3 &currentPosition1, const osg::Vec3 &currentPosition2) {
    // #ifdef USE_COVISE
    //     float c = currentPosition1 * currentNormal;
    //     c /= currentNormal.length();
    //     char ch;
    //     char buf[10000];
    //     CoviseRender::set_feedback_info(m_feedbackInfo.c_str());
    //     auto sendFeedback = [&](const char *msgType) { CoviseRender::send_feedback_message(msgType, buf); };
    //     auto sendParam = [&sendFeedback]() { sendFeedback("PARAM"); };
    //     auto sendExec = [&sendFeedback]() { sendFeedback("EXEC"); };
    //     switch (ch = CoviseRender::get_feedback_type()) {
    //         case 'C':
    //             /* button just pressed */
    //             {
    //                 fprintf(stdout, "\a");
    //                 fflush(stdout);
    //                 sprintf(buf, "vertex\nFloatVector\n%f %f %f\n", currentNormal[0], currentNormal[1], currentNormal[2]);
    //                 sendParam();
    //                 sendParam();
    //                 sprintf(buf, "scalar\nFloatScalar\n%f\n", c);
    //                 sendParam();
    //                 sendParam();
    //                 buf[0] = '\0';
    //                 sendExec();
    //             }
    //             break;
    //         case 'G':  // CutGeometry with new parameter names
    //         {
    //             fprintf(stdout, "\a");
    //             fflush(stdout);
    //             sprintf(buf, "normal\nFloatVector\n%f %f %f\n", currentNormal[0], currentNormal[1], currentNormal[2]);
    //             sendParam();
    //             sprintf(buf, "distance\nFloatScalar\n%f\n", c);
    //             sendParam();
    //             buf[0] = '\0';
    //             sendExec();
    //         } break;
    //         case 'Z': {
    //             fprintf(stdout, "\a");
    //             fflush(stdout);
    //             sprintf(buf, "vertex\nFloatVector\n%f %f %f\n", currentPosition1[0], currentPosition1[1], currentPosition1[2]);
    //             sendParam();
    //             sendExec();
    //         } break;
    //         case 'A': {
    //             fprintf(stdout, "\a");
    //             fflush(stdout);
    //             sprintf(buf, "position\nFloatVector\n%f %f %f\n", currentPosition1[0], currentPosition1[1], currentPosition1[2]);
    //             sendParam();
    //             sprintf(buf, "normal\nFloatVector\n%f %f %f\n", currentNormal[0], currentNormal[1], currentNormal[2]);
    //             sendParam();
    //             sprintf(buf, "normal2\nFloatVector\n%f %f %f\n", currentNormal2[0], currentNormal2[1], currentNormal2[2]);
    //             sendParam();
    //             buf[0] = '\0';
    //             sendExec();
    //         } break;
    //         case 'T': {
    //             fprintf(stdout, "\a");
    //             fflush(stdout);
    //             sprintf(buf, "startpoint1\nFloatVector\n%f %f %f\n", currentPosition1[0], currentPosition1[1], currentPosition1[2]);
    //             sendParam();
    //             sprintf(buf, "startpoint2\nFloatVector\n%f %f %f\n", currentPosition2[0], currentPosition2[1], currentPosition2[2]);
    //             sendParam();
    //             buf[0] = '\0';
    //             sendExec();
    //         } break;
    //         case 'P': {
    //             fprintf(stdout, "\a");
    //             fflush(stdout);
    //             sprintf(buf, "startpoint1\nFloatVector\n%f %f %f\n", currentPosition1[0], currentPosition1[1], currentPosition1[2]);
    //             sendParam();
    //             sprintf(buf, "startpoint2\nFloatVector\n%f %f %f\n", currentPosition2[0], currentPosition2[1], currentPosition2[2]);
    //             sendParam();
    //             // TracerUsg has no normal parameter (nor Tracer)...
    //             /* && strcmp(currentFeedbackInfo+1,"Tracer")!= 0 */
    //             if (strncmp(m_feedbackInfo.c_str() + 1, "Tracer", strlen("Tracer")) != 0) {
    //                 sprintf(buf, "normal\nFloatVector\n%f %f %f\n", currentNormal[0], currentNormal[1], currentNormal[2]);
    //                 sendParam();
    //             }
    //             // ... but has direction
    //             sprintf(buf, "direction\nFloatVector\n%f %f %f\n", currentNormal2[0], currentNormal2[1], currentNormal2[2]);
    //             sendParam();
    //             buf[0] = '\0';
    //             sendExec();
    //         } break;
    //         case 'I': {
    //             fprintf(stdout, "\a");
    //             fflush(stdout);
    //             sprintf(buf, "isopoint\nFloatVector\n%f %f %f\n", currentPosition1[0], currentPosition1[1], currentPosition1[2]);
    //             sendParam();
    //             buf[0] = '\0';
    //             sendExec();
    //         } break;
    //         default:
    //             printf("unknown feedback type %c\n", ch);
    //     }
    // #else
    //     printf("Old style COVISE feedback not supported, recompile with -DUSE_COVISE\n");
    // #endif
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
        leftCameraTrans = osg::Matrix::translate(-(vrviewer->getSeparation() / 2.0), 0, 0) * leftCameraTrans;

    const auto &MarkerInWorld = MarkerPos * leftCameraTrans;
    const auto &MarkerInLocalCoords = MarkerInWorld * cover->getInvBaseMat();
    for (auto &inter : m_ModInteractors) {
        inter.updateCurPos(MarkerInLocalCoords);
        inter.updateNormal(MarkerInLocalCoords);
    }
    if (m_positionChanged) {
        resetInteractorsLastPos();
    } else {
        const auto &frameTimeDiff = cover->frameTime() - m_oldTime;
        const bool &validInterval = 0 < frameTimeDiff;
        if ((visibilityChanged || m_doUpdate || validInterval) && m_oldVisibility) {
            // send
            m_doUpdate = false;
            m_oldTime = cover->frameTime();

            auto &currentNormal = firstModuleInteractor().currentNormal();
            auto &currentNormal2 = secondModuleInteractor().currentNormal();
            auto &currentPosition1 = firstModuleInteractor().currentPosition();
            auto &currentPosition2 = secondModuleInteractor().currentPosition();
            currentNormal.normalize();
            currentNormal2.normalize();
            // fprintf(stderr, "%s %f %f %f    %f %f %f\n", m_moduleName.c_str(), currentPosition1[0], currentPosition1[1], currentPosition1[2], currentNormal[0], currentNormal[1], currentNormal[2]);

            if (m_interactor) {
                if (strncmp(m_interactor->getPluginName(), "Tracer", 6) == 0) {
                    m_interactor->setVectorParam("startpoint1", currentPosition1[0], currentPosition1[1], currentPosition1[2]);
                    m_interactor->setVectorParam("startpoint2", currentPosition2[0], currentPosition2[1], currentPosition2[2]);
                    m_interactor->executeModule();
                }
            }
            // } else if (!m_feedbackInfo.empty()) {
            //     handleCOVISEFeedback(currentNormal, currentNormal2, currentPosition1, currentPosition2);
            // }
        }
    }
}

void VariantAR::tabletEvent(coTUIElement *item) {
    if (item == ui->getTUIARCombobox()) {
        auto cb = dynamic_cast<coTUIComboBox *>(item);
        auto &marker = m_traceModule->getMarker();
        auto select = cb->getSelectedText();
        if (select != "None") {
            marker = std::make_shared<ARToolKitMarker>(select.c_str());
            // m_traceModule->update();
        }
    }
    Variant::tabletEvent(item);
}