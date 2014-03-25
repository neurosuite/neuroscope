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
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

// include files for Qt
#include <QDir>
#include <QWidget>
#include <QPixmap>
#include <QImage>
#include <QRegExp>

#include <QList>
#include <QInputDialog>
#include <QFileDialog>
#include <QApplication>
#include <QMessageBox>

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


extern QString version;

NeuroscopeDoc::NeuroscopeDoc(QWidget* parent, ChannelPalette& displayChannelPalette, ChannelPalette& spikeChannelPalette, int channelNbDefault,
                             double datSamplingRateDefault, double eegSamplingRateDefault, int initialOffset, int voltageRangeDefault,
                             int amplificationDefault, float screenGainDefault, int resolutionDefault, int eventPosition, int clusterPosition,
                             int nbSamples, int peakSampleIndex, double videoSamplingRate, int width, int height, const QString& backgroundImage,
                             const QString &traceBackgroundImage, int rotation, int flip, bool positionsBackground)
    : displayChannelPalette(displayChannelPalette),
      spikeChannelPalette(spikeChannelPalette),
      resolutionDefault(resolutionDefault),
      channelNbDefault(channelNbDefault),
      datSamplingRateDefault(datSamplingRateDefault),
      eegSamplingRateDefault(eegSamplingRateDefault),
      videoSamplingRateDefault(videoSamplingRate),
      initialOffsetDefault(initialOffset),
      screenGainDefault(screenGainDefault),
      voltageRangeDefault(voltageRangeDefault),
      amplificationDefault(amplificationDefault),
      isCommandLineProperties(false),
      channelColorList(0L),
      tracesProvider(0L),
      parent(parent),
      nbSamplesDefault(nbSamples),
      peakSampleIndexDefault(peakSampleIndex),
      eventPosition(eventPosition),
      clusterPosition(clusterPosition),
      newEventDescriptionCreated(false),
      videoWidthDefault(width),
      videoHeightDefault(height),
      backgroundImageDefault(backgroundImage),
      traceBackgroundImageDefault(traceBackgroundImage),
      rotationDefault(rotation),
      flipDefault(flip),
      drawPositionsOnBackgroundDefault(positionsBackground),
      positionFileOpenOnce(false)
{
    viewList = new QList<NeuroscopeView*>();

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
    if(channelColorList){
        delete channelColorList;
        delete tracesProvider;
    }
    qDeleteAll(providers);
    providers.clear();
    qDeleteAll(providerItemColors);
    providerItemColors.clear();

}

bool NeuroscopeDoc::canCloseDocument(NeuroscopeApp* mainWindow, const QString &callingMethod){
    //Before closing, make sure that there is no thread running.
    //Loop on all the cluster and event Providers, moving to the next one when the current one has no more thread running.
    bool threadRunning = false;

    if(!threadRunning){
        QHashIterator<QString, DataProvider*> i(providers);
        while (i.hasNext()) {
            i.next();
            threadRunning = i.value()->isThreadsRunning();
            if(threadRunning)
                break;
        }
    }

    if(threadRunning){
        //Send an event to the klusters (main window) to let it know that the document can not
        //be close because some thread are still running.
        CloseDocumentEvent* event = getCloseDocumentEvent(callingMethod);
        QApplication::postEvent(mainWindow,event);
        return false;
    } else {
        return true;
    }
}

void NeuroscopeDoc::addView(NeuroscopeView* view)
{
    viewList->append(view);
}

void NeuroscopeDoc::removeView(NeuroscopeView* view)
{
    viewList->removeAll(view);
    delete view;
}

const QString& NeuroscopeDoc::url() const
{
    return docUrl;
}

QString NeuroscopeDoc::sessionPath() const
{
    return QFileInfo(sessionUrl).absoluteFilePath();
}

void NeuroscopeDoc::closeDocument()
{
    //If a document has been open reset the members
    viewList->clear();
    docUrl.clear();
    sessionUrl.clear();
    baseName.clear();
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
    transformedBackground  = QImage();
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
    qDeleteAll(providers);
    providers.clear();
    qDeleteAll(providerItemColors);
    providerItemColors.clear();
    providerUrls.clear();
    displayGroupsClusterFile.clear();

    if(channelColorList){
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
    qDebug()<<" int NeuroscopeDoc::openDocument(const QString& url)"<<url;
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
    QFileInfo urlFileInfo(url);
    QString fileName = urlFileInfo.fileName();
    QStringList fileParts = fileName.split(QLatin1Char('.'), QString::SkipEmptyParts);
    if(fileParts.count() < 2)
        return INCORRECT_FILE;
    qDebug()<<"NeuroscopeDoc::openDocument file correct";
    //QString extension;

    //Treat the case when the selected file is a neuroscope session file or a par file.
    if(fileName.contains(QLatin1String(".nrs")) || fileName.contains(QLatin1String(".xml"))){
        if((fileName.contains(".nrs") && fileParts[fileParts.count() - 1] != "nrs") || (fileName.contains(".xml") && fileParts[fileParts.count() - 1] != "xml")) {
            qDebug()<<" NeuroscopeDoc::openDocument INCORRECT FILE";
            return INCORRECT_FILE;
        } else {
            baseName = fileParts.first();
            for(uint i = 1;i < fileParts.count() - 1; ++i)
                baseName += "." + fileParts.at(i);

            //As all the files with the same base name share the same session and par files, ask the user to selected the desire one.
            QString startUrl = urlFileInfo.absolutePath() + QDir::separator() + baseName;
            //QString filter = baseName + ".dat " +  " " + baseName + ".eeg" +  " " +  baseName + ".fil";
            QString filter(tr("Data File (*.dat *.lfp *.eeg *.fil);;All files (*.*)"));
            //filter.append(baseName + ".*");

            const QString openUrl = QFileDialog::getOpenFileName(parent, tr("Open Data File..."),startUrl,filter);
            if(!openUrl.isEmpty()) {
                docUrl = openUrl;
            } else{
                QString docFile = baseName + ".dat";
                docUrl = urlFileInfo.absolutePath() + QDir::separator() + docFile;

                qDebug()<<" NeuroscopeDoc::openDocument dat file";
                if(!QFile::exists(docUrl)) {
                    qDebug()<<" NeuroscopeDoc::openDocument DOWNLOADERROR";
                    return DOWNLOAD_ERROR;
                }
            }
        }
    }

    fileName = QFileInfo(docUrl).fileName();
    fileParts = fileName.split(".", QString::SkipEmptyParts);
    baseName = fileParts[0];
    for(uint i = 1;i < fileParts.count() - 1; ++i)
        baseName += QLatin1Char('.') + fileParts.at(i);
    QString sessionFile = baseName + QLatin1String(".nrs");
    sessionUrl = QFileInfo(docUrl).absolutePath() + QDir::separator() + sessionFile;

    extension = fileParts.at(fileParts.count() - 1);

    //Look up in the parameter file
    const QString parFileUrl = QFileInfo(docUrl).absolutePath() + QDir::separator() +baseName +QLatin1String(".xml");
    parameterUrl = parFileUrl;
    
    QFileInfo parFileInfo = QFileInfo(parFileUrl);
    QFileInfo sessionFileInfo = QFileInfo(sessionUrl);

    if(parFileInfo.exists()){
        if(isCommandLineProperties){
            QApplication::restoreOverrideCursor();
            QMessageBox::information(0, tr("Warning!"),tr("A parameter file has be found, the command line\n"
                                                          "information will be discarded and the parameter file information will be used instead."));
            QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
        }

        NeuroscopeXmlReader reader = NeuroscopeXmlReader();
        if(reader.parseFile(parFileUrl,NeuroscopeXmlReader::PARAMETER)){
            //Load the general info
            loadDocumentInformation(reader);

            //try to get the extension information from the parameter file (prior to the 1.2.3 version, the information was
            //store in the session file)
            extensionSamplingRates = reader.getSampleRateByExtension();
            qDebug()<<" NeuroscopeDoc::openDocument NeuroscopeXmlReader::PARAMETER";
            reader.closeFile();
        }
        else{
            qDebug()<<" NeuroscopeDoc::openDocument PARSE_ERROR";
            return PARSE_ERROR;
        }

        //Is there a session file?
        if(sessionFileInfo.exists()){

            sessionFileExist = true;
            qDebug()<<" sessionUrl"<<sessionUrl;
            if(reader.parseFile(sessionUrl,NeuroscopeXmlReader::SESSION)) {
                //if the session file has been created by a version of NeuroScope prior to the 1.2.3, it contains the extension information
                qDebug()<<" reader.getVersion()"<<reader.getVersion();
                if(reader.getVersion().isEmpty() || reader.getVersion() == QLatin1String("1.2.2"))
                    extensionSamplingRates = reader.getSampleRateByExtension();
                qDebug()<<"extensionSamplingRates"<<extensionSamplingRates;
            } else {
                qDebug()<<" NeuroscopeDoc::openDocument PARSE_ERROR 2";
                return PARSE_ERROR;
            }
        }

        //If the file extension is not a .dat or .eeg look up the sampling rate for
        //the extension. If no sampling rate is available, prompt the user for the information.
        if(extension != "eeg" && extension != "dat" && extension != "xml"){
            if(extensionSamplingRates.contains(extension)) {
                samplingRate = extensionSamplingRates[extension];
                //Prompt the user
            } else {
                QApplication::restoreOverrideCursor();

                QString currentSamplingRate = QInputDialog::getText(0,tr("Sampling Rate"),tr("Type in the sampling rate for the current document"),QLineEdit::Normal,QString::number(datSamplingRate));
                QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
                if(!currentSamplingRate.isEmpty())
                    samplingRate = currentSamplingRate.toDouble();
                else
                    samplingRate = datSamplingRate;//default
                extensionSamplingRates.insert(extension,samplingRate);
            }
        } else {
            if(extension == "eeg")
                samplingRate = eegSamplingRate;
            //Assign the default, dat sampling rate
            else
                samplingRate = datSamplingRate;
        }

        //Create the tracesProvider with the information gather before.
        tracesProvider = new TracesProvider(docUrl,channelNb,resolution,samplingRate,initialOffset);

        //Is there a session file?
        if(sessionFileExist) {
            qDebug()<<" NeuroscopeDoc::openDocument sessionfile exit";
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
                if(reader.getVersion().isEmpty() || reader.getVersion() == "1.2.2"){
                    if(isCommandLineProperties){
                        QApplication::restoreOverrideCursor();
                        QMessageBox::information(0, tr("Warning!"),tr("A session file has been found, the command line "
                                                                      "information will be discarded and the session file information will be used instead."));
                        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
                    }

                    //Load the general info
                    loadDocumentInformation(reader);
                }else {
                    qDebug()<<" NeuroscopeDoc::openDocument MISSING FILE";
                    return MISSING_FILE;
                }

                //Load the extension-sampling rate maping (prior to the 1.2.3 version, the information was store in the session file)
                extensionSamplingRates = reader.getSampleRateByExtension();

                //If the file extension is not a .nrs, .dat or .eeg look up the sampling rate for
                //the extension. If no sampling rate is available, prompt the user for the information.
                if(extension != "eeg" && extension != "dat" && extension != "xml"){
                    if(extensionSamplingRates.contains(extension))
                        samplingRate = extensionSamplingRates[extension];
                    //Prompt the user
                    else{
                        QApplication::restoreOverrideCursor();
                        QString currentSamplingRate = QInputDialog::getText(0,tr("Sampling Rate"),tr("Type in the sampling rate for the current document"),QLineEdit::Normal,QString::number(datSamplingRate));

                        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
                        if(!currentSamplingRate.isEmpty())
                            samplingRate = currentSamplingRate.toDouble();
                        else
                            samplingRate = datSamplingRate;//default
                        extensionSamplingRates.insert(extension,samplingRate);
                    }
                }
                else{
                    if(extension == "eeg")
                        samplingRate = eegSamplingRate;
                    //Assign the default, dat sampling rate
                    else
                        samplingRate = datSamplingRate;
                }

                //Create the tracesProvider with the information gather before.
                tracesProvider = new TracesProvider(docUrl,channelNb,resolution,samplingRate,initialOffset);

                //Load the session information
                loadSession(reader);

                qDebug()<<" NeuroscopeDoc::openDocument CLOSE FILE";
                reader.closeFile();
            }
            else return PARSE_ERROR;
        }
        //No parameter or session file. Use defaults and or command line information (any of the command line arguments have overwritten the default values).
        else{
            bool isaDatFile = true;
            if(extension != "dat")
                isaDatFile = false;
            //Show a dialog to inform the user what are the parameters which will be used.
            static_cast<NeuroscopeApp*>(parent)->displayFileProperties(channelNb,samplingRate,resolution,initialOffset,voltageRange,
                                                                       amplification,screenGain,nbSamples,peakSampleIndex,videoSamplingRate,videoWidth,videoHeight,backgroundImage,
                                                                       rotation,flip,datSamplingRate,isaDatFile,drawPositionsOnBackground,traceBackgroundImage);

            if(extension == "eeg")
                eegSamplingRate = samplingRate;
            //if the extension is not dat, the sampling rate for the dat file while be the default one.
            else if(extension == "dat")
                datSamplingRate = samplingRate;
            else
                extensionSamplingRates.insert(extension,samplingRate);

            //Create the tracesProvider with the information gather before.
            tracesProvider = new TracesProvider(docUrl,channelNb,resolution,samplingRate,initialOffset);

            //No group of channels exist, put all the channels in the same group (1 for the display palette and
            //-1 (the trash group) for the spike palette) and assign them the same blue color.
            //Build the channelColorList and channelDefaultOffsets (default is 0)
            QColor color;
            QList<int> groupOne;
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

    qDebug()<<" NeuroscopeDoc::openDocument END ?";
    //if skipStatus is empty, set the default status to 0
    if(skipStatus.isEmpty()){
        for(int i = 0; i < channelNb; ++i)
            skipStatus.insert(i,false);
    }

    //Use the channel default offsets
    if(!sessionFileExist)
        emit noSession(channelDefaultOffsets,skipStatus);
    qDebug()<<" NeuroscopeDoc::openDocument END FINISH";
    return OK;
}

bool NeuroscopeDoc::saveEventFiles(){
    QMap<QString,QString>::ConstIterator iterator;
    for(iterator = providerUrls.constBegin(); iterator != providerUrls.constEnd(); ++iterator){
        DataProvider* provider = providers[iterator.key()];
        if(qobject_cast<EventsProvider*>(provider)){
            EventsProvider* eventProvider = static_cast<EventsProvider*>(provider);
            if(eventProvider->isModified()){
                QFile eventFile(iterator.value());
                const bool status = eventFile.open(QIODevice::WriteOnly);
                if(!status)
                    return false;
                bool saveStatus = eventProvider->save(&eventFile);
                eventFile.close();
                if(!saveStatus)
                    return false;
            }
        }
    }
    return true;
}

NeuroscopeDoc::OpenSaveCreateReturnMessage NeuroscopeDoc::saveSession(){
    //Save the document information
    QFileInfo parFileInfo = QFileInfo(parameterUrl);
    //If the parameter file exists, modify it
    if(parFileInfo.exists()){
        //Check that the file is writable
        if(!parFileInfo.isWritable()) return NOT_WRITABLE;
        bool status;
        ParameterXmlModifier parameterModifier = ParameterXmlModifier();
        status = parameterModifier.parseFile(parameterUrl);
        if(!status)
            return PARSE_ERROR;
        status = parameterModifier.setAcquisitionSystemInformation(resolution,channelNb,datSamplingRate,voltageRange,amplification,initialOffset);
        if(!status)
            return PARSE_ERROR;
        if(positionFileOpenOnce){
            status = parameterModifier.setVideoInformation(videoWidth,videoHeight);
            if(!status)
                return PARSE_ERROR;
        }
        status = parameterModifier.setLfpInformation(eegSamplingRate);
        if(!status)
            return PARSE_ERROR;
        if(!extensionSamplingRates.empty()){
            status = parameterModifier.setSampleRateByExtension(extensionSamplingRates);
            if(!status)
                return PARSE_ERROR;
        }
        status = parameterModifier.setSpikeDetectionInformation(nbSamples,peakSampleIndex,spikeGroupsChannels);
        if(!status)
            return PARSE_ERROR;
        status = parameterModifier.setAnatomicalDescription(displayGroupsChannels,displayChannelPalette.getSkipStatus());
        if(!status)
            return PARSE_ERROR;

        parameterModifier.setNeuroscopeVideoInformation(rotation,flip,backgroundImage,drawPositionsOnBackground);
        parameterModifier.setMiscellaneousInformation(screenGain,traceBackgroundImage);
        status = parameterModifier.setChannelDisplayInformation(channelColorList,displayChannelsGroups,channelDefaultOffsets);
        if(!status)
            return PARSE_ERROR;

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
    QList<SessionFile> fileList;
    QMap<QString,QString>::Iterator iterator;
    for(iterator = providerUrls.begin(); iterator != providerUrls.end(); ++iterator){
        SessionFile sessionFile;
        sessionFile.setUrl(iterator.value());
        QFileInfo fileInfo = QFileInfo(iterator.value());
        sessionFile.setModification(fileInfo.lastModified());
        DataProvider* provider = providers[iterator.key()];

        if(qobject_cast<ClustersProvider*>(provider)){
            //Type of the file, CLUSTER is the default
            ItemColors* clusterColors = providerItemColors[iterator.key()];
            QList<int> clusterList = static_cast<ClustersProvider*>(provider)->clusterIdList();
            QList<int>::iterator it;
            for(it = clusterList.begin(); it != clusterList.end(); ++it){
                QColor color = clusterColors->color(*it);
                sessionFile.setItemColor(EventDescription(QString::number(*it)),color.name());
            }
        }
        else if(qobject_cast<EventsProvider*>(provider)){
            sessionFile.setType(SessionFile::EVENT);
            ItemColors* eventColors = providerItemColors[iterator.key()];
            QMap<EventDescription,int> eventMap = static_cast<EventsProvider*>(provider)->eventDescriptionIdMap();
            QMap<EventDescription,int>::Iterator it;
            for(it = eventMap.begin(); it != eventMap.end(); ++it){
                QColor color = eventColors->color(it.value());
                sessionFile.setItemColor(it.key(),color.name());
            }
        }
        else if(qobject_cast<PositionsProvider*>(provider)){
            sessionFile.setType(SessionFile::POSITION);
        }
        fileList.append(sessionFile);
    }

    sessionWriter.setLoadedFilesInformation(fileList);

    //Create the list of display information
    QList<DisplayInformation> displayList;

    for(int i = 0; i<viewList->count(); ++i) {
        NeuroscopeView* view = viewList->at(i);
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
        QHashIterator<QString, DataProvider*> it(providers);
        while (it.hasNext()) {
            it.next();
            const QString name = it.key();
            if(qobject_cast<ClustersProvider*>(it.value())){
                QList<int> clusterIds = *(view->getSelectedClusters(name));
                displayInformation.setSelectedClusters(providerUrls[name],clusterIds);
                QList<int> skippedClusterIds = *(view->getClustersNotUsedForBrowsing(name));
                displayInformation.setSkippedClusters(providerUrls[name],skippedClusterIds);
            }
            else if(qobject_cast<EventsProvider*>(it.value())){
                //An id has been assigned to each event, this id is used internally in NeuroScope and in the session file.
                QList<int> eventIds = *(view->getSelectedEvents(name));
                displayInformation.setSelectedEvents(providerUrls[name],eventIds);
                QList<int> skippedEventIds = *(view->getEventsNotUsedForBrowsing(name));
                displayInformation.setSkippedEvents(providerUrls[name],skippedEventIds);
            }
        }
        /***********TO DO**************************/
        //loop on all the loaded spike files and set the spikes show in the current display
        /* QValueList<QString> files;
  displayInformation.setSelectedSpikeFiles(files)*/

        displayInformation.setAutocenterChannels(view->getAutocenterChannels());

		  //loop on all the channels to store their gain and offsets
        QList<TracePosition> tracePositions;

        const QList<int>& offsets = view->getChannelOffset();
        const QList<int>& gains = view->getGains();
        for(int i = 0; i < channelNb; ++i){
            TracePosition tracePosition;
            tracePosition.setId(i);
            tracePosition.setGain(gains.at(i));
            tracePosition.setOffset(offsets.at(i));
            tracePositions.append(tracePosition);
        }

        displayInformation.setPositions(tracePositions);

        //Get the shown channels
        const QList<int>& channelsIds = view->channels();
        displayInformation.setChannelIds(channelsIds);

        //Get the selected channels
        const QList<int>& selectedChannelIds = view->getSelectedChannels();
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
    for(int i = 0; i<viewList->count(); ++i) {
        NeuroscopeView* view = viewList->at(i);
        if(view != activeView)
            view->singleChannelColorUpdate(channelId,false);
        else
            view->singleChannelColorUpdate(channelId,true);
    }

    //Ask the active view to take the modification into account immediately
    activeView->showAllWidgets();
}

void NeuroscopeDoc::channelGroupColorUpdate(int groupId,NeuroscopeView* activeView){
    //Notify all the views of the modification
    for(int i = 0; i<viewList->count(); ++i) {
        NeuroscopeView* view = viewList->at(i);
        if(view != activeView)
            view->channelGroupColorUpdate(groupId,false);
        else
            view->channelGroupColorUpdate(groupId,true);
    }

    //Ask the active view to take the modification into account immediately
    activeView->showAllWidgets();
}

/*void NeuroscopeDoc::channelsColorUpdate(QValueList<int>selectedChannels,NeuroscopeView& view){
qDebug()<<"in NeuroscopeDoc::channelsColorUpdate"<<endl;
}*/

void NeuroscopeDoc::showCalibration(bool show,NeuroscopeView* activeView){
    //Notify all the views of the modification
    for(int i = 0; i<viewList->count(); ++i) {
        NeuroscopeView* view = viewList->at(i);
        if(view != activeView)
            view->showCalibration(show,false);
        else
            view->showCalibration(show,true);
    }

    //Ask the active view to take the modification into account immediately
    activeView->showAllWidgets();
}

void NeuroscopeDoc::groupsModified(NeuroscopeView* activeView){
    if(!providerUrls.isEmpty())
        //compute which cluster files give data for a given anatomical group
        computeClusterFilesMapping();

    //Notify all the views of the modification
    for(int i = 0; i<viewList->count(); ++i) {
        NeuroscopeView* view = viewList->at(i);
        if(view != activeView) view->groupsModified(false);
        else view->groupsModified(true);
    }

    //Ask the active view to take the modification into account immediately
    activeView->showAllWidgets();
}

void NeuroscopeDoc::setBackgroundColor(const QColor& backgroundColor){

    //If a position file is loaded and all the positions are drawn on the background (without using an image as the background)
    //update the background image
    bool isPositionFileLoaded = dynamic_cast<NeuroscopeApp*>(parent)->isApositionFileLoaded();
    if(isPositionFileLoaded && backgroundImage.isEmpty() && drawPositionsOnBackground){
        transformedBackground = transformBackgroundImage();
        NeuroscopeView* view;
        //Get the active view.
        NeuroscopeView* activeView = dynamic_cast<NeuroscopeApp*>(parent)->activeView();
        if(rotation != 90 && rotation != 270){
            for(int i = 0; i<viewList->count(); ++i) {
                view = viewList->at(i);
                if(view != activeView)
                    view->updatePositionInformation(videoWidth,videoHeight,transformedBackground,false,false);
                else
                    view->updatePositionInformation(videoWidth,videoHeight,transformedBackground,false,true);
            }
        }
        //If there is a rotation of 90 or 270 degree, the with and height have to be inverted.
        else{
            for(int i = 0; i<viewList->count(); ++i) {
                view = viewList->at(i);
                if(view != activeView)
                    view->updatePositionInformation(videoHeight,videoWidth,transformedBackground,false,false);
                else
                    view->updatePositionInformation(videoHeight,videoWidth,transformedBackground,false,true);
            }
        }
    }

    //Notify all the views of the modification
    for(int i = 0; i<viewList->count(); ++i) {
        NeuroscopeView* view = viewList->at(i);
        view->updateBackgroundColor(backgroundColor);
    }

    //Get the active view.
    NeuroscopeView* activeView = dynamic_cast<NeuroscopeApp*>(parent)->activeView();

    //Ask the active view to take the modification into account immediately
    activeView->showAllWidgets();
}


void NeuroscopeDoc::setTraceBackgroundImage(const QString& traceBackgroundImagePath){
    traceBackgroundImage = traceBackgroundImagePath;
    if(tracesProvider){
        QImage traceBackgroundImage(traceBackgroundImagePath);

        //The views are updated. If the image is null, the background will be a plain color (no image)
        //Get the active view and make it the first to take the modification into account.
        NeuroscopeView* activeView = dynamic_cast<NeuroscopeApp*>(parent)->activeView();

        //Notify all the views of the modification
        for(int i = 0; i<viewList->count(); ++i) {
            NeuroscopeView* view = viewList->at(i);

            if(view != activeView)
                view->updateTraceBackgroundImage(traceBackgroundImage,false);
            else
                view->updateTraceBackgroundImage(traceBackgroundImage,true);
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
        for(int i = 0; i<viewList->count(); ++i) {
            NeuroscopeView* view = viewList->at(i);
            if(view != activeView) view->documentFeaturesModified();
        }

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
        for(int i = 0; i<viewList->count(); ++i) {
            NeuroscopeView* view = viewList->at(i);

            if(view != activeView) view->setGains(gain,acquisitionGain);
        }

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

        for(int i = 0; i<viewList->count(); ++i) {
            NeuroscopeView* view = viewList->at(i);
            if(view != activeView) view->documentFeaturesModified();
        }

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
    QHashIterator<QString, DataProvider*> i(providers);
    while (i.hasNext()) {
        i.next();
        if(qobject_cast<ClustersProvider*>(i.value())){
            static_cast<ClustersProvider*>(i.value())->updateSamplingRate(samplingRate);
        }
        if(qobject_cast<EventsProvider*>(i.value())){
            static_cast<EventsProvider*>(i.value())->updateSamplingRate(samplingRate);
        }
    }

    if(tracesProvider){
        //Inform the tracesProvider
        tracesProvider->setSamplingRate(rate);
        qlonglong length = tracesProvider->recordingLength();

        //Get the active view and make it the first to take the modification into account.
        NeuroscopeView* activeView = dynamic_cast<NeuroscopeApp*>(parent)->activeView();
        activeView->samplingRateModified(length);

        //Notify all the views of the modification
        for(int i = 0; i<viewList->count(); ++i) {
            NeuroscopeView* view = viewList->at(i);
            if(view != activeView)
                view->samplingRateModified(length);
        }

        //Ask the active view to take the modification into account immediately
        activeView->updateViewContents();
    }
}

void NeuroscopeDoc::setAcquisitionSystemSamplingRate(double rate){
    datSamplingRate = rate;
    //update the cluster providers
    QHashIterator<QString, DataProvider*> i(providers);
    while (i.hasNext()) {
        i.next();

        if(qobject_cast<ClustersProvider*>(i.value())){
            static_cast<ClustersProvider*>(i.value())->updateAcquisitionSystemSamplingRate(datSamplingRate,samplingRate);
        }
    }

    if(tracesProvider){
        //Get the active view and make it the first to take the modification into account.
        NeuroscopeView* activeView = dynamic_cast<NeuroscopeApp*>(parent)->activeView();

        //Notify all the views of the modification
        for(int i = 0; i<viewList->count(); ++i) {
            NeuroscopeView* view = viewList->at(i);
            if(view != activeView)
                view->updateClusterData(false);
            else
                view->updateClusterData(true);
        }
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
        QList<int> groupOne;
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
        for(int i = 0; i<viewList->count(); ++i) {
            NeuroscopeView* view = viewList->at(i);

            if(view != activeView)
                view->setChannelNb(nb);
        }

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
        if(reader.getBackgroundImage() != "-")
            backgroundImage = reader.getBackgroundImage();

        if(!backgroundImage.isEmpty()){
            QFileInfo fileInfo = QFileInfo(backgroundImage);
            if(!fileInfo.exists()){
                QString imageUrl(backgroundImage);
                QString fileName = QFileInfo(imageUrl).fileName();
                imageUrl = docUrl + QDir::separator() + fileName;
                backgroundImage = QFileInfo(imageUrl).absolutePath();
            }
        }
    }

    //The background image information for the trace view is stored in the parameter file starting with the version 1.3.4
    if(reader.getType() == NeuroscopeXmlReader::PARAMETER){

        if(reader.getTraceBackgroundImage() != "-")
            traceBackgroundImage = reader.getTraceBackgroundImage();

        if(!traceBackgroundImage.isEmpty()){
            QFileInfo fileInfo = QFileInfo(traceBackgroundImage);
            if(!fileInfo.exists()){
                QString imageUrl = traceBackgroundImage;
                QString fileName = QFileInfo(imageUrl).fileName();
                imageUrl = docUrl + QDir::separator() + fileName;
                traceBackgroundImage = QFileInfo(imageUrl).absolutePath();
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

    if(displayGroupsChannels.contains(0))
        spikeGroupsChannels.insert(0,displayGroupsChannels[0]);
    reader.getSpikeDescription(channelNb,channelsSpikeGroups,spikeGroupsChannels);

    //compute which cluster files give data for a given anatomical group
    computeClusterFilesMapping();

    //Build the channelColorList
    //the checkColors will be used that their is a color information for each channel.
    QList<int> checkColors;
    for(int i = 0; i < channelNb; ++i) checkColors.append(i);
    QList<ChannelDescription> colorsList = reader.getChannelDescription();
    if(!colorsList.isEmpty()){
        QList<ChannelDescription>::iterator colorIterator;
        for(colorIterator = colorsList.begin(); colorIterator != colorsList.end(); ++colorIterator){
            int channelId = static_cast<ChannelDescription>(*colorIterator).getId();
            int removed = checkColors.removeAll(channelId);
            //it is a duplicate
            if(removed ==0)
                continue;
            QColor color = static_cast<ChannelDescription>(*colorIterator).getColor();
            QColor groupColor = static_cast<ChannelDescription>(*colorIterator).getGroupColor();
            QColor spikeGroupColor = static_cast<ChannelDescription>(*colorIterator).getSpikeGroupColor();
            channelColorList->append(channelId,color,groupColor,spikeGroupColor);
        }
        //if a channel does not have color information, set the default (everything to blue)
        if(!checkColors.isEmpty()){
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
    if(channelDefaultOffsets.isEmpty()){
        for(int i = 0; i < channelNb; ++i)
            channelDefaultOffsets.insert(i,0);
    }
    //if a channel does not have a default offset, assign it the value 0
    if(channelDefaultOffsets.size() != channelNb){
        for(int i = 0; i < channelNb; ++i){
            if(!channelDefaultOffsets.contains(i))
                channelDefaultOffsets.insert(i,0);
        }
    }

    if(reader.getScreenGain() != 0)
        screenGain = reader.getScreenGain();
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
    } else {
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
    QMap<int, QList<int> >::Iterator iterator;
    for(iterator = displayGroupsChannels.begin(); iterator != displayGroupsChannels.end(); ++iterator){
        QList<int> clusterFileList;
        QList<int> anatomicalList = iterator.value();
        QMap<int, QList<int> >::Iterator spikeGroupIterator;
        for(spikeGroupIterator = spikeGroupsChannels.begin(); spikeGroupIterator != spikeGroupsChannels.end(); ++spikeGroupIterator){
            QList<int> channels = spikeGroupIterator.value();
            QList<int>::iterator channelIterator;
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
    if(reader.getRotation() != 0) {
        rotation = reader.getRotation();
    }
    if(reader.getFlip() != 0) {
        flip = reader.getFlip();
    }

    QList<SessionFile> filesToLoad = reader.getFilesToLoad();
    QStringList loadedClusterFiles;
    QStringList loadedEventFiles;
    QString loadedPositionFile;
    QMap< QString, QMap<EventDescription,int> > loadedEventItems;

    //Get the displays information
    QList<DisplayInformation> displayList = reader.getDisplayInformation();

    bool first = true;
    QList<DisplayInformation>::ConstIterator iterator;
    QList<DisplayInformation>::ConstIterator end(displayList.constEnd());
   for(iterator = displayList.constBegin(); iterator != end; ++iterator) {
		  QList<int> offsets;
        QList<int> channelGains;
        QList<int>* channelsToDisplay = new QList<int>();
        QList<int> selectedChannels;
        bool verticalLines = false;
        bool raster = false;
        bool waveforms = false;
        bool multipleColumns = false;

        //Get the information store in DisplayInformation
        DisplayInformation::mode presentationMode = static_cast<DisplayInformation>(*iterator).getMode();
        bool autocenterChannels = static_cast<DisplayInformation>(*iterator).getAutocenterChannels();
        long startTime = static_cast<DisplayInformation>(*iterator).getStartTime();
        long duration = static_cast<DisplayInformation>(*iterator).getTimeWindow();
        bool greyMode = static_cast<DisplayInformation>(*iterator).getGreyScale();
        QList<DisplayInformation::spikeDisplayType> spikeDisplayTypes = static_cast<DisplayInformation>(*iterator).getSpikeDisplayTypes();
        int rasterHeight = static_cast<DisplayInformation>(*iterator).getRasterHeight();
        QMap<QString, QList<int> > selectedClusters = static_cast<DisplayInformation>(*iterator).getSelectedClusters();
        //An id has been assigned to each event, this id will be used internally in NeuroScope and in the session file.
        QMap<QString, QList<int> > selectedEvents = static_cast<DisplayInformation>(*iterator).getSelectedEvents();
        //QStringList shownSpikeFiles = static_cast<DisplayInformation>(*iterator).getSelectedSpikeFiles();
        QMap<QString, QList<int> > skippedClusters = static_cast<DisplayInformation>(*iterator).getSkippedClusters();
        QMap<QString, QList<int> > skippedEvents = static_cast<DisplayInformation>(*iterator).getSkippedEvents();
        QList<TracePosition> positions = static_cast<DisplayInformation>(*iterator).getPositions();
        QList<int> channelIds = static_cast<DisplayInformation>(*iterator).getChannelIds();
        QList<int> selectedChannelIds = static_cast<DisplayInformation>(*iterator).getSelectedChannelIds();
        QString tabLabel = static_cast<DisplayInformation>(*iterator).getTabLabel();
        bool showLabels = static_cast<DisplayInformation>(*iterator).getLabelStatus();
        bool showEventsInPositionView = static_cast<DisplayInformation>(*iterator).isEventsDisplayedInPositionView();

        //info on the trace presentation
        if(presentationMode == DisplayInformation::MULTIPLE)
            multipleColumns = true;

        //info on the spike presentation
        QList<DisplayInformation::spikeDisplayType>::ConstIterator typeIterator;
        QList<DisplayInformation::spikeDisplayType>::ConstIterator typeIteratorEnd(spikeDisplayTypes.end());
        for(typeIterator = spikeDisplayTypes.constBegin(); typeIterator != typeIteratorEnd; ++typeIterator){
            if(*typeIterator == DisplayInformation::LINES)
                verticalLines = true;
            if(*typeIterator == DisplayInformation::RASTER)
                raster = true;
            if(*typeIterator == DisplayInformation::WAVEFORMS)
                waveforms = true;
        }

        //Info regarding the positionView
        bool isAPositionView = static_cast<DisplayInformation>(*iterator).isAPositionView();

        /*****************TO FINISH***************************/

        //Get the information concerning the channel positions (gain and offset)
        QList<TracePosition>::ConstIterator positionIterator;
        QList<TracePosition>::ConstIterator positionIteratorEnd(positions.constEnd());
        for (positionIterator = positions.constBegin(); positionIterator != positionIteratorEnd; ++positionIterator) {
            int gain = static_cast<TracePosition>(*positionIterator).getGain();
            int offset = static_cast<TracePosition>(*positionIterator).getOffset();
            offsets.append(offset);
            channelGains.append(gain);
        }

        //Get the information concerning the channels shown in the display
        QList<int>::ConstIterator channelIterator;
        QList<int>::ConstIterator channelIteratorEnd(channelIds.constEnd());
        for (channelIterator = channelIds.constBegin(); channelIterator != channelIteratorEnd; ++channelIterator) {
            channelsToDisplay->append(*channelIterator);
        }

        //Get the information concerning the channels selected in the display
        QList<int>::ConstIterator channelSelectedIterator;
        QList<int>::ConstIterator channelSelectedIteratorEnd(selectedChannelIds.constEnd());
        for (channelSelectedIterator = selectedChannelIds.constBegin(); channelSelectedIterator != channelSelectedIteratorEnd; ++channelSelectedIterator) {
            selectedChannels.append(*channelSelectedIterator);
        }

        //Create the displays
        if(first){
            first = false;

            emit loadFirstDisplay(channelsToDisplay,verticalLines,raster,waveforms,showLabels,multipleColumns,greyMode,autocenterChannels,offsets,
                                  channelGains,selectedChannels,skipStatus,startTime,duration,tabLabel,isAPositionView,rasterHeight,showEventsInPositionView);

            //Now that the channel palettes are created, load the files and create the palettes
            bool fistClusterFile = true;
            bool fistEventFile = true;
            QList<SessionFile>::iterator sessionIterator;
            for(sessionIterator = filesToLoad.begin(); sessionIterator != filesToLoad.end(); ++sessionIterator){
                SessionFile sessionFile = static_cast<SessionFile>(*sessionIterator);
                QString fileUrl = sessionFile.getUrl().path();
                SessionFile::type fileType = sessionFile.getType();
                QDateTime lastModified = sessionFile.getModification();
                QMap<EventDescription,QColor> itemColors = sessionFile.getItemColors();
                if(fileType == SessionFile::CLUSTER){
                    //If the file does not exist in the location specified in the session file (absolute path), look up in the directory
                    //where the session file is. This is useful if you moved your file or you backup them (<=> the absolute path is not good anymore)
                    QFileInfo fileInfo = QFileInfo(fileUrl).absolutePath();
                    if(!fileInfo.exists()){
                        QList<int> ids = selectedClusters[fileUrl];
                        QList<int> skippedIds = skippedClusters[fileUrl];
                        selectedClusters.remove(fileUrl);
                        skippedClusters.remove(fileUrl);
                        QString fileName = QFileInfo(fileUrl).fileName();
                        fileUrl = QFileInfo(sessionUrl).absolutePath() + QDir::separator() + fileName;
                        selectedClusters.insert(fileUrl,ids);
                        skippedClusters.insert(fileUrl,skippedIds);
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
                    QFileInfo fileInfo = QFileInfo(fileUrl).absolutePath();
                    if(!fileInfo.exists()){
                        QList<int> ids = selectedEvents[fileUrl];
                        QList<int> skippedIds = skippedEvents[fileUrl];
                        selectedEvents.remove(fileUrl);
                        skippedEvents.remove(fileUrl);
                        QString fileName = QFileInfo(fileUrl).fileName();
                        fileUrl = QFileInfo(sessionUrl).absolutePath() + QDir::separator() + fileName;
                        selectedEvents.insert(fileUrl,ids);
                        skippedEvents.insert(fileUrl,skippedIds);
                    }
                    OpenSaveCreateReturnMessage status = loadEventFile(fileUrl,itemColors,lastModified,fistEventFile);
                    if(status == OK){
                        loadedEventFiles.append(lastLoadedProvider);
                        fistEventFile = false;
                        QMap<EventDescription,int> loadedItems;
                        QMap<EventDescription,QColor>::ConstIterator it;
                        QMap<EventDescription,QColor>::ConstIterator endColor(itemColors.constEnd());
                        int index = 1;
                        for(it = itemColors.constBegin(); it != endColor; ++it){
                            loadedItems.insert(it.key(),index);
                            index++;
                        }
                        loadedEventItems.insert(lastLoadedProvider,loadedItems);
                    }
                }
                if(fileType == SessionFile::POSITION){
                    //If the file does not exist in the location specified in the session file (absolute path), look up in the directory
                    //where the session file is. This is useful if you moved your file or you backup them (<=> the absolute path is not good anymore)
                    QFileInfo fileInfo = QFileInfo(fileUrl).absolutePath();
                    if(!fileInfo.exists()){
                        QString fileName = QFileInfo(fileUrl).fileName();
                        fileUrl = QFileInfo(sessionUrl).absolutePath()+QDir::separator() + fileName;
                    }

                    //Create the transformedBackground
                    //The background image information is stored in the parameter file starting with the version 1.2.3
                    if(reader.getVersion().isEmpty() || reader.getVersion() == "1.2.2"){
                        if(reader.getBackgroundImage() != "-")
                            backgroundImage = sessionFile.getBackgroundPath();
                        if(!backgroundImage.isEmpty()){
                            fileInfo = QFileInfo(backgroundImage);
                            if(!fileInfo.exists()){
                                QString imageUrl= backgroundImage;
                                QString fileName = QFileInfo(imageUrl).fileName();
                                imageUrl = sessionUrl + QDir::separator() + fileName;
                                backgroundImage = QFileInfo(imageUrl).absolutePath();
                            }
                        }
                    }

                    OpenSaveCreateReturnMessage status = loadPositionFile(fileUrl);
                    if(status == OK){
                        loadedPositionFile = lastLoadedProvider;
                        if(!backgroundImage.isEmpty() || (backgroundImage.isEmpty() && drawPositionsOnBackground))
                            transformedBackground = transformBackgroundImage();
                        static_cast<NeuroscopeApp*>(parent)->positionFileLoaded();
                    }
                }
            }
        } else {
            static_cast<NeuroscopeApp*>(parent)->createDisplay(channelsToDisplay,verticalLines,raster,waveforms,showLabels,multipleColumns,
                                                                greyMode,autocenterChannels,offsets,channelGains,selectedChannels,startTime,duration,rasterHeight,tabLabel);
        }


        //the new view is the last one in the list of view (viewList)
        NeuroscopeView* view = viewList->last();

        //If the data file is not a dat file, do not display the waveforms but keep the information
        if(extension != "dat")
            view->ignoreWaveformInformation();

        //Inform the view of the available providers
        QStringList::iterator providerIterator;
        //Cluster files
        for(providerIterator = loadedClusterFiles.begin(); providerIterator != loadedClusterFiles.end(); ++providerIterator){
            QString name = *providerIterator;
            QString fileURL = providerUrls[name];
            QList<int> clustersIds;
            QList<int> clustersIdsToSkip;
            QList<int> ids = selectedClusters[fileURL];
            QList<int> skippedIds = skippedClusters[fileURL];
            QList<int> clusterList = static_cast<ClustersProvider*>(providers[name])->clusterIdList();
            //only keep the cluster ids which are still present
            QList<int>::iterator shownClustersIterator;
            for(shownClustersIterator = ids.begin(); shownClustersIterator != ids.end(); ++shownClustersIterator)
                if(clusterList.contains(*shownClustersIterator)) clustersIds.append(*shownClustersIterator);
            QList<int>::iterator skippedClustersIterator;
            for(skippedClustersIterator = skippedIds.begin(); skippedClustersIterator != skippedIds.end(); ++skippedClustersIterator)
                if(clusterList.contains(*skippedClustersIterator)) clustersIdsToSkip.append(*skippedClustersIterator);

            //an unselected cluster has to be skipped, check and correct if need it
            QList<int>::iterator iterator;
            for(iterator = clusterList.begin(); iterator != clusterList.end(); ++iterator)
                if(!clustersIds.contains(*iterator) && !clustersIdsToSkip.contains(*iterator)) clustersIdsToSkip.append(*iterator);
            qSort(clustersIdsToSkip);
            view->setClusterProvider(static_cast<ClustersProvider*>(providers[name]),name,providerItemColors[name],true,clustersIds,
                                     &displayGroupsClusterFile,&channelsSpikeGroups,peakSampleIndex - 1,nbSamples - peakSampleIndex,clustersIdsToSkip);
        }
        //Event files
        for(providerIterator = loadedEventFiles.begin(); providerIterator != loadedEventFiles.end(); ++providerIterator){
            QString name = *providerIterator;
            QString fileURL = providerUrls[name];
            QList<int> eventsIds;
            QList<int> eventsIdsToSkip;
            QList<int> ids = selectedEvents[fileURL];
            QList<int> skippedIds = skippedEvents[fileURL];
            QMap<int,EventDescription> eventMap = static_cast<EventsProvider*>(providers[name])->eventIdDescriptionMap();
            //only keep the event ids which are still present
            QMap<EventDescription,int> loadedItems = loadedEventItems[name];
            ItemColors* eventColors = providerItemColors[name];
            QList<int>::iterator shownEventsIterator;
            for(shownEventsIterator = ids.begin(); shownEventsIterator != ids.end(); ++shownEventsIterator){
                EventDescription description = EventDescription(eventColors->itemLabelById(*shownEventsIterator));
                if(eventMap.contains(*shownEventsIterator) && loadedItems.contains(description) && loadedItems[description] == *shownEventsIterator)
                    eventsIds.append(*shownEventsIterator);
            }
            QList<int>::iterator skippedEventsIterator;
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
        if(!loadedPositionFile.isEmpty()){
            if(isAPositionView){
                if(rotation != 90 && rotation != 270)
                    view->addPositionView(static_cast<PositionsProvider*>(providers[loadedPositionFile]),transformedBackground, dynamic_cast<NeuroscopeApp*>(parent)->getBackgroundColor(),
                                          startTime,duration,videoWidth,videoHeight,showEventsInPositionView);

                //If there is a rotation of 90 or 270 degree, the with and height have to be inverted.
                else
                    view->addPositionView(static_cast<PositionsProvider*>(providers[loadedPositionFile]),transformedBackground, dynamic_cast<NeuroscopeApp*>(parent)->getBackgroundColor(),
                                          startTime,duration,videoHeight,videoWidth,showEventsInPositionView);

            }
        }
    }
}

void NeuroscopeDoc::setProviders(NeuroscopeView* activeView){  
    //the new view is the last one in the list of view (viewList)
    NeuroscopeView* newView = viewList->last();
    QHashIterator<QString, DataProvider*> i(providers);
    while (i.hasNext()) {
        i.next();
        const QString name = i.key();
        if(qobject_cast<ClustersProvider*>(i.value())){
            QList<int> clusterIds = *(activeView->getSelectedClusters(name));
            QList<int> clusterIdsToSkip = *(activeView->getClustersNotUsedForBrowsing(name));
            newView->setClusterProvider(static_cast<ClustersProvider*>(i.value()),name,providerItemColors[name],true
                                        ,clusterIds,&displayGroupsClusterFile,&channelsSpikeGroups,peakSampleIndex - 1,nbSamples - peakSampleIndex,clusterIdsToSkip);
        }
        if(qobject_cast<EventsProvider*>(i.value())){
            QList<int> eventIds = *(activeView->getSelectedEvents(name));
            QList<int> eventIdsToSkip = *(activeView->getEventsNotUsedForBrowsing(name));
            newView->setEventProvider(static_cast<EventsProvider*>(i.value()),name,providerItemColors[name],true,eventIds,eventIdsToSkip);
        }
        if(qobject_cast<PositionsProvider*>(i.value())){
            if(activeView->isPositionView()){
                if(rotation != 90 && rotation != 270)
                    newView->addPositionView(static_cast<PositionsProvider*>(i.value()),transformedBackground, dynamic_cast<NeuroscopeApp*>(parent)->getBackgroundColor(),
                                             videoWidth,videoHeight);

                //If there is a rotation of 90 or 270 degree, the with and height have to be inverted.
                else newView->addPositionView(static_cast<PositionsProvider*>(i.value()),transformedBackground, dynamic_cast<NeuroscopeApp*>(parent)->getBackgroundColor(),
                                              videoHeight,videoWidth);
            }
        }
    }
}

void NeuroscopeDoc::setWaveformInformation(int nb,int index,NeuroscopeView* activeView){
    nbSamples = nb;
    peakSampleIndex = index;

    const int nbView = viewList->count();
    for(int i = 0; i<nbView; ++i) {
        NeuroscopeView* view = viewList->at(i);

        if(view != activeView)
            view->updateWaveformInformation(peakSampleIndex - 1,nbSamples - peakSampleIndex,false);
        else
            view->updateWaveformInformation(peakSampleIndex - 1,nbSamples - peakSampleIndex,true);
    }
}


void NeuroscopeDoc::setPositionInformation(double newVideoSamplingRate, int newWidth, int newHeight, const QString &newBackgroundImage,
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
    QHashIterator<QString, DataProvider*> i(providers);
    while (i.hasNext()) {
        i.next();
        if(qobject_cast<PositionsProvider*>(i.value())){
            static_cast<PositionsProvider*>(i.value())->updateVideoInformation(videoSamplingRate,rotation,flip,videoWidth,videoHeight);
            break;
        }
    }

    //if a position file is open, update the sampling rate for the corresponding extension
    if(extensionSamplingRates.contains(positionFileExtension))
        extensionSamplingRates.insert(positionFileExtension,videoSamplingRate);

    if(!backgroundImage.isEmpty() || (backgroundImage.isEmpty() && drawPositionsOnBackground))
        transformedBackground = transformBackgroundImage();
    else
        transformedBackground = QImage();

    //Update the views
    if(rotation != 90 && rotation != 270){
        for(int i = 0; i<viewList->count(); ++i) {
            NeuroscopeView* view = viewList->at(i);
            if(view != activeView) view->updatePositionInformation(videoWidth,videoHeight,transformedBackground,newOrientation,false);
            else view->updatePositionInformation(videoWidth,videoHeight,transformedBackground,newOrientation,true);
        }
    }
    //If there is a rotation of 90 or 270 degree, the with and height have to be inverted.
    else{
        for(int i = 0; i<viewList->count(); ++i) {
            NeuroscopeView* view = viewList->at(i);

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
        QHashIterator<QString, DataProvider*> i(providers);
        while (i.hasNext()) {
            i.next();
            if(qobject_cast<PositionsProvider*>(i.value())){
                positionsProvider = static_cast<PositionsProvider*>(i.value());
                break;
            }
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

    if(!image.isNull()){
        //apply first the rotation and then the flip
        QImage rotatedImage = image;
        QTransform rot;
        //KDE counts clockwise, to have a counterclock-wise rotation 90 and 270 are inverted
        if(rotation == 90)
            rot.rotate(90);
        else if(rotation == 180)
            rot.rotate(180);
        else if(rotation == 270)
            rot.rotate(270);
        rotatedImage = image.transformed(rot);
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
            flippedImage = rotatedImage.mirrored(horizontal,vertical);
        }
        return flippedImage;
    }

    return image;
}


void NeuroscopeDoc::selectAllChannels(NeuroscopeView& activeView,bool editMode){
    const QList<int> channelsSelected = displayChannelsGroups.keys();

    //The new selection of channels only means for the active view
    if(editMode) {
        activeView.setSelectedChannels(channelsSelected);
    } else {
        activeView.shownChannelsUpdate(channelsSelected);
    }
}

void NeuroscopeDoc::showAllClustersExcept(ItemPalette* clusterPalette,NeuroscopeView* activeView, const QList<int> &clustersToHide){
    QHashIterator<QString, DataProvider*> i(providers);
    while (i.hasNext()) {
        i.next();
        const QString providerName = i.key();
        if(qobject_cast<ClustersProvider*>(i.value())){
            QList<int> clusterList = static_cast<ClustersProvider*>(i.value())->clusterIdList();
            QList<int> clustersToShow;

            if(clustersToHide.isEmpty()){
                //The new selection of clusters only means for the active view
                activeView->shownClustersUpdate(providerName,clusterList);
            }
            else{
                QList<int>::iterator clustersToAdd;
                for(clustersToAdd = clusterList.begin(); clustersToAdd != clusterList.end(); ++clustersToAdd ){
                    if(!clustersToHide.contains(*clustersToAdd))
                        clustersToShow.append(*clustersToAdd);
                }

                const QList<int>* skippedClusterIds = activeView->getClustersNotUsedForBrowsing(providerName);
                clusterPalette->selectItems(providerName,clustersToShow,*skippedClusterIds);
                //The new selection of clusters only means for the active view
                activeView->shownClustersUpdate(providerName,clustersToShow);
            }
        }
        if(clustersToHide.isEmpty()) clusterPalette->selectAllItems();
    }
}

void NeuroscopeDoc::deselectAllClusters(ItemPalette* clusterPalette,NeuroscopeView* activeView){
    QHashIterator<QString, DataProvider*> i(providers);
    while (i.hasNext()) {
        i.next();

        const QString providerName = i.key();
        if(qobject_cast<ClustersProvider*>(i.value())){
            QList<int> clustersToShow;
            //The new selection of clusters only means for the active view
            activeView->shownClustersUpdate(providerName,clustersToShow);
        }
    }
    clusterPalette->deselectAllItems();
}

void NeuroscopeDoc::showAllEvents(ItemPalette* eventPalette,NeuroscopeView* activeView){
    QHashIterator<QString, DataProvider*> i(providers);
    while (i.hasNext()) {
        i.next();
        const QString providerName = i.key();
        if(qobject_cast<EventsProvider*>(i.value())){
            QMap<EventDescription,int> events = static_cast<EventsProvider*>(i.value())->eventDescriptionIdMap();
            QList<int> eventList = events.values();
            //The new selection of events only means for the active view
            activeView->shownEventsUpdate(providerName,eventList);
        }
    }
    eventPalette->selectAllItems();
}

void NeuroscopeDoc::deselectAllEvents(ItemPalette* eventPalette,NeuroscopeView* activeView){
    QHashIterator<QString, DataProvider*> i(providers);
    while (i.hasNext()) {
        i.next();
        QString providerName = i.key();
        if(qobject_cast<EventsProvider*>(i.value())){
            QList<int> eventsToShow;
            //The new selection of events only means for the active view
            activeView->shownEventsUpdate(providerName,eventsToShow);
        }
    }
    eventPalette->deselectAllItems();
}

void NeuroscopeDoc::deselectAllChannels(NeuroscopeView& activeView,bool editMode){
    QList<int> channelsSelected;

    //The new selection of channels only means for the active view
    if(editMode) activeView.setSelectedChannels(channelsSelected);
    else activeView.shownChannelsUpdate(channelsSelected);
}

void NeuroscopeDoc::synchronize(){
    //The information for spikes detection become the same as the one for displaying the field potentials.
    channelsSpikeGroups.clear();
    spikeGroupsChannels.clear();

    QMap<int,int>::Iterator iterator;
    QMap<int,int>::ConstIterator end(displayChannelsGroups.end());
    for(iterator = displayChannelsGroups.begin(); iterator != end; ++iterator){
        //set the color for the spike group to the one for the display group.
        QColor color = channelColorList->groupColor(iterator.key());
        channelColorList->setSpikeGroupColor(iterator.key(),color);
        channelsSpikeGroups.insert(iterator.key(),iterator.value());
    }

    QMap<int, QList<int> >::Iterator iterator2;
    for(iterator2 = displayGroupsChannels.begin(); iterator2 != displayGroupsChannels.end(); ++iterator2){
        spikeGroupsChannels.insert(iterator2.key(),iterator2.value());
    }
}

qlonglong NeuroscopeDoc::recordingLength() const{
    return tracesProvider->recordingLength();
}

void NeuroscopeDoc::setNoneEditMode(NeuroscopeView* activeView){
    //Notify all the views of the modification
    //In none edit mode, the shown channels becom the selected ones.
    for(int i = 0; i<viewList->count(); ++i) {
        NeuroscopeView* view = viewList->at(i);
        if(view != activeView){
            view->setMode(TraceView::ZOOM,false);
            //view->setSelectedChannels(view->channels());
        }
        else{
            view->setMode(TraceView::ZOOM,true);
            view->setSelectedChannels(view->channels());
        }
    }
}


NeuroscopeDoc::OpenSaveCreateReturnMessage NeuroscopeDoc::loadClusterFile(const QString &clusterUrl,NeuroscopeView* activeView){
    //Check that the selected file is a cluster file
    QString fileName = clusterUrl;
    if(fileName.indexOf(".clu") == -1)
        return INCORRECT_FILE;

    ClustersProvider* clustersProvider = new ClustersProvider(clusterUrl,datSamplingRate,samplingRate,tracesProvider->getTotalNbSamples(),clusterPosition);
    QString name = clustersProvider->getName();

    //The name should only contains digits
    if(name.contains(QRegExp("\\D")) != 0){
        delete clustersProvider;
        return INCORRECT_FILE;
    }

    if(providers.contains(name) && qobject_cast<ClustersProvider*>(providers[name])){
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
    QList<int> clustersToSkip;
    //Constructs the clusterColorList and clustersToSkip
    QList<int> clusterList = clustersProvider->clusterIdList();
    QList<int>::iterator it;
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
    QList<int> clustersToShow;
    for(int i = 0; i<viewList->count(); ++i) {
        NeuroscopeView* view = viewList->at(i);
        if(view != activeView) view->setClusterProvider(clustersProvider,name,clusterColors,false,clustersToShow,&displayGroupsClusterFile,&channelsSpikeGroups,peakSampleIndex - 1,nbSamples - peakSampleIndex,clustersToSkip);
        else view->setClusterProvider(clustersProvider,name,clusterColors,true,clustersToShow,&displayGroupsClusterFile,&channelsSpikeGroups,peakSampleIndex - 1,nbSamples - peakSampleIndex,clustersToSkip);
    }

    return OK;
}

NeuroscopeDoc::OpenSaveCreateReturnMessage NeuroscopeDoc::loadClusterFile(const QString &clusterUrl, QMap<EventDescription,QColor>& itemColors, const QDateTime &lastModified, bool firstFile){
    //Check that the selected file is a cluster file (should always be the case as the file has
    //already be loaded once).
    QString fileName = clusterUrl;
    if(fileName.indexOf(".clu") == -1){
        QApplication::restoreOverrideCursor();
        QMessageBox::critical(0, tr("Error!"),tr("The requested cluster file %1 has an incorrect name, it has to be of the form baseName.n.clu or baseName.clu.n, with n a number identifier."
                                                 "Therefore will not be loaded.").arg(clusterUrl));
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
        return INCORRECT_FILE;
    }


    //check if the file still exist before trying to load it
    QFileInfo fileInfo = QFileInfo(clusterUrl);

    if(!fileInfo.exists()){
        QApplication::restoreOverrideCursor();
        QMessageBox::critical (0, tr("Error!"),tr("The file %1 does not exist anymore.").arg(clusterUrl));
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
        QMessageBox::critical(0, tr("Error!"),tr("The requested cluster file %1 has an incorrect name, it has to be of the form baseName.n.clu or baseName.clu.n, with n a number identifier. Therefore will not be loaded.").arg(clusterUrl));
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
        return INCORRECT_FILE;
    }

    int returnStatus = clustersProvider->loadData();
    if(returnStatus == ClustersProvider::OPEN_ERROR){
        delete clustersProvider;
        QApplication::restoreOverrideCursor();
        QMessageBox::critical (0, tr("Error!"),tr("Could not load the file %1").arg(clusterUrl));
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
        return OPEN_ERROR;
    }
    else if(returnStatus == ClustersProvider::MISSING_FILE){
        delete clustersProvider;
        QApplication::restoreOverrideCursor();
        QMessageBox::critical (0, tr("Error!"),tr("There is no time file (.res) corresponding to the requested file %1"
                                                  ", therefore this file will not be loaded.").arg(clusterUrl));
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
        return MISSING_FILE;
    } else if(returnStatus == ClustersProvider::COUNT_ERROR) {
        delete clustersProvider;
        QApplication::restoreOverrideCursor();
        QMessageBox::critical (0, tr("Error!"),tr("The number of spikes of the requested file %1 could not be determined."
                                                  " Therefore this file will not be loaded.").arg(clusterUrl));
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
        return CREATION_ERROR;
    } else if(returnStatus == ClustersProvider::INCORRECT_CONTENT) {
        delete clustersProvider;
        QApplication::restoreOverrideCursor();
        QMessageBox::critical (0, tr("Error!"),tr("The number of spikes read in the requested file %1 or the corresponding time file (.res) does not correspond to number of spikes computed."
                                                  " Therefore this file will not be loaded.").arg(clusterUrl));
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
        return INCORRECT_CONTENT;
    }

    providers.insert(name,clustersProvider);
    providerUrls.insert(name,clusterUrl);
    lastLoadedProvider = name;

    ItemColors* clusterColors = new ItemColors();
    //Build the clusterColorList
    QList<int> clusterList = clustersProvider->clusterIdList();
    QList<int>::iterator it;
    QList<int>::iterator end(clusterList.end());
    for(it = clusterList.begin(); it != end; ++it){
        if(itemColors.contains(QString::number(*it))){
            clusterColors->append(static_cast<int>(*it),itemColors[QString::number(*it)]);
        } else {
            modified = true;
            QColor color;
            if(*it == 1)
                color.setHsv(0,0,220);//Cluster 1 is always gray
            else if(*it == 0)
                color.setHsv(0,255,255);//Cluster 0 is always red
            else
                color.setHsv(static_cast<int>(fmod(static_cast<double>(*it)*7,36))*10,255,255);
            clusterColors->append(static_cast<int>(*it),color);
        }
    }

    providerItemColors.insert(name,clusterColors);

    if(modified == true){
        QApplication::restoreOverrideCursor();
        QMessageBox::information(0, tr("Error!"),tr("The requested file %1 has been modified since the last session,"
                                                    " therefore some session information may be lost.").arg(clusterUrl));
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    }

    if(firstFile) {
        static_cast<NeuroscopeApp*>(parent)->createClusterPalette(name);
    } else {
        static_cast<NeuroscopeApp*>(parent)->addClusterFile(name);
    }
    return OK;
}

void NeuroscopeDoc::removeClusterFile(QString providerName,NeuroscopeView* activeView){  
    //Informs the views than the cluster provider will be removed.
    for(int i = 0; i<viewList->count(); ++i) {
        NeuroscopeView* view = viewList->at(i);
        if(view != activeView)
            view->removeClusterProvider(providerName,false);
        else
            view->removeClusterProvider(providerName,true);
    }

    delete providers.take(providerName);
    delete providerItemColors.take(providerName);
    providerUrls.remove(providerName);
}

void NeuroscopeDoc::clusterColorUpdate(const QString &providerName,int clusterId,NeuroscopeView* activeView, const QColor &color){
    //Notify all the views of the modification
    for(int i = 0; i<viewList->count(); ++i) {
        NeuroscopeView* view = viewList->at(i);
        if(view != activeView)
            view->clusterColorUpdate(color, providerName,clusterId,false);
        else
            view->clusterColorUpdate(color, providerName,clusterId,true);
    }

    //Ask the active view to take the modification into account immediately
    activeView->showAllWidgets();
}

void NeuroscopeDoc::setClusterPosition(int position){
    clusterPosition = position;
    QMap<QString,QString>::Iterator iterator;
    for(iterator = providerUrls.begin(); iterator != providerUrls.end(); ++iterator){
        DataProvider* provider = providers[iterator.key()];
        if(qobject_cast<ClustersProvider*>(provider))
            static_cast<ClustersProvider*>(provider)->setClusterPosition(position);
    }
}

NeuroscopeDoc::OpenSaveCreateReturnMessage NeuroscopeDoc::loadEventFile(const QString &eventUrl,NeuroscopeView*activeView){
    //Check that the selected file is a event file
    QString fileName = eventUrl;
    if(fileName.indexOf(".evt") == -1)
        return INCORRECT_FILE;

    EventsProvider* eventsProvider = new EventsProvider(eventUrl,samplingRate,eventPosition);
    QString name = eventsProvider->getName();

    //The name should contains 3 characters with at least one none digit character.
    if(name.length() != 3 || name.contains(QRegExp("\\d{3}"))){
        delete eventsProvider;
        return INCORRECT_FILE;
    }

    if(providers.contains(name) && qobject_cast<EventsProvider*>(providers[name])){
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
    QList<int> eventsToSkip;
    //Constructs the eventColorList and eventsToSkip
    //An id is assign to each event, this id will be used internally in NeuroScope and in the session file.
    QMap<EventDescription,int> eventMap = eventsProvider->eventDescriptionIdMap();
    QMap<EventDescription,int>::Iterator it;
    for(it = eventMap.begin(); it != eventMap.end(); ++it){
        QColor color;
        color.setHsv(static_cast<int>(fmod(static_cast<double>(it.value())*7,36))*10,255,255);
        eventColors->append(static_cast<int>(it.value()),static_cast<QString>(it.key()),color);
        eventsToSkip.append(static_cast<int>(it.value()));
    }

    providerItemColors.insert(name,eventColors);

    //Install the connections with the provider
    connect(eventsProvider, SIGNAL(newEventDescriptionCreated(QString,QMap<int,int>,QMap<int,int>,QString)),this, SLOT(slotNewEventDescriptionCreated(QString,QMap<int,int>,QMap<int,int>,QString)));
    connect(eventsProvider, SIGNAL(eventDescriptionRemoved(QString,QMap<int,int>,QMap<int,int>,int,QString)),this, SLOT(slotEventDescriptionRemoved(QString,QMap<int,int>,QMap<int,int>,int,QString)));

    //Informs the views than there is a new event provider.
    QList<int> eventsToShow;
    for(int i = 0; i<viewList->count(); ++i) {
        NeuroscopeView* view = viewList->at(i);

        if(view != activeView) view->setEventProvider(eventsProvider,name,eventColors,false,eventsToShow,eventsToSkip);
        else view->setEventProvider(eventsProvider,name,eventColors,true,eventsToShow,eventsToSkip);
    }

    return OK;
}

NeuroscopeDoc::OpenSaveCreateReturnMessage NeuroscopeDoc::loadEventFile(const QString &eventUrl,QMap<EventDescription,QColor>& itemColors, const QDateTime& lastModified,bool firstFile){
    //Check that the selected file is a event file (should always be the case as the file has
    //already be loaded once).
    QString fileName = eventUrl;
    if(fileName.indexOf(".evt") == -1){
        QApplication::restoreOverrideCursor();
        QMessageBox::critical (0, tr("Error!"),tr("The requested event file %1 has an incorrect name, it has to be of the form baseName.id.evt or baseName.evt.id (with id a 3 character identifier). Therefore it will not be loaded.").arg(eventUrl));
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
        return INCORRECT_FILE;
    }

    //check if the file still exist before trying to load it
    QFileInfo fileInfo = QFileInfo(eventUrl);

    if(!fileInfo.exists()){
        QApplication::restoreOverrideCursor();
        QMessageBox::critical (0, tr("Error!"),tr("The file %1 does not exist anymore.").arg(eventUrl));
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
        QMessageBox::critical (0, tr("Error!"),tr("The requested event file %1 has an incorrect name, it has to be of the form baseName.id.evt or baseName.evt.id (with id a 3 character identifier). Therefore it will not be loaded.").arg(eventUrl));
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
        return INCORRECT_FILE;
    }

    int returnStatus = eventsProvider->loadData();

    if(returnStatus == EventsProvider::OPEN_ERROR){
        delete eventsProvider;
        QApplication::restoreOverrideCursor();
        QMessageBox::critical (0, tr("Error!"),tr("Could not load the file %1").arg(eventUrl));
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
        return OPEN_ERROR;
    }
    else if(returnStatus == EventsProvider::COUNT_ERROR){
        delete eventsProvider;
        QApplication::restoreOverrideCursor();
        QMessageBox::critical (0, tr("Error!"),tr("The number of events of the requested file %1 could not be determined. Therefore this file will not be loaded.").arg(eventUrl));
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
        return CREATION_ERROR;
    }
    else if(returnStatus == EventsProvider::INCORRECT_CONTENT){
        delete eventsProvider;
        QApplication::restoreOverrideCursor();
        QMessageBox::critical (0, tr("Error!"),tr("The content of the requested file %1 is incorrect. Therefore this file will not be loaded.").arg(eventUrl));
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
            eventColors->append(static_cast<int>(it.value()),static_cast<QString>(it.key()),itemColors[it.key()]);
        }
        else{
            modified = true;
            QColor color;
            color.setHsv(static_cast<int>(fmod(static_cast<double>(it.value())*7,36))*10,255,255);
            eventColors->append(static_cast<int>(it.value()),static_cast<QString>(it.key()),color);
        }
    }

    providerItemColors.insert(name,eventColors);

    if(modified == true){
        QApplication::restoreOverrideCursor();
        QMessageBox::information(0, tr("Error!"),tr("The requested file %1 has been modified since the last session,"
                                                    " therefore some session information may be lost.").arg(eventUrl));
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    }

    //Install the connections with the provider
    connect(eventsProvider, SIGNAL(newEventDescriptionCreated(QString,QMap<int,int>,QMap<int,int>,QString)),this, SLOT(slotNewEventDescriptionCreated(QString,QMap<int,int>,QMap<int,int>,QString)));
    connect(eventsProvider, SIGNAL(eventDescriptionRemoved(QString,QMap<int,int>,QMap<int,int>,int,QString)),this, SLOT(slotEventDescriptionRemoved(QString,QMap<int,int>,QMap<int,int>,int,QString)));

    if(firstFile) dynamic_cast<NeuroscopeApp*>(parent)->createEventPalette(name);
    else dynamic_cast<NeuroscopeApp*>(parent)->addEventFile(name);

    return OK;
}

void NeuroscopeDoc::removeEventFile(const QString& providerName,NeuroscopeView* activeView,bool lastFile){
    //Informs the views than the event provider will be removed.
    for(int i = 0; i<viewList->count(); ++i) {
        NeuroscopeView* view = viewList->at(i);

        if(view != activeView)
            view->removeEventProvider(providerName,false,lastFile);
        else
            view->removeEventProvider(providerName,true,lastFile);
    }

    delete providers.take(providerName);
    delete providerItemColors.take(providerName);
    providerUrls.remove(providerName);
}

void NeuroscopeDoc::eventColorUpdate(const QColor &color, const QString& providerName,int eventId,NeuroscopeView* activeView){
    //Notify all the views of the modification
    for(int i = 0; i<viewList->count(); ++i) {
        NeuroscopeView* view = viewList->at(i);

        if(view != activeView)
            view->eventColorUpdate(color, providerName,eventId,false);
        else
            view->eventColorUpdate(color, providerName,eventId,true);
    }

    //Ask the active view to take the modification into account immediately
    activeView->showAllWidgets();
}

void NeuroscopeDoc::setEventPosition(int position){
    eventPosition = position;
    QMap<QString,QString>::Iterator iterator;
    for(iterator = providerUrls.begin(); iterator != providerUrls.end(); ++iterator){
        DataProvider* provider = providers[iterator.key()];
        if(qobject_cast<EventsProvider*>(provider)) static_cast<EventsProvider*>(provider)->setEventPosition(position);
    }
}

void NeuroscopeDoc::eventModified(const QString& providerName,int selectedEventId,double time,double newTime,NeuroscopeView* activeView){
    //clear the undo/redo data of all the event providers except providerName
    QHashIterator<QString, DataProvider*> i(providers);
    while (i.hasNext()) {
        i.next();
        QString name = i.key();
        if(qobject_cast<EventsProvider*>(i.value()) && name != providerName){
            static_cast<EventsProvider*>(i.value())->clearUndoRedoData();
        }
    }

    //Informs the views than an event has been modified.
    for(int i = 0; i<viewList->count(); ++i) {
        NeuroscopeView* view = viewList->at(i);

        if(view != activeView)
            view->updateEvents(providerName,selectedEventId,time,newTime,false);
    }

    //Prepare the undo/redo mechanism
    undoRedoProviderName = providerName;
    undoRedoEventId = selectedEventId;
    undoRedoEventTime = time;
    modifiedEventTime = newTime;
}


void NeuroscopeDoc::eventRemoved(const QString& providerName,int selectedEventId,double time,NeuroscopeView* activeView){
    //clear the undo/redo data of all the event providers except providerName
    QHashIterator<QString, DataProvider*> i(providers);
    while (i.hasNext()) {
        i.next();
        QString name = i.key();
        if(qobject_cast<EventsProvider*>(i.value()) && name != providerName){
            static_cast<EventsProvider*>(i.value())->clearUndoRedoData();
        }
    }

    //Informs the views than an event has been removed.
    for(int i = 0; i<viewList->count(); ++i) {
        NeuroscopeView* view = viewList->at(i);
        if(view != activeView) view->updateEventsAfterRemoval(providerName,selectedEventId,time,false);
        else view->updateEventsAfterRemoval(providerName,selectedEventId,time,true);
    }

    //Prepare the undo/redo mechanism
    undoRedoProviderName = providerName;
    undoRedoEventId = selectedEventId;
    undoRedoEventTime = time;
    modifiedEventTime = -1;
}


void NeuroscopeDoc::eventAdded(const QString &providerName,const QString &addEventDescription,double time,NeuroscopeView* activeView){
    int addedEventId = 0;

    //clear the undo/redo data of all the event providers except providerName and lookup for the selectedEventId
    QHashIterator<QString, DataProvider*> i(providers);
    while (i.hasNext()) {
        i.next();
        QString name = i.key();
        if(qobject_cast<EventsProvider*>(i.value()) && name != providerName){
            static_cast<EventsProvider*>(i.value())->clearUndoRedoData();
        }
        else if(qobject_cast<EventsProvider*>(i.value()) && name == providerName){
            QMap<EventDescription,int> eventMap = static_cast<EventsProvider*>(i.value())->eventDescriptionIdMap();
            addedEventId = eventMap[addEventDescription];
        }
    }

    //Informs the views than an event has been added only if the added event in not of a new type.
    //In that case the views have already be informed by a call to updateSelectedEventsIds (via slotNewEventDescriptionCreated)
    if(!newEventDescriptionCreated){
        for(int i = 0; i<viewList->count(); ++i) {
            NeuroscopeView* view = viewList->at(i);

            if(view != activeView)
                view->updateEventsAfterAddition(providerName,addedEventId,time,false);
            else
                view->updateEventsAfterAddition(providerName,addedEventId,time,true);

        }
    }
    else newEventDescriptionCreated = false;


    //Update the event palette
    ItemPalette* eventPalette = static_cast<NeuroscopeApp*>(parent)->getEventPalette();
    const QList<int>* selectedEvents = activeView->getSelectedEvents(providerName);
    const QList<int>* skippedEvents = activeView->getEventsNotUsedForBrowsing(providerName);
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
    if(undoRedoEventTime != -1){
        //Informs the views than an event has been modified.
        for(int i = 0; i<viewList->count(); ++i) {
            NeuroscopeView* view = viewList->at(i);

            if(view != activeView) view->updateEvents(undoRedoProviderName,undoRedoEventId,undoRedoEventTime,modifiedEventTime,false);
            else view->updateEvents(undoRedoProviderName,undoRedoEventId,undoRedoEventTime,modifiedEventTime,true);
        }
    }
    else{
        //Informs the views than an event has been added back (event previously removed) or added (event previously removed).
        for(int i = 0; i<viewList->count(); ++i) {
            NeuroscopeView* view = viewList->at(i);
            if(view != activeView)
                view->updateEvents(undoRedoProviderName,undoRedoEventId,modifiedEventTime,false);
            else
                view->updateEvents(undoRedoProviderName,undoRedoEventId,modifiedEventTime,true);
        }
    }
}

void NeuroscopeDoc::redo(NeuroscopeView* activeView){
    float time = modifiedEventTime;
    modifiedEventTime = undoRedoEventTime;
    undoRedoEventTime = time;

    static_cast<EventsProvider*>(providers[undoRedoProviderName])->redo();
    newEventDescriptionCreated = false;
    if(modifiedEventTime != -1){
        //Informs the views than an event has been modified.
        for(int i = 0; i<viewList->count(); ++i) {
            NeuroscopeView* view = viewList->at(i);
            if(view != activeView) view->updateEvents(undoRedoProviderName,undoRedoEventId,undoRedoEventTime,modifiedEventTime,false);
            else view->updateEvents(undoRedoProviderName,undoRedoEventId,undoRedoEventTime,modifiedEventTime,true);
        }
    }
    else{
        //Informs the views than an event has been added back (event previously removed) or removed (event previously added).
        for(int i = 0; i<viewList->count(); ++i) {
            NeuroscopeView* view = viewList->at(i);
            if(view != activeView) view->updateEvents(undoRedoProviderName,undoRedoEventId,undoRedoEventTime,false);
            else view->updateEvents(undoRedoProviderName,undoRedoEventId,undoRedoEventTime,true);
        }
    }
}

QList<EventDescription> NeuroscopeDoc::eventIds(const QString &providerName){
    QMap<EventDescription,int> eventMap;

    QHashIterator<QString, DataProvider*> i(providers);
    while (i.hasNext()) {
        i.next();

        QString name = i.key();
        if(qobject_cast<EventsProvider*>(i.value()) && name == providerName){
            eventMap = static_cast<EventsProvider*>(i.value())->eventDescriptionIdMap();
            break;
        }
    }
    return eventMap.keys();
}

void NeuroscopeDoc::slotNewEventDescriptionCreated(const QString &providerName,QMap<int,int> oldNewEventIds,QMap<int,int> newOldEventIds, const QString& eventDescriptionAdded){
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
            QColor color = currentEventColors->color(newOldEventIds[static_cast<int>(it.value())]);
            eventColors->append(static_cast<int>(it.value()),static_cast<QString>(it.key()),color);
        }
        else{
            QColor color;
            if(eventDescriptionAdded == removedDescription.first){
                color = QColor(removedDescription.second);
            }
            else color.setHsv(static_cast<int>(fmod(static_cast<double>(it.value())*7,36))*10,255,255);
            eventColors->append(static_cast<int>(it.value()),static_cast<QString>(it.key()),color);
        }
    }

    currentEventColors->removeAll();
    for(it = eventMap.begin(); it != eventMap.end(); ++it){
        QColor color = eventColors->color(static_cast<int>(it.value()));
        currentEventColors->append(static_cast<int>(it.value()),static_cast<QString>(it.key()),color);
    }


    //Update the event palette
    ItemPalette* eventPalette = static_cast<NeuroscopeApp*>(parent)->getEventPalette();
    eventPalette->removeGroup(providerName);
    eventPalette->createItemList(currentEventColors,providerName,eventsProvider->getDescriptionLength());
    eventPalette->selectGroup(providerName);

    //Informs the views than the event ids have changed.
    NeuroscopeView* activeView = dynamic_cast<NeuroscopeApp*>(parent)->activeView();
    for(int i = 0; i<viewList->count(); ++i) {
        NeuroscopeView* view = viewList->at(i);
        if(view != activeView)
            view->updateSelectedEventsIds(providerName,oldNewEventIds,addedEventId,false,true);
        else
            view->updateSelectedEventsIds(providerName,oldNewEventIds,addedEventId,true,true);
    }

    //Get the active view and update the eventPalette with the selected events.
    const QList<int>* selectedEvents = activeView->getSelectedEvents(providerName);
    const QList<int>* skippedEvents = activeView->getEventsNotUsedForBrowsing(providerName);
    eventPalette->selectItems(providerName,*selectedEvents,*skippedEvents);
}

void NeuroscopeDoc::slotEventDescriptionRemoved(const QString &providerName,QMap<int,int> oldNewEventIds,QMap<int,int> newOldEventIds,int eventIdToRemove, const QString &eventDescriptionToRemove){
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
        QColor color = currentEventColors->color(newOldEventIds[static_cast<int>(it.value())]);
        eventColors->append(static_cast<int>(it.value()),static_cast<QString>(it.key()),color);
    }

    currentEventColors->removeAll();
    for(it = eventMap.begin(); it != eventMap.end(); ++it){
        QColor color = eventColors->color(static_cast<int>(it.value()));
        currentEventColors->append(static_cast<int>(it.value()),static_cast<QString>(it.key()),color);
    }


    //Update the event palette
    ItemPalette* eventPalette = static_cast<NeuroscopeApp*>(parent)->getEventPalette();
    eventPalette->removeGroup(providerName);
    eventPalette->createItemList(currentEventColors,providerName,eventsProvider->getDescriptionLength());
    eventPalette->selectGroup(providerName);

    //Informs the views than the event ids have changed.
    NeuroscopeView* activeView = dynamic_cast<NeuroscopeApp*>(parent)->activeView();
    for(int i = 0; i<viewList->count(); ++i) {
        NeuroscopeView* view = viewList->at(i);
        if(view != activeView) view->updateSelectedEventsIds(providerName,oldNewEventIds,eventIdToRemove,false,false);
        else view->updateSelectedEventsIds(providerName,oldNewEventIds,eventIdToRemove,true,false);
    }
    //Get the active view and update the eventPalette with the selected events.
    const QList<int>* selectedEvents = activeView->getSelectedEvents(providerName);
    const QList<int>* skippedEvents = activeView->getEventsNotUsedForBrowsing(providerName);
    eventPalette->selectItems(providerName,*selectedEvents,*skippedEvents);
}

NeuroscopeDoc::OpenSaveCreateReturnMessage NeuroscopeDoc::createEventFile(const QString &eventUrl,NeuroscopeView*activeView){
    //Check that the selected file is a event file name
    const QString fileName = eventUrl;
    if(fileName.indexOf(QLatin1String(".evt")) == -1)
        return INCORRECT_FILE;

    EventsProvider* eventsProvider = new EventsProvider(eventUrl,samplingRate,eventPosition);
    const QString name = eventsProvider->getName();

    //The name should be of 3 characters length with at least one none digit character.
    if(name.length() != 3 || name.contains(QRegExp("\\d{3}"))){
        delete eventsProvider;
        return INCORRECT_FILE;
    }

    if(providers.contains(name)){
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
    QList<int> eventsToShow;
    QList<int> eventsToSkip;
    for(int i = 0; i<viewList->count(); ++i) {
        NeuroscopeView* view = viewList->at(i);

        if(view != activeView)
            view->setEventProvider(eventsProvider,name,eventColors,false,eventsToShow,eventsToSkip);
        else
            view->setEventProvider(eventsProvider,name,eventColors,true,eventsToShow,eventsToSkip);
    }

    return OK;

}

NeuroscopeDoc::OpenSaveCreateReturnMessage NeuroscopeDoc::loadPositionFile(const QString &url, NeuroscopeView* activeView){
    //get the sampling rate for the given position file extension, if there is none already set, use the default
    QString positionFileName = url;
    QStringList fileParts = positionFileName.split(".", QString::SkipEmptyParts);
    if(fileParts.count() < 2) return INCORRECT_FILE;
    positionFileExtension = fileParts[fileParts.count() - 1];

    if(extensionSamplingRates.contains(positionFileExtension)) videoSamplingRate = extensionSamplingRates[positionFileExtension];
    else extensionSamplingRates.insert(positionFileExtension,videoSamplingRate);

    PositionsProvider* positionsProvider = new PositionsProvider(url,videoSamplingRate,videoWidth,videoHeight,rotation,flip);
    QString name = positionsProvider->getName();
    if(providers.contains(name)){
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

    if(!backgroundImage.isEmpty() || (backgroundImage.isEmpty() && drawPositionsOnBackground))
        transformedBackground = transformBackgroundImage();

    //Informs the views than there is a new position provider.
    for(int i = 0; i<viewList->count(); ++i) {
        NeuroscopeView* view = viewList->at(i);
        if(view == activeView){
            if(rotation != 90 && rotation != 270)
                view->addPositionView(positionsProvider,transformedBackground,dynamic_cast<NeuroscopeApp*>(parent)->getBackgroundColor(),videoWidth,videoHeight);
            //If there is a rotation of 90 or 270 degree, the with and height have to be inverted.
            else
                view->addPositionView(positionsProvider,transformedBackground,dynamic_cast<NeuroscopeApp*>(parent)->getBackgroundColor(),videoHeight,videoWidth);
        }
    }
    return OK;
}

NeuroscopeDoc::OpenSaveCreateReturnMessage NeuroscopeDoc::loadPositionFile(const QString& fileUrl) {
    //get the sampling rate for the given position file extension, if there is none already set, use the default
    QString positionUrl = fileUrl;
    QString positionFileName = positionUrl;
    QStringList fileParts = positionFileName.split(QLatin1String("."), QString::SkipEmptyParts);
    if(fileParts.count() < 2)
        return INCORRECT_FILE;
    positionFileExtension = fileParts[fileParts.count() - 1];

    //check if the file still exist before trying to load it
    QFileInfo fileInfo = QFileInfo(fileUrl);

    if(!fileInfo.exists()){
        QApplication::restoreOverrideCursor();
        QMessageBox::critical (0, tr("Error!"),tr("The file %1 does not exist anymore.").arg(fileUrl));
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

void NeuroscopeDoc::addPositionView(NeuroscopeView* activeView,const QColor& backgroundColor){
    QHashIterator<QString, DataProvider*> i(providers);
    while (i.hasNext()) {
        i.next();
        if(qobject_cast<PositionsProvider*>(i.value())){
            if(rotation != 90 && rotation != 270)
                activeView->addPositionView(static_cast<PositionsProvider*>(i.value()),transformedBackground,
                                            backgroundColor,videoWidth,videoHeight);

            //If there is a rotation of 90 or 270 degree, the with and height have to be inverted.
            else activeView->addPositionView(static_cast<PositionsProvider*>(i.value()),transformedBackground,
                                             backgroundColor,videoHeight,videoWidth);
            break;
        }
    }
}


void NeuroscopeDoc::removePositionFile(NeuroscopeView* activeView){
    QString name;
    QHashIterator<QString, DataProvider*> i(providers);
    while (i.hasNext()) {
        i.next();
        name = i.key();
        if(qobject_cast<PositionsProvider*>(i.value()))
            break;
    }

    //Informs the views than the position provider will be removed.
    for(int i = 0; i<viewList->count(); ++i) {
        NeuroscopeView* view = viewList->at(i);
        if(view != activeView) view->removePositionProvider(name,false);
        else view->removePositionProvider(name,true);
    }

    delete providers.take(name);
    providerUrls.remove(name);
}

void NeuroscopeDoc::setDefaultPositionInformation(double videoSamplingRate, int width, int height, const QString &backgroundImage,
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
    QHashIterator<QString, DataProvider*> i(providers);
    while (i.hasNext()) {
        i.next();
        if(qobject_cast<PositionsProvider*>(i.value())){
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

void NeuroscopeDoc::updateSkippedChannelColors(bool whiteBackground, const QColor &backgroundColor){
    QColor color;
    if(whiteBackground) color = Qt::white;
    else color = backgroundColor;
    skipStatus = displayChannelPalette.getSkipStatus();
}

void NeuroscopeDoc::updateSkipStatus(){
    QList<int> skippedChannels;
    skipStatus = displayChannelPalette.getSkipStatus();
    QMap<int,bool>::const_iterator iterator;
    for(iterator = skipStatus.begin(); iterator != skipStatus.end(); ++iterator) {
        if(iterator.value()) {
            skippedChannels.append(iterator.key());
        }
    }

    //Informs the views than the Skip Status has changed.
    for(int i = 0; i<viewList->count(); ++i) {
        NeuroscopeView* view = viewList->at(i);
        view->updateSkipStatus(skippedChannels);
    }
}

void NeuroscopeDoc::setDefaultOffsets(NeuroscopeView* activeView){
    channelDefaultOffsets.clear();
    const QList<int>& offsets = activeView->getChannelOffset();
    for(int i = 0; i < channelNb; ++i)
        channelDefaultOffsets.insert(i,offsets[i]);
}

void NeuroscopeDoc::resetDefaultOffsets(){
    channelDefaultOffsets.clear();
    for(int i = 0; i < channelNb; ++i)
        channelDefaultOffsets.insert(i,0);
}

bool NeuroscopeDoc::isCurrentFileAdatFile() const{
    if(extension == "dat")
        return true;
    else
        return false;
}

QImage NeuroscopeDoc::getWhiteTrajectoryBackground() {
    if(!drawPositionsOnBackground)
        return QImage();
    return transformBackgroundImage(true);
}
