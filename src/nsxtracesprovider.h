/***************************************************************************
                          nsxtracesprovider.h  -  description
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

#ifndef NSXTRACESPROVIDER_H
#define NSXTRACESPROVIDER_H

//include files for the application
#include "tracesprovider.h"
#include "array.h"
#include "types.h"
#include "blackrock.h"

// include files for QT
#include <QObject>
#include <QtDebug>

/** Class providing the row recorded data (contained in a .nsx file).
  * @author Florian Franzen
  */

class NSXTracesProvider : public TracesProvider  {
    Q_OBJECT
public:

    /**Constructor.
    * @param fileUrl the url of the file containing the data provided by this class.
    */
    NSXTracesProvider(const QString &fileUrl);
	virtual ~NSXTracesProvider();

    /**Sets the number of channels corresponding to the file identified by fileUrl.
    * @param nb the number of channels.
    */
    virtual void setNbChannels(int nb){
        qDebug() << "NSX file used. Ignoring setNbChannels(" << nb << ")";
    }

    /**Sets the resolution used to record the data contained in the file identified by fileUrl.
    * @param res resolution.
    */
    virtual void setResolution(int res){
        qDebug() << "NSX file used.  Ignoring setResolution(" << res << ")";
    }

    /**Sets the sampling rate used to record the data contained in the file identified by fileUrl.
    * @param rate the sampling rate.
    */
    virtual void setSamplingRate(double rate){
        qDebug() << "NSX file used. Ignoring setSamplingRate(" << rate << ")";
    }

    /**Sets the voltage range used to record the data contained in the file identified by fileUrl.
    * @param range the voltage range.
    */
    virtual void setVoltageRange(int range){
        qDebug() << "NSX file used. Ignoring setVoltageRange(" << range << ")";
    }

    /**Sets the amplification used to record the data contained in the file identified by fileUrl.
    * @param value the amplification.
    */
    virtual void setAmplification(int value){
        qDebug() << "NSX file used. Ignoring setAmplification(" << value << ")";
    }

    /** Initializes the object by reading the nsx files header.
    */
    bool init();

    /**Computes the number of samples between @p start and @p end.
    * @param start begining of the time frame from which the data have been retrieved, given in milisecond.
    * @param end end of the time frame from which to retrieve the data, given in milisecond.
    * @return number of samples in the given time frame.
    * @param startTimeInRecordingUnits begining of the time frame from which the data have been retrieved, given in recording units.
    */
    virtual long getNbSamples(long start, long end, long startInRecordingUnits);

    /** Return the labels of each channel as read from nsx file. */
    virtual QMap<int, QString> getLabels();

Q_SIGNALS:
    /**Signals that the data have been retrieved.
    * @param data array of data in uV (number of channels X number of samples).
    * @param initiator instance requesting the data.
    */
    void dataReady(Array<dataType>& data,QObject* initiator);

private:
    static const int NSX_RESOLUTION;
    static const int NSX_OFFSET;

    // Headers that are parsed from the file to open
    NSXBasicHeader mBasicHeader;
    NSXExtensionHeader* mExtensionHeaders;
    NSXDataHeader mDataHeader;

    // Start of first sample in first data package.
    long mDataFilePos;
    bool mInitialized;

    //Functions

    /**Retrieves the traces included in the time frame given by @p startTime and @p endTime.
    * @param startTime begining of the time frame from which to retrieve the data, given in milisecond.
    * @param endTime end of the time frame from which to retrieve the data, given in milisecond.
    * @param initiator instance requesting the data.
    * @param startTimeInRecordingUnits begining of the time interval from which to retrieve the data in recording units.
    */
    virtual void retrieveData(long startTime, long endTime, QObject* initiator, long startTimeInRecordingUnits);

    /**Computes the total length of the document in miliseconds.*/
    virtual void computeRecordingLength();
};

#endif
