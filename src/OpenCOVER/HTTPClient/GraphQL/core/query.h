#ifndef HTTPCLIENT_GRAPHQL_CORE_QUERY_H
#define HTTPCLIENT_GRAPHQL_CORE_QUERY_H

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
    Query(const std::string &name, const FieldType<Types> &...fields): BaseTypeConstructor(name, fields...)
    {
        std::string queryFmtStr = "query %s{ " + name + " %s{ ";
        // std::stringstream ss;
        // auto visitor = [&queryFmtStr](auto &&f) { queryFmtStr += f.name + "\n"; };
        // auto visitor = [](auto &&f) { std::cout << f.name << ": "<< f.value<<"\n";};
        for (const auto &field: BaseType::m_fields)
            queryFmtStr += field.name + " ";
        // queryFmtStr += field.name + " " + field.value;
        // std::visit([&](auto&& v) {ss << field.name << " " << v << " ";}, field.value);
        // queryFmtStr += ss.str();
        queryFmtStr += "}}";
        m_queryFmt = boost::format(queryFmtStr);
    };
    ~Query() = default;
    Query(const Query &) = delete;
    Query &operator=(const Query &) = delete;

private:
    using BaseType = Type<Types...>;
    using BaseTypeConstructor = typename BaseType::Type;

    std::string createGraphQLString() override
    {
        m_queryFmt % "" % "";
        // if (!BaseType::m_variables.empty()) {
        //     // 1. iterate through m_variables
        //     for (const auto &variable: BaseType::m_variables) {
        //         // 2. add variables to header of query and add variables as function parameter in query
        //         // m_queryFmt %  variable
        //         m_queryFmt % "test";
        //     }
        // }
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
    boost::format m_queryFmt;
};
} // namespace opencover::httpclient::graphql

#endif