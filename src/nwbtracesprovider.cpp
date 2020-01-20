/***************************************************************************
                          nwbtracesprovider.cpp  -  description

    Processes Neurodata Without Borders data .nwb
    Adapted by Robert H. Moore (RHM) for Dr. Ben Dichter.

    Adapted from nsxtracesprovider.h:
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

#include "nwbtracesprovider.h"

#include <QFile>
#include <stdint.h>
#include "nwbreader.h"


NWBTracesProvider::NWBTracesProvider(const QString &fileName)
    : AltTracesProvider(fileName, -1, 16, 20, 1000, 0, 0)
{
    mInitialized = false;
}

NWBTracesProvider::~NWBTracesProvider() {
    //if(mInitialized)
    //    delete[] mExtensionHeaders;
}

bool NWBTracesProvider::init() {
    // Do not initialize twice
    if(mInitialized)
        return true;

    int nbChannels; int resolution; double samplingRate; int offset; long length;
    NWBReader *nwbFile = new NWBReader(this->fileName.toUtf8().constData());
    nwbFile->ReadNWBAttribs(nbChannels, resolution, samplingRate, offset, length);
    delete nwbFile;
    this->nbChannels = nbChannels;
    this->resolution = resolution;
    this->samplingRate = samplingRate;
    this->offset = 0;
    this->length = length;

    mInitialized = true;
    return true;
}

long NWBTracesProvider::getNbSamples(long start, long end, long startInRecordingUnits) {
    // Check if startInRecordingUnits was supplied, else compute it.
    if(startInRecordingUnits == 0)
        startInRecordingUnits = this->samplingRate * start / 1000.0;

    // The caller should have checked that we do not go over the end of the file.
    // The recording starts at time equals 0 and ends at length of the file minus one.
    // Therefore the sample at endInRecordingUnits is never returned.
    long endInRecordingUnits = (this->samplingRate * end / 1000.0);

    return endInRecordingUnits - startInRecordingUnits;
}

void NWBTracesProvider::retrieveData(long start, long end, QObject* initiator, long startInRecordingUnits) {
    Array<dataType> data;

    if(!mInitialized) {
        qDebug() << "no init!";
        emit dataReady(data,initiator);
        return;
    }

    // Check if startInRecordingUnits was supplied, else compute it.
    if(startInRecordingUnits == 0)
        startInRecordingUnits = this->samplingRate * start / 1000.0;
    // The caller should have checked that we do not go over the end of the file.
    // RHM believes that the caller may not have checked, so some zero padding is added when necessary.

    long endInRecordingUnits = (this->samplingRate * end / 1000.0);
    dataType nbSamples = static_cast<dataType>(endInRecordingUnits - startInRecordingUnits) + 1;

    qint64 nbValues = 0;// nbSamples * this->nbChannels;
    //data.setSize(nbValues, this->nbChannels);

    if (startInRecordingUnits+nbSamples >= this->length)
    {
        // make the array very large anyway, even if we are past the end
        nbValues = nbSamples * this->nbChannels;
        data.setSize(nbValues, this->nbChannels);
        // Now zero pad the array
        for(qint64 i = 0; i < nbValues; ++i)
            data[i] = 0;
        if (startInRecordingUnits >= this->length-1)
        {
            emit dataReady(data,initiator);
            return;
        }

        // Now shorten the sized for the HDF5 call
        nbSamples = this->length - startInRecordingUnits;
        if (nbSamples <=0)
            nbSamples=1;
        nbValues = nbSamples * this->nbChannels;
    }
    else
    {
        // We are not past the end of the file
        nbValues = nbSamples * this->nbChannels;
        data.setSize(nbValues, this->nbChannels);
    }


    qDebug() << "NWS";
    // Modified by RHM to read Neurodata Without Borders format
    NWBReader nwbr(this->fileName.toUtf8().constData());


    Array<int16_t> traceData(nbSamples,nbChannels);
    nwbr.ReadVoltageTraces(traceData, startInRecordingUnits, nbSamples, this->nbChannels);
    qDebug() << "nbSamples " << nbSamples;

    // Compute acquisition gain
    double acquisitionGain = (voltageRange * 1000000) / (pow(2.0, resolution) * amplification);

    //Apply the offset if need it and store the values in data.
    if(offset != 0){
        for(qint64 i = 0; i < nbValues; ++i)
            data[i] = round(traceData[i] - offset * acquisitionGain);
    }
    else{
        for(qint64 i = 0; i < nbValues; ++i){
            data[i] = round(traceData[i] * acquisitionGain);
        }
    }

    qDebug() << "About to emit RHM";
    //Send the information to the receiver.
    emit dataReady(data,initiator);

}

QStringList NWBTracesProvider::getLabels() {
    QStringList labels;

    // Initialize to group "0" at first, in case the groups are not listed
    for(int i = 0; i < this->nbChannels; i++) {
        labels << QString("0");
    }

    long nbSamples = 1;
    Array<short> indexData(nbSamples,this->nbChannels);
    Array<short> groupData(nbSamples,this->nbChannels);
    NWBReader nwbr(this->fileName.toUtf8().constData());
    nwbr.getVoltageGroups(indexData, groupData, this->nbChannels);

    for (int i=0; i<this->nbChannels; ++i)
    {
        //qDebug() << "index data " << i << " " << channelNb << indexData[i+1]  << "\n";
        int iIndex = indexData[i];
        int iGroup = groupData[iIndex]+1;

        //labels[iIndex] = QString::number(iGroup);

        labels[iIndex] = QString::number(i);
    }

    return labels;
}
