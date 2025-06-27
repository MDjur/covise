#include "power.h"

namespace core::simulation::power {

void PowerSimulation::computeParameters() {
    computeParameter(m_buses);
    computeParameter(m_generators);
    computeParameter(m_transformators);
    computeParameter(m_cables);
}
}  // namespace core::simulation::power
