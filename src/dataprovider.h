/***************************************************************************
                          dataprovider.h  -  description
                             -------------------
    begin                : Mon Mar 1 2004
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

#ifndef DATAPROVIDER_H
#define DATAPROVIDER_H

// include files for QT
#include <QObject> 




/**
  * Base class for all the types of provider of data: TracesProvider,ClustersProvider, EventsProvider,
  * PositionProvider.
  *@author Lynn Hazan
  */

class DataProvider : public QObject{
    Q_OBJECT
public:
    /**Constructor.
  * @param fileUrl the url of the file containing the data provided by this class.
  */
    DataProvider(const QString &fileUrl);
    virtual ~DataProvider();

    /**Triggers the retrieve of the traces included in the time interval given by @p startTime and @p endTime.
  * @param startTime begining of the time interval from which to retrieve the data.
  * @param endTime end of the time interval from which to retreive the data.
  * @param initiator instance requesting the data.
  * @param startTimeInRecordingUnits begining of the time interval from which to retrieve the data in recording units.
  */
    virtual void requestData(long startTime,long endTime,QObject* initiator,long startTimeInRecordingUnits=0){}

    /**Enables the caller to know if there is any thread running launch by the provider.*/
    virtual inline bool isThreadsRunning(){return false;}

protected:
    /**The name of the file containing the data provided by this class.*/
    QString fileName;

};

#endif
