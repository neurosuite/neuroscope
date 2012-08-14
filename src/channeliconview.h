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
#include <qpainter.h>
#include <qiconview.h>
#include <qdragobject.h>
#include <qwidget.h>

//General C++ include files
#include <iostream>
using namespace std;

/**Utilitary class used to build the channel palettes (anatomical and spike).
  *@author Lynn Hazan
  */

class ChannelIconView : public QIconView  {
   Q_OBJECT
public:
   ChannelIconView(QColor backgroundColor,int gridX,int gridY,bool edit,QWidget* parent = 0,const char* name = 0, WFlags f = 0);
   inline ~ChannelIconView(){};
   
  protected:
   virtual QDragObject* dragObject();
   virtual void contentsDropEvent(QDropEvent* event);
   virtual void contentsMousePressEvent(QMouseEvent* event);
   inline void contentsWheelEvent(QWheelEvent* event){event->accept();};

  signals:
   void channelsMoved(QString targetGroup,QIconViewItem* after);
   void channelsMoved(const QValueList<int>& channelIds,QString sourceGroup,QIconViewItem* after);
   void dropLabel(int sourceId,int targetId,int start,int destination);
   void moussePressWoModificators(QString sourceGroup);

  public slots:
    inline void setDragAndDrop(bool dragDrop){drag = dragDrop;};
      
  protected slots:
   void slotDropped(QDropEvent* event,const QValueList<QIconDragItem>& draggedList);

  private:
  /**Return 0 if it has to be before the first one.*/
   QIconViewItem* findItemToInsertAfter(QPoint position);

   /**True the drag and drop is allow, false otherwise.*/
   bool drag;
      
};

#endif
