project(neuroscope-src)
set(neuroscope_SRCS
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

qt4_add_resources(neuroscope_SRCS neuroscope-icons.qrc)

set(neuroscope_UI  propertieslayout.ui prefdefaultslayout.ui positionpropertieslayout.ui prefgenerallayout.ui clusterpropertieslayout.ui)

qt4_wrap_ui(neuroscope_SRCS ${neuroscope_UI})

if(USE_MACOSX_BUNDLE)
	add_executable(neuroscope MACOSX_BUNDLE ${neuroscope_SRCS})
else()
	add_executable(neuroscope WIN32 ${neuroscope_SRCS})
endif()

if(Qt5Core_FOUND)
  # Since Qt5WebKitWidgets is optional, we use the old variable here that will not throw an error if not set.
  target_link_libraries( neuroscope ${LIBKLUSTERSSHARED_LIBRARY} Qt5::Widgets Qt5::Xml Qt5::PrintSupport Qt5::WinMain ${Qt5WebKitWidgets_LIBRARIES})
else(Qt5Core_FOUND)
  target_link_libraries( neuroscope ${LIBKLUSTERSSHARED_LIBRARY} ${QT_LIBRARIES} ${QT_QTXML_LIBRARY})
endif()

if(WIN32)
	install(TARGETS neuroscope DESTINATION bin)

	# Use fix_bundle to install qt libraries
	set(APPS "\${CMAKE_INSTALL_PREFIX}/bin/neuroscope.exe") # paths to executables
	set(DIRS "${QTDIR}/lib;${QTDIR}/bin") # directories to search for prerequisites
	
	INSTALL(CODE "include(BundleUtilities)
	              fixup_bundle(\"${APPS}\" \"\" \"${DIRS}\")")
elseif(USE_MACOSX_BUNDLE)
	install(TARGETS neuroscope DESTINATION ".")

	set(MACOSX_BUNDLE_RESOURCES "${CMAKE_CURRENT_BINARY_DIR}/neuroscope.app/Contents/Resources")
	set(MACOSX_BUNDLE_ICON "${CMAKE_CURRENT_SOURCE_DIR}/neuroscope.icns")
	execute_process(COMMAND ${CMAKE_COMMAND} -E make_directory ${MACOSX_BUNDLE_RESOURCES})
	execute_process(COMMAND ${CMAKE_COMMAND} -E copy_if_different ${MACOSX_BUNDLE_ICON} ${MACOSX_BUNDLE_RESOURCES})
	
	# Use DeployQt4 to install qt libraries
    include(DeployQt4)
    install_qt4_executable("neuroscope.app")
else()
	install(TARGETS neuroscope DESTINATION bin)

	install(FILES hi16-app-neuroscope.png DESTINATION share/icons/hicolor/16x16/apps/ RENAME neuroscope.png)
	install(FILES hi22-app-neuroscope.png DESTINATION share/icons/hicolor/22x22/apps/ RENAME neuroscope.png)
	install(FILES hi32-app-neuroscope.png DESTINATION share/icons/hicolor/32x32/apps/ RENAME neuroscope.png)
	install(FILES hi48-app-neuroscope.png DESTINATION share/icons/hicolor/48x48/apps/ RENAME neuroscope.png)
	install(FILES hi64-app-neuroscope.png DESTINATION share/icons/hicolor/64x64/apps/ RENAME neuroscope.png)

	install(FILES hi16-nphys-nrs.png DESTINATION share/icons/hicolor/16x16/mimetypes/ RENAME application-nphys-nrs.png)
	install(FILES hi22-nphys-nrs.png DESTINATION share/icons/hicolor/22x22/mimetypes/ RENAME application-nphys-nrs.png)
	install(FILES hi32-nphys-nrs.png DESTINATION share/icons/hicolor/32x32/mimetypes/ RENAME application-nphys-nrs.png)
	install(FILES hi48-nphys-nrs.png DESTINATION share/icons/hicolor/48x48/mimetypes/ RENAME application-nphys-nrs.png)
	install(FILES hi64-nphys-nrs.png DESTINATION share/icons/hicolor/64x64/mimetypes/ RENAME application-nphys-nrs.png)

	install(FILES neuroscope.desktop DESTINATION share/applications/)
	install(FILES neuroscope.xml DESTINATION share/mime/packages/)
endif()



