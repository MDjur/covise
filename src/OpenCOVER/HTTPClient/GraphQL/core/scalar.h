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
// TODO: define ID later
// TODO: Add custom types

typedef std::variant<Int, Float, Boolean, String> Scalar;

inline constexpr const char * scalarTypeToString(const Scalar &s)
{
    auto visitor = [](const auto &v) -> const char * { 
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, Int>) {
            return "Int";
        } else if constexpr (std::is_same_v<T, Float>) {
            return "Float";
        } else if constexpr (std::is_same_v<T, Boolean>) {
            return "Boolean";
        } else if constexpr (std::is_same_v<T, String>) {
            return "String";
        } else {
            return "Unknown";
        }
    };
    return std::visit(visitor, s);
}
} // namespace opencover::httpclient::graphql

#endif