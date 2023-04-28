/* This file is part of COVISE.

   You can use it under the terms of the GNU Lesser General Public License
   version 2.1 or later, see lgpl-2.1.txt.

 * License: LGPL 2+ */

/**************************************************************************
** ODD: OpenDRIVE Designer
**   Frank Naegele (c) 2010
**   <mail@f-naegele.de>
**   05.07.2010
**
**************************************************************************/

#ifndef TRACKROADITEM_HPP
#define TRACKROADITEM_HPP

#include "src/graph/items/roadsystem/roaditem.hpp"

class TrackRoadSystemItem;
class TrackComponentItem;
class RSystemElementRoad;

class TrackEditor;

class TrackRoadItem : public RoadItem
{

    //################//
    // FUNCTIONS      //
    //################//

public:
    explicit TrackRoadItem(TrackRoadSystemItem *parentTrackRoadSystemItem, RSystemElementRoad *road, TrackEditor *trackEditor);
    virtual ~TrackRoadItem();

    // Garbage //
    //
    virtual void notifyDeletion(); // to be implemented by subclasses

    // Handles //
    //
    void rebuildMoveRotateHandles(bool delHandles);
    void rebuildAddHandles(bool delHandles, bool perLane);
    void rebuildRoadMoveRotateHandles(bool delHandles);
    void deleteHandles();

    // Components //
    //
    void setComponentItemsSelectable(bool selectable);

    // Graph //
    //
    virtual TopviewGraph *getTopviewGraph() const;

    // Obsever Pattern //
    //
    virtual void updateObserver();

private:
    TrackRoadItem(); /* not allowed */
    TrackRoadItem(const TrackRoadItem &); /* not allowed */
    TrackRoadItem &operator=(const TrackRoadItem &); /* not allowed */

    void init();
    void rebuildSections(bool fullRebuild = false);

    //################//
    // EVENTS         //
    //################//

public:
    // virtual QVariant itemChange(GraphicsItemChange change, const QVariant & value);

    //################//
    // PROPERTIES     //
    //################//

private:
    // TrackEditor //
    //
    TrackEditor *trackEditor_;

    // TrackRoadSystemItem //
    //
    TrackRoadSystemItem *parentTrackRoadSystemItem_;

    // Handles //
    //
    QGraphicsPathItem *handlesItem_;

    // TrackComponentItems //
    //
    QMap<TrackComponent *, TrackComponentItem *> trackComponentItems_;
};

#endif // TRACKROADITEM_HPP
