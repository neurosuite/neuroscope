/***************************************************************************
                          parameterxmlcreator.cpp  -  description
                             -------------------
    begin                : Fri Apr 2 2004
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
//application specific include files.
#include "parameterxmlcreator.h"
#include "tags.h"
#include "channelcolors.h"


#include <QTextStream>
#include <QList>

//include files for QT
#include <QFile> 
#include <QString> 

#include "config-neuroscope.h"

using namespace neuroscope;

const QString  ParameterXmlCreator::parameterVersion = "1.0";

ParameterXmlCreator::ParameterXmlCreator():doc(){
    //create the processing instruction
    QDomProcessingInstruction processingInstruction = doc.createProcessingInstruction("xml","version='1.0'");
    doc.appendChild(processingInstruction);

    //Create the root element and its attributes.
    root = doc.createElement(PARAMETERS);
    root.setAttribute(VERSION,parameterVersion);
    root.setAttribute(CREATOR, QString::fromLatin1("neuroscope-%1").arg(NEUROSCOPE_VERSION));
    doc.appendChild(root);
}

ParameterXmlCreator::~ParameterXmlCreator(){}

bool ParameterXmlCreator::writeTofile(const QString& url){ 
    QFile parameterFile(url);
    bool status = parameterFile.open(QIODevice::WriteOnly);
    if(!status)
        return status;

    QDomElement neuroscope = doc.createElement(NEUROSCOPE);
    neuroscope.setAttribute(VERSION,NEUROSCOPE_VERSION);
    neuroscope.appendChild(miscellaneous);
    neuroscope.appendChild(neuroscopeVideo);
    if(!spikes.isNull())
        neuroscope.appendChild(spikes);
    neuroscope.appendChild(channels);

    root.appendChild(acquisitionSystem);
    if(!video.isNull())
        root.appendChild(video);
    root.appendChild(lfp);
    if(!files.isNull())
        root.appendChild(files);
    root.appendChild(anatomicalDescription);
    root.appendChild(spikeDetection);
    root.appendChild(neuroscope);

    QString xmlDocument = doc.toString();

    QTextStream stream(&parameterFile);
    stream<< xmlDocument;
    parameterFile.close();

    return true;
}

void ParameterXmlCreator::setAcquisitionSystemInformation(int resolution,int nbChannels,double samplingRate,int voltageRange,int amplification,int offset){
    acquisitionSystem = doc.createElement(ACQUISITION);

    QDomElement resolutionElement = doc.createElement(BITS);
    QDomText resolutionValue = doc.createTextNode(QString::number(resolution));
    resolutionElement.appendChild(resolutionValue);

    QDomElement nbChannelsElement = doc.createElement(NB_CHANNELS);
    QDomText nbChannelsValue = doc.createTextNode(QString::number(nbChannels));
    nbChannelsElement.appendChild(nbChannelsValue);

    QDomElement samplingRateElement = doc.createElement(SAMPLING_RATE);
    QDomText samplingRateValue = doc.createTextNode(QString::number(samplingRate));
    samplingRateElement.appendChild(samplingRateValue);

    QDomElement voltageRangeElement = doc.createElement(VOLTAGE_RANGE);
    QDomText voltageRangeValue = doc.createTextNode(QString::number(voltageRange));
    voltageRangeElement.appendChild(voltageRangeValue);

    QDomElement amplificationElement = doc.createElement(AMPLIFICATION);
    QDomText amplificationValue = doc.createTextNode(QString::number(amplification));
    amplificationElement.appendChild(amplificationValue);

    QDomElement offsetElement = doc.createElement(OFFSET);
    QDomText offsetValue = doc.createTextNode(QString::number(offset));
    offsetElement.appendChild(offsetValue);

    acquisitionSystem.appendChild(resolutionElement);
    acquisitionSystem.appendChild(nbChannelsElement);
    acquisitionSystem.appendChild(samplingRateElement);
    acquisitionSystem.appendChild(voltageRangeElement);
    acquisitionSystem.appendChild(amplificationElement);
    acquisitionSystem.appendChild(offsetElement);
}

void ParameterXmlCreator::setLfpInformation(double lfpSamplingRate){
    lfp = doc.createElement(FIELD_POTENTIALS);
    QDomElement lfpElement = doc.createElement(LFP_SAMPLING_RATE);
    QDomText lfpValue = doc.createTextNode(QString::number(lfpSamplingRate));
    lfpElement.appendChild(lfpValue);

    lfp.appendChild(lfpElement);
}


void ParameterXmlCreator::setMiscellaneousInformation(float screenGain,const QString& traceBackgroundImage){
    miscellaneous = doc.createElement(MISCELLANEOUS);

    QDomElement gainElement = doc.createElement(SCREENGAIN);
    QDomText gainValue = doc.createTextNode(QString::number(screenGain));
    gainElement.appendChild(gainValue);

    QDomElement imageElement = doc.createElement(TRACE_BACKGROUND_IMAGE);
    QDomText imageValue = doc.createTextNode(traceBackgroundImage);
    imageElement.appendChild(imageValue);

    miscellaneous.appendChild(gainElement);
    miscellaneous.appendChild(imageElement);
}



void ParameterXmlCreator::setChannelDisplayInformation(ChannelColors* channelColors,QMap<int,int>& channelsGroups,QMap<int,int>& channelDefaultOffsets){
    channels = doc.createElement(CHANNELS);

    QMap<int,int>::Iterator iterator;
    for(iterator = channelsGroups.begin(); iterator != channelsGroups.end(); ++iterator){
        //Get the channel information (id, default offset and colors)
        int channelId = iterator.key();
        QColor color = channelColors->color(channelId);
        QColor anatomicalColor = channelColors->groupColor(channelId);
        QColor spikeColor = channelColors->spikeGroupColor(channelId);
        int offset = channelDefaultOffsets[channelId];

        QDomElement idElement = doc.createElement(CHANNEL);
        QDomText idValue = doc.createTextNode(QString::number(channelId));
        idElement.appendChild(idValue);

        QDomElement colorElement = doc.createElement(COLOR);
        QDomText colorValue = doc.createTextNode(color.name());
        colorElement.appendChild(colorValue);

        QDomElement anatomicalColorElement = doc.createElement(ANATOMY_COLOR);
        QDomText anatomicalColorValue = doc.createTextNode(anatomicalColor.name());
        anatomicalColorElement.appendChild(anatomicalColorValue);

        QDomElement spikeColorElement = doc.createElement(SPIKE_COLOR);
        QDomText spikeColorValue = doc.createTextNode(spikeColor.name());
        spikeColorElement.appendChild(spikeColorValue);

        QDomElement channelDisplay = doc.createElement(CHANNEL_COLORS);
        channelDisplay.appendChild(idElement);
        channelDisplay.appendChild(colorElement);
        channelDisplay.appendChild(anatomicalColorElement);
        channelDisplay.appendChild(spikeColorElement);

        QDomElement idElement2 = doc.createElement(CHANNEL);
        QDomText idValue2 = doc.createTextNode(QString::number(channelId));
        idElement2.appendChild(idValue2);

        QDomElement offsetElement = doc.createElement(DEFAULT_OFFSET);
        QDomText offsetValue = doc.createTextNode(QString::number(offset));
        offsetElement.appendChild(offsetValue);

        QDomElement channelOffset = doc.createElement(CHANNEL_OFFSET);
        channelOffset.appendChild(idElement2);
        channelOffset.appendChild(offsetElement);

        channels.appendChild(channelDisplay);
        channels.appendChild(channelOffset);
    }
}

void ParameterXmlCreator::setAnatomicalDescription(QMap<int, QList<int> >& anatomicalGroups,const QMap<int,bool> &skipStatus){
    anatomicalDescription = doc.createElement(ANATOMY);
    QDomElement channelGroupsElement = doc.createElement(CHANNEL_GROUPS);

    //Create the anatomical groups
    QMap<int,QList<int> >::Iterator iterator;
    //The iterator gives the keys sorted.
    for(iterator = anatomicalGroups.begin(); iterator != anatomicalGroups.end(); ++iterator){
        //the trash group is not stored
        if(iterator.key() == 0) continue;
        QList<int> channelIds = iterator.value();
        QList<int>::iterator channelIterator;

        QDomElement groupElement = doc.createElement(GROUP);

        for(channelIterator = channelIds.begin(); channelIterator != channelIds.end(); ++channelIterator){
            QDomElement idElement = doc.createElement(CHANNEL);
            QDomText idValue = doc.createTextNode(QString::number(*channelIterator));
            idElement.appendChild(idValue);
            idElement.setAttribute(SKIP,skipStatus[*channelIterator]);
            groupElement.appendChild(idElement);
        }

        channelGroupsElement.appendChild(groupElement);
    }//end of groups loop

    if(channelGroupsElement.hasChildNodes()) anatomicalDescription.appendChild(channelGroupsElement);
}

void ParameterXmlCreator::setSpikeDetectionInformation(int nbSamples,int peakSampleIndex,QMap<int, QList<int> >& spikeGroups){
    //The spikes element is a neuroscope specific element. The tag contains nbSamples and peakSampleIndex information used for all the spike groups
    //in Neuroscope.
    spikes = doc.createElement(SPIKES);

    QDomElement nbSamplesElement = doc.createElement(NB_SAMPLES);
    QDomText nbSamplesValue = doc.createTextNode(QString::number(nbSamples));
    nbSamplesElement.appendChild(nbSamplesValue);

    QDomElement peakElement = doc.createElement(PEAK_SAMPLE_INDEX);
    QDomText peakValue = doc.createTextNode(QString::number(peakSampleIndex));
    peakElement.appendChild(peakValue);

    spikes.appendChild(nbSamplesElement);
    spikes.appendChild(peakElement);

    /*******Creation of the spike groups******/
    spikeDetection = doc.createElement(SPIKE);
    QDomElement channelGroupsElement = doc.createElement(CHANNEL_GROUPS);

    //Create the spike groups
    QMap<int,QList<int> >::Iterator iterator;
    //The iterator gives the keys sorted.
    for(iterator = spikeGroups.begin(); iterator != spikeGroups.end(); ++iterator){
        //the trashs groups are not stored
        if(iterator.key() == -1 || iterator.key() == 0) continue;
        QList<int> channelIds = iterator.value();
        QList<int>::iterator channelIterator;

        QDomElement groupElement = doc.createElement(GROUP);
        QDomElement channelListElement = doc.createElement(CHANNELS);

        for(channelIterator = channelIds.begin(); channelIterator != channelIds.end(); ++channelIterator){
            QDomElement idElement = doc.createElement(CHANNEL);
            QDomText idValue = doc.createTextNode(QString::number(*channelIterator));
            idElement.appendChild(idValue);
            channelListElement.appendChild(idElement);
        }

        groupElement.appendChild(channelListElement);
        channelGroupsElement.appendChild(groupElement);
    }//end of groups loop


    if(channelGroupsElement.hasChildNodes()) spikeDetection.appendChild(channelGroupsElement);
}


void ParameterXmlCreator::setSpikeDetectionInformation(QMap<int, QList<int> >& spikeGroups){
    spikeDetection = doc.createElement(SPIKE);

    QDomElement channelGroupsElement = doc.createElement(CHANNEL_GROUPS);

    //Create the spike groups
    QMap<int,QList<int> >::Iterator iterator;
    //The iterator gives the keys sorted.
    for(iterator = spikeGroups.begin(); iterator != spikeGroups.end(); ++iterator){
        //the trashs groups are not stored
        if(iterator.key() == -1 || iterator.key() == 0) continue;
        QList<int> channelIds = iterator.value();
        QList<int>::iterator channelIterator;

        QDomElement groupElement = doc.createElement(GROUP);
        QDomElement channelListElement = doc.createElement(CHANNELS);

        for(channelIterator = channelIds.begin(); channelIterator != channelIds.end(); ++channelIterator){
            QDomElement idElement = doc.createElement(CHANNEL);
            QDomText idValue = doc.createTextNode(QString::number(*channelIterator));
            idElement.appendChild(idValue);
            channelListElement.appendChild(idElement);
        }

        groupElement.appendChild(channelListElement);
        channelGroupsElement.appendChild(groupElement);
    }//end of groups loop


    if(channelGroupsElement.hasChildNodes()) spikeDetection.appendChild(channelGroupsElement);
}


void ParameterXmlCreator::setNeuroscopeVideoInformation(int rotation,int flip,const QString& backgroundPath,int drawTrajectory){
    neuroscopeVideo = doc.createElement(VIDEO);

    QDomElement rotationElement = doc.createElement(ROTATE);
    QDomText rotationValue = doc.createTextNode(QString::number(rotation));
    rotationElement.appendChild(rotationValue);

    QDomElement flipElement = doc.createElement(FLIP);
    QDomText flipValue = doc.createTextNode(QString::number(flip));
    flipElement.appendChild(flipValue);

    QDomElement pathElement = doc.createElement(VIDEO_IMAGE);
    QDomText pathValue = doc.createTextNode(backgroundPath);
    pathElement.appendChild(pathValue);

    QDomElement drawTrajectoryElement = doc.createElement(POSITIONS_BACKGROUND);
    QDomText drawTrajectoryValue = doc.createTextNode(QString::number(drawTrajectory));
    drawTrajectoryElement.appendChild(drawTrajectoryValue);

    neuroscopeVideo.appendChild(rotationElement);
    neuroscopeVideo.appendChild(flipElement);
    neuroscopeVideo.appendChild(pathElement);
    neuroscopeVideo.appendChild(drawTrajectoryElement);
}

void ParameterXmlCreator::setVideoInformation(int width,int height){
    //The sampling rate used in NeuroScope is the one contained in the file information section.
    //The sampling rate contained in the video section correspond to the video acquisition system sampling rate and not the
    //sampling rate used to create the position files.
    video = doc.createElement(VIDEO);

    QDomElement widthElement = doc.createElement(WIDTH);
    QDomText widthValue = doc.createTextNode(QString::number(width));
    widthElement.appendChild(widthValue);

    QDomElement heightElement = doc.createElement(HEIGHT);
    QDomText heightValue = doc.createTextNode(QString::number(height));
    heightElement.appendChild(heightValue);

    video.appendChild(widthElement);
    video.appendChild(heightElement);
}

void ParameterXmlCreator::setSampleRateByExtension(const QMap<QString,double>& extensionSamplingRates){
    files = doc.createElement(FILES);

    QMap<QString,double>::ConstIterator iterator;
    for(iterator = extensionSamplingRates.constBegin(); iterator != extensionSamplingRates.constEnd(); ++iterator){
        //Get the extension information (extension and sampling rate)
        QString extension = iterator.key();
        double samplingRate = iterator.value();

        QDomElement extensionElement = doc.createElement(EXTENSION);
        QDomText extensionValue = doc.createTextNode(extension);
        extensionElement.appendChild(extensionValue);

        QDomElement samplingRateElement = doc.createElement(SAMPLING_RATE);
        QDomText samplingRateValue = doc.createTextNode(QString::number(samplingRate));
        samplingRateElement.appendChild(samplingRateValue);

        QDomElement extensionSamplingRate = doc.createElement(neuroscope::FILE);
        extensionSamplingRate.appendChild(extensionElement);
        extensionSamplingRate.appendChild(samplingRateElement);

        files.appendChild(extensionSamplingRate);
    }
}

