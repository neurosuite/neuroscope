/***************************************************************************
                          channelmimedata.h
                             -------------------
    begin                : 24/10/2013
    copyright            : (C) 2013 by David Faure
    email                : david.faure@kdab.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#ifndef CHANNELMIMEDATA_H
#define CHANNELMIMEDATA_H

#include <QMimeData>

class ChannelMimeData : public QMimeData
{
public:
  void setInformation(int groupSource, int mouseY);

  static bool hasInformation(const QMimeData *mimeData);
  static void getInformation(const QMimeData *mimeData, int *groupSource, int *mouseY);
};

#endif
