/***************************************************************************
                          test_clustersprovider.cpp  -  description
                             -------------------
    purpose              : Tests for reading .clu and .res files
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

#include "clustersprovider.h"
#include "testutils.h"

#include <QRandomGenerator>
#include <QTemporaryDir>
#include <QtTest>

#include <cmath>

using namespace testutils;

namespace
{

const double SAMPLING_RATE = 20000.0;

struct Spike
{
    std::int64_t time; // in samples
    int cluster;
};

/** Writes X.clu.n and X.res.n for the given spikes. */
void writeClusterFiles(const QString& cluPath, const QString& resPath, const QList<Spike>& spikes, int nbClusters)
{
    QString clu = QString::number(nbClusters) + "\n";
    QString res;
    for (const Spike& spike : spikes)
    {
        clu += QString::number(spike.cluster) + "\n";
        res += QString::number(spike.time) + "\n";
    }
    writeTextFile(cluPath, clu);
    writeTextFile(resPath, res);
}

struct Result
{
    Matrix data;
    QString name;
    std::int64_t startingTime = -1;
    std::int64_t startingTimeInRecordingUnits = -1;
};

Result request(ClustersProvider& provider, std::int64_t startTime, std::int64_t endTime)
{
    Result result;
    int emitted = 0;
    auto connection = QObject::connect(&provider, &ClustersProvider::dataReady,
                                       [&](Array<dataType>& data, QObject*, QString name)
                                       {
                                           result.data = toMatrix(data);
                                           result.name = name;
                                           ++emitted;
                                       });
    provider.requestData(static_cast<long>(startTime), static_cast<long>(endTime), nullptr, 0);
    QObject::disconnect(connection);
    if (emitted != 1)
        qFatal("dataReady emitted %d times", emitted);
    return result;
}

Result requestNext(ClustersProvider& provider, std::int64_t startTime, std::int64_t timeFrame, const QList<int>& selectedIds,
                   std::int64_t startTimeInRecordingUnits)
{
    Result result;
    int emitted = 0;
    auto connection = QObject::connect(&provider, &ClustersProvider::nextClusterDataReady,
                                       [&](Array<dataType>& data, QObject*, QString name, long startingTime, long startingTimeInRecordingUnits)
                                       {
                                           result = {toMatrix(data), name, startingTime, startingTimeInRecordingUnits};
                                           ++emitted;
                                       });
    provider.requestNextClusterData(static_cast<long>(startTime), static_cast<long>(timeFrame), selectedIds, nullptr,
                                    static_cast<long>(startTimeInRecordingUnits));
    QObject::disconnect(connection);
    if (emitted != 1)
        qFatal("nextClusterDataReady emitted %d times", emitted);
    return result;
}

Result requestPrevious(ClustersProvider& provider, std::int64_t startTime, std::int64_t timeFrame,
                       const QList<int>& selectedIds, std::int64_t startTimeInRecordingUnits)
{
    Result result;
    int emitted = 0;
    auto connection = QObject::connect(&provider, &ClustersProvider::previousClusterDataReady,
                                       [&](Array<dataType>& data, QObject*, QString name, long startingTime, long startingTimeInRecordingUnits)
                                       {
                                           result = {toMatrix(data), name, startingTime, startingTimeInRecordingUnits};
                                           ++emitted;
                                       });
    provider.requestPreviousClusterData(static_cast<long>(startTime), static_cast<long>(timeFrame), selectedIds, nullptr,
                                        static_cast<long>(startTimeInRecordingUnits));
    QObject::disconnect(connection);
    if (emitted != 1)
        qFatal("previousClusterDataReady emitted %d times", emitted);
    return result;
}

/** The spikes of a window computed directly from the spike list. */
Matrix expectedWindow(const QList<Spike>& spikes, std::int64_t startTime, std::int64_t endTime, double currentSamplingRate)
{
    const std::int64_t fileMaxTime = static_cast<std::int64_t>(std::floor(0.5 + spikes.last().time * 1000.0 / SAMPLING_RATE));
    Matrix m;
    m.rows = 2;
    if (startTime > fileMaxTime)
        return m;
    endTime = qMin(endTime, fileMaxTime);

    const std::int64_t start = static_cast<std::int64_t>(startTime * (SAMPLING_RATE / 1000.0));
    const std::int64_t end = endTime == fileMaxTime ? spikes.last().time : static_cast<std::int64_t>(endTime * (SAMPLING_RATE / 1000.0));
    const float ratio = static_cast<float>(SAMPLING_RATE / currentSamplingRate);

    QVector<std::int64_t> times;
    QVector<std::int64_t> ids;
    for (const Spike& spike : spikes)
    {
        if (spike.time < start || spike.time > end)
            continue;
        if (ratio == 1)
            times.append(spike.time - start);
        else
            times.append(static_cast<std::int64_t>(std::floor(0.5 + (spike.time - start) / static_cast<double>(ratio))));
        ids.append(spike.cluster);
    }
    m.cols = times.size();
    m.values = times + ids;
    return m;
}

QString describe(const Matrix& m)
{
    QStringList columns;
    for (std::int64_t c = 1; c <= m.cols; ++c)
        columns << QString("(%1, %2)").arg(m(1, c)).arg(m(2, c));
    return columns.join(' ');
}

} // namespace

class TestClustersProvider : public QObject
{
    Q_OBJECT

  private:
    QTemporaryDir dir;
    QList<Spike> manySpikes;

    // A few spikes for the browsing tests (times in samples at 20 kHz):
    // 100 ms, 200 ms, 500 ms, 1500 ms and 3000 ms.
    const QList<Spike> fewSpikes = {{2000, 2}, {4000, 3}, {10000, 2}, {30000, 3}, {60000, 2}};

    QString path(const QString& name) const { return dir.filePath(name); }

  private Q_SLOTS:
    void initTestCase()
    {
        QVERIFY(dir.isValid());

        // More than 1000 spikes, so that the bisection in the window search is used.
        QRandomGenerator random(42);
        const QList<int> clusters = {0, 1, 2, 5};
        std::int64_t time = 100;
        for (int i = 0; i < 5000; ++i)
        {
            manySpikes.append({time, clusters[random.bounded(clusters.size())]});
            time += 1 + random.bounded(400);
        }
        writeClusterFiles(path("many.clu.1"), path("many.res.1"), manySpikes, 4);
        writeClusterFiles(path("few.clu.2"), path("few.res.2"), fewSpikes, 2);
    }

    void name_data()
    {
        QTest::addColumn<QString>("file");
        QTest::addColumn<QString>("name");

        QTest::newRow("X.clu.n") << "session.clu.3" << "3";
        QTest::newRow("X.n.clu") << "session.12.clu" << "12";
    }

    void name()
    {
        QFETCH(QString, file);
        QFETCH(QString, name);
        ClustersProvider provider(path(file), SAMPLING_RATE, SAMPLING_RATE, 0);
        QCOMPARE(provider.getName(), name);
    }

    void load()
    {
        ClustersProvider provider(path("many.clu.1"), SAMPLING_RATE, SAMPLING_RATE, 0);
        QCOMPARE(provider.loadData(), int(ClustersProvider::OK));
        QCOMPARE(provider.clusterIdList(), QList<int>({0, 1, 2, 5}));
    }

    void loadErrors()
    {
        writeTextFile(path("nores.clu.4"), "1\n1\n");
        ClustersProvider noRes(path("nores.clu.4"), SAMPLING_RATE, SAMPLING_RATE, 0);
        QCOMPARE(noRes.loadData(), int(ClustersProvider::MISSING_FILE));

        // Fewer cluster ids than spike times.
        writeTextFile(path("short.clu.5"), "2\n1\n2\n");
        writeTextFile(path("short.res.5"), "10\n20\n30\n");
        ClustersProvider shortClu(path("short.clu.5"), SAMPLING_RATE, SAMPLING_RATE, 0);
        QCOMPARE(shortClu.loadData(), int(ClustersProvider::INCORRECT_CONTENT));
    }

    void readWindow_data()
    {
        QTest::addColumn<std::int64_t>("startTime");
        QTest::addColumn<std::int64_t>("endTime");

        QTest::newRow("start of file") << 0_i64 << 1000_i64;
        QTest::newRow("middle") << 20000_i64 << 21000_i64;
        QTest::newRow("single spike window") << 5_i64 << 5_i64;
        QTest::newRow("end of file") << 49000_i64 << 60000_i64;
        QTest::newRow("past end of file") << 60000_i64 << 61000_i64;
    }

    void readWindow()
    {
        QFETCH(std::int64_t, startTime);
        QFETCH(std::int64_t, endTime);

        ClustersProvider provider(path("many.clu.1"), SAMPLING_RATE, SAMPLING_RATE, 0);
        QCOMPARE(provider.loadData(), int(ClustersProvider::OK));
        const Result result = request(provider, startTime, endTime);
        QCOMPARE(result.name, QString("1"));
        const Matrix expected = expectedWindow(manySpikes, startTime, endTime, SAMPLING_RATE);
        QCOMPARE(describe(result.data), describe(expected));
    }

    void browseWindows_data()
    {
        QTest::addColumn<double>("currentSamplingRate");

        QTest::newRow(".dat sampling rate") << SAMPLING_RATE;
        QTest::newRow(".eeg sampling rate") << 1250.0;
    }

    void browseWindows()
    {
        // The provider remembers the previous window to speed up the search. Request a sequence of
        // windows (paging forward, jumping around, changing the duration) and compare each with the
        // spikes computed directly from the files.
        QFETCH(double, currentSamplingRate);

        ClustersProvider provider(path("many.clu.1"), SAMPLING_RATE, currentSamplingRate, 0);
        QCOMPARE(provider.loadData(), int(ClustersProvider::OK));

        const std::int64_t fileMaxTime = static_cast<std::int64_t>(std::floor(0.5 + manySpikes.last().time * 1000.0 / SAMPLING_RATE));
        QRandomGenerator random(7);
        std::int64_t startTime = 0;
        std::int64_t duration = 1000;
        for (int i = 0; i < 500; ++i)
        {
            switch (random.bounded(4))
            {
            case 0: // next page
                startTime += duration;
                break;
            case 1: // previous page
                startTime = qMax(0_i64, startTime - duration);
                break;
            case 2: // jump
                startTime = random.bounded(static_cast<int>(fileMaxTime + 500));
                break;
            case 3: // change the duration
                duration = 1 + random.bounded(5000);
                break;
            }
            const std::int64_t endTime = startTime + duration;
            const Matrix data = request(provider, startTime, endTime).data;
            const Matrix expected = expectedWindow(manySpikes, startTime, endTime, currentSamplingRate);
            if (describe(data) != describe(expected))
                QFAIL(qPrintable(QString("request %1: window %2-%3 ms\nactual:   %4\nexpected: %5")
                                     .arg(i).arg(startTime).arg(endTime).arg(describe(data), describe(expected))));
            if (startTime > fileMaxTime)
                startTime = 0;
        }
    }

    void nextCluster()
    {
        // Window of 1 s with the spike placed at 25 % of it.
        ClustersProvider provider(path("few.clu.2"), SAMPLING_RATE, SAMPLING_RATE, 100000, 25);
        QCOMPARE(provider.loadData(), int(ClustersProvider::OK));

        // The next spike of cluster 3 after 250 ms is at 1500 ms: the window starts at 1250 ms.
        const Result next = requestNext(provider, 0, 1000, {3}, 0);
        QCOMPARE(next.name, QString("2"));
        QCOMPARE(next.startingTime, 1250_i64);
        QCOMPARE(next.startingTimeInRecordingUnits, 25000_i64);
        QCOMPARE(describe(next.data), QString("(5000, 3)"));

        // The previous spike of cluster 3 is at 200 ms; the window cannot start before 0.
        const Result previous = requestPrevious(provider, next.startingTime, 1000, {3}, next.startingTimeInRecordingUnits);
        QCOMPARE(previous.startingTime, 0_i64);
        QCOMPARE(previous.startingTimeInRecordingUnits, 0_i64);
        QCOMPARE(describe(previous.data), QString("(2000, 2) (4000, 3) (10000, 2)"));
    }

    void nextClusterWithoutMatchingSpike()
    {
        ClustersProvider provider(path("few.clu.2"), SAMPLING_RATE, SAMPLING_RATE, 100000, 25);
        QCOMPARE(provider.loadData(), int(ClustersProvider::OK));

        // No spike of cluster 7: the start time is returned unchanged with no data.
        const Result next = requestNext(provider, 500, 1000, {7}, 10000);
        QVERIFY(next.data.isEmpty());
        QCOMPARE(next.startingTime, 500_i64);
        QCOMPARE(next.startingTimeInRecordingUnits, 10000_i64);
    }
};

QTEST_GUILESS_MAIN(TestClustersProvider)
#include "test_clustersprovider.moc"
