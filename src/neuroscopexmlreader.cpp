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
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
//application specific include files.
#include "neuroscopexmlreader.h"
#include "tags.h"

// include files for KDE



//General C++ include files
#include <iostream>
//Added by qt3to4:
#include <QList>
using namespace std;

//include files for QT
#include <QFileInfo> 
#include <QString> 

using namespace neuroscope;

NeuroscopeXmlReader::NeuroscopeXmlReader()
    :readVersion("")
{
}

NeuroscopeXmlReader::~NeuroscopeXmlReader(){
}

bool NeuroscopeXmlReader::parseFile(const QString& url,fileType type){
    this->type = type;

    // Init libxml
    xmlInitParser();

    // Load XML document
    doc = xmlParseFile(url);
    if(doc == NULL) return false;

    // Create xpath evaluation context
    xpathContex = xmlXPathNewContext(doc);
    if(xpathContex == NULL){
        xmlFreeDoc(doc);
        return false;
    }

    //Read the document version
    xmlNodePtr rootElement = xmlDocGetRootElement(doc);
    xmlChar* versionTag = xmlCharStrdup(VERSION);
    if(rootElement != NULL){
        xmlChar* sVersion = xmlGetProp(rootElement,versionTag);//get the attribute with the name versionTag
        if(sVersion != NULL) readVersion = QString((char*)sVersion);
        xmlFree(sVersion);
    }
    xmlFree(versionTag);
    
    return true;
}


void NeuroscopeXmlReader::closeFile(){
    //Cleanup
    xmlXPathFreeContext(xpathContex);
    xmlFreeDoc(doc);
    readVersion.clear();

    //Shutdown libxml
    xmlCleanupParser();
}


int NeuroscopeXmlReader::getResolution()const{
    int resolution = 0;
    xmlXPathObjectPtr result;
    xmlChar* searchPath = xmlCharStrdup("//" + ACQUISITION + "/" + BITS);

    //Evaluate xpath expression
    result = xmlXPathEvalExpression(searchPath,xpathContex);
    if(result != NULL){
        xmlNodeSetPtr nodeset = result->nodesetval;
        if(!xmlXPathNodeSetIsEmpty(nodeset)){
            //Should be only one resolution element, so take the first one.
            xmlChar* sResolution = xmlNodeListGetString(doc,nodeset->nodeTab[0]->children, 1);
            resolution = QString((char*)sResolution).toInt();
            xmlFree(sResolution);
        }
    }

    xmlFree(searchPath);
    xmlXPathFreeObject(result);
    return resolution;
}

int NeuroscopeXmlReader::getNbChannels()const{
    int nbChannels = 0;
    xmlXPathObjectPtr result;
    xmlChar* searchPath = xmlCharStrdup("//" + ACQUISITION + "/" + NB_CHANNELS);

    //Evaluate xpath expression
    result = xmlXPathEvalExpression(searchPath,xpathContex);
    if(result != NULL){
        xmlNodeSetPtr nodeset = result->nodesetval;
        if(!xmlXPathNodeSetIsEmpty(nodeset)){
            //Should be only one nbChannels element, so take the first one.
            xmlChar* sNbChannels = xmlNodeListGetString(doc,nodeset->nodeTab[0]->children, 1);
            nbChannels = QString((char*)sNbChannels).toInt();
            xmlFree(sNbChannels);
        }
    }

    xmlFree(searchPath);
    xmlXPathFreeObject(result);
    return nbChannels;
}

double NeuroscopeXmlReader::getSamplingRate()const{
    double samplingRate = 0;
    xmlXPathObjectPtr result;
    xmlChar* searchPath = xmlCharStrdup("//" + ACQUISITION + "/" + SAMPLING_RATE);

    //Evaluate xpath expression
    result = xmlXPathEvalExpression(searchPath,xpathContex);
    if(result != NULL){
        xmlNodeSetPtr nodeset = result->nodesetval;
        if(!xmlXPathNodeSetIsEmpty(nodeset)){
            //Should be only one sampling rate element at that level, so take the first one.
            xmlChar* sSamplingRate = xmlNodeListGetString(doc,nodeset->nodeTab[0]->children, 1);
            samplingRate = QString((char*)sSamplingRate).toDouble();
            xmlFree(sSamplingRate);
        }
    }

    xmlFree(searchPath);
    xmlXPathFreeObject(result);
    return samplingRate;
}

double NeuroscopeXmlReader::getUpsamplingRate()const{
    double upsamplingRate = 0;
    xmlXPathObjectPtr result;
    xmlChar* searchPath = xmlCharStrdup("//" + NEUROSCOPE + "/" + SPIKES + "/" + UPSAMPLING_RATE);

    //Evaluate xpath expression
    result = xmlXPathEvalExpression(searchPath,xpathContex);
    if(result != NULL){
        xmlNodeSetPtr nodeset = result->nodesetval;
        if(!xmlXPathNodeSetIsEmpty(nodeset)){
            //Should be only one upsampling rate element at that level, so take the first one.
            xmlChar* sUpsamplingRate = xmlNodeListGetString(doc,nodeset->nodeTab[0]->children, 1);
            upsamplingRate = QString((char*)sUpsamplingRate).toDouble();
            xmlFree(sUpsamplingRate);
        }
    }

    xmlFree(searchPath);
    xmlXPathFreeObject(result);
    return upsamplingRate;
}


double NeuroscopeXmlReader::getLfpInformation()const{
    double lfpSamplingRate = 0;
    xmlXPathObjectPtr result;
    xmlChar* searchPath = xmlCharStrdup("//" + FIELD_POTENTIALS + "/" + LFP_SAMPLING_RATE);

    //Evaluate xpath expression
    result = xmlXPathEvalExpression(searchPath,xpathContex);
    if(result != NULL){
        xmlNodeSetPtr nodeset = result->nodesetval;
        if(!xmlXPathNodeSetIsEmpty(nodeset)){
            //Should be only one lfpSamplingRate element, so take the first one.
            xmlChar* sLfpSamplingRate = xmlNodeListGetString(doc,nodeset->nodeTab[0]->children, 1);
            lfpSamplingRate = QString((char*)sLfpSamplingRate).toDouble();
            xmlFree(sLfpSamplingRate);
        }
    }

    xmlFree(searchPath);
    xmlXPathFreeObject(result);
    return lfpSamplingRate;

}


float NeuroscopeXmlReader::getScreenGain() const{
    float gain = 0;
    xmlXPathObjectPtr result;
    xmlChar* searchPath = xmlCharStrdup("//" + MISCELLANEOUS + "/" + SCREENGAIN);

    //Evaluate xpath expression
    result = xmlXPathEvalExpression(searchPath,xpathContex);
    if(result != NULL){
        xmlNodeSetPtr nodeset = result->nodesetval;
        if(!xmlXPathNodeSetIsEmpty(nodeset)){
            //Should be only one gain element, so take the first one.
            xmlChar* sGain = xmlNodeListGetString(doc,nodeset->nodeTab[0]->children, 1);
            gain = QString((char*)sGain).toFloat();
            xmlFree(sGain);
        }
    }

    xmlFree(searchPath);
    xmlXPathFreeObject(result);
    return gain;
}


int NeuroscopeXmlReader::getVoltageRange() const{
    int range = 0;
    xmlXPathObjectPtr result;
    xmlChar* searchPath;
    //The tag has change of location, it was inside the MISCELLANEOUS element, it is now inside the ACQUISITION element.
    if(type == SESSION && readVersion == "" ) searchPath = xmlCharStrdup("//" + MISCELLANEOUS + "/" + VOLTAGE_RANGE);
    else searchPath = xmlCharStrdup("//" + ACQUISITION + "/" + VOLTAGE_RANGE);

    //Evaluate xpath expression
    result = xmlXPathEvalExpression(searchPath,xpathContex);
    if(result != NULL){
        xmlNodeSetPtr nodeset = result->nodesetval;
        if(!xmlXPathNodeSetIsEmpty(nodeset)){
            //Should be only one range element, so take the first one.
            xmlChar* sRange = xmlNodeListGetString(doc,nodeset->nodeTab[0]->children, 1);
            range = QString((char*)sRange).toInt();
            xmlFree(sRange);
        }
    }

    xmlFree(searchPath);
    xmlXPathFreeObject(result);
    return range;
}


int NeuroscopeXmlReader::getAmplification() const{
    int amplification = 0;
    xmlXPathObjectPtr result;
    xmlChar* searchPath;
    //The tag has change of location, it was inside the MISCELLANEOUS element, it is now inside the ACQUISITION element.
    if(type == SESSION && readVersion == "") searchPath = xmlCharStrdup("//" + MISCELLANEOUS + "/" + AMPLIFICATION);
    else searchPath = xmlCharStrdup("//" + ACQUISITION + "/" + AMPLIFICATION);

    //Evaluate xpath expression
    result = xmlXPathEvalExpression(searchPath,xpathContex);
    if(result != NULL){
        xmlNodeSetPtr nodeset = result->nodesetval;
        if(!xmlXPathNodeSetIsEmpty(nodeset)){
            //Should be only one amplification element, so take the first one.
            xmlChar* sAmplification = xmlNodeListGetString(doc,nodeset->nodeTab[0]->children, 1);
            amplification = QString((char*)sAmplification).toInt();
            xmlFree(sAmplification);
        }
    }

    xmlFree(searchPath);
    xmlXPathFreeObject(result);
    return amplification;
}


int NeuroscopeXmlReader::getOffset()const{
    int offset = 0;
    xmlXPathObjectPtr result;
    xmlChar* searchPath;
    //The tag has change of location, it was inside the MISCELLANEOUS element, it is now inside the ACQUISITION element.
    if(type == SESSION && readVersion == "") searchPath = xmlCharStrdup("//" + MISCELLANEOUS + "/" + OFFSET);
    else searchPath = xmlCharStrdup("//" + ACQUISITION + "/" + OFFSET);

    //Evaluate xpath expression
    result = xmlXPathEvalExpression(searchPath,xpathContex);
    if(result != NULL){
        xmlNodeSetPtr nodeset = result->nodesetval;
        if(!xmlXPathNodeSetIsEmpty(nodeset)){
            //Should be only one offset element, so take the first one.
            xmlChar* sOffset = xmlNodeListGetString(doc,nodeset->nodeTab[0]->children, 1);
            offset = QString((char*)sOffset).toInt();
            xmlFree(sOffset);
        }
    }

    xmlFree(searchPath);
    xmlXPathFreeObject(result);

    return offset;
}


QList<ChannelDescription> NeuroscopeXmlReader::getChannelDescription(){
    QList<ChannelDescription> list;

    xmlXPathObjectPtr result;
    xmlChar* searchPath = xmlCharStrdup("//" + CHANNELS + "/" + CHANNEL_COLORS);

    //Evaluate xpath expression
    result = xmlXPathEvalExpression(searchPath,xpathContex);
    if(result != NULL){
        xmlNodeSetPtr nodeset = result->nodesetval;
        if(!xmlXPathNodeSetIsEmpty(nodeset)){
            //loop on all the CHANNEL_COLORS.
            int nbChannels = nodeset->nodeNr;
            for(int i = 0; i < nbChannels; ++i){
                ChannelDescription channelDescription;
                xmlNodePtr child;
                for(child = nodeset->nodeTab[i]->children;child != NULL;child = child->next){
                    //skip the carriage return (text node named text and containing /n)
                    if(child->type == XML_TEXT_NODE) continue;

                    if(QString((char*)child->name) == CHANNEL){
                        xmlChar* sId = xmlNodeListGetString(doc,child->children, 1);
                        int channelId = QString((char*)sId).toInt();
                        xmlFree(sId);
                        channelDescription.setId(channelId) ;
                    }
                    if(QString((char*)child->name) == COLOR){
                        xmlChar* sColor = xmlNodeListGetString(doc,child->children, 1);
                        QString color = QString((char*)sColor);
                        xmlFree(sColor);
                        channelDescription.setColor(color) ;
                    }
                    if(QString((char*)child->name) == ANATOMY_COLOR){
                        xmlChar* sColor = xmlNodeListGetString(doc,child->children, 1);
                        QString color = QString((char*)sColor);
                        xmlFree(sColor);
                        channelDescription.setGroupColor(color) ;
                    }
                    if(QString((char*)child->name) == SPIKE_COLOR){
                        xmlChar* sColor = xmlNodeListGetString(doc,child->children, 1);
                        QString color = QString((char*)sColor);
                        xmlFree(sColor);
                        channelDescription.setSpikeGroupColor(color) ;
                    }
                }
                list.append(channelDescription);
            }
        }
    }

    xmlFree(searchPath);
    xmlXPathFreeObject(result);
    return list;
}

void NeuroscopeXmlReader::getChannelDefaultOffset(QMap<int,int>& channelDefaultOffsets){
    xmlXPathObjectPtr result;
    xmlChar* searchPath = xmlCharStrdup("//" + CHANNELS + "/" + CHANNEL_OFFSET);

    //Evaluate xpath expression
    result = xmlXPathEvalExpression(searchPath,xpathContex);
    if(result != NULL){
        xmlNodeSetPtr nodeset = result->nodesetval;
        if(!xmlXPathNodeSetIsEmpty(nodeset)){
            //loop on all the CHANNEL_OFFSET.
            int nbChannels = nodeset->nodeNr;
            for(int i = 0; i < nbChannels; ++i){
                int channelId = i;
                int offset = 0;
                xmlNodePtr child;
                for(child = nodeset->nodeTab[i]->children;child != NULL;child = child->next){
                    //skip the carriage return (text node named text and containing /n)
                    if(child->type == XML_TEXT_NODE) continue;

                    if(QString((char*)child->name) == CHANNEL){
                        xmlChar* sId = xmlNodeListGetString(doc,child->children, 1);
                        channelId = QString((char*)sId).toInt();
                        xmlFree(sId);
                    }
                    if(QString((char*)child->name) == DEFAULT_OFFSET){
                        xmlChar* sOffset = xmlNodeListGetString(doc,child->children, 1);
                        offset =  QString((char*)sOffset).toInt();
                        xmlFree(sOffset);
                    }
                    //the channels must be numbered continuously from 0.
                    if(channelId < nbChannels) channelDefaultOffsets.insert(channelId,offset);
                }
            }
        }
    }

    xmlFree(searchPath);
    xmlXPathFreeObject(result);
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

    xmlXPathObjectPtr result;
    xmlChar* searchPath;
    //The tag has change of location, it was inside CHANNEL_GROUPS/GROUP tag, it is now inside CHANNEL_GROUPS/GROUP/CHANNELS.
    if(readVersion == "" || readVersion == "1.2.2") searchPath = xmlCharStrdup("//" + SPIKE + "/" + CHANNEL_GROUPS + "/" + GROUP);
    else searchPath = xmlCharStrdup("//" + SPIKE + "/" + CHANNEL_GROUPS + "/" + GROUP + "/" + CHANNELS);

    //Evaluate xpath expression
    result = xmlXPathEvalExpression(searchPath,xpathContex);
    if(result != NULL){
        xmlNodeSetPtr nodeset = result->nodesetval;
        if(!xmlXPathNodeSetIsEmpty(nodeset)){
            //loop on all the GROUP.
            int nbGroups = nodeset->nodeNr;
            for(int i = 0; i < nbGroups; ++i){
                QList<int> channelList;
                xmlNodePtr child;
                for(child = nodeset->nodeTab[i]->children;child != NULL;child = child->next){
                    //skip the carriage return (text node named text and containing /n)
                    if(child->type == XML_TEXT_NODE) continue;

                    if(QString((char*)child->name) == CHANNEL){
                        xmlChar* sId = xmlNodeListGetString(doc,child->children, 1);
                        int channelId = QString((char*)sId).toInt();
                        xmlFree(sId);
                        channelList.append(channelId);
                        spikeChannelsGroups.insert(channelId,i + 1);//overwrite the entry for the spike trash group (-1)
                        //remove the channel from the spike trash list as it is part of a group
                        spikeTrashList.remove(channelId);
                    }
                }
                spikeGroupsChannels.insert(i + 1,channelList);
            }
        }
    }

    if(spikeTrashList.size() != 0) spikeGroupsChannels.insert(-1,spikeTrashList);

    xmlFree(searchPath);
    xmlXPathFreeObject(result);
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

    xmlXPathObjectPtr result;
    xmlChar* searchPath = xmlCharStrdup("//" + ANATOMY + "/" + CHANNEL_GROUPS + "/" + GROUP);

    //Evaluate xpath expression
    result = xmlXPathEvalExpression(searchPath,xpathContex);
    if(result != NULL){
        xmlNodeSetPtr nodeset = result->nodesetval;
        if(!xmlXPathNodeSetIsEmpty(nodeset)){
            //loop on all the GROUP.
            int nbGroups = nodeset->nodeNr;
            for(int i = 0; i < nbGroups; ++i){
                QList<int> channelList;
                xmlNodePtr child;
                for(child = nodeset->nodeTab[i]->children;child != NULL;child = child->next){
                    //skip the carriage return (text node named text and containing /n)
                    if(child->type == XML_TEXT_NODE) continue;

                    if(QString((char*)child->name) == CHANNEL){
                        xmlChar* sId = xmlNodeListGetString(doc,child->children, 1);
                        int channelId = QString((char*)sId).toInt();
                        xmlFree(sId);
                        channelList.append(channelId);
                        displayChannelsGroups.insert(channelId,i + 1);//overwrite the entry for the trash group (0)
                        //remove the channel from the trash list as it is part of a group
                        trashList.remove(channelId);

                        //Look up for the SKIP attribute
                        xmlChar* skipTag = xmlCharStrdup(SKIP);
                        xmlChar* sSkip = xmlGetProp(child,skipTag);
                        if(sSkip != NULL){
                            skipStatus.insert(channelId,QString((char*)sSkip).toInt());
                        }
                        xmlFree(skipTag);
                        xmlFree(sSkip);
                    }
                }
                displayGroupsChannels.insert(i + 1,channelList);
            }
        }
    }

    if(trashList.size() != 0) displayGroupsChannels.insert(0,trashList);

    xmlFree(searchPath);
    xmlXPathFreeObject(result);
}

int NeuroscopeXmlReader::getNbSamples()const{
    int nbSamples = 0;
    xmlXPathObjectPtr result;
    xmlChar* searchPath;
    //The tag has change of location, it was inside the SPIKE element, it is now inside the NEUROSCOPE/SPIKES element.
    if(type == SESSION && (readVersion == "" || readVersion == "1.2.2")) searchPath = xmlCharStrdup("//" + SPIKE + "/" + NB_SAMPLES);
    else searchPath = xmlCharStrdup("//" + NEUROSCOPE + "/" + SPIKES + "/" + NB_SAMPLES);


    //Evaluate xpath expression
    result = xmlXPathEvalExpression(searchPath,xpathContex);
    if(result != NULL){
        xmlNodeSetPtr nodeset = result->nodesetval;
        if(!xmlXPathNodeSetIsEmpty(nodeset)){
            //Should be only one nbSamples element, so take the first one.
            xmlChar* sNbSamples = xmlNodeListGetString(doc,nodeset->nodeTab[0]->children, 1);
            nbSamples = QString((char*)sNbSamples).toInt();
            xmlFree(sNbSamples);
        }
    }

    xmlFree(searchPath);
    xmlXPathFreeObject(result);
    return nbSamples;
}

float NeuroscopeXmlReader::getWaveformLength()const{
    float waveformLength = 0;
    xmlXPathObjectPtr result;
    xmlChar* searchPath = xmlCharStrdup("//" + NEUROSCOPE + "/" + SPIKES + "/" + WAVEFORM_LENGTH);


    //Evaluate xpath expression
    result = xmlXPathEvalExpression(searchPath,xpathContex);
    if(result != NULL){
        xmlNodeSetPtr nodeset = result->nodesetval;
        if(!xmlXPathNodeSetIsEmpty(nodeset)){
            //Should be only one nbSamples element, so take the first one.
            xmlChar* sWaveformLength = xmlNodeListGetString(doc,nodeset->nodeTab[0]->children, 1);
            waveformLength = QString((char*)sWaveformLength).toInt();
            xmlFree(sWaveformLength);
        }
    }

    xmlFree(searchPath);
    xmlXPathFreeObject(result);

    return waveformLength;
}


int NeuroscopeXmlReader::getPeakSampleIndex()const{
    int index = 0;
    xmlXPathObjectPtr result;
    xmlChar* searchPath;
    //The tag has change of location, it was inside the SPIKE element, it is now inside the NEUROSCOPE/SPIKES element.
    if(type == SESSION && (readVersion == "" || readVersion == "1.2.2")) searchPath = xmlCharStrdup("//" + SPIKE + "/" + PEAK_SAMPLE_INDEX);
    else searchPath = xmlCharStrdup("//" + NEUROSCOPE + "/" + SPIKES + "/" + PEAK_SAMPLE_INDEX);


    //Evaluate xpath expression
    result = xmlXPathEvalExpression(searchPath,xpathContex);
    if(result != NULL){
        xmlNodeSetPtr nodeset = result->nodesetval;
        if(!xmlXPathNodeSetIsEmpty(nodeset)){
            //Should be only one index element, so take the first one.
            xmlChar* sindex = xmlNodeListGetString(doc,nodeset->nodeTab[0]->children, 1);
            index = QString((char*)sindex).toInt();
            xmlFree(sindex);
        }
    }

    xmlFree(searchPath);
    xmlXPathFreeObject(result);
    return index;
}

float NeuroscopeXmlReader::getPeakSampleLength()const{
    float indexLength = 0;
    xmlXPathObjectPtr result;
    xmlChar* searchPath = xmlCharStrdup("//" + NEUROSCOPE + "/" + SPIKES + "/" + PEAK_SAMPLE_LENGTH);


    //Evaluate xpath expression
    result = xmlXPathEvalExpression(searchPath,xpathContex);
    if(result != NULL){
        xmlNodeSetPtr nodeset = result->nodesetval;
        if(!xmlXPathNodeSetIsEmpty(nodeset)){
            //Should be only one index element, so take the first one.
            xmlChar* sIndexLength = xmlNodeListGetString(doc,nodeset->nodeTab[0]->children, 1);
            indexLength = QString((char*)sIndexLength).toInt();
            xmlFree(sIndexLength);
        }
    }

    xmlFree(searchPath);
    xmlXPathFreeObject(result);

    return indexLength;
}


int NeuroscopeXmlReader::getVideoWidth()const{
    int width = 0;
    xmlXPathObjectPtr result;
    xmlChar* searchPath = xmlCharStrdup("//" + VIDEO + "/" + WIDTH);

    //Evaluate xpath expression
    result = xmlXPathEvalExpression(searchPath,xpathContex);
    if(result != NULL){
        xmlNodeSetPtr nodeset = result->nodesetval;
        if(!xmlXPathNodeSetIsEmpty(nodeset)){
            //Should be only one width element, so take the first one.
            xmlChar* sWidth = xmlNodeListGetString(doc,nodeset->nodeTab[0]->children, 1);
            width = QString((char*)sWidth).toInt();
            xmlFree(sWidth);
        }
    }

    xmlFree(searchPath);
    xmlXPathFreeObject(result);
    return width;
}


int NeuroscopeXmlReader::getVideoHeight()const{
    int height = 0;
    xmlXPathObjectPtr result;
    xmlChar* searchPath = xmlCharStrdup("//" + VIDEO + "/" + HEIGHT);

    //Evaluate xpath expression
    result = xmlXPathEvalExpression(searchPath,xpathContex);
    if(result != NULL){
        xmlNodeSetPtr nodeset = result->nodesetval;
        if(!xmlXPathNodeSetIsEmpty(nodeset)){
            //Should be only one height element, so take the first one.
            xmlChar* sHeight = xmlNodeListGetString(doc,nodeset->nodeTab[0]->children, 1);
            height = QString((char*)sHeight).toInt();
            xmlFree(sHeight);
        }
    }

    xmlFree(searchPath);
    xmlXPathFreeObject(result);
    return height;
}


int NeuroscopeXmlReader::getRotation()const{
    int angle = 0;
    xmlXPathObjectPtr result;
    xmlChar* searchPath = xmlCharStrdup("//" + VIDEO + "/" + ROTATE);

    //Evaluate xpath expression
    result = xmlXPathEvalExpression(searchPath,xpathContex);
    if(result != NULL){
        xmlNodeSetPtr nodeset = result->nodesetval;
        if(!xmlXPathNodeSetIsEmpty(nodeset)){
            //Should be only one rotation element, so take the first one.
            xmlChar* sAngle = xmlNodeListGetString(doc,nodeset->nodeTab[0]->children, 1);
            angle = QString((char*)sAngle).toInt();
            xmlFree(sAngle);
        }
    }

    xmlFree(searchPath);
    xmlXPathFreeObject(result);
    return angle;
}


int NeuroscopeXmlReader::getFlip()const{
    int orientation = 0;
    xmlXPathObjectPtr result;
    xmlChar* searchPath = xmlCharStrdup("//" + VIDEO + "/" + FLIP);

    //Evaluate xpath expression
    result = xmlXPathEvalExpression(searchPath,xpathContex);
    if(result != NULL){
        xmlNodeSetPtr nodeset = result->nodesetval;
        if(!xmlXPathNodeSetIsEmpty(nodeset)){
            //Should be only one flip element, so take the first one.
            xmlChar* sOrientation = xmlNodeListGetString(doc,nodeset->nodeTab[0]->children, 1);
            orientation = QString((char*)sOrientation).toInt();
            xmlFree(sOrientation);
        }
    }

    xmlFree(searchPath);
    xmlXPathFreeObject(result);
    return orientation;
}

int NeuroscopeXmlReader::getTrajectory()const{
    int drawTrajectory = false;
    xmlXPathObjectPtr result;
    xmlChar* searchPath = xmlCharStrdup("//" + VIDEO + "/" + POSITIONS_BACKGROUND);

    //Evaluate xpath expression
    result = xmlXPathEvalExpression(searchPath,xpathContex);
    if(result != NULL){
        xmlNodeSetPtr nodeset = result->nodesetval;
        if(!xmlXPathNodeSetIsEmpty(nodeset)){
            //Should be only one POSITIONS_BACKGROUND element, so take the first one.
            xmlChar* sTrajectory = xmlNodeListGetString(doc,nodeset->nodeTab[0]->children, 1);
            drawTrajectory = QString((char*)sTrajectory).toInt();
            xmlFree(sTrajectory);
        }
    }

    xmlFree(searchPath);
    xmlXPathFreeObject(result);
    return drawTrajectory;
}  

QString NeuroscopeXmlReader::getBackgroundImage()const{
    QString backgroundPath = "-";
    xmlXPathObjectPtr result;
    xmlChar* searchPath = xmlCharStrdup("//" + VIDEO + "/" + VIDEO_IMAGE);

    //Evaluate xpath expression
    result = xmlXPathEvalExpression(searchPath,xpathContex);
    if(result != NULL){
        xmlNodeSetPtr nodeset = result->nodesetval;
        if(!xmlXPathNodeSetIsEmpty(nodeset)){
            //Should be only one VIDEO_IMAGE element, so take the first one.
            xmlChar* sBackgroundPath = xmlNodeListGetString(doc,nodeset->nodeTab[0]->children, 1);
            backgroundPath = QString((char*)sBackgroundPath);
            xmlFree(sBackgroundPath);
        }
    }

    xmlFree(searchPath);
    xmlXPathFreeObject(result);

    return backgroundPath;
}  

QString NeuroscopeXmlReader::getTraceBackgroundImage()const{
    QString traceBackgroundPath = "-";
    xmlXPathObjectPtr result;
    xmlChar* searchPath = xmlCharStrdup("//" + MISCELLANEOUS + "/" + TRACE_BACKGROUND_IMAGE);

    //Evaluate xpath expression
    result = xmlXPathEvalExpression(searchPath,xpathContex);
    if(result != NULL){
        xmlNodeSetPtr nodeset = result->nodesetval;
        if(!xmlXPathNodeSetIsEmpty(nodeset)){
            //Should be only one TRACE_BACKGROUND_IMAGE element, so take the first one.
            xmlChar* sTraceBackgroundPath = xmlNodeListGetString(doc,nodeset->nodeTab[0]->children, 1);
            traceBackgroundPath = QString((char*)sTraceBackgroundPath);
            xmlFree(sTraceBackgroundPath);
        }
    }

    xmlFree(searchPath);
    xmlXPathFreeObject(result);

    return traceBackgroundPath;
} 

QList<SessionFile> NeuroscopeXmlReader::getFilesToLoad(){
    QList<SessionFile> list;
    xmlXPathObjectPtr result;
    xmlChar* searchPath = xmlCharStrdup("/" + NEUROSCOPE + "/" + FILES + "/" + neuroscope::FILE);

    //Evaluate xpath expression
    result = xmlXPathEvalExpression(searchPath,xpathContex);
    if(result != NULL){
        xmlNodeSetPtr nodeset = result->nodesetval;
        if(!xmlXPathNodeSetIsEmpty(nodeset)){
            //loop on all the FILE.
            int nbFiles = nodeset->nodeNr;
            for(int i = 0; i < nbFiles; ++i){
                SessionFile sessionFile;
                xmlNodePtr child;
                for(child = nodeset->nodeTab[i]->children;child != NULL;child = child->next){
                    //skip the carriage return (text node named text and containing /n)
                    if(child->type == XML_TEXT_NODE) continue;

                    if(QString((char*)child->name) == TYPE){
                        xmlChar* sType = xmlNodeListGetString(doc,child->children, 1);
                        int type = QString((char*)sType).toInt();
                        xmlFree(sType);
                        sessionFile.setType(static_cast<SessionFile::type>(type)) ;
                    }
                    if(QString((char*)child->name) == URL){
                        xmlChar* sUrl = xmlNodeListGetString(doc,child->children, 1);
                        QString url = QString((char*)sUrl);
                        xmlFree(sUrl);
                        sessionFile.setUrl(QString(url));
                    }
                    if(QString((char*)child->name) == DATE){
                        xmlChar* sDate = xmlNodeListGetString(doc,child->children, 1);
                        QString date = QString((char*)sDate);
                        xmlFree(sDate);
                        sessionFile.setModification(QDateTime::fromString(date,Qt::ISODate));
                    }
                    if(QString((char*)child->name) == ITEMS){
                        //loop on the items
                        xmlNodePtr items;
                        for(items = child->children;items != NULL;items = items->next){
                            //skip the carriage return (text node named text and containing /n)
                            if(items->type == XML_TEXT_NODE) continue;

                            //loop on the elements in each itemDescription tag (item and color)
                            xmlNodePtr itemDescription;
                            QString id;
                            QString color;
                            for(itemDescription = items->children;itemDescription != NULL;itemDescription = itemDescription->next){
                                //skip the carriage return (text node named text and containing /n)
                                if(itemDescription->type == XML_TEXT_NODE) continue;

                                if(QString((char*)itemDescription->name) == ITEM){
                                    xmlChar* sId = xmlNodeListGetString(doc,itemDescription->children, 1);
                                    id = QString((char*)sId);
                                    xmlFree(sId);
                                }
                                if(QString((char*)itemDescription->name) == COLOR){
                                    xmlChar* sColor = xmlNodeListGetString(doc,itemDescription->children, 1);
                                    color = QString((char*)sColor);
                                    xmlFree(sColor);
                                }
                            }
                            sessionFile.setItemColor(id,color);
                        }
                    }
                    //obsolet, here for backware compatibility
                    if(QString((char*)child->name) == VIDEO_IMAGE){
                        xmlChar* sBackgroundPath = xmlNodeListGetString(doc,child->children, 1);
                        QString backgroundPath = QString((char*)sBackgroundPath);
                        xmlFree(sBackgroundPath);
                        sessionFile.setBackgroundPath(backgroundPath);
                    }
                }
                list.append(sessionFile);
            }
        }
    }

    xmlFree(searchPath);
    xmlXPathFreeObject(result);
    return list;

}


QList<DisplayInformation> NeuroscopeXmlReader::getDisplayInformation(){
    QList<DisplayInformation> list;
    xmlXPathObjectPtr result;
    xmlChar* searchPath = xmlCharStrdup("/" + NEUROSCOPE + "/" + DISPLAYS + "/" + DISPLAY);

    //Evaluate xpath expression
    result = xmlXPathEvalExpression(searchPath,xpathContex);
    if(result != NULL){

        xmlNodeSetPtr nodeset = result->nodesetval;
        if(!xmlXPathNodeSetIsEmpty(nodeset)){
            //loop on all the DISPLAY.
            int nbDisplays = nodeset->nodeNr;

            for(int i = 0; i < nbDisplays; ++i){
                DisplayInformation displayInformation;
                xmlNodePtr child;
                for(child = nodeset->nodeTab[i]->children;child != NULL;child = child->next){
                    //skip the carriage return (text node named text and containing /n)
                    if(child->type == XML_TEXT_NODE) continue;

                    if(QString((char*)child->name) == TAB_LABEL){
                        xmlChar* sLabel = xmlNodeListGetString(doc,child->children, 1);
                        QString label = QString((char*)sLabel);
                        xmlFree(sLabel);
                        displayInformation.setTabLabel(label);
                    }

                    if(QString((char*)child->name) == SHOW_LABELS){
                        xmlChar* sShowLabels = xmlNodeListGetString(doc,child->children, 1);
                        int showLabels = QString((char*)sShowLabels).toInt();
                        xmlFree(sShowLabels);
                        displayInformation.setLabelStatus(showLabels);
                    }

                    if(QString((char*)child->name) == START_TIME){
                        xmlChar* sStartTime = xmlNodeListGetString(doc,child->children, 1);
                        long startTime = QString((char*)sStartTime).toLong();
                        xmlFree(sStartTime);
                        displayInformation.setStartTime(startTime);
                    }

                    if(QString((char*)child->name) == DURATION){
                        xmlChar* sDuration = xmlNodeListGetString(doc,child->children, 1);
                        long duration = QString((char*)sDuration).toLong();
                        xmlFree(sDuration);
                        displayInformation.setTimeWindow(duration);
                    }

                    if(QString((char*)child->name) == MULTIPLE_COLUMNS){
                        xmlChar* sPresentation = xmlNodeListGetString(doc,child->children, 1);
                        int presentationMode = QString((char*)sPresentation).toInt();
                        xmlFree(sPresentation);
                        displayInformation.setMode(static_cast<DisplayInformation::mode>(presentationMode));
                    }

                    if(QString((char*)child->name) == GREYSCALE){
                        xmlChar* sGreyScale = xmlNodeListGetString(doc,child->children, 1);
                        int greyScale = QString((char*)sGreyScale).toInt();
                        xmlFree(sGreyScale);
                        displayInformation.setGreyScale(greyScale);
                    }

                    if(QString((char*)child->name) == POSITIONVIEW){
                        xmlChar* sPositionView = xmlNodeListGetString(doc,child->children, 1);
                        int positionView = QString((char*)sPositionView).toInt();
                        xmlFree(sPositionView);
                        displayInformation.setPositionView(positionView);
                    }

                    if(QString((char*)child->name) == SHOWEVENTS){
                        xmlChar* sShowEvents = xmlNodeListGetString(doc,child->children, 1);
                        int showEvents = QString((char*)sShowEvents).toInt();
                        xmlFree(sShowEvents);
                        displayInformation.setEventsInPositionView(showEvents);
                    }

                    if(QString((char*)child->name) == SPIKE_PRESENTATION){
                        xmlChar* sPresentation = xmlNodeListGetString(doc,child->children, 1);
                        int spikePresentation = QString((char*)sPresentation).toInt();
                        xmlFree(sPresentation);
                        displayInformation.addSpikeDisplayType(static_cast<DisplayInformation::spikeDisplayType>(spikePresentation));
                    }

                    if(QString((char*)child->name) == RASTER_HEIGHT){
                        xmlChar* sheight = xmlNodeListGetString(doc,child->children, 1);
                        int height = QString((char*)sheight).toInt();
                        xmlFree(sheight);
                        displayInformation.setRasterHeight(height);
                    }

                    if(QString((char*)child->name) == CLUSTERS_SELECTED){
                        //loop on the CLUSTERS
                        xmlNodePtr clusters;
                        QString clusterFile;
                        QList<int> clusterIds;
                        for(clusters = child->children;clusters != NULL;clusters = clusters->next){
                            //skip the carriage return (text node named text and containing /n)
                            if(clusters->type == XML_TEXT_NODE) continue;

                            if(QString((char*)clusters->name) == FILE_URL){
                                xmlChar* sUrl = xmlNodeListGetString(doc,clusters->children, 1);
                                clusterFile = QString((char*)sUrl);
                                xmlFree(sUrl);
                            }
                            if(QString((char*)clusters->name) == CLUSTER){
                                xmlChar* sClusterId = xmlNodeListGetString(doc,clusters->children, 1);
                                int clusterId = QString((char*)sClusterId).toInt();
                                xmlFree(sClusterId);
                                clusterIds.append(clusterId);
                            }
                        }
                        displayInformation.setSelectedClusters(clusterFile,clusterIds);
                    }

                    if(QString((char*)child->name) == SPIKES_SELECTED){
                        QList<QString> files;
                        //loop on the urls of the files
                        xmlNodePtr spikes;
                        for(spikes = child->children;spikes != NULL;spikes = spikes->next){
                            //skip the carriage return (text node named text and containing /n)
                            if(spikes->type == XML_TEXT_NODE) continue;

                            if(QString((char*)spikes->name) == FILE_URL){
                                xmlChar* sUrl = xmlNodeListGetString(doc,spikes->children, 1);
                                files.append(QString((char*)sUrl));
                                xmlFree(sUrl);
                            }
                        }
                        displayInformation.setSelectedSpikeFiles(files);
                    }

                    if(QString((char*)child->name) == EVENTS_SELECTED){
                        //loop on the EVENTS
                        xmlNodePtr events;
                        QString eventFile;
                        QList<int> eventIds;
                        for(events = child->children;events != NULL;events = events->next){
                            //skip the carriage return (text node named text and containing /n)
                            if(events->type == XML_TEXT_NODE) continue;

                            if(QString((char*)events->name) == FILE_URL){
                                xmlChar* sUrl = xmlNodeListGetString(doc,events->children, 1);
                                eventFile = QString((char*)sUrl);
                                xmlFree(sUrl);
                            }
                            if(QString((char*)events->name) == EVENT){
                                xmlChar* sEventId = xmlNodeListGetString(doc,events->children, 1);
                                int eventId = QString((char*)sEventId).toInt();
                                xmlFree(sEventId);
                                eventIds.append(eventId);
                            }
                        }
                        displayInformation.setSelectedEvents(eventFile,eventIds);
                    }

                    if(QString((char*)child->name) == CLUSTERS_SKIPPED){
                        //loop on the CLUSTERS
                        xmlNodePtr clusters;
                        QString clusterFile;
                        QList<int> clusterIds;
                        for(clusters = child->children;clusters != NULL;clusters = clusters->next){
                            //skip the carriage return (text node named text and containing /n)
                            if(clusters->type == XML_TEXT_NODE) continue;

                            if(QString((char*)clusters->name) == FILE_URL){
                                xmlChar* sUrl = xmlNodeListGetString(doc,clusters->children, 1);
                                clusterFile = QString((char*)sUrl);
                                xmlFree(sUrl);
                            }
                            if(QString((char*)clusters->name) == CLUSTER){
                                xmlChar* sClusterId = xmlNodeListGetString(doc,clusters->children, 1);
                                int clusterId = QString((char*)sClusterId).toInt();
                                xmlFree(sClusterId);
                                clusterIds.append(clusterId);
                            }
                        }
                        displayInformation.setSkippedClusters(clusterFile,clusterIds);
                    }

                    if(QString((char*)child->name) == EVENTS_SKIPPED){
                        //loop on the EVENTS
                        xmlNodePtr events;
                        QString eventFile;
                        QList<int> eventIds;
                        for(events = child->children;events != NULL;events = events->next){
                            //skip the carriage return (text node named text and containing /n)
                            if(events->type == XML_TEXT_NODE) continue;

                            if(QString((char*)events->name) == FILE_URL){
                                xmlChar* sUrl = xmlNodeListGetString(doc,events->children, 1);
                                eventFile = QString((char*)sUrl);
                                xmlFree(sUrl);
                            }
                            if(QString((char*)events->name) == EVENT){
                                xmlChar* sEventId = xmlNodeListGetString(doc,events->children, 1);
                                int eventId = QString((char*)sEventId).toInt();
                                xmlFree(sEventId);
                                eventIds.append(eventId);
                            }
                        }
                        displayInformation.setSkippedEvents(eventFile,eventIds);
                    }


                    if(QString((char*)child->name) == CHANNEL_POSITIONS){
                        QList<TracePosition> positions;
                        //loop on the POSITIONS
                        xmlNodePtr positionElements;
                        for(positionElements = child->children;positionElements != NULL;positionElements = positionElements->next){
                            //skip the carriage return (text node named text and containing /n)
                            if(positionElements->type == XML_TEXT_NODE) continue;

                            TracePosition tracePosition;
                            //loop on the elements in each positions tag (channelId, gain and offset)
                            xmlNodePtr positionInfo;
                            for(positionInfo = positionElements->children;positionInfo != NULL;positionInfo = positionInfo->next){
                                //skip the carriage return (text node named text and containing /n)
                                if(positionInfo->type == XML_TEXT_NODE) continue;

                                if(QString((char*)positionInfo->name) == CHANNEL){
                                    xmlChar* sId = xmlNodeListGetString(doc,positionInfo->children, 1);
                                    int channelId = QString((char*)sId).toInt();
                                    xmlFree(sId);
                                    tracePosition.setId(channelId) ;
                                }
                                if(QString((char*)positionInfo->name) == GAIN){
                                    xmlChar* sGain = xmlNodeListGetString(doc,positionInfo->children, 1);
                                    int gain = QString((char*)sGain).toInt();
                                    tracePosition.setGain(gain);
                                    xmlFree(sGain);
                                }
                                if(QString((char*)positionInfo->name) == OFFSET){
                                    xmlChar* sOffset = xmlNodeListGetString(doc,positionInfo->children, 1);
                                    int offset = QString((char*)sOffset).toInt();
                                    tracePosition.setOffset(offset);
                                    xmlFree(sOffset);
                                }
                            }
                            positions.append(tracePosition);
                        }

                        displayInformation.setPositions(positions);
                    }

                    if(QString((char*)child->name) == CHANNELS_SELECTED){
                        QList<int> channelIds;
                        //loop on the urls of the files
                        xmlNodePtr channels;
                        for(channels = child->children;channels != NULL;channels = channels->next){
                            //skip the carriage return (text node named text and containing /n)
                            if(channels->type == XML_TEXT_NODE) continue;

                            if(QString((char*)channels->name) == CHANNEL){
                                xmlChar* sId = xmlNodeListGetString(doc,channels->children, 1);
                                int channelId = QString((char*)sId).toInt();
                                xmlFree(sId);
                                channelIds.append(channelId) ;
                            }
                        }
                        displayInformation.setSelectedChannelIds(channelIds);
                    }

                    if(QString((char*)child->name) == CHANNELS_SHOWN){
                        QList<int> channelIds;
                        //loop on the urls of the files
                        xmlNodePtr channels;
                        for(channels = child->children;channels != NULL;channels = channels->next){
                            //skip the carriage return (text node named text and containing /n)
                            if(channels->type == XML_TEXT_NODE) continue;

                            if(QString((char*)channels->name) == CHANNEL){
                                xmlChar* sId = xmlNodeListGetString(doc,channels->children, 1);
                                int channelId = QString((char*)sId).toInt();
                                xmlFree(sId);
                                channelIds.append(channelId) ;
                            }
                        }
                        displayInformation.setChannelIds(channelIds);
                    }

                }
                list.append(displayInformation);
            }
        }
    }

    xmlFree(searchPath);
    xmlXPathFreeObject(result);
    return list;
}


QMap<QString,double> NeuroscopeXmlReader::getSampleRateByExtension(){
    QMap<QString,double> samplingRatesMap;
    xmlXPathObjectPtr result;
    xmlChar* searchPath;

    //The tag has change of location, it was inside the session file (at SAMPLING_RATES/EXTENSION_SAMPLING_RATE),
    //it is now inside the the parameter file (at FILES/FILE).
    if(type == SESSION && (readVersion == "" || readVersion == "1.2.2"))
        searchPath = xmlCharStrdup("/" + NEUROSCOPE + "/" + SAMPLING_RATES + "/" + EXTENSION_SAMPLING_RATE);
    else if(type == PARAMETER) searchPath = xmlCharStrdup("//" + FILES + "/" + neuroscope::FILE);

    //Evaluate xpath expression
    result = xmlXPathEvalExpression(searchPath,xpathContex);
    if(result != NULL){
        xmlNodeSetPtr nodeset = result->nodesetval;
        if(!xmlXPathNodeSetIsEmpty(nodeset)){
            //loop on all the child.
            int nbExtensions = nodeset->nodeNr;
            for(int i = 0; i < nbExtensions; ++i){
                xmlNodePtr child;
                double samplingRate;
                QString extension;
                for(child = nodeset->nodeTab[i]->children;child != NULL;child = child->next){
                    //skip the carriage return (text node named text and containing /n)
                    if(child->type == XML_TEXT_NODE) continue;

                    if(QString((char*)child->name) == EXTENSION){
                        xmlChar* sExtendion = xmlNodeListGetString(doc,child->children, 1);
                        extension = QString((char*)sExtendion);
                        xmlFree(sExtendion);
                    }
                    if(QString((char*)child->name) == SAMPLING_RATE){
                        xmlChar* sSamplingRate = xmlNodeListGetString(doc,child->children, 1);
                        samplingRate = QString((char*)sSamplingRate).toDouble();
                        xmlFree(sSamplingRate);
                    }
                }
                samplingRatesMap.insert(extension,samplingRate);
            }
        }
    }

    xmlFree(searchPath);
    xmlXPathFreeObject(result);
    return samplingRatesMap;
}



