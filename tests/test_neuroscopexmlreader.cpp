/***************************************************************************
                          test_neuroscopexmlreader.cpp  -  description
                             -------------------
    purpose              : Tests for reading parameter files
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

#include "neuroscopexmlreader.h"
#include "sessionInformation.h"
#include "testutils.h"

#include <QTemporaryDir>
#include <QtTest>

using namespace testutils;

namespace
{

// Parameter file as written by NDManager, with a NeuroScope section.
// Channels 0-5 of 7; channel 6 is in no anatomical group.
const char* PARAMETER_FILE = R"(<?xml version='1.0'?>
<parameters version="1.0" creator="ndManager-3.0.0">
 <generalInfo>
  <date>2026-10-07</date>
 </generalInfo>
 <acquisitionSystem>
  <nBits>16</nBits>
  <nChannels>7</nChannels>
  <samplingRate>20000</samplingRate>
  <voltageRange>20</voltageRange>
  <amplification>1000</amplification>
  <offset>0</offset>
 </acquisitionSystem>
 <fieldPotentials>
  <lfpSamplingRate>1250</lfpSamplingRate>
 </fieldPotentials>
 <files>
  <file>
   <extension>whl</extension>
   <samplingRate>39.0625</samplingRate>
  </file>
 </files>
 <anatomicalDescription>
  <channelGroups>
   <group>
    <channel skip="0">0</channel>
    <channel skip="1">1</channel>
    <channel skip="0">2</channel>
   </group>
   <group>
    <channel skip="0">3</channel>
    <channel skip="0">4</channel>
    <channel skip="0">5</channel>
   </group>
  </channelGroups>
 </anatomicalDescription>
 <spikeDetection>
  <channelGroups>
   <group>
    <channels>
     <channel>0</channel>
     <channel>1</channel>
     <channel>2</channel>
    </channels>
    <nSamples>32</nSamples>
    <peakSampleIndex>16</peakSampleIndex>
   </group>
  </channelGroups>
 </spikeDetection>
 <neuroscope version="2.0.0">
  <miscellaneous>
   <screenGain>0.2</screenGain>
   <traceBackgroundImage>background.png</traceBackgroundImage>
  </miscellaneous>
  <video>
   <rotate>90</rotate>
   <flip>1</flip>
   <videoImage>arena.png</videoImage>
   <positionsBackground>1</positionsBackground>
  </video>
  <spikes>
   <nSamples>40</nSamples>
   <peakSampleIndex>20</peakSampleIndex>
  </spikes>
  <channels>
   <channelColors>
    <channel>0</channel>
    <color>#0080ff</color>
    <anatomyColor>#ff0000</anatomyColor>
    <spikeColor>#00ff00</spikeColor>
   </channelColors>
   <channelOffset>
    <channel>0</channel>
    <defaultOffset>-120</defaultOffset>
   </channelOffset>
   <channelColors>
    <channel>3</channel>
    <color>#daff36</color>
    <anatomyColor>#daff36</anatomyColor>
    <spikeColor>#0080ff</spikeColor>
   </channelColors>
   <channelOffset>
    <channel>3</channel>
    <defaultOffset>45</defaultOffset>
   </channelOffset>
  </channels>
 </neuroscope>
</parameters>
)";

} // namespace

class TestNeuroscopeXmlReader : public QObject
{
    Q_OBJECT

  private:
    QTemporaryDir dir;
    NeuroscopeXmlReader reader;

    QString path(const QString& name) const { return dir.filePath(name); }

  private Q_SLOTS:
    void initTestCase()
    {
        QVERIFY(dir.isValid());
        writeTextFile(path("session.xml"), PARAMETER_FILE);
        QVERIFY(reader.parseFile(path("session.xml"), NeuroscopeXmlReader::PARAMETER));
    }

    void acquisitionSystem()
    {
        QCOMPARE(reader.getResolution(), 16);
        QCOMPARE(reader.getNbChannels(), 7);
        QCOMPARE(reader.getSamplingRate(), 20000.0);
        QCOMPARE(reader.getVoltageRange(), 20);
        QCOMPARE(reader.getAmplification(), 1000);
        QCOMPARE(reader.getOffset(), 0);
        QCOMPARE(reader.getLfpInformation(), 1250.0);
    }

    void neuroscopeSettings()
    {
        QCOMPARE(reader.getScreenGain(), 0.2f);
        QCOMPARE(reader.getTraceBackgroundImage(), QString("background.png"));
        // Current behaviour: the video settings are only read from a top-level <video> element,
        // as in session files. The values NeuroScope writes to <neuroscope><video> in parameter files
        // are not read, and the defaults are returned.
        QCOMPARE(reader.getRotation(), 0);
        QCOMPARE(reader.getFlip(), 0);
        QCOMPARE(reader.getBackgroundImage(), QString("-"));
        QCOMPARE(reader.getTrajectory(), 0);
        // For parameter files the spike waveform comes from the NeuroScope section.
        QCOMPARE(reader.getNbSamples(), 40);
        QCOMPARE(reader.getPeakSampleIndex(), 20);
    }

    void sampleRateByExtension()
    {
        QMap<QString, double> expected;
        expected.insert("whl", 39.0625);
        QCOMPARE(reader.getSampleRateByExtension(), expected);
    }

    void anatomicalGroups()
    {
        QMap<int, int> channelsGroups;
        QMap<int, QList<int>> groupsChannels;
        QMap<int, bool> skipStatus;
        reader.getAnatomicalDescription(7, channelsGroups, groupsChannels, skipStatus);

        // Groups are numbered from 1; channels in no group are in the trash group 0.
        QCOMPARE(channelsGroups, (QMap<int, int>{{0, 1}, {1, 1}, {2, 1}, {3, 2}, {4, 2}, {5, 2}, {6, 0}}));
        QCOMPARE(groupsChannels, (QMap<int, QList<int>>{{0, {6}}, {1, {0, 1, 2}}, {2, {3, 4, 5}}}));
        QCOMPARE(skipStatus.value(1), true);
        QCOMPARE(skipStatus.value(0), false);
        QCOMPARE(skipStatus.value(6), false);
    }

    void spikeGroups()
    {
        // The trash group of the anatomical description is passed in.
        QMap<int, int> channelsGroups;
        QMap<int, QList<int>> groupsChannels{{0, {6}}};
        reader.getSpikeDescription(7, channelsGroups, groupsChannels);

        // Channels in no spike group are in group -1, except those in the anatomical trash group.
        QCOMPARE(channelsGroups, (QMap<int, int>{{0, 1}, {1, 1}, {2, 1}, {3, -1}, {4, -1}, {5, -1}, {6, 0}}));
        QCOMPARE(groupsChannels, (QMap<int, QList<int>>{{-1, {3, 4, 5}}, {0, {6}}, {1, {0, 1, 2}}}));
    }

    void channelColors()
    {
        const QList<ChannelDescription> channels = reader.getChannelDescription();
        QCOMPARE(channels.size(), 2);
        QCOMPARE(channels[0].getId(), 0);
        QCOMPARE(channels[0].getColor(), QColor("#0080ff"));
        QCOMPARE(channels[0].getGroupColor(), QColor("#ff0000"));
        QCOMPARE(channels[0].getSpikeGroupColor(), QColor("#00ff00"));
        QCOMPARE(channels[1].getId(), 3);
        QCOMPARE(channels[1].getColor(), QColor("#daff36"));
    }

    void channelOffsets()
    {
        QMap<int, int> offsets;
        reader.getChannelDefaultOffset(offsets);
        QCOMPARE(offsets, (QMap<int, int>{{0, -120}, {3, 45}}));
    }

    void invalidFile()
    {
        writeTextFile(path("invalid.xml"), "<parameters><acquisitionSystem>");
        NeuroscopeXmlReader invalid;
        QVERIFY(!invalid.parseFile(path("invalid.xml"), NeuroscopeXmlReader::PARAMETER));
        QVERIFY(!invalid.parseFile(path("missing.xml"), NeuroscopeXmlReader::PARAMETER));
    }
};

QTEST_GUILESS_MAIN(TestNeuroscopeXmlReader)
#include "test_neuroscopexmlreader.moc"
