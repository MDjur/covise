#ifndef HTTPCLIENT_GRAPHQL_CORE_QUERY_H
#define HTTPCLIENT_GRAPHQL_CORE_QUERY_H

#include "type.h"
#include "export.h"

namespace opencover::httpclient::graphql {
template<typename... Types>
class GRAPHQLCLIENTEXPORT Query: public Type<Types...> {
public:
    using Type<Types...>::Type;
    std::string createGraphQLString() const override { return "Query access"; };
};
} // namespace opencover::httpclient::graphql

#endif