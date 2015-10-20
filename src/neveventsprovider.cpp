/***************************************************************************
                          nsxtracesprovider.h  -  description
                             -------------------
    copyright            : (C) 2015 by Florian Franzen
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "neveventsprovider.h"

#include <QtDebug>

NEVEventsProvider::NEVEventsProvider(const QString &fileUrl, int position) : EventsProvider(".nev.evt", 0, position), mExtensionHeaders(NULL){
    this->fileName = fileUrl;
}

NEVEventsProvider::~NEVEventsProvider() {
    if(mExtensionHeaders)
        delete[] mExtensionHeaders;
}

int NEVEventsProvider::loadData(){
    // Empty previous data
    events.setSize(0,0);
    timeStamps.setSize(0,0);

     // Try to open file
    QFile eventFile(this->fileName);
    if(!eventFile.open(QIODevice::ReadOnly)) {
        return OPEN_ERROR;
    }

    // Read basic header
    qint64 bytesToRead = sizeof(NEVBasicHeader);
    qint64 bytesRead = eventFile.read(reinterpret_cast<char*>(&(mBasicHeader)), bytesToRead);

    if(bytesToRead != bytesRead) {
        eventFile.close();
        return INCORRECT_CONTENT;
    }
    this->currentSamplingRate = mBasicHeader.global_time_resolution / 1000.0;
    this->nbEvents = (eventFile.size() - mBasicHeader.header_size) / mBasicHeader.data_package_size;

    // Read extension headers
    if(mExtensionHeaders)
        delete[] mExtensionHeaders;
    mExtensionHeaders = new NEVExtensionHeader[mBasicHeader.extension_count];

    bytesToRead = sizeof(NEVExtensionHeader);
    for(int extension = 0; extension < mBasicHeader.extension_count; extension++) {
        bytesRead = eventFile.read(reinterpret_cast<char*>(mExtensionHeaders + extension), bytesToRead);

        if(bytesToRead != bytesRead) goto fail;
    }

    // Extension header information should be extracted here. Right now we do not use the information.

    // Read data packages
    NEVDataHeader dataHeader;
    timeStamps.setSize(1, this->nbEvents);
    events.setSize(1, this->nbEvents);
    for(long eventIndex = 0L; eventIndex < this->nbEvents; eventIndex++) {
        if(!readStruct<NEVDataHeader>(eventFile, dataHeader))
            goto fail;

        if(dataHeader.timestamp == 0xFFFFFFFF){
            qDebug() << "Continuation packages are not supported!";
            goto fail;
        }

        timeStamps[eventIndex] = dataHeader.timestamp;

        // Create proper label
        EventDescription label;
        switch(dataHeader.id) {
            case NEVDigitalSerialID:
                NEVDigitalSerialData digtalData;
                if(!readStruct<NEVDigitalSerialData>(eventFile, digtalData))
                    goto fail;

                if(digtalData.reason & (1 << 7))
                    label = QString("serial data");
                else
                    label = QString("digital data");

                // Skip the padding
                if(!eventFile.seek(eventFile.pos() + mBasicHeader.data_package_size
                                                   - sizeof(NEVDigitalSerialData)
                                                   - sizeof(NEVDataHeader)))
                    goto fail;
                break;
            case NEVConfigurationID:
                NEVConfigurationDataHeader configData;
                if(!readStruct<NEVConfigurationDataHeader>(eventFile, configData))
                    goto fail;

                switch(configData.type) {
                    case 0:  label = EventDescription("config change normal");    break;
                    case 1:  label = EventDescription("config change critical");  break;
                    default: label = EventDescription("config change undefined"); break;
                }

                // Skip the config change data
                if(!eventFile.seek(eventFile.pos() + mBasicHeader.data_package_size
                                                   - sizeof(NEVConfigurationDataHeader)
                                                   - sizeof(NEVDataHeader)))
                    goto fail;
                break;
            case NEVButtonID:
                NEVButtonData buttonData;
                if(!readStruct<NEVButtonData>(eventFile, buttonData))
                    goto fail;

                switch(buttonData.trigger) {
                    case 1:  label = EventDescription("button press"); break;
                    case 2:  label = EventDescription("button reset");   break;
                    default: label = EventDescription("button undefined");   break;
                }

                // Skip the padding
                if(!eventFile.seek(eventFile.pos() + mBasicHeader.data_package_size
                                                   - sizeof(NEVButtonData)
                                                   - sizeof(NEVDataHeader)))
                    goto fail;
                break;
            case NEVTrackingID:
                NEVTrackingDataHeader trackingData;
                if(!readStruct<NEVTrackingDataHeader>(eventFile, trackingData))
                    goto fail;

                label = QString("tracking (p: %1 n: %1)").arg(trackingData.parent_id).arg(trackingData.node_id);

                // Skip the tracking data points
                if(!eventFile.seek(eventFile.pos() + mBasicHeader.data_package_size
                                                   - sizeof(NEVTrackingDataHeader)
                                                   - sizeof(NEVDataHeader)))
                    goto fail;
                break;
            case NEVVideoSyncID:
                NEVVideoSyncData syncData;
                if(!readStruct<NEVVideoSyncData>(eventFile, syncData))
                    goto fail;

                label = QString("video sync (s: %1)").arg(syncData.video_source);

                // Skip the padding
                if(!eventFile.seek(eventFile.pos() + mBasicHeader.data_package_size
                                                   - sizeof(NEVVideoSyncData)
                                                   - sizeof(NEVDataHeader)))
                    goto fail;
                break;
            case NEVCommentID:
                label = EventDescription("comment");

                if(!eventFile.seek(eventFile.pos() + mBasicHeader.data_package_size
                                                   - sizeof(NEVDataHeader)))
                    goto fail;
                break;
            default:
                if(dataHeader.id > 0 && dataHeader.id < 2049) {
                    // Spike Event on channel dataHeader.id
                    NEVSpikeDataHeader spikeData;
                    if(!readStruct<NEVSpikeDataHeader>(eventFile, spikeData))
                        goto fail;

                    switch(spikeData.unit_class) {
                        case 0:   label = QString("spike unclassified (e%1)").arg(dataHeader.id);                      break;
                        case 255: label = QString("spike noise (e%1)").arg(dataHeader.id);                             break;
                        default:  label = QString("spike unit %1 (e%2)").arg(spikeData.unit_class).arg(dataHeader.id); break;
                    }

                    // Skip waveform
                    if(!eventFile.seek(eventFile.pos() + mBasicHeader.data_package_size
                                                       - sizeof(NEVSpikeDataHeader)
                                                       - sizeof(NEVDataHeader)))
                        goto fail;
                } else {
                    qDebug() << "Unknown package id:" << dataHeader.id;
                    goto fail;
                }
        }

        // Save label for event
        events[eventIndex] = label;

        // Update event category count
        long eventCount = eventDescriptionCounter.contains(label) ? eventDescriptionCounter[label]: 0;
        eventDescriptionCounter.insert(label, eventCount + 1);
    }
    eventFile.close();

    this->updateMappingAndDescriptionLength();

    //Initialize the variables
    previousStartTime = 0;
    previousStartIndex = 1;
    previousEndIndex = this->nbEvents;
    previousEndTime = static_cast<long>(floor(0.5 + this->timeStamps(1,this->nbEvents)));
    fileMaxTime = previousEndTime;

    return OK;

fail:
    delete[] mExtensionHeaders;
    mExtensionHeaders = NULL;
    events.setSize(0,0);
    timeStamps.setSize(0,0);
    return INCORRECT_CONTENT;
}
