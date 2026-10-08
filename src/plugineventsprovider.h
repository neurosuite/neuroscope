/***************************************************************************
                          plugineventsprovider.h  -  description
                             -------------------
    purpose              : Events of a file read by a plugin
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

#ifndef PLUGINEVENTSPROVIDER_H
#define PLUGINEVENTSPROVIDER_H

#include "eventsprovider.h"
#include "pluginregistry.h"

#include <memory>

/** Provides one event list of a file read by a file format plugin. The provider is named after the
  * list, and its events are read-only: changes made in NeuroScope are not saved. */
class PluginEventsProvider : public EventsProvider
{
    Q_OBJECT
  public:
    /** @param samplingRate sampling rate of the traces shown, in Hz.
      * @param position percentage from the beginning of the window where the events are shown when browsing. */
    PluginEventsProvider(const std::shared_ptr<PluginFile>& file, int list, double samplingRate, int position);

    /** Reads the events from the plugin. Returns INCORRECT_CONTENT, with errorMessage() set, if it fails. */
    int loadData() override;

    QString errorMessage() const { return error; }

  private:
    std::shared_ptr<PluginFile> pluginFile;
    int listIndex;
    QString error;
};

#endif
