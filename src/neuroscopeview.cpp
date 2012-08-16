/***************************************************************************
                          neuroscopeview.cpp  -  description
                             -------------------
    begin                : Wed Feb 25 19:05:25 EST 2004
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

// include files for Qt
#include <qpainter.h>
#include <qpaintdevicemetrics.h> 

// include files for KDE
#include <kmessagebox.h>
#include <kstatusbar.h>

#include <kglobal.h>

// application specific includes
#include "neuroscopeview.h"
#include "neuroscopedoc.h"
#include "neuroscope.h"
#include "tracewidget.h"


class EventData;

NeuroscopeView::NeuroscopeView(NeuroscopeApp& mainWindow,QString label,long startTime,long duration,QColor backgroundColor,int wflags,KStatusBar* statusBar,QValueList<int>* channelsToDisplay,
                bool greyScale,TracesProvider& tracesProvider,bool multiColumns,bool verticalLines,
                bool raster,bool waveforms,bool labelsDisplay,int unitGain,int acquisitionGain,ChannelColors* channelColors,
                QMap<int,QValueList<int> >* groupsChannels,QMap<int,int>* channelsGroups,
                QValueList<int> offsets,QValueList<int> channelGains,QValueList<int> selected,QMap<int,bool> skipStatus,int rasterHeight,QString backgroundImagePath,QWidget* parent, const char* name):
                KDockArea(parent, name),shownChannels(channelsToDisplay),mainWindow(mainWindow),greyScaleMode(greyScale),
                multiColumns(multiColumns),verticalLines(verticalLines),raster(raster),waveforms(waveforms),selectMode(false),
                channelOffsets(),gains(),selectedChannels(),tabLabel(label),startTime(startTime),timeWindow(duration),
                labelsDisplay(labelsDisplay),isPositionFileShown(false),positionView(0L),eventsInPositionView(false){

  //Will automatically delete the list of selected clusters while deleting the view.
  selectedClusters.setAutoDelete(false);
                 
  //Will automatically delete the list of selected events while deleting the view.
  selectedEvents.setAutoDelete(false);

 //Duplicate the offset,gain and channelSelected lists
  QValueList<int>::iterator offsetIterator;
  for(offsetIterator = offsets.begin(); offsetIterator != offsets.end(); ++offsetIterator)
   channelOffsets.append(*offsetIterator);

  QValueList<int>::iterator gainIterator;
  for(gainIterator = channelGains.begin(); gainIterator != channelGains.end(); ++gainIterator)
   gains.append(*gainIterator);

  QValueList<int>::iterator selectedIterator;
  for(selectedIterator = selected.begin(); selectedIterator != selected.end(); ++selectedIterator)
   selectedChannels.append(*selectedIterator);

      
  QValueList<int> skippedChannels;
  QMap<int,bool>::Iterator iterator;
  for(iterator = skipStatus.begin(); iterator != skipStatus.end(); ++iterator) if(iterator.data()) skippedChannels.append(iterator.key());
   
  //Create the mainDock
  mainDock = createDockWidget("Main", QPixmap(), 0L, tr("field potentials"), tr("field potentials"));

  traceWidget = new TraceWidget(startTime,duration,greyScale,tracesProvider,multiColumns,verticalLines,raster,
                                  waveforms,labelsDisplay,*shownChannels,unitGain,acquisitionGain,channelColors,groupsChannels,channelsGroups,
                                  channelOffsets,gains,skippedChannels,rasterHeight,QImage(backgroundImagePath),mainDock,"traces",backgroundColor,statusBar,5);  
  
  mainDock->setWidget(traceWidget);
  mainDock->setEnableDocking(KDockWidget::DockCorner);
  mainDock->setDockSite(KDockWidget::DockCorner);
  setMainDockWidget(mainDock);

  //Set Connection(s) common to all widgets.
  connect(this,SIGNAL(updateContents()),traceWidget,SLOT(updateContents()));
  connect(this,SIGNAL(changeBackgroundColor(QColor)),traceWidget, SLOT(changeBackgroundColor(QColor)));
  connect(this,SIGNAL(greyScale(bool)),traceWidget, SLOT(setGreyScale(bool)));
  connect(traceWidget,SIGNAL(channelsSelected(const QValueList<int>&)),this, SLOT(slotChannelsSelected(const QValueList<int>&)));
  connect(this,SIGNAL(modeToSet(BaseFrame::Mode,bool)),traceWidget,SLOT(setMode(BaseFrame::Mode,bool)));
  connect(this,SIGNAL(multiColumnsDisplay(bool)),traceWidget,SLOT(setMultiColumns(bool)));
  connect(this,SIGNAL(clusterVerticalLinesDisplay(bool)),traceWidget,SLOT(setClusterVerticalLines(bool)));
  connect(this,SIGNAL(clusterRasterDisplay(bool)),traceWidget,SLOT(setClusterRaster(bool)));
  connect(this,SIGNAL(clusterWaveformsDisplay(bool)),traceWidget,SLOT(setClusterWaveforms(bool)));
  connect(this,SIGNAL(showChannels(const QValueList<int>&)),traceWidget,SLOT(showChannels(const QValueList<int>&)));
  connect(this,SIGNAL(channelColorUpdate(int,bool)),traceWidget,SLOT(channelColorUpdate(int,bool)));
  connect(this,SIGNAL(groupColorUpdate(int,bool)),traceWidget,SLOT(groupColorUpdate(int,bool)));
  connect(this,SIGNAL(increaseAllAmplitude()),traceWidget,SLOT(increaseAllChannelsAmplitude()));
  connect(this,SIGNAL(decreaseAllAmplitude()),traceWidget,SLOT(decreaseAllChannelsAmplitude()));
  connect(this,SIGNAL(increaseAmplitude(const QValueList<int>&)),traceWidget,SLOT(increaseSelectedChannelsAmplitude(const QValueList<int>&)));
  connect(this,SIGNAL(decreaseAmplitude(const QValueList<int>&)),traceWidget,SLOT(decreaseSelectedChannelsAmplitude(const QValueList<int>&)));
  connect(this,SIGNAL(updateGains(int,int)),traceWidget,SLOT(setGains(int,int)));
  connect(this,SIGNAL(updateDrawing()),traceWidget, SLOT(updateDrawing()));
  connect(this,SIGNAL(groupsHaveBeenModified(bool)),traceWidget, SLOT(groupsModified(bool)));
  connect(this,SIGNAL(channelsToBeSelected(const QValueList<int>&)),traceWidget,SLOT(selectChannels(const QValueList<int>&)));
  connect(this,SIGNAL(resetChannelOffsets(const QMap<int,int>&)),traceWidget,SLOT(resetOffsets(const QMap<int,int>&)));
  connect(this,SIGNAL(resetChannelGains(const QValueList<int>&)),traceWidget,SLOT(resetGains(const QValueList<int>&))); 
  connect(this,SIGNAL(drawTraces()),traceWidget,SLOT(drawTraces()));
  connect(this,SIGNAL(reset()),traceWidget,SLOT(reset()));
  connect(traceWidget,SIGNAL(updateStartAndDuration(long,long)),this, SLOT(setStartAndDuration(long,long)));
  connect(this,SIGNAL(showLabels(bool)),traceWidget, SLOT(showLabels(bool)));
  connect(this,SIGNAL(displayCalibration(bool,bool)),traceWidget, SLOT(showCalibration(bool,bool)));
  connect(this,SIGNAL(newSamplingRate(long long)),traceWidget,SLOT(samplingRateModified(long long)));
  connect(this,SIGNAL(newClusterProvider(ClustersProvider*,QString,ItemColors*,bool,QValueList<int>&,QMap<int, QValueList<int> >*,QMap<int,int>*,int,int,const QValueList<int>&)),traceWidget,
          SLOT(addClusterProvider(ClustersProvider*,QString,ItemColors*,bool,QValueList<int>&,QMap<int, QValueList<int> >*,QMap<int,int>*,int,int,const QValueList<int>&)));
  connect(this,SIGNAL(clusterProviderRemoved(QString,bool)),traceWidget,SLOT(removeClusterProvider(QString,bool)));
  connect(this,SIGNAL(showClusters(QString,QValueList<int>&)),traceWidget,SLOT(showClusters(QString,QValueList<int>&)));
  connect(this,SIGNAL(clusterColorUpdated(QString,int,bool)),traceWidget,SLOT(clusterColorUpdate(QString,int,bool)));
  connect(this,SIGNAL(print(QPainter&,QPaintDeviceMetrics&,QString,bool)),traceWidget,SLOT(print(QPainter&,QPaintDeviceMetrics&,QString,bool)));
  connect(this,SIGNAL(newEventProvider(EventsProvider*,QString,ItemColors*,bool,QValueList<int>&,const QValueList<int>&)),traceWidget,
          SLOT(addEventProvider(EventsProvider*,QString,ItemColors*,bool,QValueList<int>&,const QValueList<int>&)));
  connect(this,SIGNAL(eventProviderRemoved(QString,bool,bool)),traceWidget,SLOT(removeEventProvider(QString,bool)));
  connect(this,SIGNAL(showEvents(QString,QValueList<int>&)),traceWidget,SLOT(showEvents(QString,QValueList<int>&)));
  connect(this,SIGNAL(eventColorUpdated(QString,int,bool)),traceWidget,SLOT(eventColorUpdate(QString,int,bool)));
  connect(this,SIGNAL(nextEvent()),traceWidget,SLOT(showNextEvent()));
  connect(this,SIGNAL(previousEvent()),traceWidget,SLOT(showPreviousEvent()));
  connect(traceWidget,SIGNAL(eventModified(QString,int,double,double)),this, SLOT(slotEventModified(QString,int,double,double)));
  connect(this,SIGNAL(updateEvents(bool,QString,double,double)),traceWidget,SLOT(updateEvents(bool,QString,double,double)));
  connect(this,SIGNAL(eventToRemove()),traceWidget,SLOT(removeEvent()));
  connect(traceWidget,SIGNAL(eventRemoved(QString,int,double)),this, SLOT(slotEventRemoved(QString,int,double)));
  connect(this,SIGNAL(updateEvents(bool,QString,double)),traceWidget,SLOT(updateEvents(bool,QString,double)));
  connect(this,SIGNAL(newEventProperties(QString,QString)),traceWidget,SLOT(eventToAddProperties(QString,QString)));
  connect(traceWidget,SIGNAL(eventAdded(QString,QString,double)),this, SLOT(slotEventAdded(QString,QString,double)));
  connect(this,SIGNAL(updateEvents(QString,QValueList<int>&,bool)),traceWidget,SLOT(updateEvents(QString,QValueList<int>&,bool)));
  connect(this,SIGNAL(nextCluster()),traceWidget,SLOT(showNextCluster()));
  connect(this,SIGNAL(previousCluster()),traceWidget,SLOT(showPreviousCluster()));
  connect(this,SIGNAL(waveformInformationUpdated(int,int,bool)),traceWidget,SLOT(updateWaveformInformation(int,int,bool)));
  connect(this,SIGNAL(clusterProviderUpdated(bool)),traceWidget,SLOT(updateClusterData(bool)));
  connect(this,SIGNAL(noneBrowsingClusterListUpdated(QString,const QValueList<int>&)),traceWidget,SLOT(updateNoneBrowsingClusterList(QString,const QValueList<int>&)));
  connect(this,SIGNAL(noneBrowsingEventListUpdated(QString,const QValueList<int>&)),traceWidget,SLOT(updateNoneBrowsingEventList(QString,const QValueList<int>&)));   
  connect(this,SIGNAL(skipStatusChanged(const QValueList<int>&)),traceWidget,SLOT(updateSkipStatus(const QValueList<int>&)));
  connect(this,SIGNAL(decreaseTheRasterHeight()),traceWidget,SLOT(decreaseRasterHeight()));
  connect(this,SIGNAL(increaseTheRasterHeight()),traceWidget,SLOT(increaseRasterHeight()));
  connect(this,SIGNAL(traceBackgroundImageUpdate(QImage,bool)),traceWidget,SLOT(traceBackgroundImageUpdate(QImage,bool)));  
    
  connect(&globalEventProvider,SIGNAL(getCurrentEventInformation(long,long,QObject*)),traceWidget,SLOT(getCurrentEventInformation(long,long,QObject*)));

  
  
  mainDock->setEnableDocking(KDockWidget::DockNone);  
}

NeuroscopeView::~NeuroscopeView()
{
 delete shownChannels;
}


void NeuroscopeView::print(KPrinter* printer,QString filePath,bool whiteBackground)
{
  QPainter printPainter;
  QPaintDeviceMetrics metrics(printer);
  printPainter.begin(printer);
  
  //For the moment there is no list of contained views, therefore the signal is the only way to trigger the print of 
  //the traceWidget
  //Print the TraceView
  emit print(printPainter,metrics,filePath,whiteBackground);

  //Print the positionView.
  if(isPositionFileShown){
   printer->newPage();  
   NeuroscopeDoc* doc = mainWindow.getDocument();   
   positionView->print(printPainter,metrics,whiteBackground,doc->getWhiteTrajectoryBackground());     
  }

  printPainter.end();
} 

void NeuroscopeView::setChannelNb(int nb){
 shownChannels->clear();
 selectedChannels.clear();

 emit reset();   
}

void NeuroscopeView::shownChannelsUpdate(const QValueList<int>& channelsToShow){
 shownChannels->clear();
 selectedChannels.clear();

 //update the list of shown channels and the list of selected channels
 QValueList<int>::const_iterator shownChannelsIterator;
 for(shownChannelsIterator = channelsToShow.begin(); shownChannelsIterator != channelsToShow.end(); ++shownChannelsIterator){
  shownChannels->append(*shownChannelsIterator);
  selectedChannels.append(*shownChannelsIterator);
 }
 
 emit showChannels(*shownChannels);
 
 //Show all the enclosed widgets of the dockWindows.
 showAllWidgets(); 
}


void NeuroscopeView::setMultiColumns(bool multiple){
 multiColumns = multiple;
 emit multiColumnsDisplay(multiple);
}

void NeuroscopeView::setClusterVerticalLines(bool lines){
 verticalLines = lines;  
 emit clusterVerticalLinesDisplay(lines);
}

void NeuroscopeView::setClusterRaster(bool raster){
 this->raster = raster;
 emit clusterRasterDisplay(raster);
}
void NeuroscopeView::setClusterWaveforms(bool waveforms){
 this->waveforms = waveforms;
 emit clusterWaveformsDisplay(waveforms);
}

void NeuroscopeView::setClusterProvider(ClustersProvider* clustersProvider,QString name,ItemColors* clusterColors,bool active,
                                        QValueList<int>& clustersToShow,QMap<int,QValueList<int> >* displayGroupsClusterFile,
                                        QMap<int,int>* channelsSpikeGroups,int nbSamplesBefore,int nbSamplesAfter,const QValueList<int>& clustersToSkip){
  QValueList<int>* currentSelectedClusters = new QValueList<int>();
  QValueList<int>::iterator shownClustersIterator;
  for(shownClustersIterator = clustersToShow.begin(); shownClustersIterator != clustersToShow.end(); ++shownClustersIterator)
   currentSelectedClusters->append(*shownClustersIterator);    
   
  selectedClusters.insert(name,currentSelectedClusters);

  
  QValueList<int>* currentSkippedClusters = new QValueList<int>();
  QValueList<int>::const_iterator skippedClustersIterator;
  for(skippedClustersIterator = clustersToSkip.begin(); skippedClustersIterator != clustersToSkip.end(); ++skippedClustersIterator)
   currentSkippedClusters->append(*skippedClustersIterator);
   
  clustersNotUsedForBrowsing.insert(name,currentSkippedClusters);
  
  emit newClusterProvider(clustersProvider,name,clusterColors,active,clustersToShow,displayGroupsClusterFile,
                          channelsSpikeGroups,nbSamplesBefore,nbSamplesAfter,clustersToSkip);
}

void NeuroscopeView::removeClusterProvider(QString name,bool active){
  selectedClusters.remove(name);
  clustersNotUsedForBrowsing.remove(name);
  emit clusterProviderRemoved(name,active);
}

void NeuroscopeView::shownClustersUpdate(QString name,QValueList<int>& clustersToShow){
 QValueList<int>* currentSelectedClusters = selectedClusters[name];
 currentSelectedClusters->clear();
 
 //update the list of shown clusters
 QValueList<int>::iterator iterator;
 for(iterator = clustersToShow.begin(); iterator != clustersToShow.end(); ++iterator){
  currentSelectedClusters->append(*iterator);
 }

 emit showClusters(name,*currentSelectedClusters);

 //Show all the enclosed widgets of the dockWindows.
 showAllWidgets(); 
}

void NeuroscopeView::updateNoneBrowsingClusterList(QString providerName,const QValueList<int>& clustersToNotBrowse){
 QValueList<int>* currentSkippedClusters = clustersNotUsedForBrowsing[providerName];
 currentSkippedClusters->clear();

 //update the list of skipped events
 QValueList<int>::const_iterator skippedClustersIterator;
 for(skippedClustersIterator = clustersToNotBrowse.begin(); skippedClustersIterator != clustersToNotBrowse.end(); ++skippedClustersIterator){
  currentSkippedClusters->append(*skippedClustersIterator);
 }

 emit noneBrowsingClusterListUpdated(providerName,clustersToNotBrowse);
}


void NeuroscopeView::setEventProvider(EventsProvider* eventsProvider,QString name,ItemColors* eventColors,bool active,
                                      QValueList<int>& eventsToShow,const QValueList<int>& eventsToSkip){
  QValueList<int>* currentSelectedEvents = new QValueList<int>();
  QValueList<int>::iterator shownEventsIterator;
  for(shownEventsIterator = eventsToShow.begin(); shownEventsIterator != eventsToShow.end(); ++shownEventsIterator)
   currentSelectedEvents->append(*shownEventsIterator);

  selectedEvents.insert(name,currentSelectedEvents);


  QValueList<int>* currentSkippedEvents = new QValueList<int>();
  QValueList<int>::const_iterator skippedEventsIterator;
  for(skippedEventsIterator = eventsToSkip.begin(); skippedEventsIterator != eventsToSkip.end(); ++skippedEventsIterator)
   currentSkippedEvents->append(*skippedEventsIterator);

  eventsNotUsedForBrowsing.insert(name,currentSkippedEvents);

  //Warn the TraceWidget(s)
  emit newEventProvider(eventsProvider,name,eventColors,active,eventsToShow,eventsToSkip);
}

void NeuroscopeView::removeEventProvider(QString name,bool active,bool lastFile){
  selectedEvents.remove(name);
  eventsNotUsedForBrowsing.remove(name);
  if(lastFile) eventsInPositionView = false;

  //Warn the TraceWidget(s) and positionView
  emit eventProviderRemoved(name,active,lastFile);
}

void NeuroscopeView::shownEventsUpdate(QString name,QValueList<int>& eventsToShow){
 QValueList<int>* currentSelectedEvents = selectedEvents[name];
 currentSelectedEvents->clear();

 //update the list of shown clusters
 QValueList<int>::iterator iterator;
 for(iterator = eventsToShow.begin(); iterator != eventsToShow.end(); ++iterator){
  currentSelectedEvents->append(*iterator);
 }

 emit showEvents(name,*currentSelectedEvents);
 emit updateEventDisplay();

 //Show all the enclosed widgets of the dockWindows.
 showAllWidgets();
}


void NeuroscopeView::updateNoneBrowsingEventList(QString providerName,const QValueList<int>& eventsToNotBrowse){
 QValueList<int>* currentSkippedEvents = eventsNotUsedForBrowsing[providerName];
 currentSkippedEvents->clear();
 
 //update the list of skipped events
 QValueList<int>::const_iterator skippedEventsIterator;
 for(skippedEventsIterator = eventsToNotBrowse.begin(); skippedEventsIterator != eventsToNotBrowse.end(); ++skippedEventsIterator)
  currentSkippedEvents->append(*skippedEventsIterator);
  
 emit noneBrowsingEventListUpdated(providerName,eventsToNotBrowse);
}

void NeuroscopeView::updateEvents(QString providerName,int selectedEventId,float time,float newTime,bool active){
 emit updateEvents(active,providerName,time,newTime);
 emit updateEventDisplay();
}

void NeuroscopeView::updateEventsAfterRemoval(QString providerName,int eventId,float time,bool active){
 emit updateEvents(active,providerName,time);
 emit updateEventDisplay();
}

void NeuroscopeView::updateEventsAfterAddition(QString providerName,int eventId,float time,bool active){
 QValueList<int>* currentSelectedEvents = selectedEvents[providerName];
 
 if(active && !currentSelectedEvents->contains(eventId)){
   currentSelectedEvents->append(eventId);
   emit updateEvents(providerName,*currentSelectedEvents,active);
 }
 else emit updateEvents(active,providerName,time);   

 emit updateEventDisplay();
}

void NeuroscopeView::updateSelectedEventsIds(QString providerName,QMap<int,int>& oldNewEventIds,int modifiedEventId,bool active,bool added){

  if(eventsNotUsedForBrowsing.find(providerName) != 0){ 
   QValueList<int>* currentSkippedEvents = eventsNotUsedForBrowsing.take(providerName);
   QValueList<int>* newSkippedEventsIds = new QValueList<int>();
   QValueList<int>::iterator iterator;

   //An event description has been added
   if(added){
    for(iterator = currentSkippedEvents->begin(); iterator != currentSkippedEvents->end(); ++iterator)
     newSkippedEventsIds->append(oldNewEventIds[*iterator]);

    //The events are skipped by default 
    newSkippedEventsIds->append(modifiedEventId);
   }
   //an event description has been removed
   else{
    for(iterator = currentSkippedEvents->begin(); iterator != currentSkippedEvents->end(); ++iterator)
     if(oldNewEventIds.contains(*iterator)) newSkippedEventsIds->append(oldNewEventIds[*iterator]);
   }
   
   eventsNotUsedForBrowsing.insert(providerName,newSkippedEventsIds);
   delete currentSkippedEvents;

   emit noneBrowsingEventListUpdated(providerName,*newSkippedEventsIds);   
  }
  
  if(selectedEvents.find(providerName) != 0){ 
  QValueList<int>* currentSelectedEvents = selectedEvents.take(providerName);
  QValueList<int>* newSelectedEventsIds = new QValueList<int>();
  QValueList<int>::iterator iterator;
  
  //An event description has been added
  if(added){
   for(iterator = currentSelectedEvents->begin(); iterator != currentSelectedEvents->end(); ++iterator)
    newSelectedEventsIds->append(oldNewEventIds[*iterator]);
   
   //Add the new type of event to the active view in order to display the added event right away.
   if(active)newSelectedEventsIds->append(modifiedEventId);
  }
  //an event description has been removed
  else{
   for(iterator = currentSelectedEvents->begin(); iterator != currentSelectedEvents->end(); ++iterator){
    if(oldNewEventIds.contains(*iterator)){
     newSelectedEventsIds->append(oldNewEventIds[*iterator]);
    }
   }    
  }

  selectedEvents.insert(providerName,newSelectedEventsIds);
  delete currentSelectedEvents;
  
  //If at least one of the selected events has had his id modified, warn the traceView.
  emit updateEvents(providerName,*newSelectedEventsIds,active);
  emit updateEventDisplay();
 }
}

void NeuroscopeView::removePositionProvider(QString name,bool active){ 
 if(positionView != 0L) removePositionView();

 //Show all the enclosed widgets of the dockWindows.
 if(active) showAllWidgets();
}

void NeuroscopeView::addPositionView(PositionsProvider* positionsProvider,QImage backgroundImage,QColor backgroundColor,long startTime,long duration,int width,int height,bool showEvents){
 isPositionFileShown = true;
 eventsInPositionView = showEvents;
   
 //Create and add the position view
 KDockWidget* positions = createDockWidget( "Positions", QPixmap());
 positionView = new PositionView(*positionsProvider,globalEventProvider,backgroundImage,startTime,duration,showEvents,height,width,positions,"PositionView",backgroundColor); 
 positions->setWidget(positionView);//assign the widget
 
 positions->manualDock(mainDock,KDockWidget::DockTop,25);
 positions->setEnableDocking(KDockWidget::DockCorner);  
 positions->setDockSite(KDockWidget::DockCorner);
 
 //Enable the View to be inform that the positions dockWidget is being close.
 //To do so, connect the positions dockwidget close button to the dockBeingClosed slot of is contained widget
 //and connect this widget parentDockBeingClosed signal to the view positionDockClosed slot.
 connect(positions, SIGNAL(headerCloseButtonClicked()),positionView, SLOT(dockBeingClosed()));
 connect(positionView, SIGNAL(parentDockBeingClosed(QWidget*)), this, SLOT(positionDockClosed(QWidget*)));

 //Set the different connections with the view
 connect(this,SIGNAL(positionInformationUpdated(int,int,QImage,bool,bool)),positionView,SLOT(updatePositionInformation(int,int,QImage,bool,bool)));
 connect(this,SIGNAL(timeChanged(long,long)),positionView,SLOT(displayTimeFrame(long,long)));
 connect(this,SIGNAL(changeBackgroundColor(QColor)),positionView, SLOT(changeBackgroundColor(QColor)));
 connect(traceWidget,SIGNAL(eventsAvailable(QDict<EventData>&,QMap<QString, QValueList<int> >&,
         QDict<ItemColors>&,QObject*,double)),positionView,SLOT(dataAvailable(QDict<EventData>&,QMap<QString, QValueList<int> >&,QDict<ItemColors>&,QObject*,double)));
 connect(this,SIGNAL(updateEventDisplay()),positionView,SLOT(updateEventDisplay()));        
 connect(this,SIGNAL(eventColorUpdated(QString,int,bool)),positionView,SLOT(eventColorUpdate(QString,int,bool)));
 connect(this,SIGNAL(updateDrawing()),positionView, SLOT(updateDrawing()));
 connect(this,SIGNAL(newEventProvider(EventsProvider*,QString,ItemColors*,bool,QValueList<int>&,const QValueList<int>&)),positionView,SLOT(addEventProvider()));
 connect(this,SIGNAL(eventProviderRemoved(QString,bool,bool)),positionView,SLOT(removeEventProvider(QString,bool,bool)));
 connect(this,SIGNAL(eventsShownInPositionView(bool)),positionView,SLOT(setEventsInPositionView(bool))); 
       
 //Request the data for all the events (can be done only after the connection has be set)
 if(eventsInPositionView) globalEventProvider.requestData(startTime,startTime + duration,positionView);
  
 //Show all the enclosed widgets of the dockWindows.
 showAllWidgets(); 
}


void  NeuroscopeView::positionDockClosed(QWidget* view){
 removePositionView();
 emit positionViewClosed();
}

void NeuroscopeView::removePositionView(){
 KDockWidget* dock = dockManager->findWidgetParentDock(positionView);
 dock->undock();
 delete dock;
 positionView = 0L;
 isPositionFileShown = false;  
}
 
void NeuroscopeView::resetOffsets(const QValueList<int>& selectedIds){
 NeuroscopeDoc* doc = mainWindow.getDocument(); 
 
 const QMap<int,int>& channelDefaultOffsets = doc->getChannelDefaultOffsets();
 QMap<int,int> selectedChannelDefaultOffsets;
 
 //update the list of selected channels
 selectedChannels.clear();
 QValueList<int>::const_iterator selectedIterator;
 for(selectedIterator = selectedIds.begin(); selectedIterator != selectedIds.end(); ++selectedIterator){
  selectedChannels.append(*selectedIterator);
  selectedChannelDefaultOffsets.insert(*selectedIterator,channelDefaultOffsets[*selectedIterator]);
 }
  
 emit resetChannelOffsets(selectedChannelDefaultOffsets);
}

int NeuroscopeView::getRasterHeight(){
 return traceWidget->getRasterHeight();
}

void NeuroscopeView::setEventsInPositionView(bool shown){
 eventsInPositionView = shown;
 emit eventsShownInPositionView(shown);
}


#include "neuroscopeview.moc"
