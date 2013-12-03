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
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef POSITIONPROPERTIES_H
#define POSITIONPROPERTIES_H

// include files for QT
#include <QWidget>
#include <QValidator>
#include <QComboBox>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include <QPixmap>
#include <QImage>
#include <QCheckBox> 


//include files for the application
#include <positionpropertieslayout.h>

/**Class representing the NeuroScope position preferences tab contained in the preferences or properties dialogs.
  *@author Lynn Hazan
  */

class PositionProperties : public PositionPropertiesLayout  {
    Q_OBJECT
public: 
    explicit PositionProperties(QWidget *parent=0);
    ~PositionProperties();

    /**Sets the video acquisition sampling rate.*/
    void setSamplingRate(double rate){samplingRateLineEdit->setText(QString::fromLatin1("%1").arg(rate,0,'g',14));}

    /**Sets the video image width.*/
    void setWidth(int width){widthLineEdit->setText(QString::number(width));}

    /**Sets the video image height.*/
    void setHeight(int height){heightLineEdit->setText(QString::number(height));}

    /**Sets the background image.*/
    void setBackgroundImage(const QString& image);

    /**All the positions contained in a position file can be used to create a background image for the PositionView.
  * This function sets if such background has to be created.
  */
    void setPositionsBackground(bool draw){checkBoxBackground->setChecked(draw);}

    /**Sets the video image rotation angle.*/
    void setRotation(int angle);

    /**Sets the video image flip orientation.
  * 0 stands for none, 1 for vertical and 2 for horizontal.
  */
    void setFlip(int orientation);

    /**Returns the video acquisition sampling rate.*/
    double getSamplingRate()const{return samplingRateLineEdit->text().toDouble();}

    /**Returns the video image width.*/
    int getWidth()const{return widthLineEdit->text().toInt();}

    /**Returns the video image height.*/
    int getHeight()const{return heightLineEdit->text().toInt();}

    /**Returns the background image.*/
    QString getBackgroundImage()const{return backgroundLineEdit->text();}

    /**All the positions contained in a position file can be used to create a background image for the PositionView.
  * The value return by this function tells if such background has to be created.
  * @return true if the all the positions contain in the position file have to be drawn on the background, false otherwise.
  */
    bool getPositionsBackground()const{return checkBoxBackground->isChecked();}

    
    /**Returns the video image rotation angle.*/
    int getRotation()const;

    /**Returns the video image flip orientation.
  * 0 stands for none, 1 for vertical and 2 for horizontal.
  */
    int getFlip() const;

public Q_SLOTS:
    /**Sets whether the widget is enabled
  * @param state true if the widget is enable, false otherwise.
  */
    void setEnabled (bool state);


private Q_SLOTS:
    void updateBackgroundImage();

    void updateBackgroundImage(const QString& image);
    void updateDisplayedImage();

private:
    QIntValidator intValidator;
    QDoubleValidator doubleValidator;
    QImage backgroungImage;
};

#endif
