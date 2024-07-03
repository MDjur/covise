#ifndef HTTPCLIENT_GRAPHQL_CORE_QUERY_H
#define HTTPCLIENT_GRAPHQL_CORE_QUERY_H

#include "type.h"
#include "export.h"

namespace graphql {
template<typename... Types>
class GRAPHQLCLIENTEXPORT Query: public Type<Types...> {
public:
    using Type<Types...>::Type;
    std::string createGraphQLString() const override;
};
} // namespace graphql

#endif