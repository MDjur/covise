#ifndef _LIB_MUTATION_H
#define _LIB_MUTATION_H

#include "type.h"
#include "export.h"

namespace opencover::httpclient::graphql {
template<typename... Types>
class GRAPHQLCLIENTEXPORT Mutation: public ObjectType<Types...> {
public:
    using ObjectType<Types...>::Type;
    // std:: string queryStr_new = R"(mutation Test($lat: Float!, $lon:Float!){ updateMarker(lat: $lat, lng: $lon){lat lng}})";
    ~Mutation() = default;
    Mutation(const Mutation &) = delete;
    Mutation &operator=(const Mutation &) = delete;
private:
    std::string createGraphQLString() override {};
};
} // namespace opencover::httpclient::graphql

#endif