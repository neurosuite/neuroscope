/***************************************************************************
                          parameterxmlmodifier.h  -  description
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

#ifndef PARAMETERXMLMODIFIER_H
#define PARAMETERXMLMODIFIER_H




//Application specific includes
#include "sessionInformation.h"

//include files for QT
#include <QList>
#include <QMap>
#include <qdom.h> 

// forward declaration
class ChannelColors;

/**
  * Class modifying the parameter xml file.
  *@author Lynn Hazan
  */

class ParameterXmlModifier {
public:

    /**Constructor.
  */
    ParameterXmlModifier();

    ~ParameterXmlModifier();

    /**Opens and parses the file with the @p url.
  * @param url url of the file to open.
  * @return true if the file was correctly parse, false othewise.
  */
    bool parseFile(const QString& url);

    /**Writes the modified xml tree to a parameter file given by @p url.
  * @param url url of the file to write to.
  * @return true if the parameter file could be write to disk, false otherwise.
  */
    bool writeTofile(const QString& url);

    /**
  * Finds the first child node with the tag name @p childName direct child of the root node.
  * @param childName name of the child node tag to look up.
  * @return the first node starting from root having @p tagName as tag name.
  */
    QDomNode findDirectChild(const QString& childName);

    /**
  * Finds the first child node with the tag name @p childName direct child of the @p ancestor node.
  * @param childName name of thechild node tag to look up.
  * @param ancestor starting node from which to look up for the node.
  * @return the first node starting from @p ancestor having @p tagName as tag name.
  */
    QDomNode findDirectChild(const QString& childName,const QDomNode& ancestor);

    /**
  * Finds the first child node with the tag name @p childName direct child of the @p ancestor node.
  * This node has to contain a direct child node with the tag name @p grandChildName with a text
  * value of @p value.
  * @param childName name of the child node tag to look up.
  * @param grandChildName name of the grandchild node tag to look up.
  * @param value value of the textNode contained in @p grandChildName.
  * @param ancestor starting node from which to look up for the child node.
  * @return the first child node corresponding to the criteria.
  */
    QDomNode findDirectChild(const QString &childName, const QString &grandChildName, const QString &value, const QDomNode &ancestor);

    /**
  * Modifies the elements related to the acquisition system.
  * @param resolution resolution of the system in bits.
  * @param nbChannels number of channels.
  * @param samplingRate the sampling rate in hertz.
  * @param voltageRange voltage range of the acquisition system in volts.oltage range of the acquisition system in volts.
  * @param amplification amplification of the acquisition system.
  * @param offset initial offset for all the traces.
  * @return true if the modification succeded, false otherwise.
  */
    bool setAcquisitionSystemInformation(int resolution,int nbChannels,double samplingRate,int voltageRange,int amplification,int offset);

    /**
  * Modifies the element related to the field potentials.
  * @param lfpSamplingRate local field potential sampling rate in hertz.
  * @return true if the modification succeded, false otherwise.
  */
    bool setLfpInformation(double lfpSamplingRate);

    /**
  * Creates the elements containing NeuroScope miscellaneous information.
  * @param screenGain screen gain in milivolts by centimeters used to display the field potentiels.
  * @param traceBackgroungImage image used as background for the trace view.
  */
    void setMiscellaneousInformation(float screenGain,const QString& traceBackgroungImage);


    /**
  * Creates the elements containing the video information of the neuroscope element.
  * @param rotation video image rotation angle.
  * @param flip video image flip orientation, 0 stands for none, 1 for vertical and 2 for horizontal.
  * @param backgroundPath path of the background image.
  * @param drawTrajectory all the positions contained in a position file can be used to create a background image for the PositionView.
  * This value tells if such background has to be created.
  */
    void setNeuroscopeVideoInformation(int rotation,int flip,const QString& backgroundPath,int drawTrajectory);

    /**
  * Modifies the elements containing the video information.
  * @param width video image width.
  * @param height video image height.
  * @return true if the modification succeded, false otherwise.
  */
    bool setVideoInformation(int width,int height);

    /**
  * Modifies the elements related to the channels display.
  * @param channelColors list of colors for the channels (color use to display the channel,
  * color of the anatomical group to which the channel belongs,
  * color of the spike group to which the channel belongs).
  * @param channelsGroups map given to which group each channel belongs.
  * @param channelDefaultOffsets map given the default channel offsets.
  * @return true if the modification succeded, false otherwise.
  */
    bool setChannelDisplayInformation(ChannelColors* channelColors,QMap<int,int>& channelsGroups,QMap<int,int>& channelDefaultOffsets);

    /**
  * Modifies the elements related to the anatomical description.
  * @param anatomicalGroups map given to which anatomical group each channel belongs.
  * @param skipStatus map given the skip status of the channels.
  * @return true if the modification succeded, false otherwise.
  */
    bool setAnatomicalDescription(QMap<int, QList<int> >& anatomicalGroups,QMap<int,bool> skipStatus);

    /**
  * Modifies the elements related to the spike detection.
  * @param nbSamples number of samples in a spike.
  * @param peakSampleIndex sample index corresponding to the peak of the spike.
  * @param spikeGroups map given to which spike group each channel belongs.
  * @return true if the modification succeded, false otherwise.
  */
    bool setSpikeDetectionInformation(int nbSamples,int peakSampleIndex,QMap<int, QList<int> >& spikeGroups);

    /**
  * Modifies the elements related to the spike detection.
  * @param spikeGroups map given to which spike group each channel belongs.
  * @return true if the modification succeded, false otherwise.
  */
    bool setSpikeDetectionInformation(QMap<int, QList<int> >& spikeGroups);

    /**A base file name can be used for different kind of files corresponding to the same data and having
  * different sampling rates. Each file is identified by its extension. this function modifies the elements related to the mapping
  * between the file extensions with the sampling rates for the current document. This map does not
  * includes the sampling rates for the extension dat and eeg, they are treated separately.
  * @param extensionSamplingRates map between file extension and the sampling rate.
  * @return true if the modification succeded, false otherwise.
  */
    bool setSampleRateByExtension(const QMap<QString, double> &extensionSamplingRates);


private:

    /**The session document.*/
    QDomDocument doc;

    /**The root element.*/
    QDomNode root;

    /**The miscellaneous element.*/
    QDomNode miscellaneous;

    /**The acquisition system element.*/
    QDomNode acquisitionSystem;

    /**The element containing the information about the channel display.*/
    QDomNode channels;

    /**The anatomical description element.*/
    QDomNode anatomicalDescription;

    /**The spike description element.*/
    QDomNode spikeDetection;

    /**The neuroscope element.*/
    QDomNode neuroscope;

    /**The spikes element contained in the neuroscope element.*/
    QDomElement spikes;
    /**The element containing the video information in the neuroscope element.*/
    QDomElement neuroscopeVideo;

    /**The element containing the video information.*/
    QDomNode video;

    /**The local field potential sampling rate element.*/
    QDomNode lfp;

    /**The files element contains the sampling rates by extension.*/
    QDomNode files;

    /**True if a video node has been created, false otherwise.*/
    bool  newVideoNode;

    /**True if a files node has been created, false otherwise.*/
    bool newFilesNode;

    /**The parameter file as it has been loaded.*/
    QString initialXmlDocument;

    static const QString parameterVersion;

};

#endif
