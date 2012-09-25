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

ItemPalette::ItemPalette(PaletteType type, const QColor &backgroundColor, QWidget* parent, const char* name, Qt::WFlags fl)
    : Q3ScrollView(parent,name,fl),backgroundColor(backgroundColor),isInSelectItems(false),
      spaceWidget(0L),type(type),selected(""),updateIconPixmap(false)
{
    itemGroupViewDict.setAutoDelete(true);

    setAutoFillBackground(true);
    //Set the palette color
    QPalette palette; palette.setColor(backgroundRole(), backgroundColor); setPalette(palette);
    //Set the palette color, the foreground color depends on the background color
    int h;
    int s;
    int v;
    backgroundColor.getHsv(&h,&s,&v);
    QColor legendColor;
    if(s <= 80 && v >= 240 || (s <= 40 && v >= 220)) legendColor = Qt::black;
    else legendColor = Qt::white;
    setPaletteForegroundColor(legendColor);
    setHScrollBarMode(Q3ScrollView::AlwaysOff);

    setResizePolicy(Q3ScrollView::AutoOneFit);
    verticalContainer = new Q3VBox(viewport(),"verticalContainer");
    addChild(verticalContainer);
    verticalContainer->setSpacing(5);

    QFont f("Helvetica",8);
    QFontInfo fontInfo = QFontInfo(f);
    if(type == CLUSTER) labelSize = fontInfo.pixelSize() * 2;
    else labelSize = fontInfo.pixelSize() * 3;

    //Set the legend in the good language
    languageChange();

    verticalContainer->adjustSize();
    adjustSize();
}    


ItemPalette::~ItemPalette()
{
    // no need to delete child widgets, Qt does it all for us
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


void ItemPalette::updateItemList(QString groupName){  
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
        (void)new Q3IconViewItem(iconView,itemColors->itemLabel(i),pix);
    }

    browsingStatus.insert(groupName,browsingMap);

    if(nbItems == 0) iconView->resizeContents(50,20);

    else iconView->adjustSize();
}


void ItemPalette::slotMousePressed(int button,Q3IconViewItem* item,QString sourceGroupName){ 
    if (!item) {
        return; //pressed on viewport
    } else {
        // middle pressed on item
        if(button == Qt::MidButton) {
            changeColor(item,sourceGroupName);
        }
    }
}

void ItemPalette::selectGroupLabel(QString sourceGroupName){
    if(!selected.isEmpty()){
        ItemGroupView* previousSelectedGroup = itemGroupViewDict[selected];
        if(previousSelectedGroup != 0){
            GroupNameLabel* previousLabel = dynamic_cast<GroupNameLabel*>(previousSelectedGroup->child("label"));
            previousLabel->setPaletteBackgroundColor(colorGroup().background());
        }
    }
    if(!sourceGroupName.isEmpty()){
        ItemGroupView* group = itemGroupViewDict[sourceGroupName];
        QLabel* label = dynamic_cast<QLabel*>(group->child("label"));
        label->setPaletteBackgroundColor(colorGroup().highlight());
    }

    selected = sourceGroupName;
    if(type == EVENT)
        emit selectedGroupChanged(selected);
}

void ItemPalette::slotMousePressed(QString sourceGroupName,bool shiftKey,bool ctrlAlt){

    if(!selected.isEmpty()){
        ItemGroupView* previousSelectedGroup = itemGroupViewDict[selected];
        if(previousSelectedGroup != 0){
            GroupNameLabel* previousLabel = dynamic_cast<GroupNameLabel*>(previousSelectedGroup->child("label"));
            previousLabel->setPaletteBackgroundColor(colorGroup().background());
        }
    }
    if(!sourceGroupName.isEmpty()){
        ItemGroupView* group = itemGroupViewDict[sourceGroupName];
        QLabel* label = dynamic_cast<QLabel*>(group->child("label"));
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
                    if(type == CLUSTER) emit noClustersToBrowse();
                    else emit noEventsToBrowse();
                }
                else{
                    if(type == CLUSTER) emit clustersToBrowse();
                    else emit eventsToBrowse();
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
                        if(!isBrowsingEnable()) emit noClustersToBrowse();
                        else emit clustersToBrowse();
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
}


const QMap<QString,QList<int> > ItemPalette::selectedItems(){
    QMap<QString,QList<int> > selection;

    Q3DictIterator<ItemIconView> iterator(iconviewDict);
    for(;iterator.current();++iterator){
        QString groupName = iterator.currentKey();
        ItemColors* itemColors = itemColorsDict[groupName];
        QList<int> selectedItems;
        for(Q3IconViewItem* item = iterator.current()->firstItem(); item; item = item->nextItem()){
            if(item->isSelected()){
                selectedItems.append(itemColors->itemId(item->index()));
            }
        }
        selection.insert(groupName,selectedItems);
    }

    return selection;
}

void ItemPalette::slotClickRedraw(){
    if(!isInSelectItems){
        bool browsingEnable = false;
        bool needToBeUpdated = false;
        QMap<QString,QList<int> > selection;
        Q3DictIterator<ItemIconView> iterator(iconviewDict);
        for(;iterator.current();++iterator){
            QString groupName = iterator.currentKey();
            QMap<int,bool> browsingMap = browsingStatus[groupName];
            ItemColors* itemColors = itemColorsDict[groupName];
            QList<int> selectedItems;
            QList<int> itemsToSkip;
            QList<int> itemsToRedraw;
            for(Q3IconViewItem* item = iterator.current()->firstItem(); item; item = item->nextItem()){
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
            if(type == CLUSTER) emit noClustersToBrowse();
            else emit noEventsToBrowse();
        }
        else{
            if(type == CLUSTER) emit clustersToBrowse();
            else emit eventsToBrowse();
        }

        emit updateShownItems(selection);

        if(needToBeUpdated){
            updateIconPixmap = true;
            update();
        }
    }
}

void ItemPalette::slotMousePressWoModificators(QString sourceGroup){  

    ItemIconView* iconView = iconviewDict[sourceGroup];
    int count = 0;
    for(Q3IconViewItem* item = iconView->firstItem(); item; item = item->nextItem()){
        if(item->isSelected()) count++;
        if(count > 1) break;
    }

    if(count <= 2){
        //Set isInSelectItems to true to prevent the emission of signals due to selectionChange
        isInSelectItems = true;

        Q3DictIterator<ItemIconView> iterator(iconviewDict);
        for(;iterator.current();++iterator){
            if(iterator.currentKey() != sourceGroup)
                iterator.current()->selectAll(false);
        }

        //reset isInSelectItems to false to enable again the the emission of signals due to selectionChange
        isInSelectItems = false;

        //If no items were selected in the current group,slotClickRedraw won't be call, so to update the view correctly
        //The updateShownItems signal has to be emitted.
        if(count == 0){
            QMap<QString,QList<int> > selection;
            Q3DictIterator<ItemIconView> iterator(iconviewDict);
            for(;iterator.current();++iterator){
                QList<int> selectedItems;

                selection.insert(iterator.currentKey(),selectedItems);

                //update the browsing status, it is set to false for all the elements
                QString groupName = iterator.currentKey();
                ItemColors* itemColors = itemColorsDict[groupName];
                QMap<int,bool> browsingMap = browsingStatus[groupName];
                QList<int> itemsToSkip;
                for(Q3IconViewItem* item = iterator.current()->firstItem(); item; item = item->nextItem()){
                    int currentIndex = item->index();
                    if(browsingMap[currentIndex]){
                        browsingMap[currentIndex] = false;
                        QString label = item->text();
                        redrawItem(iterator.current(),itemColors,currentIndex,browsingMap);
                        isInSelectItems = true;////redrawItem sets it back to false
                        item = iterator.current()->findItem(label,Q3ListBox::ExactMatch|Qt::CaseSensitive);
                        itemsToSkip.append(itemColors->itemId(currentIndex));
                    }
                    else itemsToSkip.append(itemColors->itemId(currentIndex));
                }
                browsingStatus.insert(groupName,browsingMap);
                emit updateItemsToSkip(groupName,itemsToSkip);
            }

            emit updateShownItems(selection);

            if(type == CLUSTER) emit noClustersToBrowse();
            else emit noEventsToBrowse();
        }
    }
}

void ItemPalette::slotMouseReleased(QString sourceGroupName){
    if(needRedrawing.count() != 0){
        updateIconPixmap = true;
        update();
    }
}


void ItemPalette::redrawItem(ItemIconView* iconView,ItemColors* itemColors,int index,QMap<int,bool> browsingMap){
    //Set isInSelectItems to true to prevent the emission of signals due to selectionChange
    isInSelectItems = true;

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

    QList<int> itemsToSkip;
    for(Q3IconViewItem* item = iconView->firstItem(); item; item = item->nextItem())
        if(!browsingMap[item->index()]) itemsToSkip.append(itemColors->itemId(item->index()));

    emit updateItemsToSkip(sourceGroup,itemsToSkip);

    if(!browsingEnable){
        if(type == CLUSTER) emit noClustersToBrowse();
        else emit noEventsToBrowse();
    }
    else{
        if(type == CLUSTER) emit clustersToBrowse();
        else emit eventsToBrowse();
    }
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

    Q3DictIterator<ItemIconView> iterator(iconviewDict);
    for(;iterator.current();++iterator){
        iterator.current()->setPaletteBackgroundColor(color);
        iterator.current()->setPaletteForegroundColor(legendColor);
        //Redraw the icons
        QList<int> selectedItems = selected[iterator.currentKey()];


        //Set isInSelectItems to true to prevent the emission of signals due to selectionChange
        isInSelectItems = true;

        ItemIconView* iconView = iconviewDict[iterator.currentKey()];
        iconView->clear();

        //Get the list of items with their color
        ItemColors* itemColors = itemColorsDict[iterator.currentKey()];
        int nbItems = itemColors->numberOfItems();

        //Construct one icon for each item
        QPainter painter;

        for(int i = 0; i<nbItems; ++i){
            QPixmap pix(14,14);
            pix.fill(backgroundColor);
            painter.begin(&pix);
            painter.fillRect(0,0,12,12,itemColors->color(i,ItemColors::BY_INDEX));
            painter.end();
            (void)new Q3IconViewItem(iconView,itemColors->itemLabel(i),pix);
        }

        //reselect the item which were selected.
        for(Q3IconViewItem* item = iterator.current()->firstItem(); item; item = item->nextItem()){
            if(selectedItems.contains(itemColors->itemId(item->index()))) item->setSelected(true,true);
        }

        //reset isInSelectItems to false to enable again the the emission of signals due to selectionChange
        isInSelectItems = false;
    }

    Q3DictIterator<ItemGroupView> iterator2(itemGroupViewDict);
    for(;iterator2.current();++iterator2){
        iterator2.current()->setPaletteBackgroundColor(color);
        iterator2.current()->setPaletteForegroundColor(legendColor);
    }

    QPalette palette; palette.setColor(backgroundRole(), backgroundColor); setPalette(palette);
    setPaletteForegroundColor(legendColor);
    viewport()->setPaletteBackgroundColor(backgroundColor);
    viewport()->setPaletteForegroundColor(legendColor);
    verticalContainer->setPaletteBackgroundColor(backgroundColor);
    verticalContainer->setPaletteForegroundColor(legendColor);

    update();
}

void ItemPalette::changeColor(Q3IconViewItem* item,QString groupName){
    int index = item->index();

    //Get the itemColor associated with the item
    ItemColors* itemColors = itemColorsDict[groupName];
    QColor color = itemColors->color(index,ItemColors::BY_INDEX);

    const QColor result = QColorDialog::getColor(color,0);
    if(result.isValid()){
        //Update the itemColor
        itemColors->setColor(index,result,ItemColors::BY_INDEX);

        //Update the icon
        QPixmap* pixmap = item->pixmap();
        QPainter painter;
        painter.begin(pixmap);
        painter.fillRect(0,0,12,12,result);
        painter.end();

        //As soon a color changes a signal is emitted.
        emit colorChanged(itemColors->itemId(index),groupName);
    }
}

void ItemPalette::slotMidButtonPressed(QString groupName){

}


/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void ItemPalette::languageChange()
{
    setCaption( tr( "Item palette" ) );
}

void ItemPalette::selectItems(QString groupName,QList<int> itemsToSelect,QList<int> itemsToSkip){
    //Set isInSelectItems to true to prevent the emission of signals due to selectionChange
    isInSelectItems = true;
    
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

void ItemPalette::createGroup(QString id){  
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
        iconView = new ItemIconView(backgroundColor,Q3IconView::Bottom,fontInfo.pixelSize() * 2,5,group,id);
    else
        iconView = new ItemIconView(backgroundColor,Q3IconView::Right,gridX,5,group,id);
    iconView->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);

    if(iconviewDict.count() >= 1){
        Q3DictIterator<ItemIconView> iterator(iconviewDict);
        iconView->resizeContents((iconviewDict[iterator.currentKey()])->contentsWidth(),2);
    }
    else
        iconView->adjustSize();

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
    connect(iconView,SIGNAL(mouseButtonPressed(int,Q3IconViewItem*,QString)),this, SLOT(slotMousePressed(int,Q3IconViewItem*,QString)));
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

void ItemPalette::removeGroup(QString groupName){
    itemColorsDict.remove(groupName);
    itemGroupViewDict.remove(groupName);
    iconviewDict.remove(groupName);
    browsingStatus.remove(groupName);
    selectionStatus.remove(groupName);
    if(type == CLUSTER) clusterGroupList.remove(groupName.toInt());
    else itemGroupList.remove(groupName);

    //a group must always be selected.
    if(selected == groupName){
        if(type == CLUSTER && clusterGroupList.count() > 0){
            qSort(clusterGroupList);
            selectGroupLabel(QString::fromLatin1("%1").arg(clusterGroupList[0]));
        }
        else if(type == EVENT && itemGroupList.count() > 0){
            qSort(itemGroupList);
            selectGroupLabel(itemGroupList[0]);
        }
        else  selected.clear();//never reach
    }

}

void ItemPalette::selectGroup(QString groupName){
    if(type == CLUSTER && clusterGroupList.count() > 0){
        qSort(clusterGroupList);
        if(clusterGroupList.contains(groupName.toInt())) selectGroupLabel(groupName);
        else selectGroupLabel(QString::fromLatin1("%1").arg(clusterGroupList[0]));
    }
    else if(type == EVENT && itemGroupList.count() > 0){
        qSort(itemGroupList);
        if(itemGroupList.contains(groupName)) selectGroupLabel(groupName);
        else selectGroupLabel(itemGroupList[0]);
    }
    else  selected.clear();//never reach
}

void ItemPalette::selectAllItems(){
    //Set isInSelectItems to true to prevent the emission of signals due to selectionChange
    isInSelectItems = true;

    Q3DictIterator<ItemIconView> iterator(iconviewDict);
    for(;iterator.current();++iterator)
        iterator.current()->selectAll(true);

    QMap<QString,QList<int> > selection = selectedItems();
    emit updateShownItems(selection);

    //reset isInSelectItems to false to enable again the the emission of signals due to selectionChange
    isInSelectItems = false;
}

void ItemPalette::deselectAllItems(){
    //Set isInSelectItems to true to prevent the emission of signals due to selectionChange
    isInSelectItems = true;

    QMap<QString,QList<int> > selection;
    Q3DictIterator<ItemIconView> iterator(iconviewDict);
    for(;iterator.current();++iterator){
        iterator.current()->selectAll(false);
        QList<int> selectedItems;
        selection.insert(iterator.currentKey(),selectedItems);
    }

    emit updateShownItems(selection);

    //update the browsing status, it is set to false for all the elements
    for(iterator.toFirst();iterator.current();++iterator){
        QString groupName = iterator.currentKey();
        ItemColors* itemColors = itemColorsDict[groupName];
        QMap<int,bool> browsingMap = browsingStatus[groupName];
        QList<int> itemsToSkip;
        for(Q3IconViewItem* item = iterator.current()->firstItem(); item; item = item->nextItem()){
            int currentIndex = item->index();
            if(browsingMap[currentIndex]){
                browsingMap[currentIndex] = false;
                QString label = item->text();
                redrawItem(iterator.current(),itemColors,currentIndex,browsingMap);
                isInSelectItems = true;//redrawItem sets it back to false
                item = iterator.current()->findItem(label,Q3ListBox::ExactMatch|Qt::CaseSensitive);
                itemsToSkip.append(itemColors->itemId(currentIndex));
            }
            else itemsToSkip.append(itemColors->itemId(currentIndex));
        }
        browsingStatus.insert(groupName,browsingMap);
        emit updateItemsToSkip(groupName,itemsToSkip);
    }
    
    //reset isInSelectItems to false to enable again the the emission of signals due to selectionChange
    isInSelectItems = false;
}


void ItemPalette::orderTheGroups(){
    //Remove all the children of the verticalContainer (spaceWidget and groups)
    verticalContainer->removeChild(spaceWidget);

    Q3DictIterator<ItemGroupView> it(itemGroupViewDict);
    for(;it.current();++it) verticalContainer->removeChild(it.current());

    if(type == CLUSTER){
        qSort(clusterGroupList);
        QList<int>::iterator iterator;
        for(iterator = clusterGroupList.begin(); iterator != clusterGroupList.end(); ++iterator)
            verticalContainer->insertChild(itemGroupViewDict[QString::fromLatin1("%1").arg(*iterator)]);
    }
    else{
        qSort(itemGroupList);
        QList<QString>::iterator iterator;
        for(iterator = itemGroupList.begin(); iterator != itemGroupList.end(); ++iterator)
            verticalContainer->insertChild(itemGroupViewDict[QString::fromLatin1("%1").arg(*iterator)]);
    }

    delete spaceWidget;
    spaceWidget = new QWidget(verticalContainer);
    spaceWidget->show();
    verticalContainer->setStretchFactor(spaceWidget,2);
}



#include "itempalette.moc"
