/***************************************************************************
                          plugintracesprovider.cpp  -  description
                             -------------------
    purpose              : Traces of a stream of a file read by a plugin
    copyright            : (C) 2026 by Joscha Schmiedt
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "plugintracesprovider.h"

#include <QDebug>

#include <vector>

const int PluginTracesProvider::RESOLUTION = 16;

PluginTracesProvider::PluginTracesProvider(const std::shared_ptr<PluginFile>& file, int stream)
    : TracesProvider(file->path(), file->streams().at(stream).channelCount, RESOLUTION, 0, 0, file->streams().at(stream).samplingRate, 0),
      pluginFile(file),
      streamIndex(stream)
{
    // The base class computed the length from the size of the file.
    computeRecordingLength();
}

void PluginTracesProvider::computeRecordingLength()
{
    if (!pluginFile)
        return;
    length = static_cast<qlonglong>(static_cast<double>(streamInfo().sampleCount) * 1000.0 / samplingRate);
}

void PluginTracesProvider::retrieveData(long startTime, long endTime, QObject* initiator, long startTimeInRecordingUnits)
{
    // The samples are those of TracesProvider (.dat files): from the start time to the end time,
    // both included.
    const qint64 first = startTimeInRecordingUnits != 0 ? startTimeInRecordingUnits
                                                        : static_cast<qint64>(startTime * (samplingRate / 1000.0));
    const qint64 count = getNbSamples(startTime, endTime, startTimeInRecordingUnits);

    Array<dataType> data;
    if (count <= 0)
    {
        emit dataReady(data, initiator);
        return;
    }

    std::vector<double> microvolts(static_cast<size_t>(count) * nbChannels);
    QString error;
    if (!pluginFile->read(streamIndex, first, count, microvolts.data(), &error))
    {
        qWarning() << "Cannot read" << fileName << ":" << error;
        emit dataReady(data, initiator);
        return;
    }

    data.setSize(count, nbChannels);
    for (qint64 sample = 0; sample < count; ++sample)
    {
        for (int channel = 0; channel < nbChannels; ++channel)
            data(sample + 1, channel + 1) = round(microvolts[sample * nbChannels + channel]);
    }
    emit dataReady(data, initiator);
}
