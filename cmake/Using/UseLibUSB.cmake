MACRO(USE_LIBUSB)
  FIND_PACKAGE(LIBUSB)
  IF ((NOT LIBUSB_FOUND) AND (${ARGC} LESS 1))
    USING_MESSAGE("Skipping because of missing LIBUSB")
    RETURN()
  ENDIF((NOT LIBUSB_FOUND) AND (${ARGC} LESS 1))
  IF(NOT LIBUSB_USED AND LIBUSB_FOUND)
    SET(LIBUSB_USED TRUE)
    INCLUDE_DIRECTORIES(SYSTEM ${LIBUSB_INCLUDE_DIR})
    SET(EXTRA_LIBS ${EXTRA_LIBS} ${LIBUSB_LIBRARIES})
    USING_MESSAGE("found LIBUSB")
  ENDIF()
ENDMACRO(USE_LIBUSB)
  
