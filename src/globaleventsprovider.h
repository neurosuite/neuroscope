/***************************************************************************
 *   Copyright (C) 2004 by Lynn Hazan                                      *
 *   lynn@myrealbox.com                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
#ifndef GLOBALEVENTSPROVIDER_H
#define GLOBALEVENTSPROVIDER_H

// include files for QT
#include <q3dict.h> 
//Added by qt3to4:
#include <QList>
#include <QMap>
//include files for the application
#include <dataprovider.h>
#include <types.h>
#include "eventdata.h"

// include files for KDE


class ItemColors;

/**
@author Lynn Hazan
*/
class GlobalEventsProvider : public DataProvider
{
    Q_OBJECT
public:
    GlobalEventsProvider():DataProvider(QString()){}
    ~GlobalEventsProvider(){}

    /**Triggers the retrieve of the events included in the time interval given by @p startTime and @p endTime.
  * @param startTime begining of the time interval from which to retrieve the data in miliseconds.
  * @param endTime end of the time interval from which to retrieve the data.
  * @param initiator instance requesting the data.
  */
    inline void requestData(long startTime,long endTime,QObject* initiator){
        emit getCurrentEventInformation(startTime,endTime,initiator);
    }

Q_SIGNALS:
    void getCurrentEventInformation(long startTime,long endTime,QObject* initiator);

    /**Informs that data of the selected events providers corresponding to current time frame are available.
  * @param eventsData dictionary between the event provider names and the event data and status.
  * @param selectedEvents map between the event provider names and the list of currently selected events.
  * @param providerItemColors dictionary between the provider names and the item color lists.
  */
    void eventsAvailable(Q3Dict<EventData>& eventsData,QMap<QString, QList<int> >& selectedEvents,Q3Dict<ItemColors>& providerItemColors);


public Q_SLOTS:
    /**Informs that data of the selected events providers corresponding to current time frame are available.
  * @param eventsData dictionary between the event provider names and the event data and status.
  * @param selectedEvents map between the event provider names and the list of currently selected events.
  * @param providerItemColors dictionary between the provider names and the item color lists.
  * @param initiator instance requesting the data.
  */
    inline void eventInformationAvailable(Q3Dict<EventData>& eventsData,QMap<QString, QList<int> >& selectedEvents,Q3Dict<ItemColors>& providerItemColors,QObject* initiator){
        emit eventsAvailable(eventsData,selectedEvents,providerItemColors);
    }
    
};

#endif
