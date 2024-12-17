#include "EnergyGrid.h"

#include <utils/color.h>
#include <utils/osgUtils.h>

#include <osg/Shape>
#include <osg/ref_ptr>

namespace core {

EnergyGrid::EnergyGrid(const std::string &name, const grid::Points &points,
                       const grid::Indices &cl, osg::ref_ptr<osg::Group> parent,
                       const float &connectionRadius)
    : m_name(name), m_points(points), m_parent(parent) {
  if (m_parent == nullptr) {
    m_parent = new osg::Group;
    m_parent->setName(name);
  }

  for (auto i = 0; i < cl.size(); ++i) {
    for (auto j = 0; j < cl[i].size(); ++j) {
      grid::ConnectionData<grid::Point> data("connection", *m_points[i],
                                             *m_points[cl[i][j]], connectionRadius,
                                             nullptr);
      m_connections.push_back(new grid::Connection(data));
    }
  }
}

void EnergyGrid::initDrawables() {
  for (auto &point : m_points) {
    m_drawables.push_back(point);
    m_parent->addChild(point);
  }

  for (auto &connection : m_connections) {
    m_drawables.push_back(connection);
    m_parent->addChild(connection);
  }
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
