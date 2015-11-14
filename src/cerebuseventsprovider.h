/***************************************************************************
            cerebuseventsprovider.h  -  description
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

 #ifndef _CEREBUSEVENTSPROVIDER_H_
 #define _CEREBUSEVENTSPROVIDER_H_

#include "eventsprovider.h"
#include "cerebustraceprovider.h"

 class CerebusEventsProvider : public EventsProvider  {
     Q_OBJECT
 public:
    CerebusEventsProvider(CerebusTracesProvider* source, int samplingRate);
    ~CerebusEventsProvider();

    /**Triggers the retrieve of the events included in the time interval given by @p startTime and @p endTime.
    * @param startTime begining of the time interval from which to retrieve the data in miliseconds.
    * @param endTime end of the time interval from which to retrieve the data.
    * @param initiator instance requesting the data.
    * @param startTimeInRecordingUnits begining of the time interval from which to retrieve the data in recording units.
    */
    virtual void requestData(long startTime,long endTime,QObject* initiator,long startTimeInRecordingUnits = 0);

    /**Looks up for the first of the events included in the list @p selectedIds existing after the time @p startTime.
    * All the events included in the time interval given by @p timeFrame are retrieved. The time interval start time is
    * computed in order to have the first event found located at @p eventPosition percentage of the time interval.
    * @param startTime starting time for the look up.
    * @param timeFrame time interval for which to retrieve the data.
    * @param selectedIds list of event ids to look up for.
    * @param initiator instance requesting the data.
    */
    virtual void requestNextEventData(long startTime,long timeFrame,const QList<int> &selectedIds,QObject* initiator);

    /**Looks up for the first of the events included in the list @p selectedIds existing before the time @p endTime.
    * All the events included in the time interval given by @p timeFrame are retrieved. The time interval start time is
    * computed in order to have the first event found located at @p eventPosition percentage of the time interval.
    * @param endTime starting time for the look up.
    * @param timeFrame time interval for which to retrieve the data.
    * @param selectedIds list of event ids to look up for.
    * @param initiator instance requesting the data.
    */
    virtual void requestPreviousEventData(long endTime,long timeFrame,QList<int> selectedIds,QObject* initiator);

    /**Loads the event ids and the corresponding spike time.
    * @return an loadReturnMessage enum giving the load status
    */
    virtual int loadData();

 private:
    // The actual data source of the event data.
    CerebusTracesProvider* mDataProvider;
 };

#endif
