#include "heating.h"

namespace core::simulation::heating {

void HeatingSimulation::computeParameters() {
    // TODO: rewrite to use factory pattern
    computeParameter(m_consumers);
    computeParameter(m_producers);
}
}  // namespace core::simulation::heating
