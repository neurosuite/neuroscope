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
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef CHANNELICONVIEW_H
#define CHANNELICONVIEW_H

//QT include files
#include <QWidget>
#include <QPainter>
#include <QListWidget>
#include <QListWidgetItem>
#include <QWidget>

#include <QMouseEvent>
#include <QList>
#include <QWheelEvent>
#include <QDropEvent>

/**Utilitary class used to build the channel palettes (anatomical and spike).
  *@author Lynn Hazan
  */
class ChannelIconViewItem : public QListWidgetItem {
public:
    ChannelIconViewItem( QListWidget *view = 0)
        : QListWidgetItem(view)
    {
        // Drop between items, not onto items
        setFlags(flags() | (Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled));
    }

    ChannelIconViewItem(const QIcon &icon, const QString &text, QListWidget *view = 0)
        : QListWidgetItem(icon, text, view)
    {
        // Drop between items, not onto items
        setFlags(flags() | (Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled));
    }

};


class ChannelIconView : public QListWidget  {
    Q_OBJECT
public:
    explicit ChannelIconView(const QColor& backgroundColor,int gridX,int gridY,bool edit,QWidget* parent = 0,const QString& name = QString());
    ~ChannelIconView();

    void setNewWidth(int width);

    QSize sizeHint() const;

public Q_SLOTS:
    void setDragAndDrop(bool dragDrop);
    void slotRowInsered();

Q_SIGNALS:
    void mousePressMiddleButton(QListWidgetItem* item);
    void channelsMoved(const QString& targetGroup,QListWidgetItem* after );
    void channelsMoved(const QList<int>& channelIds,const QString& sourceGroup,QListWidgetItem* after);
    void dropLabel(int sourceId,int targetId,int start,int destination);

    void removeGroup(const QString &name);
    void moveListItem(const QList<int> &listId, const QString &sourceGroupName, const QString &destGroupName, int index, bool moveAll);
    void rowInsered();

protected:
    void keyPressEvent(QKeyEvent *event);
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
    // Skip internal dnd handling in QListWidget ---- how is one supposed to figure this out
    // without reading the QListWidget code !?
    virtual void dropEvent(QDropEvent* ev) {
        QAbstractItemView::dropEvent(ev);
    }

};


#endif
