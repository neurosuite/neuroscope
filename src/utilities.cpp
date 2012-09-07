/***************************************************************************
 *   Copyright (C) 2004 by Lynn Hazan                                      *
 *   lynn@myrealbox.com                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
#include "utilities.h"


int Utilities::getNbLines(QString path){
    // ' are added around the path to take care of directory names with blank.
    path = "'" + path + "'";

    int numLines = 0;
    QFile file(path);
    if(file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        while (!file.atEnd()) {
            file.readLine();
            ++numLines;
        }
    }
    return numLines;
}



void Utilities::createBackup(QString path){
    QFile original(path);
    QFile backup(path+"~");
    original.open(QIODevice::ReadOnly);
    backup.open(QIODevice::WriteOnly);
    backup.write(original.readAll());
    original.close();
    backup.close();
}

