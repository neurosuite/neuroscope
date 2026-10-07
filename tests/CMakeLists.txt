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
