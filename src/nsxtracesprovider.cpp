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

#include "nsxtracesprovider.h"

#include <QFile>
#include <stdint.h>


const int NSXTracesProvider::NSX_RESOLUTION = 16;
const int NSXTracesProvider::NSX_OFFSET = 0;

NSXTracesProvider::NSXTracesProvider(const QString &fileName)
    : TracesProvider(fileName, -1, NSX_RESOLUTION, 0, 0, 0, NSX_OFFSET), initialized(false) {
}

NSXTracesProvider::~NSXTracesProvider() {
    if(this->initialized)
        delete[] this->extensionHeaders;
}

bool NSXTracesProvider::init() {
    // Do not initialize twice
    if(this->initialized)
        return true;

    // Try to open file
    QFile dataFile(this->fileName);
    if(!dataFile.open(QIODevice::ReadOnly)) {
        return false;
    }

    // Read basic header
    qint64 bytesToRead = sizeof(NSXBasicHeader);
    qint64 bytesRead = dataFile.read(reinterpret_cast<char*>(&(this->basicHeader)), bytesToRead);

    if(bytesToRead != bytesRead) {
        dataFile.close();
        return false;
    }

    // Parse basic header
    this->nbChannels = this->basicHeader.channel_count;
    this->samplingRate = 30000.0 / this->basicHeader.sampling_period;

    // Read extension headers
    this->extensionHeaders = new NSXExtensionHeader[this->nbChannels];

    bytesToRead = sizeof(NSXExtensionHeader);
    for(int channel = 0; channel < this->nbChannels; channel++) {
        bytesRead = dataFile.read(reinterpret_cast<char*>(this->extensionHeaders + channel), bytesToRead);

        if(bytesToRead != bytesRead) {
            delete[] this->extensionHeaders;
            dataFile.close();
            return false;
        }
    }

    // Read data header
    bytesToRead = sizeof(NSXDataHeader);
    bytesRead = dataFile.read(reinterpret_cast<char *>(&this->dataHeader), bytesToRead);

    if(bytesToRead != bytesRead) {
        delete[] this->extensionHeaders;
        dataFile.close();
        return false;
    }

    // Save beginning of data block
    this->dataFilePos = dataFile.pos();

    // Close file and update recording length
    dataFile.close();
    this->initialized = true;
    this->computeRecordingLength();
    return true;
}

long NSXTracesProvider::getNbSamples(long start, long end, long startInRecordingUnits) {
    // Check if startInRecordingUnits was supplied, else compute it.
    if(startInRecordingUnits == 0)
        startInRecordingUnits = this->samplingRate * start / 1000.0;

    long fileOffset = startInRecordingUnits * this->nbChannels * sizeof(int16_t);

    // The caller should have check that we do not go over the end of the file.
    // The recording starts at time equals 0 and ends at length of the file minus one.
    // Therefore the sample at endInRecordingUnits is never returned.
    long endInRecordingUnits = (this->samplingRate * end / 1000.0);

    return endInRecordingUnits - startInRecordingUnits;
}

void NSXTracesProvider::retrieveData(long start, long end, QObject* initiator, long startInRecordingUnits) {
    Array<dataType> data;

    if(!this->initialized) {
        qDebug() << "no init!";
        emit dataReady(data,initiator);
        return;
    }

    QFile dataFile(this->fileName);
    if(!dataFile.open(QIODevice::ReadOnly)) {
        qDebug() << "cant open!";
        emit dataReady(data,initiator);
        return;
    }

    // Check if startInRecordingUnits was supplied, else compute it.
    if(startInRecordingUnits == 0)
        startInRecordingUnits = this->samplingRate * start / 1000.0;

    long fileOffset = startInRecordingUnits * this->nbChannels * sizeof(int16_t);

    // The caller should have check that we do not go over the end of the file.
    // The recording starts at time equals 0 and ends at length of the file minus one.
    // Therefore the sample at endInRecordingUnits is never returned.
    long endInRecordingUnits = (this->samplingRate * end / 1000.0);

    long lengthInRecordingUnits = endInRecordingUnits - startInRecordingUnits;

    // Jump to position to read (skipping headers)
    if(!dataFile.seek(this->dataFilePos + fileOffset)) {
        qDebug() << "cant skip!";
        dataFile.close();
        emit dataReady(data,initiator);
        return;
    }

    // Allocate buffer
    Array<int16_t> buffer(lengthInRecordingUnits, this->nbChannels);

    // Read data
    qint64 bytesToRead = lengthInRecordingUnits * this->nbChannels * sizeof(int16_t);
    qint64 bytesRead = dataFile.read(reinterpret_cast<char*>(&buffer[0]), bytesToRead);

    if(bytesToRead != bytesRead) {
        qDebug() << "cant read " << bytesRead  << " not " << bytesToRead;
        dataFile.close();
        emit dataReady(data,initiator);
        return;
    }
    dataFile.close();

    // Copy data to dataType array after translating them to uV
    data.setSize(lengthInRecordingUnits, this->nbChannels);
    for(int channel = 0; channel < this->nbChannels; channel++) {
        // Determine unit data is saved in
        int unit_correction = 0;
        if(!strncmp(this->extensionHeaders[channel].unit, "uV", 16)) {
            unit_correction = 1;
        } else if(!strncmp(this->extensionHeaders[channel].unit, "mV", 16)) {
            unit_correction = 1000;
        } else {
            qDebug() << "unknown unit: " << this->extensionHeaders[channel].unit;
            dataFile.close();
            data.setSize(0, 0);
            emit dataReady(data, initiator);
            return;
        }

        // Get all the values needed to translate measurement unit to uV
        int min_digital =  this->extensionHeaders[channel].min_digital_value;
        int range_digital =  this->extensionHeaders[channel].max_digital_value - min_digital;
        int min_analog =  this->extensionHeaders[channel].min_analog_value;
        int range_analog =  this->extensionHeaders[channel].max_analog_value - min_analog;

        // Convert data
        for(int i = 1; i <= lengthInRecordingUnits; i++) {
            data(i, channel + 1) = static_cast<dataType>((((static_cast<double>(buffer(i, channel + 1)) - min_digital) / range_digital) * range_analog + min_analog) * unit_correction);
        }

    }

    //Send the information to the receiver.
    emit dataReady(data, initiator);
}

void NSXTracesProvider::computeRecordingLength(){
    // We don't know the length if not initialized
    if(!this->initialized) {
        this->length = -1;
        return;
    }

    this->length = (1000.0 * this->dataHeader.length) / this->samplingRate;
}
