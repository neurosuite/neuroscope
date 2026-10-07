################################################################################
# Build config
################################################################################
# File readers and parameter/session files, without widgets.
# Linked by the application and the tests.
set(DATAIO_SOURCE_FILES
    dataprovider.cpp
    tracesprovider.cpp
    nsxtracesprovider.cpp
    clustersprovider.cpp
    nevclustersprovider.cpp
    eventsprovider.cpp
    neveventsprovider.cpp
    positionsprovider.cpp
    eventdata.cpp
    neuroscopexmlreader.cpp
    sessionxmlwriter.cpp
    parameterxmlcreator.cpp
    parameterxmlmodifier.cpp
    tags.cpp
)

add_library(neuroscope_dataio STATIC ${DATAIO_SOURCE_FILES})
target_include_directories(neuroscope_dataio PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(neuroscope_dataio PUBLIC neurosuite::neurosuite Qt6::Core Qt6::Gui Qt6::Xml)

# Normal C++ source files
set(SOURCE_FILES
    main.cpp
    neuroscope.cpp
    neuroscopeview.cpp
    baseframe.cpp
    channelgroupview.cpp
    channeliconview.cpp
    channelpalette.cpp
    channelmimedata.cpp
    clustercolors.cpp
    clusterproperties.cpp
    configuration.cpp
    itemgroupview.cpp
    itemiconview.cpp
    itempalette.cpp
    neuroscopedoc.cpp
    positionproperties.cpp
    positionview.cpp
    prefdefaults.cpp
    prefdialog.cpp
    prefgeneral.cpp
    properties.cpp
    propertiesdialog.cpp
    traceview.cpp
    tracewidget.cpp
    imagecreator.cpp
    globaleventsprovider.cpp
    clusterpropertieslayout.cpp
    positionpropertieslayout.cpp
    prefdefaultslayout.cpp
    prefgenerallayout.cpp
    propertieslayout.cpp
)

# windows resources
if(WIN32)
    list(APPEND SOURCE_FILES
         neuroscope.rc)
endif()

# Qt resources
qt_add_resources(SOURCE_FILES neuroscope-icons.qrc)

# Ui files
set(UI_FILES
    propertieslayout.ui
    prefdefaultslayout.ui
    positionpropertieslayout.ui
    prefgenerallayout.ui
    clusterpropertieslayout.ui
)

qt_wrap_ui(SOURCE_FILES ${UI_FILES})

# Optional C++ files
if(WITH_CEREBUS)
    # Add network specific source files
    list(APPEND SOURCE_FILES
         cerebustraceprovider.cpp
         cerebusclustersprovider.cpp
         cerebuseventsprovider.cpp)
endif()

# Combine it all
if(APPBUNDLE)
    add_executable(neuroscope MACOSX_BUNDLE ${SOURCE_FILES})
else()
    add_executable(neuroscope WIN32 ${SOURCE_FILES})
endif()

################################################################################
# Linker config
################################################################################
target_link_libraries(neuroscope PRIVATE neuroscope_dataio neurosuite::neurosuite Qt6::Widgets Qt6::PrintSupport Qt6::Xml)

if(WITH_CEREBUS)
    target_link_libraries(neuroscope PRIVATE cbsdk)
endif()

################################################################################
# Install config
################################################################################
if(APPBUNDLE)
    install(TARGETS neuroscope BUNDLE DESTINATION ".")

    # Copy the icon into the bundle's Resources directory
    set(MACOSX_BUNDLE_RESOURCES
        "${CMAKE_CURRENT_BINARY_DIR}/neuroscope.app/Contents/Resources")
    file(MAKE_DIRECTORY ${MACOSX_BUNDLE_RESOURCES})
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/neuroscope.icns
         DESTINATION ${MACOSX_BUNDLE_RESOURCES})
    # Qt frameworks and libneurosuite are added by macdeployqt (see release workflow).
else()
    install(TARGETS neuroscope RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
    # On Windows the Qt runtime is added by windeployqt (see release workflow).
endif()

# Bundle the Qt runtime on Windows and macOS at install time, so that
# "cmake --install" and CPack (NSIS, DragNDrop) produce self-contained packages.
if(WIN32 OR APPBUNDLE)
    qt_generate_deploy_app_script(
        TARGET neuroscope
        OUTPUT_SCRIPT NEUROSCOPE_DEPLOY_SCRIPT
        NO_UNSUPPORTED_PLATFORM_ERROR
    )
    install(SCRIPT ${NEUROSCOPE_DEPLOY_SCRIPT})
endif()

if(UNIX AND NOT APPLE)
    set(APP_ID io.github.neurosuite.NeuroScope)
    set(ICON_DIR ${CMAKE_INSTALL_DATADIR}/icons/hicolor)
    foreach(SIZE 16 22 32 48 64)
        install(FILES hi${SIZE}-app-neuroscope.png
                DESTINATION ${ICON_DIR}/${SIZE}x${SIZE}/apps
                RENAME neuroscope.png)
        install(FILES hi${SIZE}-nphys-nrs.png
                DESTINATION ${ICON_DIR}/${SIZE}x${SIZE}/mimetypes
                RENAME application-nphys-nrs.png)
    endforeach()

    install(FILES neuroscope.desktop
            DESTINATION ${CMAKE_INSTALL_DATADIR}/applications
            RENAME ${APP_ID}.desktop)
    install(FILES ${APP_ID}.metainfo.xml
            DESTINATION ${CMAKE_INSTALL_DATADIR}/metainfo)
    install(FILES neuroscope.xml
            DESTINATION ${CMAKE_INSTALL_DATADIR}/mime/packages)
endif()
