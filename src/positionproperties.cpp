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
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
//Application specific includes.
#include "positionproperties.h"

//QT includes
#include <QIcon>
#include <QFileDialog>

#include <QPixmap>


PositionProperties::PositionProperties(QWidget *parent ) : PositionPropertiesLayout(parent),
    intValidator(this),doubleValidator(this){
    //Set a validator on the line edits, the values have to be integers.
    samplingRateLineEdit->setValidator(&doubleValidator);
    widthLineEdit->setValidator(&intValidator);
    heightLineEdit->setValidator(&intValidator);

    connect(backgroundButton,SIGNAL(clicked()),this,SLOT(updateBackgroundImage()));
    connect(backgroundLineEdit,SIGNAL(textChanged(QString)),this,SLOT(updateBackgroundImage(QString)));
    connect(rotateComboBox,SIGNAL(activated(int)),this,SLOT(updateDisplayedImage()));
    connect(filpComboBox,SIGNAL(activated(int)),this,SLOT(updateDisplayedImage()));

    //Set an icon on the backgroundButton button

    backgroundButton->setIcon(QIcon(":/shared-icons/folder-open"));

}

PositionProperties::~PositionProperties(){
}

void PositionProperties::updateDisplayedImage()
{

    if(!backgroungImage.isNull()){
        //apply first the rotation and then the flip
        QImage rotatedImage = backgroungImage;
        QPixmap pixmap;

        int angle = getRotation();
        if(angle != 0){
            QTransform rot;
            //KDE counts clockwise, to have a counterclock-wise rotation 90 and 270 are inverted
            if(angle == 90)
                rot.rotate(90);
            else if(angle == 180)
                rot.rotate(180);
            else if(angle == 270)
                rot.rotate(270);
            rotatedImage = backgroungImage.transformed(rot);
        }

        //Scale
        QImage flippedImage = rotatedImage;
        // 0 stands for none, 1 for vertical flip and 2 for horizontal flip.
        int flip = getFlip();
        if(flip != 0){
            bool horizontal;
            bool vertical;
            if(flip == 1){
                horizontal = false;
                vertical = true;
            } else {
                horizontal = true;
                vertical = false;
            }
            flippedImage = rotatedImage.mirrored(horizontal,vertical);
        }
        if(pixmap.convertFromImage(flippedImage))
            backgroundPixmap2->setPixmap(pixmap);
    }
}

void PositionProperties::updateBackgroundImage()
{
    const QString image = QFileDialog::getOpenFileName(this, tr("Select the background image..."));
    if(!image.isEmpty())
        setBackgroundImage(image);
}

void PositionProperties::updateBackgroundImage(const QString& image)
{
    if(!image.isEmpty()) {
        setBackgroundImage(image);
    } else {
        QPixmap pixmap(getWidth(),getHeight());
        pixmap.fill(Qt::black);
        backgroundPixmap2->setPixmap(pixmap);
    }
}

int PositionProperties::getFlip() const
{
    switch(filpComboBox->currentIndex()){
    case 0:
        return 0;
    case 1:
        return 1;
    case 2:
        return 2;
    default:
        return 0;
    }
}


int PositionProperties::getRotation() const {
    switch(rotateComboBox->currentIndex()){
    case 0:
        return 0;
    case 1:
        return 90;
    case 2:
        return 180;
    case 3:
        return 270;
    default:
        return 0;
    }
}


void PositionProperties::setFlip(int orientation){
    switch(orientation){
    case 0:
        filpComboBox->setCurrentIndex(0);
        break;
    case 1:
        filpComboBox->setCurrentIndex(1);
        break;
    case 2:
        filpComboBox->setCurrentIndex(2);
        break;
    default:
        filpComboBox->setCurrentIndex(0);
        break;
    }
}


void PositionProperties::setRotation(int angle){
    switch(angle){
    case 0:
        rotateComboBox->setCurrentIndex(0);
        break;
    case 90:
        rotateComboBox->setCurrentIndex(1);
        break;
    case 180:
        rotateComboBox->setCurrentIndex(2);
        break;
    case 270:
        rotateComboBox->setCurrentIndex(3);
        break;
    default:
        rotateComboBox->setCurrentIndex(0);
        break;
    }
}


void PositionProperties::setBackgroundImage(const QString& image){
    backgroundLineEdit->setText(image);
    if(!image.isEmpty()){
       backgroungImage.load(image);
        if(!backgroungImage.isNull()){
            //flip and rotation values should have been set before any call to this function.
            updateDisplayedImage();
        }
    }
}


void PositionProperties::setEnabled (bool state){
    groupBox1->setEnabled(state);
    groupBox2->setEnabled(state);
    samplingRateLineEdit->setEnabled(state);
    widthLineEdit->setEnabled(state);
    heightLineEdit->setEnabled(state);
    backgroundLineEdit->setEnabled(state);
    rotateComboBox->setEnabled(state);
    filpComboBox->setEnabled(state);
    backgroundButton->setEnabled(state);
    sampleRateLabel->setEnabled(state);
    widthLabel->setEnabled(state);
    heightLabel->setEnabled(state);
    backgroundLabel->setEnabled(state);
    rotateLabel->setEnabled(state);
    flipLabel->setEnabled(state);

    if(!state){
        QPixmap pixmap(getWidth(),getHeight());
        pixmap.fill(Qt::black);
        backgroundPixmap2->setPixmap(pixmap);
    }
}
