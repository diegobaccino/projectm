# Install script for directory: C:/Source/projectm/src/playlist

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "C:/Program Files (x86)/libprojectM")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Release")
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
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Devel" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY OPTIONAL FILES "C:/Source/projectm/build-copilot-ninja-ui/src/playlist/projectM-4-playlist.lib")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Runtime" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" TYPE SHARED_LIBRARY FILES "C:/Source/projectm/build-copilot-ninja-ui/src/playlist/projectM-4-playlist.dll")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Devel" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/projectM-4" TYPE FILE FILES
    "C:/Source/projectm/build-copilot-ninja-ui/src/playlist/include/projectM-4/projectM_playlist_export.h"
    "C:/Source/projectm/src/playlist/api/projectM-4/playlist.h"
    "C:/Source/projectm/src/playlist/api/projectM-4/playlist_callbacks.h"
    "C:/Source/projectm/src/playlist/api/projectM-4/playlist_core.h"
    "C:/Source/projectm/src/playlist/api/projectM-4/playlist_filter.h"
    "C:/Source/projectm/src/playlist/api/projectM-4/playlist_items.h"
    "C:/Source/projectm/src/playlist/api/projectM-4/playlist_memory.h"
    "C:/Source/projectm/src/playlist/api/projectM-4/playlist_playback.h"
    "C:/Source/projectm/src/playlist/api/projectM-4/playlist_types.h"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Devel" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/projectM4Playlist" TYPE FILE FILES
    "C:/Source/projectm/build-copilot-ninja-ui/src/playlist/libprojectMPlaylist/projectM4PlaylistConfigVersion.cmake"
    "C:/Source/projectm/build-copilot-ninja-ui/src/playlist/libprojectMPlaylist/projectM4PlaylistConfig.cmake"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Devel" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/projectM4Playlist/projectM4PlaylistTargets.cmake")
    file(DIFFERENT _cmake_export_file_changed FILES
         "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/projectM4Playlist/projectM4PlaylistTargets.cmake"
         "C:/Source/projectm/build-copilot-ninja-ui/src/playlist/CMakeFiles/Export/fce45fea70d0901e2efaeb2cc31668a0/projectM4PlaylistTargets.cmake")
    if(_cmake_export_file_changed)
      file(GLOB _cmake_old_config_files "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/projectM4Playlist/projectM4PlaylistTargets-*.cmake")
      if(_cmake_old_config_files)
        string(REPLACE ";" ", " _cmake_old_config_files_text "${_cmake_old_config_files}")
        message(STATUS "Old export file \"$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/projectM4Playlist/projectM4PlaylistTargets.cmake\" will be replaced.  Removing files [${_cmake_old_config_files_text}].")
        unset(_cmake_old_config_files_text)
        file(REMOVE ${_cmake_old_config_files})
      endif()
      unset(_cmake_old_config_files)
    endif()
    unset(_cmake_export_file_changed)
  endif()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/projectM4Playlist" TYPE FILE FILES "C:/Source/projectm/build-copilot-ninja-ui/src/playlist/CMakeFiles/Export/fce45fea70d0901e2efaeb2cc31668a0/projectM4PlaylistTargets.cmake")
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/projectM4Playlist" TYPE FILE FILES "C:/Source/projectm/build-copilot-ninja-ui/src/playlist/CMakeFiles/Export/fce45fea70d0901e2efaeb2cc31668a0/projectM4PlaylistTargets-release.cmake")
  endif()
endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
if(CMAKE_INSTALL_LOCAL_ONLY)
  file(WRITE "C:/Source/projectm/build-copilot-ninja-ui/src/playlist/install_local_manifest.txt"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
endif()
