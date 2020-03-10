#-------------------------------------------------
#
# Project created by QtCreator 2019-08-11T11:54:29
#
#-------------------------------------------------

QT       += core gui widgets printsupport xml


TARGET = neuroscope
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS
//DEFINES += NEUROSCOPE_VERSION=2.1
//DEFINES += NEUROSCOPE_DOC_PATH=""

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

CONFIG += c++11


INCLUDEPATH += ../../libneurosuite/src/gui
INCLUDEPATH += ../../libneurosuite/src/shared
INCLUDEPATH += ../../libneurosuite/src
INCLUDEPATH += ../../libneurosuite/src/gui/page

INCLUDEPATH += ../../HDF5/include


SOURCES += \
    ../src/baseframe.cpp \
    ../src/cerebusclustersprovider.cpp \
    ../src/cerebuseventsprovider.cpp \
    ../src/cerebustraceprovider.cpp \
    ../src/channelgroupview.cpp \
    ../src/channeliconview.cpp \
    ../src/channelmimedata.cpp \
    ../src/channelpalette.cpp \
    ../src/clustercolors.cpp \
    ../src/clusterproperties.cpp \
    ../src/clusterpropertieslayout.cpp \
    ../src/clustersprovider.cpp \
    ../src/configuration.cpp \
    ../src/dataprovider.cpp \
    ../src/eventdata.cpp \
    ../src/eventsprovider.cpp \
    ../src/globaleventsprovider.cpp \
    ../src/imagecreator.cpp \
    ../src/itemgroupview.cpp \
    ../src/itemiconview.cpp \
    ../src/itempalette.cpp \
    ../src/main.cpp \
    ../src/neuroscope.cpp \
    ../src/neuroscopedoc.cpp \
    ../src/neuroscopeview.cpp \
    ../src/neuroscopexmlreader.cpp \
    ../src/nevclustersprovider.cpp \
    ../src/neveventsprovider.cpp \
    ../src/nsxtracesprovider.cpp \
    ../src/parameterxmlcreator.cpp \
    ../src/parameterxmlmodifier.cpp \
    ../src/positionproperties.cpp \
    ../src/positionpropertieslayout.cpp \
    ../src/positionsprovider.cpp \
    ../src/positionview.cpp \
    ../src/prefdefaults.cpp \
    ../src/prefdefaultslayout.cpp \
    ../src/prefdialog.cpp \
    ../src/prefgeneral.cpp \
    ../src/prefgenerallayout.cpp \
    ../src/properties.cpp \
    ../src/propertiesdialog.cpp \
    ../src/propertieslayout.cpp \
    ../src/sessionxmlwriter.cpp \
    ../src/tags.cpp \
    ../src/tracesprovider.cpp \
    ../src/traceview.cpp \
    ../src/tracewidget.cpp \
    ../src/ReadHDF5.cpp \
    ../src/hdf5utilities.cpp \
    ../src/nwblocations.cpp \
    ../src/nwbreader.cpp \
 #   ../src/nevclustersprovider.cpp \
 #   ../src/neveventsprovider.cpp \
 #   ../src/nsxtracesprovider.cpp \
    ../src/nwbtracesprovider.cpp \
#    ../src/cerebustraceprovider.cpp \
#    ../src/cerebusclustersprovider.cpp \
#    ../src/cerebuseventsprovider.cpp \
    ../src/nwbeventsprovider.cpp \
    ../src/nwbclustersprovider.cpp

HEADERS += \
    ../src/baseframe.h \
    ../src/blackrock.h \
    ../src/cerebusclustersprovider.h \
    ../src/cerebuseventsprovider.h \
    ../src/cerebustraceprovider.h \
    ../src/channelgroupview.h \
    ../src/channeliconview.h \
    ../src/channelmimedata.h \
    ../src/channelpalette.h \
    ../src/clustercolors.h \
    ../src/clusterproperties.h \
    ../src/clusterpropertieslayout.h \
    ../src/clustersprovider.h \
    ../src/configuration.h \
    ../src/dataprovider.h \
    ../src/eventdata.h \
    ../src/eventsprovider.h \
    ../src/gettimeofday.h \
    ../src/globaleventsprovider.h \
    ../src/imagecreator.h \
    ../src/itemgroupview.h \
    ../src/itemiconview.h \
    ../src/itempalette.h \
    ../src/neuroscope.h \
    ../src/neuroscopedoc.h \
    ../src/neuroscopeview.h \
    ../src/neuroscopexmlreader.h \
    ../src/nevclustersprovider.h \
    ../src/neveventsprovider.h \
    ../src/nsxtracesprovider.h \
    ../src/parameterxmlcreator.h \
    ../src/parameterxmlmodifier.h \
    ../src/positionproperties.h \
    ../src/positionpropertieslayout.h \
    ../src/positionsprovider.h \
    ../src/positionview.h \
    ../src/prefdefaults.h \
    ../src/prefdefaultslayout.h \
    ../src/prefdialog.h \
    ../src/prefgeneral.h \
    ../src/prefgenerallayout.h \
    ../src/properties.h \
    ../src/propertiesdialog.h \
    ../src/propertieslayout.h \
    ../src/sessionInformation.h \
    ../src/sessionxmlwriter.h \
    ../src/tags.h \
    ../src/timer.h \
    ../src/tracesprovider.h \
    ../src/traceview.h \
    ../src/tracewidget.h \
    ../src/config-neuroscope.h\
    ../src/hdf5utilities.h \
    ../src/nwblocations.h \
    ../src/nwbreader.h \
    ../src/blackrock.h \
    ../src/cerebusclustersprovider.h \
    ../src/cerebuseventsprovider.h \
    ../src/cerebustraceprovider.h \
    ../src/nevclustersprovider.h \
    ../src/neveventsprovider.h \
    ../src/nsxtracesprovider.h \
    ../src/nwbtracesprovider.h \
    ../src/alttracesprovider.h \
    ../src/nwbeventsprovider.h \
    ../src/nwbclustersprovider.h

INCLUDEPATH += ../../libneuosuite/src
INCLUDEPATH += ../../libneuosuite/src/shared
INCLUDEPATH += ../../libneuosuite/src/gui
INCLUDEPATH += ../../libneuosuite/src/page
INCLUDEPATH += ../../libneuosuite/src/gui/page

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    ../src/neuroscope-icons.qrc

FORMS += \
    ../src/clusterpropertieslayout.ui \
    ../src/positionpropertieslayout.ui \
    ../src/prefdefaultslayout.ui \
    ../src/prefgenerallayout.ui \
    ../src/propertieslayout.ui

DISTFILES += \
    ../src/data-file.desktop \
    ../src/eeg-file.desktop \
    ../src/filter-file.desktop \
    ../src/neuroscope.desktop \
    ../src/neuroscope-session.desktop \
    ../src/neuroscope.xml \
    ../src/neuroscope_session.xsd \
    ../src/parameter.xsd \
    ../src/hi16-app-neuroscope.png \
    ../src/hi16-nphys-nrs.png \
    ../src/hi22-app-neuroscope.png \
    ../src/hi22-nphys-nrs.png \
    ../src/hi32-app-neuroscope.png \
    ../src/hi32-nphys-nrs.png \
    ../src/hi48-app-neuroscope.png \
    ../src/hi48-nphys-nrs.png \
    ../src/hi64-app-neuroscope.png \
    ../src/hi64-nphys-nrs.png \
    ../src/neuroscope.ico \
    ../src/neuroscope.icns \
    ../src/nphys-nrs.icns \
    ../src/neuroscope.lsm \
    ../src/test \
    ../src/CMakeLists.txt \
    ../src/neuroscope.rc


win32:CONFIG(release, debug|release) {
    INCLUDEPATH += $$PWD/../../../Neurosuite/libneurosuite/build-libneurosuite-Desktop_Qt_5_12_2_MSVC2017_64bit-Debug/debug
    DEPENDPATH += $$PWD/../../../Neurosuite/libneurosuite/build-libneurosuite-Desktop_Qt_5_12_2_MSVC2017_64bit-Debug/debug
    LIBS += -L$$PWD/../../../Neurosuite/libneurosuite/build-libneurosuite-Desktop_Qt_5_12_2_MSVC2017_64bit-Release/release/ -llibneurosuite
}
else:win32:CONFIG(debug, debug|release) {
    INCLUDEPATH += $$PWD/../../../Neurosuite/libneurosuite/build-libneurosuite-Desktop_Qt_5_12_2_MSVC2017_64bit-Release/release
    DEPENDPATH += $$PWD/../../../Neurosuite/libneurosuite/build-libneurosuite-Desktop_Qt_5_12_2_MSVC2017_64bit-Release/release
    LIBS += -L$$PWD/../../../Neurosuite/libneurosuite/build-libneurosuite-Desktop_Qt_5_12_2_MSVC2017_64bit-Debug/debug/ -llibneurosuite
}
else: macx: {
    LIBS += -L$$PWD/../../libneurosuite/build-libneurosuite-Unnamed_cfaaec-Debug/ -llibneurosuite.1.0.0
    INCLUDEPATH += $$PWD/../../libneurosuite/build-libneurosuite-Unnamed_cfaaec-Debug
    DEPENDPATH += $$PWD/../../libneurosuite/build-libneurosuite-Unnamed_cfaaec-Debug
}
else:unix: LIBS += -L$$PWD/../../../Neurosuite/libneurosuite/build-libneurosuite-Desktop_Qt_5_12_2_MSVC2017_64bit-Debug/ -llibneurosuite




win32:CONFIG(release, debug|release){
  LIBS += -L$$PWD/../../Cerebus/libcbsdk/build/src/release/ -lcbsdk
  INCLUDEPATH += $$PWD/../../Cerebus/libcbsdk/src
  INCLUDEPATH += $$PWD/../../Cerebus/libcbsdk/src/cbhwlib
  DEPENDPATH += $$PWD/../../Cerebus/libcbsdk/build/src/Release
}
else:win32:CONFIG(debug, debug|release){
  LIBS += -L$$PWD/../../Cerebus/libcbsdk/build/src/debug/ -lcbsdk
  INCLUDEPATH += $$PWD/../../Cerebus/libcbsdk/src
  INCLUDEPATH += $$PWD/../../Cerebus/libcbsdk/src/cbhwlib
  DEPENDPATH += $$PWD/../../Cerebus/libcbsdk/build/src/debug
}

unix:CONFIG(release, debug|release){
  LIBS += -L$$PWD/../../Cerebus/libcbsdk/build/src/release/ -lcbsdk
  INCLUDEPATH += $$PWD/../../Cerebus/libcbsdk/src
  INCLUDEPATH += $$PWD/../../Cerebus/libcbsdk/src/cbhwlib
  DEPENDPATH += $$PWD/../../Cerebus/libcbsdk/build/src/Release
}
else:unix:CONFIG(debug, debug|release){
  LIBS += -L$$PWD/../../Cerebus/libcbsdk/build/src/debug/ -lcbsdk
  INCLUDEPATH += $$PWD/../../Cerebus/libcbsdk/src
  INCLUDEPATH += $$PWD/../../Cerebus/libcbsdk/src/cbhwlib
  DEPENDPATH += $$PWD/../../Cerebus/libcbsdk/build/src/debug
}



win32:CONFIG(debug, debug|release){
    HDF5_PATH = $$PWD/../../HDF5/Debug/

    LIBS += -L$${HDF5_PATH}/ -llibszip_D -llibzlib_D -llibhdf5_D -llibhdf5_hl_cpp_D -llibhdf5_cpp_D

    INCLUDEPATH += $$PWD/../HDF5/include/
    DEPENDPATH += $$PWD/../HDF5/include/
}
else:win32:CONFIG(release, debug|release){
    HDF5_PATH = $$PWD/../../HDF5/release/

    LIBS += -L$${HDF5_PATH}/ -llibszip -llibzlib -llibhdf5 -llibhdf5_hl_cpp -llibhdf5_cpp

    INCLUDEPATH += $$PWD/../HDF5/include/
    DEPENDPATH += $$PWD/../HDF5/include/
}



unix:CONFIG(release, debug|release) {
    message(Release1)

    HDF5_PATH = $$PWD/../../HDF5/MACFiles/Release/

    PRE_TARGETDEPS += $${HDF5_PATH}/libszip.a
    PRE_TARGETDEPS += $${HDF5_PATH}/libz.a
    PRE_TARGETDEPS += $${HDF5_PATH}/libhdf5.a
    PRE_TARGETDEPS += $${HDF5_PATH}/libhdf5_hl_cpp.a
    PRE_TARGETDEPS += $${HDF5_PATH}/libhdf5_cpp.a

    LIBS += $${HDF5_PATH}/libszip.a
    LIBS += $${HDF5_PATH}/libz.a
    LIBS += $${HDF5_PATH}/libhdf5.a
    LIBS += $${HDF5_PATH}/libhdf5_hl_cpp.a
    LIBS += $${HDF5_PATH}/libhdf5_cpp.a

    INCLUDEPATH += $$PWD/../../HDF5/MACFiles/include/
    DEPENDPATH += $${HDF5_PATH}
}
else:unix:CONFIG(debug, debug|release) {
    message(debug1)

    HDF5_PATH = $$PWD/../../HDF5/MACFiles/Debug/

    PRE_TARGETDEPS += $${HDF5_PATH}/libszip_debug.a
    PRE_TARGETDEPS += $${HDF5_PATH}/libz_debug.a
    PRE_TARGETDEPS += $${HDF5_PATH}/libhdf5_debug.a
    PRE_TARGETDEPS += $${HDF5_PATH}/libhdf5_hl_cpp_debug.a
    PRE_TARGETDEPS += $${HDF5_PATH}/libhdf5_cpp_debug.a

    LIBS += $${HDF5_PATH}/libszip_debug.a
    LIBS += $${HDF5_PATH}/libz_debug.a
    LIBS += $${HDF5_PATH}/libhdf5_debug.a
    LIBS += $${HDF5_PATH}/libhdf5_hl_cpp_debug.a
    LIBS += $${HDF5_PATH}/libhdf5_cpp_debug.a

    INCLUDEPATH += $${HDF5_PATH}/include
    DEPENDPATH += $${HDF5_PATH}/Debug
}


osx:CONFIG(release, debug|release) {
    CONFIG += app_bundle
    mySetOfExtraFiles.files = $$DISTFILES
    mySetOfExtraFiles.path = Contents/Resources
    QMAKE_BUNDLE_DATA += mySetOfExtraFiles
}


