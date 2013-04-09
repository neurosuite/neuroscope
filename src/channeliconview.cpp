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

#include <QDropEvent>
#include <QFrame>
#include <QList>
#include <QMouseEvent>
#include <QDebug>


ChannelIconView::ChannelIconView(const QColor& backgroundColor, int gridX, int gridY, bool edit, QWidget* parent, const QString& name):
    QListWidget(parent){
    setObjectName(name);
    QFont font( "Helvetica",8);
    setFont(font);
    setSpacing(4);
    setFrameStyle(QFrame::Box | QFrame::Plain);
    setLineWidth(1);
    setResizeMode(QListWidget::Adjust);
    setGridSize(QSize(gridX, gridY));
    setViewMode(QListView::IconMode);

    //arrangeItemsInGrid();

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

    setSelectionMode(QAbstractItemView::MultiSelection);

    if (edit) {
        setDragEnabled(true);
        setMovement(QListView::Snap);
    } else {
        setDragEnabled(false);
        setMovement(QListView::Static);
    }
    setSelectionRectVisible(false);
    setSpacing(4);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

}

void ChannelIconView::mousePressEvent(QMouseEvent* event)
{
    //If the user did not clicked on an item, ignore the click
    QListWidgetItem* item = itemAt(event->pos());
    if (item == 0L)
        return;

    //  if (event->button() == Qt::LeftButton && !(event->modifiers() & Qt::ShiftModifier) &&
    //   !(event->modifiers() & Qt::ControlModifier)){
    //    emit moussePressWoModificators(this->name());
    //  }

    if (event->button() == Qt::MiddleButton) {
        emit mousePressMiddleButton(item);
    }
    QListWidget::mousePressEvent(event);
}

void ChannelIconView::wheelEvent ( QWheelEvent * event )
{
    event->accept();
}

void ChannelIconView::setDragAndDrop(bool dragDrop)
{
    setDragEnabled(dragDrop);
}

void ChannelIconView::dragEnterEvent(QDragEnterEvent *event)
{
    QListWidget::dragEnterEvent(event);
}

void ChannelIconView::dragMoveEvent(QDragMoveEvent *event)
{
    QListWidget::dragMoveEvent(event);
}
#if 0

void ChannelIconView::dropEvent(QDropEvent *event)
{
    const QMimeData *mimeData = event->mimeData();
    if (mimeData->hasText()) {
        QString information = mimeData->text();
        const int groupSource = information.section("-",0,0).toInt();
        const int start = information.section("-",1,1).toInt();
        const QString groupTarget = objectName();
        emit dropLabel(groupSource,groupTarget.toInt(),start,QWidget::mapToGlobal(event->pos()).y());
        event->acceptProposedAction();
        return;
    }
    QListWidget::dropEvent(event);
}

void ChannelIconView::startDrag(Qt::DropActions /*supportedActions*/)
{
    QListWidgetItem *item = currentItem();
    if (!item)
        return;

#if 0
    QMimeData *mimeData = new QMimeData;
    mimeData->setData("image/x-icon-view", itemData);

    QDrag *drag = new QDrag(this);
    drag->setMimeData(mimeData);
    drag->setHotSpot(QPoint(pixmap.width()/2, pixmap.height()/2));
    drag->setPixmap(pixmap);

    if (drag->exec(Qt::MoveAction) == Qt::MoveAction)
        delete takeItem(row(item));
#endif

}

#if PORTING_KDAB

Q3DragObject* ChannelIconView::dragObject(){
    if (!currentItem() || !drag)
        return 0;

    Q3IconDrag* drag = new Q3IconDrag(viewport());
    drag->setPixmap(*currentItem()->pixmap(),
                    QPoint(currentItem()->pixmapRect().width() / 2, currentItem()->pixmapRect().height() / 2));

    QPoint orig = viewportToContents( viewport()->mapFromGlobal(QCursor::pos()));
    //Insert only one item to have a pixmap to draw.
    for(Q3IconViewItem* item = firstItem();item; item = item->nextItem()){
        if (item->isSelected()){
            Q3IconDragItem id;
            id.setData(item->text().toLocal8Bit());
            drag->append(id,
                         QRect(item->pixmapRect(false).x() - orig.x(),
                               item->pixmapRect(false).y() - orig.y(),
                               item->pixmapRect().width(),item->pixmapRect().height()),
                         QRect(item->textRect(false).x() - orig.x(),
                               item->textRect(false).y() - orig.y(),
                               item->textRect().width(),item->textRect().height()));
            break;
        }
    }

    return drag;
}

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
            if (item->isSelected()) selectedChannels.append(item->text().toInt());

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
#endif
