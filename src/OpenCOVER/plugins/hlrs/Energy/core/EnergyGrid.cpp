#include "EnergyGrid.h"

#include <utils/color.h>
#include <utils/osgUtils.h>

#include <cassert>
#include <iostream>
#include <osg/Shape>
#include <osg/ref_ptr>
#include <osgText/Text>
#include <sstream>
#include <variant>
namespace core {

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

void EnergyGrid::initDrawables() {
  osg::ref_ptr<osg::Group> points = new osg::Group;
  points->setName("Points");
  osg::ref_ptr<osg::Group> connections = new osg::Group;
  connections->setName("Connections");

  for (auto &point : m_points) {
    m_drawables.push_back(point);
    points->addChild(point);
  }

  for (auto &connection : m_connections) {
    m_drawables.push_back(connection);
    connections->addChild(connection);

    auto print_variant = [&](const auto &data) {
      std::stringstream ss;
      ss << data << std::endl;
      return ss.str();
    };
    std::string toPrint = "";
    for (const auto &[name, data] : connection->getAdditionalData()) {
      toPrint += name + ": " + std::visit(print_variant, data);
    }
    auto center = connection->getCenter();
    auto name = connection->getName();
    auto text =
        utils::osgUtils::createTextBox(name, center, 2, "DroidSans-Bold.ttf", 50, 5);
    text->setText(toPrint);
    osg::ref_ptr<osg::Geode> geode = new osg::Geode;
    geode->addDrawable(text);
    geode->setName("Textbox");
    connection->addChild(geode);
  }

  m_parent->addChild(points);
  m_parent->addChild(connections);
}

void EnergyGrid::updateColor(const osg::Vec4 &color) {
  for (auto &connection : m_connections) {
    utils::color::overrideGeodeColor(connection->getGeode(), color);
  }
  for (auto &point : m_points) {
    utils::color::overrideGeodeColor(point->getGeode(), color);
  }
}

void EnergyGrid::updateDrawables() {}

}  // namespace core
