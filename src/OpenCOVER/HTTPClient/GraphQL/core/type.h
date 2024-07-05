#ifndef HTTPCLIENT_GRAPHQL_CORE_TYPE_H
#define HTTPCLIENT_GRAPHQL_CORE_TYPE_H

#include "field.h"
#include "export.h"
#include <string>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
namespace opencover::httpclient::graphql {
template<typename... Types>
class GRAPHQLCLIENTEXPORT Type {
public:
    Type() = delete;
    Type(const Type &) = delete;
    Type(Type &&) = delete;
    Type &operator=(const Type &) = delete;
    Type &operator=(Type &&) = delete;
    Type(const std::string &name, const FieldType<Types> &...fields): m_name(name), m_fields(fields...)
    {
        initVariables();
    };

    json &getVariables() { return m_variables; };
    const Fields<Types...> &getFields() const { return m_fields; };
    std::string toString() { return createGraphQLString(); };

private:
    bool initVariables()
    {
        if (m_fields.empty())
            return false;

        for (const auto &f : m_fields)
            // m_variables[f.name] = f.value;
            std::visit([&](const auto &v) { m_variables[f.name] = v; }, f.value);

        return true;
    };

protected:
    virtual std::string createGraphQLString() = 0;
    json m_variables;
    std::string m_name;
    Fields<Types...> m_fields;
};
} // namespace opencover::httpclient::graphql
#endif