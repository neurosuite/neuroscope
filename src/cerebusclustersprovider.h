/***************************************************************************
            cerebusclustersprovider.h  -  description
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

#ifndef _CEREBUSCLUSTERSPROVIDER_H_
#define _CEREBUSCLUSTERSPROVIDER_H_

#include "clustersprovider.h"
#include "cerebustraceprovider.h"

class CerebusClustersProvider : public ClustersProvider  {
    Q_OBJECT

public:

    CerebusClustersProvider(CerebusTracesProvider* source, unsigned int channel, int samplingRate);
    ~CerebusClustersProvider();

    /** Loads the event ids and the corresponding spike time.
     * @return an loadReturnMessage enum giving the load status
     *
     * Since data is supplied by CerebusTraceProvider, this is
     * just a wrapper around isInitialized()
     */
    virtual int loadData();


	/**Triggers the retrieve of the cluster information included in the time interval given by @p startTime and @p endTime.
	* @param startTime begining of the time interval from which to retrieve the data in miliseconds.
	* @param endTime end of the time interval from which to retrieve  the data.
	* @param initiator instance requesting the data.
	* @param startTimeInRecordingUnits begining of the time interval from which to retrieve the data in recording units.
	*/
	virtual void requestData(long startTime, long endTime, QObject* initiator, long startTimeInRecordingUnits);


	/**Looks up for the first of the clusters included in the list @p selectedIds existing after the time @p startTime.
	* All the clusters included in the time interval given by @p timeFrame are retrieved. The time interval start time is
	* computed in order to have the first cluster found located at @p clusterPosition percentage of the time interval.
	* @param startTime starting time, in miliseconds, for the look up.
	* @param timeFrame time interval for which to retrieve  the data.
	* @param selectedIds list of cluster ids to look up for.
	* @param initiator instance requesting the data.
	* @param startTimeInRecordingUnits starting time, in recording units, for the look up.
	*/
	virtual void requestNextClusterData(long startTime, long timeFrame, const QList<int> &selectedIds, QObject* initiator, long startTimeInRecordingUnits);


	/**Looks up for the first of the clusters included in the list @p selectedIds existing before the time @p endTime.
	* All the clusters included in the time interval given by @p timeFrame are retrieved. The time interval start time is
	* computed in order to have the first cluster found located at @p clusterPosition percentage of the time interval.
	* @param startTime starting time, in miliseconds, for the look up.
	* @param timeFrame time interval for which to retrieve  the data.
	* @param selectedIds list of cluster ids to look up for.
	* @param initiator instance requesting the data.
	* @param startTimeInRecordingUnits starting time, in recording units, for the look up.
	*/
	virtual void requestPreviousClusterData(long startTime, long timeFrame, QList<int> selectedIds, QObject* initiator, long startTimeInRecordingUnits);

private:
    // The actual data source of the cluster data.
    CerebusTracesProvider* mDataProvider;

    // Channel this provider is responsible for
    unsigned int mChannel;
};

#endif
