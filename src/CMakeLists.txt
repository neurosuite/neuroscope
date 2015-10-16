################################################################################
# Build config
################################################################################
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
    clustersprovider.cpp
    configuration.cpp
    dataprovider.cpp
    eventsprovider.cpp
    neveventsprovider.cpp
    itemgroupview.cpp
    itemiconview.cpp
    itempalette.cpp
    neuroscopedoc.cpp
    neuroscopexmlreader.cpp
    positionproperties.cpp
    positionsprovider.cpp
    positionview.cpp
    prefdefaults.cpp
    prefdialog.cpp
    prefgeneral.cpp
    properties.cpp
    propertiesdialog.cpp
    tags.cpp
    tracesprovider.cpp
    nsxtracesprovider.cpp
    traceview.cpp
    tracewidget.cpp
    sessionxmlwriter.cpp
    parameterxmlcreator.cpp
    parameterxmlmodifier.cpp
    imagecreator.cpp
    globaleventsprovider.cpp
    eventdata.cpp
    clusterpropertieslayout.cpp
    positionpropertieslayout.cpp
    prefdefaultslayout.cpp
    prefgenerallayout.cpp
    propertieslayout.cpp
)


if(WITH_QT4)
    qt4_add_resources(SOURCE_FILES neuroscope-icons.qrc)
else()
    qt5_add_resources(SOURCE_FILES neuroscope-icons.qrc)
endif()

set(UI_FILES
    propertieslayout.ui
    prefdefaultslayout.ui
    positionpropertieslayout.ui
    prefgenerallayout.ui
    clusterpropertieslayout.ui
)

if(WITH_QT4)
    qt4_wrap_ui(SOURCE_FILES ${UI_FILES})
else()
    qt5_wrap_ui(SOURCE_FILES ${UI_FILES})
endif()

if(WITH_CEREBUS)
    # Add network specific source files
    list(APPEND SOURCE_FILES
         cerebustraceprovider.cpp)
endif(WITH_CEREBUS)

if(APPBUNDLE)
    add_executable(neuroscope MACOSX_BUNDLE ${SOURCE_FILES})
else()
    add_executable(neuroscope WIN32 ${SOURCE_FILES})
endif()

################################################################################
# Linker config
################################################################################
target_link_libraries(neuroscope neurosuite)

if(WITH_QT4)
    target_link_libraries(neuroscope Qt4::QtGui Qt4::QtXml)
else()
    target_link_libraries(neuroscope Qt5::Widgets Qt5::Xml Qt5::PrintSupport)
endif()

if(WITH_CEREBUS)
    target_link_libraries(neuroscope cbsdk)
endif()

################################################################################
# Install config
################################################################################
if(WIN32)
    install(TARGETS neuroscope DESTINATION bin)

    include(DeployNeurosuite)
    install_neurosuite_dependencies(
        "bin/neuroscope.exe"
    )
elseif(APPBUNDLE)
    install(TARGETS neuroscope DESTINATION ".")

    # Set and create Resources directory and install icon
    set(MACOSX_BUNDLE_RESOURCES
        "${CMAKE_CURRENT_BINARY_DIR}/neuroscope.app/Contents/Resources"
    )
    set(MACOSX_BUNDLE_ICON
        "${CMAKE_CURRENT_SOURCE_DIR}/neuroscope.icns"
    )
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E make_directory  ${MACOSX_BUNDLE_RESOURCES}
    )
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${MACOSX_BUNDLE_ICON} ${MACOSX_BUNDLE_RESOURCES}
    )

    include(DeployNeurosuite)
    install_neurosuite_dependencies(
        "neuroscope.app"
    )
else()
    install(TARGETS neuroscope DESTINATION bin)

    if(NOT APPLE)
        install(FILES hi16-app-neuroscope.png
                DESTINATION share/icons/hicolor/16x16/apps/
                RENAME neuroscope.png)
        install(FILES hi22-app-neuroscope.png
                DESTINATION share/icons/hicolor/22x22/apps/
                RENAME neuroscope.png)
        install(FILES hi32-app-neuroscope.png
                DESTINATION share/icons/hicolor/32x32/apps/
                RENAME neuroscope.png)
        install(FILES hi48-app-neuroscope.png
                DESTINATION share/icons/hicolor/48x48/apps/
                RENAME neuroscope.png)
        install(FILES hi64-app-neuroscope.png
                DESTINATION share/icons/hicolor/64x64/apps/
                RENAME neuroscope.png)

        install(FILES hi16-nphys-nrs.png
                DESTINATION share/icons/hicolor/16x16/mimetypes/
                RENAME application-nphys-nrs.png)
        install(FILES hi22-nphys-nrs.png
                DESTINATION share/icons/hicolor/22x22/mimetypes/
                RENAME application-nphys-nrs.png)
        install(FILES hi32-nphys-nrs.png
                DESTINATION share/icons/hicolor/32x32/mimetypes/
                RENAME application-nphys-nrs.png)
        install(FILES hi48-nphys-nrs.png
                DESTINATION share/icons/hicolor/48x48/mimetypes/
                RENAME application-nphys-nrs.png)
        install(FILES hi64-nphys-nrs.png
                DESTINATION share/icons/hicolor/64x64/mimetypes/
                RENAME application-nphys-nrs.png)

        install(FILES neuroscope.desktop
                DESTINATION share/applications/)
        install(FILES neuroscope.xml
                DESTINATION share/mime/packages/)
    endif()
endif()
