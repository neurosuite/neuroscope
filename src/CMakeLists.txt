project(neuroscope-src)
set(neuroscope_SRCS main.cpp neuroscope.cpp neuroscopeview.cpp baseframe.cpp channelcolors.cpp channelgroupview.cpp channeliconview.cpp channelpalette.cpp clustercolors.cpp clusterproperties.cpp 
	clustersprovider.cpp configuration.cpp dataprovider.cpp eventsprovider.cpp itemcolors.cpp itemgroupview.cpp itemiconview.cpp itempalette.cpp neuroscopedoc.cpp neuroscopexmlreader.cpp 
	positionproperties.cpp positionsprovider.cpp positionview.cpp prefdefaults.cpp prefdialog.cpp prefgeneral.cpp  
	properties.cpp propertiesdialog.cpp tags.cpp tracesprovider.cpp traceview.cpp tracewidget.cpp zoomwindow.cpp 
	sessionxmlwriter.cpp parameterxmlcreator.cpp parameterxmlmodifier.cpp imagecreator.cpp utilities.cpp globaleventsprovider.cpp
	eventdata.cpp )

kde3_automoc(${neuroscope_SRCS})

set(neuroscope_UI  propertieslayout.ui prefdefaultslayout.ui positionpropertieslayout.ui prefgenerallayout.ui clusterpropertieslayout.ui)

KDE3_ADD_UI_FILES(neuroscope_SRCS ${neuroscope_UI})

add_executable(neuroscope ${neuroscope_SRCS} )

target_link_libraries(neuroscope ${QT_LIBRARIES} kdeui kdecore kio kdefx kdeprint ${LIBXML2_LIBRARIES})

