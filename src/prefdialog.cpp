/***************************************************************************
                          prefdialog.cpp  -  description
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
// include files for QT
#include <QLayout>        // for QVBoxLayout
#include <QLabel>         // for QLabel
#include <q3frame.h>         // for QFrame
#include <QTabWidget>
//Added by qt3to4:
#include <Q3VBoxLayout>

#include <QMessageBox>


//include files for the application
#include "prefdialog.h"     // class PrefDialog

#include "configuration.h"          // class Configuration and Config()
#include "prefgeneral.h"            // class PrefGeneral
#include "prefdefaults.h" // class prefDefaults
#include "positionproperties.h"
#include "clusterproperties.h"

/**
  *@author Lynn Hazan
*/

PrefDialog::PrefDialog(QWidget *parent)
    : QPageDialog(parent)
{

    setButtons(Help | Default | Ok | Apply | Cancel);
    setDefaultButton(Ok);
    setFaceType(List);
    setCaption(tr("Preferences"));


    setHelp("settings","neuroscope");
    
    //adding page "General options"
    QWidget * w = new QWidget(this);
    prefGeneral = new PrefGeneral(w);
    QPageWidgetItem *item = new QPageWidgetItem(prefGeneral,tr("General"));
    item->setHeader(tr("NeuroScope Configuration"));
    item->setIcon(QIcon(":/icons/kfm"));


    addPage(item);


    w = new QWidget(this);
    QTabWidget* tabWidget = new QTabWidget(w);
    //adding "Channels" tab
    prefDefaults = new PrefDefaults();
    tabWidget->addTab(prefDefaults,tr("Channels"));
    //connect(prefDefaults,SIGNAL(changed( bool )),this, SIGNAL(changed( bool )) );
    //adding "Units" tab
    clusterProperties = new ClusterProperties();
    tabWidget->addTab(clusterProperties,tr("Units"));
    //adding "Positions" tab
    positionProperties = new PositionProperties();
    tabWidget->addTab(positionProperties,tr("Positions"));


    item = new QPageWidgetItem(tabWidget,tr("Defaults"));
    item->setHeader(tr("NeuroScope Defaults"));
    item->setIcon(QIcon(":/icons/default"));


    addPage(item);

    // connect interactive widgets and selfmade signals to the enableApply slotDefault
    connect(prefGeneral->headerCheckBox,SIGNAL(clicked()),this,SLOT(enableApply()));
    connect(prefGeneral->backgroundColorButton,SIGNAL(colorChanged(const QColor&)),this,SLOT(enableApply()));
    connect(prefGeneral->eventPositionSpinBox,SIGNAL(valueChanged(int)),this,SLOT(enableApply()));
    connect(prefGeneral->clusterPositionSpinBox,SIGNAL(valueChanged(int)),this,SLOT(enableApply()));
    connect(prefDefaults->screenGainLineEdit,SIGNAL(textChanged(const QString&)),this,SLOT(enableApply()));
    connect(prefDefaults->voltageRangeLineEdit,SIGNAL(textChanged(const QString&)),this,SLOT(enableApply()));
    connect(prefDefaults->amplificationLineEdit,SIGNAL(textChanged(const QString&)),this,SLOT(enableApply()));
    connect(prefDefaults->nbChannelsLineEdit,SIGNAL(textChanged(const QString&)),this,SLOT(enableApply()));
    connect(prefDefaults->datSamplingRateLineEdit,SIGNAL(textChanged(const QString&)),this,SLOT(enableApply()));
    connect(prefDefaults->eegSamplingRateLineEdit,SIGNAL(textChanged(const QString&)),this,SLOT(enableApply()));
    connect(prefDefaults->offsetLineEdit,SIGNAL(textChanged(const QString&)),this,SLOT(enableApply()));
    connect(prefDefaults->resolutionComboBox,SIGNAL(activated(int)),this,SLOT(enableApply()));
    connect(prefDefaults->traceBackgroundLineEdit,SIGNAL(textChanged(const QString&)),this,SLOT(enableApply()));
    connect(clusterProperties->nbSamplesLineEdit,SIGNAL(textChanged(const QString&)),this,SLOT(enableApply()));
    connect(clusterProperties->peakIndexLineEdit,SIGNAL(textChanged(const QString&)),this,SLOT(enableApply()));
    connect(positionProperties->samplingRateLineEdit,SIGNAL(textChanged(const QString&)),this,SLOT(enableApply()));
    connect(positionProperties->widthLineEdit,SIGNAL(textChanged(const QString&)),this,SLOT(enableApply()));
    connect(positionProperties->heightLineEdit,SIGNAL(textChanged(const QString&)),this,SLOT(enableApply()));
    connect(positionProperties->backgroundLineEdit,SIGNAL(textChanged(const QString&)),this,SLOT(enableApply()));
    connect(positionProperties->rotateComboBox,SIGNAL(activated(int)),this,SLOT(enableApply()));
    connect(positionProperties->filpComboBox,SIGNAL(activated(int)),this,SLOT(enableApply()));
    connect(positionProperties->checkBoxBackground,SIGNAL(clicked()),this,SLOT(enableApply()));

    connect(this, SIGNAL(applyClicked()), SLOT(slotApply()));
    connect(this, SIGNAL(defaultClicked()), SLOT(slotDefault()));

    applyEnable = false;
}

void PrefDialog::updateDialog() {
    prefGeneral->setBackgroundColor(configuration().getBackgroundColor());
    prefGeneral->setPaletteHeaders(configuration().isPaletteHeadersDisplayed());
    prefGeneral->setEventPosition(configuration().getEventPosition());
    prefGeneral->setClusterPosition(configuration().getClusterPosition());
    prefDefaults->setScreenGain(configuration().getScreenGain());
    prefDefaults->setVoltageRange(configuration().getVoltageRange());
    prefDefaults->setAmplification(configuration().getAmplification());
    prefDefaults->setNbChannels(configuration().getNbChannels());
    prefDefaults->setDatSamplingRate(configuration().getDatSamplingRate());
    prefDefaults->setEegSamplingRate(configuration().getEegSamplingRate());
    prefDefaults->setOffset(configuration().getOffset());
    prefDefaults->setResolutionIndex(configuration().getResolutionIndex());
    prefDefaults->setTraceBackgroundImage(configuration().getTraceBackgroundImage());
    clusterProperties->setNbSamples(configuration().getNbSamples());
    clusterProperties->setPeakIndex(configuration().getPeakIndex());
    positionProperties->setSamplingRate(configuration().getVideoSamplingRate());
    positionProperties->setWidth(configuration().getWidth());
    positionProperties->setHeight(configuration().getHeight());
    //Rotation and flip values have to set before calling setBackgroundImage
    positionProperties->setRotation(configuration().getRotation());
    positionProperties->setFlip(configuration().getFlip());
    positionProperties->setBackgroundImage(configuration().getBackgroundImage());
    positionProperties->setPositionsBackground(configuration().getPositionsBackground());

    enableButtonApply(false);   // disable apply button
    applyEnable = false;
}


void PrefDialog::updateConfiguration(){
    configuration().setBackgroundColor(prefGeneral->getBackgroundColor());
    configuration().setPaletteHeaders(prefGeneral->isPaletteHeadersDisplayed());
    configuration().setEventPosition(prefGeneral->getEventPosition());
    configuration().setClusterPosition(prefGeneral->getClusterPosition());
    configuration().setScreenGain(prefDefaults->getScreenGain());
    configuration().setVoltageRange(prefDefaults->getVoltageRange());
    configuration().setAmplification(prefDefaults->getAmplification());
    configuration().setNbChannels(prefDefaults->getNbChannels());
    configuration().setDatSamplingRate(prefDefaults->getDatSamplingRate());
    configuration().setEegSamplingRate(prefDefaults->getEegSamplingRate());
    configuration().setOffset(prefDefaults->getOffset());
    configuration().setResolutionIndex(prefDefaults->getResolutionIndex());
    configuration().setTraceBackgroundImage(prefDefaults->getTraceBackgroundImage());
    configuration().setNbSamples(clusterProperties->getNbSamples());
    configuration().setPeakIndex(clusterProperties->getPeakIndex());
    configuration().setSamplingRate(positionProperties->getSamplingRate());
    configuration().setWidth(positionProperties->getWidth());
    configuration().setHeight(positionProperties->getHeight());
    configuration().setBackgroundImage(positionProperties->getBackgroundImage());
    configuration().setRotation(positionProperties->getRotation());
    configuration().setFlip(positionProperties->getFlip());
    configuration().setPositionsBackground(positionProperties->getPositionsBackground());

    enableButtonApply(false);   // disable apply button
    applyEnable = false;
}


void PrefDialog::slotDefault() {
    if(QMessageBox::warning(this, tr("Set default options?"), tr("This will set the default options "
                                                   "in ALL pages of the preferences dialog! Do you wish to continue?"),
                                          tr("Set defaults"))==QMessageBox::Ok){
        
        prefGeneral->setBackgroundColor(configuration().getBackgroundColorDefault());
        prefGeneral->setPaletteHeaders(configuration().isPaletteHeadersDisplayedDefault());
        prefGeneral->setEventPosition(configuration().getEventPositionDefault());
        prefGeneral->setClusterPosition(configuration().getClusterPositionDefault());
        prefDefaults->setScreenGain(configuration().getScreenGainDefault());
        prefDefaults->setVoltageRange(configuration().getVoltageRangeDefault());
        prefDefaults->setAmplification(configuration().getAmplificationDefault());
        prefDefaults->setNbChannels(configuration().getNbChannelsDefault());
        prefDefaults->setDatSamplingRate(configuration().getDatSamplingRateDefault());
        prefDefaults->setEegSamplingRate(configuration().getEegSamplingRateDefault());
        prefDefaults->setOffset(configuration().getOffsetDefault());
        //KDAB_PORTING prefDefaults->setResolution(configuration().getResolutionIndexDefault());
        prefDefaults->setTraceBackgroundImage(configuration().getTraceBackgroundImage());
        clusterProperties->setNbSamples(configuration().getNbSamplesDefault());
        clusterProperties->setPeakIndex(configuration().getPeakIndexDefault());
        positionProperties->setSamplingRate(configuration().getVideoSamplingRateDefault());
        positionProperties->setWidth(configuration().getWidthDefault());
        positionProperties->setHeight(configuration().getHeightDefault());
        positionProperties->setBackgroundImage(configuration().getBackgroundImageDefault());
        positionProperties->setRotation(configuration().getRotationDefault());
        positionProperties->setFlip(configuration().getFlipDefault());
        positionProperties->setPositionsBackground(configuration().getPositionsBackgroundDefault());

        enableApply();   // enable apply button
    }
}


void PrefDialog::slotApply() {
    updateConfiguration();      // transfer settings to configuration object
    emit settingsChanged();     // apply the preferences
    enableButtonApply(false);   // disable apply button again
}


void PrefDialog::enableApply() {
    enableButtonApply(true);   // enable apply button
    applyEnable = true;
}


#include "prefdialog.moc"
