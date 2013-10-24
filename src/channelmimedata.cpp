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

static const char* s_mimetype = "application/x-channelinformation";

void ChannelMimeData::setInformation(int groupSource, int mouseY)
{
  const QByteArray information = QByteArray::number(groupSource) + '/' + QByteArray::number(mouseY);
  setData(QString::fromLatin1(s_mimetype), information);
}

bool ChannelMimeData::hasInformation(const QMimeData *mimeData)
{
  return mimeData->hasFormat(s_mimetype);
}

void ChannelMimeData::getInformation(const QMimeData *mimeData, int *groupSource, int *mouseY)
{
  const QByteArray information = mimeData->data(s_mimetype);
  const int pos = information.indexOf('/');
  *groupSource = information.left(pos).toInt();
  *mouseY = information.mid(pos+1).toInt();
}
