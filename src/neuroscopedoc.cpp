/***************************************************************************
                          neuroscopedoc.cpp  -  description
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

// include files for Qt
#include <QDir>
#include <qwidget.h>
#include <QPixmap>
#include <QImage>
#include <QRegExp>
//Added by qt3to4:
#include <Q3ValueList>
#include <Q3PtrList>
#include <QInputDialog>
#include <QFileDialog>


// application specific includes
#include "neuroscopedoc.h"
#include "neuroscope.h"
#include "neuroscopeview.h"
#include "tracesprovider.h"
#include "traceview.h"
#include "channelcolors.h"
#include "neuroscopexmlreader.h"
#include "parameterxmlmodifier.h"
#include "parameterxmlcreator.h"
#include "sessionxmlwriter.h"
#include "sessionInformation.h"
#include "clustersprovider.h"
#include "itemcolors.h"
#include "itempalette.h"
#include "positionsprovider.h"
#include "imagecreator.h"
#include "utilities.h"


//C, C++ include files
//#define _LARGEFILE_SOURCE already defined in /usr/include/features.h
#define _FILE_OFFSET_BITS 64
#include <stdio.h>
#include <iostream>
using namespace std;

extern QString version;

NeuroscopeDoc::NeuroscopeDoc(QWidget* parent,ChannelPalette& displayChannelPalette,ChannelPalette& spikeChannelPalette,int channelNbDefault,double datSamplingRateDefault,
                             double eegSamplingRateDefault,int initialOffset,int voltageRangeDefault,int amplificationDefault,float screenGainDefault,int resolutionDefault,int eventPosition,int clusterPosition,
                             int nbSamples, int peakSampleIndex,double videoSamplingRate, int width, int height, QString backgroundImage,QString traceBackgroundImage,int rotation,int flip,bool positionsBackground) :
    docUrl(),sessionUrl(),displayChannelPalette(displayChannelPalette),spikeChannelPalette(spikeChannelPalette),resolutionDefault(resolutionDefault),channelNbDefault(channelNbDefault),datSamplingRateDefault(datSamplingRateDefault),
    eegSamplingRateDefault(eegSamplingRateDefault),videoSamplingRateDefault(videoSamplingRate),initialOffsetDefault(initialOffset),screenGainDefault(screenGainDefault),voltageRangeDefault(voltageRangeDefault),amplificationDefault(amplificationDefault),isCommandLineProperties(false),
    channelColorList(0L),tracesProvider(0L),parent(parent),baseName(""),nbSamplesDefault(nbSamples),peakSampleIndexDefault(peakSampleIndex),extension(""),eventPosition(eventPosition),clusterPosition(clusterPosition),
    newEventDescriptionCreated(false),videoWidthDefault(width),videoHeightDefault(height),backgroundImageDefault(backgroundImage),traceBackgroundImageDefault(traceBackgroundImage),rotationDefault(rotation),flipDefault(flip),drawPositionsOnBackgroundDefault(positionsBackground),positionFileOpenOnce(false)
{
    viewList = new Q3PtrList<NeuroscopeView>;
    viewList->setAutoDelete(false);
    providers.setAutoDelete(true);
    providerItemColors.setAutoDelete(true);

    //Set the properties to the default values
    channelNb = channelNbDefault;
    //default to the data file sampling rate.
    datSamplingRate = samplingRate = datSamplingRateDefault;
    eegSamplingRate = eegSamplingRateDefault;

    voltageRange = voltageRangeDefault;
    amplification = amplificationDefault;
    screenGain = screenGainDefault;
    acquisitionGainDefault = static_cast<int>(0.5 +
                                              static_cast<float>(pow(static_cast<double>(2),static_cast<double>(resolutionDefault))
                                                                 / static_cast<float>(voltageRangeDefault * 1000))
                                              * amplificationDefault);

    gainDefault = static_cast<int>(0.5 + screenGainDefault * acquisitionGainDefault);

    gain = gainDefault;
    acquisitionGain = acquisitionGainDefault;
    resolution = resolutionDefault;
    this->initialOffset = initialOffsetDefault;
    this->nbSamples = nbSamplesDefault;
    this->peakSampleIndex = peakSampleIndexDefault;
    this->videoSamplingRate = videoSamplingRateDefault;
    this->rotation = rotationDefault;
    this->flip = flipDefault;
    this->backgroundImage = backgroundImageDefault;
    this->videoWidth = videoWidthDefault;
    this->videoHeight = videoHeightDefault;
    drawPositionsOnBackground = drawPositionsOnBackgroundDefault;
    this->traceBackgroundImage = traceBackgroundImageDefault;

    //Temp
    waveformLength = waveformLengthDefault = 1.6;
    indexLength = indexLengthDefault = 0.8;
}

NeuroscopeDoc::~NeuroscopeDoc(){
    delete viewList;
    if(channelColorList != 0L){
        delete channelColorList;
        delete tracesProvider;
    }
}

bool NeuroscopeDoc::canCloseDocument(NeuroscopeApp* mainWindow,QString callingMethod){
    //Before closing, make sure that there is no thread running.
    //Loop on all the cluster and event Providers, moving to the next one when the current one has no more thread running.
    bool threadRunning = false;

    if(!threadRunning){
        Q3DictIterator<DataProvider> iterator(providers);
        for(;iterator.current();++iterator){
            threadRunning = iterator.current()->isThreadsRunning();
            if(threadRunning) break;
        }
    }

    if(threadRunning){
        //Send an event to the klusters (main window) to let it know that the document can not
        //be close because some thread are still running.
        CloseDocumentEvent* event = getCloseDocumentEvent(callingMethod);
        QApplication::postEvent(mainWindow,event);
        return false;
    }
    else return true;
}

void NeuroscopeDoc::addView(NeuroscopeView* view)
{
    viewList->append(view);
}

void NeuroscopeDoc::removeView(NeuroscopeView* view)
{
    viewList->remove(view);
}
void NeuroscopeDoc::setURL(const QString &url)
{
    docUrl = url;
}

const QString& NeuroscopeDoc::url() const
{
    return docUrl;
}

QString NeuroscopeDoc::sessionPath() const{
    return sessionUrl.path();
}

void NeuroscopeDoc::closeDocument(){  
    //If a document has been open reset the members
    viewList->clear();
    docUrl = QString();
    sessionUrl = QString();
    baseName = "";
    //Use the default values
    channelNb = channelNbDefault;
    datSamplingRate = samplingRate = datSamplingRateDefault;
    eegSamplingRate = eegSamplingRateDefault;
    voltageRange = voltageRangeDefault;
    amplification = amplificationDefault;
    screenGain = screenGainDefault;
    gain = gainDefault;
    acquisitionGainDefault = acquisitionGainDefault;
    resolution = resolutionDefault;
    initialOffset = initialOffsetDefault;
    isCommandLineProperties = false;
    nbSamples = nbSamplesDefault;
    peakSampleIndex = peakSampleIndexDefault;
    waveformLength = waveformLengthDefault;
    indexLength = indexLengthDefault;

    newEventDescriptionCreated = false;

    videoSamplingRate = videoSamplingRateDefault;
    videoWidth = videoWidthDefault;
    videoHeight = videoHeightDefault;
    backgroundImage = backgroundImageDefault;
    transformedBackground.reset();
    drawPositionsOnBackground = drawPositionsOnBackgroundDefault;
    rotation = rotationDefault;
    flip = flipDefault;
    positionFileOpenOnce = false;
    traceBackgroundImage = traceBackgroundImageDefault;

    displayChannelsGroups.clear();
    channelsSpikeGroups.clear();
    displayGroupsChannels.clear();
    spikeGroupsChannels.clear();
    skipStatus.clear();

    //Variables used for cluster and event providers
    providers.clear();
    providerItemColors.clear();
    providerUrls.clear();
    displayGroupsClusterFile.clear();

    if(channelColorList != 0L){
        delete channelColorList;
        channelColorList = 0L;
        delete tracesProvider;
        tracesProvider = 0L;
    }

    channelDefaultOffsets.clear();
}

bool NeuroscopeDoc::isADocumentToClose(){
    return (channelColorList != 0L);
}


int NeuroscopeDoc::openDocument(const QString& url)
{

    channelColorList = new ChannelColors();
    docUrl = url;

    //We look for the general information:
    // - the type of file: dat or eeg
    // - the number of channels
    // - the sampling rate for the dat file
    // - the sampling rate for the eeg file
    // - the composition of the groups of electrodes
    // - the acquisition system resolution
    //in the following locations and the following order: parameter file, session file, command line, defaults.

    //If there is information from the command line and also a parameter file or a session file, warn the user that
    //the information will be taken from the file.

    //The session file also provides the following information (used only at the display level):
    // - the color of the individual electrode, their group colors (display and spike)
    // - the res, cluster and event files which have to be loaded
    // - the global offset for all the traces.
    // - the gain.
    //for each display:
    // - the display mode (single or multicolumn)
    // - the electrodes to display and their individual offset
    // - the spike display (raster, waveforms, vertical lines)
    // - which spike, cluster and event to display


    bool sessionFileExist = false;
    QString fileName = url.fileName();
    QStringList fileParts = QStringList::split(".", fileName);
    if(fileParts.count() < 2) return INCORRECT_FILE;
    //QString extension;

    //Treat the case when the selected file is a neuroscope session file or a par file.
    if(fileName.contains(".nrs") || fileName.contains(".xml")){
        if((fileName.contains(".nrs") && fileParts[fileParts.count() - 1] != "nrs") || (fileName.contains(".xml") && fileParts[fileParts.count() - 1] != "xml")) return INCORRECT_FILE;
        else{
            baseName = fileParts[0];
            for(uint i = 1;i < fileParts.count() - 1; ++i) baseName += "." + fileParts[i];

            //As all the files with the same base name share the same session and par files, ask the user to selected the desire one.
            QString startUrl(url);
            startUrl.setFileName(baseName);
            QString filter = baseName + ".dat " +  " " + baseName + ".eeg" +  " " +  baseName + ".fil";
            filter.append(tr("|Data File (*.dat), EEG File (*.eeg), Filter File (*.fil)\n"));
            filter.append(baseName + ".*");
            QString openUrl = QFileDialog::getOpenFileName(parent, tr("Open Data File..."),startUrl.path(),filter);
            if(!openUrl.isEmpty()) docUrl = openUrl;
            else{
                QString docFile = baseName + ".dat";
                docUrl.setFileName(docFile);
                if(!KIO::NetAccess::exists(docUrl)) return DOWNLOAD_ERROR;
            }
        }
    }

    fileName = docUrl.fileName();
    fileParts = QStringList::split(".", fileName);
    baseName = fileParts[0];
    for(uint i = 1;i < fileParts.count() - 1; ++i) baseName += "." + fileParts[i];
    QString sessionFile = baseName + ".nrs";
    sessionUrl = docUrl;
    sessionUrl.setFileName(sessionFile);

    extension = fileParts[fileParts.count() - 1];

    //Look up in the parameter file
    QString parFileUrl(docUrl);
    parFileUrl.setFileName(baseName +".xml");
    parameterUrl = parFileUrl;
    
    QFileInfo parFileInfo = QFileInfo(parFileUrl.path());
    QFileInfo sessionFileInfo = QFileInfo(sessionUrl.path());

    if(parFileInfo.exists()){
        if(isCommandLineProperties){
            QApplication::restoreOverrideCursor();
            KMessageBox::information(0,tr("A parameter file has be found, the command line\n"
                                          "information will be discarded and the parameter file information will be used instead."), tr("Warning!"));
            QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
        }

        NeuroscopeXmlReader reader = NeuroscopeXmlReader();
        if(reader.parseFile(parFileUrl,NeuroscopeXmlReader::PARAMETER)){
            //Load the general info
            loadDocumentInformation(reader);

            //try to get the extension information from the parameter file (prior to the 1.2.3 version, the information was
            //store in the session file)
            extensionSamplingRates = reader.getSampleRateByExtension();
            reader.closeFile();
        }
        else return PARSE_ERROR;

        //Is there a session file?
        if(sessionFileInfo.exists()){
            sessionFileExist = true;
            if(reader.parseFile(sessionUrl,NeuroscopeXmlReader::SESSION)){
                //if the session file has been created by a version of NeuroScope prior to the 1.2.3, it contains the extension information
                if(reader.getVersion() == "" || reader.getVersion() == "1.2.2") extensionSamplingRates = reader.getSampleRateByExtension();
            }
            else return PARSE_ERROR;
        }

        //If the file extension is not a .dat or .eeg look up the sampling rate for
        //the extension. If no sampling rate is available, prompt the user for the information.
        if(extension != "eeg" && extension != "dat" && extension != "xml"){
            if(extensionSamplingRates.contains(extension)) samplingRate = extensionSamplingRates[extension];
            //Prompt the user
            else{
                QApplication::restoreOverrideCursor();

                QString currentSamplingRate = QInputDialog::getText(this,tr("Sampling Rate"),tr("Type in the sampling rate for the current document"),QLineEdit::Normal,QString("%1").arg(datSamplingRate));
                QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
                if(!currentSamplingRate.isEmpty())
                    samplingRate = currentSamplingRate.toDouble();
                else
                    samplingRate = datSamplingRate;//default
                extensionSamplingRates.insert(extension,samplingRate);
            }
        }
        else{
            if(extension == "eeg") samplingRate = eegSamplingRate;
            //Assign the default, dat sampling rate
            else samplingRate = datSamplingRate;
        }

        //Create the tracesProvider with the information gather before.
        tracesProvider = new TracesProvider(docUrl.path(),channelNb,resolution,samplingRate,initialOffset);

        //Is there a session file?
        if(sessionFileExist){
            loadSession(reader);
            reader.closeFile();
        }
    }
    //there is no parameter file
    else{
        //look up in the session file
        if(sessionFileInfo.exists()){
            sessionFileExist = true;
            NeuroscopeXmlReader reader = NeuroscopeXmlReader();

            if(reader.parseFile(sessionUrl,NeuroscopeXmlReader::SESSION)){
                //get the file version. If it is "" or "1.2.2, the documentation information can be stored in the session file,
                //read it from there, otherwise it is an error. After version 1.2.2 a parameter file should always exit at the same time that the session
                //file => return an error.
                if(reader.getVersion() == "" || reader.getVersion() == "1.2.2"){
                    if(isCommandLineProperties){
                        QApplication::restoreOverrideCursor();
                        KMessageBox::information(0,tr("A session file has be found, the command line "
                                                      "information will be discarded and the session file information will be used instead."), tr("Warning!"));
                        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
                    }

                    //Load the general info
                    loadDocumentInformation(reader);
                }
                else return MISSING_FILE;

                //Load the extension-sampling rate maping (prior to the 1.2.3 version, the information was store in the session file)
                extensionSamplingRates = reader.getSampleRateByExtension();

                //If the file extension is not a .nrs, .dat or .eeg look up the sampling rate for
                //the extension. If no sampling rate is available, prompt the user for the information.
                if(extension != "eeg" && extension != "dat" && extension != "xml"){
                    if(extensionSamplingRates.contains(extension)) samplingRate = extensionSamplingRates[extension];
                    //Prompt the user
                    else{
                        QApplication::restoreOverrideCursor();
                        QString currentSamplingRate = QInputDialog::getText(this,tr("Sampling Rate"),tr("Type in the sampling rate for the current document"),QLineEdit::Normal,QString("%1").arg(datSamplingRate));

                        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
                        if(!currentSamplingRate.isEmpty())
                            samplingRate = currentSamplingRate.toDouble();
                        else
                            samplingRate = datSamplingRate;//default
                        extensionSamplingRates.insert(extension,samplingRate);
                    }
                }
                else{
                    if(extension == "eeg") samplingRate = eegSamplingRate;
                    //Assign the default, dat sampling rate
                    else samplingRate = datSamplingRate;
                }

                //Create the tracesProvider with the information gather before.
                tracesProvider = new TracesProvider(docUrl.path(),channelNb,resolution,samplingRate,initialOffset);

                //Load the session information
                loadSession(reader);

                reader.closeFile();
            }
            else return PARSE_ERROR;
        }
        //No parameter or session file. Use defaults and or command line information (any of the command line arguments have overwritten the default values).
        else{
            bool isaDatFile = true;
            if(extension != "dat") isaDatFile = false;
            //Show a dialog to inform the user what are the parameters which will be used.
            dynamic_cast<NeuroscopeApp*>(parent)->displayFileProperties(channelNb,samplingRate,resolution,initialOffset,voltageRange,
                                                                        amplification,screenGain,nbSamples,peakSampleIndex,videoSamplingRate,videoWidth,videoHeight,backgroundImage,
                                                                        rotation,flip,datSamplingRate,isaDatFile,drawPositionsOnBackground,traceBackgroundImage);

            if(extension == "eeg") eegSamplingRate = samplingRate;
            //if the extension is not dat, the sampling rate for the dat file while be the default one.
            else if(extension == "dat") datSamplingRate = samplingRate;
            else extensionSamplingRates.insert(extension,samplingRate);

            //Create the tracesProvider with the information gather before.
            tracesProvider = new TracesProvider(docUrl.path(),channelNb,resolution,samplingRate,initialOffset);

            //No group of channels exist, put all the channels in the same group (1 for the display palette and
            //-1 (the trash group) for the spike palette) and assign them the same blue color.
            //Build the channelColorList and channelDefaultOffsets (default is 0)
            QColor color;
            Q3ValueList<int> groupOne;
            color.setHsv(210,255,255);
            for(int i = 0; i < channelNb; ++i){
                channelColorList->append(i,color);
                displayChannelsGroups.insert(i,1);
                channelsSpikeGroups.insert(i,-1);
                groupOne.append(i);
                channelDefaultOffsets.insert(i,0);
            }
            displayGroupsChannels.insert(1,groupOne);
            spikeGroupsChannels.insert(-1,groupOne);

            acquisitionGain = static_cast<int>(0.5 +
                                               static_cast<float>(pow(static_cast<double>(2),static_cast<double>(resolution))
                                                                  / static_cast<float>(voltageRange * 1000))
                                               * amplification);

            gain = static_cast<int>(0.5 + screenGain * acquisitionGain);
        }
    }

    //if skipStatus is empty, set the default status to 0
    if(skipStatus.size() == 0){
        for(int i = 0; i < channelNb; ++i) skipStatus.insert(i,false);
    }

    //Use the channel default offsets
    if(!sessionFileExist) emit noSession(channelDefaultOffsets,skipStatus);
    return OK;
}

int NeuroscopeDoc::saveEventFiles(){
    QMap<QString,QString>::Iterator iterator;
    for(iterator = providerUrls.begin(); iterator != providerUrls.end(); ++iterator){
        DataProvider* provider = providers[iterator.key()];
        if(provider->isA("EventsProvider")){
            EventsProvider* eventProvider = static_cast<EventsProvider*>(provider);
            if(eventProvider->isModified()){
                QFile eventFile(iterator.data().path());
                bool status = eventFile.open(QIODevice::WriteOnly);
                if(!status) return SAVE_ERROR;
                int saveStatus;
                saveStatus = eventProvider->save(&eventFile);
                eventFile.close();
                if(saveStatus != IO_Ok) return saveStatus;
            }
        }
    }
    return IO_Ok;
}

NeuroscopeDoc::OpenSaveCreateReturnMessage NeuroscopeDoc::saveSession(){
    //Save the document information
    QFileInfo parFileInfo = QFileInfo(parameterUrl.path());
    //If the parameter file exists, modify it
    if(parFileInfo.exists()){
        //Check that the file is writable
        if(!parFileInfo.isWritable()) return NOT_WRITABLE;
        bool status;
        ParameterXmlModifier parameterModifier = ParameterXmlModifier();
        status = parameterModifier.parseFile(parameterUrl);
        if(!status) return PARSE_ERROR;
        status = parameterModifier.setAcquisitionSystemInformation(resolution,channelNb,datSamplingRate,voltageRange,amplification,initialOffset);
        if(!status) return PARSE_ERROR;
        if(positionFileOpenOnce){
            status = parameterModifier.setVideoInformation(videoWidth,videoHeight);
            if(!status) return PARSE_ERROR;
        }
        status = parameterModifier.setLfpInformation(eegSamplingRate);
        if(!status) return PARSE_ERROR;
        if(!extensionSamplingRates.empty()){
            status = parameterModifier.setSampleRateByExtension(extensionSamplingRates);
            if(!status) return PARSE_ERROR;
        }
        status = parameterModifier.setSpikeDetectionInformation(nbSamples,peakSampleIndex,spikeGroupsChannels);
        if(!status) return PARSE_ERROR;
        status = parameterModifier.setAnatomicalDescription(displayGroupsChannels,displayChannelPalette.getSkipStatus());
        if(!status) return PARSE_ERROR;

        parameterModifier.setNeuroscopeVideoInformation(rotation,flip,backgroundImage,drawPositionsOnBackground);
        parameterModifier.setMiscellaneousInformation(screenGain,traceBackgroundImage);
        status = parameterModifier.setChannelDisplayInformation(channelColorList,displayChannelsGroups,channelDefaultOffsets);
        if(!status) return PARSE_ERROR;

        status = parameterModifier.writeTofile(parameterUrl);
        if(!status) return CREATION_ERROR;
    }
    //If the parameter file does not exist, create it
    else{
        ParameterXmlCreator parameterCreator = ParameterXmlCreator();
        parameterCreator.setAcquisitionSystemInformation(resolution,channelNb,datSamplingRate,voltageRange,amplification,initialOffset);
        if(positionFileOpenOnce) parameterCreator.setVideoInformation(videoWidth,videoHeight);
        parameterCreator.setLfpInformation(eegSamplingRate);
        if(!extensionSamplingRates.empty()) parameterCreator.setSampleRateByExtension(extensionSamplingRates);
        parameterCreator.setSpikeDetectionInformation(nbSamples,peakSampleIndex,spikeGroupsChannels);
        parameterCreator.setAnatomicalDescription(displayGroupsChannels,displayChannelPalette.getSkipStatus());
        parameterCreator.setMiscellaneousInformation(screenGain,traceBackgroundImage);
        parameterCreator.setNeuroscopeVideoInformation(rotation,flip,backgroundImage,drawPositionsOnBackground);
        parameterCreator.setChannelDisplayInformation(channelColorList,displayChannelsGroups,channelDefaultOffsets);

        bool status = parameterCreator.writeTofile(parameterUrl);
        if(!status) return CREATION_ERROR;
    }

    //Save the session
    SessionXmlWriter sessionWriter = SessionXmlWriter();

    //Create the list of loaded files
    Q3ValueList<SessionFile> fileList;
    QMap<QString,QString>::Iterator iterator;
    for(iterator = providerUrls.begin(); iterator != providerUrls.end(); ++iterator){
        SessionFile sessionFile;
        sessionFile.setUrl(iterator.data());
        QFileInfo fileInfo = QFileInfo(iterator.data().path());
        sessionFile.setModification(fileInfo.lastModified());
        DataProvider* provider = providers[iterator.key()];

        if(provider->isA("ClustersProvider")){
            //Type of the file, CLUSTER is the default
            ItemColors* clusterColors = providerItemColors[iterator.key()];
            Q3ValueList<int> clusterList = static_cast<ClustersProvider*>(provider)->clusterIdList();
            Q3ValueList<int>::iterator it;
            for(it = clusterList.begin(); it != clusterList.end(); ++it){
                QColor color = clusterColors->color(*it);
                sessionFile.setItemColor(EventDescription(QString("%1").arg(*it)),color.name());
            }
        }
        else if(provider->isA("EventsProvider")){
            sessionFile.setType(SessionFile::EVENT);
            ItemColors* eventColors = providerItemColors[iterator.key()];
            QMap<EventDescription,int> eventMap = static_cast<EventsProvider*>(provider)->eventDescriptionIdMap();
            QMap<EventDescription,int>::Iterator it;
            for(it = eventMap.begin(); it != eventMap.end(); ++it){
                QColor color = eventColors->color(it.data());
                sessionFile.setItemColor(it.key(),color.name());
            }
        }
        else if(provider->isA("PositionsProvider")){
            sessionFile.setType(SessionFile::POSITION);
        }
        fileList.append(sessionFile);
    }

    sessionWriter.setLoadedFilesInformation(fileList);

    //Create the list of display information
    Q3ValueList<DisplayInformation> displayList;
    NeuroscopeView* view;
    for(view = viewList->first(); view!=0; view = viewList->next()){
        DisplayInformation displayInformation;
        //the default value is SINGLE
        if(view->getMultiColumns()) displayInformation.setMode(DisplayInformation::MULTIPLE);
        displayInformation.setGreyScale(view->getGreyScale());
        displayInformation.setPositionView(view->isPositionView());
        displayInformation.setEventsInPositionView(view->isEventsInPositionView());
        displayInformation.setTabLabel(view->getTabName());
        displayInformation.setLabelStatus(view->getLabelStatus());
        displayInformation.setStartTime(view->getStartTime());
        displayInformation.setTimeWindow(view->getTimeWindow());

        if(view->getClusterRaster()) displayInformation.addSpikeDisplayType(DisplayInformation::RASTER);
        if(view->getClusterWaveforms()) displayInformation.addSpikeDisplayType(DisplayInformation::WAVEFORMS);
        if(view->getClusterVerticalLines()) displayInformation.addSpikeDisplayType(DisplayInformation::LINES);
        displayInformation.setRasterHeight(view->getRasterHeight());


        //loop on all the loaded files and set the clusters,event ids show in the current display
        Q3DictIterator<DataProvider> it(providers);
        for( ; it.current(); ++it){
            QString name = it.currentKey();
            if(it.current()->isA("ClustersProvider")){
                Q3ValueList<int> clusterIds = *(view->getSelectedClusters(name));
                displayInformation.setSelectedClusters(static_cast<QString>(providerUrls[name]).url(),clusterIds);
                Q3ValueList<int> skippedClusterIds = *(view->getClustersNotUsedForBrowsing(name));
                displayInformation.setSkippedClusters(static_cast<QString>(providerUrls[name]).url(),skippedClusterIds);
            }
            if(it.current()->isA("EventsProvider")){
                //An id has been assigned to each event, this id is used internally in NeuroScope and in the session file.
                Q3ValueList<int> eventIds = *(view->getSelectedEvents(name));
                displayInformation.setSelectedEvents(static_cast<QString>(providerUrls[name]).url(),eventIds);
                Q3ValueList<int> skippedEventIds = *(view->getEventsNotUsedForBrowsing(name));
                displayInformation.setSkippedEvents(static_cast<QString>(providerUrls[name]).url(),skippedEventIds);
            }
        }

        /***********TO DO**************************/
        //loop on all the loaded spike files and set the spikes show in the current display
        /* QValueList<QString> files;
  displayInformation.setSelectedSpikeFiles(files)*/

        //loop on all the channels to store their gain and offsets
        Q3ValueList<TracePosition> tracePositions;

        const Q3ValueList<int>& offsets = view->getChannelOffset();
        const Q3ValueList<int>& gains = view->getGains();
        for(int i = 0; i < channelNb; ++i){
            TracePosition tracePosition;
            tracePosition.setId(i);
            tracePosition.setGain(gains[i]);
            tracePosition.setOffset(offsets[i]);
            tracePositions.append(tracePosition);
        }

        displayInformation.setPositions(tracePositions);

        //Get the shown channels
        const Q3ValueList<int>& channelsIds = view->channels();
        displayInformation.setChannelIds(channelsIds);

        //Get the selected channels
        const Q3ValueList<int>& selectedChannelIds = view->getSelectedChannels();
        displayInformation.setSelectedChannelIds(selectedChannelIds);
        displayList.append(displayInformation);
    }

    sessionWriter.setDisplayInformation(displayList);

    bool status = sessionWriter.writeTofile(sessionUrl);
    if(!status) return CREATION_ERROR;
    else return OK;
}


void NeuroscopeDoc::singleChannelColorUpdate(int channelId,NeuroscopeView* activeView){
    //Notify all the views of the modification
    NeuroscopeView* view;
    for(view = viewList->first(); view!=0; view = viewList->next()){
        if(view != activeView) view->singleChannelColorUpdate(channelId,false);
        else view->singleChannelColorUpdate(channelId,true);
    }

    //Ask the active view to take the modification into account immediately
    activeView->showAllWidgets();
}

void NeuroscopeDoc::channelGroupColorUpdate(int groupId,NeuroscopeView* activeView){
    //Notify all the views of the modification
    NeuroscopeView* view;
    for(view = viewList->first(); view!=0; view = viewList->next()){
        if(view != activeView) view->channelGroupColorUpdate(groupId,false);
        else view->channelGroupColorUpdate(groupId,true);
    }

    //Ask the active view to take the modification into account immediately
    activeView->showAllWidgets();
}

/*void NeuroscopeDoc::channelsColorUpdate(QValueList<int>selectedChannels,NeuroscopeView& view){
cout<<"in NeuroscopeDoc::channelsColorUpdate"<<endl;
}*/

void NeuroscopeDoc::showCalibration(bool show,NeuroscopeView* activeView){
    //Notify all the views of the modification
    NeuroscopeView* view;
    for(view = viewList->first(); view!=0; view = viewList->next()){
        if(view != activeView) view->showCalibration(show,false);
        else view->showCalibration(show,true);
    }

    //Ask the active view to take the modification into account immediately
    activeView->showAllWidgets();
}

void NeuroscopeDoc::groupsModified(NeuroscopeView* activeView){
    if(!providerUrls.isEmpty())
        //compute which cluster files give data for a given anatomical group
        computeClusterFilesMapping();

    //Notify all the views of the modification
    NeuroscopeView* view;
    for(view = viewList->first(); view!=0; view = viewList->next()){
        if(view != activeView) view->groupsModified(false);
        else view->groupsModified(true);
    }

    //Ask the active view to take the modification into account immediately
    activeView->showAllWidgets();
}

void NeuroscopeDoc::setBackgroundColor(QColor backgroundColor){

    //If a position file is loaded and all the positions are drawn on the background (without using an image as the background)
    //update the background image
    bool isPositionFileLoaded = dynamic_cast<NeuroscopeApp*>(parent)->isApositionFileLoaded();
    if(isPositionFileLoaded && backgroundImage == "" && drawPositionsOnBackground){
        transformedBackground = transformBackgroundImage();
        NeuroscopeView* view;
        //Get the active view.
        NeuroscopeView* activeView = dynamic_cast<NeuroscopeApp*>(parent)->activeView();
        if(rotation != 90 && rotation != 270){
            for(view = viewList->first(); view!=0; view = viewList->next()){
                if(view != activeView) view->updatePositionInformation(videoWidth,videoHeight,transformedBackground,false,false);
                else view->updatePositionInformation(videoWidth,videoHeight,transformedBackground,false,true);
            }
        }
        //If there is a rotation of 90 or 270 degree, the with and height have to be inverted.
        else{
            for(view = viewList->first(); view!=0; view = viewList->next()){
                if(view != activeView) view->updatePositionInformation(videoHeight,videoWidth,transformedBackground,false,false);
                else view->updatePositionInformation(videoHeight,videoWidth,transformedBackground,false,true);
            }
        }
    }

    //Notify all the views of the modification
    NeuroscopeView* view;
    for(view = viewList->first(); view!=0; view = viewList->next())
        view->updateBackgroundColor(backgroundColor);

    //Get the active view.
    NeuroscopeView* activeView = dynamic_cast<NeuroscopeApp*>(parent)->activeView();

    //Ask the active view to take the modification into account immediately
    activeView->showAllWidgets();
}


void NeuroscopeDoc::setTraceBackgroundImage(QString traceBackgroundImagePath){
    traceBackgroundImage = traceBackgroundImagePath;
    if(tracesProvider){
        QImage traceBackgroundImage = QImage(traceBackgroundImagePath);

        //The views are updated. If the image is null, the background will be a plain color (no image)
        //Get the active view and make it the first to take the modification into account.
        NeuroscopeView* activeView = dynamic_cast<NeuroscopeApp*>(parent)->activeView();

        //Notify all the views of the modification
        NeuroscopeView* view;
        for(view = viewList->first(); view!=0; view = viewList->next()){
            if(view != activeView)view->updateTraceBackgroundImage(traceBackgroundImage,false);
            else view->updateTraceBackgroundImage(traceBackgroundImage,true);
        }
    }
}


void NeuroscopeDoc::setInitialOffset(int offset){
    initialOffset = offset;

    if(tracesProvider){
        //Inform the tracesProvider
        tracesProvider->setOffset(offset);

        //Get the active view and make it the first to take the modification into account.
        NeuroscopeView* activeView = dynamic_cast<NeuroscopeApp*>(parent)->activeView();
        activeView->documentFeaturesModified();

        //Notify all the views of the modification
        NeuroscopeView* view;
        for(view = viewList->first(); view!=0; view = viewList->next())
            if(view != activeView) view->documentFeaturesModified();

        //Ask the active view to take the modification into account immediately
        activeView->updateViewContents();
    }
}

void NeuroscopeDoc::setGains(int voltageRange,int amplification,float screenGain){  
    this->voltageRange = voltageRange;
    this->amplification = amplification;
    this->screenGain = screenGain;

    acquisitionGain = static_cast<int>(0.5 +
                                       static_cast<float>(pow(static_cast<double>(2),static_cast<double>(resolution))
                                                          / static_cast<float>(voltageRange * 1000))
                                       * amplification);

    gain = static_cast<int>(0.5 + screenGain * acquisitionGain);

    if(tracesProvider){
        //Get the active view and make it the first to take the modification into account.
        NeuroscopeView* activeView = dynamic_cast<NeuroscopeApp*>(parent)->activeView();
        activeView->setGains(gain,acquisitionGain);

        //Notify all the views of the modification
        NeuroscopeView* view;
        for(view = viewList->first(); view!=0; view = viewList->next())
            if(view != activeView) view->setGains(gain,acquisitionGain);

        //Ask the active view to take the modification into account immediately
        activeView->updateViewContents();
    }
}

void NeuroscopeDoc::setResolution(int resolution){
    this->resolution = resolution;
    if(tracesProvider){
        //Inform the tracesProvider
        tracesProvider->setResolution(resolution);

        //Get the active view and make it the first to take the modification into account.
        NeuroscopeView* activeView = dynamic_cast<NeuroscopeApp*>(parent)->activeView();
        activeView->documentFeaturesModified();

        //Notify all the views of the modification
        NeuroscopeView* view;
        for(view = viewList->first(); view!=0; view = viewList->next())
            if(view != activeView) view->documentFeaturesModified();

        //Ask the active view to take the modification into account immediately
        activeView->updateViewContents();
    }
}

void NeuroscopeDoc::setSamplingRate(double rate){
    samplingRate = rate;
    if(extension == "eeg") eegSamplingRate = rate;
    else if(extension == "dat") datSamplingRate = rate;
    else extensionSamplingRates.insert(extension,samplingRate);

    //update the cluster and event providers
    Q3DictIterator<DataProvider> iterator(providers);
    for(;iterator.current();++iterator){
        QString name = iterator.currentKey();
        if(iterator.current()->isA("ClustersProvider")){
            static_cast<ClustersProvider*>(iterator.current())->updateSamplingRate(samplingRate);
        }
        if(iterator.current()->isA("EventsProvider")){
            static_cast<EventsProvider*>(iterator.current())->updateSamplingRate(samplingRate);
        }
    }

    if(tracesProvider){
        //Inform the tracesProvider
        tracesProvider->setSamplingRate(rate);
        long long length = tracesProvider->recordingLength();

        //Get the active view and make it the first to take the modification into account.
        NeuroscopeView* activeView = dynamic_cast<NeuroscopeApp*>(parent)->activeView();
        activeView->samplingRateModified(length);

        //Notify all the views of the modification
        NeuroscopeView* view;
        for(view = viewList->first(); view!=0; view = viewList->next())
            if(view != activeView) view->samplingRateModified(length);

        //Ask the active view to take the modification into account immediately
        activeView->updateViewContents();
    }
}

void NeuroscopeDoc::setAcquisitionSystemSamplingRate(double rate){
    datSamplingRate = rate;
    //update the cluster providers
    Q3DictIterator<DataProvider> iterator(providers);
    for(;iterator.current();++iterator){
        QString name = iterator.currentKey();
        if(iterator.current()->isA("ClustersProvider")){
            static_cast<ClustersProvider*>(iterator.current())->updateAcquisitionSystemSamplingRate(datSamplingRate,samplingRate);
        }
    }

    if(tracesProvider){
        //Get the active view and make it the first to take the modification into account.
        NeuroscopeView* activeView = dynamic_cast<NeuroscopeApp*>(parent)->activeView();

        //Notify all the views of the modification
        NeuroscopeView* view;
        for(view = viewList->first(); view!=0; view = viewList->next())
            if(view != activeView) view->updateClusterData(false);
            else view->updateClusterData(true);
    }
}


void NeuroscopeDoc::setChannelNb(int nb){
    channelNb = nb;

    if(tracesProvider){
        //All existing electrode-group information will be deleted and
        //all the channels will be put in the same group (1 for the display palette and
        //-1 (the trash group) for the spike palette) and assign them the same blue color.
        //Constructs the channelColorList
        displayChannelsGroups.clear();
        channelsSpikeGroups.clear();
        displayGroupsChannels.clear();
        spikeGroupsChannels.clear();
        channelColorList->removeAll();
        displayChannelPalette.reset();
        spikeChannelPalette.reset();

        //Refill the channelColorList
        QColor color;
        Q3ValueList<int> groupOne;
        color.setHsv(210,255,255);
        for(int i = 0; i < channelNb; ++i){
            channelColorList->append(i,color);
            displayChannelsGroups.insert(i,1);
            channelsSpikeGroups.insert(i,-1);
            groupOne.append(i);
        }
        displayGroupsChannels.insert(1,groupOne);
        spikeGroupsChannels.insert(-1,groupOne);


        //Update and show the channel Palettes.
        displayChannelPalette.createChannelLists(channelColorList,&displayGroupsChannels,&displayChannelsGroups);
        displayChannelPalette.updateShowHideStatus(groupOne,false);
        spikeChannelPalette.createChannelLists(channelColorList,&spikeGroupsChannels,&channelsSpikeGroups);
        spikeChannelPalette.updateShowHideStatus(groupOne,false);

        //Resize the panel
        dynamic_cast<NeuroscopeApp*>(parent)->resizePalettePanel();

        //Inform the tracesProvider
        tracesProvider->setNbChannels(nb);

        //Get the active view and make it the first to take the modification into account.
        NeuroscopeView* activeView = dynamic_cast<NeuroscopeApp*>(parent)->activeView();
        activeView->setChannelNb(nb);

        //Notify all the views of the modification
        NeuroscopeView* view;
        for(view = viewList->first(); view!=0; view = viewList->next())
            if(view != activeView) view->setChannelNb(nb);

        //Ask the active view to take the modification into account immediately
        activeView->showAllWidgets();
    }
}

void NeuroscopeDoc::loadDocumentInformation(NeuroscopeXmlReader reader){
    int resolutionRead = reader.getResolution();
    if(resolutionRead != 0) resolution = resolutionRead;
    int channelNbRead = reader.getNbChannels();
    if(channelNbRead != 0) channelNb = channelNbRead;
    double datSamplingRateRead = reader.getSamplingRate();
    if(datSamplingRateRead != 0) datSamplingRate =  datSamplingRateRead;
    upsamplingRate = reader.getUpsamplingRate();
    double eegSamplingRateRead = reader.getLfpInformation();
    if(eegSamplingRateRead != 0) eegSamplingRate = eegSamplingRateRead;
    //the sampling rate for the video is store in the section file extension/sampling rate
    int videoWidthRead = reader.getVideoWidth();
    if(videoWidthRead != 0) videoWidth = videoWidthRead;
    int videoHeightRead = reader.getVideoHeight();
    if(videoHeightRead != 0) videoHeight = videoHeightRead;
    drawPositionsOnBackground = reader.getTrajectory();

    //The background image information is stored in the parameter file starting with the version 1.2.3
    if(reader.getType() == NeuroscopeXmlReader::PARAMETER){
        if(reader.getBackgroundImage() != "-") backgroundImage = reader.getBackgroundImage();

        if(backgroundImage != ""){
            QFileInfo fileInfo = QFileInfo(backgroundImage);
            if(!fileInfo.exists()){
                QString imageUrl;
                imageUrl.setPath(backgroundImage);
                QString fileName = imageUrl.fileName();
                imageUrl = docUrl;
                imageUrl.setFileName(fileName);
                backgroundImage = imageUrl.path();
            }
        }
    }

    //The background image information for the trace view is stored in the parameter file starting with the version 1.3.4
    if(reader.getType() == NeuroscopeXmlReader::PARAMETER){

        if(reader.getTraceBackgroundImage() != "-") traceBackgroundImage = reader.getTraceBackgroundImage();

        if(traceBackgroundImage != ""){
            QFileInfo fileInfo = QFileInfo(traceBackgroundImage);
            if(!fileInfo.exists()){
                QString imageUrl;
                imageUrl.setPath(traceBackgroundImage);
                QString fileName = imageUrl.fileName();
                imageUrl = docUrl;
                imageUrl.setFileName(fileName);
                traceBackgroundImage = imageUrl.path();
            }
        }
    }


    if(reader.getVoltageRange() != 0) voltageRange = reader.getVoltageRange();
    //Neuroscope for the moment uses a unique amplification for all the channels
    if(reader.getAmplification() != 0) amplification = reader.getAmplification();
    if(reader.getOffset() != 0) initialOffset = reader.getOffset();

    acquisitionGain = static_cast<int>(0.5 +
                                       static_cast<float>(pow(static_cast<double>(2),static_cast<double>(resolution))
                                                          / static_cast<float>(voltageRange * 1000))
                                       * amplification);

    reader.getAnatomicalDescription(channelNb,displayChannelsGroups,displayGroupsChannels,skipStatus);

    if(displayGroupsChannels.contains(0)) spikeGroupsChannels.insert(0,displayGroupsChannels[0]);
    reader.getSpikeDescription(channelNb,channelsSpikeGroups,spikeGroupsChannels);

    //compute which cluster files give data for a given anatomical group
    computeClusterFilesMapping();

    //Build the channelColorList
    //the checkColors will be used that their is a color information for each channel.
    Q3ValueList<int> checkColors;
    for(int i = 0; i < channelNb; ++i) checkColors.append(i);
    Q3ValueList<ChannelDescription> colorsList = reader.getChannelDescription();
    if(colorsList.size() != 0){
        Q3ValueList<ChannelDescription>::iterator colorIterator;
        for(colorIterator = colorsList.begin(); colorIterator != colorsList.end(); ++colorIterator){
            int channelId = static_cast<ChannelDescription>(*colorIterator).getId();
            uint removed = checkColors.remove(channelId);
            //it is a duplicate
            if(removed ==0) continue;
            QColor color = static_cast<ChannelDescription>(*colorIterator).getColor();
            QColor groupColor = static_cast<ChannelDescription>(*colorIterator).getGroupColor();
            QColor spikeGroupColor = static_cast<ChannelDescription>(*colorIterator).getSpikeGroupColor();
            channelColorList->append(channelId,color,groupColor,spikeGroupColor);
        }
        //if a channel does not have color information, set the default (everything to blue)
        if(checkColors.size() != 0){
            QColor color;
            color.setHsv(210,255,255);
            for(int i = 0; i < channelNb; ++i){
                if(!channelColorList->contains(i)){
                    channelColorList->insert(i,i,color,color,color);
                }
            }
        }
    }
    //if no color are available in the file, set the default (everything to blue)
    else{
        QColor color;
        color.setHsv(210,255,255);
        for(int i = 0; i < channelNb; ++i){
            channelColorList->append(i,color);
        }
    }

    //Build the list of channel default offsets
    reader.getChannelDefaultOffset(channelDefaultOffsets);
    //if no default offset are available in the file, set the default offset to 0
    if(channelDefaultOffsets.size() == 0){
        for(int i = 0; i < channelNb; ++i) channelDefaultOffsets.insert(i,0);
    }
    //if a channel does not have a default offset, assign it the value 0
    if(channelDefaultOffsets.size() != channelNb){
        for(int i = 0; i < channelNb; ++i){
            if(!channelDefaultOffsets.contains(i)) channelDefaultOffsets.insert(i,0);
        }
    }

    if(reader.getScreenGain() != 0) screenGain = reader.getScreenGain();
    gain = static_cast<int>(0.5 + screenGain * acquisitionGain);

    //For the moment Neuroscope stores it own values for the nbSamples and the peakSampleIndex inside the specific neuroscope tag.
    //Therefore Neuroscope uses the same values for all the groups
    //If the data do not exist in the session file a zero is return by the reader.

    //Old way: the nbSamples and peakSampleIndex are given directly
    //New way: the nbSamples and peakSampleIndex are given in time. The sampling rate is used to compute the information.

    //If no upsampling exists <=> Old way
    if(upsamplingRate == 0){
        //the upsampling is set to the sampling rate.
        upsamplingRate = datSamplingRate;
        int nbSamplesRead = reader.getNbSamples();
        int peakSampleIndexRead = reader.getPeakSampleIndex();
        if(nbSamplesRead != 0) nbSamples = nbSamplesRead;
        if(peakSampleIndexRead != 0) peakSampleIndex = peakSampleIndexRead;
    }
    else{
        float waveformLengthRead = reader.getWaveformLength();
        float indexLengthRead = reader.getPeakSampleLength();
        if(waveformLengthRead != 0) waveformLength = waveformLengthRead;
        if(indexLengthRead != 0) indexLength = indexLengthRead;

        //Compute the number of samples using the datSamplingRate.
        nbSamples = static_cast<int>(static_cast<float>(datSamplingRate / 1000) * waveformLength);

        //Compute the peak index using the datSamplingRate.
        peakSampleIndex = static_cast<int>(static_cast<float>(datSamplingRate / 1000) * indexLength);
    }
}

void NeuroscopeDoc::computeClusterFilesMapping(){
    displayGroupsClusterFile.clear();
    //compute which cluster files give data for a given anatomical group
    QMap<int, Q3ValueList<int> >::Iterator iterator;
    for(iterator = displayGroupsChannels.begin(); iterator != displayGroupsChannels.end(); ++iterator){
        Q3ValueList<int> clusterFileList;
        Q3ValueList<int> anatomicalList = iterator.data();
        QMap<int, Q3ValueList<int> >::Iterator spikeGroupIterator;
        for(spikeGroupIterator = spikeGroupsChannels.begin(); spikeGroupIterator != spikeGroupsChannels.end(); ++spikeGroupIterator){
            Q3ValueList<int> channels = spikeGroupIterator.data();
            Q3ValueList<int>::iterator channelIterator;
            for(channelIterator = channels.begin(); channelIterator != channels.end(); ++channelIterator){
                if(anatomicalList.contains(*channelIterator)){
                    clusterFileList.append(spikeGroupIterator.key());
                    break;
                }
            }
        }
        //The trash group (index 0) is always at the bottom in the display, so reindex it with the highest index.
        if(iterator.key() == 0) displayGroupsClusterFile.insert(displayGroupsChannels.count(),clusterFileList);
        else displayGroupsClusterFile.insert(iterator.key(),clusterFileList);
    }
}

void NeuroscopeDoc::loadSession(NeuroscopeXmlReader reader){  

    //Get the file video information
    if(reader.getRotation() != 0) rotation = reader.getRotation();
    if(reader.getFlip() != 0) flip = reader.getFlip();

    Q3ValueList<SessionFile> filesToLoad = reader.getFilesToLoad();
    Q3ValueList<QString> loadedClusterFiles;
    Q3ValueList<QString> loadedEventFiles;
    QString loadedPositionFile;
    QMap< QString, QMap<EventDescription,int> > loadedEventItems;

    //Get the displays information
    Q3ValueList<DisplayInformation> displayList = reader.getDisplayInformation();

    bool first = true;
    Q3ValueList<DisplayInformation>::iterator iterator;
    for(iterator = displayList.begin(); iterator != displayList.end(); ++iterator){
        Q3ValueList<int> offsets;
        Q3ValueList<int> channelGains;
        Q3ValueList<int>* channelsToDisplay = new Q3ValueList<int>();
        Q3ValueList<int> selectedChannels;
        bool verticalLines = false;
        bool raster = false;
        bool waveforms = false;
        bool multipleColumns = false;
        bool greyMode = false;
        QString tabLabel;
        bool showLabels = false;
        long startTime;
        long duration;
        bool isAPositionView = false;
        int rasterHeight;
        bool showEventsInPositionView = false;

        //Get the information store in DisplayInformation
        DisplayInformation::mode presentationMode = static_cast<DisplayInformation>(*iterator).getMode();
        startTime = static_cast<DisplayInformation>(*iterator).getStartTime();
        duration = static_cast<DisplayInformation>(*iterator).getTimeWindow();
        greyMode = static_cast<DisplayInformation>(*iterator).getGreyScale();
        Q3ValueList<DisplayInformation::spikeDisplayType> spikeDisplayTypes = static_cast<DisplayInformation>(*iterator).getSpikeDisplayTypes();
        rasterHeight = static_cast<DisplayInformation>(*iterator).getRasterHeight();
        QMap<QString, Q3ValueList<int> > selectedClusters = static_cast<DisplayInformation>(*iterator).getSelectedClusters();
        //An id has been assigned to each event, this id will be used internally in NeuroScope and in the session file.
        QMap<QString, Q3ValueList<int> > selectedEvents = static_cast<DisplayInformation>(*iterator).getSelectedEvents();
        Q3ValueList<QString> shownSpikeFiles = static_cast<DisplayInformation>(*iterator).getSelectedSpikeFiles();
        QMap<QString, Q3ValueList<int> > skippedClusters = static_cast<DisplayInformation>(*iterator).getSkippedClusters();
        QMap<QString, Q3ValueList<int> > skippedEvents = static_cast<DisplayInformation>(*iterator).getSkippedEvents();
        Q3ValueList<TracePosition> positions = static_cast<DisplayInformation>(*iterator).getPositions();
        Q3ValueList<int> channelIds = static_cast<DisplayInformation>(*iterator).getChannelIds();
        Q3ValueList<int> selectedChannelIds = static_cast<DisplayInformation>(*iterator).getSelectedChannelIds();
        tabLabel = static_cast<DisplayInformation>(*iterator).getTabLabel();
        showLabels = static_cast<DisplayInformation>(*iterator).getLabelStatus();
        showEventsInPositionView = static_cast<DisplayInformation>(*iterator).isEventsDisplayedInPositionView();

        //info on the trace presentation
        if(presentationMode == DisplayInformation::MULTIPLE) multipleColumns = true;

        //info on the spike presentation
        Q3ValueList<DisplayInformation::spikeDisplayType>::iterator typeIterator;
        for(typeIterator = spikeDisplayTypes.begin(); typeIterator != spikeDisplayTypes.end(); ++typeIterator){
            if(*typeIterator == DisplayInformation::LINES) verticalLines = true;
            if(*typeIterator == DisplayInformation::RASTER) raster = true;
            if(*typeIterator == DisplayInformation::WAVEFORMS) waveforms = true;
        }

        //Info regarding the positionView
        isAPositionView = static_cast<DisplayInformation>(*iterator).isAPositionView();

        /*****************TO FINISH***************************/
        //Get the information concerning the spike files
        Q3ValueList<QString>::iterator spikeFileIterator;
        for(spikeFileIterator = shownSpikeFiles.begin(); spikeFileIterator != shownSpikeFiles.end(); ++spikeFileIterator){
            QString fileUrl = *spikeFileIterator;
        }

        /*****************TO FINISH***************************/

        //Get the information concerning the channel positions (gain and offset)
        Q3ValueList<TracePosition>::iterator positionIterator;
        for(positionIterator = positions.begin(); positionIterator != positions.end(); ++positionIterator){
            int gain = static_cast<TracePosition>(*positionIterator).getGain();
            int offset = static_cast<TracePosition>(*positionIterator).getOffset();
            offsets.append(offset);
            channelGains.append(gain);
        }

        //Get the information concerning the channels shown in the display
        Q3ValueList<int>::iterator channelIterator;
        for(channelIterator = channelIds.begin(); channelIterator != channelIds.end(); ++channelIterator){
            channelsToDisplay->append(*channelIterator);
        }

        //Get the information concerning the channels selected in the display
        Q3ValueList<int>::iterator channelSelectedIterator;
        for(channelSelectedIterator = selectedChannelIds.begin(); channelSelectedIterator != selectedChannelIds.end(); ++channelSelectedIterator){
            selectedChannels.append(*channelSelectedIterator);
        }

        //Create the displays
        if(first){
            first = false;

            emit loadFirstDisplay(channelsToDisplay,verticalLines,raster,waveforms,showLabels,multipleColumns,greyMode,offsets,
                                  channelGains,selectedChannels,skipStatus,startTime,duration,tabLabel,isAPositionView,rasterHeight,showEventsInPositionView);

            //Now that the channel palettes are created, load the files and create the palettes
            bool fistClusterFile = true;
            bool fistEventFile = true;
            Q3ValueList<SessionFile>::iterator sessionIterator;
            for(sessionIterator = filesToLoad.begin(); sessionIterator != filesToLoad.end(); ++sessionIterator){
                SessionFile sessionFile = static_cast<SessionFile>(*sessionIterator);
                QString fileUrl = sessionFile.getUrl();
                SessionFile::type fileType = sessionFile.getType();
                QDateTime lastModified = sessionFile.getModification();
                QMap<EventDescription,QColor> itemColors = sessionFile.getItemColors();
                if(fileType == SessionFile::CLUSTER){
                    //If the file does not exist in the location specified in the session file (absolute path), look up in the directory
                    //where the session file is. This is useful if you moved your file or you backup them (<=> the absolute path is not good anymore)
                    QFileInfo fileInfo = QFileInfo(fileUrl.path());
                    if(!fileInfo.exists()){
                        Q3ValueList<int> ids = selectedClusters[fileUrl.url()];
                        Q3ValueList<int> skippedIds = skippedClusters[fileUrl.url()];
                        selectedClusters.remove(fileUrl.url());
                        skippedClusters.remove(fileUrl.url());
                        QString fileName = fileUrl.fileName();
                        fileUrl = sessionUrl;
                        fileUrl.setFileName(fileName);
                        selectedClusters.insert(fileUrl.url(),ids);
                        skippedClusters.insert(fileUrl.url(),skippedIds);
                    }
                    OpenSaveCreateReturnMessage status = loadClusterFile(fileUrl,itemColors,lastModified,fistClusterFile);
                    if(status == OK){
                        loadedClusterFiles.append(lastLoadedProvider);
                        fistClusterFile = false;
                    }
                }
                if(fileType == SessionFile::EVENT){
                    //If the file does not exist in the location specified in the session file (absolute path), look up in the directory
                    //where the session file is. This is useful if you moved your file or ypu backup them (<=> the absolute path is not good anymore)
                    QFileInfo fileInfo = QFileInfo(fileUrl.path());
                    if(!fileInfo.exists()){
                        Q3ValueList<int> ids = selectedEvents[fileUrl.url()];
                        Q3ValueList<int> skippedIds = skippedEvents[fileUrl.url()];
                        selectedEvents.remove(fileUrl.url());
                        skippedEvents.remove(fileUrl.url());
                        QString fileName = fileUrl.fileName();
                        fileUrl = sessionUrl;
                        fileUrl.setFileName(fileName);
                        selectedEvents.insert(fileUrl.url(),ids);
                        skippedEvents.insert(fileUrl.url(),skippedIds);
                    }
                    OpenSaveCreateReturnMessage status = loadEventFile(fileUrl,itemColors,lastModified,fistEventFile);
                    if(status == OK){
                        loadedEventFiles.append(lastLoadedProvider);
                        fistEventFile = false;
                        QMap<EventDescription,int> loadedItems;
                        QMap<EventDescription,QColor>::Iterator it;
                        int index = 1;
                        for(it = itemColors.begin(); it != itemColors.end(); ++it){
                            loadedItems.insert(it.key(),index);
                            index++;
                        }
                        loadedEventItems.insert(lastLoadedProvider,loadedItems);
                    }
                }
                if(fileType == SessionFile::POSITION){
                    //If the file does not exist in the location specified in the session file (absolute path), look up in the directory
                    //where the session file is. This is useful if you moved your file or you backup them (<=> the absolute path is not good anymore)
                    QFileInfo fileInfo = QFileInfo(fileUrl.path());
                    if(!fileInfo.exists()){
                        QString fileName = fileUrl.fileName();
                        fileUrl = sessionUrl;
                        fileUrl.setFileName(fileName);
                    }

                    //Create the transformedBackground
                    //The background image information is stored in the parameter file starting with the version 1.2.3
                    if(reader.getVersion() == "" || reader.getVersion() == "1.2.2"){
                        if(reader.getBackgroundImage() != "-") backgroundImage = sessionFile.getBackgroundPath();
                        if(backgroundImage != ""){
                            fileInfo = QFileInfo(backgroundImage);
                            if(!fileInfo.exists()){
                                QString imageUrl;
                                imageUrl.setPath(backgroundImage);
                                QString fileName = imageUrl.fileName();
                                imageUrl = sessionUrl;
                                imageUrl.setFileName(fileName);
                                backgroundImage = imageUrl.path();
                            }
                        }
                    }

                    OpenSaveCreateReturnMessage status = loadPositionFile(fileUrl.path());
                    if(status == OK){
                        loadedPositionFile = lastLoadedProvider;
                        if(backgroundImage != "" || (backgroundImage == "" && drawPositionsOnBackground)) transformedBackground = transformBackgroundImage();
                        dynamic_cast<NeuroscopeApp*>(parent)->positionFileLoaded();
                    }
                }
            }
        }
        else
            dynamic_cast<NeuroscopeApp*>(parent)->createDisplay(channelsToDisplay,verticalLines,raster,waveforms,showLabels,multipleColumns,
                                                                greyMode,offsets,channelGains,selectedChannels,startTime,duration,rasterHeight,tabLabel);


        //the new view is the last one in the list of view (viewList)
        NeuroscopeView* view = viewList->last();

        //If the data file is not a dat file, do not display the waveforms but keep the information
        if(extension != "dat") view->ignoreWaveformInformation();

        //Inform the view of the available providers
        Q3ValueList<QString>::iterator providerIterator;
        //Cluster files
        for(providerIterator = loadedClusterFiles.begin(); providerIterator != loadedClusterFiles.end(); ++providerIterator){
            QString name = *providerIterator;
            QString fileURL = static_cast<QString>(providerUrls[name]).url();
            Q3ValueList<int> clustersIds;
            Q3ValueList<int> clustersIdsToSkip;
            Q3ValueList<int> ids = selectedClusters[fileURL];
            Q3ValueList<int> skippedIds = skippedClusters[fileURL];
            Q3ValueList<int> clusterList = static_cast<ClustersProvider*>(providers[name])->clusterIdList();
            //only keep the cluster ids which are still present
            Q3ValueList<int>::iterator shownClustersIterator;
            for(shownClustersIterator = ids.begin(); shownClustersIterator != ids.end(); ++shownClustersIterator)
                if(clusterList.contains(*shownClustersIterator)) clustersIds.append(*shownClustersIterator);
            Q3ValueList<int>::iterator skippedClustersIterator;
            for(skippedClustersIterator = skippedIds.begin(); skippedClustersIterator != skippedIds.end(); ++skippedClustersIterator)
                if(clusterList.contains(*skippedClustersIterator)) clustersIdsToSkip.append(*skippedClustersIterator);

            //an unselected cluster has to be skipped, check and correct if need it
            Q3ValueList<int>::iterator iterator;
            for(iterator = clusterList.begin(); iterator != clusterList.end(); ++iterator)
                if(!clustersIds.contains(*iterator) && !clustersIdsToSkip.contains(*iterator)) clustersIdsToSkip.append(*iterator);
            qSort(clustersIdsToSkip);
            view->setClusterProvider(static_cast<ClustersProvider*>(providers[name]),name,providerItemColors[name],true,clustersIds,
                                     &displayGroupsClusterFile,&channelsSpikeGroups,peakSampleIndex - 1,nbSamples - peakSampleIndex,clustersIdsToSkip);
        }
        //Event files
        for(providerIterator = loadedEventFiles.begin(); providerIterator != loadedEventFiles.end(); ++providerIterator){
            QString name = *providerIterator;
            QString fileURL = static_cast<QString>(providerUrls[name]).url();
            Q3ValueList<int> eventsIds;
            Q3ValueList<int> eventsIdsToSkip;
            Q3ValueList<int> ids = selectedEvents[fileURL];
            Q3ValueList<int> skippedIds = skippedEvents[fileURL];
            QMap<int,EventDescription> eventMap = static_cast<EventsProvider*>(providers[name])->eventIdDescriptionMap();
            //only keep the event ids which are still present
            QMap<EventDescription,int> loadedItems = loadedEventItems[name];
            ItemColors* eventColors = providerItemColors[name];
            Q3ValueList<int>::iterator shownEventsIterator;
            for(shownEventsIterator = ids.begin(); shownEventsIterator != ids.end(); ++shownEventsIterator){
                EventDescription description = EventDescription(eventColors->itemLabelById(*shownEventsIterator));
                if(eventMap.contains(*shownEventsIterator) && loadedItems.contains(description) && loadedItems[description] == *shownEventsIterator)
                    eventsIds.append(*shownEventsIterator);
            }
            Q3ValueList<int>::iterator skippedEventsIterator;
            for(skippedEventsIterator = skippedIds.begin(); skippedEventsIterator != skippedIds.end(); ++skippedEventsIterator){
                EventDescription description = EventDescription(eventColors->itemLabelById(*skippedEventsIterator));
                if(eventMap.contains(*skippedEventsIterator) && loadedItems.contains(description) && loadedItems[description] == *skippedEventsIterator)
                    eventsIdsToSkip.append(*skippedEventsIterator);
            }

            //an unselected event has to be skipped, check and correct if need it
            QMap<int,EventDescription>::iterator iterator;
            for(iterator = eventMap.begin(); iterator != eventMap.end(); ++iterator)
                if(!eventsIds.contains(iterator.key()) && !eventsIdsToSkip.contains(iterator.key())) eventsIdsToSkip.append(iterator.key());
            qSort(eventsIdsToSkip);

            view->setEventProvider(static_cast<EventsProvider*>(providers[name]),name,providerItemColors[name],true,eventsIds,eventsIdsToSkip);
        }
        //Position file
        if(loadedPositionFile != ""){
            if(isAPositionView){
                if(rotation != 90 && rotation != 270)
                    view->addPositionView(static_cast<PositionsProvider*>(providers[loadedPositionFile]),transformedBackground, dynamic_cast<NeuroscopeApp*>(parent)->getBackgroundColor(),
                                          startTime,duration,videoWidth,videoHeight,showEventsInPositionView);

                //If there is a rotation of 90 or 270 degree, the with and height have to be inverted.
                else view->addPositionView(static_cast<PositionsProvider*>(providers[loadedPositionFile]),transformedBackground, dynamic_cast<NeuroscopeApp*>(parent)->getBackgroundColor(),
                                           startTime,duration,videoHeight,videoWidth,showEventsInPositionView);

            }
        }
    }
}

void NeuroscopeDoc::setProviders(NeuroscopeView* activeView){  
    //the new view is the last one in the list of view (viewList)
    NeuroscopeView* newView = viewList->last();
    Q3DictIterator<DataProvider> iterator(providers);
    for(;iterator.current();++iterator){
        QString name = iterator.currentKey();
        if(iterator.current()->isA("ClustersProvider")){
            Q3ValueList<int> clusterIds = *(activeView->getSelectedClusters(name));
            Q3ValueList<int> clusterIdsToSkip = *(activeView->getClustersNotUsedForBrowsing(name));
            newView->setClusterProvider(static_cast<ClustersProvider*>(iterator.current()),name,providerItemColors[name],true
                                        ,clusterIds,&displayGroupsClusterFile,&channelsSpikeGroups,peakSampleIndex - 1,nbSamples - peakSampleIndex,clusterIdsToSkip);
        }
        if(iterator.current()->isA("EventsProvider")){
            Q3ValueList<int> eventIds = *(activeView->getSelectedEvents(name));
            Q3ValueList<int> eventIdsToSkip = *(activeView->getEventsNotUsedForBrowsing(name));
            newView->setEventProvider(static_cast<EventsProvider*>(iterator.current()),name,providerItemColors[name],true,eventIds,eventIdsToSkip);
        }
        if(iterator.current()->isA("PositionsProvider")){
            if(activeView->isPositionView()){
                if(rotation != 90 && rotation != 270)
                    newView->addPositionView(static_cast<PositionsProvider*>(iterator.current()),transformedBackground, dynamic_cast<NeuroscopeApp*>(parent)->getBackgroundColor(),
                                             videoWidth,videoHeight);

                //If there is a rotation of 90 or 270 degree, the with and height have to be inverted.
                else newView->addPositionView(static_cast<PositionsProvider*>(iterator.current()),transformedBackground, dynamic_cast<NeuroscopeApp*>(parent)->getBackgroundColor(),
                                              videoHeight,videoWidth);
            }
        }
    }
}

void NeuroscopeDoc::setWaveformInformation(int nb,int index,NeuroscopeView* activeView){
    nbSamples = nb;
    peakSampleIndex = index;

    NeuroscopeView* view;
    for(view = viewList->first(); view!=0; view = viewList->next()){
        if(view != activeView) view->updateWaveformInformation(peakSampleIndex - 1,nbSamples - peakSampleIndex,false);
        else view->updateWaveformInformation(peakSampleIndex - 1,nbSamples - peakSampleIndex,true);
    }
}


void NeuroscopeDoc::setPositionInformation(double newVideoSamplingRate, int newWidth, int newHeight, QString newBackgroundImage,
                                           int newRotation,int newFlip,bool positionsBackground,NeuroscopeView* activeView){

    videoSamplingRate = newVideoSamplingRate;
    bool newOrientation = false;
    if(rotation != newRotation || flip != newFlip) newOrientation = true;
    rotation = newRotation;
    flip = newFlip;
    backgroundImage = newBackgroundImage;
    videoWidth = newWidth;
    videoHeight = newHeight;
    drawPositionsOnBackground = positionsBackground;

    //Update the position provider
    Q3DictIterator<DataProvider> iterator(providers);
    for(;iterator.current();++iterator){
        if(iterator.current()->isA("PositionsProvider")){
            static_cast<PositionsProvider*>(iterator.current())->updateVideoInformation(videoSamplingRate,rotation,flip,videoWidth,videoHeight);
            break;
        }
    }

    //if a position file is open, update the sampling rate for the corresponding extension
    if(extensionSamplingRates.contains(positionFileExtension))
        extensionSamplingRates.insert(positionFileExtension,videoSamplingRate);

    if(backgroundImage != "" || (backgroundImage == "" && drawPositionsOnBackground)) transformedBackground = transformBackgroundImage();
    else transformedBackground.reset();

    //Update the views
    NeuroscopeView* view;
    if(rotation != 90 && rotation != 270){
        for(view = viewList->first(); view!=0; view = viewList->next()){
            if(view != activeView) view->updatePositionInformation(videoWidth,videoHeight,transformedBackground,newOrientation,false);
            else view->updatePositionInformation(videoWidth,videoHeight,transformedBackground,newOrientation,true);
        }
    }
    //If there is a rotation of 90 or 270 degree, the with and height have to be inverted.
    else{
        for(view = viewList->first(); view!=0; view = viewList->next()){
            if(view != activeView) view->updatePositionInformation(videoHeight,videoWidth,transformedBackground,newOrientation,false);
            else view->updatePositionInformation(videoHeight,videoWidth,transformedBackground,newOrientation,true);
        }
    }
}

QImage NeuroscopeDoc::transformBackgroundImage(bool useWhiteBackground){
    QImage image;

    //Draw the positions on the background if need it
    if(drawPositionsOnBackground){
        //Get the PositionProvider
        PositionsProvider* positionsProvider;
        Q3DictIterator<DataProvider> iterator(providers);
        for(;iterator.current();++iterator)
            if(iterator.current()->isA("PositionsProvider")){
                positionsProvider = static_cast<PositionsProvider*>(iterator.current());
                break;
            }
        //Create the image with the positions
        if(useWhiteBackground){
            ImageCreator creator(*positionsProvider,videoWidth,videoHeight,"",Qt::white);
            image = creator.createImage();
        }
        else{
            ImageCreator creator(*positionsProvider,videoWidth,videoHeight,backgroundImage,dynamic_cast<NeuroscopeApp*>(parent)->getBackgroundColor());
            image = creator.createImage();
        }
    }
    else image = QImage(backgroundImage);

    if(image != NULL){
        //apply first the rotation and then the flip
        QImage rotatedImage = image;

        if(rotation != 0){
            KImageEffect::RotateDirection angle;
            //KDE counts clockwise, to have a counterclock-wise rotation 90 and 270 are inverted
            if(rotation == 90) angle = KImageEffect::Rotate270;
            if(rotation == 180) angle = KImageEffect::Rotate180;
            if(rotation == 270) angle = KImageEffect::Rotate90;
            rotatedImage = KImageEffect::rotate(image,angle);
        }

        QImage flippedImage = rotatedImage;
        // 0 stands for none, 1 for vertical flip and 2 for horizontal flip.
        int flip = getFlip();
        if(flip != 0){
            bool horizontal;
            bool vertical;
            if(flip == 1){
                horizontal = false;
                vertical = true;
            }
            else{
                horizontal = true;
                vertical = false;
            }
            flippedImage = rotatedImage.mirror(horizontal,vertical);
        }
        return flippedImage;
    }

    return image;
}


void NeuroscopeDoc::selectAllChannels(NeuroscopeView& activeView,bool editMode){
    Q3ValueList<int> channelsSelected = displayChannelsGroups.keys();

    //The new selection of channels only means for the active view
    if(editMode) activeView.setSelectedChannels(channelsSelected);
    else activeView.shownChannelsUpdate(channelsSelected);
}

void NeuroscopeDoc::showAllClustersExcept(ItemPalette* clusterPalette,NeuroscopeView* activeView,Q3ValueList<int> clustersToHide){
    Q3DictIterator<DataProvider> iterator(providers);
    for(;iterator.current();++iterator){
        QString providerName = iterator.currentKey();
        if(iterator.current()->isA("ClustersProvider")){
            Q3ValueList<int> clusterList = static_cast<ClustersProvider*>(iterator.current())->clusterIdList();
            Q3ValueList<int> clustersToShow;

            if(clustersToHide.isEmpty()){
                //The new selection of clusters only means for the active view
                activeView->shownClustersUpdate(providerName,clusterList);
            }
            else{
                Q3ValueList<int>::iterator clustersToAdd;
                for(clustersToAdd = clusterList.begin(); clustersToAdd != clusterList.end(); ++clustersToAdd ){
                    if(!clustersToHide.contains(*clustersToAdd)) clustersToShow.append(*clustersToAdd);
                }

                const Q3ValueList<int>* skippedClusterIds = activeView->getClustersNotUsedForBrowsing(providerName);
                clusterPalette->selectItems(providerName,clustersToShow,*skippedClusterIds);
                //The new selection of clusters only means for the active view
                activeView->shownClustersUpdate(providerName,clustersToShow);
            }
        }
        if(clustersToHide.isEmpty()) clusterPalette->selectAllItems();
    }
}

void NeuroscopeDoc::deselectAllClusters(ItemPalette* clusterPalette,NeuroscopeView* activeView){
    Q3DictIterator<DataProvider> iterator(providers);
    for(;iterator.current();++iterator){
        QString providerName = iterator.currentKey();
        if(iterator.current()->isA("ClustersProvider")){
            Q3ValueList<int> clustersToShow;
            //The new selection of clusters only means for the active view
            activeView->shownClustersUpdate(providerName,clustersToShow);
        }
    }
    clusterPalette->deselectAllItems();
}

void NeuroscopeDoc::showAllEvents(ItemPalette* eventPalette,NeuroscopeView* activeView){
    Q3DictIterator<DataProvider> iterator(providers);
    for(;iterator.current();++iterator){
        QString providerName = iterator.currentKey();
        if(iterator.current()->isA("EventsProvider")){
            QMap<EventDescription,int> events = static_cast<EventsProvider*>(iterator.current())->eventDescriptionIdMap();
            Q3ValueList<int> eventList = events.values();
            //The new selection of events only means for the active view
            activeView->shownEventsUpdate(providerName,eventList);
        }
    }
    eventPalette->selectAllItems();
}

void NeuroscopeDoc::deselectAllEvents(ItemPalette* eventPalette,NeuroscopeView* activeView){
    Q3DictIterator<DataProvider> iterator(providers);
    for(;iterator.current();++iterator){
        QString providerName = iterator.currentKey();
        if(iterator.current()->isA("EventsProvider")){
            Q3ValueList<int> eventsToShow;
            //The new selection of events only means for the active view
            activeView->shownEventsUpdate(providerName,eventsToShow);
        }
    }
    eventPalette->deselectAllItems();
}

void NeuroscopeDoc::deselectAllChannels(NeuroscopeView& activeView,bool editMode){
    Q3ValueList<int> channelsSelected;

    //The new selection of channels only means for the active view
    if(editMode) activeView.setSelectedChannels(channelsSelected);
    else activeView.shownChannelsUpdate(channelsSelected);
}

void NeuroscopeDoc::synchronize(){
    //The information for spikes detection become the same as the one for displaying the field potentials.
    channelsSpikeGroups.clear();
    spikeGroupsChannels.clear();

    QMap<int,int>::Iterator iterator;
    for(iterator = displayChannelsGroups.begin(); iterator != displayChannelsGroups.end(); ++iterator){
        //set the color for the spike group to the one for the display group.
        QColor color = channelColorList->groupColor(iterator.key());
        channelColorList->setSpikeGroupColor(iterator.key(),color);
        channelsSpikeGroups.insert(iterator.key(),iterator.data());
    }

    QMap<int, Q3ValueList<int> >::Iterator iterator2;
    for(iterator2 = displayGroupsChannels.begin(); iterator2 != displayGroupsChannels.end(); ++iterator2){
        spikeGroupsChannels.insert(iterator2.key(),iterator2.data());
    }
}

long long NeuroscopeDoc::recordingLength(){
    return tracesProvider->recordingLength();
}

void NeuroscopeDoc::setNoneEditMode(NeuroscopeView* activeView){
    //Notify all the views of the modification
    //In none edit mode, the shown channels becom the selected ones.
    NeuroscopeView* view;
    for(view = viewList->first(); view!=0; view = viewList->next()){
        if(view != activeView){
            view->setMode(TraceView::ZOOM,false);
            view->setSelectedChannels(view->channels());
        }
        else{
            view->setMode(TraceView::ZOOM,true);
            view->setSelectedChannels(view->channels());
        }
    }
}


NeuroscopeDoc::OpenSaveCreateReturnMessage NeuroscopeDoc::loadClusterFile(QString clusterUrl,NeuroscopeView* activeView){
    //Check that the selected file is a cluster file
    QString fileName = clusterUrl.fileName();
    if(fileName.find(".clu") == -1) return INCORRECT_FILE;

    ClustersProvider* clustersProvider = new ClustersProvider(clusterUrl,datSamplingRate,samplingRate,tracesProvider->getTotalNbSamples(),clusterPosition);
    QString name = clustersProvider->getName();

    //The name should only contains digits
    if(name.contains(QRegExp("\\D")) != 0){
        delete clustersProvider;
        return INCORRECT_FILE;
    }

    if(providers.find(name) != 0 && providers.find(name)->isA("ClustersProvider")){
        delete clustersProvider;
        return ALREADY_OPENED;
    }

    int returnStatus = clustersProvider->loadData();
    if(returnStatus == ClustersProvider::OPEN_ERROR){
        delete clustersProvider;
        return OPEN_ERROR;
    }
    else if(returnStatus == ClustersProvider::MISSING_FILE){
        delete clustersProvider;
        return MISSING_FILE;
    }
    else if(returnStatus == ClustersProvider::COUNT_ERROR){
        delete clustersProvider;
        return CREATION_ERROR;
    }
    else if(returnStatus == ClustersProvider::INCORRECT_CONTENT){
        delete clustersProvider;
        return INCORRECT_CONTENT;
    }

    lastLoadedProvider = name;
    providers.insert(name,clustersProvider);
    providerUrls.insert(name,clusterUrl);

    ItemColors* clusterColors = new ItemColors();
    Q3ValueList<int> clustersToSkip;
    //Constructs the clusterColorList and clustersToSkip
    Q3ValueList<int> clusterList = clustersProvider->clusterIdList();
    Q3ValueList<int>::iterator it;
    for(it = clusterList.begin(); it != clusterList.end(); ++it){
        QColor color;
        if(*it == 1) color.setHsv(0,0,220);//Cluster 1 is always gray
        else if(*it == 0) color.setHsv(0,255,255);//Cluster 0 is always red
        else color.setHsv(static_cast<int>(fmod(static_cast<double>(*it)*7,36))*10,255,255);
        clusterColors->append(static_cast<int>(*it),color);
        clustersToSkip.append(static_cast<int>(*it));
    }

    providerItemColors.insert(name,clusterColors);


    if(displayGroupsClusterFile.isEmpty())
        //compute which cluster files give data for a given anatomical group
        computeClusterFilesMapping();

    //Informs the views than there is a new cluster provider.
    NeuroscopeView* view;
    Q3ValueList<int> clustersToShow;
    for(view = viewList->first(); view!=0; view = viewList->next()){
        if(view != activeView) view->setClusterProvider(clustersProvider,name,clusterColors,false,clustersToShow,&displayGroupsClusterFile,&channelsSpikeGroups,peakSampleIndex - 1,nbSamples - peakSampleIndex,clustersToSkip);
        else view->setClusterProvider(clustersProvider,name,clusterColors,true,clustersToShow,&displayGroupsClusterFile,&channelsSpikeGroups,peakSampleIndex - 1,nbSamples - peakSampleIndex,clustersToSkip);
    }

    return OK;
}

NeuroscopeDoc::OpenSaveCreateReturnMessage NeuroscopeDoc::loadClusterFile(QString clusterUrl,QMap<EventDescription,QColor>& itemColors,QDateTime lastModified,bool firstFile){
    //Check that the selected file is a cluster file (should always be the case as the file has
    //already be loaded once).
    QString fileName = clusterUrl.fileName();
    if(fileName.find(".clu") == -1){
        QApplication::restoreOverrideCursor();
        KMessageBox::error(0,tr("The requested cluster file " + clusterUrl.path() + " has an incorrect name, it has to be of the form baseName.n.clu or baseName.clu.n, with n a number identifier."
                                + "Therefore will not be loaded."), tr("Error!"));
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
        return INCORRECT_FILE;
    }


    //check if the file still exist before trying to load it
    QFileInfo fileInfo = QFileInfo(clusterUrl.path());

    if(!fileInfo.exists()){
        QApplication::restoreOverrideCursor();
        KMessageBox::error (0,tr("The file " + clusterUrl.path() + " does not exist anymore."), tr("Error!"));
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
        return OPEN_ERROR;
    }


    bool modified = false;
    //check if the file has been modified since the last session.
    if(fileInfo.lastModified() != lastModified) modified = true;

    ClustersProvider* clustersProvider = new ClustersProvider(clusterUrl,datSamplingRate,samplingRate,tracesProvider->getTotalNbSamples(),clusterPosition);
    QString name = clustersProvider->getName();

    //The name should only contains digits
    if(name.contains(QRegExp("\\D")) != 0){
        delete clustersProvider;
        QApplication::restoreOverrideCursor();
        KMessageBox::error(0,tr("The requested cluster file " + clusterUrl.path() + " has an incorrect name, it has to be of the form baseName.n.clu or baseName.clu.n, with n a number identifier."
                                + "Therefore will not be loaded."), tr("Error!"));
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
        return INCORRECT_FILE;
    }

    int returnStatus = clustersProvider->loadData();
    if(returnStatus == ClustersProvider::OPEN_ERROR){
        delete clustersProvider;
        QApplication::restoreOverrideCursor();
        KMessageBox::error (0,tr("Could not load the file " + clusterUrl.path()), tr("Error!"));
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
        return OPEN_ERROR;
    }
    else if(returnStatus == ClustersProvider::MISSING_FILE){
        delete clustersProvider;
        QApplication::restoreOverrideCursor();
        KMessageBox::error (0,tr("There is no time file (.res) corresponding to the requested file " + clusterUrl.path() +
                                 ", therefore this file will not be loaded."), tr("Error!"));
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
        return MISSING_FILE;
    }
    else if(returnStatus == ClustersProvider::COUNT_ERROR){
        delete clustersProvider;
        QApplication::restoreOverrideCursor();
        KMessageBox::error (0,tr("The number of spikes of the requested file " + clusterUrl.path() + " could not be determined."
                                 " Therefore this file will not be loaded."), tr("Error!"));
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
        return CREATION_ERROR;
    }
    else if(returnStatus == ClustersProvider::INCORRECT_CONTENT){
        delete clustersProvider;
        QApplication::restoreOverrideCursor();
        KMessageBox::error (0,tr("The number of spikes read in the requested file  " + clusterUrl.path() + " or the corresponding time file (.res) does not correspond to number of spikes computed." +
                                 " Therefore this file will not be loaded."), tr("Error!"));
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
        return INCORRECT_CONTENT;
    }

    providers.insert(name,clustersProvider);
    providerUrls.insert(name,clusterUrl);
    lastLoadedProvider = name;

    ItemColors* clusterColors = new ItemColors();
    //Build the clusterColorList
    Q3ValueList<int> clusterList = clustersProvider->clusterIdList();
    Q3ValueList<int>::iterator it;
    for(it = clusterList.begin(); it != clusterList.end(); ++it){
        if(itemColors.contains(QString("%1").arg(*it))){
            clusterColors->append(static_cast<int>(*it),itemColors[QString("%1").arg(*it)]);
        }
        else{
            modified = true;
            QColor color;
            if(*it == 1) color.setHsv(0,0,220);//Cluster 1 is always gray
            else if(*it == 0) color.setHsv(0,255,255);//Cluster 0 is always red
            else color.setHsv(static_cast<int>(fmod(static_cast<double>(*it)*7,36))*10,255,255);
            clusterColors->append(static_cast<int>(*it),color);
        }
    }

    providerItemColors.insert(name,clusterColors);

    if(modified == true){
        QApplication::restoreOverrideCursor();
        KMessageBox::information(0,tr("The requested file " + clusterUrl.path() + " has been modified since the last session,"
                                      " therefore some session information may be lost."), tr("Error!"));
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    }

    if(firstFile) dynamic_cast<NeuroscopeApp*>(parent)->createClusterPalette(name);
    else dynamic_cast<NeuroscopeApp*>(parent)->addClusterFile(name);

    return OK;
}

void NeuroscopeDoc::removeClusterFile(QString providerName,NeuroscopeView* activeView){  
    //Informs the views than the cluster provider will be removed.
    NeuroscopeView* view;
    for(view = viewList->first(); view!=0; view = viewList->next()){
        if(view != activeView) view->removeClusterProvider(providerName,false);
        else view->removeClusterProvider(providerName,true);
    }

    providers.remove(providerName);
    providerItemColors.remove(providerName);
    providerUrls.remove(providerName);
}

void NeuroscopeDoc::clusterColorUpdate(QString providerName,int clusterId,NeuroscopeView* activeView){
    //Notify all the views of the modification
    NeuroscopeView* view;
    for(view = viewList->first(); view!=0; view = viewList->next()){
        if(view != activeView) view->clusterColorUpdate(providerName,clusterId,false);
        else view->clusterColorUpdate(providerName,clusterId,true);
    }

    //Ask the active view to take the modification into account immediately
    activeView->showAllWidgets();
}

void NeuroscopeDoc::setClusterPosition(int position){
    clusterPosition = position;
    QMap<QString,QString>::Iterator iterator;
    for(iterator = providerUrls.begin(); iterator != providerUrls.end(); ++iterator){
        DataProvider* provider = providers[iterator.key()];
        if(provider->isA("ClustersProvider")) static_cast<ClustersProvider*>(provider)->setClusterPosition(position);
    }
}

NeuroscopeDoc::OpenSaveCreateReturnMessage NeuroscopeDoc::loadEventFile(QString eventUrl,NeuroscopeView*activeView){
    //Check that the selected file is a event file
    QString fileName = eventUrl.fileName();
    if(fileName.find(".evt") == -1) return INCORRECT_FILE;

    EventsProvider* eventsProvider = new EventsProvider(eventUrl,samplingRate,eventPosition);
    QString name = eventsProvider->getName();

    //The name should contains 3 characters with at least one none digit character.
    if(name.length() != 3 || name.contains(QRegExp("\\d{3}"))){
        delete eventsProvider;
        return INCORRECT_FILE;
    }

    if(providers.find(name) != 0 && providers.find(name)->isA("EventsProvider")){
        delete eventsProvider;
        return ALREADY_OPENED;
    }

    int returnStatus = eventsProvider->loadData();
    if(returnStatus == EventsProvider::OPEN_ERROR){
        delete eventsProvider;
        return OPEN_ERROR;
    }
    else if(returnStatus == EventsProvider::COUNT_ERROR){
        delete eventsProvider;
        return CREATION_ERROR;
    }
    else if(returnStatus == EventsProvider::INCORRECT_CONTENT){
        delete eventsProvider;
        return INCORRECT_CONTENT;
    }

    lastLoadedProvider = name;
    lastEventProviderGridX = eventsProvider->getDescriptionLength();
    providers.insert(name,eventsProvider);
    providerUrls.insert(name,eventUrl);

    ItemColors* eventColors = new ItemColors();
    Q3ValueList<int> eventsToSkip;
    //Constructs the eventColorList and eventsToSkip
    //An id is assign to each event, this id will be used internally in NeuroScope and in the session file.
    QMap<EventDescription,int> eventMap = eventsProvider->eventDescriptionIdMap();
    QMap<EventDescription,int>::Iterator it;
    for(it = eventMap.begin(); it != eventMap.end(); ++it){
        QColor color;
        color.setHsv(static_cast<int>(fmod(static_cast<double>(it.data())*7,36))*10,255,255);
        eventColors->append(static_cast<int>(it.data()),static_cast<QString>(it.key()),color);
        eventsToSkip.append(static_cast<int>(it.data()));
    }

    providerItemColors.insert(name,eventColors);

    //Install the connections with the provider
    connect(eventsProvider, SIGNAL(newEventDescriptionCreated(QString,QMap<int,int>,QMap<int,int>,QString)),this, SLOT(slotNewEventDescriptionCreated(QString,QMap<int,int>,QMap<int,int>,QString)));
    connect(eventsProvider, SIGNAL(eventDescriptionRemoved(QString,QMap<int,int>,QMap<int,int>,int,QString)),this, SLOT(slotEventDescriptionRemoved(QString,QMap<int,int>,QMap<int,int>,int,QString)));

    //Informs the views than there is a new event provider.
    NeuroscopeView* view;
    Q3ValueList<int> eventsToShow;
    for(view = viewList->first(); view!=0; view = viewList->next()){
        if(view != activeView) view->setEventProvider(eventsProvider,name,eventColors,false,eventsToShow,eventsToSkip);
        else view->setEventProvider(eventsProvider,name,eventColors,true,eventsToShow,eventsToSkip);
    }

    return OK;
}

NeuroscopeDoc::OpenSaveCreateReturnMessage NeuroscopeDoc::loadEventFile(QString eventUrl,QMap<EventDescription,QColor>& itemColors,QDateTime lastModified,bool firstFile){
    //Check that the selected file is a event file (should always be the case as the file has
    //already be loaded once).
    QString fileName = eventUrl.fileName();
    if(fileName.find(".evt") == -1){
        QApplication::restoreOverrideCursor();
        KMessageBox::error (0,tr("The requested event file " + eventUrl.path() + " has an incorrect name, it has to be of the form baseName.id.evt or baseName.evt.id (with id a 3 character identifier). Therefore it will not be loaded."), tr("Error!"));
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
        return INCORRECT_FILE;
    }

    //check if the file still exist before trying to load it
    QFileInfo fileInfo = QFileInfo(eventUrl.path());

    if(!fileInfo.exists()){
        QApplication::restoreOverrideCursor();
        KMessageBox::error (0,tr("The file " + eventUrl.path() + " does not exist anymore."), tr("Error!"));
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
        return OPEN_ERROR;
    }

    bool modified = false;
    if(fileInfo.lastModified() != lastModified) modified = true;

    EventsProvider* eventsProvider = new EventsProvider(eventUrl,samplingRate,eventPosition);
    QString name = eventsProvider->getName();

    //The name should be of 3 characters length with at least one none digit character.
    if(name.length() != 3 || name.contains(QRegExp("\\d{3}"))){
        delete eventsProvider;
        QApplication::restoreOverrideCursor();
        KMessageBox::error (0,tr("The requested event file " + eventUrl.path() + " has an incorrect name, it has to be of the form baseName.id.evt or baseName.evt.id (with id a 3 character identifier). Therefore it will not be loaded."), tr("Error!"));
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
        return INCORRECT_FILE;
    }

    int returnStatus = eventsProvider->loadData();

    if(returnStatus == EventsProvider::OPEN_ERROR){
        delete eventsProvider;
        QApplication::restoreOverrideCursor();
        KMessageBox::error (0,tr("Could not load the file " + eventUrl.path()), tr("Error!"));
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
        return OPEN_ERROR;
    }
    else if(returnStatus == EventsProvider::COUNT_ERROR){
        delete eventsProvider;
        QApplication::restoreOverrideCursor();
        KMessageBox::error (0,tr("The number of events of the requested file " + eventUrl.path() + " could not be determined."
                                 " Therefore this file will not be loaded."), tr("Error!"));
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
        return CREATION_ERROR;
    }
    else if(returnStatus == EventsProvider::INCORRECT_CONTENT){
        delete eventsProvider;
        QApplication::restoreOverrideCursor();
        KMessageBox::error (0,tr("The content of the requested file " + eventUrl.path() + " is incorrect." +
                                 " Therefore this file will not be loaded."), tr("Error!"));
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
        return INCORRECT_CONTENT;
    }

    providers.insert(name,eventsProvider);
    providerUrls.insert(name,eventUrl);
    lastLoadedProvider = name;
    lastEventProviderGridX = eventsProvider->getDescriptionLength();

    //Buil the eventColors
    ItemColors* eventColors = new ItemColors();
    QMap<EventDescription,int> eventMap = eventsProvider->eventDescriptionIdMap();
    //if possible, use the data from the session file to build the eventColorList.
    //The iterator gives the keys sorted.
    //An id is assign to each event, this id will be used internally in NeuroScope and in the session file.
    QMap<EventDescription,int>::Iterator it;
    for(it = eventMap.begin(); it != eventMap.end(); ++it){
        if(itemColors.contains(it.key())){
            eventColors->append(static_cast<int>(it.data()),static_cast<QString>(it.key()),itemColors[it.key()]);
        }
        else{
            modified = true;
            QColor color;
            color.setHsv(static_cast<int>(fmod(static_cast<double>(it.data())*7,36))*10,255,255);
            eventColors->append(static_cast<int>(it.data()),static_cast<QString>(it.key()),color);
        }
    }

    providerItemColors.insert(name,eventColors);

    if(modified == true){
        QApplication::restoreOverrideCursor();
        KMessageBox::information(0,tr("The requested file " + eventUrl.path() + " has been modified since the last session,"
                                      " therefore some session information may be lost."), tr("Error!"));
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    }

    //Install the connections with the provider
    connect(eventsProvider, SIGNAL(newEventDescriptionCreated(QString,QMap<int,int>,QMap<int,int>,QString)),this, SLOT(slotNewEventDescriptionCreated(QString,QMap<int,int>,QMap<int,int>,QString)));
    connect(eventsProvider, SIGNAL(eventDescriptionRemoved(QString,QMap<int,int>,QMap<int,int>,int,QString)),this, SLOT(slotEventDescriptionRemoved(QString,QMap<int,int>,QMap<int,int>,int,QString)));

    if(firstFile) dynamic_cast<NeuroscopeApp*>(parent)->createEventPalette(name);
    else dynamic_cast<NeuroscopeApp*>(parent)->addEventFile(name);

    return OK;
}

void NeuroscopeDoc::removeEventFile(QString providerName,NeuroscopeView* activeView,bool lastFile){
    //Informs the views than the event provider will be removed.
    NeuroscopeView* view;
    for(view = viewList->first(); view!=0; view = viewList->next()){
        if(view != activeView) view->removeEventProvider(providerName,false,lastFile);
        else view->removeEventProvider(providerName,true,lastFile);
    }

    providers.remove(providerName);
    providerItemColors.remove(providerName);
    providerUrls.remove(providerName);
}

void NeuroscopeDoc::eventColorUpdate(QString providerName,int eventId,NeuroscopeView* activeView){
    //Notify all the views of the modification
    NeuroscopeView* view;
    for(view = viewList->first(); view!=0; view = viewList->next()){
        if(view != activeView) view->eventColorUpdate(providerName,eventId,false);
        else view->eventColorUpdate(providerName,eventId,true);
    }

    //Ask the active view to take the modification into account immediately
    activeView->showAllWidgets();
}

void NeuroscopeDoc::setEventPosition(int position){
    eventPosition = position;
    QMap<QString,QString>::Iterator iterator;
    for(iterator = providerUrls.begin(); iterator != providerUrls.end(); ++iterator){
        DataProvider* provider = providers[iterator.key()];
        if(provider->isA("EventsProvider")) static_cast<EventsProvider*>(provider)->setEventPosition(position);
    }
}

void NeuroscopeDoc::eventModified(QString providerName,int selectedEventId,double time,double newTime,NeuroscopeView* activeView){
    //clear the undo/redo data of all the event providers except providerName
    Q3DictIterator<DataProvider> it(providers);
    for( ; it.current(); ++it){
        QString name = it.currentKey();
        if(it.current()->isA("EventsProvider") && name != providerName){
            static_cast<EventsProvider*>(it.current())->clearUndoRedoData();
        }
    }

    //Informs the views than an event has been modified.
    NeuroscopeView* view;
    for(view = viewList->first(); view!=0; view = viewList->next()){
        if(view != activeView) view->updateEvents(providerName,selectedEventId,time,newTime,false);
    }

    //Prepare the undo/redo mechanism
    undoRedoProviderName = providerName;
    undoRedoEventId = selectedEventId;
    undoRedoEventTime = time;
    modifiedEventTime = newTime;
}


void NeuroscopeDoc::eventRemoved(QString providerName,int selectedEventId,double time,NeuroscopeView* activeView){
    //clear the undo/redo data of all the event providers except providerName
    Q3DictIterator<DataProvider> it(providers);
    for( ; it.current(); ++it){
        QString name = it.currentKey();
        if(it.current()->isA("EventsProvider") && name != providerName){
            static_cast<EventsProvider*>(it.current())->clearUndoRedoData();
        }
    }

    //Informs the views than an event has been removed.
    NeuroscopeView* view;
    for(view = viewList->first(); view!=0; view = viewList->next()){
        if(view != activeView) view->updateEventsAfterRemoval(providerName,selectedEventId,time,false);
        else view->updateEventsAfterRemoval(providerName,selectedEventId,time,true);
    }

    //Prepare the undo/redo mechanism
    undoRedoProviderName = providerName;
    undoRedoEventId = selectedEventId;
    undoRedoEventTime = time;
    modifiedEventTime = -1;
}


void NeuroscopeDoc::eventAdded(QString providerName,QString addEventDescription,double time,NeuroscopeView* activeView){
    int addedEventId = 0;

    //clear the undo/redo data of all the event providers except providerName and lookup for the selectedEventId
    Q3DictIterator<DataProvider> it(providers);
    for( ; it.current(); ++it){
        QString name = it.currentKey();
        if(it.current()->isA("EventsProvider") && name != providerName){
            static_cast<EventsProvider*>(it.current())->clearUndoRedoData();
        }
        else if(it.current()->isA("EventsProvider") && name == providerName){
            QMap<EventDescription,int> eventMap = static_cast<EventsProvider*>(it.current())->eventDescriptionIdMap();
            addedEventId = eventMap[addEventDescription];
        }
    }

    //Informs the views than an event has been added only if the added event in not of a new type.
    //In that case the views have already be informed by a call to updateSelectedEventsIds (via slotNewEventDescriptionCreated)
    if(!newEventDescriptionCreated){
        NeuroscopeView* view;
        for(view = viewList->first(); view!=0; view = viewList->next()){
            if(view != activeView) view->updateEventsAfterAddition(providerName,addedEventId,time,false);
            else view->updateEventsAfterAddition(providerName,addedEventId,time,true);
        }
    }
    else newEventDescriptionCreated = false;


    //Update the event palette
    ItemPalette* eventPalette = dynamic_cast<NeuroscopeApp*>(parent)->getEventPalette();
    const Q3ValueList<int>* selectedEvents = activeView->getSelectedEvents(providerName);
    const Q3ValueList<int>* skippedEvents = activeView->getEventsNotUsedForBrowsing(providerName);
    eventPalette->selectItems(providerName,*selectedEvents,*skippedEvents);

    //Prepare the undo/redo mechanism
    undoRedoProviderName = providerName;
    undoRedoEventId = addedEventId;
    undoRedoEventTime = time;
    modifiedEventTime = -1;
}

void NeuroscopeDoc::undo(NeuroscopeView* activeView){
    float time = modifiedEventTime;
    modifiedEventTime = undoRedoEventTime;
    undoRedoEventTime = time;

    static_cast<EventsProvider*>(providers[undoRedoProviderName])->undo();
    newEventDescriptionCreated = false;
    NeuroscopeView* view;
    if(undoRedoEventTime != -1){
        //Informs the views than an event has been modified.
        for(view = viewList->first(); view!=0; view = viewList->next()){
            if(view != activeView) view->updateEvents(undoRedoProviderName,undoRedoEventId,undoRedoEventTime,modifiedEventTime,false);
            else view->updateEvents(undoRedoProviderName,undoRedoEventId,undoRedoEventTime,modifiedEventTime,true);
        }
    }
    else{
        //Informs the views than an event has been added back (event previously removed) or added (event previously removed).
        for(view = viewList->first(); view!=0; view = viewList->next()){
            if(view != activeView) view->updateEvents(undoRedoProviderName,undoRedoEventId,modifiedEventTime,false);
            else view->updateEvents(undoRedoProviderName,undoRedoEventId,modifiedEventTime,true);
        }
    }
}

void NeuroscopeDoc::redo(NeuroscopeView* activeView){
    float time = modifiedEventTime;
    modifiedEventTime = undoRedoEventTime;
    undoRedoEventTime = time;

    static_cast<EventsProvider*>(providers[undoRedoProviderName])->redo();
    newEventDescriptionCreated = false;
    NeuroscopeView* view;
    if(modifiedEventTime != -1){
        //Informs the views than an event has been modified.
        for(view = viewList->first(); view!=0; view = viewList->next()){
            if(view != activeView) view->updateEvents(undoRedoProviderName,undoRedoEventId,undoRedoEventTime,modifiedEventTime,false);
            else view->updateEvents(undoRedoProviderName,undoRedoEventId,undoRedoEventTime,modifiedEventTime,true);
        }
    }
    else{
        //Informs the views than an event has been added back (event previously removed) or removed (event previously added).
        for(view = viewList->first(); view!=0; view = viewList->next()){
            if(view != activeView) view->updateEvents(undoRedoProviderName,undoRedoEventId,undoRedoEventTime,false);
            else view->updateEvents(undoRedoProviderName,undoRedoEventId,undoRedoEventTime,true);
        }
    }
}

Q3ValueList<EventDescription> NeuroscopeDoc::eventIds(QString providerName){
    QMap<EventDescription,int> eventMap;

    Q3DictIterator<DataProvider> it(providers);
    for( ; it.current(); ++it){
        QString name = it.currentKey();
        if(it.current()->isA("EventsProvider") && name == providerName){
            eventMap = static_cast<EventsProvider*>(it.current())->eventDescriptionIdMap();
            break;
        }
    }
    return eventMap.keys();
}

void NeuroscopeDoc::slotNewEventDescriptionCreated(QString providerName,QMap<int,int> oldNewEventIds,QMap<int,int> newOldEventIds,QString eventDescriptionAdded){
    newEventDescriptionCreated = true;
    EventsProvider* eventsProvider = static_cast<EventsProvider*>(providers[providerName]);

    //Constructs the new eventColorList
    //An id is assign to each event, this id will be used internally in NeuroScope and in the session file.
    ItemColors* eventColors = new ItemColors();
    QMap<EventDescription,int> eventMap = eventsProvider->eventDescriptionIdMap();
    ItemColors* currentEventColors = providerItemColors[providerName];
    int addedEventId = eventMap[EventDescription(eventDescriptionAdded)];
    QMap<EventDescription,int>::Iterator it;
    for(it = eventMap.begin(); it != eventMap.end(); ++it){
        //use the current color
        if(it.key() != eventDescriptionAdded){
            QColor color = currentEventColors->color(newOldEventIds[static_cast<int>(it.data())]);
            eventColors->append(static_cast<int>(it.data()),static_cast<QString>(it.key()),color);
        }
        else{
            QColor color;
            if(eventDescriptionAdded == removedDescription.first){
                color = QColor(removedDescription.second);
            }
            else color.setHsv(static_cast<int>(fmod(static_cast<double>(it.data())*7,36))*10,255,255);
            eventColors->append(static_cast<int>(it.data()),static_cast<QString>(it.key()),color);
        }
    }

    currentEventColors->removeAll();
    for(it = eventMap.begin(); it != eventMap.end(); ++it){
        QColor color = eventColors->color(static_cast<int>(it.data()));
        currentEventColors->append(static_cast<int>(it.data()),static_cast<QString>(it.key()),color);
    }


    //Update the event palette
    ItemPalette* eventPalette = dynamic_cast<NeuroscopeApp*>(parent)->getEventPalette();
    eventPalette->removeGroup(providerName);
    eventPalette->createItemList(currentEventColors,providerName,eventsProvider->getDescriptionLength());
    eventPalette->selectGroup(providerName);

    //Informs the views than the event ids have changed.
    NeuroscopeView* view;
    NeuroscopeView* activeView = dynamic_cast<NeuroscopeApp*>(parent)->activeView();
    for(view = viewList->first(); view!=0; view = viewList->next())
        if(view != activeView) view->updateSelectedEventsIds(providerName,oldNewEventIds,addedEventId,false,true);
        else view->updateSelectedEventsIds(providerName,oldNewEventIds,addedEventId,true,true);

    //Get the active view and update the eventPalette with the selected events.
    const Q3ValueList<int>* selectedEvents = activeView->getSelectedEvents(providerName);
    const Q3ValueList<int>* skippedEvents = activeView->getEventsNotUsedForBrowsing(providerName);
    eventPalette->selectItems(providerName,*selectedEvents,*skippedEvents);
}

void NeuroscopeDoc::slotEventDescriptionRemoved(QString providerName,QMap<int,int> oldNewEventIds,QMap<int,int> newOldEventIds,int eventIdToRemove,QString eventDescriptionToRemove){
    EventsProvider* eventsProvider = static_cast<EventsProvider*>(providers[providerName]);

    //Constructs the new eventColorList
    ItemColors* eventColors = new ItemColors();
    QMap<EventDescription,int> eventMap = eventsProvider->eventDescriptionIdMap();
    ItemColors* currentEventColors = providerItemColors[providerName];

    removedDescription.first = eventDescriptionToRemove;
    removedDescription.second = currentEventColors->color(eventIdToRemove).name();

    QMap<EventDescription,int>::Iterator it;
    for(it = eventMap.begin(); it != eventMap.end(); ++it){
        //use the current color
        QColor color = currentEventColors->color(newOldEventIds[static_cast<int>(it.data())]);
        eventColors->append(static_cast<int>(it.data()),static_cast<QString>(it.key()),color);
    }

    currentEventColors->removeAll();
    for(it = eventMap.begin(); it != eventMap.end(); ++it){
        QColor color = eventColors->color(static_cast<int>(it.data()));
        currentEventColors->append(static_cast<int>(it.data()),static_cast<QString>(it.key()),color);
    }


    //Update the event palette
    ItemPalette* eventPalette = dynamic_cast<NeuroscopeApp*>(parent)->getEventPalette();
    eventPalette->removeGroup(providerName);
    eventPalette->createItemList(currentEventColors,providerName,eventsProvider->getDescriptionLength());
    eventPalette->selectGroup(providerName);

    //Informs the views than the event ids have changed.
    NeuroscopeView* view;
    NeuroscopeView* activeView = dynamic_cast<NeuroscopeApp*>(parent)->activeView();
    for(view = viewList->first(); view!=0; view = viewList->next())
        if(view != activeView) view->updateSelectedEventsIds(providerName,oldNewEventIds,eventIdToRemove,false,false);
        else view->updateSelectedEventsIds(providerName,oldNewEventIds,eventIdToRemove,true,false);

    //Get the active view and update the eventPalette with the selected events.
    const Q3ValueList<int>* selectedEvents = activeView->getSelectedEvents(providerName);
    const Q3ValueList<int>* skippedEvents = activeView->getEventsNotUsedForBrowsing(providerName);
    eventPalette->selectItems(providerName,*selectedEvents,*skippedEvents);
}

NeuroscopeDoc::OpenSaveCreateReturnMessage NeuroscopeDoc::createEventFile(QString eventUrl,NeuroscopeView*activeView){
    //Check that the selected file is a event file name
    QString fileName = eventUrl.fileName();
    if(fileName.find(".evt") == -1) return INCORRECT_FILE;

    EventsProvider* eventsProvider = new EventsProvider(eventUrl,samplingRate,eventPosition);
    QString name = eventsProvider->getName();

    //The name should be of 3 characters length with at least one none digit character.
    if(name.length() != 3 || name.contains(QRegExp("\\d{3}"))){
        delete eventsProvider;
        return INCORRECT_FILE;
    }

    if(providers.find(name) != 0){
        delete eventsProvider;
        return ALREADY_OPENED;
    }

    lastLoadedProvider = name;
    eventsProvider->initializeEmptyProvider();
    lastEventProviderGridX = eventsProvider->getDescriptionLength();
    providers.insert(name,eventsProvider);
    providerUrls.insert(name,eventUrl);

    ItemColors* eventColors = new ItemColors();
    providerItemColors.insert(name,eventColors);

    //Install the connections with the provider
    connect(eventsProvider, SIGNAL(newEventDescriptionCreated(QString,QMap<int,int>,QMap<int,int>,QString)),this, SLOT(slotNewEventDescriptionCreated(QString,QMap<int,int>,QMap<int,int>,QString)));
    connect(eventsProvider, SIGNAL(eventDescriptionRemoved(QString,QMap<int,int>,QMap<int,int>,int,QString)),this, SLOT(slotEventDescriptionRemoved(QString,QMap<int,int>,QMap<int,int>,int,QString)));

    //Informs the views than there is a new event provider.
    NeuroscopeView* view;
    Q3ValueList<int> eventsToShow;
    Q3ValueList<int> eventsToSkip;
    for(view = viewList->first(); view!=0; view = viewList->next()){
        if(view != activeView) view->setEventProvider(eventsProvider,name,eventColors,false,eventsToShow,eventsToSkip);
        else view->setEventProvider(eventsProvider,name,eventColors,true,eventsToShow,eventsToSkip);
    }

    return OK;

}

NeuroscopeDoc::OpenSaveCreateReturnMessage NeuroscopeDoc::loadPositionFile(QString url,NeuroscopeView* activeView){
    //get the sampling rate for the given position file extension, if there is none already set, use the default
    QString positionFileName = url.fileName();
    QStringList fileParts = QStringList::split(".", positionFileName);
    if(fileParts.count() < 2) return INCORRECT_FILE;
    positionFileExtension = fileParts[fileParts.count() - 1];

    if(extensionSamplingRates.contains(positionFileExtension)) videoSamplingRate = extensionSamplingRates[positionFileExtension];
    else extensionSamplingRates.insert(positionFileExtension,videoSamplingRate);

    PositionsProvider* positionsProvider = new PositionsProvider(url,videoSamplingRate,videoWidth,videoHeight,rotation,flip);
    QString name = positionsProvider->getName();
    if(providers.find(name) != 0){
        delete positionsProvider;
        return ALREADY_OPENED;
    }

    int returnStatus = positionsProvider->loadData();
    if(returnStatus == PositionsProvider::OPEN_ERROR){
        delete positionsProvider;
        return OPEN_ERROR;
    }

    lastLoadedProvider = name;
    providers.insert(name,positionsProvider);
    providerUrls.insert(name,url);
    positionFileOpenOnce = true;

    if(backgroundImage != "" || (backgroundImage == "" && drawPositionsOnBackground)) transformedBackground = transformBackgroundImage();

    //Informs the views than there is a new position provider.
    NeuroscopeView* view;
    for(view = viewList->first(); view!=0; view = viewList->next())
        if(view == activeView){
            if(rotation != 90 && rotation != 270)
                view->addPositionView(positionsProvider,transformedBackground,dynamic_cast<NeuroscopeApp*>(parent)->getBackgroundColor(),videoWidth,videoHeight);
            //If there is a rotation of 90 or 270 degree, the with and height have to be inverted.
            else view->addPositionView(positionsProvider,transformedBackground,dynamic_cast<NeuroscopeApp*>(parent)->getBackgroundColor(),videoHeight,videoWidth);
        }

    return OK;
}

NeuroscopeDoc::OpenSaveCreateReturnMessage NeuroscopeDoc::loadPositionFile(QString fileUrl){
    //get the sampling rate for the given position file extension, if there is none already set, use the default
    QString positionUrl = QString();
    positionUrl.setPath(fileUrl);
    QString positionFileName = positionUrl.fileName();
    QStringList fileParts = QStringList::split(".", positionFileName);
    if(fileParts.count() < 2) return INCORRECT_FILE;
    positionFileExtension = fileParts[fileParts.count() - 1];

    //check if the file still exist before trying to load it
    QFileInfo fileInfo = QFileInfo(fileUrl);

    if(!fileInfo.exists()){
        QApplication::restoreOverrideCursor();
        KMessageBox::error (0,tr("The file " + fileUrl + " does not exist anymore."), tr("Error!"));
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
        return OPEN_ERROR;
    }

    if(extensionSamplingRates.contains(positionFileExtension)) videoSamplingRate = extensionSamplingRates[positionFileExtension];
    else extensionSamplingRates.insert(positionFileExtension,videoSamplingRate);

    PositionsProvider* positionsProvider = new PositionsProvider(fileUrl,videoSamplingRate,videoWidth,videoHeight,rotation,flip);
    QString name = positionsProvider->getName();

    int returnStatus = positionsProvider->loadData();
    if(returnStatus == PositionsProvider::OPEN_ERROR){
        delete positionsProvider;
        return OPEN_ERROR;
    }

    lastLoadedProvider = name;
    providers.insert(name,positionsProvider);
    providerUrls.insert(name,fileUrl);
    positionFileOpenOnce = true;

    return OK;
}

void NeuroscopeDoc::addPositionView(NeuroscopeView* activeView,QColor backgroundColor){
    Q3DictIterator<DataProvider> it(providers);
    for( ; it.current(); ++it){
        if(it.current()->isA("PositionsProvider")){
            if(rotation != 90 && rotation != 270)
                activeView->addPositionView(static_cast<PositionsProvider*>(it.current()),transformedBackground,
                                            backgroundColor,videoWidth,videoHeight);

            //If there is a rotation of 90 or 270 degree, the with and height have to be inverted.
            else activeView->addPositionView(static_cast<PositionsProvider*>(it.current()),transformedBackground,
                                             backgroundColor,videoHeight,videoWidth);
            break;
        }
    }
}


void NeuroscopeDoc::removePositionFile(NeuroscopeView* activeView){
    QString name;
    Q3DictIterator<DataProvider> it(providers);
    for( ; it.current(); ++it){
        name = it.currentKey();
        if(it.current()->isA("PositionsProvider")) break;
    }

    //Informs the views than the position provider will be removed.
    NeuroscopeView* view;
    for(view = viewList->first(); view!=0; view = viewList->next()){
        if(view != activeView) view->removePositionProvider(name,false);
        else view->removePositionProvider(name,true);
    }

    providers.remove(name);
    providerUrls.remove(name);
}

void NeuroscopeDoc::setDefaultPositionInformation(double videoSamplingRate, int width, int height, QString backgroundImage,
                                                  int rotation,int flip,bool positionsBackground){

    videoSamplingRateDefault = videoSamplingRate;
    videoWidthDefault = width;
    videoHeightDefault = height;
    backgroundImageDefault = backgroundImage;
    rotationDefault = rotation;
    flipDefault = flip;
    drawPositionsOnBackgroundDefault = positionsBackground;

    //Update the current values if no position file is currently opened
    bool exists = false;
    Q3DictIterator<DataProvider> iterator(providers);
    for(;iterator.current();++iterator){
        if(iterator.current()->isA("PositionsProvider")){
            exists = true;
            break;
        }
    }
    if(!exists){
        this->videoSamplingRate = videoSamplingRate;
        this->rotation = rotation;
        this->flip = flip;
        this->backgroundImage = backgroundImage;
        this->videoWidth = videoWidth;
        this->videoHeight = videoHeight;
        drawPositionsOnBackground = positionsBackground;
    }
}

void NeuroscopeDoc::updateSkippedChannelColors(bool whiteBackground,QColor backgroundColor){
    QColor color;
    if(whiteBackground) color = Qt::white;
    else color = backgroundColor;
    skipStatus = displayChannelPalette.getSkipStatus();
    QMap<int,bool>::const_iterator iterator;
}

void NeuroscopeDoc::updateSkipStatus(){
    Q3ValueList<int> skippedChannels;
    skipStatus = displayChannelPalette.getSkipStatus();
    QMap<int,bool>::const_iterator iterator;
    for(iterator = skipStatus.begin(); iterator != skipStatus.end(); ++iterator) if(iterator.data()) skippedChannels.append(iterator.key());

    //Informs the views than the Skip Status has changed.
    NeuroscopeView* view;
    for(view = viewList->first(); view!=0; view = viewList->next()) view->updateSkipStatus(skippedChannels);
}

void NeuroscopeDoc::setDefaultOffsets(NeuroscopeView* activeView){
    channelDefaultOffsets.clear();
    const Q3ValueList<int>& offsets = activeView->getChannelOffset();
    for(int i = 0; i < channelNb; ++i) channelDefaultOffsets.insert(i,offsets[i]);
}

void NeuroscopeDoc::resetDefaultOffsets(){
    channelDefaultOffsets.clear();
    for(int i = 0; i < channelNb; ++i) channelDefaultOffsets.insert(i,0);
}



#include "neuroscopedoc.moc"
