if(NOT COVISE_USE_QT)
    return()
endif()

INCLUDE_DIRECTORIES(${XERCESC_INCLUDE_DIR})

SET(HEADERS
  SimpleRopePlugin.h
)

SET(SOURCES
  SimpleRopePlugin.cpp
)

cover_add_plugin(SimpleRope ${HEADERS} ${SOURCES})

IF(MSVC AND NOT MSVC10)
 # can't have this in vs2017 ADD_COVISE_COMPILE_FLAGS(SimpleRope "/Zc:wchar_t-")
ENDIF()

TARGET_LINK_LIBRARIES(SimpleRope ${XERCESC_LIBRARIES}) 
