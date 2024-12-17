#ifndef _CORE_ENERGYGRIND_H
#define _CORE_ENERGYGRIND_H

#include <osg/Geode>
#include <osg/Group>
#include <osg/Shape>
#include <osg/ShapeDrawable>
#include <osg/Vec3>
#include <osg/ref_ptr>

#include "grid.h"
#include "interfaces/IEnergyGrid.h"

namespace core {

/**
 * @class EnergyGrid
 * @brief A class representing an energy grid, inheriting from interface::EnergyGrid.
 *
 * This class is responsible for managing and visualizing an energy grid using OpenSceneGraph.
 *
 * @param name The name of the energy grid.
 * @param points The points that define the grid.
 * @param cl The indices that define the connections between points.
 * @param parent The parent OSG group node (default is nullptr).
 * @param connectionRadius The radius for connections (default is 1.0f).
 */
class EnergyGrid : public interface::EnergyGrid {
 public:
  EnergyGrid(const std::string &name, const grid::Points &points,
             const grid::Indices &cl, osg::ref_ptr<osg::Group> parent = nullptr,
             const float &connectionRadius = 1.0f);
  void initDrawables() override;
  void updateColor(const osg::Vec4 &color) override;
  void updateDrawables() override;
  const auto &getName() const { return m_name; }
  osg::ref_ptr<osg::Group> getParent() { return m_parent; }

 private:
  std::string m_name;
  grid::Points m_points;
  grid::Connections m_connections;
  osg::ref_ptr<osg::Group> m_parent;
};
}  // namespace core
#endif
