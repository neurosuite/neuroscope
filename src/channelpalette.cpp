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
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/



// application specific includes
#include "channelpalette.h"
#include "channelcolors.h"
#include "channelmimedata.h"

// include files for Qt
#include <QVariant>
#include <QPainter>
#include <QLayout>
#include <QToolTip>
#include <QDebug>

#include <QPixmap>
#include <QBitmap>
#include <QLayout>
#include <QStyle>
#include <QColorDialog>

#include <QList>
#include <QLabel>
#include <QResizeEvent>
#include <QTimer>


ChannelPalette::ChannelPalette(PaletteType type,const QColor& backgroundColor,bool edition,QWidget* parent,const char* name)
    : QScrollArea(parent)
    ,channelColors(0L)
    ,backgroundColor(backgroundColor)
    ,isInSelectItems(false),
      spaceWidget(0L)
    ,channelsGroups(0L)
    ,groupsChannels(0L)
    ,greyScale(false)
    ,isGroupToRemove(false)
    ,type(type)
    ,edit(edition)
{
    setObjectName(name);
    setWidgetResizable(true);
    //Set the palette color, the foreground color depends on the background color
    int h;
    int s;
    int v;
    backgroundColor.getHsv(&h,&s,&v);
    QColor legendColor;
    if(s <= 80 && v >= 240 || (s <= 40 && v >= 220))
        legendColor = Qt::black;
    else
        legendColor = Qt::white;
    setAutoFillBackground(true);
    QPalette palette;
    viewport()->setAutoFillBackground(false);
    palette.setColor(backgroundRole(), backgroundColor);
    palette.setColor(foregroundRole(), legendColor);
    setPalette(palette);
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setFrameStyle(QFrame::NoFrame);
    QWidget *w = new QWidget;
    verticalContainer = new QVBoxLayout;
    w->setLayout(verticalContainer);

    setWidget(w);
    verticalContainer->setSpacing(5);

    QFont f("Helvetica",8);
    QFontInfo fontInfo = QFontInfo(f);
    labelSize = fontInfo.pixelSize() * 2;

    //Set the legend in the good language
    languageChange();

    adjustSize();
}    


ChannelPalette::~ChannelPalette()
{
    qDeleteAll(channelGroupViewDict);
    channelGroupViewDict.clear();
    // no need to delete child widgets, Qt does it all for us
}

void ChannelPalette::setGreyScale(bool grey){
    if(greyScale == grey) {
        return;
    }
    greyScale = grey;

    ChannelIconView* iconView = 0L;
    QPainter painter;
    QMap<int,int>::Iterator iterator;
    QMap<int,int>::Iterator end(channelsGroups->end());
    for(iterator = channelsGroups->begin(); iterator != end; ++iterator){
        int groupId = (*channelsGroups)[iterator.key()];
        iconView = iconviewDict[QString::number(groupId)];
        QListWidgetItem * item = 0L;
        QList<QListWidgetItem*>lstItem =  iconView->findItems(QString::number(iterator.key()),Qt::MatchExactly);
        if(!lstItem.isEmpty())
            item = lstItem.first();
        else
            continue;

        //Get the channelColor associated with the item
        const QColor color = channelColors->color(iterator.key());

        //Update the icon
        QIcon icon = item->icon();
        QPixmap pixmap(icon.pixmap(QSize(22,22)).size());
        drawItem(painter,&pixmap,color,channelsShowHideStatus[iterator.key()],channelsSkipStatus[iterator.key()]);
        item->setIcon(QIcon(pixmap));
    }

}

void ChannelPalette::paintEvent ( QPaintEvent*){

    //When all the channels of a group have been remove by a drag-drop it can not be suppress immediately
    //(the mouse is still in the iconView area). To make sure the groups are suppressed, isGroupToRemove
    //is set to inform that empty groups have to be suppressed and a repaint of the palette is asked.
    if(isGroupToRemove){
        QTimer::singleShot(100, this, SLOT(deleteEmptyGroups()));
        //deleteEmptyGroups();
        isGroupToRemove = false;
        //Inform the application that the groups have been modified
        emit groupModified();
    }

    emit paletteResized(viewport()->width(),labelSize);
}

void ChannelPalette::resizeEvent(QResizeEvent* event){
    QScrollArea::resizeEvent(event);
    emit paletteResized(viewport()->width(),labelSize);
}

void ChannelPalette::createChannelLists(ChannelColors* channelColors,QMap<int, QList<int> >* groupsChannels,QMap<int,int>* channelsGroups){
    //Assign the channelColors, groupsChannels and channelsGroups for future use.
    this->channelColors = channelColors;
    this->groupsChannels = groupsChannels;
    this->channelsGroups = channelsGroups;

    //Create the iconViews
    QMap<int, QList<int> >::ConstIterator iterator;
    QMap<int, QList<int> >::ConstIterator end(groupsChannels->constEnd());
    for(iterator = groupsChannels->constBegin(); iterator != end; ++iterator)
        createGroup(iterator.key());

    setChannelLists();
}

void ChannelPalette::setChannelLists()
{
    QHashIterator<QString, ChannelIconView*> iteratordict(iconviewDict);
    while (iteratordict.hasNext()) {
        iteratordict.next();
        iteratordict.value()->clear();
    }

    //Construct one icon for each channel and set the show/hide status to false
    QPainter painter;

    QMap<int, QList<int> >::Iterator iterator;
    QMap<int, QList<int> >::Iterator end(groupsChannels->end());
    for(iterator = groupsChannels->begin(); iterator != end; ++iterator){
        QString groupId = QString::number(iterator.key());
        QList<int> channelList = iterator.value();
        for(uint i = 0; i<channelList.size(); ++i){
            //The default show/hide status is hide
            channelsShowHideStatus.insert(channelList.at(i),false);
            //The default skip status is not skipped
            channelsSkipStatus.insert(channelList.at(i),false);
            QPixmap pixmap(14,14);
            QColor color = channelColors->color(channelList.at(i));
            drawItem(painter,&pixmap,color,false,false);
            QIcon icon(pixmap);

            new ChannelIconViewItem(icon,(QString::number(channelList.at(i))),iconviewDict[groupId]);
        }
    }
}

void ChannelPalette::slotMousePressMiddleButton(QListWidgetItem*item) {
    if (!item) {
        return; //pressed on viewport
    } else {
        changeColor(item);
    }
}


void ChannelPalette::slotMousePressed(const QString& sourceGroupName){
    if(!sourceGroupName.isEmpty()){
        //If shiftKey is false, either select all the items of the group or deselect them all (it is a toggle between the 2 states)
        ChannelIconView* iconView = iconviewDict[sourceGroupName];
        bool unselect = selectionStatus[sourceGroupName];

        //Set isInSelectItems to true to prevent the emission of signals due to selectionChange
        isInSelectItems = true;

        if(unselect){
            selectionStatus[sourceGroupName] = false;
            iconView->clearSelection();

            QList<int> selected = selectedChannels();
            if(!edit)
                emit updateShownChannels(selected);
            emit channelsSelected(selected);
        }
        else{
            selectionStatus[sourceGroupName] = true;
            iconView->selectAll();
            QList<int> selected = selectedChannels();
            if(!edit)
                emit updateShownChannels(selected);
            emit channelsSelected(selected);
        }
        //reset isInSelectItems to false to enable again the the emission of signals due to selectionChange
        isInSelectItems = false;
    }
}

void ChannelPalette::slotMidButtonPressed(const QString &sourceGroupId){
    //Change the color of the group, take the first channel of the group.
    ChannelIconView* iconView = iconviewDict[sourceGroupId];
    if(iconView->count() > 0) {
        QListWidgetItem* item = iconView->item(0);
        changeColor(item,false);
    }
}

const QList<int> ChannelPalette::selectedChannels(){
    QList<int> selectedChannels;
    QHashIterator<QString, ChannelIconView*> iteratordict(iconviewDict);
    while (iteratordict.hasNext()) {
        iteratordict.next();
        const int count(iteratordict.value()->count());
        for(int i = 0; i < count; ++i) {
            QListWidgetItem* item = iteratordict.value()->item(i);
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


void ChannelPalette::showChannels(){
    //Change the status show/hide of the selected channels
    updateShowHideStatus(true);

    emit updateShownChannels(getShowHideChannels(true));
}

void ChannelPalette::hideChannels()
{
    //Change the status show/hide of the selected channels
    updateShowHideStatus(false);

    emit updateHideChannels(getShowHideChannels(false));
    emit updateShownChannels(getShowHideChannels(true));
}

void ChannelPalette::hideUnselectAllChannels(){
    //Set isInSelectItems to true to prevent the emission of signals due to selectionChange
    isInSelectItems = true;

    ChannelIconView* iconView = 0L;
    QPainter painter;
    QMap<int,int>::Iterator iterator;
    QMap<int,int>::Iterator end(channelsGroups->end());
    for(iterator = channelsGroups->begin(); iterator != end; ++iterator){
        //update the status
        channelsShowHideStatus.remove(iterator.key());
        channelsShowHideStatus.insert(iterator.key(),false);

        //Update the pixmap
        int groupId = (*channelsGroups)[iterator.key()];
        iconView = iconviewDict[QString::number(groupId)];
        QList<QListWidgetItem*> lstItem =  iconView->findItems(QString::number(iterator.key()),Qt::MatchExactly);
        if(!lstItem.isEmpty()) {
            //Add an item to the target group with the same text but an update icon.
            QPixmap pixmap(14,14);
            //Get the channelColor associated with the item
            const QColor color = channelColors->color(iterator.key());
            drawItem(painter,&pixmap,color,false,channelsSkipStatus[iterator.key()]);
            lstItem.first()->setIcon(QIcon(pixmap));
        }
    }

    //reset isInSelectItems to false to enable again the the emission of signals due to selectionChange
    isInSelectItems = false;
}

void ChannelPalette::updateShowHideStatus(bool showStatus)
{

    //Get the selected channels and set them the showStatus
    const QList<int> channelIds = selectedChannels();

    updateShowHideStatus(channelIds,showStatus);
}

void ChannelPalette::updateShowHideStatus(const QList<int>& channelIds,bool showStatus){  
    //Set isInSelectItems to true to prevent the emission of signals due to selectionChange
    isInSelectItems = true;

    if(!edit){
        QHashIterator<QString, ChannelIconView*> iteratordict(iconviewDict);
        while (iteratordict.hasNext()) {
            iteratordict.next();
            iteratordict.value()->clearSelection();
        }
    }
    
    QList<int>::const_iterator channelIterator;
    ChannelIconView* iconView = 0L;
    QPainter painter;
    QList<int> selectedIds;

    for(channelIterator = channelIds.begin(); channelIterator != channelIds.end(); ++channelIterator){
        //update the status
        channelsShowHideStatus.remove(*channelIterator);
        channelsShowHideStatus.insert(*channelIterator,showStatus);
        //Update the pixmap
        int groupId = (*channelsGroups)[*channelIterator];

        iconView = iconviewDict[QString::number(groupId)];
        QList<QListWidgetItem*>lstItem = iconView->findItems(QString::number(*channelIterator),Qt::MatchExactly);
        if(!lstItem.isEmpty()) {
            QListWidgetItem *item = lstItem.first();
            bool selected;
            if(!edit && showStatus)
                selected = true;
            else
                selected = item->isSelected();

            //Add an item to the target group with the same text but an update icon.
            QPixmap pixmap(14,14);
            //Get the channelColor associated with the item
            QColor color = channelColors->color(*channelIterator);
            drawItem(painter,&pixmap,color,showStatus,channelsSkipStatus[*channelIterator]);
            item->setIcon(QIcon(pixmap));
            if(selected)
                selectedIds.append(*channelIterator);
        }
    }


    selectChannels(selectedIds);

    //reset isInSelectItems to false to enable again the the emission of signals due to selectionChange
    isInSelectItems = false;
}

const QList<int> ChannelPalette::getShowHideChannels(bool showStatus){
    QList<int> channelIds;
    QMap<int,bool>::ConstIterator iterator;
    QMap<int,bool>::ConstIterator end(channelsShowHideStatus.constEnd());
    for(iterator = channelsShowHideStatus.constBegin(); iterator != end; ++iterator) {
        if(iterator.value() == showStatus) {
            channelIds.append(iterator.key());
        }
    }

    return channelIds;
}


void ChannelPalette::updateSkipStatus(const QMap<int,bool>& skipStatus){  
    //Set isInSelectItems to true to prevent the emission of signals due to selectionChange
    isInSelectItems = true;

    if(!edit){
        QHashIterator<QString, ChannelIconView*> iteratordict(iconviewDict);
        while (iteratordict.hasNext()) {
            iteratordict.next();
            iteratordict.value()->clearSelection();
        }
    }
    
    QMap<int,bool>::const_iterator channelIterator;
    ChannelIconView* iconView = 0L;
    QPainter painter;
    QList<int> selectedIds;

    for(channelIterator = skipStatus.begin(); channelIterator != skipStatus.end(); ++channelIterator){
        int channelId = channelIterator.key();
        bool status = channelIterator.value();

        //update the status
        channelsSkipStatus.remove(channelId);
        channelsSkipStatus.insert(channelId,status);

        //Update the pixmap
        int groupId = (*channelsGroups)[channelId];

        iconView = iconviewDict[QString::number(groupId)];
        QList<QListWidgetItem*>lstItem =  iconView->findItems(QString::number(channelId),Qt::MatchExactly);
        if(!lstItem.isEmpty()) {
            QListWidgetItem *item = lstItem.first();
            bool selected = item->isSelected();

            //Add an item to the target group with the same text but an update icon.
            QPixmap pixmap(14,14);
            //Get the channelColor associated with the item
            QColor color = channelColors->color(channelId);
            //set the channelColor associated with the item to the background color if the status is true
            if(status) {
                color = backgroundColor;
            }else{
                //if the status is false and the item has the background color has color change it to the group color.
                if(color == backgroundColor){
                    if(type == DISPLAY)
                        color = channelColors->groupColor(channelId);
                    else
                        color = channelColors->spikeGroupColor(channelId);
                }
            }

            channelColors->setColor(channelId,color);
            drawItem(painter,&pixmap,color,channelsShowHideStatus[channelId],status);
            item->setIcon(QIcon(pixmap));
            if(selected)
                selectedIds.append(channelId);
        }
    }

    selectChannels(selectedIds);

    //reset isInSelectItems to false to enable again the the emission of signals due to selectionChange
    isInSelectItems = false;
}


void ChannelPalette::updateSkipStatus(const QList<int>&channelIds,bool skipStatus){
    //Set isInSelectItems to true to prevent the emission of signals due to selectionChange
    isInSelectItems = true;

    if(!edit){
        QHashIterator<QString, ChannelIconView*> iteratordict(iconviewDict);
        while (iteratordict.hasNext()) {
            iteratordict.next();
            iteratordict.value()->clearSelection();
        }
    }
    
    QList<int>::const_iterator channelIterator;
    ChannelIconView* iconView = 0L;
    QPainter painter;
    QList<int> selectedIds;
    QList<int>::const_iterator end(channelIds.end());
    for(channelIterator = channelIds.begin(); channelIterator != end; ++channelIterator){
        //update the status
        channelsSkipStatus.remove(*channelIterator);
        channelsSkipStatus.insert(*channelIterator,skipStatus);
        //Update the pixmap
        int groupId = (*channelsGroups)[*channelIterator];

        iconView = iconviewDict[QString::number(groupId)];
        QList<QListWidgetItem*>lstItem =  iconView->findItems(QString::number(*channelIterator),Qt::MatchExactly);
        if(!lstItem.isEmpty()) {
            QListWidgetItem *item = lstItem.first();
            bool selected = item->isSelected();

            //Add an item to the target group with the same text but an update icon.
            QPixmap pixmap(14,14);
            //Get the channelColor associated with the item
            QColor color = channelColors->color(*channelIterator);

            //set the channelColor associated with the item to the background color if the status is true
            if(skipStatus) {
                color = backgroundColor;
            } else {
                //if the status is false and the item has the background color has color change it to the group color.
                if(color == backgroundColor){
                    if(type == DISPLAY)
                        color = channelColors->groupColor(*channelIterator);
                    else
                        color = channelColors->spikeGroupColor(*channelIterator);
                }
            }

            channelColors->setColor(*channelIterator,color);

            drawItem(painter,&pixmap,color,channelsShowHideStatus[*channelIterator],skipStatus);
            item->setIcon(QIcon(pixmap));

            if(selected)
                selectedIds.append(*channelIterator);
        }
    }

    selectChannels(selectedIds);

    //reset isInSelectItems to false to enable again the the emission of signals due to selectionChange
    isInSelectItems = false;
}


void ChannelPalette::updateColor(const QList<int> &channelIds){
    QList<int>::const_iterator channelIterator;
    ChannelIconView* iconView = 0L;
    QPainter painter;

    for(channelIterator = channelIds.begin(); channelIterator != channelIds.end(); ++channelIterator){
        int groupId = (*channelsGroups)[*channelIterator];
        iconView = iconviewDict[QString::number(groupId)];
        QList<QListWidgetItem*>lstItem =  iconView->findItems(QString::number(*channelIterator),Qt::MatchExactly);
        if(lstItem.isEmpty())
            return;
        //Get the channelColor associated with the item
        QColor color = channelColors->color(*channelIterator);

        QListWidgetItem* item = lstItem.first();
        //Update the icon
        QPixmap pixmap(14,14);
        drawItem(painter,&pixmap,color,channelsShowHideStatus[*channelIterator],channelsSkipStatus[*channelIterator]);
        item->setIcon(QIcon(pixmap));
    }
}


void ChannelPalette::updateColor(int channelId){
    QPainter painter;

    int groupId = (*channelsGroups)[channelId];
    ChannelIconView* iconView = iconviewDict[QString::number(groupId)];

    QList<QListWidgetItem*>lstItem =  iconView->findItems(QString::number(channelId),Qt::MatchExactly);
    if(lstItem.isEmpty())
        return;

    //Get the channelColor associated with the item
    QColor color = channelColors->color(channelId);

    //Update the icon
    QListWidgetItem* item = lstItem.first();
    QPixmap pixmap(14,14);
    drawItem(painter,&pixmap,color,channelsShowHideStatus[channelId],channelsSkipStatus[channelId]);
    item->setIcon(QIcon(pixmap));
}

void ChannelPalette::updateGroupColor(int channelId){
    QPainter painter;
    //Get the group Color associated with the item depending on the type. Take the the color associated
    //with the type of the other palette.
    QColor color;
    if(type == DISPLAY)
        color = channelColors->spikeGroupColor(channelId);
    else
        color = channelColors->groupColor(channelId);

    //Find the group to which channelId belongs
    int groupId = (*channelsGroups)[channelId];
    ChannelIconView* iconView = iconviewDict[QString::number(groupId)];

    for(int i = 0; i<iconView->count();++i) {
        QListWidgetItem* item = iconView->item(i);
        //Update the icon
        QPixmap pixmap(14,14);
        drawItem(painter,&pixmap,color,channelsShowHideStatus[item->text().toInt()],channelsSkipStatus[item->text().toInt()]);
        item->setIcon(QIcon(pixmap));
    }
}

void ChannelPalette::applyGroupColor(PaletteType paletteType){
    ChannelIconView* iconView = 0L;
    QPainter painter;
    QMap<int,int>::Iterator iterator;
    for(iterator = channelsGroups->begin(); iterator != channelsGroups->end(); ++iterator){
        int groupId = (*channelsGroups)[iterator.key()];
        iconView = iconviewDict[QString::number(groupId)];
        QList<QListWidgetItem*>lstItem =  iconView->findItems(QString::number(iterator.key()),Qt::MatchExactly);
        if(!lstItem.isEmpty()) {
            QListWidgetItem *item = lstItem.first();
            //Get the channelColor associated with the item
            QColor color;
            if(paletteType == SPIKE)
                color = channelColors->spikeGroupColor(iterator.key());
            else
                color = channelColors->groupColor(iterator.key());

            //Update the channelColor if the channel is not skipped
            if(!channelsSkipStatus[iterator.key()])
                channelColors->setColor(iterator.key(),color);

            //Update the icon
            QPixmap pixmap(14,14);
            drawItem(painter,&pixmap,color,channelsShowHideStatus[iterator.key()],channelsSkipStatus[iterator.key()]);
            item->setIcon(QIcon(pixmap));
        }
    }
}

void ChannelPalette::applyCustomColor(){
    ChannelIconView* iconView = 0L;
    QPainter painter;
    QMap<int,int>::Iterator iterator;
    for(iterator = channelsGroups->begin(); iterator != channelsGroups->end(); ++iterator){
        int groupId = (*channelsGroups)[iterator.key()];
        iconView = iconviewDict[QString::number(groupId)];
        QList<QListWidgetItem*>lstItem =  iconView->findItems(QString::number(iterator.key()),Qt::MatchExactly);
        if(!lstItem.isEmpty()) {
            QListWidgetItem *item = lstItem.first();

            //Get the channelColor associated with the item
            QColor color = channelColors->color(iterator.key());

            //Update the icon
            QPixmap pixmap(14,14);
            drawItem(painter,&pixmap,color,channelsShowHideStatus[iterator.key()],channelsSkipStatus[iterator.key()]);
            item->setIcon(QIcon(pixmap));
        }
    }
}

void ChannelPalette::changeBackgroundColor(const QColor &color){
    backgroundColor = color;

    int h;
    int s;
    int v;
    color.getHsv(&h,&s,&v);
    QColor legendColor;
    if(s <= 80 && v >= 240 || (s <= 40 && v >= 220))
        legendColor = Qt::black;
    else
        legendColor = Qt::white;

    QPalette palette;
    palette.setColor(backgroundRole(), backgroundColor);
    palette.setColor(foregroundRole(), legendColor);
    setPalette(palette);

    viewport()->setPalette(palette);

    QHashIterator<QString, ChannelIconView*> iteratordict(iconviewDict);
    while (iteratordict.hasNext()) {
        iteratordict.next();
        QPalette palette;
        palette.setColor(backgroundRole(), backgroundColor);
        palette.setColor(foregroundRole(), legendColor);
        iteratordict.value()->setPalette(palette);
    }

    QHashIterator<QString, ChannelGroupView*> iterator(channelGroupViewDict);
    while (iterator.hasNext()) {
        iterator.next();
        QPalette palette;
        palette.setColor(backgroundRole(), backgroundColor);
        palette.setColor(foregroundRole(), legendColor);
        iterator.value()->setPalette(palette);
    }

    ChannelIconView* iconView = 0L;
    QPainter painter;
    QMap<int,int>::Iterator groupIterator;
    for(groupIterator = channelsGroups->begin(); groupIterator != channelsGroups->end(); ++groupIterator){
        int groupId = (*channelsGroups)[groupIterator.key()];
        iconView = iconviewDict[QString::number(groupId)];
        QList<QListWidgetItem*> lstItem =  iconView->findItems(QString::number(groupIterator.key()),Qt::MatchExactly);
        if(!lstItem.isEmpty()) {

            //update the color of the skip channels
            if(channelsSkipStatus[groupIterator.key()])
                channelColors->setColor(groupIterator.key(),backgroundColor);

            //Get the channelColor associated with the item
            QColor color = channelColors->color(groupIterator.key());

            //Update the icon
            QPixmap pixmap(14,14);
            drawItem(painter,&pixmap,color,channelsShowHideStatus[groupIterator.key()],channelsSkipStatus[groupIterator.key()]);
            QListWidgetItem *item = lstItem.first();
            item->setIcon(QIcon(pixmap));
        }
    }
    update();
}

void ChannelPalette::changeColor(QListWidgetItem* item,bool single){
    int id = item->text().toInt();

    //Get the channelColor associated with the item
    const QColor oldColor = channelColors->color(id);
    qDebug()<<" void ChannelPalette::changeColor(QListWidgetItem* item,bool single){";
    QColor color = QColorDialog::getColor(oldColor,0);
    if(color.isValid()){
        if(single){
            //Update the channelColor only if the channel is not skipped
            if(!channelsSkipStatus[id])
                channelColors->setColor(id,color);

            //Update the icon
            QPixmap pixmap = item->icon().pixmap(QSize(22,22));
            QPainter painter;
            drawItem(painter,&pixmap,color,channelsShowHideStatus[id],channelsSkipStatus[id]);
            item->setIcon(QIcon(pixmap));
            //As soon a color changes a signal is emitted.
            emit singleChangeColor(id);
        }
        else{
            //Change the color for all the other channels of the current group, depending on the PaletteType.
            ChannelIconView* iconViewParent =  static_cast<ChannelIconView*>(item->listWidget());
            for(int i = 0; i<iconViewParent->count();++i) {
                QListWidgetItem *current = iconViewParent->item(i);
                //Update the colors for the channel (color and group color)
                if(type == DISPLAY)
                    channelColors->setGroupColor(current->text().toInt(),color);
                else
                    channelColors->setSpikeGroupColor(current->text().toInt(),color);
                if(!channelsSkipStatus[current->text().toInt()])
                    channelColors->setColor(current->text().toInt(),color);

                //Update the icon
                QPixmap pixmap(14,14);
                QPainter painter;
                drawItem(painter,&pixmap,color,channelsShowHideStatus[current->text().toInt()],channelsSkipStatus[current->text().toInt()]);
                current->setIcon(QIcon(pixmap));
            }
            const QString groupId = iconViewParent->objectName();
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
    setWindowTitle( tr( "Channel palette" ) );
}

void ChannelPalette::selectChannels(const QList<int>& selectedChannels){
    //Set isInSelectItems to true to prevent the emission of signals due to selectionChange
    isInSelectItems = true;
    //unselect all the items first
    QHashIterator<QString, ChannelIconView*> iteratordict(iconviewDict);
    while (iteratordict.hasNext()) {
        iteratordict.next();
        iteratordict.value()->clearSelection();
    }

    //Loop on the channels to be selected
    QList<int>::const_iterator channelIterator;

    QListWidgetItem* currentIcon = 0L;
    ChannelIconView* iconView = 0L;
    for(channelIterator = selectedChannels.begin(); channelIterator != selectedChannels.end(); ++channelIterator){
        int groupId = (*channelsGroups)[*channelIterator];
        iconView = iconviewDict[QString::number(groupId)];
        QList<QListWidgetItem*> lstItem = iconView->findItems(QString::number(*channelIterator),Qt::MatchExactly);
        if(!lstItem.isEmpty()) {
            currentIcon = lstItem.first();
            currentIcon->setSelected(true);
        }
    }
    //Last item in selection gets focus if it exists
    if(!selectedChannels.isEmpty())
        iconView->setCurrentItem(currentIcon);

    //reset isInSelectItems to false to enable again the the emission of signals due to selectionChange
    isInSelectItems = false;
}

void ChannelPalette::reset(){
    iconviewDict.clear();
    qDeleteAll(channelGroupViewDict);
    channelGroupViewDict.clear();

    channelsShowHideStatus.clear();
    edit = true;
    selected.clear();
    selectionStatus.clear();

    isInSelectItems = false;
}

void ChannelPalette::createGroup(int id){  
    ChannelGroupView* group = new ChannelGroupView(edit,backgroundColor);
    verticalContainer->addWidget(group);
    group->setObjectName(QString::number(id));
    GroupLabel* label = new GroupLabel(QString::number(id),group);
    group->setLabel(label);
    if(id == -1){
        label->setText("?");
    } else if(id == 0){
        label->setPixmap(QPixmap(":/icons/trash"));
    }

    //Set the size to 2 digits, max 99 groups
    label->setFixedWidth(labelSize);
    label->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    label->setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed);
    QFont f("Helvetica",8);
    label->setFont(f);
    label->adjustSize();

    ChannelIconView* iconView = new ChannelIconView(backgroundColor,labelSize+7,15*2,edit,group,QString::number(id));
    iconView->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
    if(iconviewDict.count() >= 1){
        if(iconviewDict.contains("1") ){
            iconView->resize((iconviewDict["1"])->size().width(),labelSize);
        }
        //Everything is in the trash group
        else if(iconviewDict.contains("0"))
            iconView->resize((iconviewDict["0"])->size().width(),labelSize);
        //In the spike palette at the begining only the spikeTrashGroup (gpId-1) exists.
        else
            iconView->resize((iconviewDict["-1"])->size().width(),labelSize);
    }
    else
        iconView->adjustSize();
    //group->setStretchFactor(label,0);
    //group->setStretchFactor(iconView,200);
    group->setIconView(iconView);

    iconviewDict.insert(QString::number(id),iconView);
    channelGroupViewDict.insert(QString::number(id),group);
    selectionStatus.insert(QString::number(id),false);

    group->adjustSize();
    iconView->show();
    group->show();

    delete spaceWidget;
    spaceWidget = new SpaceWidget(this,edit);
    verticalContainer->addWidget(spaceWidget);
    spaceWidget->show();
    verticalContainer->setStretchFactor(spaceWidget,2);

    connect(iconView,SIGNAL(itemSelectionChanged()),this, SLOT(slotClickRedraw()));
    connect(iconView,SIGNAL(mousePressMiddleButton(QListWidgetItem*)),this, SLOT(slotMousePressMiddleButton(QListWidgetItem*)));
    connect(this,SIGNAL(paletteResized(int,int)),group,SLOT(reAdjustSize(int,int)));
    connect(iconView,SIGNAL(channelsMoved(QString,QListWidgetItem*)),this, SLOT(slotChannelsMoved(QString,QListWidgetItem*)));
    connect(iconView,SIGNAL(channelsMoved(QList<int>,QString,QListWidgetItem*)),this, SLOT(slotChannelsMoved(QList<int>,QString,QListWidgetItem*)));

    connect(label,SIGNAL(middleClickOnLabel(QString)),this, SLOT(slotMidButtonPressed(QString)));
    connect(label,SIGNAL(leftClickOnLabel(QString)),this, SLOT(slotMousePressed(QString)));

    connect(this,SIGNAL(setDragAndDrop(bool)),iconView, SLOT(setDragAndDrop(bool)));
    connect(this,SIGNAL(setDragAndDrop(bool)),group, SLOT(setDragAndDrop(bool)));
    connect(this,SIGNAL(setDragAndDrop(bool)),spaceWidget, SLOT(setDragAndDrop(bool)));
    connect(iconView,SIGNAL(dropLabel(int,int,int,int)),this, SLOT(groupToMove(int,int,int,int)));
    connect(group,SIGNAL(dropLabel(int,int,int,int)),this, SLOT(groupToMove(int,int,int,int)));
    connect(spaceWidget,SIGNAL(dropLabel(int,int,int,int)),this, SLOT(groupToMove(int,int,int,int)));
    connect(group,SIGNAL(dragObjectMoved(QPoint)),this, SLOT(slotDragLabeltMoved(QPoint)));

    connect(iconView, SIGNAL(moveListItem(QList<int>,QString,QString,int, bool)),
            SLOT(slotMoveListItem(QList<int>,QString,QString,int, bool)));

    connect(iconView, SIGNAL(rowInsered()), SLOT(slotRowInsered()));


    if(id != 0 && id != -1 && (iconviewDict.contains("0")  || iconviewDict.contains("-1") ))
        moveTrashesToBottom();
}

void ChannelPalette::slotRowInsered()
{
    emit paletteResized(viewport()->width(),labelSize);
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
            if(iconviewDict.contains("0")) targetId--;
            if(iconviewDict.contains("-1")) targetId--;
        }
        else{
            ChannelGroupView* group = channelGroupViewDict[QString::number(targetId)];
            int gpPosition = QWidget::mapToGlobal(group->pos()).y();
            if(gpPosition + (group->height() / 2) > destination) targetId--;

            //Insert after the biggest group id (before the trash group)
            if(targetId == 0 || targetId == -1 || targetId == -2){
                targetId = channelGroupViewDict.count();
                if(iconviewDict.contains("0"))
                    targetId--;
                if(iconviewDict.contains("-1"))
                    targetId--;
            }
        }

        if(targetId == sourceId)
            return;

        //insert after targetId
        //Rename the groups
        sourceIconView = iconviewDict.take(QString::number(sourceId));
        sourceGroup = channelGroupViewDict.take(QString::number(sourceId));
        sourceChannelsIds = (*groupsChannels)[sourceId];
        groupsChannels->remove(sourceId);

        for(int i = sourceId + 1; i <= targetId; ++i){
            //Rename the iconView
            ChannelIconView* iconView = iconviewDict.take(QString::number(i));
            iconView->setObjectName(QString::number(i - 1));
            iconviewDict.insert(QString::number(i - 1),iconView);
            //Rename the ChannelGroupView and the label
            ChannelGroupView* group = channelGroupViewDict.take(QString::number(i));
            group->setObjectName(QString::number(i - 1));
            QLabel* label = group->label();
            label->setText(QString::number(i - 1));
            channelGroupViewDict.insert(QString::number(i - 1),group);

            //Update the groups-channels variables
            QList<int> channelIds = (*groupsChannels)[i];
            groupsChannels->remove(i);
            groupsChannels->insert(i - 1,channelIds);

            QList<int>::iterator iterator;
            for(iterator = channelIds.begin(); iterator != channelIds.end(); ++iterator) {
                channelsGroups->remove(*iterator);
                channelsGroups->insert(*iterator,i - 1);
            }
        }
    }
    else{
        //Moving upwards
        ChannelGroupView* group = channelGroupViewDict[QString::number(targetId)];
        int gpPosition = QWidget::mapToGlobal(group->pos()).y();
        if((gpPosition + (group->height() / 2)) < destination) targetId++;

        if(targetId == sourceId) return;

        //insert before targetId
        //Rename the groups
        sourceIconView = iconviewDict.take(QString::number(sourceId));
        sourceGroup = channelGroupViewDict.take(QString::number(sourceId));
        sourceChannelsIds = (*groupsChannels)[sourceId];
        groupsChannels->remove(sourceId);

        for(int i = sourceId - 1; i >= targetId; i--){
            //Rename the iconView
            ChannelIconView* iconView = iconviewDict.take(QString::number(i));
            iconView->setObjectName(QString::number(i + 1));
            iconviewDict.insert(QString::number(i + 1),iconView);
            //Rename the ChannelGroupView and the label
            ChannelGroupView* group = channelGroupViewDict.take(QString::number(i));
            group->setObjectName(QString::number(i + 1));
            QLabel* label = group->label();
            label->setText(QString::number(i + 1));
            channelGroupViewDict.insert(QString::number(i + 1),group);

            //Update the groups-channels variables
            QList<int> channelIds = (*groupsChannels)[i];
            groupsChannels->remove(i);
            groupsChannels->insert(i + 1,channelIds);

            QList<int>::iterator iterator;
            for(iterator = channelIds.begin(); iterator != channelIds.end(); ++iterator) {
                channelsGroups->remove(*iterator);
                channelsGroups->insert(*iterator,i + 1);
            }
        }
    }

    //Rename the moved group.
    sourceIconView->setObjectName(QString::number(targetId));
    iconviewDict.insert(QString::number(targetId),sourceIconView);
    //Rename the ChannelGroupView and the label
    sourceGroup->setObjectName(QString::number(targetId));
    QLabel* label = sourceGroup->label();
    label->setText(QString::number(targetId));
    channelGroupViewDict.insert(QString::number(targetId),sourceGroup);

    //Update the groups-channels variables
    groupsChannels->insert(targetId,sourceChannelsIds);

    QList<int>::iterator iterator;
    for(iterator = sourceChannelsIds.begin(); iterator != sourceChannelsIds.end(); ++iterator) {

        channelsGroups->remove(*iterator);
        channelsGroups->insert(*iterator,targetId);
    }

    //Move the groups
    verticalContainer->removeWidget(spaceWidget);

    QHashIterator<QString, ChannelGroupView*> it(channelGroupViewDict);
    while (it.hasNext()) {
        it.next();
        verticalContainer->removeWidget(it.value());
    }

    int nbGroups = channelGroupViewDict.count();
    if(iconviewDict.contains("0")) nbGroups--;
    if(iconviewDict.contains("-1") ) nbGroups--;
    for(int i = 1;i <= nbGroups;++i)
        verticalContainer->addWidget(channelGroupViewDict[QString::number(i)]);

    if(iconviewDict.contains("-1"))
        verticalContainer->addWidget(channelGroupViewDict["-1"]);
    if(iconviewDict.contains("0"))
        verticalContainer->addWidget(channelGroupViewDict["0"]);

    delete spaceWidget;
    spaceWidget = new SpaceWidget(this,edit);
    verticalContainer->addWidget(spaceWidget);
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
    if(selectedIds.isEmpty())
        return;

    int groupNb = iconviewDict.count() + 1;
    if(iconviewDict.contains("0"))
        groupNb--;
    if(iconviewDict.contains("-1"))
        groupNb--;

    createGroup(groupNb);

    emit paletteResized(viewport()->width(),labelSize);

    //Move the selected channels into the newly created group.
    moveChannels(groupNb);
}

int ChannelPalette::createEmptyGroup(){
    int groupNb = iconviewDict.count() + 1;
    if(iconviewDict.contains("0"))
        groupNb--;
    if(iconviewDict.contains("-1"))
        groupNb--;

    createGroup(groupNb);

    emit paletteResized(viewport()->width(),labelSize);

    //Add an entry in the map group-channel list
    QList<int> channels;
    groupsChannels->insert(groupNb,channels);

    return groupNb;
}

void ChannelPalette::moveChannels(int targetGroup){
    ChannelIconView* iconView = iconviewDict[QString::number(targetGroup)];

    //Get the destination group color to later update the group color of the moved channels, for a new group blue is the default
    QList<int> destinationChannels = (*groupsChannels)[targetGroup];
    QColor groupColor;
    groupColor.setHsv(210,255,255);
    if(!destinationChannels.isEmpty()){
        if(type == DISPLAY)
            groupColor = channelColors->groupColor(destinationChannels.at(0));
        else
            groupColor = channelColors->spikeGroupColor(destinationChannels.at(0));
    }

    //will store the selected channels which to be moved.
    QList<int> movedChannels;
    QList<int> movedFromTrashChannels;
    QPainter painter;
    QHashIterator<QString, ChannelIconView*> it(iconviewDict);
    while (it.hasNext()) {
        it.next();

        if(it.key().toInt() == targetGroup)
            continue;
        QList<int> currentMovedChannels;
        QList<int> channelIds;
        for(int i = 0; i <it.value()->count();++i) {
            QListWidgetItem *item = it.value()->item(i);
            int channelId = item->text().toInt();
            if(item->isSelected()){
                //if the source is the trash group, keep track of it to inform the other palette
                if(it.key() == "0")
                    movedFromTrashChannels.append(channelId);
                movedChannels.append(channelId);
                currentMovedChannels.append(channelId);

                //Add the channel to the trash group
                QPixmap pixmap(14,14);
                QColor color = channelColors->color(channelId);
                drawItem(painter,&pixmap,color,channelsShowHideStatus[channelId],channelsSkipStatus[channelId]);
                new ChannelIconViewItem(QIcon(pixmap),QString::number(channelId),iconView);

                //Modify the entry in the map channels-group
                channelsGroups->remove(channelId);
                channelsGroups->insert(channelId,targetGroup);
            }
            else
                channelIds.append(channelId);
        }
        //Delete the entries in the source group
        QList<int>::iterator it2;
        for(it2 = currentMovedChannels.begin(); it2 != currentMovedChannels.end(); ++it2){
            QList<QListWidgetItem*>lstItem = it.value()->findItems(QString::number(*it2),Qt::MatchExactly);
            if(!lstItem.isEmpty()) {
                delete lstItem.first();
            }
        }
        //Update groupsChannels
        groupsChannels->insert(it.key().toInt(),channelIds);

        //it.value()->arrangeItemsInGrid();
    }

    //Add/update the group entry in the map group-channel list
    QList<int> targetChannels;
    for(int i = 0; i<iconView->count();++i) {
        targetChannels.append(iconView->item(i)->text().toInt());
    }

    groupsChannels->insert(targetGroup,targetChannels);

    //iconView->arrangeItemsInGrid();

    //Update the group color, for a new group blue is the default
    QList<int>::iterator it2;
    for(it2 = movedChannels.begin(); it2 != movedChannels.end(); ++it2){
        if(type == DISPLAY)
            channelColors->setGroupColor(*it2,groupColor);
        else
            channelColors->setSpikeGroupColor(*it2,groupColor);
    }

    //Do not leave empty groups.
    deleteEmptyGroups();

    //Inform the application that the spike groups have been modified (use to warn the user at the end of the session)
    emit groupModified();

    //If the channels have been removed from the trash inform the other palette.
    if(!movedFromTrashChannels.isEmpty()){
        emit channelsRemovedFromTrash(movedFromTrashChannels);
    }
    update();
}


void ChannelPalette::deleteEmptyGroups(){  
    QList<int> deletedGroups;
    //First store the group to delete
    QHashIterator<QString, ChannelIconView*> iteratordict(iconviewDict);
    while (iteratordict.hasNext()) {
        iteratordict.next();
        int groupId = iteratordict.key().toInt();
        if(iteratordict.value()->count() == 0)
            deletedGroups.append(groupId);
    }

    if(!deletedGroups.isEmpty()){
        int nbGp = iconviewDict.count();
        int gpId;
        if(iconviewDict.contains("-1"))
            gpId = -1;
        else if(iconviewDict.contains("0"))
            gpId = 0;
        else
            gpId = 1;

        int minId = gpId;

        bool skipIdZero = !iconviewDict.contains("0");
        for(int j = 0;j < nbGp; ++j){
            if(deletedGroups.contains(gpId)){
                deletedGroups.append(gpId);
                iconviewDict.remove(QString::number(gpId));
                delete channelGroupViewDict.take(QString::number(gpId));
                selectionStatus.remove(QString::number(gpId));
                //-1 and 0 are the trash groups, they are not renamed
                if(gpId == 0 || gpId == -1)
                    minId++;

                groupsChannels->remove(gpId);
                if(gpId == -1 && skipIdZero){
                    gpId += 2;//from -1 to 1 directly
                    minId++;
                }else {
                    ++gpId;
                }
            }
            else{
                if(gpId != minId){
                    //Rename the iconview
                    ChannelIconView* iconView = iconviewDict.take(QString::number(gpId));
                    iconView->setObjectName(QString::number(minId));
                    iconviewDict.insert(QString::number(minId),iconView);
                    //Rename the ChannelGroupView and the label
                    ChannelGroupView* group = channelGroupViewDict.take(QString::number(gpId));
                    group->setObjectName(QString::number(minId));

                    QLabel* label = group->label();
                    label->setText(QString::number(minId));
                    channelGroupViewDict.insert(QString::number(minId),group);

                    //Update the groups-channels variables
                    QList<int> channelIds = (*groupsChannels)[gpId];
                    groupsChannels->remove(gpId);
                    groupsChannels->insert(minId,channelIds);

                    QList<int>::iterator iterator;
                    for(iterator = channelIds.begin(); iterator != channelIds.end(); ++iterator) {
                        channelsGroups->remove(*iterator);
                        channelsGroups->insert(*iterator,minId);
                    }
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

        if(iconviewDict.contains("0") || iconviewDict.contains("-1")) moveTrashesToBottom();
    }
}

void ChannelPalette::selectAllChannels(){
    //Set isInSelectItems to true to prevent the emission of signals due to selectionChange
    isInSelectItems = true;

    //unselect all the items first
    QHashIterator<QString, ChannelIconView*> iteratordict(iconviewDict);
    while (iteratordict.hasNext()) {
        iteratordict.next();
        iteratordict.value()->selectAll();
    }


    QList<int> selected = selectedChannels();
    emit channelsSelected(selected);

    //reset isInSelectItems to false to enable again the the emission of signals due to selectionChange
    isInSelectItems = false;
}

void ChannelPalette::deselectAllChannels()
{
    //Set isInSelectItems to true to prevent the emission of signals due to selectionChange
    isInSelectItems = true;

    //unselect all the items first
    QHashIterator<QString, ChannelIconView*> iteratordict(iconviewDict);
    while (iteratordict.hasNext()) {
        iteratordict.next();
        iteratordict.value()->clearSelection();
    }
    QList<int> selected;
    emit channelsSelected(selected);

    //reset isInSelectItems to false to enable again the the emission of signals due to selectionChange
    isInSelectItems = false;
}

void ChannelPalette::removeChannelsFromTrash(const QList<int>& channelIds)
{
    //Put the channels removed from the trash in a new group.
    if(type == DISPLAY){
        int targetGroup = createEmptyGroup();
        moveChannels(channelIds,"0",QString::number(targetGroup));
    }
    //Put the channels removed into the spikeGroupTrash (create it if need it).
    else{
        if(!iconviewDict.contains("-1")){
            createGroup(-1);
            emit paletteResized(viewport()->width(),labelSize);

            moveTrashesToBottom();
        }
        moveChannels(channelIds,"0","-1");
    }

}

void ChannelPalette::moveChannels(const QList<int>& channelIds, const QString &sourceGroup, const QString &targetGroup, int index, bool moveAll){
    QList<int> targetChannels = (*groupsChannels)[targetGroup.toInt()];
    QList<int> sourceChannels = (*groupsChannels)[sourceGroup.toInt()];

    QList<int>::const_iterator iterator;
    ChannelIconView* sourceIconView = iconviewDict[sourceGroup];
    ChannelIconView* targetIconView = iconviewDict[targetGroup];

    //Get the target group color to later update the group color of the moved channels, for a new group blue is the default
    QColor groupColor;
    groupColor.setHsv(210,255,255);
    if(!targetChannels.isEmpty()){
        if(type == DISPLAY)
            groupColor = channelColors->groupColor(targetChannels.first());
        else
            groupColor = channelColors->spikeGroupColor(targetChannels.first());
    }

    for(iterator = channelIds.begin(); iterator != channelIds.end(); ++iterator){
        //Delete the item from the sourceGroup
        QList<QListWidgetItem*>lstItem = sourceIconView->findItems(QString::number(*iterator),Qt::MatchExactly);
        if(!lstItem.isEmpty()) {
            delete lstItem.first();
            //Add an item to the target group.
            QPixmap pixmap(14,14);
            QColor color = channelColors->color(*iterator);            
            if(targetGroup == "0")
                channelsShowHideStatus[*iterator] = false;
            QPainter painter;
            drawItem(painter,&pixmap,color,channelsShowHideStatus[*iterator],channelsSkipStatus[*iterator]);
            qDebug()<<" index "<<index;
            if (index != -1) {
                ChannelIconViewItem *item = new ChannelIconViewItem(QIcon(pixmap),QString::number(*iterator));
                targetIconView->insertItem(index, item);
            } else {
                new ChannelIconViewItem(QIcon(pixmap),QString::number(*iterator), targetIconView);
            }
            //Update the group color
            if(type == DISPLAY)
                channelColors->setGroupColor(*iterator,groupColor);
            else
                channelColors->setSpikeGroupColor(*iterator,groupColor);

            sourceChannels.removeAll(*iterator);
            targetChannels.append(*iterator);
            channelsGroups->remove(*iterator);
            channelsGroups->insert(*iterator,targetGroup.toInt());
        }
    }

    //Modify the entry in the map group-channel list
    groupsChannels->remove(targetGroup.toInt());
    groupsChannels->insert(targetGroup.toInt(),targetChannels);
    //targetIconView->arrangeItemsInGrid();


    //The source is empty now
    if(sourceIconView->count() == 0){
        if(type == DISPLAY)  {
            if (!moveAll)
                deleteEmptyGroups();//the drag has been done in the spike palette
        }
        else{
            //When all the channels of a group have been remove by a drag-drop it can not be suppress immediately
            //(the mouse is still in the iconView area). To make sure the group is suppress, a pair of variables (isGroupToRemove,groupToRemove)
            //is set to inform that a group has to be suppress and a repaint of the palette is asked (update()).
            isGroupToRemove = true;
        }
    }
    else{
        //Modify the entries in the map group-channel list
        groupsChannels->remove(sourceGroup.toInt());
        groupsChannels->insert(sourceGroup.toInt(),sourceChannels);

        //sourceIconView->arrangeItemsInGrid();
        emit groupModified();
    }

    if(iconviewDict.contains("0") || iconviewDict.contains("-1"))
        moveTrashesToBottom();
    update();
}

void ChannelPalette::slotChannelsMoved(const QString &targetGroup, QListWidgetItem* after){
    qDebug()<<" void ChannelPalette::slotChannelsMoved(const QString &targetGroup, QListWidgetItem* after){"<<targetGroup<<" after "<<after;
    //If the channels have been moved to the trash inform the other palette.
    QString afterId;
    bool beforeFirst = false;
    if(targetGroup == "0" ){
        if(after == 0){
            beforeFirst = true;
            afterId.clear();
        } else {
            afterId = after->text();
        }
    }

    //Get the destination group color to later update the group color of the moved channels, default is blue
    QList<int> destinationChannels = (*groupsChannels)[targetGroup.toInt()];
    QColor groupColor;
    groupColor.setHsv(210,255,255);
    if(!destinationChannels.isEmpty()) {
        if(type == DISPLAY)
            groupColor = channelColors->groupColor(destinationChannels.at(0));
        else
            groupColor = channelColors->spikeGroupColor(destinationChannels.at(0));
    }

    ChannelIconView* targetIconView = iconviewDict[targetGroup];
    //deselect the item from the target group
    targetIconView->clearSelection();

    //If the items have to be inserted before the first item, insert them after the first item
    //and then move the first item after the others
    bool moveFirst = false;
    if(after == 0){
        after = targetIconView->item(0);
        moveFirst = true;
    }

    //will store the selected channels which to be moved.
    QList<int> movedChannels;
    QPainter painter;

    int nbGp = iconviewDict.count();
    int gpId;
    if(iconviewDict.contains("-1"))
        gpId = -1;
    else if(iconviewDict.contains("0"))
        gpId = 0;
    else
        gpId = 1;

    bool skipIdZero = !iconviewDict.contains("0");
    for(int j = 0;j < nbGp; ++j){
        if(gpId == targetGroup.toInt()){
            if(gpId == -1 && skipIdZero)
                gpId += 2;//from -1 to 1 directly
            else
                ++gpId;
            continue;
        }

        QList<int> currentMovedChannels;
        QList<int> channelIds;
        ChannelIconView* iconView = iconviewDict[QString::number(gpId)];
        for(int i = 0; i <iconView->count();++i) {
            QListWidgetItem *item = iconView->item(i);
            int channelId = item->text().toInt();

            if(item->isSelected()){
                movedChannels.append(channelId);
                currentMovedChannels.append(channelId);

                //Add the channel to the target group
                QPixmap pixmap(14,14);
                QColor color = channelColors->color(channelId);
                if(targetGroup == "0")
                    channelsShowHideStatus[channelId] = false;
                drawItem(painter,&pixmap,color,channelsShowHideStatus[channelId],channelsSkipStatus[channelId]);
                int index = targetIconView->row(after);
                after = new ChannelIconViewItem(QIcon(pixmap),QString::number(channelId));
                targetIconView->insertItem(index+1,after);
                i = index + 1;

                //Modify the entry in the map channels-group
                channelsGroups->remove(channelId);
                channelsGroups->insert(channelId,targetGroup.toInt());
            } else {
                channelIds.append(channelId);
            }
        }

        //Delete the entries in the source group
        QList<int>::iterator it;
        for(it = currentMovedChannels.begin(); it != currentMovedChannels.end(); ++it){
            QList<QListWidgetItem*>lstItem = iconView->findItems(QString::number(*it),Qt::MatchExactly);
            if(!lstItem.isEmpty()) {
                delete lstItem.first();
            }
        }
        //Update groupsChannels
        groupsChannels->insert(gpId,channelIds);
        //iconView->arrangeItemsInGrid();

        //If the channels have been removed from the trash inform the other palette.
        if(gpId == 0 && !currentMovedChannels.isEmpty()){
            emit channelsRemovedFromTrash(currentMovedChannels);
        }

        if(gpId == -1 && skipIdZero)
            gpId += 2;//from -1 to 1 directly
        else
            ++gpId;
    }

    if(moveFirst){
        QListWidgetItem* first = targetIconView->item(0);
        QString channelId = first->text();

        delete first;

        //Add a new item corresponding to the channel Id.
        QPixmap pixmap(14,14);
        QColor color = channelColors->color(channelId.toInt());
        drawItem(painter,&pixmap,color,channelsShowHideStatus[channelId.toInt()],channelsSkipStatus[channelId.toInt()]);
        ChannelIconViewItem *item = new ChannelIconViewItem(QIcon(pixmap),channelId);
        targetIconView->insertItem(targetIconView->row(after)+1,item);
    }

    //Modify the entry in the map group-channel list
    QList<int> targetChannels;
    for(int i = 0; i < targetIconView->count();++i) {
        QListWidgetItem *item = targetIconView->item(i);
        targetChannels.append(item->text().toInt());
    }

    groupsChannels->remove(targetGroup.toInt());
    groupsChannels->insert(targetGroup.toInt(),targetChannels);

    //targetIconView->arrangeItemsInGrid();

    //Update the group color, for a new group blue is the default
    QList<int>::iterator it2;
    for(it2 = movedChannels.begin(); it2 != movedChannels.end(); ++it2){
        if(type == DISPLAY)
            channelColors->setGroupColor(*it2,groupColor);
        else
            channelColors->setSpikeGroupColor(*it2,groupColor);
    }


    //Do not leave empty groups.
    isGroupToRemove = true;

    //If the channels have been moved to the trash inform the other palette.
    if(targetGroup == "0") {
        emit channelsMovedToTrash(movedChannels,afterId,beforeFirst);
    }
    update();
}


void ChannelPalette::trashChannelsMovedAround(const QList<int>& channelIds, const QString &afterId, bool beforeFirst){
    QListWidgetItem* after = 0;
    ChannelIconView* trash = iconviewDict["0"];
    //If the items have to be moved before the first item, insert them after the first item
    //and then move the first item after the others
    if(!beforeFirst) {
        QList<QListWidgetItem*>lstItem = trash->findItems(afterId,Qt::MatchExactly);
        if(!lstItem.isEmpty())
            after = lstItem.first();
    }

    //Actually move the channels
    moveChannels(channelIds,"0",after);
    update();
}

void ChannelPalette::moveChannels(const QList<int>& channelIds,const QString& sourceGroup,QListWidgetItem* after){
    QList<int>::const_iterator iterator;
    QPainter painter;
    qDebug()<<" sourceGroup"<<sourceGroup<<" channelIds"<<channelIds<<" after :"<<after;

    ChannelIconView* iconView = iconviewDict[sourceGroup];
    qDebug()<<" iconView "<<iconView;

    qDebug()<<" void ChannelPalette::moveChannels(const QList<int>& channelIds "<<after;
    //If the items have to be moved before the first item, insert them after the first item
    //and then move the first item after the others
    bool moveFirst = false;
    if(after == 0){
        after = iconView->item(0);
        moveFirst = true;
    }
    int afterIndex = iconView->row(after);
    for(iterator = channelIds.begin(); iterator != channelIds.end(); ++iterator){
        //Delete the item corresponding to the channel Id
        QList<QListWidgetItem*>lstItem = iconView->findItems(QString::number(*iterator),Qt::MatchExactly);
        if(!lstItem.isEmpty()) {
            delete lstItem.first();

            //Add a new item corresponding to the channel Id.
            QPixmap pixmap(14,14);
            QColor color = channelColors->color(*iterator);
            drawItem(painter,&pixmap,color,channelsShowHideStatus[*iterator],channelsSkipStatus[*iterator]);
            qDebug()<<" afterIndex"<<afterIndex;
            after = new ChannelIconViewItem(QIcon(pixmap),QString::number(*iterator));
            iconView->insertItem(afterIndex,after);
            afterIndex++;

        }
    }
    if(moveFirst){
        QListWidgetItem* first = iconView->item(0);
        const QString channelId = first->text();
        delete first;

        //Add a new item corresponding to the channel Id.
        QPixmap pixmap(14,14);
        QColor color = channelColors->color(channelId.toInt());
        drawItem(painter,&pixmap,color,channelsShowHideStatus[channelId.toInt()],channelsSkipStatus[channelId.toInt()]);
        const int afterIndex = iconView->row(after);
        after = new ChannelIconViewItem(QIcon(pixmap),channelId);
        iconView->insertItem(afterIndex,after);
    }
    //Modify the entry in the map group-channel list
    QList<int> sourceChannels;
    for(int i=0; i<iconView->count();++i) {
        sourceChannels.append(iconView->item(i)->text().toInt());
    }

    groupsChannels->remove(sourceGroup.toInt());
    groupsChannels->insert(sourceGroup.toInt(),sourceChannels);
}

void ChannelPalette::slotChannelsMoved(const QList<int>& channelIds, const QString &sourceGroup, QListWidgetItem *after){
    //If the channels have been moved around in the trash, inform the other palette.
    if(sourceGroup == "0" ){
        QString afterId;
        bool beforeFirst = false;
        if(after == 0){
            beforeFirst = true;
        } else {
            afterId = after->text();
        }

        emit channelsMovedAroundInTrash(channelIds,afterId,beforeFirst);
    }

    //Actually move the channels
    moveChannels(channelIds,sourceGroup,after);

    //Inform the application that the spike groups have been modified (use to warn the user at the end of the session)
    emit groupModified();

    //update();
}


void ChannelPalette::discardChannels()
{
    trashChannels(0);
}

void ChannelPalette::discardChannels(const QList<int>& channelsToDiscard){
    qDebug()<<" void ChannelPalette::discardChannels(const QList<int>& channelsToDiscard){ "<<objectName()<<" channelsToDiscard "<<channelsToDiscard;
    //Get the destination group color to later update the group color of the moved channels, default is blue
    QColor groupColor;
    groupColor.setHsv(210,255,255);

    //Check if a trash group exists, if not create it.
    if(!iconviewDict.contains("0")){
        createGroup(0);
        moveTrashesToBottom();
    } else {
        QList<int> trashChannels = (*groupsChannels)[0];
        if(!trashChannels.isEmpty()){
            if(type == DISPLAY)
                groupColor = channelColors->groupColor(trashChannels.at(0));
            else
                groupColor = channelColors->spikeGroupColor(trashChannels.at(0));
        }
    }
    ChannelIconView* trash = iconviewDict["0"];

    emit paletteResized(viewport()->width(),labelSize);

    //Set isInSelectItems to true to prevent the emission of signals due to selectionChange
    isInSelectItems = true;

    QPainter painter;

    QHashIterator<QString, ChannelIconView*> it(iconviewDict);
    while (it.hasNext()) {
        it.next();
        //Trash
        if(it.key() == QLatin1String("0")) {
            for(int i=0; i<it.value()->count();++i) {
                QListWidgetItem *item = it.value()->item(i);
                const int channelId = item->text().toInt();
                if(channelsToDiscard.contains(channelId)) {
                    channelsShowHideStatus[channelId] = false;
                    //Update the icon
                    QPixmap pixmap(14,14);
                    QColor color = channelColors->color(channelId);
                    drawItem(painter,&pixmap,color,false,channelsSkipStatus[channelId]);
                    item->setIcon(QIcon(pixmap));
                }
            }
            continue;
        }
        QList<int> currentDiscardedChannels;
        QList<int> channelIds;
        for(int i=0; i<it.value()->count();++i) {
            QListWidgetItem *item = it.value()->item(i);
            int channelId = item->text().toInt();
            if(channelsToDiscard.contains(channelId)){
                currentDiscardedChannels.append(channelId);

                //Modify the entry in the map channels-group
                channelsGroups->remove(channelId);
                channelsGroups->insert(channelId,0);
            }
            else
                channelIds.append(channelId);
        }
        //Delete the entries in the source group
        QList<int>::iterator it2;
        for(it2 = currentDiscardedChannels.begin(); it2 != currentDiscardedChannels.end(); ++it2){
            QList<QListWidgetItem*>lstItem = it.value()->findItems(QString::number(*it2),Qt::MatchExactly);
            if(!lstItem.isEmpty()) {
                delete lstItem.first();
            }
        }
        //Update groupsChannels
        groupsChannels->insert(it.key().toInt(),channelIds);

        //it.value()->arrangeItemsInGrid();
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
            new ChannelIconViewItem(QIcon(pixmap),QString::number(*channelIterator),trash);

            //Update the group color
            if(type == DISPLAY)
                channelColors->setGroupColor(*channelIterator,groupColor);
            else
                channelColors->setSpikeGroupColor(*channelIterator,groupColor);

            trashChannels.append(*channelIterator);
        }
    }

    //Add/update the 0 entry in the map group-channel list
    groupsChannels->remove(0);
    groupsChannels->insert(0,trashChannels);
    //trash->arrangeItemsInGrid();

    //Do not leave empty groups.
    deleteEmptyGroups();

    //reset isInSelectItems to false to enable again the the emission of signals due to selectionChange
    isInSelectItems = false;
    update();
}

void ChannelPalette::discardChannels(const QList<int>& channelsToDiscard,const QString& afterId,bool beforeFirst){
    qDebug()<<" void ChannelPalette::discardChannels(const QList<int>& channelsToDiscard,const QString& afterId,bool beforeFirst){"<<this<<" channelsToDiscard"<<channelsToDiscard<<" afterid"<<afterId;
    QListWidgetItem* after = 0;
    ChannelIconView* trash = iconviewDict["0"];
    //If the items have to be moved before the first item, insert them after the first item
    //and then move the first item after the others
    bool moveFirst = false;
    if(beforeFirst){
        after = trash->item(0);
        moveFirst = true;
    } else {
        QList<QListWidgetItem*> lstItem = trash->findItems(afterId,Qt::MatchExactly);
        if(!lstItem.isEmpty()) {
            after = lstItem.first();
        }
        qDebug() <<" after "<<after;
    }

    //Get the destination group color to later update the group color of the moved channels, default is blue
    QColor groupColor;
    groupColor.setHsv(210,255,255);
    QList<int> destinationChannels = (*groupsChannels)[0];
    if(!destinationChannels.isEmpty()){
        if(type == DISPLAY)
            groupColor = channelColors->groupColor(destinationChannels.at(0));
        else
            groupColor = channelColors->spikeGroupColor(destinationChannels.at(0));
    }

    QList<int>::const_iterator channelIterator;
    ChannelIconView* iconView = 0L;
    QPainter painter;
    for(channelIterator = channelsToDiscard.begin(); channelIterator != channelsToDiscard.end(); ++channelIterator){
        int groupId = (*channelsGroups)[*channelIterator];
        QList<int> sourceChannels = (*groupsChannels)[groupId];
        iconView = iconviewDict[QString::number(groupId)];
        QList<QListWidgetItem*>lstItem =  iconView->findItems(QString::number(*channelIterator),Qt::MatchExactly);
        if(!lstItem.isEmpty()) {
            delete lstItem.first();
        }

        //Add a new item corresponding to the channel Id. The channel is hidden.
        QPixmap pixmap(14,14);
        QColor color = channelColors->color(*channelIterator);
        channelsShowHideStatus[*channelIterator] = false;
        drawItem(painter,&pixmap,color,false,channelsSkipStatus[*channelIterator]);

        const int index = trash->row(after);
        ChannelIconViewItem *newItem = new ChannelIconViewItem(QIcon(pixmap),QString::number(*channelIterator));
        trash->insertItem(index+1, newItem);


        //new ChannelIconViewItem(trash,after,QString::number(*channelIterator),pixmap);
        sourceChannels.removeAll(*channelIterator);
        channelsGroups->remove(*channelIterator);
        channelsGroups->insert(*channelIterator,0);

        //Update the group color
        if(type == DISPLAY)
            channelColors->setGroupColor(*channelIterator,groupColor);
        else
            channelColors->setSpikeGroupColor(*channelIterator,groupColor);

        //Modify the entries in the map group-channel list
        groupsChannels->remove(groupId);
        groupsChannels->insert(groupId,sourceChannels);

        //iconView->arrangeItemsInGrid();
    }

    if(moveFirst){
        QListWidgetItem* first = trash->item(0);
        QString channelId = first->text();
        delete first;

        //Add a new item corresponding to the channel Id.
        QPixmap pixmap(14,14);
        QColor color = channelColors->color(channelId.toInt());
        drawItem(painter,&pixmap,color,channelsShowHideStatus[channelId.toInt()],channelsSkipStatus[channelId.toInt()]);
        const int index = trash->row(after);
        after = new ChannelIconViewItem(QIcon(pixmap),(channelId));
        trash->insertItem(index+1,after);
    }

    //Modify the entry in the map group-channel list
    QList<int> trashChannels;
    for(int i = 0; i <trash->count();++i) {
        QListWidgetItem *item = trash->item(i);
        trashChannels.append(item->text().toInt());
    }
    groupsChannels->remove(0);
    groupsChannels->insert(0,trashChannels);

    //trash->arrangeItemsInGrid();

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

    if(edition)
        channelsShowHideStatus.clear();

    qDebug()<<" channelsShowHideStatus"<<channelsShowHideStatus;

    //Update the item icons
    QPainter painter;
    QMap<int,int>::Iterator iterator;
    QList<int> selectedIds;

    for(iterator = channelsGroups->begin(); iterator != channelsGroups->end(); ++iterator){
        const int groupId = (*channelsGroups)[iterator.key()];
        ChannelIconView* iconView = iconviewDict[QString::number(groupId)];
        QList<QListWidgetItem*>lstItem = iconView->findItems(QString::number(iterator.key()),Qt::MatchExactly);
        if(!lstItem.isEmpty()) {
            QListWidgetItem *item = lstItem.first();
            bool selected = false;
            if(edition){
                selected = item->isSelected();
                channelsShowHideStatus.insert(iterator.key(),selected);
            } else {
                selected = channelsShowHideStatus[iterator.key()];
                qDebug()<<" iterator.key()"<<iterator.key()<<" selected"<<selected;
            }
            QIcon icon = item->icon();
            QPixmap pixmap(icon.pixmap(QSize(14,14)).size());
            const QColor color = channelColors->color(iterator.key());
            drawItem(painter,&pixmap,color,selected,channelsSkipStatus[iterator.key()]);
            item->setIcon(QIcon(pixmap));
            if(selected)
                selectedIds.append(iterator.key());
        }
    }

    selectChannels(selectedIds);
    qDebug()<<"void ChannelPalette::setEditMode "<<this<<" edition"<<edition<<" selectedIds"<<selectedIds;

    //reset isInSelectItems to false to enable again the the emission of signals due to selectionChange
    isInSelectItems = false;
}

void ChannelPalette::drawItem(QPainter& painter,QPixmap* pixmap,QColor color,bool show,bool skip){
    pixmap->fill(Qt::transparent);
    painter.begin(pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);
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
        painter.drawEllipse(QRect(1,1,12,12));
        if(!show){
            painter.setPen(backgroundColor);
            painter.setBrush(backgroundColor);
            painter.drawEllipse(QRect(4,4,6,6));
        }
    }
    else
        painter.fillRect(0,0,14,14,color);
    painter.end();
}

void ChannelPalette::moveTrashesToBottom(){
    //Remove all the children of the verticalContainer (spaceWidget and groups)
    verticalContainer->removeWidget(spaceWidget);

    QHashIterator<QString, ChannelGroupView*> iterator(channelGroupViewDict);
    while (iterator.hasNext()) {
        iterator.next();
        verticalContainer->removeWidget(iterator.value());
    }

    //Insert all the groups except the trashes which go at the bottom
    int nbGroup = channelGroupViewDict.count();
    if(iconviewDict.contains("0"))
        nbGroup--;
    if(iconviewDict.contains("-1"))
        nbGroup--;

    for(int i = 1;i <= nbGroup;++i){
        verticalContainer->addWidget(channelGroupViewDict[QString::number(i)]);
    }

    //Insert the trashes
    if(iconviewDict.contains("-1"))
        verticalContainer->addWidget(channelGroupViewDict["-1"]);
    if(iconviewDict.contains("0"))
        verticalContainer->addWidget(channelGroupViewDict["0"]);

    delete spaceWidget;
    spaceWidget = new SpaceWidget(this,edit);
    verticalContainer->addWidget(spaceWidget);
    connect(this,SIGNAL(setDragAndDrop(bool)),spaceWidget, SLOT(setDragAndDrop(bool)));
    connect(spaceWidget,SIGNAL(dropLabel(int,int,int,int)),this, SLOT(groupToMove(int,int,int,int)));
    spaceWidget->show();
    verticalContainer->setStretchFactor(spaceWidget,2);
}


void ChannelPalette::discardSpikeChannels(){
    trashChannels(-1);
}


void ChannelPalette::trashChannels(int destinationGroup){
    //Set isInSelectItems to true to prevent the emission of signals due to selectionChange
    isInSelectItems = true;

    //Check if there is anything to do
    const QList<int> selectedIds = selectedChannels();
    if(selectedIds.isEmpty())
        return;

    //Check if the destination group exists, if not create it.
    ChannelIconView* trash = 0;
    if(destinationGroup == 0){
        if(!iconviewDict.contains("0")){
            createGroup(0);
            moveTrashesToBottom();
        }
        trash = iconviewDict["0"];
    } else {
        if(!iconviewDict.contains("-1")) {
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
    if(!destinationChannels.isEmpty()){
        if(type == DISPLAY)
            groupColor = channelColors->groupColor(destinationChannels.at(0));
        else
            groupColor = channelColors->spikeGroupColor(destinationChannels.at(0));
    }

    //Will store the selected channels which to be discarded.
    QList<int> discardedChannels;
    //Get the channels which are part of the trash group.
    QList<int> trashChannels;
    QPainter painter;
    QHashIterator<QString, ChannelIconView*> it(iconviewDict);
    while (it.hasNext()) {
        it.next();
        if((it.key() == "0" && destinationGroup == 0) ||
                (it.key() == "-1" && destinationGroup == -1)){
            for(int i = 0; i <it.value()->count();++i ) {
                QListWidgetItem *item = it.value()->item(i);
                int channelId = item->text().toInt();
                if(trashChannels.contains(channelId))
                    continue;

                if(item->isSelected() && destinationGroup == 0){
                    channelsShowHideStatus[channelId] = false;
                    //Update the icon
                    QPixmap pixmap(14,14);
                    QColor color = channelColors->color(channelId);
                    drawItem(painter,&pixmap,color,false,channelsSkipStatus[channelId]);
                    item->setIcon(QIcon(pixmap));
                    //Unselect the item to be coherent with the other ways of moving items
                    item->setSelected(false);
                    discardedChannels.append(channelId);
                } else if(item->isSelected() && destinationGroup == -1) {
                    item->setSelected(false);
                }
                trashChannels.append(channelId);
            }
            continue;
        }

        QList<int> currentDiscardedChannels;
        QList<int> channelIds;
        for(int i =0; i<it.value()->count();++i) {
            QListWidgetItem *item = it.value()->item(i);
            const int channelId = item->text().toInt();
            if(item->isSelected()){
                discardedChannels.append(channelId);
                trashChannels.append(channelId);
                currentDiscardedChannels.append(channelId);
                //Add the channel to the trash group
                QPixmap pixmap(14,14);
                QColor color = channelColors->color(channelId);
                if(destinationGroup == 0)
                    channelsShowHideStatus[channelId] = false;
                drawItem(painter,&pixmap,color,channelsShowHideStatus[channelId],channelsSkipStatus[channelId]);
                new ChannelIconViewItem(QIcon(pixmap),QString::number(channelId),trash);

                //Modify the entry in the map channels-group
                channelsGroups->remove(channelId);
                channelsGroups->insert(channelId,destinationGroup);
            } else {
                channelIds.append(channelId);
            }
        }
        //Delete the entries in the source group
        QList<int>::iterator it2;
        for(it2 = currentDiscardedChannels.begin(); it2 != currentDiscardedChannels.end(); ++it2){
            QList<QListWidgetItem*> lstItem = it.value()->findItems(QString::number(*it2),Qt::MatchExactly);
            if(!lstItem.isEmpty())
                delete lstItem.first();
        }
        //Update groupsChannels
        groupsChannels->insert(it.key().toInt(),channelIds);

        if(destinationGroup == -1 && it.key() == "0" && !currentDiscardedChannels.isEmpty()){
            emit channelsRemovedFromTrash(currentDiscardedChannels);
        }
    }

    //Add/update the 0 entry in the map group-channel list
    groupsChannels->insert(destinationGroup,trashChannels);

    //Update the group color, for a new group blue is the default
    QList<int>::iterator it2;
    for(it2 = discardedChannels.begin(); it2 != discardedChannels.end(); ++it2){
        if(type == DISPLAY)
            channelColors->setGroupColor(*it2,groupColor);
        else
            channelColors->setSpikeGroupColor(*it2,groupColor);
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

void ChannelPalette::slotMoveListItem(const QList<int> &items, const QString& sourceGroup,const QString& destinationGroup,int index, bool moveAll)
{
    QString afterId;
    bool beforeFirst = false;
    if(destinationGroup == QLatin1String("0") ){
        if(index == 0){
            beforeFirst = true;
        } else {
            ChannelIconView* trash = iconviewDict["0"];
            QListWidgetItem *item = trash->item(index);
            if (item)
                afterId = item->text();
        }

        emit channelsMovedToTrash(items,afterId,beforeFirst);
    } else if ( sourceGroup == QLatin1String("0") ){
        if(index == 0){
            beforeFirst = true;
        } else {
            ChannelIconView* iconView = iconviewDict[destinationGroup];
            QListWidgetItem *item = iconView->item(index);
            if (item)
                afterId = item->text();
        }

        emit channelsMovedAroundInTrash(items, afterId, beforeFirst);
    }
    dragChannels(items,sourceGroup,destinationGroup, index, moveAll);
    //Inform the application that the spike groups have been modified (use to warn the user at the end of the session)
    emit groupModified();

    if (moveAll) {
        QTimer::singleShot(100, this, SLOT(deleteEmptyGroups()));
    }
}


void ChannelPalette::dragChannels(const QList<int>& channelIds, const QString &sourceGroup, const QString &targetGroup, int index, bool moveAll){
    QList<int> targetChannels = (*groupsChannels)[targetGroup.toInt()];
    QList<int> sourceChannels = (*groupsChannels)[sourceGroup.toInt()];

    QList<int>::const_iterator iterator;
    ChannelIconView* sourceIconView = iconviewDict[sourceGroup];
    ChannelIconView* targetIconView = iconviewDict[targetGroup];

    //Get the target group color to later update the group color of the moved channels, for a new group blue is the default
    QColor groupColor;
    groupColor.setHsv(210,255,255);
    if(!targetChannels.isEmpty()){
        if(type == DISPLAY)
            groupColor = channelColors->groupColor(targetChannels.first());
        else
            groupColor = channelColors->spikeGroupColor(targetChannels.first());
    }

    for(iterator = channelIds.begin(); iterator != channelIds.end(); ++iterator){
        //Delete the item from the sourceGroup
        QList<QListWidgetItem*>lstItem = sourceIconView->findItems(QString::number(*iterator),Qt::MatchExactly);
        if(!lstItem.isEmpty()) {
            delete lstItem.first();
            //Add an item to the target group.
            QPixmap pixmap(14,14);
            QColor color = channelColors->color(*iterator);
            if(targetGroup == "0")
                channelsShowHideStatus[*iterator] = false;
            QPainter painter;
            drawItem(painter,&pixmap,color,channelsShowHideStatus[*iterator],channelsSkipStatus[*iterator]);
            if (index!=-1) {
                ChannelIconViewItem *item = new ChannelIconViewItem(QIcon(pixmap),QString::number(*iterator));
                targetIconView->insertItem(index, item);
                targetChannels.insert(index,*iterator);
                index++;
            } else {
                new ChannelIconViewItem(QIcon(pixmap),QString::number(*iterator), targetIconView);
                targetChannels.append(*iterator);
            }
            //Update the group color
            if(type == DISPLAY)
                channelColors->setGroupColor(*iterator,groupColor);
            else
                channelColors->setSpikeGroupColor(*iterator,groupColor);

            sourceChannels.removeAll(*iterator);
            channelsGroups->remove(*iterator);
            channelsGroups->insert(*iterator,targetGroup.toInt());
        }
    }


    //Modify the entry in the map group-channel list
    groupsChannels->remove(targetGroup.toInt());
    groupsChannels->insert(targetGroup.toInt(),targetChannels);


    //The source is empty now
    if(sourceIconView->count() == 0){
        if(type == DISPLAY)  {
            if (!moveAll) {
                deleteEmptyGroups();//the drag has been done in the spike palette
            }
        } else {
            //When all the channels of a group have been remove by a drag-drop it can not be suppress immediately
            //(the mouse is still in the iconView area). To make sure the group is suppress, a pair of variables (isGroupToRemove,groupToRemove)
            //is set to inform that a group has to be suppress and a repaint of the palette is asked (update()).
            isGroupToRemove = true;
        }
    } else{
        //Modify the entries in the map group-channel list
        groupsChannels->remove(sourceGroup.toInt());
        groupsChannels->insert(sourceGroup.toInt(),sourceChannels);

        //sourceIconView->arrangeItemsInGrid();
        emit groupModified();
    }

    if(iconviewDict.contains("0") || iconviewDict.contains("-1"))
        moveTrashesToBottom();
    if (sourceGroup == "0") {
        emit channelsRemovedFromTrash(channelIds);
    }

    update();
}

void GroupLabel::mousePressEvent(QMouseEvent* e)
{
    if(e->button() == Qt::LeftButton) {
        QPoint firstClick = QWidget::mapToGlobal(e->pos());

        QDrag *drag = new QDrag(this);
        ChannelMimeData *mimeData = new ChannelMimeData;
        mimeData->setInformation(parent()->objectName().toInt(), firstClick.y());
        drag->setMimeData(mimeData);
        Qt::DropAction dropAction = drag->exec();
        e->accept();

        emit leftClickOnLabel(parent()->objectName());
    }
    else if(e->button() == Qt::MidButton){
        emit middleClickOnLabel(parent()->objectName());
    }
}

void SpaceWidget::dropEvent(QDropEvent *event)
{
  if(event->source() == 0 || !drag){
    event->ignore();
    return;
  }
  if (ChannelMimeData::hasInformation(event->mimeData())) {
    int groupSource, start;
    ChannelMimeData::getInformation(event->mimeData(), &groupSource, &start);
    //to inform that the target is the SpaceWidget, put -2 as the target group.
    emit dropLabel(groupSource,-2,start,QWidget::mapToGlobal(event->pos()).y());
  }
}

void SpaceWidget::dragEnterEvent(QDragEnterEvent *event)
{
  if(event->source() == 0 || !drag){
    event->ignore();
    return;
  }
  if (ChannelMimeData::hasInformation(event->mimeData())) {
    event->acceptProposedAction();
  }
}

