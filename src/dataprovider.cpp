/***************************************************************************
                          dataprovider.cpp  -  description
                             -------------------
    begin                : Mon Mar 1 2004
    copyright            : (C) 2004 by Lynn Hazan
    email                : lynn.hazan.myrealbox.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
//include files for the application
#include "dataprovider.h"

// include files for KDE
#include <kio/netaccess.h>

DataProvider::DataProvider(KURL fileUrl):QObject(){
 //Download the file if need it
 if(!KIO::NetAccess::download(fileUrl,fileName)) fileName = ""; 
}
DataProvider::~DataProvider(){
 //Remove the temp files if any
 KIO::NetAccess::removeTempFile(fileName);
}
