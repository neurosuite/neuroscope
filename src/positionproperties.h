/***************************************************************************
                          positionproperties.h  -  description
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

#ifndef POSITIONPROPERTIES_H
#define POSITIONPROPERTIES_H

// include files for QT
#include <qwidget.h>
#include <QValidator>
#include <QComboBox>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include <QPixmap>
#include <QImage>
#include <QFileDialog>
#include <q3groupbox.h>
#include <QCheckBox> 
#include <QFileDialog>


//include files for the application
#include <positionpropertieslayout.h>

/**Class representing the NeuroScope position preferences tab contained in the preferences or properties dialogs.
  *@author Lynn Hazan
  */

class PositionProperties : public PositionPropertiesLayout  {
    Q_OBJECT
public: 
    PositionProperties(QWidget *parent=0, const char *name=0);
    ~PositionProperties();

    /**Sets the video acquisition sampling rate.*/
    inline void setSamplingRate(double rate){samplingRateLineEdit->setText(QString::fromLatin1("%1").arg(rate,0,'g',14));}

    /**Sets the video image width.*/
    inline void setWidth(int width){widthLineEdit->setText(QString::fromLatin1("%1").arg(width));}

    /**Sets the video image height.*/
    inline void setHeight(int height){heightLineEdit->setText(QString::fromLatin1("%1").arg(height));}

    /**Sets the background image.*/
    inline void setBackgroundImage(const QString& image){
        backgroundLineEdit->setText(image);
        if(!image.isEmpty()){
           backgroungImage.load(image);
            if(!backgroungImage.isNull()){
                //flip and rotation values should have been set before any call to this function.
                updateDisplayedImage();
            }
        }
    }

    /**All the positions contained in a position file can be used to create a background image for the PositionView.
  * This function sets if such background has to be created.
  */
    inline void setPositionsBackground(bool draw){checkBoxBackground->setChecked(draw);}

    /**Sets the video image rotation angle.*/
    inline void setRotation(int angle){
        switch(angle){
        case 0:
            rotateComboBox->setCurrentItem(0);
            break;
        case 90:
            rotateComboBox->setCurrentItem(1);
            break;
        case 180:
            rotateComboBox->setCurrentItem(2);
            break;
        case 270:
            rotateComboBox->setCurrentItem(3);
            break;
        default:
            rotateComboBox->setCurrentItem(0);
            break;
        }
    }

    /**Sets the video image flip orientation.
  * 0 stands for none, 1 for vertical and 2 for horizontal.
  */
    inline void setFlip(int orientation){
        switch(orientation){
        case 0:
            filpComboBox->setCurrentItem(0);
            break;
        case 1:
            filpComboBox->setCurrentItem(1);
            break;
        case 2:
            filpComboBox->setCurrentItem(2);
            break;
        default:
            filpComboBox->setCurrentItem(0);
            break;
        }
    }

    /**Returns the video acquisition sampling rate.*/
    inline double getSamplingRate()const{return samplingRateLineEdit->text().toDouble();}

    /**Returns the video image width.*/
    inline int getWidth()const{return widthLineEdit->text().toInt();}

    /**Returns the video image height.*/
    inline int getHeight()const{return heightLineEdit->text().toInt();}

    /**Returns the background image.*/
    inline QString getBackgroundImage()const{return backgroundLineEdit->text();}

    /**All the positions contained in a position file can be used to create a background image for the PositionView.
  * The value return by this function tells if such background has to be created.
  * @return true if the all the positions contain in the position file have to be drawn on the background, false otherwise.
  */
    inline bool getPositionsBackground()const{return checkBoxBackground->isChecked();}

    
    /**Returns the video image rotation angle.*/
    inline int getRotation()const{
        switch(rotateComboBox->currentItem()){
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

    /**Returns the video image flip orientation.
  * 0 stands for none, 1 for vertical and 2 for horizontal.
  */
    inline int getFlip()const{
        switch(filpComboBox->currentItem()){
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

public slots:
    /**Sets whether the widget is enabled
  * @param state true if the widget is enable, false otherwise.
  */
    inline void setEnabled (bool state){
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
            QPixmap pixmap;
            pixmap.resize(getWidth(),getHeight());
            pixmap.fill(Qt::black);
            backgroundPixmap2->setPixmap(pixmap);
        }
    }


private slots:
    inline void updateBackgroundImage(){
        QString image = QFileDialog::getOpenFileName(this, tr("Select the background image..."));
        if(!image.isEmpty())
            setBackgroundImage(image);
    }

    inline void updateBackgroundImage(const QString& image){
        if(!image.isEmpty())  setBackgroundImage(image);
        else{
            QPixmap pixmap;
            pixmap.resize(getWidth(),getHeight());
            pixmap.fill(Qt::black);
            backgroundPixmap2->setPixmap(pixmap);
        }
    }

    void updateDisplayedImage();

private:
    QIntValidator intValidator;
    QDoubleValidator doubleValidator;
    QImage backgroungImage;
};

#endif
