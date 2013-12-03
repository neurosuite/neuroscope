/***************************************************************************
                          parameterxmlcreator.h  -  description
                             -------------------
    begin                : Fri Apr 2 2004
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

#ifndef PARAMETERXMLCREATOR_H
#define PARAMETERXMLCREATOR_H




//Application specific includes
#include "sessionInformation.h"

//include files for QT
#include <QList>
#include <QMap>
#include <qdom.h> 

// forward declaration
class ChannelColors;

/**
  * Class creating the parameter xml file.
  *@author Lynn Hazan
  */

class ParameterXmlCreator {
public:

    /**Constructor which will create a parameter file.
  */
    ParameterXmlCreator();

    ~ParameterXmlCreator();

    /**Writes the xml tree to a parameter file given by @p url.
  * @param url url of the file to write to.
  * @return true if the parameter file could be write to disk, false otherwise.
  */
    bool writeTofile(const QString& url);

    /**
  * Creates the elements related to the acquisition system.
  * @param resolution resolution of the system in bits.
  * @param nbChannels number of channels.
  * @param samplingRate the sampling rate in hertz.
  * @param voltageRange voltage range of the acquisition system in volts.oltage range of the acquisition system in volts.
  * @param amplification amplification of the acquisition system.
  * @param offset initial offset for all the traces.
  */
    void setAcquisitionSystemInformation(int resolution,int nbChannels,double samplingRate,int voltageRange,int amplification,int offset);

    /**
  * Creates the element related to the field potentials.
  * @param lfpSamplingRate local field potential sampling rate in hertz.
  */
    void setLfpInformation(double lfpSamplingRate);

    /**
  * Creates the elements containing NeuroScope miscellaneous information.
  * @param screenGain screen gain in milivolts by centimeters used to display the field potentiels.
  * @param traceBackgroundImage image used as background for the trace view.
  */
    void setMiscellaneousInformation(float screenGain, const QString &traceBackgroundImage);

    /**
  * Creates the elements related to the channels colors.
  * @param channelColors list of colors for the channels (color use to display the channel,
  * color of the anatomical group to which the channel belongs,
  * color of the spike group to which the channel belongs).
  * @param channelsGroups map given to which group each channel belongs.
  * @param channelDefaultOffsets map given the default channel offsets.
  */
    void setChannelDisplayInformation(ChannelColors* channelColors,QMap<int,int>& channelsGroups,QMap<int,int>& channelDefaultOffsets);

    /**
  * Creates the elements related to the anatomical description.
  * @param anatomicalGroups map given to which anatomical group each channel belongs.
  * @param skipStatus map given the skip status of the channels.
  */
    void setAnatomicalDescription(QMap<int, QList<int> >& anatomicalGroups, const QMap<int, bool> &skipStatus);

    /**
  * Creates the elements related to the spike detection.
  * @param nbSamples number of samples in a spike.
  * @param peakSampleIndex sample index corresponding to the peak of the spike.
  * @param spikeGroups map given to which spike group each channel belongs.
  */
    void setSpikeDetectionInformation(int nbSamples,int peakSampleIndex,QMap<int, QList<int> >& spikeGroups);

    /**
  * Creates the elements related to the spike detection.
  * @param spikeGroups map given to which spike group each channel belongs.
  */
    void setSpikeDetectionInformation(QMap<int, QList<int> >& spikeGroups);


    /**
  * Creates the elements containing the video information of the neuroscope element.
  * @param rotation video image rotation angle.
  * @param flip video image flip orientation, 0 stands for none, 1 for vertical and 2 for horizontal.
  * @param backgroundPath path of the background image.
  * @param drawTrajectory all the positions contained in a position file can be used to create a background image for the PositionView.
  * This value tells if such background has to be created.
  */
    void setNeuroscopeVideoInformation(int rotation, int flip, const QString &backgroundPath, int drawTrajectory);

    /**
  * Creates the elements containing the video information.
  * @param width video image width.
  * @param height video image height.
  */
    void setVideoInformation(int width,int height);

    /**A base file name can be used for different kind of files corresponding to the same data and having
  * different sampling rates. Each file is identified by its extension. this function creates the elements related to the mapping
  * between the file extensions with the sampling rates for the current document. This map does not
  * includes the sampling rates for the extension dat and eeg, they are treated separately.
  * @param extensionSamplingRates map between file extension and the sampling rate.
  */
    void setSampleRateByExtension(const QMap<QString, double> &extensionSamplingRates);

private:

    /**The session document.*/
    QDomDocument doc;

    /**The root element.*/
    QDomElement root;

    /**The acquisition system element.*/
    QDomElement acquisitionSystem;

    /**The local field potential sampling rate element.*/
    QDomElement lfp;

    /**The miscellaneous element.*/
    QDomElement miscellaneous;

    /**The element containing the information about the channel display.*/
    QDomElement channels;

    /**The anatomical description element.*/
    QDomElement anatomicalDescription;

    /**The spike detection element.*/
    QDomElement spikeDetection;

    /**The spikes element contained in the neuroscope element.*/
    QDomElement spikes;
    /**The element containing the video information.*/
    QDomElement video;

    /**The element containing the video information in the neuroscope element.*/
    QDomElement neuroscopeVideo;

    /**The files element contains the sampling rates by extension.*/
    QDomElement files;

    static const QString parameterVersion;

    
};

#endif
