/***************************************************************************
                          test_nevproviders.cpp  -  description
                             -------------------
    purpose              : Tests for reading spikes and events from Blackrock NEV files
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

#include "nevclustersprovider.h"
#include "neveventsprovider.h"
#include "testutils.h"

#include <QTemporaryDir>
#include <QtTest>

#include <memory>

using namespace testutils;

namespace
{

const std::uint32_t TIME_RESOLUTION = 30000;

// Electrode ids and labels. The labels are requested by NeuroScope from the NSX file. The tests never
// request elec3, the last label, which would hit undefined behaviour in the duplicate check (#21).
const QList<QPair<std::uint16_t, QString>> ELECTRODES = {{1, "elec1"}, {2, "elec2"}, {3, "elec3"}};

// Spikes (electrode id, unit) and events, with timestamps in 30 kHz ticks.
const QList<NevPacket> PACKETS = {
    {3000, 1, 1, 0},                          // 100 ms, elec1 unit 1
    {6000, 2, 0, 0},                          // 200 ms, elec2 unclassified
    {9000, NEVDigitalSerialDataID, 1, 0x0f},  // 300 ms, digital input
    {15000, 1, 2, 0},                         // 500 ms, elec1 unit 2
    {30000, NEVButtonDataID, 0, 1},           // 1000 ms, button press
    {45000, 1, 1, 0},                         // 1500 ms, elec1 unit 1
    {60000, NEVDigitalSerialDataID, 0x80, 7}, // 2000 ms, serial input
    {75000, 3, 1, 0},                         // 2500 ms, elec3 unit 1
    {90000, NEVConfigurationDataID, 0, 0},    // 3000 ms, configuration change
};

struct ClusterResult
{
    Matrix data;
    QString name;
};

ClusterResult request(ClustersProvider& provider, std::int64_t startTime, std::int64_t endTime)
{
    ClusterResult result;
    auto connection = QObject::connect(&provider, &ClustersProvider::dataReady,
                                       [&](Array<dataType>& data, QObject*, QString name) { result = {toMatrix(data), name}; });
    provider.requestData(static_cast<long>(startTime), static_cast<long>(endTime), nullptr, 0);
    QObject::disconnect(connection);
    return result;
}

QString describe(const Matrix& m)
{
    QStringList columns;
    for (std::int64_t c = 1; c <= m.cols; ++c)
        columns << QString("(%1, %2)").arg(m(1, c)).arg(m(2, c));
    return columns.join(' ');
}

} // namespace

class TestNEVProviders : public QObject
{
    Q_OBJECT

  private:
    QTemporaryDir dir;

    QString path(const QString& name) const { return dir.filePath(name); }

  private Q_SLOTS:
    void initTestCase()
    {
        QVERIFY(dir.isValid());
        writeFile(path("session.nev"), nevFile(TIME_RESOLUTION, ELECTRODES, PACKETS));
    }

    void clustersPerRequestedChannel()
    {
        QList<NEVClustersProvider*> providers = NEVClustersProvider::fromFile(path("session.nev"), {"elec1", "elec2"},
                                                                              TIME_RESOLUTION, 120000, 25);
        QCOMPARE(providers.size(), 2);
        std::unique_ptr<NEVClustersProvider> elec1(providers[0]);
        std::unique_ptr<NEVClustersProvider> elec2(providers[1]);

        // Providers are named after their position in the requested channel list, starting at 1.
        QCOMPARE(elec1->getName(), QString("1"));
        QCOMPARE(elec2->getName(), QString("2"));
        QCOMPARE(elec1->loadData(), int(ClustersProvider::OK));

        // Clusters are the unit classifications, in order of appearance.
        QCOMPARE(elec1->clusterIdList(), QList<int>({1, 2}));
        QCOMPARE(elec2->clusterIdList(), QList<int>({0}));

        // Spike times in ticks relative to the window start.
        QCOMPARE(describe(request(*elec1, 0, 1000).data), QString("(3000, 1) (15000, 2)"));
        QCOMPARE(describe(request(*elec1, 400, 1600).data), QString("(3000, 2) (33000, 1)"));
        QCOMPARE(describe(request(*elec2, 0, 1000).data), QString("(6000, 0)"));
    }

    void clustersAtLowerDisplaySamplingRate()
    {
        // Showing the spikes over a 1 kHz signal: times are converted to the display sampling rate.
        QList<NEVClustersProvider*> providers = NEVClustersProvider::fromFile(path("session.nev"), {"elec1", "elec2"},
                                                                              1000.0, 4000, 25);
        QCOMPARE(providers.size(), 2);
        std::unique_ptr<NEVClustersProvider> elec1(providers[0]);
        std::unique_ptr<NEVClustersProvider> elec2(providers[1]);
        QCOMPARE(describe(request(*elec1, 400, 1600).data), QString("(100, 2) (1100, 1)"));
    }

    void unknownLabelGivesNoProviders()
    {
        QVERIFY(NEVClustersProvider::fromFile(path("session.nev"), {"elec1", "nolabel"}, TIME_RESOLUTION, 120000, 25).isEmpty());
    }

    void missingFileGivesNoProviders()
    {
        QVERIFY(NEVClustersProvider::fromFile(path("missing.nev"), {"elec1"}, TIME_RESOLUTION, 120000, 25).isEmpty());
    }

    void events()
    {
        NEVEventsProvider provider(path("session.nev"), 25);
        QCOMPARE(provider.getName(), QString("nev"));
        QCOMPARE(provider.loadData(), int(EventsProvider::OK));

        // Spike packets are skipped.
        QCOMPARE(provider.getNbEvents(), 4);
        const QMap<int, EventDescription> descriptions = provider.eventIdDescriptionMap();
        QCOMPARE(QStringList(descriptions.begin(), descriptions.end()),
                 QStringList({"button press", "config change normal", "digital data", "serial data"}));
    }

    void eventTimes()
    {
        NEVEventsProvider provider(path("session.nev"), 25);
        QCOMPARE(provider.loadData(), int(EventsProvider::OK));
        const int digital = provider.eventDescriptionIdMap().value(EventDescription("digital data"));
        const int button = provider.eventDescriptionIdMap().value(EventDescription("button press"));

        Matrix times;
        Matrix ids;
        connect(&provider, &EventsProvider::dataReady,
                [&](Array<dataType>& t, Array<int>& i, QObject*, QString)
                {
                    times = toMatrix(t);
                    ids = toMatrix(i);
                });

        // The digital event at 300 ms and the button press at 1000 ms, in ticks from the window start.
        provider.requestData(0, 1100, nullptr);
        QEXPECT_FAIL("", "NEV event timestamps are kept in ticks, but the window is given in ms (#16)", Abort);
        QCOMPARE(ids.values, QVector<std::int64_t>({digital, button}));
        QCOMPARE(times.values, QVector<std::int64_t>({9000, 30000}));
    }

    void missingEventFile()
    {
        NEVEventsProvider provider(path("missing.nev"), 25);
        QCOMPARE(provider.loadData(), int(EventsProvider::OPEN_ERROR));
    }
};

QTEST_GUILESS_MAIN(TestNEVProviders)
#include "test_nevproviders.moc"
