/***************************************************************************
                          test_tracesprovider.cpp  -  description
                             -------------------
    purpose              : Tests for reading .dat, .eeg and Neuralynx .ncs files
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

#include "testutils.h"
#include "tracesprovider.h"

#include <QTemporaryDir>
#include <QtTest>

#include <cmath>

using namespace testutils;

namespace
{

const int VOLTAGE_RANGE = 20;
const int AMPLIFICATION = 1000;

/** Gain from raw values to µV, as documented for the parameter file. */
double gain(int resolution)
{
    return VOLTAGE_RANGE * 1e6 / (std::pow(2.0, resolution) * AMPLIFICATION);
}

std::int16_t raw16(std::int64_t sample, int channel)
{
    return static_cast<std::int16_t>(channel * 4000 + (sample % 3000) - 6000);
}

std::int32_t raw32(std::int64_t sample, int channel)
{
    return static_cast<std::int32_t>(channel * 100000 + sample - 50000);
}

std::int16_t rawNcs(std::int64_t sample, int channel)
{
    return static_cast<std::int16_t>(channel * 2000 + sample - 1000);
}

/** Requests a window and returns the emitted data. */
Matrix request(TracesProvider& provider, std::int64_t startTime, std::int64_t endTime,
               std::int64_t startTimeInRecordingUnits = 0)
{
    Matrix result;
    int emitted = 0;
    auto connection = QObject::connect(&provider, &TracesProvider::dataReady,
                                       [&](Array<dataType>& data, QObject*)
                                       {
                                           result = toMatrix(data);
                                           ++emitted;
                                       });
    provider.requestData(static_cast<long>(startTime), static_cast<long>(endTime), nullptr,
                         static_cast<long>(startTimeInRecordingUnits));
    QObject::disconnect(connection);
    if (emitted != 1)
        qFatal("dataReady emitted %d times", emitted);
    return result;
}

} // namespace

class TestTracesProvider : public QObject
{
    Q_OBJECT

  private:
    QTemporaryDir dir;

    QString path(const QString& name) const { return dir.filePath(name); }

  private Q_SLOTS:
    void initTestCase()
    {
        QVERIFY(dir.isValid());
        // 4 channels, 2000 samples: 2 s at 1 kHz or 1.6 s at 1250 Hz.
        writeFile(path("session.dat"), interleaved<std::int16_t>(2000, 4, raw16));
        writeFile(path("session32.dat"), interleaved<std::int32_t>(2000, 4, raw32));
        // Neuralynx: one file per channel, 1500 samples (two full records and a partial one).
        writeFile(path("CSC1.ncs"), ncsFile(1500, [](std::int64_t s) { return rawNcs(s, 0); }));
        writeFile(path("CSC2.ncs"), ncsFile(1500, [](std::int64_t s) { return rawNcs(s, 1); }));
        // Zero-padded channel numbers.
        writeFile(path("TT01.ncs"), ncsFile(1500, [](std::int64_t s) { return rawNcs(s, 0); }));
        writeFile(path("TT02.ncs"), ncsFile(1500, [](std::int64_t s) { return rawNcs(s, 1); }));
    }

    void recordingLength_data()
    {
        QTest::addColumn<QString>("file");
        QTest::addColumn<int>("resolution");
        QTest::addColumn<double>("samplingRate");
        QTest::addColumn<std::int64_t>("length");
        QTest::addColumn<std::int64_t>("totalNbSamples");

        QTest::newRow("16 bit at 1 kHz") << "session.dat" << 16 << 1000.0 << 2000_i64 << 2000_i64;
        QTest::newRow("16 bit at 1250 Hz") << "session.dat" << 16 << 1250.0 << 1600_i64 << 2000_i64;
        QTest::newRow("12 bit is stored as 16 bit") << "session.dat" << 12 << 1000.0 << 2000_i64 << 2000_i64;
        QTest::newRow("32 bit at 1 kHz") << "session32.dat" << 32 << 1000.0 << 2000_i64 << 2000_i64;
        // Lengths are truncated to whole milliseconds.
        QTest::newRow("20 kHz") << "session.dat" << 16 << 20000.0 << 100_i64 << 2000_i64;
        QTest::newRow("non-integer duration") << "session.dat" << 16 << 3000.0 << 666_i64 << 1998_i64;
    }

    void recordingLength()
    {
        QFETCH(QString, file);
        QFETCH(int, resolution);
        QFETCH(double, samplingRate);
        QFETCH(std::int64_t, length);
        QFETCH(std::int64_t, totalNbSamples);

        TracesProvider provider(path(file), 4, resolution, VOLTAGE_RANGE, AMPLIFICATION, samplingRate, 0);
        QCOMPARE(std::int64_t(provider.recordingLength()), length);
        QCOMPARE(std::int64_t(provider.getTotalNbSamples()), totalNbSamples);
    }

    void recordingLengthFollowsParameterChanges()
    {
        TracesProvider provider(path("session.dat"), 4, 16, VOLTAGE_RANGE, AMPLIFICATION, 1000.0, 0);
        provider.setNbChannels(2);
        QCOMPARE(std::int64_t(provider.recordingLength()), 4000_i64);
        provider.setSamplingRate(2000.0);
        QCOMPARE(std::int64_t(provider.recordingLength()), 2000_i64);
        provider.setResolution(32);
        QCOMPARE(std::int64_t(provider.recordingLength()), 1000_i64);
    }

    void readWindow_data()
    {
        QTest::addColumn<double>("samplingRate");
        QTest::addColumn<std::int64_t>("startTime");
        QTest::addColumn<std::int64_t>("endTime");
        QTest::addColumn<std::int64_t>("startTimeInRecordingUnits");
        QTest::addColumn<std::int64_t>("firstSample");
        QTest::addColumn<std::int64_t>("nbSamples");

        // Both ends are included.
        QTest::newRow("1 kHz") << 1000.0 << 100_i64 << 199_i64 << 0_i64 << 100_i64 << 100_i64;
        QTest::newRow("single sample") << 1000.0 << 0_i64 << 0_i64 << 0_i64 << 0_i64 << 1_i64;
        QTest::newRow("start of file") << 1000.0 << 0_i64 << 999_i64 << 0_i64 << 0_i64 << 1000_i64;
        // Times are converted to samples by truncation: 10 ms -> 12.5 -> 12, 20 ms -> 25.
        QTest::newRow("1250 Hz") << 1250.0 << 10_i64 << 20_i64 << 0_i64 << 12_i64 << 14_i64;
        // A window ending at the recording length excludes the sample at the end time.
        QTest::newRow("end of file") << 1000.0 << 1900_i64 << 2000_i64 << 0_i64 << 1900_i64 << 100_i64;
        // A start in recording units, from a previous browsing request, replaces the start time.
        QTest::newRow("start in recording units") << 1000.0 << 100_i64 << 199_i64 << 50_i64 << 50_i64 << 150_i64;
    }

    void readWindow()
    {
        QFETCH(double, samplingRate);
        QFETCH(std::int64_t, startTime);
        QFETCH(std::int64_t, endTime);
        QFETCH(std::int64_t, startTimeInRecordingUnits);
        QFETCH(std::int64_t, firstSample);
        QFETCH(std::int64_t, nbSamples);

        TracesProvider provider(path("session.dat"), 4, 16, VOLTAGE_RANGE, AMPLIFICATION, samplingRate, 0);
        const Matrix data = request(provider, startTime, endTime, startTimeInRecordingUnits);

        QCOMPARE(data.rows, nbSamples);
        QCOMPARE(data.cols, 4_i64);
        for (std::int64_t s = 0; s < nbSamples; ++s)
            for (int c = 0; c < 4; ++c)
                QCOMPARE(data(s + 1, c + 1), roundHalfAway(raw16(firstSample + s, c) * gain(16)));
    }

    void gainUsesResolution()
    {
        // 12 and 14 bit data are stored as 16 bit values, but the gain uses the given resolution.
        TracesProvider provider(path("session.dat"), 4, 12, VOLTAGE_RANGE, AMPLIFICATION, 1000.0, 0);
        const Matrix data = request(provider, 10, 10);
        QCOMPARE(data.rows, 1_i64);
        for (int c = 0; c < 4; ++c)
            QCOMPARE(data(1, c + 1), roundHalfAway(raw16(10, c) * gain(12)));
    }

    void offsetIsSubtracted()
    {
        // Current behaviour: with a non-zero offset, the raw value is not scaled, only the offset is:
        // value = raw - offset * gain. Without an offset, value = raw * gain (#17).
        const int offset = 100;
        TracesProvider provider(path("session.dat"), 4, 16, VOLTAGE_RANGE, AMPLIFICATION, 1000.0, offset);
        const Matrix data = request(provider, 10, 11);
        QCOMPARE(data.rows, 2_i64);
        for (std::int64_t s = 0; s < 2; ++s)
            for (int c = 0; c < 4; ++c)
                QCOMPARE(data(s + 1, c + 1), roundHalfAway(raw16(10 + s, c) - offset * gain(16)));
    }

    void read32Bit()
    {
        TracesProvider provider(path("session32.dat"), 4, 32, VOLTAGE_RANGE, AMPLIFICATION, 1000.0, 0);
        const Matrix data = request(provider, 100, 109);
        QCOMPARE(data.rows, 10_i64);
        QVector<std::int64_t> expected;
        for (std::int64_t s = 0; s < 10; ++s)
            for (int c = 0; c < 4; ++c)
                expected.append(roundHalfAway(raw32(100 + s, c) * gain(32)));
        if (sizeof(dataType) != sizeof(std::int32_t))
            QEXPECT_FAIL("", "32 bit samples are read into an array of long, which has 64 bits on Linux and macOS (#13)", Abort);
        QCOMPARE(data.values, expected);
    }

    void windowPastEndOfFileIsEmpty()
    {
        // The caller is expected to clip windows to the recording; a window past the end gives no data.
        TracesProvider provider(path("session.dat"), 4, 16, VOLTAGE_RANGE, AMPLIFICATION, 1000.0, 0);
        QVERIFY(request(provider, 1950, 2100).isEmpty());
    }

    void recordingShorterThanWindowIsEmpty()
    {
        // 2000 samples at 1 kHz is 2 s; a 5 s window from the start gives no data (see #2).
        TracesProvider provider(path("session.dat"), 4, 16, VOLTAGE_RANGE, AMPLIFICATION, 1000.0, 0);
        QVERIFY(request(provider, 0, 4999).isEmpty());
    }

    void missingFile()
    {
        TracesProvider provider(path("missing.dat"), 4, 16, VOLTAGE_RANGE, AMPLIFICATION, 1000.0, 0);
        QCOMPARE(std::int64_t(provider.recordingLength()), 0_i64);
        QCOMPARE(std::int64_t(provider.getTotalNbSamples()), 0_i64);
        QVERIFY(request(provider, 0, 9).isEmpty());
    }

    void labelsAreChannelIndices()
    {
        TracesProvider provider(path("session.dat"), 4, 16, VOLTAGE_RANGE, AMPLIFICATION, 1000.0, 0);
        QCOMPARE(provider.getLabels(), QStringList({"0", "1", "2", "3"}));
    }

    void ncsRecordingLength()
    {
        // The length is computed from the first channel's file, with one channel per file.
        TracesProvider provider(path("CSC1.ncs"), 2, 16, VOLTAGE_RANGE, AMPLIFICATION, 1000.0, 0);
        // The partial last record is padded to 512 samples in the file.
        QCOMPARE(std::int64_t(provider.recordingLength()), 1536_i64);
    }

    void ncsReadWindow_data()
    {
        QTest::addColumn<QString>("file");
        QTest::addColumn<std::int64_t>("startTime");
        QTest::addColumn<std::int64_t>("endTime");

        QTest::newRow("first record to second") << "CSC1.ncs" << 100_i64 << 700_i64;
        QTest::newRow("spanning three records") << "CSC1.ncs" << 500_i64 << 1100_i64;
        QTest::newRow("record boundary") << "CSC1.ncs" << 512_i64 << 1100_i64;
        QTest::newRow("zero-padded file names") << "TT01.ncs" << 100_i64 << 700_i64;
    }

    void ncsReadWindow()
    {
        QFETCH(QString, file);
        QFETCH(std::int64_t, startTime);
        QFETCH(std::int64_t, endTime);

        TracesProvider provider(path(file), 2, 16, VOLTAGE_RANGE, AMPLIFICATION, 1000.0, 0);
        const Matrix data = request(provider, startTime, endTime);

        const std::int64_t nbSamples = endTime - startTime + 1;
        QCOMPARE(data.rows, nbSamples);
        QCOMPARE(data.cols, 2_i64);
        for (std::int64_t s = 0; s < nbSamples - 1; ++s)
            for (int c = 0; c < 2; ++c)
                QCOMPARE(data(s + 1, c + 1), roundHalfAway(rawNcs(startTime + s, c) * gain(16)));

        const QVector<std::int64_t> lastSample = {data(nbSamples, 1), data(nbSamples, 2)};
        const QVector<std::int64_t> expectedLastSample = {roundHalfAway(rawNcs(endTime, 0) * gain(16)),
                                                          roundHalfAway(rawNcs(endTime, 1) * gain(16))};
        QEXPECT_FAIL("", "The last sample of the window is not read from .ncs files (off by one in inLastRecord, #14)", Abort);
        QCOMPARE(lastSample, expectedLastSample);
    }

    void ncsMissingChannelFileIsEmpty()
    {
        // Three channels requested, but there is no CSC3.ncs.
        TracesProvider provider(path("CSC1.ncs"), 3, 16, VOLTAGE_RANGE, AMPLIFICATION, 1000.0, 0);
        QVERIFY(request(provider, 100, 700).isEmpty());
    }
};

QTEST_GUILESS_MAIN(TestTracesProvider)
#include "test_tracesprovider.moc"
