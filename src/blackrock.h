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

#ifndef _BLACKROCK_H_
#define _BLACKROCK_H_

// include c/c++ headers
#include <cstdint>

#pragma pack(push, 1)

//  Shared data structures
typedef struct {
  int16_t year;
  int16_t month;
  int16_t dayofweek;
  int16_t day;
  int16_t hour;
  int16_t minute;
  int16_t second;
  int16_t milliseconds;
} WindowsSystemTime;

// NEV file data structure
typedef struct {
  char file_type[8]; // “NEURALEV”
  uint16_t file_spec; // e.g. 0x0202 = Spec 2.2
  uint16_t flags; // Bit0: waveform pure 16bit
  uint32_t header_size;
  uint32_t data_package_size;
  uint32_t global_time_resolution; // counts per minute
  uint32_t waveform_time_resolution; // counts per minute
  WindowsSystemTime time_origin;
  char application[32]; // name of program that created this file.
  char comment[256];
  uint32_t extension_count;
} NEVBasicHeader;

typedef struct {
  char id[8];
  char data[24];
} NEVExtensionHeader;

#define NEVDigitalSerialID  0x0
//      NEVSpikeID          0x1 - 0x800
#define NEVConfigurationID  0xFFFB
#define NEVButtonID         0xFFFC
#define NEVTrackingID       0xFFFD
#define NEVVideoSyncID      0xFFFE
#define NEVCommentID        0xFFFF
#define NEVContinuationID   0xFFFFFFFF

typedef struct {
  uint32_t timestamp;
  uint16_t id;
} NEVDataHeader;

typedef struct {
  uint8_t reason; // bit 0 = digital change, bit 7 = serial
  uint8_t reserved;
  uint16_t input;
} NEVDigitalSerialData;

typedef struct {
  uint8_t unit_class; // 0 = unclassified, 1-16 unit, 255 = noise
  uint8_t reserved;
  // Followed by uint8_t waveform[size - 8];
} NEVSpikeDataHeader;

typedef struct {
  uint16_t type; // 0 = normal, 1 = critical;
  // Followed by char change[size - 8];
} NEVConfigurationDataHeader;

typedef struct {
  uint16_t trigger; // 0 = undefined, 1 = press, 2 = reset
} NEVButtonData;

typedef struct {
  uint16_t file_number;
  uint32_t frame_number;
  uint32_t elapsed_time;
  uint32_t video_source;
} NEVVideoSyncData;

typedef struct {
  uint16_t parent_id;
  uint16_t node_id;
  uint16_t node_count;
  uint16_t point_count;
  // Followed by uint16_t points[size - 14]
} NEVTrackingDataHeader;

typedef struct {
  uint8_t char_set; // 0 = Ansi, 1 = UTF-16
  uint8_t reserved;
  uint32_t color; // RGBA
  // Followed by char comment[size - 12];
} NEVCommentDataHeader;


// NSX file data structure
typedef struct {
  char file_type[8]; // “NEURALCD” or “NEURALSG”.
  uint16_t file_spec; // e.g. 0x0202 = Spec 2.2
  uint32_t header_size;
  char label[16];
  char comment[256];
  uint32_t sampling_period; // sampling period e.g. w1 = 30ks, 10ks = 3
  uint32_t time_resolution; // (counts per second) of the global clock used to index the time samples of the individual data packet entries.
  WindowsSystemTime time_origin;
  uint32_t channel_count;
} NSXBasicHeader;

typedef struct {
  char type[2]; // “CC” for “Continuous Channels”
  uint16_t id;
  char label[16];
  uint8_t bank;
  uint8_t pin;
  int16_t min_digital_value;
  int16_t max_digital_value;
  int16_t min_analog_value;
  int16_t max_analog_value;
  char unit[16]; // units of analog range values (“mV”, “μV”)
  uint32_t highpass_corner; // high cutoff frequency in MHz
  uint32_t highpass_order; // order of high cutoff: 0 = NONE
  uint16_t highpass_type; // type of high cutoff: 0 = NONE, 1 = Butterworth
  uint32_t lowpass_corner; // low cutoff frequency in MHz
  uint32_t lowpass_order; // order of low cutoff: 0 = NONE
  uint16_t lowpass_type; // type of low cutoff: 0 = NONE, 1 = Butterworth
} NSXExtensionHeader;

typedef struct {
  uint8_t header;
  uint32_t timestamp;
  uint32_t length;
} NSXDataHeader;

#pragma pack(pop)     // return to previous pack setting

#endif
