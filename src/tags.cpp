/***************************************************************************
                          tags.cpp  -  description
                             -------------------
    begin                : Wed Mar 31 2004
    copyright            : (C) 2004 by Lynn Hazan
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

#include "tags.h"

namespace neuroscope{

extern const QString NEUROSCOPE = "neuroscope";
extern const QString PARAMETERS = "parameters";
extern const QString CREATOR = "creator";
extern const QString VERSION = "version";
extern const QString ACQUISITION = "acquisitionSystem";
extern const QString FIELD_POTENTIALS = "fieldPotentials";
extern const QString MISCELLANEOUS = "miscellaneous";
extern const QString VIDEO = "video";
extern const QString SAMPLING_RATES = "samplingRates";
extern const QString CHANNELS = "channels";
extern const QString ANATOMY = "anatomicalDescription";
extern const QString SPIKE = "spikeDetection";
extern const QString FILES = "files";
extern const QString DISPLAYS = "displays";

//Tags included in ACQUISITION
extern const QString BITS = "nBits";
extern const QString NB_CHANNELS = "nChannels";
extern const QString SAMPLING_RATE = "samplingRate";
extern const QString VOLTAGE_RANGE = "voltageRange";
extern const QString AMPLIFICATION = "amplification";
extern const QString OFFSET = "offset";

//Tags included in FILED_POTENTIALS
extern const QString LFP_SAMPLING_RATE = "lfpSamplingRate";

//Tags included in MISCELLANEOUS
extern const QString SCREENGAIN = "screenGain";
extern const QString TRACE_BACKGROUND_IMAGE = "traceBackgroundImage";

extern const QString SPIKES = "spikes";

//Tags included in VIDEO
extern const QString WIDTH = "width";
extern const QString HEIGHT = "height";
extern const QString ROTATE = "rotate";
extern const QString FLIP = "flip";
extern const QString VIDEO_IMAGE = "videoImage";
extern const QString POSITIONS_BACKGROUND = "positionsBackground";

//Tag included in SAMPLING_RATES, obsolete
extern const QString EXTENSION_SAMPLING_RATE = "extensionSamplingRate";

extern const QString EXTENSION = "extension";

//Tag included in CHANNELS
extern const QString CHANNEL_COLORS = "channelColors";

//Tags included in NEUROSCOPE
extern const QString CHANNEL = "channel";
extern const QString SKIP = "skip";
extern const QString COLOR = "color";
extern const QString ANATOMY_COLOR = "anatomyColor";
extern const QString SPIKE_COLOR = "spikeColor";
extern const QString CHANNEL_OFFSET = "channelOffset";
extern const QString DEFAULT_OFFSET = "defaultOffset";


//Tag included in ANATOMY
extern const QString CHANNEL_GROUPS = "channelGroups";

//Tag included in CHANNEL_GROUPS
extern const QString GROUP = "group";

//Tags included in SPIKE
extern const QString NB_SAMPLES = "nSamples";
extern const QString PEAK_SAMPLE_INDEX = "peakSampleIndex";
extern const QString UPSAMPLING_RATE = "upsamplingRate";
extern const QString WAVEFORM_LENGTH = "waveformLength";
extern const QString PEAK_SAMPLE_LENGTH = "peakSampleLength";

//Tag included in FILES
extern const QString FILE = "file";

//Tags included in FILE
extern const QString TYPE = "type";
extern const QString URL = "url";
extern const QString DATE = "modificationDate";
extern const QString ITEMS = "items";


//Tag included in ITEMS
extern const QString ITEM_DESCRIPTION = "itemDescription";

//Tag included in ITEM_DESCRIPTION
extern const QString ITEM = "item";

//Tag included in DISPLAYS
extern const QString DISPLAY = "display";

//Tags included in DISPLAY
extern const QString TAB_LABEL = "tabLabel";
extern const QString SHOW_LABELS = "showLabels";
extern const QString MULTIPLE_COLUMNS = "multipleColumns";
extern const QString START_TIME = "startTime";
extern const QString DURATION = "duration";
extern const QString GREYSCALE = "greyScale";
extern const QString AUTOCENTER_CHANNELS = "autocenterChannels";
extern const QString POSITIONVIEW = "positionView";
extern const QString SHOWEVENTS = "showEvents";
extern const QString SPIKE_PRESENTATION = "spikePresentation";
extern const QString RASTER_HEIGHT = "rasterHeight";
extern const QString CLUSTERS_SELECTED = "clustersSelected";
extern const QString SPIKES_SELECTED = "spikesSelected";
extern const QString EVENTS_SELECTED = "eventsSelected";
extern const QString CLUSTERS_SKIPPED = "clustersSkipped";
extern const QString EVENTS_SKIPPED = "eventsSkipped";
extern const QString CHANNEL_POSITIONS = "channelPositions";
extern const QString CHANNELS_SELECTED = "channelsSelected";
extern const QString CHANNELS_SHOWN = "channelsShown";

//Tag included in CLUSTERS_SELECTED
extern const QString FILE_URL = "fileUrl";
extern const QString CLUSTER = "cluster";

//Tag included in EVENTS_SELECTED
extern const QString EVENT = "event";

//Tag included in CHANNEL_POSITIONS
extern const QString CHANNEL_POSITION = "channelPosition";

//Tag included in CHANNEL_POSITION
extern const QString GAIN = "gain";

}

