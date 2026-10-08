/***************************************************************************
                          testplugin.h  -  description
                             -------------------
    purpose              : Synthetic data of the test plugin, shared with the tests
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

#ifndef TESTPLUGIN_H
#define TESTPLUGIN_H

#include <stdint.h>

#ifndef TEST_PLUGIN_NAME
#define TEST_PLUGIN_NAME "Test format"
#endif

#ifndef TEST_DEFAULT_STREAM
#define TEST_DEFAULT_STREAM 0
#endif

/* Stream 0, "1000Hz": two channels, 3 s. The recording starts at 100 ms and pauses from 1000 ms to
 * 1500 ms; samples without data are 0. */
#define TEST_FAST_RATE 1000.0
#define TEST_FAST_CHANNELS 2
#define TEST_FAST_SAMPLES 3000
#define TEST_FAST_FIRST_RECORDED 100
#define TEST_FAST_PAUSE_BEGIN 1000
#define TEST_FAST_PAUSE_END 1500

/* Stream 1, "250Hz": one channel, 3 s. */
#define TEST_SLOW_RATE 250.0
#define TEST_SLOW_CHANNELS 1
#define TEST_SLOW_SAMPLES 750

/* Values in µV, with fractions to check the rounding (half away from zero). */
static inline double test_fast_value(int64_t sample, int channel)
{
    if (sample < TEST_FAST_FIRST_RECORDED || (sample >= TEST_FAST_PAUSE_BEGIN && sample < TEST_FAST_PAUSE_END))
        return 0.0;
    const double value = (double)(sample % 1000) + 0.5;
    return channel == 0 ? value : -value - 10000.0;
}

static inline double test_slow_value(int64_t sample)
{
    return (double)sample * 2.0;
}

/* Event list 0, "Trials": "start" at 100 ms and 1600 ms, "stop" at 900 ms. List 1, "Nothing", is
 * empty. */
#define TEST_EVENT_COUNT 3
static const int64_t test_event_times_ns[TEST_EVENT_COUNT] = {100000000, 900000000, 1600000000};
static const int32_t test_event_labels[TEST_EVENT_COUNT] = {0, 1, 0};

#endif
