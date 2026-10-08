/***************************************************************************
                          neuroscope_plugin.h  -  description
                             -------------------
    purpose              : C interface of NeuroScope file format plugins
    copyright            : (C) 2026 by Joscha Schmiedt
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

/* A file format plugin is a shared library that exports neuroscope_plugin(), which returns a
 * description of the format and the functions that read it. The interface is plain C, so a plugin
 * needs neither Qt nor the compiler NeuroScope was built with.
 *
 * Streams: a file contains one or more streams of continuous data. Each stream has one sampling
 * rate and a fixed set of channels, and NeuroScope shows one stream per window. The format decides
 * what a stream is, e.g. all DH5 CONT blocks with the same sampling period.
 *
 * Time: sample k of a stream is at k / sampling_rate seconds on the file's own clock, so time 0 is
 * the file's time 0. Samples without recorded data (before the recording started, or in pauses)
 * read as 0.
 *
 * Values are in microvolts (µV); the plugin applies the calibration of each channel.
 *
 * Strings returned by a plugin belong to it and remain valid until the file is closed (strings of
 * the ns_plugin itself as long as the library is loaded). They are UTF-8.
 *
 * NeuroScope calls the functions of one file from one thread at a time.
 *
 * Compatibility: members are only ever added at the end of ns_plugin, and NeuroScope checks `size`
 * before using a member added after version 1. api_version changes only for incompatible changes,
 * and NeuroScope refuses plugins of another version. */

#ifndef NEUROSCOPE_PLUGIN_H
#define NEUROSCOPE_PLUGIN_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define NS_PLUGIN_API_VERSION 1

/** Name of the function a plugin exports. */
#define NS_PLUGIN_ENTRY_NAME "neuroscope_plugin"

#if defined(_WIN32)
#define NS_PLUGIN_EXPORT __declspec(dllexport)
#else
#define NS_PLUGIN_EXPORT __attribute__((visibility("default")))
#endif

/** Return values of the functions that can fail. */
#define NS_OK 0
#define NS_ERROR -1

/** An open file, owned by the plugin. */
typedef struct ns_file ns_file;

/** Error message filled in by a function that returns NS_ERROR. */
typedef struct ns_error
{
    char message[512];
} ns_error;

typedef struct ns_stream_info
{
    /** Stable identifier, e.g. "30000Hz"; used to select the stream on the command line and to name
      * session files, so it should only contain letters, digits, '-' and '_'. */
    const char* id;
    /** Description for the user, e.g. "30 kHz: CONT1-CONT32 (32 channels)". */
    const char* label;
    int32_t channel_count;
    /** Samples per second. */
    double sampling_rate;
    /** Number of samples from time 0 to the end of the last recorded sample. */
    int64_t sample_count;
} ns_stream_info;

typedef struct ns_event_list_info
{
    /** Name of the list, e.g. "Trials". */
    const char* name;
    /** Number of events. */
    int64_t count;
    /** Number of distinct labels (event descriptions) of the list. */
    int32_t label_count;
} ns_event_list_info;

typedef struct ns_plugin
{
    /** NS_PLUGIN_API_VERSION the plugin was built with. */
    uint32_t api_version;
    /** sizeof(ns_plugin) the plugin was built with. */
    uint32_t size;
    /** Name of the format, e.g. "DAQ-HDF5". */
    const char* name;
    /** File name extensions without the dot, terminated by NULL, e.g. {"dh5", NULL}. */
    const char* const* extensions;

    /** How certain the plugin is that it can read the file: 0 (not at all) to 100 (certain). Called
      * for files with one of the extensions; may be NULL, which counts as 50. */
    int (*probe)(const char* path);

    /** Opens a file for reading. */
    int (*open)(const char* path, ns_file** file, ns_error* error);
    void (*close)(ns_file* file);

    /* Continuous data */

    int (*stream_count)(ns_file* file);
    /** Index of the stream that opens when the user does not choose one. */
    int (*default_stream)(ns_file* file);
    int (*stream_info)(ns_file* file, int stream, ns_stream_info* info);
    /** Label of a channel, e.g. "CONT1:0"; may return NULL, then NeuroScope numbers the channels. */
    const char* (*channel_label)(ns_file* file, int stream, int channel);
    /** Reads `count` samples of all channels from sample `first` on, row-major:
      * microvolts[sample * channel_count + channel]. NeuroScope only asks for samples in
      * [0, sample_count). */
    int (*read)(ns_file* file, int stream, int64_t first, int64_t count, double* microvolts, ns_error* error);

    /* Events; all NULL if the format has none */

    int (*event_list_count)(ns_file* file);
    int (*event_list_info)(ns_file* file, int list, ns_event_list_info* info);
    /** Description of label index `label` of a list, e.g. "trial start". */
    const char* (*event_label)(ns_file* file, int list, int label);
    /** Reads all events of a list in time order: their times in nanoseconds on the file's clock and
      * their label indices. Both arrays have ns_event_list_info.count elements. */
    int (*read_events)(ns_file* file, int list, int64_t* times_ns, int32_t* labels, ns_error* error);
} ns_plugin;

/** The function a plugin exports. It returns NULL if the plugin cannot work with a NeuroScope that
  * uses `host_api_version`. */
typedef const ns_plugin* (*ns_plugin_entry)(uint32_t host_api_version);

#ifdef __cplusplus
}
#endif

#endif
