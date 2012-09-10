/***************************************************************************
                          itemiconview.h  -  description
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

#ifndef ITEMICONVIEW_H
#define ITEMICONVIEW_H

//QT include files
#include <qwidget.h>
#include <q3iconview.h>
#include <q3dragobject.h>
#include <qwidget.h>
//Added by qt3to4:
#include <QMouseEvent>
#include <QWheelEvent>


/**Utilitary class used to build the cluster and event palettes.
  *@author Lynn Hazan
  */

class ItemIconView : public Q3IconView  {
    Q_OBJECT
public:
    ItemIconView(const QColor &backgroundColor, Q3IconView::ItemTextPos position, int gridX, int gridY, QWidget* parent = 0, const char* name = 0, Qt::WFlags f = 0);
    inline ~ItemIconView(){}

private slots:
    inline void slotMousePressed(int button,Q3IconViewItem* item){
        emit mouseButtonPressed(button,item,this->name());
    }
    inline void contentsMouseReleaseEvent(QMouseEvent* event){
        Q3IconView::contentsMouseReleaseEvent(event);
        emit mouseReleased(this->name());
    }


protected:
    void contentsMousePressEvent(QMouseEvent* event);
    inline void contentsWheelEvent(QWheelEvent* event){event->accept();}

signals:
    void mousePressWoModificators(QString sourceGroup);
    void mouseButtonPressed(int,Q3IconViewItem*,QString sourceGroup);
    void mousePressWAltButton(QString sourceGroup,int index);
    void mouseButtonClicked(int,Q3IconViewItem*,QString sourceGroup);
    void mouseReleased(QString sourceGroup);

};

#endif
