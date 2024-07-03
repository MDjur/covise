#include "query.h"
#include <nlohmann/json.hpp>

namespace graphql {
template<typename... Types>
std::string Query<Types...>::createGraphQLString() const
{
    //example: query Test($userId: Int = 2) {
    //      user(id: $userId) {
    //          id
    //          name
    //          age
    //      }
    // }";
    // curl -X POST http://127.0.0.1:8081/graphql -H "Content-Type: application/json" -d '{"query":"query Test($userId: Int = 2){ user(id: $userId) { id name age }}", "variables": { "userId":3 }}'
    std::string queryStr = "query {";
    // for (auto i = 0; i < fields.size(); ++i)
    //     queryStr += fields[i].name + " ";
    queryStr += "}";
    json query;
    query["query"] = queryStr;
    return query.dump(4);
}
} // namespace graphql