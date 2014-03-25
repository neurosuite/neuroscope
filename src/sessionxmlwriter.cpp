/***************************************************************************
                          sessionxmlwriter.cpp  -  description
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
#include "sessionxmlwriter.h"
#include "tags.h"
#include "channelcolors.h"
#include "eventsprovider.h"


#include <QTextStream>
#include <QList>

//include files for QT
#include <QFile> 
#include <QString> 

using namespace neuroscope;

SessionXmlWriter::SessionXmlWriter()
    :doc()
{
    //create the processing instruction
    QDomProcessingInstruction processingInstruction = doc.createProcessingInstruction("xml","version='1.0'");
    doc.appendChild(processingInstruction);

    //Create the document and the root element.
    root = doc.createElement(NEUROSCOPE);
    root.setAttribute(VERSION,NEUROSCOPE_VERSION);
    doc.appendChild(root);
}

SessionXmlWriter::~SessionXmlWriter(){}

bool SessionXmlWriter::writeTofile(const QString& url){ 
    QFile sessionFile(url);
    bool status = sessionFile.open(QIODevice::WriteOnly);
    if(!status)
        return status;

    root.appendChild(video);
    if(!samplingRates.isNull()) root.appendChild(samplingRates);
    root.appendChild(loadedFiles);
    root.appendChild(displays);

    QString xmlDocument = doc.toString();

    QTextStream stream(&sessionFile);
    stream.setCodec("UTF-8");
    stream<< xmlDocument;
    sessionFile.close();

    return true;
}

void SessionXmlWriter::setLoadedFilesInformation(const QList<SessionFile>& fileList){
    loadedFiles = doc.createElement(FILES);

    QList<SessionFile>::ConstIterator iterator;
    for(iterator = fileList.begin(); iterator != fileList.end(); ++iterator){
        //Get the file information
        QString fileUrl = static_cast<SessionFile>(*iterator).getUrl().toString();
        int fileType = static_cast<SessionFile>(*iterator).getType();
        QDateTime dateTime = static_cast<SessionFile>(*iterator).getModification();
        QMap<EventDescription,QColor> colors = static_cast<SessionFile>(*iterator).getItemColors();
        QString backgroundPath = static_cast<SessionFile>(*iterator).getBackgroundPath();

        QDomElement typeElement = doc.createElement(TYPE);
        QDomText typeValue = doc.createTextNode(QString::number(fileType));
        typeElement.appendChild(typeValue);

        QDomElement urlElement = doc.createElement(URL);
        QDomText urlValue = doc.createTextNode(fileUrl);
        urlElement.appendChild(urlValue);

        QDomElement dateElement = doc.createElement(DATE);
        QDomText dateValue = doc.createTextNode(dateTime.toString(Qt::ISODate));
        dateElement.appendChild(dateValue);

        QDomElement fileElement = doc.createElement(neuroscope::FILE);
        fileElement.appendChild(typeElement);
        fileElement.appendChild(urlElement);
        fileElement.appendChild(dateElement);

        //If there is no color, the file correspond to a position file with no items.
        if(!colors.isEmpty()){
            QDomElement itemsElement = doc.createElement(neuroscope::ITEMS);

            QMap<EventDescription,QColor>::Iterator iterator;
            for(iterator = colors.begin(); iterator != colors.end(); ++iterator){
                //Get the item information (id and color)
                QString id = iterator.key();
                QColor color = iterator.value();

                QDomElement idElement = doc.createElement(ITEM);
                QDomText idValue = doc.createTextNode(id);
                idElement.appendChild(idValue);

                QDomElement colorElement = doc.createElement(COLOR);
                QDomText colorValue = doc.createTextNode(color.name());
                colorElement.appendChild(colorValue);

                QDomElement itemDescription = doc.createElement(ITEM_DESCRIPTION);
                itemDescription.appendChild(idElement);
                itemDescription.appendChild(colorElement);

                itemsElement.appendChild(itemDescription);
            }
            fileElement.appendChild(itemsElement);
        }

        loadedFiles.appendChild(fileElement);
    }
}

void SessionXmlWriter::setDisplayInformation(const QList<DisplayInformation>& displayList){
    displays = doc.createElement(DISPLAYS);

    QList<DisplayInformation>::ConstIterator iterator;
    for(iterator = displayList.constBegin(); iterator != displayList.constEnd(); ++iterator){

        QDomElement displayElement = doc.createElement(neuroscope::DISPLAY);

        //Get the information store in DisplayInformation
        QString tabLabel = static_cast<DisplayInformation>(*iterator).getTabLabel();
        int showLabels = static_cast<DisplayInformation>(*iterator).getLabelStatus();
        long startTime = static_cast<DisplayInformation>(*iterator).getStartTime();
        long duration = static_cast<DisplayInformation>(*iterator).getTimeWindow();
        int presentationMode = static_cast<DisplayInformation>(*iterator).getMode();
        int rasterHeight = static_cast<DisplayInformation>(*iterator).getRasterHeight();
        int greyScale = static_cast<DisplayInformation>(*iterator).getGreyScale();
        bool autocenterChannels = static_cast<DisplayInformation>(*iterator).getAutocenterChannels();
        int positionView = static_cast<DisplayInformation>(*iterator).isAPositionView();
        int showEvents = static_cast<DisplayInformation>(*iterator).isEventsDisplayedInPositionView();
        QList<DisplayInformation::spikeDisplayType> spikeDisplayTypes = static_cast<DisplayInformation>(*iterator).getSpikeDisplayTypes();
        QMap<QString, QList<int> > selectedClusters = static_cast<DisplayInformation>(*iterator).getSelectedClusters();
        QMap<QString, QList<int> > selectedEvents = static_cast<DisplayInformation>(*iterator).getSelectedEvents();
        QStringList shownSpikeFiles = static_cast<DisplayInformation>(*iterator).getSelectedSpikeFiles();
        QMap<QString, QList<int> > skippedClusters = static_cast<DisplayInformation>(*iterator).getSkippedClusters();
        QMap<QString, QList<int> > skippedEvents = static_cast<DisplayInformation>(*iterator).getSkippedEvents();
        QList<TracePosition> positions = static_cast<DisplayInformation>(*iterator).getPositions();
        QList<int> channelIds = static_cast<DisplayInformation>(*iterator).getChannelIds();
        QList<int> selectedChannelIds = static_cast<DisplayInformation>(*iterator).getSelectedChannelIds();

        //Label when in the display is in tab mode
        QDomElement labelElement = doc.createElement(TAB_LABEL);
        QDomText labelValue = doc.createTextNode(tabLabel);
        labelElement.appendChild(labelValue);
        displayElement.appendChild(labelElement);

        //info to know if the labels next to the traces have to be shown
        QDomElement showLabelsElement = doc.createElement(SHOW_LABELS);
        QDomText showLabelsValue = doc.createTextNode(QString::number(showLabels));
        showLabelsElement.appendChild(showLabelsValue);
        displayElement.appendChild(showLabelsElement);

        //info concering the portion of the document presented
        QDomElement startTimeElement = doc.createElement(START_TIME);
        QDomText startTimeValue = doc.createTextNode(QString::number(startTime));
        startTimeElement.appendChild(startTimeValue);
        displayElement.appendChild(startTimeElement);

        QDomElement durationElement = doc.createElement(DURATION);
        QDomText durationValue = doc.createTextNode(QString::number(duration));
        durationElement.appendChild(durationValue);
        displayElement.appendChild(durationElement);

        //info on the trace presentation (single or multiple columns)
        QDomElement presentationElement = doc.createElement(MULTIPLE_COLUMNS);
        QDomText presentationValue = doc.createTextNode(QString::number(presentationMode));
        presentationElement.appendChild(presentationValue);
        displayElement.appendChild(presentationElement);

        //info on the grey-scale
        QDomElement greyScaleElement = doc.createElement(GREYSCALE);
        QDomText greyScaleValue = doc.createTextNode(QString::number(greyScale));
        greyScaleElement.appendChild(greyScaleValue);
        displayElement.appendChild(greyScaleElement);

        //info on the channel autocenter status
        QDomElement autocenterChannelsElement = doc.createElement(AUTOCENTER_CHANNELS);
        QDomText autocenterChannelsValue = doc.createTextNode(QString::number(autocenterChannels));
        autocenterChannelsElement.appendChild(autocenterChannelsValue);
        displayElement.appendChild(autocenterChannelsElement);

        //info on the PositionView
        QDomElement positionViewElement = doc.createElement(POSITIONVIEW);
        QDomText positionViewValue = doc.createTextNode(QString::number(positionView));
        positionViewElement.appendChild(positionViewValue);
        displayElement.appendChild(positionViewElement);

        //info on the display of events in the PositionView
        QDomElement showEventsElement = doc.createElement(SHOWEVENTS);
        QDomText showEventsValue = doc.createTextNode(QString::number(showEvents));
        showEventsElement.appendChild(showEventsValue);
        displayElement.appendChild(showEventsElement);

        //info on the spike presentation
        QList<DisplayInformation::spikeDisplayType>::iterator typeIterator;
        for(typeIterator = spikeDisplayTypes.begin(); typeIterator != spikeDisplayTypes.end(); ++typeIterator){
            QDomElement typeElement = doc.createElement(SPIKE_PRESENTATION);
            QDomText typeValue = doc.createTextNode(QString::number(*typeIterator));
            typeElement.appendChild(typeValue);
            displayElement.appendChild(typeElement);
        }

        //Info on the raster height
        QDomElement rasterHeightElement = doc.createElement(RASTER_HEIGHT);
        QDomText rasterHeightValue = doc.createTextNode(QString::number(rasterHeight));
        rasterHeightElement.appendChild(rasterHeightValue);
        displayElement.appendChild(rasterHeightElement);

        //Create the information concerning the selected clusters
        QMap<QString, QList<int> >::Iterator clustersIterator;
        //The iterator gives the keys sorted.
        for(clustersIterator = selectedClusters.begin(); clustersIterator != selectedClusters.end(); ++clustersIterator){
            QDomElement clustersElement = doc.createElement(CLUSTERS_SELECTED);

            //url of the cluster file
            QDomElement fileElement = doc.createElement(FILE_URL);
            QDomText fileValue = doc.createTextNode(clustersIterator.key());
            fileElement.appendChild(fileValue);
            clustersElement.appendChild(fileElement);

            //list of cluster ids
            QList<int> clustersIds = clustersIterator.value();
            QList<int>::iterator idIterator;
            for(idIterator = clustersIds.begin(); idIterator != clustersIds.end(); ++idIterator){
                QDomElement idElement = doc.createElement(CLUSTER);
                QDomText idValue = doc.createTextNode(QString::number(*idIterator));
                idElement.appendChild(idValue);
                clustersElement.appendChild(idElement);
            }
            displayElement.appendChild(clustersElement);
        }


        //Create the information concerning the selected events
        QMap<QString, QList<int> >::Iterator eventsIterator;
        //The iterator gives the keys sorted.
        for(eventsIterator = selectedEvents.begin(); eventsIterator != selectedEvents.end(); ++eventsIterator){
            QDomElement eventsElement = doc.createElement(EVENTS_SELECTED);

            //url of the event file
            QDomElement fileElement = doc.createElement(FILE_URL);
            QDomText fileValue = doc.createTextNode(eventsIterator.key());
            fileElement.appendChild(fileValue);
            eventsElement.appendChild(fileElement);

            //list of event ids
            QList<int> eventIds = eventsIterator.value();
            QList<int>::iterator idIterator;
            for(idIterator = eventIds.begin(); idIterator != eventIds.end(); ++idIterator){
                QDomElement idElement = doc.createElement(EVENT);
                QDomText idValue = doc.createTextNode(QString::number(*idIterator));
                idElement.appendChild(idValue);
                eventsElement.appendChild(idElement);
            }
            displayElement.appendChild(eventsElement);
        }

        //Create the information concerning the spike files
        QStringList::iterator spikeFileIterator;
        for(spikeFileIterator = shownSpikeFiles.begin(); spikeFileIterator != shownSpikeFiles.end(); ++spikeFileIterator){
            QDomElement selectedSpikesElement = doc.createElement(SPIKES_SELECTED);

            QDomElement fileElement = doc.createElement(FILE_URL);
            QDomText fileValue = doc.createTextNode(*spikeFileIterator);
            fileElement.appendChild(fileValue);
            selectedSpikesElement.appendChild(fileElement);
            displayElement.appendChild(selectedSpikesElement);
        }

        //Create the information concerning the skipped clusters
        //The iterator gives the keys sorted.
        for(clustersIterator = skippedClusters.begin(); clustersIterator != skippedClusters.end(); ++clustersIterator){
            QDomElement clustersElement = doc.createElement(CLUSTERS_SKIPPED);

            //url of the cluster file
            QDomElement fileElement = doc.createElement(FILE_URL);
            QDomText fileValue = doc.createTextNode(clustersIterator.key());
            fileElement.appendChild(fileValue);
            clustersElement.appendChild(fileElement);

            //list of cluster ids
            QList<int> clustersIds = clustersIterator.value();
            QList<int>::iterator idIterator;
            for(idIterator = clustersIds.begin(); idIterator != clustersIds.end(); ++idIterator){
                QDomElement idElement = doc.createElement(CLUSTER);
                QDomText idValue = doc.createTextNode(QString::number(*idIterator));
                idElement.appendChild(idValue);
                clustersElement.appendChild(idElement);
            }
            displayElement.appendChild(clustersElement);
        }

        //Create the information concerning the skipped events
        //The iterator gives the keys sorted.
        for(eventsIterator = skippedEvents.begin(); eventsIterator != skippedEvents.end(); ++eventsIterator){
            QDomElement eventsElement = doc.createElement(EVENTS_SKIPPED);

            //url of the event file
            QDomElement fileElement = doc.createElement(FILE_URL);
            QDomText fileValue = doc.createTextNode(eventsIterator.key());
            fileElement.appendChild(fileValue);
            eventsElement.appendChild(fileElement);

            //list of event ids
            QList<int> eventIds = eventsIterator.value();
            QList<int>::iterator idIterator;
            for(idIterator = eventIds.begin(); idIterator != eventIds.end(); ++idIterator){
                QDomElement idElement = doc.createElement(EVENT);
                QDomText idValue = doc.createTextNode(QString::number(*idIterator));
                idElement.appendChild(idValue);
                eventsElement.appendChild(idElement);
            }
            displayElement.appendChild(eventsElement);
        }

        //Create the information concerning the channel positions (gain and offset)
        QDomElement channelPositionsElement = doc.createElement(CHANNEL_POSITIONS);
        QList<TracePosition>::iterator positionIterator;
        for(positionIterator = positions.begin(); positionIterator != positions.end(); ++positionIterator){
            int channelId = static_cast<TracePosition>(*positionIterator).getId();
            int gain = static_cast<TracePosition>(*positionIterator).getGain();
            int offset = static_cast<TracePosition>(*positionIterator).getOffset();

            QDomElement idElement = doc.createElement(CHANNEL);
            QDomText idValue = doc.createTextNode(QString::number(channelId));
            idElement.appendChild(idValue);

            QDomElement gainsetElement = doc.createElement(GAIN);
            QDomText gainsetValue = doc.createTextNode(QString::number(gain));
            gainsetElement.appendChild(gainsetValue);

            QDomElement offsetElement = doc.createElement(OFFSET);
            QDomText offsetValue = doc.createTextNode(QString::number(offset));
            offsetElement.appendChild(offsetValue);

            QDomElement channelPositionElement = doc.createElement(CHANNEL_POSITION);
            channelPositionElement.appendChild(idElement);
            channelPositionElement.appendChild(gainsetElement);
            channelPositionElement.appendChild(offsetElement);

            channelPositionsElement.appendChild(channelPositionElement);
        }
        displayElement.appendChild(channelPositionsElement);

        //Create the information concerning the channels selected in the display
        QDomElement channelSelectedElement = doc.createElement(CHANNELS_SELECTED);
        QList<int>::iterator channelSelectedIterator;
        for(channelSelectedIterator = selectedChannelIds.begin(); channelSelectedIterator != selectedChannelIds.end(); ++channelSelectedIterator){
            QDomElement idElement = doc.createElement(CHANNEL);
            QDomText idValue = doc.createTextNode(QString::number(*channelSelectedIterator));
            idElement.appendChild(idValue);
            channelSelectedElement.appendChild(idElement);
        }
        displayElement.appendChild(channelSelectedElement);

        //Create the information concerning the channels shown in the display
        QDomElement channelsElement = doc.createElement(CHANNELS_SHOWN);
        QList<int>::iterator channelIterator;
        for(channelIterator = channelIds.begin(); channelIterator != channelIds.end(); ++channelIterator){
            QDomElement idElement = doc.createElement(CHANNEL);
            QDomText idValue = doc.createTextNode(QString::number(*channelIterator));
            idElement.appendChild(idValue);
            channelsElement.appendChild(idElement);
        }
        displayElement.appendChild(channelsElement);


        displays.appendChild(displayElement);
    }
}




