#pragma once

#include <lib/core/simulation/heating.h>
#include <lib/core/utils/color.h>

#include <memory>
#include <osg/Vec4>

#include "app/presentation/EnergyGrid.h"
#include "app/ui/simulation/BaseSimulationUI.h"

using namespace core::simulation::heating;

/**
 * @brief UI class for managing and visualizing a HeatingSimulation.
 *
 * This template class extends BaseSimulationUI and provides specialized
 * functionality for updating the UI based on the state of a HeatingSimulation.
 * It handles updating energy grid colors for consumers and producers,
 * retrieving minimum and maximum values for species, and updating timestep colors.
 *
 * @tparam T The parent UI type.
 */
template <typename T>
class HeatingSimulationUI : public BaseSimulationUI<T> {
 public:
  HeatingSimulationUI(std::shared_ptr<HeatingSimulation> sim,
                      std::shared_ptr<T> parent)
      : BaseSimulationUI<T>(sim, parent) {}
  ~HeatingSimulationUI() = default;
  HeatingSimulationUI(const HeatingSimulationUI&) = delete;
  HeatingSimulationUI& operator=(const HeatingSimulationUI&) = delete;

  void updateTime(int timestep) override {
    auto parent = this->m_parent.lock();
    if (!parent) return;
    // TODO: rethink this pls => maybe use a visitor pattern
    std::shared_ptr<EnergyGrid> energyGrid =
        std::dynamic_pointer_cast<EnergyGrid>(parent);
    if (energyGrid) {
      auto heatingSim = this->heatingSimulationPtr();
      if (!heatingSim) return;
      this->updateEnergyGridColors(
          timestep, energyGrid,
          {std::ref(heatingSim->Consumers()), std::ref(heatingSim->Producers())});
    } else {
      std::cerr << "Parent is not an EnergyGrid." << std::endl;
    }
  }

  float min(const std::string& species) override {
    return this->heatingSimulationPtr()->getMin(species);
  }

  float max(const std::string& species) override {
    return this->heatingSimulationPtr()->getMax(species);
  }

  void updateTimestepColors(const opencover::ColorMap& colorMap) override {
    // compute colors
    auto heatingSim = this->heatingSimulationPtr();
    if (!heatingSim) return;
    this->computeColors(colorMap, {std::ref(heatingSim->Consumers()),
                                   std::ref(heatingSim->Producers())});
  }

 private:
  std::shared_ptr<HeatingSimulation> heatingSimulationPtr() {
    return std::dynamic_pointer_cast<HeatingSimulation>(this->m_simulation.lock());
  }
};
