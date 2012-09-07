/***************************************************************************
                          itemcolors.h  -  description
                             -------------------
    begin                : Tue Sep 16 2004
    copyright            : (C) 2003 by Lynn Hazan
    email                : lynn.hazan@myrealbox.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef ITEMCOLORS_H
#define ITEMCOLORS_H

// include files for Qt
#include <q3ptrlist.h>
#include <QColor>
#include <QList>


/**
  * This class represents the list of the items with their associated id, color
  * and color status (i.e the color has been changed or not).
  * The list index is zero based.
  *@author Lynn Hazan
  */

class ItemColors  {

public:

    enum SelectionMode {BY_INDEX=0,BY_ITEM_NUMBER=1};

    ItemColors();
    virtual ~ItemColors();
    ItemColors(const ItemColors& itemcolors);

private:

    /**Structure representing a color item.*/
    struct ItemColor{
        int itemId;
        QColor color;
        bool isChanged;
        QString label;

        ItemColor(int id, const QColor& c,const QString& l = QString()){
            itemId = id;
            color = c;
            isChanged = false;
            if(l.isEmpty())
                label = QString::fromLatin1("%1").arg(id);
            else
                label = l;
        }
        ItemColor(const ItemColor& origin):itemId(origin.itemId),
            color(origin.color),isChanged(origin.isChanged),label(origin.label){}
        ~ItemColor(){}
    };

    /**list of the ItemColor contained in the class.*/
    Q3PtrList<ItemColor> itemList;

    /**True if at least one color has changed, false otherwise.*/
    bool colorChanged;

    /**
  * Looks up for the item id @p itemId.
  * @param itemId the item id
  * @return the itemColor corresponding to the item id or null if not found
  */
    virtual ItemColor* itemColor(int itemId) const;

    /**
  * Looks up for the item index of the item id @p itemId.
  * @param itemId the item id
  * @return the index in the list corresponding to the item id or -1 if not found
  */
    virtual int itemColorIndex(int itemId) const;

public:

    /**
  * Suppress all the elements of
  */
    inline void removeAll(){itemList.clear();}

    /**
  * Returns the color for a item with a given id or position in the list (@p identifier).
  * @param identifier depending on the mode the index in the list of items or the item id.
  * @param mode the way of looking up for the color.
  * @return the QColor for the given item.
  */
    virtual QColor color(int identifier, SelectionMode mode = BY_ITEM_NUMBER);

    /**
  * Sets the color for a item with a given id or position in the list (@p identifier) to color.
  * @param identifier identifier depending on the mode the index in the list of items or the item id.
  * @param mode the way of looking up for the item.
  * @param color color to attribute to the item.
  */
    virtual void setColor(int identifier, QColor color, SelectionMode mode = BY_ITEM_NUMBER);

    /**
  * Returns the item id corresponding to a given position in the list (@p index).
  * @param index the index in the list of items.
  * @return the item id.
  */
    virtual int itemId(int index);

    /**
  * Looks up for the item index of the item id @p itemId.
  * @param itemId the item id
  * @return the index in the list corresponding to the item id or -1 if not found
  */
    inline int itemIndex(int itemId) const{return itemColorIndex(itemId);}

    /**
  * Returns the item label corresponding to a given position in the list (@p index).
  * @param index the index in the list of items.
  * @return the item label.
  */
    virtual QString itemLabel(int index);

    /**
  * Returns the item label corresponding to a given item id (@p id).
  * @param id the id of the item to be look up.
  * @return the item label.
  */
    virtual QString itemLabelById(int id);

    /**
  * Returns true if the item Id exists false otherwise.
  * @param itemId the id of the item to check for existence.
  * @return the boolean value for the existance of the item.
  */
    virtual bool contains(int itemId);

    /**
  * Returns true if the color for a item with a given id or position in the list (@p identifier)
  * has been changed, otherwise returns false.
  * @param identifier depending on the mode the index in the list of items or the item id.
  * @param mode the way of looking up for the color.
  * @return the color status for the given item.
  */
    virtual bool isColorChanged(int identifier, SelectionMode mode = BY_ITEM_NUMBER);

    /**
  * If changed is true, the color for a item with a given id or position in the list (@p identifier)
  * is said to have changed, otherwise is said not to have changed.
  * @param identifier depending on the mode the index in the list of items or the item id.
  * @param mode the way of looking up for the color.
  * @param changed color status.
  */
    virtual void setColorChanged(int identifier, bool changed, SelectionMode mode = BY_ITEM_NUMBER);

    /**
  * Returns the number of items.
  * @return the number of items in the list,
  */
    virtual inline uint numberOfItems() const {return itemList.count();}

    /**
  * Returns true if at least the color of one item have changed, otherwise returns false.
  * @return the color status for the list of items.
  */
    virtual inline bool isColorChanged()const{return colorChanged;}

    /**
  * Sets the color status for the entire list of items.
  * @param changed color status.
  */
    virtual inline void setColorChanged(bool changed){colorChanged = changed;}


    /**
  * Appends a item to the list of items, the label is set to the item id and the color status is set to false.
  * @param itemId the item id.
  * @param color the color of the item.
  * @return the index in the list.
  */
    virtual uint append(int itemId, QColor color);

    /**
  * Appends a item to the list of items, the color status is set to false.
  * @param itemId the item id.
  * @param label the item label.
  * @param color the color of the item.
  * @return the index in the list.
  */
    virtual uint append(int itemId,QString label,QColor color);

    /**
  * Inserts a item at position @p index in the list of items, the label is set to the item id and the color status is set to false.
  * @param itemId the item id.
  * @param index index position where to insert the item.
  * @param color the color of the item.
  */
    virtual void insert(int itemId, QColor color,int index);

    /**
  * Inserts a item at position @p index in the list of items, the color status is set to false.
  * @param itemId the item id.
  * @param label the item label.
  * @param index index position where to insert the item.
  * @param color the color of the item.
  */
    virtual void insert(int itemId,QString label, QColor color,int index);


    /**
  * Removes a item, with a given id or position in the list (@p identifier),from the list of items.
  * @param identifier depending on the mode the index in the list of items or the item id.
  * @param mode the way of looking up for the item,
  * @return true if successful,i.e. if identifier is in range, otherwise returns false.
  */
    virtual bool remove(int identifier, SelectionMode mode = BY_ITEM_NUMBER);

    /**
  * Returns the list of item ids for which the color has been changed since.
  * the last reset of their status.
  * @return item ids list.
  */
    virtual QList<int> colorChangedItemList();

    /**
  * Resets the status color of the object to false.
  * and do the same for all the items.
  */
    virtual void resetAllColorStatus();

    /**
  * Changes the itemId of a given element in the list.
  * @param index position of the item in the list.
  * @param newItemId the new id to assign.
  */
    virtual void changeItemId(int index, int newItemId);

    /**
  * Changes the item label of a given element in the list.
  * @param index position of the item in the list.
  * @param newItemLabel the new label to assign.
  */
    virtual void changeItemLabel(int index,QString newItemLabel);

};

#endif
