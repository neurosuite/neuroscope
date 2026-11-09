/***************************************************************************
                          test_events.cpp  -  description
                             -------------------
    purpose              : Tests for the events in the trace and position
                           views
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
#include "itempalette.h"
#include "neuroscopeview.h"
#include "traceview.h"

#include <QAction>
#include <QMouseEvent>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QtTest>

using namespace testutils;

namespace
{

const int NB_CHANNELS = 4;
const double SAMPLING_RATE = 20000;
const std::int64_t NB_SAMPLES = 20000;
const QString DESCRIPTION = "stim";

/** Shows the event palette and calls slotCloseEventFile(), as Close Event File does for the file selected in it. */
void closeSelectedEventFile(NeuroscopeApp* app)
{
    QWidget* palette = app->findChild<QWidget*>("eventPanel");
    QVERIFY(palette);
    // The palettes are the pages of the stacked widget of the palette tabs.
    qobject_cast<QTabWidget*>(palette->parentWidget()->parentWidget())->setCurrentWidget(palette);
    QMetaObject::invokeMethod(app, "slotCloseEventFile");
}

void clickInTheTraces(NeuroscopeApp* app)
{
    TraceView* view = app->activeView()->findChild<TraceView*>();
    QVERIFY(view);
    QTest::mouseClick(view, Qt::LeftButton, Qt::NoModifier, view->rect().center());
}

/** Selects the first event of the event file "abc", which is shown, with the Select Event tool. */
void selectAnEvent(NeuroscopeApp* app)
{
    // The events of a file are numbered from 1 in the order of their descriptions.
    app->activeView()->shownEventsUpdate("abc", { 1 });
    QMetaObject::invokeMethod(app, "slotSelectEvent");
    clickInTheTraces(app);
}

} // namespace

class TestEvents : public QObject
{
    Q_OBJECT

  private:
    QTemporaryDir dir;

    /** Opens a recording with the event file @p baseName.abc.evt loaded and, with @p otherFile, @p baseName.def.evt. */
    std::unique_ptr<NeuroscopeApp> openWithEvents(const QString& baseName, bool otherFile)
    {
        const QString datPath = writeRecording(QDir(dir.path()), baseName, NB_CHANNELS, SAMPLING_RATE, NB_SAMPLES);
        auto app = openRecording(datPath);
        writeTextFile(dir.filePath(baseName + ".abc.evt"), "5\t" + DESCRIPTION + "\n500\t" + DESCRIPTION + "\n");
        loadFile(app.get(), "slotLoadEventFiles", dir.filePath(baseName + ".abc.evt"));
        if (otherFile)
        {
            writeTextFile(dir.filePath(baseName + ".def.evt"), "900\t" + DESCRIPTION + "\n");
            loadFile(app.get(), "slotLoadEventFiles", dir.filePath(baseName + ".def.evt"));
        }
        return app;
    }

  private Q_SLOTS:
    void initTestCase()
    {
        isolateSettings();
        QVERIFY(dir.isValid());
    }

    void closeTheFileEventsAreAddedTo_data()
    {
        QTest::addColumn<bool>("inactiveView");
        QTest::newRow("only file") << false;
        QTest::newRow("added to in an inactive view") << true;
    }

    // Closing the event file that events are added to leaves the add-event mode, as the Select tool does.
    void closeTheFileEventsAreAddedTo()
    {
        QFETCH(bool, inactiveView);
        auto app = openWithEvents(QString("adding-%1").arg(QTest::currentDataTag()).replace(' ', '-'), false);
        app->findChild<ItemPalette*>("eventPanel")->selectGroup("abc");
        NeuroscopeView* adding = app->activeView();
        const QString added = "added";
        QAction choice(added);
        QMetaObject::invokeMethod(app.get(), "slotAddEventButtonActivated", Q_ARG(QAction*, &choice));
        // The displays are the pages of the stacked widget of the display tabs.
        QTabWidget* displayTabs = qobject_cast<QTabWidget*>(adding->parentWidget()->parentWidget());
        if (inactiveView)
        {
            QMetaObject::invokeMethod(app.get(), "slotNewDisplay");
            displayTabs->setCurrentIndex(1);
            QVERIFY(app->activeView() != adding);
        }

        closeSelectedEventFile(app.get());

        displayTabs->setCurrentWidget(adding);
        clickInTheTraces(app.get());
        QVERIFY(adding->isSelectionTool());
    }

    void editASelectedEventAfterItsFileIsClosed_data()
    {
        QTest::addColumn<bool>("drag");
        QTest::newRow("remove") << false;
        QTest::newRow("drag") << true;
    }

    // An event stays selected while its file is closed; removing or dragging it must not reach the closed file.
    void editASelectedEventAfterItsFileIsClosed()
    {
        QFETCH(bool, drag);
        auto app = openWithEvents(QString("selected-") + QTest::currentDataTag(), true);
        selectAnEvent(app.get());

        QMetaObject::invokeMethod(app.get(), "slotEventGroupSelected", Q_ARG(QString, "abc"));
        closeSelectedEventFile(app.get());

        if (drag)
        {
            TraceView* view = app->findChild<TraceView*>();
            const QPoint from = view->rect().center();
            const QPoint to = from + QPoint(10, 0);
            QTest::mousePress(view, Qt::LeftButton, Qt::NoModifier, from);
            QMouseEvent move(QEvent::MouseMove, to, view->mapToGlobal(to), Qt::NoButton, Qt::LeftButton, Qt::NoModifier);
            QApplication::sendEvent(view, &move);
            QTest::mouseRelease(view, Qt::LeftButton, Qt::NoModifier, to);
        }
        else
        {
            QMetaObject::invokeMethod(app.get(), "removeEvent");
        }
    }

    void undoAnEditAfterAFileIsClosed_data()
    {
        QTest::addColumn<bool>("undoBeforeClosing");
        QTest::newRow("undo") << false;
        QTest::newRow("redo") << true;
    }

    // Undo and Redo revert the last edit of an event file; closing that file leaves nothing to revert.
    void undoAnEditAfterAFileIsClosed()
    {
        QFETCH(bool, undoBeforeClosing);
        auto app = openWithEvents(QString("undo-") + QTest::currentDataTag(), true);
        QAction* undo = nullptr;
        QAction* redo = nullptr;
        for (QAction* action: app->findChildren<QAction*>())
        {
            if (action->text() == "Undo")
                undo = action;
            if (action->text() == "Redo")
                redo = action;
        }
        QVERIFY(undo && redo);
        selectAnEvent(app.get());
        QMetaObject::invokeMethod(app.get(), "removeEvent");
        QVERIFY(undo->isEnabled());
        if (undoBeforeClosing)
        {
            undo->trigger();
            QVERIFY(redo->isEnabled());
        }

        QMetaObject::invokeMethod(app.get(), "slotEventGroupSelected", Q_ARG(QString, "abc"));
        closeSelectedEventFile(app.get());

        undo->trigger();
        redo->trigger();
        QVERIFY(!undo->isEnabled());
        QVERIFY(!redo->isEnabled());
    }
};

QTEST_MAIN(TestEvents)
#include "test_events.moc"
