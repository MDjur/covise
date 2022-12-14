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
// class coVRLabel;
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

        struct ModuleInteractorPoint {
            friend class TraceModule;

            typedef tracemodule::Axis Axis;
            typedef std::array<float, Axis::NumAxis> AxisFields;

           public:
            ModuleInteractorPoint() = delete;
            ModuleInteractorPoint(float x, float y, float z) : m_coord{x, y, z} {};
            const AxisFields &Coord() { return m_coord; }
            void setAxisValue(float val, Axis axis) {
                m_coord[axis] = val;
            }

           private:
            AxisFields m_coord;
        };

        class ModuleInteractor {
            friend class TraceModule;

           public:
            ModuleInteractor() = delete;
            ModuleInteractor(std::shared_ptr<ModuleInteractorPoint> modInterPoint, const osg::Vec3 &startPointOffset, const osg::Vec3 &startNormal, const osg::Vec3 &lastPos, const osg::Vec3 &curPos, const osg::Vec3 &curNormal)
                : m_interactorPoint(modInterPoint),
                  m_startpointOffset(startPointOffset),
                  m_startNormal(startNormal),
                  m_lastPosition(lastPos),
                  m_currentPosition(curPos),
                  m_currentNormal(curNormal) {
                if (!modInterPoint)
                    throw std::exception("Nullptr for ModuleInteractorPoint");
            }

            void resetLastPos() { m_lastPosition = m_currentPosition; }
            void updateCurPos(const osg::Matrix &localMarkerCoords) {
                m_currentPosition = localMarkerCoords.preMult(m_startpointOffset);
            }
            void updateNormal(const osg::Matrix &localMarkerCoords) {
                m_currentNormal = osg::Matrix::transform3x3(localMarkerCoords, m_startNormal);
            }
            osg::Vec3 getDiff() { return m_lastPosition - m_currentPosition; }
            osg::Vec3 &currentNormal() { return m_currentNormal; }
            osg::Vec3 &currentPosition() { return m_currentPosition; }
            std::shared_ptr<ModuleInteractorPoint> &interactorPoint() { return m_interactorPoint; }

           private:
            osg::Vec3 m_startpointOffset;
            osg::Vec3 m_startNormal;
            osg::Vec3 m_lastPosition;
            osg::Vec3 m_currentPosition;
            osg::Vec3 m_currentNormal;
            std::shared_ptr<ModuleInteractorPoint> m_interactorPoint = nullptr;
        };

       public:
        TraceModule() = delete;
        TraceModule(const std::string &name, std::shared_ptr<coInteractor> inter);
        TraceModule(TraceModule &&) = delete;
        TraceModule(const TraceModule &) = delete;
        TraceModule &operator=(TraceModule &&) = delete;
        TraceModule &operator=(const TraceModule &) = delete;

        void update();
        std::shared_ptr<ARToolKitMarker> &getMarker() { return m_marker; }

       private:
        ModuleInteractor &firstModuleInteractor() { return m_ModInteractors[0]; }
        ModuleInteractor &secondModuleInteractor() { return m_ModInteractors[1]; }
        bool calcPositionChanged();
        void resetInteractorsLastPos() {
            std::for_each(m_ModInteractors.begin(), m_ModInteractors.end(), [](auto &inter) { inter.resetLastPos(); });
        }
        void handleCOVISEFeedback(const osg::Vec3 &currentNormal, const osg::Vec3 &currentNormal2, const osg::Vec3 &currentPosition1, const osg::Vec3 &currentPosition2);

        bool m_enabled;
        bool m_positionChanged;
        bool m_oldVisibility;
        bool m_firstUpdate;
        bool m_doUpdate;
        float m_positionThreshold;
        float m_oldTime;
        std::string m_moduleName;

        std::array<ModuleInteractor, 2> m_ModInteractors;
        std::shared_ptr<ARToolKitMarker> m_marker = nullptr;
        std::shared_ptr<coInteractor> m_interactor = nullptr;
    };

    typedef TraceModule::ModuleInteractor ModuleInteractor;
    typedef TraceModule::ModuleInteractorPoint ModuleInteractorPoint;

   public:
    VariantAR(coInteractor *interactor, const Variant &variant);
    ~VariantAR() {
        resetCombobox();
    };

    std::unique_ptr<TraceModule> &traceModule() { return m_traceModule; }
    void resetCombobox() {
        auto comboBox = ui->getTUIARCombobox();
        comboBox->clear();
        comboBox->addEntry("None");
    }

   private:
    std::unique_ptr<TraceModule> m_traceModule = nullptr;
};
#endif