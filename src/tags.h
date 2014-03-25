/***************************************************************************
                          tags.h  -  description
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

#ifndef TAGS_H
#define TAGS_H

// include files for QT
#include <QString>

/**
  * This class contains the XML tags used in the session files.
  *@author Lynn Hazan
  */

/* @namespace neuroscope*/
namespace neuroscope{

/**Tag for the neuroscope session file root element.*/
extern const QString NEUROSCOPE;
/**Tag for the parameter file root element.*/
extern const QString PARAMETERS;
/**Tag for the creator attribut of the parameter file root element.*/
extern const QString CREATOR;
/**Tag for the version attribute of the root element.*/
extern const QString VERSION;
/**Tag for the acquisition element.*/
extern const QString ACQUISITION;
/**Tag for the filedPotentials element.*/
extern const QString FIELD_POTENTIALS;
/**Tag for the miscellaneous element.*/
extern const QString MISCELLANEOUS;
/**Tag for the video element.*/
extern const QString VIDEO;
/**Tag for the samplingRate element.*/
extern const QString SAMPLING_RATES;
/**Tag for the channels element.*/
extern const QString CHANNELS;
/**Tag for the anatomy element.*/
extern const QString ANATOMY;
/**Tag for the spike element.*/
extern const QString SPIKE;
/**Tag for the file element.*/
extern const QString FILES;
/**Tag for the display element.*/
extern const QString DISPLAYS;

//Tags included in ACQUISITION
/**Tag for the bits element included in the acquisition element.*/
extern const QString BITS;
/**Tag for the nbChannels element included in the acquisition element.*/
extern const QString NB_CHANNELS;
/**Tag for the samplingRate element included in the acquisition element.*/
extern const QString SAMPLING_RATE;
/**Tag for the voltageRange element included in the acquisition element.*/
extern const QString VOLTAGE_RANGE;
/**Tag for the amplification element included in the acquisition element.*/
extern const QString AMPLIFICATION;
/**Tag for the offset element included in the acquisition element.*/
extern const QString OFFSET;

//Tags included in FILED_POTENTIALS
/**Tag for the lfpSamplingRate element included in the filedPotentials element.*/
extern const QString LFP_SAMPLING_RATE;

//Tags included in MISCELLANEOUS
/**Tag for the screenGain element included in the miscellaneous element.*/
extern const QString SCREENGAIN;
/**Tag for the traceBackgroundImage element included in the miscellaneous element.*/
extern const QString TRACE_BACKGROUND_IMAGE;

/**Tag for the spikes element.*/
extern const QString SPIKES;

//Tags included in VIDEO
/**Tag for the width element included in the video element.*/
extern const QString WIDTH;
/**Tag for the height element included in the video element.*/
extern const QString HEIGHT;
/**Tag for the rotate element included in the video element.*/
extern const QString ROTATE;
/**Tag for the flip element included in the video element.*/
extern const QString FLIP;
/**Tag for the video image element included in the file element.*/
extern const QString VIDEO_IMAGE;
/**Tag for the positions backgroung element included in the file element.*/
extern const QString POSITIONS_BACKGROUND;


/**Tag for the extensionSamplingRate element included in the file element. Obsolete*/
extern const QString EXTENSION_SAMPLING_RATE;

/**Tag for the extension element included in the file element.*/
extern const QString EXTENSION;

/**Tag for the channelColors element included in the channels element.*/
extern const QString CHANNEL_COLORS;

//Tags included in NEUROSCOPE
/**Tag for the channel element.*/
extern const QString CHANNEL;
/**Tag for the skip attribute of the channel element.*/
extern const QString SKIP;
/**Tag for the colors element.*/
extern const QString COLOR;
/**Tag for the anatomyColors element.*/
extern const QString ANATOMY_COLOR;
/**Tag for the spikeColors element.*/
extern const QString SPIKE_COLOR;
/**Tag for the channelOffset element.*/
extern const QString CHANNEL_OFFSET;
/**Tag for the defaultOffset element.*/
extern const QString DEFAULT_OFFSET;

/**Tag for the channelGroups element.*/
extern const QString CHANNEL_GROUPS;

/**Tag for the groups element.*/
extern const QString GROUP;

/**Tag for the channel element.*/
extern const QString CHANNEL;

//Tags included in SPIKE
/**Tag for the nbSamples element included in the spike element.*/
extern const QString NB_SAMPLES;
/**Tag for the peakSAmpleIndex element included in the spike element.*/
extern const QString PEAK_SAMPLE_INDEX;
/**Tag for the upsamplingRate element included in the spike element.*/
extern const QString UPSAMPLING_RATE;
/**Tag for the waveformLength element included in the spike element.*/
extern const QString WAVEFORM_LENGTH;
/**Tag for the peakSampleLength element included in the spike element.*/
extern const QString PEAK_SAMPLE_LENGTH;

/**Tag for the file element included in the files element.*/
extern const QString FILE;

//Tags included in FILE
/**Tag for the type element included in the file element.*/
extern const QString TYPE;
/**Tag for the url element included in the file element.*/
extern const QString URL;
/**Tag for the date element included in the file element.*/
extern const QString DATE;
/**Tag for the items element included in the file element.*/
extern const QString ITEMS;

/**Tag for the itemDescription element included in the items element.*/
extern const QString ITEM_DESCRIPTION;

/**Tag for the item element included in the itemDescription element.*/
extern const QString ITEM;

/**Tag for the display element included in the displays element.*/
extern const QString DISPLAY;

//Tags included in DISPLAY
/**Tag for the tabLabel element included in the display element.*/
extern const QString TAB_LABEL;
/**Tag for the showLabels element included in the display element.*/
extern const QString SHOW_LABELS;
/**Tag for the startTime element included in the display element.*/
extern const QString START_TIME;
/**Tag for the duration element included in the display element.*/
extern const QString DURATION;
/**Tag for the multipleColumns element included in the display element.*/
extern const QString MULTIPLE_COLUMNS;
/**Tag for the greyScale element included in the display element.*/
extern const QString GREYSCALE;
/**Tag for the autocenter element included in the display element.*/
extern const QString AUTOCENTER_CHANNELS;
/**Tag for the positionView element included in the display element.*/
extern const QString POSITIONVIEW;
/**Tag for the showEvents element included in the display element.*/
extern const QString SHOWEVENTS;
/**Tag for the spikePresentation element included in the display element.*/
extern const QString SPIKE_PRESENTATION;
/**Tag for the raster height element included in the display element.*/
extern const QString RASTER_HEIGHT;
/**Tag for the clustersSelected element included in the display element.*/
extern const QString CLUSTERS_SELECTED;
/**Tag for the spikesSelected element included in the display element.*/
extern const QString SPIKES_SELECTED;
/**Tag for the eventsSelected element included in the display element.*/
extern const QString EVENTS_SELECTED;
/**Tag for the clustersSkipped element included in the display element.*/
extern const QString CLUSTERS_SKIPPED;
/**Tag for the eventsSkippedelement included in the display element.*/
extern const QString EVENTS_SKIPPED;
/**Tag for the channelPositions element included in the display element.*/
extern const QString CHANNEL_POSITIONS;
/**Tag for the channelsSelected element included in the display element.*/
extern const QString CHANNELS_SELECTED;
/**Tag for the channelsShown element included in the display element.*/
extern const QString CHANNELS_SHOWN;

//Tags included in CLUSTERS_SELECTED
/**Tag for the fileUrl element included in the clustersSelected element.*/
extern const QString FILE_URL;
/**Tag for the cluster element included in the clustersSelected element.*/
extern const QString CLUSTER;


/**Tag for the event element included in the eventsSelected element.*/
extern const QString EVENT;

/**Tag for the channelPosition element included in the channelPositions element.*/
extern const QString CHANNEL_POSITION;

/**Tag for the gain element included in the channelPosition element.*/
extern const QString GAIN;

}

#endif
