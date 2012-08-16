/***************************************************************************
                          channeliconview.h  -  description
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

#ifndef CHANNELICONVIEW_H
#define CHANNELICONVIEW_H

//QT include files
#include <qwidget.h>
#include <QPainter>
#include <q3iconview.h>
#include <q3dragobject.h>
#include <qwidget.h>
//Added by qt3to4:
#include <QMouseEvent>
#include <Q3ValueList>
#include <QWheelEvent>
#include <QDropEvent>

//General C++ include files
#include <iostream>
using namespace std;

/**Utilitary class used to build the channel palettes (anatomical and spike).
  *@author Lynn Hazan
  */

class ChannelIconView : public Q3IconView  {
   Q_OBJECT
public:
   ChannelIconView(QColor backgroundColor,int gridX,int gridY,bool edit,QWidget* parent = 0,const char* name = 0, Qt::WFlags f = 0);
   inline ~ChannelIconView(){};
   
  protected:
   virtual Q3DragObject* dragObject();
   virtual void contentsDropEvent(QDropEvent* event);
   virtual void contentsMousePressEvent(QMouseEvent* event);
   inline void contentsWheelEvent(QWheelEvent* event){event->accept();};

  signals:
   void channelsMoved(QString targetGroup,Q3IconViewItem* after);
   void channelsMoved(const Q3ValueList<int>& channelIds,QString sourceGroup,Q3IconViewItem* after);
   void dropLabel(int sourceId,int targetId,int start,int destination);
   void moussePressWoModificators(QString sourceGroup);

  public slots:
    inline void setDragAndDrop(bool dragDrop){drag = dragDrop;};
      
  protected slots:
   void slotDropped(QDropEvent* event,const Q3ValueList<Q3IconDragItem>& draggedList);

  private:
  /**Return 0 if it has to be before the first one.*/
   Q3IconViewItem* findItemToInsertAfter(QPoint position);

   /**True the drag and drop is allow, false otherwise.*/
   bool drag;
      
};

#endif
