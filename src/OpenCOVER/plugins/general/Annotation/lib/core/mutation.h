#ifndef _LIB_MUTATION_H
#define _LIB_MUTATION_H

#include "type.h"

namespace graphql {
template<typename... Types>
class Mutation: public Type<Types...> {};
} // namespace graphql

#endif