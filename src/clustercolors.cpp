/***************************************************************************
                          clustercolors.cpp  -  description
                             -------------------
    begin                : Tue Sep 16 2004
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

#include "clustercolors.h"

#include <QDebug>

ClusterColors::ClusterColors():ItemColors(){}

ClusterColors::~ClusterColors(){
    qDebug() << "~ClusterColors()";
}

ClusterColors::ClusterColors(const ClusterColors& origin){
    ItemColors(dynamic_cast<const ItemColors&>(origin));
}
