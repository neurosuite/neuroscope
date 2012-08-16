project(neuroscope)
cmake_minimum_required(VERSION 2.8.6)
find_package(Qt4 REQUIRED)
find_package(LibXml2 REQUIRED)
add_definitions(${QT_DEFINITIONS} ${KDE3_DEFINITIONS})

# tell cmake where to search for Qt/KDE headers:
include_directories(${QT_INCLUDE_DIR} ${CMAKE_CURRENT_BINARY_DIR})

#FIXME
add_definitions(-fexceptions)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config-neuroscope.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-neuroscope.h )

set(CMAKE_INCLUDE_CURRENT_DIR TRUE)
set(CMAKE_AUTOMOC TRUE)

add_subdirectory(src)
add_subdirectory(po)
add_subdirectory(doc)
