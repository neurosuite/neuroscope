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
#include <QMessageBox>
//include files for the application
#include "propertiesdialog.h"

#include "qhelpviewer.h"
#include "config-neuroscope.h"


PropertiesDialog::PropertiesDialog(QWidget *parent)
    : QPageDialog(parent)
    ,modified(false)
    ,nbChannelsModified(false)
    ,oops(false)
    ,atStartUp(false)
{

    setButtons(Help | Ok | Cancel);
    setDefaultButton(Ok);
    setFaceType(Tabbed);
    setWindowTitle(tr("File Properties"));


    setHelp("properties","neuroscope");

    //page "Channels"

    QWidget * w = new QWidget(this);
    properties = new Properties(w);
    addPage(properties,tr("Channels"));

    //adding "Units" page
    w = new QWidget(this);
    clusterProperties = new ClusterProperties(w);
    addPage(clusterProperties,tr("Units"));

    //adding "Positions" page
    w = new QWidget(this);
    positionProperties = new PositionProperties(w);
    mPositionPageIndex = addPage(positionProperties,tr("Positions"));

    // connect interactive widgets and selfmade signals to the enableApply slotDefault
    connect(properties->nbChannelsLineEdit,SIGNAL(textChanged(QString)),this,SLOT(channelNbModified()));
    connect(properties->screenGainLineEdit,SIGNAL(textChanged(QString)),this,SLOT(propertyModified()));
    connect(properties->voltageRangeLineEdit,SIGNAL(textChanged(QString)),this,SLOT(propertyModified()));
    connect(properties->amplificationLineEdit,SIGNAL(textChanged(QString)),this,SLOT(propertyModified()));
    connect(properties->samplingRateLineEdit,SIGNAL(textChanged(QString)),this,SLOT(propertyModified()));
    connect(properties->asSamplingRateLineEdit,SIGNAL(textChanged(QString)),this,SLOT(propertyModified()));
    connect(properties->offsetLineEdit,SIGNAL(textChanged(QString)),this,SLOT(propertyModified()));
    connect(properties->resolutionComboBox,SIGNAL(activated(int)),this,SLOT(propertyModified()));
    connect(properties->traceBackgroundLineEdit,SIGNAL(textChanged(QString)),this,SLOT(propertyModified()));
    connect(this,SIGNAL(okClicked()),this,SLOT(slotVerify()));
    connect(clusterProperties->nbSamplesLineEdit,SIGNAL(textChanged(QString)),this,SLOT(propertyModified()));
    connect(clusterProperties->peakIndexLineEdit,SIGNAL(textChanged(QString)),this,SLOT(propertyModified()));
    connect(positionProperties->samplingRateLineEdit,SIGNAL(textChanged(QString)),this,SLOT(propertyModified()));
    connect(positionProperties->widthLineEdit,SIGNAL(textChanged(QString)),this,SLOT(propertyModified()));
    connect(positionProperties->heightLineEdit,SIGNAL(textChanged(QString)),this,SLOT(propertyModified()));
    connect(positionProperties->backgroundLineEdit,SIGNAL(textChanged(QString)),this,SLOT(propertyModified()));
    connect(positionProperties->rotateComboBox,SIGNAL(activated(int)),this,SLOT(propertyModified()));
    connect(positionProperties->filpComboBox,SIGNAL(activated(int)),this,SLOT(propertyModified()));
    connect(positionProperties->checkBoxBackground,SIGNAL(clicked()),this,SLOT(propertyModified()));

    connect(this,SIGNAL(helpClicked()),SLOT(slotHelp()));
}
PropertiesDialog::~PropertiesDialog(){
}

void PropertiesDialog::slotHelp()
{
    QHelpViewer *helpDialog = new QHelpViewer(this);
    helpDialog->setHtml(NEUROSCOPE_DOC_PATH + QLatin1String("index.html"));
    helpDialog->setAttribute( Qt::WA_DeleteOnClose );
    helpDialog->show();

}

void PropertiesDialog::updateDialog(int channelNb,double SR, int resolution,int offset,float screenGain,int voltageRange,
                                    int amplification,int nbSamples,int peakIndex,double videoSamplingRate, int width,
                                    int height, const QString& backgroundImage,int rotation,int flip,double acquisitionSystemSamplingRate,bool positionsBackground,const QString& traceBackgroundImage){
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
        else
            modified = true;
    }
    else{
        if(nbChannelsModified)
            modified = true;
    }
}

