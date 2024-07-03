#ifndef _LIB_QUERY_H
#define _LIB_QUERY_H

#include "type.h"

namespace graphql {
template<typename... Types>
class Query: public Type<Types...> {
public:
    using Type<Types...>::Type;
protected:
    std::string createGraphQLString() const override;
};
} // namespace graphql

#endif