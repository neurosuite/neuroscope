/***************************************************************************
                          channelgroupview.cpp  -  description
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

#include "channelgroupview.h"
#include "channeliconview.h"
#include <QListWidget>
#include <QLabel>
#include <QMimeData>

ChannelGroupView::ChannelGroupView(bool drag,const QColor& backgroundColor,QWidget* parent)
    :QWidget(parent),
      iconView(0L),
      drag(drag),
      mLabel(0),
      init(true)
{

    mLayout = new QHBoxLayout;
    mLayout->setMargin(0);
    mLayout->setSpacing(0);
    setLayout(mLayout);
    setAutoFillBackground(true);

    //Set the groupview color, the foreground color depends on the background color
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
    adjustSize();

    setAcceptDrops(true);
}

void ChannelGroupView::reAdjustSize(int parentWidth,int labelSize)
{
    if((iconView->size().width() != 1 && width() != parentWidth) || init){
        init = false;
        int futurWidth = parentWidth ;

        setFixedWidth(futurWidth-10);
        int viewfuturWidth = width() - labelSize - 6;//give so space on the right
        iconView->setNewWidth(viewfuturWidth);

        /*
        if(iconView->size().height() != 1 && height() != iconView->size().height())
            setFixedHeight(iconView->size().height());
            */

    }
    int iconHeight = iconView->sizeHint().height()+5;
    if (iconHeight != 1 && height() != iconHeight) {
        setFixedHeight(iconHeight);
    }
}

void ChannelGroupView::dropEvent(QDropEvent* event)
{
    if(event->source() == 0 || !drag){
        event->ignore();
        return;
    }
    if(event->mimeData()->hasText()){
        QString information = event->mimeData()->text();
        const int groupSource = information.section("-",0,0).toInt();
        const int start = information.section("-",1,1).toInt();
        const QString groupTarget = this->objectName();
        emit dropLabel(groupSource,groupTarget.toInt(),start,QWidget::mapToGlobal(event->pos()).y());
    }
}

void ChannelGroupView::dragEnterEvent(QDragEnterEvent* event){
    if(event->source() == 0 || !drag){
        event->ignore();
        return;
    }

    if (event->mimeData()->hasText())
        event->acceptProposedAction();
    //Enable the parent (ChannelPalette) to ensure that the current group is visible (will scroll if need it)
    emit dragObjectMoved(QWidget::mapToParent(event->pos()));
}

void ChannelGroupView::setIconView(ChannelIconView *view){
    iconView = view;
    iconView->viewport()->setAutoFillBackground(false);
    mLayout->addWidget(iconView);
}

void ChannelGroupView::setLabel(QLabel* label){
    mLabel = label;
    mLayout->addWidget(mLabel);
}

QLabel* ChannelGroupView::label(){
    return mLabel;
}
