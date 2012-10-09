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
#include <QListWidget>
#include <q3dragobject.h>
#include <qwidget.h>
//Added by qt3to4:
#include <QMouseEvent>
#include <QList>
#include <QWheelEvent>
#include <QDropEvent>

/**Utilitary class used to build the channel palettes (anatomical and spike).
  *@author Lynn Hazan
  */

class ChannelIconView : public QListWidget  {
    Q_OBJECT
public:
    explicit ChannelIconView(const QColor& backgroundColor,int gridX,int gridY,bool edit,QWidget* parent = 0,const char* name = 0);
    ~ChannelIconView(){}

public Q_SLOTS:
    void setDragAndDrop(bool dragDrop);

protected:
    void mousePressEvent(QMouseEvent* event);
    void wheelEvent ( QWheelEvent * e );
    void contentsWheelEvent(QWheelEvent* event){event->accept();}

#if 0

protected:
    virtual Q3DragObject* dragObject();
    virtual void contentsDropEvent(QDropEvent* event);

Q_SIGNALS:
    void channelsMoved(QString targetGroup,Q3IconViewItem* after);
    void channelsMoved(const QList<int>& channelIds,QString sourceGroup,Q3IconViewItem* after);
    void dropLabel(int sourceId,int targetId,int start,int destination);
    void moussePressWoModificators(QString sourceGroup);


protected Q_SLOTS:
    void slotDropped(QDropEvent* event, const Q3ValueList<Q3IconDragItem> &draggedList);

private:
    /**Return 0 if it has to be before the first one.*/
    Q3IconViewItem* findItemToInsertAfter(QPoint position);
#endif
    /**True the drag and drop is allow, false otherwise.*/
    bool drag;

};

#endif
