#pragma once

#include <lib/core/simulation/heating.h>
#include <lib/core/utils/color.h>

#include <iostream>
#include <memory>
#include <osg/Vec4>

#include "app/presentation/EnergyGrid.h"
#include "app/ui/simulation/BaseSimulationUI.h"

using namespace core::simulation::heating;

template <typename T>
class HeatingSimulationUI : public BaseSimulationUI<T> {
 public:
  HeatingSimulationUI(std::shared_ptr<HeatingSimulation> sim,
                      std::shared_ptr<T> parent, std::shared_ptr<ColorMap> colorMap)
      : BaseSimulationUI<T>(sim, parent, colorMap) {}
  ~HeatingSimulationUI() = default;
  HeatingSimulationUI(const HeatingSimulationUI &) = delete;
  HeatingSimulationUI &operator=(const HeatingSimulationUI &) = delete;

  void updateTime(int timestep) override {
    auto parent = this->m_parent.lock();
    if (!parent) return;
    // TODO: rethink this pls => maybe use a visitor pattern
    std::shared_ptr<EnergyGrid> energyGrid =
        std::dynamic_pointer_cast<EnergyGrid>(parent);
    if (energyGrid) {
      auto heatingSim = this->heatingSimulationPtr();
      if (!heatingSim) return;
      for (const auto &[nameOfConsumer, consumer] : heatingSim->Consumers().get()) {
        const auto &name = consumer.getName();
        auto colorIt = this->m_colors.find(name);
        if (colorIt == this->m_colors.end()) continue;

        const auto &colors = colorIt->second;
        if (timestep >= colors.size()) continue;

        const auto &color = colors[timestep];
        if (auto point = energyGrid->getPointByName(name)) {
          point->updateColor(color);
        }
      }
    }
  }

  void updateTimestepColors(const std::string &key, float min = 0.0f,
                            float max = 1.0f, bool resetMinMax = false) override {
    auto color_map = this->m_colorMapRef.lock();
    if (!color_map) {
      std::cerr << "ColorMap is not available for update of colors." << std::endl;
      return;
    }

    if (min > max) min = max;
    color_map->max = max;
    color_map->min = min;

    if (resetMinMax) {
      auto &[res_min, res_max] = heatingSimulationPtr()->getMinMax(key);
      color_map->max = res_max;
      color_map->min = res_min;
    }

    // compute colors
    auto heatingSim = this->heatingSimulationPtr();
    if (!heatingSim) return;
    this->computeColors(color_map, key, min, max, heatingSim->Consumers().get());
    this->computeColors(color_map, key, min, max, heatingSim->Producers().get());
  }

 private:
  std::shared_ptr<HeatingSimulation> heatingSimulationPtr() {
    return std::dynamic_pointer_cast<HeatingSimulation>(this->m_simulation.lock());
  }
};
