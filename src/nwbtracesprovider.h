
/***************************************************************************
                          nwbtracesprovider.h  -  description
                             -------------------

    Processes Neurodata Without Borders data .nwb
    Adapted by Robert H. Moore (RHM) for Dr. Ben Dichter.

    Adapted from nsxtracesprovider.h:
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

#ifndef NWBTRACESPROVIDER_H
#define NWBTRACESPROVIDER_H


//include files for the application
#include "alttracesprovider.h"
#include "array.h"
#include "types.h"


// include files for QT
#include <QObject>
#include <QtDebug>

/** Class providing the row recorded data (contained in a .nwb file).
  * @author Florian Franzen - adapted by Robert H. Moore
  */

class NWBTracesProvider : public AltTracesProvider  {
    Q_OBJECT
public:

    /**Constructor.
    * @param fileUrl the url of the file containing the data provided by this class.
    */
    NWBTracesProvider(const QString &fileUrl);
    virtual ~NWBTracesProvider();

    /** Initializes the object by reading the nwb files header.
    */
    virtual bool init();

    /**Computes the number of samples between @p start and @p end.
    * @param start begining of the time frame from which the data have been retrieved, given in milisecond.
    * @param end end of the time frame from which to retrieve the data, given in milisecond.
    * @return number of samples in the given time frame.
    * @param startTimeInRecordingUnits begining of the time frame from which the data have been retrieved, given in recording units.
    */
    virtual long getNbSamples(long start, long end, long startInRecordingUnits);

    /** Return the labels of each channel as read from nwb file. */
    virtual QStringList getLabels();

Q_SIGNALS:
    /**Signals that the data have been retrieved.
    * @param data array of data in uV (number of channels X number of samples).
    * @param initiator instance requesting the data.
    */
    void dataReady(Array<dataType>& data,QObject* initiator);

protected:
    virtual QString qsShortName() {return "NWB";}

private:

    /**Retrieves the traces included in the time frame given by @p startTime and @p endTime.
    * @param startTime begining of the time frame from which to retrieve the data, given in milisecond.
    * @param endTime end of the time frame from which to retrieve the data, given in milisecond.
    * @param initiator instance requesting the data.
    * @param startTimeInRecordingUnits begining of the time interval from which to retrieve the data in recording units.
    */
    virtual void retrieveData(long startTime, long endTime, QObject* initiator, long startTimeInRecordingUnits);

};



#endif // NWBTRACESPROVIDER_H
