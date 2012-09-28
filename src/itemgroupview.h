/***************************************************************************
                          itemgroupview.h  -  description
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

#ifndef ITEMGROUPVIEW_H
#define ITEMGROUPVIEW_H

#include <qwidget.h>
#include <QHBoxLayout>
#include <q3iconview.h>
#include <QObject> 
#include <QDebug>


/**Utilitary class used to build the cluster and event palettes.
  *@author Lynn Hazan
  */

class ItemGroupView : public QWidget  {
    Q_OBJECT
public: 
    inline ItemGroupView(const QColor& backgroundColor,QWidget* parent=0)
        :QWidget(parent),iconView(0L),init(true)
    {
        mLayout = new QHBoxLayout;
        mLayout->setMargin(0);
        mLayout->setSpacing(0);

        setAutoFillBackground(true);
        setLayout(mLayout);
        //Set the groupview color, the foreground color depends on the background color
        QPalette palette; palette.setColor(backgroundRole(), backgroundColor); 
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

        adjustSize();

        setAcceptDrops(TRUE);
    }

    inline ~ItemGroupView(){qDebug()<<"in ~ItemGroupView()";}

    inline void setIconView(Q3IconView* view){
        iconView = view;
        mLayout->addWidget(iconView);
    }

public Q_SLOTS:
    inline void reAdjustSize(int parentWidth,int labelSize){
        if((iconView->contentsWidth() != 1 && width() != parentWidth) || init){
            init = false;
            int futurWidth = parentWidth ;

            setFixedWidth(futurWidth);
            int viewfuturWidth = width() - labelSize - 6;//give so space on the right
            iconView->setFixedWidth(viewfuturWidth);

            if(iconView->contentsHeight() != 1 && height() != iconView->contentsHeight())
                setFixedHeight(iconView->contentsHeight());
        }
        //If items have been moved in or out of the iconview, its sized has changed and the ItemGroupView has to compensate
        if(iconView->contentsHeight() != 1 && height() != iconView->contentsHeight())
            setFixedHeight(iconView->contentsHeight());
    }


private:
    Q3IconView* iconView;
    QHBoxLayout *mLayout;
    bool init;

};

#endif
