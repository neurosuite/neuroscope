/***************************************************************************
                          itemiconview.cpp  -  description
                             -------------------
    begin                : Fri Mar 5 2004
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
// application specific includes
#include "itemiconview.h"

// include files for KDE



// include files for Qt
#include <qcursor.h>
#include <qtextcodec.h>
//Added by qt3to4:
#include <QMouseEvent>
#include <Q3Frame>


ItemIconView::ItemIconView(QColor backgroundColor,Q3IconView::ItemTextPos position,int gridX,int gridY,QWidget* parent,const char* name,Qt::WFlags f):
Q3IconView(parent,name,f){
  QFont font( "Helvetica",8);
  setFont(font);
  setSpacing(4);
  setFrameStyle(Q3Frame::NoFrame);
  setArrangement(LeftToRight);
  setResizeMode(Q3IconView::Adjust); 
  setItemTextPos(position);
  setGridX(gridX);
  setGridY(gridY);  
  arrangeItemsInGrid();  
  setWordWrapIconText(false);
  setDragAutoScroll(false);
 
  //Set the iconView color, the foreground color depends on the background color
  setPaletteBackgroundColor(backgroundColor);
  int h;
  int s;
  int v;
  backgroundColor.hsv(&h,&s,&v);
  QColor legendColor;
  if(s <= 80 && v >= 240 || (s <= 40 && v >= 220)) legendColor = Qt::black;
  else legendColor = Qt::white;
  setPaletteForegroundColor(legendColor);
  setSelectionMode(Q3IconView::Extended);
  setItemsMovable(false);

  setSpacing(4);
  setAutoArrange(true);
  setSorting(false);

  setHScrollBarMode(Q3ScrollView::AlwaysOff);
  setVScrollBarMode(Q3ScrollView::AlwaysOff);

  setFrameStyle(Q3Frame::Box | Q3Frame::Plain);
  setLineWidth(1);

  connect(this,SIGNAL(mouseButtonPressed(int,Q3IconViewItem*,const QPoint&)),this, SLOT(slotMousePressed(int,Q3IconViewItem*)));
}

void ItemIconView::contentsMousePressEvent(QMouseEvent* event){  

 //If the user did not clicked on an item, ignore the click
 Q3IconViewItem* item = findItem(event->pos()); 
 if(item == 0L) return;

 /*if(event->button() == Qt::LeftButton && !(event->state() & Qt::ShiftModifier) &&
   !(event->state() & Qt::ControlModifier) && !(event->state() & Qt::AltModifier)){
  emit mousePressWoModificators(this->name());
 }*/

 //ask for mark an item skip
 if(event->button() == Qt::LeftButton && (event->state() & Qt::AltModifier) && (event->state() & Qt::ControlModifier)){
 
  Q3IconViewItem* item = findItem(event->pos()); 
  if(item != 0L){
   emit mousePressWQt::AltModifier(this->name(),item->index());
  }  
  return;
 }

 //Lets the parent take care of the usual iconview managment
 Q3IconView::contentsMousePressEvent(event);
}



#include "itemiconview.moc"
