/***************************************************************************
 *   Copyright (C) 2004 by Lynn Hazan                                      *
 *   lynn@myrealbox.com                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
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

#include "globaleventsprovider.h"

void GlobalEventsProvider::requestData(long startTime,long endTime,QObject* initiator){
    emit getCurrentEventInformation(startTime,endTime,initiator);
}

void GlobalEventsProvider::eventInformationAvailable(QHash<QString, EventData*>& eventsData,QMap<QString, QList<int> >& selectedEvents,QHash<QString, ItemColors*>& providerItemColors,QObject* initiator)
{
    emit eventsAvailable(eventsData,selectedEvents,providerItemColors);
}

