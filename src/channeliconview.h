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
#include <QListWidgetItem>
#include <qwidget.h>

#include <QMouseEvent>
#include <QList>
#include <QWheelEvent>
#include <QDropEvent>

/**Utilitary class used to build the channel palettes (anatomical and spike).
  *@author Lynn Hazan
  */
class ChannelIconViewItem : public QListWidgetItem {
public:
    ChannelIconViewItem(const QIcon &icon, const QString &text, QListWidget *view, int type)
        : QListWidgetItem(icon, text, view, type)
    {
        // Drop between items, not onto items
        setFlags((flags() | Qt::ItemIsDragEnabled) & ~Qt::ItemIsDropEnabled);
    }

};


class ChannelIconView : public QListWidget  {
    Q_OBJECT
public:
    explicit ChannelIconView(const QColor& backgroundColor,int gridX,int gridY,bool edit,QWidget* parent = 0,const QString& name = QString());
    ~ChannelIconView(){}

public Q_SLOTS:
    void setDragAndDrop(bool dragDrop);

Q_SIGNALS:
    void mousePressMiddleButton(QListWidgetItem* item);
    void moussePressWoModificators(const QString& sourceGroup);
    void channelsMoved(const QString& targetGroup,QListWidgetItem* after);
    void channelsMoved(const QList<int>& channelIds,const QString& sourceGroup,QListWidgetItem* after);
    void dropLabel(int sourceId,int targetId,int start,int destination);


protected:
    void contentsWheelEvent(QWheelEvent* event){event->accept();}
    void mousePressEvent(QMouseEvent *event);
    void wheelEvent ( QWheelEvent * e );
    QMimeData* mimeData(const QList<QListWidgetItem*> items) const;
    bool dropMimeData(int index, const QMimeData * data, Qt::DropAction action);
    Qt::DropActions supportedDropActions() const
    {
        return Qt::MoveAction;
    }
    QStringList mimeTypes() const
    {
        return QStringList() << "application/x-channeliconview";
    }

};

#endif
