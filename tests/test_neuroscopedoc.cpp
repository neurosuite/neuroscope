/***************************************************************************
                          test_neuroscopedoc.cpp  -  description
                             -------------------
    purpose              : Tests for the document of an opened recording
    copyright            : (C) 2026 smenp
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "apptestutils.h"
#include "tracewidget.h"

#include <QAction>
#include <QLineEdit>
#include <QSpinBox>
#include <QTemporaryDir>
#include <QtTest>

using namespace testutils;

namespace
{

const int NB_CHANNELS = 4;
const double SAMPLING_RATE = 20000;
const std::int64_t NB_SAMPLES = 20000;

} // namespace

class TestNeuroscopeDoc : public QObject
{
    Q_OBJECT

  private:
    QTemporaryDir dir;

  private Q_SLOTS:
    void initTestCase()
    {
        isolateSettings();
        QVERIFY(dir.isValid());
    }

    void changeTheChannelCount_data()
    {
        QTest::addColumn<int>("nbChannels");
        QTest::newRow("more") << NB_CHANNELS + 3;
        QTest::newRow("fewer") << NB_CHANNELS - 1;
    }

    // The channel palettes label every channel of the new count.
    void changeTheChannelCount()
    {
        QFETCH(int, nbChannels);
        const QString datPath = writeRecording(QDir(dir.path()), QString("count-") + QTest::currentDataTag(), NB_CHANNELS, SAMPLING_RATE, NB_SAMPLES);
        auto app = testutils::openRecording(datPath);
        NeuroscopeDoc* doc = app->getDocument();

        doc->setChannelNb(nbChannels);

        QCOMPARE(doc->getChannelNb(), nbChannels);
        QCOMPARE(doc->getChannelLabels()->size(), nbChannels);
    }

    void closeWithTheFocusInTheStartTime_data()
    {
        QTest::addColumn<QString>("closing");
        QTest::newRow("close the window") << "window";
        QTest::newRow("quit") << "quit";
        QTest::newRow("close the only display") << "display";
    }

    // Closing takes the keyboard focus from the start time box, which then applies the digits typed in it.
    void closeWithTheFocusInTheStartTime()
    {
        QFETCH(QString, closing);
        const QString datPath = writeRecording(QDir(dir.path()), QString("close-") + QTest::currentDataTag(), 1, 1250, 200 * 1250);
        auto app = testutils::openRecording(datPath);
        app->activateWindow();
        QVERIFY(QTest::qWaitForWindowActive(app.get()));
        QSpinBox* seconds = nullptr;
        for (QSpinBox* box: app->findChild<TraceWidget*>()->findChildren<QSpinBox*>())
        {
            if (box->suffix().trimmed() == "s")
            {
                seconds = box;
            }
        }
        QVERIFY(seconds);
        QLineEdit* edit = seconds->findChild<QLineEdit*>();
        QTest::mouseClick(edit, Qt::LeftButton);
        QVERIFY(seconds->hasFocus());
        QTest::keyClick(edit, Qt::Key_A, Qt::ControlModifier);
        QTest::keyClicks(edit, "5");

        if (closing == "window")
        {
            QVERIFY(app->close());
        }
        else if (closing == "quit")
        {
            // File > Quit, through its action: a platform may bind no key to Quit.
            QAction* quit = nullptr;
            for (QAction* action: app->findChildren<QAction*>())
            {
                if (action->text() == "Quit")
                {
                    quit = action;
                }
            }
            QVERIFY(quit);
            quit->trigger();
        }
        else
        {
            QTest::keySequence(edit, QKeySequence(Qt::CTRL | Qt::Key_W));
        }

        QVERIFY(!app->getDocument()->isADocumentToClose());
    }
};

QTEST_MAIN(TestNeuroscopeDoc)
#include "test_neuroscopedoc.moc"
