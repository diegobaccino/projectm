#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "projectM::ns-eel2" for configuration "Release"
set_property(TARGET projectM::ns-eel2 APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(projectM::ns-eel2 PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "C"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/projectM_ns-eel2.lib"
  )

list(APPEND _cmake_import_check_targets projectM::ns-eel2 )
list(APPEND _cmake_import_check_files_for_projectM::ns-eel2 "${_IMPORT_PREFIX}/lib/projectM_ns-eel2.lib" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
