if (COVISE_BUILD_ONLY_COVER)
    return()
endif()

add_subdirectory(CovisePluginUtil)

set(PLUGIN_CATEGORY "Covise")
add_subdirectory(COVISE)
ADD_SUBDIRECTORY(COVERScript)
#add_subdirectory(ElectricField)
add_subdirectory(Famu)
ADD_SUBDIRECTORY(FileBrowserParam)
add_subdirectory(PickSphere)
add_subdirectory(SceneEditor)
