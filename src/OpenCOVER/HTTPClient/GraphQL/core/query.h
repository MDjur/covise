#ifndef HTTPCLIENT_GRAPHQL_CORE_QUERY_H
#define HTTPCLIENT_GRAPHQL_CORE_QUERY_H

#include "scalar.h"
#include "type.h"
#include "export.h"
#include <boost/format.hpp>
// #include <sstream>
#include <variant>
#include <iostream>

namespace opencover::httpclient::graphql {
template<typename... Types>
class GRAPHQLCLIENTEXPORT Query: public Type<Types...> {
public:
    Query(const std::string &name, const FieldType<Types> &...fields): BaseTypeConstructor(name, fields...){};
    ~Query() = default;
    Query(const Query &) = delete;
    Query &operator=(const Query &) = delete;

private:
    using BaseType = Type<Types...>;
    using BaseTypeConstructor = typename BaseType::Type;

    std::string createGraphQLString() override
    {
        boost::format m_queryFmt;
        std::string queryFmtStr = "query %s{ " + BaseType::m_name + " %s{ ";
        for (const auto &field: BaseType::m_fields)
            queryFmtStr += field.name + " ";
        queryFmtStr += "}}";
        m_queryFmt = boost::format(queryFmtStr);
        auto header = boost::format("Test(%s)");
        auto headerParameter = boost::format("$%s: %s");
        auto queryFunction = boost::format("(%s)");
        auto queryFunctionParameter = boost::format("%s: $%s");
        std::string queryFunctionParameterList = "";
        std::string headerParameterList = "";
        for (const auto &field: BaseType::m_fields) {
            headerParameter % field.name % scalarTypeToString(field.value);
            queryFunctionParameter % field.name % field.name;
            headerParameterList += headerParameter.str() + ", ";
            queryFunctionParameterList += queryFunctionParameter.str() + ", ";
            queryFunctionParameter.clear();
            headerParameter.clear();
        }
        header % headerParameterList.substr(0, headerParameterList.size() - 2);
        queryFunction % queryFunctionParameterList.substr(0, queryFunctionParameterList.size() - 2);
        m_queryFmt % header % queryFunction;

        json query;
        query["query"] = m_queryFmt.str();
        query["variables"] = BaseType::m_variables;
        return query.dump(4);
    };

    //example: query Test($userId: Int = 2) {
    //      user(id: $userId) {
    //          id
    //          name
    //          age
    //      }
    // }";
    // curl -X POST http://127.0.0.1:8081/graphql -H "Content-Type: application/json" -d '{"query":"query Test($userId: Int = 2){ user(id: $userId) { id name age }}", "variables": { "userId":3 }}'
    // boost::format m_queryFmt;
};
} // namespace opencover::httpclient::graphql

#endif