/***************************************************************************
            cerebustracesprovider.h  -  description
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

#ifndef CEREBUSTRACESPROVIDER_H
#define CEREBUSTRACESPROVIDER_H

// Include Qt Library files
#include <QObject>
#include <QMutex>
#include <QtDebug>

// Include cerebus sdk
#include <cbsdk.h>

// Include project files
#include "tracesprovider.h"
#include "types.h"


/** CerebusTracesProvider uses a Blackrock Cerebus NSP as data source.
  *
  * Continous recorded channels are grouped into so called sampling groups based
  * on their sampling rate. This class allows you to use one of these sampling
  * group as data source for traces.
  *
  * If the number of channels in the sampling group change, the is currently no
  * way to let the GUI know.
  *
  * @author Florian Franzen
  */

class CerebusTracesProvider : public TracesProvider  {
    Q_OBJECT

public:
    // Callback that reacts to new data
    static void packageCallback(UINT32 instance, const cbSdkPktType type,  const void* data, void* object);

    // Different sampling groups one can describe to
	enum SamplingGroup {
		RATE_500 = 1,
		RATE_1K = 2,
		RATE_2K = 3,
		RATE_10k = 4,
		RATE_30k = 5
	};

    /**
    * @param group is the sampling group to subscribe to.
    */
	CerebusTracesProvider(SamplingGroup group);
	virtual ~CerebusTracesProvider();

    /** Initializes the object by trying to connect to the Cerebus NSP.
    *
    * Fails on connection error or when there are no channels in sampling group.
    */
    bool init();

    /**Computes the number of samples between @p start and @p end.
    * @param start begining of the time frame from which the data have been retrieved, given in milisecond.
    * @param end end of the time frame from which to retrieve the data, given in milisecond.
    * @return number of samples in the given time frame.
    * @param startTimeInRecordingUnits begining of the time frame from which the data have been retrieved, given in recording units.
    */
    virtual long getNbSamples(long start, long end, long startInRecordingUnits);

    // Called by callback to add data to buffer.
    void processData(const cbPKT_GROUP* package);

    // Called by callback to process configuration changes.
    void processConfig(const cbPKT_GROUPINFO* package);

	// Return last error message as string
	std::string getLastErrorMessage();

    /** Dummy function, definded to work around the bad interface design.
    * @param nb the number of channels.
    */
    virtual void setNbChannels(int nb){
        qDebug() << "Cerebus NSP used. Ignoring setNbChannels(" << nb << ")";
    }

    /** Dummy function, definded to work around the bad interface design.
    * @param res resolution.
    */
    virtual void setResolution(int res){
        qDebug() << "Cerebus NSP used.  Ignoring setResolution(" << res << ")";
    }

    /** Dummy function, definded to work around the bad interface design.
    * @param rate the sampling rate.
    */
    virtual void setSamplingRate(double rate){
        qDebug() << "Cerebus NSP used. Ignoring setSamplingRate(" << rate << ")";
    }

    /** Dummy function, definded to work around the bad interface design.
    * @param range the voltage range.
    */
    virtual void setVoltageRange(int range){
        qDebug() << "Cerebus NSP used. Ignoring setVoltageRange(" << range << ")";
    }

    /**  Dummy function, definded to work around the bad interface design.
    * @param value the amplification.
    */
    virtual void setAmplification(int value){
        qDebug() << "Cerebus NSP used. Ignoring setAmplification(" << value << ")";
    }

    /** Called when paging is started.
     *  Recouples the buffer that is updated with the one that is
     *  viewed/returened.
     *  This essentially unpauses the signal being played and shows the
     *  live signal again.
     */
    virtual void slotPagingStarted();

    /** Called when paging is stopped.
    * Decouples the buffer that is viewed/returned from the one updated.
    * This essentialy pauses the signal that is being displayed.
    */
    virtual void slotPagingStopped();

Q_SIGNALS:
    /**Signals that the data have been retrieved.
    * @param data array of data in uV (number of channels X number of samples).
    * @param initiator instance requesting the data.
    */
    void dataReady(Array<dataType>& data, QObject* initiator);

private:
    // Resolution of data packages received.
    static const int CEREBUS_RESOLUTION ;
    // Default instance id to use to talk to CB SDK
    static const unsigned int CEREBUS_INSTANCE;
    // Length of buffer in seconds
    static const unsigned int BUFFER_SIZE;
    // Sampling rate of each sampling group
    static const unsigned int SAMPLING_RATES[6];

    // True if connection, buffers and callback were initialized
    bool mInitialized;
	// True if set of channels in sampling group were changed
	bool mReconfigured;

    // Sampling group to listen to
    SamplingGroup mGroup;
    // List of scalings for each channel
    cbSCALING* mScales;
    // List of NSP channel numbers we are listing to
    UINT32* mChannels;

    // Capacity of buffer (see data and paused_data)
    size_t mCapacity;

    // Return value of last CBSDK library call
    int mLastResult;

    // Data storage mutex
    QMutex mMutex;

    // Data storage
    INT16* mLiveData;
    size_t* mLivePosition;
    INT16* mViewData;
    size_t* mViewPosition;

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
