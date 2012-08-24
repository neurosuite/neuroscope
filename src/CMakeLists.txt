project(neuroscope-src)
set(neuroscope_SRCS 
	main.cpp 
	#neuroscope.cpp 
	neuroscopeview.cpp 
	baseframe.cpp 
	channelcolors.cpp 
	channelgroupview.cpp 
	channeliconview.cpp 
	channelpalette.cpp 
	clustercolors.cpp 
	clusterproperties.cpp 
clustersprovider.cpp 
	configuration.cpp 
	dataprovider.cpp 
	eventsprovider.cpp 
	itemcolors.cpp 
	itemgroupview.cpp 
	itemiconview.cpp 
	itempalette.cpp 
	#neuroscopedoc.cpp 
	neuroscopexmlreader.cpp 
	positionproperties.cpp 
	positionsprovider.cpp 
	positionview.cpp 
	prefdefaults.cpp 
	#prefdialog.cpp 
	prefgeneral.cpp  
	#properties.cpp 
	#propertiesdialog.cpp 
	tags.cpp 
	tracesprovider.cpp 
	traceview.cpp 
	tracewidget.cpp 
	zoomwindow.cpp 
	sessionxmlwriter.cpp 
	parameterxmlcreator.cpp 
	parameterxmlmodifier.cpp 
	imagecreator.cpp 
	utilities.cpp 
	globaleventsprovider.cpp
	eventdata.cpp 
	clusterpropertieslayout.cpp 
	positionpropertieslayout.cpp  
	prefdefaultslayout.cpp  
	prefgenerallayout.cpp 
	propertieslayout.cpp
	)

qt4_add_resources(neuroscope_SRCS neuroscope-cursors.qrc neuroscope-icons.qrc)

set(neuroscope_UI  propertieslayout.ui4 prefdefaultslayout.ui4 positionpropertieslayout.ui4 prefgenerallayout.ui4 clusterpropertieslayout.ui4)

qt4_wrap_ui(neuroscope_SRCS ${neuroscope_UI})

add_executable(neuroscope ${neuroscope_SRCS} )

target_link_libraries(neuroscope ${QT_LIBRARIES} ${LIBXML2_LIBRARIES} ${QT_QT3SUPPORT_LIBRARY} ${QT_QTXML_LIBRARY})

install(PROGRAMS neuroscope DESTINATION bin)

