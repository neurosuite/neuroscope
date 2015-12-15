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

#include "nevclustersprovider.h"

#include <QMap>

NEVClustersProvider::NEVClustersProvider(unsigned int channel,Array<dataType>& data,
                                         int spikeCount,
                                         double samplingRate,
                                         double currentSamplingRate,
                                         dataType fileMaxTime,
                                         int position)
    :   ClustersProvider(QString("nev.%1.clu").arg(channel + 1),
                         samplingRate,
                         currentSamplingRate,
                         fileMaxTime,
                         position) {

    // Override base clase settings
    // TODO: Clean up base clases so this is no longer required.
    this->name = QString::number(channel + 1);
    this->nbSpikes = spikeCount;
    this->clusters.setSize(2, spikeCount);

    // Copy data to internal structure
    clusters.copySubset(data, this->nbSpikes);

    // Determine list of unique cluster
    for(int i = 1; i <= this->nbSpikes; i++) {
        if(!clusterIds.contains(clusters(1, i)))
            clusterIds << clusters(1, i);
    }
    this->nbClusters = clusterIds.size();

    //Initialize the variables
    this->previousStartTime = 0;
    this->previousStartIndex = 1;
    this->previousEndIndex = this->nbSpikes;
    this->previousEndTime = (clusters(2, this->nbSpikes) * 1000.0 / samplingRate) + 0.5;
    this->fileMaxTime = this->previousEndTime;
}


int NEVClustersProvider::loadData() {
    // Do nothing, this is all done by fromFile(...)
    return OK;
}

NEVClustersProvider::~NEVClustersProvider() {

}

QList<NEVClustersProvider*> NEVClustersProvider::fromFile(const QString& fileUrl,
                                                          QStringList channelLabels,
                                                          double currentSamplingRate,
                                                          dataType fileMaxTime,
                                                          int position = 25) {
    QList<NEVClustersProvider*> result;

     // Try to open file
    QFile clusterFile(fileUrl);
    if(!clusterFile.open(QIODevice::ReadOnly)) {
        return result;
    }

    // Read basic header
    NEVBasicHeader basicHeader;
    if(!readStruct<NEVBasicHeader>(clusterFile, basicHeader)) {
        clusterFile.close();
        return result;
    }

    // Extract information we need from basic header
    double samplingRate = basicHeader.global_time_resolution;
    long eventCount = (clusterFile.size() - basicHeader.header_size) / basicHeader.data_package_size;

    // Read extension headers and extract label mapping
    QMap<QString, int> channelLabelsToIds;
    NEVExtensionHeader extensionHeader;
    for(int extension = 0; extension < basicHeader.extension_count; extension++) {
        if(!readStruct<NEVExtensionHeader>(clusterFile, extensionHeader)) {
            clusterFile.close();
            return result;
        }

        // Extract all the label headers
        if(!strncmp(extensionHeader.id, NEVNeuralLabelID, 8)) {
            NEVNeuralLabelExtensionData* labelHeader = reinterpret_cast<NEVNeuralLabelExtensionData*>(extensionHeader.data);
            channelLabelsToIds.insert(QString(labelHeader->label), labelHeader->id);
        }
    }

    // Find channel ids we need to extract
    QList<int> channelIds;
    for(int i = 0; i < channelLabels.size(); i++) {
        QMap<QString, int>::iterator id = channelLabelsToIds.find(channelLabels[i]);
        if(id == channelLabelsToIds.end()) {
            // Label could not be found in label header.
            qCritical("Can not find label '%s' in nev file.", qUtf8Printable(channelLabels[i]));
            return result;
        }
        channelIds.append(id.value());
        if((++id).key() == channelLabels[i]) {
            // Only complain about duplicate labels if we are actually using them.
            qCritical("Duplicate label '%s' found in nev file. Can not determine channel id correctly.", qUtf8Printable(channelLabels[i]));
            return result;
        }
    }

    // Prepare data structure
    int channelCount = channelLabels.size();

    QList<long> spikeCount;
    // Array copy constructor is broken, so we have to use pointers.
    QList<Array<dataType>*> data;
    for(int i = 0; i < channelCount; i++) {
        data << new Array<dataType>(2, eventCount);
        spikeCount << 0;
    }

    // Read data packages
    NEVDataHeader dataHeader;
    for(long i = 0; i < eventCount; i++) {
        if(!readStruct<NEVDataHeader>(clusterFile, dataHeader))
            return result;

        if(dataHeader.timestamp == 0xFFFFFFFF){
            qCritical("Continuation packages are not supported!");
            return result;
        }
        if(dataHeader.id > 0 && dataHeader.id < 2049) {
            NEVSpikeDataHeader spikeData;
            if(!readStruct<NEVSpikeDataHeader>(clusterFile, spikeData))
                return result;

            // Check if we are interested in spikes on this channel.
            int index = channelIds.indexOf(dataHeader.id);
            if(index != -1) {
                // We are interested, so copy spike info;
                spikeCount[index]++;
                (*data[index])(1, spikeCount[index]) = spikeData.unit_class;
                (*data[index])(2, spikeCount[index]) = dataHeader.timestamp;
            }

            // Skip the rest of the data
            if(!clusterFile.seek(clusterFile.pos() + basicHeader.data_package_size
                                                   - sizeof(NEVSpikeDataHeader)
                                                   - sizeof(NEVDataHeader))) {
                return result;
            }
        } else {
            // Skip event package
            if(!clusterFile.seek(clusterFile.pos() + basicHeader.data_package_size
                                                   - sizeof(NEVDataHeader)))
                return result;
        }
    }
    clusterFile.close();

    // Create provider objects
    for(int i = 0; i < channelCount; i++) {
        result.append(new NEVClustersProvider(i,
                                              *data[i],
                                              spikeCount[i],
                                              samplingRate,
                                              currentSamplingRate,
                                              fileMaxTime,
                                              position));
    }

    // Array copy constructor is broken, so we have to use pointers.
    for(int i = 0; i < channelCount; i++) {
        delete data[i];
    }

    return result;
}
