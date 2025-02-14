#pragma once

#include <lib/core/interfaces/IColorable.h>
#include <lib/core/interfaces/IDrawables.h>
#include <lib/core/interfaces/ITimedependable.h>
#include <lib/core/simulation/heating.h>
#include <lib/core/utils/color.h>

#include <memory>
#include <algorithm>
#include <osg/Vec4>

#include "../../presentation/EnergyGrid.h"

using namespace core::simulation::heating;

template <typename T>
class HeatingSimulationUI {
  static_assert(std::is_base_of_v<core::interface::IDrawables, T>,
                "T must be derived from IDrawable");
  static_assert(std::is_base_of_v<core::interface::IColorable, T>,
                "T must be derived from IColorable");
  static_assert(std::is_base_of_v<core::interface::ITimedependable, T>,
                "T must be derived from IColorable");

  typedef core::utils::color::ColorMapExtended ColorMapExtended;

 public:
  HeatingSimulationUI(const HeatingSimulation &sim, std::shared_ptr<T> parent,
                      std::shared_ptr<ColorMapExtended> colorMap)
      : m_simulation(sim), m_parent(parent), m_colorMapRef(colorMap) {}
  ~HeatingSimulationUI() = default;
  HeatingSimulationUI(const HeatingSimulationUI &) = delete;
  HeatingSimulationUI &operator=(const HeatingSimulationUI &) = delete;
  HeatingSimulationUI(HeatingSimulationUI &&other) {
    m_parent = std::move(other.getParent());
    m_simulation = std::move(other.getData());
    other.getParent().reset();
  }

  HeatingSimulationUI &operator=(HeatingSimulationUI &&other) noexcept {
    if (this == &other) return *this;
    m_parent = std::move(other.getParent());
    m_simulation = std::move(other.getData());

    other.getParent().reset();
    return *this;
  }

  void updateTimestepColors(const std::string &key) {
    for (auto &[nameOfConsumer, consumer] : m_simulation.getConsumers()) {
      const auto &data = consumer.getData();
      const auto &values = data.at(key);
      if (auto color_it = m_colors.find(nameOfConsumer);
          color_it == m_colors.end()) {
        m_colors.insert({nameOfConsumer, std::vector<osg::Vec4>(values.size())});
      }
      const auto &[min, max] = std::minmax_element(values.begin(), values.end());
      auto &colors = m_colors[nameOfConsumer];
      auto color_map = m_colorMapRef.lock();
      color_map->max = *max;
      color_map->min = *min;
      for (auto i = 0; i < values.size(); ++i) {
        auto value = values[i];
        auto color =
            covise::getColor(value, color_map->map, color_map->min, color_map->max);
        colors[i] = color;
      }
    }
  }

  void updateTime(int timestep) {
    auto parent = m_parent.lock();
    if (!parent) return;
    std::shared_ptr<EnergyGrid> energyGrid =
        std::dynamic_pointer_cast<EnergyGrid>(parent);
    if (energyGrid) {
      for (auto &[nameOfConsumer, consumer] : m_simulation.getConsumers()) {
        const auto &name = consumer.getName();
        if (auto it = m_colors.find(name); it != m_colors.end()) {
          auto &colors = it->second;
          if (timestep >= colors.size()) continue;
          auto color = colors[timestep];
          auto point = energyGrid->getPointByName(name);
          if (point) {
            point->updateColor(color);
          }
        }
      }
    }
  }

  auto &getParent() { return m_parent; }
  auto &getData() { return m_simulation.getData(); }
  const auto &getConsumers() const { return m_simulation.getConsumers(); }
  const auto &getProducers() const { return m_simulation.getProducers(); }

 private:
  HeatingSimulation m_simulation;
  std::weak_ptr<T> m_parent;  // parent which manages drawable
  std::weak_ptr<ColorMapExtended> m_colorMapRef;
  std::map<std::string, std::vector<osg::Vec4>> m_colors;
};
