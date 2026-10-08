# neuroscope_add_plugin(<target> <sources>...)
#
# Builds a NeuroScope file format plugin: a loadable module that exports only the entry function
# neuroscope_plugin() (see neuroscope_plugin.h). Link the plugin's dependencies statically, so that
# their symbols do not clash with libraries of NeuroScope or of other plugins.
function(neuroscope_add_plugin target)
    add_library(${target} MODULE ${ARGN})
    target_link_libraries(${target} PRIVATE NeuroScope::plugin)
    set_target_properties(${target} PROPERTIES
        AUTOMOC OFF
        C_VISIBILITY_PRESET hidden
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN ON)
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        # Also hide the symbols of static libraries linked into the plugin.
        target_link_options(${target} PRIVATE "LINKER:--exclude-libs,ALL")
        # Include the C++ runtime, so that the plugin also loads on systems with an older libstdc++
        # than the one it was built with. Only C types cross the plugin interface, so the plugin's
        # runtime never meets NeuroScope's.
        if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
            include(CheckLinkerFlag)
            check_linker_flag(CXX "-static-libstdc++" NEUROSCOPE_HAVE_STATIC_LIBSTDCXX)
            if(NEUROSCOPE_HAVE_STATIC_LIBSTDCXX)
                target_link_options(${target} PRIVATE -static-libstdc++ -static-libgcc)
            else()
                get_property(warned GLOBAL PROPERTY NEUROSCOPE_WARNED_STATIC_LIBSTDCXX)
                if(NOT warned)
                    message(WARNING "Plugins link the C++ runtime dynamically, because the static "
                                    "libstdc++ is missing (e.g. the package libstdc++-static).")
                    set_property(GLOBAL PROPERTY NEUROSCOPE_WARNED_STATIC_LIBSTDCXX ON)
                endif()
            endif()
        endif()
    endif()
endfunction()
