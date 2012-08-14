/***************************************************************************
                          propertiesdialog.cpp  -  description
                             -------------------
    begin                : Sun Feb 29 2004
    copyright            : (C) 2004 by Lynn Hazan
    email                : lynn.hazan.myrealbox.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
// include files for QT
#include <qlayout.h>        // for QVBoxLayout
#include <qlabel.h>         // for QLabel
#include <qframe.h>         // for QFrame

// include files for KDE
#include <kpushbutton.h>    // for KPushButton
#include <klocale.h>        // for i18n()
#include <kglobal.h>        // for KGlobal
#include <klineedit.h>      // for KLineEdit
#include <kmessagebox.h>    // for KMessageBox

//include files for the application
#include "propertiesdialog.h"
        
//General C++ include files
#include <iostream>
using namespace std;

PropertiesDialog::PropertiesDialog(QWidget *parent, const char *name, WFlags f):
 KDialogBase(Tabbed, i18n("File Properties"), Help|Ok|Cancel, Ok, parent, name, f),
 modified(false),nbChannelsModified(false),oops(false),atStartUp(false){

 setHelp("properties","neuroscope");

 //page "Channels"
 QFrame* channelFrame = addPage(i18n("Channels"));
 QVBoxLayout* frameLayout = new QVBoxLayout(channelFrame,0,0);
 properties = new Properties(channelFrame);
 frameLayout->addWidget(properties);

 //adding "Units" page
 QFrame* clusterFrame = addPage(i18n("Units"));
 frameLayout = new QVBoxLayout(clusterFrame,0,0);
 clusterProperties = new ClusterProperties(clusterFrame);
 frameLayout->addWidget(clusterProperties);

 //adding "Positions" page
 QFrame* positionFrame = addPage(i18n("Positions"));
 frameLayout = new QVBoxLayout(positionFrame,0,0);
 positionProperties = new PositionProperties(positionFrame);
 //hard coded as there is a problem with the pageIndex() method
 positionPageIndex = 2;
 frameLayout->addWidget(positionProperties);

 
 // connect interactive widgets and selfmade signals to the enableApply slotDefault
 connect(properties->nbChannelsLineEdit,SIGNAL(textChanged(const QString&)),this,SLOT(channelNbModified()));
 connect(properties->screenGainLineEdit,SIGNAL(textChanged(const QString&)),this,SLOT(propertyModified()));
 connect(properties->voltageRangeLineEdit,SIGNAL(textChanged(const QString&)),this,SLOT(propertyModified()));
 connect(properties->amplificationLineEdit,SIGNAL(textChanged(const QString&)),this,SLOT(propertyModified()));
 connect(properties->samplingRateLineEdit,SIGNAL(textChanged(const QString&)),this,SLOT(propertyModified()));
 connect(properties->asSamplingRateLineEdit,SIGNAL(textChanged(const QString&)),this,SLOT(propertyModified()));
 connect(properties->offsetLineEdit,SIGNAL(textChanged(const QString&)),this,SLOT(propertyModified()));
 connect(properties->resolutionComboBox,SIGNAL(activated(int)),this,SLOT(propertyModified()));
 connect(properties->traceBackgroundLineEdit,SIGNAL(textChanged(const QString&)),this,SLOT(propertyModified()));
 connect(this,SIGNAL(okClicked()),this,SLOT(slotVerify()));
 connect(clusterProperties->nbSamplesLineEdit,SIGNAL(textChanged(const QString&)),this,SLOT(propertyModified()));
 connect(clusterProperties->peakIndexLineEdit,SIGNAL(textChanged(const QString&)),this,SLOT(propertyModified()));
 connect(positionProperties->samplingRateLineEdit,SIGNAL(textChanged(const QString&)),this,SLOT(propertyModified()));
 connect(positionProperties->widthLineEdit,SIGNAL(textChanged(const QString&)),this,SLOT(propertyModified()));
 connect(positionProperties->heightLineEdit,SIGNAL(textChanged(const QString&)),this,SLOT(propertyModified()));
 connect(positionProperties->backgroundLineEdit,SIGNAL(textChanged(const QString&)),this,SLOT(propertyModified()));
 connect(positionProperties->rotateComboBox,SIGNAL(activated(int)),this,SLOT(propertyModified()));
 connect(positionProperties->filpComboBox,SIGNAL(activated(int)),this,SLOT(propertyModified()));
 connect(positionProperties->checkBoxBackground,SIGNAL(clicked()),this,SLOT(propertyModified()));
}
PropertiesDialog::~PropertiesDialog(){
}

void PropertiesDialog::updateDialog(int channelNb,double SR, int resolution,int offset,float screenGain,int voltageRange,
                                    int amplification,int nbSamples,int peakIndex,double videoSamplingRate, int width,
                                    int height, QString backgroundImage,int rotation,int flip,double acquisitionSystemSamplingRate,bool positionsBackground,QString traceBackgroundImage){
  properties->setScreenGain(screenGain);
  properties->setAcquisitionSystemSamplingRate(acquisitionSystemSamplingRate);  
  properties->setVoltageRange(voltageRange);
  properties->setAmplification(amplification);
  properties->setNbChannels(channelNb);
  properties->setSamplingRate(SR);
  properties->setOffset(offset);
  properties->setResolution(resolution);
  properties->setTraceBackgroundImage(traceBackgroundImage);
  clusterProperties->setNbSamples(nbSamples);
  clusterProperties->setPeakIndex(peakIndex);
  positionProperties->setSamplingRate(videoSamplingRate);
  positionProperties->setWidth(width);
  positionProperties->setHeight(height);
  //Rotation and flip values have to be set before calling setBackgroundImage
  positionProperties->setRotation(rotation);
  positionProperties->setFlip(flip);
  positionProperties->setBackgroundImage(backgroundImage);
  positionProperties->setPositionsBackground(positionsBackground);
  
  nbChannels = channelNb;
}


void PropertiesDialog::slotVerify(){  
 if(nbChannels != properties->getNbChannels() && !atStartUp){
  if(KMessageBox::warningContinueCancel(this, i18n("Changing the number of channels "
      "will rest all the groups. Do you wish to continue?"), i18n("Change the number of channels?"),
      i18n("Continue"))==KMessageBox::Cancel){
   properties->setNbChannels(nbChannels);
   nbChannelsModified = false;
   oops = true;
  }
  else modified = true;   
 }
 else{
  if(nbChannelsModified) modified = true; 
 }
}





#include "propertiesdialog.moc"
