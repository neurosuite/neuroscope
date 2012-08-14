/***************************************************************************
                          prefgeneral.cpp  -  description
                             -------------------
    begin                : Fri Feb 27 2004
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
//Application specific includes.
#include "prefgeneral.h"

//QT includes
#include <qiconset.h>

// include files for KDE
#include <kiconloader.h>

PrefGeneral::PrefGeneral(QWidget *parent, const char *name ) : PrefGeneralLayout(parent,name),validator(this) {
}
PrefGeneral::~PrefGeneral(){
}

#include "prefgeneral.moc"
