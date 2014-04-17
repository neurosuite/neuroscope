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
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
// application specific includes
#include "channeliconview.h"
#include "channelmimedata.h"
// include files for Qt
#include <QCursor>
#include <QTextCodec>
#include <QMimeData>

#include <QDropEvent>
#include <QFrame>
#include <QList>
#include <QMouseEvent>
#include <QDebug>
#include <QApplication>
#include <QAbstractItemModel>

ChannelIconView::ChannelIconView(const QColor& backgroundColor, int gridX, int gridY, bool edit, QWidget* parent, const QString& name)
    : QListWidget(parent)
{
    setObjectName(name);
    QFont font( "Helvetica",8);
    setFont(font);
    setSpacing(4);
    setFrameStyle(QFrame::Box | QFrame::Plain);
    setLineWidth(1);
    setResizeMode(QListWidget::Adjust);
    setGridSize(QSize(gridX, gridY));
    setViewMode(QListView::IconMode);
    setDragDropMode(QAbstractItemView::DragDrop); // no internal moves
    setAutoFillBackground(true);
    //Set the iconView color, the foreground color depends on the background color
    QPalette palette = this->palette();
    palette.setColor(backgroundRole(), backgroundColor);
    int h;
    int s;
    int v;
    backgroundColor.getHsv(&h,&s,&v);
    QColor legendColor;
    if (s <= 80 && v >= 240 || (s <= 40 && v >= 220))
        legendColor = Qt::black;
    else
        legendColor = Qt::white;

    palette.setColor(QPalette::Text, legendColor);
    palette.setColor(QPalette::HighlightedText, legendColor);
    setPalette(palette);

    setSelectionMode(QAbstractItemView::ExtendedSelection);

    if (edit) {
        setDragEnabled(true);
        setMovement(QListView::Snap);
    } else {
        setDragEnabled(false);
        setMovement(QListView::Static);
    }
    setSelectionRectVisible(false);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setSizePolicy(QSizePolicy::Minimum,QSizePolicy::Minimum);
    connect(model(), SIGNAL(rowsInserted(QModelIndex, int,int)), this, SIGNAL(rowInsered()));
}


ChannelIconView::~ChannelIconView()
{
}

void ChannelIconView::slotRowInsered()
{
    adjustSize();
    resize(sizeHint());
}

QMimeData* ChannelIconView::mimeData(const QList<QListWidgetItem*> items) const
{
    if (items.isEmpty())
        return 0;
    QMimeData* mimedata = new QMimeData();

    QByteArray data;
    //For the moment just one item
    QDataStream stream(&data, QIODevice::WriteOnly);
    Q_FOREACH(QListWidgetItem *item, items) {
        stream << *item;
    }

    mimedata->setData("application/x-channeliconview", data);
    mimedata->setData("application/x-channeliconview-number-item", QByteArray::number(items.count()));
    mimedata->setData("application/x-channeliconview-name", objectName().toUtf8());
    mimedata->setData("application/x-channeliconview-move-all-channels", (items.count() == count()) ? "true" : "false");

    return mimedata;
}

void ChannelIconView::setNewWidth(int width)
{
    setFixedWidth(width);
    doItemsLayout();
    resize(sizeHint());
}

QSize ChannelIconView::sizeHint() const
{
    const int height = rectForIndex(model()->index(model()->rowCount() - 1, 0)).bottom() + 5;
    return QSize(width(), height);
}

void ChannelIconView::setDragAndDrop(bool dragDrop)
{
    setDragEnabled(dragDrop);
}

void ChannelIconView::wheelEvent ( QWheelEvent * event )
{
    event->accept();
}

bool ChannelIconView::dropMimeData(int index, const QMimeData * mimeData, Qt::DropAction action)
{
    Q_UNUSED(action);

    if (ChannelMimeData::hasInformation(mimeData)) {
        int groupSource, start;
        ChannelMimeData::getInformation(mimeData, &groupSource, &start);
        QString groupTarget = objectName();
        emit dropLabel(groupSource,groupTarget.toInt(),start,/*QWidget::mapToGlobal(event->pos()).y()*/0);
        return true;
    }

    const QByteArray data = mimeData->data("application/x-channeliconview");
    if (data.isEmpty())
        return false;

    qDebug()<<" index "<<index;

    const QString sourceGroupName = QString::fromUtf8(mimeData->data("application/x-channeliconview-name"));
    QDataStream stream(data);
    const bool moveAllGroup = (mimeData->data("application/x-channeliconview-move-all-channels") == "true");
    const int numberOfItems = mimeData->data("application/x-channeliconview-number-item").toInt();

    if (sourceGroupName!= objectName()) {
        //TODO this part is buggy
        QList<int> channelIds;
        for (int i=0; i< numberOfItems; ++i) {
            ChannelIconViewItem *item = new ChannelIconViewItem(this);
            stream >> *item;
            channelIds.append(item->text().toInt());
            qDebug()<<" channelIds"<<channelIds;
            delete item;
        }
        if (!channelIds.isEmpty()) {
            emit moveListItem(channelIds, sourceGroupName, objectName(), index, moveAllGroup);
        }
    } else {
        //Same group
        if (moveAllGroup) {
            //don't move it.
            return false;
        }

        QList<int> channelIds;
        for (int i=0; i< numberOfItems; ++i) {
            ChannelIconViewItem *item = new ChannelIconViewItem(this);
            stream >> *item;
            channelIds.append(item->text().toInt());
            delete item;
        }

        QListWidgetItem *posItem = item(index);
        if (!posItem) {
            //Find last item
            posItem = item(count()-1);
        }
        emit channelsMoved(channelIds, sourceGroupName, posItem);
    }
    return true;
}

void ChannelIconView::mousePressEvent(QMouseEvent* event)
{
    //If the user did not clicked on an item, ignore the click
    QListWidgetItem* item = itemAt(event->pos());
    if (item == 0L)
        return;

    if (event->button() == Qt::MiddleButton) {
        emit mousePressMiddleButton(item);
    }
    QListWidget::mousePressEvent(event);
}

void ChannelIconView::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Right) {
        QListWidgetItem *c = currentItem();
        if (c) {
            const int i = row(c);
            if (i < count()-1) {
                setCurrentRow(i+1);
            }
        }
    } else if (event->key() == Qt::Key_Left) {
        QListWidgetItem *c = currentItem();
        if (c) {
            const int i = row(c);
            if (i > 0) {
                setCurrentRow(i-1);
            }
        }
    } else {
        QListWidget::keyPressEvent(event);
    }
}
