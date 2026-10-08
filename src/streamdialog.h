/***************************************************************************
                          streamdialog.h  -  description
                             -------------------
    purpose              : Choice of the stream to open from a file read by
                           a plugin
    copyright            : (C) 2026 by Joscha Schmiedt
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef STREAMDIALOG_H
#define STREAMDIALOG_H

#include "pluginregistry.h"

#include <QDialog>

class QCheckBox;
class QListWidget;

/** Lets the user choose which stream of a file to open in this window, and whether to open the others in
  * new windows. */
class StreamDialog : public QDialog
{
    Q_OBJECT
  public:
    StreamDialog(const QList<PluginFile::Stream>& streams, int defaultStream, QWidget* parent = nullptr);

    /** Index of the chosen stream. */
    int selectedStream() const;

    /** Whether the other streams should be opened in new windows. */
    bool openOthers() const;

  private:
    QListWidget* list;
    QCheckBox* others;
};

#endif
