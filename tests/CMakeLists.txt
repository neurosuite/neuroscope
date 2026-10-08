find_package(Qt6 REQUIRED COMPONENTS Test)

# The providers only write diagnostics with qDebug(); keep the test output readable.
set(NEUROSCOPE_TEST_ENVIRONMENT
    "QT_QPA_PLATFORM=offscreen"
    "QT_LOGGING_RULES=default.debug=false")

function(neuroscope_add_test name)
    add_executable(${name} ${name}.cpp testutils.h)
    target_link_libraries(${name} PRIVATE neuroscope_dataio Qt6::Test)
    add_test(NAME ${name} COMMAND ${name})
    set_tests_properties(${name} PROPERTIES ENVIRONMENT "${NEUROSCOPE_TEST_ENVIRONMENT}")
    if(WIN32)
        # Find the bundled libneurosuite DLL next to the build tree.
        set_tests_properties(${name} PROPERTIES ENVIRONMENT_MODIFICATION
            "PATH=path_list_prepend:$<TARGET_FILE_DIR:neurosuite::neurosuite>")
    endif()
endfunction()

neuroscope_add_test(test_tracesprovider)
neuroscope_add_test(test_nsxtracesprovider)
neuroscope_add_test(test_clustersprovider)
neuroscope_add_test(test_eventsprovider)
neuroscope_add_test(test_nevproviders)
neuroscope_add_test(test_positionsprovider)
neuroscope_add_test(test_neuroscopexmlreader)

# File format plugins: the test plugin, and libraries that must be rejected. Each is built into its own
# directory, so a test can load one directory without the others.
enable_language(C)
set(NEUROSCOPE_TEST_PLUGIN_DIR ${CMAKE_CURRENT_BINARY_DIR}/plugins)

function(neuroscope_add_test_plugin target directory)
    neuroscope_add_plugin(${target} plugins/testplugin.c)
    target_include_directories(${target} PRIVATE plugins)
    target_compile_definitions(${target} PRIVATE ${ARGN})
    set_target_properties(${target} PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY ${NEUROSCOPE_TEST_PLUGIN_DIR}/${directory}
        RUNTIME_OUTPUT_DIRECTORY ${NEUROSCOPE_TEST_PLUGIN_DIR}/${directory})
endfunction()

neuroscope_add_test_plugin(neuroscope_testplugin good)
neuroscope_add_test_plugin(neuroscope_testplugin_slowdefault slowdefault
    TEST_PLUGIN_NAME="Test format, slow default" TEST_DEFAULT_STREAM=1)
neuroscope_add_test_plugin(neuroscope_testplugin_duplicate duplicate)
neuroscope_add_test_plugin(neuroscope_testplugin_newer bad TEST_PLUGIN_API_VERSION=99)
neuroscope_add_test_plugin(neuroscope_testplugin_noentry bad TEST_PLUGIN_NO_ENTRY)

neuroscope_add_test(test_plugins)
target_include_directories(test_plugins PRIVATE plugins)
target_compile_definitions(test_plugins PRIVATE NEUROSCOPE_TEST_PLUGIN_DIR="${NEUROSCOPE_TEST_PLUGIN_DIR}")
add_dependencies(test_plugins
    neuroscope_testplugin neuroscope_testplugin_slowdefault neuroscope_testplugin_duplicate
    neuroscope_testplugin_newer neuroscope_testplugin_noentry)
