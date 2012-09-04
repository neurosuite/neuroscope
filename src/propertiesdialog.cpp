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
#include <QLayout>        // for QVBoxLayout
#include <QLabel>         // for QLabel
#include <q3frame.h>         // for QFrame
//Added by qt3to4:
#include <Q3VBoxLayout>
#include <QMessageBox>
//include files for the application
#include "propertiesdialog.h"


PropertiesDialog::PropertiesDialog(QWidget *parent, const char *name, Qt::WFlags f):
    QPageDialog(parent)
  ,modified(false),
    nbChannelsModified(false)
  ,oops(false)
  ,atStartUp(false)
{

    setButtons(Help | Default | Ok | Apply | Cancel);
    setDefaultButton(Ok);
    setFaceType(Tabbed);
    setCaption(tr("File Properties"));


    setHelp("properties","neuroscope");

    //page "Channels"

    QWidget * w = new QWidget(this);
    properties = new Properties(w);
    addPage(properties,tr("Channels"));

    //adding "Units" page
    w = new QWidget(this);
    clusterProperties = new ClusterProperties(w);
    addPage(properties,tr("Units"));

    //adding "Positions" page
    w = new QWidget(this);
    positionProperties = new PositionProperties(w);
    addPage(positionProperties,tr("Positions"));

    //hard coded as there is a problem with the pageIndex() method
    positionPageIndex = 2;

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
        if(QMessageBox::warning(this, tr("Changing the number of channels "
                                                       "will rest all the groups. Do you wish to continue?"), tr("Change the number of channels?"),
                                              tr("Continue"))==QMessageBox::Cancel){
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
