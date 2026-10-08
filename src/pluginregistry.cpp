/***************************************************************************
                          pluginregistry.cpp  -  description
                             -------------------
    purpose              : Loading of file format plugins
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

#include "pluginregistry.h"
#include "config-neuroscope.h"

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QLibrary>
#include <QStandardPaths>

#include <algorithm>
#include <cstddef>

namespace
{

/** The message of @p error, which the plugin may have filled without a terminating null. */
QString message(const ns_error& error, const QString& fallback)
{
    const QString text = QString::fromUtf8(error.message, static_cast<int>(qstrnlen(error.message, sizeof(error.message))));
    return text.isEmpty() ? fallback : text;
}

} // namespace

FormatPlugin::FormatPlugin(const QString& libraryPath, const ns_plugin* api)
    : path(libraryPath), api(api)
{
    for (const char* const* extension = api->extensions; extension && *extension; ++extension)
        extensionList.append(QString::fromUtf8(*extension).toLower());
}

bool FormatPlugin::handlesExtension(const QString& extension) const
{
    return extensionList.contains(extension.toLower());
}

int FormatPlugin::probe(const QString& file) const
{
    if (!api->probe)
        return 50;
    return std::clamp(api->probe(QFile::encodeName(file).constData()), 0, 100);
}

PluginRegistry& PluginRegistry::instance()
{
    static PluginRegistry registry = []
    {
        PluginRegistry loaded;
        for (const QString& directory : defaultDirectories())
            loaded.loadDirectory(directory);
        for (const QString& error : loaded.errors())
            qWarning() << "Plugin not loaded:" << error;
        return loaded;
    }();
    return registry;
}

QStringList PluginRegistry::defaultDirectories()
{
    QStringList directories = qEnvironmentVariable("NEUROSCOPE_PLUGIN_PATH").split(QDir::listSeparator(), Qt::SkipEmptyParts);
    const QString user = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    if (!user.isEmpty())
        directories.append(user + QLatin1String("/plugins"));
    directories.append(QDir::cleanPath(QCoreApplication::applicationDirPath() + QLatin1String(NEUROSCOPE_PLUGIN_PATH)));
    return directories;
}

void PluginRegistry::loadDirectory(const QString& directory)
{
    const QDir dir(directory);
    if (!dir.exists())
        return;
    const QFileInfoList entries = dir.entryInfoList(QDir::Files, QDir::Name);
    for (const QFileInfo& entry : entries)
    {
        if (QLibrary::isLibrary(entry.fileName()))
            load(entry.absoluteFilePath());
    }
}

bool PluginRegistry::load(const QString& libraryPath)
{
    QLibrary library(libraryPath);
    if (!library.load())
    {
        errorList.append(library.errorString());
        return false;
    }
    const auto entry = reinterpret_cast<ns_plugin_entry>(library.resolve(NS_PLUGIN_ENTRY_NAME));
    if (!entry)
    {
        errorList.append(QStringLiteral("%1: not a NeuroScope plugin (no %2 function)").arg(libraryPath, QLatin1String(NS_PLUGIN_ENTRY_NAME)));
        library.unload();
        return false;
    }
    const ns_plugin* api = entry(NS_PLUGIN_API_VERSION);
    if (!api || api->api_version != NS_PLUGIN_API_VERSION)
    {
        errorList.append(QStringLiteral("%1: built for plugin interface version %2, this NeuroScope uses version %3")
                             .arg(libraryPath)
                             .arg(api ? QString::number(api->api_version) : QStringLiteral("(unknown)"))
                             .arg(NS_PLUGIN_API_VERSION));
        library.unload();
        return false;
    }
    if (api->size < offsetof(ns_plugin, event_list_count) || !api->name || !api->open || !api->close || !api->stream_count ||
        !api->default_stream || !api->stream_info || !api->read)
    {
        errorList.append(QStringLiteral("%1: incomplete plugin description").arg(libraryPath));
        library.unload();
        return false;
    }

    auto plugin = std::make_shared<FormatPlugin>(libraryPath, api);
    for (const auto& loaded : pluginList)
    {
        if (loaded->name() == plugin->name())
        {
            qDebug() << "Skipping plugin" << libraryPath << ", the plugin" << loaded->libraryPath() << "has the same name";
            return true;
        }
    }
    // The QLibrary goes out of scope without unloading, so the library stays loaded.
    pluginList.append(plugin);
    return true;
}

std::shared_ptr<FormatPlugin> PluginRegistry::pluginFor(const QString& file) const
{
    const QString extension = QFileInfo(file).suffix();
    std::shared_ptr<FormatPlugin> best;
    int bestScore = 0;
    for (const auto& plugin : pluginList)
    {
        if (!plugin->handlesExtension(extension))
            continue;
        const int score = plugin->probe(file);
        if (score > bestScore)
        {
            best = plugin;
            bestScore = score;
        }
    }
    return best;
}

QStringList PluginRegistry::fileFilters() const
{
    QStringList filters;
    for (const auto& plugin : pluginList)
    {
        QStringList patterns;
        for (const QString& extension : plugin->extensions())
            patterns.append(QLatin1String("*.") + extension);
        filters.append(QStringLiteral("%1 (%2)").arg(plugin->name(), patterns.join(QLatin1Char(' '))));
    }
    return filters;
}

std::shared_ptr<PluginFile> PluginFile::open(const std::shared_ptr<FormatPlugin>& plugin, const QString& path, QString* error)
{
    const ns_plugin* api = plugin->functions();
    ns_error status{};
    ns_file* handle = nullptr;
    if (api->open(QFile::encodeName(path).constData(), &handle, &status) != NS_OK || !handle)
    {
        if (error)
            *error = message(status, QStringLiteral("The %1 plugin cannot open the file").arg(plugin->name()));
        return nullptr;
    }
    std::shared_ptr<PluginFile> file(new PluginFile(plugin, path, handle));

    const int count = api->stream_count(handle);
    for (int i = 0; i < count; ++i)
    {
        ns_stream_info info{};
        if (api->stream_info(handle, i, &info) != NS_OK)
        {
            if (error)
                *error = QStringLiteral("Cannot read the description of stream %1").arg(i);
            return nullptr;
        }
        Stream stream;
        stream.id = QString::fromUtf8(info.id);
        stream.label = info.label ? QString::fromUtf8(info.label) : stream.id;
        stream.channelCount = info.channel_count;
        stream.samplingRate = info.sampling_rate;
        stream.sampleCount = info.sample_count;
        if (stream.id.isEmpty() || stream.channelCount <= 0 || !(stream.samplingRate > 0) || stream.sampleCount < 0)
        {
            if (error)
                *error = QStringLiteral("Invalid description of stream %1").arg(i);
            return nullptr;
        }
        file->streamList.append(stream);
    }
    if (file->streamList.isEmpty())
    {
        if (error)
            *error = QStringLiteral("The file contains no continuous data");
        return nullptr;
    }
    file->defaultIndex = std::clamp(api->default_stream(handle), 0, static_cast<int>(file->streamList.size()) - 1);

    // Event lists are optional; a list that is not described properly is left out.
    if (api->event_list_count && api->event_list_info && api->read_events)
    {
        const int listCount = api->event_list_count(handle);
        for (int i = 0; i < listCount; ++i)
        {
            ns_event_list_info info{};
            if (api->event_list_info(handle, i, &info) != NS_OK || !info.name || info.count < 0 || info.label_count < 0)
            {
                qWarning() << path << ": invalid description of event list" << i;
                continue;
            }
            EventList list;
            list.name = QString::fromUtf8(info.name);
            list.count = info.count;
            list.pluginIndex = i;
            for (int label = 0; label < info.label_count; ++label)
            {
                const char* text = api->event_label ? api->event_label(handle, i, label) : nullptr;
                list.labels.append(text ? QString::fromUtf8(text) : QString::number(label));
            }
            file->eventListList.append(list);
        }
    }
    return file;
}

PluginFile::PluginFile(const std::shared_ptr<FormatPlugin>& plugin, const QString& path, ns_file* file)
    : format(plugin), filePath(path), file(file)
{
}

PluginFile::~PluginFile()
{
    format->functions()->close(file);
}

int PluginFile::streamIndex(const QString& id) const
{
    for (int i = 0; i < streamList.size(); ++i)
    {
        if (streamList[i].id == id)
            return i;
    }
    return -1;
}

QStringList PluginFile::channelLabels(int stream) const
{
    const ns_plugin* api = format->functions();
    QStringList labels;
    for (int channel = 0; channel < streamList[stream].channelCount; ++channel)
    {
        const char* label = api->channel_label ? api->channel_label(file, stream, channel) : nullptr;
        labels.append(label ? QString::fromUtf8(label) : QString::number(channel));
    }
    return labels;
}

QList<int> PluginFile::channelGroups(int stream) const
{
    const ns_plugin* api = format->functions();
    QList<int> groups;
    if (!format->provides(offsetof(ns_plugin, channel_group)) || !api->channel_group)
        return groups;
    for (int channel = 0; channel < streamList[stream].channelCount; ++channel)
        groups.append(std::max(api->channel_group(file, stream, channel), 0));
    return groups;
}

bool PluginFile::read(int stream, qint64 first, qint64 count, double* microvolts, QString* error) const
{
    const Stream& info = streamList[stream];
    std::fill(microvolts, microvolts + count * info.channelCount, 0.0);

    // Only samples within the stream are read from the plugin.
    const qint64 begin = std::max<qint64>(first, 0);
    const qint64 end = std::min(first + count, info.sampleCount);
    if (begin >= end)
        return true;

    ns_error status{};
    double* destination = microvolts + (begin - first) * info.channelCount;
    if (format->functions()->read(file, stream, begin, end - begin, destination, &status) != NS_OK)
    {
        if (error)
            *error = message(status, QStringLiteral("The %1 plugin cannot read the file").arg(format->name()));
        return false;
    }
    return true;
}

bool PluginFile::readEvents(int list, QVector<qint64>* timesNs, QVector<int>* labels, QString* error) const
{
    const EventList& info = eventListList[list];
    timesNs->resize(info.count);
    labels->resize(info.count);
    ns_error status{};
    static_assert(sizeof(qint64) == sizeof(int64_t) && sizeof(int) == sizeof(int32_t), "event arrays are passed to the plugin");
    if (format->functions()->read_events(file, info.pluginIndex, reinterpret_cast<int64_t*>(timesNs->data()),
                                         reinterpret_cast<int32_t*>(labels->data()), &status) != NS_OK)
    {
        if (error)
            *error = message(status, QStringLiteral("The %1 plugin cannot read the events %2").arg(format->name(), info.name));
        return false;
    }
    for (const int label : *labels)
    {
        if (label < 0 || label >= info.labels.size())
        {
            if (error)
                *error = QStringLiteral("The events %1 have the unknown label %2").arg(info.name).arg(label);
            return false;
        }
    }
    return true;
}
