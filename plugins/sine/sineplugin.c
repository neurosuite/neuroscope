/***************************************************************************
                          sineplugin.c  -  description
                             -------------------
    purpose              : Example file format plugin with synthetic sinusoids
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

/* Example of a NeuroScope file format plugin, and synthetic data to inspect the display.
 *
 * A .sine file is a text file of settings, one "name = value" per line (see demo.sine); missing
 * settings have their defaults. The plugin computes the data from them:
 *
 * - two streams with the same channels: "broadband" at `rate` and "LFP" at `lfp_rate`;
 * - channel c (from 0) is a sinusoid of c + 1 Hz and 300 µV; the broadband stream adds one of
 *   200 * (c + 1) Hz and 50 µV;
 * - at each full second, all channels have a pulse of 1000 µV and 10 ms, to check the time axis;
 * - the recording starts at `start` seconds on the file's clock and lasts until `start + duration`,
 *   with a pause from `pause_start` to `pause_end` seconds; samples without data are 0. */

#include <neuroscope/neuroscope_plugin.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define STREAM_COUNT 2
#define BROADBAND 0
#define LFP 1

struct ns_file
{
    int channels;
    double rates[STREAM_COUNT];
    double start;
    double duration;
    double pause_start;
    double pause_end;

    char ids[STREAM_COUNT][32];
    char labels[STREAM_COUNT][96];
    char (*channel_labels)[32];
};

static int fail(ns_error* error, const char* message)
{
    if (error)
        snprintf(error->message, sizeof(error->message), "%s", message);
    return NS_ERROR;
}

static int probe(const char* path)
{
    FILE* f = fopen(path, "r");
    if (!f)
        return 0;
    fclose(f);
    return 100;
}

static int read_settings(FILE* f, ns_file* file, ns_error* error)
{
    char line[256];
    while (fgets(line, sizeof(line), f))
    {
        char name[64];
        double value;
        if (line[0] == '#' || sscanf(line, " %63[a-z_] = %lf", name, &value) != 2)
            continue;
        if (strcmp(name, "channels") == 0)
            file->channels = (int)value;
        else if (strcmp(name, "rate") == 0)
            file->rates[BROADBAND] = value;
        else if (strcmp(name, "lfp_rate") == 0)
            file->rates[LFP] = value;
        else if (strcmp(name, "start") == 0)
            file->start = value;
        else if (strcmp(name, "duration") == 0)
            file->duration = value;
        else if (strcmp(name, "pause_start") == 0)
            file->pause_start = value;
        else if (strcmp(name, "pause_end") == 0)
            file->pause_end = value;
    }
    if (file->channels < 1 || file->channels > 1024)
        return fail(error, "channels must be between 1 and 1024");
    if (!(file->rates[BROADBAND] > 0) || !(file->rates[LFP] > 0))
        return fail(error, "the sampling rates must be positive");
    if (!(file->start >= 0) || !(file->duration > 0))
        return fail(error, "start must not be negative and duration must be positive");
    return NS_OK;
}

static int open_file(const char* path, ns_file** result, ns_error* error)
{
    FILE* f = fopen(path, "r");
    if (!f)
        return fail(error, "cannot open the file");

    ns_file* file = (ns_file*)calloc(1, sizeof(ns_file));
    file->channels = 8;
    file->rates[BROADBAND] = 20000;
    file->rates[LFP] = 1250;
    file->start = 1;
    file->duration = 60;
    file->pause_start = 20;
    file->pause_end = 25;
    const int status = read_settings(f, file, error);
    fclose(f);
    if (status != NS_OK)
    {
        free(file);
        return status;
    }

    for (int s = 0; s < STREAM_COUNT; ++s)
    {
        snprintf(file->ids[s], sizeof(file->ids[s]), "%.0fHz", file->rates[s]);
        snprintf(file->labels[s], sizeof(file->labels[s]), "%s, %g Hz (%d channels)", s == BROADBAND ? "Broadband" : "LFP",
                 file->rates[s], file->channels);
    }
    file->channel_labels = calloc((size_t)file->channels, sizeof(*file->channel_labels));
    for (int c = 0; c < file->channels; ++c)
        snprintf(file->channel_labels[c], sizeof(file->channel_labels[c]), "sine %d Hz", c + 1);

    *result = file;
    return NS_OK;
}

static void close_file(ns_file* file)
{
    free(file->channel_labels);
    free(file);
}

static int stream_count(ns_file* file)
{
    (void)file;
    return STREAM_COUNT;
}

static int default_stream(ns_file* file)
{
    (void)file;
    return BROADBAND;
}

static int stream_info(ns_file* file, int stream, ns_stream_info* info)
{
    if (stream < 0 || stream >= STREAM_COUNT)
        return NS_ERROR;
    info->id = file->ids[stream];
    info->label = file->labels[stream];
    info->channel_count = file->channels;
    info->sampling_rate = file->rates[stream];
    info->sample_count = (int64_t)ceil((file->start + file->duration) * file->rates[stream]);
    return NS_OK;
}

static const char* channel_label(ns_file* file, int stream, int channel)
{
    (void)stream;
    return channel >= 0 && channel < file->channels ? file->channel_labels[channel] : NULL;
}

static int recorded(const ns_file* file, double t)
{
    return t >= file->start && t < file->start + file->duration && !(t >= file->pause_start && t < file->pause_end);
}

static int read_data(ns_file* file, int stream, int64_t first, int64_t count, double* microvolts, ns_error* error)
{
    if (stream < 0 || stream >= STREAM_COUNT)
        return fail(error, "no such stream");
    const double rate = file->rates[stream];
    for (int64_t i = 0; i < count; ++i)
    {
        const double t = (double)(first + i) / rate;
        double* row = microvolts + i * file->channels;
        if (!recorded(file, t))
        {
            memset(row, 0, sizeof(double) * (size_t)file->channels);
            continue;
        }
        const double pulse = t - floor(t) < 0.010 ? 1000.0 : 0.0;
        for (int c = 0; c < file->channels; ++c)
        {
            double value = 300.0 * sin(2 * M_PI * (c + 1) * t) + pulse;
            if (stream == BROADBAND)
                value += 50.0 * sin(2 * M_PI * 200.0 * (c + 1) * t);
            row[c] = value;
        }
    }
    return NS_OK;
}

static const char* const extensions[] = {"sine", NULL};

static const ns_plugin plugin = {
    NS_PLUGIN_API_VERSION,
    sizeof(ns_plugin),
    "Synthetic sinusoids",
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

NS_PLUGIN_EXPORT const ns_plugin* neuroscope_plugin(uint32_t host_api_version)
{
    (void)host_api_version;
    return &plugin;
}
