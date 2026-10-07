# ************************************************************************ #
#                        CMakeLists.txt  -  description                    #
#                             -------------------                          #
#    copyright            : (C) 2015 by Florian Franzen                    #
#                         : (C) 2025 by Joscha Schmiedt                    #
# ************************************************************************ #

# ************************************************************************ #
#                                                                          #
#    This program is free software; you can redistribute it and/or modify  #
#    it under the terms of the GNU General Public License as published by  #
#    the Free Software Foundation; either version 3 of the License, or     #
#    (at your option) any later version.                                   #
#                                                                          #
# ************************************************************************ #
cmake_minimum_required(VERSION 3.16)
project(neuroscope
    VERSION 3.0.0
    DESCRIPTION "Viewer for neurophysiological and behavioral data"
    HOMEPAGE_URL "https://neurosuite.github.io"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

################################################################################
# Options
################################################################################
option(WITH_CEREBUS
       "Enable online streaming from Blackrock Cerebus systems (needs cbsdk)"
       OFF)
option(NEUROSCOPE_BUNDLE_NEUROSUITE
       "Build libneurosuite as part of this project instead of using an installed copy"
       OFF)
set(NEUROSUITE_GIT_TAG "v3.0.0" CACHE STRING
    "libneurosuite tag to fetch when NEUROSCOPE_BUNDLE_NEUROSUITE is ON")

if(APPLE)
    option(APPBUNDLE
           "Enable this if you want to produce an .app bundle."
           OFF)
endif()

################################################################################
# CMake build environment
################################################################################
include(GNUInstallDirs)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Find a bundled libneurosuite next to the binary (AppImage, macOS bundle, relocatable installs)
if(APPBUNDLE)
    set(NEUROSUITE_BUNDLED_LIBDIR "neuroscope.app/Contents/Frameworks")
    set(CMAKE_INSTALL_RPATH "@executable_path/../Frameworks")
elseif(APPLE)
    set(CMAKE_INSTALL_RPATH "@loader_path/../${CMAKE_INSTALL_LIBDIR}")
elseif(UNIX)
    set(CMAKE_INSTALL_RPATH "$ORIGIN/../${CMAKE_INSTALL_LIBDIR}")
endif()

# Ship the MSVC runtime next to the executable on Windows.
if(WIN32)
    set(CMAKE_INSTALL_SYSTEM_RUNTIME_DESTINATION ${CMAKE_INSTALL_BINDIR})
    include(InstallRequiredSystemLibraries)
endif()

if(MSVC)
    add_definitions(-Dfseeko64=_fseeki64)
    add_definitions(-Dftello64=_ftelli64)
    add_definitions(-DNOMINMAX)
endif()

if(APPLE)
    # File operations are always 64 bit on macOS.
    add_definitions(-Dfseeko64=fseeko)
    add_definitions(-Dftello64=ftello)
endif()

################################################################################
# Qt configuration
################################################################################
set(CMAKE_AUTOMOC ON)
add_definitions(-DQT_USE_QSTRINGBUILDER)

find_package(Qt6 6.4 REQUIRED COMPONENTS Widgets PrintSupport Xml)

################################################################################
# Neurosuite library
################################################################################
if(NEUROSCOPE_BUNDLE_NEUROSUITE)
    include(FetchContent)
    FetchContent_Declare(neurosuite
        GIT_REPOSITORY https://github.com/neurosuite/libneurosuite.git
        GIT_TAG ${NEUROSUITE_GIT_TAG}
    )
    FetchContent_MakeAvailable(neurosuite)
else()
    find_package(neurosuite 3.0 REQUIRED)
endif()

################################################################################
# Blackrock Cerebus SDK (untested with Qt6; see CHANGELOG)
################################################################################
if(WITH_CEREBUS)
    find_package(cbsdk REQUIRED)
    add_definitions(-DWITH_CEREBUS)
endif()

################################################################################
# .app bundle specific settings
################################################################################
if(APPBUNDLE)
    set(MACOSX_BUNDLE_BUNDLE_NAME "NeuroScope")
    set(MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}")
    set(MACOSX_BUNDLE_INFO_STRING "NeuroScope ${PROJECT_VERSION}")
    set(MACOSX_BUNDLE_LONG_VERSION_STRING "${PROJECT_VERSION}")
    set(MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}")
    set(MACOSX_BUNDLE_ICON_FILE "neuroscope.icns")
    set(MACOSX_BUNDLE_GUI_IDENTIFIER "io.github.neurosuite.NeuroScope")
endif()

################################################################################
# Documentation location (looked up relative to the executable at runtime)
################################################################################
if(APPBUNDLE)
    set(NEUROSCOPE_DOC_INSTALL_DIR "neuroscope.app/Contents/Documentation")
    set(NEUROSCOPE_DOC_RELATIVE_PATH "../Documentation")
else()
    set(NEUROSCOPE_DOC_INSTALL_DIR "${CMAKE_INSTALL_DATADIR}/doc/neuroscope/HTML/en")
    file(RELATIVE_PATH NEUROSCOPE_DOC_RELATIVE_PATH
         "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}"
         "${CMAKE_INSTALL_PREFIX}/${NEUROSCOPE_DOC_INSTALL_DIR}")
endif()

configure_file(config-neuroscope.h.cmake
               src/config-neuroscope.h)

################################################################################
# Sources and documentation
################################################################################
add_subdirectory(src)
add_subdirectory(doc)

################################################################################
# CPack configuration
################################################################################
include(cmake/CPackConfig.cmake)
