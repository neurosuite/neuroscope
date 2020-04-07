/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#include <QtDebug>
#include "nwbeventsprovider.h"
#include "nwbreader.h"

NWBEventsProvider::NWBEventsProvider(const QString &fileUrl, int position, int iNWBIndex)
    : EventsProvider(".nw"+ QString::number(iNWBIndex) +".evt", 0, position){
    realFileName = fileUrl;
    this->iNWBIndex = iNWBIndex;
}

NWBEventsProvider::~NWBEventsProvider() {

}

int NWBEventsProvider::loadData(){
    // Empty previous data
    events.setSize(0,0);
    timeStamps.setSize(0,0);

    NWBReader nwbr(realFileName.toUtf8().constData());
    NamedArray<double> *nwbEvents = nwbr.ReadEvents(iNWBIndex);
    if (!nwbEvents)
        return OPEN_ERROR; // INCORRECT_CONTENT;

    this->currentSamplingRate = nwbr.getSamplingRate();
    this->nbEvents = nwbEvents->length();

    // Extension header information should be extracted here. Right now we do not use the information.

    // Read data packages

    Array<double> tempTimestamps(1, this->nbEvents);
    pArray<EventDescription> tempDescription; //pArray is missing a constructor
    tempDescription.setSize(1, this->nbEvents);
    long eventIndex(0L);
    long eventsSkipped(0L);

    // Create proper label
    EventDescription label = EventDescription(QString::fromUtf8(nwbEvents->strName.c_str()));
    for(eventIndex=0; eventIndex < this->nbEvents; ++eventIndex) {

        // Save time and label of event
        tempTimestamps[eventIndex]  = nwbEvents->arrayData[eventIndex];
        tempDescription[eventIndex] = label;

        // Update event category count
        long eventCount = eventDescriptionCounter.contains(label) ? eventDescriptionCounter[label]: 0;
        eventDescriptionCounter.insert(label, eventCount + 1);
    }

    // Now we know how many events were skipped, we can update the object
    this->nbEvents -= eventsSkipped;

    timeStamps.setSize(1, this->nbEvents);
    events.setSize(1, this->nbEvents);

    timeStamps.copySubset(tempTimestamps, this->nbEvents);
    events.copySubset(tempDescription, this->nbEvents);

    // Update internal struture
    this->updateMappingAndDescriptionLength();

    //for (int jj=1; jj <= this->nbEvents; ++jj)
    //    qDebug() << "time stamps " << jj << " " << this->timeStamps(1, jj) << " ";


    //Initialize the variables
    previousStartTime = 0;
    previousStartIndex = 1;
    previousEndIndex = this->nbEvents;

    previousEndTime = static_cast<long>(floor(0.5 + this->timeStamps(1, this->nbEvents)));
    fileMaxTime =  previousEndTime;

    return OK;
}
