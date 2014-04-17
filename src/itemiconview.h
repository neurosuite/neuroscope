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
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef ITEMICONVIEW_H
#define ITEMICONVIEW_H

//QT include files
#include <QWidget>
#include <QListWidget>
#include <QWidget>

#include <QMouseEvent>
#include <QWheelEvent>


/**Utilitary class used to build the cluster and event palettes.
  *@author Lynn Hazan
  */
class ItemWidgetItem : public QListWidgetItem {
public:
    explicit ItemWidgetItem(const QIcon &icon, const QString &text, QListWidget *view = 0, int type = Type);
    virtual bool operator<(const QListWidgetItem &other) const;
};

class ItemIconView : public QListWidget {
    Q_OBJECT
public:
    explicit ItemIconView(const QColor &backgroundColor, ViewMode mode, int gridX, int gridY, QWidget* parent = 0, const QString& name = QString());
    ~ItemIconView(){}

    enum IndexItem {
        INDEXICON = Qt::UserRole + 1,
        Color = Qt::UserRole + 2
    };

    void setNewWidth(int width);

    QSize sizeHint() const;


protected:
    void mousePressEvent ( QMouseEvent * event );
    void mouseMoveEvent ( QMouseEvent * event );
    void wheelEvent ( QWheelEvent * e );
    void mouseReleaseEvent ( QMouseEvent * event );
    void keyPressEvent(QKeyEvent *event);

Q_SIGNALS:
    void mousePressMiddleButton(const QString& sourceGroup,QListWidgetItem*);
    void mousePressWAltButton(const QString &sourceGroup, QListWidgetItem*);
    void mouseReleased(const QString &sourceGroup);
    void mousePressMiddleButton(QListWidgetItem*);
    void rowInsered();

};

#endif
