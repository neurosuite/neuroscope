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
#ifndef UTILITIES_H
#define UTILITIES_H

// include files for Qt
#include <QString>
#include <QStringList> 
#include <QFile>
#include <QFileInfo>


/**
@author Lynn Hazan
*/
class Utilities{
public:
    /**
    * Compares the versions
    * @param oldVersion the old version
    * @param newVersion the new version
    * @return true if @p newVersion is superior to @p oldVersion, false otherwise.
    */
    inline static bool compareVersion(const QString& oldVersion, const QString& newVersion){
        QStringList oldList = oldVersion.split(".", QString::SkipEmptyParts);
        QStringList newList = newVersion.split(".", QString::SkipEmptyParts);
        int minLength = qMin(oldList.count(),newList.count());
        for(int i = 0; i< minLength;++i){
            if(newList[i] > oldList[i])
                return true;
        }
        if(newList.count() > oldList.count())
            return true;
        else
            return false;
    }

    /**Counts and returns the number of lines in the file @p path.
    * @param path file path.
    * @return the number of lines in the file.
    */
    static int getNbLines(const QString &path);
    
    /**Creates a backup of the file @p path. The backup file name is the original
    * file name with an additional ~.
    * @param path file path.
    */
    static void createBackup(const QString& path);
};

#endif
