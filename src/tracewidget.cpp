/***************************************************************************
                          tracewidget.cpp  -  description
                             -------------------
    begin                : Wed Mar 17 2004
    copyright            : (C) 2004 by Lynn Hazan
    email                : lynn.hazan.myrealbox.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
//include files for the application
#include "tracewidget.h"

// include files for QT
#include <QString>

#include <QFrame>
#include <QList>
#include <QLabel>
#include <QKeyEvent>
#include <QDebug>
#include <QVBoxLayout>


/// Added by M.Zugaro to enable automatic forward paging
#include <QTimer>

TraceWidget::TraceWidget(long startTime,long duration,bool greyScale,TracesProvider& tracesProvider,bool multiColumns,bool verticalLines,
                         bool raster,bool waveforms,bool labelsDisplay,QList<int>& channelsToDisplay,int gain,int acquisitionGain,
                         ChannelColors* channelColors,QMap<int, QList<int> >* groupsChannels,
                         QMap<int,int>* channelsGroups,bool autocenterChannels,QList<int>& channelOffsets,QList<int>& gains,const QList<int>& skippedChannels,
                         int rasterHeight,const QImage& backgroundImage,QWidget* parent,
                         const char* name,const QColor& backgroundColor,QStatusBar* statusBar,
                         int minSize,int maxSize,int windowTopLeft,int windowBottomRight,int border):
   QWidget(parent),timeWindow(duration),
    view(tracesProvider,greyScale,multiColumns,verticalLines,raster,waveforms,labelsDisplay,channelsToDisplay,gain,acquisitionGain,
         startTime,timeWindow,channelColors,groupsChannels,channelsGroups,autocenterChannels,channelOffsets,gains,skippedChannels,rasterHeight,backgroundImage,this,name,
         backgroundColor,statusBar,minSize,maxSize,windowTopLeft,windowBottomRight,border),
    startTime(startTime),
    validator(this),
    isInit(true),
    updateView(true),
    statusBar(statusBar)
{

    QVBoxLayout *lay = new QVBoxLayout;
    setLayout(lay);
    recordingLength = tracesProvider.recordingLength();

    selectionWidgets = new QWidget(this);
    lay->addWidget(&view);
    lay->addWidget(selectionWidgets);
    lay->setMargin(0);
    lay->setSpacing(0);
    lay->setStretchFactor(selectionWidgets,0);
    lay->setStretchFactor(&view,200);

    setFocusPolicy(Qt::NoFocus);

    initSelectionWidgets();
    adjustSize();

    connect(&view,SIGNAL(channelsSelected(QList<int>)),this, SLOT(slotChannelsSelected(QList<int>)));
    connect(&view,SIGNAL(setStartAndDuration(long,long)),this, SLOT(slotSetStartAndDuration(long,long)));
    connect(&view,SIGNAL(eventModified(QString,int,double,double)),this, SLOT(slotEventModified(QString,int,double,double)));
    connect(&view,SIGNAL(eventRemoved(QString,int,double)),this, SLOT(slotEventRemoved(QString,int,double)));
    connect(&view,SIGNAL(eventAdded(QString,QString,double)),this, SLOT(slotEventAdded(QString,QString,double)));
    connect(&view,SIGNAL(eventsAvailable(QHash<QString,EventData*>&,QMap<QString,QList<int> >&,QHash<QString,ItemColors*>&,QObject*,double)),this, SLOT(slotEventsAvailable(QHash<QString,EventData*>&,QMap<QString,QList<int> >&,QHash<QString,ItemColors*>&,QObject*,double)));

    isInit = false;
    /// Added by M.Zugaro to enable automatic forward paging
    timer = new QTimer(this);
    connect(timer,SIGNAL(timeout()),this,SLOT(advance()));
    pageTime = 500;
}

TraceWidget::~TraceWidget(){
}

/// Added by M.Zugaro to enable automatic forward paging
void TraceWidget::page()
{
    if ( timer->isActive() )
        timer->stop();
    else
	 {
        timer->start(pageTime);
		  statusBar->showMessage(tr("Auto-advance every %1 ms").arg(pageTime));
	 }
}

bool TraceWidget::isStill()
{
	return ! ( timer != NULL && timer->isActive() );
}

void TraceWidget::stop()
{
	if ( timer->isActive() )
	{
			timer->stop();
			emit stopped();
	}
}

void TraceWidget::accelerate()
{
    if ( !timer->isActive() )
        return;
    pageTime -= 125;
    if ( pageTime < 0 ) pageTime = 0;
    statusBar->showMessage(tr("Auto-advance every %1 ms").arg(pageTime));
    timer->start(pageTime);
}

void TraceWidget::decelerate()
{
    if ( !timer->isActive() ) return;
    pageTime += 125;
    if ( pageTime > 1000 ) pageTime = 1000;
    statusBar->showMessage(tr("Auto-advance every %1 ms").arg(pageTime));
    timer->start(pageTime);
}

/// Added by M.Zugaro to enable automatic forward paging
void TraceWidget::advance()
{
	 // Temporarily disconnect so that changes to scrollbar and time boxes do not automatically stop paging!
	 disconnect(startMinute,SIGNAL(valueChanged(int)),this, SLOT(stop()));
    disconnect(startSecond,SIGNAL(valueChanged(int)),this, SLOT(stop()));
    disconnect(startMilisecond,SIGNAL(valueChanged(int)),this, SLOT(stop()));
    disconnect(scrollBar,SIGNAL(valueChanged(int)),this, SLOT(stop()));
	 
    // Because data files are expected to have grown, update recording length,
    // as well as spin box and scroll bar in the view
    view.updateRecordingLength();
    recordingLength = view.recordingLength();
    minutePart = recordingLength / 60000;
    int remainingSeconds = static_cast<int>(fmod(static_cast<double>(recordingLength),60000));
    secondPart = remainingSeconds / 1000;
    milisecondPart = static_cast<int>(fmod(static_cast<double>(remainingSeconds),1000));
    startMinute->setMaximum(minutePart);
    scrollBar->setMaximum(recordingLength - timeWindow);

    // Move one page
    /*	startTime += timeWindow;
    if ( startTime + timeWindow > recordingLength ) correctStartTime();*/
    updateView = false; // do not redraw yet
    correctStartTime();
    updateView = true;
    //Inform the traceView
    view.displayTimeFrame(startTime,timeWindow);
	 
    //Inform listener of the modification
    emit updateStartAndDuration(startTime,timeWindow);
	 
	 // Reconnect
	 connect(startMinute,SIGNAL(valueChanged(int)),this, SLOT(stop()));
    connect(startSecond,SIGNAL(valueChanged(int)),this, SLOT(stop()));
    connect(startMilisecond,SIGNAL(valueChanged(int)),this, SLOT(stop()));
    connect(scrollBar,SIGNAL(valueChanged(int)),this, SLOT(stop()));
	 
    timer->start(pageTime); // restart timer
}

void TraceWidget::changeBackgroundColor(const QColor &color)
{
    view.changeBackgroundColor(color);
    update();
}

void TraceWidget::setGreyScale(bool grey)
{
    view.setGreyScale(grey);
}

void TraceWidget::initSelectionWidgets()
{
    QHBoxLayout *lay = new QHBoxLayout;
    selectionWidgets->setLayout(lay);
    QFont font("Helvetica",9);

    //Create and initialize the spin boxe and lineEdit.
    startLabel = new QLabel("Start time",selectionWidgets);
    startLabel->setFrameStyle(QFrame::StyledPanel|QFrame::Plain);
    startLabel->setFont(font);
    lay->addWidget(startLabel);

    minutePart = recordingLength / 60000;
    int remainingSeconds = static_cast<int>(fmod(static_cast<double>(recordingLength),60000));
    secondPart = remainingSeconds / 1000;
    milisecondPart = static_cast<int>(fmod(static_cast<double>(remainingSeconds),1000));

    int nbMinutes = startTime / 60000;
    remainingSeconds = static_cast<int>(fmod(static_cast<double>(startTime),60000));
    int nbSeconds = remainingSeconds / 1000;
    int remainingMiliseconds = static_cast<int>(fmod(static_cast<double>(remainingSeconds),1000));

    startMinute = new QSpinBox(selectionWidgets);
    startMinute->setMinimum(0);
    startMinute->setMaximum(minutePart);
    startMinute->setSingleStep(1);
    lay->addWidget(startMinute);
    startMinute->setSuffix( tr(" min") );
    startMinute->setWrapping(true);
    startMinute->setValue(nbMinutes);
    startSecond = new QSpinBox(selectionWidgets);
    startSecond->setMinimum(0);
    startSecond->setMaximum(recordingLength/1000);
    startSecond->setSingleStep(1);
    lay->addWidget(startSecond);
    startSecond->setSuffix( tr(" s") );
    startSecond->setValue(nbSeconds);
    startMilisecond = new QSpinBox(selectionWidgets);

    startMilisecond->setMinimum(0);
    startMilisecond->setMaximum(recordingLength);
    startMilisecond->setSingleStep(1);

    lay->addWidget(startMilisecond);
    startMilisecond->setSuffix( tr(" ms") );
    startMilisecond->setValue(remainingMiliseconds);


    durationLabel = new QLabel(tr("  Duration (ms)"),selectionWidgets);
    lay->addWidget(durationLabel);
    durationLabel->setFrameStyle(QFrame::StyledPanel|QFrame::Plain);
    durationLabel->setFont(font);
    duration = new QLineEdit(QString::number(timeWindow),selectionWidgets);
    lay->addWidget(duration);
    duration->setMinimumSize(50,duration->minimumHeight());
    duration->setMaximumSize(50,duration->maximumHeight());
    duration->setMaxLength(5);
    //duration will only accept integers between 0 and a max equal
    //to maximum of time for the current document (set when the document will be opened)
    duration->setValidator(&validator);

#if 0
    connect(startMinute,SIGNAL(valueChanged(int)),this, SLOT(slotStartMinuteTimeUpdated(int)));
    connect(startSecond,SIGNAL(valueChanged(int)),this, SLOT(slotStartSecondTimeUpdated(int)));
    connect(startMilisecond,SIGNAL(valueChanged(int)),this, SLOT(slotStartMilisecondTimeUpdated(int)));
#else
    connect(startMinute,SIGNAL(editingFinished()),this, SLOT(slotStartMinuteTimeUpdated()));
    connect(startSecond,SIGNAL(editingFinished()),this, SLOT(slotStartSecondTimeUpdated()));
    connect(startMilisecond,SIGNAL(editingFinished()),this, SLOT(slotStartMilisecondTimeUpdated()));

	 /// Added by M.Zugaro to enable automatic forward paging
	 connect(startMinute,SIGNAL(valueChanged(int)),this, SLOT(stop()));
    connect(startSecond,SIGNAL(valueChanged(int)),this, SLOT(stop()));
    connect(startMilisecond,SIGNAL(valueChanged(int)),this, SLOT(stop()));
#endif
    connect(duration,SIGNAL(returnPressed()),this, SLOT(slotDurationUpdated()));

    //Create and initialize the scrollbar. The line step is a 20iest of the page step
    pageStep = timeWindow;
    lineStep = static_cast<long>(floor(0.5 + static_cast<float>(static_cast<float>(timeWindow) / static_cast<float>(20))));

    scrollBar = new QScrollBar(selectionWidgets);
    scrollBar->setOrientation(Qt::Horizontal);
    scrollBar->setMinimum(0);
    scrollBar->setMaximum(recordingLength - timeWindow);
    scrollBar->setSingleStep(lineStep);
    scrollBar->setPageStep(pageStep);


    lay->addWidget(scrollBar);
    scrollBar->setValue(startTime);
    connect(scrollBar,SIGNAL(sliderReleased()),this, SLOT(slotScrollBarUpdated()));
    connect(scrollBar,SIGNAL(valueChanged(int)),this, SLOT(slotScrollBarUpdated()));
    connect(scrollBar,SIGNAL(valueChanged(int)),this, SLOT(stop()));

    //enable the user to use the keyboard to interact with the scrollbar.
    scrollBar->setMouseTracking(false);
    scrollBar->setFocusPolicy(Qt::StrongFocus);

    lay->setMargin(0);
    lay->setSpacing(0);
    lay->setStretchFactor(startLabel,0);
    lay->setStretchFactor(startMinute,0);
    lay->setStretchFactor(startSecond,0);
    lay->setStretchFactor(startMilisecond,0);
    lay->setStretchFactor(durationLabel,0);
    lay->setStretchFactor(duration,0);
    lay->setStretchFactor(scrollBar,200);
}

void TraceWidget::samplingRateModified(qlonglong length)
{
    recordingLength = length;

    view.samplingRateModified(length);

    //Reset the position
    slotSetStartAndDuration(0,50);
}

void TraceWidget::keyPressEvent(QKeyEvent* event)
{
    switch(event->key()){
    case Qt::Key_Plus:                               // double the duration
        timeWindow = timeWindow * 2;
        duration->setText(QString::number(timeWindow));
        slotDurationUpdated();
        break;
    case Qt::Key_Minus:                              // reduce the duration of an half
        timeWindow = timeWindow / 2;
        duration->setText(QString::number(timeWindow));
        slotDurationUpdated();
        break;
    }
}

void TraceWidget::slotDurationUpdated()
{
    if(!isInit && updateView){
        //Modify updateView to prevent the scrollBar to trigger a changeEvent while been updated.
        updateView = false;

        timeWindow = (duration->displayText()).toLong();

        //Test if the time window is bigger than the time of the recording, if so fix it to the time of the recording
        if(timeWindow > recordingLength){
            timeWindow = recordingLength;
            duration->setText(QString::number(timeWindow));
        }
        //Test if the time window is inferior to 1 ms, if so fix set it to the minimum 1.
        if(timeWindow < 1){
            timeWindow = 1;
            duration->setText("1");
        }


        //Test if we go over the time of the recording if so keep the time window and move back in time
        if((startTime + timeWindow) > recordingLength)
            correctStartTime();
        else{
            startMinute->setMaximum(minutePart);
            startSecond->setMaximum(recordingLength/1000);
            startMilisecond->setMaximum(recordingLength);
        }

        //beyond 10 ms the lineStep is fixe at 1 ms
        if(timeWindow < 10)
            lineStep = 1;
        else
            lineStep =  static_cast<long>(floor(0.5 + static_cast<float>(static_cast<float>(timeWindow) / static_cast<float>(20))));
        pageStep = timeWindow;

        scrollBar->setMaximum(recordingLength - timeWindow);
        scrollBar->setSingleStep(lineStep);
        scrollBar->setPageStep(pageStep);

        updateView = true;

        //Inform the traceView
        view.displayTimeFrame(startTime,timeWindow);
        //Inform the listeners of the modification
        emit updateStartAndDuration(startTime,timeWindow);
    }
}

void TraceWidget::correctStartTime()
{
    //update the selection widgets
    int extraMinutes = timeWindow / 60000;
    int remainingSeconds = static_cast<int>(fmod(static_cast<double>(timeWindow),60000));
    int extraSeconds = remainingSeconds / 1000;
    int extraMiliseconds = static_cast<int>(fmod(static_cast<double>(remainingSeconds),1000));

    int nbMinutes = minutePart - extraMinutes;
    int nbSeconds = secondPart - extraSeconds;
    int nbMiliseconds = milisecondPart - extraMiliseconds;

    if(nbMiliseconds < 0){
        int additionalSeconds = static_cast<int>(abs(nbMiliseconds) / 1000);
        startMilisecond->setMaximum(recordingLength);
        startMilisecond->setValue(1000 - extraMiliseconds + milisecondPart);
        if(additionalSeconds == 0) additionalSeconds = 1;
        nbSeconds -= additionalSeconds;
    }
    else{
        startMilisecond->setMaximum(recordingLength);
        startMilisecond->setValue(nbMiliseconds);
    }

    if(nbSeconds < 0){
        int additionalMinutes = static_cast<int>(abs(nbSeconds) / 60);
        if(additionalMinutes == 0) additionalMinutes = 1;
        nbMinutes -= additionalMinutes;
        if(nbMinutes <= 0){
            startSecond->setMaximum(0);
            startSecond->setValue(0);
        }
        else{
            startSecond->setMaximum(recordingLength/1000);
            startSecond->setValue(59 + nbSeconds + 1);
        }
    }
    else{
        startSecond->setMaximum(recordingLength/1000);
        startSecond->setValue(nbSeconds);
    }

    if (nbMinutes < 0) {
        startMinute->setMaximum(0);
        startMinute->setValue(0);
        startSecond->setMaximum(0);
        startSecond->setValue(0);
        startMilisecond->setMaximum(0);
        startMilisecond->setValue(0);
    } else {
        startMinute->setMaximum(nbMinutes);
        startMinute->setValue(nbMinutes);
    }

    startTime = startMinute->value()* 60000 + startSecond->value() * 1000 + startMilisecond->value();
    scrollBar->setMaximum(recordingLength - timeWindow);
    scrollBar->setValue(startTime);
}

void TraceWidget::slotStartMinuteTimeUpdated(/*int start*/){
    if(!isInit && updateView){
        int start = startMinute->value();
        //Modify updateView to prevent the scrollBar and other spinboxes to trigger a changeEvent while been updated.
        updateView = false;

        long modifiedStartTime = start * 60000 + startSecond->value() * 1000 + startMilisecond->value();

        //Test if we go over the time of the recording if so keep the time window and move back in time
        if((modifiedStartTime + timeWindow) > recordingLength)  {
           correctStartTime();
        }else{
            startTime = modifiedStartTime;
/*
            startMinute->setMaximum(minutePart);
            startSecond->setMaximum(recordingLength/1000);
            startMilisecond->setMaximum(recordingLength);
*/
            scrollBar->blockSignals(true);
            scrollBar->setValue(startTime);
            scrollBar->blockSignals(false);

        }

        updateView = true;

        //Inform the traceView
        view.displayTimeFrame(startTime,timeWindow);
        //Inform listern of the modification
        emit updateStartAndDuration(startTime,timeWindow);
    }
}

void TraceWidget::slotStartSecondTimeUpdated(){
    if(!isInit && updateView){
        int start = startSecond->value();
        //Modify updateView to prevent the scrollBar and other spinboxes to trigger a changeEvent while been updated.
        updateView = false;

        long modifiedStartTime = startMinute->value() * 60000 + start * 1000 + startMilisecond->value();

        //Test if we go over the time of the recording if so keep the time window and move back in time
        if((modifiedStartTime + timeWindow) > recordingLength) correctStartTime();
        else if(start > 59){
            int remainingSeconds = static_cast<int>(fmod(static_cast<double>(start),60));
            startSecond->setValue(remainingSeconds);
            int additionalMinutes = static_cast<int>(abs(start) / 60);
            if(additionalMinutes == 0) additionalMinutes = 1;
            int nbMinutes = startMinute->value() + additionalMinutes;

            if(nbMinutes > minutePart) correctStartTime();
            else{
                startMinute->setValue(nbMinutes);
                startTime = startMinute->value()* 60000 + startSecond->value() * 1000 + startMilisecond->value();

                scrollBar->blockSignals(true);
                scrollBar->setMaximum(recordingLength - timeWindow);
                scrollBar->setValue(startTime);
                scrollBar->blockSignals(false);
            }
        }
        else{
            startTime = modifiedStartTime;
            //startMinute->setMaximum(minutePart);
            //startSecond->setMaximum(recordingLength/1000);
            //startMilisecond->setMaximum(recordingLength);
            scrollBar->blockSignals(true);
            scrollBar->setValue(startTime);
            scrollBar->blockSignals(false);
        }

        updateView = true;

        //Inform the traceView
        view.displayTimeFrame(startTime,timeWindow);
        //Inform listern of the modification
        emit updateStartAndDuration(startTime,timeWindow);
    }
}

void TraceWidget::slotStartMilisecondTimeUpdated(){
    if(!isInit && updateView){
        int start = startMilisecond->value();
        //Modify updateView to prevent the scrollBar to trigger a changeEvent while been updated.
        updateView = false;

        long modifiedStartTime = startMinute->value() * 60000 + startSecond->value() * 1000 + start;
        //Test if we go over the time of the recording if so keep the time window and move back in time
        if((modifiedStartTime + timeWindow) > recordingLength) correctStartTime();
        else if(start > 999){
            int remainingMiliseconds = static_cast<int>(fmod(static_cast<double>(start),1000));
            startMilisecond->setValue(remainingMiliseconds);
            int additionalSeconds = static_cast<int>(abs(start) / 1000);
            if(additionalSeconds == 0) additionalSeconds = 1;
            int nbSeconds = startSecond->value() + additionalSeconds;

            if(nbSeconds > 59){
                int remainingSeconds = static_cast<int>(fmod(static_cast<double>(nbSeconds),60));
                startSecond->setValue(remainingSeconds);
                int additionalMinutes = static_cast<int>(abs(nbSeconds) / 60);
                if(additionalMinutes == 0) additionalMinutes = 1;
                int nbMinutes = startMinute->value() + additionalMinutes;

                if(nbMinutes > minutePart) correctStartTime();
                else{
                    startMinute->setValue(nbMinutes);
                    startTime = startMinute->value()* 60000 + startSecond->value() * 1000 + startMilisecond->value();
                    scrollBar->blockSignals(true);
                    scrollBar->setMaximum(recordingLength - timeWindow);
                    scrollBar->setValue(startTime);
                    scrollBar->blockSignals(false);
                }
            }
            else{
                startSecond->setValue(nbSeconds);
                startTime = startMinute->value()* 60000 + startSecond->value() * 1000 + startMilisecond->value();
                scrollBar->blockSignals(true);
                scrollBar->setMaximum(recordingLength - timeWindow);
                scrollBar->setValue(startTime);
                scrollBar->blockSignals(false);
            }
        }
        else{
            startTime = modifiedStartTime;
            //startMinute->setMaximum(minutePart);
            //startSecond->setMaximum(recordingLength/1000);
            //startMilisecond->setMaximum(recordingLength);
            scrollBar->blockSignals(true);
            scrollBar->setValue(startTime);
            scrollBar->blockSignals(false);
        }
        updateView = true;

        //Inform the traceView
        view.displayTimeFrame(startTime,timeWindow);
        //Inform listener of the modification
        emit updateStartAndDuration(startTime,timeWindow);
    }
}

void TraceWidget::slotScrollBarUpdated(){
    if(!isInit && updateView){
        //Modify updateView to prevent the spinboxes to trigger a changeEvent while been updated.
        updateView = false;

        long modifiedStartTime = scrollBar->value();//in miliseconds

        if(modifiedStartTime == startTime){
            updateView = true;
            return;
        }

        //Test if we go over the time of the recording if so keep the time window and move back in time
        if((modifiedStartTime + timeWindow) > recordingLength) correctStartTime();
        else{
            startTime = modifiedStartTime;
            int nbMinutes = startTime / 60000;
            int remainingSeconds = static_cast<int>(fmod(static_cast<double>(startTime),60000));
            int nbSeconds = remainingSeconds / 1000;
            int remainingMiliseconds = static_cast<int>(fmod(static_cast<double>(remainingSeconds),1000));


            startMinute->blockSignals(true);
            startSecond->blockSignals(true);
            startMilisecond->blockSignals(true);
            startMinute->setValue(nbMinutes);
            startSecond->setValue(nbSeconds);
            startMilisecond->setValue(remainingMiliseconds);

            startMinute->blockSignals(false);
            startSecond->blockSignals(false);
            startMilisecond->blockSignals(false);


            //startMinute->setMaximum(minutePart);
            //startSecond->setMaximum(recordingLength/1000);
            //startMilisecond->setMaximum(recordingLength);

        }

        updateView = true;

        //Inform the traceView
        view.displayTimeFrame(startTime,timeWindow);
        //Inform listener of the modification
        emit updateStartAndDuration(startTime,timeWindow);
    }
}

void TraceWidget::moveToTime(long time){
    if(!isInit && updateView){
        //Test if we go over the time of the recording
        if(time > recordingLength)
            return;

        //Modify updateView to prevent the spinboxes to trigger a changeEvent while been updated.
        updateView = false;

        //Test if we go over the time of the recording if so keep the time window and move back in time
        if((time + timeWindow) > recordingLength) correctStartTime();
        else{
            scrollBar->setValue(time);
            startTime = time;
            int nbMinutes = startTime / 60000;
            int remainingSeconds = static_cast<int>(fmod(static_cast<double>(startTime),60000));
            int nbSeconds = remainingSeconds / 1000;
            int remainingMiliseconds = static_cast<int>(fmod(static_cast<double>(remainingSeconds),1000));

            startMinute->blockSignals(true);
            startSecond->blockSignals(true);
            startMilisecond->blockSignals(true);

            startMinute->setValue(nbMinutes);
            startSecond->setValue(nbSeconds);
            startMilisecond->setValue(remainingMiliseconds);
            startMinute->blockSignals(false);
            startSecond->blockSignals(false);
            startMilisecond->blockSignals(false);
        }
        updateView = true;

        //Inform the traceView
        view.displayTimeFrame(startTime,timeWindow);
        //Inform listern of the modification
        emit updateStartAndDuration(startTime,timeWindow);
    }
}

void TraceWidget::slotSetStartAndDuration(long time,long duration){
    if(!isInit && updateView){
        //Test if we go over the time of the recoTraceWidget::rding
        if(time > recordingLength) return;

        //Modify updateView to prevent the spinboxes to trigger a changeEvent while been updated.
        updateView = false;

        //First set the duration then the start time

        //Duration
        //Test if the time window is inferior to 1 ms, if so fix set it to the minimum 1.
        if(duration < 1) duration = 1;
        this->duration->setText(QString::number(duration));
        timeWindow = duration;

        //Test if we go over the time of the recording if so keep the time window and move back in time
        if((startTime + timeWindow) > recordingLength) correctStartTime();
        else{
            startMinute->setMaximum(minutePart);
            startSecond->setMaximum(recordingLength/1000);
            startMilisecond->setMaximum(recordingLength);
        }

        //beyond 10 ms the lineStep is fixe at 1 ms
        if(timeWindow < 10)
            lineStep = 1;
        else
            lineStep =  static_cast<long>(floor(0.5 + static_cast<float>(static_cast<float>(timeWindow) / static_cast<float>(20))));
        pageStep = timeWindow;

        scrollBar->setMaximum(recordingLength - timeWindow);
        scrollBar->setSingleStep(lineStep);
        scrollBar->setPageStep(pageStep);

        //Start time
        //Test if we go over the time of the recording if so keep the time window and move back in time
        if((time + timeWindow) > recordingLength)
            correctStartTime();
        else{
            scrollBar->setValue(time);
            startTime = time;
            int nbMinutes = startTime / 60000;
            int remainingSeconds = static_cast<int>(fmod(static_cast<double>(startTime),60000));
            int nbSeconds = remainingSeconds / 1000;
            int remainingMiliseconds = static_cast<int>(fmod(static_cast<double>(remainingSeconds),1000));

            startMinute->setValue(nbMinutes);
            startSecond->setValue(nbSeconds);
            startMilisecond->setValue(remainingMiliseconds);
        }
        updateView = true;

        //Inform the traceView
        view.displayTimeFrame(startTime,timeWindow);
        //Inform listern of the modification
        emit updateStartAndDuration(startTime,timeWindow);
    }
}

void TraceWidget::selectChannels(const QList<int>& selectedIds)
{
    view.selectChannels(selectedIds);
}

void TraceWidget::setMode(BaseFrame::Mode selectedMode,bool active)
{
    view.setMode(selectedMode,active);
}

void TraceWidget::setAutocenterChannels(bool status)
{
    view.setAutocenterChannels(status);
}

void TraceWidget::showLabels(bool show)
{
    view.showHideLabels(show);
}

void TraceWidget::slotChannelsSelected(const QList<int>& selectedIds)
{
    emit channelsSelected(selectedIds);
}

void TraceWidget::slotEventAdded(const QString &providerName,const QString& addedEventDescription,double time){
    emit eventAdded(providerName,addedEventDescription,time);
}

void TraceWidget::updateEvents(const QString& providerName,QList<int>& eventsToShow,bool active){
    view.updateEvents(providerName,eventsToShow,active);
}

void TraceWidget::updateEvents(bool active,const QString& providerName,double time){
    long eventTime = static_cast<long>(floor(0.5 + time));
    if((eventTime >= startTime  && eventTime <= (startTime + timeWindow)))
        view.updateEvents(providerName,active);
}
