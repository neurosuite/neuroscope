## Makefile.am for neuroscope

# this is the program that gets installed.  it's name is used for all
# of the other Makefile.am variables
bin_PROGRAMS = neuroscope

# set the include path for X, qt and KDE
INCLUDES = -I$(top_srcdir)/src/cursors -I$(top_srcdir)/src/icons $(all_includes) $(LIBXML_CFLAGS)

# the library search path. 
neuroscope_LDFLAGS = $(KDE_RPATH) $(all_libraries) $(LIBXML_LIBS)

# the libraries to link against.
neuroscope_LDADD   = $(LIB_KFILE) $(LIB_KDEPRINT)

# which sources should be compiled for neuroscope
neuroscope_SOURCES = main.cpp neuroscope.cpp neuroscopeview.cpp baseframe.cpp channelcolors.cpp channelgroupview.cpp channeliconview.cpp channelpalette.cpp clustercolors.cpp clusterproperties.cpp clustersprovider.cpp configuration.cpp dataprovider.cpp eventsprovider.cpp itemcolors.cpp itemgroupview.cpp itemiconview.cpp itempalette.cpp neuroscopedoc.cpp neuroscopexmlreader.cpp positionproperties.cpp positionpropertieslayout.ui positionsprovider.cpp positionview.cpp prefdefaults.cpp prefdefaultslayout.ui prefdialog.cpp prefgeneral.cpp prefgenerallayout.ui properties.cpp propertiesdialog.cpp propertieslayout.ui tags.cpp tracesprovider.cpp traceview.cpp tracewidget.cpp zoomwindow.cpp clusterpropertieslayout.ui sessionxmlwriter.cpp parameterxmlcreator.cpp parameterxmlmodifier.cpp imagecreator.cpp utilities.cpp eventdata.cpp

# these are the headers for your project
noinst_HEADERS = neuroscope.h neuroscopeview.h array.h baseframe.h channelcolors.h channelgroupview.h channeliconview.h channelpalette.h clustercolors.h clusterproperties.h clustersprovider.h configuration.h dataprovider.h eventsprovider.h itemcolors.h itemgroupview.h itemiconview.h itempalette.h neuroscopedoc.h neuroscopexmlreader.h positionproperties.h positionsprovider.h positionview.h prefdefaults.h prefdialog.h prefgeneral.h propertiesdialog.h properties.h sessionInformation.h tags.h timer.h tracesprovider.h traceview.h tracewidget.h zoomwindow.h types.h sessionxmlwriter.h parameterxmlcreator.h parameterxmlmodifier.h imagecreator.h utilities.h eventdata.h globaleventsprovider.h

# client stuff

# let automoc handle all of the meta source files (moc)
METASOURCES = AUTO

messages: rc.cpp
	$(XGETTEXT) *.cpp -o $(podir)/neuroscope.pot

KDE_ICON = neuroscope

# this is where the kdelnk file will go 
kdelnkdir = $(kde_appsdir)/Science
kdelnk_DATA = neuroscope.desktop

# this is where the XML-GUI resource file goes
rcdir = $(kde_datadir)/neuroscope
rc_DATA = neuroscopeui.rc
noinst_DATA = neuroscope_session.xsd parameter.xsd
SUBDIRS = cursors icons
mimeTypesdir = $(kde_mimedir)/application
mimeTypes_DATA = data-file.desktop eeg-file.desktop neuroscope-session.desktop filter-file.desktop
AM_CXXFLAGS = -pedantic
