/***************************************************************************
                          itemgroupview.cpp  -  description
                             -------------------
    begin                : Thu Mar 4 2004
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

#include "itemgroupview.h"
#include "itemiconview.h"
#include <QListWidget>
#include <QLabel>

ItemGroupView::ItemGroupView(const QColor& backgroundColor,QWidget* parent)
    :QWidget(parent),
      iconView(0L),
      mLabel(0),
      init(true)
{
    mLayout = new QHBoxLayout;
    mLayout->setMargin(0);
    mLayout->setSpacing(0);

    setAutoFillBackground(true);
    setLayout(mLayout);
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
    setPalette(palette);

    setAcceptDrops(true);
}

ItemGroupView::~ItemGroupView()
{
    qDebug()<<"in ~ItemGroupView()";
}

void ItemGroupView::setIconView(ItemIconView *view){
    iconView = view;
    mLayout->addWidget(iconView);
}

void ItemGroupView::reAdjustSize(int parentWidth,int labelSize){
    if ((iconView->size().width() != 1 && width() != parentWidth) || init) {
        init = false;
        int futurWidth = parentWidth -10;
        if (futurWidth<0)
            return;
        setFixedWidth(futurWidth);
        int viewfuturWidth = parentWidth - labelSize - 16;//give so space on the right
        if(viewfuturWidth < 0)
            return;

        iconView->setNewWidth(viewfuturWidth);

        /*if(iconView->size().height() != 1 && height() != iconView->size().height()) {
            qDebug() << "setFixedHeight" << iconView->size().height();
            setFixedHeight(iconView->size().height());
        }*/
    }

     int iconHeight = iconView->sizeHint().height() + 5;

    //If items have been moved in or out of the iconview, its sized has changed and the ItemGroupView has to compensate
    if (iconHeight != 1 && height() != iconHeight) {
        setFixedHeight(iconHeight);
    }
}

void ItemGroupView::setLabel(QLabel* label){
    mLabel = label;
    mLabel->setAutoFillBackground(true);
    mLayout->addWidget(mLabel);
}

QLabel* ItemGroupView::label() const{
    return mLabel;
}


