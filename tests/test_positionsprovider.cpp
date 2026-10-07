/***************************************************************************
                          test_positionsprovider.cpp  -  description
                             -------------------
    purpose              : Tests for reading position files
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

#include "positionsprovider.h"
#include "testutils.h"

#include <QTemporaryDir>
#include <QtTest>

using namespace testutils;

namespace
{

const double SAMPLING_RATE = 50.0; // one position every 20 ms
const int WIDTH = 368;
const int HEIGHT = 240;
const int NB_POSITIONS = 100;

long x(long line) { return 2 * line; }
long y(long line) { return 100 + line; }

Matrix request(PositionsProvider& provider, long startTime, long endTime)
{
    Matrix result;
    int emitted = 0;
    auto connection = QObject::connect(&provider, &PositionsProvider::dataReady,
                                       [&](Array<dataType>& data, QObject*)
                                       {
                                           result = toMatrix(data);
                                           ++emitted;
                                       });
    provider.requestData(startTime, endTime, nullptr);
    QObject::disconnect(connection);
    if (emitted != 1)
        qFatal("dataReady emitted %d times", emitted);
    return result;
}

} // namespace

class TestPositionsProvider : public QObject
{
    Q_OBJECT

  private:
    QTemporaryDir dir;

    QString path(const QString& name) const { return dir.filePath(name); }

  private Q_SLOTS:
    void initTestCase()
    {
        QVERIFY(dir.isValid());
        QString content;
        for (long line = 0; line < NB_POSITIONS; ++line)
            content += QString("%1 %2\n").arg(x(line)).arg(y(line));
        writeTextFile(path("session.pos"), content);
    }

    void load()
    {
        PositionsProvider provider(path("session.pos"), SAMPLING_RATE, WIDTH, HEIGHT, 0, 0);
        QCOMPARE(provider.loadData(), int(PositionsProvider::OK));
        QCOMPARE(provider.getNbSpots(), 1);
        QCOMPARE(provider.getSamplingRate(), SAMPLING_RATE);
    }

    void valuesAreRounded()
    {
        writeTextFile(path("decimals.pos"), "0 0\n1.4 2.5\n-3.6 1e2\n");
        PositionsProvider provider(path("decimals.pos"), SAMPLING_RATE, WIDTH, HEIGHT, 0, 0);
        QCOMPARE(provider.loadData(), int(PositionsProvider::OK));
        const Matrix data = request(provider, 40, 60);
        QCOMPARE(data.rows, 2L);
        QCOMPARE(data.values, QVector<dataType>({1, 3, -4, 100}));
    }

    void firstPosition()
    {
        PositionsProvider provider(path("session.pos"), SAMPLING_RATE, WIDTH, HEIGHT, 0, 0);
        QCOMPARE(provider.loadData(), int(PositionsProvider::OK));
        const Matrix data = request(provider, 0, 10);
        QCOMPARE(data.rows, 1L);
        QCOMPARE(data(1, 1), x(0));
        QEXPECT_FAIL("", "The first line is stored in the first column instead of the first row: its y is not set", Abort);
        QCOMPARE(data(1, 2), y(0));
    }

    void inconsistentLinesAreIncorrect()
    {
        writeTextFile(path("inconsistent.pos"), "1 2\n3 4 5\n6 7\n");
        PositionsProvider provider(path("inconsistent.pos"), SAMPLING_RATE, WIDTH, HEIGHT, 0, 0);
        QCOMPARE(provider.loadData(), int(PositionsProvider::INCORRECT_CONTENT));
    }

    void readWindow_data()
    {
        QTest::addColumn<long>("startTime");
        QTest::addColumn<long>("endTime");
        QTest::addColumn<long>("firstLine");
        QTest::addColumn<long>("nbLines");

        // Current behaviour: the start is rounded up and the end rounded to the nearest position, and
        // position n (counting from 1) is used for the time n / samplingRate: the first two positions
        // in the file are both shown at 0 and 20 ms.
        QTest::newRow("start of file") << 0L << 100L << 0L << 5L;
        QTest::newRow("start at second position") << 20L << 100L << 0L << 5L;
        QTest::newRow("start at third position") << 40L << 100L << 1L << 4L;
        QTest::newRow("start rounded up") << 41L << 100L << 2L << 3L;
        QTest::newRow("end past end of file") << 1900L << 3000L << 94L << 6L;
    }

    void readWindow()
    {
        QFETCH(long, startTime);
        QFETCH(long, endTime);
        QFETCH(long, firstLine);
        QFETCH(long, nbLines);

        PositionsProvider provider(path("session.pos"), SAMPLING_RATE, WIDTH, HEIGHT, 0, 0);
        QCOMPARE(provider.loadData(), int(PositionsProvider::OK));
        const Matrix data = request(provider, startTime, endTime);
        QCOMPARE(data.rows, nbLines);
        QCOMPARE(data.cols, 2L);
        for (long i = 0; i < nbLines; ++i)
        {
            QCOMPARE(data(i + 1, 1), x(firstLine + i));
            // The y of the first position is not set, see firstPosition().
            if (firstLine + i > 0)
                QCOMPARE(data(i + 1, 2), y(firstLine + i));
        }
    }

    void startPastEndOfFileIsEmpty()
    {
        PositionsProvider provider(path("session.pos"), SAMPLING_RATE, WIDTH, HEIGHT, 0, 0);
        QCOMPARE(provider.loadData(), int(PositionsProvider::OK));
        QVERIFY(request(provider, 3000, 4000).isEmpty());
    }

    void flip_data()
    {
        QTest::addColumn<int>("flip");
        QTest::addColumn<long>("expectedX");
        QTest::addColumn<long>("expectedY");

        // Position 10: x = 18, y = 109.
        QTest::newRow("none") << 0 << 18L << 109L;
        QTest::newRow("vertical") << 1 << 18L << long(HEIGHT - 109);
        QTest::newRow("horizontal") << 2 << long(WIDTH - 18) << 109L;
    }

    void flip()
    {
        QFETCH(int, flip);
        QFETCH(long, expectedX);
        QFETCH(long, expectedY);

        PositionsProvider provider(path("session.pos"), SAMPLING_RATE, WIDTH, HEIGHT, 0, flip);
        QCOMPARE(provider.loadData(), int(PositionsProvider::OK));
        const Matrix data = request(provider, 200, 200);
        QCOMPARE(data.rows, 1L);
        QCOMPARE(data(1, 1), expectedX);
        QCOMPARE(data(1, 2), expectedY);
    }

    void retrieveAllData()
    {
        PositionsProvider provider(path("session.pos"), SAMPLING_RATE, WIDTH, HEIGHT, 0, 1);
        QCOMPARE(provider.loadData(), int(PositionsProvider::OK));

        Matrix result;
        connect(&provider, &PositionsProvider::dataReady, [&](Array<dataType>& data, QObject*) { result = toMatrix(data); });
        provider.retrieveAllData(nullptr);

        // All positions, without the flip.
        QCOMPARE(result.rows, long(NB_POSITIONS));
        QCOMPARE(result(2, 2), y(1));
        QCOMPARE(result(NB_POSITIONS, 2), y(NB_POSITIONS - 1));
    }
};

QTEST_GUILESS_MAIN(TestPositionsProvider)
#include "test_positionsprovider.moc"
