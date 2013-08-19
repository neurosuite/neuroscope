/***************************************************************************
                          parameterxmlmodifier.cpp  -  description
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
#include "config-neuroscope.h"
#include "parameterxmlmodifier.h"
#include "tags.h"
#include "channelcolors.h"


#include <QTextStream>
#include <QList>

//include files for QT
#include <QFile> 
#include <QString> 

using namespace neuroscope;

ParameterXmlModifier::ParameterXmlModifier(){}
ParameterXmlModifier::~ParameterXmlModifier(){}

bool ParameterXmlModifier::parseFile(const QString& url){

    QFile file(url);
    if(!file.open(QIODevice::ReadWrite))
        return false;
    //actually load the file in a tree in  memory
    if(!doc.setContent(&file)){
        file.close();
        return false;
    }

    file.close();

    //Find the root element
    root = doc.firstChild();
    if(root.isNull())
        return false;
    //if the first element is an Processing Instruction takes the sibiling child as the root.
    if(root.isProcessingInstruction())
        root = root.nextSibling();
    if(root.isNull())
        return false;

    //Find the neuroscope node
    neuroscope = findDirectChild(NEUROSCOPE);

    newVideoNode = false;
    newFilesNode = false;

    //Keep a copy of the original document
    initialXmlDocument = doc.toString();

    return true;
}

bool ParameterXmlModifier::writeTofile(const QString& url){ 
    QFile sessionFile(url);
    bool status = sessionFile.open(QIODevice::WriteOnly);
    if(!status) return status;

    //insert a video node if one has been created
    QDomNode newChild;
    if(newVideoNode){
        newChild = root.insertAfter(video,acquisitionSystem);
        if(newChild.isNull()){
            QTextStream stream(&sessionFile);
            stream<< initialXmlDocument;
            sessionFile.close();
            return false;
        }
    }

    //insert a files node if one has been created
    if(newFilesNode){
        newChild = root.insertAfter(files,lfp);
        if(newChild.isNull()){
            QTextStream stream(&sessionFile);
            stream<< initialXmlDocument;
            sessionFile.close();
            return false;
        }
    }

    //Creates the neuroscope tag
    QDomElement neuroscopeElement = doc.createElement(NEUROSCOPE);
    neuroscopeElement.setAttribute(VERSION,NEUROSCOPE_VERSION);
    neuroscopeElement.appendChild(miscellaneous);
    neuroscopeElement.appendChild(neuroscopeVideo);

    if(!spikes.isNull())neuroscopeElement.appendChild(spikes);
    neuroscopeElement.appendChild(channels);

    if(!neuroscope.isNull()) newChild = root.replaceChild(neuroscopeElement,neuroscope);
    else newChild = root.insertAfter(neuroscopeElement,spikeDetection);
    if (newChild.isNull()){
        QTextStream stream(&sessionFile);
        stream<< initialXmlDocument;
        sessionFile.close();
        return false;
    }

    QString xmlDocument = doc.toString();

    QTextStream stream(&sessionFile);
    stream<< xmlDocument;
    sessionFile.close();

    return true;
}

QDomNode ParameterXmlModifier::findDirectChild(const QString &childName){
    QDomNode child = root.firstChild();
    while(!child.isNull()){
        // the node really is an element and has the right tag.
        if(child.isElement() && child.nodeName() == childName) return child;
        child = child.nextSibling();
    }

    //No node has been found, return an empty node
    return QDomNode();
}

QDomNode ParameterXmlModifier::findDirectChild(const QString& childName,const QDomNode& ancestor){
    if(ancestor.isNull()) return QDomNode();
    QDomNode child = ancestor.firstChild();
    while(!child.isNull()){
        // the node really is an element and has the right tag.
        if(child.isElement() && child.nodeName() == childName)  return child;
        child = child.nextSibling();
    }

    //No node has been found, return an empty node
    return QDomNode();
}

QDomNode ParameterXmlModifier::findDirectChild(const QString& childName,const QString &grandChildName,const QString& value,const QDomNode &ancestor){
    if(ancestor.isNull())
        return QDomNode();
    QDomNode child = ancestor.firstChild();
    while(!child.isNull()){
        // the node really is an element and has the right tag.
        if(child.isElement() && child.nodeName() == childName){
            QDomNodeList list = child.childNodes();
            for(uint i=0;i<list.count();++i){
                QDomNode grandChild = list.item(i);
                if(grandChild.isElement() && grandChild.nodeName() == grandChildName){
                    QDomNode textNode = grandChild.firstChild();
                    if(textNode.isText() && textNode.nodeValue() == value) return child;
                    else break;
                }
            }
        }
        child = child.nextSibling();
    }

    //No node has been found, return an empty node
    return QDomNode();
}


bool ParameterXmlModifier::setAcquisitionSystemInformation(int resolution,int nbChannels,double samplingRate,int voltageRange,int amplification,int offset){
    acquisitionSystem = findDirectChild(ACQUISITION);
    if(acquisitionSystem.isNull()) return false;

    QDomNode resolutionNode = findDirectChild(BITS,acquisitionSystem);
    if(!resolutionNode.isNull()){
        QDomText resolutionTextChild = resolutionNode.firstChild().toText();
        if(!resolutionTextChild.isNull()) resolutionTextChild.setNodeValue(QString::number(resolution));
        else return false;
    }
    else return false;

    QDomNode nbChannelsNode = findDirectChild(NB_CHANNELS,acquisitionSystem);
    if(!nbChannelsNode.isNull()){
        QDomText nbChannelsTextChild = nbChannelsNode.firstChild().toText();
        if(!nbChannelsTextChild.isNull()) nbChannelsTextChild.setNodeValue(QString::number(nbChannels));
        else return false;
    }
    else return false;

    QDomNode samplingRateNode = findDirectChild(SAMPLING_RATE,acquisitionSystem);
    if(!samplingRateNode.isNull()){
        QDomText samplingRateTextChild = samplingRateNode.firstChild().toText();
        if(!samplingRateTextChild.isNull()) samplingRateTextChild.setNodeValue(QString::fromLatin1("%1").arg(samplingRate,0,'g',14));
        else return false;
    }
    else return false;

    QDomNode voltageRangeNode = findDirectChild(VOLTAGE_RANGE,acquisitionSystem);
    if(!voltageRangeNode.isNull()){
        QDomText voltageRangeTextChild = voltageRangeNode.firstChild().toText();
        if(!voltageRangeTextChild.isNull()) voltageRangeTextChild.setNodeValue(QString::number(voltageRange));
        else return false;
    }
    else return false;

    QDomNode amplificationNode = findDirectChild(AMPLIFICATION,acquisitionSystem);
    if(!amplificationNode.isNull()){
        QDomText amplificationTextChild = amplificationNode.firstChild().toText();
        if(!amplificationTextChild.isNull()) amplificationTextChild.setNodeValue(QString::number(amplification));
        else return false;
    }
    else return false;

    QDomNode offsetNode = findDirectChild(OFFSET,acquisitionSystem);
    if(!offsetNode.isNull()){
        QDomText offsetTextChild = offsetNode.firstChild().toText();
        if(!offsetTextChild.isNull()) offsetTextChild.setNodeValue(QString::number(offset));
        else return false;
    }
    else return false;

    return true;
}

bool ParameterXmlModifier::setLfpInformation(double lfpSamplingRate){
    lfp = findDirectChild(FIELD_POTENTIALS);
    if(lfp.isNull())
        return false;

    QDomNode lfpSamplingRateNode = findDirectChild(LFP_SAMPLING_RATE,lfp);
    if(!lfpSamplingRateNode.isNull()){
        QDomText lfpSamplingRateTextChild = lfpSamplingRateNode.firstChild().toText();
        if(!lfpSamplingRateTextChild.isNull())
            lfpSamplingRateTextChild.setNodeValue(QString::fromLatin1("%1").arg(lfpSamplingRate,0,'g',14));
        else
            return false;
    }
    else
        return false;

    return true;
}


void ParameterXmlModifier::setMiscellaneousInformation(float screenGain,const QString& traceBackgroungImage){
    //AS part of the NEUROSCOPE tag, this tag is overwritten: the current MISCELLANEOUS tag will be replace by this new one
    miscellaneous = doc.createElement(MISCELLANEOUS);
    QDomElement gainElement = doc.createElement(SCREENGAIN);
    QDomText gainValue = doc.createTextNode(QString::number(screenGain));
    gainElement.appendChild(gainValue);

    QDomElement imageElement = doc.createElement(TRACE_BACKGROUND_IMAGE);
    QDomText imageValue = doc.createTextNode(traceBackgroungImage);
    imageElement.appendChild(imageValue);

    miscellaneous.appendChild(gainElement);
    miscellaneous.appendChild(imageElement);
}

void ParameterXmlModifier::setNeuroscopeVideoInformation(int rotation,int flip,const QString& backgroundPath,int drawTrajectory){

    //AS part of the NEUROSCOPE tag, this tag is overwritten: the current NEUROSCOPE/VIDEO tag will be replace by this new one
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

bool ParameterXmlModifier::setVideoInformation(int width,int height){
    video = findDirectChild(VIDEO);
    //If the video element does not exist, create it
    if(video.isNull()){
        newVideoNode = true;
        video = doc.createElement(VIDEO);

        QDomElement widthElement = doc.createElement(WIDTH);
        QDomText widthValue = doc.createTextNode(QString::number(width));
        widthElement.appendChild(widthValue);

        QDomElement heightElement = doc.createElement(HEIGHT);
        QDomText heightValue = doc.createTextNode(QString::number(height));
        heightElement.appendChild(heightValue);

        //  video.appendChild(samplingRateElement);
        video.appendChild(widthElement);
        video.appendChild(heightElement);
    }
    //Modify the existing video element
    else{
        QDomNode widthNode = findDirectChild(WIDTH,video);
        if(!widthNode.isNull()){
            QDomText widthTextChild = widthNode.firstChild().toText();
            if(!widthTextChild.isNull()) widthTextChild.setNodeValue(QString::number(width));
            else return false;
        }
        else return false;

        QDomNode heightNode = findDirectChild(HEIGHT,video);
        if(!heightNode.isNull()){
            QDomText heightTextChild = heightNode.firstChild().toText();
            if(!heightTextChild.isNull()) heightTextChild.setNodeValue(QString::number(height));
            else return false;
        }
        else return false;
    }

    return true;
}


bool ParameterXmlModifier::setChannelDisplayInformation(ChannelColors* channelColors,QMap<int,int>& channelsGroups,QMap<int,int>& channelDefaultOffsets){
    //AS part of the NEUROSCOPE tag, this tag is overwritten: the current CHANNELS tag will be replace by this new one
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
    return true;
}

bool ParameterXmlModifier::setAnatomicalDescription(QMap<int, QList<int> >& anatomicalGroups,QMap<int,bool> skipStatus){
    anatomicalDescription = findDirectChild(ANATOMY);
    if(anatomicalDescription.isNull())
        return false;
    QDomNode channelGroupsNode = findDirectChild(CHANNEL_GROUPS,anatomicalDescription);

    QDomElement channelGroupsElement = doc.createElement(CHANNEL_GROUPS);

    //Create the anatomical groups
    QMap<int,QList<int> >::Iterator iterator;
    //The iterator gives the keys sorted.
    for(iterator = anatomicalGroups.begin(); iterator != anatomicalGroups.end(); ++iterator){
        //the trash groups are not stored
        if(iterator.key() == 0)
            continue;
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

    //This erase all previous information contained in the CHANNEL_GROUPS tag including information which have been set by external applications.
    if(channelGroupsElement.hasChildNodes()){
        if(!channelGroupsNode.isNull()){
            QDomNode newChild = anatomicalDescription.replaceChild(channelGroupsElement,channelGroupsNode);
            if(newChild.isNull())
                return false;
            else
                return true;
        } else {
            anatomicalDescription.appendChild(channelGroupsElement);
        }
        return true;
    }
    else{
        if(!channelGroupsNode.isNull()) anatomicalDescription.removeChild(channelGroupsNode);
        return true;
    }
}

bool ParameterXmlModifier::setSpikeDetectionInformation(int nbSamples,int peakSampleIndex,QMap<int, QList<int> >& spikeGroups){
    //AS part of the NEUROSCOPE tag, this tag is overwritten: the current SPIKES tag will be replace by this new one
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
    return setSpikeDetectionInformation(spikeGroups);
}


bool ParameterXmlModifier::setSpikeDetectionInformation(QMap<int, QList<int> >& spikeGroups){
    spikeDetection = findDirectChild(SPIKE);
    if(spikeDetection.isNull()) return false;

    QDomNode channelGroupsNode = findDirectChild(CHANNEL_GROUPS,spikeDetection);
    QDomElement channelGroupsElement = doc.createElement(CHANNEL_GROUPS);

    //The trash groups are not store in the parameter file, they should not be counted when comparing the number of groups between the parameter
    //file and the current groups.
    int nbSpikegroups = spikeGroups.count();
    if(spikeGroups.contains(0)) nbSpikegroups--;
    if(spikeGroups.contains(-1)) nbSpikegroups--;

    //if the number of groups in the parameter file and currently defined are the same, check if the groups are identical
    //if so do not do anything otherwise go to the next section and erase all previous information contained in the CHANNEL_GROUPS tag including
    //information which have been set by external applications.
    QDomNode groupNode;
    if(!channelGroupsNode.isNull() && channelGroupsNode.childNodes().count() == nbSpikegroups){
        bool identical = true;
        groupNode = findDirectChild(GROUP,channelGroupsNode);
        QMap<int,QList<int> >::Iterator iterator;
        //The iterator gives the keys sorted.
        for(iterator = spikeGroups.begin(); iterator != spikeGroups.end(); ++iterator){
            //the trashs groups are not stored
            if(iterator.key() == -1 || iterator.key() == 0) continue;
            QList<int> channelIds = iterator.value();
            int nbChannels = channelIds.size();
            QDomNode channelList = findDirectChild(CHANNELS,groupNode);

            //If the 2 groups do not have the same number of channels stop here.
            if(static_cast<int>(channelList.childNodes().count()) != nbChannels){
                identical = false;
                break;
            }
            else{
                QDomNode channelNode = channelList.firstChild();
                for(int i = 0; i < nbChannels; ++i){
                    QDomText channelTextChild = channelNode.firstChild().toText();

                    if(channelIds[i] != channelTextChild.nodeValue().toInt()){
                        identical = false;
                        break;
                    }
                    channelNode = channelNode.nextSibling();
                }
                if(!identical) break;
            }
            groupNode = groupNode.nextSibling();
        }

        //if the groups are identical, return otherwise go to the next step and erase the current CHANNEL_GROUPS tag by a new one.
        if(identical) return true;
    }


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

    //This erase all previous information contained in the CHANNEL_GROUPS tag including information which have been set by external applications.
    if(channelGroupsElement.hasChildNodes()){
        if(!channelGroupsNode.isNull()){
            QDomNode newChild = spikeDetection.replaceChild(channelGroupsElement,channelGroupsNode);
            if(newChild.isNull()) return false;
            else return true;
        }
        else spikeDetection.appendChild(channelGroupsElement);
        return true;
    }
    else{
        if(!channelGroupsNode.isNull()) spikeDetection.removeChild(channelGroupsNode);
        return true;
    }
}



bool ParameterXmlModifier::setSampleRateByExtension(const QMap<QString,double>& extensionSamplingRates){
    files = findDirectChild(FILES);
    //If the files element does not exist, create it
    if(files.isNull()){
        newFilesNode = true;
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
            QDomText samplingRateValue = doc.createTextNode(QString::fromLatin1("%1").arg(samplingRate,0,'g',14));
            samplingRateElement.appendChild(samplingRateValue);

            QDomElement file = doc.createElement(neuroscope::FILE);
            file.appendChild(extensionElement);
            file.appendChild(samplingRateElement);

            files.appendChild(file);
        }
        return true;
    }
    else{
        QMap<QString,double>::ConstIterator iterator;
        for(iterator = extensionSamplingRates.constBegin(); iterator != extensionSamplingRates.constEnd(); ++iterator){
            //Get the extension information (extension and sampling rate)
            QString extension = iterator.key();
            double samplingRate = iterator.value();
            QDomNode file = findDirectChild(neuroscope::FILE,EXTENSION,extension,files);
            //if a node with the given criteria exists modify it
            if(!file.isNull()){
                QDomNode samplingRateNode = findDirectChild(SAMPLING_RATE,file);
                QDomText samplingRateTextChild = samplingRateNode.firstChild().toText();
                if(!samplingRateTextChild.isNull()) samplingRateTextChild.setNodeValue(QString::fromLatin1("%1").arg(samplingRate,0,'g',14));
                else return false;
            }
            //else create a new one
            else{
                QDomElement extensionElement = doc.createElement(EXTENSION);
                QDomText extensionValue = doc.createTextNode(extension);
                extensionElement.appendChild(extensionValue);

                QDomElement samplingRateElement = doc.createElement(SAMPLING_RATE);
                QDomText samplingRateValue = doc.createTextNode(QString::fromLatin1("%1").arg(samplingRate,0,'g',14));
                samplingRateElement.appendChild(samplingRateValue);

                QDomElement file = doc.createElement(neuroscope::FILE);
                file.appendChild(extensionElement);
                file.appendChild(samplingRateElement);
                //add the new node as the last child of the files node
                files.appendChild(file);
            }
        }
        return true;
    }
}

