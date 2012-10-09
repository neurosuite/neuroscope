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
#include <QObject> 
#include <QDebug>
class QListWidget;

/**Utilitary class used to build the cluster and event palettes.
  *@author Lynn Hazan
  */

class ItemGroupView : public QWidget  {
    Q_OBJECT
public: 
    explicit ItemGroupView(const QColor& backgroundColor,QWidget* parent=0);
    ~ItemGroupView();

    void setIconView(QListWidget *view);

public Q_SLOTS:
    void reAdjustSize(int parentWidth,int labelSize);

private:
    QListWidget* iconView;
    QHBoxLayout *mLayout;
    bool init;

};

#endif
