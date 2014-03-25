/***************************************************************************
                          neuroscopedoc.h  -  description
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

#ifndef NEUROSCOPEDOC_H
#define NEUROSCOPEDOC_H


// include files for QT
#include <QObject>
#include <QString>
#include <QPair>

#include <QList>

#include <QEvent>




//include files for the application
#include "channelpalette.h"
#include "dataprovider.h"
#include "eventsprovider.h"


// forward declaration of the Neuroscope classes
class NeuroscopeView;
class NeuroscopeApp;
class ChannelColors;
class TracesProvider;
class NeuroscopeXmlReader;
class ItemColors;
class ItemPalette;

/**
  * The NeuroscopeDoc class provides a document object that can be used in conjunction with the classes
  * NeuroscopeApp and NeuroscopeView to create a document-view model for MDI (Multiple Document Interface)
  * based on KApplication and KDockMainWindow as main classes.
  * Thereby, the document object is created by the NeuroscopeApp instance and contains
  * the document structure with the according methods for manipulating the document
  * data by NeuroscopeView objects. Also, NeuroscopeDoc contains the methods for serialization of the document data
  * from and to files.
  * @author Lynn Hazan
  */
class NeuroscopeDoc : public QObject
{
    Q_OBJECT
public:

    /**Information retun after a call to openFile/saveDocument/createFeatureFile*/
    enum OpenSaveCreateReturnMessage {OK=0,OPEN_ERROR=1,DOWNLOAD_ERROR=3,INCORRECT_FILE=4,SAVE_ERROR=5,
                                      UPLOAD_ERROR=6,INCORRECT_CONTENT=7,CREATION_ERROR=8,PARSE_ERROR=9,MISSING_FILE=10,
                                      ALREADY_OPENED=11,NOT_WRITABLE=12};

    /** Constructs a document.
    * @param parent the parent QWidget.
    * @param displayChannelPalette a reference to the channel palette used to specify the traces display.
    * @param spikeChannelPalette a reference to the channel palette used to create the spike groups.
    * @param channelNbDefault default number of channels in the settings.
    * @param datSamplingRateDefault default sampling rate of the dat file in the settings.
    * @param eegSamplingRateDefault default sampling rate of the EEG file in the settings.
    * @param initialOffset initial default offset for all the field potentials.
    * @param voltageRangeDefault voltage range of the acquisition system in volts.
    * @param amplificationDefault default amplification of the acquisition system.
    * @param screenGainDefault default screen gain in milivolts by centimeters used to display the field potentiels
    * @param resolutionDefault default resolution of the acquisition system.
    * @param eventPosition represents the event position, in percentage from the begining of the window, where the events are display when browsing.
    * @param clusterPosition represents the cluster position, in percentage from the begining of the window, where the clusters are display when browsing.
    * @param nbSamples number of samples per spike waveform.
    * @param peakSampleIndex sample index corresponding to the peak of a spike waveform.
    * @param videoSamplingRate video acquisition sampling rate.
    * @param width video image width.
    * @param height video image height.
    * @param backgroundImage image used as a background for the position view.
    * @param traceBackgroundImage image used as a background for the trace view.
    * @param rotation video image rotation angle.
    * @param flip video image flip orientation, 0 stands for none, 1 for vertical and 2 for horizontal.
    * @param positionsBackground true if the all the positions contain in the position file have to be drawn on the background, false otherwise.
   */
    NeuroscopeDoc(QWidget* parent,ChannelPalette& displayChannelPalette,ChannelPalette& spikeChannelPalette,int channelNbDefault,double datSamplingRateDefault,
                  double eegSamplingRateDefault, int initialOffset,int voltageRangeDefault,int amplificationDefault,float screenGainDefault,int resolutionDefault,
                  int eventPosition,int clusterPosition,int nbSamples, int peakSampleIndex,double videoSamplingRate, int width, int height, const QString& backgroundImage,
                  const QString& traceBackgroundImage,
                  int rotation,int flip,bool positionsBackground);
    /** Destructor for the fileclass of the application. */
    ~NeuroscopeDoc();

    /**Adds a view to the document which represents the document contents. */
    void addView(NeuroscopeView* view);

    /**Removes a view from the list of currently connected views.*/
    void removeView(NeuroscopeView* view);

    /**Closes the actual document.*/
    void closeDocument();

    /** Opens the document by filename.
    * @param url url of the file to open (dat file or eeg file).
    * @return an OpenSaveCreateReturnMessage enum giving the open status.
    */
    int openDocument(const QString& url);

    /**Saves the current session: displays, spike, cluster, event files opened and selected clusters and events.
    * It also saves the relevant changes in the parameter files (creating one if there is none).
     @return an OpenSaveCreateReturnMessage enum giving the saving status.
    */
    OpenSaveCreateReturnMessage saveSession();

    /**Saves the event files.
    * @return an OpenSaveCreateReturnMessage enum giving the save status.
    */
    bool saveEventFiles();

    /**Saves the current session: displays, spike, cluster, event files opened and selected clusters and events.
    * @param newSessionUrl new url where to write the session information to.
    @return an OpenSaveCreateReturnMessage enum giving the saving status.
    */
    OpenSaveCreateReturnMessage saveSession(const QString& newSessionUrl){
        sessionUrl = newSessionUrl;
        return saveSession();
    }

    /**Creates a new event file.
    * @param eventUrl url of the new event file.
    * @param activeView the view in which the change has to be immediate.
    @return an OpenSaveCreateReturnMessage enum giving the creation status.
    */
    OpenSaveCreateReturnMessage createEventFile(const QString &eventUrl, NeuroscopeView*activeView);

    /**Returns the QString of the document. */
    const QString& url() const;


    /**Changes the color of a channel.
    * @param channelId id of the channel to redraw.
    * @param activeView the view in which the change has to be immediate.
    */
    void singleChannelColorUpdate(int channelId,NeuroscopeView* activeView);

    /**Changes the color of a cluster.
    * @param providerName identifier of the cluster provider containing the updated cluster.
    * @param clusterId id of the cluster to redraw.
    * @param activeView the view in which the change has to be immediate.
    */
    void clusterColorUpdate(const QString &providerName,int clusterId,NeuroscopeView* activeView, const QColor&);

    /**Changes the color of a event.
    * @param providerName identifier of the event provider containing the updated cluster.
    * @param eventId id of the event to redraw.
    * @param activeView the view in which the change has to be immediate.
    */
    void eventColorUpdate(const QColor &color, const QString &providerName, int eventId, NeuroscopeView* activeView);

    /**Changes the color of a group of channels.
    * @param groupId id of the group for which the color have been changed.
    * @param activeView the view in which the change has to be immediate.
    */
    void channelGroupColorUpdate(int groupId,NeuroscopeView* activeView);

    /**Changes the color of selected channels.
    * @param selectedChannels ids of the channels  which have had its color changed.
    * @param activeView the view in which the change has to be immediate.
    */
    //void channelsColorUpdate(QValueList<int>selectedChannels,NeuroscopeView& view);

    /**Triggers the update of the displays due to a change in the display groups. Only the active display
    * is updated immediately.
    * @param activeView the view in which the change has to be immediate.
    */
    void groupsModified(NeuroscopeView* activeView);

    /**Updates the background color used in the views.
   * @param backgroundColor color of the new background.
   */
    void setBackgroundColor(const QColor &backgroundColor);

    /**Updates the background image used in the trace views.
   * @param backgroundImagePath path of the image for the traces background.
   */
    void setTraceBackgroundImage(const QString &traceBackgroundImagePath);

    /**Sets the initial offset for all the traces for the current document.*/
    void setInitialOffset(int offset);

    /**Sets the various gains.
   * @param voltageRange voltage range of the acquisition system in volts.oltage range of the acquisition system in volts.
   * @param amplification amplification of the acquisition system.
   * @param screenGain screen gain in milivolts by centimeters used to display the field potentiels.
   */
    void setGains(int voltageRange,int amplification,float screenGain);

    /**Sets the resolution of the acquisition system of the current document.
   * @param resolution current resolution.
   */
    void setResolution(int resolution);

    /**Sets the sampling rate for the current document.
   * @param rate sampling rate.
   */
    void setSamplingRate(double rate);

    /**Sets the acquisition system sampling rate. This function is call only if the current opened
   * file is not a dat file.
   * @param rate sampling rate.
   */
    void setAcquisitionSystemSamplingRate(double rate);

    /**Sets the number of channels for the current document.
   * @param nb current number of channels.
   */
    void setChannelNb(int nb);

    /**Gets the initial offset for all the traces for the current document.
   * @return initial offset.
   */
    int getInitialOffset()const{return initialOffset;}

    /**Gets the acquisition system gains.
   * @return current acquisition gain.
   */
    int getAcquisitionGain()const{return acquisitionGain;}

    /**Gets the current gain based on the screen gain and the acquisition system gain.
   * @return current gain.
   */
    int getGain()const{return gain;}

    /**Gets the voltage range of the acquisition system in volts for the current document.
   * @return current voltage range.
   */
    int getVoltageRange()const{return voltageRange;}

    /**Gets the amplification of the acquisition system for the current document.
   * @return current amplification.
   */
    int getAmplification()const{return amplification;}

    /**Gets the screen gain in milivolts by centimeters used to display the field potentiels for the current document.
   * @return current screen gain.
   */
    float getScreenGain()const{return screenGain;}

    /**Gets the resolution of the acquisition system for the current document.
   * @return current resolution.
   */
    int getResolution()const{return resolution;}

    /**Gets the sampling rate  for the current document.
   * @return current sampling rate.
   */
    double getSamplingRate()const{return samplingRate;}


    /**Gets the acquisition system sampling rate.
   * @return acquisition system  sampling rate.
   */
    double getAcquisitionSystemSamplingRate()const{return datSamplingRate;}

    /**All the positions contained in a position file can be used to create a background image for the PositionView.
   * The value return by this function tells if such background has to be created.
   * @return true if the all the positions contain in the position file have to be drawn on the background, false otherwise.
   */
    bool getPositionsBackground()const{return drawPositionsOnBackground;}


    /**Sets the various default gains.
   * @param voltageRangeDefault default voltage range of the acquisition system in volts.
   * @param amplificationDefault default amplification of the acquisition system.
   * @param screenGainDefault default screen gain in milivolts by centimeters used to display the field potentiels.
   */
    void setDefaultGains(int voltageRangeDefault,int amplificationDefault,float screenGainDefault){
        acquisitionGainDefault = static_cast<int>(0.5 +
                                                  static_cast<float>(pow(static_cast<double>(2),static_cast<double>(resolutionDefault))
                                                                     / static_cast<float>(voltageRangeDefault * 1000))
                                                  * amplificationDefault);

        gainDefault = static_cast<int>(0.5 + screenGainDefault * acquisitionGainDefault);
        this->voltageRangeDefault = voltageRangeDefault;
        this->amplificationDefault = amplificationDefault;
        this->screenGainDefault = screenGainDefault;
    }

    /**Sets the voltage range of the acquisition system in volts for the current document.
   * @param range current voltage range.
   */
    void setVoltageRange(int range){
        voltageRange = range;
        acquisitionGain = static_cast<int>(0.5 +
                                           static_cast<float>(pow(static_cast<double>(2),static_cast<double>(resolution))
                                                              / static_cast<float>(voltageRange * 1000))
                                           * amplification);

        gain = static_cast<int>(0.5 + screenGain * acquisitionGain);
    }

    /**Sets the amplification of the acquisition system for the current document.
   * @param amplification current amplification.
   */
    void setAmplification(int amplification){
        this->amplification = amplification;
        acquisitionGain = static_cast<int>(0.5 +
                                           static_cast<float>(pow(static_cast<double>(2),static_cast<double>(resolution))
                                                              / static_cast<float>(voltageRange * 1000))
                                           * amplification);

        gain = static_cast<int>(0.5 + screenGain * acquisitionGain);
    }

    /**Sets the screen gain in milivolts by centimeters used to display the field potentiels for the current document.
   * @param gain current screen gain.
   */
    void setScreenGain(float gain){
        screenGain = gain;
        acquisitionGain = static_cast<int>(0.5 +
                                           static_cast<float>(pow(static_cast<double>(2),static_cast<double>(resolution))
                                                              / static_cast<float>(voltageRange * 1000))
                                           * amplification);

        gain = static_cast<int>(0.5 + screenGain * acquisitionGain);
    }

    /**Sets the default initial offset for all the traces.
   * @param offset initial offset.
   */
    void setDefaultInitialOffset(int offset){initialOffsetDefault = offset;}

    /**Sets the default resolution of the acquisition system.
   * @param resolution default resolution.
   */
    void setDefaultResolution(int resolution){resolutionDefault = resolution;}

    /**Sets the default sampling rate of the EEG file.
   * @param rate default sampling rate.
   */
    void setDefaultEegSamplingRate(double rate){eegSamplingRateDefault = rate;}

    /**Sets the default sampling rate of the dat file.
   * @param rate default sampling rate.
   */
    void setDefaultDatSamplingRate(double rate){datSamplingRateDefault = rate;}

    /**Sets the default number of channels.
   * @param nb default number of channels.
   */
    void setDefaultChannelNb(int nb){channelNbDefault = nb;}

    /**Sets the default background image for the trace view.
   * @param traceBackgroundImagePath background image.
   */
    void setDefaultTraceBackgroundImage(QString traceBackgroundImagePath){
        traceBackgroundImageDefault = traceBackgroundImagePath;
    }

    /**Sets that some of the properties of the current document were provided on the command line.*/
    void propertiesFromCommandLine(){isCommandLineProperties = true;}

    /**Returns a pointer on the list of ItemColor objects used to represent the channel colors.
   * @return ChannelColors containing the information on the channels and their associated color.
   */
    ChannelColors* channelColors() const {return channelColorList;}

    /**Returns a reference on the DataProvider containing the information on the traces (TracesProvider).
   * @return TracesProvider object.
   */
    TracesProvider& tracesDataProvider() const {return *tracesProvider;}

    /**Returns a reference on the Map given the correspondance between the channel ids and the display group ids.
   */
    QMap<int,int>* getDisplayChannelsGroups() {return &displayChannelsGroups;}

    /**Returns a reference on the map given th correspondance between the display group ids and the channel ids.
   */
    QMap<int, QList<int> >* getDisplayGroupsChannels() {return &displayGroupsChannels;}

    /**Returns a reference on the Map given the correspondance between the channel ids and the spike group ids.
   */
    QMap<int,int>* getChannelsSpikeGroups() {return &channelsSpikeGroups;}

    /**Returns a reference on the map given th correspondance between the spike group ids and the channel ids.
   */
    QMap<int, QList<int> >* getSpikeGroupsChannels() {return &spikeGroupsChannels;}

    /**Selects all the channels and shows them if the edit mode is not selected.
   * @param activeView the view in which the change has to be immediate.
   * @param editMode true if the edit mode is selected, false otherwise.
   */
    void selectAllChannels(NeuroscopeView& activeView,bool editMode);

    /**Deselects all the channels and hides them if the edit mode is not selected.
   * @param activeView the view in which the change has to be immediate.
   * @param editMode true if the edit mode is selected, false otherwise.
   */
    void deselectAllChannels(NeuroscopeView& activeView,bool editMode);

    /**The two channel palettes are synchronized.
   */
    void synchronize();

    /**Updates the properties for the current file
   * (number of channels, sampling rate of the dat file and eeg file). This function is only called at the opening
   * of a file if the user has changed the default or command line values.
   * @param channelNb number of channels.
   * @param SR sampling rate of the current file..
   * @param resolution resolution of the acquisition system.
   * @param offset initial offset for all the traces.
   * @param voltageRange voltage range of the acquisition system in volts.
   * @param amplification amplification of the acquisition system.
   * @param screenGain screen gain in milivolts by centimeters used to display the field potentiels.
   * @param newNbSamples number of samples per spike waveform.
   * @param newPeakSampleIndex sample index corresponding to the peak of a spike waveform.
   * @param videoSamplingRate video acquisition sampling rate.
   * @param width video image width.
   * @param height video image height.
   * @param backgroundImage image used as a background for the position view.
   * @param traceBackgroundImage image used as a background for the trace view.
   * @param rotation video image rotation angle.
   * @param flip video image flip orientation, 0 stands for none, 1 for vertical and 2 for horizontal.
   * @param acquisitionSystemSamplingRate acquisition system sampling.
   * @param positionsBackground true if the all the positions contain in the position file have to be drawn on the background, false otherwise.
   */
    void updateFileProperties(int channelNb,double SR,int resolution,int offset,int voltageRange,int amplification,
                              float screenGain,int newNbSamples,int newPeakSampleIndex,double videoSamplingRate,
                              int width, int height, const QString& backgroundImage, const QString& traceBackgroundImage,int rotation,int flip,double acquisitionSystemSamplingRate,
                              bool positionsBackground){
        this->channelNb = channelNb;
        if(extension != "dat")
            samplingRate = SR;
        else
            samplingRate = acquisitionSystemSamplingRate;
        datSamplingRate = acquisitionSystemSamplingRate;
        this->voltageRange = voltageRange;
        this->amplification = amplification;
        this->screenGain = screenGain;
        this->resolution = resolution;
        initialOffset = offset;
        nbSamples = newNbSamples;
        peakSampleIndex = newPeakSampleIndex;
        this->videoSamplingRate = videoSamplingRate;
        videoWidth = width;
        videoHeight = height;
        this->backgroundImage = backgroundImage;
        this->traceBackgroundImage = traceBackgroundImage;
        this->rotation = rotation;
        this->flip = flip;
        drawPositionsOnBackground = positionsBackground;

        acquisitionGain = static_cast<int>(0.5 +
                                           static_cast<float>(pow(static_cast<double>(2),static_cast<double>(resolution))
                                                              / static_cast<float>(voltageRange * 1000))
                                           * amplification);

        gain = static_cast<int>(0.5 + screenGain * acquisitionGain);
    }

    /**Returns true if the current opened file is a dat file (intial recorded file), false otherwise.*/
    bool isCurrentFileAdatFile() const;

    /**Returns the base name of the document (common name for all the files). */
    QString documentBaseName() const {return baseName;}

    /**Returns the session file path.*/
    QString sessionPath() const;

    /**Returns the total length of the document in seconds.*/
    qlonglong recordingLength() const;

    /**Tells if there is opened document to be closed.
   * @return true if there is a document to close, false otherwise.
   */
    bool isADocumentToClose();

    /**Informs the displays that the mode hase switch from edit to none edit.
   * @param activeView the view in which the change has to be immediate.
   */
    void setNoneEditMode(NeuroscopeView* activeView);

    /**Verifies if the document can be close.
    * @param mainWindow the main window calling this method.
    * @param callingMethod the mainWindow's method which call this method.
    * @return true if the document can be close, false if there still thread running and
    * the document could not be close.
    */
    bool canCloseDocument(NeuroscopeApp* mainWindow,const QString &callingMethod);

    /** Shows or hides the calibration bar. This bar is meaningful only when all the channels
    *  have the same amplification.
    * @param show true if the bar has to be shown false otherwise.
    * @param activeView the view in which the change has to be immediate.
    */
    void showCalibration(bool show,NeuroscopeView* activeView);

    /**Returns the name used to identified the last loaded provider.
    */
    QString lastLoadedProviderName() const {return lastLoadedProvider;}

    /**Returns the item color list for the given provider.
    * @param fileName name of the file containing the data of the provider.
    */
    ItemColors* providerColorList(const QString &fileName){return providerItemColors[fileName];}

    /**Loads the cluster file identified by @p clusterUrl.
    * @param clusterUrl url of the cluster file to load.
    * @param activeView the view in which the change has to be immediate.
    * @return an OpenSaveCreateReturnMessage enum giving the load status.
    */
    OpenSaveCreateReturnMessage loadClusterFile(const QString &clusterUrl,NeuroscopeView* activeView);

    /**Loads the cluster file store in the session file and identified by @p clusterUrl.
    * @param clusterUrl url of the cluster file to load.
    * @param itemColors a map given the colors for the clusters contained in the file.
    * @param lastModified the date of last modification of the file store in the session file.
    * @param firstFile true if the file to load if the first one, false otherwise.
    * @return an OpenSaveCreateReturnMessage enum giving the load status.
    */
    OpenSaveCreateReturnMessage loadClusterFile(const QString &clusterUrl,QMap<EventDescription,QColor>& itemColors,const QDateTime &lastModified,bool firstFile);


    /**Loads the position file and creates the position view in the current display.
    * @param url file to be opened.
    * @param activeView the view in which the change has to be immediate.
    * @return an OpenSaveCreateReturnMessage enum giving the load status.
    */
    OpenSaveCreateReturnMessage loadPositionFile(const QString &url,NeuroscopeView*activeView);

    /**Loads the position file.
    * @param filePath path of the file to be opened.
    * @return an OpenSaveCreateReturnMessage enum giving the load status.
    */
    OpenSaveCreateReturnMessage loadPositionFile(const QString &filePath);

    /**Removes the cluster provider corresponding to the identifier @p providerName
    * from the list of providers.
    * @param providerName identifier of the cluster provider.
    * @param activeView the view in which the change has to be immediate.
    */
    void removeClusterFile(QString providerName,NeuroscopeView* activeView);

    /**Loads the event file identified by @p eventUrl.
    * @param eventUrl url of the event file to load.
    * @param activeView the view in which the change has to be immediate.
    * @return an OpenSaveCreateReturnMessage enum giving the load status.
    */
    OpenSaveCreateReturnMessage loadEventFile(const QString &eventUrl,NeuroscopeView* activeView);

    /**Loads the event file store in the session file and identified by @p eventUrl.
    * @param eventUrl url of the event file to load.
    * @param itemColors a map given the colors for the events contained in the file.
    * @param lastModified the date of last modification of the file store in the session file.
    * @param firstFile true if the file to load if the first one, false otherwise.
    * @return an OpenSaveCreateReturnMessage enum giving the load status.
    */
    OpenSaveCreateReturnMessage loadEventFile(const QString &eventUrl,QMap<EventDescription,QColor>& itemColors,const QDateTime &lastModified,bool firstFile);

    /**Removes the event provider corresponding to the identifier @p providerName
    * from the list of providers.
    * @param providerName identifier of the event provider.
    * @param activeView the view in which the change has to be immediate.
    * @param lastFile true if the event file removed is the last event provider, false otherwise.
    */
    void removeEventFile(const QString &providerName, NeuroscopeView* activeView, bool lastFile);

    /**Sets the data providers to the newly created view.
    * @param activeView the view which gives its parameters to the new view.
    */
    void setProviders(NeuroscopeView* activeView);

    /**Updates the selection of clusters to be shown by showing all the clusters
    * except those contained in @p clustersToNotShow.
    * @param clusterPalette the palette containing the clusters to be shown.
    * @param activeView the view in which the change has to be immediate.
    * @param clustersToHide list of clusters to not show.
    */
    void showAllClustersExcept(ItemPalette* clusterPalette, NeuroscopeView* activeView, const QList<int> &clustersToHide);

    /**Updates the selection of clusters to be shown by hiding all the clusters.
    * @param clusterPalette the palette containing the clusters to be shown.
    * @param activeView the view in which the change has to be immediate.
    */
    void deselectAllClusters(ItemPalette* clusterPalette,NeuroscopeView* activeView);


    /**Updates the selection of events to be shown by showing all the events.
    * @param eventPalette the palette containing the events to be shown.
    * @param activeView the view in which the change has to be immediate.
    */
    void showAllEvents(ItemPalette* eventPalette,NeuroscopeView* activeView);

    /**Updates the selection of events to be shown by hiding all the events.
    * @param eventPalette the palette containing the events to be shown.
    * @param activeView the view in which the change has to be immediate.
    */
    void deselectAllEvents(ItemPalette* eventPalette,NeuroscopeView* activeView);

    /**Returns the value to use as the length for the event descriptions in the event palette for the the last loaded event provider.*/
    int getLastEventProviderGridX() const {return lastEventProviderGridX;}

    /**Sets the event position in percentage from the begining of the window where the events are display when browsing.*/
    void setEventPosition(int position);

    /**Sets the cluster position in percentage from the begining of the window where the clusters are display when browsing.*/
    void setClusterPosition(int position);

    /**Informs that an event has been modified.
    * @param providerName name use to identified the event provider containing the modified event.
    * @param selectedEventId id of the modified event.
    * @param time initial time of the modified event.
    * @param newTime new time of the modified event.
    * @param activeView the view in which the change has been made.
    */
    void eventModified(const QString &providerName, int selectedEventId, double time, double newTime, NeuroscopeView* activeView);

    /**Informs that an event has been removed.
    * @param providerName name use to identified the event provider containing the removed event.
    * @param selectedEventId id of the removed event.
    * @param time initial time of the removed event.
    * @param activeView the view in which the change has been made.
    */
    void eventRemoved(const QString &providerName, int selectedEventId, double time, NeuroscopeView* activeView);

    /** Reverts the last user action.
    * @param activeView the currently active view.
    */
    void undo(NeuroscopeView* activeView);

    /** Reverts the last undo action.
    * @param activeView the currently active view.
    */
    void redo(NeuroscopeView* activeView);

    /**Informs that an event has been added.
    * @param providerName name use to identified the event provider containing the added event.
    * @param addedEventDescription description of the added event.
    * @param time time of the added event.
    * @param activeView the view in which the change has been made.
    */
    void eventAdded(const QString &providerName, const QString &addedEventDescription, double time, NeuroscopeView* activeView);


    /** Returns the list of existing event descriptions contained in the currently selected event file.
    * @param providerName identifier of the event file.
    * @return list of the event descriptions.
    */
    QList<EventDescription> eventIds(const QString &providerName);


    /**Sets the information used to display spike waveforms.
    * @param nb number of samples per spike waveform.
    * @param index sample index corresponding to the peak of a spike waveform.
    * @param activeView the view in which the change has been made.
    */
    void setWaveformInformation(int nb,int index,NeuroscopeView* activeView);


    /**Sets the default information used to display spike waveforms.
    * @param nb number of samples per spike waveform.
    * @param index sample index corresponding to the peak of a spike waveform.
    */
    void setDefaultWaveformInformation(int nb,int index){
        nbSamplesDefault = nb;
        peakSampleIndexDefault = index;
    }

    /**Returns the number of samples per spike waveform.*/
    int getNbSamples()const{return nbSamples;}

    /**Returns the index of the peak sample in the spike waveform.*/
    int getPeakIndex()const{return peakSampleIndex;}


    /**Sets the information used to display the animal position.
   * @param videoSamplingRate video acquisition sampling rate.
   * @param width video image width.
   * @param height video image height.
   * @param backgroundImage image used as a background for the position view.
   * @param rotation video image rotation angle.
   * @param flip video image flip orientation, 0 stands for none, 1 for vertical and 2 for horizontal.
   * @param positionsBackground true if the all the positions contain in the position file have to be drawn on the background, false otherwise.
   * @param activeView the view in which the change has been made.
   */
    void setPositionInformation(double videoSamplingRate, int width, int height, const QString &backgroundImage,
                                int rotation,int flip,bool positionsBackground,NeuroscopeView* activeView);

    /**Sets the default information used to display the animal position.
   * @param videoSamplingRate video acquisition sampling rate.
   * @param width video image width.
   * @param height video image height.
   * @param backgroundImage image used as a background for the position view.
   * @param rotation video image rotation angle.
   * @param flip video image flip orientation, 0 stands for none, 1 for vertical and 2 for horizontal.
   * @param positionsBackground true if the all the positions contain in the position file have to be drawn on the background, false otherwise.
   */
    void setDefaultPositionInformation(double videoSamplingRate, int width, int height, const QString &backgroundImage,int rotation,int flip,bool positionsBackground);

    /**Gets the video sampling rate.
   * @return current video sampling rate.
   */
    double getVideoSamplingRate()const{return videoSamplingRate;}

    /**Gets the number of channels for the current document.
   * @return current number of channels.
   */
    int getChannelNb()const{return channelNb;}

    /**Returns the video image width.
   * @return current video image width.
   */
    int getWidth()const{return videoWidth;}

    /**Returns the video image height.
   * @return current video image height.
   */
    int getHeight()const{return videoHeight;}

    /**Returns the background image for the PositionView.
   * @return current background image;
   */
    QString getBackgroundImage()const{return backgroundImage;}

    /**Returns the background image for the TraceView.
   * @return current traceBackground image;
   */
    QString getTraceBackgroundImage()const{return traceBackgroundImage;}

    /**Returns the video image rotation angle.
   * @return current rotation angle.
   */
    int getRotation()const{return rotation;}

    /**Returns the video image flip orientation.
   * 0 stands for none, 1 for vertical and 2 for horizontal.
   * @return current flip orientation.
   */
    int getFlip()const{return flip;}

    /**Removes the positon provider corresponding to the position file
   * from the list of providers.
   * @param activeView the view in which the change has to be immediate.
   */
    void removePositionFile(NeuroscopeView* activeView);

    /**Adds a PositionView in the current display.
   * @param activeView the view in which the position view will be added.
   * @param backgroundColor
   */
    void addPositionView(NeuroscopeView* activeView, const QColor &backgroundColor);

    class CloseDocumentEvent;
    friend class CloseDocumentEvent;

    CloseDocumentEvent* getCloseDocumentEvent(QString origin){
        return new CloseDocumentEvent(origin);
    }

    /**
    * Internal class use to send information to the main window to inform it that
    * the document could not be closed has there still have thread running.
    */
    class CloseDocumentEvent : public QEvent{
        //Only the method getCloseDocumentEvent of NeuroscopeDoc has access to the private part of CloseDocumentEvent,
        //the constructor of CloseDocumentEvent being private, only this method con create a new CloseDocumentEvent
        friend CloseDocumentEvent* NeuroscopeDoc::getCloseDocumentEvent(QString origin);

    public:
        QString methodOfOrigin() const {return origin;}
        ~CloseDocumentEvent(){}

    private:
        CloseDocumentEvent(const QString& origin):QEvent(QEvent::Type(QEvent::User + 200)),origin(origin){}

        QString origin;
    };


    /**Gets a background image containing the animal trajectory on a white background. Transformation (rotation and flip have
   * been applied if necessary).
   * @return a QImage which is a transform copy of this image (he original QImage is not changed) or an null image if the animal trajectory
   * is not currently set.
   */
    QImage getWhiteTrajectoryBackground();


    /**Updates the color of the skipped channels to either white or background color.
   * @param whiteBackground true if the skipped channels should be colored in white, false otherwise.
   * @param backgroundColor current background color.
   */
    void updateSkippedChannelColors(bool whiteBackground, const QColor &backgroundColor);

    /**Informs the views that the list of skipped channel has changed.*/
    void updateSkipStatus();

    /*Sets the current channel offsets has the default offsets.
   * @param activeView the view containing the offsets to be used as default.
   */
    void setDefaultOffsets(NeuroscopeView* activeView);

    /**Resets the channel default offsets to zero.*/
    void resetDefaultOffsets();

    /**Returns a reference on the the map given the of channels default offsets.*/
     const QMap<int,int>& getChannelDefaultOffsets()const{return channelDefaultOffsets;}

public Q_SLOTS:

    /**Updates the event palette and the views after the creation of a new event description.
   * @param providerName provider identifier.
   * @param oldNewEventIds map between the previous eventIds and the new ones.
   * @param newOldEventIds map between the new eventIds and the previous ones.
   * @param eventDescriptionAdded new event description added.
   */
    void slotNewEventDescriptionCreated(const QString &providerName, QMap<int, int> oldNewEventIds, QMap<int, int> newOldEventIds, const QString &eventDescriptionAdded);

    /**Updates the event palette and the views after the suppression of an event description.
   * @param providerName provider identifier.
   * @param oldNewEventIds map between the previous eventIds and the new ones.
   * @param newOldEventIds map between the new eventIds and the previous ones.
   * @param eventIdToRemove event id removed.
   * @param eventDescriptionToRemove removed event description.
   */
    void slotEventDescriptionRemoved(const QString& providerName,QMap<int,int> oldNewEventIds,QMap<int,int> newOldEventIds,int eventIdToRemove, const QString& eventDescriptionToRemove);

Q_SIGNALS:
    /**Informs the application that there is no session file available.
    * @param channelDefaultOffsets map given the channel default offsets.
    * @param skipStatus map given the skip status of the channels.
    */
    void noSession(QMap<int,int>& channelDefaultOffsets,QMap<int,bool>& skipStatus);

    /**Informs the application that there the first display to create will show
   * the channels contained in @p channelsToDisplay.
   * @param channelsToDisplay list of channel ids to show at start up.
   * @param verticalLines true if vertical lines will be drawn if clusters are selected.
   * @param raster true if a raster will be drawn if clusters are selected.
   * @param waveforms true if the spike waveforms will be drawn if clusters are selected.
   * @param showLabels true if labels are displayed next to the traces, false otherwise.
   * @param multipleColumns true if the traces are diplay on several columns,false otherwise.
   * @param greyMode true if the channels are displayed in grey-scale, false otherwise.
   * @param autocenterChannels whether channels should be centered around their offset.
   * @param offsets list containing the offset for each channel.
   * @param channelGains list of the exponents used to compute the drawing gain for each channel.
   * @param selectedChannels list of the selected channels.
   * @param skipStatus map given the skip status of the channels.
   * @param startTime starting time in miliseconds.
   * @param duration time window in miliseconds.
   * @param tabLabel label for the display when in tab page mode.
   * @param positionView true if a position view in shown in the display, false otherwise.
   * @param rasterHeight height of the rasters in the world coordinate system.
   * @param showEventsInPositionView 1 if events are displayed in the PositionView, 0 otherwise.
   */
    void loadFirstDisplay(QList<int>* channelsToDisplay,bool verticalLines,bool raster,bool waveforms,bool showLabels,
                          bool multipleColumns,bool greyMode,bool autocenterChannels,QList<int> offsets,QList<int> channelGains,
                          QList<int> selectedChannels,QMap<int,bool>& skipStatus,long startTime,long duration,QString tabLabel,bool positionView,int rasterHeight,
                          bool showEventsInPositionView);

private:
    /** The list of the views currently connected to the document */
    QList<NeuroscopeView*>* viewList;



private:
    /**The url of the document .*/
    QString docUrl;

    /**The url of the session file.*/
    QString sessionUrl;
    
    /**The url of the parameter file.*/
    QString parameterUrl;
    
    /**Reference on the channelPalette used to specify the traces display.*/
    ChannelPalette& displayChannelPalette;

    /**Reference on the channelPalette used to create the spike groups.*/
    ChannelPalette& spikeChannelPalette;

    /**Number of channels.*/
    int channelNb;

    /**Current sampling rate.*/
    double samplingRate;

    /**Initial offset for all the traces.*/
    int initialOffset;

    /**Gain which takes the screen gain into account.*/
    int gain;

    /**Acquisition system gain.*/
    int acquisitionGain;

    /**Screen gain in milivolts by centimeters used to display the field potentiels.*/
    float screenGain;

    /**Voltage range of the acquisition system in volts.*/
    int voltageRange;

    /**Amplification of the acquisition system.*/
    int amplification;

    /**Resolution of the acquisition system.*/
    int resolution;

    /**Default resolution of the acquisition system.*/
    int resolutionDefault;

    /**Default number of channels in the settings.*/
    int channelNbDefault;

    /**Default sampling rate of the dat file.*/
    double datSamplingRateDefault;

    /**Default sampling rate of the EEG file.*/
    double eegSamplingRateDefault;

    /**Default video acquisition sampling rate.*/
    double videoSamplingRateDefault;

    /**Default initial offset for all the traces.*/
    int initialOffsetDefault;

    /**Default gain.*/
    int gainDefault;

    /**Default acquisition system gain.*/
    int acquisitionGainDefault;
    
    /**Default screen gain in milivolts by centimeters used to display the field potentiels.*/
    float screenGainDefault;

    /**Default voltage range of the acquisition system in volts.*/
    int voltageRangeDefault;

    /**Default amplification of the acquisition system.*/
    int amplificationDefault;

    /**Map given the correspondance between the channel ids and the display group ids.*/
    QMap<int,int> displayChannelsGroups;

    /**Map given the correspondance between the display group ids and the channel ids.*/
    QMap<int, QList<int> > displayGroupsChannels;

    /**Map given the correspondance between the channel ids and the spike group ids.*/
    QMap<int,int> channelsSpikeGroups;

    /**Map given the correspondance between the spike group ids and the channel ids.*/
    QMap<int, QList<int> > spikeGroupsChannels;

    /**True if some properties for the current file were provided by the command line, false otherwise.*/
    bool isCommandLineProperties;

    /**Represents the list of channels with their associated color and status.*/
    ChannelColors* channelColorList;

    /*Map given the of channels default offsets.*/
    QMap<int,int> channelDefaultOffsets;
    
    /**Provider of the channels data.*/
    TracesProvider* tracesProvider;

    /**Pointer on the parent widget (main window).*/
    QWidget* parent;

    /**The base name of the document. */
    QString baseName;

    /**Sampling rate of the dat file.*/
    double datSamplingRate;

    /**Sampling rate of the EEG file.*/
    double eegSamplingRate;

    /**Video acquisition sampling rate.*/
    double videoSamplingRate;
    
    /**Number of samples in a spike waveform.*/
    int nbSamples;

    /**Sample index corresponding to the peak of a spike waveform.*/
    int peakSampleIndex;

    /**Default number of samples in a spike waveform.*/
    int nbSamplesDefault;

    /**Default Sample index corresponding to the peak of a spike waveform.*/
    int peakSampleIndexDefault;

    /**Length for a spike*/
    float waveformLength;
    
    /**Length corresponding to the index of peak of the spike.*/
    float indexLength;
    
    /**Default length for a spike*/
    float waveformLengthDefault;
    
    /**Default length corresponding to the index of peak of the spike.*/
    float indexLengthDefault;

    /** Dictionary between the provider names and the provider except the TracesProvider.*/
    QHash<QString,DataProvider*> providers;

    /**Map between the provider's name display at the top of the palette and the paths to the provider's file.*/
    QMap<QString,QString> providerUrls;

    /**Name of the last loaded provider. This name is displayed at the top of provider's palette*/
    QString lastLoadedProvider;

    /**Dictionary between the provider names and the item color lists except for the TracesProvider.*/
    QHash<QString,ItemColors*> providerItemColors;

    /**A base file name can be used for different kind of files corresponding to the same data and having
    * different sampling rates. Each file is identified by its extension. This map contains the correspondance
    * between the file extensions with the sampling rates for the current document. This map does not
    * includes the sampling rates for the extension dat and eeg, they treated separately.
    */
    QMap<QString,double> extensionSamplingRates;

    /**Map given the list of cluster file containing data for a given display group.
    * This assumes that the cluster file names contain the identifier of
    * the spike group used to create them (myFile.clu.1 correspond to the
    * spike group 1).
    */
    QMap<int, QList<int> > displayGroupsClusterFile;

    /**Extension of the open file.*/
    QString extension;

    /*The value to use as the length for the event descriptions in the event palette for the the last loaded event provider.*/
    int lastEventProviderGridX;

    /**Represents the event position, in percentage from the begining of the window, where the events are display when browsing.*/
    int eventPosition;

    /**Represents the cluster position, in percentage from the begining of the window, where the clusters are display when browsing.*/
    int clusterPosition;

    /**Stores the name of the event provider which will provide the data for the next undo/redo action.*/
    QString undoRedoProviderName;

    /**Stores the event id for the next undo/redo action.*/
    int undoRedoEventId;

    /**Stores the time of the last modified event.*/
    double modifiedEventTime;

    /**Stores the event time for the next undo/redo action.*/
    double undoRedoEventTime;

    /**True if a new event type has been created, false otherwise.*/
    bool newEventDescriptionCreated;

    /**Pair storing the the latest removed event description and the corresponding color.*/
    QPair<QString,QString> removedDescription;

    /**Default video image width.*/
    int videoWidthDefault;
    /**Default video image height.*/
    int videoHeightDefault;
    /**Default background image for the position view.*/
    QString backgroundImageDefault;
    
    /**Default background image for the trace view.*/
    QString traceBackgroundImageDefault;

    /**Default angle of rotation of the video records.*/
    int rotationDefault;
    /**Default flip orientation of the video records.
    * 0 stands for none, 1 for vertical flip and 2 for horizontal flip.    */
    int flipDefault;

    /**Video image width.*/
    int videoWidth;
    /**Video image height.*/
    int videoHeight;
    /**Background image for the position view.*/
    QString backgroundImage;
    
    /**Background image for the trace view.*/
    QString traceBackgroundImage;
    
    /**Angle of rotation of the video records.*/
    int rotation;
    /**Flip orientation of the video records.
    * 0 stands for none, 1 for vertical flip and 2 for horizontal flip.
    */
    int flip;
    
    /**Transformed background image for the position view (rotated and or flip if need it).*/
    QImage transformedBackground;
    
    /**True if the all the positions contain in the position file have to be drawn on the background image, false otherwise.*/
    bool drawPositionsOnBackground;
    
    /**Default value for drawPositionsOnBackground.*/
    bool drawPositionsOnBackgroundDefault;
    
    /**True if a position file has been opened at least one during the session (it can have been closed and not reopened), false otherwise.*/
    bool positionFileOpenOnce;
    
    /**Extension of the opened position file.*/
    QString positionFileExtension;
    
    /**Map between the channel and skip status.*/
    QMap<int,bool> skipStatus;
    
    /**Upsampling rate used to create the spike file.*/
    double upsamplingRate;

    //Functions

    /**Loads the session information, trigger the creation of displays.
    * @param reader xml parser which has loaded the session file.
    */
    void loadSession(NeuroscopeXmlReader reader);

    /**Loads the document information for either the parameter file of the session file.
    * @param reader xml parser which has loaded the session file.
    */
    void loadDocumentInformation(NeuroscopeXmlReader reader);

    /**Computes which cluster files give data for a given anatomical group.*/
    void computeClusterFilesMapping();

    /**Transforms the choosen background image to apply the user settings : rotates, flips and computes the animal
    * trajectory.
    * @param useWhiteBackground true if the background has to be white, false otherwise.
    * @return a QImage which is a transform copy of this image. The original QImage is not changed.
    */
    QImage transformBackgroundImage(bool useWhiteBackground = false);

};

#endif // NEUROSCOPEDOC_H
