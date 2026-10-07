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

int16_t raw(long sample, int channel)
{
    return static_cast<int16_t>(channel * 1000 + sample * 3 - 1500);
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
dataType toMicroVolts(int16_t value, const NsxChannel& channel)
{
    const double unit = channel.unit == "mV" ? 1000 : 1;
    const double digitalRange = channel.maxDigital - channel.minDigital;
    const double analogRange = channel.maxAnalog - channel.minAnalog;
    return static_cast<dataType>(((value - channel.minDigital) / digitalRange * analogRange + channel.minAnalog) * unit);
}

bool closeTo(dataType actual, dataType expected)
{
    return qAbs(actual - expected) <= 1;
}

Matrix request(NSXTracesProvider& provider, long startTime, long endTime, long startTimeInRecordingUnits = 0)
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
    provider.requestData(startTime, endTime, nullptr, startTimeInRecordingUnits);
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
        QCOMPARE(provider.recordingLength(), 1000LL);
        QCOMPARE(provider.getTotalNbSamples(), 1000L);
        QCOMPARE(provider.getLabels(), QStringList({"elec1", "elec2", "ainp1"}));
    }

    void samplingRateFromPeriod()
    {
        writeFile(path("fast.ns5"), nsxFile(1, CHANNELS, 3000, raw));
        NSXTracesProvider provider(path("fast.ns5"));
        QVERIFY(provider.init());
        QCOMPARE(provider.getSamplingRate(), 30000.0);
        QCOMPARE(provider.recordingLength(), 100LL);
    }

    void readWindow_data()
    {
        QTest::addColumn<long>("startTime");
        QTest::addColumn<long>("endTime");
        QTest::addColumn<long>("startTimeInRecordingUnits");
        QTest::addColumn<long>("firstSample");
        QTest::addColumn<long>("nbSamples");

        // Unlike .dat files, the sample at the end time is not included.
        QTest::newRow("window") << 100L << 200L << 0L << 100L << 100L;
        QTest::newRow("start of file") << 0L << 10L << 0L << 0L << 10L;
        QTest::newRow("end of file") << 900L << 1000L << 0L << 900L << 100L;
        QTest::newRow("start in recording units") << 100L << 200L << 50L << 50L << 150L;
    }

    void readWindow()
    {
        QFETCH(long, startTime);
        QFETCH(long, endTime);
        QFETCH(long, startTimeInRecordingUnits);
        QFETCH(long, firstSample);
        QFETCH(long, nbSamples);

        NSXTracesProvider provider(path("session.ns2"));
        QVERIFY(provider.init());
        QCOMPARE(provider.getNbSamples(startTime, endTime, startTimeInRecordingUnits), nbSamples);

        const Matrix data = request(provider, startTime, endTime, startTimeInRecordingUnits);
        QCOMPARE(data.rows, nbSamples);
        QCOMPARE(data.cols, 3L);
        for (long s = 0; s < nbSamples; ++s)
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
        QCOMPARE(data.rows, 1L);
        QCOMPARE(raw(500, 0), int16_t(0));
        QCOMPARE(data(1, 1), 0L);
        QCOMPARE(raw(500, 1), int16_t(1000));
        QVERIFY(closeTo(data(1, 2), 250));
        QCOMPARE(raw(500, 2), int16_t(2000));
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
