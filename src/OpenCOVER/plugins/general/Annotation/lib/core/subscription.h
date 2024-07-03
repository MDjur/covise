#ifndef _LIB_SUBSCRIPTION_H
#define _LIB_SUBSCRIPTION_H

#include "type.h"

namespace graphql {
template<typename... Types>
class Subscription: public Type<Types...> {};
} // namespace graphql

#endif