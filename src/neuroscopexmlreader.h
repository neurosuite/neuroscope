/***************************************************************************
                          neuroscopexmlreader.h  -  description
                             -------------------
    begin                : Tue Mar 2 2004
    copyright            : (C) 2004 by Lynn Hazan
    email                : lynn.hazan.myrealbox.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef NEUROSCOPEXMLREADER_H
#define NEUROSCOPEXMLREADER_H

//include files for QT
#include <QList> 
#include <QDomNode>

//Application specific includes
#include "sessionInformation.h"

/**
  * Class reading the parameter and session xml file.
  *@author Lynn Hazan
  */

class NeuroscopeXmlReader {
public:

    /**Type of xml file supported by this class.*/
    enum fileType{PARAMETER=0,SESSION=1};

    NeuroscopeXmlReader();
    ~NeuroscopeXmlReader();

    /**Opens and parses the file with the @p url.
  * @param url url of the file to open.
  * @param type type of the xml file to open.
  * @return true if the file was correctly parse, false othewise.
  */
    bool parseFile(const QString& url,fileType type);

    /**Closes the currently open file.*/
    void closeFile();

    /**
  * Returns the resolution in bits.
  * @return resolution.
  */
    int getResolution() const;

    /**
  * Returns the number of channels.
  * @return number of channels.
  */
    int getNbChannels() const;

    /**
  * Returns the sampling rate of the system in hertz.
  * @return sampling rate in hertz.
  */
    double getSamplingRate() const;

    /**
  * Returns the sampling rate used to upsample the waveforms.
  * @return upsampling rate in hertz.
  */
    double getUpsamplingRate() const;

    /**
  * Returns the local field potential sampling rate in hertz.
  * @return local field potential sampling rate in hertz.
  */
    double getLfpInformation() const;


    /**Returns the screen gain in milivolts by centimeters used to display the field potentiels
  * or zero if the element could not be found in the file.
  * @return the screen gain.
  */
    float getScreenGain() const;

    /**Returns the voltage range of the acquisition system in volts,
  * or zero if the element could not be found in the file.
  * @return the voltage range.
  */
    int getVoltageRange() const;

    /**Returns the amplification of the acquisition system,
  * or zero if the element could not be found in the file.
  * @return the amplification.
  */
    int getAmplification() const;


    /**Returns the offset store in the session file,
  * or zero if the element could not be found in the file.
  * @return offset.
  */
    int getOffset() const;

    /** Returns the list of ChannelDescription, class given the color for each channel.
  * @return list of ChannelDescription.
  */
    QList<ChannelDescription> getChannelDescription() const;

    /** Returns the list of channel default offsets.
  * @param channelDefaultOffsets empty map to be filled with the channel default offsets.
  */
    void getChannelDefaultOffset(QMap<int,int>& channelDefaultOffsets);

    /**
  * Returns the anatomical description.
  * @param nbChannels total number of channels.
  * @param displayChannelsGroups reference to the map given the correspondance between the channel ids and the display group ids.
  * @param displayGroupsChannels reference to the map given the correspondance between the display group ids and the channel ids.
  * @param skipStatus reference to the map given the correspondance between the channels and their skip status.
  */
    void getAnatomicalDescription(int nbChannels,QMap<int,int>& displayChannelsGroups,QMap<int, QList<int> >& displayGroupsChannels,
                                  QMap<int,bool>& skipStatus);

    /**
  * Returns the spike description.
  * @param nbChannels total number of channels.
  * @param spikeChannelsGroups reference to the map given the correspondance between the channel ids and the spike group ids.
  * @param spikeGroupsChannels reference to the map given the correspondance between the spike group ids and the channel ids.
  */
    void getSpikeDescription(int nbChannels,QMap<int,int>& spikeChannelsGroups,QMap<int, QList<int> >& spikeGroupsChannels);

    /**Returns the number of samples in a spike,
  * or zero if the element could not be found in the file.
  * @return number of samples.
  */
    int getNbSamples() const;

    /**Returns the length for a spike,
  * or zero if the corresponding information could not be found in the file.
  * @return length of a spike in miliseconds.
  */
    float getWaveformLength() const;

    /**Returns the sample index corresponding to the peak of the spike,
  * or zero if the element could not be found in the file.
  * @return index.
  */
    int getPeakSampleIndex() const;

    /**Returns the Length corresponding to the index of the spike peak,
  * or zero if the corresponding could not be found in the file.
  * @return length of the index of the peak index in miliseconds.
  */
    float getPeakSampleLength() const;


    /** Returns the list of files which where loaded in the last session, cluster, spike or event files.
  * @return list of the files to load.
  */
    QList<SessionFile> getFilesToLoad();

    /** Returns the list of DisplayInformation, class given the information on a display.
  * @return list of DisplayInformation.
  */
    QList<DisplayInformation> getDisplayInformation();


    /**A base file name can be used for different kind of files corresponding to the same data and having
  * different sampling rates. Each file is identified by its extension. this function returns the map
  * of the file extensions with the sampling rates for the current document. This map does not
  * includes the sampling rates for the extension dat and eeg, they treated separately.
  * @return map between file extension and the sampling rate.
  */
    QMap<QString,double> getSampleRateByExtension();

    /**Returns the video image width.
  * @return width.
  */
    int getVideoWidth() const;

    /**Returns the video image height.
  * @return height.
  */
    int getVideoHeight() const;

    /**Returns the video image rotation angle (0,90,180,270). The angle is counted counterclockwise.
  * @return rotation angle.
  */
    int getRotation() const;

    /**Returns the video image flip orientation. 0 stands for none, 1 for vertical and 2 for horizontal.
  * @return flip orientation.
  */
    int getFlip() const;

    /**All the positions contained in a position file can be used to create a background image for the PositionView.
  * The value return by this function tells if such background has to be created.
  * @return 1 if the all the positions contain in the position file have to be drawn on the background, 0 otherwise.
   */
    int getTrajectory() const;

    /**Gets the url of the background image.
  * @return the url of the background image, an empty string if no background has been choosen and a dash if the entry does not exist in the file.
  */
    QString getBackgroundImage() const;

    /**Gets the url of the background image use for the trace view.
  * @return the url of the background image use for the trace view, an empty string if no background has been choosen and a dash if the entry does not exist in the file.
  */
    QString getTraceBackgroundImage() const;

    /**Returns the version of current file.
  * @return version.
  */
    QString getVersion()const{return readVersion;}

    /**Returns the version of current file.
  * @return version.
  */
    NeuroscopeXmlReader::fileType getType()const{return type;}

private:
    fileType type;
    QString readVersion;
    QDomNode documentNode;

};

#endif
