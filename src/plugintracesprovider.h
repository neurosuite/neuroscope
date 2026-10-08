/***************************************************************************
                          plugintracesprovider.h  -  description
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

#ifndef PLUGINTRACESPROVIDER_H
#define PLUGINTRACESPROVIDER_H

#include "pluginregistry.h"
#include "tracesprovider.h"

#include <memory>

/** Provides the traces of one stream of a file read by a file format plugin. The file describes
  * itself: the number of channels, the sampling rate and the calibration cannot be changed. */
class PluginTracesProvider : public TracesProvider
{
    Q_OBJECT
  public:
    PluginTracesProvider(const std::shared_ptr<PluginFile>& file, int stream);

    std::shared_ptr<PluginFile> file() const { return pluginFile; }
    int stream() const { return streamIndex; }
    PluginFile::Stream streamInfo() const { return pluginFile->streams().at(streamIndex); }

    void setNbChannels(int) override {}
    void setResolution(int) override {}
    void setSamplingRate(double) override {}
    void setVoltageRange(int) override {}
    void setAmplification(int) override {}

    QStringList getLabels() override { return pluginFile->channelLabels(streamIndex); }

  protected:
    void retrieveData(long startTime, long endTime, QObject* initiator, long startTimeInRecordingUnits) override;
    void computeRecordingLength() override;

  private:
    /** The resolution reported to NeuroScope; the values are already in µV. */
    static const int RESOLUTION;

    std::shared_ptr<PluginFile> pluginFile;
    int streamIndex;
};

#endif
