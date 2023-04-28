/* This file is part of COVISE.

   You can use it under the terms of the GNU Lesser General Public License
   version 2.1 or later, see lgpl-2.1.txt.

 * License: LGPL 2+ */

 /**************************************************************************
 ** ODD: OpenDRIVE Designer
 **   Frank Naegele (c) 2010
 **   <mail@f-naegele.de>
 **   11.03.2010
 **
 **************************************************************************/

#ifndef TRACKEDITOR_HPP
#define TRACKEDITOR_HPP

#include "projecteditor.hpp"
#include "src/data/roadsystem/odrID.hpp"
#include "src/data/prototypemanager.hpp"

#include <QMultiMap>
#include <QPointF>
#include <QGraphicsItem>

class ProjectData;
class TopviewGraph;

class RoadSystem;
class RSystemElementRoad;
class TrackComponent;

class TrackMoveHandle;
class TrackRotateHandle;
class TrackAddHandle;

class RoadMoveHandle;
class RoadRotateHandle;

class QGraphicsLineItem;
class QGraphicsEllipseItem;
class CircularRotateHandle;

class TrackRoadSystemItem;

class ToolParameter;


// TODO
class SectionHandle;

class TrackEditor : public ProjectEditor
{
    Q_OBJECT

        //################//
        // STATIC         //
        //################//

public:

    enum CacheMode
    {
        NoCache,
        DeviceCache
    };

private:
    enum TrackEditorState
    {
        STE_NONE,
        STE_NEW_PRESSED,
        STE_HANDLE_PRESSED,
        STE_ROADSYSTEM_ADD
    };

    enum TransformType
    {
        TT_MOVE = 1,
        TT_ROTATE = 2
    };

    struct TrackMoveProperties
    {
        TrackComponent *highSlot;
        TrackComponent *lowSlot;
        QPointF dPos;
        double heading;
        short int transform;
    };

    enum GeometryPrimitive
    {
        LINE,
        ARC_SPIRAL,
        POLYNOMIAL
    };

    //################//
    // FUNCTIONS      //
    //################//

public:
    explicit TrackEditor(ProjectWidget *projectWidget, ProjectData *projectData, TopviewGraph *topviewGraph);
    virtual ~TrackEditor();

    // Tool, Mouse & Key //
    //
    virtual void toolAction(ToolAction *toolAction);
    virtual void mouseAction(MouseAction *mouseAction);
    virtual void keyAction(KeyAction *keyAction);

    bool translateTrack(TrackComponent *track, const QPointF &pressPos, const QPointF &mousePos);

    // MoveHandles //
    //
    void registerTrackMoveHandle(TrackMoveHandle *handle);
    int unregisterTrackMoveHandle(TrackMoveHandle *handle);
    bool translateTrackMoveHandles(const QPointF &pressPos, const QPointF &mousePos);
    bool translateTrackComponents(const QPointF &pressPos, const QPointF &mousePos);
    bool validate(TrackMoveProperties *props);
    void translate(TrackMoveProperties *props);

    // RoadMoveHandles //
    //
    void registerRoadMoveHandle(RoadMoveHandle *handle);
    int unregisterRoadMoveHandle(RoadMoveHandle *handle);
    bool translateRoadMoveHandles(const QPointF &pressPos, const QPointF &mousePos);

    // RoadRotateHandles //
    //
    void registerRoadRotateHandle(RoadRotateHandle *handle);
    int unregisterRoadRotateHandle(RoadRotateHandle *handle);
    bool rotateRoadRotateHandles(const QPointF &pivotPoint, double angleDegrees);
    void setCacheMode(RSystemElementRoad *road, CacheMode cache);
    void setChildCacheMode(QGraphicsItem *child, QGraphicsItem::CacheMode mode);

    // AddHandles //
    //
    void registerTrackAddHandle(TrackAddHandle *handle);
    int unregisterTrackAddHandle(TrackAddHandle *handle);

    // Find selected roads //
    //
    QMap<QGraphicsItem *, RSystemElementRoad *> getSelectedRoads(int count);

    // Register Roads //
    bool registerRoad(QGraphicsItem *trackItem, RSystemElementRoad *road);
    bool deregisterRoad(QGraphicsItem *trackItem, RSystemElementRoad *road);


    // Fetch Road Prototypes //
    //
    void fetchRoadPrototypes(RSystemElementRoad *road);


#if 0
    // RotateHandles //
    //
    void      registerTrackRotateHandle(TrackRotateHandle *handle);
    int      unregisterTrackRotateHandle(TrackRotateHandle *handle);
    double     rotateTrackRotateHandles(double dHeading, double globalHeading);
#endif

    // Section Handle //
    //
    SectionHandle *getSectionHandle() const;

protected:
    virtual void init();
    virtual void kill();

    void clearToolObjectSelection(QList<ToolParameter *> *parameterVector = NULL, RSystemElementRoad **firstRoad = NULL, RSystemElementRoad **secondRoad = NULL);
    void getToolObjectSelection(QList<ToolParameter *> *parameterList, RSystemElementRoad **firstRoad, RSystemElementRoad **secondRoad);

private:
    TrackEditor(); /* not allowed */
    TrackEditor(const TrackEditor &); /* not allowed */
    TrackEditor &operator=(const TrackEditor &); /* not allowed */

    ToolParameter *addToolParameter(const PrototypeManager::PrototypeType &type, const ODD::ToolId &toolId, const QString &labelText);
    void appendToolParameter(const PrototypeManager::PrototypeType &type, const ODD::ToolId &toolId, const ODD::ToolId &paramToolId, RSystemElementRoad *road);

    //################//
    // SLOTS          //
    //################//

public slots:
    // Parameter Settings //
    //
    virtual void apply();
    virtual void reject();
    virtual void reset();

    //################//
    // PROPERTIES     //
    //################//

private:
    // RoadSystem //
    //
    TrackRoadSystemItem *trackRoadSystemItem_;

    // Prototype Manager //
    //
    PrototypeManager *prototypeManager_;

    // Selected roads //
    //
    QList<RSystemElementRoad *> selectedRoads_;

    // MouseAction //
    //
    QPointF pressPoint_;
    QPointF firstPressPoint_;
    QVector2D tangent_;

    // New Road Tool //
    //
    QGraphicsLineItem *newRoadLineItem_;

    // New Polynomial Tool //
    //
    TrackMoveHandle *newRoadPolyItem_;
    RSystemElementRoad *newPolyRoad_;


    // New Circle Tool //
    //
    QGraphicsEllipseItem *newRoadCircleItem_;

    // Add RoadSystem Tool //
    //
    CircularRotateHandle *addRoadSystemHandle_;

    // Move/Add/Rotate Tool //
    //
    QMultiMap<int, TrackMoveHandle *> selectedTrackMoveHandles_;
    QMultiMap<int, TrackAddHandle *> selectedTrackAddHandles_;
    //QMultiMap<double, TrackRotateHandle *> selectedTrackRotateHandles_;

    QMultiMap<int, RoadMoveHandle *> selectedRoadMoveHandles_;
    QMultiMap<int, RoadRotateHandle *> selectedRoadRotateHandles_;

    // Add Tool //
    //
    RSystemElementRoad *currentRoadPrototype_;
    PrototypeContainer<RoadSystem *> *currentRoadSystemPrototype_;

    // Move Tile Tool //
    //
    odrID currentTile_;

    // State //
    //
    TrackEditor::TrackEditorState state_;

    // TODO
    SectionHandle *sectionHandle_;

    // Prototypes
    //
    QMap<PrototypeManager::PrototypeType, PrototypeContainer<RSystemElementRoad *> *> currentPrototypes_;
    GeometryPrimitive geometryPrimitiveType_;

    // Road_MERGE Road_SNAP
    //
    QGraphicsItem *mergeItem_;
    QGraphicsItem *appendItem_;

    // necessary selected elements to make APPLY visible //
    //
    int applyCount_;
};

#endif // TRACKEDITOR_HPP
