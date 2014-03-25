/***************************************************************************
                          neuroscopexmlreader.cpp  -  description
                             -------------------
    begin                : Tue Mar 2 2004
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
#include "neuroscopexmlreader.h"
#include "tags.h"

#include <QList>

//include files for QT
#include <QFileInfo> 
#include <QString> 
#include <QDomDocument>
#include <QDebug>

using namespace neuroscope;

NeuroscopeXmlReader::NeuroscopeXmlReader()
{
}

NeuroscopeXmlReader::~NeuroscopeXmlReader(){
}

bool NeuroscopeXmlReader::parseFile(const QString& url,fileType type){
    this->type = type;
    QFile input(url);

    QDomDocument docElement;
    QString errorMsg;
    int errorRow;
    int errorCol;
    if ( !docElement.setContent( &input, &errorMsg, &errorRow, &errorCol ) ) {
        qWarning() << "Unable to load document.Parse error in " << url << ", line " << errorRow << ", col " << errorCol << ": " << errorMsg << endl;
        return false;
    }

    QDomElement element = docElement.documentElement();

    if (element.tagName() == QLatin1String("parameters")) {
        if( element.hasAttribute(VERSION)) {
            readVersion = element.attribute(VERSION);
        }
    } else if (element.tagName() == QLatin1String("neuroscope")) {
        if( element.hasAttribute(VERSION)) {
            readVersion = element.attribute(VERSION);
        }
    }

    documentNode = element;
    return true;
}


void NeuroscopeXmlReader::closeFile(){
    readVersion.clear();
}


int NeuroscopeXmlReader::getResolution()const{
    int resolution = 0;
    QDomNode n = documentNode.firstChild();
    if (!n.isNull()) {
        while(!n.isNull()) {
            QDomElement e = n.toElement(); // try to convert the node to an element.
            if(!e.isNull()) {
                QString tag = e.tagName();
                if (tag == ACQUISITION) {
                    QDomNode acquisition = e.firstChild(); // try to convert the node to an element.
                    while(!acquisition.isNull()) {
                        QDomElement u = acquisition.toElement();
                        if (!u.isNull()) {
                            tag = u.tagName();
                            if (tag == BITS) {
                                resolution = u.text().toInt();
                                return resolution;
                            }
                        }
                        acquisition = acquisition.nextSibling();
                    }
                    break;
                }
            }
            n = n.nextSibling();
        }
    }

    return resolution;
}

int NeuroscopeXmlReader::getNbChannels()const{
    int nbChannels = 0;
    QDomNode n = documentNode.firstChild();
    if (!n.isNull()) {
        while(!n.isNull()) {
            QDomElement e = n.toElement(); // try to convert the node to an element.
            if(!e.isNull()) {
                QString tag = e.tagName();
                if (tag == ACQUISITION) {
                    QDomNode acquisition = e.firstChild(); // try to convert the node to an element.
                    while(!acquisition.isNull()) {
                        QDomElement u = acquisition.toElement();
                        if (!u.isNull()) {
                            tag = u.tagName();
                            if (tag == NB_CHANNELS) {
                                nbChannels = u.text().toInt();
                                return nbChannels;
                            }
                        }
                        acquisition = acquisition.nextSibling();
                    }
                    break;
                }
            }
            n = n.nextSibling();
        }
    }
    return nbChannels;
}

double NeuroscopeXmlReader::getSamplingRate()const{
    double samplingRate = 0;

    QDomNode n = documentNode.firstChild();
    if (!n.isNull()) {
        while(!n.isNull()) {
            QDomElement e = n.toElement(); // try to convert the node to an element.
            if(!e.isNull()) {
                QString tag = e.tagName();
                if (tag == ACQUISITION) {
                    QDomNode acquisition = e.firstChild(); // try to convert the node to an element.
                    while(!acquisition.isNull()) {
                        QDomElement u = acquisition.toElement();
                        if (!u.isNull()) {
                            tag = u.tagName();
                            if (tag == SAMPLING_RATE) {
                                samplingRate = u.text().toDouble();
                                return samplingRate;
                            }
                        }
                        acquisition = acquisition.nextSibling();
                    }
                    break;
                }
            }
            n = n.nextSibling();
        }
    }
    return samplingRate;
}

double NeuroscopeXmlReader::getUpsamplingRate()const{
    double upsamplingRate = 0;

    QDomNode n = documentNode.firstChild();
    if (!n.isNull()) {
        while(!n.isNull()) {
            QDomElement e = n.toElement(); // try to convert the node to an element.
            if(!e.isNull()) {
                QString tag = e.tagName();
                if (tag == NEUROSCOPE) {
                    QDomNode video = e.firstChildElement(SPIKES); // try to convert the node to an element.
                    if (!video.isNull()) {
                        QDomNode b = video.firstChild();
                        while(!b.isNull()) {
                            QDomElement w = b.toElement();
                            if(!w.isNull()) {
                                tag = w.tagName();
                                if (tag == UPSAMPLING_RATE) {
                                    upsamplingRate =  w.text().toDouble();
                                    return upsamplingRate;
                                }
                            }
                            b = b.nextSibling();
                        }
                    }
                }
            }
            n = n.nextSibling();
        }
    }
    return upsamplingRate;
}


double NeuroscopeXmlReader::getLfpInformation()const{
    double lfpSamplingRate = 0;
    QDomNode n = documentNode.firstChild();
    if (!n.isNull()) {
        while(!n.isNull()) {
            QDomElement e = n.toElement(); // try to convert the node to an element.
            if(!e.isNull()) {
                QString tag = e.tagName();
                if (tag == FIELD_POTENTIALS) {
                    QDomNode video = e.firstChild(); // try to convert the node to an element.
                    while(!video.isNull()) {
                        QDomElement u = video.toElement();
                        if (!u.isNull()) {
                            tag = u.tagName();
                            if (tag == LFP_SAMPLING_RATE) {
                                lfpSamplingRate = u.text().toDouble();
                                return lfpSamplingRate;
                            }
                        }
                        video = video.nextSibling();
                    }
                    break;
                }
            }
            n = n.nextSibling();
        }
    }
    return lfpSamplingRate;
}


float NeuroscopeXmlReader::getScreenGain() const{
    float gain = 0;

    QDomNode n = documentNode.firstChild();
    if (!n.isNull()) {
        while(!n.isNull()) {
            QDomElement e = n.toElement(); // try to convert the node to an element.
            if(!e.isNull()) {
                QString tag = e.tagName();
                if (tag == NEUROSCOPE) {
                    QDomNode video = e.firstChildElement(MISCELLANEOUS); // try to convert the node to an element.
                    if (!video.isNull()) {
                        QDomNode b = video.firstChild();
                        while(!b.isNull()) {
                            QDomElement w = b.toElement();
                            if(!w.isNull()) {
                                tag = w.tagName();
                                if (tag == SCREENGAIN) {
                                    gain = w.text().toFloat();
                                    return gain;
                                }
                            }
                            b = b.nextSibling();
                        }
                    }
                }
            }
            n = n.nextSibling();
        }
    }
    return gain;
}


int NeuroscopeXmlReader::getVoltageRange() const{
    int range = 0;

    if(type == SESSION && readVersion.isEmpty() ) {
        QDomNode n = documentNode.firstChild();
        if (!n.isNull()) {
            while(!n.isNull()) {
                QDomElement e = n.toElement(); // try to convert the node to an element.
                if(!e.isNull()) {
                    QString tag = e.tagName();
                    if (tag == MISCELLANEOUS) {
                        QDomNode video = e.firstChild(); // try to convert the node to an element.
                        while(!video.isNull()) {
                            QDomElement u = video.toElement();
                            if (!u.isNull()) {
                                tag = u.tagName();
                                if(tag == VOLTAGE_RANGE) {
                                    range = u.text().toInt();
                                    return range;
                                }
                            }
                            video = video.nextSibling();
                        }
                        break;
                    }
                }
                n = n.nextSibling();
            }
        }
    } else {
        QDomNode n = documentNode.firstChild();
        if (!n.isNull()) {
            while(!n.isNull()) {
                QDomElement e = n.toElement(); // try to convert the node to an element.
                if(!e.isNull()) {
                    QString tag = e.tagName();
                    if (tag == NEUROSCOPE) {
                        QDomNode video = e.firstChildElement(ACQUISITION); // try to convert the node to an element.
                        if (!video.isNull()) {
                            QDomNode b = video.firstChild();
                            while(!b.isNull()) {
                                QDomElement w = b.toElement();
                                if(!w.isNull()) {
                                    tag = w.tagName();
                                    if (tag == VOLTAGE_RANGE) {
                                        range =  w.text().toInt();
                                        return range;
                                    }
                                }
                                b = b.nextSibling();
                            }
                        }
                    }
                }
                n = n.nextSibling();
            }
        }
    }
    return range;
}


int NeuroscopeXmlReader::getAmplification() const{
    int amplification = 0;
    //The tag has change of location, it was inside the MISCELLANEOUS element, it is now inside the ACQUISITION element.
    if(type == SESSION && readVersion.isEmpty() ) {
        QDomNode n = documentNode.firstChild();
        if (!n.isNull()) {
            while(!n.isNull()) {
                QDomElement e = n.toElement(); // try to convert the node to an element.
                if(!e.isNull()) {
                    QString tag = e.tagName();
                    if (tag == MISCELLANEOUS) {
                        QDomNode video = e.firstChild(); // try to convert the node to an element.
                        while(!video.isNull()) {
                            QDomElement u = video.toElement();
                            if (!u.isNull()) {
                                tag = u.tagName();
                                if(tag == AMPLIFICATION) {
                                    amplification = u.text().toInt();
                                    return amplification;
                                }
                            }
                            video = video.nextSibling();
                        }
                        break;
                    }
                }
                n = n.nextSibling();
            }
        }
    } else {
        QDomNode n = documentNode.firstChild();
        if (!n.isNull()) {
            while(!n.isNull()) {
                QDomElement e = n.toElement(); // try to convert the node to an element.
                if(!e.isNull()) {
                    QString tag = e.tagName();
                    if (tag == NEUROSCOPE) {
                        QDomNode video = e.firstChildElement(ACQUISITION); // try to convert the node to an element.
                        if (!video.isNull()) {
                            QDomNode b = video.firstChild();
                            while(!b.isNull()) {
                                QDomElement w = b.toElement();
                                if(!w.isNull()) {
                                    tag = w.tagName();
                                    if (tag == AMPLIFICATION) {
                                        amplification =  w.text().toInt();
                                        return amplification;
                                    }
                                }
                                b = b.nextSibling();
                            }
                        }
                    }
                }
                n = n.nextSibling();
            }
        }
    }
    return amplification;
}


int NeuroscopeXmlReader::getOffset()const{
    int offset = 0;

    if(type == SESSION && readVersion.isEmpty() ) {
        QDomNode n = documentNode.firstChild();
        if (!n.isNull()) {
            while(!n.isNull()) {
                QDomElement e = n.toElement(); // try to convert the node to an element.
                if(!e.isNull()) {
                    QString tag = e.tagName();
                    if (tag == MISCELLANEOUS) {
                        QDomNode video = e.firstChild(); // try to convert the node to an element.
                        while(!video.isNull()) {
                            QDomElement u = video.toElement();
                            if (!u.isNull()) {
                                tag = u.tagName();
                                if(tag == OFFSET) {
                                    offset = u.text().toInt();
                                    return offset;
                                }
                            }
                            video = video.nextSibling();
                        }
                        break;
                    }
                }
                n = n.nextSibling();
            }
        }
    } else {
        QDomNode n = documentNode.firstChild();
        if (!n.isNull()) {
            while(!n.isNull()) {
                QDomElement e = n.toElement(); // try to convert the node to an element.
                if(!e.isNull()) {
                    QString tag = e.tagName();
                    if (tag == NEUROSCOPE) {
                        QDomNode video = e.firstChildElement(ACQUISITION); // try to convert the node to an element.
                        if (!video.isNull()) {
                            QDomNode b = video.firstChild();
                            while(!b.isNull()) {
                                QDomElement w = b.toElement();
                                if(!w.isNull()) {
                                    tag = w.tagName();
                                    if (tag == OFFSET) {
                                        offset =  w.text().toInt();
                                        return offset;
                                    }
                                }
                                b = b.nextSibling();
                            }
                        }
                    }
                }
                n = n.nextSibling();
            }
        }
    }
    return offset;
}


QList<ChannelDescription> NeuroscopeXmlReader::getChannelDescription() const {
    QList<ChannelDescription> list;
    QDomNode n = documentNode.firstChild();
    if (!n.isNull()) {
        while(!n.isNull()) {
            QDomElement e = n.toElement(); // try to convert the node to an element.
            if(!e.isNull()) {
                QString tag = e.tagName();
                if (tag == NEUROSCOPE) {
                    QDomNode channels = e.firstChildElement(CHANNELS); // try to convert the node to an element.
                    if (!channels.isNull()) {
                        QDomNode channelColors = channels.firstChild();
                        while(!channelColors.isNull()) {
                            QDomElement w = channelColors.toElement();
                            if(!w.isNull()) {
                                tag = w.tagName();
                                if (tag == CHANNEL_COLORS) {
                                    QDomNode channelGroup = w.firstChild(); // try to convert the node to an element.
                                    ChannelDescription channelColors;
                                    while(!channelGroup.isNull()) {
                                        QDomElement val = channelGroup.toElement();
                                        if (!val.isNull()) {
                                            tag = val.tagName();
                                            if (tag == CHANNEL) {
                                                int channelId = val.text().toInt();
                                                channelColors.setId(channelId) ;
                                            } else if(tag == COLOR) {
                                                QString color = val.text();
                                                channelColors.setColor(color) ;
                                            } else if(tag == ANATOMY_COLOR) {
                                                QString color = val.text();
                                                channelColors.setGroupColor(color) ;
                                            } else if(tag == SPIKE_COLOR){
                                                QString color = val.text();
                                                channelColors.setSpikeGroupColor(color);
                                            }
                                        }
                                        channelGroup =  channelGroup.nextSibling();
                                    }
                                    list.append(channelColors);
                                }
                            }
                            channelColors = channelColors.nextSibling();
                        }
                    }
                }
            }
            n = n.nextSibling();
        }
    }
    return list;
}

void NeuroscopeXmlReader::getChannelDefaultOffset(QMap<int,int>& channelDefaultOffsets){
    QDomNode n = documentNode.firstChild();
    if (!n.isNull()) {
        while(!n.isNull()) {
            QDomElement e = n.toElement(); // try to convert the node to an element.
            if(!e.isNull()) {
                QString tag = e.tagName();
                if (tag == NEUROSCOPE) {
                    QDomNode channels = e.firstChildElement(CHANNELS); // try to convert the node to an element.
                    if (!channels.isNull()) {
                        QDomNode channelColors = channels.firstChild();
                        int i = 0;
                        while(!channelColors.isNull()) {
                            QDomElement w = channelColors.toElement();
                            if(!w.isNull()) {
                                tag = w.tagName();
                                if (tag == CHANNEL_OFFSET) {
                                    QDomNode channelGroup = w.firstChild(); // try to convert the node to an element.
                                    int channelId = i;
                                    int offset = 0;

                                    while(!channelGroup.isNull()) {
                                        QDomElement val = channelGroup.toElement();
                                        if (!val.isNull()) {
                                            tag = val.tagName();
                                            if (tag == CHANNEL) {
                                                channelId = val.text().toInt();
                                            } else if(tag == DEFAULT_OFFSET) {
                                                offset = val.text().toInt();
                                            }
                                        }
                                        //the channels must be numbered continuously from 0.
                                        //if(channelId < nbChannels)
                                        channelDefaultOffsets.insert(channelId,offset);
                                        channelGroup =  channelGroup.nextSibling();
                                    }
                                }
                            }
                            channelColors = channelColors.nextSibling();
                            i++;
                        }
                    }
                }
            }
            n = n.nextSibling();
        }
    }
}

void NeuroscopeXmlReader::getSpikeDescription(int nbChannels,QMap<int,int>& spikeChannelsGroups,QMap<int, QList<int> >& spikeGroupsChannels){ 
    //Anatomical goups and spike groups share the trash group. the spikeChannelsGroups already contains the trash group, if any, set after retrieving the anatomical groups information.
    //At first, if a channel is not in the trash group it is put in the undefined group, the -1 (this correspond to no spike group).
    //Then reading for the file, the right information is set.
    QList<int> trashList;
    if(spikeGroupsChannels.contains(0)) trashList = spikeGroupsChannels[0];
    QList<int> spikeTrashList;
    for(int i = 0; i < nbChannels; ++i){
        if(!trashList.contains(i)){
            spikeTrashList.append(i);
            spikeChannelsGroups.insert(i,-1);
        }
        else spikeChannelsGroups.insert(i,0);
    }
    if(readVersion.isEmpty() || readVersion == "1.2.2") {
        QDomNode n = documentNode.firstChild();
        if (!n.isNull()) {
            while(!n.isNull()) {
                QDomElement e = n.toElement(); // try to convert the node to an element.
                if(!e.isNull()) {
                    QString tag = e.tagName();
                    if (tag == SPIKE) {
                        QDomNode anatomy = e.firstChild(); // try to convert the node to an element.
                        while(!anatomy.isNull()) {
                            QDomElement u = anatomy.toElement();
                            if (!u.isNull()) {
                                tag = u.tagName();
                                if (tag == CHANNEL_GROUPS) {
                                    QDomNode channelGroup = u.firstChild(); // try to convert the node to an element.
                                    int i = 0;
                                    while(!channelGroup.isNull()) {
                                        QDomElement val = channelGroup.toElement();
                                        if (!val.isNull()) {
                                            tag = val.tagName();
                                            if (tag == GROUP) {
                                                QDomNode group = val.firstChild(); // try to convert the node to an element.
                                                QList<int> channelList;
                                                while(!group.isNull()) {
                                                    QDomElement valGroup = group.toElement();

                                                    if (!valGroup.isNull()) {
                                                        tag = valGroup.tagName();
                                                        if (tag == CHANNEL) {
                                                            int channelId = valGroup.text().toInt();
                                                            channelList.append(channelId);
                                                            spikeChannelsGroups.insert(channelId,i + 1);//overwrite the entry for the spike trash group (-1)
                                                            //remove the channel from the spike trash list as it is part of a group
                                                            spikeTrashList.removeAll(channelId);
                                                        }
                                                    }
                                                    group = group.nextSibling();
                                                }
                                                spikeGroupsChannels.insert(i + 1,channelList);
                                                ++i;
                                            }
                                        }
                                        channelGroup = channelGroup.nextSibling();
                                    }
                                }
                            }
                            anatomy = anatomy.nextSibling();
                        }
                        break;
                    }
                }
                n = n.nextSibling();
            }
        }
    } else {
        QDomNode n = documentNode.firstChild();
        if (!n.isNull()) {
            while(!n.isNull()) {
                QDomElement e = n.toElement(); // try to convert the node to an element.
                if(!e.isNull()) {
                    QString tag = e.tagName();
                    if (tag == SPIKE) {
                        QDomNode anatomy = e.firstChild(); // try to convert the node to an element.
                        while(!anatomy.isNull()) {
                            QDomElement u = anatomy.toElement();
                            if (!u.isNull()) {
                                tag = u.tagName();
                                if (tag == CHANNEL_GROUPS) {
                                    QDomNode channelGroup = u.firstChild(); // try to convert the node to an element.
                                    int i = 0;
                                    while(!channelGroup.isNull()) {
                                        QDomElement val = channelGroup.toElement();
                                        if (!val.isNull()) {
                                            tag = val.tagName();
                                            if (tag == GROUP) {
                                                QDomNode group = val.firstChild(); // try to convert the node to an element.

                                                QList<int> channelList;
                                                while(!group.isNull()) {
                                                    QDomElement valGroup = group.toElement();


                                                    if (!valGroup.isNull()) {
                                                        tag = valGroup.tagName();
                                                        if( tag == CHANNELS) {
                                                            QDomNode channelsNode = valGroup.firstChild(); // try to convert the node to an element.
                                                            while(!channelsNode.isNull()) {
                                                                QDomElement channelsElement = channelsNode.toElement();
                                                                if (!channelsElement.isNull()) {
                                                                    tag = channelsElement.tagName();
                                                                    if (tag == CHANNEL) {
                                                                        int channelId = channelsElement.text().toInt();
                                                                        channelList.append(channelId);
                                                                        spikeChannelsGroups.insert(channelId,i + 1);//overwrite the entry for the spike trash group (-1)
                                                                        //remove the channel from the spike trash list as it is part of a group
                                                                        spikeTrashList.removeAll(channelId);
                                                                    }
                                                                }
                                                                channelsNode = channelsNode.nextSibling();
                                                            }
                                                        }
                                                    }
                                                    group = group.nextSibling();
                                                }

                                                spikeGroupsChannels.insert(i + 1,channelList);
                                                ++i;
                                            }
                                        }
                                        channelGroup = channelGroup.nextSibling();
                                    }
                                }
                            }
                            anatomy = anatomy.nextSibling();
                        }
                        break;
                    }
                }
                n = n.nextSibling();
            }
        }

    }
    if(!spikeTrashList.isEmpty())
        spikeGroupsChannels.insert(-1,spikeTrashList);

}

void NeuroscopeXmlReader::getAnatomicalDescription(int nbChannels,QMap<int,int>& displayChannelsGroups,QMap<int, QList<int> >& displayGroupsChannels,QMap<int,bool>& skipStatus){
    //First, everything is put in the trash group with a skip status at false (this correspond to no anatomical group).
    //Then reading for the file, the right information is set.
    QList<int> trashList;
    for(int i = 0; i < nbChannels; ++i){
        trashList.append(i);
        displayChannelsGroups.insert(i,0);
        skipStatus.insert(i,false);
    }


    int i = 0;
    QDomNode n = documentNode.firstChild();
    if (!n.isNull()) {
        while(!n.isNull()) {
            QDomElement e = n.toElement(); // try to convert the node to an element.
            if(!e.isNull()) {
                QString tag = e.tagName();
                if (tag == ANATOMY) {
                    QDomNode video = e.firstChild(); // try to convert the node to an element.
                    if (!video.isNull()) {

                        QDomNode b = video.firstChild();
                        while(!b.isNull()) {
                            QDomElement w = b.toElement();
                            if(!w.isNull()) {
                                tag = w.tagName();
                                if (tag == GROUP) {
                                    QList<int> channelList;
                                    QDomNode fileNode = w.firstChild(); // try to convert the node to an element.
                                    while(!fileNode.isNull()) {
                                        QDomElement fileElement = fileNode.toElement();

                                        if (!fileElement.isNull()) {
                                            tag = fileElement.tagName();
                                            if (tag == CHANNEL) {
                                                int channelId = fileElement.text().toInt();
                                                channelList.append(channelId);
                                                displayChannelsGroups.insert(channelId,i + 1);//overwrite the entry for the trash group (0)

                                                //remove the channel from the trash list as it is part of a group
                                                trashList.removeAll(channelId);
                                                if (fileElement.hasAttribute(SKIP)) {
                                                    int skip = fileElement.attribute(SKIP).toInt();
                                                    skipStatus.insert(channelId,skip);
                                                }
                                            }
                                        }
                                        fileNode = fileNode.nextSibling();
                                    }
                                    displayGroupsChannels.insert(i + 1,channelList);
                                    ++i;

                                }
                            }
                            b = b.nextSibling();
                        }
                    }
                }
            }
            n = n.nextSibling();
        }
    }

    if(!trashList.isEmpty())
        displayGroupsChannels.insert(0,trashList);

}

int NeuroscopeXmlReader::getNbSamples()const{
    int nbSamples = 0;
    //The tag has change of location, it was inside the SPIKE element, it is now inside the NEUROSCOPE/SPIKES element.
    if(type == SESSION && (readVersion.isEmpty() || readVersion == "1.2.2")) {
        QDomNode n = documentNode.firstChild();
        if (!n.isNull()) {
            while(!n.isNull()) {
                QDomElement e = n.toElement(); // try to convert the node to an element.
                if(!e.isNull()) {
                    QString tag = e.tagName();
                    if (tag == SPIKE) {
                        QDomNode video = e.firstChild(); // try to convert the node to an element.
                        while(!video.isNull()) {
                            QDomElement u = video.toElement();
                            if (!u.isNull()) {
                                tag = u.tagName();
                                if(tag == NB_SAMPLES) {
                                    nbSamples = u.text().toInt();
                                    return nbSamples;
                                }
                            }
                            video = video.nextSibling();
                        }
                        break;
                    }
                }
                n = n.nextSibling();
            }
        }
    } else {
        QDomNode n = documentNode.firstChild();
        if (!n.isNull()) {
            while(!n.isNull()) {
                QDomElement e = n.toElement(); // try to convert the node to an element.
                if(!e.isNull()) {
                    QString tag = e.tagName();
                    if (tag == NEUROSCOPE) {
                        QDomNode video = e.firstChildElement(SPIKES); // try to convert the node to an element.
                        if (!video.isNull()) {
                            QDomNode b = video.firstChild();
                            while(!b.isNull()) {
                                QDomElement w = b.toElement();
                                if(!w.isNull()) {
                                    tag = w.tagName();
                                    if (tag == NB_SAMPLES) {
                                        nbSamples =  w.text().toInt();
                                        return nbSamples;
                                    }
                                }
                                b = b.nextSibling();
                            }
                        }
                    }
                }
                n = n.nextSibling();
            }
        }
    }
    return nbSamples;
}

float NeuroscopeXmlReader::getWaveformLength()const{
    float waveformLength = 0;

    QDomNode n = documentNode.firstChild();
    if (!n.isNull()) {
        while(!n.isNull()) {
            QDomElement e = n.toElement(); // try to convert the node to an element.
            if(!e.isNull()) {
                QString tag = e.tagName();
                if (tag == NEUROSCOPE) {
                    QDomNode video = e.firstChildElement(SPIKES); // try to convert the node to an element.
                    if (!video.isNull()) {
                        QDomNode b = video.firstChild();
                        while(!b.isNull()) {
                            QDomElement w = b.toElement();
                            if(!w.isNull()) {
                                tag = w.tagName();
                                if (tag == WAVEFORM_LENGTH) {
                                    waveformLength =  w.text().toFloat();
                                    return waveformLength;
                                }
                            }
                            b = b.nextSibling();
                        }
                    }
                }
            }
            n = n.nextSibling();
        }
    }
    return waveformLength;
}


int NeuroscopeXmlReader::getPeakSampleIndex()const{
    int index = 0;
    //The tag has change of location, it was inside the SPIKE element, it is now inside the NEUROSCOPE/SPIKES element.
    if(type == SESSION && (readVersion.isEmpty() || readVersion == "1.2.2")) {
        QDomNode n = documentNode.firstChild();
        if (!n.isNull()) {
            while(!n.isNull()) {
                QDomElement e = n.toElement(); // try to convert the node to an element.
                if(!e.isNull()) {
                    QString tag = e.tagName();
                    if (tag == SPIKE) {
                        QDomNode video = e.firstChild(); // try to convert the node to an element.
                        while(!video.isNull()) {
                            QDomElement u = video.toElement();
                            if (!u.isNull()) {
                                tag = u.tagName();
                                if(tag == PEAK_SAMPLE_INDEX) {
                                    index = u.text().toInt();
                                    return index;
                                }
                            }
                            video = video.nextSibling();
                        }
                        break;
                    }
                }
                n = n.nextSibling();
            }
        }
    } else {
        QDomNode n = documentNode.firstChild();
        if (!n.isNull()) {
            while(!n.isNull()) {
                QDomElement e = n.toElement(); // try to convert the node to an element.
                if(!e.isNull()) {
                    QString tag = e.tagName();
                    if (tag == NEUROSCOPE) {
                        QDomNode video = e.firstChildElement(SPIKES); // try to convert the node to an element.
                        if (!video.isNull()) {
                            QDomNode b = video.firstChild();
                            while(!b.isNull()) {
                                QDomElement w = b.toElement();
                                if(!w.isNull()) {
                                    tag = w.tagName();
                                    if (tag == PEAK_SAMPLE_INDEX) {
                                        index =  w.text().toInt();
                                        return index;
                                    }
                                }
                                b = b.nextSibling();
                            }
                        }
                    }
                }
                n = n.nextSibling();
            }
        }
    }
    return index;
}

float NeuroscopeXmlReader::getPeakSampleLength()const{
    float indexLength = 0;


    QDomNode n = documentNode.firstChild();
    if (!n.isNull()) {
        while(!n.isNull()) {
            QDomElement e = n.toElement(); // try to convert the node to an element.
            if(!e.isNull()) {
                QString tag = e.tagName();
                if (tag == NEUROSCOPE) {
                    QDomNode video = e.firstChildElement(SPIKES); // try to convert the node to an element.
                    if (!video.isNull()) {
                        QDomNode b = video.firstChild();
                        while(!b.isNull()) {
                            QDomElement w = b.toElement();
                            if(!w.isNull()) {
                                tag = w.tagName();
                                if (tag == PEAK_SAMPLE_LENGTH) {
                                    indexLength =  w.text().toFloat();
                                    return indexLength;
                                }
                            }
                            b = b.nextSibling();
                        }
                    }
                }
            }
            n = n.nextSibling();
        }
    }

    return indexLength;
}


int NeuroscopeXmlReader::getVideoWidth()const{
    int width = 0;
    QDomNode n = documentNode.firstChild();
    if (!n.isNull()) {
        while(!n.isNull()) {
            QDomElement e = n.toElement(); // try to convert the node to an element.
            if(!e.isNull()) {
                QString tag = e.tagName();
                if (tag == VIDEO) {
                    QDomNode video = e.firstChild(); // try to convert the node to an element.
                    while(!video.isNull()) {
                        QDomElement u = video.toElement();
                        if (!u.isNull()) {
                            tag = u.tagName();
                            if(tag == WIDTH) {
                                width = u.text().toInt();
                                return width;
                            }
                        }
                        video = video.nextSibling();
                    }
                    break;
                }
            }
            n = n.nextSibling();
        }
    }
    return width;
}


int NeuroscopeXmlReader::getVideoHeight()const{
    int height = 0;
    QDomNode n = documentNode.firstChild();
    if (!n.isNull()) {
        while(!n.isNull()) {
            QDomElement e = n.toElement(); // try to convert the node to an element.
            if(!e.isNull()) {
                QString tag = e.tagName();
                if (tag == VIDEO) {
                    QDomNode video = e.firstChild(); // try to convert the node to an element.
                    while(!video.isNull()) {
                        QDomElement u = video.toElement();
                        if (!u.isNull()) {
                            tag = u.tagName();
                            if(tag == HEIGHT) {
                                height = u.text().toInt();
                                return height;
                            }
                        }
                        video = video.nextSibling();
                    }
                    break;
                }
            }
            n = n.nextSibling();
        }
    }
    return height;
}


int NeuroscopeXmlReader::getRotation()const{
    int angle = 0;

    QDomNode n = documentNode.firstChild();
    if (!n.isNull()) {
        while(!n.isNull()) {
            QDomElement e = n.toElement(); // try to convert the node to an element.
            if(!e.isNull()) {
                QString tag = e.tagName();
                if (tag == VIDEO) {
                    QDomNode video = e.firstChild(); // try to convert the node to an element.
                    while(!video.isNull()) {
                        QDomElement u = video.toElement();
                        if (!u.isNull()) {
                            tag = u.tagName();
                            if(tag == ROTATE) {
                                angle = u.text().toInt();
                                return angle;
                            }
                        }
                        video = video.nextSibling();
                    }
                    break;
                }
            }
            n = n.nextSibling();
        }
    }
    return angle;
}


int NeuroscopeXmlReader::getFlip()const{
    int orientation = 0;
    QDomNode n = documentNode.firstChild();
    if (!n.isNull()) {
        while(!n.isNull()) {
            QDomElement e = n.toElement(); // try to convert the node to an element.
            if(!e.isNull()) {
                QString tag = e.tagName();
                if (tag == VIDEO) {
                    QDomNode video = e.firstChild(); // try to convert the node to an element.
                    while(!video.isNull()) {
                        QDomElement u = video.toElement();
                        if (!u.isNull()) {
                            tag = u.tagName();
                            if(tag == FLIP) {
                                orientation = u.text().toInt();
                                return orientation;
                            }
                        }
                        video = video.nextSibling();
                    }
                    break;
                }
            }
            n = n.nextSibling();
        }
    }
    return orientation;
}

int NeuroscopeXmlReader::getTrajectory()const{
    int drawTrajectory = 0;
    QDomNode n = documentNode.firstChild();
    if (!n.isNull()) {
        while(!n.isNull()) {
            QDomElement e = n.toElement(); // try to convert the node to an element.
            if(!e.isNull()) {
                QString tag = e.tagName();
                if (tag == VIDEO) {
                    QDomNode video = e.firstChild(); // try to convert the node to an element.
                    while(!video.isNull()) {
                        QDomElement u = video.toElement();
                        if (!u.isNull()) {
                            tag = u.tagName();
                            if(tag == POSITIONS_BACKGROUND) {
                                drawTrajectory = u.text().toInt();
                                return drawTrajectory;
                            }
                        }
                        video = video.nextSibling();
                    }
                    break;
                }
            }
            n = n.nextSibling();
        }
    }
    return drawTrajectory;
}  

QString NeuroscopeXmlReader::getBackgroundImage()const{
    QString backgroundPath = "-";
    QDomNode n = documentNode.firstChild();
    if (!n.isNull()) {
        while(!n.isNull()) {
            QDomElement e = n.toElement(); // try to convert the node to an element.
            if(!e.isNull()) {
                QString tag = e.tagName();
                if (tag == VIDEO) {
                    QDomNode video = e.firstChild(); // try to convert the node to an element.
                    while(!video.isNull()) {
                        QDomElement u = video.toElement();
                        if (!u.isNull()) {
                            tag = u.tagName();
                            if(tag == VIDEO_IMAGE) {
                                backgroundPath = u.text();
                                return backgroundPath;
                            }
                        }
                        video = video.nextSibling();
                    }
                    break;
                }
            }
            n = n.nextSibling();
        }
    }
    return backgroundPath;
}  

QString NeuroscopeXmlReader::getTraceBackgroundImage()const{
    QString traceBackgroundPath = "-";

    QDomNode n = documentNode.firstChild();
    if (!n.isNull()) {
        while(!n.isNull()) {
            QDomElement e = n.toElement(); // try to convert the node to an element.
            if(!e.isNull()) {
                QString tag = e.tagName();
                if (tag == NEUROSCOPE) {
                    QDomNode video = e.firstChildElement(MISCELLANEOUS); // try to convert the node to an element.
                    if (!video.isNull()) {
                        QDomNode b = video.firstChild();
                        while(!b.isNull()) {
                            QDomElement w = b.toElement();
                            if(!w.isNull()) {
                                tag = w.tagName();
                                if (tag == TRACE_BACKGROUND_IMAGE) {
                                    traceBackgroundPath =  w.text();
                                    return traceBackgroundPath;
                                }
                            }
                            b = b.nextSibling();
                        }
                    }
                }
            }
            n = n.nextSibling();
        }
    }

    return traceBackgroundPath;
} 

QList<SessionFile> NeuroscopeXmlReader::getFilesToLoad(){
    QList<SessionFile> list;

    QDomNode n = documentNode.firstChild();
    if (!n.isNull()) {
        while(!n.isNull()) {
            QDomElement e = n.toElement(); // try to convert the node to an element.
            if(!e.isNull()) {
                QString tag = e.tagName();
                if (tag == FILES) {
                    QDomNode file = e.firstChild();
                    while(!file.isNull()) {
                        QDomElement fileElement = file.toElement();
                        tag = fileElement.tagName();
                        if (tag == neuroscope::FILE) {
                            QDomNode fileNode = fileElement.firstChild(); // try to convert the node to an element.
                            SessionFile sessionFile;
                            while(!fileNode.isNull()) {
                                QDomElement sfileElement = fileNode.toElement();

                                if (!sfileElement.isNull()) {
                                    tag = sfileElement.tagName();
                                    if (tag == TYPE) {
                                        int type = sfileElement.text().toInt();
                                        sessionFile.setType(static_cast<SessionFile::type>(type)) ;
                                    } else if (tag == URL) {
                                        QString url = sfileElement.text();
                                        sessionFile.setUrl(url);
                                    } else if (tag == DATE) {
                                        QString date = sfileElement.text();
                                        sessionFile.setModification(QDateTime::fromString(date,Qt::ISODate));
                                    } else if (tag == VIDEO_IMAGE) {
                                        QString backgroundPath = fileElement.text();
                                        sessionFile.setBackgroundPath(backgroundPath);
                                    } else if (tag == ITEMS) {
                                        QDomNode itemsNode = sfileElement.firstChild(); // try to convert the node to an element.
                                        QString id;
                                        QString color;
                                        while(!itemsNode.isNull()) {

                                            QDomElement itemsElement = itemsNode.toElement();
                                            if (!itemsElement.isNull()) {
                                                tag = itemsElement.tagName();
                                                if (tag == ITEM_DESCRIPTION) {
                                                    QDomNode itemsDescriptionNode = itemsElement.firstChild(); // try to convert the node to an element.
                                                    while(!itemsDescriptionNode.isNull()) {
                                                        QDomElement itemsDescriptionElement = itemsDescriptionNode.toElement();
                                                        if (!itemsDescriptionElement.isNull()) {
                                                            tag = itemsDescriptionElement.tagName();
                                                            if (tag == ITEM) {
                                                                id = itemsDescriptionElement.text();
                                                            } else if (tag == COLOR) {
                                                                color = itemsDescriptionElement.text();
                                                            }
                                                        }
                                                        itemsDescriptionNode = itemsDescriptionNode.nextSibling();
                                                    }
                                                    sessionFile.setItemColor(id,color);

                                                }
                                            }
                                            itemsNode = itemsNode.nextSibling();
                                        }
                                    }
                                }
                                fileNode = fileNode.nextSibling();
                            }
                            list.append(sessionFile);
                        }
                        file = file.nextSibling();
                    }
                }
            }
            n = n.nextSibling();
        }
    }
    return list;

}


QList<DisplayInformation> NeuroscopeXmlReader::getDisplayInformation(){
    QList<DisplayInformation> list;

    QDomNode n = documentNode.firstChild();
    if (!n.isNull()) {
        while(!n.isNull()) {
            QDomElement e = n.toElement(); // try to convert the node to an element.
            if(!e.isNull()) {
                QString tag = e.tagName();
                if (tag == DISPLAYS) {
                    QDomNode b = e.firstChild();
                    while(!b.isNull()) {
                        QDomElement w = b.toElement();
                        if(!w.isNull()) {
                            tag = w.tagName();
                            if (tag == DISPLAY) {
                                QDomNode fileNode = w.firstChild(); // try to convert the node to an element.
                                DisplayInformation displayInformation;
                                while(!fileNode.isNull()) {
                                    QDomElement fileElement = fileNode.toElement();

                                    if (!fileElement.isNull()) {
                                        tag = fileElement.tagName();
                                        if (tag == TAB_LABEL) {
                                            QString label = fileElement.text();
                                            displayInformation.setTabLabel(label);
                                        } else if(tag == SHOW_LABELS){
                                            int showLabels = fileElement.text().toInt();
                                            displayInformation.setLabelStatus(showLabels);
                                        } else if(tag == START_TIME){
                                            long startTime = fileElement.text().toLong();
                                            displayInformation.setStartTime(startTime);
                                        } else if(tag == DURATION){
                                            long duration = fileElement.text().toLong();
                                            displayInformation.setTimeWindow(duration);
                                        } else if(tag == MULTIPLE_COLUMNS){
                                            int presentationMode = fileElement.text().toInt();
                                            displayInformation.setMode(static_cast<DisplayInformation::mode>(presentationMode));
                                        } else if(tag == GREYSCALE){
                                            int greyScale = fileElement.text().toInt();
                                            displayInformation.setGreyScale(greyScale);
                                        } else if(tag == AUTOCENTER_CHANNELS){
                                            bool autocenterChannels = fileElement.text().toInt();
                                            displayInformation.setAutocenterChannels(autocenterChannels);
                                        } else if(tag == POSITIONVIEW){
                                            int positionView = fileElement.text().toInt();
                                            displayInformation.setPositionView(positionView);
                                        } else if(tag  == SHOWEVENTS){
                                            int showEvents = fileElement.text().toInt();
                                            displayInformation.setEventsInPositionView(showEvents);
                                        } else if(tag  == SPIKE_PRESENTATION){
                                            int spikePresentation = fileElement.text().toInt();
                                            displayInformation.addSpikeDisplayType(static_cast<DisplayInformation::spikeDisplayType>(spikePresentation));
                                        } else if(tag  == RASTER_HEIGHT){
                                            int height = fileElement.text().toInt();
                                            displayInformation.setRasterHeight(height);
                                        } else if(tag == CLUSTERS_SELECTED){
                                            //loop on the CLUSTERS
                                            QString clusterFile;
                                            QList<int> clusterIds;
                                            QDomNode clustersNode = fileElement.firstChild(); // try to convert the node to an element.
                                            while(!clustersNode.isNull()) {
                                                QDomElement clustersElement = clustersNode.toElement();
                                                if (!clustersElement.isNull()) {
                                                    tag = clustersElement.tagName();
                                                    if (tag == FILE_URL) {
                                                        clusterFile = clustersElement.text();
                                                    } else if (tag == CLUSTER) {
                                                        clusterIds.append(clustersElement.text().toInt());
                                                    }
                                                }
                                                clustersNode = clustersNode.nextSibling();
                                            }
                                            displayInformation.setSelectedClusters(clusterFile,clusterIds);
                                        } else if(tag == SPIKES_SELECTED){
                                            QStringList files;
                                            //loop on the urls of the files
                                            QDomNode spikesNode = fileElement.firstChild(); // try to convert the node to an element.
                                            while(!spikesNode.isNull()) {
                                                QDomElement spikesElement = spikesNode.toElement();
                                                if (!spikesElement.isNull()) {
                                                    tag = spikesElement.tagName();
                                                    if (tag == FILE_URL) {
                                                        files.append(spikesElement.text());
                                                    }
                                                }
                                                spikesNode = spikesNode.nextSibling();

                                            }
                                            displayInformation.setSelectedSpikeFiles(files);
                                        } else if(tag == EVENTS_SELECTED){
                                            //loop on the EVENTS
                                            QString eventFile;
                                            QList<int> eventIds;
                                            QDomNode eventsNode = fileElement.firstChild(); // try to convert the node to an element.
                                            while(!eventsNode.isNull()) {
                                                QDomElement eventsElement = eventsNode.toElement();
                                                if (!eventsElement.isNull()) {
                                                    tag = eventsElement.tagName();
                                                    if (tag == FILE_URL) {
                                                        eventFile.append(eventsElement.text());
                                                    } else if(tag == EVENT) {
                                                        int eventId = eventsElement.text().toInt();
                                                        eventIds.append(eventId);
                                                    }
                                                }
                                                eventsNode = eventsNode.nextSibling();
                                            }
                                            displayInformation.setSelectedEvents(eventFile,eventIds);
                                        } else if(tag == CLUSTERS_SKIPPED){
                                            //loop on the CLUSTERS
                                            QString clusterFile;
                                            QList<int> clusterIds;

                                            QDomNode eventsNode = fileElement.firstChild(); // try to convert the node to an element.
                                            while(!eventsNode.isNull()) {
                                                QDomElement eventsElement = eventsNode.toElement();
                                                if (!eventsElement.isNull()) {
                                                    tag = eventsElement.tagName();
                                                    if (tag == FILE_URL) {
                                                        clusterFile.append(eventsElement.text());
                                                    } else if(tag == EVENT) {
                                                        int eventId = eventsElement.text().toInt();
                                                        clusterIds.append(eventId);
                                                    }
                                                }
                                                eventsNode = eventsNode.nextSibling();
                                            }
                                            displayInformation.setSkippedClusters(clusterFile,clusterIds);
                                        } else if(tag == EVENTS_SKIPPED){
                                            //loop on the EVENTS
                                            QString eventFile;
                                            QList<int> eventIds;
                                            QDomNode eventsNode = fileElement.firstChild(); // try to convert the node to an element.
                                            while(!eventsNode.isNull()) {
                                                QDomElement eventsElement = eventsNode.toElement();
                                                if (!eventsElement.isNull()) {
                                                    tag = eventsElement.tagName();
                                                    if (tag == FILE_URL) {
                                                        eventFile.append(eventsElement.text());
                                                    } else if(tag == EVENT) {
                                                        int eventId = eventsElement.text().toInt();
                                                        eventIds.append(eventId);
                                                    }
                                                }
                                                eventsNode = eventsNode.nextSibling();
                                            }
                                            displayInformation.setSkippedEvents(eventFile,eventIds);
                                        } else if(tag == CHANNELS_SELECTED){
                                            QList<int> channelIds;
                                            //loop on the urls of the files

                                            QDomNode eventsNode = fileElement.firstChild(); // try to convert the node to an element.
                                            while(!eventsNode.isNull()) {
                                                QDomElement eventsElement = eventsNode.toElement();
                                                if (!eventsElement.isNull()) {
                                                    tag = eventsElement.tagName();
                                                    if (tag == CHANNEL) {
                                                        channelIds.append(eventsElement.text().toInt());
                                                    }
                                                }
                                                eventsNode = eventsNode.nextSibling();
                                            }
                                            displayInformation.setSelectedChannelIds(channelIds);
                                        } else if(tag == CHANNELS_SHOWN){
                                            QList<int> channelIds;
                                            //loop on the urls of the files
                                            QDomNode eventsNode = fileElement.firstChild(); // try to convert the node to an element.
                                            while(!eventsNode.isNull()) {
                                                QDomElement eventsElement = eventsNode.toElement();
                                                if (!eventsElement.isNull()) {
                                                    tag = eventsElement.tagName();
                                                    if (tag == CHANNEL) {
                                                        channelIds.append(eventsElement.text().toInt());
                                                    }
                                                }
                                                eventsNode = eventsNode.nextSibling();
                                            }
                                            displayInformation.setChannelIds(channelIds);
                                        } else if(tag == CHANNEL_POSITIONS){
                                            QList<TracePosition> positions;
                                            //loop on the POSITIONS
                                            QDomNode eventsNode = fileElement.firstChild(); // try to convert the node to an element.

                                            TracePosition tracePosition;
                                            while(!eventsNode.isNull()) {
                                                QDomElement eventsElement = eventsNode.toElement();
                                                if (!eventsElement.isNull()) {
                                                    tag = eventsElement.tagName();
                                                    if (tag == CHANNEL_POSITION) {
                                                        QDomNode channelNode = eventsElement.firstChild(); // try to convert the node to an element.
                                                        tag = eventsElement.tagName();
                                                        while(!channelNode.isNull()) {
                                                            QDomElement cElement = channelNode.toElement();
                                                            if (!cElement.isNull()) {
                                                                tag = cElement.tagName();

                                                                if (tag == CHANNEL) {
                                                                    int channelId = cElement.text().toInt();
                                                                    tracePosition.setId(channelId) ;
                                                                } else if(tag == GAIN) {
                                                                    int gain = cElement.text().toInt();
                                                                    tracePosition.setGain(gain);
                                                                } else if(tag == OFFSET) {

                                                                    int offset = cElement.text().toInt();
                                                                    tracePosition.setOffset(offset);
                                                                }
                                                            }
                                                            channelNode = channelNode.nextSibling();
                                                        }
                                                    }
                                                }
                                                eventsNode = eventsNode.nextSibling();
                                                positions.append(tracePosition);
                                            }
                                            displayInformation.setPositions(positions);
                                        }
                                    }

                                    fileNode = fileNode.nextSibling();
                                }
                                list.append(displayInformation);
                            }
                        }
                        b = b.nextSibling();
                    }
                }
            }
            n = n.nextSibling();
        }
    }
    return list;
}


QMap<QString,double> NeuroscopeXmlReader::getSampleRateByExtension(){
    QMap<QString,double> samplingRatesMap;

    //The tag has change of location, it was inside the session file (at SAMPLING_RATES/EXTENSION_SAMPLING_RATE),
    //it is now inside the the parameter file (at FILES/FILE).
    if(type == SESSION && (readVersion.isEmpty() || readVersion == "1.2.2")) {

        QDomNode n = documentNode.firstChild();
        if (!n.isNull()) {
            while(!n.isNull()) {
                QDomElement e = n.toElement(); // try to convert the node to an element.
                if(!e.isNull()) {
                    QString tag = e.tagName();
                    if (tag == NEUROSCOPE) {
                        QDomNode video = e.firstChildElement(SAMPLING_RATES); // try to convert the node to an element.
                        if (!video.isNull()) {
                            QDomNode b = video.firstChild();
                            while(!b.isNull()) {
                                QDomElement w = b.toElement();
                                if(!w.isNull()) {
                                    tag = w.tagName();
                                    if (tag == EXTENSION_SAMPLING_RATE) {
                                        double samplingRate;
                                        QString extension;
                                        QDomNode sampling = w.firstChild();
                                        while(!sampling.isNull()) {
                                            QDomElement samplingElement = b.toElement();
                                            if (!samplingElement.isNull()) {
                                                tag = samplingElement.tagName();
                                                if (tag == EXTENSION) {
                                                    extension = samplingElement.text();
                                                } else if( tag == SAMPLING_RATE){
                                                    samplingRate = samplingElement.text().toDouble();
                                                }
                                            }
                                            sampling = sampling.nextSibling();
                                        }
                                        samplingRatesMap.insert(extension,samplingRate);
                                    }
                                }
                                b = b.nextSibling();
                            }
                        }
                    }
                }
                n = n.nextSibling();
            }
        }
    } else {
        QDomNode n = documentNode.firstChild();
        if (!n.isNull()) {
            while(!n.isNull()) {
                QDomElement e = n.toElement(); // try to convert the node to an element.
                if(!e.isNull()) {
                    QString tag = e.tagName();
                    if (tag == FILES) {
                        QDomNode video = e.firstChildElement(neuroscope::FILE); // try to convert the node to an element.
                        while (!video.isNull()) {
                            QDomNode b = video.firstChild();
                            double samplingRate;
                            QString extension;

                            while(!b.isNull()) {
                                QDomElement w = b.toElement();
                                if(!w.isNull()) {
                                    tag = w.tagName();
                                    if (tag == EXTENSION) {
                                        extension = w.text();
                                    } else if( tag == SAMPLING_RATE){
                                        samplingRate = w.text().toDouble();
                                    }
                                }
                                b = b.nextSibling();
                            }
                            samplingRatesMap.insert(extension,samplingRate);
                            video = video.nextSibling();
                        }

                    }
                }
                n = n.nextSibling();
            }
        }
    }

    return samplingRatesMap;
}



