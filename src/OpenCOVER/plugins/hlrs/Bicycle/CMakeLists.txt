USING(LibUSB1)
USING(Pthreads)
using(VRML)

if(APPLE)
   return()
endif()

SET(HEADERS
  AVRserialcom.h
  Tacx.h
  Bicycle.h
  FlightGear.h
  Skateboard.h
)
SET(SOURCES
  AVRserialcom.cpp
  Tacx.cpp
  Bicycle.cpp
  FlightGear.cpp
  Skateboard.cpp
)
cover_add_plugin(Bicycle)
TARGET_LINK_LIBRARIES(Bicycle coUtil ${EXTRA_LIBS})
#COVER_INSTALL_PLUGIN(Bicycle)

SET(TS_HEADERS
  Tacx.h
)
SET(TS_SOURCES
  TacxServer.cpp
  Tacx.cpp
)

ADD_COVISE_EXECUTABLE(TacxServer  ${TS_SOURCES} ${TS_HEADERS})

TARGET_LINK_LIBRARIES(TacxServer ${EXTRA_LIBS} coNet coUtil coConfig )
