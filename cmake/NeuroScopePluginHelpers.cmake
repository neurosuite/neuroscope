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
    endif()
endfunction()
