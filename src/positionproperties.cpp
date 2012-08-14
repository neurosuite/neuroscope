/***************************************************************************
                          positionproperties.cpp  -  description
                             -------------------
    begin                : Thu Jul 22 2004
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
//Application specific includes.
#include "positionproperties.h"

//QT includes
#include <qiconset.h>

// include files for KDE
#include <kiconloader.h>
#include <kimageeffect.h>


PositionProperties::PositionProperties(QWidget *parent, const char *name ) : PositionPropertiesLayout(parent,name),
                                      intValidator(this),doubleValidator(this){
 //Set a validator on the line edits, the values have to be integers.
 samplingRateLineEdit->setValidator(&doubleValidator);
 widthLineEdit->setValidator(&intValidator);
 heightLineEdit->setValidator(&intValidator);
              
 connect(backgroundButton,SIGNAL(clicked()),this,SLOT(updateBackgroundImage()));
 connect(backgroundLineEdit,SIGNAL(textChanged(const QString&)),this,SLOT(updateBackgroundImage(const QString&)));
 connect(rotateComboBox,SIGNAL(activated(int)),this,SLOT(updateDisplayedImage()));
 connect(filpComboBox,SIGNAL(activated(int)),this,SLOT(updateDisplayedImage()));

  //Set an icon on the backgroundButton button
 KIconLoader* loader = KGlobal::iconLoader();
 backgroundButton->setIconSet(QIconSet(loader->loadIcon("fileopen", KIcon::Small)));
 
}

PositionProperties::~PositionProperties(){
}

void PositionProperties::updateDisplayedImage(){ 
 if(backgroungImage != NULL){
   //apply first the rotation and then the flip
  QImage rotatedImage = backgroungImage;
  QPixmap pixmap;
  
  int angle = getRotation();
  if(angle != 0){
   KImageEffect::RotateDirection rotationAngle;
   //KDE counts clockwise, to have a counterclock-wise rotation 90 and 270 are inverted
   if(angle == 90) rotationAngle = KImageEffect::Rotate270;
   if(angle == 180) rotationAngle = KImageEffect::Rotate180;
   if(angle == 270) rotationAngle = KImageEffect::Rotate90;
    rotatedImage = KImageEffect::rotate(backgroungImage,rotationAngle);
  }

  QImage flippedImage = rotatedImage;
  // 0 stands for none, 1 for vertical flip and 2 for horizontal flip.
  int flip = getFlip();
  if(flip != 0){
   bool horizontal;
   bool vertical;
   if(flip == 1){
    horizontal = false;
    vertical = true;
   }
   else{
    horizontal = true;
    vertical = false;
   }
   flippedImage = rotatedImage.mirror(horizontal,vertical);
  }
  if(pixmap.convertFromImage(flippedImage)) backgroundPixmap->setPixmap(pixmap); 
 }
}

