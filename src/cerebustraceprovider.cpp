#include <string>

#include "cerebustraceprovider.h"
#include <QDebug>

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
		mScales(NULL),
		mChannels(NULL),
        mLiveData(NULL),
        mLivePosition(NULL),
        mViewData(NULL),
        mViewPosition(NULL) {

	// The sampling rate is hardwired to the sampling group
	this->samplingRate = SAMPLING_RATES[group];
	mCapacity = this->samplingRate * BUFFER_SIZE;

	// The buffer always has the same size
	this->length = 1000 * BUFFER_SIZE;
}

CerebusTracesProvider::~CerebusTracesProvider() {
	if (mInitialized) {
		// Disable callbacks first
		cbSdkUnRegisterCallback(CEREBUS_INSTANCE, CBSDKCALLBACK_CONTINUOUS);
		cbSdkUnRegisterCallback(CEREBUS_INSTANCE, CBSDKCALLBACK_GROUPINFO);

		// Close network thread and connection
		cbSdkClose(CEREBUS_INSTANCE);

		// Free uninitalize internal structures
		delete[] mScales;
		delete[] mChannels;
        if(mViewData != mLiveData) {
            // We are in paused mode
            delete[] mViewData;
            delete mViewPosition;
        }
        delete[] mLiveData;
        delete mLivePosition;
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

	if (!retrieveScaling()) {
		delete[] mChannels;
		cbSdkClose(CEREBUS_INSTANCE);
		return false;
	}

	// Allocate live sample buffer
	mLiveData = new INT16[this->nbChannels * mCapacity];
	memset(mLiveData, 0, this->nbChannels * mCapacity * sizeof(INT16));
    mLivePosition = new size_t(0);

    // Link view buffer to live sample buffer
    mViewData = mLiveData;
    mViewPosition = mLivePosition;

	// Register data callback
	mMutex.lock();
	mLastResult = cbSdkRegisterCallback(CEREBUS_INSTANCE, CBSDKCALLBACK_CONTINUOUS, dataCallback, this);

	if (mLastResult != CBSDKRESULT_SUCCESS) {
		delete[] mChannels;
		delete[] mScales;
		mScales = NULL;
		delete[] mLiveData;
        delete mLivePosition;
		cbSdkClose(CEREBUS_INSTANCE);
		return false;
	}

	// Register config callback
	mLastResult = cbSdkRegisterCallback(CEREBUS_INSTANCE, CBSDKCALLBACK_GROUPINFO, configCallback, this);

	if (mLastResult != CBSDKRESULT_SUCCESS) {
		delete[] mChannels;
		delete[] mScales;
		mScales = NULL;
		delete[] mLiveData;
        delete mLivePosition;
		cbSdkUnRegisterCallback(CEREBUS_INSTANCE, CBSDKCALLBACK_CONTINUOUS);
		cbSdkClose(CEREBUS_INSTANCE);
		return false;
	}

	mInitialized = true;
	mMutex.unlock();
	return true;
}

void CerebusTracesProvider::processData(const cbPKT_GROUP* package) {
	// Ignore package if for different sampling group
	if (package->type != mGroup)
		return;

	mMutex.lock();
	// Copy sampled to history
	memcpy(mLiveData + ((*mLivePosition) * this->nbChannels), package->data, this->nbChannels * sizeof(INT16));

	// Adjust position, wrap around if necessary.
	(*mLivePosition)++;
	if ((*mLivePosition) == mCapacity) (*mLivePosition) = 0;
	mMutex.unlock();
}

void CerebusTracesProvider::processConfig(const cbPKT_GROUPINFO* package) {
	// Ignore package if for different sampling group or channel count did not change
	if (package->group != mGroup || (package->dlen - 8) == this->nbChannels)
		return;

	mMutex.lock();
	// Update channel count
	this->nbChannels = (package->dlen - 8);

	// Update channel list
	delete[] mChannels;
	mChannels = new UINT32[this->nbChannels];
	memcpy(mChannels, package->list, this->nbChannels * sizeof(UINT32));

	// Update scaling
	retrieveScaling();

	// Update buffer size
	delete[] mLiveData;
	mLiveData = new INT16[this->nbChannels * mCapacity];
	memset(mLiveData, 0, this->nbChannels * mCapacity * sizeof(INT16));
    (*mLivePosition) = 0;

	mMutex.unlock();
}

void CerebusTracesProvider::dataCallback(UINT32 /*instance*/, const cbSdkPktType type, const void* data, void* object) {
	CerebusTracesProvider* provider = reinterpret_cast<CerebusTracesProvider*>(object);

	// TODO: Take care of package lost here!
	if (provider && type == cbSdkPkt_CONTINUOUS && data) {
		provider->processData(reinterpret_cast<const cbPKT_GROUP *>(data));
	}
}

void CerebusTracesProvider::configCallback(UINT32 /*instance*/, const cbSdkPktType type, const void* data, void* object) {
	CerebusTracesProvider* provider = reinterpret_cast<CerebusTracesProvider*>(object);

	// TODO: Take care of package lost here!
	if (provider && type == cbSdkPkt_GROUPINFO && data) {
		provider->processConfig(reinterpret_cast<const cbPKT_GROUPINFO *>(data));
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

	// Allocate result array
	result.setSize(lengthInRecordingUnits, this->nbChannels);

	// Get data from buffer.
	mMutex.lock();
	for (int channel = 0; channel < this->nbChannels; channel++) {
		// Compute start in relation to current ringbuffer position
		size_t offset = startInRecordingUnits + (*mViewPosition);

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
			qDebug() << "unknown unit for channel " << channel << ": " << unit_string;
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
			if (absolute_position >= mCapacity) {
				absolute_position -= mCapacity;
				offset -= mCapacity;
			}
			// Scale data using channel scaling
			size_t index = (absolute_position * this->nbChannels) + channel;
			result(i + 1, channel + 1) = static_cast<dataType>((((static_cast<double>(mViewData[index]) - min_digital) / range_digital) * range_analog + min_analog) * unit_correction);
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
    if(mLiveData == mViewData)
        return;

    mMutex.lock();

    // Use the same buffer for new and displayed samples
    delete[] mViewData;
    delete mViewPosition;
    mViewData = mLiveData;
    mViewPosition = mLivePosition;

	mMutex.unlock();
}

void CerebusTracesProvider::slotPagingStopped() {
    // Check if we are already paused.
    if(mLiveData != mViewData)
        return;

    mMutex.lock();

    // Write all new data to a new empty buffer.
    mLiveData = new INT16[this->nbChannels * mCapacity];
    memset(mLiveData, 0, this->nbChannels * mCapacity * sizeof(INT16));
    mLivePosition = new size_t(0);

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

bool CerebusTracesProvider::retrieveScaling() {
	// Delete old scaling data if there is any
	if (mScales) {
		delete[] mScales;
		mScales = NULL;
	}

	// Get scaling for each channel of group
	mScales = new cbSCALING[this->nbChannels];

	cbPKT_CHANINFO info;
	for (unsigned int i = 0; i < this->nbChannels; i++)
	{
		mLastResult = cbSdkGetChannelConfig(CEREBUS_INSTANCE, mChannels[i], &info);

		if (mLastResult != CBSDKRESULT_SUCCESS) {
			delete[] mScales;
			mScales = NULL;
			return false;
		}

		// Copy only physcal input scaling for now
		mScales[i] = info.physcalin;
	}
	return true;
}
