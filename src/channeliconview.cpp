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
    connect(model(), SIGNAL(rowsInserted(QModelIndex, int,int)), this, SLOT(slotRowInsered()));
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

    qDebug()<<" index "<<index;
    if (mimeData->hasText()) {
        const QString information = mimeData->text();
        const int groupSource = information.section("-",0,0).toInt();
        const int start = information.section("-",1,1).toInt();
        QString groupTarget = objectName();
        emit dropLabel(groupSource,groupTarget.toInt(),start,/*QWidget::mapToGlobal(event->pos()).y()*/0);
        return true;
    }

    const QByteArray data = mimeData->data("application/x-channeliconview");
    if (data.isEmpty())
        return false;

    QDataStream stream(data);

    QList<int> channelIds;
    const int numberOfItems = mimeData->data("application/x-channeliconview-number-item").toInt();
    for (int i=0; i< numberOfItems; ++i) {
        ChannelIconViewItem *item = new ChannelIconViewItem(this);
        stream >> *item;
        channelIds<<item->text().toInt();
        delete item;
    }

    const QString sourceGroupName = QString::fromUtf8(mimeData->data("application/x-channeliconview-name"));
    const bool moveAllGroup = (mimeData->data("application/x-channeliconview-move-all-channels") == "true");
    if (moveAllGroup) {
        const int numberOfItems = mimeData->data("application/x-channeliconview-number-item").toInt();
        for (int i=0; i< numberOfItems; ++i) {
            ChannelIconViewItem *item = new ChannelIconViewItem(this);
            stream >> *item;
        }
        emit removeGroup(sourceGroupName);
        //emit channelsMoved(objectName(), /*item(index)*/0);
        return true;
    }

    QListWidgetItem *posItem = item(index);
    if (!posItem) {
        //Find last item
        posItem = item(count()-1);
    }
    emit channelsMoved(objectName(), posItem);
    emit channelsMoved(channelIds, sourceGroupName, posItem);
    return true;
}

void ChannelIconView::mousePressEvent(QMouseEvent* event)
{
    //If the user did not clicked on an item, ignore the click
    QListWidgetItem* item = itemAt(event->pos());
    if (item == 0L)
        return;

    qDebug()<<"void ChannelIconView::mousePressEvent(QMouseEvent* event) ";
    //  if (event->button() == Qt::LeftButton && !(event->modifiers() & Qt::ShiftModifier) &&
    //   !(event->modifiers() & Qt::ControlModifier)){
    //    emit moussePressWoModificators(this->name());
    //  }

    if (event->button() == Qt::MiddleButton) {
        emit mousePressMiddleButton(item);
    }
    QListWidget::mousePressEvent(event);
}

#if PORTING_KDAB


void ChannelIconView::slotDropped(QDropEvent* event,const Q3ValueList<Q3IconDragItem>& draggedList){
    //The source of the drag is not a widget of the application
    if (event->source() == 0 || !drag){
        event->ignore();
        return;
    }

    //Drop of a label to move the whole block
    if (QString(event->format()).contains("text/plain")){
        QString information;
        if (Q3TextDrag::decode(event,information)){
            int groupSource = information.section("-",0,0).toInt();
            int start = information.section("-",1,1).toInt();
            QString groupTarget = this->name();
            emit dropLabel(groupSource,groupTarget.toInt(),start,QWidget::mapToGlobal(event->pos()).y());
            return;
        }
    }


    if (event->action() == QDropEvent::Move){
        event->acceptAction();

        QString groupSource = (event->source())->parent()->name();

        QList<int> channelIds;
        QList<Q3IconDragItem>::const_iterator iterator;
        for(iterator = draggedList.begin(); iterator != draggedList.end(); ++iterator){
            QTextCodec* codec = QTextCodec::codecForLocale();
            QByteArray data =  (*iterator).data();
            int channelId = codec->toUnicode(data).toInt();
            channelIds.append(channelId);
        }

        QList<int> selectedChannels;
        for(Q3IconViewItem* item = firstItem(); item; item = item->nextItem())
            if (item->isSelected())
                selectedChannels.append(item->text().toInt());

        //If all the channels are selected insert after the first one.
        if (selectedChannels.size() == this->count()){
            emit channelsMoved(this->name(),0);
            return;
        }

        Q3IconViewItem* after = findItemToInsertAfter(event->pos());
        emit channelsMoved(this->name(),after);
    }
}

void ChannelIconView::contentsDropEvent(QDropEvent* event){
    if (event->source() == 0 || !drag){
        event->ignore();
        return;
    }
    if ((event->source())->parent()->objectName() != objectName()){
        Q3IconView::contentsDropEvent(event);
        return;
    }

    //Move items around in the iconview
    Q3IconViewItem* item = findItem(event->pos());
    if (item == 0){
        QList<int> selectedChannels;
        for(Q3IconViewItem* item = firstItem(); item; item = item->nextItem())
            if (item->isSelected()) selectedChannels.append(item->text().toInt());

        //If all the items have been selected, do not do anything
        if (selectedChannels.size() == count()){
            Q3IconView::contentsDropEvent(event);
            arrangeItemsInGrid();
            return;
        }

        Q3IconViewItem* after = findItemToInsertAfter(event->pos());

        Q3IconView::contentsDropEvent(event);
        emit channelsMoved(selectedChannels,this->objectName(),after);
    }
    else{
        Q3IconView::contentsDropEvent(event);
        return;
    }
}

Q3IconViewItem* ChannelIconView::findItemToInsertAfter(QPoint position){
    int posX = position.x();
    int posY = position.y();
    
    int firstY = firstItem()->pos().y();
    //test if the position is above all the other items, if so find the item to insert after.
    if (posY < firstY){
        Q3IconViewItem* after = 0L;
        for(Q3IconViewItem* item = firstItem(); item; item = item->nextItem()){
            if (!item->isSelected()){
                after = item;
                if (((item->pos().y() == firstY) && (item->pos().x() >= posX)) || (item->pos().y() > firstY)){
                    if (item->index() == 0) return 0;
                    else{
                        //take the first item before the current one which is not selected (<=> not to be moved)
                        Q3IconViewItem* after = item->prevItem();
                        while(after->isSelected()){
                            if (after->index() == 0) return 0;
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
    if (posY > lastY){
        for(Q3IconViewItem* item = lastItem(); item; item = item->prevItem()){
            if (!item->isSelected()){
                if (((item->pos().y() == lastY) && (item->pos().x() <= posX)) || (item->pos().y() < lastY)){
                    if (item->index() == 0 && posX < item->pos().x()) return 0;
                    else return item;
                }
            }
        }
    }
    //else the other cases
    Q3IconViewItem* item;
    for(item = lastItem(); item;item = item->prevItem())
        if (!item->isSelected() && (item->pos().y() <= posY)) break;

    int maxY = item->pos().y();
    for(;item;item = item->prevItem()){
        if (!item->isSelected()){
            if ((item->pos().x() <= posX) || (item->pos().y() < maxY) || item == firstItem()){
                if (item->index() == 0 && posX < item->pos().x()) return 0;
                else return item;
            }
        }
    }

    //Normally never reach
    return lastItem();
}
#endif

