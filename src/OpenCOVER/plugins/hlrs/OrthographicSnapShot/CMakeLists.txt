if(NOT COVISE_USE_QT)
    return()
endif()

SET(HEADERS
  OrthographicSnapShot.h
)
SET(SOURCES
  OrthographicSnapShot.cpp
)
cover_add_plugin(OrthographicSnapShot)
#COVER_INSTALL_PLUGIN(VRAnim)
