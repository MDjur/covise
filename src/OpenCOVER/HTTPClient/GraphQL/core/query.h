#ifndef HTTPCLIENT_GRAPHQL_CORE_QUERY_H
#define HTTPCLIENT_GRAPHQL_CORE_QUERY_H

#include "type.h"
#include "export.h"
#include <nlohmann/json.hpp>

namespace {
std::string json_type_to_string(const nlohmann::json &j)
{
    if (j.is_string()) {
        return "String";
    } else if (j.is_number_integer()) {
        return "Int!";
    } else if (j.is_number_float()) {
        return "Float";
    } else if (j.is_boolean()) {
        return "Boolean";
    } else {
        return "Unknown";
    }
}
} // namespace

namespace opencover::httpclient::graphql {
template<typename... Types>
class GRAPHQLCLIENTEXPORT Query: public ObjectType<Types...> {
public:
    Query(const std::string &name, const nlohmann::json &variables, const FieldType<Types> &...fields)
    : BaseType(name, fields...), m_variables(variables){};
    ~Query() = default;
    Query(const Query &) = delete;
    Query &operator=(const Query &) = delete;
    nlohmann::json &getVariables() { return m_variables; };

private:
    using BaseType = ObjectType<Types...>;
    nlohmann::json m_variables;
};
} // namespace opencover::httpclient::graphql

#endif