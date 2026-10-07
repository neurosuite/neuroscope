/***************************************************************************
                          testutils.h  -  description
                             -------------------
    purpose              : Helpers to write synthetic data files and to
                           capture the data emitted by the providers
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

#ifndef TESTUTILS_H
#define TESTUTILS_H

#include "array.h"
#include "blackrock.h"
#include "types.h"

#include <QByteArray>
#include <QFile>
#include <QList>
#include <QString>
#include <QStringList>
#include <QVector>

#include <cstdint>
#include <cstring>
#include <functional>

namespace testutils
{

/** Copy of an Array emitted by a provider. Array itself has no safe copy assignment. */
struct Matrix
{
    long rows = 0;
    long cols = 0;
    QVector<dataType> values;

    /** Element access, rows and columns start at 1 like Array. */
    dataType operator()(long row, long col) const { return values[(row - 1) * cols + (col - 1)]; }
    bool isEmpty() const { return values.isEmpty(); }
};

template<typename T>
Matrix toMatrix(const Array<T>& array)
{
    Matrix m;
    m.rows = array.nbOfRows();
    m.cols = array.nbOfColumns();
    m.values.reserve(m.rows * m.cols);
    for (long r = 1; r <= m.rows; ++r)
        for (long c = 1; c <= m.cols; ++c)
            m.values.append(static_cast<dataType>(array(r, c)));
    return m;
}

inline void writeFile(const QString& path, const QByteArray& content)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly) || file.write(content) != content.size())
        qFatal("Cannot write test file %s", qPrintable(path));
}

inline void writeTextFile(const QString& path, const QString& content)
{
    writeFile(path, content.toUtf8());
}

template<typename T>
void append(QByteArray& bytes, const T& value)
{
    bytes.append(reinterpret_cast<const char*>(&value), sizeof(T));
}

/** Fixed-size, zero-padded copy of @p text, as used in the Blackrock headers. */
inline void copyString(char* destination, size_t size, const char* text)
{
    std::memset(destination, 0, size);
    std::strncpy(destination, text, size);
}

/** Interleaved samples (sample-major, as in .dat and .eeg files). */
template<typename T>
QByteArray interleaved(long nbSamples, int nbChannels, const std::function<T(long sample, int channel)>& value)
{
    QByteArray bytes;
    bytes.reserve(nbSamples * nbChannels * sizeof(T));
    for (long s = 0; s < nbSamples; ++s)
        for (int c = 0; c < nbChannels; ++c)
            append<T>(bytes, value(s, c));
    return bytes;
}

/** Neuralynx .ncs: 16 kB text header, then records of a 20 byte header and 512 samples. */
inline QByteArray ncsFile(long nbSamples, const std::function<int16_t(long sample)>& value)
{
    const int samplesPerRecord = 512;
    QByteArray bytes(16 * 1024, '\0');
    const QByteArray header("######## Neuralynx Data File Header (synthetic test file)\r\n");
    bytes.replace(0, header.size(), header);

    for (long first = 0; first < nbSamples; first += samplesPerRecord)
    {
        bytes.append(QByteArray(20, '\0'));
        for (long s = first; s < first + samplesPerRecord; ++s)
            append<int16_t>(bytes, s < nbSamples ? value(s) : int16_t(0));
    }
    return bytes;
}

/** Description of one channel of a synthetic NSX file. */
struct NsxChannel
{
    QString label;
    int16_t minDigital;
    int16_t maxDigital;
    int16_t minAnalog;
    int16_t maxAnalog;
    QString unit;
};

/** Blackrock NSX 2.2 file with one data block. */
inline QByteArray nsxFile(uint32_t samplingPeriod, const QList<NsxChannel>& channels, long nbSamples,
                          const std::function<int16_t(long sample, int channel)>& value)
{
    NSXBasicHeader basic;
    std::memset(&basic, 0, sizeof(basic));
    std::memcpy(basic.file_type, "NEURALCD", 8);
    basic.file_spec = 0x0202;
    basic.header_size = sizeof(NSXBasicHeader) + channels.size() * sizeof(NSXExtensionHeader);
    copyString(basic.label, sizeof(basic.label), "test");
    basic.sampling_period = samplingPeriod;
    basic.time_resolution = 30000;
    basic.channel_count = channels.size();

    QByteArray bytes;
    append(bytes, basic);
    for (int c = 0; c < channels.size(); ++c)
    {
        NSXExtensionHeader extension;
        std::memset(&extension, 0, sizeof(extension));
        std::memcpy(extension.type, "CC", 2);
        extension.id = c + 1;
        copyString(extension.label, sizeof(extension.label), channels[c].label.toLatin1().constData());
        extension.min_digital_value = channels[c].minDigital;
        extension.max_digital_value = channels[c].maxDigital;
        extension.min_analog_value = channels[c].minAnalog;
        extension.max_analog_value = channels[c].maxAnalog;
        copyString(extension.unit, sizeof(extension.unit), channels[c].unit.toLatin1().constData());
        append(bytes, extension);
    }

    NSXDataHeader data;
    data.header = 1;
    data.timestamp = 0;
    data.length = nbSamples;
    append(bytes, data);
    bytes.append(interleaved<int16_t>(nbSamples, channels.size(), value));
    return bytes;
}

/** One data packet of a synthetic NEV file. */
struct NevPacket
{
    uint32_t timestamp;
    uint16_t id;           // 0 = digital, 1-2048 = spike on electrode id, 0xFFFC = button, ...
    uint8_t unitOrReason;  // unit class for spikes, reason for digital packets
    uint16_t value;        // digital input, button trigger or configuration type
};

/** Blackrock NEV 2.2 file with neural label extension headers for the given electrodes. */
inline QByteArray nevFile(uint32_t timeResolution, const QList<QPair<uint16_t, QString>>& electrodeLabels,
                          const QList<NevPacket>& packets, uint32_t packetSize = 104)
{
    NEVBasicHeader basic;
    std::memset(&basic, 0, sizeof(basic));
    std::memcpy(basic.file_type, "NEURALEV", 8);
    basic.file_spec = 0x0202;
    basic.header_size = sizeof(NEVBasicHeader) + electrodeLabels.size() * sizeof(NEVExtensionHeader);
    basic.data_package_size = packetSize;
    basic.global_time_resolution = timeResolution;
    basic.waveform_time_resolution = timeResolution;
    basic.extension_count = electrodeLabels.size();

    QByteArray bytes;
    append(bytes, basic);
    for (const auto& electrode : electrodeLabels)
    {
        NEVExtensionHeader extension;
        std::memset(&extension, 0, sizeof(extension));
        std::memcpy(extension.id, NEVNeuralLabelID, 8);
        NEVNeuralLabelExtensionData* label = reinterpret_cast<NEVNeuralLabelExtensionData*>(extension.data);
        label->id = electrode.first;
        copyString(label->label, sizeof(label->label), electrode.second.toLatin1().constData());
        append(bytes, extension);
    }

    for (const NevPacket& packet : packets)
    {
        QByteArray p;
        append(p, NEVDataHeader{packet.timestamp, packet.id});
        if (packet.id == NEVDigitalSerialDataID)
            append(p, NEVDigitalSerialData{packet.unitOrReason, 0, packet.value});
        else if (packet.id == NEVButtonDataID)
            append(p, NEVButtonData{packet.value});
        else if (packet.id == NEVConfigurationDataID)
            append(p, NEVConfigurationDataHeader{packet.value});
        else if (packet.id > 0 && packet.id < 2049)
            append(p, NEVSpikeDataHeader{packet.unitOrReason, 0});
        p.append(QByteArray(packetSize - p.size(), '\0'));
        bytes.append(p);
    }
    return bytes;
}

} // namespace testutils

#endif
