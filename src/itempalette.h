/***************************************************************************
                          itemPalette.h  -  description
                             -------------------
    begin                : Thur feb  26  12:06:21 EDT 2004
    copyright            : (C) 2003 by Lynn Hazan
    email                : lynn.hazan@myrealbox.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#ifndef ITEMPALETTE_H
#define ITEMPALETTE_H

//QT include files
#include <QVariant>
#include <QWidget>
#include <QHash>
#include <QMap>
#include <QLabel>
#include <QCursor>
#include <QPainter>

#include <QResizeEvent>
#include <QList>
#include <QMouseEvent>

// application specific includes
#include "itemgroupview.h"
#include "itemiconview.h"

#include <QScrollArea>
// forward declaration
class ItemColors;

/**
  * This class is used to create the cluster and event palettes of the application.
  * It receives the user selections and triggers the actions which have to be done.
  *@author Lynn Hazan
  */
class ItemPalette : public QScrollArea
{
    Q_OBJECT
    
public:

    enum PaletteType {CLUSTER=0,EVENT=1};

    /**Constructor.
                     * @param type type of palette (clusters or events).
                     * @param backgroundColor background color.
                     * @param parent parent widget.
                     * @param name internal name of the palette.
                     * @param fl widget flags.
                     */
    ItemPalette(PaletteType type,const QColor& backgroundColor,QWidget* parent = 0, const char* name = 0 );
    /*
   *  Destroys the object and frees any allocated resources.
   */
    ~ItemPalette();

    /**
    * Creates a list of the items of the group @p groupName.
    * @param itemColors list of colors for the given group.
    * @param groupName name of the group of items.
    * @param descriptionLength value to use as the length for the event descriptions.
    */
    void createItemList(ItemColors* itemColors,const QString& groupName,int descriptionLength);

    /** Selects the items specified in the map @p selection.
    * @param groupName name of the group containing the items to be selected.
    * @param itemsToSelect list of items to be selected.
    * @param itemsToSkip list of items to be marked as skiped while browsing.
    */
    void selectItems(const QString &groupName, const QList<int> &itemsToSelect, const QList<int> &itemsToSkip);

    /**Resets the internal variables.*/
    void reset();
    
    /**Returns the list of selected items by group
    * @return map given the list of selected items for a given group.*/
    const QMap<QString,QList<int> > selectedItems();

    /**Updates the background color of the palette.*/
    void changeBackgroundColor(const QColor &color);

    /**Returns the currently selected group.
    * @return the name of the selected group.
    */
    QString selectedGroup() const {return selected;}

    /** Removes a group from the palette.
    * @param groupName name of the group to be removed.
    */
    void removeGroup(const QString& groupName);

    /** Selects a group.
    * @param groupName name of the group to be selected.
    */
    void selectGroup(const QString &groupName);
    
    /**Selects all the items.*/
    void selectAllItems();

    /**Deselects all the items.*/
    void deselectAllItems();

    /** Checks if browsing on items of the palette can be down.
    * @return true if at least one item can be use for browsing, false otherwise.
    */
    bool isBrowsingEnable();
    
public Q_SLOTS:
    void slotMousePressWAltButton(const QString &sourceGroup, QListWidgetItem *item);
    
protected Q_SLOTS:
    void slotMousePressed(const QString &sourceGroupName,QListWidgetItem* item);
    void slotMousePressed(const QString &sourceGroupName, bool shiftKey = false, bool ctrlAlt = false);
    void slotClickRedraw();
    void languageChange();
    void resizeEvent(QResizeEvent* event);
    void slotMouseReleased(const QString &sourceGroupName);
    void slotRowInsered();

Q_SIGNALS:
    void colorChanged(int item, const QString &groupName, const QColor&);
    void updateShownItems(const QMap<QString,QList<int> >& selectedItems);
    void paletteResized(int parentWidth,int labelSize);
    void selectedGroupChanged(const QString &eventGroupName);
    void updateItemsToSkip(const QString &groupName,const QList<int>& itemsToSkip);
    void noClustersToBrowse();
    void noEventsToBrowse();
    void clustersToBrowse();
    void eventsToBrowse();
    
private:    

    /**Background color.*/
    QColor backgroundColor;

    /**Prevent from emitting signal while globaly selecting items*/
    bool isInSelectItems;

    QVBoxLayout* verticalContainer;

    /**Dictionnary of the iconviews representing the group of items.*/
    QHash<QString, ItemIconView*> iconviewDict;

    /**Dictionnary of layout containing the iconviews.*/
    QHash<QString, ItemGroupView*> itemGroupViewDict;

    /**Dummy widget used to keep the iconviews nicely display in the pannel.*/
    QWidget* spaceWidget;

    /**Size of the group label.*/
    int labelSize;

    /**Type of the palette usage: cluster or event.*/
    PaletteType type;
    
    /**Stores the group currently beeing selected.*/
    QString selected;

    /**List used to order the electrode groups.*/
    QList<int> clusterGroupList;
    
    /**List used to order the event groups.*/
    QStringList itemGroupList;

    /**The width for the columns of the event iconviews.*/
    int gridX;

    /**Stores the browsing status of each item. The browsing status is true if the item is used for browsing, false otherwise.*/
    QMap<QString, QMap<int,bool> > browsingStatus;

    /**Stores the items that have to be redrawn.*/
    QMap<QString, QList<int> > needRedrawing;

    /**Stores the selection status of each group. The selection status is true if all the group items have been selected by a click on the group label, false otherwise.*/
    QMap<QString,bool> selectionStatus;
    
    //Functions

    /** Changes the color of a given item.
    * @param item item for which the color has to be changed.
    * @param groupName name of the group containing the item.
    */
    void changeColor(QListWidgetItem *item, const QString &groupName);

    /**Creates a new group for the name @p id
    * @param id name of the group to be created.
    */
    void createGroup(const QString& id);

    /** Redraws the list corresponding to the group @p groupName.
    * @param groupName name of the list to update.
    */
    void updateItemList(const QString& groupName, ItemColors *itemColors);

    /**Reorders the groups to display them in either alphabetic order or numerical order.*/
    void orderTheGroups();

    /** Redraws an item after a change of his browsing status.
    * @param iconView iconview containing the item to recreate.
    * @param itemColors itemColors storing the color information for the items contained in the iconView.
    * @param index index in itemColors or in the iconview (the are identical) of the item to recreate.
    * @param browsingMap map giving the browsing status of the items contained in the iconView.
    */
    void redrawItem(ItemIconView* iconView, int index, const QMap<int, bool> &browsingMap);
    
    /**Selects the group identify by @p groupName.
    * @param groupName the group to be selected.
    */
    void selectGroupLabel(const QString& groupName);

    /** Updates the items that need redrawing
     */
    void updateIconPixmaps();
};

/**
  *Utility class used to create the group labels on the left side of the group boxes in the cluster and event palettes.
  *@author Lynn Hazan
  */
class GroupNameLabel : public QLabel{
    Q_OBJECT
public:
    inline GroupNameLabel(const QString& text,QWidget* parent):
        QLabel(text,parent){}

Q_SIGNALS:
    void leftClickOnLabel(const QString& sourceId,bool shiftKey,bool ctrlAlt);
    void middleClickOnLabel(const QString& sourceId);

protected:
    void mousePressEvent(QMouseEvent* e){
        if(e->button() == Qt::LeftButton && !(e->modifiers() & Qt::ShiftModifier) && !(e->modifiers() & Qt::ControlModifier) && !(e->modifiers() & Qt::AltModifier)){
            emit leftClickOnLabel(parent()->objectName(),false,false);
        }
        if(e->button() == Qt::LeftButton && (e->modifiers() & Qt::ShiftModifier) && !(e->modifiers() & Qt::ControlModifier) && !(e->modifiers() & Qt::AltModifier)){
            emit leftClickOnLabel(parent()->objectName(),true,false);
        }
        if(e->button() == Qt::LeftButton && (e->modifiers() & Qt::ControlModifier) && (e->modifiers() & Qt::AltModifier)){
            emit leftClickOnLabel(parent()->objectName(),false,true);
        }
        if(e->button() == Qt::MidButton){
            emit middleClickOnLabel(parent()->objectName());
        }
    }

};

#endif // CHANNELPALETTE_H
