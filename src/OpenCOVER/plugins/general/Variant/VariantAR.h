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
            ModuleInteractorPoint(float x, float y, float z, int plugID);

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
            ModuleInteractor(std::shared_ptr<ModuleInteractorPoint> modInterPoint, int plugID, const osg::Vec3 &startPointOffset, const osg::Vec3 &startNormal, const osg::Vec3 &lastPos, const osg::Vec3 &curPos, const osg::Vec3 &curNormal)
                : InteractorPoint(modInterPoint),
                  PluginID(plugID),
                  StartpointOffset(startPointOffset),
                  StartNormal(startNormal),
                  LastPosition(lastPos),
                  CurrentPosition(curPos),
                  CurrentNormal(curNormal) {
                if (modInterPoint)
                    throw std::exception("Nullptr for ModuleInteractorPoint");
            }

            void resetLastPos() { LastPosition = CurrentPosition; }
            void updateCurPos(const osg::Matrix &localMarkerCoords) { CurrentPosition = localMarkerCoords.preMult(StartpointOffset); }
            void updateNormal(const osg::Matrix &localMarkerCoords) { CurrentNormal = osg::Matrix::transform3x3(localMarkerCoords, StartNormal); }
            void interactorEvent(coTUIElement *tuiItem);
            int pluginID() { return PluginID; }
            osg::Vec3 getDiff() { return LastPosition - CurrentPosition; }
            osg::Vec3 &currentNormal() { return CurrentNormal; }
            osg::Vec3 &currentPosition() { return CurrentPosition; }
            std::shared_ptr<ModuleInteractorPoint> &interactorPoint() { return InteractorPoint; }

           private:
            int PluginID;
            osg::Vec3 StartpointOffset;
            osg::Vec3 StartNormal;
            osg::Vec3 LastPosition;
            osg::Vec3 CurrentPosition;
            osg::Vec3 CurrentNormal;
            std::shared_ptr<ModuleInteractorPoint> InteractorPoint;
        };

       public:
        TraceModule(int ID, const std::string &name, int instanceID, const std::string &fbInfo, std::shared_ptr<coTUITab> plugin, std::shared_ptr<coInteractor> inter);

        virtual void tabletEvent(coTUIElement *tUIItem);
        virtual void tabletPressEvent(coTUIElement *tUIItem);

        bool calcPositionChanged();
        void menuEvent(coMenuItem *);
        void update();
        void resetInteractorsLastPos() {
            std::for_each(m_ModInteractors.begin(), m_ModInteractors.end(), [](auto &inter) { inter.resetLastPos(); });
        }
        // template <typename... Args>
        // void for_each_modinteractor(std::function<void(Args...)> func, Args... args) { std::for_each(m_ModInteractors.begin(), m_ModInteractors.end(), func(args...)); }

       private:
        ModuleInteractor &firstModuleInteractor() { return m_ModInteractors[0]; }
        ModuleInteractor &secondModuleInteractor() { return m_ModInteractors[1]; }

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
        std::shared_ptr<ARToolKitMarker> m_marker;
        std::shared_ptr<coTUITab> m_tab;
        std::shared_ptr<coTUILabel> m_TracerModuleLabel;
        std::shared_ptr<coTUIToggleButton> m_updateOnVisibilityChange;
        std::shared_ptr<coTUIEditFloatField> m_updateInterval;
        std::shared_ptr<coTUIButton> m_updateNow;
        std::shared_ptr<coInteractor> m_interactor;
        std::shared_ptr<coSubMenuItem> m_arMenuEntry;
        std::shared_ptr<coRowMenu> m_moduleMenu;
        std::shared_ptr<coCheckboxMenuItem> m_enabledToggle;
    };

    typedef TraceModule::ModuleInteractor ModuleInteractor;
    typedef TraceModule::ModuleInteractorPoint ModuleInteractorPoint;

   public:
    VariantAR(std::string varName, osg::Node *node, osg::Node::ParentList parents,
              ui::Menu *VariantMenu, coTUITab *VariantPluginTab, int numVar,
              QDomDocument *qdomDoc, QDomElement *qdomElem, coVRBoxOfInterest *boxOfInterest, bool defaultState);
    ~VariantAR(){};

   private:
    int ID;
    std::shared_ptr<ARToolKitMarker> m_timestepMarker;
    covise::DLinkList<TraceModule *> m_modules;
};
#endif