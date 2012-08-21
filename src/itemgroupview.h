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
#include <q3hbox.h>
#include <q3iconview.h>
#include <QObject> 
#include <QDebug>

//General C++ include files
#include <iostream>
using namespace std;

/**Utilitary class used to build the cluster and event palettes.
  *@author Lynn Hazan
  */

class ItemGroupView : public Q3HBox  {
   Q_OBJECT
public: 
	inline ItemGroupView(QColor backgroundColor,QWidget* parent=0, const char* name=0):Q3HBox(parent,name),iconView(0L),init(true){

  //Set the groupview color, the foreground color depends on the background color
    setPaletteBackgroundColor(backgroundColor);
    int h;
    int s;
    int v;
    backgroundColor.hsv(&h,&s,&v);
    QColor legendColor;
    if(s <= 80 && v >= 240 || (s <= 40 && v >= 220)) legendColor = Qt::black;
    else legendColor = Qt::white;
    setPaletteForegroundColor(legendColor);

    setMargin(0);
    setSpacing(0);
    adjustSize();

    setAcceptDrops(TRUE);
  };

  inline ~ItemGroupView(){qDebug()<<"in ~ItemGroupView()"<<endl;};

  inline void setIconView(Q3IconView* view){
   iconView = view;
  };

public slots:
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
  };


 private:
  Q3IconView* iconView;

  bool init;

};

#endif
