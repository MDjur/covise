#ifndef _CORE_INTERFACES_ENERGYGRID_H
#define _CORE_INTERFACES_ENERGYGRID_H

#include "IColorable.h"
#include "IDrawables.h"

namespace core::interface {
class EnergyGrid : public IDrawables, public IColorable {};
}  // namespace core::interface

#endif
