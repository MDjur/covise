#ifndef _LIB_CORE_FIELD_H
#define _LIB_CORE_FIELD_H

#include "scalar.h"
#include <variant>
#include <vector>

namespace graphql {

template<typename T, typename = std::enable_if_t<is_graphql_type<T>::value>>
struct Field {
    std::string name;
    T value;
};

template<typename... Types>
class Fields {
public:
    using FieldsType = std::vector<std::variant<Types...>>;

    Fields(const Types &...fields)
    {
        m_fields.reserve(sizeof...(fields));
        (m_fields.push_back(fields), ...);
    }
    typename FieldsType::iterator begin() { return m_fields.begin(); }
    typename FieldsType::iterator end() { return m_fields.end(); }

private:
    template<typename T>
    void push_back(const std::string &name, const T &val)
    {
        m_fields.push_back(Field<T>{name, val});
    }
    FieldsType m_fields;
};

} // namespace graphql

#endif