/***************************************************************************
                          eventdata.h  -  description
                             -------------------
    begin                : Tus Feb 15 2005
    copyright            : (C) 2005 by Lynn Hazan
    email                : lynn.hazan.myrealbox.com
 ***************************************************************************/
/***************************************************************************                                                                     *
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
#ifndef EVENTDATA_H
#define EVENTDATA_H

//include files for the application
#include "array.h"
#include "types.h"

//General C++ include files
#include <iostream>
using namespace std;


/**Class representing the data of an event file for a given starting time and timeframe.
@author Lynn Hazan
*/
class EventData{
public:
    /**Constructor.
   * @param d Array containing the events ids.
   * @param t Array containing the time indexes of the events relative to the current starting time. The indexes are computed using the currently open data file sampling rate.
   * @param status status of the data, true if the data are available, false otherwise.
   */
    inline EventData(Array<int> d,Array<dataType> t,bool status){
        times = t;
        ids = d;
        ready = status;
    }

    inline EventData(){
        ready = false;
    }
    
    inline EventData& operator=(const EventData& source){
        if(&source != this){
            ready = source.ready;
            times = source.times;
            ids = source.ids;
        }
        return *this;
    }

    inline void setStatus(bool status){ready = status;}
    inline void setData(Array<dataType>& t,Array<int>& d){
        times = t;
        ids = d;
    }
    /**Returns the true if the data are available, false otherwise.*/
    inline bool status(){return ready;}

    /**Returns an Array containing the events ids.*/
    inline Array<int>& getIds(){return ids;}

    /**Returns an Array containing the time indexes of the events relative to the current starting time.
   * The indexes are computed using the currently open data file sampling rate.*/
    inline Array<dataType>& getTimes(){return times;}

    /**
   * @param samplingRate sampling rate of the current open data file in Hz.
   * @param positionSamplingRate sampling rate of the position file in Hz.
   * @param startTime current start time of the time window in milisecond.
   */
    void computePositions(double samplingRate,double positionSamplingRate,long startTime);

    /**Returns an Array containing the positions indexes computed for the events.
   * The positions indexes are computed using the position file sampling rate.*/
    inline Array<dataType>& getPositions(){return positions;}

    
private:    
    Array<dataType> times;
    Array<int> ids;
    bool ready;
    Array<dataType> positions;
};

#endif
