#ifndef HTTPCLIENT_GRAPHQL_CORE_SUBSCRIPTION_H
#define HTTPCLIENT_GRAPHQL_CORE_SUBSCRIPTION_H

#include "type.h"
#include "export.h"

namespace opencover::httpclient::graphql {
template<typename... Types>
class GRAPHQLCLIENTEXPORT Subscription: public ObjectType<Types...> {
public:
    using ObjectType<Types...>::Type;
    ~Subscription() = default;
    Subscription(const Subscription &) = delete;
    Subscription &operator=(const Subscription &) = delete;

private:
    std::string createGraphQLString() override {};
};
} // namespace opencover::httpclient::graphql

#endif