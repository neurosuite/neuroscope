/***************************************************************************
                          properties.cpp  -  description
                             -------------------
    begin                : Sun Feb 29 2004
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

//includes files for the application
#include "properties.h"

Properties::Properties(QWidget *parent)
    : PropertiesLayout(parent),
      intValidator(this),doubleValidator(this)
{
    //Set a validator on the line edits, the values have to be integers or doubles.
    nbChannelsLineEdit->setValidator(&intValidator);
    samplingRateLineEdit->setValidator(&doubleValidator);
    offsetLineEdit->setValidator(&intValidator);
    screenGainLineEdit->setValidator(&doubleValidator);
    voltageRangeLineEdit->setValidator(&intValidator);
    amplificationLineEdit->setValidator(&intValidator);
    asSamplingRateLineEdit->setValidator(&doubleValidator);

    connect(traceBackgroundButton,SIGNAL(clicked()),this,SLOT(updateTraceBackgroundImage()));

    //Set an icon on the backgroundButton button

    traceBackgroundButton->setIcon(QIcon(":/shared-icons/folder-open"));

}
Properties::~Properties(){
}
