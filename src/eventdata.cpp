/***************************************************************************
                          eventdata.cpp  -  description
                             -------------------
    begin                : Wen Feb 16 2005
    copyright            : (C) 2005 by Lynn Hazan
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

//include files for the application
#include "eventdata.h"

//QT include files
#include <QObject>

//include files for c/c++ libraries
#include <math.h>

void EventData::computePositions(double samplingRate,double positionSamplingRate,long startTime){

    //The first position does not necessarily correspond to startTime, the event times have to be adjusted to take the difference into account.
    long startInRecordingUnits = static_cast<dataType>(ceil(static_cast<float>(static_cast<double>(startTime) * static_cast<double>(static_cast<double>(positionSamplingRate) / static_cast<double>(1000)))));
    long difference = static_cast<dataType>(ceil(static_cast<float>(1000 * static_cast<double>(startInRecordingUnits)/positionSamplingRate))) - startTime;

    double samplingRateInMs = static_cast<double>(static_cast<double>(samplingRate) / 1000.0);
    double positionSamplingInterval = 1000.0 / positionSamplingRate;

    int nbEvents = times.nbOfColumns();
    positions.setSize(1,nbEvents);

    for(int i = 1; i <= nbEvents;++i){
        dataType index = times(1,i);
        double time = static_cast<double>(static_cast<double>(index) / samplingRateInMs);

        //Positions start at 1.
        dataType position = static_cast<dataType>(floor(static_cast<float>(0.5 + (static_cast<double>(time - difference) / positionSamplingInterval)))) + 1;

        positions(1,i) = qMax(position,1L);
    }
}


