/***************************************************************************
                          testplugin.c  -  description
                             -------------------
    purpose              : File format plugin with synthetic data for the tests
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

/* Reads files with the extension .nstest that start with "NSTEST". Files that start with
 * "NSTEST-broken" cannot be opened. The data are synthetic (see testplugin.h). */

#include "testplugin.h"

#include <neuroscope/neuroscope_plugin.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef TEST_PLUGIN_API_VERSION
#define TEST_PLUGIN_API_VERSION NS_PLUGIN_API_VERSION
#endif

struct ns_file
{
    int open;
};

static int probe(const char* path)
{
    char header[6];
    FILE* f = fopen(path, "rb");
    if (!f)
        return 0;
    const size_t n = fread(header, 1, sizeof(header), f);
    fclose(f);
    return n == sizeof(header) && memcmp(header, "NSTEST", sizeof(header)) == 0 ? 100 : 0;
}

static int fail(ns_error* error, const char* message)
{
    if (error)
        snprintf(error->message, sizeof(error->message), "%s", message);
    return NS_ERROR;
}

static int open_file(const char* path, ns_file** file, ns_error* error)
{
    char header[14] = {0};
    FILE* f = fopen(path, "rb");
    if (!f)
        return fail(error, "cannot open the file");
    const size_t n = fread(header, 1, sizeof(header) - 1, f);
    fclose(f);
    if (n < 6 || memcmp(header, "NSTEST", 6) != 0)
        return fail(error, "not a test file");
    if (strcmp(header, "NSTEST-broken") == 0)
        return fail(error, "broken test file");
    *file = (ns_file*)calloc(1, sizeof(ns_file));
    (*file)->open = 1;
    return NS_OK;
}

static void close_file(ns_file* file)
{
    free(file);
}

static int stream_count(ns_file* file)
{
    (void)file;
    return 2;
}

static int default_stream(ns_file* file)
{
    (void)file;
    return TEST_DEFAULT_STREAM;
}

static int stream_info(ns_file* file, int stream, ns_stream_info* info)
{
    (void)file;
    if (stream == 0)
    {
        info->id = "1000Hz";
        info->label = "1 kHz: 2 channels";
        info->channel_count = TEST_FAST_CHANNELS;
        info->sampling_rate = TEST_FAST_RATE;
        info->sample_count = TEST_FAST_SAMPLES;
        return NS_OK;
    }
    if (stream == 1)
    {
        info->id = "250Hz";
        info->label = "250 Hz: 1 channel";
        info->channel_count = TEST_SLOW_CHANNELS;
        info->sampling_rate = TEST_SLOW_RATE;
        info->sample_count = TEST_SLOW_SAMPLES;
        return NS_OK;
    }
    return NS_ERROR;
}

static const char* channel_label(ns_file* file, int stream, int channel)
{
    static const char* const labels[] = {"A0", "A1"};
    (void)file;
    return stream == 0 && channel >= 0 && channel < 2 ? labels[channel] : NULL;
}

static int read_data(ns_file* file, int stream, int64_t first, int64_t count, double* microvolts, ns_error* error)
{
    (void)file;
    ns_stream_info info;
    if (stream_info(file, stream, &info) != NS_OK)
        return fail(error, "no such stream");
    if (first < 0 || count < 0 || first + count > info.sample_count)
        return fail(error, "samples out of range");
    for (int64_t s = 0; s < count; ++s)
        for (int c = 0; c < info.channel_count; ++c)
            microvolts[s * info.channel_count + c] = stream == 0 ? test_fast_value(first + s, c) : test_slow_value(first + s);
    return NS_OK;
}

static const char* const extensions[] = {"nstest", NULL};

static const ns_plugin plugin = {
    TEST_PLUGIN_API_VERSION,
    sizeof(ns_plugin),
    TEST_PLUGIN_NAME,
    extensions,
    probe,
    open_file,
    close_file,
    stream_count,
    default_stream,
    stream_info,
    channel_label,
    read_data,
    NULL,
    NULL,
    NULL,
    NULL,
};

#ifndef TEST_PLUGIN_NO_ENTRY
NS_PLUGIN_EXPORT const ns_plugin* neuroscope_plugin(uint32_t host_api_version)
{
    (void)host_api_version;
    return &plugin;
}
#else
NS_PLUGIN_EXPORT const ns_plugin* something_else(void)
{
    return &plugin;
}
#endif
