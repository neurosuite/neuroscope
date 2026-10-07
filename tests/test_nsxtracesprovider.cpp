/***************************************************************************
                          test_nsxtracesprovider.cpp  -  description
                             -------------------
    purpose              : Tests for reading Blackrock NSX files
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

#include "nsxtracesprovider.h"
#include "testutils.h"

#include <QTemporaryDir>
#include <QtTest>

using namespace testutils;

namespace
{

std::int16_t raw(std::int64_t sample, int channel)
{
    return static_cast<std::int16_t>(channel * 1000 + sample * 3 - 1500);
}

const QList<NsxChannel> CHANNELS = {
    {"elec1", -32764, 32764, -8191, 8191, "uV"},
    {"elec2", -32764, 32764, -8191, 8191, "uV"},
    {"ainp1", -32764, 32764, -5000, 5000, "mV"},
};

/** Conversion to µV from the channel's digital and analog ranges.
  * The provider truncates the converted value instead of rounding it, so a value that is a whole number
  * of µV in exact arithmetic can come out 1 µV lower, depending on how the compiler evaluates the
  * expression (e.g. with fused multiply-add on arm64). Converted values are compared with a tolerance of
  * 1 µV. */
std::int64_t toMicroVolts(std::int16_t value, const NsxChannel& channel)
{
    const double unit = channel.unit == "mV" ? 1000 : 1;
    const double digitalRange = channel.maxDigital - channel.minDigital;
    const double analogRange = channel.maxAnalog - channel.minAnalog;
    return static_cast<std::int64_t>(((value - channel.minDigital) / digitalRange * analogRange + channel.minAnalog) * unit);
}

bool closeTo(std::int64_t actual, std::int64_t expected)
{
    return qAbs(actual - expected) <= 1;
}

Matrix request(NSXTracesProvider& provider, std::int64_t startTime, std::int64_t endTime,
               std::int64_t startTimeInRecordingUnits = 0)
{
    Matrix result;
    int emitted = 0;
    // NSXTracesProvider declares its own dataReady signal, which hides TracesProvider::dataReady.
    auto connection = QObject::connect(&provider, &NSXTracesProvider::dataReady,
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

class TestNSXTracesProvider : public QObject
{
    Q_OBJECT

  private:
    QTemporaryDir dir;

    QString path(const QString& name) const { return dir.filePath(name); }

  private Q_SLOTS:
    void initTestCase()
    {
        QVERIFY(dir.isValid());
        // Sampling period 30 at the 30 kHz base rate: 1 kHz, 1000 samples.
        writeFile(path("session.ns2"), nsxFile(30, CHANNELS, 1000, raw));
        writeFile(path("unknownunit.ns2"), nsxFile(30, {{"elec1", -32764, 32764, -8191, 8191, "V"}}, 100, raw));

        QByteArray truncated = nsxFile(30, CHANNELS, 0, raw);
        truncated.chop(sizeof(NSXDataHeader) + 10);
        writeFile(path("truncated.ns2"), truncated);
    }

    void metadataFromHeader()
    {
        NSXTracesProvider provider(path("session.ns2"));
        QVERIFY(provider.init());
        QCOMPARE(provider.getNbChannels(), 3);
        QCOMPARE(provider.getSamplingRate(), 1000.0);
        QCOMPARE(provider.getResolution(), 16);
        QCOMPARE(provider.getOffset(), 0);
        QCOMPARE(std::int64_t(provider.recordingLength()), 1000_i64);
        QCOMPARE(std::int64_t(provider.getTotalNbSamples()), 1000_i64);
        QCOMPARE(provider.getLabels(), QStringList({"elec1", "elec2", "ainp1"}));
    }

    void samplingRateFromPeriod()
    {
        writeFile(path("fast.ns5"), nsxFile(1, CHANNELS, 3000, raw));
        NSXTracesProvider provider(path("fast.ns5"));
        QVERIFY(provider.init());
        QCOMPARE(provider.getSamplingRate(), 30000.0);
        QCOMPARE(std::int64_t(provider.recordingLength()), 100_i64);
    }

    void readWindow_data()
    {
        QTest::addColumn<std::int64_t>("startTime");
        QTest::addColumn<std::int64_t>("endTime");
        QTest::addColumn<std::int64_t>("startTimeInRecordingUnits");
        QTest::addColumn<std::int64_t>("firstSample");
        QTest::addColumn<std::int64_t>("nbSamples");

        // Unlike .dat files, the sample at the end time is not included.
        QTest::newRow("window") << 100_i64 << 200_i64 << 0_i64 << 100_i64 << 100_i64;
        QTest::newRow("start of file") << 0_i64 << 10_i64 << 0_i64 << 0_i64 << 10_i64;
        QTest::newRow("end of file") << 900_i64 << 1000_i64 << 0_i64 << 900_i64 << 100_i64;
        QTest::newRow("start in recording units") << 100_i64 << 200_i64 << 50_i64 << 50_i64 << 150_i64;
    }

    void readWindow()
    {
        QFETCH(std::int64_t, startTime);
        QFETCH(std::int64_t, endTime);
        QFETCH(std::int64_t, startTimeInRecordingUnits);
        QFETCH(std::int64_t, firstSample);
        QFETCH(std::int64_t, nbSamples);

        NSXTracesProvider provider(path("session.ns2"));
        QVERIFY(provider.init());
        QCOMPARE(std::int64_t(provider.getNbSamples(static_cast<long>(startTime), static_cast<long>(endTime),
                                                  static_cast<long>(startTimeInRecordingUnits))),
                 nbSamples);

        const Matrix data = request(provider, startTime, endTime, startTimeInRecordingUnits);
        QCOMPARE(data.rows, nbSamples);
        QCOMPARE(data.cols, 3_i64);
        for (std::int64_t s = 0; s < nbSamples; ++s)
            for (int c = 0; c < 3; ++c)
                QVERIFY2(closeTo(data(s + 1, c + 1), toMicroVolts(raw(firstSample + s, c), CHANNELS[c])),
                         qPrintable(QString("sample %1, channel %2: %3").arg(firstSample + s).arg(c).arg(data(s + 1, c + 1))));
    }

    void conversionToMicroVolts()
    {
        // 0.25 µV per bit for the electrodes, 152.6 µV per bit for the analog input in mV.
        NSXTracesProvider provider(path("session.ns2"));
        QVERIFY(provider.init());
        const Matrix data = request(provider, 500, 501);
        QCOMPARE(data.rows, 1_i64);
        QCOMPARE(raw(500, 0), std::int16_t(0));
        QCOMPARE(data(1, 1), 0_i64);
        QCOMPARE(raw(500, 1), std::int16_t(1000));
        QVERIFY(closeTo(data(1, 2), 250));
        QCOMPARE(raw(500, 2), std::int16_t(2000));
        QVERIFY(closeTo(data(1, 3), 305213));
    }

    void windowPastEndOfFileIsEmpty()
    {
        NSXTracesProvider provider(path("session.ns2"));
        QVERIFY(provider.init());
        QVERIFY(request(provider, 950, 1100).isEmpty());
    }

    void unknownUnitIsEmpty()
    {
        NSXTracesProvider provider(path("unknownunit.ns2"));
        QVERIFY(provider.init());
        QVERIFY(request(provider, 0, 10).isEmpty());
    }

    void notInitialized()
    {
        NSXTracesProvider provider(path("session.ns2"));
        QVERIFY(request(provider, 0, 10).isEmpty());
    }

    void initFailures()
    {
        QVERIFY(!NSXTracesProvider(path("missing.ns2")).init());
        QVERIFY(!NSXTracesProvider(path("truncated.ns2")).init());
    }
};

QTEST_GUILESS_MAIN(TestNSXTracesProvider)
#include "test_nsxtracesprovider.moc"
