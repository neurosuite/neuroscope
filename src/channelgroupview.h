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
#include <QObject> 
#include <QPainter>
//Added by qt3to4:
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QHBoxLayout>

class QListWidget;

/**Utilitary class used to build the channel palettes (anatomical and spike).
  *@author Lynn Hazan
  */

class ChannelGroupView : public QWidget  {
    Q_OBJECT
public: 
    explicit ChannelGroupView(bool drag,const QColor& backgroundColor,QWidget* parent=0);

    ~ChannelGroupView(){}

    void setIconView(QListWidget *view);

Q_SIGNALS:
    void dropLabel(int sourceId,int targetId,int start, int destination);
    void dragObjectMoved(QPoint position);
    
public Q_SLOTS:
    void reAdjustSize(int parentWidth,int labelSize);

    void setDragAndDrop(bool dragDrop){drag = dragDrop;}

protected:
    virtual void dropEvent(QDropEvent* event);

    virtual void dragEnterEvent(QDragEnterEvent* event);

private:
    QListWidget* iconView;

    /**True the drag and drop is allow, false otherwise.*/
    bool drag;
    QHBoxLayout *mLayout;
    bool init;

};

#endif
