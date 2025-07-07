#pragma once

#include <iostream>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

#include "object.h"

namespace core::simulation {
struct UnitPair {
  std::vector<std::string> names;
  std::string unit;
};

class UnitMap {
 public:
  UnitMap(std::vector<UnitPair> &&unitPairs) {
    for (const auto &pair : unitPairs) {
      for (const auto &name : pair.names) {
        unit_map[name] = pair.unit;
      }
    }
  }

  const std::string operator[](const std::string &key) const {
    auto it = unit_map.find(key);
    if (it != unit_map.end()) {
      return it->second;
    }
    return "unknown";
  }

  auto begin() { return unit_map.begin(); }
  auto end() { return unit_map.end(); }

 private:
  std::unordered_map<std::string, std::string> unit_map;
};

const UnitMap UNIT_MAP = UnitMap({
    {{"kWh", "leistung", "power"}, "kWh"},
    {{"kW"}, "kW"},
    {{"q_dem_w", "waermestromdichte"}, "W/m2"},
    {{"delta_q", "aenderung_stromdichte"}, "W/m2"},
    {{"mass_flow", "massenstrom"}, "kg/s"},
    {{"celcius", "temp", "inlet_temp", "outlet_temp"}, "°C"},
    {{"electricity_selling_price"}, "Cent/kWh"},
    {{"heating_cost"}, "€"},
    {{"voltage", "volt"}, "V"},
    {{"current", "ampere"}, "A"},
    {{"i_ka"}, "kA"},
    {{"resistance", "ohm"}, "Ω"},
    {{"power_factor", "cos_phi"}, ""},
    {{"efficiency", "eta"}, ""},
    {{"reactive_power", "q"}, "var"},
    {{"active_power", "p"}, "W"},
    {{"apparent_power", "s"}, "VA"},
    {{"vm_pu"}, "pu (voltage per unit)"},
    {{"q_mvar"}, "Mvar"},
    {{"loading_percent", "percent"}, "%"},
});

struct ScalarProperty {
  std::string unit;
  std::string species;
  double min;
  double max;
  size_t timesteps;
};

typedef std::map<std::string, ScalarProperty> ScalarProperties;

class Simulation {
 public:
  Simulation() = default;

  void addData(const std::string &key, const std::vector<double> &value) {
    m_data[key] = value;
  }

  void addData(const std::string &key, const double &value) {
    m_data[key].push_back(value);
  }

  const auto &ScalarProp(const std::string &key) const {
    auto it = m_scalarProperties.find(key);
    if (it == m_scalarProperties.end())
      throw std::out_of_range("Key not found Simulation: " + key);
    return it->second;
  }

  auto &getData() { return m_data; }

  auto getMax(const std::string &key) const { return ScalarProp(key).max; }
  auto getMin(const std::string &key) const { return ScalarProp(key).min; }
  auto getMinMax(const std::string &key) const {
    return std::make_pair(getMin(key), getMax(key));
  }
  auto getTimesteps(const std::string &key) const {
    return ScalarProp(key).timesteps;
  }
  auto getSpecies(const std::string &key) const { return ScalarProp(key).species; }
  auto getUnit(const std::string &key) const { return ScalarProp(key).unit; }
  const auto &getScalarProperties() const { return m_scalarProperties; }
  auto &getScalarProperties() { return m_scalarProperties; }

  virtual void computeParameters() {};
  virtual const std::vector<double> *getTimedependentScalar(
      const std::string &species, const std::string &node) const = 0;

 protected:
  template <typename T>
  void computeParameter(const ObjectContainer<T> &container, float trim = 0.01) {
    static_assert(std::is_base_of_v<Object, T>,
                  "T must be derived from core::simulation::Object");
    std::map<std::string, std::vector<double>> allValues{};
    for (const auto &[_, object] : container.get()) {
      const auto &data = object.getData();
      for (const auto &[key, values] : data)
        allValues[key].insert(allValues[key].end(), values.begin(), values.end());
    }

    for (const auto &[key, values] : allValues) {
      setUnit(key);
      computeMinMax(key, values, trim);  // 1% trimming
      computeMaxTimestep(key, values);
      m_scalarProperties[key].species = key;
    }
  }

  template <typename T>
  const std::vector<double> *getTimedependentScalar(
      const ObjectContainer<T> &container, const std::string &species,
      const std::string &node) const {
    static_assert(std::is_base_of_v<Object, T>,
                  "T must be derived from core::simulation::Object");
    for (const auto &[name, object] : container) {
      if (object.getName() == node) {
        const auto &data = object.getData();
        if (data.find(species) != data.end()) {
          return &data.at(species);
        }
      }
    }
    return nullptr;
  }

  virtual void computeMinMax(const std::string &key,
                             const std::vector<double> &values,
                             const double &trimPercent = 0.00);
  virtual void computeMaxTimestep(const std::string &key,
                                  const std::vector<double> &values);
  virtual void setUnit(const std::string &key);

  ScalarProperties m_scalarProperties;
  // general meta data for the simulation
  Data m_data;
};
}  // namespace core::simulation
