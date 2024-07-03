#ifndef _LIB_CORE_TYPE_H
#define _LIB_CORE_TYPE_H

#include "field.h"
#include <string>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace graphql {

template<typename... Types>
class Type {
public:
    Type() = delete;
    Type(const json &query);
    Type(const std::string &name, const Types &...types): name(name), fields(types...){};

    const json &getVariables() const { return variables; };
    std::string toString() const;

private:
    bool initVariables();

protected:

    virtual std::string createGraphQlString() const = 0;
    json variables;
    std::string name;
    Fields<Types...> fields;
};

} // namespace graphql
#endif