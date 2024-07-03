#include "type.h"
#include "field.h"
#include <iostream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace graphql {

template <typename... Types>
bool Type<Types...>::initVariables()
{
    if (fields.empty())
        return false;
    
    for (const auto &f : fields)
        std::visit([&](const auto &v) {std::cout << v << "\n";}, f.value);
    
    return true;
}

// Type::Type(const json &query)
// {
//     for (const auto& [key, value] : query.items())
//         fields.push_back(key, value);
//         // fields.push_back(std::make_any<Field<typeid(value)>>(key, value));
//     if (initVariables())
//         std::cout << "Variables initialized" << std::endl;
//     else
//         std::cout << "Variables not initialized" << std::endl;
// }


template <typename... Types>
std::string Type<Types...>::toString() const
{
    return "Type: " + name;
    // return createGraphQLString();
}
} // namespace graphql