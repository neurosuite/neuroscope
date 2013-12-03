/***************************************************************************
                          properties.h  -  description
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

#ifndef PROPERTIES_H
#define PROPERTIES_H

// include files for QT
#include <QWidget>
#include <QComboBox>
#include <qspinbox.h>
#include <QPushButton>
#include <QLineEdit>
#include <QValidator>
#include <QFileDialog>

//includes files for KDE



//include files for the application
#include <propertieslayout.h>



/**
  * Class used to set the properties of the current document.
  *@author Lynn Hazan
  */
class Properties : public PropertiesLayout  {
    Q_OBJECT
public: 
    explicit Properties(QWidget *parent=0);
    ~Properties();

    /**Sets the screen gain in milivolts by centimeters used to display the field potentiels..
  */
    void setScreenGain(float gain){
        screenGainLineEdit->setText(QString::number(gain));
    }

    /**Sets the voltage range of the acquisition system in milivolts.
  */
    void setVoltageRange(int value){
        voltageRangeLineEdit->setText(QString::number(value));
    }

    /**Sets the amplification of the acquisition system.
  */
    void setAmplification(int value){
        amplificationLineEdit->setText(QString::number(value));
    }

    /**Sets the number of channels.*/
    void setNbChannels(int nb){nbChannelsLineEdit->setText(QString::number(nb));}

    /**Sets the sampling rate for the current file.*/
    void setSamplingRate(double rate){samplingRateLineEdit->setText(QString::fromLatin1("%1").arg(rate,0,'g',14));}

    /**Sets the sampling rate of the acquisition system.*/
    void setAcquisitionSystemSamplingRate(double rate){asSamplingRateLineEdit->setText(QString::fromLatin1("%1").arg(rate,0,'g',14));}

    /**Sets the initial offset for all the field potentials.*/
    void setOffset(int offset){offsetLineEdit->setText(QString::number(offset));}

    /**Sets the resolution of the acquisition system.*/
    void setResolution(int res){
        switch(res){
        case 12:
            resolutionComboBox->setCurrentIndex(0);
            break;
        case 14:
            resolutionComboBox->setCurrentIndex(1);
            break;
        case 16:
            resolutionComboBox->setCurrentIndex(2);
            break;
        case 32:
            resolutionComboBox->setCurrentIndex(3);
            break;
        default:
            resolutionComboBox->setCurrentIndex(2);
            break;
        }
    }

    /**Sets the background image.*/
    void setTraceBackgroundImage(const QString& image){
        traceBackgroundLineEdit->setText(image);
    }

    /**Returns the screen gain in milivolts by centimeters used to display the field potentiels.
  */
    float getScreenGain() const{
        return screenGainLineEdit->text().toFloat();
    }

    /**Returns the voltage range of the acquisition system in volts.
  */
    int getVoltageRange() const{
        return voltageRangeLineEdit->text().toInt();
    }

    /**Returns the amplification of the acquisition system.
  */
    int getAmplification() const{
        return amplificationLineEdit->text().toInt();
    }

    /**Returns the number of channels.*/
    int getNbChannels() const{return nbChannelsLineEdit->text().toInt();}

    /**Returns the sampling rate  for the current file.*/
    double getSamplingRate() const{return samplingRateLineEdit->text().toDouble();}

    /**Returns the sampling rate of the acquisition system.*/
    double getAcquisitionSystemSamplingRate(){return asSamplingRateLineEdit->text().toDouble();}

    /**Returns the initial offset for all the field potentials.*/
    int getOffset() const{return offsetLineEdit->text().toInt();}

    /**Returns the resolution of the acquisition system.*/
    int getResolution()const{
        switch(resolutionComboBox->currentIndex()){
        case 0:
            return 12;
        case 1:
            return 14;
        case 2:
            return 16;
        case 3:
            return 32;
        default:
            return 16;
        }
    }

    /**Returns the background image.*/
    QString getTraceBackgroundImage()const{
        return traceBackgroundLineEdit->text();}

    /**Sets whether the sampling rate for the current file is enabled.
  * @param state true enable, false otherwise.
  */
    void setCurrentSamplingRateEnabled(bool state){samplingRateLineEdit->setEnabled(state);}


private Q_SLOTS:
    void updateTraceBackgroundImage(){
        const QString image = QFileDialog::getOpenFileName(this, tr("Select the background image..."));
        if(!image.isEmpty())
            setTraceBackgroundImage(image);
    }
    
private:
    QIntValidator intValidator;
    QDoubleValidator doubleValidator;
};

#endif
