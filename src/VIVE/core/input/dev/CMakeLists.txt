set(PLUGIN_CATEGORY "Input")
add_subdirectory(dtrack) 
add_subdirectory(NatNet) 
add_subdirectory(mousebuttons) 
#don't build legacy tracking system, port it to the new system if you need anything
#add_subdirectory(legacy)
#add_subdirectory(vrc) 
add_subdirectory(rift) 
add_subdirectory(mike)
add_subdirectory(vrpn)
add_subdirectory(cavelib)
add_subdirectory(zspace)
add_subdirectory(spacenavigator)
add_subdirectory(Joystick) 
IF(WIN32)
  add_subdirectory(windowsHID) 
ENDIF(WIN32)
