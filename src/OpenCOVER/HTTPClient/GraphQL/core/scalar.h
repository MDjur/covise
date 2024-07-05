#ifndef HTTPCLIENT_GRAPHQL_CORE_SCALAR_H
#define HTTPCLIENT_GRAPHQL_CORE_SCALAR_H

#include <string>
#include <variant>
#include "export.h"

namespace opencover::httpclient::graphql {

template<typename T>
struct GRAPHQLCLIENTEXPORT is_graphql_type {
    static constexpr bool value = false;
};

#define DEFINE_SCALAR_TYPE(TYPE, NAME) \
    typedef TYPE NAME; \
    template<> \
    struct GRAPHQLCLIENTEXPORT is_graphql_type<TYPE> { \
        static constexpr bool value = true; \
    };

// Define the types that can be used in the schema like mentioned in the graphql specification
DEFINE_SCALAR_TYPE(int, Int)
DEFINE_SCALAR_TYPE(float, Float)
DEFINE_SCALAR_TYPE(bool, Boolean)
DEFINE_SCALAR_TYPE(std::string, String)
// TODO: Add custom types

typedef std::variant<Int, Float, Boolean, String> Scalar;
} // namespace opencover::httpclient::graphql

#endif