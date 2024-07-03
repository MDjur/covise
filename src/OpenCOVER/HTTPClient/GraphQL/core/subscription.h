#ifndef HTTPCLIENT_GRAPHQL_CORE_SUBSCRIPTION_H
#define HTTPCLIENT_GRAPHQL_CORE_SUBSCRIPTION_H

#include "type.h"
#include "export.h"

namespace opencover::httpclient::graphql {
template<typename... Types>
class GRAPHQLCLIENTEXPORT Subscription: public Type<Types...> {
public:
    using Type<Types...>::Type;
    // std::string createGraphQLString() const override{return "";};
};
} // namespace graphql

#endif