option(TEST_ENERGYCAMPUS "Test EnergyCampus" OFF)

if(NOT TEST_ENERGYCAMPUS)
    return()
endif()

cmake_minimum_required(VERSION 3.14)
project(EnergyTest)

# GoogleTest requires at least C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY "https://github.com/google/googletest.git"
    GIT_TAG "v1.17.0"
    GIT_SHALLOW TRUE #copy version and not git history

)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

file(GLOB_RECURSE TEST_SOURCES "src/*.cpp")

enable_testing()

set(Name "EnergyTest")
add_executable(
    ${Name} ${TEST_SOURCES} 
)

# Ensure the test binary can find the project's shared libraries at runtime.
# This embeds absolute RPATHs pointing to the linux64opt lib directories from
# the source and build trees so GoogleTest's discovery (which executes the
# binary) won't fail due to unresolved shared libraries in CI.
if(UNIX)
  set(_rpaths
    "${CMAKE_SOURCE_DIR}/linux64opt/lib"
    "${CMAKE_BINARY_DIR}/linux64opt/lib"
  )
  set_target_properties(${Name} PROPERTIES
    BUILD_RPATH "${_rpaths}"
    INSTALL_RPATH "${_rpaths}"
    BUILD_WITH_INSTALL_RPATH ON
  )
endif()

# include directories and linkage
target_include_directories(${Name} 
    PRIVATE ${CMAKE_SOURCE_DIR}/src/OpenCOVER/plugins/hlrs/Energy 
    PRIVATE ${OPENSCENEGRAPH_INCLUDE_DIRS}
)

target_link_libraries(
    ${Name}
    PRIVATE Ennovatis 
    PRIVATE coCurlHTTPClient 
    PRIVATE coThreadUtil 
    PRIVATE coStringUtil 
    PRIVATE coReadCSVUtil 
    PRIVATE core::simulation 
    PRIVATE nlohmann_json::nlohmann_json 
    PRIVATE GTest::gtest_main 
    ${EXTRA_LIBS}
)

add_definitions(-DENERGYCAMPUS_TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data")

include(GoogleTest)
gtest_discover_tests(${Name})
