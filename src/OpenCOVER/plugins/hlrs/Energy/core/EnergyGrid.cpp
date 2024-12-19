#include "EnergyGrid.h"

#include <utils/color.h>
#include <utils/osgUtils.h>

#include <cassert>
#include <osg/Shape>
#include <osg/ref_ptr>
#include <osgText/Text>
#include <sstream>
#include <variant>

#include "TxtInfoboard.h"

namespace core {

EnergyGrid::InfoboardSensor::InfoboardSensor(
    osg::ref_ptr<osg::Group> parent,
    std::unique_ptr<interface::IInfoboard<std::string>> &&infoboard,
    const std::string &content)
    : coPickSensor(parent), m_infoBoard(std::move(infoboard)) {
  m_infoBoard->initInfoboard();
  m_infoBoard->initDrawable();
  m_infoBoard->updateInfo(content);
  parent->addChild(m_infoBoard->getDrawable());
}

int EnergyGrid::InfoboardSensor::hit(vrui::vruiHit *hit) {
  if (!interaction) return 0;

  // click on object and hold to show info
  if (interaction->wasStarted() && !active) {
    m_infoBoard->showInfo();
    active = true;
  }
  // release to hide info if your not out of the object
  // otherwise the billboard will be shown until you click again
  if (interaction->wasStopped() && active) {
    m_infoBoard->hideInfo();
    active = false;
  }
  return coPickSensor::hit(hit);
}

EnergyGrid::EnergyGrid(const std::string &name, const grid::Points &points,
                       const grid::Indices &indices, osg::ref_ptr<osg::Group> parent,
                       const float &connectionRadius,
                       const grid::DataList &additionalConnectionData)
    : m_name(name), m_points(points), m_parent(parent) {
  if (m_parent == nullptr) {
    m_parent = new osg::Group;
    m_parent->setName(name);
  }
  initConnections(indices, connectionRadius, additionalConnectionData);
}

void EnergyGrid::initConnections(const grid::Indices &indices, const float &radius,
                                 const grid::DataList &additionalConnectionData) {
  assert(indices.size() == m_points.size());

  bool hasAdditionalData = !additionalConnectionData.empty();

  for (auto i = 0; i < indices.size(); ++i) {
    for (auto j = 0; j < indices[i].size(); ++j) {
      std::unique_ptr<grid::ConnectionData<grid::Point>> data;
      if (hasAdditionalData) {
        auto &additionalData = additionalConnectionData[i + j];
        std::string name = "connection";
        if (additionalData.find("name") != additionalData.end()) {
          name = std::get<std::string>(additionalData.at("name"));
        }
        data = std::make_unique<grid::ConnectionData<grid::Point>>(
            name, *m_points[i], *m_points[indices[i][j]], radius, nullptr,
            additionalConnectionData[i]);
      } else {
        data = std::make_unique<grid::ConnectionData<grid::Point>>(
            "connection", *m_points[i], *m_points[indices[i][j]], radius);
      }
      m_connections.push_back(new grid::DirectedConnection(*data));
    }
  }
}

void EnergyGrid::initDrawablePoints() {
  osg::ref_ptr<osg::Group> points = new osg::Group;
  points->setName("Points");
  for (auto &point : m_points) {
    m_drawables.push_back(point);
    points->addChild(point);
  }
  m_parent->addChild(points);
}

void EnergyGrid::initDrawableConnections() {
  osg::ref_ptr<osg::Group> connections = new osg::Group;
  connections->setName("Connections");

  // TODO: calculate height and width of text box
  TxtBoxAttributes attributes(osg::Vec3(0, 0, 0), "Energy Grid",
                              "DroidSans-Bold.ttf", 50, 50, 2.0f, 0.1, 2);

  auto get_string = [&](const auto &data) {
    std::stringstream ss;
    ss << data << "\n\n";
    return ss.str();
  };

  for (auto &connection : m_connections) {
    m_drawables.push_back(connection);
    connections->addChild(connection);

    std::string toPrint = "";
    for (const auto &[name, data] : connection->getAdditionalData()) {
      toPrint += " > " + name + ": " + std::visit(get_string, data);
    }
    auto center = connection->getCenter();
    center.z() += 30;
    auto name = connection->getName();
    attributes.position = center;
    attributes.title = name;
    TxtInfoboard infoboard(attributes);
    m_infoboards.push_back(std::make_unique<InfoboardSensor>(
        connection, std::make_unique<TxtInfoboard>(infoboard), toPrint));
  }

  m_parent->addChild(connections);
}

void EnergyGrid::initDrawables() {
  initDrawablePoints();
  initDrawableConnections();
}

void EnergyGrid::updateColor(const osg::Vec4 &color) {
  for (auto &connection : m_connections) {
    utils::color::overrideGeodeColor(connection->getGeode(), color);
  }
  for (auto &point : m_points) {
    utils::color::overrideGeodeColor(point->getGeode(), color);
  }
}

void EnergyGrid::updateDrawables() {
  for (auto &infoboard : m_infoboards) {
    infoboard->updateDrawable();
  }
}

}  // namespace core
