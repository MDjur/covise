find_package(nlohmann_json)

if (NOT nlohmann_json_FOUND)
    message("nlohmann_json not found")
endif()

set(SOURCES 
    core/query.cpp
    core/type.cpp
    graphql.cpp
)

set(HEADERS
    core/export.h
    core/query.h
    core/type.h
    core/field.h
    core/scalar.h
    core/subscription.h
    core/mutation.h
    graphql.h
)

ADD_COVISE_LIBRARY(coGraphQL ${SOURCES} ${HEADERS})
add_dependencies(coGraphQL coCurlHTTPClient)
target_link_libraries(
    coGraphQL 
    coCurlHTTPClient
    nlohmann_json::nlohmann_json)
COVISE_INSTALL_TARGET(coGraphQL)
