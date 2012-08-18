/***************************************************************************
                          neuroscopeview.h  -  description
                             -------------------
    begin                : Wed Feb 25 19:05:25 EST 2004
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

#ifndef NEUROSCOPEVIEW_H
#define NEUROSCOPEVIEW_H


//Include files for KDE
#include <kdockwidget.h>
#include <QStatusBar>


// include files for Qt
#include <qwidget.h>
#include <q3dict.h>
#include <QImage>
#include <QPrinter>
//Added by qt3to4:
#include <Q3ValueList>

// application specific includes
#include "baseframe.h"
#include "positionview.h"
#include "globaleventsprovider.h"
#include "tracewidget.h"

class NeuroscopeApp;
class TracesProvider;
class ChannelColors;
class ClustersProvider;
class EventsProvider;
class PositionsProvider;
class ItemColors;
class TraceWidget;

//General C++ include files
#include <iostream>
using namespace std;


/** The NeuroscopeView class provides the view widgets for the document instance (NeuroscopeDoc instance) and is displayed
 * as a MDI child window in the main view area of the NeuroscopeApp class instance.
 * The document instance keeps a list of all views that represent the document contents as there
 * can be more than one view. Views get created in NeuroscopeApp::createDisplay() and automatically added to the list of views.
 * The NeuroscopeView class inherits QWidget as a KDockArea to give it the ability to contains dockWidgets.
 * @author Lynn Hazan
 */
class NeuroscopeView : public KDockArea
{
  Q_OBJECT
  public:
    /**
     * @param mainWindow a reference on the main window of the application.
     * @param label label for the display when in tab page mode.
     * @param startTime starting time in miliseconds.
     * @param duration time window in miliseconds.
     * @param backgroundColor color used as background.
     * @param wflags common QWidget flags.
     * @param statusBar a reference to the application status bar.
     * @param channelsToDisplay list of channel to be shown at the opening of the view.
     * @param greyScale true if all the channels are display either in a gradation of grey false if they are display in color.
     * @param tracesProvider provider of the channels data.
     * @param multiColumns true if the traces are displayed on multiple columns, false otherwise.
     * @param verticalLines true if vertical lines are displayed to show the clusters, false otherwise.
     * @param raster true if a raster is drawn to show the clusters, false otherwise
     * @param waveforms true if waveforms are drawn on top of the traces, false otherwise.
     * @param labelsDisplay true if labels are drawn next to the traces, false otherwise.
     * @param unitGain initial gain use to draw the traces in the TraceView.
     * @param acquisitionGain acquisition gain.
     * @param channelColors a pointer on the list of colors for the channels.
     * @param groupsChannels a pointer on the map given the list of channels for each group.
     * @param channelsGroups a pointer on the map given to which group each channel belongs.
     * @param offsets list containing the offset for each channel.
     * @param channelGains list of the exponents used to compute the drawing gain for each channel.
     * @param selected list of the currently selected channels.
     * @param skipStatus map given the skip status of the channels.
     * @param rasterHeight height of the rasters in the world coordinate system.
     * @param backgroundImagePath path of the image used as background.
     * @param parent the parent QWidget.
     * @param name name of the widget (can be used for introspection).
     */
    NeuroscopeView(NeuroscopeApp& mainWindow,QString label,long startTime,long duration,QColor backgroundColor,int wflags,QStatusBar * statusBar,Q3ValueList<int>* channelsToDisplay,bool greyScale,
                  TracesProvider& tracesProvider,bool multiColumns,bool verticalLines,
                  bool raster,bool waveforms,bool labelsDisplay,int unitGain,int acquisitionGain,ChannelColors* channelColors,
                  QMap<int,Q3ValueList<int> >* groupsChannels,QMap<int,int>* channelsGroups,
                  Q3ValueList<int> offsets,Q3ValueList<int> channelGains,Q3ValueList<int> selected,QMap<int,bool> skipStatus,int rasterHeight,QString backgroundImagePath,
                  QWidget *parent = 0, const char *name=0);
    /** Destructor for the main view. */
    ~NeuroscopeView();

	/// Added by M.Zugaro to enable automatic forward paging
  	void page() { traceWidget->page(); };
  	void accelerate() { traceWidget->accelerate(); };
  	void decelerate() { traceWidget->decelerate(); };

    /**Contains the implementation for printing functionality.
    * @param pPrinter pointer on the printer object.
    * @param filePath path of the opened document.
    * @param whiteBackground true if the printed background has to be white, false otherwise.
    */
    void print(QPrinter* pPrinter,QString filePath,bool whiteBackground);

    /**Sets the unit gain and the acquisition system gain.
    * @param gain initial gain use to draw the traces in the TraceView.
    * @param acquisitionGain acquisition gain.
    */
    inline void setGains(int gain,int acquisitionGain ){
     emit updateGains(gain,acquisitionGain);
     emit drawTraces();
    };

    /**Updates the background color used in the views.
    * @param color color of the new background.
    */
    inline void updateBackgroundColor(QColor color){emit changeBackgroundColor(color);};

    /**
    * Informs all the child widgets that one of the features for the document have changed.
    * The features are: the initial offset or the acquisition system resolution.
    */
    inline void documentFeaturesModified(){emit drawTraces();};

    /**
    * Informs all the child widgets that the sampling rate for the document has changed.
    * @param recordingLength the newly computed length of the document.
    */
    inline void samplingRateModified(long long recordingLength){
      emit newSamplingRate(recordingLength);
    }

    /** Updates the number of channels in the child widgets.
    * @param nb the new number of channels.
    */
    void setChannelNb(int nb);

    /**Show all the encapsulated widgets contain in the view. The widgets know how to draw themselves.*/
    inline void showAllWidgets(){
      dockManager->activate();
      emit updateContents();
    };

    /**All the channels of the display are now display either in a gradation of grey or in color.
    * @param grey true if the channels have to be displayed in grey false otherwise.
    */
    inline void setGreyScale(bool grey){
     greyScaleMode = grey;
     emit greyScale(grey);
    };

    /**Retuns true if the channels of the display are display in a gradation of grey,
    * false if they are display in color.*/
    inline bool getGreyScale() const {return greyScaleMode;};

    /**
    * Updates the list of channels shown with @p channelsToShow.
    * Makes the different views update their drawing if need it. This method is called only on the active view.
    * @param channelsToShow new list of channels to be shown.
    */
    void shownChannelsUpdate(const Q3ValueList<int>& channelsToShow);

    /**Returns the list of the channels presented in the view.
    * @return list of channels ids.
    */
    inline const Q3ValueList<int>& channels() const {return *shownChannels;};

    /**
    * Updates the list of clusters for the cluster provider @p name shown with @p clustersToShow.
    * Makes the different views update their drawing if need it. This method is called only on the active view.
    * @param name name use to identified the cluster provider containing the clusters to show.
    * @param clustersToShow list of clusters to be shown.
    */
    void shownClustersUpdate(QString name,Q3ValueList<int>& clustersToShow);

    /**
    * Updates the list of events for the event provider @p name shown with @p eventsToShow.
    * Makes the different views update their drawing if need it. This method is called only on the active view.
    * @param name name use to identified the event provider containing the events to show.
    * @param eventsToShow list of events to be shown.
    */
    void shownEventsUpdate(QString name,Q3ValueList<int>& eventsToShow);

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

    /**
    * Returns true if traces are displayed on multiple columns, false otherwise.
    */
    inline bool getMultiColumns() const {return multiColumns;};

    /**Returns true if vertical lines are displayed to show the clusters, false otherwise.
    */
    inline bool getClusterVerticalLines() const {return verticalLines;};

    /**Returns true if a raster is drawn to show the clusters, false otherwise.
    */
    inline bool getClusterRaster() const {return raster;};

    /**Returns true if waveforms are drawn on top of the traces, false otherwise.
    */
    inline bool getClusterWaveforms() const {return waveforms;};

   /** Returns the list containing the selected clusters for the given cluster provider @p identified by name.
   * @param name name use to identified the cluster provider containing selected the clusters.
   * @return the list of selected clusters.
   */
   inline const Q3ValueList<int>* getSelectedClusters(QString name) const{return selectedClusters[name];};

   /** Returns the list containing the selected events for the given event provider @p identified by name.
   * @param name name use to identified the event provider containing selected the events.
   * @return the list of selected events.
   */
   inline const Q3ValueList<int>* getSelectedEvents(QString name) const{return selectedEvents[name];};

   /**Updates the selected tool, methode call after the user selected a tool.
   * @param selectedMode the new mode.
   * @param active true if the view is the active one, false otherwise.
   */
   inline void setMode(BaseFrame::Mode selectedMode,bool active){
    if(selectedMode == 2) selectMode = true;
    else selectMode = false;
    emit modeToSet(selectedMode,active);
   };

   /** Tests if the currently selected tool is the selection one.
   * @ return true if the current tool is the selection one, false otherwise.
   */
   inline bool isSelectionTool(){return selectMode;};

   /**Changes the color of a channel.
   * @param channelId id of the channel to redraw.
   * @param active true if the view is the active one, false otherwise.
   */
   inline void singleChannelColorUpdate(int channelId,bool active){emit channelColorUpdate(channelId,active);};

   /**Changes the color of a group of channels.
   * @param groupId id of the group for which the color have been changed.
   * @param active true if the view is the active one, false otherwise.
   */
   inline void channelGroupColorUpdate(int groupId,bool active){emit groupColorUpdate(groupId,active);};

   /**Triggers the increase of the amplitude of all the channels.
   */
   inline void increaseAllChannelsAmplitude(){emit increaseAllAmplitude();};

   /**Triggers the decrease of the amplitude of all the channels.
   */
   inline void decreaseAllChannelsAmplitude(){emit decreaseAllAmplitude();};

   /**Triggers the increase of the amplitude of the selected channels.
   */
   inline void increaseSelectedChannelsAmplitude(const Q3ValueList<int> selectedIds){
    //update the list of selected channels
    selectedChannels.clear();
    Q3ValueList<int>::const_iterator selectedIterator;
    for(selectedIterator = selectedIds.begin(); selectedIterator != selectedIds.end(); ++selectedIterator)
     selectedChannels.append(*selectedIterator);

    emit increaseAmplitude(selectedIds);
   };

   /**Triggers the decrease of the amplitude of the selected channels.
   */
   inline void decreaseSelectedChannelsAmplitude(const Q3ValueList<int> selectedIds){
    //update the list of selected channels
    selectedChannels.clear();
    Q3ValueList<int>::const_iterator selectedIterator;
    for(selectedIterator = selectedIds.begin(); selectedIterator != selectedIds.end(); ++selectedIterator)
     selectedChannels.append(*selectedIterator);

    emit decreaseAmplitude(selectedIds);
   };

    /**Update all the encapsulated widgets contain in the view. The widgets know how to draw themselves.*/
    inline void updateViewContents(){
      emit updateDrawing();
    };

    /**Triggers the update of the display due to a change in the display groups.
    * @param active true if the view is the active one, false otherwise.
    */
    inline void groupsModified(bool active){emit groupsHaveBeenModified(active);};

   /**Selects the channels in the TraceView.
   *@param selectedIds ids of the selected channels.
   */
   inline void selectChannels(const Q3ValueList<int>& selectedIds){
    selectedChannels.clear();
    Q3ValueList<int>::const_iterator selectedIterator;
    for(selectedIterator = selectedIds.begin(); selectedIterator != selectedIds.end(); ++selectedIterator){
     selectedChannels.append(*selectedIterator);
    }

    emit channelsToBeSelected(selectedIds);
   };

   /**Resets the offset of the selected channels.
   *@param selectedIds ids of the selected channels.
   */
   void resetOffsets(const Q3ValueList<int>& selectedIds);

   /**Resets the gain of the selected channels.
   *@param selectedIds ids of the selected channels.
   */
   void resetGains(const Q3ValueList<int>& selectedIds){
    //update the list of selected channels
    selectedChannels.clear();
    Q3ValueList<int>::const_iterator selectedIterator;
    for(selectedIterator = selectedIds.begin(); selectedIterator != selectedIds.end(); ++selectedIterator)
     selectedChannels.append(*selectedIterator);

    emit resetChannelGains(selectedChannels);
   };


   /** Returns the list containing the offset for each channel.
   * @return the list of the offsets.
   */
   inline const Q3ValueList<int>& getChannelOffset() const{return channelOffsets;};

   /** Returns the list containing the exponents used to compute the drawing gain for each channel.
   * @return the list of gains.
   */
   inline const Q3ValueList<int>& getGains() const{return gains;};

   /** Returns the list containing the selected channels.
   * @return the list of selected channels.
   */
   inline const Q3ValueList<int>& getSelectedChannels() const{return selectedChannels;};

   /** Sets the channels selected in the channel palettes.
   *@param selectedIds ids of the selected channels.
   */
   inline void setSelectedChannels(const Q3ValueList<int>& selectedIds){
    //update the list of selected channels
    selectedChannels.clear();
    Q3ValueList<int>::const_iterator selectedIterator;
    for(selectedIterator = selectedIds.begin(); selectedIterator != selectedIds.end(); ++selectedIterator){
     selectedChannels.append(*selectedIterator);
    }
   };

   /** Sets the label for the display when in tab page mode.
   * @param newLabel the new label for the display.
   */
   inline void setTabName(QString newLabel){tabLabel = newLabel;};

   /** Gets the label for the display when in tab page mode.
   * @return newLabel the new label for the display.
   */
   inline QString getTabName(){return tabLabel;};

  /**Gets the starting time in miliseconds.
  * @return starting time.
  */
  inline long getStartTime(){return startTime;};

  /**Gets the time window in miliseconds.
  * @return duration time window.
  */
  inline long getTimeWindow(){return timeWindow;};

  /**Displays or hides the labels next to the traces.
  * @param status true if the labels have to be drawn, false otherwise.
  */
  inline void showLabelsUpdate(bool status){
   labelsDisplay = status;
   emit showLabels(status);
  };

  /**Returns true if labels are drawn next to the traces, false otherwise.
  */
  inline bool getLabelStatus() const{return labelsDisplay;};

  /** Shows or hides the calibration bar. This bar is meaningful only when all the channels
  *  have the same amplification.
  * @param show true if the bar has to be shown false otherwise.
  * @param active true if the view is the active one, false otherwise.
  */
  inline void showCalibration(bool show,bool active){emit displayCalibration(show,active);};

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
  * @param clustersToSkip list of clusters to be skipped while browsing.
  */
  void setClusterProvider(ClustersProvider* clustersProvider,QString name,ItemColors* clusterColors,bool active,
                          Q3ValueList<int>& clustersToShow,QMap<int, Q3ValueList<int> >* displayGroupsClusterFile,QMap<int,int>* channelsSpikeGroups,
                          int nbSamplesBefore,int nbSamplesAfter,const Q3ValueList<int>& clustersToSkip);

  /**Removes a provider of cluster data.
  * @param name name use to identified the cluster provider.
  * @param active true if the view is the active one, false otherwise.
  */
  void removeClusterProvider(QString name,bool active);

  /**Changes the color of a cluster.
  * @param name name use to identified the cluster provider containing the updated cluster.
  * @param clusterId id of the cluster to redraw.
  * @param active true if the view is the active one, false otherwise.
  */
  inline void  clusterColorUpdate(QString name,int clusterId,bool active){emit clusterColorUpdated(name,clusterId,active);};

  /**Hides the cluster waveforms on top of the traces but keep the setting of displaying or hidding them.*/
  inline void ignoreWaveformInformation(){ emit clusterWaveformsDisplay(false);};

  /**Retrieves the next cluster.*/
  inline void showNextCluster(){emit nextCluster();};

  /**Retrieves the previous cluster.*/
  inline void showPreviousCluster(){emit previousCluster();};

  /**Adds a new provider of event data.
  * @param eventsProvider provider of event data.
  * @param name name use to identified the event provider.
  * @param eventColors list of colors for the events.
  * @param active true if the view is the active one, false otherwise.
  * @param eventsToShow list of events to be shown.
  * @param eventsToSkip list of clusters to be skipped while browsing.
  */
  void setEventProvider(EventsProvider* eventsProvider,QString name,ItemColors* eventColors,bool active,
                          Q3ValueList<int>& eventsToShow,const Q3ValueList<int>& eventsToSkip);

  /**Removes a provider of event data.
  * @param name name use to identified the event provider.
  * @param active true if the view is the active one, false otherwise.
  * @param lastFile true if the event file removed is the last event provider, false otherwise.
  */
  void removeEventProvider(QString name,bool active,bool lastFile);

  /**Changes the color of a event.
  * @param name name use to identified the event provider containing the updated event.
  * @param eventId id of the event to redraw.
  * @param active true if the view is the active one, false otherwise.
  */
  inline void  eventColorUpdate(QString name,int eventId,bool active){emit eventColorUpdated(name,eventId,active);};

  /**Retrieves the next event.*/
  inline void showNextEvent(){
   emit nextEvent();
  };

  /**Retrieves the previous event.*/
  inline void showPreviousEvent(){
   emit previousEvent();
  };

  /**Updates the traceView if concerned by the modification of the event.
  * @param providerName name use to identified the event provider containing the modified event.
  * @param selectedEventId id of the modified event.
  * @param time initial time of the modified event.
  * @param newTime new time of the modified event.
  * @param active true if the view is the active one, false otherwise.
  */
  void updateEvents(QString providerName,int selectedEventId,float time,float newTime,bool active);

  /**Deletes the selected event.
  */
  inline void removeEvent(){
   emit eventToRemove();
   emit updateEventDisplay();
  };

  /**Updates the traceView if concerned by the suppression of the event.
  * @param providerName name use to identified the event provider containing the removed event.
  * @param eventId id of the removed event.
  * @param time initial time of the removed event.
  * @param active true if the view is the active one, false otherwise.
  */
  void updateEventsAfterRemoval(QString providerName,int eventId,float time,bool active);

  /**Updates the traceView if concerned by the addition of the event.
  * @param providerName name use to identified the event provider containing the added event.
  * @param eventId id of the added event.
  * @param time initial time of the added event.
  * @param active true if the view is the active one, false otherwise.
  */
  void updateEventsAfterAddition(QString providerName,int eventId,float time,bool active);

  /**Updates the traceView if concerned by the modification of the event.
  * @param providerName name use to identified the event provider containing the modified event.
  * @param eventId id of the modified event.
  * @param time initial time of the modified event.
  * @param active true if the view is the active one, false otherwise.
  */
  inline void updateEvents(QString providerName,int eventId,float time,bool active){
   emit updateEvents(active,providerName,time);
   emit updateEventDisplay();
  };

  /**Informs the traceView of the properties for the next event to be added.
  * @param providerName name use to identified the event provider which will contain the added event.
  * @param eventDescription description of the next event to be created.
  */
  inline void eventToAddProperties(QString providerName,QString eventDescription){emit newEventProperties(providerName,eventDescription);};

  /**Updates the ids of the selected events due to the modification of event ids due to the addition or suppression of an
  * event description.
  * @param providerName provider identifier.
  * @param oldNewEventIds map between the previous eventIds and the new ones.
  * @param addedEventId id corresponding to the new event description added.
  * @param active true if the view is the active one, false otherwise.
  * @param added true if an event description has been added, false if it has been removed.
  */
  void updateSelectedEventsIds(QString providerName,QMap<int,int>& oldNewEventIds,int addedEventId,bool active,bool added);

  /**Removes the position provider.
  * @param name name use to identified the position provider.
  * @param active true if the view is the active one, false otherwise.
  */
  void removePositionProvider(QString name,bool active);

  /**
  * Returns true if a position view is displayed, false otherwise.
  */
  inline bool isPositionView()const {return isPositionFileShown;};

  /**Adds a PositionView to the display.
  * @param positionsProvider provider of position data.
  * @param backgroundImage image used as a background for the position view.
  * @param backgroundColor color used as background if no image is set.
  * @param startTime starting time in miliseconds.
  * @param duration time window in miliseconds.
  * @param width video image width.
  * @param height video image height.
  * @param showEvents 1 if events are displayed in the PositionView, 0 otherwise.
  */
  void addPositionView(PositionsProvider* positionsProvider,QImage backgroundImage,QColor backgroundColor,long startTime,long duration,int width,int height,bool showEvents);

  /**Adds a PositionView to the display.
  * @param positionsProvider provider of position data.
  * @param backgroundImage image used as a background for the position view.
  * @param backgroundColor color used as background if no image is set.
  * @param width video image width.
  * @param height video image height.
  */
  inline void addPositionView(PositionsProvider* positionsProvider,QImage backgroundImage,QColor backgroundColor,int width,int height){
   addPositionView(positionsProvider,backgroundImage,backgroundColor,startTime,timeWindow,width,height,eventsInPositionView);
  };

  /**Removes the PositionView from the display.*/
  void removePositionView();

  /**Updates the cluster information presented on the display.
  * @param active true if the view is the active one, false otherwise.
  */
  inline void updateClusterData(bool active){emit clusterProviderUpdated(active);};

  /** Returns the list of clusters to not be used for browsing for the given cluster provider @p identified by name.
  * @param name name use to identified the cluster provider containing the clusters to skip.
  * @return the list of skipped clusters.
  */
  inline const Q3ValueList<int>* getClustersNotUsedForBrowsing(QString name) const{return clustersNotUsedForBrowsing[name];};

  /** Returns the list of events to be not used for browsing for the given event provider @p identified by name.
  * @param name name use to identified the event provider containing the events to skip.
  * @return the list of skipped events.
  */
  inline const Q3ValueList<int>* getEventsNotUsedForBrowsing(QString name) const{return eventsNotUsedForBrowsing[name];};

  /**Updates the list of events to not use while browsing.
  * @param providerName name use to identified the event provider containing the modified event.
  * @param eventsToNotBrowse new list of events to not use while browsing.
  */
  void updateNoneBrowsingEventList(QString providerName,const Q3ValueList<int>& eventsToNotBrowse);

  /**Updates the list of clusters to not use while browsing.
  * @param providerName name use to identified the event provider containing the modified event.
  * @param clustersToNotBrowse new list of clusters to not use while browsing.
  */
  void updateNoneBrowsingClusterList(QString providerName,const Q3ValueList<int>& clustersToNotBrowse);

  /**Updates the list of skipped channels.
  * @param skippedChannels list of skipped channels
  **/
  inline void updateSkipStatus(const Q3ValueList<int>& skippedChannels){emit skipStatusChanged(skippedChannels);};

  /**Increases the height of the rasters.*/
  inline void increaseRasterHeight(){emit increaseTheRasterHeight();};

  /**Decreases the height of the rasters.*/
  inline void decreaseRasterHeight(){emit decreaseTheRasterHeight();};

  /**Returns the height of the rasters.
  *@return raster height.
  */
  int getRasterHeight();

 /**Returns if events are displayed in the PositionView.
  * @return 1 if events are displayed in the PositionView, 0 otherwise.
  */
  inline int isEventsInPositionView()const{return eventsInPositionView;};

public slots:
    /** Informs listener that the channels @p selectedIds have been selected.
    * @param selectedIds the list of channels selected by the user in a view.
    */
    inline void slotChannelsSelected(const Q3ValueList<int>& selectedIds){
    selectedChannels.clear();
    Q3ValueList<int>::const_iterator selectedIterator;
    for(selectedIterator = selectedIds.begin(); selectedIterator != selectedIds.end(); ++selectedIterator)
     selectedChannels.append(*selectedIterator);

     emit channelsSelected(selectedIds);
    };

   /**Sets the starting time and the time window in miliseconds.
   * @param start starting time.
   * @param duration time window.
   */
   inline void setStartAndDuration(long start,long duration){
    startTime = start;
    timeWindow = duration;
    emit timeChanged(start,duration);
   };

  /**Informs the application that an event has been modified.
  * @param providerName name use to identified the event provider containing the modified event.
  * @param selectedEventId id of the modified event.
  * @param time initial time of the modified event.
  * @param newTime new time of the modified event.
  */
  inline void slotEventModified(QString providerName,int selectedEventId,double time,double newTime){
    emit eventModified(providerName,selectedEventId,time,newTime);
    emit updateEventDisplay();
  };

  /**Informs the application that an event has been removed.
  * @param providerName name use to identified the event provider containing the removed event.
  * @param selectedEventId id of the removed event.
  * @param time initial time of the removed event.
  */
  inline void slotEventRemoved(QString providerName,int selectedEventId,double time){
   emit eventRemoved(providerName,selectedEventId,time);
   emit updateEventDisplay();
  };

  /**Informs that an event has been added.
  * @param providerName name use to identified the event provider containing the added event.
  * @param addedEventDescription description of the added event.
  * @param time time of the added event.
  */
  inline void slotEventAdded(QString providerName,QString addedEventDescription,double time){
   emit eventAdded(providerName,addedEventDescription,time);
  };

  /** Updates the description of a spike waveform.
  * @param nbSamplesBefore number of samples contained in the waveform of a spike before the sample of the peak.
  * @param nbSamplesAfter number of samples contained in the waveform of a spike after the sample of the peak.
  * @param active true if the view is the active one, false otherwise.
  */
  inline void updateWaveformInformation(int nbSamplesBefore, int nbSamplesAfter,bool active){
   emit waveformInformationUpdated(nbSamplesBefore,nbSamplesAfter,active);
  };


  /**Updates the information needed to draw the position of the animal.
  * @param width video image width.
  * @param height video image height.
  * @param backgroundImage image used as a background for the position view.
  * @param newOrientation true if the image has been transformed (rotate and or flip), false otherwise.
  * @param active true if the view is the active one, false otherwise.
  */
  inline void updatePositionInformation(int width, int height,QImage backgroundImage,bool newOrientation,bool active){
   emit positionInformationUpdated(width,height,backgroundImage,newOrientation,active);
  };


  /**Takes care of the closing of the PositionView.
  * @param view the PositionView to be closed.
  */
  void positionDockClosed(QWidget* view);

  /**Sets if events are displayed in the PositionView.
  * @param shown 1 if events are displayed in the PositionView, 0 otherwise.
  */
  void setEventsInPositionView(bool shown);

  /**Updates the background image.
  * @param traceBackgroundImage image to be used as background.
  * @param active true if the view is the active one, false otherwise.
  */
   inline void updateTraceBackgroundImage(QImage traceBackgroundImage,bool active){emit traceBackgroundImageUpdate(traceBackgroundImage,active);};



  signals:
    void changeBackgroundColor(QColor color);
    void updateContents();
    void greyScale(bool geyScale);
    void clusterWaveformsDisplay(bool waveforms);
    void clusterRasterDisplay(bool raster);
    void clusterVerticalLinesDisplay(bool lines);
    void multiColumnsDisplay(bool multi);
    void channelsSelected(const Q3ValueList<int>& selectedIds);
    void modeToSet(BaseFrame::Mode selectedMode,bool active);
    void showChannels(const Q3ValueList<int>& channelsToShow);
    void channelColorUpdate(int channelId,bool active);
    void groupColorUpdate(int groupId,bool active);
    void increaseAllAmplitude();
    void decreaseAllAmplitude();
    void increaseAmplitude(const Q3ValueList<int>& selectedIds);
    void decreaseAmplitude(const Q3ValueList<int>& selectedIds);
    void updateGains(int gain,int acquisitionGain);
    void updateDrawing();
    void groupsHaveBeenModified(bool);
    void channelsToBeSelected(const Q3ValueList<int>& selectedIds);
    void resetChannelOffsets(const QMap<int,int>& selectedChannelDefaultOffsets);
    void resetChannelGains(const Q3ValueList<int>& selectedChannels);
    void drawTraces();
    void reset();
    void showLabels(bool show);
    void displayCalibration(bool show,bool active);
    void newClusterProvider(ClustersProvider* clustersProvider,QString name,ItemColors* clusterColors,bool active,
                            Q3ValueList<int>& clustersToShow,QMap<int, Q3ValueList<int> >* displayGroupsClusterFile,
                            QMap<int,int>* channelsSpikeGroups,int nbSamplesBefore,int nbSamplesAfter,const Q3ValueList<int>& clustersToSkip);
    void newSamplingRate(long long recordingLength);
    void clusterProviderRemoved(QString name,bool active);
    void showClusters(QString name,Q3ValueList<int>& clustersToShow);
    void clusterColorUpdated(QString name,int clusterId,bool active);
    void nextCluster();
    void previousCluster();
    void print(QPainter& printPainter,Q3PaintDeviceMetrics& metrics,QString filePath,bool whiteBackground);
    void newEventProvider(EventsProvider* eventsProvider,QString name,ItemColors* eventColors,bool active,
                            Q3ValueList<int>& eventsToShow,const Q3ValueList<int>& eventsToSkip);
    void eventProviderRemoved(QString name,bool active,bool lastFile);
    void showEvents(QString name,Q3ValueList<int>& eventsToShow);
    void eventColorUpdated(QString name,int eventId,bool active);
    void nextEvent();
    void previousEvent();
    void eventModified(QString providerName,int selectedEventId,double time,double newTime);
    void updateEvents(bool active,QString providerName,double time,double newTime);
    void eventToRemove();
    void eventRemoved(QString providerName,int selectedEventId,double time);
    void updateEvents(bool active,QString providerName,double time);
    void newEventProperties(QString providerName,QString eventId);
    void eventAdded(QString providerName,QString addedEventDescription,double time);
    void updateEvents(QString providerName,Q3ValueList<int>& eventsToShow,bool active);
    void waveformInformationUpdated(int nbSamplesBefore,int nbSamplesAfter,bool active);
    void positionInformationUpdated(int width, int height,QImage backgroundImage,bool newOrientation,bool active);
    void timeChanged(long start,long duration);
    void positionViewClosed();
    void clusterProviderUpdated(bool active);
    void noneBrowsingClusterListUpdated(QString providerName,const Q3ValueList<int>& clustersToNotBrowse);
    void noneBrowsingEventListUpdated(QString providerName,const Q3ValueList<int>& eventsToNotBrowse);
    void skipStatusChanged(const Q3ValueList<int>& skippedChannels);
    void decreaseTheRasterHeight();
    void increaseTheRasterHeight();
    void updateEventDisplay();
    void eventsShownInPositionView(bool shown);
    void traceBackgroundImageUpdate(QImage traceBackgroundImage,bool active);

  private:

   /**List of the presented channels.*/
   Q3ValueList<int>* shownChannels;

   /**Reference on the main window.*/
   NeuroscopeApp& mainWindow;

  /**True if the the colors are in grey-scale*/
  bool greyScaleMode;

  /** mainDock is the main DockWidget to which all other dockWidget will be dock. Inititalized in
  * the constructor.
  */
  KDockWidget* mainDock;

  /**True if the traces are displayed on multiple columns, false otherwise.*/
  bool multiColumns;

  /**True if vertical lines are displayed to show the clusters, false otherwise.*/
  bool verticalLines;

  /**True if a raster is drawn to show the clusters, false otherwise.*/
  bool raster;

  /**True if waveforms are drawn on top of the traces, false otherwise.*/
  bool waveforms;

  /**True if the selected tool is the selection one, false otherwise.*/
  bool selectMode;

  /**List containing the offset for each channel.*/
  Q3ValueList<int> channelOffsets;

  /**List of the exponents used to compute the drawing gain for each channel.
 */
  Q3ValueList<int> gains;

  /*List of the currently selected channels.*/
  Q3ValueList<int> selectedChannels;

  /**Label for the display when in tab page mode.*/
  QString tabLabel;

  /*Starting time in miliseconds.*/
  long startTime;

  /*Time window in miliseconds.*/
  long timeWindow;

  /**True if labels are drawn next to the traces, false otherwise.*/
  bool labelsDisplay;

  /**Dictionary given the list of selected clusters of each cluster provider.*/
  Q3Dict< Q3ValueList<int> > selectedClusters;

  /**Dictionary given the list of selected events of each event provider.*/
  Q3Dict< Q3ValueList<int> > selectedEvents;

  /**Dictionary given the list of clusters to not be used for browsing for each cluster provider.*/
  Q3Dict<Q3ValueList<int> > clustersNotUsedForBrowsing;

  /**Dictionary given the list of events to be not used for browsing for each event provider.*/
  Q3Dict<Q3ValueList<int> > eventsNotUsedForBrowsing;

  /**Boolean indicating whether a position file is been shown.*/
  bool isPositionFileShown;

  /**Reference on the PositionView.*/
  PositionView* positionView;

  /**Reference on the TraceWidget.*/
  TraceWidget* traceWidget;

  /**Provider for the whole set of event files.*/
  GlobalEventsProvider globalEventProvider;

  /**Boolean indicating whether events are displayed in the PositionView.*/
  bool eventsInPositionView;
 };

#endif // NEUROSCOPEVIEW_H
