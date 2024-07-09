#ifndef HTTPCLIENT_GRAPHQL_CORE_FIELD_H
#define HTTPCLIENT_GRAPHQL_CORE_FIELD_H

#include "scalar.h"
#include "export.h"
#include <vector>
#include <tuple>
#include <iostream>

namespace opencover::httpclient::graphql {

template<typename T>
using FieldType = std::tuple<std::string, T, bool>;

template<typename T, typename = std::enable_if<is_graphql_type<T>::value>>
struct GRAPHQLCLIENTEXPORT Field {
    std::string name;
    T value;
    bool nullable = false;
};

template<typename... Types>
class GRAPHQLCLIENTEXPORT Fields {
public:
    using FieldsType = std::vector<Field<Scalar>>;
    Fields(const FieldType<Types> &...fields)
    {
        m_fields.reserve(sizeof...(fields));
        (add(fields), ...);
    }
    typename FieldsType::iterator begin() { return m_fields.begin(); }
    typename FieldsType::iterator end() { return m_fields.end(); }
    size_t size() const { return m_fields.size(); }
    bool empty() const { return m_fields.empty(); }
    void print() const
    {
        for (const auto &f: m_fields)
            std::visit([&](auto &&v) { std::cout << f.name << ": " << v << "\n"; }, f.value);
    }

private:
    template<typename T>
    void add(const FieldType<T> &field)
    {
        const auto &[name, value, nullable] = field;
        m_fields.emplace_back(name, value, nullable);
    }
    FieldsType m_fields;
};
} // namespace opencover::httpclient::graphql
#endif