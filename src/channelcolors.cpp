/***************************************************************************
                          channelcolors.cpp  -  description
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
#include "channelcolors.h"

//C, C++ include files
#include <iostream>
//Added by qt3to4:
#include <Q3PtrList>
#include <QDebug>


using namespace std;

ChannelColors::ChannelColors():channelList(){
    //The list owns the objects, it will delete the channels that are removed.
    channelList.setAutoDelete(true);
}

ChannelColors::~ChannelColors(){
    qDebug() << "~ChannelColors()";
}


ChannelColors::ChannelColors(const ChannelColors& origin){
    //Insert into channelList a deep copy of all the elements of origin.channelList
    ChannelColor* channelColor;
    Q3PtrList<ChannelColor> originChannelList =  origin.channelList;

    for(channelColor = originChannelList.first(); channelColor; channelColor = originChannelList.next()){
        channelList.append(new ChannelColor(*channelColor));
    }
}

QColor ChannelColors::color(int identifier){
    ChannelColors::ChannelColor* theChannelColor = 0L;
    theChannelColor = channelColor(identifier);

    //In case no ChannelColor have been find (should not happen), return black.
    if(theChannelColor == NULL) return QColor(Qt::black);
    else return theChannelColor->color;
}


QColor ChannelColors::groupColor(int identifier){
    ChannelColors::ChannelColor* theChannelColor = 0L;
    theChannelColor = channelColor(identifier);

    //In case no ChannelColor have been find (should not happen), return black.
    if(theChannelColor == NULL) return QColor(Qt::black);
    else return theChannelColor->groupColor;
}

QColor ChannelColors::spikeGroupColor(int identifier){
    ChannelColors::ChannelColor* theChannelColor = 0L;
    theChannelColor = channelColor(identifier);

    //In case no ChannelColor have been find (should not happen), return black.
    if(theChannelColor == NULL) return QColor(Qt::black);
    else return theChannelColor->spikeGroupColor;
}


void ChannelColors::setColor(int identifier, QColor color){
    ChannelColors::ChannelColor* theChannelColor = 0L;
    theChannelColor = channelColor(identifier);
    theChannelColor->color = color;
}

void ChannelColors::setGroupColor(int identifier,QColor color){
    ChannelColors::ChannelColor* theChannelColor = 0L;
    theChannelColor = channelColor(identifier);
    theChannelColor->groupColor = color;
}

void ChannelColors::setSpikeGroupColor(int identifier,QColor color){
    ChannelColors::ChannelColor* theChannelColor = 0L;
    theChannelColor = channelColor(identifier);
    theChannelColor->spikeGroupColor = color;
}

int ChannelColors::channelId(int index){
    return (channelList.at(static_cast<uint>(index)))->channelId;
}

bool ChannelColors::contains(int channelId){
    if(channelColorIndex(channelId) == -1) return false;
    else return true;
}


uint ChannelColors::append(int channelId, QColor color,QColor groupColor,QColor spikeGroupColor){
    channelList.append(new ChannelColor(channelId,color,groupColor,spikeGroupColor));
    return channelList.count();
}

uint ChannelColors::append(int channelId, QColor color){
    channelList.append(new ChannelColor(channelId,color,color,color));
    return channelList.count();
}

void ChannelColors::insert(int channelId,int index,QColor color,QColor groupColor,QColor spikeGroupColor){
    channelList.insert(index, new ChannelColor(channelId,color,groupColor,spikeGroupColor));
}

bool ChannelColors::remove(int identifier){
    return channelList.remove(channelColorIndex(identifier));
}

ChannelColors::ChannelColor* ChannelColors::channelColor(int channelId) const{

    //Iterate on the list until the channel is find
    Q3PtrListIterator<ChannelColors::ChannelColor> iterator(channelList);
    ChannelColors::ChannelColor* channelColor = 0L;
    while((channelColor = iterator.current()) != 0) {
        ++iterator;
        if (channelColor->channelId == channelId) return channelColor;
    }
    return NULL;//Normally never reached
}

int ChannelColors::channelColorIndex(int channelId) const{
    //Iterate on the list until the channel is find
    Q3PtrListIterator<ChannelColors::ChannelColor> iterator(channelList);
    ChannelColors::ChannelColor* channelColor;
    int index = 0;
    while((channelColor = iterator.current()) != 0) {
        if (channelColor->channelId == channelId) return index;
        ++index;
        ++iterator;
    }
    return -1;//Normally never reach
}



