/***************************************************************************
                          AltTracesProvider.h  -  description
                             -------------------

    Processes alternative to the standard Neuroscope TracesProvider.
    Will be the base for a derived Neurodata Without Borders data .nwb provider.
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

#ifndef ALTTRACESPROVIDER_H
#define ALTTRACESPROVIDER_H


//include files for the application
#include "tracesprovider.h"
#include "array.h"
#include "types.h"


// include files for QT
#include <QObject>
#include <QtDebug>

/** Class providing generic support for an Alterate Traces Provider.
  * @author Florian Franzen - adapted by Robert H. Moore
  */

class AltTracesProvider : public TracesProvider  {
    Q_OBJECT
public:

    /**Constructor.
    * @param fileUrl the url of the file containing the data provided by this class.
    */
   // AltTracesProvider(const QString &fileUrl)
    AltTracesProvider(const QString &fileUrl, int nbChannels, int resolution, int voltageRange, int amplification, double samplingRate, int offset)
        :TracesProvider(fileUrl, nbChannels, resolution, voltageRange, amplification, samplingRate, offset){}
    virtual ~AltTracesProvider(){}

    /**Sets the number of channels corresponding to the file identified by fileUrl.
    * @param nb the number of channels.
    */
    virtual void setNbChannels(int nb){
        qDebug() << qsShortName() << " file used. Ignoring setNbChannels(" << nb << ")";
    }

    /**Sets the resolution used to record the data contained in the file identified by fileUrl.
    * @param res resolution.
    */
    virtual void setResolution(int res){
        qDebug() << qsShortName() << " file used.  Ignoring setResolution(" << res << ")";
    }

    /**Sets the sampling rate used to record the data contained in the file identified by fileUrl.
    * @param rate the sampling rate.
    */
    virtual void setSamplingRate(double rate){
        qDebug() << qsShortName() << " file used. Ignoring setSamplingRate(" << rate << ")";
    }

    /**Sets the voltage range used to record the data contained in the file identified by fileUrl.
    * @param range the voltage range.
    */
    virtual void setVoltageRange(int range){
        qDebug() << qsShortName() << " file used. Ignoring setVoltageRange(" << range << ")";
    }

    /**Sets the amplification used to record the data contained in the file identified by fileUrl.
    * @param value the amplification.
    */
    virtual void setAmplification(int value){
        qDebug() << qsShortName() << " file used. Ignoring setAmplification(" << value << ")";
    }

    /** Initializes the object by reading the -- files header.
    */
    virtual bool init(){return false;}

    /**Computes the number of samples between @p start and @p end.
    * @param start begining of the time frame from which the data have been retrieved, given in milisecond.
    * @param end end of the time frame from which to retrieve the data, given in milisecond.
    * @return number of samples in the given time frame.
    * @param startTimeInRecordingUnits begining of the time frame from which the data have been retrieved, given in recording units.
    */
    virtual long getNbSamples(long /*start*/, long /*end*/, long /*startInRecordingUnits*/) {return 0;}

    /** Return the labels of each channel as read from --- file. */
    virtual QStringList getLabels() {QStringList labels; return labels;}

Q_SIGNALS:
    /**Signals that the data have been retrieved.
    * @param data array of data in uV (number of channels X number of samples).
    * @param initiator instance requesting the data.
    */
    virtual void dataReady(Array<dataType>& data,QObject* initiator);

protected:
    virtual QString qsShortName() {return "ALT";}
    bool mInitialized;

private:

};



#endif // ALTTRACESPROVIDER_H
