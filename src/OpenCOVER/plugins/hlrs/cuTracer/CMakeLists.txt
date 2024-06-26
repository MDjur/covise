FIND_PACKAGE(CUDPP)

USING(CUDPP GLEW)
IF(COVISE_BUILD_ONLY_COVER OR NOT CUDA_FOUND OR NOT COVISE_USE_CUDA)
  MESSAGE("Skipping OpenCOVER - cuTracer plugin since CUDA was not found")
  RETURN()
ENDIF()

INCLUDE_DIRECTORIES(
  ${CMAKE_SOURCE_DIR}/src/module/hlrs/cuIsoSurfaceUSG
  ${CUDA_INCLUDE_DIRS}
  ${CUDA_SDK_ROOT_DIR}/common/inc
  ${CUDA_SDK_ROOT_DIR}/common/inc/cudpp
)

SET(cuTracer_CUDA_SOURCES
  cutrace_kernels.cu
)

SET(HEADERS
  cuTracer.h
  utils.h
  tree.h
  bb.h
)
SET(SOURCES
  cuTracer.cpp
  tree.cpp
  utils.cpp
  ${cuTracer_CUDA_SOURCES}
)

ADD_DEFINITIONS(-DUSE_VBO)
ADD_DEFINITIONS(-DRENDER_STATE)

SET(EXTRA_LIBS ${EXTRA_LIBS} ${CUDA_LIBRARIES})

cover_add_plugin(cuTracer ${SOURCES})
TARGET_LINK_LIBRARIES(cuTracer coCore coOpenVRUI ${EXTRA_LIBS})
