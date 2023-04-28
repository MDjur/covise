/* This file is part of COVISE.

   You can use it under the terms of the GNU Lesser General Public License
   version 2.1 or later, see lgpl-2.1.txt.

 * License: LGPL 2+ */

#ifndef VR_COLLABORATION_H
#define VR_COLLABORATION_H

/*! \file
 \brief  handle collaboration menu

 \author Uwe Woessner <woessner@hlrs.de>
 \author (C) 2001
         Computer Centre University of Stuttgart,
         Allmandring 30,
         D-70550 Stuttgart,
         Germany

 \date
 */

#include <util/coExport.h>
#include <util/common.h>
#include <set>
#include <osg/Matrix>
#include <vrb/client/SharedState.h>
#include "ui/Owner.h"
#include "MatrixSerializer.h"

namespace osg
{
class Group;
}

namespace opencover
{

namespace ui
{
class Group;
class Menu;
class Button;
class Slider;
class SelectionList;
class Action;
}


class COVEREXPORT coVRCollaboration: public ui::Owner
{
    static coVRCollaboration *s_instance;
    coVRCollaboration();

public:
    enum SyncMode
    {
        LooseCoupling,
        MasterSlaveCoupling,
        TightCoupling
    };

    void updateUi();

private:
    int readConfigFile();
    void initCollMenu();
    void setSyncInterval();
    std::set<int> m_sessions;
    bool syncXform = false;
	bool wasLo = false;
    float syncInterval;
    bool oldMasterStatus = true;
    float oldSyncInterval = -1;
    bool oldAvatarVisibility = true;
    vrb::SharedState<int> syncMode; ///0: LooseCoupling, 1: MasterSlaveCoupling, 2 TightCoupling
    vrb::SharedState<osg::Matrix> avatarPosition;
    double couplingModeChangedTime = 0;
    bool updated = false;

public:
    virtual ~coVRCollaboration();
    void config();
    void showCollaborative(bool visible);
    void showAvatars(bool visible);
    static coVRCollaboration *instance();
    bool showAvatar;
    vrb::SharedState<float> scaleFactor;
    float getSyncInterval();
    // returns collaboration mode
    SyncMode getCouplingMode() const;

    void setSyncMode(const char *mode); // set one of "LOOSE", "MS", "TIGHT"

    void updateSharedStates(bool force = false);
    // returns true if this COVER ist master in a collaborative session
    bool isMaster();
    // Collaborative menu:
    bool m_visible = false;
    ui::Menu *m_collaborativeMenu = nullptr;
    ui::Group *m_partnerGroup = nullptr;
    ui::Button *m_showAvatar = nullptr;
    ui::Button *m_master = nullptr;
    ui::Action *m_returnToMaster = nullptr;
    ui::Slider *m_syncInterval = nullptr;
    ui::SelectionList *m_collaborationMode = nullptr;
    ui::Menu *menu() const;
    ui::Group *partnerGroup() const;

    bool updateCollaborativeMenu();
    void syncModeChanged(int mode);
    void init();

    bool update();
	//sync transform of viewer with partners
    void SyncXform();
	//sync scale of world with partner
    void UnSyncXform();
    void sessionChanged(bool isPrivate);
    void remoteTransform(osg::Matrix &mat);
    void remoteScale(float d);
};
}
#endif
