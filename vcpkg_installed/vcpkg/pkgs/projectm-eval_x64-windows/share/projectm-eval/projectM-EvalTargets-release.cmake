#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "projectM::Eval" for configuration "Release"
set_property(TARGET projectM::Eval APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(projectM::Eval PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "C"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/projectM_eval.lib"
  )

list(APPEND _cmake_import_check_targets projectM::Eval )
list(APPEND _cmake_import_check_files_for_projectM::Eval "${_IMPORT_PREFIX}/lib/projectM_eval.lib" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
