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
 *   the Free Software Foundation; either version 3 of the License, or     *
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
#include <QLayout> 
#include <QStyle>
#include <QColorDialog>

#include <QVector>
#include <QResizeEvent>
#include <QList>
#include <QLabel>

ItemPalette::ItemPalette(PaletteType type, const QColor &backgroundColor, QWidget* parent, const char* name)
    : QScrollArea(parent),
      backgroundColor(backgroundColor),
      isInSelectItems(false),
      spaceWidget(0L),
      type(type)
{
    setObjectName(name);
    setWidgetResizable(true);

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
    if(s <= 80 && v >= 240 || (s <= 40 && v >= 220))
        legendColor = Qt::black;
    else
        legendColor = Qt::white;
    palette.setColor(foregroundRole(), legendColor);
    setPalette(palette);
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    QWidget *w = new QWidget;
    verticalContainer = new QVBoxLayout;
    w->setLayout(verticalContainer);
    setWidget(w);
    verticalContainer->setSpacing(5);

    QFont f("Helvetica",8);
    QFontInfo fontInfo = QFontInfo(f);
    if(type == CLUSTER)
        labelSize = fontInfo.pixelSize() * 2;
    else
        labelSize = fontInfo.pixelSize() * 3;

    //Set the legend in the good language
    languageChange();
    adjustSize();
}    


ItemPalette::~ItemPalette()
{
    // no need to delete child widgets, Qt does it all for us
    qDeleteAll(itemGroupViewDict);
    itemGroupViewDict.clear();
}


void ItemPalette::updateIconPixmaps()
{
    if (needRedrawing.isEmpty())
        return;

    const QMap<QString,QList<int> > selected = selectedItems();
    //update the icons if need it
    QMap<QString,QList<int> >::const_iterator it;
    const QMap<QString,QList<int> >::const_iterator end(needRedrawing.end());
    for(it = needRedrawing.begin(); it != end; ++it){
        QList<int> items = it.value();
        QString  groupName = it.key();
        QMap<int,bool> browsingMap = browsingStatus[groupName];
        ItemIconView* iconView = iconviewDict[groupName];

        QList<int> selectedItems = selected[groupName];

        //redraw the items which have been modified
        QList<int>::iterator iterator;
        for(iterator = items.begin(); iterator != items.end(); ++iterator){
            redrawItem(iconView,*iterator,browsingMap);
        }

        //In order to avoid problems when double clicking, all the icons of the iconview are redrawn
        if(items.isEmpty() && selectedItems.size() == 1){
            for(int i = 0; i < iconView->count();++i){
                redrawItem(iconView,i,browsingMap);
            }
        }
    }

    needRedrawing.clear();
}

void ItemPalette::resizeEvent(QResizeEvent* event){
    //Make the viewport to have the visible size (size of the scrollview)
    viewport()->resize(event->size());
    QScrollArea::resizeEvent(event);
    emit paletteResized(viewport()->width(),labelSize);
}

void ItemPalette::createItemList(ItemColors* itemColors, const QString &groupName, int descriptionLength){
    //Compute gridX used for the event palette where the text is next to the icon (14px)
    QFontInfo fontInfo = QFontInfo(QFont());
    gridX = descriptionLength * fontInfo.pixelSize() + 15;

    //In the case of cluster files, the groupName (<=> electrode id) correspond to a number and the groups are
    //order numerically
    if(type == CLUSTER)
        clusterGroupList.append(groupName.toInt());
    else
        itemGroupList.append(groupName);

    createGroup(groupName);
    updateItemList(groupName, itemColors);

    //always select a group
    if(selected.isEmpty())
        selectGroupLabel(groupName);

    emit paletteResized(viewport()->width(),labelSize);
    update();
}


void ItemPalette::updateItemList(const QString& groupName, ItemColors* itemColors){
    ItemIconView* iconView = iconviewDict[groupName];
    iconView->clear();

    QMap<int,bool> browsingMap = browsingStatus[groupName];
    browsingMap.clear();
    selectionStatus.insert(groupName,false);

    //Construct one icon for each item
    QPainter painter;

    const int nbItems = itemColors->numberOfItems();
    for(int i = 0; i<nbItems; ++i){
        const QColor col =itemColors->color(i,ItemColors::BY_INDEX);
        browsingMap.insert(i,false);
        QPixmap pix(12,12);
        pix.fill(backgroundColor);
        painter.begin(&pix);
        painter.fillRect(0,0,12,12, col);
        painter.end();
        QIcon icon;
        icon.addPixmap(pix);
        ItemWidgetItem *item  = new ItemWidgetItem(icon, itemColors->itemLabel(i), iconView);
        item->setData(ItemIconView::Color,col);
        item->setData(ItemIconView::INDEXICON, itemColors->itemId(i));
    }

    browsingStatus.insert(groupName,browsingMap);
    if(nbItems == 0)
        iconView->resize(50,20);
    else
        iconView->adjustSize();
}


void ItemPalette::slotMousePressed(const QString&sourceGroupName,QListWidgetItem*item){
    if (item) {
        changeColor(item,sourceGroupName);
    }
}

void ItemPalette::selectGroupLabel(const QString &sourceGroupName){
    if(!selected.isEmpty()){
        ItemGroupView* previousSelectedGroup = itemGroupViewDict[selected];
        if(previousSelectedGroup != 0){
            GroupNameLabel* previousLabel = static_cast<GroupNameLabel*>(previousSelectedGroup->label());

            QPalette palette;
            palette.setColor(previousLabel->backgroundRole(), palette.window().color());
            previousLabel->setPalette(palette);
        }
    }
    if(!sourceGroupName.isEmpty()){
        ItemGroupView* group = itemGroupViewDict[sourceGroupName];
        QLabel* label = group->label();
        QPalette palette;
        palette.setColor(label->backgroundRole(), palette.highlight().color());
        label->setPalette(palette);
    }

    selected = sourceGroupName;
    if(type == EVENT)
        emit selectedGroupChanged(selected);
}

void ItemPalette::slotMousePressed(const QString& sourceGroupName,bool shiftKey,bool ctrlAlt){

    if(!selected.isEmpty()){
        ItemGroupView* previousSelectedGroup = itemGroupViewDict[selected];
        if(previousSelectedGroup != 0){
            GroupNameLabel* previousLabel = static_cast<GroupNameLabel*>(previousSelectedGroup->label());

            QPalette palette;
            palette.setColor(previousLabel->backgroundRole(), this->palette().background().color());
            previousLabel->setPalette(palette);
        }
    }

    if (!sourceGroupName.isEmpty()){
        ItemGroupView* group = itemGroupViewDict[sourceGroupName];
        QLabel* label = static_cast<QLabel*>(group->label());
        QPalette palette;
        palette.setColor(label->backgroundRole(), palette.highlight ().color());
        label->setPalette(palette);
        ItemIconView* iconView = iconviewDict[sourceGroupName];
        bool unselect = selectionStatus[sourceGroupName];
        //Set isInSelectItems to true to prevent the emission of signals due to selectionChange
        isInSelectItems = true;
        //If ctrlAlt is true, either set all the select items of the group for browsing or unset them (it is a toggle between the 2 states)
        if(ctrlAlt){
            QMap<int,bool> browsingMap = browsingStatus[sourceGroupName];
            QList<int> itemsToSkip;
            if(unselect){
                selectionStatus[sourceGroupName] = false;
                for(int i = 0; i <iconView->count(); ++i) {
                    QListWidgetItem * item = iconView->item(i);
                    if (item->isSelected()) {
                        if(browsingMap[i]) {
                            browsingMap[i] = false;
                            redrawItem(iconView,i,browsingMap);
                            isInSelectItems = true;//redrawItem sets it back to false
                        }
                    }
                    itemsToSkip.append(i);
                }
            }else{
                selectionStatus[sourceGroupName] = true;
                for(int i = 0; i <iconView->count(); ++i) {
                    QListWidgetItem * item = iconView->item(i);
                    if(item->isSelected()){
                        if(!browsingMap[i]){
                            browsingMap[i] = true;
                            redrawItem(iconView,i,browsingMap);
                            isInSelectItems = true;//redrawItem sets it back to false
                        } else  {
                            itemsToSkip.append(i);
                        }
                    }
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
        } else{
            //If shiftKey is false, either select all the items of the group or deselect them all (it is a toggle between the 2 states)
            if(unselect){
                selectionStatus[sourceGroupName] = false;
                iconView->clearSelection();
                QMap<QString,QList<int> > selection = selectedItems();
                emit updateShownItems(selection);

                QMap<int,bool> browsingMap = browsingStatus[sourceGroupName];
                QList<int> itemsToSkip;
                for(int i = 0; i <iconView->count(); ++i) {
                    //QListWidgetItem * item = iconView->item(i);
                    if(browsingMap[i]){
                        browsingMap[i] = false;
                        redrawItem(iconView,i,browsingMap);
                        isInSelectItems = true;//redrawItem sets it back to false
                        itemsToSkip.append(i);
                    }
                    else itemsToSkip.append(i);

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
            } else{
                selectionStatus[sourceGroupName] = true;
                iconView->selectAll();
                //If it is a cluster palette and the shift key was press, select everything except 0 and 1
                if(shiftKey && type == CLUSTER){
                    QMap<int,bool> browsingMap = browsingStatus[sourceGroupName];
                    QList<int> itemsToSkip;
                    bool hasChanged = false;
                    for (int i = 0; i < iconView->count(); ++i) {
                        QListWidgetItem * item = iconView->item(i);
                        if (item->text() == QLatin1String("0") || item->text() == QLatin1String("1")) {
                            item->setSelected(false);
                            if(browsingMap[i]){
                                hasChanged = true;
                                browsingMap[i] = false;
                                redrawItem(iconView,i,browsingMap);
                                isInSelectItems = true;//redrawItem sets it back to false
                            }
                            itemsToSkip.append(item->data(ItemIconView::INDEXICON).toInt());
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
}


const QMap<QString,QList<int> > ItemPalette::selectedItems(){
    QMap<QString,QList<int> > selection;
    QHashIterator<QString, ItemIconView*> iterator(iconviewDict);
    while (iterator.hasNext()) {
        iterator.next();
        QString groupName = iterator.key();
        QList<int> selectedItems;
        for(int i = 0; i < iterator.value()->count(); ++i) {
            QListWidgetItem *item = iterator.value()->item(i);
            if(item->isSelected()){
                selectedItems.append(item->data(ItemIconView::INDEXICON).toInt());
            }
        }
        selection.insert(groupName,selectedItems);
    }
    return selection;
}

void ItemPalette::slotClickRedraw(){
    if(isInSelectItems)
        return;

    bool browsingEnable = false;
    bool needToBeUpdated = false;
    QMap<QString,QList<int> > selection;
    QHashIterator<QString, ItemIconView*> iterator(iconviewDict);
    while (iterator.hasNext()) {
        iterator.next();
        QString groupName = iterator.key();
        QMap<int,bool> browsingMap = browsingStatus[groupName];
        QList<int> selectedItems;
        QList<int> itemsToSkip;
        QList<int> itemsToRedraw;
        for(int i = 0 ; i<iterator.value()->count(); ++i ){
            QListWidgetItem *item = iterator.value()->item(i);
            if(item->isSelected()){
                selectedItems.append(item->data(ItemIconView::INDEXICON).toInt());
                if(!browsingMap[i])
                    itemsToSkip.append(item->data(ItemIconView::INDEXICON).toInt());
                else
                    browsingEnable = true;
            }
            else{
                if(browsingMap[i]){
                    browsingMap[i] = false;
                    itemsToRedraw.append(i);
                    needToBeUpdated = true;
                }
                itemsToSkip.append(item->data(ItemIconView::INDEXICON).toInt());
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

    updateIconPixmaps();
}

// this is dead code
#if 0
void ItemPalette::slotMousePressWoModificators(const QString& sourceGroup){

    ItemIconView* iconView = iconviewDict[sourceGroup];
    int count = 0;
    for(int i = 0; i <iconView->count();++i) {
        if(iconView->item(i)->isSelected())
            count++;
        if(count > 1)
            break;
    }

    if(count <= 2){
        //Set isInSelectItems to true to prevent the emission of signals due to selectionChange
        isInSelectItems = true;

        QHashIterator<QString, ItemIconView*> iterator(iconviewDict);
        while (iterator.hasNext()) {
            iterator.next();
            if(iterator.key() != sourceGroup)
                iterator.value()->clearSelection();
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
                const QString groupName = iterator.key();
                QMap<int,bool> browsingMap = browsingStatus[groupName];
                QList<int> itemsToSkip;
                for(int i = 0;i <iterator.value()->count();++i) {
                    QListWidgetItem *item = iterator.value()->item(i);
                    int currentIndex = item->data(ItemIconView::INDEXICON).toInt();
                    if(browsingMap[currentIndex]){
                        browsingMap[currentIndex] = false;
                        const QString label = item->text();
                        redrawItem(iterator.value(),currentIndex,browsingMap);
                        isInSelectItems = true;////redrawItem sets it back to false
                        QList<QListWidgetItem*>lstItem = iterator.value()->findItems(label,Qt::MatchExactly);
                        if(!lstItem.isEmpty()) {
                            i = iterator.value()->row(lstItem.first());
                        }
                        itemsToSkip.append(currentIndex);
                    } else {
                        itemsToSkip.append(currentIndex);
                    }
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
}
#endif

void ItemPalette::slotMouseReleased(const QString& sourceGroupName)
{
    Q_UNUSED(sourceGroupName);
    updateIconPixmaps();
}


void ItemPalette::redrawItem(ItemIconView* iconView,int index, const QMap<int,bool>& browsingMap){
    //Set isInSelectItems to true to prevent the emission of signals due to selectionChange
    QListWidgetItem *item = iconView->item(index);
    if (!item)
        return;

    isInSelectItems = true;
    bool browsingStatus = browsingMap[index];
    //Recreate the item
    QPixmap pixmap(12,12);
    pixmap.fill(Qt::transparent);
    QColor color = qvariant_cast<QColor>(item->data(ItemIconView::Color));
    QPainter painter;
    painter.begin(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);
    if(!browsingStatus){
        painter.fillRect(0,0,12,12,color);
    } else {

        QPolygon polygon(4);
        polygon.putPoints(0,3,0,0,12,0,6,12);
        painter.setPen(color);
        painter.setBrush(color);
        painter.drawPolygon(polygon);
    }
    painter.end();
    item->setIcon(QIcon(pixmap));
    //reset isInSelectItems to false to enable again the the emission of signals due to selectionChange
    isInSelectItems = false;
}

bool ItemPalette::isBrowsingEnable(){
    bool browsingEnable = false;
    QMap<QString, QMap<int,bool> > ::Iterator it;
    QMap<QString, QMap<int,bool> > ::Iterator end(browsingStatus.end());
    for(it = browsingStatus.begin(); it != end; ++it){
        QMap<int,bool>  currentMap = it.value();
        QMap<int,bool> ::Iterator it2;
        for(it2 = currentMap.begin(); it2 != currentMap.end(); ++it2){
            if(it2.value()){
                browsingEnable = true;
                break;
            }
        }
        if(browsingEnable)
            break;
    }
    return browsingEnable;
}

void ItemPalette::slotMousePressWAltButton(const QString& sourceGroup,QListWidgetItem *item){
    QMap<int,bool> browsingMap = browsingStatus[sourceGroup];

    ItemIconView* iconView = iconviewDict[sourceGroup];

    QList<int> itemsToRedraw;
    bool browsingEnable = false;

    if(!item->isSelected())
        return;

    int index = -1;
    for(int i = 0; i < iconView->count();++i) {
        if (iconView->item(i) == item) {
            index = i;
            break;
        }
    }
    if (index == -1) {
        return;
    }
    isInSelectItems = true;
    if(browsingMap[index]){
        browsingMap[index] = false;
        browsingStatus.insert(sourceGroup,browsingMap);
        browsingEnable = isBrowsingEnable();
    } else {
        browsingMap[index] = true;
        browsingStatus.insert(sourceGroup,browsingMap);
        browsingEnable = true;
    }

    itemsToRedraw.append(index);
    needRedrawing.insert(sourceGroup,itemsToRedraw);
    QList<int> itemsToSkip;
    for(int i = 0; i < iconView->count();++i) {
        if(!browsingMap[i]) {
            itemsToSkip.append(iconView->item(i)->data(ItemIconView::INDEXICON).toInt());
        }
    }
    emit updateItemsToSkip(sourceGroup,itemsToSkip);

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
    isInSelectItems = false;
}

void ItemPalette::changeBackgroundColor(const QColor& color){
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

    QHashIterator<QString, ItemIconView*> iterator(iconviewDict);
    while (iterator.hasNext()) {
        iterator.next();


        QPalette palette;
        palette.setColor(iterator.value()->backgroundRole(), color);
        palette.setColor(iterator.value()->foregroundRole(), legendColor);
        iterator.value()->setPalette(palette);
    }

    QHashIterator<QString, ItemGroupView*> iterator2(itemGroupViewDict);
    while (iterator2.hasNext()) {
        iterator2.next();

        QPalette palette;
        palette.setColor(iterator2.value()->backgroundRole(), color);
        palette.setColor(iterator2.value()->foregroundRole(), legendColor);
        iterator2.value()->setPalette(palette);
    }

    QPalette palette;
    palette.setColor(backgroundRole(), backgroundColor);
    palette.setColor(foregroundRole(), legendColor);
    setPalette(palette);
    viewport()->setPalette(palette);

    update();
}

void ItemPalette::changeColor(QListWidgetItem* item, const QString& groupName){
    if (!item)
        return;
    QColor color = qvariant_cast<QColor>(item->data(ItemIconView::Color));

    const QColor result = QColorDialog::getColor(color,0);
    if(result.isValid()){
        //Update the icon
        QPixmap pixmap(12,12);
        QPainter painter;
        painter.begin(&pixmap);
        painter.fillRect(0,0,12,12,result);
        painter.end();
        item->setIcon(QIcon(pixmap));
        item->setData(ItemIconView::Color, result);

        ItemIconView* iconView = iconviewDict[groupName];
        //As soon a color changes a signal is emitted.
        emit colorChanged(/*item->data(ItemIconView::INDEXICON).toInt()*/iconView->row(item),groupName, result);
    }
}


/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void ItemPalette::languageChange()
{
    setWindowTitle( tr( "Item palette" ) );
}

void ItemPalette::selectItems(const QString& groupName,const QList<int> &itemsToSelect,const QList<int> &itemsToSkip){
    //Set isInSelectItems to true to prevent the emission of signals due to selectionChange
    if (isInSelectItems)
        return;
    isInSelectItems = true;
    ItemIconView* iconView = iconviewDict[groupName];
    iconView->clearSelection();

    iconView->blockSignals(true);
    //update the browsing map and rebuild the icons
    QMap<int,bool> browsingMap = browsingStatus[groupName];
    browsingMap.clear();
    QListWidgetItem *item = 0;
    QListWidgetItem *lastSelectedItem = 0;
    for(int i=0;i<iconView->count();++i) {
        item = iconView->item(i);
        int realValue = item->data(ItemIconView::INDEXICON).toInt();
        if(itemsToSkip.contains(realValue))
            browsingMap.insert(i,false);
        else
            browsingMap.insert(i,true);
        redrawItem(iconView,i,browsingMap);
        if (itemsToSelect.contains(realValue)) {
            item->setSelected(true);
            lastSelectedItem = item;
        }

    }
    browsingStatus.insert(groupName,browsingMap);

    iconView->blockSignals(false);

    //Last item in selection gets focus if it exists
    if(itemsToSelect.isEmpty())
        iconView->setCurrentItem(lastSelectedItem);
    //reset isInSelectItems to false to enable again the the emission of signals due to selectionChange
    isInSelectItems = false;
}



void ItemPalette::reset(){
    iconviewDict.clear();
    itemGroupViewDict.clear();
    selected.clear();
    clusterGroupList.clear();
    itemGroupList.clear();
    browsingStatus.clear();
    needRedrawing.clear();
    selectionStatus.clear();

    isInSelectItems = false;
}

void ItemPalette::createGroup(const QString &id)
{
    ItemGroupView* group = new ItemGroupView(backgroundColor,this);

    group->setObjectName(id);
    GroupNameLabel* label = new GroupNameLabel(id,group);
    group->setLabel(label);

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
        iconView = new ItemIconView(backgroundColor,QListView::IconMode,fontInfo.pixelSize() * 2,15 *2,group,id);
    else
        iconView = new ItemIconView(backgroundColor,QListView::ListMode,gridX,15*2,group,id);
    iconView->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);

    if(!iconviewDict.isEmpty()){
        QHashIterator<QString, ItemIconView*> iterator(iconviewDict);
        while (iterator.hasNext()) {
            iterator.next();
            iconView->resize((iconviewDict[iterator.key()])->size().width(),2);
        }
    }
    else
        iconView->adjustSize();
    //group->setStretchFactor(label,0);
    //group->setStretchFactor(iconView,200);
    group->setIconView(iconView);

    delete spaceWidget;
    spaceWidget = new QWidget;
    verticalContainer->addWidget(spaceWidget);
    spaceWidget->show();
    verticalContainer->setStretchFactor(spaceWidget,2);

    iconviewDict.insert(id,iconView);

    itemGroupViewDict.insert(id,group);
    group->adjustSize();
    iconView->show();
    group->show();
    if (iconView->size().isNull()) {
        iconView->resize(50,50);
    }

    //Signal and slot connection
    connect(iconView,SIGNAL(itemSelectionChanged()),this, SLOT(slotClickRedraw()));
    connect(iconView,SIGNAL(mousePressMiddleButton(QString,QListWidgetItem*)),this, SLOT(slotMousePressed(QString,QListWidgetItem*)));
    connect(this,SIGNAL(paletteResized(int,int)),group,SLOT(reAdjustSize(int,int)));
    connect(iconView,SIGNAL(mousePressWAltButton(QString,QListWidgetItem*)),this, SLOT(slotMousePressWAltButton(QString,QListWidgetItem*)));
    connect(iconView,SIGNAL(mouseReleased(QString)),this, SLOT(slotMouseReleased(QString)));

    connect(label,SIGNAL(leftClickOnLabel(QString,bool,bool)),this, SLOT(slotMousePressed(QString,bool,bool)));
    connect(iconView, SIGNAL(rowInsered()), SLOT(slotRowInsered()));

    orderTheGroups();
    emit paletteResized(viewport()->width(),labelSize);
    update();
}

void ItemPalette::slotRowInsered()
{
    emit paletteResized(viewport()->width(),labelSize);
}

void ItemPalette::removeGroup(const QString &groupName){
    delete itemGroupViewDict.take(groupName);
    iconviewDict.remove(groupName);
    browsingStatus.remove(groupName);
    selectionStatus.remove(groupName);
    if(type == CLUSTER)
        clusterGroupList.removeAll(groupName.toInt());
    else
        itemGroupList.removeAll(groupName);

    //a group must always be selected.
    if(selected == groupName){
        if(type == CLUSTER && !clusterGroupList.isEmpty()){
            qSort(clusterGroupList);
            selectGroupLabel(QString::number(clusterGroupList.at(0)));
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
        else selectGroupLabel(QString::number(clusterGroupList.at(0)));
    }
    else if(type == EVENT && !itemGroupList.isEmpty()){
        qSort(itemGroupList);
        if(itemGroupList.contains(groupName)) selectGroupLabel(groupName);
        else selectGroupLabel(itemGroupList.at(0));
    }
    else  selected.clear();//never reach
}

void ItemPalette::selectAllItems(){
    //Set isInSelectItems to true to prevent the emission of signals due to selectionChange
    isInSelectItems = true;

    QHashIterator<QString, ItemIconView*> iterator(iconviewDict);
    while (iterator.hasNext()) {
        iterator.next();
        iterator.value()->selectAll();
    }

    QMap<QString,QList<int> > selection = selectedItems();
    emit updateShownItems(selection);

    //reset isInSelectItems to false to enable again the the emission of signals due to selectionChange
    isInSelectItems = false;
}

void ItemPalette::deselectAllItems(){
    //Set isInSelectItems to true to prevent the emission of signals due to selectionChange
    isInSelectItems = true;

    QMap<QString,QList<int> > selection;
    QHashIterator<QString, ItemIconView*> iterator(iconviewDict);
    while (iterator.hasNext()) {
        iterator.next();
        iterator.value()->clearSelection();
        QList<int> selectedItems;
        selection.insert(iterator.key(),selectedItems);
    }

    emit updateShownItems(selection);

    //update the browsing status, it is set to false for all the elements
    QHashIterator<QString, ItemIconView*> iterator2(iconviewDict);
    while (iterator2.hasNext()) {
        iterator2.next();
        QString groupName = iterator2.key();
        QMap<int,bool> browsingMap = browsingStatus[groupName];
        QList<int> itemsToSkip;
        for(int i = 0; i<iterator2.value()->count();++i) {
            QListWidgetItem *item = iterator2.value()->item(i);
            int currentIndex = item->data(ItemIconView::INDEXICON).toInt();
            if(browsingMap[currentIndex]){
                browsingMap[currentIndex] = false;
                QString label = item->text();
                redrawItem(iterator2.value(),currentIndex,browsingMap);
                isInSelectItems = true;//redrawItem sets it back to false
                QList<QListWidgetItem*>lstItem = iterator.value()->findItems(label,Qt::MatchExactly);
                if(!lstItem.isEmpty()) {
                    i = iterator.value()->row(lstItem.first());
                }
                itemsToSkip.append(item->data(ItemIconView::INDEXICON).toInt());
            } else {
                itemsToSkip.append(item->data(ItemIconView::INDEXICON).toInt());
            }
        }
        browsingStatus.insert(groupName,browsingMap);
        emit updateItemsToSkip(groupName,itemsToSkip);
    }
    
    //reset isInSelectItems to false to enable again the the emission of signals due to selectionChange
    isInSelectItems = false;
}


void ItemPalette::orderTheGroups(){
    //Remove all the children of the verticalContainer (spaceWidget and groups)
    verticalContainer->removeWidget(spaceWidget);

    QHashIterator<QString, ItemGroupView*> iterator(itemGroupViewDict);
    while (iterator.hasNext()) {
        iterator.next();
        verticalContainer->removeWidget(iterator.value());
    }

    if(type == CLUSTER) {
        qSort(clusterGroupList);
        QList<int>::iterator iterator;
        for(iterator = clusterGroupList.begin(); iterator != clusterGroupList.end(); ++iterator)
            verticalContainer->addWidget(itemGroupViewDict[QString::number(*iterator)]);
    } else {
        qSort(itemGroupList);
        QStringList::iterator iterator;
        for(iterator = itemGroupList.begin(); iterator != itemGroupList.end(); ++iterator)
            verticalContainer->addWidget(itemGroupViewDict[*iterator]);
    }
    delete spaceWidget;
    spaceWidget = new QWidget;
    verticalContainer->addWidget(spaceWidget);
    //spaceWidget->show();
    verticalContainer->setStretchFactor(spaceWidget,2);
}

