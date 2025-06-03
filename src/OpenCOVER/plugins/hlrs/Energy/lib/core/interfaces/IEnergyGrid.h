#ifndef _CORE_INTERFACES_IENERGYGRID_H
#define _CORE_INTERFACES_IENERGYGRID_H

#include "IColorable.h"
#include "IDrawables.h"
#include "ITimedependable.h"
#include <PluginUtil/colors/coColorMap.h>
#include "../simulation/simulation.h"
namespace core::interface {
class IEnergyGrid : public IDrawables, public IColorable, public ITimedependable {
 public:
  virtual ~IEnergyGrid() = default;
  virtual void update() = 0;
  virtual void setColorMap(const opencover::ColorMap &colorMap) = 0;
  virtual void setData(const core::simulation::Simulation& sim, const std::string & species) = 0;
};
}  // namespace core::interface

#endif
