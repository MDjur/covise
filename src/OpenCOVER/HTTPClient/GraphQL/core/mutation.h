#ifndef _LIB_MUTATION_H
#define _LIB_MUTATION_H

#include "type.h"
#include "export.h"

namespace graphql {
template<typename... Types>
class GRAPHQLCLIENTEXPORT Mutation: public Type<Types...> {
public:
    using Type<Types...>::Type;
    std::string createGraphQLString() const override{return "";};
    // std:: string queryStr_new = R"(mutation Test($lat: Float!, $lon:Float!){ updateMarker(lat: $lat, lng: $lon){lat lng}})";
};
} // namespace graphql

#endif