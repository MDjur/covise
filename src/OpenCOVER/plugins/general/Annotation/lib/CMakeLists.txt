find_package(nlohmann_json)

if (NOT nlohmann_json_FOUND)
    message("nlohmann_json not found")
endif()

if (NOT CURL_FOUND)
    message("CURL not found")
endif()

set(SOURCES 
    core/query.cpp
    core/type.cpp
    graphql.cpp
)

set(HEADERS
    core/query.h
    core/type.h
    core/field.h
    core/scalar.h
    graphql.h
)

add_library(coGraphQL OBJECT ${SOURCES} ${HEADERS})
add_dependencies(coGraphQL coCurlHTTPClient)
target_link_libraries(
    coGraphQL 
    PRIVATE coCurlHTTPClient
    PRIVATE nlohmann_json::nlohmann_json)
set_target_properties(coGraphQL PROPERTIES LINKER_LANGUAGE CXX POSITION_INDEPENDENT_CODE ON)
