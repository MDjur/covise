#pragma once
#include <PluginUtil/colors/coColorMap.h>
#include <lib/core/interfaces/IEnergyGrid.h>
#include <lib/core/interfaces/IInfoboard.h>
#include <lib/core/simulation/simulation.h>

#include <memory>
#include <osg/Geode>
#include <osg/Group>
#include <osg/Shape>
#include <osg/ShapeDrawable>
#include <osg/Vec3>
#include <osg/ref_ptr>
#include <string>

#include "OsgTxtInfoboard.h"
#include "PluginUtil/coSensor.h"
#include "grid.h"
#include "app/typedefs.h"

using namespace core;

enum class EnergyGridConnectionType { Index, Line };

/**
 * @struct EnergyGridConfig
 * @brief A struct representing the data needed to create an energy grid.
 *
 * This struct is used to store the data needed to create an energy grid.
 *
 * @param name The name of the energy grid.
 * @param points The points that define the grid.
 * @param indices The indices that define the connections between points.
 * @param parent The parent OSG group node (default is nullptr).
 * @param connectionRadius The radius for connections (default is 1.0f).
 * @param additionalConData Additional connection data (default is an empty list).
 */
struct EnergyGridConfig {
  EnergyGridConfig(const std::string &gridName, const grid::Points &gridPoints,
                   const grid::Indices &gridIndices,
                   const grid::PointsMap &gridPointsMap = {},
                   osg::ref_ptr<osg::MatrixTransform> gridParent = nullptr,
                   const float &gridConnectionRadius = 1.0f,
                   const grid::ConnectionDataList &extraConnectionData =
                       grid::ConnectionDataList(),
                   const OsgTxtBoxAttributes &gridInfoAttributes =
                       OsgTxtBoxAttributes(osg::Vec3(0, 0, 0), "EnergyGridText",
                                           "DejaVuSans-Bold.ttf", 50, 50, 2.0f, 0.1,
                                           2),
                   const EnergyGridConnectionType &gridConnectionType =
                       EnergyGridConnectionType::Index,
                   const grid::Lines &gridLines = grid::Lines())
      : name(gridName),
        points(gridPoints),
        indices(gridIndices),
        pointsMap(gridPointsMap),
        parent(gridParent),
        connectionRadius(gridConnectionRadius),
        additionalConnectionData(extraConnectionData),
        infoboardAttributes(gridInfoAttributes),
        connectionType(gridConnectionType),
        lines(gridLines) {}

  // mandatory
  std::string name;
  grid::Points points;
  grid::Indices indices;
  // optional
  grid::PointsMap pointsMap;  // for faster access
  osg::ref_ptr<osg::MatrixTransform> parent;
  float connectionRadius;
  grid::ConnectionDataList additionalConnectionData;
  OsgTxtBoxAttributes infoboardAttributes;
  EnergyGridConnectionType connectionType;
  grid::Lines lines;

  bool valid() const {
    bool isMandatoryValid = !name.empty() ||
                            ((points.empty() || pointsMap.empty()) &&
                             (points.empty() && pointsMap.empty())) ||
                            !indices.empty();
    return connectionType == EnergyGridConnectionType::Index ? isMandatoryValid
                                                             : !lines.empty();
  }
};

/**
 * @class InfoboardSensor
 * @brief A sensor class that interacts with an infoboard and responds to pick
 * events.
 *
 * Inherits from coPickSensor and manages an infoboard displaying string content.
 * Provides methods to activate the sensor, update its state, and refresh the
 * infoboard's drawable.
 *
 * @constructor
 * @param parent The parent osg::Group node to attach the sensor to.
 * @param infoboard A unique pointer to an IInfoboard instance for displaying
 * information.
 * @param content Optional initial content to display on the infoboard.
 *
 * @method updateDrawable Updates the drawable representation of the infoboard.
 * @method activate Activates the sensor (overrides coPickSensor::activate).
 * @method update Updates the sensor state (overrides coPickSensor::update).
 *
 * @private
 * @var m_enabled Indicates whether the sensor is enabled.
 * @var m_infoBoard Unique pointer to the managed infoboard instance.
 */
class InfoboardSensor : public coPickSensor {

 public:
  InfoboardSensor(osg::ref_ptr<osg::Group> parent,
                  std::unique_ptr<OsgInfoboard> &&infoboard,
                  const std::string &content = "");

  void updateDrawable() { m_infoBoard->updateDrawable(); }
  void activate() override;
  void update() override;

 private:
  bool m_enabled = false;
  std::unique_ptr<OsgInfoboard> m_infoBoard;
};

/**
 * @class EnergyGrid
 * @brief A class representing an energy grid, inheriting from interface::EnergyGrid.
 *
 * This class is responsible for managing and visualizing an energy grid using
 * OpenSceneGraph.
 *
 */
class EnergyGrid : public interface::IEnergyGrid {
 public:
  EnergyGrid(const EnergyGridConfig &data, bool ignoreOverlap = true);
  void initDrawables() override;
  void update() override {
    for (auto &infoboard : m_infoboards) infoboard->update();
  }
  void updateColor(const osg::Vec4 &color) override;
  void updateDrawables() override;
  void updateTime(int timestep) override;

  void setColorMap(const opencover::ColorMap &colorMap,
                   const opencover::ColorMap &vm_pu_Colormap);
  void setData(const core::simulation::Simulation &sim, const std::string &species,
               bool interpolate = false);
  osg::ref_ptr<grid::DirectedConnection> getConnectionByName(
      const std::string &name);
  osg::ref_ptr<grid::DirectedConnection> getConnectionByIdx(int idx) {
    if (idx < 0 || idx >= m_connections.size()) return nullptr;
    return m_connections[idx];
  }
  const osg::ref_ptr<grid::Point> getPointByName(const std::string &name) const;
  osg::ref_ptr<grid::Point> getPointByIdx(int idx) {
    if (idx < 0 || idx >= m_config.points.size()) return nullptr;
    return m_config.points[idx];
  }

 private:
  template <typename T>
  void initDrawableGridObject(osg::ref_ptr<osg::Group> parent, const T &gridObj) {
    m_drawables.push_back(gridObj);
    parent->addChild(gridObj);
    std::string toPrint(createDataString(gridObj->getAdditionalData()));
    auto center = gridObj->getCenter();
    center.z() += 30;
    auto name = gridObj->getName();

    m_config.infoboardAttributes.position = center;
    m_config.infoboardAttributes.title = name;
    OsgTxtInfoboard infoboard(m_config.infoboardAttributes);
    m_infoboards.push_back(std::make_unique<InfoboardSensor>(
        gridObj, std::make_unique<OsgTxtInfoboard>(infoboard), toPrint));
  }

  std::string createDataString(const grid::Data &data) const;

  void initConnections();
  void initConnectionsByIndex(
      const grid::Indices &indices, const float &radius,
      const grid::ConnectionDataList &additionalConnectionData);
  void initDrawableConnections();
  void initDrawableLines();
  void initDrawablePoints();
  bool validPointIdx(int idx) { return idx < 0 || idx >= m_config.points.size(); }
  void findCorrectHeightForLine(float radius, osg::ref_ptr<grid::Line> line,
                                grid::Lines &lines);

  EnergyGridConfig m_config;
  grid::Connections m_connections;
  grid::Lines m_lines;
  std::vector<std::unique_ptr<InfoboardSensor>> m_infoboards;
  bool m_ignoreOverlap;
};
