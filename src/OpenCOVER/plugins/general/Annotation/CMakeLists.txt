# OpenCOVER plugins general Annotation

# INCLUDE_DIRECTORIES( )

SET(HEADERS
  AnnotationPlugin.h
  Annotation.h
  AnnotationSensor.h
)

SET(SOURCES
  AnnotationPlugin.cpp
  Annotation.cpp 
  AnnotationSensor.cpp
)

add_subdirectory(lib)
cover_add_plugin(Annotation $<TARGET_OBJECTS:coGraphQL> coCurlHTTPClient)