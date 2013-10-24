/***************************************************************************
                          channelmimedata.cpp
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

#include "channelmimedata.h"


void ChannelMimeData::setInformation(int groupSource, int mouseY)
{
  const QString information = QString::fromLatin1("%1-%2").arg(groupSource).arg(mouseY);
  setText(information);
}

bool ChannelMimeData::hasInformation(const QMimeData *mimeData)
{
  return mimeData->hasText();
}

void ChannelMimeData::getInformation(const QMimeData *mimeData, int *groupSource, int *mouseY)
{
  const QString information = mimeData->text();
  *groupSource = information.section("-",0,0).toInt();
  *mouseY = information.section("-",1,1).toInt();
}
