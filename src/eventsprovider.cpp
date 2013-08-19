/***************************************************************************
                          eventsprovider.cpp  -  description
                             -------------------
    begin                : Wed Apr 14 2004
    copyright            : (C) 2004 by Lynn Hazan
    email                : lynn.hazan.myrealbox.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
//QT include files
#include <QStringList>
#include <QFileInfo> 
#include <QRegExp>

#include <QTextStream>
#include <QList>
#include <QDebug>


//Unix include file
#include <unistd.h>

//include files for the application
#include "eventsprovider.h"
#include "timer.h"
#include "utilities.h"


EventsProvider::EventsProvider(const QString &fileUrl, double currentSamplingRate, int position): DataProvider(fileUrl),nbEvents(0),
    eventPosition(static_cast<float>(position) / 100.0),modified(false){

    this->currentSamplingRate = static_cast<double>(currentSamplingRate / 1000.0);

    //Find the event file identifier and use it as the name for the provider
    //the file name is X.id.evt (id is a 3 character identifier)
    QString fileName = fileUrl;
    const int startingIndex = fileName.lastIndexOf("evt");
    if(startingIndex == static_cast<int>(fileName.length()) - 3){//X.id.evt
        int nBStartingIndex = fileName.lastIndexOf(".",startingIndex - 2);
        name = fileName.mid(nBStartingIndex + 1,(startingIndex - 1) - (nBStartingIndex + 1));
    } else {//X.evt.id
        const int nBStartingIndex = fileName.lastIndexOf(".");
        name = fileName.right(static_cast<int>(fileName.length()) - (nBStartingIndex + 1));
    }
}

EventsProvider::~EventsProvider(){
    qDebug()<<"in ~EventsProvider "<<endl;
}

int EventsProvider::loadData(){
    RestartTimer();

    //Get the number of events
    nbEvents = Utilities::getNbLines(fileName);

    //qDebug()<<"nbEvents "<<nbEvents<<endl;

    if(nbEvents == -1){
        events.setSize(0,0);
        timeStamps.setSize(0,0);
        return COUNT_ERROR;
    }

    if(nbEvents == 0){
        initializeEmptyProvider();
        return OK;
    }

    //Create a reader on the eventFile
    QFile eventFile(fileName);
    bool status = eventFile.open(QIODevice::ReadOnly);
    if(!status){
        events.setSize(0,0);
        timeStamps.setSize(0,0);
        return OPEN_ERROR;
    }

    //Set the size of the Arrays containing the time and ids of the events.
    events.setSize(1,nbEvents);
    timeStamps.setSize(1,nbEvents);

    QTextStream fileStream(&eventFile);
    QString line;
    int lineCounter = 0;
    for(line = fileStream.readLine(); !line.isNull() && lineCounter< nbEvents;line = fileStream.readLine()){
        line = line.trimmed();

        int index1 = line.indexOf(QRegExp("\\s"));
        int index2 = line.indexOf(QRegExp("\\S"),index1);

        timeStamps[lineCounter] = line.left(index1).toDouble();
        EventDescription label = line.right(line.length() - index2);
        events[lineCounter] = label;
        if(eventDescriptionCounter.contains(label)){
            eventDescriptionCounter.insert(label,eventDescriptionCounter[label] + 1);
        }
        else eventDescriptionCounter.insert(label,1);
        lineCounter ++;
    }

    eventFile.close();
    qDebug()<< "Loading evt file into memory: "<<Timer() << endl;


    //The number of events read has to be coherent with the number of events read.
    if(lineCounter != nbEvents){
        events.setSize(0,0);
        timeStamps.setSize(0,0);
        return INCORRECT_CONTENT;
    }

    //Assign an id to each event description
    //The iterator iterates on the keys sorted
    QMap<EventDescription,int>::Iterator iterator;
    int id = 1;
    long maxSize = 0;
    long sum = 0;
    long sumOfSquares = 0;
    for(iterator = eventDescriptionCounter.begin(); iterator != eventDescriptionCounter.end(); ++iterator){
        eventIds.insert(iterator.key(),id);
        idsDescriptions.insert(id,iterator.key());
        long length = static_cast<long>(iterator.key().length());
        if(length > maxSize) maxSize = length;
        sum += length;
        sumOfSquares += (length * length);
        id++;
    }

    //Compute the length to use to display the descrption of the events in the event palette.
    // descriptionLength = min(mean + 1 * standard deviation, maxsize)
    long mean = sum / eventIds.size();
    //variance(X) = mean(X^2) - mean(X)^2
    long variance =  (sumOfSquares / eventIds.size()) - (mean * mean);
    //standard deviation = square root of the variance
    long stdVar = static_cast<long>(sqrt(static_cast<double>(variance)));
    descriptionLength = static_cast<int>(qMin((mean + stdVar),maxSize));
    //Be sure that the length is minimum 2 digits
    descriptionLength = qMax(descriptionLength,2);

    //Initialize the variables
    previousStartTime = 0;
    previousStartIndex = 1;
    previousEndIndex = nbEvents;
    previousEndTime = static_cast<long>(floor(0.5 + timeStamps(1,nbEvents)));
    fileMaxTime = previousEndTime;

    return OK;
}

void EventsProvider::initializeEmptyProvider(){
    modified = true;
    nbEvents = 0;
    events.setSize(0,0);
    timeStamps.setSize(0,0);
    ///Default description length is 2 characters
    descriptionLength = 2;

    //Initialize the variables
    previousStartTime = 0;
    previousStartIndex = 1;
    previousEndIndex = 1;
    previousEndTime = 0;
    fileMaxTime = 0;
}

void EventsProvider::requestData(long startTime,long endTime,QObject* initiator,long startTimeInRecordingUnits){
    retrieveData(startTime,endTime,initiator);
}

void EventsProvider::retrieveData(long startTime,long endTime,QObject* initiator){   
    Array<dataType> times;
    Array<int> ids;

    if(nbEvents == 0 || startTime > fileMaxTime){
        //Send the information to the receiver.
        emit dataReady(times,ids,initiator,name);
        return;
    }
    if(endTime > fileMaxTime) endTime = fileMaxTime;

    long startIndex = previousStartIndex;
    long endIndex = previousEndIndex;
    long dicotomyBreak = 1000;
    long time;

    //Look up for the closest starting index to the one corresponding to startTime
    //Dicotomy will be used with a stop at dicotomyBreak.
    if((startTime != previousStartTime) && (startTime != previousEndTime)){
        if(startTime == 0){
            startIndex = 1;
            if(endTime <= previousStartTime) endIndex = previousStartIndex;
            else if(endTime <= previousEndTime) endIndex = previousEndIndex;
            else if(endTime > previousEndTime) endIndex = nbEvents;
        }
        if(startTime < previousStartTime){
            startIndex = static_cast<int>(previousStartIndex / 2);
            if(startIndex <= 0) startIndex = 1;
            if(endTime <= previousStartTime) endIndex = previousStartIndex;
            else if(endTime <= previousEndTime) endIndex = previousEndIndex;
            else if(endTime > previousEndTime) endIndex = nbEvents;
        }
        else if(startTime < previousEndTime && startTime > previousStartTime){
            startIndex = previousStartIndex + static_cast<int>((previousEndIndex - previousStartIndex + 1)/ 2);
            if(endTime <= previousEndTime) endIndex = previousEndIndex;
            else if(endTime > previousEndTime) endIndex = nbEvents;
        }
        else if(startTime > previousEndTime){
            startIndex = previousEndIndex;
            endIndex = nbEvents;
        }

        long newStartIndex = startIndex;
        long newEndIndex = endIndex;
        //dicotomy
        while((newEndIndex - newStartIndex + 1) > dicotomyBreak){
            time = static_cast<long>(floor(0.5 + timeStamps(1,newStartIndex)));
            if(time == startTime) break;
            else if(time > startTime){
                long previousStart = newStartIndex;
                newStartIndex = previousStart - ((newEndIndex - previousStart + 1) / 2);
                if(newStartIndex <= 0){
                    newStartIndex = 1;
                    break;
                }
                newEndIndex = previousStart;
            }
            else{
                newStartIndex = newStartIndex + ((newEndIndex - newStartIndex + 1) / 2);
                if(newStartIndex > nbEvents){
                    newStartIndex = nbEvents;
                    break;
                }
            }
        }


        //look up for the startIndex index by index
        time = static_cast<long>(floor(0.5 + timeStamps(1,newStartIndex)));

        if(time < startTime && (newStartIndex < nbEvents)){
            while(time < startTime){
                newStartIndex++;
                if(newStartIndex > nbEvents){
                    newStartIndex = nbEvents;
                    time = static_cast<long>(floor(0.5 + timeStamps(1,newStartIndex)));
                    break;
                }
                else{
                    time = static_cast<long>(floor(0.5 + timeStamps(1,newStartIndex)));
                }
            }
        }
        else if(time > startTime && (newStartIndex > 1)){
            while(time > startTime){
                newStartIndex--;
                if(newStartIndex <= 0){
                    newStartIndex = 1;
                    time =  static_cast<long>(floor(0.5 + timeStamps(1,newStartIndex)));
                    break;
                }
                else{
                    time =  static_cast<long>(floor(0.5 + timeStamps(1,newStartIndex)));
                    if(time < startTime){
                        newStartIndex++;
                        time =  static_cast<long>(floor(0.5 + timeStamps(1,newStartIndex)));
                        break;
                    }
                }
            }
        }

        startIndex = newStartIndex;
    }//end (startTime != previousStartTime) || (startTime != previousEndTime)

    else{
        if(startTime == previousStartTime) startIndex = previousStartIndex;
        else if(startTime == previousEndTime){
            if(static_cast<long>(floor(0.5 + timeStamps(1,previousEndIndex))) < startTime){
                startIndex = previousEndIndex + 1;
                if(startIndex > nbEvents) startIndex = nbEvents;
            }
            else startIndex = previousEndIndex;
        }

        if(endTime <= previousEndTime) endIndex = previousEndIndex;
        else if(endTime > previousEndTime) endIndex = nbEvents;
    }


    //Store the information for the next request
    previousStartTime = startTime;
    previousStartIndex = startIndex;

    //look up for the event ids and indexes.
    //The exact size (<=> number of events is not known yet, so the size of data is set to the maximum possible)
    times.setSize(1,(endIndex - startIndex + 1));
    ids.setSize(1,(endIndex - startIndex + 1));
    time = static_cast<long>(floor(0.5 + timeStamps(1,startIndex)));

    long count = 0;
    while(time <= endTime && startIndex <= nbEvents){
        times(1,count + 1) = qMax(static_cast<dataType>(floor(static_cast<float>(0.5 +(timeStamps(1,startIndex) - static_cast<double>(startTime)) * currentSamplingRate))),0L);
        ids(1,count + 1) = eventIds[events(1,startIndex)];

        count++;
        startIndex++;
        if(startIndex > nbEvents) break;
        time = static_cast<long>(floor(0.5 + timeStamps(1,startIndex)));
    }


    //Store the data in a array of the good size
    Array<dataType> finalTimes;
    Array<int> finalIds;
    finalTimes.setSize(1,count);
    finalIds.setSize(1,count);
    finalTimes.copySubset(times,count);
    finalIds.copySubset(ids,count);

    //Store the information for the next request
    previousEndTime = endTime;
    previousEndIndex = startIndex - 1;

    //Send the information to the receiver.
    emit dataReady(finalTimes,finalIds,initiator,name);
}

void EventsProvider::requestNextEventData(long startTime,long timeFrame,const QList<int> &selectedIds,QObject* initiator){
    long initialStartTime = startTime;
    //Compute the start time for the event look up
    startTime = initialStartTime + static_cast<long>(timeFrame * eventPosition);

    //first look up for the index corresponding to the startTime and then for the first valid event (contained in selectedIds)
    //after that time
    Array<dataType> times;
    Array<int> ids;

    if(startTime > fileMaxTime){
        //Send the information to the receiver.
        emit dataReady(times,ids,initiator,name);
        return;
    }

    long startIndex = previousStartIndex;
    //By default we will go up to the end of the file
    long endTime = fileMaxTime;
    long endIndex = nbEvents;
    long dicotomyBreak = 1000;
    long time;


    //Look up for the closest starting index to the one corresponding to startTime
    //Dicotomy will be used with a stop at dicotomyBreak.
    if((startTime != previousStartTime) && (startTime != previousEndTime)){
        if(startTime == 0){
            startIndex = 1;
        }
        if(startTime < previousStartTime){
            startIndex = static_cast<int>(previousStartIndex / 2);
            if(startIndex <= 0) startIndex = 1;
        }
        else if(startTime < previousEndTime && startTime > previousStartTime){
            startIndex = previousStartIndex + static_cast<int>((previousEndIndex - previousStartIndex + 1)/ 2);
        }
        else if(startTime > previousEndTime){
            startIndex = previousEndIndex;
        }

        long newStartIndex = startIndex;
        long newEndIndex = endIndex;
        //dicotomy
        while((newEndIndex - newStartIndex + 1) > dicotomyBreak){
            time = static_cast<long>(floor(0.5 + timeStamps(1,newStartIndex)));
            if(time == startTime) break;
            else if(time > startTime){
                long previousStart = newStartIndex;
                newStartIndex = previousStart - ((newEndIndex - previousStart + 1) / 2);
                if(newStartIndex <= 0){
                    newStartIndex = 1;
                    break;
                }
                newEndIndex = previousStart;
            }
            else{
                newStartIndex = newStartIndex + ((newEndIndex - newStartIndex + 1) / 2);
                if(newStartIndex > nbEvents){
                    newStartIndex = nbEvents;
                    break;
                }
            }
        }


        //look up for the startIndex index by index
        time = static_cast<long>(floor(0.5 + timeStamps(1,newStartIndex)));

        if(time < startTime && (newStartIndex < nbEvents)){
            while(time < startTime){
                newStartIndex++;
                if(newStartIndex > nbEvents){
                    newStartIndex = nbEvents;
                    time = static_cast<long>(floor(0.5 + timeStamps(1,newStartIndex)));
                    break;
                }
                else{
                    time = static_cast<long>(floor(0.5 + timeStamps(1,newStartIndex)));
                }
            }
        }
        else if(time > startTime && (newStartIndex > 1)){
            while(time > startTime){
                newStartIndex--;
                if(newStartIndex <= 0){
                    newStartIndex = 1;
                    time =  static_cast<long>(floor(0.5 + timeStamps(1,newStartIndex)));
                    break;
                }
                else{
                    time =  static_cast<long>(floor(0.5 + timeStamps(1,newStartIndex)));
                    if(time < startTime){
                        newStartIndex++;
                        time =  static_cast<long>(floor(0.5 + timeStamps(1,newStartIndex)));
                        break;
                    }
                }
            }
        }

        startIndex = newStartIndex;
    }//end (startTime != previousStartTime) || (startTime != previousEndTime)

    else{
        if(startTime == previousStartTime) startIndex = previousStartIndex;
        else if(startTime == previousEndTime){
            if(static_cast<long>(floor(0.5 + timeStamps(1,previousEndIndex))) < startTime){
                startIndex = previousEndIndex + 1;
                if(startIndex > nbEvents) startIndex = nbEvents;
            }
            else startIndex = previousEndIndex;
        }
    }

    //the found event will be placed at eventPosition*100 % of the timeFrame
    //check that the event corresponding to the current startIndex
    //is not the one already at eventPosition, if so take the following start index
    time = static_cast<long>(floor(0.5 + timeStamps(1,startIndex)));
    dataType startingTime = qMax(time - static_cast<long>(timeFrame * eventPosition),0L);
    while ((time == startTime) && (startIndex < nbEvents)){
        startIndex++;
        time = static_cast<long>(floor(0.5 + timeStamps(1,startIndex)));
    }

    //look up for the first event contained in selectedIds which exist after startTime
    while(true){
        int id = eventIds[events(1,startIndex)];

        if(selectedIds.contains(id)) break;
        startIndex++;
        if(startIndex > nbEvents){
            startIndex = nbEvents;
            break;
        }
    }

    //check that a valid index has been found (the startIndex corresponds to an event included in selectedIds)
    //if that is not the case return startTime as the startingTime => no change will be done in the view)

    int id = eventIds[events(1,startIndex)];
    if(!selectedIds.contains(id)){
        Array<dataType> finalTimes;
        Array<int> finalIds;
        emit nextEventDataReady(finalTimes,finalIds,initiator,name,initialStartTime);
        return;
    }

    time = static_cast<long>(floor(0.5 + timeStamps(1,startIndex)));

    //compute the final starting time and the corresponding index
    startingTime = qMax(time - static_cast<long>(timeFrame * eventPosition),0L);

    long newStartIndex = startIndex;
    while(time > startingTime){
        newStartIndex--;
        if(newStartIndex <= 0){
            newStartIndex = 1;
            time =  static_cast<long>(floor(0.5 + timeStamps(1,newStartIndex)));
            break;
        }
        else{
            time =  static_cast<long>(floor(0.5 + timeStamps(1,newStartIndex)));
            if(time < startingTime){
                newStartIndex++;
                time =  static_cast<long>(floor(0.5 + timeStamps(1,newStartIndex)));
                break;
            }
        }
    }

    startIndex = newStartIndex;

    //Store the information for the next request
    previousStartTime = startingTime;
    previousStartIndex = startIndex;


    //look up for the event ids and indexes.
    //The exact size (<=> number of events is not known yet, so the size of data is set to the maximum possible)
    times.setSize(1,(endIndex - startIndex + 1));
    ids.setSize(1,(endIndex - startIndex + 1));
    time = static_cast<long>(floor(0.5 + timeStamps(1,startIndex)));
    endTime = startingTime + timeFrame;

    long count = 0;
    while(time <= endTime && startIndex <= nbEvents){
        times(1,count + 1) = qMax(static_cast<dataType>(floor(static_cast<float>(0.5 +(timeStamps(1,startIndex) - static_cast<float>(startingTime)) * currentSamplingRate))),0L);
        ids(1,count + 1) = eventIds[events(1,startIndex)];

        count++;
        startIndex++;
        if(startIndex > nbEvents) break;
        time = static_cast<long>(floor(0.5 + timeStamps(1,startIndex)));
    }

    //Store the data in a array of the good size
    Array<dataType> finalTimes;
    Array<int> finalIds;
    finalTimes.setSize(1,count);
    finalIds.setSize(1,count);
    finalTimes.copySubset(times,count);
    finalIds.copySubset(ids,count);

    //Store the information for the next request
    previousEndTime = endTime;
    previousEndIndex = startIndex - 1;

    //Send the information to the receiver.
    emit nextEventDataReady(finalTimes,finalIds,initiator,name,startingTime);
}

void EventsProvider::requestPreviousEventData(long startTime,long timeFrame,QList<int> selectedIds,QObject* initiator){
    long initialStartTime = startTime;
    //Compute the start time for the event look up
    startTime = initialStartTime + static_cast<long>(timeFrame * eventPosition);

    //first look up for the index corresponding to the startTime and then for the first valid event (contained in selectedIds)
    //before that time.

    //first look up for the index corresponding to the startTime
    Array<dataType> times;
    Array<int> ids;

    long startIndex = previousStartIndex;
    //By default we will go up to the end of the file
    long endTime = fileMaxTime;
    long endIndex = nbEvents;
    long dicotomyBreak = 1000;
    long time;

    //Look up for the closest starting index to the one corresponding to startTime
    //Dicotomy will be used with a stop at dicotomyBreak.
    if((startTime != previousStartTime) && (startTime != previousEndTime)){
        if(startTime == 0){
            startIndex = 1;
        }
        if(startTime < previousStartTime){
            startIndex = static_cast<int>(previousStartIndex / 2);
            if(startIndex <= 0) startIndex = 1;
        }
        else if(startTime < previousEndTime && startTime > previousStartTime){
            startIndex = previousStartIndex + static_cast<int>((previousEndIndex - previousStartIndex + 1)/ 2);
        }
        else if(startTime > previousEndTime){
            startIndex = previousEndIndex;
        }

        long newStartIndex = startIndex;
        long newEndIndex = endIndex;

        //dicotomy
        while((newEndIndex - newStartIndex + 1) > dicotomyBreak){
            time = static_cast<long>(floor(0.5 + timeStamps(1,newStartIndex)));
            if(time == startTime) break;
            else if(time > startTime){
                long previousStart = newStartIndex;
                newStartIndex = previousStart - ((newEndIndex - previousStart + 1) / 2);
                if(newStartIndex <= 0){
                    newStartIndex = 1;
                    break;
                }
                newEndIndex = previousStart;
            }
            else{
                newStartIndex = newStartIndex + ((newEndIndex - newStartIndex + 1) / 2);
                if(newStartIndex > nbEvents){
                    newStartIndex = nbEvents;
                    break;
                }
            }
        }


        //look up for the startIndex index by index
        time = static_cast<long>(floor(0.5 + timeStamps(1,newStartIndex)));

        if(time < startTime && (newStartIndex < nbEvents)){
            while(time < startTime){
                newStartIndex++;
                if(newStartIndex > nbEvents){
                    newStartIndex = nbEvents;
                    time = static_cast<long>(floor(0.5 + timeStamps(1,newStartIndex)));
                    break;
                }
                else{
                    time = static_cast<long>(floor(0.5 + timeStamps(1,newStartIndex)));
                    if(time > startTime){
                        newStartIndex--;
                        time =  static_cast<long>(floor(0.5 + timeStamps(1,newStartIndex)));
                        break;
                    }
                }
            }
        }
        else if(time > startTime && (newStartIndex > 1)){
            while(time > startTime){
                newStartIndex--;
                if(newStartIndex <= 0){
                    newStartIndex = 1;
                    time =  static_cast<long>(floor(0.5 + timeStamps(1,newStartIndex)));
                    break;
                }
                else{
                    time =  static_cast<long>(floor(0.5 + timeStamps(1,newStartIndex)));
                    if(time < startTime) break;
                }
            }
        }

        startIndex = newStartIndex;
    }//end (startTime != previousStartTime) || (startTime != previousEndTime)

    else{
        if(startTime == previousStartTime) startIndex = previousStartIndex;
        else if(startTime == previousEndTime){
            if(static_cast<long>(floor(0.5 + timeStamps(1,previousEndIndex))) < startTime){
                startIndex = previousEndIndex + 1;
                if(startIndex > nbEvents) startIndex = nbEvents;
            }
            else startIndex = previousEndIndex;
        }
    }


    //look up for the first event contained in selectedIds which exist before startTime

    //the found event will be placed at eventPosition*100 % of the timeFrame
    //check that the event corresponding to the startIndex
    //is not the one already at eventPosition, if so take the previous index.
    time = static_cast<long>(floor(0.5 + timeStamps(1,startIndex)));
    dataType startingTime = qMax(time - static_cast<long>(timeFrame * eventPosition),0L);
    while((time == startTime) && (startIndex > 1)){
        startIndex--;
        time = static_cast<long>(floor(0.5 + timeStamps(1,startIndex)));
    }

    //now, look up for the first event contained in selectedIds which exist before endTime
    while(true){
        int id = eventIds[events(1,startIndex)];
        if(selectedIds.contains(id)) break;
        startIndex--;
        if(startIndex <= 0){
            startIndex = 1;
            break;
        }
    }

    //check that a valid index has been found: the startIndex corresponds to an event included in selectedIds
    ////and the corresponding time in before the startTime
    //if that is not the case return startTime as the startingTime => no change will be done in the view)
    int id = eventIds[events(1,startIndex)];
    time = static_cast<long>(floor(0.5 + timeStamps(1,startIndex)));
    if(!selectedIds.contains(id) || time > startTime){
        Array<dataType> finalTimes;
        Array<int> finalIds;
        emit previousEventDataReady(finalTimes,finalIds,initiator,name,initialStartTime);
        return;
    }

    //compute the final starting time and the corresponding index
    startingTime = qMax(time - static_cast<long>(timeFrame * eventPosition),0L);
    long newStartIndex = startIndex;

    while(time > startingTime){
        newStartIndex--;
        if(newStartIndex <= 0){
            newStartIndex = 1;
            time =  static_cast<long>(floor(0.5 + timeStamps(1,newStartIndex)));
            break;
        }
        else{
            time =  static_cast<long>(floor(0.5 + timeStamps(1,newStartIndex)));
            if(time < startingTime){
                newStartIndex++;
                time =  static_cast<long>(floor(0.5 + timeStamps(1,newStartIndex)));
                break;
            }
        }
    }

    startIndex = newStartIndex;
    //Store the information for the next request
    previousStartTime = startingTime;
    previousStartIndex = startIndex;

    //look up for the event ids and indexes.
    //The exact size (<=> number of events is not known yet, so the size of data is set to the maximum possible)
    times.setSize(1,(endIndex - startIndex + 1));
    ids.setSize(1,(endIndex - startIndex + 1));
    time = static_cast<long>(floor(0.5 + timeStamps(1,startIndex)));
    endTime = startingTime + timeFrame;

    long count = 0;
    while(time <= endTime && startIndex <= nbEvents){
        times(1,count + 1) = qMax(static_cast<dataType>(floor(static_cast<float>(0.5 +(timeStamps(1,startIndex) - static_cast<float>(startingTime)) * currentSamplingRate))),0L);
        ids(1,count + 1) = eventIds[events(1,startIndex)];
        count++;
        startIndex++;
        if(startIndex > nbEvents) break;
        time = static_cast<long>(floor(0.5 + timeStamps(1,startIndex)));
    }

    //Store the data in a array of the good size
    Array<dataType> finalTimes;
    Array<int> finalIds;
    finalTimes.setSize(1,count);
    finalIds.setSize(1,count);
    finalTimes.copySubset(times,count);
    finalIds.copySubset(ids,count);

    //Store the information for the next request
    previousEndTime = endTime;
    previousEndIndex = startIndex - 1;

    //Send the information to the receiver.
    emit previousEventDataReady(finalTimes,finalIds,initiator,name,startingTime);

}

void EventsProvider::modifiedEvent(int selectedEventId,double time,double newTime){
    modified = true;

    long timeIndex = findIndex(time,selectedEventId);
    long newTimeIndex = findIndex(newTime,selectedEventId);
    EventDescription selectedEvent = idsDescriptions[selectedEventId];

    //Clear the redo variables
    eventsRedo.setSize(0,0);
    timeStampsRedo.setSize(0,0);
    eventDescriptionCounterRedo.clear();

    //Prepare the undo variables
    eventsUndo = events;
    timeStampsUndo = timeStamps;
    eventDescriptionCounterUndo = eventDescriptionCounter;

    if(timeIndex == newTimeIndex){
        events(1,timeIndex) = selectedEvent;
        timeStamps(1,timeIndex) = newTime;
        //Update fileMaxTime
        fileMaxTime = static_cast<long>(floor(0.5 + timeStamps(1,nbEvents)));
    }
    //Moved forward
    else if(newTime > time){
        for(int i = timeIndex; i < (newTimeIndex - 1);++i){
            events(1,i) = events(1,i+1);
            timeStamps(1,i) = timeStamps(1,i+1);
        }

        if(newTime > timeStamps(1,newTimeIndex)){
            events(1,newTimeIndex - 1) = events(1,newTimeIndex);
            timeStamps(1,newTimeIndex - 1) = timeStamps(1,newTimeIndex);
            events(1,newTimeIndex) = selectedEvent;
            timeStamps(1,newTimeIndex) = newTime;
        }
        else{
            events(1,newTimeIndex - 1) = selectedEvent;
            timeStamps(1,newTimeIndex - 1) = newTime;
        }

        //Update fileMaxTime
        fileMaxTime = static_cast<long>(floor(0.5 + timeStamps(1,nbEvents)));
    }

    //Moved backward
    else if(newTime < time){
        for(int i = timeIndex; i > (newTimeIndex + 1);--i){
            events(1,i) = events(1,i-1);
            timeStamps(1,i) = timeStamps(1,i-1);
        }

        if(newTime < timeStamps(1,newTimeIndex)){
            events(1,newTimeIndex + 1) = events(1,newTimeIndex);
            timeStamps(1,newTimeIndex + 1) = timeStamps(1,newTimeIndex);
            events(1,newTimeIndex) = selectedEvent;
            timeStamps(1,newTimeIndex) = newTime;
        }
        else{
            events(1,newTimeIndex + 1) = selectedEvent;
            timeStamps(1,newTimeIndex + 1) = newTime;
        }

        //Update fileMaxTime
        fileMaxTime = static_cast<long>(floor(0.5 + timeStamps(1,nbEvents)));
    }

    previousStartTime = 0;
    previousStartIndex = 1;
    previousEndIndex = nbEvents;
    previousEndTime = static_cast<long>(floor(0.5 + timeStamps(1,nbEvents)));
}

long EventsProvider::findIndex(double eventTime,int eventId){  
    if(eventTime > fileMaxTime) return nbEvents;

    long startTime = static_cast<long>(floor(0.5 + eventTime));

    //By default we will go up to the end of the file
    long endIndex = nbEvents;
    long startIndex = previousStartIndex;
    long dicotomyBreak = 1000;
    long time;

    //Look up for the closest starting index to the one corresponding to startTime
    //Dicotomy will be used with a stop at dicotomyBreak.
    if((startTime != previousStartTime) && (startTime != previousEndTime)){
        if(startTime == 0) startIndex = 1;
        if(startTime < previousStartTime){
            startIndex = static_cast<int>(previousStartIndex / 2);
            if(startIndex <= 0) startIndex = 1;
        }
        else if(startTime < previousEndTime && startTime > previousStartTime){
            startIndex = previousStartIndex + static_cast<int>((previousEndIndex - previousStartIndex + 1)/ 2);
        }
        else if(startTime > previousEndTime)startIndex = previousEndIndex;

        long newStartIndex = startIndex;
        long newEndIndex = endIndex;
        //dicotomy
        while((newEndIndex - newStartIndex + 1) > dicotomyBreak){
            time = static_cast<long>(floor(0.5 + timeStamps(1,newStartIndex)));
            if(time == startTime) break;
            else if(time > startTime){
                long previousStart = newStartIndex;
                newStartIndex = previousStart - ((newEndIndex - previousStart + 1) / 2);
                if(newStartIndex <= 0){
                    newStartIndex = 1;
                    break;
                }
                newEndIndex = previousStart;
            }
            else{
                newStartIndex = newStartIndex + ((newEndIndex - newStartIndex + 1) / 2);
                if(newStartIndex > nbEvents){
                    newStartIndex = nbEvents;
                    break;
                }
            }
        }


        //look up for the startIndex index by index
        time = static_cast<long>(floor(0.5 + timeStamps(1,newStartIndex)));

        if(time < startTime && (newStartIndex < (nbEvents))){
            while(time < startTime){
                newStartIndex++;
                if(newStartIndex > nbEvents){
                    newStartIndex = nbEvents;
                    time = static_cast<long>(floor(0.5 + timeStamps(1,newStartIndex)));
                    break;
                }
                else{
                    time = static_cast<long>(floor(0.5 + timeStamps(1,newStartIndex)));
                }
            }
        }
        else if(time > startTime && (newStartIndex > 1)){
            while(time > startTime){
                newStartIndex--;
                if(newStartIndex <= 0){
                    newStartIndex = 1;
                    time =  static_cast<long>(floor(0.5 + timeStamps(1,newStartIndex)));
                    break;
                }
                else{
                    time =  static_cast<long>(floor(0.5 + timeStamps(1,newStartIndex)));
                    if(time < startTime){
                        newStartIndex++;
                        time =  static_cast<long>(floor(0.5 + timeStamps(1,newStartIndex)));
                        break;
                    }
                }
            }
        }

        startIndex = newStartIndex;
    }//end (startTime != previousStartTime) || (startTime != previousEndTime)

    else{
        if(startTime == previousStartTime) startIndex = previousStartIndex;
        else if(startTime == previousEndTime){
            if(static_cast<long>(floor(0.5 + timeStamps(1,previousEndIndex))) < startTime){
                startIndex = previousEndIndex + 1;
                if(startIndex > nbEvents) startIndex = nbEvents;
            }
            else startIndex = previousEndIndex;
        }
    }

    if(eventId != -1){
        int id = eventIds[events(1,startIndex)];
        if(id != eventId){
            double diff1 = fabs(timeStamps(1,startIndex) - timeStamps(1,startIndex - 1));
            double diff2 = fabs(timeStamps(1,startIndex + 1) - timeStamps(1,startIndex));
            if(diff1 < diff2 && eventIds[events(1,startIndex - 1)] == eventId) startIndex--;
            else if(diff1 < diff2 && eventIds[events(1,startIndex - 1)] != eventId && eventIds[events(1,startIndex + 1)] == eventId ) startIndex++;
            else if(diff2 < diff1 && eventIds[events(1,startIndex + 1)] == eventId) startIndex++;
        }
    }

    return startIndex;
}


void EventsProvider::undo(){
    modified = true;//in case the user saved and then undo, this will allowed to save again

    eventsRedo = events;
    timeStampsRedo = timeStamps;
    eventDescriptionCounterRedo = eventDescriptionCounter;

    events = eventsUndo;
    timeStamps = timeStampsUndo;
    eventDescriptionCounter = eventDescriptionCounterUndo;

    //update the number of events in case some have been removed or added
    nbEvents = events.nbOfColumns();

    //Clear the undo variables
    eventsUndo.setSize(0,0);
    timeStampsUndo.setSize(0,0);
    eventDescriptionCounterUndo.clear();

    previousStartTime = 0;
    previousStartIndex = 1;

    if(nbEvents == 0){
        previousEndIndex = 1;
        previousEndTime = 0;
        fileMaxTime = 0;
    }
    else{
        previousEndIndex = nbEvents;
        previousEndTime = static_cast<long>(floor(0.5 + timeStamps(1,nbEvents)));
        fileMaxTime = previousEndTime;
    }

    if(eventDescriptionCounter.count() < eventDescriptionCounterRedo.count()){
        QMap<EventDescription,int>::Iterator iterator;
        for(iterator = eventDescriptionCounterRedo.begin(); iterator != eventDescriptionCounterRedo.end(); ++iterator){
            if(!eventDescriptionCounter.contains(iterator.key())){
                removeEventDescription(iterator.key());
                break;
            }
        }
    }
    if(eventDescriptionCounter.count() > eventDescriptionCounterRedo.count()){
        QMap<EventDescription,int>::Iterator iterator;
        for(iterator = eventDescriptionCounter.begin(); iterator != eventDescriptionCounter.end(); ++iterator){
            if(!eventDescriptionCounterRedo.contains(iterator.key())){
                addEventDescription(iterator.key());
                break;
            }
        }
    }
}

void EventsProvider::redo(){
    modified = true;//in case the user saved and then undo, this will allowed to save again

    eventsUndo = events;
    timeStampsUndo = timeStamps;
    eventDescriptionCounterUndo = eventDescriptionCounter;

    events = eventsRedo;
    timeStamps = timeStampsRedo;
    eventDescriptionCounter = eventDescriptionCounterRedo;

    //update the number of events in case some have been removed or added
    nbEvents = events.nbOfColumns();

    //Clear the redo variables
    eventsRedo.setSize(0,0);
    timeStampsRedo.setSize(0,0);
    eventDescriptionCounterRedo.clear();

    previousStartTime = 0;
    previousStartIndex = 1;

    if(nbEvents == 0){
        previousEndIndex = 1;
        previousEndTime = 0;
        fileMaxTime = 0;
    }
    else{
        previousEndIndex = nbEvents;
        previousEndTime = static_cast<long>(floor(0.5 + timeStamps(1,nbEvents)));
        fileMaxTime = previousEndTime;
    }

    if(eventDescriptionCounter.count() < eventDescriptionCounterUndo.count()){
        QMap<EventDescription,int>::Iterator iterator;
        for(iterator = eventDescriptionCounterUndo.begin(); iterator != eventDescriptionCounterUndo.end(); ++iterator){
            if(!eventDescriptionCounter.contains(iterator.key())){
                removeEventDescription(iterator.key());
                break;
            }
        }
    }
    if(eventDescriptionCounter.count() > eventDescriptionCounterUndo.count()){
        QMap<EventDescription,int>::Iterator iterator;
        for(iterator = eventDescriptionCounter.begin(); iterator != eventDescriptionCounter.end(); ++iterator){
            if(!eventDescriptionCounterUndo.contains(iterator.key())){
                addEventDescription(iterator.key());
                break;
            }
        }
    }
}

void EventsProvider::clearUndoRedoData(){
    //Clear the redo arrays
    eventsRedo.setSize(0,0);
    timeStampsRedo.setSize(0,0);
    eventDescriptionCounterRedo.clear();

    //Clear the undo arrays
    eventsUndo.setSize(0,0);
    timeStampsUndo.setSize(0,0);
    eventDescriptionCounterUndo.clear();
}

bool EventsProvider::save(QFile* eventFile){
    QTextStream fileStream(eventFile);
    fileStream.setRealNumberPrecision(12);

    for(int i = 1;i<=nbEvents;++i)
        fileStream<<timeStamps(1,i)<<"\t"<<events(1,i)<< "\n";

    bool status = eventFile->isOpen();
    if(status)
        modified = false;
    return status;
}


void EventsProvider::removeEvent(int selectedEventId,double time){
    modified = true;
    long timeIndex;
    if(nbEvents != 1) timeIndex = findIndex(time,selectedEventId);

    //Clear the redo variables
    eventsRedo.setSize(0,0);
    timeStampsRedo.setSize(0,0);
    eventDescriptionCounterRedo.clear();

    //Prepare the undo variables
    eventsUndo = events;
    timeStampsUndo = timeStamps;
    eventDescriptionCounterUndo = eventDescriptionCounter;

    EventDescription description = idsDescriptions[selectedEventId];
    eventDescriptionCounter[description] = eventDescriptionCounter[description] - 1;

    if(eventDescriptionCounter[description] == 0){
        removeEventDescription(description);
        eventDescriptionCounter.remove(description);
    }

    if(nbEvents != 1){
        events.setSize(1,nbEvents - 1);
        timeStamps.setSize(1,nbEvents - 1);

        events.copySubset(eventsUndo,1,(timeIndex - 1),1);
        events.copySubset(eventsUndo,(timeIndex + 1),nbEvents,timeIndex);
        timeStamps.copySubset(timeStampsUndo,1,(timeIndex - 1),1);
        timeStamps.copySubset(timeStampsUndo,(timeIndex + 1),nbEvents,timeIndex);

        nbEvents--;
        previousStartTime = 0;
        previousStartIndex = 1;
        previousEndIndex = nbEvents;
        previousEndTime = static_cast<long>(floor(0.5 + timeStamps(1,nbEvents)));
        fileMaxTime = previousEndTime;
    }
    else{
        events.setSize(0,0);
        timeStamps.setSize(0,0);

        nbEvents = 0;
        previousStartTime = 0;
        previousStartIndex = 1;
        previousEndIndex = 1;
        previousEndTime = 0;
        fileMaxTime = 0;
    }
}

void EventsProvider::addEvent(const QString &eventDescriptionToAdd, double time){
    modified = true;

    //Clear the redo variables
    eventsRedo.setSize(0,0);
    timeStampsRedo.setSize(0,0);
    eventDescriptionCounterRedo.clear();

    //Prepare the undo variables
    eventsUndo = events;
    timeStampsUndo = timeStamps;
    eventDescriptionCounterUndo = eventDescriptionCounter;

    if(nbEvents != 0){
        long timeIndex = findIndex(time);
        double timeAtTimeIndex = timeStamps(1,timeIndex);

        events.setSize(1,nbEvents + 1);
        timeStamps.setSize(1,nbEvents + 1);

        if(time <= timeAtTimeIndex){
            events.copySubset(eventsUndo,1,(timeIndex - 1),1);
            events(1,timeIndex) = EventDescription(eventDescriptionToAdd);
            events.copySubset(eventsUndo,timeIndex,nbEvents,timeIndex + 1);

            timeStamps.copySubset(timeStampsUndo,1,(timeIndex - 1),1);
            timeStamps(1,timeIndex) = time;
            timeStamps.copySubset(timeStampsUndo,timeIndex,nbEvents,timeIndex + 1);
        }
        else{
            events.copySubset(eventsUndo,1,timeIndex,1);
            events(1,timeIndex + 1) = EventDescription(eventDescriptionToAdd);
            events.copySubset(eventsUndo,timeIndex + 1,nbEvents,timeIndex + 2);

            timeStamps.copySubset(timeStampsUndo,1,timeIndex,1);
            timeStamps(1,timeIndex + 1) = time;
            timeStamps.copySubset(timeStampsUndo,timeIndex + 1,nbEvents,timeIndex + 2);
        }
        if(fileMaxTime < static_cast<long>(floor(0.5 + time))) fileMaxTime = static_cast<long>(floor(0.5 + time));
    }
    //New (empty) event file
    else{
        events.setSize(1,1);
        timeStamps.setSize(1,1);

        events(1,1) = EventDescription(eventDescriptionToAdd);
        timeStamps(1,1) = time;
        fileMaxTime = static_cast<long>(floor(0.5 + time));
    }

    nbEvents++;
    previousStartTime = 0;
    previousStartIndex = 1;
    previousEndIndex = nbEvents;
    previousEndTime = static_cast<long>(floor(0.5 + timeStamps(1,nbEvents)));

    //Check if the added event description is a new event description.
    bool newDescription = !eventIds.contains(EventDescription(eventDescriptionToAdd));

    //Add the new description to the list of existing ones and compute the new descriptionLength
    if(newDescription) addEventDescription(eventDescriptionToAdd);
    else eventDescriptionCounter[eventDescriptionToAdd] = eventDescriptionCounter[eventDescriptionToAdd] + 1;

}

void EventsProvider::addEventDescription(QString eventDescriptionToAdd){

    QMap<int,int> oldNewEventIds;
    QMap<int,int> newOldEventIds;
    QMap<EventDescription,int> eventIdsTmp;

    //Add the new description to the list of existing ones and compute the new descriptionLength
    idsDescriptions.clear();
    QList<EventDescription> descriptions = eventIds.keys();

    descriptions.append(EventDescription(eventDescriptionToAdd));

    qSort(descriptions);
    long maxSize = 0;
    long sum = 0;
    long sumOfSquares = 0;
    for(int i = 0; i< static_cast<int>(descriptions.size());++i){
        EventDescription description = descriptions[i];
        if(description != eventDescriptionToAdd){
            oldNewEventIds.insert(eventIds[description],i + 1);
            newOldEventIds.insert(i + 1,eventIds[description]);
        }
        eventIdsTmp.insert(description,i + 1);
        idsDescriptions.insert(i + 1,description);
        long length = static_cast<long>(description.length());
        if(length > maxSize) maxSize = length;
        sum += length;
        sumOfSquares += (length * length);
    }

    //Rebuild eventIds
    eventIds.clear();
    QMap<EventDescription,int>::Iterator it;
    for(it = eventIdsTmp.begin(); it != eventIdsTmp.end(); ++it){
        eventIds.insert(it.key(),it.value());
    }


    //Compute the length to use to display the description of the events in the event palette.
    // descriptionLength = min(mean + 1 * standard deviation, maxsize)
    long mean = sum / eventIds.size();
    //variance(X) = mean(X^2) - mean(X)^2
    long variance =  (sumOfSquares / eventIds.size()) - (mean * mean);
    //standard deviation = square root of the variance
    long stdVar = static_cast<long>(sqrt(static_cast<double>(variance)));
    descriptionLength = static_cast<int>(qMin((mean + stdVar),maxSize));
    //Be sure that the length is minimum 2 digits
    descriptionLength = qMax(descriptionLength,2);

    eventDescriptionCounter.insert(eventDescriptionToAdd,1);

    emit newEventDescriptionCreated(name,oldNewEventIds,newOldEventIds,eventDescriptionToAdd);
}

void EventsProvider::removeEventDescription(QString eventDescriptionToRemove){    
    QMap<int,int> oldNewEventIds;
    QMap<int,int> newOldEventIds;
    QMap<EventDescription,int> eventIdsTmp;
    int removedEventId = eventIds[eventDescriptionToRemove];

    //Remove the description of the list of existing ones and compute the new descriptionLength
    idsDescriptions.clear();
    QList<EventDescription> newDescriptions = eventIds.keys();
    newDescriptions.removeAll(EventDescription(eventDescriptionToRemove));

    qSort(newDescriptions);
    long maxSize = 0;
    long sum = 0;
    long sumOfSquares = 0;
    for(int i = 0; i< static_cast<int>(newDescriptions.size());++i){
        EventDescription description = newDescriptions[i];
        oldNewEventIds.insert(eventIds[description],i + 1);
        newOldEventIds.insert(i + 1,eventIds[description]);
        eventIdsTmp.insert(description,i + 1);
        idsDescriptions.insert(i + 1,description);
        long length = static_cast<long>(description.length());
        if(length > maxSize) maxSize = length;
        sum += length;
        sumOfSquares += (length * length);
    }

    //Rebuild eventIds
    eventIds.clear();
    QMap<EventDescription,int>::Iterator it;
    for(it = eventIdsTmp.begin(); it != eventIdsTmp.end(); ++it){
        eventIds.insert(it.key(),it.value());
    }

    //the default length is 2 digits
    if(eventIds.size() == 0) descriptionLength = 2;
    else{
        //Compute the length to use to display the descrption of the events in the event palette.
        // descriptionLength = min(mean + 1 * standard deviation, maxsize)
        long mean = sum / eventIds.size();
        //variance(X) = mean(X^2) - mean(X)^2
        long variance =  (sumOfSquares / eventIds.size()) - (mean * mean);
        //standard deviation = square root of the variance
        long stdVar = static_cast<long>(sqrt(static_cast<double>(variance)));
        descriptionLength = static_cast<int>(qMin((mean + stdVar),maxSize));
        //Be sure that the length is minimum 2 digits
        descriptionLength = qMax(descriptionLength,2);
    }

    emit eventDescriptionRemoved(name,oldNewEventIds,newOldEventIds,removedEventId,eventDescriptionToRemove);
}

void EventsProvider::renameEvent(int selectedEventId, const QString &newEventDescription, double time){
    modified = true;

    //Clear the redo variables
    eventsRedo.setSize(0,0);
    timeStampsRedo.setSize(0,0);
    eventDescriptionCounterRedo.clear();

    //Prepare the undo variables
    eventsUndo = events;
    timeStampsUndo = timeStamps;
    eventDescriptionCounterUndo = eventDescriptionCounter;

    //remove the old event description
    EventDescription description = idsDescriptions[selectedEventId];
    eventDescriptionCounter[description] = eventDescriptionCounter[description] - 1;

    if(eventDescriptionCounter[description] == 0){
        removeEventDescription(description);
        eventDescriptionCounter.remove(description);
    }

    //replace the old description by the new one.
    if(nbEvents != 0){
        long timeIndex = findIndex(time);

        events(1,timeIndex) = EventDescription(newEventDescription);
    }
    //New (empty) event file, should never be possible
    else return;


    //Check if the newEventDescription is a new event description.
    bool newDescription = !eventIds.contains(EventDescription(newEventDescription));

    //Add the new description to the list of existing ones and compute the new descriptionLength
    if(newDescription) addEventDescription(newEventDescription);
    else eventDescriptionCounter[newEventDescription] = eventDescriptionCounter[newEventDescription] + 1;

}


//Operator < on EventDescription to sort them in an case-insensitive maner.
bool operator<(const EventDescription& s1,const EventDescription& s2){
    if(s1.toLower() == s2.toLower()) return (static_cast<QString>(s1) < static_cast<QString>(s2));
    else return (static_cast<QString>(s1.toLower()) < static_cast<QString>(s2.toLower()));
}
