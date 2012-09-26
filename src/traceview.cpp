/***************************************************************************
                          traceview.cpp  -  description
                             -------------------
    begin                : Wed Mar 17 2004
    copyright            : (C) 2004 by Lynn Hazan
    email                : lynn.hazan.myrealbox.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


//include files for the application
#include "traceview.h"
#include "channelcolors.h"
#include "clustersprovider.h"
#include "eventsprovider.h"
#include "itemcolors.h"
#include "timer.h"


// include files for QT
#include <qmap.h>
#include <QList>
#include <QPainter>
#include <QPrinter>

//Added by qt3to4:
#include <QMouseEvent>
#include <QPolygon>
#include <QApplication>
#include <QMessageBox>

#include <QDebug>




const int TraceView::XMARGIN = 50;
const int TraceView::YMARGIN = 0;

TraceView::TraceView(TracesProvider& tracesProvider,bool greyScale,bool multiColumns,bool verticalLines,
                     bool raster,bool waveforms,bool labelsDisplay,QList<int>& channelsToDisplay,int unitGain,int acquisitionGain,long start,long timeFrameWidth,
                     ChannelColors* channelColors,QMap<int, QList<int> >* groupsChannels,QMap<int,int>* channelsGroups,
                     QList<int>& channelOffsets,QList<int>& gains,const QList<int>& skippedChannels,int rasterHeight,const QImage& backgroundImage,QWidget* parent, const char* name,QColor backgroundColor,QStatusBar* statusBar,
                     int minSize,int maxSize,int windowTopLeft,int windowBottomRight,int border):
    BaseFrame(10,0,parent,name,backgroundColor,minSize,maxSize,windowTopLeft,windowBottomRight,border),
    greyScaleMode(greyScale),
    statusBar(statusBar),
    tracesProvider(tracesProvider),
    multiColumns(multiColumns),
    verticalLines(verticalLines),
    raster(raster),
    waveforms(waveforms),
    dataReady(false),data(),
    channelOffsets(channelOffsets),
    gains(gains),
    channelColors(channelColors),
    groupsChannels(groupsChannels),
    channelsGroups(channelsGroups),
    doublebuffer(),
    background(backgroundImage),
    acquisitionGain(acquisitionGain),
    unitGain(unitGain),
    xMargin(10),
    yMargin(0),
    columnDisplayChanged(false),
    resized(false),
    groupsChanged(false),
    previousDragOrdinate(0),
    lastClickOrdinate(0),
    nbSamplesModified(false),
    alreadySelected(false),
    isInit(true),
    channelforVoltageComputation(0),
    startingIndex(0),
    showLabels(labelsDisplay),
    showCalibrationScale(false),
    downSampling(1),
    zoomed(false),
    firstZoom(true),
    doubleClick(false),
    zoomFactor(0),
    maxZoomReached(false),
    zoomOut(false),
    printState(false),
    startTimeInRecordingUnits(0),
    previousStartTimeInRecordingUnits(0),
    spikeBrowsing(false),
    newEventPosition(-1),
    eventBeingModified(false),
    retrieveClusterData(false)
{


    QList<int>::iterator channelsToShowIterator;
    for(channelsToShowIterator = channelsToDisplay.begin(); channelsToShowIterator != channelsToDisplay.end(); ++channelsToShowIterator)
        shownChannels.append(*channelsToShowIterator);

    skipStatusChanged(skippedChannels);

    //Determine the screen resolution in pixel by centimeters
    QPrinter printer;
    screenResolution = static_cast<int>(0.5 + printer.resolution() / 2.54);

    //Cluster related variables
    clustersData.setAutoDelete(true);
    nbClusters = 0;

    //Event related variables
    eventsData.setAutoDelete(true);
    selectedEvent.first.clear();
    selectedEvent.second = 0;

    //Set the default modes
    mode = ZOOM;

    startTime = start;
    endTime = start + timeFrameWidth;
    this->timeFrameWidth = timeFrameWidth;
    length = tracesProvider.recordingLength();

    //Set the drawing variables
    nbChannels = tracesProvider.getNbChannels();
    int samplingRate = tracesProvider.getSamplingRate();

    updateShownGroupsChannels(shownChannels);

    //rasterHeight is equals to -1 if no value is available.
    if(rasterHeight == -1)
        this->rasterHeight = 33;
    else
        this->rasterHeight = rasterHeight;

    borderX = 0;
    Xstep = 1;
    timeStepUnit = timeStep = static_cast<float>(static_cast<float>(1000) / static_cast<float>(samplingRate));
    traceVspace = 100;
    Yspace = traceVspace/10;
    Yshift = Yspace + traceVspace;
    borderY = Yshift / 2;
    YGroupSpace = Yshift / 2;
    YTracesRasterSeparator = Yshift / 2;
    //rasterHeight =  static_cast<int>(floor(0.5 + static_cast<float>(traceVspace)/3));
    YRasterSpace = rasterHeight/3;

    ordinateMax = 0;
    abscissaMin = 0;
    X0 = borderX;


    //The initial offset for each channel is set to zero.
    if(channelOffsets.isEmpty())
        for(int i = 0; i < nbChannels; ++i)
            channelOffsets.append(0);

    //The initial amplitude and factor for each channel.
    if(gains.size() == 0)
        setGains(unitGain,acquisitionGain);
    else{
        //Compute alpha: (3.traceVspace) / (Utheta . acquisitionGain)
        //Utheta: amplitude maximal of theta in milivolts, 0.4 mv
        alpha =  static_cast<float>(static_cast<float>(3 * traceVspace) /
                                    static_cast<float>(0.4 *  static_cast<float>(acquisitionGain)));
        //factor = alpha * (4/3)^gain
        for(int i = 0; i < nbChannels; ++i){
            float Yfactor = static_cast<float>(alpha * pow(0.75,gains[i]));
            channelFactors.append(Yfactor);
        }
    }

    //Set Connection(s).
    connect(&tracesProvider,SIGNAL(dataReady(Array<dataType>&,QObject*)),this,SLOT(dataAvailable(Array<dataType>&,QObject*)));

    //Set the display of the labels, the default is to hide them, if need it change that.
    if(showLabels){
        xMargin = XMARGIN;
        yMargin = YMARGIN;
        setBorders(xMargin,yMargin);
    }

    //compute the size of the window if the display is not on multicolumns,
    //in that case the viewport needs to be compute before.
    if(!multiColumns) updateWindow();

    //Create the cursors

    measureCursor = QCursor(QPixmap(":/cursors/measure_cursor"),0,0);
    selectTimeCursor = QCursor(QPixmap(":/cursors/select_time_cursor"),0,0);
    selectEventCursor = QCursor(QPixmap(":/cursors/select_event_cursor"),0,0);
    addEventCursor = QCursor(QPixmap(":/cursors/add_event_cursor"),0,0);
    selectCursor = QCursor(QPixmap(":/cursors/select_channels_cursor"),0,0);
    drawLineCursor = QCursor(QPixmap(":/cursors/time_line_cursor"),0,0);

    //Set the cursor shap to a magnifier as the only action allowed on the widget is to zoom.
    setCursor(zoomCursor);

    //Allowed the mouse tracking to write the time in the status bar.
    setMouseTracking(true);

    //The backgound image is scaled too fit to the window size.
    scaleBackgroundImage();

    //Get the data.
    tracesProvider.requestData(startTime,endTime,this,startTimeInRecordingUnits);
}



TraceView::~TraceView(){}

void TraceView::changeCursor()
{
    if(mode == SELECT)
        setCursor(selectCursor);
    else if(mode == ZOOM)
        setCursor(zoomCursor);
    else if(mode == MEASURE)
        setCursor(measureCursor);
    else if(mode == SELECT_TIME)
        setCursor(selectTimeCursor);
    else if(mode == SELECT_EVENT)
        setCursor(selectEventCursor);
    else if(mode == ADD_EVENT)
        setCursor(addEventCursor);
    else if(mode == DRAW_LINE)
        setCursor(drawLineCursor);

}

void TraceView::dataAvailable(Array<dataType>& data,QObject* initiator){


    //If another widget was the initiator of the request, ignore the data.
    if(initiator != this) return;

    if(data.nbOfRows() == 0){
        QApplication::restoreOverrideCursor();

        QMessageBox::critical(this, tr("IO Error"),tr("An error has occured, the data file could not be opened or the file size is incorrect."));
        changeCursor();
        return;
    }

    this->data = data;
    dataReady = true;

    updateWindow();

    //The following code was done in case of threads, without thread the trace data arrive always last
    //No clusters or events selected
    if(clustersData.count() == 0 && eventsData.count() == 0){
        changeCursor();
        //Everything has to be redraw
        repaint(false);
    }
    //Check if the cluster and event data are available
    else{
        bool ready = false;
        Q3DictIterator<ClusterData> iterator(clustersData);
        for(;iterator.current();++iterator){
            ready = iterator.current()->status();
            if(!ready) break;
        }
        Q3DictIterator<EventData> iterator2(eventsData);
        for(;iterator2.current();++iterator2){
            ready = iterator2.current()->status();
            if(!ready) break;
        }
        if(ready){
            changeCursor();

            //Everything has to be redraw
            repaint(false);
        }
    }

}

void TraceView::dataAvailable(Array<dataType>& data,QObject* initiator,QString providerName){
    //If another widget was the initiator of the request, ignore the data.
    if(initiator != this)
        return;


    qDebug()<<" in  dataAvailable, providerName"<<providerName<<" data.nbOfColumns() " <<data.nbOfColumns() ;

    ClusterData* clusterData = clustersData[providerName];
    clusterData->setStatus(true);
    clusterData->setData(data);

    //The following code was done in case of threads, without thread the trace data arrive always last
    bool ready = false;
    Q3DictIterator<ClusterData> iterator(clustersData);
    for(;iterator.current();++iterator){
        ready = iterator.current()->status();
        if(!ready) break;
    }
    Q3DictIterator<EventData> iterator2(eventsData);
    for(;iterator2.current();++iterator2){
        ready = iterator2.current()->status();
        if(!ready) break;
    }
    if(dataReady && ready){
        changeCursor();

        //Everything has to be redraw
        drawContentsMode = REDRAW;
        update();
    }
}

void TraceView::dataAvailable(Array<dataType>& times,Array<int>& ids,QObject* initiator,QString providerName){
    //If another widget was the initiator of the request, ignore the data.
    if(initiator != this) return;

    EventData* eventData = eventsData[providerName];
    eventData->setStatus(true);
    eventData->setData(times,ids);

    //The following code was done in case of threads, without thread the trace data arrive always last
    bool ready = false;
    Q3DictIterator<EventData> iterator(eventsData);
    for(;iterator.current();++iterator){
        ready = iterator.current()->status();
        if(!ready) break;
    }
    //If all the data for the events are available, send an signal for the listeners interested in it.
    // if(ready) emit eventsAvailable(eventsData,selectedEvents,providerItemColors);

    Q3DictIterator<ClusterData> iterator2(clustersData);
    for(;iterator2.current();++iterator2){
        ready = iterator2.current()->status();
        if(!ready) break;
    }
    if(dataReady && ready){
        changeCursor();

        //Everything has to be redraw
        drawContentsMode = REDRAW;
        update();
    }
}

void TraceView::updateClusterData(bool active){
    if(!active){
        retrieveClusterData = true;
        return;
    }
    else retrieveClusterData = false;

    if(clustersData.isEmpty()) return;

    //Request the data

    //Retrieve the data for the clusters, only request data from the provider for which clusters have been selected
    if(verticalLines || raster || waveforms){
        QList<int> toRemove;
        QMap<int, QList<int> >::Iterator providersIterator;
        for(providersIterator = selectedClusters.begin(); providersIterator != selectedClusters.end(); ++providersIterator){
            if(static_cast< QList<int> >(providersIterator.data()).isEmpty())
                toRemove.append(providersIterator.key());
        }

        QList<int>::iterator toRemoveIterator;
        for(toRemoveIterator = toRemove.begin(); toRemoveIterator != toRemove.end(); ++toRemoveIterator){
            selectedClusters.remove(*toRemoveIterator);
            clustersData.remove(QString::fromLatin1("%1").arg(*toRemoveIterator));
        }

        if(clustersData.count() != 0) setCursor(Qt::WaitCursor);
        Q3DictIterator<ClusterData> iterator(clustersData);
        for(;iterator.current();++iterator){
            if(iterator.currentKey() != clusterProviderToSkip) iterator.current()->setStatus(false);
        }
        iterator.toFirst();
        for(;iterator.current();++iterator){
            if(iterator.currentKey() != clusterProviderToSkip){
                static_cast<ClustersProvider*>(clusterProviders[iterator.currentKey()])->requestData(startTime,endTime,this,startTimeInRecordingUnits);
            }
            else
                clusterProviderToSkip.clear();
        }
    }

}

void TraceView::displayTimeFrame(long start,long timeFrameWidth){
    startTime = start;
    endTime = start + timeFrameWidth;
    if(this->timeFrameWidth != timeFrameWidth) nbSamplesModified = true;
    this->timeFrameWidth = timeFrameWidth;

    //Request the data
    dataReady = false;
    setCursor(Qt::WaitCursor);

    //If the update is not due to spike browsing reset startTimeInRecordingUnits
    if(!spikeBrowsing) startTimeInRecordingUnits = 0;
    else spikeBrowsing = false;

    //Retreive the data for the clusters, only request data from the provider for which clusters have been selected
    if(verticalLines || raster || waveforms){
        QList<int> toRemove;
        QMap<int, QList<int> >::Iterator providersIterator;
        for(providersIterator = selectedClusters.begin(); providersIterator != selectedClusters.end(); ++providersIterator){
            if(static_cast< QList<int> >(providersIterator.data()).isEmpty())
                toRemove.append(providersIterator.key());
        }

        QList<int>::iterator toRemoveIterator;
        for(toRemoveIterator = toRemove.begin(); toRemoveIterator != toRemove.end(); ++toRemoveIterator){
            selectedClusters.remove(*toRemoveIterator);
            clustersData.remove(QString::fromLatin1("%1").arg(*toRemoveIterator));
        }

        Q3DictIterator<ClusterData> iterator(clustersData);
        for(;iterator.current();++iterator){
            if(iterator.currentKey() != clusterProviderToSkip) iterator.current()->setStatus(false);
        }
        iterator.toFirst();
        for(;iterator.current();++iterator){
            if(iterator.currentKey() != clusterProviderToSkip){
                static_cast<ClustersProvider*>(clusterProviders[iterator.currentKey()])->requestData(startTime,endTime,this,startTimeInRecordingUnits);
            }
            else clusterProviderToSkip.clear();
        }
    }

    //Retreive the data for the events, only request data from the provider for which events have been selected
    if(!selectedEvents.isEmpty()){
        QList<QString> toRemove;
        QMap<QString, QList<int> >::Iterator providersIterator;
        for(providersIterator = selectedEvents.begin(); providersIterator != selectedEvents.end(); ++providersIterator){
            if(static_cast< QList<int> >(providersIterator.data()).isEmpty())
                toRemove.append(providersIterator.key());
        }

        QList<QString>::iterator toRemoveIterator;
        for(toRemoveIterator = toRemove.begin(); toRemoveIterator != toRemove.end(); ++toRemoveIterator){
            selectedEvents.remove(*toRemoveIterator);
            eventsData.remove(*toRemoveIterator);
        }

        //If there is only one event provider and the data rewuest have been made by a request for the next or previous event, this signal make sur
        //listeners of the event data will receive the nformation
        //  if(selectedEvents.count() == 1 && eventProviderToSkip != "") emit eventsAvailable(eventsData,selectedEvents,providerItemColors);

        Q3DictIterator<EventData> iterator(eventsData);
        for(;iterator.current();++iterator){
            if(iterator.currentKey() != eventProviderToSkip) iterator.current()->setStatus(false);
        }
        iterator.toFirst();
        for(;iterator.current();++iterator){
            if(iterator.currentKey() != eventProviderToSkip){
                static_cast<EventsProvider*>(eventProviders[iterator.currentKey()])->requestData(startTime,endTime,this);
            }
            else eventProviderToSkip.clear();
        }
    }

    tracesProvider.requestData(startTime,endTime,this,startTimeInRecordingUnits);
}

void TraceView::showHideLabels(bool show){
    showLabels = show;

    if(show){
        xMargin = XMARGIN;
        yMargin = YMARGIN;
    }
    else{
        xMargin = 10;
        yMargin = 0;
    }

    setBorders(xMargin,yMargin);

    //Everything has to be redraw
    updateWindow();
    update();
}

void TraceView::paintEvent ( QPaintEvent*){
    bool isInitAndResized = false;
    if(isInit){
        if(resized){
            isInitAndResized = true;
            scaleBackgroundImage();
        }

        if(multiColumns){
            QRect contentsRec = contentsRect();
            viewport = QRect(contentsRec.left() + xMargin,contentsRec.top(),contentsRec.width() - xMargin,contentsRec.height());
            if(viewport.width() == 0) update();
            else{
                isInit = false;
                columnDisplayChanged = true;
                scaleBackgroundImage();
                updateWindow();
                update();
            }
        }
        else{
            QRect contentsRec = contentsRect();
            viewport = QRect(contentsRec.left() + xMargin,contentsRec.top(),contentsRec.width() - xMargin,contentsRec.height());
            if(viewport.width() == 0) update();
            else{
                scaleBackgroundImage();
                computeChannelDisplayGain();
            }
        }
        isInit = false;
    }
    else if(!eventProvidersToUpdate.isEmpty()){
        QList<QString>::iterator providerIterator;
        EventData* eventData;
        for(providerIterator = eventProvidersToUpdate.begin(); providerIterator != eventProvidersToUpdate.end(); ++providerIterator){
            eventData = eventsData[*providerIterator];
            if(eventData == 0){
                eventData = new EventData();
                eventsData.insert(*providerIterator,eventData);
            }
            else eventsData[*providerIterator]->setStatus(false);
        }

        Q3DictIterator<EventData> iterator(eventsData);
        for(;iterator.current();++iterator){
            if(eventProvidersToUpdate.contains(iterator.currentKey())){
                static_cast<EventsProvider*>(eventProviders[iterator.currentKey()])->requestData(startTime,endTime,this);
                eventProvidersToUpdate.remove(iterator.currentKey());
            }
        }
        return;
    }
    QPainter p(this);
    if(drawContentsMode == REDRAW && dataReady){
        QRect contentsRec = contentsRect();
        QRect r((QRect)window);

        //If the left margin is not visible (the user zoomed without taking it in his selection), the viewport and the contentsRec
        //have the same size.
        if(r.left() != 0) viewport = QRect(contentsRec.left(),contentsRec.top(),contentsRec.width(),contentsRec.height());
        else viewport = QRect(contentsRec.left() + xMargin,contentsRec.top(),contentsRec.width() - xMargin,contentsRec.height());

        if(resized)
            scaleBackgroundImage();

        if(resized || (selectedClusters.count() != 0 && nbClusters == 0))
            updateWindow();

        if(!isInitAndResized){
            //Resize the double buffer with the width and the height of the widget(QFrame)
            doublebuffer.resize(contentsRec.width(),contentsRec.height());

            //Create a painter to paint on the double buffer
            QPainter painter;
            painter.begin(&doublebuffer);

            //if need it, draw the background image before applying any transformation to the painter.
            if(!background.isNull())
                painter.drawPixmap(0,0,scaledBackground);

            //Correct the window after the user zoomed if need it.
            correctZoom(r);

            //Set the window (part of the world I want to show)
            painter.setWindow(r.left(),r.top(),r.width()-1,r.height()-1);//hack because Qt QRect is used differently in this function

            //Set the viewport (part of the device I want to write on).
            //By default, the viewport is the same as the device's rectangle (contentsRec), taking a smaller
            //one will ensure that the legends (cluster ids) will not ovelap a correlogram.
            painter.setViewport(viewport);

            //Fill the double buffer with the background color if no image has been set.
            if(background.isNull())
                doublebuffer.fill(paletteBackgroundColor());

            //Paint all the traces in the shownChannels list (in the double buffer,on top of the background image or the background color )
            drawTraces(painter);

            //reset transformation due to setWindow and setViewport
            painter.resetMatrix() ;

            //Draw channel ids and amplitude on the left side.
            if(showLabels)
                drawChannelIdsAndGain(painter);

            //Draw the calibration scale
            if(showCalibrationScale)
                drawCalibrationScale(painter);

            //Closes the painter on the double buffer
            painter.end();
        }

        //Back to the default
        drawContentsMode = REFRESH;
    }
    if(!mSelectedChannels.isEmpty())
        drawTraces(mSelectedChannels,true);
    if(!mDeselectedChannels.isEmpty())
        drawTraces(mDeselectedChannels,false);

    //if drawContentsMode == REFRESH, we reuse the double buffer (pixmap)

    //Draw the double buffer (pixmap) by copying it into the widget device.
    p.drawPixmap(0, 0, doublebuffer);

    if(resized){
        resized = false;
        drawContentsMode = REDRAW;
        update();
    }
}

void TraceView::setMultiColumns(bool multiple){
    multiColumns = multiple;

    //Everything has to be redraw
    columnDisplayChanged = true;
    updateWindow();
    update();
}

void TraceView::setClusterVerticalLines(bool lines){
    verticalLines = lines;

    //Everything has to be redraw
    drawContentsMode = REDRAW;
    update();
}

void TraceView::setClusterRaster(bool raster){
    this->raster = raster;

    //Everything has to be redraw
    updateWindow();
    update();
}

void TraceView::setClusterWaveforms(bool waveforms){
    this->waveforms = waveforms;

    //Everything has to be redraw
    drawContentsMode = REDRAW;
    update();
}

void TraceView::setGreyScale(bool grey){
    greyScaleMode = grey;

    //Everything has to be redraw
    drawContentsMode = REDRAW;
    update();
}

void TraceView::resetOffsets(const QMap<int,int>& selectedChannelDefaultOffsets){
    QMap<int,int>::const_iterator iterator;
    for(iterator = selectedChannelDefaultOffsets.begin(); iterator != selectedChannelDefaultOffsets.end(); ++iterator)
        channelOffsets[iterator.key()] = iterator.data();

    //Everything has to be redraw
    drawContentsMode = REDRAW;
    update();
}


void TraceView::resetGains(const QList<int>& selectedChannels){
    //factor = alpha * (4/3)^gain, gain equals 0 at the begining

    QList<int>::const_iterator iterator;
    for(iterator = selectedChannels.begin(); iterator != selectedChannels.end(); ++iterator){
        gains[*iterator] = 0;
        channelFactors[*iterator] = alpha;
    }

    computeChannelDisplayGain();

    //Everything has to be redraw
    drawContentsMode = REDRAW;
    update();
}

void TraceView::showChannels(const QList<int>& channelsToShow){
    shownChannels.clear();

    QList<int>::const_iterator channelsToShowIterator;
    for(channelsToShowIterator = channelsToShow.begin(); channelsToShowIterator != channelsToShow.end(); ++channelsToShowIterator){
        shownChannels.append(*channelsToShowIterator);
    }

    //remove the channels in selectedChannels which are no more shown
    QList<int>::iterator iterator;
    QList<int> toRemoved;
    for(iterator = selectedChannels.begin(); iterator != selectedChannels.end(); ++iterator)
        if(!shownChannels.contains(*iterator))toRemoved.append(*iterator);

    for(iterator = toRemoved.begin(); iterator != toRemoved.end(); ++iterator)
        selectedChannels.remove(*iterator);

    updateShownGroupsChannels(channelsToShow);

    //Request the data
    dataReady = false;
    tracesProvider.requestData(startTime,endTime,this,startTimeInRecordingUnits);
}

void TraceView::updateShownGroupsChannels(const QList<int>& channelsToShow){
    uint nbGpsShown = shownGroupsChannels.count();
    shownGroupsChannels.clear();

    int nbGps = groupsChannels->count();
    QMap<int, QList<int> >::Iterator iterator;
    for(iterator = groupsChannels->begin(); iterator != groupsChannels->end(); ++iterator){
        shownGroupsChannels.insert(iterator.key(),iterator.data());
    }

    for(int i = 0; i < nbChannels; ++i){
        if(!channelsToShow.contains(i)){
            int groupId = (*channelsGroups)[i];
            QList<int> channelIds = shownGroupsChannels[groupId];
            channelIds.remove(i);
            if(channelIds.size() == 0) shownGroupsChannels.remove(groupId);
            else shownGroupsChannels.replace(groupId,channelIds);
        }
    }

    //The trash group (index 0) is always at the bottom in the display, so reindex it with the highest index.
    if(shownGroupsChannels.contains(0)){
        QList<int> channelIds = shownGroupsChannels[0];
        shownGroupsChannels.remove(0);
        shownGroupsChannels.insert(nbGps,channelIds);
    }

    //If the number of shown groups has changed, update groupsChanged to correctly update the window
    if(shownGroupsChannels.count() != nbGpsShown) groupsChanged = true;
}

void TraceView::groupsModified(bool active){
    updateShownGroupsChannels(shownChannels);

    groupsChanged = true;
    updateWindow();
    if(active) update();
}

void TraceView::setGains(int gain,int acquisitionGain){
    unitGain = gain;
    this->acquisitionGain = acquisitionGain;

    //Compute alpha: (3.traceVspace) / (Utheta . acquisitionGain)
    //Utheta: amplitude maximal of theta in milivolts, 0.4 mv
    alpha =  static_cast<float>(static_cast<float>(3 * traceVspace) /
                                static_cast<float>(0.4 *  static_cast<float>(acquisitionGain)));

    //factor = alpha * (4/3)^gain, gain equals 0 at the begining
    gains.clear();
    channelFactors.clear();

    for(int i = 0; i < nbChannels; ++i){
        gains.append(0);
        channelFactors.append(alpha);
    }

    computeChannelDisplayGain();

    //Everything has to be redraw
    drawContentsMode = REDRAW;
}



void TraceView::computeChannelDisplayGain(){
    // Those gains are computed as (unitGain.alpha / screenResolution) .(world-viewport height ratio) .channelFactor).
    QRect r((QRect)window);
    float heightRatio = static_cast<float>(static_cast<float>(viewport.height()) / static_cast<float>(r.height()));
    float beta = static_cast<float>((static_cast<float>(unitGain) * alpha)/ static_cast<float>(screenResolution)) * heightRatio;

    channelDisplayGains.clear();

    for(int i = 0; i < nbChannels; ++i){
        channelDisplayGains.append(beta * pow(0.75,gains[i]));
    }

}

void TraceView::computeChannelDisplayGain(const QList<int>& channelIds){
    // Those gains are computed as (unitGain.alpha / screenResolution) .(world-viewport height ratio) .0.75^gain[i]).
    QRect r((QRect)window);
    float heightRatio = static_cast<float>(static_cast<float>(viewport.height()) / static_cast<float>(r.height()));
    float beta = static_cast<float>((static_cast<float>(unitGain) * alpha)/ static_cast<float>(screenResolution)) * heightRatio;

    for(int i = 0; i < static_cast<int>(channelIds.size()); ++i){
        int channelId = channelIds[i];
        channelDisplayGains[channelId] = static_cast<float>(beta * pow(0.75,gains[channelId]));
    }
}

void TraceView::increaseAllAmplitude(){
    //Increases the ordinate scale resulting in
    //an reduction of the traces in the ordinate direction.
    //factor = traceVspace / ((4/3)^gain * unitGain)
    for(int i = 0; i < nbChannels; ++i){
        gains[i]--;
        channelFactors[i] = static_cast<float>(alpha * pow(0.75,gains[i]));
    }

    computeChannelDisplayGain();

    //Everything has to be redraw
    drawContentsMode = REDRAW;
    update();
}


void TraceView::decreaseAllAmplitude(){
    //Decreases the ordinate scale resulting in
    //an enlargement of the traces in the ordinate direction.
    //factor = traceVspace / ((4/3)^gain * unitGain)
    for(int i = 0; i < nbChannels; ++i){
        gains[i]++;
        channelFactors[i] = static_cast<float>(alpha * pow(0.75,gains[i]));
    }

    computeChannelDisplayGain();

    //Everything has to be redraw
    drawContentsMode = REDRAW;
    update();
}

void TraceView::increaseSelectedChannelsAmplitude(const QList<int>& channelIds){
    //Increases the ordinate scale resulting in
    //an reduction of the traces in the ordinate direction.
    for(int i = 0; i < static_cast<int>(channelIds.size()); ++i){
        int channelId = channelIds[i];
        gains[channelId]--;
        channelFactors[channelId] = static_cast<float>(alpha * pow(0.75,gains[channelId]));
    }

    computeChannelDisplayGain(channelIds);

    //Everything has to be redraw
    drawContentsMode = REDRAW;
    update();
}

void TraceView::decreaseSelectedChannelsAmplitude(const QList<int>& channelIds){
    //Decreases the ordinate scale resulting in
    //an enlargement of the traces in the ordinate direction.
    for(int i = 0; i < static_cast<int>(channelIds.size()); ++i){
        int channelId = channelIds[i];
        gains[channelId]++;
        channelFactors[channelId] = static_cast<float>(alpha * pow(0.75,gains[channelId]));
    }

    computeChannelDisplayGain(channelIds);

    //Everything has to be redraw
    drawContentsMode = REDRAW;
    update();
}

void TraceView::updateWindow(){
    //reset the variables for the zoom state.
    firstZoom = true;
    zoomed = false;
    maxZoomReached = false;
    zoomFactor = 0;

    int nbGps = shownGroupsChannels.count();
    int nbShownchannels = shownChannels.size();
    int nbSamples = tracesProvider.getNbSamples(startTime,endTime,startTimeInRecordingUnits);

    int oldXshift = Xshift;

    //traces presented on a single column
    if(!multiColumns){
        if(!isInit){
            if(nbSamples < viewport.width() || printState) downSampling = 1;
            else downSampling = static_cast<float>(nbSamples) / static_cast<float>(viewport.width());
        }

        int nbSamplesToDraw = static_cast<int>(floor(0.5 + static_cast<float>(nbSamples)/downSampling));
        timeStep = downSampling * timeStepUnit;

        abscissaMax = 2 * borderX + (nbSamplesToDraw -1) * Xstep;

        int nbYspaces = 0;
        QMap<int, QList<int> >::Iterator iterator;
        for(iterator = shownGroupsChannels.begin(); iterator != shownGroupsChannels.end(); ++iterator){
            int currentNbChannels = static_cast<QList<int> >(iterator.data()).size();
            nbYspaces += (currentNbChannels - 1);
        }

        nbClusters = 0;
        QMap<int, QList<int> >::Iterator clustersIterator;
        for(clustersIterator = selectedClusters.begin(); clustersIterator != selectedClusters.end(); ++clustersIterator){
            int currentNbClusters = static_cast<QList<int> >(clustersIterator.data()).size();
            nbClusters += currentNbClusters;
        }

        //there are cluster rasters
        if(raster && nbClusters != 0){
            int rasterTotalHeight = YTracesRasterSeparator + nbClusters * rasterHeight + (nbClusters - 1) * YRasterSpace;

            ordinateMin = -(borderY + borderY/2 + (nbGps - 1) * YGroupSpace + nbShownchannels * traceVspace + nbYspaces * Yspace
                            + rasterTotalHeight);

            Y0 = borderY/2 + (nbGps - 1) * YGroupSpace + nbShownchannels * traceVspace + nbYspaces * Yspace - (traceVspace/2)
                    + rasterTotalHeight;

            Y0Raster = borderY/2 + rasterTotalHeight - YTracesRasterSeparator;
        }
        else{
            ordinateMin = -(2 * borderY + (nbGps - 1) * YGroupSpace + nbShownchannels * traceVspace + nbYspaces * Yspace);
            Y0 = borderY + (nbGps - 1) * YGroupSpace + nbShownchannels * traceVspace + nbYspaces * Yspace - (traceVspace/2);
        }
    }
    //traces presented on multiple columns
    else{
        if(!isInit){
            if(nbSamples < viewport.width() || printState) downSampling = 1;
            else{
                int nbGroups = shownGroupsChannels.count();
                if(nbGroups == 0) {
                    traceWidth = 0;
                    downSampling = 1;
                }
                else{
                    traceWidth = (viewport.width() - (nbGroups - 1) * xMargin) / nbGroups;
                    downSampling = static_cast<float>(nbSamples) / static_cast<float>(traceWidth);
                }
            }
        }

        int nbSamplesToDraw = static_cast<int>(floor(0.5 + static_cast<float>(nbSamples)/downSampling));
        timeStep = downSampling * timeStepUnit;

        //Those variables depend on the number of groups,the number of channels and the size of the previous window and viewport rectangles.
        XGroupSpace = viewportToWorldWidth(xMargin);
        abscissaMax = 2 * borderX + (nbSamplesToDraw -1) * Xstep * nbGps + (nbGps - 1) * XGroupSpace;
        Xshift = (nbSamplesToDraw - 1) * Xstep + XGroupSpace;

        int maxNbChannels = 0;
        QMap<int, QList<int> >::Iterator iterator;
        for(iterator = shownGroupsChannels.begin(); iterator != shownGroupsChannels.end(); ++iterator){
            int currentNbChannels = static_cast<QList<int> >(iterator.data()).size();
            if(currentNbChannels > maxNbChannels) maxNbChannels = currentNbChannels;
        }

        nbClusters = 0;
        QMap<int, QList<int> >::Iterator clustersIterator;
        for(clustersIterator = selectedClusters.begin(); clustersIterator != selectedClusters.end(); ++clustersIterator){
            int currentNbClusters = static_cast<QList<int> >(clustersIterator.data()).size();
            nbClusters += currentNbClusters;
        }

        //there are cluster raster
        if(raster && nbClusters != 0){
            int maxNbClusters = 0;

            //Compute the maximum number of clusters which will be drawn beneath a group
            QList<int> groupIds = shownGroupsChannels.keys();
            QList<int>::iterator iterator;
            for(iterator = groupIds.begin(); iterator != groupIds.end(); ++iterator){
                int currentNbClusters = 0;
                QList<int> clusterFileList = (*groupClusterFiles)[*iterator];
                QList<int>::iterator spikeGroupIterator;
                for(spikeGroupIterator = clusterFileList.begin(); spikeGroupIterator != clusterFileList.end(); ++spikeGroupIterator){
                    if(selectedClusters.contains(*spikeGroupIterator)){
                        QList<int> selection = static_cast<QList<int> >(selectedClusters[*spikeGroupIterator]);
                        currentNbClusters += selection.size();
                    }
                }
                if(currentNbClusters > maxNbClusters) maxNbClusters = currentNbClusters;
            }

            int rasterTotalHeight = YTracesRasterSeparator + maxNbClusters * rasterHeight + (maxNbClusters - 1) * YRasterSpace;

            ordinateMin = -(borderY + borderY/2 + maxNbChannels * traceVspace + (maxNbChannels - 1) * Yspace
                            + rasterTotalHeight);
            Y0 = borderY/2 + maxNbChannels * traceVspace + (maxNbChannels - 1) * Yspace - (traceVspace/2)
                    + rasterTotalHeight;

            Y0Raster = borderY/2 + rasterTotalHeight - YTracesRasterSeparator;
        }
        else{
            ordinateMin = -(2 * borderY + maxNbChannels * traceVspace + (maxNbChannels - 1) * Yspace);
            Y0 = borderY + maxNbChannels * traceVspace + (maxNbChannels - 1) * Yspace - (traceVspace/2);
        }
    }

    //Set the window
    window = ZoomWindow(QRect(QPoint(abscissaMin,ordinateMin),QPoint(abscissaMax,ordinateMax)));


    if(columnDisplayChanged || groupsChanged){
        columnDisplayChanged = false;
        groupsChanged = false;
        updateWindow();
    }

    //Certain variables used to compute the window in multicolumns mode depend on the size of the previous window and viewport rectangles.
    //To have an acurate window size, we loop until the size of the window is stabilized.
    if(multiColumns && oldXshift != Xshift) updateWindow();

    //Recompute the display gains
    computeChannelDisplayGain();

    //Everything has to be redraw
    drawContentsMode = REDRAW;
}


void TraceView::drawTrace(QPainter& painter,int limit,int basePosition,int X,int channelId,int nbSamplesToDraw,bool mouseMoveEvent){
    bool areClustersToDraw = false;
    int clusterFileId = 0;
    QString providerName;
    if(!clusterProviders.isEmpty()){
        clusterFileId = (*channelClusterFiles)[channelId];
        providerName = QString::fromLatin1("%1").arg(clusterFileId);
        if(clustersData.find(providerName) != 0) areClustersToDraw = true;
    }

    if(!waveforms || (waveforms && !areClustersToDraw) || (waveforms && areClustersToDraw && mouseMoveEvent) || (waveforms && areClustersToDraw && !selectedClusters.contains(clusterFileId))){
        int start = 1;
        int stop = static_cast<int>(floor(downSampling + 0.5));//included
        long min = data(1,channelId + 1);
        long max = min;
        long value;
        int nbSamples = tracesProvider.getNbSamples(startTime,endTime,startTimeInRecordingUnits);

        for(int k = start + 1;k<= stop;++k){
            value = data(k,channelId + 1);
            if(value < min) min = value;
            if(value > max) max = value;
        }
        int yMin = basePosition - static_cast<long>(min * channelFactors[channelId]);
        int yMax = basePosition - static_cast<long>(max * channelFactors[channelId]);
        if((yMax - yMin) <= limit){
            painter.drawPoint(X,yMin);
        }
        else{
            painter.drawLine(X,yMin,X,yMax);
        }
        X += Xstep;
        long previousMin = min;
        long previousMax = max;

        for(int i = 2; i <= nbSamplesToDraw;++i){
            start = static_cast<int>(floor((i-1) * downSampling + 0.5 + 1));//the index in data starts at 1
            stop = qMin(static_cast<int>(floor(i * downSampling + 0.5)),nbSamples);

            min = data(start,channelId + 1);
            max = min;
            for(int k = start + 1;k<= stop;++k){
                value = data(k,channelId + 1);
                if(value < min) min = value;
                if(value > max) max = value;
            }
            if(min > previousMax) min = previousMax;
            if(max < previousMin) max = previousMin;
            previousMax = max;
            previousMin = min;

            yMax = basePosition - static_cast<long>(min * channelFactors[channelId]);
            yMin = basePosition - static_cast<long>(max * channelFactors[channelId]);

            if((yMax - yMin) <= limit){
                painter.drawPoint(X,yMin);
            }
            else{
                painter.drawLine(X,yMin,X,yMax);
            }
            X += Xstep;
        }
    }
    else{
        //Array containing 4 lines: sample starting index, abscissa, ordinate min, ordinate max
        Array<dataType> traceInfo(4,nbSamplesToDraw);

        int start = 1;
        int stop = static_cast<int>(floor(downSampling + 0.5));//included
        long min = data(1,channelId + 1);
        long max = min;
        long value;
        int nbSamples = tracesProvider.getNbSamples(startTime,endTime,startTimeInRecordingUnits);

        for(int k = start + 1;k<= stop;++k){
            value = data(k,channelId + 1);
            if(value < min) min = value;
            if(value > max) max = value;
        }
        int yMin = basePosition - static_cast<long>(min * channelFactors[channelId]);
        int yMax = basePosition - static_cast<long>(max * channelFactors[channelId]);
        if((yMax - yMin) <= limit){
            painter.drawPoint(X,yMin);
        }
        else{
            painter.drawLine(X,yMin,X,yMax);
        }

        traceInfo(1,1) = 1;
        traceInfo(2,1) = X;
        traceInfo(3,1) = yMin;
        traceInfo(4,1) = yMax;

        X += Xstep;
        long previousMin = min;
        long previousMax = max;

        for(int i = 2; i <= nbSamplesToDraw;++i){
            start = static_cast<int>(floor((i-1) * downSampling + 0.5 + 1));//the index in data starts at 1
            stop = qMin(static_cast<int>(floor(i * downSampling + 0.5)),nbSamples);

            min = data(start,channelId + 1);
            max = min;
            for(int k = start + 1;k<= stop;++k){
                value = data(k,channelId + 1);
                if(value < min) min = value;
                if(value > max) max = value;
            }
            if(min > previousMax) min = previousMax;
            if(max < previousMin) max = previousMin;
            previousMax = max;
            previousMin = min;

            yMax = basePosition - static_cast<long>(min * channelFactors[channelId]);
            yMin = basePosition - static_cast<long>(max * channelFactors[channelId]);

            if((yMax - yMin) <= limit){
                painter.drawPoint(X,yMin);
            }
            else{
                painter.drawLine(X,yMin,X,yMax);
            }

            traceInfo(1,i) = start;
            traceInfo(2,i) = X;
            traceInfo(3,i) = yMin;
            traceInfo(4,i) = yMax;

            X += Xstep;
        }

        //Draw the waveforms on top of the trace
        ItemColors* colors = providerItemColors[providerName];
        Array<dataType>& currentData = static_cast<ClusterData*>(clustersData[providerName])->getData();
        int nbSpikes = currentData.nbOfColumns();
        QList<int> clusterList = selectedClusters[clusterFileId];
        int currentIndex = 1;

        for(int i = 1; i < nbSpikes + 1;++i){
            dataType index = currentData(1,i);
            int firstIndex = index - nbSamplesBefore;
            int lastIndex = index + nbSamplesAfter;
            dataType clusterId = currentData(2,i);

            if(clusterList.contains(clusterId)){
                QColor color = colors->color(clusterId);
                painter.setPen(color);
                for(int j = currentIndex; j <= nbSamplesToDraw;++j){
                    if(firstIndex > traceInfo(1,j)) continue;//case 1
                    else if(firstIndex <= traceInfo(1,j)){//case 2
                        for(int k = j;k<= nbSamplesToDraw;++k){
                            if(traceInfo(1,k) > lastIndex){
                                currentIndex = k;
                                break;
                            }
                            else if(traceInfo(1,k) <= lastIndex){
                                if(traceInfo(3,k) != traceInfo(4,k)) painter.drawLine(traceInfo(2,k),traceInfo(3,k),traceInfo(2,k),traceInfo(4,k));
                                else painter.drawPoint(traceInfo(2,k),traceInfo(3,k));
                            }
                        }
                        break;
                    }
                }//loop on samples to draw
            }
        }//loop on spikes
    }//waveform to draw

}

void TraceView::drawTraces( const QList<int>& channels,bool highlight){
    QRect r((QRect)window);

    //Create a painter to paint on the double buffer
    QPainter painter;
    painter.begin(&doublebuffer);

    painter.setWindow(r.left(),r.top(),r.width()-1,r.height()-1);//hack because Qt QRect is used differently in this function

    //Set the viewport (part of the device I want to write on).
    //By default, the viewport is the same as the device's rectangle (contentsRec), taking a smaller
    //one will ensure that the legends (cluster ids) will not ovelap a correlogram.
    painter.setViewport(viewport);


    //Paint the selected channels
    int nbSamples = tracesProvider.getNbSamples(startTime,endTime,startTimeInRecordingUnits);
    int nbSamplesToDraw = static_cast<int>(floor(0.5 + static_cast<float>(nbSamples)/downSampling));
    int limit = viewportToWorldHeight(1);

    QList<int>::const_iterator iterator;
    for(iterator = channels.constBegin();iterator != channels.constEnd();++iterator){
        //if the channel is skipped, do no draw it
        if(skippedChannels.contains(*iterator)) continue;

        int basePosition = channelsStartingOrdinate[*iterator] +  static_cast<long>(data(1,*iterator + 1) * channelFactors[*iterator]);

        //The abscissa of the system coordinate center for the current channel
        int X;
        if(!multiColumns) X = X0;
        else X = channelsStartingAbscissa[*iterator];

        //Get the color associated with the channel and set the color to use to this color
        QColor color = channelColors->color(*iterator);
        if(greyScaleMode){
            int greyvalue = qGray(color.rgb());
            color.setHsv(0,0,greyvalue);
        }

        if(downSampling != 1){
            //draw the trace
            if(highlight){
                QPen pen(color,2);
                painter.setPen(pen);
                drawTrace(painter,limit,basePosition,X,*iterator,nbSamplesToDraw);
            }//highlight
            else{
                //erase the previous trace
                QPen pen(backgroundColor(),2);
                painter.setPen(pen);
                drawTrace(painter,limit,basePosition,X,*iterator,nbSamplesToDraw);

                //redraw the trace
                pen.setColor(color);
                pen.setWidth(1);
                painter.setPen(pen);
                if(!multiColumns) X = X0;
                else X = channelsStartingAbscissa[*iterator];
                drawTrace(painter,limit,basePosition,X,*iterator,nbSamplesToDraw);
            }
        }
        else{
            bool areClustersToDraw = false;
            int clusterFileId = 0;
            QString providerName;
            if(!clusterProviders.isEmpty()){
                areClustersToDraw = true;
                clusterFileId = (*channelClusterFiles)[*iterator];
                providerName = QString::fromLatin1("%1").arg(clusterFileId);
                if(clustersData.find(providerName) != 0) areClustersToDraw = true;
            }

            if(!waveforms || (waveforms && !areClustersToDraw) || (waveforms && areClustersToDraw && !selectedClusters.contains(clusterFileId))){
                QPolygon trace(nbSamples);
                for(int i = 0; i < nbSamples;++i){
                    int y = basePosition - static_cast<long>(data(i + 1,*iterator + 1) * channelFactors[*iterator]);
                    trace.setPoint(i,X,y);
                    X += Xstep;
                }
                if(highlight){
                    QPen pen(color,2);
                    painter.setPen(pen);
                    painter.drawPolyline(trace);
                }
                else{
                    QPen pen(backgroundColor(),2);
                    painter.setPen(pen);
                    painter.drawPolyline(trace);
                    pen.setColor(color);
                    pen.setWidth(1);
                    painter.setPen(pen);
                    painter.drawPolyline(trace);
                }
            }
            else{
                //Array containing 3 lines: sample index, abscissa and ordinate
                Array<dataType> traceInfo(3,nbSamples);
                QPolygon trace(nbSamples);
                for(int i = 1; i <= nbSamples;++i){
                    int y = basePosition - static_cast<long>(data(i,*iterator + 1) * channelFactors[*iterator]);
                    trace.setPoint(i - 1,X,y);
                    traceInfo(1,i) = i;
                    traceInfo(2,i) = X;
                    traceInfo(3,i) = y;
                    X += Xstep;
                }
                if(highlight){
                    QPen pen(color,2);
                    painter.setPen(pen);
                    painter.drawPolyline(trace);
                }
                else{
                    QPen pen(backgroundColor(),2);
                    painter.setPen(pen);
                    painter.drawPolyline(trace);
                    pen.setColor(color);
                    pen.setWidth(1);
                    painter.setPen(pen);
                    painter.drawPolyline(trace);
                }

                //Draw the waveforms on top of the trace
                ItemColors* colors = providerItemColors[providerName];
                Array<dataType>& currentData = static_cast<ClusterData*>(clustersData[providerName])->getData();
                int nbSpikes = currentData.nbOfColumns();
                QList<int> clusterList = selectedClusters[clusterFileId];
                int currentIndex = 1;

                for(int i = 1; i < nbSpikes + 1;++i){
                    dataType index = currentData(1,i);
                    int firstIndex = qMax(0L,index - nbSamplesBefore);
                    int lastIndex = qMin((long)nbSamples,index + nbSamplesAfter);
                    int nbWaveformSamples = lastIndex - firstIndex + 1;
                    dataType clusterId = currentData(2,i);

                    if(clusterList.contains(clusterId)){
                        QColor color = colors->color(clusterId);
                        for(int j = currentIndex; j <= nbSamples;++j){
                            if(firstIndex > traceInfo(1,j)) continue;//case 1
                            else if(firstIndex == traceInfo(1,j)){//case 2
                                QPolygon trace(nbWaveformSamples);
                                int pos = 0;
                                for(int k = firstIndex;k<= lastIndex;++k){
                                    trace.setPoint(pos,traceInfo(2,k),traceInfo(3,k));
                                    pos++;
                                }
                                if(highlight){
                                    QPen pen(color,2);
                                    painter.setPen(pen);
                                    painter.drawPolyline(trace);
                                }
                                else{
                                    QPen pen(backgroundColor(),2);
                                    painter.setPen(pen);
                                    painter.drawPolyline(trace);
                                    pen.setColor(color);
                                    pen.setWidth(1);
                                    painter.setPen(pen);
                                    painter.drawPolyline(trace);
                                }
                                currentIndex = firstIndex;
                                break;
                            }
                        }//loop on samples to draw
                    }
                }//loop on spikes
            }//else waveform
        }
    }

    //reset transformation due to setWindow and setViewport
    painter.resetMatrix() ;

    //Draw channel ids and amplitude on the left side.
    if(showLabels){
        QFont f("Helvetica",8);
        painter.setFont(f);
        painter.setPen(colorLegend); //set the color for the legends.
        QRect windowRectangle((QRect)window);
        for(iterator = channels.constBegin();iterator != channels.constEnd();++iterator){
            //if the channel is skipped, do no draw it
            if(skippedChannels.contains(*iterator)) continue;

            int position = channelsStartingOrdinate[*iterator];
            int abscissa = channelsStartingAbscissa[*iterator];
            QRect r;
            QRect rHighlight;
            //If the view was zoomed and the left margin (where the ids and gains of the channels of the first group are displayed) is not
            //shown (r.left() != 0), the coordinates have to be adjusted. Indeed, this margin is outside the world but in the viewport.
            if(windowRectangle.left() != 0){
                r = QRect(worldToViewport(abscissa,position).x() - xMargin,worldToViewport(abscissa,position).y(),xMargin - 4,worldToViewportHeight(traceVspace + Yspace));
                rHighlight = QRect(worldToViewport(abscissa,position).x() - xMargin,worldToViewport(abscissa,position).y(),xMargin - 4,12);
            }
            else{
                r = QRect(worldToViewport(abscissa,position).x() + 4,worldToViewport(abscissa,position).y(),xMargin - 4,worldToViewportHeight(traceVspace + Yspace));
                rHighlight = QRect(worldToViewport(abscissa,position).x() + 4,worldToViewport(abscissa,position).y(),xMargin - 4,12);
            }
            float gain = channelDisplayGains[*iterator];
            if(highlight) painter.fillRect(rHighlight,colorGroup().highlight());
            else painter.fillRect(rHighlight,backgroundColor());
            painter.drawText(r,Qt::AlignHCenter | Qt::AlignTop,QString("%1 x%2").arg(*iterator).arg(gain,0,'f',2));
        }
    }


    //Closes the painter on the double buffer
    painter.end();

    //Draw the double buffer (pixmap) by copying it into the widget device.
    update();

}

void TraceView::drawTraces(QPainter& painter){
    channelsStartingOrdinate.clear();
    int limit = viewportToWorldHeight(1);
    int nbSamples = tracesProvider.getNbSamples(startTime,endTime,startTimeInRecordingUnits);
    int nbSamplesToDraw = static_cast<int>(floor(0.5 + static_cast<float>(nbSamples)/downSampling));

    //traces presented on multiple columns
    if(multiColumns){
        //The abscissa of the system coordinate center for the current channel
        int X = X0;
        //The ordinate of the system coordinate center for the current channel
        int Y = Y0;

        //Start at the top of the view.

        //Loop on all the groups (one by column)
        clustersOrder.clear();
        rasterOrdinates.clear();
        rasterAbscisses.clear();
        QList<int> groupIds = shownGroupsChannels.keys();
        QList<int>::iterator iterator;
        for(iterator = groupIds.begin(); iterator != groupIds.end(); ++iterator){

            //Draw events
            if(!selectedEvents.isEmpty()){
                QRect windowRectangle((QRect)window);
                int top = windowRectangle.top();
                int bottom = windowRectangle.bottom();
                QPen pen(Qt::DotLine);

                QMap<QString, QList<int> >::Iterator iterator;
                for(iterator = selectedEvents.begin(); iterator != selectedEvents.end(); ++iterator){
                    QList<int> eventList = iterator.data();
                    QString providerName = iterator.key();
                    if(eventList.size() == 0 || eventsData[providerName] == 0) continue;
                    ItemColors* colors = providerItemColors[providerName];
                    Array<dataType>& currentData = static_cast<EventData*>(eventsData[providerName])->getTimes();
                    Array<int>& currentIds = static_cast<EventData*>(eventsData[providerName])->getIds();
                    int nbEvents = currentData.nbOfColumns();
                    for(int i = 1; i <= nbEvents;++i){
                        dataType index = currentData(1,i);
                        int eventId = currentIds(1,i);
                        if(eventList.contains(eventId)){
                            QColor color = colors->color(eventId);
                            pen.setColor(color);
                            painter.setPen(pen);
                            int abscissa = X + static_cast<int>(0.5 + (static_cast<float>(index) / downSampling));
                            painter.drawLine(abscissa,top,abscissa,bottom);
                        }
                    }
                }
            }//events
            if(verticalLines && nbClusters != 0){
                QRect windowRectangle((QRect)window);
                int top = windowRectangle.top();
                int bottom = windowRectangle.bottom();
                QList<int> clusterFileList = (*groupClusterFiles)[*iterator];

                QMap<int,QList<int> >::Iterator selectedIterator;
                for(selectedIterator = selectedClusters.begin(); selectedIterator != selectedClusters.end(); ++selectedIterator){
                    //Only draw vertical lines for clusters contained in a cluster file containing data for channels of the current group
                    if(!clusterFileList.contains(selectedIterator.key())) continue;
                    QString providerName = QString::fromLatin1("%1").arg(selectedIterator.key());
                    if(clustersData[providerName] == 0) continue;

                    ItemColors* colors = providerItemColors[providerName];
                    Array<dataType>& currentData = static_cast<ClusterData*>(clustersData[providerName])->getData();
                    int nbSpikes = currentData.nbOfColumns();
                    QList<int> clusterList = selectedIterator.data();
                    QList<int>::iterator clusterIterator;
                    for(clusterIterator = clusterList.begin(); clusterIterator != clusterList.end(); ++clusterIterator){
                        QColor color = colors->color(*clusterIterator);
                        painter.setPen(color);
                        for(int i = 1; i <= nbSpikes;++i){
                            dataType index = currentData(1,i);
                            dataType clusterId = currentData(2,i);
                            if(clusterId == *clusterIterator){
                                int abscissa = X + static_cast<int>(0.5 + (static_cast<float>(index) / downSampling));
                                painter.drawLine(abscissa,top,abscissa,bottom);
                            }
                        }
                    }
                }
            }//verticalLines

            QList<int> channelIds = shownGroupsChannels[*iterator];
            int currentNbChannels = channelIds.size();

            QList<int> positions;
            int y = Y;
            for(int j = 0; j < currentNbChannels; ++j){
                int channelId = channelIds[j];
                int position = -y + channelOffsets[channelId];
                positions.append(position);
                channelsStartingOrdinate.insert(channelId,position - static_cast<long>(data(1,channelId + 1) * channelFactors[channelId]));
                channelsStartingAbscissa.insert(channelId,X);
                y -= Yshift;
            }

            for(int j = 0; j < currentNbChannels; ++j){
                int channelId = channelIds[j];
                //The abscissa of the system coordinate center for the current channel
                int x = 0;

                //if the channel is skipped, do no draw it
                if(skippedChannels.contains(channelId)) continue;

                QColor color = channelColors->color(channelId);

                if(greyScaleMode){
                    int greyvalue = qGray(color.rgb());
                    color.setHsv(0,0,greyvalue);
                }
                QPen pen(color,1);
                if(selectedChannels.contains(channelId)) pen.setWidth(2);
                painter.setPen(pen);

                if(downSampling != 1){
                    drawTrace(painter,limit,positions[j],X,channelId,nbSamplesToDraw);
                }
                else{
                    bool areClustersToDraw = false;
                    int clusterFileId = 0;
                    QString providerName;
                    if(!clusterProviders.isEmpty()){
                        areClustersToDraw = true;
                        clusterFileId = (*channelClusterFiles)[channelId];
                        providerName = QString::fromLatin1("%1").arg(clusterFileId);
                        if(clustersData.find(providerName) != 0) areClustersToDraw = true;
                    }

                    if(!waveforms || (waveforms && !areClustersToDraw) || (waveforms && areClustersToDraw && !selectedClusters.contains(clusterFileId))){
                        QPolygon trace(nbSamples);
                        for(int i = 0; i < nbSamples;++i){
                            int y = positions[j] - static_cast<long>(data(i + 1,channelId + 1) * channelFactors[channelId]);
                            trace.setPoint(i,X + x,y);
                            x += Xstep;
                        }
                        painter.drawPolyline(trace);
                    }
                    else{
                        //Array containing 3 lines: sample index, abscissa and ordinate
                        Array<dataType> traceInfo(3,nbSamples);
                        QPolygon trace(nbSamples);
                        for(int i = 1; i <= nbSamples;++i){
                            int y = positions[j] - static_cast<long>(data(i,channelId + 1) * channelFactors[channelId]);
                            trace.setPoint(i - 1,X + x,y);
                            traceInfo(1,i) = i;
                            traceInfo(2,i) = X + x;
                            traceInfo(3,i) = y;
                            x += Xstep;
                        }
                        painter.drawPolyline(trace);

                        //Draw the waveforms on top of the trace
                        ItemColors* colors = providerItemColors[providerName];
                        Array<dataType>& currentData = static_cast<ClusterData*>(clustersData[providerName])->getData();
                        int nbSpikes = currentData.nbOfColumns();
                        QList<int> clusterList = selectedClusters[clusterFileId];
                        int currentIndex = 1;

                        for(int i = 1; i < nbSpikes + 1;++i){
                            dataType index = currentData(1,i);
                            int firstIndex = qMax(1L,index - nbSamplesBefore);
                            int lastIndex = qMin((long)nbSamples,index + nbSamplesAfter);
                            int nbWaveformSamples = lastIndex - firstIndex + 1;
                            dataType clusterId = currentData(2,i);

                            if(clusterList.contains(clusterId)){
                                QColor color = colors->color(clusterId);
                                QPen pen(color,1);
                                if(selectedChannels.contains(channelId)) pen.setWidth(2);
                                painter.setPen(pen);

                                for(int j = currentIndex; j <= nbSamples;++j){
                                    if(firstIndex > traceInfo(1,j)) continue;//case 1
                                    else if(firstIndex == traceInfo(1,j)){//case 2
                                        QPolygon trace(nbWaveformSamples);
                                        int pos = 0;
                                        for(int k = firstIndex;k<= lastIndex;++k){
                                            trace.setPoint(pos,traceInfo(2,k),traceInfo(3,k));
                                            pos++;
                                        }
                                        painter.drawPolyline(trace);
                                        currentIndex = firstIndex;
                                        break;
                                    }
                                }//loop on samples to draw
                            }
                        }//loop on spikes
                    }//else waveform
                }
            }

            //Loop on all the selected clusters (first on the cluster files containing selected clusters) if the raster is asked.
            if(raster && nbClusters != 0){
                int y = Y0Raster;

                QList<int> clusterFileList = (*groupClusterFiles)[*iterator];

                QMap<int,QList<int> >::Iterator selectedIterator;
                for(selectedIterator = selectedClusters.begin(); selectedIterator != selectedClusters.end(); ++selectedIterator){
                    //Only draw rasters for clusters contained in a cluster file containing data for channels of the current group
                    if(!clusterFileList.contains(selectedIterator.key())) continue;
                    QList<int> clusterList = selectedIterator.data();
                    if(clusterList.size() == 0) continue;
                    QString providerName = QString::fromLatin1("%1").arg(selectedIterator.key());
                    ItemColors* colors = providerItemColors[providerName];
                    Array<dataType>& currentData = static_cast<ClusterData*>(clustersData[providerName])->getData();
                    int nbSpikes = currentData.nbOfColumns();
                    QList<int>::iterator clusterIterator;
                    for(clusterIterator = clusterList.begin(); clusterIterator != clusterList.end(); ++clusterIterator){
                        QString identifier = QString("%1-%2").arg(providerName).arg(*clusterIterator);

                        qDebug()<<" identifier " <<identifier<<" nbSpikes " <<nbSpikes ;


                        clustersOrder.append(identifier);
                        rasterOrdinates.append(-y);
                        rasterAbscisses.append(X);
                        QColor color = colors->color(*clusterIterator);
                        painter.setPen(color);
                        int bottom = y - rasterHeight;
                        for(int i = 1; i <= nbSpikes;++i){
                            dataType index = currentData(1,i);
                            dataType clusterId = currentData(2,i);
                            if(clusterId == *clusterIterator){
                                int abscissa = X + static_cast<int>(0.5 + (static_cast<float>(index) / downSampling));
                                painter.drawLine(abscissa,-y,abscissa,-bottom);
                            }
                        }
                        y -= (rasterHeight + YRasterSpace);
                    }
                }
            }//raster
            X += Xshift;
        }//groups (<=> columns)
    }//multicolumns
    //traces presented on a single column
    else{
        //Draw events
        if(!selectedEvents.isEmpty()){
            QRect windowRectangle((QRect)window);
            int top = windowRectangle.top();
            int bottom = windowRectangle.bottom();
            QPen pen(Qt::DotLine);
            QMap<QString, QList<int> >::Iterator iterator;
            for(iterator = selectedEvents.begin(); iterator != selectedEvents.end(); ++iterator){
                QList<int> eventList = iterator.data();
                QString providerName = iterator.key();

                if(eventList.size() == 0 || eventsData[providerName] == 0) continue;
                ItemColors* colors = providerItemColors[providerName];
                Array<dataType>& currentData = static_cast<EventData*>(eventsData[providerName])->getTimes();
                Array<int>& currentIds = static_cast<EventData*>(eventsData[providerName])->getIds();
                int nbEvents = currentData.nbOfColumns();
                for(int i = 1; i <= nbEvents;++i){
                    dataType index = currentData(1,i);
                    int eventId = currentIds(1,i);
                    if(eventList.contains(eventId)){
                        QColor color = colors->color(eventId);
                        pen.setColor(color);
                        painter.setPen(pen);
                        int abscissa = static_cast<int>(0.5 + (static_cast<float>(index) / downSampling));
                        painter.drawLine(abscissa,top,abscissa,bottom);
                    }
                }
            }
        }//events

        //Draw clusters on vertical lines
        if(verticalLines && nbClusters != 0){
            QRect windowRectangle((QRect)window);
            int top = windowRectangle.top();
            int bottom = windowRectangle.bottom();
            Q3DictIterator<ClusterData> iterator(clustersData);
            for(;iterator.current();++iterator){
                ItemColors* colors = providerItemColors[iterator.currentKey()];
                QList<int> clusterList = selectedClusters[iterator.currentKey().toInt()];
                Array<dataType>& currentData = iterator.current()->getData();
                int nbSpikes = currentData.nbOfColumns();

                for(int i = 1; i < nbSpikes + 1;++i){
                    dataType index = currentData(1,i);
                    dataType clusterId = currentData(2,i);

                    if(clusterList.contains(clusterId)){
                        QColor color = colors->color(clusterId);
                        painter.setPen(color);
                        int abscissa = static_cast<int>(0.5 + (static_cast<float>(index) / downSampling));
                        painter.drawLine(abscissa,top,abscissa,bottom);
                    }
                }
            }
        }//verticalLines

        //The ordinate of the system coordinate center for the current channel
        int Y = Y0;
        //Start at the top of the view.

        //Loop on all the groups
        QList<int> groupIds = shownGroupsChannels.keys();
        QList<int>::iterator iterator;
        for(iterator = groupIds.begin(); iterator != groupIds.end(); ++iterator){
            QList<int> channelIds = shownGroupsChannels[*iterator];
            int currentNbChannels = channelIds.size();

            QList<int> positions;
            int y = Y;
            for(int j = 0; j < currentNbChannels; ++j){
                int channelId = channelIds[j];
                int position = -y + channelOffsets[channelId];
                positions.append(position);
                channelsStartingOrdinate.insert(channelId,position - static_cast<long>(data(1,channelId + 1) * channelFactors[channelId]));
                channelsStartingAbscissa.insert(channelId,X0);
                y -= Yshift;
            }


            for(int j = 0; j < currentNbChannels; ++j){
                int channelId = channelIds[j];

                //if the channel is skipped, do no draw it
                if(skippedChannels.contains(channelId)) continue;

                //The abscissa of the system coordinate center for the current channel
                int X = X0;

                //Get the color associated with the channel and set the color to use to this color
                QColor color = channelColors->color(channelId);
                if(greyScaleMode){
                    int greyvalue = qGray(color.rgb());
                    color.setHsv(0,0,greyvalue);
                }
                QPen pen(color,1);
                if(selectedChannels.contains(channelId)) pen.setWidth(2);
                painter.setPen(pen);

                if(downSampling != 1){
                    drawTrace(painter,limit,positions[j],X,channelId,nbSamplesToDraw);
                }
                else{
                    bool areClustersToDraw = false;
                    int clusterFileId = 0;
                    QString providerName;

                    if(!clusterProviders.isEmpty()){
                        areClustersToDraw = true;
                        clusterFileId = (*channelClusterFiles)[channelId];
                        providerName = QString::fromLatin1("%1").arg(clusterFileId);
                        if(clustersData.find(providerName) != 0) areClustersToDraw = true;
                    }

                    if(!waveforms || (waveforms && !areClustersToDraw) || (waveforms && areClustersToDraw && !selectedClusters.contains(clusterFileId))){
                        QPolygon trace(nbSamples);
                        for(int i = 0; i < nbSamples;++i){
                            int y = positions[j] - static_cast<long>(data(i + 1,channelId + 1) * channelFactors[channelId]);
                            trace.setPoint(i,X,y);
                            X += Xstep;
                        }
                        painter.drawPolyline(trace);
                    }
                    else{
                        //Array containing 3 lines: sample index, abscissa and ordinate
                        Array<dataType> traceInfo(3,nbSamples);
                        QPolygon trace(nbSamples);
                        for(int i = 1; i <= nbSamples;++i){
                            int y = positions[j] - static_cast<long>(data(i,channelId + 1) * channelFactors[channelId]);
                            trace.setPoint(i - 1,X,y);
                            traceInfo(1,i) = i;
                            traceInfo(2,i) = X;
                            traceInfo(3,i) = y;
                            X += Xstep;
                        }
                        painter.drawPolyline(trace);

                        //Draw the waveforms on top of the trace
                        ItemColors* colors = providerItemColors[providerName];

                        Array<dataType>& currentData = static_cast<ClusterData*>(clustersData[providerName])->getData();
                        int nbSpikes = currentData.nbOfColumns();
                        QList<int> clusterList = selectedClusters[clusterFileId];
                        int currentIndex = 1;

                        for(int i = 1; i < nbSpikes + 1;++i){
                            dataType index = currentData(1,i);
                            int firstIndex = qMax(1L,index - nbSamplesBefore);
                            int lastIndex = qMin((long)nbSamples,index + nbSamplesAfter);
                            int nbWaveformSamples = lastIndex - firstIndex + 1;
                            dataType clusterId = currentData(2,i);

                            if(clusterList.contains(clusterId)){
                                QColor color = colors->color(clusterId);
                                QPen pen(color,1);
                                if(selectedChannels.contains(channelId)) pen.setWidth(2);
                                painter.setPen(pen);

                                for(int j = currentIndex; j <= nbSamples;++j){
                                    if(firstIndex > traceInfo(1,j)) continue;//case 1
                                    else if(firstIndex == traceInfo(1,j)){//case 2
                                        QPolygon trace(nbWaveformSamples);
                                        int pos = 0;
                                        for(int k = firstIndex;k<= lastIndex;++k){
                                            trace.setPoint(pos,traceInfo(2,k),traceInfo(3,k));
                                            pos++;
                                        }
                                        painter.drawPolyline(trace);
                                        currentIndex = firstIndex;
                                        break;
                                    }
                                }//loop on samples to draw
                            }
                        }//loop on spikes
                    }//else waveform
                }

            }

            Y -= (currentNbChannels * traceVspace + (currentNbChannels -1) * Yspace);
            Y -= YGroupSpace;
        }//groups

        clustersOrder.clear();
        rasterOrdinates.clear();
        rasterAbscisses.clear();

        //Loop on all the selected clusters (first on the cluster files containing selected clusters) if the raster is asked.
        if(raster && nbClusters != 0){
            Y = Y0Raster;

            QMap<int,QList<int> >::Iterator iterator;
            for(iterator = selectedClusters.begin(); iterator != selectedClusters.end(); ++iterator){
                QList<int> clusterList = iterator.data();
                if(clusterList.size() == 0) continue;
                QString providerName = QString::fromLatin1("%1").arg(iterator.key());
                ItemColors* colors = providerItemColors[providerName];
                Array<dataType>& currentData = static_cast<ClusterData*>(clustersData[providerName])->getData();
                int nbSpikes = currentData.nbOfColumns();
                QList<int>::iterator clusterIterator;
                for(clusterIterator = clusterList.begin(); clusterIterator != clusterList.end(); ++clusterIterator){
                    QString identifier = QString("%1-%2").arg(providerName).arg(*clusterIterator);

                    qDebug()<<" *** identifier " <<identifier<<" nbSpikes " <<nbSpikes ;

                    clustersOrder.append(identifier);
                    rasterOrdinates.append(-Y);
                    rasterAbscisses.append(0);
                    QColor color = colors->color(*clusterIterator);
                    painter.setPen(color);
                    int bottom = Y - rasterHeight;
                    for(int i = 1; i < nbSpikes + 1;++i){
                        dataType index = currentData(1,i);
                        dataType clusterId = currentData(2,i);
                        if(clusterId == *clusterIterator){
                            int abscissa = static_cast<int>(0.5 + (static_cast<float>(index) / downSampling));
                            painter.drawLine(abscissa,-Y,abscissa,-bottom);
                        }
                    }
                    Y -= (rasterHeight + YRasterSpace);
                }
            }
        }//raster
    }//single column

}

void TraceView::drawChannelIdsAndGain(QPainter& painter){
    QFont f("Helvetica",8);
    painter.setFont(f);
    painter.setPen(colorLegend); //set the color for the legends.
    QRect windowRectangle((QRect)window);

    if(!multiColumns && (windowRectangle.left() >= xMargin)){
    }
    else{
        QList<int> groupIds = shownGroupsChannels.keys();
        QList<int>::iterator iterator;
        for(iterator = groupIds.begin(); iterator != groupIds.end(); ++iterator){
            QList<int> channelIds = shownGroupsChannels[*iterator];
            QList<int>::iterator channelIterator;
            for(channelIterator = channelIds.begin(); channelIterator != channelIds.end(); ++channelIterator){
                //if the channel is skipped, do no draw it
                // if(skippedChannels.contains(*channelIterator )) continue;

                int position = channelsStartingOrdinate[*channelIterator];
                int abscissa = channelsStartingAbscissa[*channelIterator];
                QRect r;
                QRect rHighlight;
                //If the view was zoomed and the left margin (where the ids and gains of the channels of the first group are displayed) is not
                //shown (r.left() != 0), the coordinates have to be adjusted. Indeed, this margin is outside the world but in the viewport.
                if(windowRectangle.left() != 0){
                    r = QRect(worldToViewport(abscissa,position).x() - xMargin,worldToViewport(abscissa,position).y(),xMargin - 4,worldToViewportHeight(traceVspace + Yspace));
                    rHighlight = QRect(worldToViewport(abscissa,position).x() - xMargin,worldToViewport(abscissa,position).y(),xMargin - 4,12);
                }
                else{
                    r = QRect(worldToViewport(abscissa,position).x() + 4,worldToViewport(abscissa,position).y(),xMargin - 4,worldToViewportHeight(traceVspace + Yspace));
                    rHighlight = QRect(worldToViewport(abscissa,position).x() + 4,worldToViewport(abscissa,position).y(),xMargin - 4,12);
                }
                float gain = channelDisplayGains[*channelIterator];
                if(selectedChannels.contains(*channelIterator)) painter.fillRect(rHighlight,colorGroup().highlight());
                else painter.fillRect(rHighlight,backgroundColor());
                painter.drawText(r,Qt::AlignHCenter | Qt::AlignTop,QString("%1 x%2").arg(*channelIterator).arg(gain,0,'f',2));
            }
        }
        if(raster){
            //Draw the cluster labels
            for(int i = 0;i < static_cast<int>(clustersOrder.size());++i){
                int position = rasterOrdinates[i];
                QString clusterIdentifier = clustersOrder[i];
                int abscissa = rasterAbscisses[i];
                QRect r;
                QRect rHighlight;

                if(windowRectangle.left() != 0){
                    r = QRect(worldToViewport(abscissa,position).x() - xMargin,worldToViewport(abscissa,position).y(),xMargin - 4,worldToViewportHeight(traceVspace + Yspace));
                    rHighlight = QRect(worldToViewport(abscissa,position).x() - xMargin,worldToViewport(abscissa,position).y(),xMargin - 4,12);
                }
                else{
                    r = QRect(worldToViewport(abscissa,position).x() + 4,worldToViewport(abscissa,position).y(),xMargin - 4,worldToViewportHeight(traceVspace + Yspace));
                    rHighlight = QRect(worldToViewport(abscissa,position).x() + 4,worldToViewport(abscissa,position).y(),xMargin - 4,12);
                }

                painter.fillRect(rHighlight,backgroundColor());
                painter.drawText(r,Qt::AlignHCenter | Qt::AlignTop,clusterIdentifier);
            }
        }
    }
}

void TraceView::drawTimeLine(int x,bool initialLine,bool eraseLine){

    QPainter painter;
    painter.begin(this);
    //set the window (part of the world I want to show)
    QRect r((QRect)window);

    painter.setWindow(r.left(),r.top(),r.width()-1,r.height()-1);//hack because Qt QRect is used differently in this function
    painter.setViewport(viewport);

    //KDAB_PENDING painter.setRasterOp(NotROP);
    painter.setPen(QPen(Qt::color0,1));
    painter.setBrush(Qt::NoBrush);
    int top = r.top();
    int bottom = r.bottom();
    int nbSamples = tracesProvider.getNbSamples(startTime,endTime,startTimeInRecordingUnits);
    int nbSamplesToDraw = static_cast<int>(floor(0.5 + static_cast<float>(nbSamples)/downSampling));
    int delta = x - lastClickAbscissa;
    QList<int> currentAbscissae;

    int nbColumns;
    int groupIndex = 0;
    QList<int> max;
    QList<int> min;

    if(multiColumns){
        nbColumns = shownGroupsChannels.count();
        //Compute the current abscissae in each group
        for(int i = 0; i<nbColumns;++i){
            currentAbscissae.append(linePositions[i] + delta);
        }

        //left margin is visible
        if(r.left() == 0){
            if(lastClickAbscissa <= (Xshift - XGroupSpace)) groupIndex = 0;
            else groupIndex = ((lastClickAbscissa - (Xshift - XGroupSpace)) / Xshift) + 1;
        }
        //left margin is invisible
        else{
            int shift = (nbSamplesToDraw - 1) * Xstep;
            if(lastClickAbscissa < shift + XGroupSpace) groupIndex = 0;
            else groupIndex = ((lastClickAbscissa - (shift + XGroupSpace)) / Xshift) + 1;
        }
        //Compute the min and max abscissae in each group
        for(int i = 0; i<nbColumns;++i){
            min.append(X0 + i * Xshift);
            max.append(X0 + i * Xshift + nbSamplesToDraw - 1);//points draw from 0 to (nbSamplesToDraw - 1)
        }
    }
    else{
        nbColumns = 1;
        currentAbscissae.append(linePositions[0] + delta);
        max.append(nbSamplesToDraw - 1);//points draw from 0 to (nbSamplesToDraw - 1)
        min.append(borderX);
    }

    //If the user went to far on the left, draw a line at the minimum min
    if(currentAbscissae[groupIndex] < min[groupIndex]){
        if(!initialLine){
            int previousDelta = previousDragAbscissa - lastClickAbscissa;
            QList<int> previousAbscissae;
            for(int i = 0; i<nbColumns;++i) previousAbscissae.append(linePositions[i] + previousDelta);
            if(previousAbscissae[groupIndex] >= min[groupIndex]){
                for(int i = 0; i<nbColumns;++i){
                    painter.drawLine(previousAbscissae[i],top,previousAbscissae[i],bottom); //erase the previous line
                    if(!eraseLine) painter.drawLine(min[i],top,min[i],bottom);//draw a line at min
                }
                if(x > min[groupIndex]) previousDragAbscissa = x;
                else{
                    previousDragAbscissa = min[groupIndex] - linePositions[groupIndex]  + lastClickAbscissa;
                }
            }
        }
        else{
            previousDragAbscissa = x;
        }
        painter.end();
    }
    //If the user went to far on the right, draw a line at the last sample position (max)
    else if(currentAbscissae[groupIndex] > max[groupIndex]){
        if(!initialLine){
            int previousDelta = previousDragAbscissa - lastClickAbscissa;
            QList<int> previousAbscissae;
            for(int i = 0; i<nbColumns;++i) previousAbscissae.append(linePositions[i] + previousDelta);
            if(previousAbscissae[groupIndex] < max[groupIndex]){
                for(int i = 0; i<nbColumns;++i){
                    painter.drawLine(previousAbscissae[i],top,previousAbscissae[i],bottom);//erase the previous line
                    if(!eraseLine) painter.drawLine(max[i],top,max[i],bottom);//draw a line at max
                }
                //compute previousDragAbscissa in order to have the line drawn at the far rigth of the trace
                if(x <= max[groupIndex]){
                    previousDragAbscissa = x;
                    if(linePositions[groupIndex] + (previousDragAbscissa - lastClickAbscissa) > max[groupIndex]) previousDragAbscissa = max[groupIndex] - linePositions[groupIndex]  + lastClickAbscissa;
                }
                else{
                    previousDragAbscissa = max[groupIndex] - linePositions[groupIndex]  + lastClickAbscissa;
                }
            }
        }
        else{
            previousDragAbscissa = x;
        }
        painter.end();
    }
    else{
        //erase the previous line
        if(!initialLine){
            int previousDelta = previousDragAbscissa - lastClickAbscissa;
            QList<int> previousAbscissae;
            for(int i = 0; i<nbColumns;++i) previousAbscissae.append(linePositions[i] + previousDelta);
            for(int i = 0; i<nbColumns;++i){
                if(previousAbscissae[i] >= min[i]) painter.drawLine(previousAbscissae[i],top,previousAbscissae[i],bottom);
                if(previousAbscissae[i] < min[i]) painter.drawLine(min[i],top,min[i],bottom);//the line has been drawn at min
            }
        }
        else{
            previousDragAbscissa = x;
        }
        //draw the new line
        if(!eraseLine) for(int i = 0; i<nbColumns;++i)  painter.drawLine(currentAbscissae[i],top,currentAbscissae[i],bottom);
        previousDragAbscissa = x;
        painter.end();
    }
}

void TraceView::mouseDoubleClickEvent(QMouseEvent* e){
    BaseFrame::mouseDoubleClickEvent(e);

    if(mode == ZOOM){
        doubleClick = true;
        zoomed = true;
        //update the window
        window = ZoomWindow(initialWindow);
    }
}

void TraceView::mouseMoveEvent(QMouseEvent* event){
    QString message;
    //Write the current coordinates in the statusbar.
    QRect r((QRect)window);
    QPoint current;
    //If the view was zoomed and the left margin (where the ids and gains of the channels of the first group are displayed) is not
    //shown (r.left() != 0), the coordinates have to be adjusted. Indeed, this margin is outside the world but in the viewport and included in the
    //values return par the event.
    if(r.left() != 0) current = viewportToWorld(event->x(),event->y());
    else current = viewportToWorld(event->x() - xMargin,event->y());

    int x = (current.x() - static_cast<int>(borderX));

    //Compute and write the duration and the voltage in the statusbar.
    if(mode == MEASURE && (event->buttons() == Qt::LeftButton)){
        //Compute the duration between the begining of the drag and now
        int duration = abs(x - startingIndex);
        float time = static_cast<float>(duration) / static_cast<float>(Xstep) * timeStep;

        int nbMinutes = static_cast<int>(time / 60000.0);
        float remainingSeconds = static_cast<float>(fmod(static_cast<double>(time),60000));
        int nbSeconds = static_cast<int>(remainingSeconds / 1000);
        int nbMiliseconds = static_cast<int>(fmod(static_cast<double>(remainingSeconds),1000) + 0.5);
        if(nbMiliseconds == 1000){
            nbMiliseconds = 0;
            nbSeconds++;
        }
        message = "Duration: "+ QString("%1 min %2 s %3 ms ").arg(nbMinutes).arg(nbSeconds).arg(nbMiliseconds);


        int delta = current.y() - lastClickOrdinate;
        float voltage = (delta/channelFactors[channelforVoltageComputation])/acquisitionGain;
        message.append(" Voltage: "+ QString("%1 mV").arg(fabs(voltage),0,'f',2));
    }
    else{
        //Compute the time
        if(x < 0) statusBar->clearMessage();//on the left side of the display.
        else{
            float relativeTime;
            if(multiColumns){
                //left margin is visible
                if(r.left() == 0)
                    relativeTime = static_cast<float>(fmod(x,static_cast<float>(Xshift)))
                            /  static_cast<float>(Xstep) * timeStep;
                //left margin is invisible
                else{
                    int nbSamples = tracesProvider.getNbSamples(startTime,endTime,startTimeInRecordingUnits);
                    int nbSamplesToDraw = static_cast<int>(floor(0.5 + static_cast<float>(nbSamples)/downSampling));
                    int shift = (nbSamplesToDraw - 1) * Xstep;
                    if(x < shift)
                        relativeTime = static_cast<float>(x) / static_cast<float>(Xstep) * timeStep;
                    else{
                        relativeTime = static_cast<float>(fmod((x),static_cast<float>(shift + XGroupSpace)))
                                /  static_cast<float>(Xstep) * timeStep;
                    }
                }
            }
            //single column
            else relativeTime = static_cast<float>(x) / static_cast<float>(Xstep) * timeStep;

            //between columns
            if(relativeTime > timeFrameWidth){
                message.clear();
            }
            else{
                float time;
                //If the data have been browsed for spikes, startTimeInRecordingUnits !=0 and the rounding applied to startTime make the computation
                // of time wrong. The correct start time has to be recompute using startTimeInRecordingUnits.
                if(startTimeInRecordingUnits !=0){
                    double samplingRate = tracesProvider.getSamplingRate();
                    double computeTime = static_cast<double>(static_cast<double>(startTimeInRecordingUnits) * static_cast<double>(1000) / static_cast<double>(samplingRate));
                    time =  relativeTime + computeTime;
                }
                //else time =  relativeTime + startTime;
                else time =  relativeTime + static_cast<float>(startTime);

                int nbMinutes = static_cast<int>(time / static_cast<float>(60000));
                float remainingSeconds = static_cast<float>(fmod(static_cast<double>(time),60000));
                int nbSeconds = static_cast<int>(remainingSeconds / 1000);
                int nbMiliseconds = static_cast<int>(fmod(static_cast<double>(remainingSeconds),1000) + 0.5);
                if(nbMiliseconds == 1000){
                    nbMiliseconds = 0;
                    nbSeconds++;
                }
                double totalNbSeconds = static_cast<double>(time /1000.000);
                message = "Time : "+ QString("%1 min %2 s %3 ms (%4 s)").arg(nbMinutes).arg(nbSeconds).arg(nbMiliseconds).arg(totalNbSeconds,0,'f',3);
            }
        }
    }

    statusBar->showMessage(message);

    //Paint the channels selected while dragging
    if(mode == SELECT && selectedChannels.size() != 0 && (event->buttons() == Qt::LeftButton)){
        QPainter painter;
        painter.begin(this);
        //set the window (part of the world I want to show)
        QRect r((QRect)window);

        painter.setWindow(r.left(),r.top(),r.width()-1,r.height()-1);//hack because Qt QRect is used differently in this function
        painter.setViewport(viewport);

        //KDAB_PENDING painter.setRasterOp(NotROP);
        painter.setPen(QPen(Qt::color0,1));
        painter.setBrush(Qt::NoBrush);

        int nbSamples = tracesProvider.getNbSamples(startTime,endTime,startTimeInRecordingUnits);
        int nbSamplesToDraw = static_cast<int>(floor(0.5 + static_cast<float>(nbSamples)/downSampling));
        int limit = viewportToWorldHeight(1);

        QList<int>::iterator channelIterator;
        for(channelIterator = selectedChannels.begin(); channelIterator != selectedChannels.end(); ++channelIterator){
            //if the channel is skipped, do no draw it
            if(skippedChannels.contains(*channelIterator )) continue;

            int initialBasePosition = channelsStartingOrdinate[*channelIterator] +  static_cast<long>(data(1,*channelIterator + 1) * channelFactors[*channelIterator]);

            //erase the previous trace
            if(previousDragOrdinate != 0){
                int X = channelsStartingAbscissa[*channelIterator];
                int previousDelta = previousDragOrdinate - lastClickOrdinate;
                int basePosition = initialBasePosition + previousDelta;

                if(downSampling != 1){
                    drawTrace(painter,limit,basePosition,X,*channelIterator,nbSamplesToDraw,true);
                }
                else{
                    QPolygon trace(nbSamples);
                    for(int i = 0; i < nbSamples;++i){
                        int y = basePosition - static_cast<long>(data(i + 1,*channelIterator + 1) * channelFactors[*channelIterator]);
                        trace.setPoint(i,X,y);
                        X += Xstep;
                    }
                    painter.drawPolyline(trace);
                }
            }

            //draw the new trace
            int X = channelsStartingAbscissa[*channelIterator];
            int delta = current.y() - lastClickOrdinate;

            int basePosition = initialBasePosition + delta;
            if(downSampling != 1){
                drawTrace(painter,limit,basePosition,X,*channelIterator,nbSamplesToDraw,true);
            }
            else{
                QPolygon trace(nbSamples);
                for(int i = 0; i < nbSamples;++i){
                    int y = basePosition - static_cast<long>(data(i + 1,*channelIterator + 1) * channelFactors[*channelIterator]);
                    trace.setPoint(i,X,y);
                    X += Xstep;
                }
                painter.drawPolyline(trace);
            }
        }

        painter.end();
        previousDragOrdinate = current.y();
    }

    //Paint the event selected while dragging
    if((mode == SELECT_EVENT && selectedEvent.first != "") && (event->buttons() == Qt::LeftButton)){
        QPainter painter;
        painter.begin(this);
        //set the window (part of the world I want to show)
        QRect r((QRect)window);

        painter.setWindow(r.left(),r.top(),r.width()-1,r.height()-1);//hack because Qt QRect is used differently in this function
        painter.setViewport(viewport);

        //KDAB_PENDING painter.setRasterOp(NotROP);
        painter.setPen(QPen(Qt::color0,1));
        painter.setBrush(Qt::NoBrush);
        int top = r.top();
        int bottom = r.bottom();
        int nbSamples = tracesProvider.getNbSamples(startTime,endTime,startTimeInRecordingUnits);
        int nbSamplesToDraw = static_cast<int>(floor(0.5 + static_cast<float>(nbSamples)/downSampling));
        int delta = x - lastClickAbscissa;
        int currentAbscissa = selectedEventPosition[1] + delta;

        int groupIndex = 0;
        int max = nbSamplesToDraw - 1;//points draw from 0 to (nbSamplesToDraw - 1)
        int min = borderX;
        if(multiColumns){
            //left margin is visible
            if(r.left() == 0){
                if(lastClickAbscissa <= (Xshift - XGroupSpace)) groupIndex = 0;
                else groupIndex = ((lastClickAbscissa - (Xshift - XGroupSpace)) / Xshift) + 1;
            }
            //left margin is invisible
            else{
                int shift = (nbSamplesToDraw - 1) * Xstep;
                if(lastClickAbscissa < shift + XGroupSpace) groupIndex = 0;
                else groupIndex = ((lastClickAbscissa - (shift + XGroupSpace)) / Xshift) + 1;
            }
            min = X0 + groupIndex * Xshift;
            max = X0 + groupIndex * Xshift + nbSamplesToDraw - 1;//points draw from 0 to (nbSamplesToDraw - 1)
        }

        //If the user went to far on the left, draw a line at the minimum min
        if(currentAbscissa < min){
            if(!startEventDragging){
                int previousDelta = previousDragAbscissa - lastClickAbscissa;
                int previousAbscissa = selectedEventPosition[1] + previousDelta;
                if(previousAbscissa > min){
                    painter.drawLine(previousAbscissa,top,previousAbscissa,bottom);//erase the previous line
                    painter.drawLine(min,top,min,bottom);//draw a line at min
                    if(x > min) previousDragAbscissa = x;
                    else{
                        previousDragAbscissa = min - selectedEventPosition[1] + lastClickAbscissa;
                    }
                }
            }
            else{
                startEventDragging = false;
                previousDragAbscissa = x;
            }
            painter.end();
        }
        //If the user went to far on the right, draw a line at the last sample position (max)
        else if(currentAbscissa > max){
            if(!startEventDragging){
                int previousDelta = previousDragAbscissa - lastClickAbscissa;
                int previousAbscissa = selectedEventPosition[1] + previousDelta;
                if(previousAbscissa < max){
                    painter.drawLine(previousAbscissa,top,previousAbscissa,bottom);//erase the previous line
                    painter.drawLine(max,top,max,bottom);//draw a line at max
                    //compute previousDragAbscissa in order to have the line drawn at the far rigth of the trace
                    if(x <= max){
                        previousDragAbscissa = x;
                        if(selectedEventPosition[1] + (previousDragAbscissa - lastClickAbscissa) > max) previousDragAbscissa = max - selectedEventPosition[1] + lastClickAbscissa;
                    }
                    else{
                        previousDragAbscissa = max - selectedEventPosition[1] + lastClickAbscissa;
                    }
                }
            }
            else{
                startEventDragging = false;
                previousDragAbscissa = x;
            }
            painter.end();
        }
        else{
            //erase the previous line
            if(!startEventDragging){
                int previousDelta = previousDragAbscissa - lastClickAbscissa;
                int previousAbscissa = selectedEventPosition[1] + previousDelta;
                if(previousAbscissa >= min) painter.drawLine(previousAbscissa,top,previousAbscissa,bottom);
                if(previousAbscissa < min) painter.drawLine(min,top,min,bottom);//the line has been drawn at min
            }
            else{
                previousDragAbscissa = x;
                startEventDragging = false;
            }
            //draw the new line
            painter.drawLine(currentAbscissa,top,currentAbscissa,bottom);
            previousDragAbscissa = x;
            painter.end();
        }
    }

    //Paint the line while dragging
    if(mode == DRAW_LINE && (event->buttons() == Qt::LeftButton)){
        if(!linePositions.isEmpty())
            drawTimeLine(x,false);
    }

    //The parent implementation takes care of the rubber band
    BaseFrame::mouseMoveEvent(event);
}

void TraceView::mousePressEvent(QMouseEvent* event){
    if(event->button() == Qt::LeftButton){

        if(mode == ZOOM || mode == MEASURE || mode == SELECT_TIME){
            //The parent implementation takes care of the zoom.
            BaseFrame::mousePressEvent(event);
        }
        QList<int> currentlySelectedChannels;
        QList<int> deselectedChannels;

        QPair<QString,int> deselectedEvent(selectedEvent.first,selectedEvent.second);
        int deselectedEventIndex = 0;
        if(!selectedEventPosition.isEmpty()) deselectedEventIndex = selectedEventPosition[0];

        if(mode == SELECT && !shownChannels.isEmpty() || mode == MEASURE || mode == SELECT_TIME || mode == SELECT_EVENT || mode == ADD_EVENT || mode == DRAW_LINE){
            QRect r((QRect)window);
            QPoint current;
            //If the view was zoomed and the left margin (where the ids and gains of the channels of the first group are displayed) is not
            //shown (r.left() != 0), the coordinates have to be adjusted. Indeed, this margin is outside the world but in the viewport and included in the
            //values return par the event.
            if(r.left() != 0) current = viewportToWorld(event->x(),event->y());
            else current = viewportToWorld(event->x() - xMargin,event->y());
            lastClickOrdinate = current.y();

            if(multiColumns){
                //determine the group
                int groupIndex;
                int sampleIndex;
                bool labelSelected = false;
                int x = (current.x() - static_cast<int>(borderX));
                //on the left side of the display.
                if(x <= 0){
                    sampleIndex = 1;
                    groupIndex = 0;
                    labelSelected = true;
                }
                //left margin is visible
                else if(r.left() == 0){
                    if(x <= (Xshift - XGroupSpace)){
                        groupIndex = 0;
                        sampleIndex = static_cast<int>(floor(0.5 +static_cast<float>(x) / static_cast<float>(Xstep)) * downSampling);
                    }
                    else{
                        groupIndex = ((x - (Xshift - XGroupSpace)) / Xshift) + 1;
                        int samplePart = x - (groupIndex * Xshift);
                        if(samplePart <= 0){
                            sampleIndex = 1;
                            labelSelected = true;
                        }
                        else sampleIndex = static_cast<int>(floor(0.5 +static_cast<float>(samplePart) / static_cast<float>(Xstep)) * downSampling);
                    }
                }
                //left margin is invisible
                else{
                    int nbSamples = tracesProvider.getNbSamples(startTime,endTime,startTimeInRecordingUnits);
                    int nbSamplesToDraw = static_cast<int>(floor(0.5 + static_cast<float>(nbSamples)/downSampling));
                    int shift = (nbSamplesToDraw - 1) * Xstep;
                    //Click on the first column

                    //The limit between 2 traces is half the distance between them except when the legend (text containing the id and the gain)
                    // is larger. In that case an adjustment is computed to give the ability to the user to click on the legend to select the trace.
                    int overlap = viewportToWorldWidth(xMargin) - (XGroupSpace / 2);

                    if(overlap < 0) overlap = 0;
                    if(x <= (shift + (XGroupSpace / 2) - overlap)){
                        groupIndex = 0;
                        if(x >= shift) sampleIndex = nbSamples;
                        else sampleIndex = static_cast<int>(floor(0.5 +static_cast<float>(x) / static_cast<float>(Xstep)) * downSampling);
                    }
                    else{
                        groupIndex = ((x - (shift + (XGroupSpace / 2) - overlap)) / Xshift) + 1;
                        int samplePart = x - (groupIndex * Xshift);
                        if(samplePart <= 0){
                            sampleIndex = 1;
                            labelSelected = true;
                        }
                        else sampleIndex = static_cast<int>(floor(0.5 +static_cast<float>(samplePart) / static_cast<float>(Xstep)) * downSampling);
                        if(sampleIndex > nbSamples) sampleIndex = nbSamplesToDraw;
                    }
                }

                if(mode == DRAW_LINE && x >= (X0 + groupIndex * Xshift)){
                    linePositions.clear();
                    previousDragAbscissa = 0;
                    lastClickAbscissa = x;
                    int nbGroups = shownGroupsChannels.count();
                    for(int i = 0; i<nbGroups;++i){
                        linePositions.append(X0 + i * Xshift + static_cast<int>(0.5 + (static_cast<float>(sampleIndex) / downSampling)));
                    }
                }
                else if(mode == ADD_EVENT && x >= (X0 + groupIndex * Xshift)){
                    newEventPosition = sampleIndex;
                }
                else if(mode == SELECT_EVENT && x >= (X0 + groupIndex * Xshift)){
                    lastClickAbscissa = x;
                    int difference = tracesProvider.getNbSamples(startTime,endTime,startTimeInRecordingUnits); //nbSamples as a starting point
                    QMap<QString, QList<int> >::Iterator iterator;
                    for(iterator = selectedEvents.begin(); iterator != selectedEvents.end(); ++iterator){
                        QList<int> eventList = iterator.data();
                        QString providerName = iterator.key();

                        if(eventList.size() == 0 || eventsData[providerName] == 0) continue;
                        Array<dataType>& currentData = static_cast<EventData*>(eventsData[providerName])->getTimes();
                        Array<int>& currentIds = static_cast<EventData*>(eventsData[providerName])->getIds();
                        int nbEvents = currentData.nbOfColumns();
                        for(int i = 1; i <= nbEvents;++i){
                            dataType index = currentData(1,i);
                            int eventId = currentIds(1,i);
                            if(eventList.contains(eventId) && abs(index - sampleIndex) <= difference){
                                difference = abs(index - sampleIndex);
                                selectedEvent.first = providerName;
                                selectedEvent.second = eventId;
                                selectedEventPosition.clear();
                                selectedEventPosition.append(index);
                                //The abscissa takes into account the abscissa of the current group
                                selectedEventPosition.append(X0 + groupIndex * Xshift + static_cast<int>(0.5 + (static_cast<float>(index) / downSampling)));
                            }
                        }
                    }
                    previousDragAbscissa = 0;
                    startEventDragging = true;
                }//end mode == SELECT_EVENT && x >= (X0 + groupIndex * Xshift)
                else{
                    QList<int> groupIds = shownGroupsChannels.keys();
                    int groupId = groupIds[static_cast<int>(groupIndex)];
                    QList<int> channelIds = shownGroupsChannels[groupId];
                    int currentNbChannels = channelIds.size();
                    int y = Y0;
                    int channelId = channelIds[0];
                    int channelIndex = 1;
                    //look up for the first channel which is not skipped
                    if(skippedChannels.contains(channelId)){
                        for(int i = 1; i < currentNbChannels; ++i){
                            if(!skippedChannels.contains(i)){
                                channelId = channelIds[i];
                                channelIndex = i + 1;
                                break;
                            }
                            y -= Yshift;
                        }
                    }

                    int position = -y + channelOffsets[channelId] - static_cast<long>(data(sampleIndex,channelId + 1) * channelFactors[channelId]);
                    int difference = abs(current.y() - position);
                    int selectedChannel = channelId;
                    y -= Yshift;

                    for(int i = channelIndex; i < currentNbChannels; ++i){
                        channelId = channelIds[i];
                        position = -y + channelOffsets[channelId] - static_cast<long>(data(sampleIndex,channelId + 1) * channelFactors[channelId]);

                        if(abs(current.y() - position) < difference && !skippedChannels.contains(channelId)){
                            difference = abs(current.y() - position);
                            selectedChannel = channelId;
                        }
                        y -= Yshift;
                    }

                    if(mode == SELECT){
                        //If there is not modificator key and selectedChannels does not already contain the selectedChannel
                        //deselect all the channels (clear selectedChannels) otherwise remove selectedChannel from the list.
                        //if the channel is skipped,there is a special treatment: deselect all the channels (clear selectedChannels)
                        if(!(event->state() & Qt::ShiftModifier) && !(event->state() & Qt::ControlModifier)){
                            //if the channel is skipped, deselect all the channels (clear selectedChannels)
                            if(!selectedChannels.contains(selectedChannel) || skippedChannels.contains(selectedChannel)){
                                alreadySelected = false;
                                QList<int>::iterator it;
                                for(it = selectedChannels.begin();it != selectedChannels.end();++it) deselectedChannels.append(*it);
                                selectedChannels.clear();
                            }
                            else{
                                selectedChannels.remove(selectedChannel);
                                if(selectedChannels.size() != 0) alreadySelected = true;
                            }
                        }

                        //Check the modificator keys
                        if(event->state() & Qt::ControlModifier){
                            //if the channel is skipped, do not do anything
                            if(!skippedChannels.contains(selectedChannel)){
                                if(selectedChannels.contains(selectedChannel)){
                                    selectedChannels.remove(selectedChannel);
                                    deselectedChannels.append(selectedChannel);
                                }
                                else{
                                    selectedChannels.append(selectedChannel);
                                    currentlySelectedChannels.append(selectedChannel);
                                }
                            }
                        }
                        else if((event->state() & Qt::ShiftModifier) && selectedChannels.size() != 0){
                            //take all the channels, not skipped of groupId with a label ordinate in the range defined by the label ordinate of the last
                            //selected channel and the one of the currently selected channel.
                            if(labelSelected){
                                int previousChannelId = selectedChannels[selectedChannels.size() - 1];
                                //If the prevously selected channel is not in the same group, only select the currently selected.
                                //No cross group selection is done.
                                if(!channelIds.contains(previousChannelId)){
                                    if(!skippedChannels.contains(selectedChannel)){
                                        selectedChannels.append(selectedChannel);
                                        currentlySelectedChannels.append(selectedChannel);
                                    }
                                }
                                else{
                                    int previousOrdinate = channelsStartingOrdinate[previousChannelId];
                                    int currentOrdinate = channelsStartingOrdinate[selectedChannel];
                                    int min = previousOrdinate;
                                    int max = currentOrdinate;
                                    if(currentOrdinate < previousOrdinate){
                                        min = currentOrdinate;
                                        max = previousOrdinate;
                                    }

                                    for(int i = 0; i < currentNbChannels; ++i){
                                        channelId = channelIds[i];
                                        int ordinate = channelsStartingOrdinate[channelId];
                                        if(ordinate>= min && ordinate <= max && !selectedChannels.contains(channelId)){
                                            if(!skippedChannels.contains(selectedChannel)){
                                                selectedChannels.append(channelId);
                                                currentlySelectedChannels.append(channelId);
                                            }
                                        }
                                    }
                                }
                            }
                            else{
                                //TO DO
                            }
                        }
                        else{
                            if(!skippedChannels.contains(selectedChannel)){
                                selectedChannels.append(selectedChannel);
                                currentlySelectedChannels.append(selectedChannel);
                            }
                        }
                        emit channelsSelected(selectedChannels);
                    }//end of mode == SELECT
                    //mode == MEASURE
                    else if(mode == MEASURE){
                        channelforVoltageComputation = selectedChannel;
                        startingIndex = x;
                    }
                    //mode == MEASURE
                    else startingIndex = x;
                }//!(mode == SELECT_EVENT && x >= (X0 + groupIndex * Xshift))
            }//end multicolumns
            else{//single column
                int x = (current.x() - static_cast<int>(borderX));
                int sampleIndex;
                //on the left side of the display.
                if(x <= 0) sampleIndex = 1;
                //take the last sample of the downSampling one at the same abscissa
                else sampleIndex = static_cast<int>(floor(0.5 +static_cast<float>(x) / static_cast<float>(Xstep)) * downSampling);

                if(mode == DRAW_LINE && x >= 0){
                    linePositions.clear();
                    previousDragAbscissa = 0;
                    lastClickAbscissa = x;
                    linePositions.append(static_cast<int>(0.5 + (static_cast<float>(sampleIndex) / downSampling)));
                }
                else if(mode == ADD_EVENT && x >= 0){
                    newEventPosition = sampleIndex;
                }
                else if(mode == SELECT_EVENT && x >= 0){
                    lastClickAbscissa = x;
                    int difference = tracesProvider.getNbSamples(startTime,endTime,startTimeInRecordingUnits); //nbSamples as a starting point
                    QMap<QString, QList<int> >::Iterator iterator;
                    for(iterator = selectedEvents.begin(); iterator != selectedEvents.end(); ++iterator){
                        QList<int> eventList = iterator.data();
                        QString providerName = iterator.key();

                        if(eventList.size() == 0 || eventsData[providerName] == 0) continue;
                        Array<dataType>& currentData = static_cast<EventData*>(eventsData[providerName])->getTimes();
                        Array<int>& currentIds = static_cast<EventData*>(eventsData[providerName])->getIds();
                        int nbEvents = currentData.nbOfColumns();
                        for(int i = 1; i <= nbEvents;++i){
                            dataType index = currentData(1,i);
                            int eventId = currentIds(1,i);
                            if(eventList.contains(eventId) && abs(index - sampleIndex) <= difference){
                                difference = abs(index - sampleIndex);
                                selectedEvent.first = providerName;
                                selectedEvent.second = eventId;
                                selectedEventPosition.clear();
                                selectedEventPosition.append(index);
                                selectedEventPosition.append(static_cast<int>(0.5 + (static_cast<float>(index) / downSampling)));
                            }
                        }
                    }
                    previousDragAbscissa = 0;
                    startEventDragging = true;
                }
                else{
                    QList<int> groupIds = shownGroupsChannels.keys();

                    QList<int> firstGroup = shownGroupsChannels[groupIds[0]];
                    int y = Y0;
                    int channelId = firstGroup[0];
                    int channelIndex = 1;
                    int startingGroupIndex = 0;
                    //look up for the first channel which is not skipped.
                    if(skippedChannels.contains(channelId)){
                        for(uint j = 0; j<groupIds.size();++j){
                            QList<int> channelIds = shownGroupsChannels[groupIds[j]];
                            int currentNbChannels = channelIds.size();
                            for(int i = 0; i < currentNbChannels; ++i){
                                if(!skippedChannels.contains(channelIds[i])){
                                    channelId = channelIds[i];
                                    channelIndex = i + 1;
                                    startingGroupIndex = j;
                                    break;
                                }
                                y -= Yshift;
                            }
                            if(!skippedChannels.contains(channelId))  break;
                        }
                    }

                    int position = -y + channelOffsets[channelId] - static_cast<long>(data(sampleIndex,channelId + 1) * channelFactors[channelId]);
                    int difference = abs(current.y() - position);
                    int selectedChannel = channelId;
                    y -= Yshift;

                    for(int j = startingGroupIndex; j<groupIds.size();++j){
                        QList<int> channelIds = shownGroupsChannels[groupIds[j]];
                        int currentNbChannels = channelIds.size();
                        int i = 0;
                        if(j == startingGroupIndex) i = channelIndex;
                        for(; i < currentNbChannels; ++i){
                            channelId = channelIds[i];
                            position = -y + channelOffsets[channelId] - static_cast<long>(data(sampleIndex,channelId + 1) * channelFactors[channelId]);

                            if(abs(current.y() - position) < difference && !skippedChannels.contains(channelId)){
                                difference = abs(current.y() - position);
                                selectedChannel = channelId;
                            }
                            y -= Yshift;
                        }
                        y -= YGroupSpace;
                    }

                    if(mode == SELECT){
                        //If there is not modificator key and selectedChannels does not already contain the selectedChannel
                        //deselect all the channels (clear selectedChannels) otherwise remove selectedChannel from the list.
                        //if the channel is skipped,there is a special treatment: deselect all the channels (clear selectedChannels)
                        if(!(event->state() & Qt::ShiftModifier) && !(event->state() & Qt::ControlModifier)){
                            //if the channel is skipped, deselect all the channels (clear selectedChannels)
                            if(!selectedChannels.contains(selectedChannel) || skippedChannels.contains(selectedChannel)){
                                alreadySelected = false;
                                QList<int>::iterator it;
                                for(it = selectedChannels.begin();it != selectedChannels.end();++it) deselectedChannels.append(*it);
                                selectedChannels.clear();
                            }
                            else{
                                selectedChannels.remove(selectedChannel);
                                if(selectedChannels.size() != 0) alreadySelected = true;
                            }
                        }

                        if(event->state() & Qt::ControlModifier){
                            //if the channel is skipped, do not do anything
                            if(!skippedChannels.contains(selectedChannel)){
                                if(selectedChannels.contains(selectedChannel)){
                                    selectedChannels.remove(selectedChannel);
                                    deselectedChannels.append(selectedChannel);
                                }
                                else{
                                    selectedChannels.append(selectedChannel);
                                    currentlySelectedChannels.append(selectedChannel);
                                }
                            }
                        }
                        else if((event->state() & Qt::ShiftModifier) && selectedChannels.size() != 0){
                            //take all the channels of groupId with a label ordinate in the range defined by the label ordinate of the last
                            //selected channel and the one of the currently selected channel.
                            if(x < 0){
                                int previousChannelId = selectedChannels[selectedChannels.size() - 1];
                                int previousOrdinate = channelsStartingOrdinate[previousChannelId];
                                int currentOrdinate = channelsStartingOrdinate[selectedChannel];
                                int min = previousOrdinate;
                                int max = currentOrdinate;
                                if(currentOrdinate < previousOrdinate){
                                    min = currentOrdinate;
                                    max = previousOrdinate;
                                }
                                int currentNbChannels = shownChannels.size();
                                for(int i = 0; i < currentNbChannels; ++i){
                                    channelId = shownChannels[i];
                                    int ordinate = channelsStartingOrdinate[channelId];
                                    if(ordinate>= min && ordinate <= max && !selectedChannels.contains(channelId)){
                                        if(!skippedChannels.contains(selectedChannel)){
                                            selectedChannels.append(channelId);
                                            currentlySelectedChannels.append(channelId);
                                        }
                                    }
                                }
                            }
                            else{
                                //TO DO
                            }
                        }
                        else{
                            if(!skippedChannels.contains(selectedChannel)){
                                selectedChannels.append(selectedChannel);
                                currentlySelectedChannels.append(selectedChannel);
                            }
                        }
                        emit channelsSelected(selectedChannels);
                    }//end of mode == SELECT
                    //mode == MEASURE
                    else if(mode == MEASURE){
                        channelforVoltageComputation = selectedChannel;
                        startingIndex = x;
                    }
                    //mode == SELECT_TIME
                    else
                        startingIndex = x;
                }//mode != SELECT_EVENT
            }//single column
            if(mode == SELECT){
                mDeselectedChannels.clear();
                mSelectedChannels.clear();
                mSelectedChannels = currentlySelectedChannels;
                mDeselectedChannels = deselectedChannels;
                update();
            }
            else if(mode == SELECT_EVENT){
                if(!deselectedEvent.first.isEmpty())
                    drawEvent(deselectedEvent.first,deselectedEvent.second,deselectedEventIndex,false);
                if(!selectedEvent.first.isEmpty())
                    drawEvent(selectedEvent.first,selectedEvent.second,selectedEventPosition[0],true);
            }
            else if(mode == DRAW_LINE){
                if(!linePositions.isEmpty())
                    drawTimeLine(lastClickAbscissa,true);
            }
            previousDragOrdinate = 0;
        }//mode == SELECT && shownChannels.size() != 0 || mode == MEASURE || mode == SELECT_TIME || mode == SELECT_EVENT || mode == ADD_EVENT || mode == DRAW_LINE
    }//Qt::LeftButton
}


void TraceView::mouseReleaseEvent(QMouseEvent* event){
    if(mode == SELECT){
        if(event->button() & Qt::LeftButton && !(event->state() & Qt::ShiftModifier) && !(event->state() & Qt::ControlModifier)){
            //There was a drag of channels
            if(previousDragOrdinate != 0){
                int delta = previousDragOrdinate - lastClickOrdinate;
                QList<int>::iterator channelIterator;
                for(channelIterator = selectedChannels.begin(); channelIterator != selectedChannels.end(); ++channelIterator)
                    channelOffsets[*channelIterator] += delta;

                previousDragOrdinate = 0;
                drawContentsMode = REDRAW;
                //Redraw
                update();
            }
            else{
                //deselect all the channels except the last one.
                if(alreadySelected){
                    int channelId = selectedChannels[selectedChannels.size() - 1];
                    selectedChannels.remove(channelId);

                    mDeselectedChannels.clear();
                    QList<int> deselectedChannels;
                    QList<int>::iterator it;
                    for(it = selectedChannels.begin();it != selectedChannels.end();++it)
                        mDeselectedChannels.append(*it);

                    selectedChannels.clear();
                    update();
                    selectedChannels.append(channelId);
                    emit channelsSelected(selectedChannels);
                }
            }
        }
    }

    if(mode == SELECT_EVENT && (event->button() & Qt::LeftButton)){
        //There was a drag of an event
        if(!startEventDragging){
            int delta = previousDragAbscissa - lastClickAbscissa;

            Array<dataType>& currentData = static_cast<EventData*>(eventsData[selectedEvent.first])->getTimes();
            Array<int>& currentIds = static_cast<EventData*>(eventsData[selectedEvent.first])->getIds();
            int nbEvents = currentData.nbOfColumns();
            for(int i = 1; i <= nbEvents;++i){
                dataType index = currentData(1,i);
                int eventId = currentIds(1,i);
                if(eventId == selectedEvent.second && index == selectedEventPosition[0]){
                    currentData(1,i) += static_cast<dataType>(delta * downSampling);
                    if(currentData(1,i) < 0) currentData(1,i) = 0;
                    double samplingRate = static_cast<double>(tracesProvider.getSamplingRate()) / 1000.0;//in eventProvider the time is in miliseconds
                    double previousTime = (static_cast<double>(index) / samplingRate) + static_cast<double>(startTime);
                    double newTime = (static_cast<double>(currentData(1,i)) / samplingRate) + static_cast<double>(startTime);

                    //Notify the provider of the modification
                    static_cast<EventsProvider*>(eventProviders[selectedEvent.first])->modifiedEvent(eventId,previousTime,newTime);
                    //notify everybody of the modification
                    emit eventModified(selectedEvent.first,eventId,previousTime,newTime);
                    break;
                }
            }
            previousDragAbscissa = 0;
            selectedEvent.first.clear();
            selectedEvent.second = 0;
            startEventDragging = true;
            drawContentsMode = REDRAW;
            //Redraw
            update();
        }
    }
    if(mode == ADD_EVENT && (event->button() & Qt::LeftButton)){
        if(eventDescriptionToCreate.isEmpty()){
            QMessageBox::critical (this, tr("Unselected description type!"),tr("In order to add an event you have to choose an event description first!"));
            return;
        }

        //The location for the new event has been selected
        if(newEventPosition != -1){
            double samplingRate = static_cast<double>(tracesProvider.getSamplingRate()) / 1000.0;//in eventProvider the time is in miliseconds
            double time = (static_cast<double>(newEventPosition) / samplingRate) + static_cast<double>(startTime);

            eventBeingModified = true;
            //Notify the provider of the creation
            static_cast<EventsProvider*>(eventProviders[eventProvider])->addEvent(eventDescriptionToCreate,time);

            eventBeingModified = false;
            //notify everybody of the modification
            emit eventAdded(eventProvider,eventDescriptionToCreate,time);
        }
    }

    if(mode == ZOOM && (event->button() & Qt::LeftButton)){
        //Zoom out
        if(event->state() & Qt::ShiftModifier){
            previousWindow = (QRect)window;
            zoomOut = true;
            zoomed = true;
            /*QRect r((QRect)window);
   QPoint click;
   if(r.left() != 0) click = viewportToWorld(e->x(),e->y());
   else click = viewportToWorld(e->x() - xMargin,e->y());
   if(click.x() < 0) click.setX(0);*/
        }
        else if(!doubleClick){
            if(!maxZoomReached) previousWindow = (QRect)window;
            zoomed = true;
        }

        //The parent implementation takes care of the rubber band
        BaseFrame::mouseReleaseEvent(event);

        computeChannelDisplayGain();
        drawContentsMode = REDRAW;
        update();
    }
    if(mode == MEASURE && (event->button() & Qt::LeftButton)){
        //The parent implementation takes care of the rubber band
        BaseFrame::mouseReleaseEvent(event);
    }
    if(mode == SELECT_TIME && (event->button() & Qt::LeftButton)){
        //The parent implementation takes care of the rubber band
        BaseFrame::mouseReleaseEvent(event);
        QPoint current;
        QRect r((QRect)window);
        if(r.left() != 0) current = viewportToWorld(event->x(),event->y());
        else current = viewportToWorld(event->x() - xMargin,event->y());
        int x = (current.x() - static_cast<int>(borderX));

        //Compute the starting and ending time
        float relativeStartingTime;
        float relativeEndingTime;
        int startingTime;
        int columnNb = 0;
        //starting time
        if(startingIndex < 0){
            columnNb = 1;
            relativeStartingTime = 0;
        }
        else{
            if(multiColumns){
                //left margin is visible
                if(r.left() == 0){
                    relativeStartingTime = static_cast<float>(fmod(startingIndex,static_cast<float>(Xshift)))
                            /  static_cast<float>(Xstep) * timeStep;
                    if(startingIndex < Xshift) columnNb = 1;
                    else columnNb = ((startingIndex - (Xshift)) / Xshift) + 2;
                }
                //left margin is invisible
                else{
                    int nbSamples = tracesProvider.getNbSamples(startTime,endTime,startTimeInRecordingUnits);
                    int nbSamplesToDraw = static_cast<int>(floor(0.5 + static_cast<float>(nbSamples)/downSampling));
                    int shift = (nbSamplesToDraw - 1) * Xstep;
                    if(startingIndex < shift){
                        relativeStartingTime = static_cast<float>(startingIndex) / static_cast<float>(Xstep) * timeStep;
                        columnNb = 1;
                    }
                    else{
                        relativeStartingTime = static_cast<float>(fmod((startingIndex),static_cast<float>(shift + XGroupSpace)))
                                /  static_cast<float>(Xstep) * timeStep;
                        columnNb = ((startingIndex - (Xshift - XGroupSpace)) / Xshift) + 2;
                    }
                }
            }
            //single column
            else{
                relativeStartingTime = static_cast<float>(startingIndex) / static_cast<float>(Xstep) * timeStep;
            }
            //between columns
            if(relativeStartingTime > timeFrameWidth) relativeStartingTime = 0;
        }
        startingTime = startTime + static_cast<int>(0.5 + relativeStartingTime);

        //ending time
        if(x < 0){
            startingTime = startTime;
            relativeEndingTime = relativeStartingTime;
            relativeStartingTime = 0;
        }
        else{
            if(multiColumns){
                //left margin is visible
                if(r.left() == 0){
                    relativeEndingTime = static_cast<float>(fmod(x,static_cast<float>(Xshift)))
                            /  static_cast<float>(Xstep) * timeStep;
                    int currentColumnNb;
                    if(x < Xshift) currentColumnNb = 1;
                    else currentColumnNb = ((x - (Xshift)) / Xshift) + 2;
                    if(currentColumnNb == columnNb && relativeEndingTime < relativeStartingTime){
                        startingTime = startTime + static_cast<int>(0.5 + relativeEndingTime);
                    }
                    if(currentColumnNb > columnNb && relativeStartingTime != 0) relativeEndingTime = timeFrameWidth;
                    if(currentColumnNb < columnNb){//the user went backwards
                        startingTime = startTime;
                        relativeEndingTime = relativeStartingTime;
                        relativeStartingTime = 0;
                    }
                }
                //left margin is invisible
                else{
                    int nbSamples = tracesProvider.getNbSamples(startTime,endTime,startTimeInRecordingUnits);
                    int nbSamplesToDraw = static_cast<int>(floor(0.5 + static_cast<float>(nbSamples)/downSampling));
                    int shift = (nbSamplesToDraw - 1) * Xstep;
                    if(x < shift){
                        relativeEndingTime = static_cast<float>(x) / static_cast<float>(Xstep) * timeStep;
                        if(columnNb != 1){//the user went backwards
                            startingTime = startTime;
                            relativeEndingTime = relativeStartingTime;
                            relativeStartingTime = 0;
                        }
                        if(columnNb == 1 && relativeEndingTime < relativeStartingTime){//the user went backwards on the first column
                            startingTime = startTime + static_cast<int>(0.5 + relativeEndingTime);
                        }
                    }
                    else{
                        relativeEndingTime = static_cast<float>(fmod((x),static_cast<float>(shift + XGroupSpace)))
                                /  static_cast<float>(Xstep) * timeStep;

                        int currentColumnNb = ((x - (Xshift - XGroupSpace)) / Xshift) + 2;
                        if(currentColumnNb == columnNb && relativeEndingTime < relativeStartingTime){
                            startingTime = startTime + static_cast<int>(0.5 + relativeEndingTime);
                        }
                        if(currentColumnNb > columnNb && relativeStartingTime != 0) relativeEndingTime = timeFrameWidth;
                        if(currentColumnNb < columnNb){//the user went backwards
                            startingTime = startTime;
                            relativeEndingTime = relativeStartingTime;
                            relativeStartingTime = 0;
                        }
                    }
                }
            }
            //single column
            else{
                relativeEndingTime = static_cast<float>(x) / static_cast<float>(Xstep) * timeStep;
                if(relativeEndingTime < relativeStartingTime){
                    startingTime = startTime + static_cast<int>(0.5 + relativeEndingTime);
                }
            }
            //between columns
            if(relativeEndingTime > timeFrameWidth) relativeEndingTime = timeFrameWidth;
        }

        //Compute the duration
        int duration = abs(static_cast<int>(0.5 + relativeEndingTime) - static_cast<int>(0.5 + relativeStartingTime));
        //The user drag over another column
        if(duration > timeFrameWidth) duration = timeFrameWidth;

        emit setStartAndDuration(startingTime,duration);
    }
    if(mode == DRAW_LINE && (event->button() & Qt::LeftButton)){
        //erase the line
        if(!linePositions.isEmpty())
            drawTimeLine(0,false,true);
        linePositions.clear();
        previousDragAbscissa = 0;
    }
}

void TraceView::selectChannels(const QList<int>& selectedIds){
    if((selectedChannels.size() == 0 && selectedIds.size() == 0)) return;

    //Unhighlight the currently selected traces which are not selected any more
    mDeselectedChannels.clear();
    QList<int>::iterator it;
    for(it = selectedChannels.begin();it != selectedChannels.end();++it)
        if(!selectedIds.contains(*it) && shownChannels.contains(*it))
            mDeselectedChannels.append(*it);


    //Highlight the newly selected traces
    mSelectedChannels.clear();
    QList<int>::const_iterator iterator;
    for(iterator = selectedIds.begin(); iterator != selectedIds.end(); ++iterator){
        if(!selectedChannels.contains(*iterator) && shownChannels.contains(*iterator)){
            //if the channel is skipped, do no draw it
            if(!skippedChannels.contains(*iterator))
                mSelectedChannels.append(*iterator);
        }
    }

    //Update the list of selected channels
    selectedChannels.clear();

    //the skipped channels are not selected
    for(iterator = selectedIds.begin(); iterator != selectedIds.end(); ++iterator){
        if(shownChannels.contains(*iterator) && !skippedChannels.contains(*iterator))
            selectedChannels.append(*iterator);
    }

    update();
}


void TraceView::reset(){
    dataReady = false;
    if(multiColumns) columnDisplayChanged = true;
    selectedChannels.clear();

    //Retreive the new information
    nbChannels = tracesProvider.getNbChannels();
    int samplingRate = tracesProvider.getSamplingRate();
    timeStepUnit = timeStep = static_cast<float>(static_cast<float>(1000) / static_cast<float>(samplingRate));

    //Update the groups to be shown
    shownChannels.clear();
    updateShownGroupsChannels(shownChannels);

    //Set the initial offset for each channel is set to zero.
    for(int i = 0; i < nbChannels; ++i) channelOffsets.append(0);

    //Set the initial amplitude and factor for each channel.
    setGains(unitGain,acquisitionGain);

    //Get the data.
    // tracesProvider.requestData(startTime,endTime,this,startTimeInRecordingUnits);
    setStartAndDuration(0,50);
}

void TraceView::drawCalibrationScale(QPainter& painter){
    QFont f("Helvetica",8);
    painter.setFont(f);
    painter.setPen(colorLegend); //set the color for the legends.

    //Calibration scale, is meaningful only if all the channels have the same amplification. Take the channel 0 as the provider
    int nbRU = static_cast<int>(unitGain * channelFactors[0]);
    float screenGain = static_cast<float>(static_cast<float>(unitGain)/static_cast<float>(acquisitionGain));
    float gain = channelDisplayGains[0];

    QFontInfo fontInfo = QFontInfo(f);
    int labelSize = fontInfo.pixelSize() * 12;

    //Draw the voltage calibration bar
    QPoint p1(viewport.right() - labelSize,viewport.bottom() - 30);
    QPoint p2(viewport.right() - labelSize,viewport.bottom() - 30 - worldToViewportHeight(nbRU));
    painter.drawLine(p1,p2);
    QPoint pTextVoltage(viewport.right() - fontInfo.pixelSize() * 11,viewport.bottom() - 30);
    painter.drawText(pTextVoltage,QString("%1 mV (x%2)").arg(screenGain,0,'f',1).arg(gain,0,'f',2));

    //draw the time calibration bar (a 20iest of the timeframe)
    long timeFrameWidth = endTime - startTime;
    long barLengthInMs = static_cast<long>(floor(0.5 + static_cast<float>(static_cast<float>(timeFrameWidth) / static_cast<float>(20))));

    float width;
    if(!multiColumns) width = static_cast<float>(viewport.width());
    else width = static_cast<float>(worldToViewportWidth(Xshift) - worldToViewportWidth(XGroupSpace));

    long barLengthInpx = static_cast<long>(floor(0.5 + (width / static_cast<float>(timeFrameWidth)) * static_cast<float>(barLengthInMs)));

    QPoint p3(viewport.right() - labelSize,viewport.bottom() - 30);
    QPoint p4(viewport.right() - labelSize - barLengthInpx,viewport.bottom() - 30);
    painter.drawLine(p3,p4);
    QPoint pTextTime(viewport.right() - labelSize - barLengthInpx + 8,viewport.bottom() - 15);
    painter.drawText(pTextTime,QString("%1 ms").arg(barLengthInMs));
}

void TraceView::correctZoom(QRect& r){

    if(zoomed && firstZoom && downSampling == 1){
        zoomed = false;
        firstZoom = false;
        initialWindow = previousWindow;
        initialDownSampling = downSampling;
        if(multiColumns){
            initialXshift = Xshift;
            initialXGroupSpace = XGroupSpace;
            initialTraceWidth = traceWidth;
        }
    }
    /* if(zoomed && !firstZoom && zoomOut){
     qDebug()<<" zoomed && !firstZoom && zoomOut r.width() "<<r.width();
    zoomOut = false;
    zoomed = false;
    if(zoomFactor != 1){
     if(multiColumns){
     }
     else{
      zoomed = false;
      qDebug()<<"zoomFactor "<<zoomFactor;
      int windowWidth = r.width();


      //float factor = 0.5;//static_cast<float>(previousWindow.width())/static_cast<float>(windowWidth);
      float previousDownSampling = downSampling;

      downSampling = 2 * downSampling;
      //float factor = static_cast<float>(previousWindow.width())/static_cast<float>(windowWidth);
      //float previousDownSampling = downSampling;
     // downSampling = downSampling/factor;



      if(downSampling < 1) downSampling = 1;
      int newWidth = static_cast<int>(2 * windowWidth * previousDownSampling/ downSampling) * Xstep;
      int newLeft = 0;//static_cast<int>(static_cast<float>(r.left()) * previousDownSampling / downSampling) * Xstep;
      timeStep = timeStepUnit * downSampling;

      //update the window
      r.setLeft(newLeft);
      r.setWidth(newWidth);
      window = ZoomWindow(r);
 qDebug()<<"previousWindow.width() "<<previousWindow.width()<<" windowWidth "<<windowWidth<<" previousDownSampling "<<previousDownSampling<<" zoomFactor "<<zoomFactor;
qDebug()<<" downSampling "<<downSampling<<" newWidth "<<newWidth<<" r.left() "<<r.left()<<" newLeft "<<newLeft<<" timeStep "<<timeStep;

     }
    }
   }*/
    else if(zoomed && downSampling == 1 && !doubleClick){
        if(maxZoomReached){
            int newLeft = r.left();
            r = QRect(previousWindow);
            r.setLeft(newLeft);
            r.setWidth(previousWindow.width());
            //update the window
            window = ZoomWindow(r);
            previousWindow = QRect(r);
        }
        else if(zoomFactor != 0){
            int windowWidth = r.width();
            zoomFactor *= static_cast<float>(initialWindow.width())/static_cast<float>(windowWidth);

            if(zoomFactor > 22.73){
                zoomFactor /= static_cast<float>(initialWindow.width())/static_cast<float>(r.width());
                float final = 22.73 / zoomFactor;
                windowWidth = static_cast<int>(static_cast<float>(initialWindow.width())/ final);
                zoomFactor = 22.73;
                maxZoomReached = true;

                //update the window
                r.setWidth(windowWidth);
                window = ZoomWindow(r);
                previousWindow = QRect(r);
            }
        }
    }
    else if(zoomed && downSampling != 1 && !doubleClick){
        zoomed = false;
        int windowWidth = r.width();

        if(firstZoom){
            firstZoom = false;
            initialWindow = previousWindow;
            initialDownSampling = downSampling;

            zoomFactor = static_cast<float>(initialWindow.width())/static_cast<float>(windowWidth);
            if(multiColumns){
                initialXshift = Xshift;
                initialXGroupSpace = XGroupSpace;
                initialTraceWidth = traceWidth;
            }
        }
        else if(!maxZoomReached){
            zoomFactor *= static_cast<float>(initialWindow.width())/static_cast<float>(windowWidth);
        }

        if(maxZoomReached){
            int newLeft = r.left();
            r = QRect(previousWindow);
            r.setLeft(newLeft);
            r.setWidth(previousWindow.width());
            //update the window
            window = ZoomWindow(r);
            previousWindow = QRect(r);
        }
        else{
            if(zoomFactor > 22.73){
                zoomFactor /= static_cast<float>(initialWindow.width())/static_cast<float>(r.width());
                float final = 22.73 / zoomFactor;
                windowWidth = static_cast<int>(static_cast<float>(initialWindow.width())/ final);
                zoomFactor = 22.73;
                maxZoomReached = true;

                //update the window
                r.setWidth(windowWidth);
                window = ZoomWindow(r);
                previousWindow = QRect(r);
                return;

            }

            if(multiColumns){
                float factor = static_cast<float>(previousWindow.width())/static_cast<float>(windowWidth);
                float previousDownSampling = downSampling;

                downSampling = downSampling/factor;
                if(downSampling < 1) downSampling = 1;

                int newWidth;
                int nbSamples = tracesProvider.getNbSamples(startTime,endTime,startTimeInRecordingUnits);
                int previousNbSamplesToDraw = static_cast<int>(floor(0.5 + static_cast<float>(nbSamples)/previousDownSampling));
                int previousXGroupSpace = XGroupSpace;
                XGroupSpace = static_cast<int>(static_cast<float>(XGroupSpace) * factor);

                //int shift = traceWidth * Xstep + previousXGroupSpace;
                int newLeft;

                if(r.left() == 0){
                    newLeft = 0;
                    //Compute the width of the window
                    if(windowWidth <= traceWidth) newWidth = static_cast<int>((windowWidth) * previousDownSampling/ downSampling) * Xstep;
                    else{
                        int nbGroupParts = static_cast<int>(windowWidth / Xshift);
                        int secondPart = windowWidth - (nbGroupParts * Xshift);
                        if(secondPart <= traceWidth){
                            newWidth = static_cast<int>((windowWidth - (nbGroupParts * previousXGroupSpace)) * previousDownSampling/ downSampling) * Xstep
                                    + (nbGroupParts * XGroupSpace);
                        }
                        else{
                            int marginPart = static_cast<int>(XGroupSpace * static_cast<float>(secondPart - traceWidth) / static_cast<float>(previousXGroupSpace));
                            newWidth = static_cast<int>((windowWidth - (nbGroupParts * previousXGroupSpace) - (secondPart - traceWidth)) * previousDownSampling/ downSampling) * Xstep
                                    + (nbGroupParts * XGroupSpace) + marginPart;
                        }
                    }
                }
                else{
                    int groupIndex;
                    if(r.left() <= Xshift){
                        groupIndex = 0;
                        if(r.left() > traceWidth){
                            int marginPart =  static_cast<int>(XGroupSpace * static_cast<float>(r.left() - traceWidth) / static_cast<float>(previousXGroupSpace));
                            newLeft = static_cast<int>(static_cast<float>(previousNbSamplesToDraw) * previousDownSampling / downSampling) * Xstep + marginPart;
                            //Compute the width of the window
                            int withoutFirstMarginPart = static_cast<int>(windowWidth - (previousXGroupSpace - (r.left() - traceWidth)));
                            if(withoutFirstMarginPart <= traceWidth)
                                newWidth = static_cast<int>(static_cast<float>(withoutFirstMarginPart) * previousDownSampling/ downSampling) * Xstep
                                        + (XGroupSpace - marginPart);
                            else{
                                int nbGroupParts = static_cast<int>(withoutFirstMarginPart / Xshift);
                                int secondPart = withoutFirstMarginPart - (nbGroupParts * Xshift);
                                if(secondPart <= traceWidth){
                                    newWidth = static_cast<int>(static_cast<float>(withoutFirstMarginPart - (nbGroupParts * previousXGroupSpace)) * previousDownSampling/ downSampling) * Xstep
                                            + (nbGroupParts * XGroupSpace) + (XGroupSpace - marginPart);
                                }
                                else{
                                    int secondMarginPart = static_cast<int>(XGroupSpace * static_cast<float>(secondPart - traceWidth) / static_cast<float>(previousXGroupSpace));
                                    newWidth = static_cast<int>(static_cast<float>(withoutFirstMarginPart - (nbGroupParts * previousXGroupSpace) - (secondPart - traceWidth)) * previousDownSampling/ downSampling) * Xstep
                                            + (nbGroupParts * XGroupSpace) + (XGroupSpace - marginPart) + secondMarginPart;
                                }
                            }
                        }
                        //r.left() <= traceWidth
                        else{
                            newLeft = static_cast<int>(static_cast<float>(r.left()) * previousDownSampling / downSampling) * Xstep;
                            //Compute the width of the window
                            int firstPart = traceWidth - r.left();
                            if(windowWidth <= firstPart){
                                newWidth =  static_cast<int>((static_cast<float>(windowWidth) * previousDownSampling/ downSampling)) * Xstep;
                            }
                            else{
                                int nbGroupParts = static_cast<int>((windowWidth - firstPart) / Xshift);
                                int secondPart = windowWidth - firstPart - (nbGroupParts * Xshift);
                                if(secondPart <= previousXGroupSpace){
                                    int marginPart = static_cast<int>(XGroupSpace * static_cast<float>(secondPart) / static_cast<float>(previousXGroupSpace));
                                    newWidth =  static_cast<int>((static_cast<float>(windowWidth - (nbGroupParts * previousXGroupSpace) - secondPart) * previousDownSampling/ downSampling)) * Xstep
                                            + (nbGroupParts * XGroupSpace) + marginPart;
                                }
                                else{
                                    newWidth =  static_cast<int>((static_cast<float>(windowWidth - ((nbGroupParts + 1) * previousXGroupSpace)) * previousDownSampling/ downSampling)) * Xstep
                                            + ((nbGroupParts + 1) * XGroupSpace);
                                }
                            }
                        }
                    }//r.left() <= Xshift
                    //r.left() > Xshift
                    else{
                        groupIndex = ((r.left() - Xshift) / Xshift) + 1;
                        int samplePart = r.left() - (groupIndex * Xshift);
                        if(samplePart <= traceWidth){
                            newLeft = static_cast<int>(static_cast<float>(r.left() - (groupIndex * previousXGroupSpace)) * previousDownSampling / downSampling) * Xstep
                                    + (groupIndex * XGroupSpace);
                            //Compute the width of the window
                            if(windowWidth <= (traceWidth - samplePart)){
                                newWidth =  static_cast<int>((static_cast<float>(windowWidth) * previousDownSampling/ downSampling)) * Xstep;
                            }
                            else{
                                int nbGroupParts = static_cast<int>((windowWidth - (traceWidth - samplePart)) / Xshift);
                                int secondPart = windowWidth - (traceWidth - samplePart) - (nbGroupParts * Xshift);
                                if(secondPart <= previousXGroupSpace){
                                    int marginPart = static_cast<int>(XGroupSpace * static_cast<float>(secondPart) / static_cast<float>(previousXGroupSpace));
                                    newWidth =  static_cast<int>((static_cast<float>(windowWidth - (nbGroupParts * previousXGroupSpace) - secondPart) * previousDownSampling/ downSampling)) * Xstep
                                            + (nbGroupParts * XGroupSpace) + marginPart;
                                }
                                else{
                                    newWidth =  static_cast<int>((static_cast<float>(windowWidth - ((nbGroupParts + 1) * previousXGroupSpace)) * previousDownSampling/ downSampling)) * Xstep
                                            + ((nbGroupParts + 1) * XGroupSpace);
                                }
                            }
                        }//samplePart <= traceWidth
                        else{
                            int marginPart =  static_cast<int>(XGroupSpace * static_cast<float>(samplePart - traceWidth) / static_cast<float>(previousXGroupSpace));
                            newLeft = static_cast<int>(static_cast<float>(r.left() - (groupIndex * previousXGroupSpace) - (samplePart - traceWidth)) * previousDownSampling / downSampling) * Xstep
                                    + (groupIndex * XGroupSpace) + marginPart;
                            //Compute the width of the window
                            int firstMargin = previousXGroupSpace - (samplePart - traceWidth);
                            int firstMarginPart = static_cast<int>(XGroupSpace * static_cast<float>(firstMargin) / static_cast<float>(previousXGroupSpace));
                            int nbGroupParts = static_cast<int>((windowWidth - firstMargin) / Xshift);
                            int secondPart = windowWidth - firstMargin - (nbGroupParts * Xshift);
                            if(secondPart <= traceWidth){
                                newWidth = static_cast<int>(static_cast<float>(windowWidth - firstMargin - (nbGroupParts * previousXGroupSpace)) * previousDownSampling/ downSampling) * Xstep
                                        + (nbGroupParts * XGroupSpace) + firstMarginPart;
                            }
                            else{
                                int secondMarginPart = static_cast<int>(XGroupSpace * static_cast<float>(secondPart - traceWidth) / static_cast<float>(previousXGroupSpace));
                                newWidth = static_cast<int>(static_cast<float>(windowWidth - firstMargin - (secondPart - traceWidth) - (nbGroupParts * previousXGroupSpace)) * previousDownSampling/ downSampling) * Xstep
                                        + (nbGroupParts * XGroupSpace) + firstMarginPart + secondMarginPart;
                            }
                        }
                    }
                }

                //Update the object variables
                int nbSamplesToDraw = static_cast<int>(floor(0.5 + static_cast<float>(nbSamples)/downSampling));
                Xshift = (nbSamplesToDraw - 1) * Xstep + XGroupSpace;
                traceWidth = static_cast<int>(traceWidth * previousDownSampling/ downSampling) * Xstep;
                timeStep = timeStepUnit * downSampling;

                //update the window
                r.setLeft(newLeft);
                r.setWidth(newWidth);
                window = ZoomWindow(r);
            }
            //Single column
            else{
                float factor = static_cast<float>(previousWindow.width())/static_cast<float>(windowWidth);
                float previousDownSampling = downSampling;

                downSampling = downSampling/factor;
                if(downSampling < 1) downSampling = 1;
                int newWidth = static_cast<int>(windowWidth * previousDownSampling/ downSampling) * Xstep;
                int newLeft = static_cast<int>(static_cast<float>(r.left()) * previousDownSampling / downSampling) * Xstep;
                timeStep = timeStepUnit * downSampling;

                //update the window
                r.setLeft(newLeft);
                r.setWidth(newWidth);
                window = ZoomWindow(r);
            }
        }
    }
    else if(zoomed && doubleClick){
        setCursor(zoomCursor);
        zoomed = false;
        maxZoomReached = false;
        zoomFactor = 0;
        downSampling = initialDownSampling;
        timeStep = downSampling * timeStepUnit;

        if(multiColumns){
            Xshift = initialXshift;
            XGroupSpace = initialXGroupSpace;
            traceWidth = initialTraceWidth;
        }

        doubleClick = false;
        firstZoom = true;
    }

}

void TraceView::addClusterProvider(ClustersProvider* clustersProvider,QString name,ItemColors* clusterColors,bool active,
                                   QList<int>& clustersToShow,QMap<int, QList<int> >* displayGroupsClusterFile,QMap<int,int>* channelsSpikeGroups,
                                   int nbSamplesBefore,int nbSamplesAfter,const QList<int>& clustersToSkip){

    //Set Connection
    connect(clustersProvider,SIGNAL(dataReady(Array<dataType>&,QObject*,QString)),this,SLOT(dataAvailable(Array<dataType>&,QObject*,QString)));
    connect(clustersProvider,SIGNAL(nextClusterDataReady(Array<dataType>&,QObject*,QString,long,long)),this,SLOT(nextClusterDataAvailable(Array<dataType>&,QObject*,QString,long,long)));
    connect(clustersProvider,SIGNAL(previousClusterDataReady(Array<dataType>&,QObject*,QString,long,long)),this,SLOT(previousClusterDataAvailable(Array<dataType>&,QObject*,QString,long,long)));

    updateNoneBrowsingClusterList(name,clustersToSkip);

    if(clustersToShow.size() != 0){
        QList<int> clusters;
        QList<int>::iterator shownClustersIterator;
        for(shownClustersIterator = clustersToShow.begin(); shownClustersIterator != clustersToShow.end(); ++shownClustersIterator){
            clusters.append(*shownClustersIterator);
        }

        selectedClusters.insert(name.toInt(),clusters);
        ClusterData* clusterData = new ClusterData();
        clustersData.insert(name,clusterData);

        //if the view is the active one, load the cluster data for the current start time and time window
        if(active){
            setCursor(Qt::WaitCursor);
            clustersProvider->requestData(startTime,endTime,this,startTimeInRecordingUnits);
        }
    }

    clusterProviders.insert(name,clustersProvider);
    providerItemColors.insert(name,clusterColors);

    groupClusterFiles = displayGroupsClusterFile;
    channelClusterFiles = channelsSpikeGroups;
    this->nbSamplesBefore = nbSamplesBefore;
    this->nbSamplesAfter = nbSamplesAfter;
}

void TraceView::removeClusterProvider(const QString &name, bool active){
    selectedClusters.remove(name.toInt());
    clustersNotUsedForBrowsing.remove(name);
    clusterProviders.remove(name);
    providerItemColors.remove(name);
    clustersData.remove(name);

    if(raster) updateWindow();
    drawContentsMode = REDRAW;
    if(active) update();
}


void TraceView::showClusters(QString name,QList<int>& clustersToShow){
    ClusterData* clusterData;
    clusterData = clustersData[name];

    QList<int> clusters;
    if(clustersToShow.size() != 0){
        QList<int>::iterator shownClustersIterator;
        for(shownClustersIterator = clustersToShow.begin(); shownClustersIterator != clustersToShow.end(); ++shownClustersIterator){
            clusters.append(*shownClustersIterator);
        }
        selectedClusters.insert(name.toInt(),clusters);
        if(clusterData == 0){
            clusterData = new ClusterData();
            clustersData.insert(name,clusterData);
            ClustersProvider* provider = clusterProviders[name];
            setCursor(Qt::WaitCursor);
            updateWindow();
            provider->requestData(startTime,endTime,this,startTimeInRecordingUnits);
        }
        //Redraw
        else{
            updateWindow();
            update();
        }
    }
    else{
        selectedClusters.insert(name.toInt(),clusters);
        updateWindow();
        update();
    }
}

void TraceView::updateNoneBrowsingClusterList(QString providerName,const QList<int>& clustersToNotBrowse){
    QList<int> clusters;
    QList<int>::const_iterator iterator;
    for(iterator = clustersToNotBrowse.begin(); iterator != clustersToNotBrowse.end(); ++iterator){
        clusters.append(*iterator);
    }
    clustersNotUsedForBrowsing.insert(providerName,clusters);
}

void TraceView::skipStatusChanged(const QList<int>& skippedChannels){
    this->skippedChannels.clear();
    QList<int>::const_iterator iterator;
    for(iterator = skippedChannels.begin(); iterator != skippedChannels.end(); ++iterator){
        this->skippedChannels.append(*iterator);
    }
}

void TraceView::clusterColorUpdate(QString name,int clusterId,bool active){
    //redraw everything
    if(active){
        drawContentsMode = REDRAW ;
        update();
    }
}

void TraceView::updateWaveformInformation(int nbSamplesBefore, int nbSamplesAfter,bool active){
    this->nbSamplesBefore = nbSamplesBefore;
    this->nbSamplesAfter = nbSamplesAfter;

    if(active){
        drawContentsMode = REDRAW ;
        update();
    }
}

void TraceView::print(QPainter& printPainter,int width, int height,bool whiteBackground){
    //Draw the double buffer (pixmap) by copying it into the printer device throught the painter.
    QRect viewportOld = QRect(viewport.left(),viewport.top(),viewport.width(),viewport.height());

    //If the left margin is not visible (the user zoomed without taking it in his selection), the viewport and the printer
    //have the same size.
    QRect r((QRect)window);
    if(r.left() != 0) viewport = QRect(printPainter.viewport().left(),printPainter.viewport().top(),printPainter.viewport().width(),printPainter.viewport().height());
    else viewport = QRect(printPainter.viewport().left() + xMargin,printPainter.viewport().top(),printPainter.viewport().width() - xMargin,printPainter.viewport().height());

    //Enable to draw all the points without down sampling in order to have one line per trace and not a multiple number of small vertical lines
    //This will suppress any zoom.
    printState = true;
    updateWindow();
    r = ((QRect)window);

    //Set the window (part of the world I want to show)
    printPainter.setWindow(r.left(),r.top(),r.width()-1,r.height()-1);//hack because Qt QRect is used differently in this function

    //Set the viewport (part of the device I want to write on).
    //By default, the viewport is the same as the device's rectangle (contentsRec), taking a smaller
    //one will ensure that the legends will not ovelap the traces.
    printPainter.setViewport(viewport);

    //Fill the background with the background color and ensure we draw the same portion of the world than on the screen
    QRect back = QRect(r.left(),r.top(),r.width(),r.height());
    float widthRatio = (static_cast<float>(back.width())/static_cast<float>(width));
    if(r.left() == 0) back.setLeft(r.left() - static_cast<long>(xMargin * widthRatio));

    QColor colorLegendTmp = colorLegend;
    QColor background= backgroundColor();
    if(whiteBackground){
        colorLegend = Qt::black;
        setPaletteBackgroundColor(Qt::white);
    }

    printPainter.fillRect(back,backgroundColor());
    printPainter.setClipRect(back);

    //Paint all the traces in the shownChannels list (in the double buffer)
    drawTraces(printPainter);

    //reset transformation due to setWindow and setViewport
    printPainter.resetMatrix() ;

    //Draw channel ids and amplitude on the left side.
    if(showLabels) drawChannelIdsAndGain(printPainter);

    //Draw the calibration scale
    if(showCalibrationScale) drawCalibrationScale(printPainter);

    printPainter.setClipping(false);

    //Restore the colors.
    if(whiteBackground){
        colorLegend = colorLegendTmp;
        setPaletteBackgroundColor(background);
    }

    //Restore the previous state
    viewport = QRect(viewportOld.left(),viewportOld.top(),viewportOld.width(),viewportOld.height());
    printState = false;
    updateWindow();
}



void TraceView::addEventProvider(EventsProvider* eventsProvider,QString name,ItemColors* eventColors,
                                 bool active,QList<int>& eventsToShow,const QList<int>& eventsToSkip){

    //Set Connections
    connect(eventsProvider,SIGNAL(dataReady(Array<dataType>&,Array<int>&,QObject*,QString)),this,SLOT(dataAvailable(Array<dataType>&,Array<int>&,QObject*,QString)));
    connect(eventsProvider,SIGNAL(nextEventDataReady(Array<dataType>&,Array<int>&,QObject*,QString,long)),this,SLOT(nextEventDataAvailable(Array<dataType>&,Array<int>&,QObject*,QString,long)));
    connect(eventsProvider,SIGNAL(previousEventDataReady(Array<dataType>&,Array<int>&,QObject*,QString,long)),this,SLOT(previousEventDataAvailable(Array<dataType>&,Array<int>&,QObject*,QString,long)));

    updateNoneBrowsingEventList(name,eventsToSkip);

    if(eventsToShow.size() != 0){
        QList<int> events;
        QList<int>::iterator shownEventsIterator;
        for(shownEventsIterator = eventsToShow.begin(); shownEventsIterator != eventsToShow.end(); ++shownEventsIterator){
            events.append(*shownEventsIterator);
        }

        selectedEvents.insert(name,events);
        EventData* eventData = new EventData();
        eventsData.insert(name,eventData);

        //if the view is the active one, load the event data for the current start time and time window
        if(active){
            setCursor(Qt::WaitCursor);
            eventsProvider->requestData(startTime,endTime,this);
        }
    }

    eventProviders.insert(name,eventsProvider);
    providerItemColors.insert(name,eventColors);
}

void TraceView::removeEventProvider(const QString& name,bool active){
    selectedEvents.remove(name);
    eventsNotUsedForBrowsing.remove(name);
    eventProviders.remove(name);
    providerItemColors.remove(name);
    eventsData.remove(name);

    drawContentsMode = REDRAW;
    if(active) update();
}

void TraceView::showEvents(QString name,QList<int>& eventsToShow){
    EventData* eventData;
    eventData = eventsData[name];

    QList<int> events;
    if(eventsToShow.size() != 0){
        QList<int>::iterator shownEventsIterator;
        for(shownEventsIterator = eventsToShow.begin(); shownEventsIterator != eventsToShow.end(); ++shownEventsIterator){
            events.append(*shownEventsIterator);
        }
        selectedEvents.insert(name,events);
        if(eventData == 0){
            eventData = new EventData();
            eventsData.insert(name,eventData);
            EventsProvider* provider = eventProviders[name];
            setCursor(Qt::WaitCursor);
            provider->requestData(startTime,endTime,this);
        }
        //Redraw
        else{
            drawContentsMode = REDRAW;
            update();
        }
    }
    else{
        selectedEvents.insert(name,events);
        drawContentsMode = REDRAW;
        update();
    }
}

void  TraceView::updateEvents(QString providerName,QList<int>& eventsToShow,bool active){
    EventData* eventData;
    eventData = eventsData[providerName];

    QList<int> events;
    QList<int>::iterator shownEventsIterator;
    for(shownEventsIterator = eventsToShow.begin(); shownEventsIterator != eventsToShow.end(); ++shownEventsIterator){
        events.append(*shownEventsIterator);
    }
    selectedEvents.insert(providerName,events);
    if(eventData == 0){
        eventData = new EventData();
        eventsData.insert(providerName,eventData);
    }

    if(!eventProvidersToUpdate.contains(providerName)) eventProvidersToUpdate.append(providerName);
    if(active) update();
}

void TraceView::updateNoneBrowsingEventList(QString providerName,const QList<int>& eventsToNotBrowse){
    QList<int> events;
    QList<int>::const_iterator iterator;
    for(iterator = eventsToNotBrowse.begin(); iterator != eventsToNotBrowse.end(); ++iterator){
        events.append(*iterator);
    }
    eventsNotUsedForBrowsing.insert(providerName,events);
}

void TraceView::eventColorUpdate(QString name,int eventId,bool active){
    if(active){
        drawContentsMode = REDRAW ;
        update();
    }
}

void TraceView::showNextEvent(){
    if(endTime == length) return;

    //Only request data from the provider for which events have been selected
    if(!selectedEvents.isEmpty()){

        QList<QString> toRemove;
        QMap<QString, QList<int> > idsToBrowse;
        QMap<QString, QList<int> >::Iterator providersIterator;
        for(providersIterator = selectedEvents.begin(); providersIterator != selectedEvents.end(); ++providersIterator){
            if(static_cast< QList<int> >(providersIterator.data()).isEmpty()) toRemove.append(providersIterator.key());
            //check if there are events to browse through taking the skip list into account
            else{
                QList<int> selectedIds = selectedEvents[providersIterator.key()];
                QList<int> idsToNotUse = eventsNotUsedForBrowsing[providersIterator.key()];
                QList<int> ids;
                QList<int>::iterator shownEventsIterator;
                for(shownEventsIterator = selectedIds.begin(); shownEventsIterator != selectedIds.end(); ++shownEventsIterator)
                    if(!idsToNotUse.contains(*shownEventsIterator)) ids.append(*shownEventsIterator);
                if(ids.size() != 0){
                    idsToBrowse.insert(providersIterator.key(),ids);
                    static_cast<EventData*>(eventsData[providersIterator.key()])->setStatus(false);
                }
            }
        }

        QList<QString>::iterator toRemoveIterator;
        for(toRemoveIterator = toRemove.begin(); toRemoveIterator != toRemove.end(); ++toRemoveIterator){
            selectedEvents.remove(*toRemoveIterator);
            eventsData.remove(*toRemoveIterator);
        }

        if(!selectedEvents.isEmpty() && idsToBrowse.count() != 0){
            setCursor(Qt::WaitCursor);
            long timeFrameWidth = endTime - startTime;
            nextEventProvider.first.clear();
            nextEventProvider.second = 0;
            Q3DictIterator<EventData> iterator(eventsData);
            for(;iterator.current();++iterator){
                QList<int> ids = idsToBrowse[iterator.currentKey()];
                if(!static_cast<EventData*>(iterator.current())->status() && ids.size() != 0) static_cast<EventsProvider*>(eventProviders[iterator.currentKey()])->requestNextEventData(startTime,timeFrameWidth,ids,this);
            }
        }
    }
}

void TraceView::showPreviousEvent(){
    if(startTime == 0) return;

    //Only request data from the provider for which events have been selected
    if(!selectedEvents.isEmpty()){

        QList<QString> toRemove;
        QMap<QString, QList<int> > idsToBrowse;
        QMap<QString, QList<int> >::Iterator providersIterator;
        for(providersIterator = selectedEvents.begin(); providersIterator != selectedEvents.end(); ++providersIterator){
            if(static_cast< QList<int> >(providersIterator.data()).isEmpty()) toRemove.append(providersIterator.key());
            //check if there are events to browse through taking the skip list into account
            else{
                QList<int> selectedIds = selectedEvents[providersIterator.key()];
                QList<int> idsToNotUse = eventsNotUsedForBrowsing[providersIterator.key()];
                QList<int> ids;
                QList<int>::iterator shownEventsIterator;
                for(shownEventsIterator = selectedIds.begin(); shownEventsIterator != selectedIds.end(); ++shownEventsIterator)
                    if(!idsToNotUse.contains(*shownEventsIterator)) ids.append(*shownEventsIterator);
                if(ids.size() != 0){
                    idsToBrowse.insert(providersIterator.key(),ids);
                    static_cast<EventData*>(eventsData[providersIterator.key()])->setStatus(false);
                }
            }
        }

        QList<QString>::iterator toRemoveIterator;
        for(toRemoveIterator = toRemove.begin(); toRemoveIterator != toRemove.end(); ++toRemoveIterator){
            selectedEvents.remove(*toRemoveIterator);
            eventsData.remove(*toRemoveIterator);
        }

        if(!selectedEvents.isEmpty() && idsToBrowse.count() != 0){
            setCursor(Qt::WaitCursor);
            long timeFrameWidth = endTime - startTime;
            previousEventProvider.first.clear();
            previousEventProvider.second = 0;
            Q3DictIterator<EventData> iterator(eventsData);
            for(;iterator.current();++iterator){
                QList<int> ids = idsToBrowse[iterator.currentKey()];
                if(!static_cast<EventData*>(iterator.current())->status() && ids.size() != 0) static_cast<EventsProvider*>(eventProviders[iterator.currentKey()])->requestPreviousEventData(startTime,timeFrameWidth,ids,this);
            }
        }
    }
}

void TraceView::nextEventDataAvailable(Array<dataType>& times,Array<int>& ids,QObject* initiator,QString providerName,long startingTime){
    //If another widget was the initiator of the request, ignore the data.
    if(initiator != this) return;

    //if no event has been found the return startingTime is the same as the send one (endTime).
    EventData* eventData = eventsData[providerName];
    if(startingTime != startTime && startingTime < length){
        eventData->setStatus(true);
        eventData->setData(times,ids);
    }
    else eventData->setStatus(true);

    if(nextEventProvider.first == "" || (nextEventProvider.first != "" && startingTime < nextEventProvider.second)){
        nextEventProvider.first = providerName;
        nextEventProvider.second = startingTime;
    }

    bool ready = false;
    Q3DictIterator<EventData> iterator(eventsData);
    for(;iterator.current();++iterator){
        ready = iterator.current()->status();
        if(!ready) break;
    }

    //if the new start time is equals to the current startTime do not do anything
    if(ready && nextEventProvider.second != startTime && nextEventProvider.second < length){
        long timeFrameWidth = endTime - startTime;
        if((nextEventProvider.second + timeFrameWidth > length)){
            //new start time =  length - timeFrameWidth
            //update the traceWidget time widgets and retrieve the data for the new start time for all the providers.
            eventProviderToSkip.clear();
            emit setStartAndDuration(length - timeFrameWidth,timeFrameWidth);
        }
        else{
            eventProviderToSkip = nextEventProvider.first;
            //update the traceWidget time widgets and retreive the data for the new start time for all the providers except the one containing the data for the new start time.
            emit setStartAndDuration(nextEventProvider.second,timeFrameWidth);
        }
    }
    else if(ready && (nextEventProvider.second == startTime || nextEventProvider.second > length)){
        changeCursor();
    }
}


void TraceView::previousEventDataAvailable(Array<dataType>& times,Array<int>& ids,QObject* initiator,QString providerName,long startingTime){
    //If another widget was the initiator of the request, ignore the data.
    if(initiator != this) return;

    //if no event has been found the return startingTime is the same as the send one (startTime).
    //If te event file is longer than the data file, the startingTime can be over the recording Length, this
    //case as to be taken into account.
    EventData* eventData = eventsData[providerName];
    if(startingTime != startTime && startingTime < length){
        eventData->setStatus(true);
        eventData->setData(times,ids);
    }
    else eventData->setStatus(true);

    if(previousEventProvider.first == "" ||
            (previousEventProvider.first != "" && startingTime > previousEventProvider.second && startingTime != startTime) ||
            (previousEventProvider.first != "" && startingTime != startTime && previousEventProvider.second == startTime) ){

        previousEventProvider.first = providerName;
        previousEventProvider.second = startingTime;
    }

    bool ready = false;
    Q3DictIterator<EventData> iterator(eventsData);
    for(;iterator.current();++iterator){
        ready = iterator.current()->status();
        if(!ready) break;
    }

    if(ready && previousEventProvider.second != startTime && previousEventProvider.second < length){
        long timeFrameWidth = endTime - startTime;
        if(previousEventProvider.second + timeFrameWidth > length){
            eventProviderToSkip.clear();
            emit setStartAndDuration(length - timeFrameWidth,timeFrameWidth);
        }
        else{
            eventProviderToSkip = previousEventProvider.first;
            //update the traceWidget time widgets and retreive the data for the new start time for all the providers except the one containing the data for the new start time.
            emit setStartAndDuration(previousEventProvider.second,timeFrameWidth);
        }
    }
    //if the new start time is equals to the current one or superior to the recording lenght do not do anything
    else if(ready && previousEventProvider.second == startTime){
        changeCursor();

    }
    else if(ready && previousEventProvider.second > length){
        long timeFrameWidth = endTime - startTime;
        previousEventProvider.first.clear();
        previousEventProvider.second = 0;
        Q3DictIterator<EventData> iterator(eventsData);
        for(;iterator.current();++iterator){
            QList<int> selectedIds = selectedEvents[iterator.currentKey()];
            static_cast<EventsProvider*>(eventProviders[iterator.currentKey()])->requestPreviousEventData(startTime,timeFrameWidth,selectedIds,this);
        }
    }
}



void TraceView::drawEvent(const QString& providerName,int selectedEventId,dataType selectedEventIndex,bool highlight){
    QPainter painter;
    painter.begin(&doublebuffer);
    //set the window (part of the world I want to show)
    QRect r((QRect)window);

    painter.setWindow(r.left(),r.top(),r.width()-1,r.height()-1);//hack because Qt QRect is used differently in this function
    painter.setViewport(viewport);
    int top = r.top();
    int bottom = r.bottom();

    Array<dataType>& currentData = static_cast<EventData*>(eventsData[providerName])->getTimes();
    Array<int>& currentIds = static_cast<EventData*>(eventsData[providerName])->getIds();
    int nbEvents = currentData.nbOfColumns();
    for(int i = 1; i <= nbEvents;++i){
        dataType index = currentData(1,i);
        int eventId = currentIds(1,i);
        if(eventId == selectedEventId && index == selectedEventIndex){
            ItemColors* colors = providerItemColors[providerName];
            QColor color = colors->color(eventId);

            if(multiColumns){
                int X = X0;
                //Loop on all the groups (one by column)
                QList<int> groupIds = shownGroupsChannels.keys();
                QList<int>::iterator iterator;
                for(iterator = groupIds.begin(); iterator != groupIds.end(); ++iterator){
                    int abscissa = X + static_cast<int>(0.5 + (static_cast<float>(index) / downSampling));
                    if(highlight){
                        //erase the previous line
                        QPen pen(backgroundColor(),1,Qt::DotLine);
                        painter.setPen(pen);
                        painter.drawLine(abscissa,top,abscissa,bottom);

                        //redraw the line
                        pen.setColor(color);
                        pen.setWidth(2);
                        painter.setPen(pen);
                        painter.drawLine(abscissa,top,abscissa,bottom);
                    }
                    else{
                        //erase the previous line
                        QPen pen(backgroundColor(),2,Qt::DotLine);
                        painter.setPen(pen);
                        painter.drawLine(abscissa,top,abscissa,bottom);

                        //redraw the line
                        pen.setColor(color);
                        pen.setWidth(1);
                        painter.setPen(pen);
                        painter.drawLine(abscissa,top,abscissa,bottom);
                    }
                    X += Xshift;
                }
            }//multicolumns
            else{//single column
                int abscissa = static_cast<int>(0.5 + (static_cast<float>(index) / downSampling));
                if(highlight){
                    //erase the previous line
                    QPen pen(backgroundColor(),1,Qt::DotLine);
                    painter.setPen(pen);
                    painter.drawLine(abscissa,top,abscissa,bottom);

                    //redraw the line
                    pen.setColor(color);
                    pen.setWidth(2);
                    painter.setPen(pen);
                    painter.drawLine(abscissa,top,abscissa,bottom);
                }
                else{
                    //erase the previous line
                    QPen pen(backgroundColor(),2,Qt::DotLine);
                    painter.setPen(pen);
                    painter.drawLine(abscissa,top,abscissa,bottom);

                    //redraw the line
                    pen.setColor(color);
                    pen.setWidth(1);
                    painter.setPen(pen);
                    painter.drawLine(abscissa,top,abscissa,bottom);
                }
            }

            break;
        }
    }

    //reset transformation due to setWindow and setViewport
    painter.resetMatrix() ;

    //Closes the painter on the double buffer
    painter.end();

    //Draw the double buffer (pixmap) by copying it into the widget device.
    update();
}

void TraceView::channelColorUpdate(int channelId,bool active){
    //Loop on all the shown groups to find if channelId is currently displayed
    QList<int> groupIds = shownGroupsChannels.keys();
    QList<int>::iterator iterator;
    mSelectedChannels.clear();
    mDeselectedChannels.clear();
    for(iterator = groupIds.begin(); iterator != groupIds.end(); ++iterator){
        QList<int> channelIds = shownGroupsChannels[*iterator];
        if(channelIds.contains(channelId)){
            if(active){
                if(mode == SELECT)
                    mSelectedChannels.append(channelId);
                else
                    mDeselectedChannels.append(channelId);
                update();
                break;
            }
            else{
                //Everything has to be redraw at the next show of the display
                drawContentsMode = REDRAW ;
            }
        }
    }
}

void TraceView::removeEvent(){
    if(selectedEvent.first != ""){
        Array<dataType>& currentData = static_cast<EventData*>(eventsData[selectedEvent.first])->getTimes();
        Array<int>& currentIds = static_cast<EventData*>(eventsData[selectedEvent.first])->getIds();
        int nbEvents = currentData.nbOfColumns();
        for(int i = 1; i <= nbEvents;++i){
            dataType index = currentData(1,i);
            int eventId = currentIds(1,i);
            if(eventId == selectedEvent.second && index == selectedEventPosition[0]){
                double samplingRate = static_cast<double>(tracesProvider.getSamplingRate()) / 1000.0;//in eventProvider the time is in miliseconds
                double time = (static_cast<double>(index) / samplingRate) + static_cast<double>(startTime);
                eventBeingModified = true;
                //Notify the provider of the deletion
                static_cast<EventsProvider*>(eventProviders[selectedEvent.first])->removeEvent(eventId,time);
                eventBeingModified = false;

                //notify everybody of the deletion
                emit eventRemoved(selectedEvent.first,eventId,time);

                break;
            }
        }
    }

    previousDragAbscissa = 0;
    selectedEvent.first.clear();
    selectedEvent.second = 0;
    startEventDragging = true;
    drawContentsMode = REDRAW;
    //Redraw
    update();
}


void TraceView::showNextCluster(){
    if(endTime == length)
        return;

    //Only request data from the provider for which clusters have been selected
    if(!selectedClusters.isEmpty()){
        QList<int> toRemove;
        QMap<int, QList<int> > idsToBrowse;
        QMap<int, QList<int> >::Iterator providersIterator;
        for(providersIterator = selectedClusters.begin(); providersIterator != selectedClusters.end(); ++providersIterator){
            if(static_cast< QList<int> >(providersIterator.data()).isEmpty()) toRemove.append(providersIterator.key());
            //check if there are clusters to browse through taking the skip list
            else{
                QList<int> selectedIds = selectedClusters[providersIterator.key()];
                QList<int> idsToNotUse = clustersNotUsedForBrowsing[QString::fromLatin1("%1").arg(providersIterator.key())];
                QList<int> ids;
                QList<int>::iterator shownClustersIterator;
                for(shownClustersIterator = selectedIds.begin(); shownClustersIterator != selectedIds.end(); ++shownClustersIterator)
                    if(!idsToNotUse.contains(*shownClustersIterator)) ids.append(*shownClustersIterator);
                if(ids.size() != 0){
                    idsToBrowse.insert(providersIterator.key(),ids);
                    static_cast<ClusterData*>(clustersData[QString::fromLatin1("%1").arg(providersIterator.key())])->setStatus(false);
                }
            }
        }

        QList<int>::iterator toRemoveIterator;
        for(toRemoveIterator = toRemove.begin(); toRemoveIterator != toRemove.end(); ++toRemoveIterator){
            selectedClusters.remove(*toRemoveIterator);
            clustersData.remove(QString::fromLatin1("%1").arg(*toRemoveIterator));
        }

        if(!selectedClusters.isEmpty() && idsToBrowse.count() != 0){
            setCursor(Qt::WaitCursor);
            long timeFrameWidth = endTime - startTime;
            nextClusterProvider.first.clear();
            nextClusterProvider.second = 0;
            previousStartTimeInRecordingUnits = startTimeInRecordingUnits;
            Q3DictIterator<ClusterData> iterator(clustersData);
            for(;iterator.current();++iterator){
                QList<int> ids = idsToBrowse[iterator.currentKey().toInt()];

                qDebug()<<"key " <<iterator.currentKey().toInt()<<" ids.size() " <<ids.size()<<" startTime " <<startTime<<" startTimeInRecordingUnits " <<startTimeInRecordingUnits ;

                if(!static_cast<ClusterData*>(iterator.current())->status() && ids.size() != 0)
                    static_cast<ClustersProvider*>(clusterProviders[iterator.currentKey()])->requestNextClusterData(startTime,timeFrameWidth,ids,this,startTimeInRecordingUnits);
            }
        }
    }
}

void TraceView::showPreviousCluster(){
    if(startTime == 0) return;

    //Only request data from the provider for which clusters have been selected
    if(!selectedClusters.isEmpty()){
        QList<int> toRemove;
        QMap<int, QList<int> > idsToBrowse;
        QMap<int, QList<int> >::Iterator providersIterator;
        for(providersIterator = selectedClusters.begin(); providersIterator != selectedClusters.end(); ++providersIterator){
            if(static_cast< QList<int> >(providersIterator.data()).isEmpty()) toRemove.append(providersIterator.key());
            //check if there are clusters to browse through taking the skip list
            else{
                QList<int> selectedIds = selectedClusters[providersIterator.key()];
                QList<int> idsToNotUse = clustersNotUsedForBrowsing[QString::fromLatin1("%1").arg(providersIterator.key())];
                QList<int> ids;
                QList<int>::iterator shownClustersIterator;
                for(shownClustersIterator = selectedIds.begin(); shownClustersIterator != selectedIds.end(); ++shownClustersIterator){
                    if(!idsToNotUse.contains(*shownClustersIterator)) ids.append(*shownClustersIterator);
                }
                if(ids.size() != 0){
                    idsToBrowse.insert(providersIterator.key(),ids);
                    static_cast<ClusterData*>(clustersData[QString::fromLatin1("%1").arg(providersIterator.key())])->setStatus(false);
                }
            }
        }

        QList<int>::iterator toRemoveIterator;
        for(toRemoveIterator = toRemove.begin(); toRemoveIterator != toRemove.end(); ++toRemoveIterator){
            selectedClusters.remove(*toRemoveIterator);
            clustersData.remove(QString::fromLatin1("%1").arg(*toRemoveIterator));
        }

        if(!selectedClusters.isEmpty() && idsToBrowse.count() != 0){
            setCursor(Qt::WaitCursor);
            long timeFrameWidth = endTime - startTime;
            previousClusterProvider.first.clear();
            previousClusterProvider.second = 0;
            previousStartTimeInRecordingUnits = startTimeInRecordingUnits;
            Q3DictIterator<ClusterData> iterator(clustersData);
            for(;iterator.current();++iterator){
                QList<int> ids = idsToBrowse[iterator.currentKey().toInt()];
                if(!static_cast<ClusterData*>(iterator.current())->status() && ids.size() != 0) static_cast<ClustersProvider*>(clusterProviders[iterator.currentKey()])->requestPreviousClusterData(startTime,timeFrameWidth,ids,this,startTimeInRecordingUnits);
            }
        }
    }
}

void TraceView::nextClusterDataAvailable(Array<dataType>& data,QObject* initiator,QString providerName,long startingTime,long startingTimeInRecordingUnits){
    //If another widget was the initiator of the request, ignore the data.
    if(initiator != this)
        return;


    qDebug()<<" providerName " <<providerName<<" data.nbOfColumns() " <<data.nbOfColumns()<<" startingTime " <<startingTime ;

    qDebug()<<" nextClusterProvider.first " <<nextClusterProvider.first<<" startingTimeInRecordingUnits " <<startingTimeInRecordingUnits<<" startTimeInRecordingUnits " <<startTimeInRecordingUnits ;


    //if no cluster has been found the return startingTime is the same as the send one (endTime).
    ClusterData* clusterData = clustersData[providerName];
    if(startingTime != startTime && startingTime < length){
        clusterData->setStatus(true);
        clusterData->setData(data);
    }
    else clusterData->setStatus(true);

    if(nextClusterProvider.first == "" || (nextClusterProvider.first != "" && startingTimeInRecordingUnits < startTimeInRecordingUnits)){
        nextClusterProvider.first = providerName;
        nextClusterProvider.second = startingTime;
        startTimeInRecordingUnits = startingTimeInRecordingUnits;
        spikeBrowsing = true;
    }

    bool ready = false;
    Q3DictIterator<ClusterData> iterator(clustersData);
    for(;iterator.current();++iterator){
        ready = iterator.current()->status();
        if(!ready) break;
    }

    //if the new start time is equals to the current startTime (in recording unit) do not do anything
    if(ready && startTimeInRecordingUnits != previousStartTimeInRecordingUnits && nextClusterProvider.second < length){
        long timeFrameWidth = endTime - startTime;
        if(nextClusterProvider.second + timeFrameWidth > length){
            //new start time =  length - timeFrameWidth
            //update the traceWidget time widgets and retrieve the data for the new start time for all the providers.
            clusterProviderToSkip.clear();
            emit setStartAndDuration(length - timeFrameWidth,timeFrameWidth);
        }
        else{
            clusterProviderToSkip = nextClusterProvider.first;

            qDebug()<<" clusterProviderToSkip " <<clusterProviderToSkip<<" startTimeInRecordingUnits "<<startTimeInRecordingUnits<<" startTime "<<startTime<<" endTime "<<endTime;
            qDebug()<<" previousStartTimeInRecordingUnits " <<previousStartTimeInRecordingUnits<<" nextClusterProvider.second " <<nextClusterProvider.second<<" length "<<length;


            //update the traceWidget time widgets and retrieve the data for the new start time for all the providers except the one containing the data for the new start time.
            emit setStartAndDuration(nextClusterProvider.second,timeFrameWidth);
        }
    }
    else if(ready && (startTimeInRecordingUnits == previousStartTimeInRecordingUnits || nextClusterProvider.second > length)){
        startTimeInRecordingUnits = 0;
        changeCursor();
    }
}

void TraceView::previousClusterDataAvailable(Array<dataType>& data,QObject* initiator,QString providerName,long startingTime,long startingTimeInRecordingUnits){
    //If another widget was the initiator of the request, ignore the data.
    if(initiator != this) return;

    //if no cluster has been found the return startingTime is the same as the send one (startTime).
    //If the cluster file is longer than the data file, the startingTime can be over the recording Length, this
    //case has to be taken into account.
    ClusterData* clusterData = clustersData[providerName];
    if(startingTime != startTime && startingTime < length){
        clusterData->setStatus(true);
        clusterData->setData(data);
    }
    else clusterData->setStatus(true);

    if(previousClusterProvider.first == "" ||
            (previousClusterProvider.first != "" && startingTimeInRecordingUnits > startTimeInRecordingUnits && startingTimeInRecordingUnits != previousStartTimeInRecordingUnits) ||
            (previousClusterProvider.first != "" && startingTimeInRecordingUnits != previousStartTimeInRecordingUnits && startTimeInRecordingUnits == previousStartTimeInRecordingUnits)){

        previousClusterProvider.first = providerName;
        previousClusterProvider.second = startingTime;
        startTimeInRecordingUnits = startingTimeInRecordingUnits;
        spikeBrowsing = true;
    }

    bool ready = false;
    Q3DictIterator<ClusterData> iterator(clustersData);
    for(;iterator.current();++iterator){
        ready = iterator.current()->status();
        if(!ready) break;
    }

    if(ready && startTimeInRecordingUnits != previousStartTimeInRecordingUnits && previousClusterProvider.second < length){
        long timeFrameWidth = endTime - startTime;
        if(previousClusterProvider.second + timeFrameWidth > length){
            clusterProviderToSkip.clear();

            emit setStartAndDuration(length - timeFrameWidth,timeFrameWidth);
        }
        else{
            clusterProviderToSkip = previousClusterProvider.first;
            //update the traceWidget time widgets and retrieve the data for the new start time for all the providers except the one containing the data for the new start time.
            emit setStartAndDuration(previousClusterProvider.second,timeFrameWidth);
        }
    }
    //if the new start time (in recording unit) is equals to the current one do not do anything
    else if(ready && startTimeInRecordingUnits == previousStartTimeInRecordingUnits){
        changeCursor();

    }
    //if the new start time is superior to the recording lenght, look up in all the files at the last timeFrameWidth of the data file
    else if(ready && previousClusterProvider.second > length){
        clusterProviderToSkip.clear();
        long timeFrameWidth = endTime - startTime;
        previousClusterProvider.first.clear();
        previousClusterProvider.second = 0;
        Q3DictIterator<ClusterData> iterator(clustersData);
        for(;iterator.current();++iterator){
            QList<int> selectedIds = selectedClusters[iterator.currentKey().toInt()];
            static_cast<ClustersProvider*>(clusterProviders[iterator.currentKey()])->requestPreviousClusterData(startTime,timeFrameWidth,selectedIds,this,previousStartTimeInRecordingUnits);
        }
    }
}

void TraceView::updateClusters(QString name,QList<int>& clustersToShow,ItemColors* clusterColors,bool active){
    providerItemColors.remove(name);
    providerItemColors.insert(name,clusterColors);
    showClusters(name,clustersToShow);
    updateClusterData(active);
}

void TraceView::increaseRatio(){
    if(raster){
        int onePixel = viewportToWorldHeight(1);
        //Limit case: if the height of the widget is greater than the width of the world, onePixel is equal to 0. Set it to 1 to keep growing
        if(onePixel == 0) onePixel = 1;
        rasterHeight += onePixel;

        YRasterSpace = rasterHeight/3;
        updateWindow();
        //Redraw
        update();
    }
}

void TraceView::decreaseRatio(){
    if(raster){
        int onePixel = viewportToWorldHeight(1);
        //Limit case: if the height of the widget is greater than the width of the world, onePixel is equal to 0. Set it to 1 to keep decreasing
        if(onePixel == 0) onePixel = 1;
        rasterHeight -= onePixel;

        //The rasterHeight has to be at least 1px in the widget
        if(rasterHeight < onePixel) rasterHeight = onePixel;
        YRasterSpace = rasterHeight/3;
        updateWindow();
        //Redraw
        update();
    }
}

void TraceView::getCurrentEventInformation(long startTime,long endTime,QObject* initiator){
    //If an event has been added or removed, make sure the information is up to date before emitting a signal with it.
    if(!eventProvidersToUpdate.isEmpty()){
        QList<QString>::iterator providerIterator;
        EventData* eventData;
        for(providerIterator = eventProvidersToUpdate.begin(); providerIterator != eventProvidersToUpdate.end(); ++providerIterator){
            eventData = eventsData[*providerIterator];
            if(eventData == 0){
                eventData = new EventData();
                eventsData.insert(*providerIterator,eventData);
            }
            else eventsData[*providerIterator]->setStatus(false);
        }

        Q3DictIterator<EventData> iterator(eventsData);
        for(;iterator.current();++iterator){
            if(eventProvidersToUpdate.contains(iterator.currentKey())){
                static_cast<EventsProvider*>(eventProviders[iterator.currentKey()])->requestData(startTime,endTime,this);
                eventProvidersToUpdate.remove(iterator.currentKey());
            }
        }
    }

    if(selectedEvents.size() != 0) emit eventsAvailable(eventsData,selectedEvents,providerItemColors,initiator,tracesProvider.getSamplingRate());
}

void TraceView::traceBackgroundImageUpdate(QImage traceBackgroundImage,bool active){
    background = traceBackgroundImage;

    scaleBackgroundImage();

    drawContentsMode = REDRAW;
    if(active)
        update();
}

void TraceView::scaleBackgroundImage(){
    if(!background.isNull()){
        QRect contentsRec = contentsRect();
        scaledBackground.convertFromImage(background.smoothScale(contentsRec.width(),contentsRec.height()),Qt::PreferDither);
    }
}




#include "traceview.moc"
