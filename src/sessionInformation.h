/***************************************************************************
                          sessionInformation.h  -  description
                             -------------------
    begin                : Tue Mar 2 2004
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

 #ifndef SESSIONINFORMATION_H
#define SESSIONINFORMATION_H

//include files for QT
#include <QDateTime>
#include <QColor>
#include <QString>
#include <qmap.h>
#include <QUrl>

//application specific include files.
#include "eventsprovider.h"

#include <iostream>
using namespace std;

/**
  *Class storing the information about the files used in a session.
  *@author Lynn Hazan
  */

class SessionFile {
public:

  /**Type of file.*/
  enum type{CLUSTER=0,SPIKE=1,EVENT=2,POSITION=3};

  inline SessionFile(){
   fileUrl = QUrl();
   fileType = CLUSTER;
   fileModification = QDateTime();//invalid QDateTime
  };

  inline SessionFile(QUrl url,type fileType,QDateTime modification):fileUrl(url),fileType(fileType),fileModification(modification){};

  inline ~SessionFile(){};

  /**Sets the url of the file.
  * @param url url of the file.
  */
  inline void setUrl(QUrl url){fileUrl = url;};

  /**Sets the type of the file (see SessionFile#type).
  * @param fileType type of the file.
  */
  inline void setType(type fileType){this->fileType = fileType;};

  /**Sets date and time when the file was last modified.
  * @param modification modification information.
  */
  inline void setModification(QDateTime modification){fileModification = modification;};

  /**Sets the color for a given item.
  * @param itemId  item identification.
  * @param color color of the item.
  */
  inline void setItemColor(EventDescription itemId,QString color){
   itemColors.insert(itemId,QColor(color));
  };

  /**Sets the url of the background image.
  * @param path path of the background image.
  * obsolete: the background image information is now store in an other class.
  */
  inline void setBackgroundPath(QString path){imagePath = path;};


  /**Gets the url of the file.
  * @return url of the file.
  */
  inline QUrl getUrl() const{return fileUrl;};

  /**Gets the type of the file (see SessionFile#type).
  * @return type of the file.
  */
  inline type getType() const{return fileType;};

  /**Gets date and time when the file was last modified.
  * @return modification information.
  */
  inline QDateTime getModification() const{return fileModification;};

  /**Gets the colors of the items.
  * @return map given the color of each item.
  */
  inline QMap<EventDescription,QColor> getItemColors() const{return itemColors;};

  /**Gets the url of the background image.
  * @return url of the background image.
  * obsolete: the background image information is now store in an other class.
  */
  inline QString getBackgroundPath() const{return imagePath;};


private:
  /**Url of the file.*/
  QUrl fileUrl;
  /**Url of the background image.*/
  QString imagePath;
  /**Type of the file.*/
  type fileType;
  /**Date and time when the file was last modified.*/
  QDateTime fileModification;
  /**Map given the color for each item.*/
  QMap<EventDescription,QColor> itemColors;
};

class TracePosition;

/**
  *Class storing the session information to be read from or write to a session file.
  *@author Lynn Hazan
  */

class DisplayInformation {
public:

  /**Type of spike display.*/
  enum spikeDisplayType{RASTER=0,WAVEFORMS=1,LINES=2};

  /**Presentation mode.*/
  enum mode{SINGLE=0,MULTIPLE=1};

  /**Constructor, sets defaults:
  * - mode of presentation is set to single,
  * - the presentation of the labels is set to hide,
  * - the grey-scale is set to unused,
  * - the start time is set to 0,
  * - the duration is set to 50 ms.
  * - the rasterHeight is set to -1 meaning no value defined.
  .*/
  inline DisplayInformation(){
   presentationMode = SINGLE;
   labelStatus = 0;
   greyScale = 0;
   startTime = 0;
   timeWindow = 50;
   autocenterChannels = false;
   isPositionView = false;
   rasterHeight = -1;
   areEventsInPositionView = false;
  };

  inline ~DisplayInformation(){};

  /**Adds a type of spike display (see DisplayInformation#displayType).
  * @param type type of spike displays.
  */
  inline void addSpikeDisplayType(spikeDisplayType type){spikeDisplayTypes.append(type);};

  /**Sets the mode of presentation (see DisplayInformation#mode).
  * @param modeOfPresentation mode of presentation.
  */
  inline void setMode(mode modeOfPresentation){presentationMode = modeOfPresentation;};

  /**Sets the height of the rasters.
  * @param height height of the rasters.
  */
  inline void setRasterHeight(int height){rasterHeight = height;};

  /**Sets the grey-scale value.
  * @param value 1 if the grey-scale is used to the view 0, otherwise.
  */
  inline void setGreyScale(int value){greyScale = value;};

  /**Sets the selected clusters by cluster file.
  * @param clusterFile cluster file url.
  * @param clusterIds list of selected cluster ids.
  */
  inline void setSelectedClusters(const QString& clusterFile,const QList<int>& clusterIds){
    selectedClusters.insert(clusterFile,clusterIds);
  };

  /**Sets the selected events by event file.
  * @param eventFile event file url.
  * @param eventIds list of selected eventIds.
  */
  inline void setSelectedEvents(const QString& eventFile,const QList<int>& eventIds){
    selectedEvents.insert(eventFile,eventIds);
  };


  /**Sets the skipped clusters by cluster file.
  * @param clusterFile cluster file url.
  * @param clusterIds list of cluster ids to skip while browsing.
  */
  inline void setSkippedClusters(const QString& clusterFile,const QList<int>& clusterIds){
    skippedClusters.insert(clusterFile,clusterIds);
  };

  /**Sets the skipped events by event file.
  * @param eventFile event file url.
  * @param eventIds list of eventIds to skip while browsing.
  */
  inline void setSkippedEvents(const QString& eventFile,const QList<int>& eventIds){
    skippedEvents.insert(eventFile,eventIds);
  };

  /**Sets the list of the spike files shown in the display (without cluster file associated).
  * @param files list of the spike files.
  */
  inline void setSelectedSpikeFiles(QList<QString> files){shownSpikeFiles = files;};

   /**Sets the channel autocenter status.
  * @param autocenterChannels whether channels should be centered around their offset.
  */
  inline void setAutocenterChannels(bool autocenterChannels){this->autocenterChannels = autocenterChannels;};

   /**Sets the list of TracePosition.
  * @param positions list of TracePosition.
  */
  inline void setPositions(QList<TracePosition> positions){this->positions = positions;};

  /**Sets the list of channel ids shown in the display.
  * @param ids list of channel ids.
  */
  inline void setChannelIds(const QList<int>& ids) {channelIds = ids;};

  /**Sets the list channel ids selected in the display.
  * @param ids list of channel ids.
  */
  inline void setSelectedChannelIds(const QList<int>& ids){selectedChannelIds = ids;};

  /**Sets the starting time in miliseconds.
  * @param start starting time.
  */
  inline void setStartTime(long start){startTime = start;};

  /**Sets the time window in miliseconds
  * @param duration time window.
  */
  inline void setTimeWindow(long duration){timeWindow = duration;};

  /** Sets the label for the display when in tab page mode.
  * @param newLabel the new label for the display.
  */
  inline void setTabLabel(QString newLabel){tabLabel = newLabel;};

  /**Sets if the labels next to the traces have to be displayed.
  * @param value 1 if the labels are displayed in the view, 0 otherwise.
  */
  inline void setLabelStatus(int value){labelStatus = value;};

  /**Sets if a PositionView is displayed.
  * @param present 1 if a PositionView is displayed in the view, 0 otherwise.
  */
  inline void setPositionView(int present){isPositionView = present;};

  /**Sets if events are displayed in the PositionView.
  * @param shown 1 if events are displayed in the PositionView, 0 otherwise.
  */
  inline void setEventsInPositionView(int shown){areEventsInPositionView = shown;};

  /**Gets the list of type of spike display used (see DisplayInformation#displayType).
  * @return list of types of spike display.
  */
  inline QList<spikeDisplayType> getSpikeDisplayTypes() const{return spikeDisplayTypes;};

  /**Gets the mode of presentation (see DisplayInformation#mode).
  * @return mode of presentation.
  */
  inline mode getMode() const{return presentationMode;};

 /**Gets the height of the rasters.
  * @return height of the rasters.
  */
  inline int getRasterHeight() const{return rasterHeight;};

  /**Gets the grey-scale value.
  * @return grey-scale value, 1 if the grey-scale is used to the view 0 otherwise.
  */
  inline int getGreyScale() const{return greyScale;};

  /**Gets the selected clusters by cluster file.
  * @return map of the cluster file url with the list of selected clusterIds.
  */
  inline QMap<QString, QList<int> > getSelectedClusters() const{return selectedClusters;};

  /**Gets the selected events by event file.
  * @return map of the event file url with the list of selected eventIds.
  */
  inline QMap<QString, QList<int> > getSelectedEvents() const{return selectedEvents;};

  /**Gets the skipped clusters by cluster file.
  * @return map of the cluster file url with the list of clusterIds to skip while browsing.
  */
  inline QMap<QString, QList<int> > getSkippedClusters() const{return skippedClusters;};

  /**Gets the skipped events by event file.
  * @return map of the event file url with the list of eventIds to skip while browsing.
  */
  inline QMap<QString, QList<int> > getSkippedEvents() const{return skippedEvents;};

  /**Gets the list of the spike files shown in the display (without cluster file associated).
  * @return list of the spike files.
  */
  inline QList<QString> getSelectedSpikeFiles() const{return shownSpikeFiles;};

   /**Gets the channel autocenter status.
  * @param autocenterChannels whether channels should be centered around their offset.
  */
  inline bool getAutocenterChannels(){return autocenterChannels;};

  /**Gets the list of TracePosition.
  * @return list of TracePosition.
  */
  inline QList<TracePosition> getPositions() const{return positions;};

  /**Gets the list of channel ids shown in the display.
  * @return list of channel ids.
  */
  inline QList<int> getChannelIds() const{return channelIds;};

  /**Gets the list channel ids selected in the display.
  * @return list of channel ids.
  */
  inline QList<int> getSelectedChannelIds() const{return selectedChannelIds;};

  /** Gets the label for the display when in tab page mode.
  * @return the new label for the display.
  */
  inline QString getTabLabel()const{return tabLabel;};

  /**Gets if the information as if the labels next to the traces have to be displayed.
  * @return 1 if the labels are displayed in the view, 0 otherwise.
  */
  inline int getLabelStatus()const {return labelStatus;};

  /**Gets the starting time in miliseconds.
  * @return starting time.
  */
  inline long getStartTime()const{return startTime;};

  /**Gets the time window in miliseconds
  * @return duration time window.
  */
  inline long getTimeWindow()const{return timeWindow;};

  /**Returns if a PositionView is displayed in the view.
  * @return 1 if a PositionView is displayed in the view, 0 otherwise.
  */
  inline int isAPositionView()const{return isPositionView;};


 /**Returns if events are displayed in the PositionView.
  * @return 1 if events are displayed in the PositionView, 0 otherwise.
  */
  inline int isEventsDisplayedInPositionView()const{return areEventsInPositionView;};


private:
  /**List of the display types.*/
  QList<spikeDisplayType> spikeDisplayTypes;

  /**Mode of presentation.*/
  mode presentationMode;

  /**Height of the rasters.*/
  int rasterHeight;

  /**Grey-scale value, 1 if the grey-scale is used to the view, 0 otherwise.*/
  int greyScale;

  /**Map of the cluster file paths with the list of selected clusterIds.*/
  QMap<QString, QList<int> > selectedClusters;

  /**Map of the event file paths with the list of selected eventIds.*/
  QMap<QString, QList<int> > selectedEvents;

  /**Map of the cluster file paths with the list of clusterIds to skip while browsing.*/
  QMap<QString, QList<int> > skippedClusters;

  /**Map of the event file paths with the list of selected eventIds to skip while browsing.*/
  QMap<QString, QList<int> > skippedEvents;

  /**List of the spike files shown in the display (without cluster file associated).*/
  QList<QString> shownSpikeFiles;

  /**Channel autocenter status.*/
  bool autocenterChannels;

  /**List of TracePosition.*/
  QList<TracePosition> positions;

  /**List of channels shown in the display.*/
  QList<int> channelIds;

  /**List of channels selected in the display.*/
  QList<int> selectedChannelIds;

  /**Label for the display when in tab page mode.*/
  QString tabLabel;

  /*Starting time in miliseconds.*/
  long startTime;

  /*Time window in miliseconds.*/
  long timeWindow;

  /* 1 if the labels are displayed in the view, 0 otherwise. The default is 0.*/
  int labelStatus;

  /**1 if a PositionView is displayed in the view, 0 otherwise.*/
  int isPositionView;

  /**1 if events are display in the PositionView, 0 otherwise.*/
  int areEventsInPositionView;
};

/**
  *Class storing the channel color information to be read from or write to a session file.
  *@author Lynn Hazan
  */
class ChannelDescription {
public:

  inline ChannelDescription(){
   id = 0;
   color = QColor(Qt::black);
  };

  inline ChannelDescription(int id,QString color):id(id){
   setColor(color);
  };

  inline ~ChannelDescription(){};

  /**Sets the channel id.
  * @param channelId channel id.
  */
  inline void setId(int channelId){id = channelId;};

  /**Sets the color used to display the channel.
  * @param colorName name of the color in the format "#RRGGBB".
  */
  inline void setColor(QString colorName){color = QColor(colorName);};

  /**Sets the group color (anatomical group) for the channel.
  * @param colorName name of the color in the format "#RRGGBB".
  */
  inline void setGroupColor(QString colorName){groupColor = QColor(colorName);};

  /**Sets the spike group color for the channel.
  * @param colorName name of the color in the format "#RRGGBB".
  */
  inline void setSpikeGroupColor(QString colorName){spikeGroupColor = QColor(colorName);};

  /**Gets the channel id.
  * @return channel id.
  */
  inline int getId() const{return id;};

  /**Gets the color used to display the channel.
  * @return name of the color in the format "#RRGGBB".
  */
  inline QColor getColor() const{return color;};

  /**Gets the group color (anatomical group) for the channel.
  * @return name of the color in the format "#RRGGBB".
  */
  inline QColor getGroupColor() const{return groupColor;};

  /**Gets the spike group color for the channel.
  * @return name of the color in the format "#RRGGBB".
  */
  inline QColor getSpikeGroupColor() const{return spikeGroupColor;};

private:
  /**Channel id*/
  int id;

  /**Color used to display the channel.*/
  QColor color;

  /**Group color (anatomical group) for the channel.*/
  QColor groupColor;

  /**Spike group color for the channel.*/
  QColor spikeGroupColor;

};

/**
  *Class storing the trace position information in the Trace View to be read from or write to a session file.
  *@author Lynn Hazan
  */
class TracePosition {
public:

  inline TracePosition(){
   id = 0;
   offset = 0;
  };

  inline TracePosition(int id,int offset):id(id),offset(offset){};

  inline ~TracePosition(){};

  /**Sets the channel id.
  * @param channelId channel id.
  */
  inline void setId(int channelId){id = channelId;};

  /**Sets the offset apply to the channel.
  * @param value offset apply to the channel.
  */
  inline void setOffset(int value){offset = value;};

  /**Sets the gain apply to the channel.
  * @param value gain apply to the channel.
  */
  inline void setGain(int value){gain = value;};

  /**Gets the channel id.
  * @return channel id.
  */
  inline int getId() const{return id;};

  /**Gets the offset apply to the channel.
  * @return offset.
  */
  inline int getOffset() const{return offset;};

  /**Gets the gain apply to the channel.
  * @return gain.
  */
  inline int getGain() const{return gain;};

private:
  /**Channel id*/
  int id;

  /**Offset used to display the channel.*/
  int offset;

  /**Gain used to display the channel.*/
  int gain;

};



#endif //SESSIONINFORMATION_H
