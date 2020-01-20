/***************************************************************************
                          nwbclusterprovider.h  -  description
                             -------------------
    Written by Robert H. Moore (RHM),
    but adapted from:
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

#include <QMap>

#include "nwbclustersprovider.h"
#include "nwbreader.h"

NWBClustersProvider::NWBClustersProvider(unsigned int channel,
                                         Array<dataType>& data,
                                         int spikeCount,
                                         double samplingRate,
                                         double currentSamplingRate,
                                         dataType fileMaxTime,
                                         int position)
    :   ClustersProvider(QString("nwb.%1.clu").arg(channel + 1),
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
    // !!! check the factor of 1000 !!!
    // !!! RHM remove the -1 below
    //this->previousEndTime = (clusters(2, this->nbSpikes -1) * 1000.0 / samplingRate) + 0.5;
    this->previousEndTime = (clusters(2, this->nbSpikes ) /* *1000 */ ) + 0.5;

    this->fileMaxTime = this->previousEndTime;
}


int NWBClustersProvider::loadData() {
    // Do nothing, this is all done by fromFile(...)
    return OK;
}

NWBClustersProvider::~NWBClustersProvider() {

}

QList<NWBClustersProvider*> NWBClustersProvider::fromFile(const QString& fileUrl,
                                                          QStringList channelLabels,
                                                          double currentSamplingRate,
                                                          dataType fileMaxTime,
                                                          int position = 25) {
    QList<NWBClustersProvider*> result;

    NWBReader nwbr(fileUrl.toUtf8().constData());
    double samplingRate  = nwbr.getSamplingRate();
    QList<NamedArray<double>> NamedSpikes =nwbr.ReadSpikeShank();

    // Create provider objects
    for(int i = 0; i < NamedSpikes.length(); i++) {
        int nLen = NamedSpikes[i].length();

        Array<dataType> *data = new Array<dataType>(2, nLen);
        for (int idx=0; idx < nLen; ++idx)
        {
            (*data)(1, idx+1) = i; // spikeData.unit_class; // RHM !!! We need the proper channel here?

            // The factor of 1000 below converts the HDF5 data from seconds to milliseconds.
            // NamedSpikes[] is ZERO based. (*data) is ONE based, hence idx+1.
            (*data)(2, idx+1) = static_cast<dataType>(NamedSpikes[i].arrayData[idx] * 1000); // HDF5 data is in seconds
        }

        result.append(new NWBClustersProvider(static_cast<unsigned>(i),
                                              *data,
                                              nLen,
                                              samplingRate,
                                              currentSamplingRate,
                                              fileMaxTime,
                                              position));
    }
    return result;
}
