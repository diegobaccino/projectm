# Install script for directory: C:/Source/projectm/vcpkg_installed/vcpkg/blds/projectm-eval/src/v1.0.6-2857e8f355.clean/projectm-eval

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "C:/Source/projectm/vcpkg_installed/vcpkg/pkgs/projectm-eval_x64-windows/debug")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Debug")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "OFF")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "C:/Source/projectm/vcpkg_installed/vcpkg/blds/projectm-eval/x64-windows-dbg/projectm-eval/projectM_eval.lib")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  include("C:/Source/projectm/vcpkg_installed/vcpkg/blds/projectm-eval/x64-windows-dbg/projectm-eval/CMakeFiles/projectM_eval.dir/install-cxx-module-bmi-Debug.cmake" OPTIONAL)
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/projectm-eval" TYPE FILE FILES "C:/Source/projectm/vcpkg_installed/vcpkg/blds/projectm-eval/src/v1.0.6-2857e8f355.clean/projectm-eval/api/projectm-eval.h")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Devel" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/projectM-Eval" TYPE FILE FILES
    "C:/Source/projectm/vcpkg_installed/vcpkg/blds/projectm-eval/x64-windows-dbg/projectm-eval/projectM-Eval/projectM-EvalConfigVersion.cmake"
    "C:/Source/projectm/vcpkg_installed/vcpkg/blds/projectm-eval/x64-windows-dbg/projectm-eval/projectM-Eval/projectM-EvalConfig.cmake"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Devel" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/projectM-Eval/projectM-EvalTargets.cmake")
    file(DIFFERENT _cmake_export_file_changed FILES
         "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/projectM-Eval/projectM-EvalTargets.cmake"
         "C:/Source/projectm/vcpkg_installed/vcpkg/blds/projectm-eval/x64-windows-dbg/projectm-eval/CMakeFiles/Export/4a253019dd725df2348e2ec422cb83ac/projectM-EvalTargets.cmake")
    if(_cmake_export_file_changed)
      file(GLOB _cmake_old_config_files "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/projectM-Eval/projectM-EvalTargets-*.cmake")
      if(_cmake_old_config_files)
        string(REPLACE ";" ", " _cmake_old_config_files_text "${_cmake_old_config_files}")
        message(STATUS "Old export file \"$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/projectM-Eval/projectM-EvalTargets.cmake\" will be replaced.  Removing files [${_cmake_old_config_files_text}].")
        unset(_cmake_old_config_files_text)
        file(REMOVE ${_cmake_old_config_files})
      endif()
      unset(_cmake_old_config_files)
    endif()
    unset(_cmake_export_file_changed)
  endif()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/projectM-Eval" TYPE FILE FILES "C:/Source/projectm/vcpkg_installed/vcpkg/blds/projectm-eval/x64-windows-dbg/projectm-eval/CMakeFiles/Export/4a253019dd725df2348e2ec422cb83ac/projectM-EvalTargets.cmake")
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/projectM-Eval" TYPE FILE FILES "C:/Source/projectm/vcpkg_installed/vcpkg/blds/projectm-eval/x64-windows-dbg/projectm-eval/CMakeFiles/Export/4a253019dd725df2348e2ec422cb83ac/projectM-EvalTargets-debug.cmake")
  endif()
endif()

