/***************************************************************************
                          pluginregistry.h  -  description
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

#ifndef PLUGINREGISTRY_H
#define PLUGINREGISTRY_H

#include <neuroscope/neuroscope_plugin.h>

#include <QList>
#include <QString>
#include <QStringList>

#include <memory>

/** A loaded file format plugin (see neuroscope_plugin.h). The library stays loaded until the program ends. */
class FormatPlugin
{
  public:
    FormatPlugin(const QString& libraryPath, const ns_plugin* api);

    QString name() const { return QString::fromUtf8(api->name); }
    /** File name extensions in lower case, without the dot. */
    QStringList extensions() const { return extensionList; }
    QString libraryPath() const { return path; }
    const ns_plugin* functions() const { return api; }

    /** Whether @p extension (without the dot) is one of the plugin's extensions, ignoring case. */
    bool handlesExtension(const QString& extension) const;

    /** How certain the plugin is that it can read @p file, 0 to 100. */
    int probe(const QString& file) const;

    /** Whether the plugin provides the member at @p offset of ns_plugin (i.e. was built with a
      * version of the interface that has it). */
    bool provides(size_t offset) const { return offset < api->size; }

  private:
    QString path;
    const ns_plugin* api;
    QStringList extensionList;
};

/** The file format plugins found in the plugin directories. */
class PluginRegistry
{
  public:
    /** The registry of the application, with the plugins of defaultDirectories() loaded on first use. */
    static PluginRegistry& instance();

    /** Directories searched for plugins, in this order:
      * - the directories in the environment variable NEUROSCOPE_PLUGIN_PATH,
      * - the user's plugin directory (e.g. ~/.local/share/neurosuite/neuroscope/plugins),
      * - the installed plugin directory next to the executable. */
    static QStringList defaultDirectories();

    /** Loads all plugins of @p directory. */
    void loadDirectory(const QString& directory);

    /** Loads the plugin @p libraryPath. Returns false and adds a message to errors() if it is not a
      * usable plugin. A plugin with the name of one already loaded is skipped. */
    bool load(const QString& libraryPath);

    QList<std::shared_ptr<FormatPlugin>> plugins() const { return pluginList; }

    /** The plugin that reads @p file: of the plugins for its extension, the one whose probe is most
      * certain. Null if no plugin reads it. */
    std::shared_ptr<FormatPlugin> pluginFor(const QString& file) const;

    /** File dialog filters of the plugins, e.g. "DAQ-HDF5 (*.dh5)". */
    QStringList fileFilters() const;

    /** Messages about libraries that could not be loaded. */
    QStringList errors() const { return errorList; }

  private:
    QList<std::shared_ptr<FormatPlugin>> pluginList;
    QStringList errorList;
};

/** A file opened by a plugin. */
class PluginFile
{
  public:
    struct Stream
    {
        QString id;
        QString label;
        int channelCount = 0;
        double samplingRate = 0;
        qint64 sampleCount = 0;
    };

    /** Opens @p path with @p plugin. Returns null and sets @p error if it fails. */
    static std::shared_ptr<PluginFile> open(const std::shared_ptr<FormatPlugin>& plugin, const QString& path, QString* error);

    ~PluginFile();
    PluginFile(const PluginFile&) = delete;
    PluginFile& operator=(const PluginFile&) = delete;

    QString path() const { return filePath; }
    std::shared_ptr<FormatPlugin> plugin() const { return format; }

    QList<Stream> streams() const { return streamList; }
    int defaultStream() const { return defaultIndex; }
    /** Index of the stream with the identifier @p id, or -1. */
    int streamIndex(const QString& id) const;

    /** Labels of the channels of @p stream; channels without a label are numbered from 0. */
    QStringList channelLabels(int stream) const;

    /** Group of each channel of @p stream, from 0; empty if the plugin has no groups. */
    QList<int> channelGroups(int stream) const;

    /** Reads @p count samples of all channels of @p stream from sample @p first on into @p microvolts
      * (row-major, count x channels). Samples outside the stream read as 0. */
    bool read(int stream, qint64 first, qint64 count, double* microvolts, QString* error) const;

  private:
    PluginFile(const std::shared_ptr<FormatPlugin>& plugin, const QString& path, ns_file* file);

    std::shared_ptr<FormatPlugin> format;
    QString filePath;
    ns_file* file;
    QList<Stream> streamList;
    int defaultIndex = 0;
};

#endif
