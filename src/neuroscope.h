/***************************************************************************
                          neuroscope.h  -  description
                             -------------------
    begin                : Wed Feb 25 19:05:25 EST 2004
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

#ifndef NEUROSCOPE_H
#define NEUROSCOPE_H


 


#include <QDockWidget>

#include <QMenu>
//QT include files
#include <QCheckBox>

#include <QList>

#include <QTabWidget>
#include <QMainWindow>
//application specific include files
#include "neuroscopeview.h"

// forward declaration of the Neuroscope classes
class NeuroscopeDoc;
class PrefDialog;
class PropertiesDialog;
class ChannelPalette;
class ItemPalette;
class QRecentFileAction;
class QExtendTabWidget;
class QSplitter;

/**
  * The Neuroscope main window and central class. It sets up the main
  * window and reads the config file as well as providing a menubar, toolbar
  * and statusbar. There is only one document open by application.
  * @author Lynn Hazan
  */
class NeuroscopeApp : public QMainWindow
{
    Q_OBJECT

    friend class NeuroscopeView;

public:
    /** Construtor of NeuroscopeApp, calls all init functions to create the application.
     */
    explicit NeuroscopeApp();
    ~NeuroscopeApp();

    /**Opens a file, only one document at the time is allowed.
    * Asking for a new one will open a new instance of the application with it.
    */
    void openDocumentFile(const QString& url=QString());
    
    /** Returns a pointer to the current document connected to the NeuroscopeApp instance and is used by
     * the View class to access the document object's methods
     */
    NeuroscopeDoc* getDocument() const;

    /**Returns the view contains in the active display.
    * @return active view.
    */
    NeuroscopeView* activeView();

    /**Sets the current file properties.
    * (number of channels, sampling rate of the dat file and eeg file).
    * @param channelNb number of channels.
    * @param SR sampling rate.
    * @param resolution resolution of the acquisition system.
    * @param offset initial offset for all the traces.
    * @param voltageRange voltage range of the acquisition system in volts.oltage range of the acquisition system in volts.
    * @param amplification amplification of the acquisition system.
    * @param screenGain screen gain in milivolts by centimeters used to display the field potentiels.
    * @param timeWindow initial time window in miliseconds.
    */
    void setFileProperties(const QString& channelNb,const QString& SR,const QString& resolution,const QString& offset,const QString& voltageRange,
                           const QString& amplification,const QString& screenGain,const QString& timeWindow);

    /**Displays the properties which will be used for the current file
    * (number of channels, sampling rate of the dat file and eeg file). This dialog is only presented at the opening
    * of a file if there is no parameter or session file.
    * @param channelNb number of channels.
    * @param SR sampling rate of the current file.
    * @param resolution resolution of the acquisition system.
    * @param offset initial offset for all the traces.
    * @param voltageRange voltage range of the acquisition system in volts.oltage range of the acquisition system in volts.
    * @param amplification amplification of the acquisition system.
    * @param screenGain screen gain in milivolts by centimeters used to display the field potentiels.
    * @param currentNbSamples number of samples per spike waveform.
    * @param currentPeakIndex sample index corresponding to the peak of a spike waveform.
    * @param videoSamplingRate video acquisition sampling rate.
    * @param width video image width.
    * @param height video image height.
    * @param backgroundImage image used as a background for the position view.
    * @param rotation video image rotation angle.
    * @param flip video image flip orientation, 0 stands for none, 1 for vertical and 2 for horizontal.
    * @param acquisitionSystemSamplingRate acquisition system sampling.
    * @param isaDatFile true if the file being opened is a dat file (intial recorded file).
    * @param positionsBackground true if the all the positions contain in the position file have to be drawn on the background, false otherwise.
    * @param traceBackgroundImage image used as a background for the trace view.
    */
    void displayFileProperties(int channelNb,double SR,int resolution,int offset,int voltageRange,int amplification,
                               float screenGain,int currentNbSamples,int currentPeakIndex,double videoSamplingRate,
                               int width, int height, QString backgroundImage,int rotation,int flip,
                               double acquisitionSystemSamplingRate,bool isaDatFile,bool positionsBackground,QString traceBackgroundImage);

    /**Resize the panel containing the palettes.*/
    void resizePalettePanel();

    /** Creates a new display.
   * @param channelsToDisplay list of the channels to be display in the new display.
   * @param verticalLines true if vertical lines will be drawn if clusters are selected.
   * @param raster true if a raster will be drawn if clusters are selected.
   * @param waveforms true if the spike waveforms will be drawn if clusters are selected.
   * @param showLabels true if labels are displayed next to the traces, false otherwise.
   * @param multipleColumns true if the traces are diplay on several columns,false otherwise.
   * @param greyMode true if the channels are displayed in grey-scale, false otherwise.
	* @param autocenterChannels whether all channels should be autocentered around their offset.
   * @param offsets list containing the offset for each channel.
   * @param channelGains list of the exponents used to compute the drawing gain for each channel.
   * @param selectedChannels list of the selected channels.
   * @param startTime starting time in miliseconds.
   * @param duration time window in miliseconds.
   * @param rasterHeight height of the rasters in the world coordinate system.
   * @param tabLabel label for the display when in tab page mode.
   */
    void createDisplay(QList<int>* channelsToDisplay,bool verticalLines,bool raster,bool waveforms,bool showLabels,
                       bool multipleColumns,bool greyMode,bool autocenterChannels,QList<int> offsets,QList<int> channelGains,
                       QList<int> selectedChannels,long startTime,long duration,int rasterHeight,QString tabLabel = "");

    /**Creates a cluster palette and adds a group corresponding to the cluster file identified by @p clusterFileId.
   * @param clusterFileId identifier of the cluster file providing the data accessible through
   * the future palette.
   */
    void createClusterPalette(const QString &clusterFileId);

    /**Adds a group corresponding to the cluster file identified by @p clusterFileId to the existing cluster palette.
   * @param clusterFileId identifier of the cluster file providing the data accessible through
   * the future palette.
   */
    void addClusterFile(const QString &clusterFileId);

    /**Creates a event palette and adds a group corresponding to the event file identified by @p eventFileId.
   * @param eventFileId identifier of the event file providing the data accessible through
   * the future palette.
   */
    void createEventPalette(const QString &eventFileId);

    /**Adds a group corresponding to the event file identified by @p eventFileId to the existing event palette.
   * @param eventFileId identifier of the event file providing the data accessible through
   * the future palette.
   */
    void addEventFile(const QString &eventFileId);

    /**Returns the event palette.
   * @return pointer on the event palette.*/
    ItemPalette* getEventPalette();

    /**Returns the current background color.
   * @return the current background color.
   */
    inline QColor getBackgroundColor()const{return backgroundColor;}

    /**Informs the application that a position file has been loaded from the session file.*/
    void positionFileLoaded(){
        isPositionFileLoaded = true;
        slotStateChanged("positionState");
    }

    /**Tells if there are cluster files loaded.
   * @return true if at least one cluster file is loaded, false otherwise.
   */
    inline bool isClusterFilesLoaded()const{return !clusterFileList.isEmpty();}
    
    /**Tells if there is a position file loaded.
   * @return true a position file is loaded, false otherwise.
   */
    inline bool isApositionFileLoaded()const{return isPositionFileLoaded;}

	 /// Added by M.Zugaro to enable automatic forward paging
	 bool isStill() { return ( !activeView() || activeView()->isStill() ); }

private Q_SLOTS:

    /// Added by M.Zugaro to enable automatic forward paging
    void neuroscopeViewStopped() { slotStateChanged("pageOffState"); }

public Q_SLOTS:

    /// Added by M.Zugaro to enable automatic forward paging
    void page() { if ( isStill() ) slotStateChanged("pageOnState"); else slotStateChanged("pageOffState"); activeView()->page(); }
    void stop() { if( activeView() ) { activeView()->stop(); slotStateChanged("pageOffState"); } }
    void accelerate() {	if( activeView()) activeView()->accelerate(); }
    void decelerate() {	if( activeView()) activeView()->decelerate(); }

    /**Called when an event has been modified.
  * @param providerName name use to identified the event provider containing the modified event.
  * @param selectedEventId id of the modified event.
  * @param time initial time of the modified event.
  * @param newTime new time of the modified event.
  */
    void slotEventModified(const QString &providerName,int selectedEventId,double time,double newTime);

    /**Deletes the selected event.
  */
    void removeEvent();

    /**Called when an event has been removed.
  * @param providerName name use to identified the event provider containing the removed event.
  * @param selectedEventId id of the removed event.
  * @param time initial time of the removed event.
  */
    void slotEventRemoved(const QString &providerName,int selectedEventId,double time);

    /**Adds an event.
  */
    void addEvent();

    /**Called when an event file has been selected in the event palette.
  * @param eventGroupName name use to identified the selected event file.
  */
    void slotEventGroupSelected(const QString &eventGroupName);

    /**Called when an event has been added.
  * @param providerName name use to identified the event provider containing the added event.
  * @param addedEventDescription description of the added event.
  * @param time time of the added event.
  */
    void slotEventAdded(const QString &providerName, const QString &addedEventDescription, double time);

    /**Update the positions menu due to the closing of the current position view.*/
    void positionViewClosed(){positionViewToggle->setChecked(false);}

    /**Disables clusters browsing as no clusters have been selected for browsing.
  */
    void slotNoClustersToBrowse();

    /**Enables clusters browsing as some clusters have been selected for browsing.
  */
    void slotClustersToBrowse();


    /**Disables events browsing as no events have been selected for browsing.
  */
    void slotNoEventsToBrowse();

    /**Enables events browsing as some events have been selected for browsing.
  */
    void slotEventsToBrowse();

    void slotStateChanged(const QString& state);
protected:
    /** initializes the KActions of the application */
    void initActions();
    /** sets up the statusbar for the main window by initialzing a statuslabel.
     */
    void initStatusBar();

    /** Initialize the first display (create the mainDockWidget).
    * @param channelsToDisplay list of the channels to be display in the main display at start up.
    * @param autocenterChannels whether all channels should be autocentered around their offset.
    * @param offsets list containing the offset for each channel.
    * @param channelGains list of the exponents used to compute the drawing gain for each channel.
    * @param selectedChannels list of the selected channels.
    * @param skipStatus map given the skip status of the channels.
    * @param rasterHeight height of the rasters in the world coordinate system.
    * @param duration time window in miliseconds, the default is 1000.
    * @param startTime starting time in miliseconds.
    * @param tabLabel label for the display when in tab page mode.
    */
    void initDisplay(QList<int>* channelsToDisplay,bool autocenterChannels,QList<int> offsets,QList<int> channelGains,
                     QList<int> selectedChannels,QMap<int,bool>& skipStatus,int rasterHeight=-1,long duration = 1000,long startTime = 0,QString tabLabel = QString());
    
    /**
     * queryClose is called by closeEvent
     */
    bool queryClose();
    
    void customEvent (QEvent* event);
    void closeEvent(QCloseEvent *event);
    
private Q_SLOTS:

    void slotAbout();

    /**Open a file and load it into the document.*/
    void slotFileOpen();

    /**Loads one or multiple cluster files.*/
    void slotLoadClusterFiles();

    /**Closes the cluster file corresponding to the currently selected cluster group.*/
    void slotCloseClusterFile();
    
    /**Loads one or multiple event files.*/
    void slotLoadEventFiles();

    /**Creates an empty event file.*/
    void slotCreateEventFile();
    
    /**Closes the event file corresponding to the currently selected event group.*/
    void slotCloseEventFile();

    /**Loads a position file to display the position of the animal in a new view.*/
    void slotLoadPositionFile();

    /**Closes the position file.*/
    void slotClosePositionFile();
    
    /**Opens a file from the recent files menu. */
    void slotFileOpenRecent(const QString& url);

    /**Opens a dialog to display and edit the file properties:
    * (number of channels, sampling rate of the dat file and eeg file).
    */
    void slotFileProperties();
    
    /**If need it, warns the user that the spike groups have been modified, then closes the actual file and displayss.*/
    void slotFileClose();

    /**Print the current display. */
    void slotFilePrint();

    /**Closes all open windows by calling close() on each memberList item until the list is empty, then quits the application.
     * If queryClose() returns false because the user canceled the saveModified() dialog, the closing breaks.
     */
    void slotFileQuit();

    /** Toggles the main tool bar.*/
    void slotViewMainToolBar();

    /**Toggles the bar for the tools.
     */
    void slotViewToolBar();

    /**Toggles the statusbar.
     */
    void slotViewStatusBar();


    /** Toggles the calibration bar. This bar is meaningful only when all the channels
    *  have the same amplification.
    */
    void slotShowCalibration();
    
    /**Changes the statusbar contents for the standard label permanently, used to indicate current actions.
     * @param text the text that is displayed in the statusbar.
     */
    void slotStatusMsg(const QString &text);


    /*Slots for the tools menu.*/
    /**Chooses the tool to select channels, enabling the user to select traces in order to move them.*/
    void slotSelect();
    
    /**Chooses the zoom tool, enabling the user to zoom.*/
    void slotZoom();

    /**Chooses the measure tool, enabling the user to compute the voltage and the time between two points.
    * The voltage is meaningful only for the channel on which the initial click has be done.*/
    void slotMeasure();

    /**Chooses the selection time tool, enabling the user to select a time frame (subset of the currently shown)
    * for which the traces are going to be displayed.*/
    void slotSelectTime();
    
    /**Chooses the tool to select an event, enabling the user to select an event in order to move or delete it.*/
    void slotSelectEvent();

    /** Executes the preferences dialog.*/
    void executePreferencesDlg();

    /** Updates the widgets so that new user settings take effect.*/
    void applyPreferences();

    /**Initializes some of the variables defined in the settings (preferences).*/
    void initializePreferences();

    /**Changes the color of a channel.
    * @param channelId id of the channel which has had its color changed.
    */
    void slotSingleChannelColorUpdate(int channelId);

    /**Changes the color of a group of channels.
    * @param groupId id of the group for which the color have been changed.
    */
    void slotChannelGroupColorUpdate(int groupId);

    /**Changes the color of selected channels.
    * @param selectedChannels ids of the channels  which have had its color changed.
    */
    //void slotChannelsColorUpdate(QValueList<int> selectedChannels);

    /**Sets the color of the channels to the color of their display group.*/
    void slotApplyDisplayColor();

    /**Sets the color of the channels to the color of their spike group.*/
    void slotApplySpikeColor();

    /**Draws the channels contain in @p selectedChannels list.
    * @param selectedChannels list of channels which have been selected to be shown.
    */
    void slotUpdateShownChannels(const QList<int>& selectedChannels);

    /**Updates the show/hide status of the channels contain in @p selectedChannels list.
    * @param hiddenChannels list of channels which have been selected to be hidden.
    */
    void slotUpdateHiddenChannels(const QList<int>& hiddenChannels);

    /**Creates the default set up: one display starting at the begining of the file and all the channels selected.
    * @param channelDefaultOffsets map given the channel default offsets.
    * @param skipStatus map given the skip status of the channels.
    */
    void slotDefaultSetUp(QMap<int,int>& channelDefaultOffsets,QMap<int,bool>& skipStatus);

    /**Creates the initial set up using the session file.
    * @param channelsToDisplay list of the channels to show.
    * @param verticalLines true if vertical lines will be drawn if clusters are selected.
    * @param raster true if a raster will be drawn if clusters are selected.
    * @param waveforms true if the spike waveforms will be drawn if clusters are selected.
    * @param showLabels true if labels are displayed next to the traces, false otherwise.
    * @param multipleColumns true if the traces are diplay on several columns,false otherwise.
    * @param greyMode true if the channels are displayed in grey-scale, false otherwise.
    * @param autocenterChannels whether all channels should be autocentered around their offset.
    * @param offsets list containing the offset for each channel.
    * @param channelGains list of the exponents used to compute the drawing gain for each channel.
    * @param selectedChannels list of the selected channels.
    * @param skipStatus map given the skip status of the channels.
    * @param startTime starting time in miliseconds.
    * @param duration time window in miliseconds.
    * @param tabLabel label for the display when in tab page mode.
    * @param positionView true if a position view in shown in the display, false otherwise.
    * @param rasterHeight height of the rasters in the world coordinate system.
    * @param showEventsInPositionView 1 if events are displayed in the PositionView, 0 otherwis.
    */
    void slotSetUp(QList<int>* channelsToDisplay,bool verticalLines,bool raster,bool waveforms,bool showLabels,bool multipleColumns,
                   bool greyMode,bool autocenterChannels,QList<int> offsets,QList<int> channelGains,QList<int> selectedChannels,QMap<int,bool>& skipStatus,
                   long startTime,long duration,const QString &tabLabel,bool positionView,int rasterHeight,bool showEventsInPositionView);

    /**All the channels of the current display are display either in a gradation of grey or in color.*/
    void slotSetGreyScale();

    /**Creates of an empty group of channels.*/
    void slotCreateGroup();

    /**Select all the channels.*/
    void slotSelectAll();

    /**Deselect all the channels.*/
    void slotDeselectAll();

    /**Selects all the clusters of the current palette except the clusters of artefact and noise
    * (clusters 0 and 1 respectively).*/
    void slotSelectAllWO01();
    
    /**Sets the mode of presentation in the current display, single or multiple columns.*/
    void slotDisplayMode();

    /**Displays or hides vertical lines to show the clusters.*/
    void slotClustersVerticalLines();
    
    /**Displays or hides a raster to show the clusters.*/
    void slotClustersRaster();

    /**Displays or hides the cluster waveforms on top of the traces.*/
    void slotClustersWaveforms();

    /**Triggers the moves of the selected channels to the trash group.*/
    void slotDiscardChannels();

    /**Triggers the moves of the selected channels to the discard spike group.*/
    void slotDiscardSpikeChannels();
    
    /**The selected channels have been moved to the trash group.
    * @param discarded ids of the channels to move to the trash group.
    */
    void slotChannelsDiscarded(const QList<int>& discarded);

    /**Shows the selected channels.*/
    void slotShowChannels();

    /**Hides the selected channels.*/
    void slotHideChannels();

    /**Updates the palettes when the active display changes.*/
    void slotTabChange(int index);

    /**Updates internal variables when the active palettechanges.*/
    void slotPaletteTabChange(int index);

    /** Closes the display and if it is the last one closes the actual file and window.*/
    void slotDisplayClose();

    /**Launches a dialog to enable the user to change the tab label of the active display.*/
    void slotRenameActiveDisplay();

    /** Creates a new display.
    */
    void slotNewDisplay();

    /**Enables or disables the edition of the groups.*/
    void slotEditMode();

    /**Synchronize the two channel palettes.*/
    void slotSynchronize();

    /**Update @p modified to keep track of spike group modification.*/
    void slotGroupsModified();

    /**Selects the channels in the active channel palette or in both if none is active.
   *@param selectedIds ids of the selected channels.
   */
    void slotSelectChannelsInPalette(const QList<int>& selectedIds);

    /**Triggers the increase of the amplitude of all the channels.
   */
    void slotIncreaseAllChannelsAmplitude(){activeView()->increaseAllChannelsAmplitude();}

    /**Triggers the decrease of the amplitude of all the channels.
   */
    void slotDecreaseAllChannelsAmplitude(){activeView()->decreaseAllChannelsAmplitude();}

    /**Triggers the increase of the amplitude of the selected channels.
   */
    void slotIncreaseSelectedChannelsAmplitude();

    /**Triggers the decrease of the amplitude of the selected channels.
   */
    void slotDecreaseSelectedChannelsAmplitude();

    /**Update the active display list of selected channels. If the selection tool is selected,
   * selects the channels in the active TraceView.
   *@param selectedIds ids of the selected channels.
   */
    void slotChannelsSelected(const QList<int>& selectedIds);

    /**Resets the offset of the selected channels.*/
    void slotResetOffsets();

    /**Resets the gain of the selected channels.*/
    void slotResetGains();

    /**Saves the current session, spike, cluster, event files opened and the selected clusters and events.
   */
    void saveSession();

    /** Save the current session, spike, cluster, event files opened and the selected clusters and events with
    a new name.
   */
    void slotSessionSaveAs();

    /**Enables or disables automatic channel centering around offsets.*/
    void slotAutocenterChannels();

    /**Enables or disables the display of labels next to the traces.*/
    void slotShowLabels();

    /**Changes the color of a cluster contained in the cluster file identified by @p groupName.
   * @param clusterId id of the cluster which has had its color changed.
   * @param groupName identifier of the file containing the cluster to update.
   */
    void slotClusterColorUpdate(int clusterId, const QString &groupName, const QColor &color);

    /**Updates the active display with the clusters selected in the cluster palette.
   *@param selection map given the list of the selected clusters by cluster file identified.
   */
    void slotUpdateShownClusters(const QMap<QString,QList<int> >& selection);

    /**Retrieves the next cluster.*/
    void slotShowNextCluster();

    /**Retrieves the previous cluster.*/
    void slotShowPreviousCluster();

    /**Changes the color of a event contained in the event file identified by @p groupName.
   * @param eventId id of the event which has had its color changed.
   * @param groupName identifier of the file containing the event to update.
   */
    void slotEventColorUpdate(int eventId, const QString &groupName, const QColor &color);

    /**Updates the active display with the events selected in the event palette.
   *@param selection map given the list of the selected events by event file identified.
   */
    void slotUpdateShownEvents(const QMap<QString,QList<int> >& selection);

    /**Retrieves the next event.*/
    void slotShowNextEvent();

    /**Retrieves the previous event.*/
    void slotShowPreviousEvent();

    /** Reverts the last user action.*/
    void slotUndo();

    /** Reverts the last undo action.*/
    void slotRedo();

    /**Creates the list of available events to used in order to create a new event.*/
    void slotAddEventAboutToShow();


    /** Stores which type of event to use for the new creation of events.
   * Called after a click on the actionbar.
   * @param index currently checked item in the addEvent submenu.
   */
    void slotAddEventButtonActivated(QAction * act );

    /**Shows or hides the position view in the current display.*/
    void slotShowPositionView();

    /**Updates the active display with the events to skip while browsing.
   * @param groupName identifier of the file containing the events to browse.
   * @param eventsToSkip new list of events to skip while browsing
   */
    void slotUpdateEventsToSkip(const QString &groupName, const QList<int>& eventsToSkip);

    /**Updates the active display with the clusters to skip while browsing.
   * @param groupName identifier of the file containing the clusters to browse.
   * @param clustersToSkip new list of clusters to skip while browsing
   */
    void slotUpdateClustersToSkip(const QString &groupName, const QList<int>& clustersToSkip);
    
    /**Marks the selected channels has keeped.*/
    void slotKeepChannels();

    /**Marks the selected channels has skipped.*/
    void slotSkipChannels();

    /*Sets the current channel offsets has the default offsets.*/
    void slotSetDefaultOffsets();

    /*Resets the channel default offsets to zero.*/
    void slotResetDefaultOffsets();

    /**Draws a vertical line at the cursor position. If the traces are displayed on multiple columns, a line is drawn in each column.
    The line(s) only last until the user release the mouse.
   */
    void slotDrawTimeLine();

    /**Increases the height of the rasters.*/
    void slotIncreaseRasterHeight();

    /**Decreases the height of the rasters.*/
    void slotDecreaseRasterHeight();

    /**Enables or disables the display of events in the PositionView.*/
    void slotShowEventsInPositionView();

    void slotHanbook();

    void slotSaveRecentFiles();

private:
    void readSettings();
    void initView();

    QSplitter *mainSplitter;

    /** Doc represents your actual document and is created only once. It keeps
     * information such as filename and does the serialization of your files.
     */
    NeuroscopeDoc *doc;

    /**Settings dialog.*/
    PrefDialog* prefDialog;

    //Action and toolbar pointers
    QAction* fileOpenRecent;
    QAction* viewMainToolBar;
    QAction* viewToolBar;
    QAction* greyScale;
    QAction* displayMode;
    QAction* clusterVerticalLines;
    QAction* clusterRaster;
    QAction* clusterWaveforms;
    QAction* editMode;
    QAction* autocenterChannels;
    QAction* showHideLabels;
    QAction* calibrationBar;
    QMenu* addEventPopup;
    QAction* addEventToolBarAction;
    QAction* positionViewToggle;
    QAction* showEventsInPositionView;
    
    QAction* mProperties;
    QAction* mLoadClusterFiles;
    QAction* mLoadEventFiles;
    QAction* mLoadPositionFile;
    QAction* mCreateEventFile;
    QAction* mCloseCluster;
    QAction* mCloseEvent;
    QAction* mClosePositionFile;
    QAction* mSelectAll;
    QAction* mDeselectAll;
    QAction* mZoomTool;
    QAction* mSelectTool;
    QAction* mMeasureTool;
    QAction* mTimeTool;
    QAction* mEventTool;
    QAction* mDrawTimeLine;
    QAction* mPage;
    QAction* mAccelerate;
    QAction* mDecelerate;
    QAction* mNewDisplay;
    QAction* mRenameActiveDisplay;
    QAction* mCloseActiveDisplay;
    QAction* mShowChannel;
    QAction* mHideChannel;
    QAction* mMoveToNewGroup;
    QAction* mRemoveChannelFromGroup;
    QAction* mDiscardChannels;
    QAction* mKeepChannels;
    QAction* mSkipChannels;
    QAction* mSynchronizeGroups;
    QAction* mColorAnatomicalGroups;
    QAction* mColorSpikeGroups;
    QAction* mIncreaseHeight;
    QAction* mDecreaseHeight;
    QAction* mNextSpike;
    QAction* mPreviousSpike;
    QAction* mNextEvent;
    QAction* mPreviousEvent;
    QAction* mRemoveEvent;
    QAction* mIncreaseAllChannelAmplitudes;
    QAction* mDecreaseAllChannelAmplitudes;
    QAction* mIncreaseSelectedChannelAmplitude;
    QAction* mDecreaseSelectedChannelAmplitude;
    QAction* mResetSelectedChannel;
    QAction* mResetSelectedChannelAmplitudes;
    QAction* mSetCurrentOffsetsAsDefault;
    QAction* mSetDefaultOffsetToZero;
    QAction* mSelectAllExcept0And1;
    QAction* mPrintAction;
    QAction* mQuitAction;
    QAction* mSaveAction;
    QAction* mSaveAsAction;
    QAction* mCloseAction;
    QAction* mOpenAction;
    QAction* mUndo;
    QAction* mRedo;
    QAction* mViewStatusBar;
    QAction* mPreferenceAction;

    QToolBar *mMainToolBar;
    QToolBar *mToolBar;
    QToolBar *mChannelToolBar;
    QToolBar *mEventToolBar;
    QToolBar *mClusterToolBar;

    QRecentFileAction *mFileOpenRecent;
    /** A counter that gets increased each time the user creates a new display of the document with "Displays"->"New ...".*/
    int displayCount;

    /** mainDock is the main DockWidget to which all other dockWidget will be dock. Inititalized in
     * initDisplay()
     */
    DockArea* mainDock;

    /**displayChannelPalette is the Widget containing the channel list used to specify the traces display.
    * Inititalized in initItemPanel().
    */
    ChannelPalette* displayChannelPalette;
    
    /**spikeChannelPalette is the Widget containing the channel list to create the spike groups.
    * Inititalized in initItemPanel().
    */
    ChannelPalette* spikeChannelPalette;

    /**tabsParent groups all the display tabs, it is updated eache time a display is added.
    * It is null when there is only one display open. It enables to get the active tab.
    */
    QExtendTabWidget* tabsParent;

    /**paletteTabsParent groups all the palettes tabs, it is updated eache time a palette is added or removed.
    * It enables to get the active palette.
    */
    QTabWidget* paletteTabsParent;

    /**Panel container (contains a KDockArea) for all the palettes of the application (ClusterPalette,EventPalette,channelsPalette).
    * The main Palette is the channel palette
    * use to specify the display order of the field potentials.*/
    //QDockWidget* palettePanel;

    /**Boolean used to prevent the trigger of changes during initialization.*/
    bool isInit;

    /**Default number of channels.*/
    int channelNbDefault;

    /**Default sampling rate of the dat file.*/
    double datSamplingRateDefault;

    /**Default sampling rate of the EEG file.*/
    double eegSamplingRateDefault;

    /**Default screen gain in milivolts by centimeters used to display the field potentiels.*/
    float screenGainDefault;

    /**Default voltage range of the acquisition system in volts.*/
    int voltageRangeDefault;

    /**Default amplification of the acquisition system.*/
    int amplificationDefault;

    /**Flag to keep track of group modifications. */
    bool groupsModified;
    
    /**Flag to keep track of color modifications. */
    bool colorModified;
    
    /**Flag to keep track of event modifications. */
    bool eventsModified;

    /**Default initial offset for all the traces.*/
    int initialOffsetDefault;

    /**Default resolution of the acquisition system.*/
    int resolutionDefault;

    /**Properties dialog.*/
    PropertiesDialog* propertiesDialog;

    /**Background color for the views.*/
    QColor backgroundColor;

    /**True if the selection tool has been selected.*/
    bool select;

    /**The path of the currently open document.*/
    QString filePath;

    /**Initial time window in miliseconds read on the command line.*/
    long initialTimeWindow;

    /**List storing the identifiers of the opened cluster files.*/
    QStringList clusterFileList;
    
    /**List storing the identifiers of the opened event files.*/
    QStringList eventFileList;

    /**Boolean indicating if the headers of the palettes have to be diplayed.*/
    bool displayPaletteHeaders;

    /**Represents the event position, in percentage from the begining of the window, where the events are display when browsing.*/
    int eventPosition;

    /**Represents the cluster position, in percentage from the begining of the window, where the clusters are display when browsing.*/
    int clusterPosition;

    /**The current number of undo used to enable/disable the the undo action.*/
    int currentNbUndo;

    /**The current number of redo used to enable/disable the the redo action.*/
    int currentNbRedo;

    /**Label corresponding to the type of event to create at the next addEvent.*/
    QString eventLabelToCreate;

    /**Identifier of the event file to which a new event will be added.*/
    QString eventProvider;

    /**Boolean indicating that an undo or redo action is in process.*/
    bool undoRedoInprocess;

    /**Number of samples per spike waveform*/
    int nbSamplesDefault;
    /**Index of the peak sample in the spike waveform.*/
    int peakIndexDefault;

    /**Default video acquisition sampling rate.*/
    double videoSamplingRateDefault;

    /**Video image width.*/
    int videoWidthDefault;
    /**Video image height.*/
    int videoHeightDefault;
    /**Background image for the position view.*/
    QString backgroundImageDefault;

    /**Background image for the trace view.*/
    QString traceBackgroundImageDefault;

    /**Angle of rotation of the video records.*/
    int rotationDefault;
    /**Flip horientation of the video records.*/
    int flipDefault;

    /**Boolean indicating whether a position file has been loaded.*/
    bool isPositionFileLoaded;

    /**True if the all the positions contain in the position file have to be drawn on the background image, false otherwise.*/
    bool drawPositionsOnBackgroundDefault;

    //Functions

    /** Creates the palette of items (left tool view).*/
    void initItemPanel();

    /**Resets the state of the application to a none document open state.*/
    void resetState();

    /**Loads the cluster files and creates the corresponding groups in the cluster palette.
   * @param urls file list to be opened.
   */
    void loadClusterFiles(const QStringList& urls);

    /**Loads the event files and creates the corresponding groups in the cluseventer palette.
   * @param urls file list to be opened.
   */
    void loadEventFiles(const QStringList& urls);

    /**Loads the position file and creates the position view in the current display.
   * @param url file to be opened.
   */
    void loadPositionFile(const QString& url);

    /**Updates the spike and event browsing status.*/
    void updateBrowsingStatus();

    bool useWhiteColorDuringPrinting;

    QAction *actNewEvent;
};

Q_DECLARE_METATYPE(QList<int>*)

#endif // NEUROSCOPE_H
