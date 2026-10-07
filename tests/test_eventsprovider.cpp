/***************************************************************************
                          test_eventsprovider.cpp  -  description
                             -------------------
    purpose              : Tests for reading and writing .evt files
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

#include "eventsprovider.h"
#include "testutils.h"

#include <QRandomGenerator>
#include <QTemporaryDir>
#include <QtTest>

#include <cmath>

using namespace testutils;

namespace
{

struct Event
{
    double time; // in ms
    QString description;
};

QString eventFileContent(const QList<Event>& events)
{
    QString content;
    for (const Event& event : events)
        content += QString::number(event.time, 'g', 12) + "\t" + event.description + "\n";
    return content;
}

struct Result
{
    Matrix times;
    Matrix ids;
    QString name;
    long startingTime = -1;
};

Result request(EventsProvider& provider, long startTime, long endTime)
{
    Result result;
    int emitted = 0;
    auto connection = QObject::connect(&provider, &EventsProvider::dataReady,
                                       [&](Array<dataType>& times, Array<int>& ids, QObject*, QString name)
                                       {
                                           result.times = toMatrix(times);
                                           result.ids = toMatrix(ids);
                                           result.name = name;
                                           ++emitted;
                                       });
    provider.requestData(startTime, endTime, nullptr);
    QObject::disconnect(connection);
    if (emitted != 1)
        qFatal("dataReady emitted %d times", emitted);
    return result;
}

Result requestNext(EventsProvider& provider, long startTime, long timeFrame, const QList<int>& selectedIds)
{
    Result result;
    int emitted = 0;
    auto connection = QObject::connect(&provider, &EventsProvider::nextEventDataReady,
                                       [&](Array<dataType>& times, Array<int>& ids, QObject*, QString name, long startingTime)
                                       {
                                           result = {toMatrix(times), toMatrix(ids), name, startingTime};
                                           ++emitted;
                                       });
    provider.requestNextEventData(startTime, timeFrame, selectedIds, nullptr);
    QObject::disconnect(connection);
    if (emitted != 1)
        qFatal("nextEventDataReady emitted %d times", emitted);
    return result;
}

Result requestPrevious(EventsProvider& provider, long startTime, long timeFrame, const QList<int>& selectedIds)
{
    Result result;
    int emitted = 0;
    auto connection = QObject::connect(&provider, &EventsProvider::previousEventDataReady,
                                       [&](Array<dataType>& times, Array<int>& ids, QObject*, QString name, long startingTime)
                                       {
                                           result = {toMatrix(times), toMatrix(ids), name, startingTime};
                                           ++emitted;
                                       });
    provider.requestPreviousEventData(startTime, timeFrame, selectedIds, nullptr);
    QObject::disconnect(connection);
    if (emitted != 1)
        qFatal("previousEventDataReady emitted %d times", emitted);
    return result;
}

QString describe(const Result& result)
{
    QStringList events;
    for (long c = 1; c <= result.times.cols; ++c)
        events << QString("(%1, %2)").arg(result.times(1, c)).arg(result.ids(1, c));
    return events.join(' ');
}

/** The events of a window computed directly from the event list, with times in samples from the start. */
QString expectedWindow(const QList<Event>& events, const QMap<QString, int>& ids, long startTime, long endTime,
                       double samplingRate)
{
    const long fileMaxTime = static_cast<long>(std::floor(0.5 + events.last().time));
    if (startTime > fileMaxTime)
        return {};
    endTime = qMin(endTime, fileMaxTime);

    const double samplesPerMs = samplingRate / 1000.0;
    QStringList result;
    for (const Event& event : events)
    {
        const long rounded = static_cast<long>(std::floor(0.5 + event.time));
        if (rounded < startTime || rounded > endTime)
            continue;
        const dataType time = qMax(static_cast<dataType>(std::floor(static_cast<float>(0.5 + (event.time - startTime) * samplesPerMs))), 0L);
        result << QString("(%1, %2)").arg(time).arg(ids[event.description]);
    }
    return result.join(' ');
}

} // namespace

class TestEventsProvider : public QObject
{
    Q_OBJECT

  private:
    QTemporaryDir dir;
    QList<Event> manyEvents;

    // Event descriptions are numbered in case-insensitive alphabetical order, starting at 1.
    const QMap<QString, int> ids = {{"lick", 1}, {"reward", 2}, {"Stim off", 3}, {"stim on", 4}};
    const QList<Event> fewEvents = {{100.0, "lick"}, {200.0, "reward"}, {500.0, "lick"}, {1500.0, "reward"}, {3000.0, "lick"}};

    QString path(const QString& name) const { return dir.filePath(name); }

  private Q_SLOTS:
    void initTestCase()
    {
        QVERIFY(dir.isValid());

        // More than 1000 events, so that the bisection in the window search is used.
        // Events are at least 1 ms apart.
        QRandomGenerator random(42);
        const QStringList descriptions = ids.keys();
        double time = 10.25;
        for (int i = 0; i < 5000; ++i)
        {
            manyEvents.append({time, descriptions[random.bounded(descriptions.size())]});
            time += 1.0 + random.bounded(400000) / 1000.0;
        }
        writeTextFile(path("many.abc.evt"), eventFileContent(manyEvents));
        writeTextFile(path("few.xyz.evt"), eventFileContent(fewEvents));
        writeTextFile(path("empty.emp.evt"), "");
    }

    void name_data()
    {
        QTest::addColumn<QString>("file");
        QTest::addColumn<QString>("name");

        QTest::newRow("X.id.evt") << "session.r01.evt" << "r01";
        QTest::newRow("X.evt.id") << "session.evt.r01" << "r01";
    }

    void name()
    {
        QFETCH(QString, file);
        QFETCH(QString, name);
        EventsProvider provider(path(file), 20000.0);
        QCOMPARE(provider.getName(), name);
    }

    void load()
    {
        EventsProvider provider(path("many.abc.evt"), 20000.0);
        QCOMPARE(provider.loadData(), int(EventsProvider::OK));
        QCOMPARE(provider.getNbEvents(), 5000);

        QMap<EventDescription, int> expectedIds;
        QMap<int, EventDescription> expectedDescriptions;
        for (auto it = ids.begin(); it != ids.end(); ++it)
        {
            expectedIds.insert(EventDescription(it.key()), it.value());
            expectedDescriptions.insert(it.value(), EventDescription(it.key()));
        }
        QCOMPARE(provider.eventDescriptionIdMap(), expectedIds);
        QCOMPARE(provider.eventIdDescriptionMap(), expectedDescriptions);
        QVERIFY(!provider.isModified());
    }

    void descriptionsWithSpaces()
    {
        writeTextFile(path("spaces.spc.evt"), "12.5 \t two  words here\n");
        EventsProvider provider(path("spaces.spc.evt"), 20000.0);
        QCOMPARE(provider.loadData(), int(EventsProvider::OK));
        QCOMPARE(provider.eventIdDescriptionMap().value(1), EventDescription("two  words here"));
    }

    void emptyFile()
    {
        EventsProvider provider(path("empty.emp.evt"), 20000.0);
        QCOMPARE(provider.loadData(), int(EventsProvider::OK));
        QCOMPARE(provider.getNbEvents(), 0);
        QVERIFY(request(provider, 0, 1000).times.isEmpty());
    }

    void readWindow_data()
    {
        QTest::addColumn<long>("startTime");
        QTest::addColumn<long>("endTime");

        QTest::newRow("start of file") << 0L << 1000L;
        QTest::newRow("middle") << 300000L << 301000L;
        QTest::newRow("end of file") << 990000L << 2000000L;
        QTest::newRow("past end of file") << 2000000L << 2001000L;
    }

    void readWindow()
    {
        QFETCH(long, startTime);
        QFETCH(long, endTime);

        EventsProvider provider(path("many.abc.evt"), 20000.0);
        QCOMPARE(provider.loadData(), int(EventsProvider::OK));
        const Result result = request(provider, startTime, endTime);
        QCOMPARE(result.name, QString("abc"));
        QCOMPARE(describe(result), expectedWindow(manyEvents, ids, startTime, endTime, 20000.0));
    }

    void browseWindows_data()
    {
        QTest::addColumn<double>("samplingRate");

        QTest::newRow(".dat sampling rate") << 20000.0;
        QTest::newRow(".eeg sampling rate") << 1250.0;
    }

    void browseWindows()
    {
        // See TestClustersProvider::browseWindows.
        QFETCH(double, samplingRate);

        EventsProvider provider(path("many.abc.evt"), samplingRate);
        QCOMPARE(provider.loadData(), int(EventsProvider::OK));

        const long fileMaxTime = static_cast<long>(std::floor(0.5 + manyEvents.last().time));
        QRandomGenerator random(7);
        long startTime = 0;
        long duration = 1000;
        for (int i = 0; i < 500; ++i)
        {
            switch (random.bounded(4))
            {
            case 0:
                startTime += duration;
                break;
            case 1:
                startTime = qMax(0L, startTime - duration);
                break;
            case 2:
                startTime = random.bounded(static_cast<int>(fileMaxTime + 500));
                break;
            case 3:
                duration = 1 + random.bounded(5000);
                break;
            }
            const long endTime = startTime + duration;
            const QString actual = describe(request(provider, startTime, endTime));
            const QString expected = expectedWindow(manyEvents, ids, startTime, endTime, samplingRate);
            if (actual != expected)
                QFAIL(qPrintable(QString("request %1: window %2-%3 ms\nactual:   %4\nexpected: %5")
                                     .arg(i).arg(startTime).arg(endTime).arg(actual, expected)));
            if (startTime > fileMaxTime)
                startTime = 0;
        }
    }

    void nextAndPreviousEvent()
    {
        // 1 kHz, so that times in samples equal times in ms. The event is placed at 25 % of the window.
        EventsProvider provider(path("few.xyz.evt"), 1000.0, 25);
        QCOMPARE(provider.loadData(), int(EventsProvider::OK));
        const int reward = provider.eventDescriptionIdMap().value(EventDescription("reward"));

        // The next reward after 250 ms is at 1500 ms: the window starts at 1250 ms.
        const Result next = requestNext(provider, 0, 1000, {reward});
        QCOMPARE(next.name, QString("xyz"));
        QCOMPARE(next.startingTime, 1250L);
        QCOMPARE(describe(next), QString("(250, %1)").arg(reward));

        // The previous reward is at 200 ms; the window cannot start before 0.
        const int lick = provider.eventDescriptionIdMap().value(EventDescription("lick"));
        const Result previous = requestPrevious(provider, next.startingTime, 1000, {reward});
        QCOMPARE(previous.startingTime, 0L);
        QCOMPARE(describe(previous), QString("(100, %1) (200, %2) (500, %1)").arg(lick).arg(reward));
    }

    void saveRoundTrip()
    {
        EventsProvider provider(path("many.abc.evt"), 20000.0);
        QCOMPARE(provider.loadData(), int(EventsProvider::OK));

        QFile file(path("saved.abc.evt"));
        QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
        QVERIFY(provider.save(&file));
        file.close();

        QFile original(path("many.abc.evt"));
        QVERIFY(original.open(QIODevice::ReadOnly));
        QVERIFY(file.open(QIODevice::ReadOnly));
        QCOMPARE(file.readAll(), original.readAll());
    }

    void addAndRemoveEvent()
    {
        EventsProvider provider(path("few.xyz.evt"), 1000.0);
        QCOMPARE(provider.loadData(), int(EventsProvider::OK));

        provider.addEvent("new", 700.0);
        QVERIFY(provider.isModified());
        QCOMPARE(provider.getNbEvents(), 6);
        // A new description gets an id in alphabetical order: lick, new, reward.
        QCOMPARE(provider.eventDescriptionIdMap().value(EventDescription("new")), 2);
        QCOMPARE(describe(request(provider, 600, 800)), QString("(100, 2)"));

        provider.undo();
        QCOMPARE(provider.getNbEvents(), 5);
        QCOMPARE(describe(request(provider, 600, 800)), QString());
    }
};

QTEST_GUILESS_MAIN(TestEventsProvider)
#include "test_eventsprovider.moc"
