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
