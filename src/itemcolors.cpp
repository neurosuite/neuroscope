/***************************************************************************
                          itemcolors.cpp  -  description
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
// application specific includes
#include "itemcolors.h"

//C, C++ include files
#include <iostream>
//Added by qt3to4:
#include <QList>
#include <Q3PtrList>

using namespace std;

ItemColors::ItemColors():itemList(),colorChanged(false){
    //The list owns the objects, it will delete the items that are removed.
    itemList.setAutoDelete(true);
}

ItemColors::~ItemColors(){
}


ItemColors::ItemColors(const ItemColors& origin):colorChanged(origin.colorChanged){
    //Insert into itemList a deep copy of all the elements of origin.itemList
    ItemColor* itemColor;
    Q3PtrList<ItemColor> originItemList =  origin.itemList;

    for(itemColor = originItemList.first(); itemColor; itemColor = originItemList.next()){
        itemList.append(new ItemColor(*itemColor));
    }
}

QColor ItemColors::color(int identifier, SelectionMode mode){
    ItemColors::ItemColor* theItemColor = 0L;

    if(mode == BY_INDEX) theItemColor = itemList.at(static_cast<uint>(identifier));
    else if (mode == BY_ITEM_NUMBER) theItemColor = itemColor(identifier);

    //In case no ItemColor have been find (should not happen), return black.
    if(theItemColor == NULL) return QColor(Qt::black);
    else return theItemColor->color;
}

void ItemColors::setColor(int identifier, QColor color, SelectionMode mode){
    ItemColors::ItemColor* theItemColor = 0L;

    if(mode == BY_INDEX) theItemColor = itemList.at(static_cast<uint>(identifier));
    else if (mode == BY_ITEM_NUMBER) theItemColor = itemColor(identifier);

    theItemColor->color = color;
}

int ItemColors::itemId(int index){
    return (itemList.at(static_cast<uint>(index)))->itemId;
}

QString ItemColors::itemLabel(int index){
    return (itemList.at(static_cast<uint>(index)))->label;
}

QString ItemColors::itemLabelById(int id){
    if(itemColorIndex(id) == -1) return "";
    else return (itemList.at(static_cast<uint>(itemColorIndex(id))))->label;
}


bool ItemColors::contains(int itemId){
    if(itemColorIndex(itemId) == -1) return false;
    else return true;
}


bool ItemColors::isColorChanged(int identifier, SelectionMode mode){
    ItemColors::ItemColor* theItemColor = 0L;

    if(mode == BY_INDEX) theItemColor = itemList.at(static_cast<uint>(identifier));
    else if (mode == BY_ITEM_NUMBER) theItemColor = itemColor(identifier);

    return theItemColor->isChanged;
}

void ItemColors::setColorChanged(int identifier, bool changed, SelectionMode mode){
    ItemColors::ItemColor* theItemColor = 0L;

    if(mode == BY_INDEX) theItemColor = itemList.at(static_cast<uint>(identifier));
    else if (mode == BY_ITEM_NUMBER) theItemColor = itemColor(identifier);

    theItemColor->isChanged = changed;
    //Update colorChanged if necessary (the change status is true and colorChanged is not already true)
    if(changed && !colorChanged) colorChanged = true;
}

uint ItemColors::append(int itemId, QColor color){
    itemList.append(new ItemColor(itemId,color));
    return itemList.count();
}

uint ItemColors::append(int itemId,QString label,QColor color){
    itemList.append(new ItemColor(itemId,color,label));
    return itemList.count();
}


void ItemColors::insert(int itemId, QColor color,int index){
    itemList.insert(index, new ItemColor(itemId,color));
}

void ItemColors::insert(int itemId,QString label,QColor color,int index){
    itemList.insert(index, new ItemColor(itemId,color,label));
}

bool ItemColors::remove(int identifier, SelectionMode mode){
    if(mode == BY_INDEX) return itemList.remove(identifier);
    else if(mode == BY_ITEM_NUMBER){
        return itemList.remove(itemColorIndex(identifier));
    }
    else return false;//unknow mode
}

ItemColors::ItemColor* ItemColors::itemColor(int itemId) const{

    //Iterate on the list until the item is find
    Q3PtrListIterator<ItemColors::ItemColor> iterator(itemList);
    ItemColors::ItemColor* itemColor = 0L;
    while((itemColor = iterator.current()) != 0) {
        ++iterator;
        if (itemColor->itemId == itemId) return itemColor;
    }
    return NULL;//Normally never reached
}

int ItemColors::itemColorIndex(int itemId) const{
    //Iterate on the list until the item is find
    Q3PtrListIterator<ItemColors::ItemColor> iterator(itemList);
    ItemColors::ItemColor* itemColor;
    int index = 0;
    while((itemColor = iterator.current()) != 0) {
        if (itemColor->itemId == itemId) return index;
        ++index;
        ++iterator;
    }
    return -1;//Normally never reach
}

QList<int> ItemColors::colorChangedItemList(){
    QList<int> changedList;
    ItemColor* itemColor;
    for (itemColor = itemList.first(); itemColor; itemColor = itemList.next()){
        if(itemColor->isChanged) changedList.append(itemColor->itemId);
    }
    return changedList;
}

void ItemColors::resetAllColorStatus(){
    setColorChanged(false);

    QList<int> changedList;
    ItemColor* itemColor;
    for (itemColor = itemList.first(); itemColor; itemColor = itemList.next()){
        if(itemColor->isChanged) itemColor->isChanged = false;
    }
}

void ItemColors::changeItemId(int index, int newItemId){
    ItemColors::ItemColor* theItemColor = itemList.at(static_cast<uint>(index));

    theItemColor->itemId = newItemId;
}

void ItemColors::changeItemLabel(int index,QString newItemLabel){
    ItemColors::ItemColor* theItemColor = itemList.at(static_cast<uint>(index));

    theItemColor->label = newItemLabel;
}



