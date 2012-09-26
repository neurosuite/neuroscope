/***************************************************************************
                          prefdefaults.h  -  description
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

#ifndef PREFDEFAULTS_H
#define PREFDEFAULTS_H

// include files for QT
#include <qwidget.h>
#include <QComboBox>
#include <QCheckBox> 
#include <qspinbox.h>
#include <QPushButton>
#include <QLineEdit> 
#include <QValidator>
#include <QFileDialog>


//include files for the application
#include <prefdefaultslayout.h>

/**
  * Class representing the NeuroScope Defaults Configuration page of the Neuroscope preferences dialog.
  *@author Lynn Hazan
  */

class PrefDefaults : public PrefDefaultsLayout  {
    Q_OBJECT
public: 
    PrefDefaults(QWidget *parent=0);
    ~PrefDefaults();


    /**Sets the screen gain in milivolts by centimeters used to display the field potentiels.
  */
    inline void setScreenGain(float gain){
        screenGainLineEdit->setText(QString::fromLatin1("%1").arg(gain));
    }

    /**Sets the voltage range of the acquisition system in milivolts.
  */
    inline void setVoltageRange(int value){
        voltageRangeLineEdit->setText(QString::fromLatin1("%1").arg(value));
    }

    /**Sets the amplification of the acquisition system.
  */
    inline void setAmplification(int value){
        amplificationLineEdit->setText(QString::fromLatin1("%1").arg(value));
    }

    /**Sets the number of channels.*/
    inline void setNbChannels(int nb){nbChannelsLineEdit->setText(QString::fromLatin1("%1").arg(nb));}

    /**Sets the sampling rate for the dat file.*/
    inline void setDatSamplingRate(double rate){datSamplingRateLineEdit->setText(QString::fromLatin1("%1").arg(rate,0,'g',14));}

    /**Sets the sampling rate for the eeg file.*/
    inline void setEegSamplingRate(double rate){eegSamplingRateLineEdit->setText(QString::fromLatin1("%1").arg(rate,0,'g',14));}

    /**Sets the initial offset for all the field potentials.*/
    inline void setOffset(int offset){offsetLineEdit->setText(QString::fromLatin1("%1").arg(offset));}

    /**Sets the resolution of the acquisition system.*/
    inline void setResolutionIndex(int index){resolutionComboBox->setCurrentIndex(index);}

    /**Sets the background image.*/
    inline void setTraceBackgroundImage(QString image){
        traceBackgroundLineEdit->setText(image);
    }
    
    /**Returns the screen gain in milivolts by centimeters used to display the field potentiels..
  */
    inline float getScreenGain() const{
        return screenGainLineEdit->text().toFloat();
    }

    /**Returns the voltage range of the acquisition system in volts.
  */
    inline int getVoltageRange() const{
        return voltageRangeLineEdit->text().toInt();
    }

    /**Returns the amplification of the acquisition system.
  */
    inline int getAmplification() const{
        return amplificationLineEdit->text().toInt();
    }

    /**Returns the number of channels.*/
    inline int getNbChannels() const{return nbChannelsLineEdit->text().toInt();}

    /**Returns the sampling rate for the dat file.*/
    inline double getDatSamplingRate() const{return datSamplingRateLineEdit->text().toDouble();}

    /**Returns the sampling rate for the eeg file.*/
    inline double getEegSamplingRate() const{return eegSamplingRateLineEdit->text().toDouble();}

    /**Returns the initial offset for all the field potentials.*/
    inline int getOffset() const{return offsetLineEdit->text().toInt();}

    /**Returns the resolution of the acquisition system.*/
    inline int getResolutionIndex()const{return resolutionComboBox->currentIndex();}

    /**Returns the background image.*/
    inline QString getTraceBackgroundImage()const{return traceBackgroundLineEdit->text();}
    
private Q_SLOTS:
    inline void updateTraceBackgroundImage(){
        QString image = QFileDialog::getOpenFileName(this, tr("Select the background image..."));

        if(!image.isEmpty()) setTraceBackgroundImage(image);
    }
    
private:

    QIntValidator intValidator;
    QDoubleValidator doubleValidator;
};

#endif
