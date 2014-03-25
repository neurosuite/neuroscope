/***************************************************************************
                          tracewidget.h  -  description
                             -------------------
    begin                : Wed Mar 17 2004
    copyright            : (C) 2004 by Lynn Hazan
    email                : lynn.hazan.myrealbox.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef TRACEWIDGET_H
#define TRACEWIDGET_H

// include files for QT
#include <QWidget>
#include <qspinbox.h>
#include <QValidator>
#include <QLineEdit>
#include <QLabel>
#include <qscrollbar.h>
#include <QPainter>
//#include <QStyle>

#include <QKeyEvent>
#include <QList>

//Inclused files for KDE
#include <QStatusBar>

//include files for the application
#include "traceview.h"

//include files for c/c++ libraries
#include <math.h>

class TracesProvider;
class BaseFrame;
class ChannelColors;

/**
  * Class containing the TraceView and all the widgets used to select the position in the file.
  * @author Lynn Hazan
  */
class TraceWidget : public QWidget  {
    Q_OBJECT

public:
    /**
  * @param startTime starting time in miliseconds.
  * @param duration time window in miliseconds.
  * @param greyScale true if all the channels are display either in a gradation of grey false if they are display in color.
  * @param tracesProvider a reference on the provider of the channels data.
  * @param multiColumns true if the traces are displayed on multiple columns, false otherwise.
  * @param verticalLines true if vertical lines are displayed to show the clusters, false otherwise.
  * @param raster true if a raster is drawn to show the clusters, false otherwise
  * @param waveforms true if waveforms are drawn on top of the traces, false otherwise.
  * @param labelsDisplay true if labels are drawn next to the traces, false otherwise.
  * @param channelsToDisplay  a reference on the list of channel to be shown at the opening of the view.
  * @param gain initial gain use to draw the traces in the TraceView.
  * @param acquisitionGain acquisition gain.
  * @param channelColors a pointer on the list of colors for the channels.
  * @param groupsChannels a pointer on the map given the list of channels for each group.
  * @param channelsGroups a pointer on the map given to which group each channel belongs.
  * @param autocenterChannels whether all channels should be autocentered around their offset.
  * @param channelOffsets a reference on the list containing the offset for each channel.
  * @param gains a reference on the list of the exponents used to compute the drawing gain for each channel.
  * @param skippedChannels list of skipped channels.
  * @param rasterHeight height of the rasters in the world coordinate system.
  * @param backgroundImage image used as background.
  * @param parent the parent QWidget.
  * @param name name of the widget (can be used for introspection).
  * @param backgroundColor color used as background.
  * @param statusBar a reference to the application status bar.
  * @param minSize minumum size of the view.
  * @param maxSize maximum size of the view.
  * @param windowTopLeft the top-left corner of the window (QRect corresponding
  * to the part of the drawing which will actually be drawn onto the widget).
  * @param windowBottomRight bottom-right corner of the window (QRect corresponding
  * to the part of the drawing which will actually be drawn onto the widget).
  * @param border size of the border between the frame and the contents.
  */
    TraceWidget(long startTime, long duration, bool greyScale, TracesProvider& tracesProvider, bool multiColumns, bool verticalLines,
                bool raster, bool waveforms, bool labelsDisplay, QList<int>& channelsToDisplay, int gain, int acquisitionGain, ChannelColors* channelColors,
                QMap<int, QList<int> >* groupsChannels, QMap<int,int>* channelsGroups, bool autocenterChannels, QList<int>& channelOffsets,
                QList<int>& gains, const QList<int>& skippedChannels, int rasterHeight, const QImage &backgroundImage, QWidget* parent=0, const char* name=0, const QColor &backgroundColor = Qt::black,
                QStatusBar* statusBar = 0L, int minSize = 0, int maxSize = 4000, int windowTopLeft = -500,
                int windowBottomRight = 1001, int border = 0);

    ~TraceWidget();


    /**Returns the time amount, in milisecond, of the time frame used by the TraceView.
 */
    long timeFrameWidth() const {
        return timeWindow;
    }

    /**Returns the start time of the time frame used by the TraceView.
 */
    long timeFrameStart() const {
        return startTime;
    }

    /**Installs the event filter on the view contained in the widget.*/
    void installEventFilter(QObject* filterObject){
        QObject::installEventFilter(filterObject);
        view.installEventFilter(filterObject);
    }

    /// Added by M.Zugaro to enable automatic forward paging
    bool isStill();

Q_SIGNALS:
	
    /// Added by M.Zugaro to enable automatic forward paging
    void stopped();

public Q_SLOTS:

    /// Added by M.Zugaro to enable automatic forward paging
    void page();
    void accelerate();
    void decelerate();
    void advance();
    void stop();

    /**Changes the color of the background.*/
    void changeBackgroundColor(const QColor &color);

    /**All the channels are now display either in a gradation of grey or in color.
  * @param grey true if the channels have to be displayed in grey false otherwise.
  */
    void setGreyScale(bool grey);

    /**Display the traces starting at the given time in miliseconds. The selection widgets are updated accordingly.
  * @param time starting time to display the traces.
  */
    void moveToTime(long time);

    /**
  * Sets the mode of presentation to single or multiple columns.
  * @param multiple true if the traces are presented on multiple columns, false if they are presented on a single column.
  */
    void setMultiColumns(bool multiple){view.setMultiColumns(multiple);}

    /**Displays or hides vertical lines to show the clusters.
  * @param lines true if the vertical lines are drawn for each cluster, false otherwise.
  */
    void setClusterVerticalLines(bool lines){view.setClusterVerticalLines(lines);}

    /**Displays or hides a raster to show the clusters.
  * @param raster true if a raster is drawn, false otherwise.
  */
    void setClusterRaster(bool raster){
        view.setClusterRaster(raster);
    }

    /**Displays or hides the cluster waveforms on top of the traces.
  * @param waveforms true if the waveforms are drawn, false otherwise.
  */
    void setClusterWaveforms(bool waveforms){view.setClusterWaveforms(waveforms);}

    /** Informs listener that the channels @p selectedIds have been selected.
  * @param selectedIds the list of channels selected by the user in a view.
  */
    void slotChannelsSelected(const QList<int>& selectedIds);

    /**Change the current mode, call by a selection of a tool.
  * @param selectedMode new mode of drawing.
  * @param active true if the view is the active one, false otherwise.
  */
    virtual void setMode(BaseFrame::Mode selectedMode,bool active);

    /**
  * Updates the list of channels shown with @p channelsToShow.
  * @param channelsToShow new list of channels to be shown.
  */
    void showChannels(const QList<int>& channelsToShow){
        view.showChannels(channelsToShow);
    }

    /**
  * Updates the list of clusters shown with @p clustersToShow for the cluster provider identified
  * by @p name.
  * @param name name use to identified the cluster provider containing the clusters to show.
  * @param clustersToShow new list of clusters to be shown.
  */
    void showClusters(const QString& name, const QList<int>& clustersToShow){
        view.showClusters(name,clustersToShow);
    }

    /**
  * Updates the list of events shown with @p eventsToShow for the event provider identified
  * by @p name.
  * @param name name use to identified the event provider containing the events to show.
  * @param eventsToShow new list of events to be shown.
  */
    void showEvents(const QString &name,QList<int>& eventsToShow){
        view.showEvents(name,eventsToShow);
    }

    /**Changes the color of a channel.
  * @param channelId id of the channel to redraw.
  * @param active true if the view is the active one, false otherwise.
  */
    void channelColorUpdate(int channelId,bool active){view.channelColorUpdate(channelId,active);}

    /**Changes the color of a cluster.
  * @param name name use to identified the cluster provider containing the updated cluster.
  * @param clusterId id of the cluster to redraw.
  * @param active true if the view is the active one, false otherwise.
  */
    void  clusterColorUpdate(const QColor &color, const QString &name,int clusterId,bool active){view.clusterColorUpdate(color, name,clusterId,active);}

    /**Changes the color of a event.
  * @param name name use to identified the event provider containing the updated event.
  * @param eventId id of the event to redraw.
  * @param active true if the view is the active one, false otherwise.
  */
    void  eventColorUpdate(const QColor &color, const QString &name,int eventId,bool active){view.eventColorUpdate(color, name,eventId,active);}

    /**Changes the color of a group of channels.
  * @param groupId id of the group for which the color have been changed.
  * @param active true if the view is the active one, false otherwise.
  */
    void groupColorUpdate(int groupId,bool active){view.groupColorUpdate(groupId,active);}

    /**Triggers the increase of the amplitude of all the channels.
  */
    void increaseAllChannelsAmplitude(){view.increaseAllAmplitude();}

    /**Triggers the decrease of the amplitude of all the channels.
  */
    void decreaseAllChannelsAmplitude(){view.decreaseAllAmplitude();}

    /**Triggers the increase of the amplitude of the selected channels.
  * @param channelIds ids of the channels for which the amplitude has to be increased.
  */
    void increaseSelectedChannelsAmplitude(const QList<int>& channelIds){view.increaseSelectedChannelsAmplitude(channelIds);}

    /**Triggers the decrease of the amplitude of the selected channels.
  * @param channelIds ids of the channels for which the amplitude has to be decreased.
  */
    void decreaseSelectedChannelsAmplitude(const QList<int>& channelIds){view.decreaseSelectedChannelsAmplitude(channelIds);}

    /**Sets the unit gain and the acquisition system gain.
  * @param gain initial gain use to draw the traces in the TraceView.
  * @param acquisitionGain acquisition gain.
  */
    void setGains(int gain,int acquisitionGain){view.setGains(gain,acquisitionGain);}

    /**Update the information presented in the view if need it.*/
    void updateDrawing(){view.updateDrawing();}

    /**Update the TraceView.*/
    void updateContents(){view.update();}

    /**Triggers the update of the display due to a change in the display groups.
  * @param active true if the view is the active one, false otherwise.
  */
    void groupsModified(bool active){view.groupsModified(active);}

    /**Selects the channels in the TraceView.
  *@param selectedIds ids of the selected channels.
  */
    void selectChannels(const QList<int>& selectedIds);

    /**Resets the offset of the selected channels to the default values.
  * @param selectedChannelDefaultOffsets map given the default offsets for the selected channels.
  */
    void resetOffsets(const QMap<int,int>& selectedChannelDefaultOffsets){view.resetOffsets(selectedChannelDefaultOffsets);}

    /**Resets the gain of the selected channels.
  *@param selectedChannels ids of the selected channels.
  */
    void resetGains(const QList<int>& selectedChannels){view.resetGains(selectedChannels);}

    /**Informs the view to update the traces for the current time frame.*/
    void drawTraces(){view.displayTimeFrame(startTime,timeWindow);}

    /**Informs the view to reset its state.*/
    void reset(){view.reset();}

    /**Informs the view to enable or disable automatic channel centering around offsets.
  * @param show true if the autocentering is on, false otherwise.
  */
    void setAutocenterChannels(bool status);

    /**Informs the view to show or hide the labels display next to the traces.
  * @param show true if the labels have to be shown, false otherwise.
  */
    void showLabels(bool show);

    /**Displays the traces starting at the given time @p time in miliseconds and for the given duration @p duration.
  * The selection widgets are updated accordingly.
  * @param time starting time to display the traces.
  * @param duration amount of time for which to display the traces.
  */
    void slotSetStartAndDuration(long time,long duration);

    /** Shows or hides the calibration bar. This bar is meaningful only when all the channels
  *  have the same amplification.
  * @param show true if the bar has to be shown false otherwise.
  * @param active true if the view is the active one, false otherwise.
  */
    void showCalibration(bool show,bool active){view.showCalibration(show,active);}

    /**
  * Updates the different time selection widgets due to a modification of the sampling rate for the document.
  * Informs the view to update the traces.
  * @param length the newly computed length of the document.
  */
    void samplingRateModified(qlonglong length);

    /**Adds a new provider of cluster data.
  * @param clustersProvider provider of cluster data.
  * @param name name use to identified the cluster provider.
  * @param clusterColors list of colors for the clusters.
  * @param active true if the view is the active one, false otherwise.
  * @param clustersToShow list of clusters to be shown.
  * @param displayGroupsClusterFile map between the anatomatical groups and the cluster files.
  * @param channelsSpikeGroups map between the channel ids and the spike group ids.
  * @param nbSamplesBefore number of samples before the sample of the peak are contained in the waveform of a spike.
  * @param nbSamplesAfter number of samples after the sample of the peak are contained in the waveform of a spike.
  * @param clustersToSkip list of clusters to not use while browsing.
  */
    void addClusterProvider(ClustersProvider* clustersProvider,const QString &name,ItemColors* clusterColors,bool active,
                                   QList<int>& clustersToShow,QMap<int, QList<int> >* displayGroupsClusterFile,
                                   QMap<int,int>* channelsSpikeGroups,int nbSamplesBefore,int nbSamplesAfter,const QList<int>& clustersToSkip){
        view.addClusterProvider(clustersProvider,name,clusterColors,active,clustersToShow,displayGroupsClusterFile,
                                channelsSpikeGroups,nbSamplesBefore,nbSamplesAfter,clustersToSkip);
    }

    /**Removes a provider of cluster data.
  * @param name name use to identified the cluster provider.
  * @param active true if the view is the active one, false otherwise.
  */
    void removeClusterProvider(const QString& name,bool active){
        view.removeClusterProvider(name,active);
    }

    /**Adds a new provider of event data.
  * @param eventsProvider provider of event data.
  * @param name name use to identified the event provider.
  * @param eventColors list of colors for the events.
  * @param active true if the view is the active one, false otherwise.
  * @param eventsToShow list of clusters to be shown.
  * @param eventsToNotBrowse list of events to not use while browsing.
  */
    void addEventProvider(EventsProvider* eventsProvider,const QString &name,ItemColors* eventColors,bool active,
                                 QList<int>& eventsToShow,const QList<int>& eventsToNotBrowse){
        view.addEventProvider(eventsProvider,name,eventColors,active,eventsToShow,eventsToNotBrowse);
    }


    /**Removes a provider of event data.
  * @param name name use to identified the event provider.
  * @param active true if the view is the active one, false otherwise.
  */
    void removeEventProvider(const QString& name,bool active){
        view.removeEventProvider(name,active);
    }

    /**Prints the current information drawn in the traceView.
  * @param printPainter painter on a printer.
  * @param metrics object providing information about the printer.
  * @param filePath path of the opened document.
  * @param whiteBackground true if the printed background has to be white, false otherwise.
  */
    void print(QPainter& printPainter,int width,int height,const QString &filePath,bool whiteBackground){

        QRect textRec = QRect(printPainter.viewport().left() + 5 ,printPainter.viewport().height() - 20,printPainter.viewport().width() - 5,20);
        QFont f("Helvetica",8);
        printPainter.setFont(f);
        printPainter.setPen(Qt::black);
        printPainter.drawText(textRec,Qt::AlignLeft | Qt::AlignVCenter,
                              tr("File: %1     Start time: %2 min %3 s %4 ms, Duration: %5 ms").arg(filePath).arg(startMinute->value()).arg(startSecond->value()).arg(startMilisecond->value()).arg(duration->displayText()));

        //Modify the viewport so the view will not draw on the legend
        QRect newViewport = QRect(printPainter.viewport().left(),printPainter.viewport().top(),printPainter.viewport().width(),printPainter.viewport().height());
        newViewport.setBottom(printPainter.viewport().bottom() - 20);
        printPainter.setViewport(newViewport);
        view.print(printPainter,width,height,whiteBackground);
        printPainter.resetMatrix();
    }

    /**Retrieves the next cluster.*/
    void showNextCluster(){view.showNextCluster();}

    /**Retrieves the previous cluster.*/
    void showPreviousCluster(){view.showPreviousCluster();}

    /**Retrieves the next event.*/
    void showNextEvent(){view.showNextEvent();}

    /**Retrieves the previous event.*/
    void showPreviousEvent(){view.showPreviousEvent();}

    /**Informs that an event has been modified.
  * @param providerName name use to identified the event provider containing the modified event.
  * @param selectedEventId id of the modified event.
  * @param time initial time of the modified event.
  * @param newTime new time of the modified event.
  */
    void slotEventModified(const QString &providerName,int selectedEventId,double time,double newTime){
        emit eventModified(providerName,selectedEventId,time,newTime);
    }

    /**Informs that an event has been removed.
  * @param providerName name use to identified the event provider containing the removed event.
  * @param selectedEventId id of the removed event.
  * @param time initial time of the removed event.
  */
    void slotEventRemoved(const QString& providerName,int selectedEventId,double time){
        emit eventRemoved(providerName,selectedEventId,time);
    }

    /**Updates the traceView if concerned by the modification of the event.
  * @param providerName name use to identified the event provider containing the modified event.
  * @param time initial time of the modified event.
  * @param newTime new time of the modified event.
  * @param active true if the view is the active one, false otherwise.
  */
    void updateEvents(bool active,const QString& providerName,double time,double newTime){
        long eventTime = static_cast<long>(floor(0.5 + time));
        long newEventTime = static_cast<long>(floor(0.5 + newTime));
        if((eventTime >= startTime  && eventTime <= (startTime + timeWindow)) ||
                (newEventTime >= startTime  && newEventTime <= (startTime + timeWindow))) view.updateEvents(providerName,active);
    }

    /**Deletes the selected event.
  */
    void removeEvent(){view.removeEvent();}

    /**Updates the traceView if concerned by the modification of the event.
  * @param providerName name use to identified the event provider containing the removed event.
  * @param time initial time of the removed event.
  * @param active true if the view is the active one, false otherwise.
  */
    void updateEvents(bool active,const QString& providerName,double time);

    /**Informs the traceView of the properties for the next event to be added.
  * @param providerName name use to identified the event provider which will contain the added event.
  * @param eventDescription description of the next event to be created.
  */
    void eventToAddProperties(const QString& providerName,const QString& eventDescription){
        view.eventToAddProperties(providerName,eventDescription);
    }

    /**Informs that an event has been added.
  * @param providerName name use to identified the event provider containing the added event.
  * @param addedEventDescription description of the added event.
  * @param time time of the added event.
  */
    void slotEventAdded(const QString &providerName,const QString& addedEventDescription,double time);

    /**Updates the event data provided by @p providerName due to the addition of an event.
  * @param providerName name use to identified the event provider containing the modified event.
  * @param eventsToShow new list of events to be shown.
  * @param active true if the view is the active one, false otherwise.
  */
    void updateEvents(const QString& providerName,QList<int>& eventsToShow,bool active);

    /** Updates the description of a spike waveform.
  * @param nbSamplesBefore number of samples contained in the waveform of a spike before the sample of the peak.
  * @param nbSamplesAfter number of samples contained in the waveform of a spike after the sample of the peak.
  * @param active true if the view is the active one, false otherwise.
  */
    void updateWaveformInformation(int nbSamplesBefore, int nbSamplesAfter,bool active){
        view.updateWaveformInformation(nbSamplesBefore,nbSamplesAfter,active);
    }

    /**Updates the cluster information presented on the display.
  * @param active true if the view is the active one, false otherwise.
  */
    void updateClusterData(bool active){view.updateClusterData(active);}

    /**Updates the list of events to not use while browsing.
  * @param providerName name use to identified the event provider containing the modified event.
  * @param eventsToNotBrowse new list of events to not use while browsing.
  */
    void updateNoneBrowsingEventList(const QString &providerName,const QList<int>& eventsToNotBrowse){
        view.updateNoneBrowsingEventList(providerName,eventsToNotBrowse);
    }

    /**Updates the list of clusters to not use while browsing.
  * @param providerName name use to identified the event provider containing the modified event.
  * @param clustersToNotBrowse new list of clusters to not use while browsing.
  */
    void updateNoneBrowsingClusterList(const QString &providerName,const QList<int>& clustersToNotBrowse){
        view.updateNoneBrowsingClusterList(providerName,clustersToNotBrowse);
    }

    /**If the frame is contained in a dockWidget, this slot can be used
  * when the enclosing dockwidget is being closed.
  * Emits the parentDockBeingClosed signal.
  */
    virtual void dockBeingClosed(){emit parentDockBeingClosed(this);}

    /**Updates the display due to modification of clusters provided by the cluster provider identified
  * by @p name.
  * @param name name use to identified the cluster provider containing the clusters to show.
  * @param clustersToShow new list of clusters to be shown.
  * @param clusterColors list of colors for the clusters.
  * @param active true if the view is the active one, false otherwise.
  */
    void updateClusters(QString name,QList<int>& clustersToShow,ItemColors* clusterColors,bool active){
        view.updateClusters(name,clustersToShow,clusterColors,active);
    }

    /**Updates the list of skipped channels.
  * @param skippedChannels list of skipped channels.
  **/
    void updateSkipStatus(const QList<int>& skippedChannels){view.skipStatusChanged(skippedChannels);}

    /**Increases the height of the rasters.*/
    void increaseRasterHeight(){view.increaseRatio();}

    /**Decreases the height of the rasters.*/
    void decreaseRasterHeight() {view.decreaseRatio();}

    /**Returns the height of the rasters.
  *@return raster height.
  */
    int getRasterHeight() {return view.getRasterHeight();}

    /**Informs that data of the selected events providers corresponding to current time frame are available.
  * @param eventsData dictionary between the event provider names and the event data and status.
  * @param selectedEvents map between the event provider names and the list of currently selected events.
  * @param providerItemColors dictionary between the provider names and the item color lists.
  * @param initiator instance requesting the data.
  * @param samplingRate sampling rate of the current open data file in Hz.
  */
    void slotEventsAvailable(QHash<QString, EventData*>& eventsData,QMap<QString, QList<int> >& selectedEvents,QHash<QString, ItemColors*>& providerItemColors,QObject* initiator,double samplingRate) {
        emit eventsAvailable(eventsData,selectedEvents,providerItemColors,initiator,samplingRate);
    }

    /**Gets the event information for all the available eventProviders.
  * @param startTime begining of the time interval from which to retrieve the data in miliseconds.
  * @param endTime end of the time interval from which to retrieve the data.
  * @param initiator instance requesting the data.
  */
    void getCurrentEventInformation(long startTime,long endTime,QObject* initiator){view.getCurrentEventInformation(startTime,endTime,initiator);}

    /**Updates the background image.
  * @param traceBackgroundImage image to be used as background.
  * @param active true if the view is the active one, false otherwise.
  */
    void traceBackgroundImageUpdate(const QImage &traceBackgroundImage,bool active){
        view.traceBackgroundImageUpdate(traceBackgroundImage,active);
    }

Q_SIGNALS:
    void channelsSelected(const QList<int>& selectedIds);

    /**Informs that the starting time and/or the timeWindow have changed.
  * @param startTime starting time.
  * @param timeWindow time window.
  */
    void updateStartAndDuration(long startTime,long timeWindow);

    /**Informs that an event has been modified.
  * @param providerName name use to identified the event provider containing the modified event.
  * @param selectedEventId id of the modified event.
  * @param time initial time of the modified event.
  * @param newTime new time of the modified event.
  */
    void eventModified(const QString &providerName,int selectedEventId,double time,double newTime);

    /**Informs that an event has been removed.
  * @param providerName name use to identified the event provider containing the removed event.
  * @param selectedEventId id of the removed event.
  * @param time initial time of the removed event.
  */
    void eventRemoved(const QString &providerName,int selectedEventId,double time);

    /**Informs that an event has been added.
  * @param providerName name use to identified the event provider containing the added event.
  * @param addedEventDescription description of the added event.
  * @param time time of the added event.
  */
    void eventAdded(const QString &providerName,const QString &addedEventDescription,double time);

    /**Signals that the enclosing dockwidget is being closed
  * @param viewWidget pointer on the the current object.
  */
    void parentDockBeingClosed(QWidget* viewWidget);

    /**Informs that data of the selected events providers corresponding to current time frame are available.
  * @param eventsData dictionary between the event provider names and the event data and status.
  * @param selectedEvents map between the event provider names and the list of currently selected events.
  * @param providerItemColors dictionary between the provider names and the item color lists.
  * @param initiator instance requesting the data.
  * @param samplingRate sampling rate of the current open data file  in Hz.
  */
    void eventsAvailable(QHash<QString, EventData*>& eventsData,QMap<QString, QList<int> >& selectedEvents,QHash<QString, ItemColors*>& providerItemColors,QObject* initiator,double samplingRate);

protected:

    /**The view responds to a key press event.
  * @param event key press event.
  */
    void keyPressEvent(QKeyEvent* event);

private:
    //Functions

    /**Initializes the different time selection widgets.*/
    void initSelectionWidgets();

private Q_SLOTS:
    /**Informs the view to present the traces for an updated time frame.*/
    void slotStartMinuteTimeUpdated();

    /**Informs the view to present the traces for an updated time frame.*/
    void slotStartSecondTimeUpdated(/*int start*/);

    /**Informs the view to present the traces for an updated time frame.*/
    void slotStartMilisecondTimeUpdated(/*int start*/);

    /**Update the selection widgets and informs view to present the traces for an updated time frame.*/
    void slotDurationUpdated();

    /**Update the selection widgets and informs view to present the traces for an updated time frame.*/
    void slotScrollBarUpdated();

private:
    /// Added by M.Zugaro to enable automatic forward paging
    QTimer	*timer;
    int		pageTime;

    /**Amount of time used when looking for the traces.
  * This amount is in miliseconds and the default is 1000.
  */
    long timeWindow;

    /**TraceView used to actually display the traces.*/
    TraceView view;

    /**Starting time when looking for the traces.
  * This amount is in miliseconds and the default is 0.
  */
    long startTime;

    /**Spin box enabling to choose the start time in minutes used to display the traces.
  */
    QSpinBox* startMinute;

    /**Spin box enabling to choose the start time in seconds used to display the traces.
  */
    QSpinBox* startSecond;

    /**Spin box enabling to choose the start time in miliseconds used to display the traces.
  */
    QSpinBox* startMilisecond;

    /**Small box where the user can enter the width of the time frame to use
  * to display the traces.*/
    QLineEdit* duration;

    static const QString INITIAL_TRACES_TIME_WINDOW;

    QIntValidator validator;

    QLabel* durationLabel;
    QLabel* startLabel;

    /**Boolean used to prevent the trigger of changes during initialization.*/
    bool isInit;

    QScrollBar* scrollBar;

    /**Length of the recording in miliseconds.*/
    qlonglong recordingLength;

    /**ScrollBar line step size in miliseconds.*/
    long lineStep;

    /**ScrollBar page step size in miliseconds.*/
    long pageStep;

    /**Container for the selection widget.*/
    QWidget* selectionWidgets;

    /**When converting the recording length in minutes, seconds and miliseconds this represents the number of minutes.*/
    int minutePart;

    /**When converting the recording length in minutes, seconds and miliseconds this represents the number of seconds.*/
    int secondPart;

    /**When converting the recording length in minutes, seconds and miliseconds this represents the number of miliseconds.*/
    int milisecondPart;

    /**True if the TraceView has to be updated, false otherwise.*/
    bool updateView;

    /**Pointer to the status bar of the application.*/
    QStatusBar* statusBar;

    //////////Functions/////////////

    /**
  * Corrects the new start time which became inferior to the recordingLength due to a change in one of the selection widgets.
  */
    void correctStartTime();

};


#endif
