/***************************************************************************
            cerebuseventsprovider.cpp  -  description
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
#include <QDebug>

#include <cbsdk.h>

#include "cerebuseventsprovider.h"

CerebusEventsProvider::CerebusEventsProvider(CerebusTracesProvider* source, int samplingRate) :
    EventsProvider("cerebus.nev", samplingRate),
    mDataProvider(source) {

    EventDescription digital("Digital Event");
    this->eventIds.insert(digital, MAX_CHANS_DIGITAL_IN);
    this->idsDescriptions.insert(MAX_CHANS_DIGITAL_IN, digital);
    this->eventDescriptionCounter.insert(digital, 1);

    EventDescription serial("Serial Event");
    this->eventIds.insert(serial, MAX_CHANS_SERIAL);
    this->idsDescriptions.insert(MAX_CHANS_SERIAL, serial);
    this->eventDescriptionCounter.insert(serial, 1);

    descriptionLength = 13;
}

CerebusEventsProvider::~CerebusEventsProvider() {
    // Nothing to do here
 }

void CerebusEventsProvider::requestData(long start, long end, QObject* initiator, long /*startTimeInRecordingUnits*/) {
    Array<dataType>* data = mDataProvider->getEventData(start, end);

    // Split data up into two arrays
    long count = data->nbOfColumns();

    Array<dataType> times(1, count);
    memcpy(&times[0], &(*data)[0], count * sizeof(dataType));

    Array<int> ids(1, count);
    for(long i = 0; i < count; i++)
        ids[i] = (*data)[count + i];

    emit dataReady(times, ids, initiator, this->name);
	delete data;
}


void CerebusEventsProvider::requestNextEventData(long startTime, long timeFrame, const QList<int> &selectedIds, QObject* initiator) {
     qCritical() << "requestNextEventData(...) not supported yet.";
 }

void CerebusEventsProvider::requestPreviousEventData(long endTime, long timeFrame, QList<int> selectedIds, QObject* initiator) {
     qCritical() << "requestPreviousEventData(...) not supported yet.";
 }

int CerebusEventsProvider::loadData() {
    if (mDataProvider->isInitialized())
         return OPEN_ERROR;
    else
         return OK;
 }
