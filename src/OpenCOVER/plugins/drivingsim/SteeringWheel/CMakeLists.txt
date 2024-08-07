IF(NOT COVISE_BUILD_DRIVINGSIM)
  RETURN()
ENDIF()
if(APPLE)
    return()
endif()

USING(XERCESC)
using(VRML)

COVISE_FIND_PACKAGE(Xenomai)
#find_package(Boost 1.40 REQUIRED COMPONENTS date_time)
#find_package(Boost 1.40)
#USING(BOOST)

using(VehicleUtil)

SET(HEADERS
 SteeringWheel.h
 CarSound.h
 EngineSound.h
 VehicleDynamics.h
 Vehicle.h
 RemoteVehicle.h
 Porsche.h
 FKFS.h
 FFWheel.h
 CAN.h
 CanInterface.h
 CanOpenBus.h
 ServoStar.h
 PorscheSteeringWheel.h
 InputDevice.h
 Keyboard.h
 EinspurDynamik.h
 ITMDynamics.h
 FKFSDynamics.h
 PorscheController.h
 PorscheRealtimeDynamics.h
 HLRSRealtimeDynamics.h
 TestDynamics.h
)
SET(SOURCES
 SteeringWheel.cpp
 CarSound.cpp
 EngineSound.cpp
 Vehicle.cpp
 RemoteVehicle.cpp
 Porsche.cpp
 FKFS.cpp
 FFWheel.cpp
 CAN.cpp
 CanOpenBus.cpp
 ServoStar.cpp
 PorscheSteeringWheel.cpp
 InputDevice.cpp
 Keyboard.cpp
 EinspurDynamik.cpp
 ITMDynamics.cpp
 FKFSDynamics.cpp
 PorscheController.cpp
 PorscheRealtimeDynamics.cpp
 VehicleDynamics.cpp
 HLRSRealtimeDynamics.cpp
 TestDynamics.cpp
)
#IF(Boost_FOUND)
# SET(HEADERS ${HEADERS} HLRSRealtimeDynamics.h)
# SET(SOURCES ${SOURCES} HLRSRealtimeDynamics.cpp)
#ENDIF(Boost_FOUND)
IF(WIN32)
SET(SOURCES ${SOURCES} TempWindow.cpp)
ENDIF(WIN32)


INCLUDE_DIRECTORIES(
)


OPTION (COVISE_USE_XENOMAI "Use Xenomai" ON)
if(XENOMAI_FOUND AND COVISE_USE_XENOMAI)
 SET(HEADERS ${HEADERS}
  EinspurDynamikRealtime.h
  FourWheelDynamicsRealtime.h
 )
 SET(SOURCES  ${SOURCES}
  EinspurDynamikRealtime.cpp
  FourWheelDynamicsRealtime.cpp
 )

 ADD_DEFINITIONS(${XENOMAI_DEFINITIONS})

 INCLUDE_DIRECTORIES(
   ${XENOMAI_INCLUDE_DIR}
   ../../../DrivingSim/gaalet/include
   ../../../DrivingSim/VehicleUtil
 )
endif(XENOMAI_FOUND AND COVISE_USE_XENOMAI)



COVER_ADD_PLUGIN_TARGET(SteeringWheel ${HEADERS} ${SOURCES})

if(XENOMAI_FOUND AND COVISE_USE_XENOMAI)
  ADD_COVISE_COMPILE_FLAGS(SteeringWheel "-g0 -O3 -fno-strict-aliasing")
endif(XENOMAI_FOUND AND COVISE_USE_XENOMAI)
IF(WIN32)
TARGET_LINK_LIBRARIES(SteeringWheel
   ${EXTRA_LIBS}
   coOpenVehicleUtil
   dinput8 comctl32
)
#dxguid nor required on Win7 (added #include <InitGuid.h> #define DIRECTINPUT_VERSION 0x0800 #include <dinput.h> #include <dsound.h> instead)
ELSE(WIN32)
TARGET_LINK_LIBRARIES(SteeringWheel
    ${EXTRA_LIBS}
)
ENDIF(WIN32)
COVISE_WNOERROR(SteeringWheel)
COVER_INSTALL_PLUGIN(SteeringWheel)

if(XENOMAI_FOUND AND COVISE_USE_XENOMAI)
  if(COVISE_USE_MERCURY)
     TARGET_LINK_LIBRARIES(SteeringWheel
     ${XENOMAI_LIBRARY_ALCHEMY}
     ${XENOMAI_LIBRARY_COPPERPLATE}
     ${CMAKE_THREAD_LIBS_INIT}
     )
   else(COVISE_USE_MERCURY)
     TARGET_LINK_LIBRARIES(SteeringWheel
     ${XENOMAI_LIBRARY_ALCHEMY}
     ${XENOMAI_LIBRARY_COPPERPLATE}
     )
   endif(COVISE_USE_MERCURY)
endif(XENOMAI_FOUND AND COVISE_USE_XENOMAI)
