/***************************************************************************
                          apptestutils.h  -  description
                             -------------------
    purpose              : Helpers to open a recording in a NeuroScope
                           main window
    copyright            : (C) 2026 smenp
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef APPTESTUTILS_H
#define APPTESTUTILS_H

#include "testutils.h"

#include "neuroscope.h"
#include "neuroscopedoc.h"

#include <QApplication>
#include <QDir>
#include <QFileDialog>
#include <QSettings>
#include <QTemporaryDir>
#include <QTimer>

#include <cstdint>
#include <memory>

namespace testutils
{

/** Parameter file of a 16-bit recording with @p nbChannels channels in one group. */
inline QString parameterFile(int nbChannels, double samplingRate)
{
    QString channels;
    for (int channel = 0; channel < nbChannels; ++channel)
        channels += QString("<channel skip=\"0\">%1</channel>").arg(channel);
    return QString(R"(<?xml version='1.0'?>
<parameters version="1.0">
 <acquisitionSystem>
  <nBits>16</nBits>
  <nChannels>%1</nChannels>
  <samplingRate>%2</samplingRate>
  <voltageRange>20</voltageRange>
  <amplification>1000</amplification>
  <offset>0</offset>
 </acquisitionSystem>
 <fieldPotentials>
  <lfpSamplingRate>1250</lfpSamplingRate>
 </fieldPotentials>
 <anatomicalDescription>
  <channelGroups>
   <group>%3</group>
  </channelGroups>
 </anatomicalDescription>
 <spikeDetection>
  <channelGroups/>
 </spikeDetection>
</parameters>
)")
        .arg(nbChannels)
        .arg(samplingRate)
        .arg(channels);
}

/**
 * Writes @p baseName.dat (@p nbSamples samples of @p nbChannels interleaved 16-bit channels)
 * and its parameter file into @p dir, and returns the path of the .dat file.
 */
inline QString writeRecording(const QDir& dir, const QString& baseName, int nbChannels, double samplingRate, std::int64_t nbSamples)
{
    QByteArray samples;
    for (std::int64_t sample = 0; sample < nbSamples; ++sample)
        for (int channel = 0; channel < nbChannels; ++channel)
            append(samples, static_cast<std::int16_t>((sample + 100 * channel) % 1000));
    const QString datPath = dir.filePath(baseName + ".dat");
    writeFile(datPath, samples);
    writeTextFile(dir.filePath(baseName + ".xml"), parameterFile(nbChannels, samplingRate));
    return datPath;
}

/**
 * Keeps the settings and recent files of the tests apart from the user's: QSettings() reads and writes INI files in
 * a directory of the test process on every platform, instead of the user's configuration or the Windows registry.
 */
inline void isolateSettings()
{
    static QTemporaryDir settingsDir;
    if (!settingsDir.isValid())
        qFatal("No directory for the settings of the tests");
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, settingsDir.filePath("user"));
    QSettings::setPath(QSettings::IniFormat, QSettings::SystemScope, settingsDir.filePath("system"));
}

/** A main window with @p datPath opened in it. */
inline std::unique_ptr<NeuroscopeApp> openRecording(const QString& datPath)
{
    auto app = std::make_unique<NeuroscopeApp>();
    app->show();
    app->openDocumentFile(datPath);
    return app;
}

/** Calls @p loadSlot of @p app, which asks for files, and answers with @p path. */
inline void loadFile(NeuroscopeApp* app, const char* loadSlot, const QString& path)
{
    QTimer::singleShot(0, [path]
                       {
        QFileDialog* dialog = qobject_cast<QFileDialog*>(QApplication::activeModalWidget());
        if (!dialog)
            qFatal("No file dialog to answer with %s", qPrintable(path));
        dialog->setDirectory(QFileInfo(path).absolutePath());
        dialog->selectFile(QFileInfo(path).fileName());
        QMetaObject::invokeMethod(dialog, "accept"); });
    QMetaObject::invokeMethod(app, loadSlot);
}

} // namespace testutils

#endif
