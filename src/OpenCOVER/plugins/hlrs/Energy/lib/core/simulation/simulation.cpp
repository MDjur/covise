#include "simulation.h"

#include <algorithm>
#include <cmath>

namespace {
std::pair<double, double> robustMinMax(const std::vector<double> &values,
                                       double trimPercent = 0.01) {
  if (values.empty()) return {0.0, 0.0};
  std::vector<double> sorted = values;
  std::sort(sorted.begin(), sorted.end());
  size_t n = sorted.size();
  size_t trim = static_cast<size_t>(std::round(n * trimPercent));
  size_t lower = std::min(trim, n - 1);
  size_t upper = std::max(n - trim - 1, lower);
  return {sorted[lower], sorted[upper]};
}
}  // namespace

namespace core::simulation {

void Simulation::computeMinMax(const std::string &key,
                               const std::vector<double> &values,
                               const double &trimPercent) {
  auto [min_elem, max_elem] = robustMinMax(values, trimPercent);
  m_scalarProperties[key].min = min_elem;
  m_scalarProperties[key].max = max_elem;
}

void Simulation::computeMaxTimestep(const std::string &key,
                                    const std::vector<double> &values) {
  m_scalarProperties[key].timesteps = values.size();
}

void Simulation::setUnit(const std::string &key) {
  m_scalarProperties[key].unit = UNIT_MAP[key];
}
}  // namespace core::simulation
