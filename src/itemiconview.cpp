/***************************************************************************
                          itemiconview.cpp  -  description
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
#include "itemiconview.h"
// include files for Qt
#include <QCursor>
#include <QTextCodec>

#include <QMouseEvent>
#include <QDebug>
#include <QFrame>

ItemWidgetItem::ItemWidgetItem(const QIcon &icon, const QString &text, QListWidget *view, int type)
    : QListWidgetItem(icon, text, view, type)
{

}

bool ItemWidgetItem::operator<(const QListWidgetItem &other) const
{
    bool v1Ok;
    bool v2Ok;
    const int v1 = data(Qt::DisplayRole).toInt(&v1Ok);
    const int v2 = other.data(Qt::DisplayRole).toInt(&v2Ok);
    if (v1Ok && v2Ok) {
        return v1 < v2;
    } else {
       return QListWidgetItem::operator <(other);
    }
}


ItemIconView::ItemIconView(const QColor& backgroundColor,QListView::ViewMode mode,int gridX,int gridY,QWidget* parent, const QString& name):
    QListWidget(parent)
{
    setObjectName(name);
    QFont font( "Helvetica",8);
    setFont(font);
    setFrameStyle(QFrame::NoFrame);
    setResizeMode(QListWidget::Adjust);
    setViewMode(mode);
    if (mode == QListView::IconMode)
        setGridSize(QSize(gridX,gridY));
    setWordWrap(false);
    setAutoFillBackground(true);
    viewport()->setAutoFillBackground(false);
    setSelectionRectVisible (false);
    //Set the iconView color, the foreground color depends on the background color
    QPalette palette;
    palette.setColor(backgroundRole(), backgroundColor);
    int h;
    int s;
    int v;
    backgroundColor.getHsv(&h,&s,&v);
    QColor legendColor;
    if(s <= 80 && v >= 240 || (s <= 40 && v >= 220))
        legendColor = Qt::black;
    else
        legendColor = Qt::white;
    palette.setColor(QPalette::Text, legendColor);
    palette.setColor(QPalette::HighlightedText, legendColor);

    setPalette(palette);
    setSelectionMode(QAbstractItemView::ExtendedSelection);

    setMovement(QListView::Static);

    setSpacing(4);

    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    setFrameStyle(QFrame::Box | QFrame::Plain);
    setLineWidth(1);
    setSortingEnabled(true);
    connect(model(), SIGNAL(rowsInserted(QModelIndex, int,int)), this, SIGNAL(rowInsered()));
}

void ItemIconView::wheelEvent ( QWheelEvent * event )
{
    event->accept();
}

void ItemIconView::mousePressEvent ( QMouseEvent * event )
{
    QListWidgetItem *item = itemAt(event->pos());
    if(!item)
        return;
    if(event->button() == Qt::LeftButton && (event->modifiers() & Qt::AltModifier) && (event->modifiers() & Qt::ControlModifier)){
        emit mousePressWAltButton(this->objectName(),item);
        return;
    } else if(event->button() == Qt::MiddleButton) {
        emit mousePressMiddleButton(this->objectName(),item);
        emit mousePressMiddleButton(item);
        return;
    }
    QListWidget::mousePressEvent(event);
}

void ItemIconView::mouseMoveEvent(QMouseEvent *event)
{
    // No rectangular selection please
    return;
}

void ItemIconView::mouseReleaseEvent ( QMouseEvent * event ) {
    QListWidget::mouseReleaseEvent(event);
    emit mouseReleased(this->objectName());
}

void ItemIconView::setNewWidth(int width)
{
    setFixedWidth(width);
    doItemsLayout();
    resize(sizeHint());
}

QSize ItemIconView::sizeHint() const
{
    const int height = rectForIndex(model()->index(model()->rowCount() - 1, 0)).bottom() + 5;
    return QSize(width(), height);
}

void ItemIconView::keyPressEvent(QKeyEvent *event)
{
    const bool hasShiftPressed = event->modifiers() & Qt::ShiftModifier;
    if (event->key() == Qt::Key_Right) {
        QListWidgetItem *c = currentItem();
        if (c) {
            const int i = row(c);
            if (i < count()-1) {
                if (hasShiftPressed) {
                    QListWidgetItem *nextItem = item(i+1);
                    if(nextItem->isSelected()) {
                        c->setSelected(false);
                    } else {
                        c->setSelected(true);
                        nextItem->setSelected(true);
                    }
                    setCurrentItem(nextItem);
                } else {
                    clearSelection();
                    setCurrentRow(i+1);
                }
            }
        }
    } else if (event->key() == Qt::Key_Left) {
        QListWidgetItem *c = currentItem();
        if (c) {
            const int i = row(c);
            if (i > 0) {
                if (hasShiftPressed) {
                    QListWidgetItem *nextItem = item(i-1);
                    if(nextItem->isSelected()) {
                        c->setSelected(false);
                    } else {
                        c->setSelected(true);
                        nextItem->setSelected(true);
                    }
                    setCurrentItem(nextItem);
                } else {
                    clearSelection();
                    setCurrentRow(i-1);
                }
            }
        }
    } else {
        QListWidget::keyPressEvent(event);
    }
}
