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
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
//include files for the application
#include "tracewidget.h"

// include files for QT
#include <QString>
//Added by qt3to4:
#include <Q3Frame>
#include <Q3ValueList>
#include <QLabel>
#include <QKeyEvent>


//General C++ include files
#include <iostream>
#include <stdlib.h>
using namespace std;

/// Added by M.Zugaro to enable automatic forward paging
#include <QTimer>

TraceWidget::TraceWidget(long startTime,long duration,bool greyScale,TracesProvider& tracesProvider,bool multiColumns,bool verticalLines,
                  bool raster,bool waveforms,bool labelsDisplay,Q3ValueList<int>& channelsToDisplay,int gain,int acquisitionGain,
                  ChannelColors* channelColors,QMap<int, Q3ValueList<int> >* groupsChannels,
                  QMap<int,int>* channelsGroups,Q3ValueList<int>& channelOffsets,Q3ValueList<int>& gains,const Q3ValueList<int>& skippedChannels,int rasterHeight,QImage backgroundImage,QWidget* parent,
                   const char* name,QColor backgroundColor,KStatusBar* statusBar,
                  int minSize,int maxSize,int windowTopLeft,int windowBottomRight,int border):
                  Q3VBox(parent,name),timeWindow(duration),
                  view(tracesProvider,greyScale,multiColumns,verticalLines,raster,waveforms,labelsDisplay,channelsToDisplay,gain,acquisitionGain,
                  startTime,timeWindow,channelColors,groupsChannels,channelsGroups,channelOffsets,gains,skippedChannels,rasterHeight,backgroundImage,this,name,
                  backgroundColor,statusBar,minSize,maxSize,windowTopLeft,windowBottomRight,border),startTime(startTime),
                  validator(this),isInit(true),updateView(true){

  recordingLength = tracesProvider.recordingLength();

  selectionWidgets = new Q3HBox(this);
  setMargin(0);
  setSpacing(0);
  setStretchFactor(selectionWidgets,0);
  setStretchFactor(&view,200);

  setFocusPolicy(Qt::StrongFocus);

  initSelectionWidgets();
  adjustSize();

  connect(&view,SIGNAL(channelsSelected(const Q3ValueList<int>&)),this, SLOT(slotChannelsSelected(const Q3ValueList<int>&)));
  connect(&view,SIGNAL(setStartAndDuration(long,long)),this, SLOT(slotSetStartAndDuration(long,long)));
  connect(&view,SIGNAL(eventModified(QString,int,double,double)),this, SLOT(slotEventModified(QString,int,double,double)));
  connect(&view,SIGNAL(eventRemoved(QString,int,double)),this, SLOT(slotEventRemoved(QString,int,double)));
  connect(&view,SIGNAL(eventAdded(QString,QString,double)),this, SLOT(slotEventAdded(QString,QString,double)));
  connect(&view,SIGNAL(eventsAvailable(Q3Dict<EventData>&,QMap<QString, Q3ValueList<int> >&,Q3Dict<ItemColors>&,QObject*,double)),this, SLOT(slotEventsAvailable(Q3Dict<EventData>&,QMap<QString, Q3ValueList<int> >&,Q3Dict<ItemColors>&,QObject*,double)));

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
	if ( timer->isActive() ) timer->stop();
	else timer->start(pageTime);
}

void TraceWidget::accelerate()
{
	if ( !timer->isActive() ) return;
	pageTime -= 125;
	if ( pageTime < 0 ) pageTime = 0;
	cout << "page time: " << pageTime << endl;
	timer->start(pageTime);
}

void TraceWidget::decelerate()
{
	if ( !timer->isActive() ) return;
	pageTime += 125;
	if ( pageTime > 1000 ) pageTime = 1000;
	cout << "page time: " << pageTime << endl;
	timer->start(pageTime);
}

/// Added by M.Zugaro to enable automatic forward paging
void TraceWidget::advance()
{
	// Because data files are expected to have grown, update recording length,
	// as well as spin box and scroll bar in the view
	view.updateRecordingLength();
	recordingLength = view.recordingLength();
	minutePart = recordingLength / 60000;
	int remainingSeconds = static_cast<int>(fmod(static_cast<double>(recordingLength),60000));
	secondPart = remainingSeconds / 1000;
	milisecondPart = static_cast<int>(fmod(static_cast<double>(remainingSeconds),1000));
	startMinute->setMaxValue(minutePart);
   scrollBar->setMaxValue(recordingLength - timeWindow);

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
	timer->start(pageTime); // restart timer
}

void TraceWidget::changeBackgroundColor(QColor color){
 view.changeBackgroundColor(color);
 update();
}

void TraceWidget::setGreyScale(bool grey){
  view.setGreyScale(grey);
}


void TraceWidget::initSelectionWidgets(){
  QFont font("Helvetica",9);

  //Create and initialize the spin boxe and lineEdit.
  startLabel = new QLabel("Start time",selectionWidgets);
  startLabel->setFrameStyle(QFrame::StyledPanel|Q3Frame::Plain);
  startLabel->setFont(font);

  minutePart = recordingLength / 60000;
  int remainingSeconds = static_cast<int>(fmod(static_cast<double>(recordingLength),60000));
  secondPart = remainingSeconds / 1000;
  milisecondPart = static_cast<int>(fmod(static_cast<double>(remainingSeconds),1000));

  int nbMinutes = startTime / 60000;
  remainingSeconds = static_cast<int>(fmod(static_cast<double>(startTime),60000));
  int nbSeconds = remainingSeconds / 1000;
  int remainingMiliseconds = static_cast<int>(fmod(static_cast<double>(remainingSeconds),1000));

  startMinute = new QSpinBox(0,minutePart,1,selectionWidgets,"minStart");
  startMinute->setSuffix( " min" );
  startMinute->setWrapping(true);
  startMinute->setValue(nbMinutes);
  startSecond = new QSpinBox(0,recordingLength/1000,1,selectionWidgets,"sStart");
  startSecond->setSuffix( " s" );
  startSecond->setValue(nbSeconds);
  startMilisecond = new QSpinBox(0,recordingLength,1,selectionWidgets,"msStart");
  startMilisecond->setSuffix( " ms" );
  startMilisecond->setValue(remainingMiliseconds);


  durationLabel = new QLabel("  Duration (ms)",selectionWidgets);
  durationLabel->setFrameStyle(QFrame::StyledPanel|Q3Frame::Plain);
  durationLabel->setFont(font);
  duration = new QLineEdit(QString("%1").arg(timeWindow),selectionWidgets);
  duration->setMinimumSize(50,duration->minimumHeight());
  duration->setMaximumSize(50,duration->maximumHeight());
  duration->setMaxLength(5);
  //duration will only accept integers between 0 and a max equal
  //to maximum of time for the current document (set when the document will be opened)
  duration->setValidator(&validator);

  connect(startMinute,SIGNAL(valueChanged(int)),this, SLOT(slotStartMinuteTimeUpdated(int)));
  connect(startSecond,SIGNAL(valueChanged(int)),this, SLOT(slotStartSecondTimeUpdated(int)));
  connect(startMilisecond,SIGNAL(valueChanged(int)),this, SLOT(slotStartMilisecondTimeUpdated(int)));
  connect(duration,SIGNAL(returnPressed()),this, SLOT(slotDurationUpdated()));

  //Create and initialize the scrollbar. The line step is a 20iest of the page step
  pageStep = timeWindow;
  lineStep = static_cast<long>(floor(0.5 + static_cast<float>(static_cast<float>(timeWindow) / static_cast<float>(20))));

  scrollBar = new QScrollBar(0,recordingLength - timeWindow,lineStep,pageStep,pageStep,Qt::Horizontal,selectionWidgets);
  scrollBar->setValue(startTime);
  connect(scrollBar,SIGNAL(sliderReleased()),this, SLOT(slotScrollBarUpdated()));
  connect(scrollBar,SIGNAL(nextLine()),this, SLOT(slotScrollBarUpdated()));
  connect(scrollBar,SIGNAL(nextPage()),this, SLOT(slotScrollBarUpdated()));
  connect(scrollBar,SIGNAL(prevLine()),this, SLOT(slotScrollBarUpdated()));
  connect(scrollBar,SIGNAL(prevPage()),this, SLOT(slotScrollBarUpdated()));

  //enable the user to use the keyboard to interact with the scrollbar.
  scrollBar->setMouseTracking(false);
  scrollBar->setFocusPolicy(Qt::StrongFocus);
  connect(scrollBar,SIGNAL(valueChanged(int)),this, SLOT(slotScrollBarUpdated()));

  selectionWidgets->setMargin(0);
  selectionWidgets->setSpacing(0);
  selectionWidgets->setStretchFactor(startLabel,0);
  selectionWidgets->setStretchFactor(startMinute,0);
  selectionWidgets->setStretchFactor(startSecond,0);
  selectionWidgets->setStretchFactor(startMilisecond,0);
  selectionWidgets->setStretchFactor(durationLabel,0);
  selectionWidgets->setStretchFactor(duration,0);
  selectionWidgets->setStretchFactor(scrollBar,200);
}

void TraceWidget::samplingRateModified(long long length){
  recordingLength = length;

  view.samplingRateModified(length);

  //Reset the position
  slotSetStartAndDuration(0,50);
}


void TraceWidget::keyPressEvent(QKeyEvent* event){
 switch(event->key()){
  case Qt::Key_Plus:                               // double the duration
    timeWindow = timeWindow * 2;
    duration->setText(QString("%1").arg(timeWindow));
    slotDurationUpdated();
    break;
  case Qt::Key_Minus:                              // reduce the duration of an half
    timeWindow = timeWindow / 2;
    duration->setText(QString("%1").arg(timeWindow));
    slotDurationUpdated();
    break;
 }
}

void TraceWidget::slotDurationUpdated(){
 if(!isInit && updateView){
  //Modify updateView to prevent the scrollBar to trigger a changeEvent while been updated.
  updateView = false;

  timeWindow = (duration->displayText()).toLong();

  //Test if the time window is bigger than the time of the recording, if so fix it to the time of the recording
  if(timeWindow > recordingLength){
   timeWindow = recordingLength;
   duration->setText(QString("%1").arg(timeWindow));
  }
  //Test if the time window is inferior to 1 ms, if so fix set it to the minimum 1.
  if(timeWindow < 1){
   timeWindow = 1;
   duration->setText("1");
  }


  //Test if we go over the time of the recording if so keep the time window and move back in time
  if((startTime + timeWindow) > recordingLength) correctStartTime();
  else{
   startMinute->setMaxValue(minutePart);
   startSecond->setMaxValue(recordingLength/1000);
   startMilisecond->setMaxValue(recordingLength);
  }

  //beyond 10 ms the lineStep is fixe at 1 ms
  if(timeWindow < 10) lineStep = 1;
  else lineStep =  static_cast<long>(floor(0.5 + static_cast<float>(static_cast<float>(timeWindow) / static_cast<float>(20))));
  pageStep = timeWindow;

  scrollBar->setMaxValue(recordingLength - timeWindow);
  scrollBar->setLineStep(lineStep);
  scrollBar->setPageStep(pageStep);

  updateView = true;

  //Inform the traceView
  view.displayTimeFrame(startTime,timeWindow);
  //Inform the listeners of the modification
  emit updateStartAndDuration(startTime,timeWindow);
 }
}

void TraceWidget::correctStartTime(){
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
    startMilisecond->setMaxValue(recordingLength);
    startMilisecond->setValue(1000 - extraMiliseconds + milisecondPart);
    if(additionalSeconds == 0) additionalSeconds = 1;
    nbSeconds -= additionalSeconds;
   }
   else{
    startMilisecond->setMaxValue(recordingLength);
    startMilisecond->setValue(nbMiliseconds);
   }

   if(nbSeconds < 0){
    int additionalMinutes = static_cast<int>(abs(nbSeconds) / 60);
    if(additionalMinutes == 0) additionalMinutes = 1;
    nbMinutes -= additionalMinutes;
    if(nbMinutes <= 0){
     startSecond->setMaxValue(0);
     startSecond->setValue(0);
    }
    else{
     startSecond->setMaxValue(recordingLength/1000);
     startSecond->setValue(59 + nbSeconds + 1);
    }
   }
   else{
    startSecond->setMaxValue(recordingLength/1000);
    startSecond->setValue(nbSeconds);
   }

   if(nbMinutes < 0){
    startMinute->setMaxValue(0);
    startMinute->setValue(0);
    startSecond->setMaxValue(0);
    startSecond->setValue(0);
    startMilisecond->setMaxValue(0);
    startMilisecond->setValue(0);
   }
   else{
    startMinute->setMaxValue(nbMinutes);
    startMinute->setValue(nbMinutes);
   }

   startTime = startMinute->value()* 60000 + startSecond->value() * 1000 + startMilisecond->value();
   scrollBar->setMaxValue(recordingLength - timeWindow);
   scrollBar->setValue(startTime);
}

void TraceWidget::slotStartMinuteTimeUpdated(int start){
 if(!isInit && updateView){
  //Modify updateView to prevent the scrollBar and other spinboxes to trigger a changeEvent while been updated.
  updateView = false;

  long modifiedStartTime = start * 60000 + startSecond->value() * 1000 + startMilisecond->value();

 //Test if we go over the time of the recording if so keep the time window and move back in time
  if((modifiedStartTime + timeWindow) > recordingLength) correctStartTime();
  else{
   startTime = modifiedStartTime;
   startMinute->setMaxValue(minutePart);
   startSecond->setMaxValue(recordingLength/1000);
   startMilisecond->setMaxValue(recordingLength);
   scrollBar->setValue(startTime);
  }

  updateView = true;

  //Inform the traceView
  view.displayTimeFrame(startTime,timeWindow);
  //Inform listern of the modification
  emit updateStartAndDuration(startTime,timeWindow);
 }
}

void TraceWidget::slotStartSecondTimeUpdated(int start){
 if(!isInit && updateView){
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

    scrollBar->setMaxValue(recordingLength - timeWindow);
    scrollBar->setValue(startTime);
   }
  }
  else{
   startTime = modifiedStartTime;
   startMinute->setMaxValue(minutePart);
   startSecond->setMaxValue(recordingLength/1000);
   startMilisecond->setMaxValue(recordingLength);
   scrollBar->setValue(startTime);
  }

  updateView = true;

  //Inform the traceView
  view.displayTimeFrame(startTime,timeWindow);
  //Inform listern of the modification
  emit updateStartAndDuration(startTime,timeWindow);
 }
}

void TraceWidget::slotStartMilisecondTimeUpdated(int start){
 if(!isInit && updateView){
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
     scrollBar->setMaxValue(recordingLength - timeWindow);
     scrollBar->setValue(startTime);
    }
   }
   else{
    startSecond->setValue(nbSeconds);
    startTime = startMinute->value()* 60000 + startSecond->value() * 1000 + startMilisecond->value();
    scrollBar->setMaxValue(recordingLength - timeWindow);
    scrollBar->setValue(startTime);
   }
  }
  else{
   startTime = modifiedStartTime;
   startMinute->setMaxValue(minutePart);
   startSecond->setMaxValue(recordingLength/1000);
   startMilisecond->setMaxValue(recordingLength);
   scrollBar->setValue(startTime);
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

   startMinute->setValue(nbMinutes);
   startSecond->setValue(nbSeconds);
   startMilisecond->setValue(remainingMiliseconds);


   startMinute->setMaxValue(minutePart);
   startSecond->setMaxValue(recordingLength/1000);
   startMilisecond->setMaxValue(recordingLength);

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
  if(time > recordingLength) return;

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
  this->duration->setText(QString("%1").arg(duration));
  timeWindow = duration;

  //Test if we go over the time of the recording if so keep the time window and move back in time
  if((startTime + timeWindow) > recordingLength) correctStartTime();
  else{
   startMinute->setMaxValue(minutePart);
   startSecond->setMaxValue(recordingLength/1000);
   startMilisecond->setMaxValue(recordingLength);
  }

  //beyond 10 ms the lineStep is fixe at 1 ms
  if(timeWindow < 10) lineStep = 1;
  else lineStep =  static_cast<long>(floor(0.5 + static_cast<float>(static_cast<float>(timeWindow) / static_cast<float>(20))));
  pageStep = timeWindow;

  scrollBar->setMaxValue(recordingLength - timeWindow);
  scrollBar->setLineStep(lineStep);
  scrollBar->setPageStep(pageStep);

  //Start time
  //Test if we go over the time of the recording if so keep the time window and move back in time
  if((time + timeWindow) > recordingLength) correctStartTime();
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



#include "tracewidget.moc"
