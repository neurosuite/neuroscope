/***************************************************************************
                          itemPalette.cpp  -  description
                             -------------------
    begin                : Thur feb  26 12:06:21 EDT 2004
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
#include "itempalette.h"
#include "itemcolors.h"


// include files for Qt
#include <QVariant>
#include <QPainter>
#include <QLayout>
#include <QToolTip>

#include <QVector>
#include <QPixmap>
#include <QBitmap>
#include <q3scrollview.h>
#include <QLayout> 
#include <QStyle>
#include <QColorDialog>
//Added by qt3to4:
#include <QVector>
#include <QResizeEvent>
#include <QList>
#include <QLabel>
#include <Q3ListBox>
#include <Q3ScrollView>
ItemPalette::ItemPalette(PaletteType type, const QColor &backgroundColor, QWidget* parent, const char* name)
    : Q3ScrollView(parent),backgroundColor(backgroundColor),isInSelectItems(false),
      spaceWidget(0L),type(type),selected(""),updateIconPixmap(false)
{
    setObjectName(name);

    setAutoFillBackground(true);
    //Set the palette color
    QPalette palette; 
    palette.setColor(backgroundRole(), backgroundColor);
    //Set the palette color, the foreground color depends on the background color
    int h;
    int s;
    int v;
    backgroundColor.getHsv(&h,&s,&v);
    QColor legendColor;
    if(s <= 80 && v >= 240 || (s <= 40 && v >= 220)) legendColor = Qt::black;
    else legendColor = Qt::white;
    palette.setColor(foregroundRole(), legendColor);
    setPalette(palette);
    setHScrollBarMode(Q3ScrollView::AlwaysOff);

    setResizePolicy(Q3ScrollView::AutoOneFit);
    verticalContainer = new Q3VBox(viewport(),"verticalContainer");
    addChild(verticalContainer);
    verticalContainer->setSpacing(5);

    QFont f("Helvetica",8);
    QFontInfo fontInfo = QFontInfo(f);
    if(type == CLUSTER)
        labelSize = fontInfo.pixelSize() * 2;
    else
        labelSize = fontInfo.pixelSize() * 3;

    //Set the legend in the good language
    languageChange();

    verticalContainer->adjustSize();
    adjustSize();
}    


ItemPalette::~ItemPalette()
{
    // no need to delete child widgets, Qt does it all for us
    qDeleteAll(itemGroupViewDict);
    itemGroupViewDict.clear();
}


void ItemPalette::paintEvent ( QPaintEvent*){
    if(updateIconPixmap){
        QMap<QString,QList<int> > selected = selectedItems();
        //update the icons if need it
        QMap<QString,QList<int> >::Iterator it;
        for(it = needRedrawing.begin(); it != needRedrawing.end(); ++it){
            QList<int> items = it.data();
            QString  groupName = it.key();
            QMap<int,bool> browsingMap = browsingStatus[groupName];
            ItemIconView* iconView = iconviewDict[groupName];
            ItemColors* itemColors = itemColorsDict[groupName];

            QList<int> selectedItems = selected[groupName];

            //redraw the items which have been modified
            QList<int>::iterator iterator;
            for(iterator = items.begin(); iterator != items.end(); ++iterator){
                redrawItem(iconView,itemColors,*iterator,browsingMap);
            }

            //In order to avoid problems when double clicking, all the icons of the iconview are redrawn
            if(items.size() == 0 && selectedItems.size() == 1){
                for(int i = 0; i < iconView->count();++i){
                    redrawItem(iconView,itemColors,i,browsingMap);
                }
            }

            //    QValueList<int>::iterator iterator;
            //    for(iterator = items.begin(); iterator != items.end(); ++iterator){
            //     redrawItem(iconView,itemColors,*iterator,browsingMap);
            //    }
        }

        needRedrawing.clear();
        updateIconPixmap = false;
    }
    emit paletteResized(viewport()->width(),labelSize);
}

void ItemPalette::resizeEvent(QResizeEvent* event){
    //Make the viewport to have the visible size (size of the scrollview)
    viewport()->resize(event->size());
    Q3ScrollView::resizeEvent(event);
}

void ItemPalette::createItemList(ItemColors* itemColors,QString groupName,int descriptionLength){
    //Compute gridX used for the event palette where the text is next to the icon (14px)
    QFontInfo fontInfo = QFontInfo(QFont());
    gridX = descriptionLength * fontInfo.pixelSize() + 15;

    itemColorsDict.insert(groupName,itemColors);
    //In the case of cluster files, the groupName (<=> electrode id) correspond to a number and the groups are
    //order numerically
    if(type == CLUSTER)
        clusterGroupList.append(groupName.toInt());
    else
        itemGroupList.append(groupName);
    createGroup(groupName);

    updateItemList(groupName);

    //always select a group
    if(selected.isEmpty())
        selectGroupLabel(groupName);

    emit paletteResized(viewport()->width(),labelSize);
    update();
}


void ItemPalette::updateItemList(const QString& groupName){
    ItemIconView* iconView = iconviewDict[groupName];
    iconView->clear();

    QMap<int,bool> browsingMap = browsingStatus[groupName];
    browsingMap.clear();
    selectionStatus.insert(groupName,false);

    //Get the list of items with their color
    ItemColors* itemColors = itemColorsDict[groupName];
    int nbItems = itemColors->numberOfItems();

    //Construct one icon for each item
    QPainter painter;

    for(int i = 0; i<nbItems; ++i){
        browsingMap.insert(i,false);
        QPixmap pix(14,14);
        pix.fill(backgroundColor);
        painter.begin(&pix);
        painter.fillRect(0,0,12,12,itemColors->color(i,ItemColors::BY_INDEX));
        painter.end();
        QIcon icon;
        icon.addPixmap(pix);
        QListWidgetItem *item  = new QListWidgetItem(icon,itemColors->itemLabel(i), iconView);
        item->setData(ItemIconView::INDEXICON,i);
    }

    browsingStatus.insert(groupName,browsingMap);
#ifdef KDAB_PENDING
    if(nbItems == 0)
        iconView->resizeContents(50,20);
    else
        iconView->adjustSize();
#endif
}


void ItemPalette::slotMousePressed(const QString&sourceGroupName,QListWidgetItem*item){
    if (!item) {
        return; //pressed on viewport
    } else {
        changeColor(item,sourceGroupName);
    }
}

void ItemPalette::selectGroupLabel(const QString &sourceGroupName){
    if(!selected.isEmpty()){
        ItemGroupView* previousSelectedGroup = itemGroupViewDict[selected];
        if(previousSelectedGroup != 0){
            GroupNameLabel* previousLabel = static_cast<GroupNameLabel*>(previousSelectedGroup->child("label"));
            previousLabel->setPaletteBackgroundColor(colorGroup().background());
        }
    }
    if(!sourceGroupName.isEmpty()){
        ItemGroupView* group = itemGroupViewDict[sourceGroupName];
        QLabel* label = static_cast<QLabel*>(group->child("label"));
        label->setPaletteBackgroundColor(colorGroup().highlight());
    }

    selected = sourceGroupName;
    if(type == EVENT)
        emit selectedGroupChanged(selected);
}

void ItemPalette::slotMousePressed(QString sourceGroupName,bool shiftKey,bool ctrlAlt){
#ifdef KDAB_PORTING
    if(!selected.isEmpty()){
        ItemGroupView* previousSelectedGroup = itemGroupViewDict[selected];
        if(previousSelectedGroup != 0){
            GroupNameLabel* previousLabel = static_cast<GroupNameLabel*>(previousSelectedGroup->child("label"));
            previousLabel->setPaletteBackgroundColor(colorGroup().background());
        }
    }
    if(!sourceGroupName.isEmpty()){
        ItemGroupView* group = itemGroupViewDict[sourceGroupName];
        QLabel* label = static_cast<QLabel*>(group->child("label"));
        label->setPaletteBackgroundColor(colorGroup().highlight());

        ItemIconView* iconView = iconviewDict[sourceGroupName];
        bool unselect = selectionStatus[sourceGroupName];

        //Set isInSelectItems to true to prevent the emission of signals due to selectionChange
        isInSelectItems = true;
        //If ctrlAlt is true, either set all the select items of the group for browsing or unset them (it is a toggle between the 2 states)
        if(ctrlAlt){
            ItemColors* itemColors = itemColorsDict[sourceGroupName];
            QMap<int,bool> browsingMap = browsingStatus[sourceGroupName];
            QList<int> itemsToSkip;
            if(unselect){
                selectionStatus[sourceGroupName] = false;
                for(Q3IconViewItem* item = iconView->firstItem(); item; item = item->nextItem()){
                    int currentIndex = item->index();
                    if(item->isSelected()){
                        if(browsingMap[currentIndex]){
                            browsingMap[currentIndex] = false;
                            QString label = item->text();
                            redrawItem(iconView,itemColors,currentIndex,browsingMap);
                            isInSelectItems = true;//redrawItem sets it back to false
                            item = iconView->findItem(label,Q3ListBox::ExactMatch|Qt::CaseSensitive);
                        }
                    }
                    itemsToSkip.append(itemColors->itemId(currentIndex));
                }
            }
            else{
                selectionStatus[sourceGroupName] = true;

                for(Q3IconViewItem* item = iconView->firstItem(); item; item = item->nextItem()){
                    int currentIndex = item->index();
                    if(item->isSelected()){
                        if(!browsingMap[currentIndex]){
                            browsingMap[currentIndex] = true;
                            QString label = item->text();
                            redrawItem(iconView,itemColors,currentIndex,browsingMap);
                            isInSelectItems = true;//redrawItem sets it back to false
                            item = iconView->findItem(label,Q3ListBox::ExactMatch|Qt::CaseSensitive);
                        }
                    }
                    else itemsToSkip.append(itemColors->itemId(currentIndex));
                }
            }
            browsingStatus.insert(sourceGroupName,browsingMap);
            emit updateItemsToSkip(sourceGroupName,itemsToSkip);

            if(!isBrowsingEnable()){
                if(type == CLUSTER)
                    emit noClustersToBrowse();
                else
                    emit noEventsToBrowse();
            }
            else{
                if(type == CLUSTER)
                    emit clustersToBrowse();
                else
                    emit eventsToBrowse();
            }
        }
        else{
            //If shiftKey is false, either select all the items of the group or deselect them all (it is a toggle between the 2 states)
            if(unselect){
                selectionStatus[sourceGroupName] = false;
                iconView->selectAll(false);
                QMap<QString,QList<int> > selection = selectedItems();
                emit updateShownItems(selection);

                ItemColors* itemColors = itemColorsDict[sourceGroupName];
                QMap<int,bool> browsingMap = browsingStatus[sourceGroupName];
                QList<int> itemsToSkip;
                for(Q3IconViewItem* item = iconView->firstItem(); item; item = item->nextItem()){
                    int currentIndex = item->index();
                    if(browsingMap[currentIndex]){
                        browsingMap[currentIndex] = false;
                        QString label = item->text();
                        redrawItem(iconView,itemColors,currentIndex,browsingMap);
                        isInSelectItems = true;//redrawItem sets it back to false
                        item = iconView->findItem(label,Q3ListBox::ExactMatch|Qt::CaseSensitive);
                        itemsToSkip.append(itemColors->itemId(currentIndex));
                    }
                    else itemsToSkip.append(itemColors->itemId(currentIndex));
                }
                browsingStatus.insert(sourceGroupName,browsingMap);
                emit updateItemsToSkip(sourceGroupName,itemsToSkip);

                if(!isBrowsingEnable()){
                    if(type == CLUSTER)
                        emit noClustersToBrowse();
                    else
                        emit noEventsToBrowse();
                }
                else{
                    if(type == CLUSTER)
                        emit clustersToBrowse();
                    else
                        emit eventsToBrowse();
                }
            }
            else{
                selectionStatus[sourceGroupName] = true;
                iconView->selectAll(true);
                //If it is a cluster palette and the shift key was press, select everything except 0 and 1
                if(shiftKey && type == CLUSTER){
                    ItemColors* itemColors = itemColorsDict[sourceGroupName];
                    QMap<int,bool> browsingMap = browsingStatus[sourceGroupName];
                    QList<int> itemsToSkip;
                    bool hasChanged = false;
                    Q3IconViewItem* item;
                    for(int i = 0;i<2;++i){
                        item = iconView->findItem(QString::fromLatin1("%1").arg(i),Q3ListBox::ExactMatch|Qt::CaseSensitive);
                        if(item != 0){
                            item->setSelected(false);
                            int currentIndex = item->index();
                            if(browsingMap[currentIndex]){
                                hasChanged = true;
                                browsingMap[currentIndex] = false;
                                QString label = item->text();
                                redrawItem(iconView,itemColors,currentIndex,browsingMap);
                                isInSelectItems = true;//redrawItem sets it back to false
                                item = iconView->findItem(label,Q3ListBox::ExactMatch|Qt::CaseSensitive);
                                itemsToSkip.append(itemColors->itemId(currentIndex));
                            }
                            else itemsToSkip.append(itemColors->itemId(currentIndex));
                        }
                    }
                    if(hasChanged){
                        browsingStatus.insert(sourceGroupName,browsingMap);
                        emit updateItemsToSkip(sourceGroupName,itemsToSkip);
                        if(!isBrowsingEnable())
                            emit noClustersToBrowse();
                        else
                            emit clustersToBrowse();
                    }
                }
                QMap<QString,QList<int> > selection = selectedItems();
                emit updateShownItems(selection);
            }
        }
        //reset isInSelectItems to false to enable again the the emission of signals due to selectionChange
        isInSelectItems = false;
    }

    selected = sourceGroupName;
    if(type == EVENT)
        emit selectedGroupChanged(selected);
#endif
}


const QMap<QString,QList<int> > ItemPalette::selectedItems(){
    QMap<QString,QList<int> > selection;
#ifdef KDAB_PORTING
    QHashIterator<QString, ItemIconView*> iterator(iconviewDict);
    while (iterator.hasNext()) {
        iterator.next();
        QString groupName = iterator.key();
        ItemColors* itemColors = itemColorsDict[groupName];
        QList<int> selectedItems;
        for(Q3IconViewItem* item = iterator.value()->firstItem(); item; item = item->nextItem()){
            if(item->isSelected()){
                selectedItems.append(itemColors->itemId(item->index()));
            }
        }
        selection.insert(groupName,selectedItems);
    }
#endif
    return selection;
}

void ItemPalette::slotClickRedraw(){
    #ifdef KDAB_PORTING
    if(!isInSelectItems){
        bool browsingEnable = false;
        bool needToBeUpdated = false;
        QMap<QString,QList<int> > selection;
        QHashIterator<QString, ItemIconView*> iterator(iconviewDict);
        while (iterator.hasNext()) {
            iterator.next();
            QString groupName = iterator.key();
            QMap<int,bool> browsingMap = browsingStatus[groupName];
            ItemColors* itemColors = itemColorsDict[groupName];
            QList<int> selectedItems;
            QList<int> itemsToSkip;
            QList<int> itemsToRedraw;
            for(Q3IconViewItem* item = iterator.value()->firstItem(); item; item = item->nextItem()){
                int index = item->index();
                if(item->isSelected()){
                    selectedItems.append(itemColors->itemId(index));
                    if(!browsingMap[index]) itemsToSkip.append(itemColors->itemId(index));
                    else browsingEnable = true;
                }
                else{
                    if(browsingMap[index]){
                        browsingMap[index] = false;
                        itemsToRedraw.append(index);
                        needToBeUpdated = true;
                    }
                    itemsToSkip.append(itemColors->itemId(index));
                }
            }
            selection.insert(groupName,selectedItems);
            browsingStatus.insert(groupName,browsingMap);
            needRedrawing.insert(groupName,itemsToRedraw);

            emit updateItemsToSkip(groupName,itemsToSkip);
        }

        if(!browsingEnable){
            if(type == CLUSTER)
                emit noClustersToBrowse();
            else
                emit noEventsToBrowse();
        } else {
            if(type == CLUSTER)
                emit clustersToBrowse();
            else
                emit eventsToBrowse();
        }

        emit updateShownItems(selection);

        if(needToBeUpdated){
            updateIconPixmap = true;
            update();
        }
    }
#endif
}

void ItemPalette::slotMousePressWoModificators(QString sourceGroup){  
#ifdef KDAB_PORTING
    ItemIconView* iconView = iconviewDict[sourceGroup];
    int count = 0;
    for(Q3IconViewItem* item = iconView->firstItem(); item; item = item->nextItem()){
        if(item->isSelected()) count++;
        if(count > 1) break;
    }

    if(count <= 2){
        //Set isInSelectItems to true to prevent the emission of signals due to selectionChange
        isInSelectItems = true;

        QHashIterator<QString, ItemIconView*> iterator(iconviewDict);
        while (iterator.hasNext()) {
            iterator.next();
            if(iterator.key() != sourceGroup)
                iterator.value()->selectAll(false);
        }

        //reset isInSelectItems to false to enable again the the emission of signals due to selectionChange
        isInSelectItems = false;

        //If no items were selected in the current group,slotClickRedraw won't be call, so to update the view correctly
        //The updateShownItems signal has to be emitted.
        if(count == 0){
            QMap<QString,QList<int> > selection;
            QHashIterator<QString, ItemIconView*> iterator(iconviewDict);
            while (iterator.hasNext()) {
                iterator.next();
                QList<int> selectedItems;

                selection.insert(iterator.key(),selectedItems);

                //update the browsing status, it is set to false for all the elements
                QString groupName = iterator.key();
                ItemColors* itemColors = itemColorsDict[groupName];
                QMap<int,bool> browsingMap = browsingStatus[groupName];
                QList<int> itemsToSkip;
                for(Q3IconViewItem* item = iterator.value()->firstItem(); item; item = item->nextItem()){
                    int currentIndex = item->index();
                    if(browsingMap[currentIndex]){
                        browsingMap[currentIndex] = false;
                        QString label = item->text();
                        redrawItem(iterator.value(),itemColors,currentIndex,browsingMap);
                        isInSelectItems = true;////redrawItem sets it back to false
                        item = iterator.value()->findItem(label,Q3ListBox::ExactMatch|Qt::CaseSensitive);
                        itemsToSkip.append(itemColors->itemId(currentIndex));
                    }
                    else itemsToSkip.append(itemColors->itemId(currentIndex));
                }
                browsingStatus.insert(groupName,browsingMap);
                emit updateItemsToSkip(groupName,itemsToSkip);
            }

            emit updateShownItems(selection);

            if(type == CLUSTER)
                emit noClustersToBrowse();
            else
                emit noEventsToBrowse();
        }
    }
#endif
}

void ItemPalette::slotMouseReleased(QString sourceGroupName){
    if(!needRedrawing.isEmpty()){
        updateIconPixmap = true;
        update();
    }
}


void ItemPalette::redrawItem(ItemIconView* iconView,ItemColors* itemColors,int index,QMap<int,bool> browsingMap){
    //Set isInSelectItems to true to prevent the emission of signals due to selectionChange
    isInSelectItems = true;
#ifdef KDAB_PORTING
    QString label =  itemColors->itemLabel(index);
    Q3IconViewItem* currentItem =  iconView->findItem(label,Q3ListBox::ExactMatch|Qt::CaseSensitive);
    bool selected = currentItem->isSelected();
    bool browsingStatus = browsingMap[index];

    //Recreate the item
    QPixmap pixmap(14,14);
    pixmap.fill(backgroundColor);
    QColor color = itemColors->color(index,ItemColors::BY_INDEX);
    QPainter painter;
    painter.begin(&pixmap);
    if(!browsingStatus){
        painter.fillRect(0,0,12,12,color);
    }
    else{
        QPolygon polygon(4);
        polygon.putPoints(0,3,0,0,14,0,7,14);
        painter.setBrush(color);
        painter.drawPolygon(polygon);
    }
    painter.end();

    Q3IconViewItem* newItem = new Q3IconViewItem(iconView,currentItem,label,pixmap);
    newItem->setSelected(selected,true);

    //Delete the old item
    delete currentItem;

    //reset isInSelectItems to false to enable again the the emission of signals due to selectionChange
    isInSelectItems = false;
#endif
}

bool ItemPalette::isBrowsingEnable(){
    bool browsingEnable = false;
    QMap<QString, QMap<int,bool> > ::Iterator it;
    for(it = browsingStatus.begin(); it != browsingStatus.end(); ++it){
        QMap<int,bool>  currentMap = it.data();
        QMap<int,bool> ::Iterator it2;
        for(it2 = currentMap.begin(); it2 != currentMap.end(); ++it2){
            if(it2.data()){
                browsingEnable = true;
                break;
            }
        }
        if(browsingEnable) break;
    }
    return browsingEnable;
}

void ItemPalette::slotMousePressWAltButton(QString sourceGroup,int index){
    #ifdef KDAB_PORTING
    QMap<int,bool> browsingMap = browsingStatus[sourceGroup];
    Q3IconViewItem* currentItem = 0L;
    ItemIconView* iconView = iconviewDict[sourceGroup];
    ItemColors* itemColors = itemColorsDict[sourceGroup];
    QString label =  itemColors->itemLabel(index);
    currentItem =  iconView->findItem(label,Q3ListBox::ExactMatch|Qt::CaseSensitive);
    QList<int> itemsToRedraw;
    bool browsingEnable = false;

    if(!currentItem->isSelected()) return;

    if(browsingMap[index]){
        browsingMap[index] = false;
        browsingStatus.insert(sourceGroup,browsingMap);
        browsingEnable = isBrowsingEnable();
    }
    else{
        browsingMap[index] = true;
        browsingStatus.insert(sourceGroup,browsingMap);
        browsingEnable = true;
    }

    itemsToRedraw.append(index);
    needRedrawing.insert(sourceGroup,itemsToRedraw);
#ifdef KDAB_PORTING
    QList<int> itemsToSkip;
    for(Q3IconViewItem* item = iconView->firstItem(); item; item = item->nextItem())
        if(!browsingMap[item->index()])
            itemsToSkip.append(itemColors->itemId(item->index()));
#endif
    emit updateItemsToSkip(sourceGroup,itemsToSkip);

    if(!browsingEnable){
        if(type == CLUSTER)
            emit noClustersToBrowse();
        else
            emit noEventsToBrowse();
    }
    else{
        if(type == CLUSTER)
            emit clustersToBrowse();
        else
            emit eventsToBrowse();
    }
#endif
}

void ItemPalette::changeBackgroundColor(QColor color){
    backgroundColor = color;

    int h;
    int s;
    int v;
    color.getHsv(&h,&s,&v);
    QColor legendColor;
    if(s <= 80 && v >= 240 || (s <= 40 && v >= 220)) legendColor = Qt::black;
    else legendColor = Qt::white;


    QMap<QString,QList<int> > selected = selectedItems();

    QHashIterator<QString, ItemIconView*> iterator(iconviewDict);
    while (iterator.hasNext()) {
        iterator.next();
        iterator.value()->setPaletteBackgroundColor(color);
        iterator.value()->setPaletteForegroundColor(legendColor);
        //Redraw the icons
        QList<int> selectedItems = selected[iterator.key()];


        //Set isInSelectItems to true to prevent the emission of signals due to selectionChange
        isInSelectItems = true;

        ItemIconView* iconView = iconviewDict[iterator.key()];
        iconView->clear();

        //Get the list of items with their color
        ItemColors* itemColors = itemColorsDict[iterator.key()];
        int nbItems = itemColors->numberOfItems();

        //Construct one icon for each item
        QPainter painter;
        for(int i = 0; i<nbItems; ++i){
            QPixmap pix(14,14);
            pix.fill(backgroundColor);
            painter.begin(&pix);
            painter.fillRect(0,0,12,12,itemColors->color(i,ItemColors::BY_INDEX));
            painter.end();
            QIcon icon;
            icon.addPixmap(pix);
            QListWidgetItem *item = new QListWidgetItem(icon,itemColors->itemLabel(i), iconView);
            //TODO item->setData();
        }
#ifdef KDAB_PORTING

        //reselect the item which were selected.
        for(Q3IconViewItem* item = iterator.value()->firstItem(); item; item = item->nextItem()){
            if(selectedItems.contains(itemColors->itemId(item->index())))
                item->setSelected(true,true);
        }
#endif
        //reset isInSelectItems to false to enable again the the emission of signals due to selectionChange
        isInSelectItems = false;
    }

    QHashIterator<QString, ItemGroupView*> iterator2(itemGroupViewDict);
    while (iterator2.hasNext()) {
        iterator2.next();
        iterator2.value()->setPaletteBackgroundColor(color);
        iterator2.value()->setPaletteForegroundColor(legendColor);

    }

    QPalette palette; palette.setColor(backgroundRole(), backgroundColor);
    palette.setColor(foregroundRole(), legendColor); 
    setPalette(palette);
    viewport()->setPaletteBackgroundColor(backgroundColor);
    viewport()->setPaletteForegroundColor(legendColor);
    verticalContainer->setPaletteBackgroundColor(backgroundColor);
    verticalContainer->setPaletteForegroundColor(legendColor);

    update();
}

void ItemPalette::changeColor(QListWidgetItem* item,const QString& groupName){
    int index = item->data(ItemIconView::INDEXICON).toInt();

    //Get the itemColor associated with the item
    ItemColors* itemColors = itemColorsDict[groupName];
    QColor color = itemColors->color(index,ItemColors::BY_INDEX);

    const QColor result = QColorDialog::getColor(color,0);
    if(result.isValid()){
        //Update the itemColor
        itemColors->setColor(index,result,ItemColors::BY_INDEX);

        //Update the icon
        QIcon icon = item->icon();
        QPixmap pixmap;
        QPainter painter;
        painter.begin(&pixmap);
        painter.fillRect(0,0,12,12,result);
        painter.end();
        icon.addPixmap(pixmap);
        item->setIcon(icon);

        //As soon a color changes a signal is emitted.
        emit colorChanged(itemColors->itemId(index),groupName);
    }
}

void ItemPalette::slotMidButtonPressed(const QString& groupName){

}


/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void ItemPalette::languageChange()
{
    setWindowTitle( tr( "Item palette" ) );
}

void ItemPalette::selectItems(QString groupName,QList<int> itemsToSelect,QList<int> itemsToSkip){
    //Set isInSelectItems to true to prevent the emission of signals due to selectionChange
    isInSelectItems = true;
    #ifdef KDAB_PORTING
    Q3IconViewItem* currentIcon = 0L;
    ItemIconView* iconView = iconviewDict[groupName];
    iconView->selectAll(false);
    ItemColors* itemColors = itemColorsDict[groupName];

    //update the browsing map and rebuild the icons
    QPainter painter;
    QMap<int,bool> browsingMap = browsingStatus[groupName];
    browsingMap.clear();
    for(Q3IconViewItem* item = iconView->firstItem(); item; item = item->nextItem()){
        int id = itemColors->itemId(item->index());
        if(itemsToSkip.contains(id)) browsingMap.insert(item->index(),false);
        else browsingMap.insert(item->index(),true);
        QString label = itemColors->itemLabel(item->index());
        redrawItem(iconView,itemColors,item->index(),browsingMap);
        isInSelectItems = true;//redrawItem sets it back to false
        item = iconView->findItem(label,Q3ListBox::ExactMatch|Qt::CaseSensitive);
    }
    browsingStatus.insert(groupName,browsingMap);

    QList<int>::iterator itemIterator;
    for(itemIterator = itemsToSelect.begin(); itemIterator != itemsToSelect.end(); ++itemIterator){
        QString label =  itemColors->itemLabelById(*itemIterator);
        currentIcon =  iconView->findItem(label,Q3ListBox::ExactMatch|Qt::CaseSensitive);
        currentIcon->setSelected(true,true);
    }

    //Last item in selection gets focus if it exists
    if(!itemsToSelect.isEmpty()) iconView->setCurrentItem(currentIcon);

    //reset isInSelectItems to false to enable again the the emission of signals due to selectionChange
    isInSelectItems = false;
    #endif
}



void ItemPalette::reset(){
    iconviewDict.clear();
    itemGroupViewDict.clear();
    itemColorsDict.clear();
    selected.clear();
    clusterGroupList.clear();
    itemGroupList.clear();
    browsingStatus.clear();
    needRedrawing.clear();
    selectionStatus.clear();

    isInSelectItems = false;
}

void ItemPalette::createGroup(const QString &id){
    ItemGroupView* group = new ItemGroupView(backgroundColor,verticalContainer);

    group->setObjectName(id);
    GroupNameLabel* label = new GroupNameLabel(id,group);

    //Set the size to 2 digits, max 99 groups
    label->setFixedWidth(labelSize);
    label->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    label->setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed);
    QFont f("Helvetica",8);
    label->setFont(f);
    label->adjustSize();

    ItemIconView* iconView;
    QFontInfo fontInfo = QFontInfo(f);
    if(type == CLUSTER)
        iconView = new ItemIconView(backgroundColor,QListView::IconMode,fontInfo.pixelSize() * 2,5,group,id);
    else
        iconView = new ItemIconView(backgroundColor,QListView::ListMode,gridX,5,group,id);
    iconView->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);

    #ifdef KDAB_PORTING
    if(iconviewDict.count() >= 1){
        QHashIterator<QString, ItemIconView*> iterator(iconviewDict);
        while (iterator.hasNext()) {
            iterator.next();
            iconView->resizeContents((iconviewDict[iterator.key()])->contentsWidth(),2);
        }
    }
    else
        iconView->adjustSize();
#endif
    //group->setStretchFactor(label,0);
    //group->setStretchFactor(iconView,200);
    group->setIconView(iconView);

    iconviewDict.insert(id,iconView);
    itemGroupViewDict.insert(id,group);

    group->adjustSize();
    iconView->show();
    group->show();

    if(spaceWidget){
        delete spaceWidget;
    }
    spaceWidget = new QWidget(verticalContainer);
    spaceWidget->show();
    verticalContainer->setStretchFactor(spaceWidget,2);

    //Signal and slot connection
    connect(iconView,SIGNAL(selectionChanged()),this, SLOT(slotClickRedraw()));
    connect(iconView,SIGNAL(mousePressMiddleButton(QString,QListWidgetItem*)),this, SLOT(slotMousePressed(QString,QListWidgetItem*)));
    connect(this,SIGNAL(paletteResized(int,int)),group,SLOT(reAdjustSize(int,int)));
    connect(iconView,SIGNAL(mousePressWoModificators(QString)),this, SLOT(slotMousePressWoModificators(QString)));
    connect(iconView,SIGNAL(mousePressWAltButton(QString,int)),this, SLOT(slotMousePressWAltButton(QString,int)));
    connect(iconView,SIGNAL(mouseReleased(QString)),this, SLOT(slotMouseReleased(QString)));

    connect(label,SIGNAL(leftClickOnLabel(QString,bool,bool)),this, SLOT(slotMousePressed(QString,bool,bool)));
    connect(label,SIGNAL(middleClickOnLabel(QString)),this, SLOT(slotMidButtonPressed(QString)));

    orderTheGroups();
    emit paletteResized(viewport()->width(),labelSize);
    update();
}

void ItemPalette::removeGroup(const QString &groupName){
    itemColorsDict.remove(groupName);
    itemGroupViewDict.remove(groupName);
    iconviewDict.remove(groupName);
    browsingStatus.remove(groupName);
    selectionStatus.remove(groupName);
    if(type == CLUSTER) clusterGroupList.remove(groupName.toInt());
    else itemGroupList.remove(groupName);

    //a group must always be selected.
    if(selected == groupName){
        if(type == CLUSTER && !clusterGroupList.isEmpty()){
            qSort(clusterGroupList);
            selectGroupLabel(QString::fromLatin1("%1").arg(clusterGroupList.at(0)));
        }
        else if(type == EVENT && !itemGroupList.isEmpty()){
            qSort(itemGroupList);
            selectGroupLabel(itemGroupList.at(0));
        }
        else  selected.clear();//never reach
    }

}

void ItemPalette::selectGroup(const QString& groupName){
    if(type == CLUSTER && !clusterGroupList.isEmpty()){
        qSort(clusterGroupList);
        if(clusterGroupList.contains(groupName.toInt())) selectGroupLabel(groupName);
        else selectGroupLabel(QString::fromLatin1("%1").arg(clusterGroupList.at(0)));
    }
    else if(type == EVENT && !itemGroupList.isEmpty()){
        qSort(itemGroupList);
        if(itemGroupList.contains(groupName)) selectGroupLabel(groupName);
        else selectGroupLabel(itemGroupList.at(0));
    }
    else  selected.clear();//never reach
}

void ItemPalette::selectAllItems(){
    #ifdef KDAB_PORTING
    //Set isInSelectItems to true to prevent the emission of signals due to selectionChange
    isInSelectItems = true;

    QHashIterator<QString, ItemIconView*> iterator(iconviewDict);
    while (iterator.hasNext()) {
        iterator.next();
        iterator.value()->selectAll(true);
    }

    QMap<QString,QList<int> > selection = selectedItems();
    emit updateShownItems(selection);

    //reset isInSelectItems to false to enable again the the emission of signals due to selectionChange
    isInSelectItems = false;
#endif
}

void ItemPalette::deselectAllItems(){
#ifdef KDAB_PORTING
    //Set isInSelectItems to true to prevent the emission of signals due to selectionChange
    isInSelectItems = true;

    QMap<QString,QList<int> > selection;
    QHashIterator<QString, ItemIconView*> iterator(iconviewDict);
    while (iterator.hasNext()) {
        iterator.next();
        iterator.value()->selectAll(false);
        QList<int> selectedItems;
        selection.insert(iterator.key(),selectedItems);
    }

    emit updateShownItems(selection);

    //update the browsing status, it is set to false for all the elements
    QHashIterator<QString, ItemIconView*> iterator2(iconviewDict);
    while (iterator2.hasNext()) {
        iterator2.next();
        QString groupName = iterator2.key();
        ItemColors* itemColors = itemColorsDict[groupName];
        QMap<int,bool> browsingMap = browsingStatus[groupName];
        QList<int> itemsToSkip;
        for(Q3IconViewItem* item = iterator2.value()->firstItem(); item; item = item->nextItem()){
            int currentIndex = item->index();
            if(browsingMap[currentIndex]){
                browsingMap[currentIndex] = false;
                QString label = item->text();
                redrawItem(iterator2.value(),itemColors,currentIndex,browsingMap);
                isInSelectItems = true;//redrawItem sets it back to false
                item = iterator2.value()->findItem(label,Q3ListBox::ExactMatch|Qt::CaseSensitive);
                itemsToSkip.append(itemColors->itemId(currentIndex));
            }
            else itemsToSkip.append(itemColors->itemId(currentIndex));
        }
        browsingStatus.insert(groupName,browsingMap);
        emit updateItemsToSkip(groupName,itemsToSkip);
    }
    
    //reset isInSelectItems to false to enable again the the emission of signals due to selectionChange
    isInSelectItems = false;
#endif
}


void ItemPalette::orderTheGroups(){
    //Remove all the children of the verticalContainer (spaceWidget and groups)
    verticalContainer->removeChild(spaceWidget);

    QHashIterator<QString, ItemGroupView*> iterator(itemGroupViewDict);
    while (iterator.hasNext()) {
        iterator.next();
        verticalContainer->removeChild(iterator.value());
    }

    if(type == CLUSTER) {
        qSort(clusterGroupList);
        QList<int>::iterator iterator;
        for(iterator = clusterGroupList.begin(); iterator != clusterGroupList.end(); ++iterator)
            verticalContainer->insertChild(itemGroupViewDict[QString::fromLatin1("%1").arg(*iterator)]);
    } else {
        qSort(itemGroupList);
        QStringList::iterator iterator;
        for(iterator = itemGroupList.begin(); iterator != itemGroupList.end(); ++iterator)
            verticalContainer->insertChild(itemGroupViewDict[QString::fromLatin1("%1").arg(*iterator)]);
    }

    delete spaceWidget;
    spaceWidget = new QWidget(verticalContainer);
    spaceWidget->show();
    verticalContainer->setStretchFactor(spaceWidget,2);
}



#include "itempalette.moc"
