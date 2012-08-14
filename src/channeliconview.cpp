/***************************************************************************
                          channeliconview.cpp  -  description
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
#include "channeliconview.h"

// include files for KDE
#include <klocale.h>
#include <kmessagebox.h>

// include files for Qt
#include <qcursor.h>
#include <qtextcodec.h>



ChannelIconView::ChannelIconView(QColor backgroundColor,int gridX,int gridY,bool edit,QWidget* parent,const char* name,WFlags f):
QIconView(parent,name,f){
  QFont font( "Helvetica",8);
  setFont(font);
  setSpacing(4);
  setFrameStyle(QFrame::Box | QFrame::Plain);
  setLineWidth(1);  
  setArrangement(QIconView::LeftToRight);
  setResizeMode(QIconView::Adjust);
  setGridX(gridX);
  setGridY(gridY);
  arrangeItemsInGrid();
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
  if(edit){
   drag = true;
   setItemsMovable(true);
  } 
  else{
   drag = false;
   setItemsMovable(false); 
  } 
  setSpacing(4);
  setAutoArrange(true);
  setSorting(false);
  
  setHScrollBarMode(QScrollView::AlwaysOff);
  setVScrollBarMode(QScrollView::AlwaysOff);

  connect(this,SIGNAL(dropped(QDropEvent *,const QValueList<QIconDragItem> &)),
   this,SLOT(slotDropped(QDropEvent *,const QValueList<QIconDragItem> &)));
}

QDragObject* ChannelIconView::dragObject(){
 if(!currentItem() || !drag) return 0;

 QIconDrag* drag = new QIconDrag(viewport());
 drag->setPixmap(*currentItem()->pixmap(),
      QPoint(currentItem()->pixmapRect().width() / 2, currentItem()->pixmapRect().height() / 2));

 QPoint orig = viewportToContents( viewport()->mapFromGlobal(QCursor::pos()));
 //Insert only one item to have a pixmap to draw.
 for(QIconViewItem* item = firstItem();item; item = item->nextItem()){
  if(item->isSelected()){
     QIconDragItem id;
     id.setData(item->text().local8Bit());
     drag->append(id,
 		  QRect(item->pixmapRect(FALSE).x() - orig.x(),
 			 item->pixmapRect(FALSE).y() - orig.y(),
 			 item->pixmapRect().width(),item->pixmapRect().height()),
 		  QRect(item->textRect(FALSE).x() - orig.x(),
 			 item->textRect(FALSE).y() - orig.y(),
 			 item->textRect().width(),item->textRect().height()));
     break;
	 }
  }  
  
  return drag;
}

void ChannelIconView::slotDropped(QDropEvent* event,const QValueList<QIconDragItem>& draggedList){
 //The source of the drag is not a widget of the application
 if(event->source() == 0 || !drag){
	event->ignore();
	return;
 }
 
 //Drop of a label to move the whole block
 if(QString(event->format()).contains("text/plain")){
  QString information;
  if(QTextDrag::decode(event,information)){
   int groupSource = information.section("-",0,0).toInt();
   int start = information.section("-",1,1).toInt();
   QString groupTarget = this->name();
   emit dropLabel(groupSource,groupTarget.toInt(),start,QWidget::mapToGlobal(event->pos()).y());
   return;
  }
 }

 
 if(event->action() == QDropEvent::Move){
  event->acceptAction();

  QString groupSource = (event->source())->parent()->name();

  QValueList<int> channelIds;
  QValueList<QIconDragItem>::const_iterator iterator;
  for(iterator = draggedList.begin(); iterator != draggedList.end(); ++iterator){
   QTextCodec* codec = QTextCodec::codecForLocale();
   QByteArray data =  (*iterator).data();
   int channelId = codec->toUnicode(data).toInt();
   channelIds.append(channelId);
  }

  QValueList<int> selectedChannels;
  for(QIconViewItem* item = firstItem(); item; item = item->nextItem())
   if(item->isSelected()) selectedChannels.append(item->text().toInt());

  //If all the channels are selected insert after the first one. 
  if(selectedChannels.size() == this->count()){
   emit channelsMoved(this->name(),0);
   return; 
  }
    
  QIconViewItem* after = findItemToInsertAfter(event->pos());
  emit channelsMoved(this->name(),after);
 }
}

void ChannelIconView::contentsDropEvent(QDropEvent* event){
 if(event->source() == 0 || !drag){
 	event->ignore();
 	return;
  }
 if((event->source())->parent()->name() != name()){
  QIconView::contentsDropEvent(event);
  return;
 }

 //Move items around in the iconview 
 QIconViewItem* item = findItem(event->pos());
 if(item == 0){
  QValueList<int> selectedChannels;
  for(QIconViewItem* item = firstItem(); item; item = item->nextItem())
   if(item->isSelected()) selectedChannels.append(item->text().toInt());

  //If all the items have been selected, do not do anything 
  if(selectedChannels.size() == count()){
   QIconView::contentsDropEvent(event);
   arrangeItemsInGrid();
   return;
  }

  QIconViewItem* after = findItemToInsertAfter(event->pos());
 
  QIconView::contentsDropEvent(event);
  emit channelsMoved(selectedChannels,this->name(),after); 
 }  
 else{
  QIconView::contentsDropEvent(event);
  return;
 }
}

QIconViewItem* ChannelIconView::findItemToInsertAfter(QPoint position){
  int posX = position.x();
  int posY = position.y();
    
  int firstY = firstItem()->pos().y();  
  //test if the position is above all the other items, if so find the item to insert after.
  if(posY < firstY){
   QIconViewItem* after = 0L; 
   for(QIconViewItem* item = firstItem(); item; item = item->nextItem()){
    if(!item->isSelected()){
      after = item;
     if(((item->pos().y() == firstY) && (item->pos().x() >= posX)) || (item->pos().y() > firstY)){
      if(item->index() == 0) return 0; 
      else{
       //take the first item before the current one which is not selected (<=> not to be moved)
       QIconViewItem* after = item->prevItem(); 
       while(after->isSelected()){
        if(after->index() == 0) return 0;
        else after = after->prevItem(); 
       } 
       return after;  
      } 
     }
    }
   }
   //if the location is on above the first line but after the last item of the first line.
   return after;
  }

  int lastY = lastItem()->pos().y(); 
  //else test if the position is below all the other items, if so find item to insert after.
  if(posY > lastY){
   for(QIconViewItem* item = lastItem(); item; item = item->prevItem()){
    if(!item->isSelected()){
     if(((item->pos().y() == lastY) && (item->pos().x() <= posX)) || (item->pos().y() < lastY)){
      if(item->index() == 0 && posX < item->pos().x()) return 0;
      else return item;
     }
    }
   }  
  }
  //else the other cases
  QIconViewItem* item;
  for(item = lastItem(); item;item = item->prevItem())
   if(!item->isSelected() && (item->pos().y() <= posY)) break;

  int maxY = item->pos().y();
  for(;item;item = item->prevItem()){
   if(!item->isSelected()){         
    if((item->pos().x() <= posX) || (item->pos().y() < maxY) || item == firstItem()){
      if(item->index() == 0 && posX < item->pos().x()) return 0;
      else return item;     
    }
   } 
  }

  //Normally never reach
  return lastItem();
}

void ChannelIconView::contentsMousePressEvent(QMouseEvent* event){
 //If the user did not clicked on an item, ignore the click
 QIconViewItem* item = findItem(event->pos()); 
 if(item == 0L) return;

//  if(event->button() == LeftButton && !(event->state() & ShiftButton) &&
//   !(event->state() & ControlButton)){
//    emit moussePressWoModificators(this->name());
//  }

 QIconView::contentsMousePressEvent(event);
}










#include "channeliconview.moc"
