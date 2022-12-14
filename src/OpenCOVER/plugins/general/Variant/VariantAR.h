/* This file is part of COVISE.

   You can use it under the terms of the GNU Lesser General Public License
   version 2.1 or later, see lgpl-2.1.txt.

 * License: LGPL 2+ */

/****************************************************************************\
 **                                                            (C)2022 HLRS  **
 **                                                                          **
 ** Description: Variant AR interface                                        **
 **                                                                          **
 **                                                                          **
 ** Author: M.Djuric    		                                             **
 **                                                                          **
 ** History:  								                                 **
 ** Dec-22  v1	    				       		                             **
 **                                                                          **
 **                                                                          **
\****************************************************************************/

#ifndef _VARIANTAR_H
#define _VARIANTAR_H

#include <OpenVRUI/coMenu.h>
#include <cover/ARToolKit.h>
#include <cover/coInteractor.h>
#include <cover/coTabletUI.h>
#include <cover/coVRLabel.h>
#include <cover/coVRPlugin.h>
#include <cover/coVRPluginSupport.h>
#include <qdom.h>
#include <util/DLinkList.h>

#include <QtCore>
#include <algorithm>
#include <array>
#include <iostream>
#include <iterator>
#include <memory>
#include <set>

#include "Variant.h"
#include "VariantUI.h"

namespace vrui {
class coRowMenu;
class coSubMenuItem;
class coCheckboxMenuItem;
class coButtonMenuItem;
class coPotiMenuItem;
class coVRLabel;
};  // namespace vrui

namespace tracemodule {
enum Axis {
    X = 0,
    Y = 1,
    Z = 2,
    NumAxis = 3
};
};

using namespace vrui;
using namespace opencover;

class VariantAR : public Variant {
    class TraceModule : public coMenuListener,
                        public coTUIListener {
        friend class VariantAR;

        class ModuleInteractorPoint {
            friend class TraceModule;

            typedef tracemodule::Axis Axis;
            typedef std::array<std::shared_ptr<coTUIEditFloatField>, Axis::NumAxis> AxisFields;

           public:
            ModuleInteractorPoint() = default;
            ModuleInteractorPoint(float x, float y, float z, int tabID);

            std::shared_ptr<coTUIEditFloatField> &X() {
                return m_coord[Axis::X];
            }

            std::shared_ptr<coTUIEditFloatField> &Y() {
                return m_coord[Axis::Y];
            }

            std::shared_ptr<coTUIEditFloatField> &Z() {
                return m_coord[Axis::Z];
            }

            const AxisFields &Coord() {
                return m_coord;
            }

            void setAxisValue(float val, Axis axis) {
                m_coord[axis]->setValue(val);
            }

            void setEventListener(TraceModule *listener) {
                std::for_each(m_coord.begin(), m_coord.end(), [&listener](auto &axis) { axis->setEventListener(listener); });
            }

            void setUIPos(int y) {
                int i = 0;
                std::for_each(m_coord.begin(), m_coord.end(), [&](auto &axis) mutable { axis->setPos(i++, y); });
            }

            void createAxis(std::string name, int pluginID, Axis axis) {
                m_coord[axis].reset();
                m_coord[axis] = std::make_shared<coTUIEditFloatField>(name, pluginID);
            }

           private:
            void setAxis(std::string name, int pluginID) {}
            AxisFields m_coord;
        };

        class ModuleInteractor {
            friend class TraceModule;

           public:
            ModuleInteractor() = default;
            ModuleInteractor(std::shared_ptr<ModuleInteractorPoint> modInterPoint, int tabID, const osg::Vec3 &startPointOffset, const osg::Vec3 &startNormal, const osg::Vec3 &lastPos, const osg::Vec3 &curPos, const osg::Vec3 &curNormal)
                : m_interactorPoint(modInterPoint),
                  m_tabID(tabID),
                  m_startpointOffset(startPointOffset),
                  m_startNormal(startNormal),
                  m_lastPosition(lastPos),
                  m_currentPosition(curPos),
                  m_currentNormal(curNormal) {
                if (!modInterPoint)
                    throw std::exception("Nullptr for ModuleInteractorPoint");
            }

            void resetLastPos() { m_lastPosition = m_currentPosition; }
            void updateCurPos(const osg::Matrix &localMarkerCoords) { m_currentPosition = localMarkerCoords.preMult(m_startpointOffset); }
            void updateNormal(const osg::Matrix &localMarkerCoords) { m_currentNormal = osg::Matrix::transform3x3(localMarkerCoords, m_startNormal); }
            void interactorEvent(coTUIElement *tuiItem);
            int pluginID() { return m_tabID; }
            osg::Vec3 getDiff() { return m_lastPosition - m_currentPosition; }
            osg::Vec3 &currentNormal() { return m_currentNormal; }
            osg::Vec3 &currentPosition() { return m_currentPosition; }
            std::shared_ptr<ModuleInteractorPoint> &interactorPoint() { return m_interactorPoint; }

           private:
            int m_tabID;
            osg::Vec3 m_startpointOffset;
            osg::Vec3 m_startNormal;
            osg::Vec3 m_lastPosition;
            osg::Vec3 m_currentPosition;
            osg::Vec3 m_currentNormal;
            std::shared_ptr<ModuleInteractorPoint> m_interactorPoint = nullptr;
        };

       public:
        TraceModule(int ID, const std::string &name, int instanceID, const std::string &fbInfo, std::shared_ptr<coTUITab> plugin, std::shared_ptr<coInteractor> inter);
        // TraceModule(const std::string &name, std::shared_ptr<coInteractor> inter);

        virtual void tabletEvent(coTUIElement *tUIItem);
        virtual void tabletPressEvent(coTUIElement *tUIItem);

        void menuEvent(coMenuItem *);
        void update();

       private:
        ModuleInteractor &firstModuleInteractor() { return m_ModInteractors[0]; }
        ModuleInteractor &secondModuleInteractor() { return m_ModInteractors[1]; }
        bool calcPositionChanged();
        void resetInteractorsLastPos() {
            std::for_each(m_ModInteractors.begin(), m_ModInteractors.end(), [](auto &inter) { inter.resetLastPos(); });
        }
        void handleCOVISEFeedback(const osg::Vec3 &currentNormal, const osg::Vec3 &currentNormal2, const osg::Vec3 &currentPosition1, const osg::Vec3 &currentPosition2);

        int m_id;
        int m_modInstanceID;
        bool m_enabled;
        bool m_positionChanged;
        bool m_oldVisibility;
        bool m_firstUpdate;
        bool m_doUpdate;
        float m_positionThreshold;
        float m_oldTime;
        std::string m_feedbackInfo;
        std::string m_moduleName;

        std::array<ModuleInteractor, 2> m_ModInteractors;
        std::shared_ptr<ARToolKitMarker> m_marker = nullptr;
        std::shared_ptr<coTUITab> m_tab = nullptr;
        std::shared_ptr<coTUILabel> m_TracerModuleLabel = nullptr;
        std::shared_ptr<coTUIToggleButton> m_updateOnVisibilityChange = nullptr;
        std::shared_ptr<coTUIEditFloatField> m_updateInterval = nullptr;
        std::shared_ptr<coTUIButton> m_updateNow = nullptr;
        std::shared_ptr<coInteractor> m_interactor = nullptr;
        std::shared_ptr<coSubMenuItem> m_arMenuEntry = nullptr;
        std::shared_ptr<coRowMenu> m_moduleMenu = nullptr;
        std::shared_ptr<coCheckboxMenuItem> m_enabledToggle = nullptr;
    };

    typedef TraceModule::ModuleInteractor ModuleInteractor;
    typedef TraceModule::ModuleInteractorPoint ModuleInteractorPoint;
    typedef set<coInteractor *> coInteractorSet;

   public:
    VariantAR(const coInteractorSet &interSet, std::string varName, osg::Node *node, const osg::Node::ParentList &parents,
              ui::Menu *VariantMenu, coTUITab *VariantPluginTab, int numVar,
              QDomDocument *qdomDoc, QDomElement *qdomElem, coVRBoxOfInterest *boxOfInterest, bool defaultState);
    VariantAR(const coInteractorSet &interSet, const Variant &variant);
    ~VariantAR(){};

    const std::unique_ptr<TraceModule> &traceModule() { return m_traceModule; }

   private:
    // void initAR();
    std::unique_ptr<TraceModule> m_traceModule = nullptr;
    coInteractorSet m_interactors;
};
#endif