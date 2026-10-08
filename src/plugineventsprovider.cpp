/***************************************************************************
                          plugineventsprovider.cpp  -  description
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

#include "plugineventsprovider.h"

#include <QVector>

#include <algorithm>

PluginEventsProvider::PluginEventsProvider(const std::shared_ptr<PluginFile>& file, int list, double samplingRate, int position)
    : EventsProvider(file->path(), samplingRate, position),
      pluginFile(file),
      listIndex(list)
{
    name = file->eventLists().at(list).name;
    readOnly = true;
}

int PluginEventsProvider::loadData()
{
    events.setSize(0, 0);
    timeStamps.setSize(0, 0);
    eventDescriptionCounter.clear();
    eventIds.clear();
    idsDescriptions.clear();
    nbEvents = 0;

    QVector<qint64> timesNs;
    QVector<int> labels;
    if (!pluginFile->readEvents(listIndex, &timesNs, &labels, &error))
        return INCORRECT_CONTENT;
    if (timesNs.isEmpty())
    {
        error = tr("The event list %1 is empty").arg(name);
        return INCORRECT_CONTENT;
    }
    // Looking up events relies on the time order.
    if (!std::is_sorted(timesNs.begin(), timesNs.end()))
    {
        error = tr("The events of %1 are not in time order").arg(name);
        return INCORRECT_CONTENT;
    }

    // Event times are kept in milliseconds.
    const QStringList descriptions = pluginFile->eventLists().at(listIndex).labels;
    nbEvents = timesNs.size();
    timeStamps.setSize(1, nbEvents);
    events.setSize(1, nbEvents);
    for (long i = 0; i < nbEvents; ++i)
    {
        timeStamps[i] = static_cast<double>(timesNs[i]) / 1e6;
        const EventDescription description(descriptions.at(labels[i]));
        events[i] = description;
        eventDescriptionCounter[description] += 1;
    }
    updateMappingAndDescriptionLength();

    previousStartTime = 0;
    previousStartIndex = 1;
    previousEndIndex = nbEvents;
    previousEndTime = static_cast<long>(floor(0.5 + timeStamps(1, nbEvents)));
    fileMaxTime = previousEndTime;
    return OK;
}
