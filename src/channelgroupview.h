/***************************************************************************
                          channelgroupview.h  -  description
                             -------------------
    begin                : Thu Mar 4 2004
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

#ifndef CHANNELGROUPVIEW_H
#define CHANNELGROUPVIEW_H

#include <qwidget.h>
#include <q3hbox.h>
#include <q3iconview.h>
#include <qobject.h> 
#include <qpainter.h>
//Added by qt3to4:
#include <QDragEnterEvent>
#include <QDropEvent>

//General C++ include files
#include <iostream>
using namespace std;

/**Utilitary class used to build the channel palettes (anatomical and spike).
  *@author Lynn Hazan
  */

class ChannelGroupView : public Q3HBox  {
   Q_OBJECT
public: 
	inline ChannelGroupView(bool drag,QColor backgroundColor,QWidget* parent=0, const char* name=0):Q3HBox(parent,name),iconView(0L),drag(drag),init(true){

    //Set the groupview color, the foreground color depends on the background color
    setPaletteBackgroundColor(backgroundColor);
    int h;
    int s;
    int v;
    backgroundColor.hsv(&h,&s,&v);
    QColor legendColor;
    if(s <= 80 && v >= 240 || (s <= 40 && v >= 220)) legendColor = Qt::black;
    else legendColor = Qt::white;
    setPaletteForegroundColor(legendColor);
    setMargin(0);
    setSpacing(0);
    adjustSize();

    setAcceptDrops(TRUE);
  };

  inline ~ChannelGroupView(){};

  inline void setIconView(Q3IconView* view){
   iconView = view;
  };
  
signals:
  void dropLabel(int sourceId,int targetId,int start, int destination);
  void dragObjectMoved(QPoint position);
    
public slots:
  inline void reAdjustSize(int parentWidth,int labelSize){
   if((iconView->contentsWidth() != 1 && width() != parentWidth) || init){
    init = false; 
    int futurWidth = parentWidth ;

    setFixedWidth(futurWidth);
    int viewfuturWidth = width() - labelSize - 6;//give so space on the right
    iconView->setFixedWidth(viewfuturWidth);
    
    if(iconView->contentsHeight() != 1 && height() != iconView->contentsHeight())
     setFixedHeight(iconView->contentsHeight());
     
   }
   //If items have been moved in or out of the iconview, its sized has changed and the ChannelGroupView has to compensate
   if(iconView->contentsHeight() != 1 && height() != iconView->contentsHeight())
    setFixedHeight(iconView->contentsHeight());   
  };

 inline void setDragAndDrop(bool dragDrop){drag = dragDrop;};
  
protected:
  inline virtual void dropEvent(QDropEvent* event){
   if(event->source() == 0 || !drag){
 	  event->ignore();
 	  return;
   }

   QString information;
   if(Q3TextDrag::decode(event,information)){
    int groupSource = information.section("-",0,0).toInt();
    int start = information.section("-",1,1).toInt();
    QString groupTarget = this->name();
    emit dropLabel(groupSource,groupTarget.toInt(),start,QWidget::mapToGlobal(event->pos()).y());
   }
  };

  inline virtual void dragEnterEvent(QDragEnterEvent* event){
   if(event->source() == 0 || !drag){
 	  event->ignore();
 	  return;
   }   
   event->accept(Q3TextDrag::canDecode(event));
   //Enable the parent (ChannelPalette) to ensure that the current group is visible (will scroll if need it)
   emit dragObjectMoved(QWidget::mapToParent(event->pos()));
  };
                    
 private:
  Q3IconView* iconView;

  /**True the drag and drop is allow, false otherwise.*/
  bool drag;

  bool init;

};

#endif
