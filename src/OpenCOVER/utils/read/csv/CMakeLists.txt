cmake_policy(SET CMP0167 NEW)
find_package(Boost REQUIRED COMPONENTS filesystem)

if(NOT Boost_FOUND)
    message(FATAL_ERROR "Boost not found, please install Boost libraries")
    return()
endif()

set(SOURCES csv.cpp)
set(HEADERS export.h csv.h)

ADD_COVISE_LIBRARY(coReadCSVUtil ${SOURCES} ${HEADERS})
# ADD_LIBRARY(coReadCSVUtil ${SOURCES} ${HEADERS})
target_link_libraries(coReadCSVUtil Boost::filesystem)
COVISE_INSTALL_TARGET(coReadCSVUtil)
COVISE_INSTALL_HEADERS(coReadCSVUtil ${HEADERS})
COVISE_WNOERROR(coReadCSVUtil BASEARCH zebu)
set_target_properties(coReadCSVUtil PROPERTIES LINKER_LANGUAGE CXX)