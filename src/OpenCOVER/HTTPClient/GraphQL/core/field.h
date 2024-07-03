#ifndef HTTPCLIENT_GRAPHQL_CORE_FIELD_H
#define HTTPCLIENT_GRAPHQL_CORE_FIELD_H

#include "scalar.h"
#include "export.h"
#include <variant>
#include <vector>

namespace opencover::httpclient::graphql {

template<typename T, typename = std::enable_if_t<is_graphql_type<T>::value>>
struct GRAPHQLCLIENTEXPORT Field {
    std::string name;
    T value;
};

template<typename... Types>
class GRAPHQLCLIENTEXPORT Fields {
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
    template<typename T, typename = std::enable_if_t<is_graphql_type<T>::value>>
    void push_back(const std::string &name, const T &val)
    {
        m_fields.push_back(Field<T>{name, val});
    }
    FieldsType m_fields;
};

} // namespace graphql

#endif