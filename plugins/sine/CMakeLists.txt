# Example plugin with synthetic sinusoids. It is built next to the plugins NeuroScope finds in the build
# tree, but not installed. Open plugins/sine/demo.sine to see it.
enable_language(C)
neuroscope_add_plugin(neuroscope_sine sineplugin.c)
if(NOT MSVC)
    target_link_libraries(neuroscope_sine PRIVATE m)
endif()
set_target_properties(neuroscope_sine PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${NEUROSCOPE_BUILD_PLUGIN_DIR}
    RUNTIME_OUTPUT_DIRECTORY ${NEUROSCOPE_BUILD_PLUGIN_DIR})
