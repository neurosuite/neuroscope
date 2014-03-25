/***************************************************************************
                          traceview.h  -  description
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

#ifndef TRACEVIEW_H
#define TRACEVIEW_H

// include files for QT
#include <QWidget>
#include <QPixmap>
#include <QHash>
#include <QPair>
#include <QImage>
#include <QDebug>

#include <QList>
#include <QResizeEvent>
#include <QMouseEvent>

//include files for the application
#include "baseframe.h"
#include "tracesprovider.h"
#include "eventdata.h"

#include <QStatusBar>

//include files for c/c++ libraries
#include <math.h>

// forward declaration of the Neuroscope classes
class ChannelColors;
class ItemColors;
class ClustersProvider;
class EventsProvider;

/**
  *Class which draws the traces, cluster and event representations.
  *@author Lynn Hazan
  */

class TraceView : public BaseFrame  {
    Q_OBJECT
public:
    /**Constructor.
  * @param tracesProvider a reference on the provider of the channels data.
  * @param greyScale true if all the channels are display either in a gradation of grey false if they are display in color.
  * @param multiColumns true if the traces are displayed on multiple columns, false otherwise.
  * @param verticalLines true if vertical lines are displayed to show the clusters, false otherwise.
  * @param raster true if a raster is drawn to show the clusters, false otherwise
  * @param waveforms true if waveforms are drawn on top of the traces, false otherwise.
  * @param labelsDisplay true if labels are drawn next to the traces, false otherwise.
  * @param channelsToDisplay a reference on the list of channel to be shown at the opening of the view.
  * @param unitGain initial gain use to draw the traces.
  * @param acquisitionGain acquisition gain.
  * @param start starting time in miliseconds.
  * @param timeFrameWidth time window in miliseconds.
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
    TraceView(TracesProvider& tracesProvider, bool greyScale, bool multiColumns, bool verticalLines,
              bool raster, bool waveforms, bool labelsDisplay, QList<int>& channelsToDisplay, int unitGain, int acquisitionGain, long start, long timeFrameWidth,
              ChannelColors* channelColors, QMap<int, QList<int> >* groupsChannels, QMap<int,int>* channelsGroups,
              bool autocenterChannels, QList<int>& channelOffsets, QList<int>& gains, const QList<int>& skippedChannels, int rasterHeight, const QImage &backgroundImage, QWidget* parent=0, const char* name=0, const QColor& backgroundColor = Qt::black, QStatusBar* statusBar = 0L,
              int minSize = 500, int maxSize = 4000, int windowTopLeft = -500, int windowBottomRight = 1001, int border = 0);



    ~TraceView();

    /// Added by M.Zugaro to enable automatic forward paging
    qlonglong recordingLength() const { return length; }
    void updateRecordingLength() { tracesProvider.updateRecordingLength();length = tracesProvider.recordingLength(); }

    /**Enum to be use as a Mode.
  * <ul>
  * <li>SELECT Enumeration indicating that the user is in a mode enabling him to select traces.</li>
  * </ul>
  */
    enum {SELECT = ZOOM+1,MEASURE = ZOOM+2,SELECT_TIME = ZOOM+3,SELECT_EVENT = ZOOM+4,ADD_EVENT = ZOOM+5,DRAW_LINE = ZOOM+6};


    /**
  * Sets the mode of presentation to single or multiple columns.
  * @param multiple true if the traces are presented on multiple columns, false if they are presented on a single column.
  */
    void setMultiColumns(bool multiple);

    /**Displays or hides vertical lines to show the clusters.
  * @param lines true if the vertical lines are drawn for each cluster, false otherwise.
  */
    void setClusterVerticalLines(bool lines);

    /**Displays or hides a raster to show the clusters.
  * @param raster true if a raster is drawn, false otherwise.
  */
    void setClusterRaster(bool raster);

    /**Displays or hides the cluster waveforms on top of the traces.
  * @param waveforms true if the waveforms are drawn, false otherwise.
  */
    void setClusterWaveforms(bool waveforms);


    /**All the channels of the display are now display either in a gradation of grey or in color.
   * @param grey true if the channels have to be displayed in grey false otherwise.
   */
    void setGreyScale(bool grey);

    /**Updates the traces to show between @p start and @p start + @p timeFrameWidth.
   * @param start starting time in miliseconds.
   * @param timeFrameWidth time window in miliseconds.
   */
    void displayTimeFrame(long start,long timeFrameWidth);

    /**
  * Updates the list of channels shown with @p channelsToShow.
  * @param channelsToShow new list of channels to be shown.
  */
    void showChannels(const QList<int>& channelsToShow);

    /**Increases of the amplitude of all the channels.
  */
    void increaseAllAmplitude();

    /**Decreases of the amplitude of all the channels.
  */
    void decreaseAllAmplitude();

    /**Increases of the amplitude of the selected channels.
  * @param channelIds ids of the channels for which the amplitude has to be increased.
  */
    void increaseSelectedChannelsAmplitude(const QList<int>& channelIds);

    /**Decreases of the amplitude of the selected channels.
  * @param channelIds ids of the channels for which the amplitude has to be decreased.
  */
    void decreaseSelectedChannelsAmplitude(const QList<int>& channelIds);

    /**Sets the unit gain and the acquisition system gain.
  * @param gain initial gain use to draw the traces in the TraceView.
  * @param acquisitionGain acquisition gain.
  */
    void setGains(int gain,int acquisitionGain);

    /**Changes the color of a channel.
  * @param channelId id of the channel to redraw.
  * @param active true if the view is the active one, false otherwise.
  */
    void channelColorUpdate(int channelId,bool active);

    /**Changes the color of a group of channels.
  * @param groupId id of the group for which the color have been changed.
  * @param active true if the view is the active one, false otherwise.
  */
    void groupColorUpdate(int groupId,bool active);

    /**Update the information presented in the view.*/
    void updateDrawing();

    /**Updates of the display due to a change in the display groups if @p active is true,
  * otherwise simply updates the internal variables.
  * @param active true if the view is the active one, false otherwise.
  */
    void groupsModified(bool active);

    /**Change the current mode, call by a selection of a tool.
  * @param selectedMode new mode of drawing.
  * @param active true if the view is the active one, false otherwise.
  */
    void setMode(BaseFrame::Mode selectedMode,bool active);

    /**Selects the channels .
  *@param selectedIds ids of the selected channels.
  */
    void selectChannels(const QList<int>& selectedIds);

    /**Resets the offset of the selected channels to the default values.
  * @param selectedChannelDefaultOffsets map given the default offsets for the selected channels.
  */
    void resetOffsets(const QMap<int,int>& selectedChannelDefaultOffsets);

    /**Resets the gain of the selected channels.
  *@param selectedChannels ids of the selected channels.
  */
    void resetGains(const QList<int>& selectedChannels);

    /**Resets the state of the view.*/
    void reset();

    /**Enables or disables automatic channel centering around offsets.
  * @param show true if the autocentering is on, false otherwise.
  */
    void setAutocenterChannels(bool status);

    /**Shows or hides the labels next to the channels displaying the channel id and the gain.
  * @param show true if the labels have to be shown, false otherwise.
  */
    void showHideLabels(bool show);

    /**Shows or hide the calibration scale.
  * @param show true if the scale has to be shown, false otherwise.
  * @param active true if the view is the active one, false otherwise.
  */
    void showCalibration(bool show,bool active){
        drawContentsMode = REDRAW;
        showCalibrationScale = show;
        if(active)
            update();
    }

    /**Adds a new provider of cluster data.
  * @param clustersProvider provider of cluster data.
  * @param name name use to identified the cluster provider.
  * @param clusterColors list of colors for the clusters.
  * @param active true if the view is the active one, false otherwise.
  * @param clustersToShow list of clusters to be shown.
  * @param displayGroupsClusterFile map between the anatomatical groups and the cluster files.
  * @param channelsSpikeGroups map between the channel ids and the spike group ids.
  * @param nbSamplesBefore number of samples contained in the waveform of a spike before the sample of the peak.
  * @param nbSamplesAfter number of samples contained in the waveform of a spike after the sample of the peak.
  * @param clustersToSkip list of clusters to not use while browsing.
  */
    void addClusterProvider(ClustersProvider* clustersProvider,QString name,ItemColors* clusterColors,
                            bool active,QList<int>& clustersToShow,QMap<int, QList<int> >* displayGroupsClusterFile,
                            QMap<int,int>* channelsSpikeGroups,int nbSamplesBefore,int nbSamplesAfter,const QList<int>& clustersToSkip);

    /**Removes a provider of cluster data.
  * @param name name use to identified the cluster provider.
  * @param active true if the view is the active one, false otherwise.
  */
    void removeClusterProvider(const QString& name,bool active);

    /**
  * Updates the list of clusters shown with @p clustersToShow for the cluster provider identified
  * by @p name.
  * @param name name use to identified the cluster provider containing the clusters to show.
  * @param clustersToShow new list of clusters to be shown.
  */
    void showClusters(const QString& name,const QList<int>& clustersToShow);

    /**Changes the color of a cluster.
  * @param name name use to identified the cluster provider containing the updated cluster.
  * @param clusterId id of the cluster to redraw.
  * @param active true if the view is the active one, false otherwise.
  */
    void clusterColorUpdate(const QColor &color, const QString &name, int clusterId, bool active);


    /**Adds a new provider of event data.
  * @param eventsProvider provider of event data.
  * @param name name use to identified the event provider.
  * @param eventColors list of colors for the events.
  * @param active true if the view is the active one, false otherwise.
  * @param eventsToShow list of events to be shown.
  * @param eventsToSkip list of events to not use while browsing.
  */
    void addEventProvider(EventsProvider* eventsProvider,QString name,ItemColors* eventColors,
                          bool active,QList<int>& eventsToShow,const QList<int>& eventsToSkip);

    /**Removes a provider of event data.
  * @param name name use to identified the event provider.
  * @param active true if the view is the active one, false otherwise.
  */
    void removeEventProvider(const QString &name, bool active);

    /**updateEvents
  * Updates the list of events shown with @p eventsToShow for the event provider identified
  * by @p name.
  * @param name name use to identified the event provider containing the events to show.
  * @param eventsToShow new list of events to be shown.
  */
    void showEvents(const QString &name, QList<int>& eventsToShow);

    /**Changes the color of an event.
  * @param name name use to identified the event provider containing the updated event.
  * @param eventId id of the event to redraw.
  * @param active true if the view is the active one, false otherwise.
  */
    void eventColorUpdate(const QColor &color, const QString &name, int eventId, bool active);

    /**Prints the currently display information on a printer via the painter @p printPainter.
  * Does not print zoomed display.
  * @param printPainter painter on a printer.
  * @param metrics object providing informatin about the printer.
  * @param whiteBackground true if the printed background has to be white, false otherwise.
  */
    void print(QPainter& printPainter,int width, int height,bool whiteBackground);

    /**Retrieves the next event.*/
    void showNextEvent();

    /**Retrieves the previous event.*/
    void showPreviousEvent();

    /**Updates the event data provided by @p providerName due to a modification of an event.
  * @param providerName name use to identified the event provider containing the modified event.
  * @param active true if the view is the active one, false otherwise.
  */
    void updateEvents(const QString& providerName,bool active){
        if(!eventProvidersToUpdate.contains(providerName))
            eventProvidersToUpdate.append(providerName);
        if(active)
            update();
    }

    /**Updates the event data provided by @p providerName due to the addition of an event.
  * @param providerName name use to identified the event provider containing the modified event.
  * @param eventsToShow new list of events to be shown.
  * @param active true if the view is the active one, false otherwise.
  */
    void updateEvents(const QString &providerName, QList<int>& eventsToShow, bool active);

    /**Deletes the selected event.
  */
    void removeEvent();

    /**Retrieves the next cluster.*/
    void showNextCluster();

    /**Retrieves the previous cluster.*/
    void showPreviousCluster();

    /**Updates the list of events to not use while browsing.
  * @param providerName name use to identified the event provider containing the modified event.
  * @param eventsToNotBrowse new list of events to not use while browsing.
  */
    void updateNoneBrowsingEventList(const QString &providerName, const QList<int>& eventsToNotBrowse);

    /**Updates the list of clusters to not use while browsing.
  * @param providerName name use to identified the event provider containing the modified event.
  * @param clustersToNotBrowse new list of clusters to not use while browsing.
  */
    void updateNoneBrowsingClusterList(const QString &providerName, const QList<int>& clustersToNotBrowse);

    /** Updates the description of a spike waveform.
  * @param nbSamplesBefore number of samples contained in the waveform of a spike before the sample of the peak.
  * @param nbSamplesAfter number of samples contained in the waveform of a spike after the sample of the peak.
  * @param active true if the view is the active one, false otherwise.
  */
    void updateWaveformInformation(int nbSamplesBefore,int nbSamplesAfter,bool active);

    /**
  * Updates the length of the document due to a modification of the sampling rate.
  * @param length the newly computed length of the document.
  */
    void samplingRateModified(qlonglong length){
        this->length = length;
        int samplingRate = tracesProvider.getSamplingRate();
        timeStepUnit = timeStep = static_cast<float>(static_cast<float>(1000) / static_cast<float>(samplingRate));
    }

    /**Updates the cluster information presented on the display.
  * @param active true if the view is the active one, false otherwise.
  */
    void updateClusterData(bool active);

    /**Updates the display due to modification of clusters provided by the cluster provider identified
  * by @p name.
  * @param name name use to identified the cluster provider containing the clusters to show.
  * @param clustersToShow new list of clusters to be shown.
  * @param clusterColors list of colors for the clusters.
  * @param active true if the view is the active one, false otherwise.
  */
    void updateClusters(QString name,QList<int>& clustersToShow,ItemColors* clusterColors,bool active);

    /**Increases the display ratio between the traces and the rasters when they are present.*/
    void increaseRatio();

    /**Decreases the display ratio between the traces and the rasters when they are present.*/
    void decreaseRatio();

    /**Gets the event information for all the available eventProviders.
  * @param startTime begining of the time interval from which to retrieve the data in miliseconds.
  * @param endTime end of the time interval from which to retrieve the data.
  * @param initiator instance requesting the data.
  */
    void getCurrentEventInformation(long startTime,long endTime,QObject* initiator);

    /**Updates the background image.
  * @param traceBackgroundImage image to be used as background.
  * @param active true if the view is the active one, false otherwise.
  */
    void traceBackgroundImageUpdate(QImage traceBackgroundImage,bool active);

public Q_SLOTS:
    /**Displays the data that has been retrieved.
  * @param data array of data (number of channels X number of samples).
  * @param initiator instance requesting the data.
  */
    void dataAvailable(Array<dataType>& data,QObject* initiator);

    /**Displays the cluster information that has been retrieved.
  * @param data 2 line array containing the sample index of the peak index of each spike existing in the requested time frame with the
  * corresponding cluster id. The first line contains the sample index and the second line the cluster id.
  * @param initiator instance requesting the data.
  * @param providerName name of the instance providing the data.
  */
    void dataAvailable(Array<dataType>& data, QObject* initiator, const QString &providerName);

    /**Displays the event information that has been retrieved.
  * @param times 1 line array containing the time (in recording samples) of each event existing in the requested time frame.
  * @param ids 1 line array containing the identifiers of each event existing in the requested time frame.
  * @param initiator instance requesting the data.
  * @param providerName name of the instance providing the data.
  */
    void dataAvailable(Array<dataType>& times, Array<int>& ids, QObject* initiator, const QString &providerName);

    /**Compute the cluster information that has been retrieved regarding the next cluster to display.
  * @param data 2 line array containing the sample index of the peak index of each spike existing in the requested time frame with the
  * corresponding cluster id. The first line contains the sample index and the second line the cluster id.
  * @param initiator instance requesting the data.
  * @param providerName name of the instance providing the data.
  * @param startingTime time from which the data have been retreived in milisesonds.
  * @param startingTimeInRecordingUnits time from which the data have been retrieved in recording units.
  */
    void nextClusterDataAvailable(Array<dataType>& data,QObject* initiator,QString providerName,long startingTime,long startingTimeInRecordingUnits);

    /**Compute the cluster information that has been retrieved regarding the previous cluster to display.
  * @param data 2 line array containing the sample index of the peak index of each spike existing in the requested time frame with the
  * corresponding cluster id. The first line contains the sample index and the second line the cluster id.
  * @param initiator instance requesting the data.
  * @param providerName name of the instance providing the data.
  * @param startingTime time from which the data have been retreived in milisesonds.
  * @param startingTimeInRecordingUnits time from which the data have been retrieved in recording units.
  */
    void previousClusterDataAvailable(Array<dataType>& data,QObject* initiator,QString providerName,long startingTime,long startingTimeInRecordingUnits);

    /**Compute the event information that has been retrieved regarding the next event to display.
  * @param times 1 line array containing the time (in recording samples) of each event existing in the requested time frame.
  * @param ids 1 line array containing the identifiers of each event existing in the requested time frame.
  * @param initiator instance requesting the data.
  * @param providerName name of the instance providing the data.
  * @param startingTime time from which the data have been retreived.
  */
    void nextEventDataAvailable(Array<dataType>& times,Array<int>& ids,QObject* initiator,QString providerName,long startingTime);

    /**Compute the event information that has been retrieved regarding the previous event to display.
  * @param times 1 line array containing the time (in recording samples) of each event existing in the requested time frame.
  * @param ids 1 line array containing the identifiers of each event existing in the requested time frame.
  * @param initiator instance requesting the data.
  * @param providerName name of the instance providing the data.
  * @param startingTime time from which the data have been retreived.
  */
    void previousEventDataAvailable(Array<dataType>& times,Array<int>& ids,QObject* initiator,QString providerName,long startingTime);

    /**Stores the properties for the next event to be added.
  * @param providerName name use to identified the event provider which will contain the added event.
  * @param eventDescription description of the next event to be created.
  */
    void eventToAddProperties(const QString &providerName, const QString &eventDescription){
qDebug()<<" eventToAddProperties***********************";
        //If an event is being modified, this function can be called with eventDescription set to empty,
        //this should not be taken into account.
        if(!eventBeingModified){
            eventDescriptionToCreate = eventDescription;
            eventProvider = providerName;
        }
    }

    /**Updates the list of skipped channels.
  * @param skippedChannels list of skipped channels
  **/
    void skipStatusChanged(const QList<int>& skippedChannels);

    /**Returns the height of the rasters in the world coordinate system.
  *@return raster height.
  */
    int getRasterHeight(){return rasterHeight;}


Q_SIGNALS:
    void channelsSelected(const QList<int>& selectedIds);
    void setStartAndDuration(long time,long duration);
    void eventModified(QString providerName,int selectedEventId,double time,double newTime);
    void eventRemoved(QString providerName,int selectedEventId,double time);
    void eventAdded(QString providerName,QString addedEventDescription,double time);
    void eventsAvailable(QHash<QString, EventData*>& eventsData,QMap<QString, QList<int> >& selectedEvents,QHash<QString, ItemColors*>& providerItemColors,QObject* initiator,double samplingRate);

protected:
    /**
  * Draws the contents of the frame
  * @param painter painter used to draw the contents
  */
    void paintEvent ( QPaintEvent*ainter);

    /**The view responds to a mouse move event.
  * The time is display in the status bar.
  * @param event mouse move event.
  */
    void mouseMoveEvent(QMouseEvent* event);

    /**The view responds to a resize event.
  * The window is recomputed.
  * @param event resize event.
  */
    void resizeEvent(QResizeEvent* event);

    /**The view responds to a mouse click.
  * @param event mouse release event.
  */
    void mousePressEvent(QMouseEvent* event);

    /**The view responds to a mouse release.
  * @param event mouse event.
  */
    void mouseReleaseEvent(QMouseEvent* event);

    /**The view responds to a double click.
  * @param event mouse event.
  */
    void mouseDoubleClickEvent(QMouseEvent* event);

private:

    /**True if the the colors are in grey-scale*/
    bool greyScaleMode;

    /**Pointer to the status bar of the application.*/
    QStatusBar* statusBar;

    /**Provider of the channels data.*/
    TracesProvider& tracesProvider;

    /**True if the traces are displayed on multiple columns, false otherwise.*/
    bool multiColumns;

    /**True if vertical lines are displayed to show the clusters, false otherwise.*/
    bool verticalLines;

    /**True if a raster is drawn to show the clusters, false otherwise.*/
    bool raster;

    /**True if waveforms are drawn on top of the traces, false otherwise.*/
    bool waveforms;

    /**List of the presented channels.*/
    QList<int> shownChannels;

    /**True if the data information needed to draw the traces are available.*/
    bool dataReady;

    /**Array containing the traces data.*/
    Array<dataType> data;

    /**Autocenter channels.*/
    bool autocenterChannels;

    /**List containing the offset for each channel.*/
    QList<int>& channelOffsets;

    /**List of the factors use to calculate the ordinate value to been drawn.
  * The factor equals 0.75 raised to the power of the gain (Yworld = alpha.factor.Ydata).
  */
    QList<float> channelFactors;

    /**List of the exponents used to compute the drawing gain for each channel.
  * The actual gain is 0.75 raised to the power of gain.
  */
    QList<int>& gains;

    /**Size in pixels corresponding to the vertical space allocated to a trace.*/
    int traceVspace;

    /**Number of channels used to record the data.*/
    int nbChannels;

    /**Position of the peak among the points decribing waveforms.*/
    int peakPositionInWaveform;

    /**Number of points used to describe a waveform.*/
    int nbSamplesInWaveform;

    /**This variable keeps track of the current start time of the time window.*/
    long startTime;

    /**This variable keeps track of the current end time of the time window.*/
    long endTime;

    /**Pointer on the ChannelColors storing the color information for the channels.*/
    ChannelColors* channelColors;

    /**Map the correspondence between the channel group ids and the channel ids.
  *Pointer to the variable belonging to NeuroscopeDoc.
  */
    QMap<int, QList<int> >* groupsChannels;

    /**Stores to which group each channel belongs. Pointer to the variable belonging to
  NeuroscopeDoc.*/
    QMap<int,int>* channelsGroups;

    /**Map the correspondence between the channel group ids and the channel ids for the displayed channels.
  */
    QMap<int, QList<int> > shownGroupsChannels;

    /**Minimal abscissa in window coordinate*/
    long abscissaMin;

    /**Maximal abscissa in window coordinate*/
    long abscissaMax;

    /**Minimal ordinate in window coordinate*/
    long ordinateMin;

    /**Maximal ordinate in window coordinate*/
    long ordinateMax;

    /**The border on the left and right sides.*/
    int borderX;

    /**The border at the top and bottom.*/
    int borderY;

    /**The delta between the starting ordinates of two channels.*/
    int Yshift;

    /**The delta between the starting abcisses of two channels.*/
    int Xshift;

    /**Abscissa step between two points of a given trace.*/
    int Xstep;

    /**Abscissa space between two groups (when groups are display on several columns).*/
    int XGroupSpace;

    /**Ordinate space between two channels.*/
    int Yspace;

    /**Ordinate space between two groups (when groups are display on a single columns).*/
    int YGroupSpace;

    /**Ordinate space between the traces and the raster.*/
    int YTracesRasterSeparator;

    /**Ordinate space between two cluster trains.*/
    int YRasterSpace;

    /**The abscissa of the system coordinate center for the channel
  * which is presented at the top of the view.*/
    long X0;

    /**The ordinate of the system coordinate center for the channel
  * which is presented at the top of the view.*/
    long Y0;

    /**The ordinate for the first cluster raster.*/
    long Y0Raster;

    /**
  * Buffer to enable smooth updating, obtain flicker-free drawing.
  * Prevent from unnecessary redrawing.
  */
    QPixmap doublebuffer;

    /**Background image.*/
    QImage background;

    /**Scaled background pixmap.*/
    QPixmap scaledBackground;


    /**Time in milisseconds corresponding to a step between two points of a given trace.*/
    float timeStep;

    /**Time in milisseconds corresponding to a step between two samples.*/
    float timeStepUnit;

    /**Acquisition system gain in recording unit by milivolts.*/
    int acquisitionGain;

    /**Unit gain in recording unit by centimeters.*/
    int unitGain;

    /**Factor, in pixels by recording units, to convert the data in recording units to data
  * in pixels of the world (Yworld = alpha.factor.Ydata).
  * This factor is computed using the amplitude maximal of theta and the acquisition system gain.
  */
    float alpha;

    /**List of the gains display next to each drawn channel.
  */
    QList<float> channelDisplayGains;

    int nbClusters;

    /**Size in pixels corresponding to the height of a raster.*/
    int rasterHeight;

    /**Time amount, in milisecond, of the time frame used to display the traces.*/
    long timeFrameWidth;

    /**Stores the ordinate of the first sample of each channel.*/
    QMap<int,int> channelsStartingOrdinate;

    /**Stores the abscissa of the first sample of each channel.*/
    QMap<int,int> channelsStartingAbscissa;

    /**Default value for the border on the left and right sides inside the window (QRect corresponding
  * to the part of the drawing which will actually be drawn onto the widget).*/
    static const int XMARGIN;

    /**Default value for the border on the top and bottom sides inside the window (QRect corresponding
  * to the part of the drawing which will actually be drawn onto the widget).*/
    static const int YMARGIN;

    /**Border on the left and right sides inside the window (QRect corresponding
  * to the part of the drawing which will actually be drawn onto the widget).*/
    int xMargin;

    /**Border on the top and bottom sides inside the window (QRect corresponding
  * to the part of the drawing which will actually be drawn onto the widget).*/
    int yMargin;

    /**Boolean used to update the display after a change in the mode of display: single or multicolumns.*/
    bool columnDisplayChanged;

    /**Boolean used to update the display after a resize event.*/
    bool resized;

    /**List of the selected channels.*/
    QList<int> mSelectedChannels;

    /**Boolean used to update the display after a change in the number of groups.*/
    bool groupsChanged;

    /**Ordinate of the previous position while dragging an event.*/
    int previousDragOrdinate;

    /**Ordinate of the last click before dragging channels.*/
    int lastClickOrdinate;

    /*Boolean used to update the display after a change in the number of samples per trace.**/
    bool nbSamplesModified;

    /*Boolean used to update the display after a change in the selection of the traces.**/
    bool alreadySelected;

    /**Boolean used to correctly display multicolumns at startup.*/
    bool isInit;

    /**Screen resolution in pixel by centimeter, the default is 30 (equivalent to 75 pixels by inch).*/
    int screenResolution;

    /**Channel for which the voltage has to be determined.*/
    int channelforVoltageComputation;

    /**Index of the starting drag, use to determine the time when selecting a zone for measure.*/
    int startingIndex;

    /**A cursor to represent the selection of channels.*/
    QCursor selectCursor;

    /**A cursor to represent the measure state.*/
    QCursor measureCursor;

    /**A cursor to represent the selection of time state.*/
    QCursor selectTimeCursor;

    /**A cursor to represent the selection of an event.*/
    QCursor selectEventCursor;

    /**A cursor to represent the addition of an event.*/
    QCursor addEventCursor;

    /**A cursor to represent the drawing of a line.*/
    QCursor drawLineCursor;

    /**Boolean indicating that the labels next to the traces have to be shown or hide.
  * If true, they have to be shown, if false they have to be hidden. They are hidden by default.
  */
    bool showLabels;

    /**Boolean indicating that the calibration scale has to be shown or hide.
  * If true, the scale has to be shown, if false it has to be hidden. It is hidden by default.
  */
    bool showCalibrationScale;


    /**Number of samples to leave out while drawing in order to speed the browsing of the data.
  * This number takes into account the size of the window
  * and the the size of the widget.*/
    float downSampling;

    /**Boolean indicating that the view has been zoomed.*/
    bool zoomed;

    /***Boolean indicating that the view has been zoomed for the no zoom state.*/
    bool firstZoom;

    /***Boolean indicating that the view has been zoomed and a reset has been asked.
  * If so the original down sampling has to be restore.*/
    bool doubleClick;

    /**Initial downSampling before any zoom.*/
    float initialDownSampling;

    /**Stores the current factor of zoom.*/
    float zoomFactor;

    /***Boolean indicating that the view has been zoomed to the maximum.*/
    bool maxZoomReached;

    /***Boolean indicating that the view has been zoomed out.*/
    bool zoomOut;

    /**Initial window before any zoom.*/
    QRect initialWindow;

    /**Window used before the current zoom..*/
    QRect previousWindow;

    /**The current size of a trace in the world while in multi columns mode.*/
    int traceWidth;

    /**The initial abscissa space between two groups (when groups are display on several columns) before any zoom..*/
    int initialXGroupSpace;

    /**The initial delta between the starting abcisses of two channels before any zoom.*/
    int initialXshift;

    /**The initial size of a trace in the world while in multi columns mode.*/
    int initialTraceWidth;

    /**Map between the cluster provider names and the list of selected clusters.*/
    QMap<int, QList<int> > selectedClusters;

    /**Structure representing cluster data, actual data and status.*/
    struct ClusterData{
        Array<dataType> data;
        bool ready;

        ClusterData(Array<dataType> d,bool status){
            data = d;
            ready = status;
        }
        ClusterData(){
            ready = false;
        }
        void setStatus(bool status){ready = status;}
        void setData(Array<dataType>& d){data = d;}
        bool status(){return ready;}
        Array<dataType>& getData(){return data;}

        ~ClusterData(){}
    };

    /**Dictionary between the cluster provider names and the cluster data and status.*/
    QHash<QString, ClusterData*> clustersData;

    /** Dictionary between the cluster provider names and the providers.*/
    QHash<QString, ClustersProvider*> clusterProviders;

    /**Dictionary between the provider names and the item color lists except for the TracesProvider.*/
    QHash<QString, ItemColors*> providerItemColors;

    /**Stores the cluster order used when they are presented in raster. Each cluster is identified
  * by a string build as the ClusterProvider name plus a dash plus the cluster id.
  */
    QStringList clustersOrder;

    /**Stores the cluster raster ordinate position.*/
    QList<int> rasterOrdinates;

    /**Stores the cluster raster abscissa position.*/
    QList<int> rasterAbscisses;

    /**Map given the list of cluster file containing data for a given display group.
  * This assumes that the cluster file names contain the identifier of
  * the spike group used to create them (myFile.clu.1 correspond to the
  * spike group 1).
  */
    QMap<int, QList<int> >* groupClusterFiles;

    /*Map between the channel ids and the spike group ids. */
    QMap<int,int>* channelClusterFiles;

    /*Number of samples before the sample of the peak are contained in the waveform of a spike.*/
    int nbSamplesBefore;

    /*Number of samples after the sample of the peak are contained in the waveform of a spike.*/
    int nbSamplesAfter;

    /*True if the view is currently been printed, false otherwise.**/
    bool printState;

    /**Map between the event provider names and the list of selected events.*/
    QMap<QString, QList<int> > selectedEvents;

    /**Pair storing the cluster provider having a selected cluster the closer in time to the current endTime,
  * the pair stores also the starting time of the retrieve data.*/
    QPair<QString,long> nextClusterProvider;

    /**Pair storing the cluster provider having a selected cluster the closer in time to the current startTime,
  * the pair stores also the starting time of the retrieve data.*/
    QPair<QString,long> previousClusterProvider;

    /**Stores the cluster provider which contains the cluster id look up in a nextCluster or previousCluster action.*/
    QString clusterProviderToSkip;

    /**This variable keeps track of the current start time of the time window in recording unit when
  * browsing on the spikes.
  */
    long startTimeInRecordingUnits;

    /**This variable keeps track of the previous start time of the time window in recording unit when
  * browsing on the spikes.
  */
    long previousStartTimeInRecordingUnits;

    /**True if the user has just browsed spikes, false otherwise.*/
    bool spikeBrowsing;

    /**Dictionary between the event provider names and the event data and status.*/
    QHash<QString, EventData*> eventsData;

    /**Dictionary between the event provider names and the providers.*/
    QHash<QString, EventsProvider*> eventProviders;

    /**Pair storing the event provider having a selected event the closer in time to the current endTime,
  * the pair stores also the starting time of the retrieve data.*/
    QPair<QString,long> nextEventProvider;

    /**Pair storing the event provider having a selected event the closer in time to the current startTime,
  * the pair stores also the starting time of the retrieve data.*/
    QPair<QString,long> previousEventProvider;

    /**Stores the event provider which contains the event id look up in a nextEvent or previousEvent action.*/
    QString eventProviderToSkip;

    /**Length of the recording in miliseconds.*/
    qlonglong length;

    /**Pair storing the event provider and the event id corresponding to the currently selected event.*/
    QPair<QString,int> selectedEvent;

    /**Abscissa of the previous position while dragging channels.*/
    int previousDragAbscissa;

    bool initialDragLine;
    /**Abscissa of the last click before dragging an event.*/
    int lastClickAbscissa;

    /**Contains the original abscissa and index of the selected event.*/
    QList<int> selectedEventPosition;

    /**True if it is the beginning of an event dragging.*/
    bool startEventDragging;

    /**Stores the event providers containing events which have been modified and thus
 * needing to be update.*/
    QStringList eventProvidersToUpdate;

    /**Index of the new event to create.*/
    int newEventPosition;

    /**Label corresponding to the type of event to create at the next addEvent.*/
    QString eventDescriptionToCreate;

    /**Identifier of the event file to which a new event will be added.*/
    QString eventProvider;

    /**Boolean indicating that the modification of an event is in process.*/
    bool eventBeingModified;

    /**Map between the cluster provider names and the list of clusters to not be used for browsing.*/
    QMap<QString, QList<int> > clustersNotUsedForBrowsing;

    /**Map between the event provider names and the list of events to not be used for browsing.*/
    QMap<QString, QList<int> > eventsNotUsedForBrowsing;

    bool retrieveClusterData;

    /*List of skipped channels.*/
    QList<int> skippedChannels;

    /*List of positions for the current lines to be drawn.*/
    QList<int> linePositions;

    //***************Functions************

    /**Updates the dimension of the window.*/
    void updateWindow();

    /**
 * Updates shownGroupsChannels.
 * @param channelsToShow list of channels to shown in the display.
 */
    void updateShownGroupsChannels(const QList<int>& channelsToShow);

    /**
 * Draws the traces.
 * @param painter painter on which to draw the traces.
 */
    void drawTraces(QPainter& painter);

    /**
 * Draws the traces for a subset of channels.
 * @param channels list of channels to draw.
 * @param highlight true if the channels have to be highlighted, false otherwise.
 */
    void drawTraces(const QList<int>& channels,bool highlight);

    /**
 * Draws the trace for the channel @p channelId.
 * @param painter painter on which to draw the traces.
 * @param limit the number of pixels in the world beneath which only one pixel is drawn
 * in the viewport.
 * @param basePosition the base ordinate for the trace to be drawn.
 * @param X the starting abscissa.
 * @param channelId the id of the channel for which the trace has to be drawn.
 * @param nbSamplesToDraw number of samples to draw for the trace.
 * @param mouseMoveEvent true if the function has been called from the mouseMoveEvent function. In that
 * case no waveforms are drawn even if requested.
 */
    void drawTrace(QPainter& painter,int limit,int basePosition,int X,int channelId,int nbSamplesToDraw,bool mouseMoveEvent = false);

    /**Draws on the left side the id and the amplitude for each channel.
 * @param painter painter on which to draw the information.
 */
    void drawChannelIdsAndGain(QPainter& painter);

    /**
     * Draws the amplitude for each channel.
     * @param painter painter on which to draw the information.
     * @param channels channels for which to draw the information
     * @param enableSkipping whether to honour skippedChannels
     */
    void drawChannelGain(QPainter& painter, const QList<int>& channels, bool enableSkipping);

    /**Computes the channelDisplayGains.*/
    void computeChannelDisplayGain();

    /**Computes the channelDisplayGains for the selected channels.
 * @param selectedChannels ids of the selected channels.
 */
    void computeChannelDisplayGain(const QList<int>& selectedChannels);

    /**Draw the calibration scale.
 * @param painter painter on which to draw the information.
 */
    void drawCalibrationScale(QPainter& painter);

    /** Corrects the window due to a zoom.
 * @param r rectangle corresponding to the window.
 */
    void correctZoom(QRect& r);


    /** Draws a single event.
 * @param providerName name of the instance providing the data for the event to draw.
 * @param selectedEventId id of the event to draw.
 * @param selectedEventIndex index of the event to draw.
 * @param highlight true if the event has to be highlighted, false otherwise.
 */
    void drawEvent(const QString &providerName, int selectedEventId, dataType selectedEventIndex, bool highlight);

    /** Draws a vertical line at the cursor position. If the traces are displayed on multiple columns, a line is drawn in each column.
 * @param x abscissa of the cursor.
 * @param initialLine true if the line is drawn for the first time, false otherwise.
 * @param eraseLine true if the line has to be erase and not redraw.
 */
    void drawTimeLine(QPainter *painter);

    /**Scale the background image to fit it in the widget.*/
    void scaleBackgroundImage();

    void changeCursor();

    QPoint m_currentPoint;
    QPoint m_selectPoint;
    bool mMoveSelectChannel;
};

#endif
