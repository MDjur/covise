IF (APPLE)
    RETURN()
ENDIF()


SET(HEADERS
   Joystick.h
)

SET(SOURCES
   Joystick.cpp
)
IF(WIN32)
SET(HEADERS ${HEADERS} TempWindow.h)
SET(SOURCES ${SOURCES} TempWindow.cpp)
ENDIF(WIN32)


vive_add_plugin(vvInput_joystick)

IF(WIN32)
TARGET_LINK_LIBRARIES(vvInput_joystick
   ${EXTRA_LIBS}
   dinput8 comctl32
)
#dxguid nor required on Win7 (added #include <InitGuid.h> #define DIRECTINPUT_VERSION 0x0800 #include <dinput.h> #include <dsound.h> instead)
ELSE(WIN32)
TARGET_LINK_LIBRARIES(vvInput_joystick
    ${EXTRA_LIBS}
)
ENDIF(WIN32)
