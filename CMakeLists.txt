# ************************************************************************ #
#                        CMakeLists.txt  -  description                    #
#                             -------------------                          #
#    copyright            : (C) 2015 by Florian Franzen                    #
# ************************************************************************ #

# ************************************************************************ #
#                                                                          #
#    This program is free software; you can redistribute it and/or modify  #
#    it under the terms of the GNU General Public License as published by  #
#    the Free Software Foundation; either version 3 of the License, or     #
#    (at your option) any later version.                                   #
#                                                                          #
# ************************************************************************ #
project(neuroscope CXX)
cmake_minimum_required(VERSION 2.8.12)

set(PROJECT_VERSION_MAJOR "2")
set(PROJECT_VERSION_MINOR "1")
set(PROJECT_VERSION_PATCH "0")
set(PROJECT_VERSION
    "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}"
)

# Supported config flags
option(WITH_QT4
       "Enable to build against Qt4 (default: Qt5)"
       OFF)

option(WITH_CEREBUS
      "Enable if you want online Cerebus supports"
      OFF)

option(REGENERATE_DOC
       "Enable if you want to regenerate doc need kde4"
       OFF)

if(APPLE)
    option(APPBUNDLE
           "Enable this if you want to produce an .app bundle."
           OFF)
endif(APPLE)

################################################################################
# CMake build environment
################################################################################
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Include our own module dir, so we can use package configs for cmake v3.0.0
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules")

if (MSVC)
    # Qt disables the native wchar_t type, do it too to avoid linking issues
    #set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Zc:wchar_t-" )
    add_definitions(-Dfseeko64=_fseeki64)
    add_definitions(-DNOMINMAX)
endif()

if(APPLE)
    # Adjust code on OS X since file operation are always 64bit there.
    add_definitions(-Dfseeko64=fseeko)
    add_definitions(-Dftello64=ftello)
endif()

################################################################################
# QT Configurations
################################################################################
# Configure Qt functionality
set(CMAKE_AUTOMOC ON)
add_definitions(-DQT_USE_QSTRINGBUILDER)

# Find correct Qt library
if(WITH_QT4)
    find_package(Qt4 "4.8.0" REQUIRED QtGui QtXml)
else()
    find_package(Qt5 COMPONENTS Widgets PrintSupport Xml WebKitWidgets REQUIRED)
endif()

################################################################################
# Neurosuite library config
################################################################################
find_package(neurosuite 2.0.0 REQUIRED)

################################################################################
# Blackrock Cerebus SDK config
################################################################################
if(WITH_CEREBUS)
    find_package(cbsdk REQUIRED)
    add_definitions(-DWITH_CEREBUS)
endif(WITH_CEREBUS)

################################################################################
# .app bundle specific settings
################################################################################
if(APPBUNDLE)
    set(MACOSX_BUNDLE_BUNDLE_NAME "${PROJECT_NAME}")
    set(MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_NAME} ${PROJECT_VERSION}")
    set(MACOSX_BUNDLE_INFO_STRING "${PROJECT_NAME} ${PROJECT_VERSION}")
    set(MACOSX_BUNDLE_LONG_VERSION_STRING "${PROJECT_NAME} ${PROJECT_VERSION}")
    set(MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}")
    set(MACOSX_BUNDLE_ICON_FILE "neuroscope.icns")
    set(MACOSX_BUNDLE_GUI_IDENTIFIER "io.github.neurosuite.neuroscope")
endif()

################################################################################
# Documentation build config
################################################################################
if(REGENERATE_DOC)
   find_package(KDE4 REQUIRED)
   find_program(DOCBOOK2HTML_EXECUTABLE NAMES docbook2html)
   if(NOT DOCBOOK2HTML_EXECUTABLE)
       message(FATAL_ERROR
               "Could not find docbook2html for documentation generation.")
   endif(DOCBOOK2HTML_EXECUTABLE)
endif()

################################################################################
# Documentation configuration
################################################################################
if(APPBUNDLE)
    set(KLUSTERS_DOC_INSTALL_PREFIX  "../../..")
    set(KLUSTERS_DOC_PREFIX "neuroscope.app/Contents/Documentation")
else()
    set(KLUSTERS_DOC_INSTALL_PREFIX  "..")
    set(KLUSTERS_DOC_PREFIX "share/doc/neuroscope/HTML/en")
endif()

configure_file(config-neuroscope.h.cmake
               src/config-neuroscope.h )

################################################################################
# Documentation configuration
################################################################################
add_subdirectory(src)
add_subdirectory(doc)

################################################################################
# CPack configuration
################################################################################
include(PackNeurosuite)
# Init CPack variables
neurosuite_cpack_init(${PROJECT_NAME}
                      ${PROJECT_VERSION}
                      "Florian Franzen <FlorianFranzen@gmail.com"
                      "Viewer for neurophysiological and behavioral data"
                       "${CMAKE_CURRENT_SOURCE_DIR}/cpack/description")

# Use NSIS on Windows
neurosuite_cpack_nsis("neuroscope"
                      "Neuroscope"
                      "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.txt")

# Use DMG if bundled to .app
if(APPBUNDLE)
    neurosuite_cpack_dmg()
endif()

# Use deb and rpm on linux
set(COMMON_DEPENDECIES "libneurosuite (>= 2.0.0)")
if(WITH_CEREBUS)
    set(COMMON_DEPENDECIES "${COMMON_DEPENDECIES}, libcbsdk")
endif()
if(WITH_QT4)
    neurosuite_cpack_ubuntu(
        "${COMMON_DEPENDECIES}, libqtgui4, libqt4-xml, libqt4-webkit4"
        "${CMAKE_CURRENT_SOURCE_DIR}/cpack"
    )
    neurosuite_cpack_suse(
        "${COMMON_DEPENDECIES}, libqt4, libqt4-x11, libQtWebKit4"
        "${CMAKE_CURRENT_SOURCE_DIR}/cpack"
    )
    neurosuite_cpack_fedora(
        "${COMMON_DEPENDECIES}, qt, qt-mobility, qtwebkit, qt-x11"
        "${CMAKE_CURRENT_SOURCE_DIR}/cpack"
    )
    neurosuite_cpack_scientific(
        "${COMMON_DEPENDECIES}, libQtCore, libQtGui, libQtNetwork, libQtWebKit"
        OFF
    )
else()
    neurosuite_cpack_ubuntu(
        "${COMMON_DEPENDECIES}, libqt5widgets5, libqt5printsupport5, libqt5concurrent5, libqt5xml5, libqt5webkit5, libqt5sensors5, libqt5location5"
        "${CMAKE_CURRENT_SOURCE_DIR}/cpack"
    )
endif()

include(CPack)
