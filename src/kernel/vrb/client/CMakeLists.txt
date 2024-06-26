USING(BOOST)
USING(Pthreads)
ADD_DEFINITIONS(-DCOVISE_VRB)

SET(VRBCLIENT_SOURCES
  ClientRegistryClass.cpp
  ClientRegistryVariable.cpp
  LaunchRequest.cpp
  SharedState.cpp
  SharedStateManager.cpp
  VRBClient.cpp
  VRBMessage.cpp
  VrbClientRegistry.cpp
  VrbCredentials.cpp
  AsyncClient.cpp
)

SET(VRBCLIENT_HEADERS
  ClientRegistryClass.h
  ClientRegistryVariable.h
  LaunchRequest.h
  SharedMap.h
  SharedState.h
  SharedStateManager.h
  VRBClient.h
  VRBMessage.h
  VrbClientRegistry.h
  VrbCredentials.h
  AsyncClient.h
)

ADD_COVISE_LIBRARY(coVRBClient ${COVISE_LIB_TYPE} ${VRBCLIENT_SOURCES} ${VRBCLIENT_HEADERS})
TARGET_LINK_LIBRARIES(coVRBClient coVRB coNet coConfig)
COVISE_INSTALL_TARGET(coVRBClient)
COVISE_INSTALL_HEADERS(vrb/client ${VRBCLIENT_HEADERS})
