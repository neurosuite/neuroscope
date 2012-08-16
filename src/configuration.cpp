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
#include <kapplication.h>       // for 'kapp'
#include <kconfig.h>            // for KConfig
#include <klocale.h>            // for tr()

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
 KConfig* conf = kapp->config();

 //read general options
 conf->setGroup("General");
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
}

void Configuration::write() const {  
 KConfig* conf = kapp->config();
 //write general options
 conf->setGroup("General");
 conf->writeEntry("screenGain",screenGain,true,false,'g',14,false);
 conf->writeEntry("voltageRange",voltageRange);
 conf->writeEntry("amplification",amplification);
 conf->writeEntry("nbChannels",nbChannels);
 conf->writeEntry("datSamplingRate",datSamplingRate,true,false,'g',14,false);
 conf->writeEntry("eegSamplingRate",eegSamplingRate,true,false,'g',14,false);
 conf->writeEntry("backgroundColor",backgroundColor);
 conf->writeEntry("displayPaletteHeaders",displayPaletteHeaders);
 conf->writeEntry("offset",offset);
 conf->writeEntry("resolutionIndex",resolutionIndex);
 conf->writeEntry("eventPosition",eventPosition);
 conf->writeEntry("clusterPosition",clusterPosition);
 conf->writeEntry("nbSamples",nbSamples);
 conf->writeEntry("peakIndex",peakIndex);
 conf->writeEntry("videoSamplingRate",videoSamplingRate,true,false,'g',14,false);
 conf->writeEntry("width",width);
 conf->writeEntry("height",height);
 conf->writeEntry("backgroundImage",backgroundImage);
 conf->writeEntry("rotation",rotation);
 conf->writeEntry("flip",flip);
 conf->writeEntry("drawPositionsOnBackground",drawPositionsOnBackground);
 conf->writeEntry("traceBackgroundImage",traceBackgroundImage);
}

Configuration& configuration() {
  //The C++ standard requires that static variables in functions
  //have to be created upon first call of the function.
  static Configuration conf;
  return conf;
}
