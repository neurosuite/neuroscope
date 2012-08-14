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
#include <klocale.h>
#include <kmessagebox.h>

// include files for Qt
#include <qcursor.h>
#include <qtextcodec.h>


ItemIconView::ItemIconView(QColor backgroundColor,QIconView::ItemTextPos position,int gridX,int gridY,QWidget* parent,const char* name,WFlags f):
QIconView(parent,name,f){
  QFont font( "Helvetica",8);
  setFont(font);
  setSpacing(4);
  setFrameStyle(QFrame::NoFrame);
  setArrangement(LeftToRight);
  setResizeMode(QIconView::Adjust); 
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
  if(s <= 80 && v >= 240 || (s <= 40 && v >= 220)) legendColor = black;
  else legendColor = white;
  setPaletteForegroundColor(legendColor);
  setSelectionMode(QIconView::Extended);
  setItemsMovable(false);

  setSpacing(4);
  setAutoArrange(true);
  setSorting(false);

  setHScrollBarMode(QScrollView::AlwaysOff);
  setVScrollBarMode(QScrollView::AlwaysOff);

  setFrameStyle(QFrame::Box | QFrame::Plain);
  setLineWidth(1);

  connect(this,SIGNAL(mouseButtonPressed(int,QIconViewItem*,const QPoint&)),this, SLOT(slotMousePressed(int,QIconViewItem*)));
}

void ItemIconView::contentsMousePressEvent(QMouseEvent* event){  

 //If the user did not clicked on an item, ignore the click
 QIconViewItem* item = findItem(event->pos()); 
 if(item == 0L) return;

 /*if(event->button() == LeftButton && !(event->state() & ShiftButton) &&
   !(event->state() & ControlButton) && !(event->state() & AltButton)){
  emit mousePressWoModificators(this->name());
 }*/

 //ask for mark an item skip
 if(event->button() == LeftButton && (event->state() & AltButton) && (event->state() & ControlButton)){
 
  QIconViewItem* item = findItem(event->pos()); 
  if(item != 0L){
   emit mousePressWAltButton(this->name(),item->index());
  }  
  return;
 }

 //Lets the parent take care of the usual iconview managment
 QIconView::contentsMousePressEvent(event);
}



#include "itemiconview.moc"
