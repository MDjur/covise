#ifndef HTTPCLIENT_GRAPHQL_CORE_TYPE_H
#define HTTPCLIENT_GRAPHQL_CORE_TYPE_H

#include "field.h"
#include "export.h"
#include <string>
#include <boost/format.hpp>
#include <nlohmann/json.hpp>

namespace opencover::httpclient::graphql {

template<typename... Types>
class GRAPHQLCLIENTEXPORT ObjectType {
public:
    ObjectType(const std::string &alias_name, const FieldType<Types> &...fields)
    : m_alias(alias_name), m_fields(fields...){};
    ObjectType() = delete;
    ObjectType(ObjectType &&) = delete;
    ObjectType &operator=(ObjectType &&) = delete;

    virtual ~ObjectType() = default;
    ObjectType(const ObjectType &) = delete;
    ObjectType &operator=(const ObjectType &) = delete;

    const Fields<Types...> &getFields() const { return m_fields; };
    std::string toString() { return createGraphQLString(); };

private:
    auto helper_createBody() {
        // raw body of the query
        boost::format m_queryFmt;
        std::string queryFmtStr = "query %s{ " + m_alias + " %s{ ";
        for (const auto &field: m_fields)
            queryFmtStr += field.name + " ";
        queryFmtStr += "}}";
        m_queryFmt = boost::format(queryFmtStr);
        return m_queryFmt;
    }
    
    auto helper_createHeaderAndFunctionPart() {
        // header: query Test($userId: Int = 2)
        auto header = boost::format("Test(%s)");
        auto headerArg = boost::format("$%s: %s");

        // queryFunction: user(id: $userId)
        auto function = boost::format("(%s)");
        auto functionArg = boost::format("%s: $%s");

        std::string functionParamArgs = "";
        std::string headerArgs = "";

        // for (const auto &[var_name, var_val]: m_variables.items()) {
        //     headerArg % var_name % json_type_to_string(var_val);
        //     functionArg % "id" % var_name;
        //     headerArgs += headerArg.str() + ", ";
        //     functionParamArgs += functionArg.str() + ", ";
        //     functionArg.clear();
        //     headerArg.clear();
        // }
        header % headerArgs.substr(0, headerArgs.size() - 2);
        function % functionParamArgs.substr(0, functionParamArgs.size() - 2);
        return std::pair<boost::format, boost::format>(header, function);
    }

    virtual std::string createGraphQLString()
    {
        //example: query Test($userId: Int = 2) {
        //      user(id: $userId) {
        //          id
        //          name
        //          age
        //      }
        // }";
        //
        // CURL-String to test the query
        // curl -X POST http://127.0.0.1:8081/graphql -H "Content-Type: application/json" -d '{"query":"query Test($userId: Int = 2){ user(id: $userId) { id name age }}", "variables": { "userId":3 }}'
        auto m_queryFmt = helper_createBody();
        auto [header, queryFunction] = helper_createHeaderAndFunctionPart();
        m_queryFmt % header % queryFunction;

        nlohmann::json query;
        query["query"] = m_queryFmt.str();
        // query["variables"] = m_variables;
        return query.dump(4);
    };

protected:
    std::string m_alias;
    Fields<Types...> m_fields;
};
} // namespace opencover::httpclient::graphql
#endif