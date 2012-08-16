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
#include <qvariant.h>
#include <qpainter.h>
#include <qlayout.h>
#include <qtooltip.h>
#include <qwhatsthis.h>
#include <qpixmap.h>
#include <qbitmap.h>
#include <qscrollview.h>
#include <qlayout.h> 
#include <qhbox.h>
#include <qstyle.h>
#include <qcolordialog.h>

//KDE includes
#include <kiconloader.h>
#include <kpopupmenu.h>

//General C++ include files
#include <iostream>
using namespace std;

ItemPalette::ItemPalette(PaletteType type,QColor backgroundColor,QWidget* parent,const char* name,WFlags fl)
    : QScrollView(parent,name,fl),backgroundColor(backgroundColor),isInSelectItems(false),
    spaceWidget(0L),type(type),selected(""),updateIconPixmap(false)
{
   itemGroupViewDict.setAutoDelete(true);

   //Set the palette color
   setPaletteBackgroundColor(backgroundColor);
   //Set the palette color, the foreground color depends on the background color
   int h;
   int s;
   int v;
   backgroundColor.hsv(&h,&s,&v);
   QColor legendColor;
   if(s <= 80 && v >= 240 || (s <= 40 && v >= 220)) legendColor = black;
   else legendColor = white;
   setPaletteForegroundColor(legendColor);
   setHScrollBarMode(QScrollView::AlwaysOff);

   setResizePolicy(QScrollView::AutoOneFit);
   verticalContainer = new QVBox(viewport(),"verticalContainer");
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


void ItemPalette::drawContents(QPainter* painter){
 if(updateIconPixmap){   
  QMap<QString,QValueList<int> > selected = selectedItems(); 
  //update the icons if need it
  QMap<QString,QValueList<int> >::Iterator it;
  for(it = needRedrawing.begin(); it != needRedrawing.end(); ++it){
   QValueList<int> items = it.data();
   QString  groupName = it.key();
   QMap<int,bool> browsingMap = browsingStatus[groupName];
   ItemIconView* iconView = iconviewDict[groupName];
   ItemColors* itemColors = itemColorsDict[groupName];
 
   QValueList<int> selectedItems = selected[groupName];
   
   //redraw the items which have been modified
   QValueList<int>::iterator iterator;
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
 QScrollView::resizeEvent(event);
}

void ItemPalette::createItemList(ItemColors* itemColors,QString groupName,int descriptionLength){
  //Compute gridX used for the event palette where the text is next to the icon (14px)
  QFontInfo fontInfo = QFontInfo(QFont());
  gridX = descriptionLength * fontInfo.pixelSize() + 15;
  
  itemColorsDict.insert(groupName,itemColors);
  //In the case of cluster files, the groupName (<=> electrode id) correspond to a number and the groups are
  //order numerically
  if(type == CLUSTER) clusterGroupList.append(groupName.toInt());
  else itemGroupList.append(groupName);
  createGroup(groupName);

  updateItemList(groupName);

  //always select a group
  if(selected == "") selectGroupLabel(groupName);

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
   (void)new QIconViewItem(iconView,itemColors->itemLabel(i),pix);
  }

  browsingStatus.insert(groupName,browsingMap);
  
  if(nbItems == 0) iconView->resizeContents(50,20);

  else iconView->adjustSize();
}

void ItemPalette::slotRightPressed(QIconViewItem* item){
 if(!item) return; // right pressed on viewport
 else{
   //Create a popmenu if need it.
 }
}

void ItemPalette::slotMousePressed(int button,QIconViewItem* item,QString sourceGroupName){ 
  if (!item) return; //pressed on viewport
  else{
   // middle pressed on item
   if(button == Qt::MidButton) changeColor(item,sourceGroupName);
  }
}

void ItemPalette::selectGroupLabel(QString sourceGroupName){
if(selected != ""){
  ItemGroupView* previousSelectedGroup = itemGroupViewDict[selected];
  if(previousSelectedGroup != 0){
   GroupNameLabel* previousLabel = dynamic_cast<GroupNameLabel*>(previousSelectedGroup->child("label"));
   previousLabel->setPaletteBackgroundColor(colorGroup().background());    
  }
 }
 if(sourceGroupName != ""){
  ItemGroupView* group = itemGroupViewDict[sourceGroupName];
  QLabel* label = dynamic_cast<QLabel*>(group->child("label"));
  label->setPaletteBackgroundColor(colorGroup().highlight());
 }

 selected = sourceGroupName;
 if(type == EVENT) emit selectedGroupChanged(selected);
}

void ItemPalette::slotMousePressed(QString sourceGroupName,bool shiftKey,bool ctrlAlt){

 if(selected != ""){
  ItemGroupView* previousSelectedGroup = itemGroupViewDict[selected];
  if(previousSelectedGroup != 0){
   GroupNameLabel* previousLabel = dynamic_cast<GroupNameLabel*>(previousSelectedGroup->child("label"));
   previousLabel->setPaletteBackgroundColor(colorGroup().background());    
  }
 }
 if(sourceGroupName != ""){
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
   QValueList<int> itemsToSkip;
   if(unselect){
    selectionStatus[sourceGroupName] = false;
    for(QIconViewItem* item = iconView->firstItem(); item; item = item->nextItem()){
     int currentIndex = item->index();
     if(item->isSelected()){
      if(browsingMap[currentIndex]){
       browsingMap[currentIndex] = false;
       QString label = item->text();
       redrawItem(iconView,itemColors,currentIndex,browsingMap);
       isInSelectItems = true;//redrawItem sets it back to false
       item = iconView->findItem(label,Qt::ExactMatch|CaseSensitive);
      }         
     }
     itemsToSkip.append(itemColors->itemId(currentIndex));
    }     
   }
   else{
    selectionStatus[sourceGroupName] = true;
    
    for(QIconViewItem* item = iconView->firstItem(); item; item = item->nextItem()){
     int currentIndex = item->index();
     if(item->isSelected()){
      if(!browsingMap[currentIndex]){
       browsingMap[currentIndex] = true;
       QString label = item->text();
       redrawItem(iconView,itemColors,currentIndex,browsingMap);
       isInSelectItems = true;//redrawItem sets it back to false
       item = iconView->findItem(label,Qt::ExactMatch|CaseSensitive);
      }        
     }
     else itemsToSkip.append(itemColors->itemId(currentIndex));
    }    
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
   //If shiftKey is false, either select all the items of the group or deselect them all (it is a toggle between the 2 states)
   if(unselect){
    selectionStatus[sourceGroupName] = false;
    iconView->selectAll(false); 
    QMap<QString,QValueList<int> > selection = selectedItems();
    emit updateShownItems(selection);
    
    ItemColors* itemColors = itemColorsDict[sourceGroupName];
    QMap<int,bool> browsingMap = browsingStatus[sourceGroupName];
    QValueList<int> itemsToSkip;
    for(QIconViewItem* item = iconView->firstItem(); item; item = item->nextItem()){
     int currentIndex = item->index();
     if(browsingMap[currentIndex]){
      browsingMap[currentIndex] = false;
      QString label = item->text();
      redrawItem(iconView,itemColors,currentIndex,browsingMap);
      isInSelectItems = true;//redrawItem sets it back to false
      item = iconView->findItem(label,Qt::ExactMatch|CaseSensitive);
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
     QValueList<int> itemsToSkip;
     bool hasChanged = false;
     QIconViewItem* item;
     for(int i = 0;i<2;++i){
      item = iconView->findItem(QString("%1").arg(i),Qt::ExactMatch|CaseSensitive);
      if(item != 0){
       item->setSelected(false);
       int currentIndex = item->index();
       if(browsingMap[currentIndex]){
        hasChanged = true;
        browsingMap[currentIndex] = false;
        QString label = item->text();
        redrawItem(iconView,itemColors,currentIndex,browsingMap);
        isInSelectItems = true;//redrawItem sets it back to false
        item = iconView->findItem(label,Qt::ExactMatch|CaseSensitive);
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
    QMap<QString,QValueList<int> > selection = selectedItems();
    emit updateShownItems(selection);    
   }  
  }
  //reset isInSelectItems to false to enable again the the emission of signals due to selectionChange
  isInSelectItems = false;
 }

 selected = sourceGroupName;
 if(type == EVENT) emit selectedGroupChanged(selected);
}


const QMap<QString,QValueList<int> > ItemPalette::selectedItems(){
 QMap<QString,QValueList<int> > selection;

  QDictIterator<ItemIconView> iterator(iconviewDict);
  for(;iterator.current();++iterator){
   QString groupName = iterator.currentKey(); 
   ItemColors* itemColors = itemColorsDict[groupName]; 
   QValueList<int> selectedItems;
   for(QIconViewItem* item = iterator.current()->firstItem(); item; item = item->nextItem()){
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
  QMap<QString,QValueList<int> > selection;
  QDictIterator<ItemIconView> iterator(iconviewDict);
  for(;iterator.current();++iterator){
   QString groupName = iterator.currentKey();   
   QMap<int,bool> browsingMap = browsingStatus[groupName]; 
   ItemColors* itemColors = itemColorsDict[groupName];
   QValueList<int> selectedItems;
   QValueList<int> itemsToSkip;
   QValueList<int> itemsToRedraw;
   for(QIconViewItem* item = iterator.current()->firstItem(); item; item = item->nextItem()){
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
 for(QIconViewItem* item = iconView->firstItem(); item; item = item->nextItem()){
  if(item->isSelected()) count++;
  if(count > 1) break;
 }
  
 if(count <= 2){
  //Set isInSelectItems to true to prevent the emission of signals due to selectionChange
  isInSelectItems = true;

  QDictIterator<ItemIconView> iterator(iconviewDict);
  for(;iterator.current();++iterator){
   if(iterator.currentKey() != sourceGroup)
    iterator.current()->selectAll(false);
  }

  //reset isInSelectItems to false to enable again the the emission of signals due to selectionChange
  isInSelectItems = false;

  //If no items were selected in the current group,slotClickRedraw won't be call, so to update the view correctly
  //The updateShownItems signal has to be emitted.
  if(count == 0){
   QMap<QString,QValueList<int> > selection;
   QDictIterator<ItemIconView> iterator(iconviewDict);
   for(;iterator.current();++iterator){
    QValueList<int> selectedItems;
    
    selection.insert(iterator.currentKey(),selectedItems);

    //update the browsing status, it is set to false for all the elements
    QString groupName = iterator.currentKey();
    ItemColors* itemColors = itemColorsDict[groupName];
    QMap<int,bool> browsingMap = browsingStatus[groupName];
    QValueList<int> itemsToSkip;
    for(QIconViewItem* item = iterator.current()->firstItem(); item; item = item->nextItem()){
     int currentIndex = item->index();
     if(browsingMap[currentIndex]){
      browsingMap[currentIndex] = false;
      QString label = item->text();
      redrawItem(iterator.current(),itemColors,currentIndex,browsingMap);
      isInSelectItems = true;////redrawItem sets it back to false
      item = iterator.current()->findItem(label,Qt::ExactMatch|CaseSensitive);
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
 QIconViewItem* currentItem =  iconView->findItem(label,Qt::ExactMatch|CaseSensitive);
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
  QPointArray polygon(3);
  polygon.putPoints(0,3,0,0,14,0,7,14);
  painter.setBrush(color);
  painter.drawPolygon(polygon);
 }
 painter.end();

 QIconViewItem* newItem = new QIconViewItem(iconView,currentItem,label,pixmap);
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
 QIconViewItem* currentItem = 0L;
 ItemIconView* iconView = iconviewDict[sourceGroup];
 ItemColors* itemColors = itemColorsDict[sourceGroup];
 QString label =  itemColors->itemLabel(index);
 currentItem =  iconView->findItem(label,Qt::ExactMatch|CaseSensitive);
 QValueList<int> itemsToRedraw;
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
 
 QValueList<int> itemsToSkip;
 for(QIconViewItem* item = iconView->firstItem(); item; item = item->nextItem())
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
 color.hsv(&h,&s,&v);
 QColor legendColor;
 if(s <= 80 && v >= 240 || (s <= 40 && v >= 220)) legendColor = black;
 else legendColor = white;


 QMap<QString,QValueList<int> > selected = selectedItems();
 
 QDictIterator<ItemIconView> iterator(iconviewDict);
 for(;iterator.current();++iterator){
  iterator.current()->setPaletteBackgroundColor(color);
  iterator.current()->setPaletteForegroundColor(legendColor);
  //Redraw the icons
  QValueList<int> selectedItems = selected[iterator.currentKey()];
  
  
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
   (void)new QIconViewItem(iconView,itemColors->itemLabel(i),pix);  
  }
    
  //reselect the item which were selected.
  for(QIconViewItem* item = iterator.current()->firstItem(); item; item = item->nextItem()){
    if(selectedItems.contains(itemColors->itemId(item->index()))) item->setSelected(true,true);
  }
  
  //reset isInSelectItems to false to enable again the the emission of signals due to selectionChange
  isInSelectItems = false;   
 }

 QDictIterator<ItemGroupView> iterator2(itemGroupViewDict);
 for(;iterator2.current();++iterator2){
  iterator2.current()->setPaletteBackgroundColor(color);
  iterator2.current()->setPaletteForegroundColor(legendColor);
 }

 setPaletteBackgroundColor(backgroundColor);
 setPaletteForegroundColor(legendColor);
 viewport()->setPaletteBackgroundColor(backgroundColor);
 viewport()->setPaletteForegroundColor(legendColor);
 verticalContainer->setPaletteBackgroundColor(backgroundColor);
 verticalContainer->setPaletteForegroundColor(legendColor);
     
 update();
}

void ItemPalette::changeColor(QIconViewItem* item,QString groupName){
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

void ItemPalette::selectItems(QString groupName,QValueList<int> itemsToSelect,QValueList<int> itemsToSkip){
 //Set isInSelectItems to true to prevent the emission of signals due to selectionChange
  isInSelectItems = true;
    
  QIconViewItem* currentIcon = 0L;
  ItemIconView* iconView = iconviewDict[groupName];
  iconView->selectAll(false);
  ItemColors* itemColors = itemColorsDict[groupName];
  
  //update the browsing map and rebuild the icons
  QPainter painter;
  QMap<int,bool> browsingMap = browsingStatus[groupName];
  browsingMap.clear();
  for(QIconViewItem* item = iconView->firstItem(); item; item = item->nextItem()){
   int id = itemColors->itemId(item->index()); 
   if(itemsToSkip.contains(id)) browsingMap.insert(item->index(),false);
   else browsingMap.insert(item->index(),true);
   QString label = itemColors->itemLabel(item->index());
   redrawItem(iconView,itemColors,item->index(),browsingMap);
   isInSelectItems = true;//redrawItem sets it back to false
   item = iconView->findItem(label,Qt::ExactMatch|CaseSensitive);
  }
  browsingStatus.insert(groupName,browsingMap);

  QValueList<int>::iterator itemIterator;
  for(itemIterator = itemsToSelect.begin(); itemIterator != itemsToSelect.end(); ++itemIterator){
   QString label =  itemColors->itemLabelById(*itemIterator);
   currentIcon =  iconView->findItem(label,Qt::ExactMatch|CaseSensitive);        
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
 selected = "";
 clusterGroupList.clear();
 itemGroupList.clear();
 browsingStatus.clear();
 needRedrawing.clear();
 selectionStatus.clear();

 isInSelectItems = false;
}

void ItemPalette::createGroup(QString id){  
  ItemGroupView* group = new ItemGroupView(backgroundColor,verticalContainer,id);
  GroupNameLabel* label = new GroupNameLabel(id,group,"label");

  //Set the size to 2 digits, max 99 groups
  label->setFixedWidth(labelSize);
  label->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
  label->setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed);
  QFont f("Helvetica",8);
  label->setFont(f);
  label->adjustSize();

  ItemIconView* iconView;
  QFontInfo fontInfo = QFontInfo(f);
  if(type == CLUSTER) iconView = new ItemIconView(backgroundColor,QIconView::Bottom,fontInfo.pixelSize() * 2,5,group,id);
  else iconView = new ItemIconView(backgroundColor,QIconView::Right,gridX,5,group,id);
  iconView->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);

  if(iconviewDict.count() >= 1){
   QDictIterator<ItemIconView> iterator(iconviewDict);
   iconView->resizeContents((iconviewDict[iterator.currentKey()])->contentsWidth(),2);
  }
  else iconView->adjustSize();
  
  group->setStretchFactor(label,0);
  group->setStretchFactor(iconView,200);
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
  connect(iconView,SIGNAL(mouseButtonPressed(int,QIconViewItem*,QString)),this, SLOT(slotMousePressed(int,QIconViewItem*,QString)));
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
   qHeapSort(clusterGroupList);
   selectGroupLabel(QString("%1").arg(clusterGroupList[0]));
  }
  else if(type == EVENT && itemGroupList.count() > 0){
   qHeapSort(itemGroupList);
   selectGroupLabel(itemGroupList[0]);
  }
  else  selected = "";//never reach
 }
 
}

void ItemPalette::selectGroup(QString groupName){
 if(type == CLUSTER && clusterGroupList.count() > 0){
  qHeapSort(clusterGroupList);
  if(clusterGroupList.contains(groupName.toInt())) selectGroupLabel(groupName);
  else selectGroupLabel(QString("%1").arg(clusterGroupList[0]));
 }
 else if(type == EVENT && itemGroupList.count() > 0){
  qHeapSort(itemGroupList);
  if(itemGroupList.contains(groupName)) selectGroupLabel(groupName);
  else selectGroupLabel(itemGroupList[0]);
 }
 else  selected = "";//never reach
}

void ItemPalette::selectAllItems(){
 //Set isInSelectItems to true to prevent the emission of signals due to selectionChange
 isInSelectItems = true;

 QDictIterator<ItemIconView> iterator(iconviewDict);
 for(;iterator.current();++iterator)
  iterator.current()->selectAll(true);

 QMap<QString,QValueList<int> > selection = selectedItems();
 emit updateShownItems(selection);

 //reset isInSelectItems to false to enable again the the emission of signals due to selectionChange
 isInSelectItems = false;
}

void ItemPalette::deselectAllItems(){
 //Set isInSelectItems to true to prevent the emission of signals due to selectionChange
 isInSelectItems = true;

 QMap<QString,QValueList<int> > selection;
 QDictIterator<ItemIconView> iterator(iconviewDict);
 for(;iterator.current();++iterator){
  iterator.current()->selectAll(false);
  QValueList<int> selectedItems;
  selection.insert(iterator.currentKey(),selectedItems);   
 }
  
 emit updateShownItems(selection);

 //update the browsing status, it is set to false for all the elements
 for(iterator.toFirst();iterator.current();++iterator){
  QString groupName = iterator.currentKey();
  ItemColors* itemColors = itemColorsDict[groupName];
  QMap<int,bool> browsingMap = browsingStatus[groupName];
  QValueList<int> itemsToSkip;
  for(QIconViewItem* item = iterator.current()->firstItem(); item; item = item->nextItem()){
   int currentIndex = item->index();
   if(browsingMap[currentIndex]){
    browsingMap[currentIndex] = false;
    QString label = item->text();
    redrawItem(iterator.current(),itemColors,currentIndex,browsingMap);
    isInSelectItems = true;//redrawItem sets it back to false
    item = iterator.current()->findItem(label,Qt::ExactMatch|CaseSensitive);
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

 QDictIterator<ItemGroupView> it(itemGroupViewDict);
 for(;it.current();++it) verticalContainer->removeChild(it.current());

 if(type == CLUSTER){
  qHeapSort(clusterGroupList);  
  QValueList<int>::iterator iterator;
  for(iterator = clusterGroupList.begin(); iterator != clusterGroupList.end(); ++iterator)
   verticalContainer->insertChild(itemGroupViewDict[QString("%1").arg(*iterator)]);   
 }
 else{
  qHeapSort(itemGroupList);  
  QValueList<QString>::iterator iterator;
  for(iterator = itemGroupList.begin(); iterator != itemGroupList.end(); ++iterator) 
   verticalContainer->insertChild(itemGroupViewDict[QString("%1").arg(*iterator)]);
 }

 delete spaceWidget;
 spaceWidget = new QWidget(verticalContainer);
 spaceWidget->show();
 verticalContainer->setStretchFactor(spaceWidget,2);  
}



#include "itempalette.moc"
