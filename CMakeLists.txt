project(neuroscope)
cmake_minimum_required(VERSION 2.8.6)
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules")

set (QT_MIN_VERSION "4.8.0")
find_package(Qt4 REQUIRED)
include(${QT_USE_FILE})
include(MacroEnsureVersion)

include(CheckCXXCompilerFlag)


find_package(LibKlustersShared 1.0.2 REQUIRED)
add_definitions(${QT_DEFINITIONS} )
set( QT_USE_XML TRUE)


# tell cmake where to search for Qt/KDE headers:
include_directories(${QT_INCLUDE_DIR} ${CMAKE_CURRENT_BINARY_DIR} ${QT_QTXML_INCLUDE_DIR} ${LIBKLUSTERSSHARED_INCLUDE_DIR})

# This macro is for internal use only
# Return the directories present in gcc's include path.
macro(_DETERMINE_GCC_SYSTEM_INCLUDE_DIRS _lang _result)
  set(${_result})
  set(_gccOutput)
  file(WRITE "${CMAKE_BINARY_DIR}/CMakeFiles/dummy" "\n" )
  execute_process(COMMAND ${CMAKE_C_COMPILER} -v -E -x ${_lang} -dD dummy
                  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/CMakeFiles
                  ERROR_VARIABLE _gccOutput
                  OUTPUT_VARIABLE _gccStdout )
  file(REMOVE "${CMAKE_BINARY_DIR}/CMakeFiles/dummy")

  if( "${_gccOutput}" MATCHES "> search starts here[^\n]+\n *(.+) *\n *End of (search) list" )
    SET(${_result} ${CMAKE_MATCH_1})
    STRING(REPLACE "\n" " " ${_result} "${${_result}}")
    SEPARATE_ARGUMENTS(${_result})
  ENDIF( "${_gccOutput}" MATCHES "> search starts here[^\n]+\n *(.+) *\n *End of (search) list" )
ENDMACRO(_DETERMINE_GCC_SYSTEM_INCLUDE_DIRS _lang)
#laurent check from kde
if (CMAKE_COMPILER_IS_GNUCXX)
  if (NOT APPLE)
    set ( CMAKE_SHARED_LINKER_FLAGS "-Wl,--fatal-warnings -Wl,--no-undefined  ${CMAKE_SHARED_LINKER_FLAGS}")
    set ( CMAKE_MODULE_LINKER_FLAGS "-Wl,--fatal-warnings -Wl,--no-undefined  ${CMAKE_MODULE_LINKER_FLAGS}")
  endif ()
   
   _DETERMINE_GCC_SYSTEM_INCLUDE_DIRS(c++ _dirs)
   set(CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES
       ${CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES} ${_dirs})

   set (KDE4_ENABLE_EXCEPTIONS "-fexceptions -UQT_NO_EXCEPTIONS")
   # Select flags.
   set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g -DNDEBUG -DQT_NO_DEBUG")
   set(CMAKE_CXX_FLAGS_RELEASE        "-O2 -DNDEBUG -DQT_NO_DEBUG")
   set(CMAKE_CXX_FLAGS_DEBUG          "-g -O2 -fno-reorder-blocks -fno-schedule-insns -fno-inline")
   set(CMAKE_CXX_FLAGS_DEBUGFULL      "-g3 -fno-inline")
   set(CMAKE_CXX_FLAGS_PROFILE        "-g3 -fno-inline -ftest-coverage -fprofile-arcs")
   set(CMAKE_C_FLAGS_RELWITHDEBINFO   "-O2 -g -DNDEBUG -DQT_NO_DEBUG")
   set(CMAKE_C_FLAGS_RELEASE          "-O2 -DNDEBUG -DQT_NO_DEBUG")
   set(CMAKE_C_FLAGS_DEBUG            "-g -O2 -fno-reorder-blocks -fno-schedule-insns -fno-inline")
   set(CMAKE_C_FLAGS_DEBUGFULL        "-g3 -fno-inline")
   set(CMAKE_C_FLAGS_PROFILE          "-g3 -fno-inline -ftest-coverage -fprofile-arcs")

  check_cxx_compiler_flag(-fvisibility=hidden __KDE_HAVE_GCC_VISIBILITY)
  set( __KDE_HAVE_GCC_VISIBILITY ${__KDE_HAVE_GCC_VISIBILITY} CACHE BOOL "GCC support for hidden visibility")

   # get the gcc version
   exec_program(${CMAKE_C_COMPILER} ARGS ${CMAKE_C_COMPILER_ARG1} --version OUTPUT_VARIABLE _gcc_version_info)

   string (REGEX MATCH "[345]\\.[0-9]\\.[0-9]" _gcc_version "${_gcc_version_info}")
   # gcc on mac just reports: "gcc (GCC) 3.3 20030304 ..." without the patch level, handle this here:
   if (NOT _gcc_version)
      string (REGEX MATCH ".*\\(GCC\\).* ([34]\\.[0-9]) .*" "\\1.0" _gcc_version "${gcc_on_macos}")
      if (gcc_on_macos)
        string (REGEX REPLACE ".*\\(GCC\\).* ([34]\\.[0-9]) .*" "\\1.0" _gcc_version "${_gcc_version_info}")
      endif (gcc_on_macos)
   endif (NOT _gcc_version)

   if (_gcc_version)
      macro_ensure_version("4.1.0" "${_gcc_version}" GCC_IS_NEWER_THAN_4_1)
      macro_ensure_version("4.2.0" "${_gcc_version}" GCC_IS_NEWER_THAN_4_2)
      macro_ensure_version("4.3.0" "${_gcc_version}" GCC_IS_NEWER_THAN_4_3)
   endif (_gcc_version)

   set(_GCC_COMPILED_WITH_BAD_ALLOCATOR FALSE)
   if (GCC_IS_NEWER_THAN_4_1)
      exec_program(${CMAKE_C_COMPILER} ARGS ${CMAKE_C_COMPILER_ARG1} -v OUTPUT_VARIABLE _gcc_alloc_info)
      string(REGEX MATCH "(--enable-libstdcxx-allocator=mt)" _GCC_COMPILED_WITH_BAD_ALLOCATOR "${_gcc_alloc_info}")
   endif (GCC_IS_NEWER_THAN_4_1)

   if (__KDE_HAVE_GCC_VISIBILITY AND GCC_IS_NEWER_THAN_4_1 AND NOT _GCC_COMPILED_WITH_BAD_ALLOCATOR AND NOT WIN32)
      set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden")
      set (KDE4_C_FLAGS "-fvisibility=hidden")
      # check that Qt defines Q_DECL_EXPORT as __attribute__ ((visibility("default")))
      # if it doesn't and KDE compiles with hidden default visibiltiy plugins will break
      set(_source "#include <QtCore/QtGlobal>\n int main()\n {\n #ifndef QT_VISIBILITY_AVAILABLE \n #error QT_VISIBILITY_AVAILABLE is not available\n #endif \n }\n")
      set(_source_file ${CMAKE_BINARY_DIR}/CMakeTmp/check_qt_visibility.cpp)
      file(WRITE "${_source_file}" "${_source}")
      set(_include_dirs "-DINCLUDE_DIRECTORIES:STRING=${QT_INCLUDES}")

      try_compile(_compile_result ${CMAKE_BINARY_DIR} ${_source_file} CMAKE_FLAGS "${_include_dirs}" COMPILE_OUTPUT_VARIABLE _compile_output_var)

      if(NOT _compile_result)
         message(FATAL_ERROR "Qt compiled without support for -fvisibility=hidden. This will break plugins and linking of some applications. Please fix your Qt installation.")
      endif(NOT _compile_result)

      if (GCC_IS_NEWER_THAN_4_2)
          set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility-inlines-hidden")
          MESSAGE(STATUS "compile with visibility hidden")
      endif (GCC_IS_NEWER_THAN_4_2)
   else (__KDE_HAVE_GCC_VISIBILITY AND GCC_IS_NEWER_THAN_4_1 AND NOT _GCC_COMPILED_WITH_BAD_ALLOCATOR AND NOT WIN32)
      set (__KDE_HAVE_GCC_VISIBILITY 0)
   endif (__KDE_HAVE_GCC_VISIBILITY AND GCC_IS_NEWER_THAN_4_1 AND NOT _GCC_COMPILED_WITH_BAD_ALLOCATOR AND NOT WIN32)

endif()


if (MSVC)
   set (KDE4_ENABLE_EXCEPTIONS -EHsc)
endif()   
add_definitions(-DQT_USE_FAST_CONCATENATION -DQT_USE_FAST_OPERATOR_PLUS)

add_definitions(${KDE4_ENABLE_EXCEPTIONS})

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config-neuroscope.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-neuroscope.h )

set(CMAKE_INCLUDE_CURRENT_DIR TRUE)
set(CMAKE_AUTOMOC TRUE)

add_subdirectory(src)
add_subdirectory(po)
add_subdirectory(doc)

INCLUDE(InstallRequiredSystemLibraries)
if(WIN32)
  set(CPACK_GENERATOR "NSIS")
  set(CPACK_PACKAGE_EXECUTABLES "neuroscope" "neuroscope")
  SET(CPACK_PACKAGE_INSTALL_REGISTRY_KEY "neuroscope")
  SET(CPACK_NSIS_INSTALL_ROOT "C:\\neuroscope")
endif()
if(UNIX)
  if(APPLE)
    set( CMAKE_INSTALL_PREFIX "/Applications" )
    set( CPACK_GENERATOR "ZIP" )
    set( CPACK_DMG_VOLUME_NAME "neuroscope" )
    set( CPACK_SYSTEM_NAME "OSX" )
  else()
    set(CPACK_GENERATOR "TBZ2") # for testing mostly, we probably want STGZ or RPM/DEB
    set(CPACK_SYSTEM_NAME "${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}")
  endif()
endif()
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "neuroscope")
set(CPACK_PACKAGE_NAME "neuroscope" )
set(CPACK_PACKAGE_VERSION_MAJOR "2")
set(CPACK_PACKAGE_VERSION_MINOR "0")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "neuroscope")
include(CPack)

