/***************************************************************************
                          prefgeneral.h  -  description
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

#ifndef PREFGENERAL_H
#define PREFGENERAL_H

// include files for QT
#include <qwidget.h>
#include <QComboBox>
#include <QCheckBox> 
#include <qspinbox.h>
#include <QPushButton>
#include <QLineEdit> 
#include <QValidator>

//includes files for KDE




//include files for the application
#include <prefgenerallayout.h>

  /**
  * Class representing the Neuroscope Configuration page of the Neuroscope preferences dialog.
  *@author Lynn Hazan
  */

class PrefGeneral : public PrefGeneralLayout  {
   Q_OBJECT
public: 
	PrefGeneral(QWidget *parent=0, const char *name=0);
	~PrefGeneral();


  /**Sets the background color.*/
  inline void setBackgroundColor(const QColor& color)
  {
      backgroundColorButton->setColor(color);
  }

  /**Sets the display of the palette headers.*/
  inline void setPaletteHeaders(bool show){headerCheckBox->setChecked(show);}

  /**Sets the event position in percentage from the begining of the window where the events are display when browsing.*/
  inline void setEventPosition(int position){eventPositionSpinBox->setValue(position);}

  /**Sets the cluster position in percentage from the begining of the window where the clusters are display when browsing.*/
  inline void setClusterPosition(int position){clusterPositionSpinBox->setValue(position);}

  /**Returns the background color.*/
  inline QColor getBackgroundColor() const{
      return backgroundColorButton->color();
  }

  /**Returns true if the palette headers are displayed, false othewise.*/
  inline bool isPaletteHeadersDisplayed() const{return headerCheckBox->isChecked();}

  /**Returns the event position in percentage from the begining of the window where the events are display when browsing.*/
  inline int getEventPosition()const{return eventPositionSpinBox->value();}

  /**Returns the cluster position in percentage from the begining of the window where the clusters are display when browsing.*/
  inline int getClusterPosition()const{return clusterPositionSpinBox->value();}
  
 private:

  QIntValidator validator;
};

#endif
