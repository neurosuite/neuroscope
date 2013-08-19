/***************************************************************************
                          eventsprovider.h  -  description
                             -------------------
    begin                : Mon Jun 07 2004
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

#ifndef EVENTSPROVIDER_H
#define EVENTSPROVIDER_H

//include files for the application
#include <dataprovider.h>
#include <array.h>
#include <types.h>

// include files for QT
#include <QObject>
#include <QFile>

#include <QList>
#include <QMap>
//include files for c/c++ libraries
#include <math.h>

/**Class storing and providing the event information.
  *@author Lynn Hazan
  */


/**Class used to compare strings in case-insensitive maner.*/
class EventDescription : public QString{
public:
    EventDescription():QString(){}
    EventDescription(const QString& s):QString(s){}
};

bool operator<(const EventDescription& s1,const EventDescription& s2);


class EventsProvider : public DataProvider  {
    Q_OBJECT
public:

    /**Information retun after a call to loadFile/saveDocument/createFeatureFile*/
    enum loadReturnMessage {OK=0,OPEN_ERROR=1,INCORRECT_CONTENT=2,COUNT_ERROR=3};


    /**Constructor.
  * @param fileUrl the url of the event file containing the event ids.
  * @param currentSamplingRate sampling rate of the current file.
  * @param position represents the percentage from the begining of the window where the events are display when browsing.
  */
    EventsProvider(const QString &fileUrl,double currentSamplingRate,int position = 25);
    ~EventsProvider();

    /**Triggers the retrieve of the events included in the time interval given by @p startTime and @p endTime.
  * @param startTime begining of the time interval from which to retrieve the data in miliseconds.
  * @param endTime end of the time interval from which to retrieve the data.
  * @param initiator instance requesting the data.
  * @param startTimeInRecordingUnits begining of the time interval from which to retrieve the data in recording units.
  */
    void requestData(long startTime,long endTime,QObject* initiator,long startTimeInRecordingUnits = 0);

    /**Looks up for the first of the events included in the list @p selectedIds existing after the time @p startTime.
  * All the events included in the time interval given by @p timeFrame are retrieved. The time interval start time is
  * computed in order to have the first event found located at @p eventPosition percentage of the time interval.
  * @param startTime starting time for the look up.
  * @param timeFrame time interval for which to retrieve the data.
  * @param selectedIds list of event ids to look up for.
  * @param initiator instance requesting the data.
  */
    void requestNextEventData(long startTime,long timeFrame,const QList<int> &selectedIds,QObject* initiator);


    /**Looks up for the first of the events included in the list @p selectedIds existing before the time @p endTime.
  * All the events included in the time interval given by @p timeFrame are retrieved. The time interval start time is
  * computed in order to have the first event found located at @p eventPosition percentage of the time interval.
  * @param endTime starting time for the look up.
  * @param timeFrame time interval for which to retrieve the data.
  * @param selectedIds list of event ids to look up for.
  * @param initiator instance requesting the data.
  */
    void requestPreviousEventData(long endTime,long timeFrame,QList<int> selectedIds,QObject* initiator);

    /**Loads the event ids and the corresponding spike time.
  * @return an loadReturnMessage enum giving the load status
  */
    int loadData();

    /**Returns the number of events in the event file the provider provides the data for
  * @return number of events.
  .*/
    int getNbEvents() const {return nbEvents;}

    /**Returns map between the description of the events and an numeric identifier.
  * @return map for the event descriptions.*/
    QMap<EventDescription,int> eventDescriptionIdMap() const{return eventIds;}

    /**Returns map between the a numeric identifier and the description of the events.
  * @return map for the event ids.*/
    QMap<int,EventDescription> eventIdDescriptionMap() const{return idsDescriptions;}

    /**Returns the name of the provider which is the event file number.
  * @return provider'name.
  */
    QString getName() const {return name;}

    /**Returns the value to use as the length for the event descriptions in the event palette.
  * @return length.
  */
    int getDescriptionLength() const {return descriptionLength;}

    /** Sets the position where the events are display when browsing.
  * @param position percentage from the begining of the window.
  */
    void setEventPosition(int position){eventPosition = static_cast<float>(position) / 100.0;}

    /** Updates the provider data to take into account the modification of an event.
  * @param selectedEventId id of the modified event.
  * @param time initial time of the modified event.
  * @param newTime new time of the modified event.
  */
    void modifiedEvent(int selectedEventId,double time,double newTime);

    /** Updates the provider data to take into account the deletion of an event.
  * @param selectedEventId id of the deleted event.
  * @param time initial time of the deleted event.
  */
    void removeEvent(int selectedEventId,double time);

    /** Updates the provider data to take into account the addition of an event.
  * @param eventDescriptionToAdd description of the added event.
  * @param time time of the added event.
  */
    void addEvent(const QString &eventDescriptionToAdd,double time);

    /** Reverts the last user action.*/
    void undo();

    /** Reverts the last undo action.*/
    void redo();

    /** Empties the undo and redo state of the data.*/
    void clearUndoRedoData();

    /**Saves the data to disk.
  * @param eventFile file where to save the data.
  * @return the output file status.
  */
    bool save(QFile* eventFile);

    /**Tells if the event file has been modified at least since the last save.
  * @return true if the file has been modified, false otherwise.
  */
    bool isModified() const {return modified;}

    /**Initializes the provider as it is the provider of a new empty event file.*/
    void initializeEmptyProvider();

    /**Updates the sampling rate for the current document.
  * @param rate sampling rate.
  */
    void updateSamplingRate(double rate){
        currentSamplingRate = static_cast<double>(rate / 1000.0);

        //Initialize the variables
        previousStartTime = 0;
        previousStartIndex = 1;
        previousEndIndex = nbEvents;
        previousEndTime = static_cast<long>(floor(0.5 + timeStamps(1,nbEvents)));
        fileMaxTime = previousEndTime;
    }

    /** Updates the provider data to take into account the renaming of an event.
  * @param selectedEventId id of the event to rename.
  * @param newEventDescription new name for the event to rename.
  * @param time time of the event to rename.
  */
    void renameEvent(int selectedEventId,const QString &newEventDescription,double time);

Q_SIGNALS:
    /**Signals that the data have been retrieved.
  * @param times 1 line array containing the time (in recording samples) of each event existing in the requested time frame.
  * @param ids 1 line array containing the identifiers of each event existing in the requested time frame.
  * @param initiator instance requesting the data.
  * @param providerName name of the instance providing the data.
  */
    void dataReady(Array<dataType>& times,Array<int>& ids,QObject* initiator,QString providerName);

    /**Signals that the data for the next event have been retrieved.
  * @param times 1 line array containing the time (in recording samples) of each event existing in the requested time frame.
  * @param ids 1 line array containing the identifiers of each event existing in the requested time frame.
  * @param initiator instance requesting the data.
  * @param providerName name of the instance providing the data.
  * @param startingTime time from which the data have been retreived.
  */
    void nextEventDataReady(Array<dataType>& times,Array<int>& ids,QObject* initiator,QString providerName,long startingTime);

    /**Signals that the data for the previous event have been retrieved.
  * @param times 1 line array containing the time (in recording samples) of each event existing in the requested time frame.
  * @param ids 1 line array containing the identifiers of each event existing in the requested time frame.
  * @param initiator instance requesting the data.
  * @param providerName name of the instance providing the data.
  * @param startingTime time from which the data have been retreived.
  */
    void previousEventDataReady(Array<dataType>& times,Array<int>& ids,QObject* initiator,QString providerName,long startingTime);

    /**Signals that a new event description has been created.
  * @param providerName provider identifier.
  * @param oldNewEventIds map between the previous eventIds and the new ones.
  * @param newOldEventIds map between the new eventIds and the previous ones.
  * @param eventDescriptionAdded new event description added.
  */
    void newEventDescriptionCreated(QString providerName,QMap<int,int> oldNewEventIds,QMap<int,int> newOldEventIds,QString eventDescriptionAdded);

    /**Signals that an event description has been removed.
  * @param providerName provider identifier.
  * @param oldNewEventIds map between the previous eventIds and the new ones.
  * @param newOldEventIds map between the new eventIds and the previous ones.
  * @param eventIdToRemove removed event id.
  * @param eventDescriptionToRemove removed event description.
  */
    void eventDescriptionRemoved(QString providerName,QMap<int,int> oldNewEventIds,QMap<int,int> newOldEventIds,int eventIdToRemove,QString eventDescriptionToRemove);

private:

    /**Provider's name.*/
    QString name;

    /**Sampling rate used in the current file containing the data in miliseconds.*/
    double currentSamplingRate;

    /**A 1 line array containing the event times.*/
    Array<double> timeStamps;

    /**A 1 line array containing the event ids.*/
    pArray<EventDescription> events;

    /**A 1 line array containing the event times for an undo action.*/
    Array<double> timeStampsUndo;

    /**A 1 line array containing the event ids for an undo action.*/
    pArray<EventDescription> eventsUndo;

    /**A 1 line array containing the event times for an redo action.*/
    Array<double> timeStampsRedo;

    /**A 1 line array containing the event ids for an redo action.*/
    pArray<EventDescription> eventsRedo;

    /**The start time for the previously requested data.*/
    long previousStartTime;

    /**The end time for the previously requested data.*/
    long previousEndTime;

    /**The start index for the previously requested data.*/
    long previousStartIndex;

    /**The end index for the previously requested data.*/
    long previousEndIndex;

    /**Number of events in the event file the provider provides the data for.*/
    long nbEvents;

    /**Map given a map between the event description and an numeric identifier.*/
    QMap<EventDescription,int> eventIds;

    /**Map given a map between an numeric identifier and the event description.*/
    QMap<int,EventDescription> idsDescriptions;

    /**The value to use as the length for the event descriptions in the event palette.*/
    int descriptionLength;

    /**The maximum time, in miliseconds, contained in the file.*/
    long fileMaxTime;

    /**Represents the percentage from the begining of the window where the events are display when browsing.*/
    float eventPosition;

    /**Flag to keep track of event modifications. */
    bool modified;

    /**Counter for each type of event.*/
    QMap<EventDescription,int> eventDescriptionCounter;

    /**Counter for each type of event for an undo action.*/
    QMap<EventDescription,int> eventDescriptionCounterUndo;

    /**Counter for each type of event for an rdo action.*/
    QMap<EventDescription,int> eventDescriptionCounterRedo;

    //Functions

    /**Retrieves the peak index of each spike included in the time frame given by @p startTime and @p endTime.
  * @param startTime begining of the time frame from which to retrieve the data, given in milisecond.
  * @param endTime end of the time frame from which to retrieve the data, given in milisecond.
  * @param initiator instance requesting the data.
  */
    void retrieveData(long startTime,long endTime,QObject* initiator);

    /**Finds the event index corresponding to the given time and event id.
  * @param eventTime time to look up.
  * @param eventId id of the event to look up.
  */
    long findIndex(double eventTime,int eventId = -1);

    /** Creates a new description event.
  *  @param eventDescriptionToAdd event description to add.
  */
    void addEventDescription(QString eventDescriptionToAdd);

    /** removes a description event.
  *  @param eventDescriptionToRemove event description to remove.
  */
    void removeEventDescription(QString eventDescriptionToRemove);
};


#endif
