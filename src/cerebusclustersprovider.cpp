/***************************************************************************
            cerebusclustersprovider.cpp  -  description
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

#include "cerebusclustersprovider.h"

#include <QDebug>

// 0 = unclassified, 1 - cbMAXUNITS = actual units, 254 = artifact (unit + noise), 255 = background (noise)
const int CerebusClustersProvider::CLUSTER_COUNT = cbMAXUNITS + 3;

CerebusClustersProvider::CerebusClustersProvider(CerebusTracesProvider* source, unsigned int channel, int samplingRate) :
    ClustersProvider(QString("cerebus.%1.clu").arg(channel + 1), samplingRate, cbSdk_TICKS_PER_SECOND, 0),
    mDataProvider(source),
    mChannel(channel) {

    // Name referes to the group, which can not be zero, because zero is trash group.
    this->name = QString::number(mChannel + 1);
    this->clusterIds << 0 << 1 << 2 << 3 << 4 << 5 << 254 << 255;
}

CerebusClustersProvider::~CerebusClustersProvider() {
    // Nothing to do here
}

int CerebusClustersProvider::loadData() {
    if (mDataProvider->isInitialized())
        return OPEN_ERROR;
    else
        return OK;
}

void CerebusClustersProvider::requestData(long start, long end, QObject* initiator, long /*startTimeInRecordingUnits*/) {
	Array<dataType>* data = mDataProvider->getClusterData(mChannel, start, end);
	emit dataReady(*data, initiator, this->name);
	delete data;
}

void CerebusClustersProvider::requestNextClusterData(long startTime, long timeFrame, const QList<int> &selectedIds, QObject* initiator, long startTimeInRecordingUnits) {
    qDebug() << "requestNextClusterData(...) not supported yet.";
}

void CerebusClustersProvider::requestPreviousClusterData(long startTime, long timeFrame, QList<int> selectedIds, QObject* initiator, long startTimeInRecordingUnits) {
    qDebug() << "requestPreviousClusterData(...) not supported yet.";
}
