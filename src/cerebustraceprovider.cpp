/***************************************************************************
            cerebustracesprovider.cpp  -  description
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

#include "cerebustraceprovider.h"

#include <string>

#include <QDebug>

#include "cerebusclustersprovider.h"

// TODO: Fix this ugly hack by implementing our own error return value.
#define CBSDKRESULT_EMPTYSAMPLINGGROUP -50

const int CerebusTracesProvider::CEREBUS_RESOLUTION = 16;
const unsigned int CerebusTracesProvider::CEREBUS_INSTANCE = 0;
const unsigned int CerebusTracesProvider::BUFFER_SIZE = 10;
const unsigned int CerebusTracesProvider::SAMPLING_RATES[] = { 0, 500, 1000, 2000, 10000, 30000 };

CerebusTracesProvider::CerebusTracesProvider(SamplingGroup group) :
		TracesProvider("", -1, CEREBUS_RESOLUTION, 0, 0, 0, 0),
		mGroup(group),
		mInitialized(false),
		mReconfigured(false),
		mScales(NULL),
		mChannels(NULL),
        mLiveTime(NULL),
        mViewTime(NULL),
        mLiveTraceData(NULL),
        mLiveTracePosition(NULL),
        mViewTraceData(NULL),
        mViewTracePosition(NULL),
        mLiveClusterTime(NULL),
        mLiveClusterID(NULL),
        mLiveClusterPosition(NULL),
        mViewClusterTime(NULL),
        mViewClusterID(NULL),
        mViewClusterPosition(NULL) {

	// The sampling rate is hardwired to the sampling group
	this->samplingRate = SAMPLING_RATES[group];
	mTraceCapacity = BUFFER_SIZE * this->samplingRate;
    mEventCapacity = BUFFER_SIZE * cbSdk_TICKS_PER_SECOND;

	// The buffer always has the same size
	this->length = 1000 * BUFFER_SIZE;
}

CerebusTracesProvider::~CerebusTracesProvider() {
	if (mInitialized) {
		// Disabled callbacks, close network thread and connection
		cbSdkClose(CEREBUS_INSTANCE);

		// Free uninitalize internal structures
		delete[] mScales;
		delete[] mChannels;

        if(mViewTraceData != mLiveTraceData) {
            // We are in paused mode
            delete mViewTime;

            delete[] mViewTraceData;
            delete mViewTracePosition;

            for(int i = 0; i < this->nbChannels; i++) {
                delete[] mViewClusterTime[i];
                delete[] mViewClusterID[i];
                delete mViewClusterPosition[i];
            }

            delete[] mViewClusterTime;
            delete[] mViewClusterID;
            delete[] mViewClusterPosition;
        }
        delete mLiveTime;

        delete[] mLiveTraceData;
        delete mLiveTracePosition;

        for(int i = 0; i < this->nbChannels; i++) {
            delete[] mLiveClusterTime[i];
            delete[] mLiveClusterID[i];
            delete mLiveClusterPosition[i];
        }

        delete[] mLiveClusterTime;
        delete[] mLiveClusterID;
        delete[] mLiveClusterPosition;
	}
}

bool CerebusTracesProvider::init() {
	if (mInitialized)
		return true;

	// Open connection to NSP
	mLastResult = cbSdkOpen(CEREBUS_INSTANCE);

	if (mLastResult != CBSDKRESULT_SUCCESS)
		return false;

	// Get number of channels in sampling group
	mLastResult = cbSdkGetSampleGroupList(CEREBUS_INSTANCE, 1, mGroup, (UINT32 *) &this->nbChannels, NULL);

	if (mLastResult != CBSDKRESULT_SUCCESS) {
		cbSdkClose(CEREBUS_INSTANCE);
		return false;
	}

	if (this->nbChannels == 0) {
		mLastResult = CBSDKRESULT_EMPTYSAMPLINGGROUP;
		cbSdkClose(CEREBUS_INSTANCE);
		return false;
	}

	// Get the list of channels in this sample group
	mChannels = new UINT32[this->nbChannels];
	mLastResult = cbSdkGetSampleGroupList(CEREBUS_INSTANCE, 1, mGroup, NULL, mChannels);

	if (mLastResult != CBSDKRESULT_SUCCESS) {
		delete[] mChannels;
		cbSdkClose(CEREBUS_INSTANCE);
		return false;
	}

	// Get scaling for each channel of group
	mScales = new cbSCALING[this->nbChannels];

	cbPKT_CHANINFO info;
	for (unsigned int i = 0; i < this->nbChannels; i++)
	{
		mLastResult = cbSdkGetChannelConfig(CEREBUS_INSTANCE, mChannels[i], &info);

		if (mLastResult != CBSDKRESULT_SUCCESS) {
			delete[] mChannels;
			delete[] mScales;
			cbSdkClose(CEREBUS_INSTANCE);
			return false;
		}

		// Copy only physcal input scaling for now
		mScales[i] = info.physcalin;
	}

    // Lock mutex to initalize cross-thread data structure.
    mMutex.lock();

	// Register data callback
	mLastResult = cbSdkRegisterCallback(CEREBUS_INSTANCE, CBSDKCALLBACK_CONTINUOUS, packageCallback, this);

	if (mLastResult != CBSDKRESULT_SUCCESS) {
		delete[] mChannels;
		delete[] mScales;
		cbSdkClose(CEREBUS_INSTANCE);
		return false;
	}

    // Register spike event callback
    mLastResult = cbSdkRegisterCallback(CEREBUS_INSTANCE, CBSDKCALLBACK_SPIKE, packageCallback, this);

    if (mLastResult != CBSDKRESULT_SUCCESS) {
    	delete[] mChannels;
    	delete[] mScales;
    	cbSdkClose(CEREBUS_INSTANCE);
    	return false;
    }

	// Register config callback
	mLastResult = cbSdkRegisterCallback(CEREBUS_INSTANCE, CBSDKCALLBACK_GROUPINFO, packageCallback, this);

	if (mLastResult != CBSDKRESULT_SUCCESS) {
		delete[] mChannels;
		delete[] mScales;
		cbSdkClose(CEREBUS_INSTANCE);
		return false;
	}

    // Allocate time storage and request system time
    mLiveTime = new UINT32(0);

    mLastResult = cbSdkGetTime(CEREBUS_INSTANCE, mLiveTime);
    if (mLastResult != CBSDKRESULT_SUCCESS) {
        delete[] mChannels;
        delete[] mScales;
        delete mLiveTime;
        cbSdkClose(CEREBUS_INSTANCE);
        return false;
    }

	// Allocate live sample buffer
	mLiveTraceData = new INT16[this->nbChannels * mTraceCapacity];
	memset(mLiveTraceData, 0, this->nbChannels * mTraceCapacity * sizeof(INT16));
    mLiveTracePosition = new size_t(0);

    // Allocate live event buffer
    mLiveClusterTime     = new UINT32*[this->nbChannels];
    mLiveClusterID       = new UINT8*[this->nbChannels];
    mLiveClusterPosition = new size_t*[this->nbChannels];

    for(int i = 0; i < this->nbChannels; i++) {
        mLiveClusterTime[i] = new UINT32[mEventCapacity];
        memset(mLiveClusterTime[i], 0, mEventCapacity * sizeof(UINT32));
        mLiveClusterID[i] = new UINT8[mEventCapacity];
        memset(mLiveClusterID[i], 0, mEventCapacity * sizeof(UINT8));
        mLiveClusterPosition[i] = new size_t(0);
    }

    // Link view buffer to live sample buffer
    mViewTime = mLiveTime;

    mViewTraceData = mLiveTraceData;
    mViewTracePosition = mLiveTracePosition;

    mViewClusterTime = mLiveClusterTime;
    mViewClusterID = mLiveClusterID;
    mViewClusterPosition = mLiveClusterPosition;

    // We are done.
	mInitialized = true;
    mMutex.unlock();
	return true;
}

void CerebusTracesProvider::processData(const cbPKT_GROUP* package) {
	// Ignore package if for different sampling group
	if (package->type != mGroup)
		return;

	mMutex.lock();

	// Channels were reconfigured, data size has changed and it is not save to copy data anymore.
	if (mReconfigured) {
		mMutex.unlock();
		return;
	}

    // Update system time (only updated here, because events are always returned in relation to trace window)
    (*mLiveTime) = package->time;

	// Copy sampled to history
	memcpy(mLiveTraceData + ((*mLiveTracePosition) * this->nbChannels), package->data, this->nbChannels * sizeof(INT16));

	// Adjust position, wrap around if necessary.
	(*mLiveTracePosition)++;
	if ((*mLiveTracePosition) == mTraceCapacity) (*mLiveTracePosition) = 0;
	mMutex.unlock();
}

void CerebusTracesProvider::processSpike(const cbPKT_SPK* package) {
	// Check if spike event was triggered by member of sampling group
    int channelIndex = -1;
    for(int i = 0; i < this->nbChannels; i++) {
        if(mChannels[i] == package->chid) {
            channelIndex = i;
            break;
        }
    }
	if (channelIndex == -1)
		return;

	mMutex.lock();

	// Channels were reconfigured and we now might receive data from channels we don't know about.
	// (No harm, but also no point to continue. We are going to abort soon anyway.)
	if (mReconfigured) {
		mMutex.unlock();
		return;
	}

    // Copy event data to history
    mLiveClusterTime[channelIndex][*mLiveClusterPosition[channelIndex]] = package->time;
    mLiveClusterID[channelIndex][*mLiveClusterPosition[channelIndex]]   = package->unit;

    // Adjust position, wrap around if necessary.
    (*mLiveClusterPosition[channelIndex])++;
    if ((*mLiveClusterPosition[channelIndex]) == mEventCapacity) {
        (*mLiveClusterPosition[channelIndex]) = 0;
    }

	mMutex.unlock();
}

void CerebusTracesProvider::processConfig(const cbPKT_GROUPINFO* package) {
	// Ignore package if for different sampling group or channel count did not change
	if (package->group != mGroup)
		return;

	mMutex.lock();

	// Tell all threads that from now on the config has changed.
	mReconfigured = true;

	mMutex.unlock();
}

void CerebusTracesProvider::packageCallback(UINT32 /*instance*/, const cbSdkPktType type, const void* data, void* object) {
	CerebusTracesProvider* provider = reinterpret_cast<CerebusTracesProvider*>(object);

    if(provider && data) {
        switch (type) {
    	case cbSdkPkt_CONTINUOUS:
            provider->processData(reinterpret_cast<const cbPKT_GROUP*>(data));
            break;
        case cbSdkPkt_SPIKE:
    		provider->processSpike(reinterpret_cast<const cbPKT_SPK*>(data));
            break;
        case cbSdkPkt_GROUPINFO:
    		provider->processConfig(reinterpret_cast<const cbPKT_GROUPINFO*>(data));
            break;
        case cbSdkPkt_PACKETLOST:
            // TODO: Take care of package lost here!
            qWarning() << "Cerebus SDK: Package lost detected!";
            break;
        }
    }
}

long CerebusTracesProvider::getNbSamples(long start, long end, long startInRecordingUnits) {
	// Check if startInRecordingUnits was supplied, else compute it.
	if (startInRecordingUnits == 0)
		startInRecordingUnits = this->samplingRate * start / 1000.0;

	// The caller should have check that we do not go over the end of the file.
	// The recording starts at time equals 0 and ends at length of the file minus one.
	// Therefore the sample at endInRecordingUnits is never returned.
	long endInRecordingUnits = (this->samplingRate * end / 1000.0);

	return endInRecordingUnits - startInRecordingUnits;
}


void CerebusTracesProvider::retrieveData(long start, long end, QObject* initiator, long startInRecordingUnits) {
	Array<dataType> result;

	// Abort if not initalized
	if (!mInitialized) {
		emit dataReady(result, initiator);
		return;
	}

	// Check if startInRecordingUnits was supplied, else compute it.
	if (startInRecordingUnits == 0)
		startInRecordingUnits = this->samplingRate * start / 1000.0;

	// The caller should have check that we do not go over the end of the file.
	// The recording starts at time equals 0 and ends at length of the file minus one.
	// Therefore the sample at endInRecordingUnits is never returned.
	long endInRecordingUnits = (this->samplingRate * end / 1000.0);

	long lengthInRecordingUnits = endInRecordingUnits - startInRecordingUnits;

	// Data structered used past this line are accessed by multiple threads
	mMutex.lock();

	// If config was changed, this is the only way we can tell Neuroscope to abort.
	// Make sure to only abort if not in pause mode!
	if (mReconfigured && mViewTraceData == mLiveTraceData) {
		mMutex.unlock();
		qDebug() << "Recofiguration not supported. Please reopen connection.";
		emit dataReady(result, initiator);
		return;
	}

	// Allocate result array
	result.setSize(lengthInRecordingUnits, this->nbChannels);

	// Copy and convert data from buffer.
	for (int channel = 0; channel < this->nbChannels; channel++) {
		// Compute start in relation to current ringbuffer position
		size_t offset = startInRecordingUnits + (*mViewTracePosition);

		// Determine unit data is saved in
		int unit_correction = 0;
		char* unit_string = mScales[channel].anaunit;
		if (!strncmp(unit_string, "uV", 16)) {
			unit_correction = 1;
		}
		else if (!strncmp(unit_string, "mV", 16)) {
			unit_correction = 1000;
		}
		else {
			qWarning() << "unknown unit for channel " << channel << ": " << unit_string;
			continue;
		}

		// Get all the values needed to translate measurement unit to uV
		int min_digital = mScales[channel].digmin;
		int range_digital = mScales[channel].digmax - min_digital;
		int min_analog = mScales[channel].anamin;
		int range_analog = mScales[channel].anamax - min_analog;

		// TODO: Add gain (anagain) to calculation, as soon as blackrock documents how it is supposed to be used.

		// Get all the samples for this channel
		for (int i = 0; i < lengthInRecordingUnits; i++) {
			size_t absolute_position = offset + i;
			// Wrap around in case we reach end of buffer
			if (absolute_position >= mTraceCapacity) {
				absolute_position -= mTraceCapacity;
				offset -= mTraceCapacity;
			}
			// Scale data using channel scaling
			size_t index = (absolute_position * this->nbChannels) + channel;
			result(i + 1, channel + 1) = static_cast<dataType>((((static_cast<double>(mViewTraceData[index]) - min_digital) / range_digital) * range_analog + min_analog) * unit_correction);
		}
	}
	mMutex.unlock();

	// Return data to initiator
	emit dataReady(result, initiator);
}

void CerebusTracesProvider::computeRecordingLength(){
	// Do not do anything here, the buffer size is always the same.
}


void CerebusTracesProvider::slotPagingStarted() {
    // We are already showing live data.
    if(mLiveTraceData == mViewTraceData)
        return;

    mMutex.lock();

    // Delete view buffers with pause data
    delete mViewTime;

    delete[] mViewTraceData;
    delete mViewTracePosition;

    for(int i = 0; i < this->nbChannels; i++) {
        delete[] mViewClusterTime[i];
        delete[] mViewClusterID[i];
        delete mViewClusterPosition[i];
    }

    delete[] mViewClusterTime;
    delete[] mViewClusterID;
    delete[] mViewClusterPosition;


    // Use the same buffer for new and displayed samples
    mViewTime = mLiveTime;

    mViewTraceData = mLiveTraceData;
    mViewTracePosition = mLiveTracePosition;

    mViewClusterTime = mLiveClusterTime;
    mViewClusterID = mLiveClusterID;
    mViewClusterPosition = mLiveClusterPosition;

	mMutex.unlock();
}

void CerebusTracesProvider::slotPagingStopped() {
    // Check if we are already paused.
    if(mLiveTraceData != mViewTraceData)
        return;

    mMutex.lock();

    // Write all new data to a new empty buffer.
    mLiveTime = new UINT32(*mViewTime);

    mLiveTraceData = new INT16[this->nbChannels * mTraceCapacity];
    memset(mLiveTraceData, 0, this->nbChannels * mTraceCapacity * sizeof(INT16));
    mLiveTracePosition = new size_t(0);

    mLiveClusterTime     = new UINT32*[this->nbChannels];
    mLiveClusterID       = new UINT8*[this->nbChannels];
    mLiveClusterPosition = new size_t*[this->nbChannels];

    for(int i = 0; i < this->nbChannels; i++) {
        mLiveClusterTime[i] = new UINT32[mEventCapacity];
        memset(mLiveClusterTime[i], 0, mEventCapacity * sizeof(UINT32));
        mLiveClusterID[i] = new UINT8[mEventCapacity];
        memset(mLiveClusterID[i], 0, mEventCapacity * sizeof(UINT8));
        mLiveClusterPosition[i] = new size_t(0);
    }

	mMutex.unlock();
}

std::string CerebusTracesProvider::getLastErrorMessage() {
	switch (mLastResult) {
	case CBSDKRESULT_WARNCONVERT:
		return "If file conversion is needed";
	case CBSDKRESULT_WARNCLOSED:
		return "Library is already closed";
	case CBSDKRESULT_WARNOPEN:
		return "Library is already opened";
	case CBSDKRESULT_SUCCESS:
		return "Successful operation";
	case CBSDKRESULT_NOTIMPLEMENTED:
		return "Not implemented";
	case CBSDKRESULT_UNKNOWN:
		return "Unknown error";
	case CBSDKRESULT_INVALIDPARAM:
		return "Invalid parameter";
	case CBSDKRESULT_CLOSED:
		return "Interface is closed cannot do this operation";
	case CBSDKRESULT_OPEN:
		return "Interface is open cannot do this operation";
	case CBSDKRESULT_NULLPTR:
		return "Null pointer";
	case CBSDKRESULT_ERROPENCENTRAL:
		return "Unable to open Central interface";
	case CBSDKRESULT_ERROPENUDP:
		return "Unable to open UDP interface (might happen if default)";
	case CBSDKRESULT_ERROPENUDPPORT:
		return "Unable to open UDP port";
	case CBSDKRESULT_ERRMEMORYTRIAL:
		return "Unable to allocate RAM for trial cache data";
	case CBSDKRESULT_ERROPENUDPTHREAD:
		return "Unable to open UDP timer thread";
	case CBSDKRESULT_ERROPENCENTRALTHREAD:
		return "Unable to open Central communication thread";
	case CBSDKRESULT_INVALIDCHANNEL:
		return "Invalid channel number";
	case CBSDKRESULT_INVALIDCOMMENT:
		return "Comment too long or invalid";
	case CBSDKRESULT_INVALIDFILENAME:
		return "Filename too long or invalid";
	case CBSDKRESULT_INVALIDCALLBACKTYPE:
		return "Invalid callback type";
	case CBSDKRESULT_CALLBACKREGFAILED:
		return "Callback register/unregister failed";
	case CBSDKRESULT_ERRCONFIG:
		return "Trying to run an unconfigured method";
	case CBSDKRESULT_INVALIDTRACKABLE:
		return "Invalid trackable id, or trackable not present";
	case CBSDKRESULT_INVALIDVIDEOSRC:
		return "Invalid video source id, or video source not present";
	case CBSDKRESULT_ERROPENFILE:
		return "Cannot open file";
	case CBSDKRESULT_ERRFORMATFILE:
		return "Wrong file format";
	case CBSDKRESULT_OPTERRUDP:
		return "Socket option error (possibly permission issue)";
	case CBSDKRESULT_MEMERRUDP:
		return "Socket memory assignment error";
	case CBSDKRESULT_INVALIDINST:
		return "Invalid range or instrument address";
	case CBSDKRESULT_ERRMEMORY:
		return "library memory allocation error";
	case CBSDKRESULT_ERRINIT:
		return "Library initialization error";
	case CBSDKRESULT_TIMEOUT:
		return "Conection timeout error";
	case CBSDKRESULT_BUSY:
		return "Resource is busy";
	case CBSDKRESULT_ERROFFLINE:
		return "Instrument is offline";
    case CBSDKRESULT_INSTOUTDATED:
        return "The instrument runs an outdated firmware version.";
    case CBSDKRESULT_LIBOUTDATED:
        return "Neuroscope uses an outdated version of libcbsdk.";
	case CBSDKRESULT_EMPTYSAMPLINGGROUP:
		return "No channels in selected sampling group.";
	}
	return "Unknown error code.";
}

QList<ClustersProvider*> CerebusTracesProvider::getClusterProviders() {
    QList<ClustersProvider*> list;

    if(mInitialized) {
		// Return a ClustersProvider wrapper for each channel.
        for(int i = 0; i < this->nbChannels; i++) {
            list.append(new CerebusClustersProvider(this, i, this->samplingRate));
        }
    }

    return list;
}

Array<dataType>* CerebusTracesProvider::getClusterData(unsigned int channel, long start, long end) {
    // The arrays assignment operator is broken, so returning a pointer is a quick fix.
	Array<dataType>* result = new Array <dataType>;

    // Abort if not initalized
    if(!mInitialized)
        return result;

    // Determine start and end index
    long startInRecordingUnits = cbSdk_TICKS_PER_SECOND * start / 1000.0;
    long endInRecordingUnits   = cbSdk_TICKS_PER_SECOND * end / 1000.0;

    // Compute values in relation to end of window.
    startInRecordingUnits -= mEventCapacity;
    endInRecordingUnits   -= mEventCapacity;

    Q_ASSERT(startInRecordingUnits <= 0);
    Q_ASSERT(endInRecordingUnits <= 0);

    // Compute correction factor for timestamps to return
    double clockToSampleRatio = cbSdk_TICKS_PER_SECOND / this->samplingRate;

	mMutex.lock();

    // Correct start and end with current time stamp
    startInRecordingUnits += (*mViewTime);
    endInRecordingUnits   += (*mViewTime);

	// In the beginning or on clock overflow make sure we stay positive.
	if (startInRecordingUnits < 0)
		startInRecordingUnits = 0;

    size_t endIndex = (*mViewClusterPosition[channel]);
    do {
        if(endIndex == 0) endIndex = mEventCapacity;
        endIndex--;
    } while(endIndex != (*mViewClusterPosition[channel]) &&
            mViewClusterTime[channel][endIndex] > endInRecordingUnits);
	endIndex++;

    size_t startIndex = endIndex;
    do {
        if(startIndex == 0) startIndex = mEventCapacity;
        startIndex--;
    } while(startIndex != (*mViewClusterPosition[channel]) &&
            mViewClusterTime[channel][startIndex] > startInRecordingUnits);
    startIndex++;

    // Count spike event and wrapp around if needed.
    size_t spikeCount = mEventCapacity + endIndex - startIndex;
    if(spikeCount >= mEventCapacity)
        spikeCount -= mEventCapacity;

    // Abort if there are no spikes
    if(spikeCount == 0) {
        mMutex.unlock();
        return result;
    }

    // Adjust result to number of events found
    result->setSize(2, spikeCount);

    size_t dataIndex = 0;
    if(startIndex <= endIndex) {
        // Adjust and copy timestamps
        for(size_t i = startIndex; i < endIndex; i++) {
			(*result)[dataIndex++] = (mViewClusterTime[channel][i] - startInRecordingUnits) / clockToSampleRatio;
        }
        // Copy cluster ids
        for(size_t i = startIndex; i < endIndex; i++) {
            (*result)[dataIndex++] = mViewClusterID[channel][i];
        }
    } else {
        // Adjust and copy timestamps
        for(size_t i = startIndex; i < mEventCapacity; i++) {
			(*result)[dataIndex++] = (mViewClusterTime[channel][i] - startInRecordingUnits) / clockToSampleRatio;
        }
        for(size_t i = 0; i < endIndex; i++) {
			(*result)[dataIndex++] = (mViewClusterTime[channel][i] - startInRecordingUnits) / clockToSampleRatio;
        }
        // Copy cluster ids
        for(size_t i = startIndex; i < mEventCapacity; i++) {
            (*result)[dataIndex++] = mViewClusterID[channel][i];
        }
        for(size_t i = 0; i < endIndex; i++) {
            (*result)[dataIndex++] = mViewClusterID[channel][i];
        }
    }

    Q_ASSERT(dataIndex == 2 * spikeCount);

    mMutex.unlock();

    return result;
}
