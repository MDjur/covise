#OpenCOVER plugins general Variant
using(VRML)
SET(HEADERS
  coVRBoxOfInterest.h
  Variant.h
  VariantPlugin.h
  VariantUI.h
  VariantAR.h
  VrmlNodeVariant.h
)

SET(SOURCES
  coVRBoxOfInterest.cpp
  Variant.cpp
  VariantAR.cpp
  VariantPlugin.cpp
  VariantUI.cpp
  VrmlNodeVariant.cpp
)

cover_add_plugin(Variant)
qt_use_modules(Variant Xml)
