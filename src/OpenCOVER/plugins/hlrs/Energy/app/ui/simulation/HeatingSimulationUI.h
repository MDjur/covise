#pragma once

#include <lib/core/interfaces/IColorable.h>
#include <lib/core/interfaces/IDrawables.h>
#include <lib/core/interfaces/ITimedependable.h>
#include <lib/core/simulation/heating.h>
#include <lib/core/utils/color.h>

#include <algorithm>
#include <memory>
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

  typedef covise::ColorMap ColorMap;

 public:
  HeatingSimulationUI(const HeatingSimulation &sim, std::shared_ptr<T> parent,
                      std::shared_ptr<ColorMap> colorMap)
      : m_simulation(sim), m_parent(parent), m_colorMapRef(colorMap) {
    m_simulation.computeParameters();
  }
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

  void updateTimestepColors(const std::string &key, float min = 0.0f,
                            float max = 1.0f,
                            bool resetMinMax = false) {
    auto color_map = m_colorMapRef.lock();
    if (min > max) min = max;
    color_map->max = max;
    color_map->min = min;

    if (resetMinMax) {
      auto &[res_min, res_max] = m_simulation.getMinMax(key);
      color_map->max = res_max;
      color_map->min = res_min;
    }

    // compute colors
    computeColors(color_map, key, min, max, m_simulation.getConsumers());
    computeColors(color_map, key, min, max, m_simulation.getProducers());
  }

  void updateTime(int timestep) {
    auto parent = m_parent.lock();
    if (!parent) return;
    // TODO: rethink this pls => maybe use a visitor pattern
    std::shared_ptr<EnergyGrid> energyGrid =
        std::dynamic_pointer_cast<EnergyGrid>(parent);
    if (energyGrid) {
      for (const auto &[nameOfConsumer, consumer] : m_simulation.getConsumers()) {
        const auto &name = consumer.getName();
        auto colorIt = m_colors.find(name);
        if (colorIt == m_colors.end()) continue;

        const auto &colors = colorIt->second;
        if (timestep >= colors.size()) continue;

        const auto &color = colors[timestep];
        if (auto point = energyGrid->getPointByName(name)) {
          point->updateColor(color);
        }
      }
    }
  }

  auto &getParent() { return m_parent; }
  auto &getData() { return m_simulation.getData(); }
  const auto &getConsumers() const { return m_simulation.getConsumers(); }
  const auto &getProducers() const { return m_simulation.getProducers(); }
  const auto &getSim() const { return m_simulation; }

 private:
  double interpolate(double value, double min, double max, double min_val,
                     double max_val) {
    return ((max_val - min_val) * (value - min) / (max - min)) + min_val;
  }

  template <typename base_type>
  void computeColors(std::shared_ptr<ColorMap> color_map,
                     const std::string &key, float min, float max,
                     const std::map<std::string, base_type> &baseMap) {
    static_assert(std::is_base_of_v<Base, base_type>,
                  "base_type must be derived from core::simulation::heating::Base");
    double minKeyVal = 0.0, maxKeyVal = 1.0;

    try {
      auto &[min_val, max_val] = m_simulation.getMinMax(key);
      minKeyVal = min_val;
      maxKeyVal = max_val;
    } catch (const std::out_of_range &e) {
      std::cerr << "Key not found in minMaxValues: " << key << std::endl;
      return;
    }

    for (auto &[name, base] : baseMap) {
      const auto &data = base.getData();
      const auto &values = data.at(key);
      if (auto color_it = m_colors.find(name); color_it == m_colors.end()) {
        m_colors.insert({name, std::vector<osg::Vec4>(values.size())});
      }
      auto &colors = m_colors[name];

      // color_map
      for (auto i = 0; i < values.size(); ++i) {
        auto interpolated_value =
            interpolate(values[i], minKeyVal, maxKeyVal, min, max);
        auto color = covise::getColor(interpolated_value, *color_map, min, max);
        colors[i] = color;
      }
    }
  }

  HeatingSimulation m_simulation;
  std::weak_ptr<T> m_parent;  // parent which manages drawable
  std::weak_ptr<ColorMap> m_colorMapRef;
  std::map<std::string, std::vector<osg::Vec4>> m_colors;
};
