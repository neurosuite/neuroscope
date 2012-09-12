/***************************************************************************
                          channelPalette.cpp  -  description
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
#include "channelpalette.h"
#include "channelcolors.h"


// include files for Qt
#include <QVariant>
#include <QPainter>
#include <QLayout>
#include <QToolTip>
#include <QDebug>

#include <QPixmap>
#include <QBitmap>
#include <q3scrollview.h>
#include <Q3ListBox>
#include <QLayout>
#include <QStyle>
#include <QColorDialog>
//Added by qt3to4:
#include <QList>
#include <QLabel>
#include <QResizeEvent>



ChannelPalette::ChannelPalette(PaletteType type,QColor backgroundColor,bool edition,QWidget* parent,const char* name,Qt::WFlags fl)
    : Q3ScrollView(parent,name,fl),channelColors(0L),backgroundColor(backgroundColor),isInSelectItems(false),
      spaceWidget(0L),channelsGroups(0L),groupsChannels(0L),greyScale(false),isGroupToRemove(false),
      type(type),edit(edition),selected("")
{
    channelGroupViewDict.setAutoDelete(true);

    //Set the palette color, the foreground color depends on the background color
    int h;
    int s;
    int v;
    backgroundColor.hsv(&h,&s,&v);
    QColor legendColor;
    if(s <= 80 && v >= 240 || (s <= 40 && v >= 220)) legendColor = Qt::black;
    else legendColor = Qt::white;
    setAutoFillBackground(true);
    setPaletteBackgroundColor(backgroundColor);
    setPaletteForegroundColor(legendColor);
    setHScrollBarMode(Q3ScrollView::AlwaysOff);

    setResizePolicy(Q3ScrollView::AutoOneFit);
    verticalContainer = new Q3VBox(viewport(),"verticalContainer");
    addChild(verticalContainer);
    verticalContainer->setSpacing(5);

    QFont f("Helvetica",8);
    QFontInfo fontInfo = QFontInfo(f);
    labelSize = fontInfo.pixelSize() * 2;

    //Set the legend in the good language
    languageChange();

    verticalContainer->adjustSize();
    adjustSize();
}    


ChannelPalette::~ChannelPalette()
{
    // no need to delete child widgets, Qt does it all for us
}

void ChannelPalette::setGreyScale(bool grey){
    if(greyScale == grey) {
        return;
    }
    greyScale = grey;

    Q3IconViewItem* item = 0L;
    ChannelIconView* iconView = 0L;
    QPainter painter;
    QMap<int,int>::Iterator iterator;

    for(iterator = channelsGroups->begin(); iterator != channelsGroups->end(); ++iterator){
        int groupId = (*channelsGroups)[iterator.key()];
        iconView = iconviewDict[QString::fromLatin1("%1").arg(groupId)];
        item =  iconView->findItem(QString::fromLatin1("%1").arg(iterator.key()),Q3ListBox::ExactMatch);

        //Get the channelColor associated with the item
        QColor color = channelColors->color(iterator.key());

        //Update the icon
        QPixmap* pixmap = item->pixmap();
        drawItem(painter,pixmap,color,channelsShowHideStatus[iterator.key()],channelsSkipStatus[iterator.key()]);
        item->repaint();
    }

}

void ChannelPalette::paintEvent ( QPaintEvent*){

    //When all the channels of a group have been remove by a drag-drop it can not be suppress immediately
    //(the mouse is still in the iconView area). To make sure the groups are suppressed, isGroupToRemove
    //is set to inform that empty groups have to be suppressed and a repaint of the palette is asked.
    if(isGroupToRemove){
        deleteEmptyGroups();
        isGroupToRemove = false;
        //Inform the application that the groups have been modified
        emit groupModified();
    }

    emit paletteResized(viewport()->width(),labelSize);
}

void ChannelPalette::resizeEvent(QResizeEvent* event){
    //Make the viewport to have the visible size (size of the scrollview)
    viewport()->resize(event->size());
    Q3ScrollView::resizeEvent(event);
}

void ChannelPalette::createChannelLists(ChannelColors* channelColors,QMap<int, QList<int> >* groupsChannels,QMap<int,int>* channelsGroups){
    //Assign the channelColors, groupsChannels and channelsGroups for future use.
    this->channelColors = channelColors;
    this->groupsChannels = groupsChannels;
    this->channelsGroups = channelsGroups;

    //Create the iconViews
    QMap<int, QList<int> >::Iterator iterator;
    for(iterator = groupsChannels->begin(); iterator != groupsChannels->end(); ++iterator)
        createGroup(iterator.key());

    setChannelLists();
}

void ChannelPalette::setChannelLists(){  
    Q3DictIterator<ChannelIconView> iconviewIterator(iconviewDict);
    for(;iconviewIterator.current();++iconviewIterator)
        iconviewIterator.current()->clear();


    //Construct one icon for each channel and set the show/hide status to false
    QPainter painter;

    QMap<int, QList<int> >::Iterator iterator;
    for(iterator = groupsChannels->begin(); iterator != groupsChannels->end(); ++iterator){
        QString groupId = QString::fromLatin1("%1").arg(iterator.key());
        QList<int> channelList = iterator.data();
        for(uint i = 0; i<channelList.size(); ++i){
            //The default show/hide status is hide
            channelsShowHideStatus.insert(channelList[i],false);
            //The default skip status is not skipped
            channelsSkipStatus.insert(channelList[i],false);
            QPixmap pixmap(14,14);
            QColor color = channelColors->color(channelList[i]);
            drawItem(painter,&pixmap,color,false,false);
            (void)new ChannelIconItem(iconviewDict[groupId],(QString::fromLatin1("%1").arg(channelList[i])),pixmap);
        }
    }
}

void ChannelPalette::slotRightPressed(Q3IconViewItem* item){
    /*  if(!item) return; // right pressed on viewport
  else{
   //Create a popmenu for the traces group related items.
   QMenu menu(this);
   menu.insertTitle(tr("Manage channels colors"));

   int changeSelectedColor = menu.insertItem(tr("Change color"));
   int changeGroupColor = menu.insertItem(tr("Change group color"));

   menu.setMouseTracking(TRUE);
   int id = menu.exec(QCursor::pos());

   if(id == changeGroupColor){
    changeColor(item,false);
   }
   else if(id == changeSelectedColor){
    //Get the channelColor associated with the item
    QColor color = channelColors->color(item->text().toInt());

    int result = QColorDialog::getColor(color,0);
    if(result == QColorDialog::Accepted){
     QValueList<int> selected;
     //Change the color of the selected channels of all the groups.
     QDictIterator<ChannelIconView> iconviewIterator(iconviewDict);
     for(;iconviewIterator.current();++iconviewIterator){
      ChannelIconView* iconViewParent =  iconviewIterator.current();
      for(QIconViewItem* item = iconViewParent->firstItem(); item; item = item->nextItem()){
       if(item->isSelected()){
        int id = item->text().toInt();
        selected.append(id);

        //Update the channelColor
        channelColors->setColor(id,color);

        //Update the icon
        QPixmap* pixmap = item->pixmap();
        QPainter painter;
        drawItem(painter,pixmap,color,channelsShowHideStatus[id]);
        item->repaint();
       }
      }
     }
     emit channelsChangeColor(selected);
    }
   }
  }*/
}

void ChannelPalette::slotMousePressed(int button,Q3IconViewItem* item){
    if (!item) return; //pressed on viewport
    else{
        // middle pressed on item
        if(button == Qt::MidButton) changeColor(item);
    }
}


void ChannelPalette::slotMousePressed(QString sourceGroupName){
    if(sourceGroupName != ""){
        //If shiftKey is false, either select all the items of the group or deselect them all (it is a toggle between the 2 states)
        ChannelIconView* iconView = iconviewDict[sourceGroupName];
        bool unselect = selectionStatus[sourceGroupName];

        //Set isInSelectItems to true to prevent the emission of signals due to selectionChange
        isInSelectItems = true;

        if(unselect){
            selectionStatus[sourceGroupName] = false;
            iconView->selectAll(false);

            QList<int> selected = selectedChannels();
            if(!edit) emit updateShownChannels(selected);
            emit channelsSelected(selected);
        }
        else{
            selectionStatus[sourceGroupName] = true;
            iconView->selectAll(true);
            QList<int> selected = selectedChannels();
            if(!edit) emit updateShownChannels(selected);
            emit channelsSelected(selected);
        }
        //reset isInSelectItems to false to enable again the the emission of signals due to selectionChange
        isInSelectItems = false;
    }
}

void ChannelPalette::slotMidButtonPressed(QString sourceGroupId){
    //Change the color of the group, take the first channel of the group.
    ChannelIconView* iconView = iconviewDict[sourceGroupId];
    Q3IconViewItem* item = iconView->firstItem();
    changeColor(item,false);
}

const QList<int> ChannelPalette::selectedChannels(){
    QList<int> selectedChannels;

    Q3DictIterator<ChannelIconView> iterator(iconviewDict);
    for(;iterator.current();++iterator){
        for(Q3IconViewItem* item = iterator.current()->firstItem(); item; item = item->nextItem()){
            if(item->isSelected()){
                selectedChannels.append(item->text().toInt());
            }
        }
    }

    return selectedChannels;
}

void ChannelPalette::slotClickRedraw(){
    if(!isInSelectItems && !edit){
        QList<int> selected = selectedChannels();
        emit updateShownChannels(selected);
    }
    if(!isInSelectItems){
        QList<int> selected = selectedChannels();
        emit channelsSelected(selected);
    }
}

void ChannelPalette::slotMousePressWoModificators(QString sourceGroup){
    ChannelIconView* iconView = iconviewDict[sourceGroup];
    int count = 0;
    for(Q3IconViewItem* item = iconView->firstItem(); item; item = item->nextItem()){
        if(item->isSelected()) count++;
        if(count > 1) break;
    }
    
    if(count <= 2){
        //Set isInSelectItems to true to prevent the emission of signals due to selectionChange
        isInSelectItems = true;

        Q3DictIterator<ChannelIconView> iterator(iconviewDict);
        for(;iterator.current();++iterator){
            if(iterator.currentKey() != sourceGroup)
                iterator.current()->selectAll(false);
        }

        //reset isInSelectItems to false to enable again the the emission of signals due to selectionChange
        isInSelectItems = false;

        //Inform the TraceView that all the channels have been deselected.
        QList<int> selected;
        emit channelsSelected(selected);

        //If we are not in edit mode (selection/deselection <=> show/hide.
        //If no channels were selected in the current group,slotClickRedraw won't be call, so to update the view correctly
        //The updateShownChannels signal has to be emitted.
        if(count == 0 && !edit){
            emit updateShownChannels(selected);
        }
    }
}

void ChannelPalette::showChannels(){
    //Change the status show/hide of the selected channels
    updateShowHideStatus(true);

    emit updateShownChannels(getShowHideChannels(true));
}

void ChannelPalette::hideChannels(){
    //Change the status show/hide of the selected channels
    updateShowHideStatus(false);

    emit updateHideChannels(getShowHideChannels(false));
    emit updateShownChannels(getShowHideChannels(true));
}

void ChannelPalette::hideUnselectAllChannels(){
    //Set isInSelectItems to true to prevent the emission of signals due to selectionChange
    isInSelectItems = true;

    Q3IconViewItem* item = 0L;
    ChannelIconView* iconView = 0L;
    QPainter painter;
    QMap<int,int>::Iterator iterator;

    for(iterator = channelsGroups->begin(); iterator != channelsGroups->end(); ++iterator){
        //update the status
        channelsShowHideStatus.replace(iterator.key(),false);

        //Update the pixmap
        int groupId = (*channelsGroups)[iterator.key()];
        iconView = iconviewDict[QString::fromLatin1("%1").arg(groupId)];
        item =  iconView->findItem(QString::fromLatin1("%1").arg(iterator.key()),Q3ListBox::ExactMatch);

        //Add an item to the target group with the same text but an update icon.
        QPixmap pixmap(14,14);
        //Get the channelColor associated with the item
        QColor color = channelColors->color(iterator.key());
        drawItem(painter,&pixmap,color,false,channelsSkipStatus[iterator.key()]);
        (void)new ChannelIconItem(iconView,item,QString::fromLatin1("%1").arg(iterator.key()),pixmap);

        //Delete the old item
        delete item;
    }

    //reset isInSelectItems to false to enable again the the emission of signals due to selectionChange
    isInSelectItems = false;
}

void ChannelPalette::updateShowHideStatus(bool showStatus){

    //Get the selected channels and set them the showStatus
    const QList<int> channelIds = selectedChannels();

    updateShowHideStatus(channelIds,showStatus);
}

void ChannelPalette::updateShowHideStatus(const QList<int>& channelIds,bool showStatus){  
    //Set isInSelectItems to true to prevent the emission of signals due to selectionChange
    isInSelectItems = true;

    if(!edit){
        Q3DictIterator<ChannelIconView> iterator(iconviewDict);
        for(;iterator.current();++iterator)
            iterator.current()->selectAll(false);
    }
    
    QList<int>::const_iterator channelIterator;
    Q3IconViewItem* item = 0L;
    ChannelIconView* iconView = 0L;
    QPainter painter;
    QList<int> selectedIds;

    for(channelIterator = channelIds.begin(); channelIterator != channelIds.end(); ++channelIterator){
        //update the status
        channelsShowHideStatus.replace(*channelIterator,showStatus);
        //Update the pixmap
        int groupId = (*channelsGroups)[*channelIterator];

        iconView = iconviewDict[QString::fromLatin1("%1").arg(groupId)];
        item =  iconView->findItem(QString::fromLatin1("%1").arg(*channelIterator),Q3ListBox::ExactMatch);

        bool selected;
        if(!edit && showStatus) selected = true;
        else selected = item->isSelected();

        //Add an item to the target group with the same text but an update icon.
        QPixmap pixmap(14,14);
        //Get the channelColor associated with the item
        QColor color = channelColors->color(*channelIterator);
        drawItem(painter,&pixmap,color,showStatus,channelsSkipStatus[*channelIterator]);
        (void)new ChannelIconItem(iconView,item,QString::fromLatin1("%1").arg(*channelIterator),pixmap);

        if(selected) selectedIds.append(*channelIterator);

        //Delete the old item
        delete item;
    }

    selectChannels(selectedIds);

    //reset isInSelectItems to false to enable again the the emission of signals due to selectionChange
    isInSelectItems = false;
}

const QList<int> ChannelPalette::getShowHideChannels(bool showStatus){
    QList<int> channelIds;
    QMap<int,bool>::Iterator iterator;

    for(iterator = channelsShowHideStatus.begin(); iterator != channelsShowHideStatus.end(); ++iterator)
        if(iterator.data() == showStatus) channelIds.append(iterator.key());

    return channelIds;
}


void ChannelPalette::updateSkipStatus(const QMap<int,bool>& skipStatus){  
    //Set isInSelectItems to true to prevent the emission of signals due to selectionChange
    isInSelectItems = true;

    if(!edit){
        Q3DictIterator<ChannelIconView> iterator(iconviewDict);
        for(;iterator.current();++iterator)
            iterator.current()->selectAll(false);
    }
    
    QMap<int,bool>::const_iterator channelIterator;
    Q3IconViewItem* item = 0L;
    ChannelIconView* iconView = 0L;
    QPainter painter;
    QList<int> selectedIds;

    for(channelIterator = skipStatus.begin(); channelIterator != skipStatus.end(); ++channelIterator){
        int channelId = channelIterator.key();
        bool status = channelIterator.data();

        //update the status
        channelsSkipStatus.replace(channelId,status);

        //Update the pixmap
        int groupId = (*channelsGroups)[channelId];

        iconView = iconviewDict[QString::fromLatin1("%1").arg(groupId)];
        item =  iconView->findItem(QString::fromLatin1("%1").arg(channelId),Q3ListBox::ExactMatch);

        bool selected = item->isSelected();

        //Add an item to the target group with the same text but an update icon.
        QPixmap pixmap(14,14);
        //Get the channelColor associated with the item
        QColor color = channelColors->color(channelId);
        //set the channelColor associated with the item to the background color if the status is true
        if(status) color = backgroundColor;
        else{
            //if the status is false and the item has the background color has color change it to the group color.
            if(color == backgroundColor){
                if(type == DISPLAY) color = channelColors->groupColor(channelId);
                else color = channelColors->spikeGroupColor(channelId);
            }
        }

        channelColors->setColor(channelId,color);
        drawItem(painter,&pixmap,color,channelsShowHideStatus[channelId],status);
        (void)new ChannelIconItem(iconView,item,QString::fromLatin1("%1").arg(channelId),pixmap);

        if(selected) selectedIds.append(channelId);

        //Delete the old item
        delete item;
    }

    selectChannels(selectedIds);

    //reset isInSelectItems to false to enable again the the emission of signals due to selectionChange
    isInSelectItems = false;
}


void ChannelPalette::updateSkipStatus(const QList<int>&channelIds,bool skipStatus){
    //Set isInSelectItems to true to prevent the emission of signals due to selectionChange
    isInSelectItems = true;

    if(!edit){
        Q3DictIterator<ChannelIconView> iterator(iconviewDict);
        for(;iterator.current();++iterator)
            iterator.current()->selectAll(false);
    }
    
    QList<int>::const_iterator channelIterator;
    Q3IconViewItem* item = 0L;
    ChannelIconView* iconView = 0L;
    QPainter painter;
    QList<int> selectedIds;

    for(channelIterator = channelIds.begin(); channelIterator != channelIds.end(); ++channelIterator){
        //update the status
        channelsSkipStatus.replace(*channelIterator,skipStatus);
        //Update the pixmap
        int groupId = (*channelsGroups)[*channelIterator];

        iconView = iconviewDict[QString::fromLatin1("%1").arg(groupId)];
        item =  iconView->findItem(QString::fromLatin1("%1").arg(*channelIterator),Q3ListBox::ExactMatch);

        bool selected = item->isSelected();

        //Add an item to the target group with the same text but an update icon.
        QPixmap pixmap(14,14);
        //Get the channelColor associated with the item
        QColor color = channelColors->color(*channelIterator);

        //set the channelColor associated with the item to the background color if the status is true
        if(skipStatus) color = backgroundColor;
        else{
            //if the status is false and the item has the background color has color change it to the group color.
            if(color == backgroundColor){
                if(type == DISPLAY) color = channelColors->groupColor(*channelIterator);
                else color = channelColors->spikeGroupColor(*channelIterator);
            }
        }

        channelColors->setColor(*channelIterator,color);

        drawItem(painter,&pixmap,color,channelsShowHideStatus[*channelIterator],skipStatus);
        (void)new ChannelIconItem(iconView,item,QString::fromLatin1("%1").arg(*channelIterator),pixmap);


        if(selected) selectedIds.append(*channelIterator);

        //Delete the old item
        delete item;
    }

    selectChannels(selectedIds);

    //reset isInSelectItems to false to enable again the the emission of signals due to selectionChange
    isInSelectItems = false;
}


void ChannelPalette::updateColor(const QList<int>& channelIds){
    QList<int>::const_iterator channelIterator;
    Q3IconViewItem* item = 0L;
    ChannelIconView* iconView = 0L;
    QPainter painter;

    for(channelIterator = channelIds.begin(); channelIterator != channelIds.end(); ++channelIterator){
        int groupId = (*channelsGroups)[*channelIterator];
        iconView = iconviewDict[QString::fromLatin1("%1").arg(groupId)];
        item =  iconView->findItem(QString::fromLatin1("%1").arg(*channelIterator),Q3ListBox::ExactMatch);

        //Get the channelColor associated with the item
        QColor color = channelColors->color(*channelIterator);

        //Update the icon
        QPixmap* pixmap = item->pixmap();
        drawItem(painter,pixmap,color,channelsShowHideStatus[*channelIterator],channelsSkipStatus[*channelIterator]);
        item->repaint();
    }
}


void ChannelPalette::updateColor(int channelId){
    Q3IconViewItem* item = 0L;
    ChannelIconView* iconView = 0L;
    QPainter painter;

    int groupId = (*channelsGroups)[channelId];
    iconView = iconviewDict[QString::fromLatin1("%1").arg(groupId)];
    item =  iconView->findItem(QString::fromLatin1("%1").arg(channelId),Q3ListBox::ExactMatch);

    //Get the channelColor associated with the item
    QColor color = channelColors->color(channelId);

    //Update the icon
    QPixmap* pixmap = item->pixmap();
    drawItem(painter,pixmap,color,channelsShowHideStatus[channelId],channelsSkipStatus[channelId]);
    item->repaint();
}

void ChannelPalette::updateGroupColor(int channelId){
    ChannelIconView* iconView = 0L;
    QPainter painter;
    //Get the group Color associated with the item depending on the type. Take the the color associated
    //with the type of the other palette.
    QColor color;
    if(type == DISPLAY) color = channelColors->spikeGroupColor(channelId);
    else color = channelColors->groupColor(channelId);

    //Find the group to which channelId belongs
    int groupId = (*channelsGroups)[channelId];
    iconView = iconviewDict[QString::fromLatin1("%1").arg(groupId)];

    for(Q3IconViewItem* item = iconView->firstItem(); item; item = item->nextItem()){
        //Update the icon
        QPixmap* pixmap = item->pixmap();
        drawItem(painter,pixmap,color,channelsShowHideStatus[item->text().toInt()],channelsSkipStatus[item->text().toInt()]);
        item->repaint();
    }
}

void ChannelPalette::applyGroupColor(PaletteType paletteType){
    Q3IconViewItem* item = 0L;
    ChannelIconView* iconView = 0L;
    QPainter painter;
    QMap<int,int>::Iterator iterator;

    for(iterator = channelsGroups->begin(); iterator != channelsGroups->end(); ++iterator){
        int groupId = (*channelsGroups)[iterator.key()];
        iconView = iconviewDict[QString::fromLatin1("%1").arg(groupId)];
        item =  iconView->findItem(QString::fromLatin1("%1").arg(iterator.key()),Q3ListBox::ExactMatch);

        //Get the channelColor associated with the item
        QColor color;
        if(paletteType == SPIKE) color = channelColors->spikeGroupColor(iterator.key());
        else color = channelColors->groupColor(iterator.key());

        //Update the channelColor if the channel is not skipped
        if(!channelsSkipStatus[iterator.key()]) channelColors->setColor(iterator.key(),color);

        //Update the icon
        QPixmap* pixmap = item->pixmap();
        drawItem(painter,pixmap,color,channelsShowHideStatus[iterator.key()],channelsSkipStatus[iterator.key()]);
        item->repaint();
    }
}

void ChannelPalette::applyCustomColor(){
    Q3IconViewItem* item = 0L;
    ChannelIconView* iconView = 0L;
    QPainter painter;
    QMap<int,int>::Iterator iterator;

    for(iterator = channelsGroups->begin(); iterator != channelsGroups->end(); ++iterator){
        int groupId = (*channelsGroups)[iterator.key()];
        iconView = iconviewDict[QString::fromLatin1("%1").arg(groupId)];
        item =  iconView->findItem(QString::fromLatin1("%1").arg(iterator.key()),Q3ListBox::ExactMatch);

        //Get the channelColor associated with the item
        QColor color = channelColors->color(iterator.key());

        //Update the icon
        QPixmap* pixmap = item->pixmap();
        drawItem(painter,pixmap,color,channelsShowHideStatus[iterator.key()],channelsSkipStatus[iterator.key()]);
        item->repaint();
    }
}

void ChannelPalette::changeBackgroundColor(QColor color){
    backgroundColor = color;

    int h;
    int s;
    int v;
    color.hsv(&h,&s,&v);
    QColor legendColor;
    if(s <= 80 && v >= 240 || (s <= 40 && v >= 220)) legendColor = Qt::black;
    else legendColor = Qt::white;

    setPaletteBackgroundColor(backgroundColor);
    setPaletteForegroundColor(legendColor);

    viewport()->setPaletteBackgroundColor(backgroundColor);
    viewport()->setPaletteForegroundColor(legendColor);
    verticalContainer->setPaletteBackgroundColor(backgroundColor);
    verticalContainer->setPaletteForegroundColor(legendColor);

    Q3DictIterator<ChannelIconView> iterator(iconviewDict);
    for(;iterator.current();++iterator){
        iterator.current()->setPaletteBackgroundColor(backgroundColor);
        iterator.current()->setPaletteForegroundColor(legendColor);
    }

    Q3DictIterator<ChannelGroupView> iterator2(channelGroupViewDict);
    for(;iterator2.current();++iterator2){
        iterator2.current()->setPaletteBackgroundColor(backgroundColor);
        iterator2.current()->setPaletteForegroundColor(legendColor);
    }

    Q3IconViewItem* item = 0L;
    ChannelIconView* iconView = 0L;
    QPainter painter;
    QMap<int,int>::Iterator groupIterator;

    for(groupIterator = channelsGroups->begin(); groupIterator != channelsGroups->end(); ++groupIterator){
        int groupId = (*channelsGroups)[groupIterator.key()];
        iconView = iconviewDict[QString::fromLatin1("%1").arg(groupId)];
        item =  iconView->findItem(QString::fromLatin1("%1").arg(groupIterator.key()),Q3ListBox::ExactMatch);

        //update the color of the skip channels
        if(channelsSkipStatus[groupIterator.key()]) channelColors->setColor(groupIterator.key(),backgroundColor);

        //Get the channelColor associated with the item
        QColor color = channelColors->color(groupIterator.key());

        //Update the icon
        QPixmap* pixmap = item->pixmap();
        drawItem(painter,pixmap,color,channelsShowHideStatus[groupIterator.key()],channelsSkipStatus[groupIterator.key()]);
    }

    update();
}

void ChannelPalette::changeColor(Q3IconViewItem* item,bool single){
    int id = item->text().toInt();

    //Get the channelColor associated with the item
    const QColor oldColor = channelColors->color(id);

    QColor color = QColorDialog::getColor(oldColor,0);
    if(color.isValid()){
        if(single){
            //Update the channelColor only if the channel is not skipped
            if(!channelsSkipStatus[id]) channelColors->setColor(id,color);

            //Update the icon
            QPixmap* pixmap = item->pixmap();
            QPainter painter;
            drawItem(painter,pixmap,color,channelsShowHideStatus[id],channelsSkipStatus[id]);

            //As soon a color changes a signal is emitted.
            emit singleChangeColor(id);
        }
        else{
            //Change the color for all the other channels of the current group, depending on the PaletteType.
            ChannelIconView* iconViewParent =  dynamic_cast<ChannelIconView*>(item->iconView());
            for(Q3IconViewItem* current = iconViewParent->firstItem(); current; current = current->nextItem()){
                //Update the colors for the channel (color and group color)
                if(type == DISPLAY) channelColors->setGroupColor(current->text().toInt(),color);
                else channelColors->setSpikeGroupColor(current->text().toInt(),color);
                if(!channelsSkipStatus[current->text().toInt()]) channelColors->setColor(current->text().toInt(),color);

                //Update the icon
                QPixmap* pixmap = current->pixmap();
                QPainter painter;
                drawItem(painter,pixmap,color,channelsShowHideStatus[current->text().toInt()],channelsSkipStatus[current->text().toInt()]);
                current->repaint();
            }
            QString groupId = QString(iconViewParent->name());
            emit groupChangeColor(groupId.toInt());
        }
    }
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void ChannelPalette::languageChange()
{
    setCaption( tr( "Channel palette" ) );
}

void ChannelPalette::selectChannels(const QList<int>& selectedChannels){
    //Set isInSelectItems to true to prevent the emission of signals due to selectionChange
    isInSelectItems = true;

    //unselect all the items first
    Q3DictIterator<ChannelIconView> iterator(iconviewDict);
    for(;iterator.current();++iterator)
        iterator.current()->selectAll(false);

    //Loop on the channels to be selected
    QList<int>::const_iterator channelIterator;

    Q3IconViewItem* currentIcon = 0L;
    ChannelIconView* iconView = 0L;
    for(channelIterator = selectedChannels.begin(); channelIterator != selectedChannels.end(); ++channelIterator){
        int groupId = (*channelsGroups)[*channelIterator];
        iconView = iconviewDict[QString::fromLatin1("%1").arg(groupId)];
        currentIcon =  iconView->findItem(QString::fromLatin1("%1").arg(*channelIterator),Q3ListBox::ExactMatch);
        currentIcon->setSelected(true,true);
    }

    //Last item in selection gets focus if it exists
    if(selectedChannels.size() != 0) iconView->setCurrentItem(currentIcon);

    //reset isInSelectItems to false to enable again the the emission of signals due to selectionChange
    isInSelectItems = false;
}

void ChannelPalette::reset(){
    iconviewDict.clear();
    channelGroupViewDict.clear();
    channelsShowHideStatus.clear();
    edit = true;
    selected.clear();
    selectionStatus.clear();

    isInSelectItems = false;
}

void ChannelPalette::createGroup(int id){  
    ChannelGroupView* group = new ChannelGroupView(edit,backgroundColor,verticalContainer);
    group->setObjectName(QString::fromLatin1("%1").arg(id));
    GroupLabel* label = new GroupLabel(QString::fromLatin1("%1").arg(id),group);
    if(id == -1){
        label->setText("?");
    }
    if(id == 0){

        label->setPixmap(QPixmap(":/icons/trash"));
    }

    //Set the size to 2 digits, max 99 groups
    label->setFixedWidth(labelSize);
    label->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    label->setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed);
    QFont f("Helvetica",8);
    label->setFont(f);
    label->adjustSize();

    ChannelIconView* iconView = new ChannelIconView(backgroundColor,labelSize,15,edit,group,QString::fromLatin1("%1").arg(id));
    iconView->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);

    if(iconviewDict.count() >= 1){
        if(iconviewDict.find("1") != 0)
            iconView->resizeContents((iconviewDict["1"])->contentsWidth(),labelSize);
        //Everything is in the trash group
        else if(iconviewDict.find("0") != 0)
            iconView->resizeContents((iconviewDict["0"])->contentsWidth(),labelSize);
        //In the spike palette at the begining only the spikeTrashGroup (gpId-1) exists.
        else
            iconView->resizeContents((iconviewDict["-1"])->contentsWidth(),labelSize);
    }
    else iconView->adjustSize();

    //group->setStretchFactor(label,0);
    //group->setStretchFactor(iconView,200);
    group->setIconView(iconView);

    iconviewDict.insert(QString::fromLatin1("%1").arg(id),iconView);
    channelGroupViewDict.insert(QString::fromLatin1("%1").arg(id),group);
    selectionStatus.insert(QString::fromLatin1("%1").arg(id),false);

    group->adjustSize();
    iconView->show();
    group->show();

    if(spaceWidget){
        delete spaceWidget;
    }
    spaceWidget = new SpaceWidget(verticalContainer,edit);
    spaceWidget->show();
    verticalContainer->setStretchFactor(spaceWidget,2);

    //Signal and slot connection
    // connect(iconView,SIGNAL(contextMenuRequested(QIconViewItem* ,const QPoint&)),this, SLOT(slotRightPressed(QIconViewItem*)));
    connect(iconView,SIGNAL(selectionChanged()),this, SLOT(slotClickRedraw()));
    connect(iconView,SIGNAL(mouseButtonPressed(int,Q3IconViewItem*,const QPoint&)),this, SLOT(slotMousePressed(int,Q3IconViewItem*)));
    connect(this,SIGNAL(paletteResized(int,int)),group,SLOT(reAdjustSize(int,int)));
    connect(iconView,SIGNAL(channelsMoved(QString,Q3IconViewItem*)),this, SLOT(slotChannelsMoved(QString,Q3IconViewItem*)));
    connect(iconView,SIGNAL(channelsMoved(const QList<int>&,QString,Q3IconViewItem*)),this, SLOT(slotChannelsMoved(const QList<int>&,QString,Q3IconViewItem*)));
    connect(iconView,SIGNAL(moussePressWoModificators(QString)),this, SLOT(slotMousePressWoModificators(QString)));

    connect(label,SIGNAL(middleClickOnLabel(QString)),this, SLOT(slotMidButtonPressed(QString)));
    connect(label,SIGNAL(leftClickOnLabel(QString)),this, SLOT(slotMousePressed(QString)));

    connect(this,SIGNAL(setDragAndDrop(bool)),iconView, SLOT(setDragAndDrop(bool)));
    connect(this,SIGNAL(setDragAndDrop(bool)),group, SLOT(setDragAndDrop(bool)));
    connect(this,SIGNAL(setDragAndDrop(bool)),spaceWidget, SLOT(setDragAndDrop(bool)));
    connect(iconView,SIGNAL(dropLabel(int,int,int,int)),this, SLOT(groupToMove(int,int,int,int)));
    connect(group,SIGNAL(dropLabel(int,int,int,int)),this, SLOT(groupToMove(int,int,int,int)));
    connect(spaceWidget,SIGNAL(dropLabel(int,int,int,int)),this, SLOT(groupToMove(int,int,int,int)));
    connect(group,SIGNAL(dragObjectMoved(QPoint)),this, SLOT(slotDragLabeltMoved(QPoint)));


    if(id != 0 && id != -1 && (iconviewDict.find("0") != 0 || iconviewDict.find("-1") != 0)) moveTrashesToBottom();
}

void ChannelPalette::groupToMove(int sourceId,int targetId,int start, int destination){  
    //The trash group can not be moved
    if((sourceId == 0) || (sourceId == targetId)) return;

    ChannelIconView* sourceIconView ;
    ChannelGroupView* sourceGroup;
    QList<int> sourceChannelsIds;

    //Moving downwards
    if(destination > start){
        //Compute the targetId if it is -2 (<=> spaceWidget)
        if(targetId == -2){
            targetId = channelGroupViewDict.count();
            //Insert after the biggest group id (before the trash groups)
            if(iconviewDict.find("0") != 0) targetId--;
            if(iconviewDict.find("-1") != 0) targetId--;
        }
        else{
            ChannelGroupView* group = channelGroupViewDict[QString::fromLatin1("%1").arg(targetId)];
            int gpPosition = QWidget::mapToGlobal(group->pos()).y();
            if(gpPosition + (group->height() / 2) > destination) targetId--;

            //Insert after the biggest group id (before the trash group)
            if(targetId == 0 || targetId == -1 || targetId == -2){
                targetId = channelGroupViewDict.count();
                if(iconviewDict.find("0") != 0) targetId--;
                if(iconviewDict.find("-1") != 0) targetId--;
            }
        }

        if(targetId == sourceId) return;

        //insert after targetId
        //Rename the groups
        sourceIconView = iconviewDict.take(QString::fromLatin1("%1").arg(sourceId));
        sourceGroup = channelGroupViewDict.take(QString::fromLatin1("%1").arg(sourceId));
        sourceChannelsIds = (*groupsChannels)[sourceId];
        groupsChannels->remove(sourceId);

        for(int i = sourceId + 1; i <= targetId; i++){
            //Rename the iconView
            ChannelIconView* iconView = iconviewDict.take(QString::fromLatin1("%1").arg(i));
            iconView->setName(QString::fromLatin1("%1").arg(i - 1));
            iconviewDict.insert(QString::fromLatin1("%1").arg(i - 1),iconView);
            //Rename the ChannelGroupView and the label
            ChannelGroupView* group = channelGroupViewDict.take(QString::fromLatin1("%1").arg(i));
            group->setName(QString::fromLatin1("%1").arg(i - 1));
            QLabel* label = dynamic_cast<QLabel*>(group->child("label"));
            label->setText(QString::fromLatin1("%1").arg(i - 1));
            channelGroupViewDict.insert(QString::fromLatin1("%1").arg(i - 1),group);

            //Update the groups-channels variables
            QList<int> channelIds = (*groupsChannels)[i];
            groupsChannels->remove(i);
            groupsChannels->insert(i - 1,channelIds);

            QList<int>::iterator iterator;
            for(iterator = channelIds.begin(); iterator != channelIds.end(); ++iterator)
                channelsGroups->replace(*iterator,i - 1);
        }
    }
    else{
        //Moving upwards
        ChannelGroupView* group = channelGroupViewDict[QString::fromLatin1("%1").arg(targetId)];
        int gpPosition = QWidget::mapToGlobal(group->pos()).y();
        if((gpPosition + (group->height() / 2)) < destination) targetId++;

        if(targetId == sourceId) return;

        //insert before targetId
        //Rename the groups
        sourceIconView = iconviewDict.take(QString::fromLatin1("%1").arg(sourceId));
        sourceGroup = channelGroupViewDict.take(QString::fromLatin1("%1").arg(sourceId));
        sourceChannelsIds = (*groupsChannels)[sourceId];
        groupsChannels->remove(sourceId);

        for(int i = sourceId - 1; i >= targetId; i--){
            //Rename the iconView
            ChannelIconView* iconView = iconviewDict.take(QString::fromLatin1("%1").arg(i));
            iconView->setName(QString::fromLatin1("%1").arg(i + 1));
            iconviewDict.insert(QString::fromLatin1("%1").arg(i + 1),iconView);
            //Rename the ChannelGroupView and the label
            ChannelGroupView* group = channelGroupViewDict.take(QString::fromLatin1("%1").arg(i));
            group->setName(QString::fromLatin1("%1").arg(i + 1));
            QLabel* label = dynamic_cast<QLabel*>(group->child("label"));
            label->setText(QString::fromLatin1("%1").arg(i + 1));
            channelGroupViewDict.insert(QString::fromLatin1("%1").arg(i + 1),group);

            //Update the groups-channels variables
            QList<int> channelIds = (*groupsChannels)[i];
            groupsChannels->remove(i);
            groupsChannels->insert(i + 1,channelIds);

            QList<int>::iterator iterator;
            for(iterator = channelIds.begin(); iterator != channelIds.end(); ++iterator)
                channelsGroups->replace(*iterator,i + 1);
        }
    }

    //Rename the moved group.
    sourceIconView->setName(QString::fromLatin1("%1").arg(targetId));
    iconviewDict.insert(QString::fromLatin1("%1").arg(targetId),sourceIconView);
    //Rename the ChannelGroupView and the label
    sourceGroup->setName(QString::fromLatin1("%1").arg(targetId));
    QLabel* label = dynamic_cast<QLabel*>(sourceGroup->child("label"));
    label->setText(QString::fromLatin1("%1").arg(targetId));
    channelGroupViewDict.insert(QString::fromLatin1("%1").arg(targetId),sourceGroup);

    //Update the groups-channels variables
    groupsChannels->insert(targetId,sourceChannelsIds);

    QList<int>::iterator iterator;
    for(iterator = sourceChannelsIds.begin(); iterator != sourceChannelsIds.end(); ++iterator)
        channelsGroups->replace(*iterator,targetId);


    //Move the groups
    verticalContainer->removeChild(spaceWidget);

    Q3DictIterator<ChannelGroupView> it(channelGroupViewDict);
    for(;it.current();++it)
        verticalContainer->removeChild(it.current());

    int nbGroups = channelGroupViewDict.count();
    if(iconviewDict.find("0") != 0) nbGroups--;
    if(iconviewDict.find("-1") != 0) nbGroups--;
    for(int i = 1;i <= nbGroups;++i)
        verticalContainer->insertChild(channelGroupViewDict[QString::fromLatin1("%1").arg(i)]);

    if(iconviewDict.find("-1") != 0) verticalContainer->insertChild(channelGroupViewDict["-1"]);
    if(iconviewDict.find("0") != 0) verticalContainer->insertChild(channelGroupViewDict["0"]);

    delete spaceWidget;
    spaceWidget = new SpaceWidget(verticalContainer,edit);
    connect(this,SIGNAL(setDragAndDrop(bool)),spaceWidget, SLOT(setDragAndDrop(bool)));
    connect(spaceWidget,SIGNAL(dropLabel(int,int,int,int)),this, SLOT(groupToMove(int,int,int,int)));
    spaceWidget->show();
    verticalContainer->setStretchFactor(spaceWidget,2);

    update();
    emit groupModified();
}


void ChannelPalette::createGroup(){
    //Check if there is anything to do
    const QList<int> selectedIds = selectedChannels();
    if(selectedIds.size() == 0) return;

    int groupNb = iconviewDict.count() + 1;
    if(iconviewDict.find("0") != 0) groupNb--;
    if(iconviewDict.find("-1") != 0) groupNb--;

    createGroup(groupNb);

    emit paletteResized(viewport()->width(),labelSize);

    //Move the selected channels into the newly created group.
    moveChannels(groupNb);
}

int ChannelPalette::createEmptyGroup(){
    int groupNb = iconviewDict.count() + 1;
    if(iconviewDict.find("0") != 0) groupNb--;
    if(iconviewDict.find("-1") != 0) groupNb--;

    createGroup(groupNb);

    emit paletteResized(viewport()->width(),labelSize);

    //Add an entry in the map group-channel list
    QList<int> channels;
    groupsChannels->insert(groupNb,channels);

    return groupNb;
}

void ChannelPalette::moveChannels(int targetGroup){
    ChannelIconView* iconView = iconviewDict[QString::fromLatin1("%1").arg(targetGroup)];

    //Get the destination group color to later update the group color of the moved channels, for a new group blue is the default
    QList<int> destinationChannels = (*groupsChannels)[targetGroup];
    QColor groupColor;
    groupColor.setHsv(210,255,255);
    if(destinationChannels.size() != 0){
        if(type == DISPLAY) groupColor = channelColors->groupColor(destinationChannels[0]);
        else groupColor = channelColors->spikeGroupColor(destinationChannels[0]);
    }

    //will store the selected channels which to be moved.
    QList<int> movedChannels;
    QList<int> movedFromTrashChannels;
    QPainter painter;
    Q3DictIterator<ChannelIconView> iterator(iconviewDict);

    for(;iterator.current();++iterator){
        if(iterator.currentKey().toInt() == targetGroup) continue;
        QList<int> currentMovedChannels;
        QList<int> channelIds;
        for(Q3IconViewItem* item = iterator.current()->firstItem(); item; item = item->nextItem()){
            int channelId = item->text().toInt();
            if(item->isSelected()){
                //if the source is the trash group, keep track of it to inform the other palette
                if(iterator.currentKey() == "0") movedFromTrashChannels.append(channelId);
                movedChannels.append(channelId);
                currentMovedChannels.append(channelId);

                //Add the channel to the trash group
                QPixmap pixmap(14,14);
                QColor color = channelColors->color(channelId);
                drawItem(painter,&pixmap,color,channelsShowHideStatus[channelId],channelsSkipStatus[channelId]);
                (void)new ChannelIconItem(iconView,QString::fromLatin1("%1").arg(channelId),pixmap);

                //Modify the entry in the map channels-group
                channelsGroups->replace(channelId,targetGroup);
            }
            else channelIds.append(channelId);
        }
        //Delete the entries in the source group
        QList<int>::iterator it;
        for(it = currentMovedChannels.begin(); it != currentMovedChannels.end(); ++it){
            Q3IconViewItem* currentIcon = iterator.current()->findItem(QString::fromLatin1("%1").arg(*it),Q3ListBox::ExactMatch);
            delete currentIcon;
        }
        //Update groupsChannels
        groupsChannels->insert(iterator.currentKey().toInt(),channelIds);

        iterator.current()->arrangeItemsInGrid();
    }

    //Add/update the group entry in the map group-channel list
    QList<int> targetChannels;
    for(Q3IconViewItem* item = iconView->firstItem(); item; item = item->nextItem())
        targetChannels.append(item->text().toInt());

    groupsChannels->insert(targetGroup,targetChannels);

    iconView->arrangeItemsInGrid();

    //Update the group color, for a new group blue is the default
    QList<int>::iterator it2;
    for(it2 = movedChannels.begin(); it2 != movedChannels.end(); ++it2){
        if(type == DISPLAY) channelColors->setGroupColor(*it2,groupColor);
        else channelColors->setSpikeGroupColor(*it2,groupColor);
    }

    //Do not leave empty groups.
    deleteEmptyGroups();

    //Inform the application that the spike groups have been modified (use to warn the user at the end of the session)
    emit groupModified();

    //If the channels have been removed from the trash inform the other palette.
    if(movedFromTrashChannels.size() != 0){
        emit channelsRemovedFromTrash(movedFromTrashChannels);
    }

    update();
}


void ChannelPalette::deleteEmptyGroups(){  
    QList<int> deletedGroups;
    //First store the group to delete
    Q3DictIterator<ChannelIconView> iterator(iconviewDict);
    for(;iterator.current();++iterator){
        int groupId = iterator.currentKey().toInt();
        if(iterator.current()->count() == 0)
            deletedGroups.append(groupId);
    }

    if(deletedGroups.size() > 0){
        int nbGp = iconviewDict.count();
        int gpId;
        if(iconviewDict.find("-1") != 0) gpId = -1;
        else if(iconviewDict.find("0") != 0) gpId = 0;
        else gpId = 1;
        int minId = gpId;

        bool skipIdZero = !iconviewDict.find("0");
        for(int j = 0;j < nbGp; ++j){
            if(deletedGroups.contains(gpId)){
                deletedGroups.append(gpId);
                iconviewDict.remove(QString::fromLatin1("%1").arg(gpId));
                channelGroupViewDict.remove(QString::fromLatin1("%1").arg(gpId));
                selectionStatus.remove(QString::fromLatin1("%1").arg(gpId));
                //-1 and 0 are the trash groups, they are not renamed
                if(gpId == 0 || gpId == -1) minId++;

                groupsChannels->remove(gpId);
                if(gpId == -1 && skipIdZero){
                    gpId += 2;//from -1 to 1 directly
                    minId++;
                }
                else ++gpId;
            }
            else{
                if(gpId != minId){
                    //Rename the iconview
                    ChannelIconView* iconView = iconviewDict.take(QString::fromLatin1("%1").arg(gpId));
                    iconView->setName(QString::fromLatin1("%1").arg(minId));
                    iconviewDict.insert(QString::fromLatin1("%1").arg(minId),iconView);
                    //Rename the ChannelGroupView and the label
                    ChannelGroupView* group = channelGroupViewDict.take(QString::fromLatin1("%1").arg(gpId));
                    group->setName(QString::fromLatin1("%1").arg(minId));
                    QLabel* label = dynamic_cast<QLabel*>(group->child("label"));
                    label->setText(QString::fromLatin1("%1").arg(minId));
                    channelGroupViewDict.insert(QString::fromLatin1("%1").arg(minId),group);

                    //Update the groups-channels variables
                    QList<int> channelIds = (*groupsChannels)[gpId];
                    groupsChannels->remove(gpId);
                    groupsChannels->insert(minId,channelIds);

                    QList<int>::iterator iterator;
                    for(iterator = channelIds.begin(); iterator != channelIds.end(); ++iterator)
                        channelsGroups->replace(*iterator,minId);
                }

                if(gpId == -1 && skipIdZero){
                    gpId += 2;//from -1 to 1 directly
                    minId += 2;
                }
                else{
                    ++gpId;
                    minId++;
                }
            }
        }

        if(iconviewDict.find("0") != 0 || iconviewDict.find("-1") != 0) moveTrashesToBottom();
    }
}

void ChannelPalette::selectAllChannels(){
    //Set isInSelectItems to true to prevent the emission of signals due to selectionChange
    isInSelectItems = true;

    Q3DictIterator<ChannelIconView> iterator(iconviewDict);
    for(;iterator.current();++iterator)
        iterator.current()->selectAll(true);

    QList<int> selected = selectedChannels();
    emit channelsSelected(selected);

    //reset isInSelectItems to false to enable again the the emission of signals due to selectionChange
    isInSelectItems = false;
}

void ChannelPalette::deselectAllChannels(){
    //Set isInSelectItems to true to prevent the emission of signals due to selectionChange
    isInSelectItems = true;

    Q3DictIterator<ChannelIconView> iterator(iconviewDict);
    for(;iterator.current();++iterator)
        iterator.current()->selectAll(false);

    QList<int> selected;
    emit channelsSelected(selected);

    //reset isInSelectItems to false to enable again the the emission of signals due to selectionChange
    isInSelectItems = false;
}

void ChannelPalette::removeChannelsFromTrash(const QList<int>& channelIds){
    //Put the channels removed from the trash in a new group.
    if(type == DISPLAY){
        int targetGroup = createEmptyGroup();
        moveChannels(channelIds,"0",QString::fromLatin1("%1").arg(targetGroup));
    }
    //Put the channels removed into the spikeGroupTrash (create it if need it).
    else{
        if(iconviewDict.find("-1") == 0){
            createGroup(-1);
            emit paletteResized(viewport()->width(),labelSize);

            moveTrashesToBottom();
        }
        moveChannels(channelIds,"0","-1");
    }

}

void ChannelPalette::moveChannels(const QList<int>& channelIds,QString sourceGroup,QString targetGroup){
    QList<int> targetChannels = (*groupsChannels)[targetGroup.toInt()];
    QList<int> sourceChannels = (*groupsChannels)[sourceGroup.toInt()];

    QList<int>::const_iterator iterator;
    QPainter painter;
    ChannelIconView* sourceIconView = iconviewDict[sourceGroup];
    ChannelIconView* targetIconView = iconviewDict[targetGroup];

    //Get the target group color to later update the group color of the moved channels, for a new group blue is the default
    QColor groupColor;
    groupColor.setHsv(210,255,255);
    if(targetChannels.size() != 0){
        if(type == DISPLAY) groupColor = channelColors->groupColor(targetChannels[0]);
        else groupColor = channelColors->spikeGroupColor(targetChannels[0]);
    }

    for(iterator = channelIds.begin(); iterator != channelIds.end(); ++iterator){
        //Delete the item from the sourceGroup
        Q3IconViewItem* currentIcon = sourceIconView->findItem(QString::fromLatin1("%1").arg(*iterator),Q3ListBox::ExactMatch);
        delete currentIcon;

        //Add an item to the target group.
        QPixmap pixmap(14,14);
        QColor color = channelColors->color(*iterator);
        drawItem(painter,&pixmap,color,channelsShowHideStatus[*iterator],channelsSkipStatus[*iterator]);
        (void)new ChannelIconItem(targetIconView,QString::fromLatin1("%1").arg(*iterator),pixmap);

        //Update the group color
        if(type == DISPLAY) channelColors->setGroupColor(*iterator,groupColor);
        else channelColors->setSpikeGroupColor(*iterator,groupColor);

        sourceChannels.remove(*iterator);
        targetChannels.append(*iterator);
        channelsGroups->replace(*iterator,targetGroup.toInt());
    }

    //Modify the entry in the map group-channel list
    groupsChannels->replace(targetGroup.toInt(),targetChannels);
    targetIconView->arrangeItemsInGrid();


    //The source is empty now
    if(sourceIconView->count() == 0){
        if(type == DISPLAY) deleteEmptyGroups();//the drag has been done in the spike palette
        else{
            //When all the channels of a group have been remove by a drag-drop it can not be suppress immediately
            //(the mouse is still in the iconView area). To make sure the group is suppress, a pair of variables (isGroupToRemove,groupToRemove)
            //is set to inform that a group has to be suppress and a repaint of the palette is asked (update()).
            isGroupToRemove = true;
        }
    }
    else{
        //Modify the entries in the map group-channel list
        groupsChannels->replace(sourceGroup.toInt(),sourceChannels);

        sourceIconView->arrangeItemsInGrid();
        emit groupModified();
    }

    if(iconviewDict.find("0") != 0 || iconviewDict.find("-1") != 0) moveTrashesToBottom();

    update();
}



void ChannelPalette::slotChannelsMoved(QString targetGroup,Q3IconViewItem* after){
    //If the channels have been moved to the trash inform the other palette.
    QString afterId;
    bool beforeFirst = false;
    if(targetGroup == "0" ){
        if(after == 0){
            beforeFirst = true;
            afterId.clear();
        }
        else afterId = after->text();
    }

    //Get the destination group color to later update the group color of the moved channels, default is blue
    QList<int> destinationChannels = (*groupsChannels)[targetGroup.toInt()];
    QColor groupColor;
    groupColor.setHsv(210,255,255);
    if(destinationChannels.size() != 0){
        if(type == DISPLAY) groupColor = channelColors->groupColor(destinationChannels[0]);
        else groupColor = channelColors->spikeGroupColor(destinationChannels[0]);
    }

    ChannelIconView* targetIconView = iconviewDict[targetGroup];
    //deselect the item from the target group
    targetIconView->selectAll(false);

    //If the items have to be inserted before the first item, insert them after the first item
    //and then move the first item after the others
    bool moveFirst = false;
    if(after == 0){
        after = targetIconView->firstItem();
        moveFirst = true;
    }

    //will store the selected channels which to be moved.
    QList<int> movedChannels;
    QPainter painter;

    int nbGp = iconviewDict.count();
    int gpId;
    if(iconviewDict.find("-1") != 0) gpId = -1;
    else if(iconviewDict.find("0") != 0) gpId = 0;
    else gpId = 1;

    bool skipIdZero = !iconviewDict.find("0");
    for(int j = 0;j < nbGp; ++j){
        if(gpId == targetGroup.toInt()){
            if(gpId == -1 && skipIdZero) gpId += 2;//from -1 to 1 directly
            else ++gpId;
            continue;
        }

        QList<int> currentMovedChannels;
        QList<int> channelIds;
        ChannelIconView* iconView = iconviewDict[QString::fromLatin1("%1").arg(gpId)];
        for(Q3IconViewItem* item = iconView->firstItem(); item; item = item->nextItem()){
            int channelId = item->text().toInt();

            if(item->isSelected()){
                movedChannels.append(channelId);
                currentMovedChannels.append(channelId);

                //Add the channel to the target group
                QPixmap pixmap(14,14);
                QColor color = channelColors->color(channelId);
                if(targetGroup == "0") channelsShowHideStatus[channelId] = false;
                drawItem(painter,&pixmap,color,channelsShowHideStatus[channelId],channelsSkipStatus[channelId]);
                after = new ChannelIconItem(targetIconView,after,QString::fromLatin1("%1").arg(channelId),pixmap);

                //Modify the entry in the map channels-group
                channelsGroups->replace(channelId,targetGroup.toInt());
            }
            else channelIds.append(channelId);
        }

        //Delete the entries in the source group
        QList<int>::iterator it;
        for(it = currentMovedChannels.begin(); it != currentMovedChannels.end(); ++it){
            Q3IconViewItem* currentIcon = iconView->findItem(QString::fromLatin1("%1").arg(*it),Q3ListBox::ExactMatch);
            delete currentIcon;
        }
        //Update groupsChannels
        groupsChannels->insert(gpId,channelIds);
        iconView->arrangeItemsInGrid();

        //If the channels have been removed from the trash inform the other palette.
        if(gpId == 0 && currentMovedChannels.size() != 0){
            emit channelsRemovedFromTrash(currentMovedChannels);
        }

        if(gpId == -1 && skipIdZero) gpId += 2;//from -1 to 1 directly
        else ++gpId;
    }

    if(moveFirst){
        Q3IconViewItem* first = targetIconView->firstItem();
        QString channelId = first->text();

        delete first;

        //Add a new item corresponding to the channel Id.
        QPixmap pixmap(14,14);
        QColor color = channelColors->color(channelId.toInt());
        drawItem(painter,&pixmap,color,channelsShowHideStatus[channelId.toInt()],channelsSkipStatus[channelId.toInt()]);
        (void)new ChannelIconItem(targetIconView,after,channelId,pixmap);
    }

    //Modify the entry in the map group-channel list
    QList<int> targetChannels;
    for(Q3IconViewItem* item = targetIconView->firstItem(); item; item = item->nextItem())
        targetChannels.append(item->text().toInt());

    groupsChannels->replace(targetGroup.toInt(),targetChannels);

    targetIconView->arrangeItemsInGrid();

    //Update the group color, for a new group blue is the default
    QList<int>::iterator it2;
    for(it2 = movedChannels.begin(); it2 != movedChannels.end(); ++it2){
        if(type == DISPLAY) channelColors->setGroupColor(*it2,groupColor);
        else channelColors->setSpikeGroupColor(*it2,groupColor);
    }


    //Do not leave empty groups.
    isGroupToRemove = true;

    //If the channels have been moved to the trash inform the other palette.
    if(targetGroup == "0") emit channelsMovedToTrash(movedChannels,afterId,beforeFirst);

    update();
}


void ChannelPalette::trashChannelsMovedAround(const QList<int>& channelIds,QString afterId,bool beforeFirst){
    Q3IconViewItem* after;
    ChannelIconView* trash = iconviewDict["0"];
    //If the items have to be moved before the first item, insert them after the first item
    //and then move the first item after the others
    if(beforeFirst) after = 0;
    else after = trash->findItem(afterId,Q3ListBox::ExactMatch);

    //Actually move the channels
    moveChannels(channelIds,"0",after);

    update();
}

void ChannelPalette::moveChannels(const QList<int>& channelIds,QString sourceGroup,Q3IconViewItem* after){
    QList<int>::const_iterator iterator;
    QPainter painter;
    ChannelIconView* iconView = iconviewDict[sourceGroup];

    //If the items have to be moved before the first item, insert them after the first item
    //and then move the first item after the others
    bool moveFirst = false;
    if(after == 0){
        after = iconView->firstItem();
        moveFirst = true;
    }
    for(iterator = channelIds.begin(); iterator != channelIds.end(); ++iterator){
        //Delete the item corresponding to the channel Id
        Q3IconViewItem* currentIcon = iconView->findItem(QString::fromLatin1("%1").arg(*iterator),Q3ListBox::ExactMatch);
        delete currentIcon;

        //Add a new item corresponding to the channel Id.
        QPixmap pixmap(14,14);
        QColor color = channelColors->color(*iterator);
        drawItem(painter,&pixmap,color,channelsShowHideStatus[*iterator],channelsSkipStatus[*iterator]);
        after = new ChannelIconItem(iconView,after,QString::fromLatin1("%1").arg(*iterator),pixmap);
    }
    if(moveFirst){
        Q3IconViewItem* first = iconView->firstItem();
        QString channelId = first->text();
        delete first;

        //Add a new item corresponding to the channel Id.
        QPixmap pixmap(14,14);
        QColor color = channelColors->color(channelId.toInt());
        drawItem(painter,&pixmap,color,channelsShowHideStatus[channelId.toInt()],channelsSkipStatus[channelId.toInt()]);
        after = new ChannelIconItem(iconView,after,channelId,pixmap);
    }

    iconView->arrangeItemsInGrid();

    //Modify the entry in the map group-channel list
    QList<int> sourceChannels;
    for(Q3IconViewItem* item = iconView->firstItem(); item; item = item->nextItem())
        sourceChannels.append(item->text().toInt());

    groupsChannels->replace(sourceGroup.toInt(),sourceChannels);
}

void ChannelPalette::slotChannelsMoved(const QList<int>& channelIds,QString sourceGroup,Q3IconViewItem* after){
    //If the channels have been moved around in the trash, inform the other palette.
    if(sourceGroup == "0" ){
        QString afterId;
        bool beforeFirst = false;
        if(after == 0){
            beforeFirst = true;
            afterId.clear();
        }
        else afterId = after->text();

        emit channelsMovedAroundInTrash(channelIds,afterId,beforeFirst);
    }

    //Actually move the channels
    moveChannels(channelIds,sourceGroup,after);

    //Inform the application that the spike groups have been modified (use to warn the user at the end of the session)
    emit groupModified();

    update();
}


void ChannelPalette::discardChannels(){
    trashChannels(0);
}

void ChannelPalette::discardChannels(const QList<int>& channelsToDiscard){    
    //Get the destination group color to later update the group color of the moved channels, default is blue
    QColor groupColor;
    groupColor.setHsv(210,255,255);

    //Check if a trash group exists, if not create it.
    if(iconviewDict.find("0") == 0){
        createGroup(0);
        moveTrashesToBottom();
    }
    else{
        QList<int> trashChannels = (*groupsChannels)[0];
        if(trashChannels.size() != 0){
            if(type == DISPLAY) groupColor = channelColors->groupColor(trashChannels[0]);
            else groupColor = channelColors->spikeGroupColor(trashChannels[0]);
        }
    }
    ChannelIconView* trash = iconviewDict["0"];

    emit paletteResized(viewport()->width(),labelSize);

    //Set isInSelectItems to true to prevent the emission of signals due to selectionChange
    isInSelectItems = true;

    QPainter painter;
    Q3DictIterator<ChannelIconView> iterator(iconviewDict);

    for(;iterator.current();++iterator){
        if(iterator.currentKey() == "0"){
            for(Q3IconViewItem* item = iterator.current()->firstItem(); item; item = item->nextItem()){
                int channelId = item->text().toInt();
                if(channelsToDiscard.contains(channelId)){
                    channelsShowHideStatus[channelId] = false;
                    //Update the icon
                    QPixmap* pixmap = item->pixmap();
                    QColor color = channelColors->color(channelId);
                    drawItem(painter,pixmap,color,false,channelsSkipStatus[channelId]);
                    item->repaint();
                }
            }
            continue;
        }
        QList<int> currentDiscardedChannels;
        QList<int> channelIds;
        for(Q3IconViewItem* item = iterator.current()->firstItem(); item; item = item->nextItem()){
            int channelId = item->text().toInt();
            if(channelsToDiscard.contains(channelId)){
                currentDiscardedChannels.append(channelId);

                //Modify the entry in the map channels-group
                channelsGroups->replace(channelId,0);
            }
            else channelIds.append(channelId);
        }
        //Delete the entries in the source group
        QList<int>::iterator it;
        for(it = currentDiscardedChannels.begin(); it != currentDiscardedChannels.end(); ++it){
            Q3IconViewItem* currentIcon = iterator.current()->findItem(QString::fromLatin1("%1").arg(*it),Q3ListBox::ExactMatch);
            delete currentIcon;
        }
        //Update groupsChannels
        groupsChannels->insert(iterator.currentKey().toInt(),channelIds);

        iterator.current()->arrangeItemsInGrid();
    }

    QList<int> trashChannels = (*groupsChannels)[0];
    QList<int>::const_iterator channelIterator;
    for(channelIterator = channelsToDiscard.begin(); channelIterator != channelsToDiscard.end(); ++channelIterator){
        if(!trashChannels.contains(*channelIterator)){
            //Add the channel to the trash group and hide it.
            QPixmap pixmap(14,14);
            QColor color = channelColors->color(*channelIterator);
            channelsShowHideStatus[*channelIterator] = false;
            drawItem(painter,&pixmap,color,false,channelsSkipStatus[*channelIterator]);
            (void)new ChannelIconItem(trash,QString::fromLatin1("%1").arg(*channelIterator),pixmap);

            //Update the group color
            if(type == DISPLAY) channelColors->setGroupColor(*channelIterator,groupColor);
            else channelColors->setSpikeGroupColor(*channelIterator,groupColor);

            trashChannels.append(*channelIterator);
        }
    }

    //Add/update the 0 entry in the map group-channel list
    groupsChannels->replace(0,trashChannels);
    trash->arrangeItemsInGrid();

    //Do not leave empty groups.
    deleteEmptyGroups();

    //reset isInSelectItems to false to enable again the the emission of signals due to selectionChange
    isInSelectItems = false;

    update();
}

void ChannelPalette::discardChannels(const QList<int>& channelsToDiscard,QString afterId,bool beforeFirst){
    Q3IconViewItem* after;
    ChannelIconView* trash = iconviewDict["0"];
    //If the items have to be moved before the first item, insert them after the first item
    //and then move the first item after the others
    bool moveFirst = false;
    if(beforeFirst){
        after = trash->firstItem();
        moveFirst = true;
    }
    else after = trash->findItem(afterId,Q3ListBox::ExactMatch);

    //Get the destination group color to later update the group color of the moved channels, default is blue
    QColor groupColor;
    groupColor.setHsv(210,255,255);
    QList<int> destinationChannels = (*groupsChannels)[0];
    if(destinationChannels.size() != 0){
        if(type == DISPLAY) groupColor = channelColors->groupColor(destinationChannels[0]);
        else groupColor = channelColors->spikeGroupColor(destinationChannels[0]);
    }

    QList<int>::const_iterator channelIterator;
    Q3IconViewItem* currentIcon = 0L;
    ChannelIconView* iconView = 0L;
    QPainter painter;
    for(channelIterator = channelsToDiscard.begin(); channelIterator != channelsToDiscard.end(); ++channelIterator){
        int groupId = (*channelsGroups)[*channelIterator];
        QList<int> sourceChannels = (*groupsChannels)[groupId];
        iconView = iconviewDict[QString::fromLatin1("%1").arg(groupId)];
        currentIcon =  iconView->findItem(QString::fromLatin1("%1").arg(*channelIterator),Q3ListBox::ExactMatch);
        delete currentIcon;

        //Add a new item corresponding to the channel Id. The channel is hidden.
        QPixmap pixmap(14,14);
        QColor color = channelColors->color(*channelIterator);
        channelsShowHideStatus[*channelIterator] = false;
        drawItem(painter,&pixmap,color,false,channelsSkipStatus[*channelIterator]);
        after = new ChannelIconItem(trash,after,QString::fromLatin1("%1").arg(*channelIterator),pixmap);
        sourceChannels.remove(*channelIterator);
        channelsGroups->replace(*channelIterator,0);

        //Update the group color
        if(type == DISPLAY) channelColors->setGroupColor(*channelIterator,groupColor);
        else channelColors->setSpikeGroupColor(*channelIterator,groupColor);

        //Modify the entries in the map group-channel list
        groupsChannels->replace(groupId,sourceChannels);

        iconView->arrangeItemsInGrid();
    }

    if(moveFirst){
        Q3IconViewItem* first = trash->firstItem();
        QString channelId = first->text();
        delete first;

        //Add a new item corresponding to the channel Id.
        QPixmap pixmap(14,14);
        QColor color = channelColors->color(channelId.toInt());
        drawItem(painter,&pixmap,color,channelsShowHideStatus[channelId.toInt()],channelsSkipStatus[channelId.toInt()]);
        after = new ChannelIconItem(trash,after,channelId,pixmap);
    }

    //Modify the entry in the map group-channel list
    QList<int> trashChannels;
    for(Q3IconViewItem* item = trash->firstItem(); item; item = item->nextItem())
        trashChannels.append(item->text().toInt());
    groupsChannels->replace(0,trashChannels);

    trash->arrangeItemsInGrid();

    //Do not leave empty groups.
    deleteEmptyGroups();

    moveTrashesToBottom();

    //Update the display as the trash channels are hidden
    emit updateShownChannels(getShowHideChannels(true));
}

void ChannelPalette::setEditMode(bool edition){  
    //Set isInSelectItems to true to prevent the emission of signals due to selectionChange
    isInSelectItems = true;

    edit = edition;
    emit setDragAndDrop(edition);

    if(edition) channelsShowHideStatus.clear();

    //Update the item icons
    Q3IconViewItem* item = 0L;
    ChannelIconView* iconView = 0L;
    QPainter painter;
    QMap<int,int>::Iterator iterator;
    QList<int> selectedIds;

    for(iterator = channelsGroups->begin(); iterator != channelsGroups->end(); ++iterator){
        int groupId = (*channelsGroups)[iterator.key()];
        iconView = iconviewDict[QString::fromLatin1("%1").arg(groupId)];
        item =  iconView->findItem(QString::fromLatin1("%1").arg(iterator.key()),Q3ListBox::ExactMatch);

        bool selected = false;
        if(edition){
            selected = item->isSelected();
            channelsShowHideStatus.insert(iterator.key(),selected);
        }
        else selected = channelsShowHideStatus[iterator.key()];

        delete item;

        //Add an item to the target group.
        QPixmap pixmap(14,14);
        //Get the channelColor associated with the item
        QColor color = channelColors->color(iterator.key());
        drawItem(painter,&pixmap,color,selected,channelsSkipStatus[iterator.key()]);
        (void)new ChannelIconItem(iconView,QString::fromLatin1("%1").arg(iterator.key()),pixmap);

        if(selected) selectedIds.append(iterator.key());
    }

    selectChannels(selectedIds);

    //reset isInSelectItems to false to enable again the the emission of signals due to selectionChange
    isInSelectItems = false;
}

void ChannelPalette::drawItem(QPainter& painter,QPixmap* pixmap,QColor color,bool show,bool skip){
    pixmap->fill(backgroundColor);
    painter.begin(pixmap);
    if(greyScale){
        int greyvalue = qGray(color.rgb());
        color.setHsv(0,0,greyvalue);
    }
    if(skip){
        color = backgroundColor;
        painter.setPen(backgroundColor);
    }
    if(edit){
        painter.setPen(color);
        painter.setBrush(color);
        painter.drawEllipse(QRect(0,0,12,12));
        if(!show){
            painter.setPen(backgroundColor);
            painter.setBrush(backgroundColor);
            painter.drawEllipse(QRect(3,3,6,6));
        }
    }
    else painter.fillRect(0,0,12,12,color);
    painter.end();
}

void ChannelPalette::moveTrashesToBottom(){
    qDebug()<<" spaceWidget"<<spaceWidget;
#if KDAB_PORTING
    //Remove all the children of the verticalContainer (spaceWidget and groups)
    verticalContainer->removeChild(spaceWidget);

    Q3DictIterator<ChannelGroupView> it(channelGroupViewDict);
    for(;it.current();++it) verticalContainer->removeChild(it.current());

    //Insert all the groups except the trashes which go at the bottom
    int nbGroup = channelGroupViewDict.count();
    if(iconviewDict.find("0") != 0) nbGroup--;
    if(iconviewDict.find("-1") != 0) nbGroup--;

    for(int i = 1;i <= nbGroup;++i)
        verticalContainer->insertChild(channelGroupViewDict[QString::fromLatin1("%1").arg(i)]);

    //Insert the trashes
    if(iconviewDict.find("-1") != 0) verticalContainer->insertChild(channelGroupViewDict["-1"]);
    if(iconviewDict.find("0") != 0) verticalContainer->insertChild(channelGroupViewDict["0"]);

    delete spaceWidget;
    spaceWidget = new SpaceWidget(verticalContainer,edit);
    connect(this,SIGNAL(setDragAndDrop(bool)),spaceWidget, SLOT(setDragAndDrop(bool)));
    connect(spaceWidget,SIGNAL(dropLabel(int,int,int,int)),this, SLOT(groupToMove(int,int,int,int)));
    spaceWidget->show();
    verticalContainer->setStretchFactor(spaceWidget,2);
#endif
}


void ChannelPalette::discardSpikeChannels(){
    trashChannels(-1);
}


void ChannelPalette::trashChannels(int destinationGroup){
    //Set isInSelectItems to true to prevent the emission of signals due to selectionChange
    isInSelectItems = true;

    //Check if there is anything to do
    const QList<int> selectedIds = selectedChannels();
    if(selectedIds.size() == 0) return;

    //Check if the destination group exists, if not create it.
    ChannelIconView* trash;
    if(destinationGroup == 0){
        if(iconviewDict.find("0") == 0){
            createGroup(0);
            moveTrashesToBottom();
        }
        trash = iconviewDict["0"];
    }
    else{
        if(iconviewDict.find("-1") == 0){
            createGroup(-1);
            moveTrashesToBottom();
        }
        trash = iconviewDict["-1"];
    }

    emit paletteResized(viewport()->width(),labelSize);

    //Get the destination group colors to later update the group colors of the moved channels, default is blue
    QColor groupColor;
    groupColor.setHsv(210,255,255);
    QList<int> destinationChannels = (*groupsChannels)[0];
    if(destinationChannels.size() != 0){
        if(type == DISPLAY) groupColor = channelColors->groupColor(destinationChannels[0]);
        else groupColor = channelColors->spikeGroupColor(destinationChannels[0]);
    }

    //Will store the selected channels which to be discarded.
    QList<int> discardedChannels;
    //Get the channels which are part of the trash group.
    QList<int> trashChannels;
    QPainter painter;
    Q3DictIterator<ChannelIconView> iterator(iconviewDict);

    for(;iterator.current();++iterator){
        if((iterator.currentKey() == "0" && destinationGroup == 0) ||
                (iterator.currentKey() == "-1" && destinationGroup == -1)){
            for(Q3IconViewItem* item = iterator.current()->firstItem(); item; item = item->nextItem()){
                int channelId = item->text().toInt();
                if(trashChannels.contains(channelId)) continue;
                if(item->isSelected() && destinationGroup == 0){
                    channelsShowHideStatus[channelId] = false;
                    //Update the icon
                    QPixmap* pixmap = item->pixmap();
                    QColor color = channelColors->color(channelId);
                    drawItem(painter,pixmap,color,false,channelsSkipStatus[channelId]);
                    item->repaint();
                    //Unselect the item to be coherent with the other ways of moving items
                    item->setSelected(false);
                    discardedChannels.append(channelId);
                }
                else if(item->isSelected() && destinationGroup == -1) item->setSelected(false);
                trashChannels.append(channelId);
            }
            continue;
        }

        QList<int> currentDiscardedChannels;
        QList<int> channelIds;
        for(Q3IconViewItem* item = iterator.current()->firstItem(); item; item = item->nextItem()){
            int channelId = item->text().toInt();
            if(item->isSelected()){
                discardedChannels.append(channelId);
                trashChannels.append(channelId);
                currentDiscardedChannels.append(channelId);
                //Add the channel to the trash group
                QPixmap pixmap(14,14);
                QColor color = channelColors->color(channelId);
                if(destinationGroup == 0) channelsShowHideStatus[channelId] = false;
                drawItem(painter,&pixmap,color,channelsShowHideStatus[channelId],channelsSkipStatus[channelId]);
                (void)new ChannelIconItem(trash,QString::fromLatin1("%1").arg(channelId),pixmap);

                //Modify the entry in the map channels-group
                channelsGroups->replace(channelId,destinationGroup);
            }
            else channelIds.append(channelId);
        }
        //Delete the entries in the source group
        QList<int>::iterator it;
        for(it = currentDiscardedChannels.begin(); it != currentDiscardedChannels.end(); ++it){
            Q3IconViewItem* currentIcon = iterator.current()->findItem(QString::fromLatin1("%1").arg(*it),Q3ListBox::ExactMatch);
            delete currentIcon;
        }
        //Update groupsChannels
        groupsChannels->insert(iterator.currentKey().toInt(),channelIds);

        iterator.current()->arrangeItemsInGrid();
        if(destinationGroup == -1 && iterator.currentKey() == "0" && currentDiscardedChannels.size() != 0){
            emit channelsRemovedFromTrash(currentDiscardedChannels);
        }
    }

    //Add/update the 0 entry in the map group-channel list
    groupsChannels->insert(destinationGroup,trashChannels);
    trash->arrangeItemsInGrid();

    //Update the group color, for a new group blue is the default
    QList<int>::iterator it2;
    for(it2 = discardedChannels.begin(); it2 != discardedChannels.end(); ++it2){
        if(type == DISPLAY) channelColors->setGroupColor(*it2,groupColor);
        else channelColors->setSpikeGroupColor(*it2,groupColor);
    }

    //Do not leave empty groups.
    deleteEmptyGroups();

    update();

    if(destinationGroup == 0){
        emit channelsDiscarded(discardedChannels);

        //Update the display as the trash channels are hidden
        emit updateShownChannels(getShowHideChannels(true));
    }
    if(destinationGroup == -1){
        QList<int> selected;
        emit channelsSelected(selected);
    }

    //reset isInSelectItems to false to enable again the the emission of signals due to selectionChange
    isInSelectItems = false;
}


#include "channelpalette.moc"
