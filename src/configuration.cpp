/***************************************************************************
                          configuration.cpp  -  description
                             -------------------
    begin                : Fri Feb 27 2004
    copyright            : (C) 2003 by Lynn Hazan
    email                :
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
// include files for KDE
#include <QSettings>

//include files for the application
#include "configuration.h"


const float  Configuration::screenGainDefault = 0.2; //half theta amplitude (0.4 ms)
const int  Configuration::voltageRangeDefault = 20;//+-10 volts
const int  Configuration::amplificationDefault = 1000;
const int  Configuration::nbChannelsDefault = 32;
const double  Configuration::datSamplingRateDefault = 20000; //50 s
const double  Configuration::eegSamplingRateDefault = 1250;//800 s
const QColor Configuration::backgroundColorDefault = QColor(Qt::black);
const int  Configuration::offsetDefault = 0;
const int  Configuration::resolutionIndexDefault = 2;
const bool Configuration::displayPaletteHeadersDefault = false;
const int  Configuration::eventPositionDefault = 25;
const int  Configuration::clusterPositionDefault = 25;
const int  Configuration::nbSamplesDefault = 32;
const int  Configuration::peakIndexDefault = 16;
const double  Configuration::videoSamplingRateDefault = 39.0625;
const int  Configuration::widthDefault = 368;
const int  Configuration::heightDefault = 240;
const QString  Configuration::backgroundImageDefault = "";
const int  Configuration::rotationDefault = 0;
const int  Configuration::flipDefault = 0;
const bool Configuration::drawPositionsOnBackgroundDefault = false;
const QString  Configuration::traceBackgroundImageDefault = "";

Configuration::Configuration(){
    read(); // read the settings or set them to the default values
}


void Configuration::read() {
    QSettings settings;
    //read general options
    settings.beginGroup("General");
    screenGain = conf->readDoubleNumEntry("screenGain",screenGainDefault);
    voltageRange = conf->readNumEntry("voltageRange",voltageRangeDefault);
    amplification = conf->readNumEntry("amplification",amplificationDefault);
    nbChannels = conf->readNumEntry("nbChannels",nbChannelsDefault);
    datSamplingRate = conf->readDoubleNumEntry("datSamplingRate",datSamplingRateDefault);
    eegSamplingRate = conf->readDoubleNumEntry("eegSamplingRate",eegSamplingRateDefault);
    backgroundColor = conf->readColorEntry("backgroundColor",&backgroundColorDefault);
    displayPaletteHeaders = conf->readBoolEntry("displayPaletteHeaders",displayPaletteHeadersDefault);
    offset = conf->readNumEntry("offset",offsetDefault);
    resolutionIndex = conf->readNumEntry("resolutionIndex",resolutionIndexDefault);
    eventPosition = conf->readNumEntry("eventPosition",eventPositionDefault);
    clusterPosition = conf->readNumEntry("clusterPosition",clusterPositionDefault);
    nbSamples = conf->readNumEntry("nbSamples",nbSamplesDefault);
    peakIndex = conf->readNumEntry("peakIndex",peakIndexDefault);
    videoSamplingRate = conf->readDoubleNumEntry("videoSamplingRate",videoSamplingRateDefault);
    width = conf->readNumEntry("width",widthDefault);
    height = conf->readNumEntry("height",heightDefault);
    backgroundImage = conf->readEntry("backgroundImage",backgroundImageDefault);
    rotation = conf->readNumEntry("rotation",rotationDefault);
    flip = conf->readNumEntry("flip",flipDefault);
    drawPositionsOnBackground = conf->readBoolEntry("drawPositionsOnBackground",drawPositionsOnBackgroundDefault);
    traceBackgroundImage = conf->readEntry("traceBackgroundImage",traceBackgroundImageDefault);
    settings.endGroup();
}

void Configuration::write() const {  
    QSettings settings;
    //write general options
    settings.beginGroup("General");
    settings.setValue("screenGain",screenGain,true,false,'g',14,false);
    settings.setValue("voltageRange",voltageRange);
    settings.setValue("amplification",amplification);
    settings.setValue("nbChannels",nbChannels);
    settings.setValue("datSamplingRate",datSamplingRate,true,false,'g',14,false);
    settings.setValue("eegSamplingRate",eegSamplingRate,true,false,'g',14,false);
    settings.setValue("backgroundColor",backgroundColor);
    settings.setValue("displayPaletteHeaders",displayPaletteHeaders);
    settings.setValue("offset",offset);
    settings.setValue("resolutionIndex",resolutionIndex);
    settings.setValue("eventPosition",eventPosition);
    settings.setValue("clusterPosition",clusterPosition);
    settings.setValue("nbSamples",nbSamples);
    settings.setValue("peakIndex",peakIndex);
    settings.setValue("videoSamplingRate",videoSamplingRate,true,false,'g',14,false);
    settings.setValue("width",width);
    settings.setValue("height",height);
    settings.setValue("backgroundImage",backgroundImage);
    settings.setValue("rotation",rotation);
    settings.setValue("flip",flip);
    settings.setValue("drawPositionsOnBackground",drawPositionsOnBackground);
    settings.setValue("traceBackgroundImage",traceBackgroundImage);
    settings.endGroup();
}

Configuration& configuration() {
  //The C++ standard requires that static variables in functions
  //have to be created upon first call of the function.
  static Configuration conf;
  return conf;
}
