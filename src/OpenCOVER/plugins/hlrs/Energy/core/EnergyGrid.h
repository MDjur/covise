#ifndef _CORE_ENERGYGRIND_H
#define _CORE_ENERGYGRIND_H

#include <memory>
#include <osg/Geode>
#include <osg/Group>
#include <osg/Shape>
#include <osg/ShapeDrawable>
#include <osg/Vec3>
#include <osg/ref_ptr>

#include "PluginUtil/coSensor.h"
#include "grid.h"
#include "interfaces/IEnergyGrid.h"
#include "interfaces/IInfoboard.h"

namespace core {

/**
 * @class EnergyGrid
 * @brief A class representing an energy grid, inheriting from interface::EnergyGrid.
 *
 * This class is responsible for managing and visualizing an energy grid using
 * OpenSceneGraph.
 *
 * @param name The name of the energy grid.
 * @param points The points that define the grid.
 * @param indices The indices that define the connections between points.
 * @param parent The parent OSG group node (default is nullptr).
 * @param connectionRadius The radius for connections (default is 1.0f).
 * @param additionalConData Additional connection data (default is an empty list).
 */
class EnergyGrid : public interface::IEnergyGrid {
 public:
  EnergyGrid(const std::string &name, const grid::Points &points,
             const grid::Indices &indices, osg::ref_ptr<osg::Group> parent = nullptr,
             const float &connectionRadius = 1.0f,
             const grid::DataList &additionalConnectionData = grid::DataList());
  void initDrawables() override;
  void updateColor(const osg::Vec4 &color) override;
  void updateDrawables() override;
  const auto &getName() const { return m_name; }
  osg::ref_ptr<osg::Group> getParent() { return m_parent; }
  void initDrawableConnections();
  void initDrawablePoints();

 private:
  class InfoboardSensor : public coPickSensor {
   public:
    InfoboardSensor(osg::ref_ptr<osg::Group> parent,
                    std::unique_ptr<interface::IInfoboard<std::string>> &&infoboard,
                    const std::string &content = "");

    void updateDrawable() { m_infoBoard->updateDrawable(); }
    int hit(vrui::vruiHit *hit) override;

    void update() override {
      m_infoBoard->updateDrawable();
      coPickSensor::update();
    }

   private:
    std::unique_ptr<interface::IInfoboard<std::string>> m_infoBoard;
  };

  void initConnections(const grid::Indices &indices, const float &radius,
                       const grid::DataList &additionalConnectionData);
  std::string m_name;
  grid::Points m_points;
  grid::Connections m_connections;
  osg::ref_ptr<osg::Group> m_parent;
  std::vector<std::unique_ptr<InfoboardSensor>> m_infoboards;
};
}  // namespace core
#endif
