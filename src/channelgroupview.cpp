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
#include <QListWidget>

ChannelGroupView::ChannelGroupView(bool drag,const QColor& backgroundColor,QWidget* parent)
    :QWidget(parent),
      iconView(0L),
      drag(drag),
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

    palette.setColor(foregroundRole(), legendColor);
    adjustSize();

    setAcceptDrops(TRUE);
}

void ChannelGroupView::reAdjustSize(int parentWidth,int labelSize){
    if((iconView->size().width() != 1 && width() != parentWidth) || init){
        init = false;
        int futurWidth = parentWidth ;

        setFixedWidth(futurWidth);
        int viewfuturWidth = width() - labelSize - 6;//give so space on the right
        iconView->setFixedWidth(viewfuturWidth);

        if(iconView->size().height() != 1 && height() != iconView->size().height())
            setFixedHeight(iconView->size().height());

    }
    //If items have been moved in or out of the iconview, its sized has changed and the ChannelGroupView has to compensate
    if(iconView->size().height() != 1 && height() != iconView->size().height())
        setFixedHeight(iconView->size().height());
}

void ChannelGroupView::dropEvent(QDropEvent* event){
    if(event->source() == 0 || !drag){
        event->ignore();
        return;
    }


    if(event->mimeData()->hasText()){
        QString information = event->mimeData()->text();
        int groupSource = information.section("-",0,0).toInt();
        int start = information.section("-",1,1).toInt();
        QString groupTarget = this->objectName();
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

void ChannelGroupView::setIconView(QListWidget* view){
    iconView = view;
    mLayout->addWidget(iconView);
}


#include "channelgroupview.moc"
