/***************************************************************************
                          prefdefaults.cpp  -  description
                             -------------------
    begin                : Fri Feb 27 2004
    copyright            : (C) 2003 by Lynn Hazan
    email                : lynn.hazan@myrealbox.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
//Application specific includes.
#include "prefdefaults.h"

// include files for KDE
#include <kiconloader.h>

PrefDefaults::PrefDefaults(QWidget *parent, const char *name ) : PrefDefaultsLayout(parent,name),
              intValidator(this),doubleValidator(this) {
 //Set a validator on the line edits, the values have to be integers.
 nbChannelsLineEdit->setValidator(&intValidator);
 datSamplingRateLineEdit->setValidator(&doubleValidator);
 eegSamplingRateLineEdit->setValidator(&doubleValidator);
 offsetLineEdit->setValidator(&intValidator);
 screenGainLineEdit->setValidator(&doubleValidator);
 voltageRangeLineEdit->setValidator(&intValidator);
 amplificationLineEdit->setValidator(&intValidator);
 
 connect(traceBackgroundButton,SIGNAL(clicked()),this,SLOT(updateTraceBackgroundImage()));
 
 //Set an icon on the backgroundButton button
 KIconLoader* loader = KGlobal::iconLoader();
 traceBackgroundButton->setIconSet(QIcon(loader->loadIcon("fileopen", KIcon::Small)));
 
}
PrefDefaults::~PrefDefaults(){
}

#include "prefdefaults.moc"
