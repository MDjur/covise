#ifndef HTTPCLIENT_GRAPHQL_CORE_TYPE_H
#define HTTPCLIENT_GRAPHQL_CORE_TYPE_H

#include "field.h"
#include "export.h"
#include <string>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace graphql {

template<typename... Types>
class GRAPHQLCLIENTEXPORT Type {
public:
    Type() = delete;
    Type(const json &query);
    Type(const std::string &name, const Types &...types): name(name), fields(types...){};

    const json &getVariables() const { return variables; };
    std::string toString() const;
    virtual std::string createGraphQLString() const = 0;

private:
    bool initVariables();

protected:
    json variables;
    std::string name;
    Fields<Types...> fields;
};

} // namespace graphql
#endif