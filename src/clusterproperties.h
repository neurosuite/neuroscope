/***************************************************************************
                          clusterproperties.h  -  description
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

#ifndef CLUSTERPROPERTIES_H
#define CLUSTERPROPERTIES_H

// include files for QT
#include <QWidget>
#include <QLineEdit>
#include <QValidator>
#include <QLabel>

//include files for the application
#include <clusterpropertieslayout.h>

/** Class representing the NeuroScope cluster preferences tab contained in NeuroScope Defaults Configuration page of the Neuroscope preferences dialog and in the properties dialog.
  *@author Lynn Hazan
  */

class ClusterProperties : public ClusterPropertiesLayout  {
    Q_OBJECT
public: 
    explicit ClusterProperties(QWidget *parent=0);
    ~ClusterProperties();


    /**Sets the number of samples per spike waveform.*/
    void setNbSamples(int nb){nbSamplesLineEdit->setText(QString::number(nb));}

    /**Sets the index of the peak sample in the spike waveform.*/
    void setPeakIndex(int index){peakIndexLineEdit->setText(QString::number(index));}

    /**Returns the number of samples per spike waveform.*/
    inline int getNbSamples()const{return nbSamplesLineEdit->text().toInt();}

    /**Returns the index of the peak sample in the spike waveform.*/
    inline int getPeakIndex()const{return peakIndexLineEdit->text().toInt();}

public Q_SLOTS:
    /**Sets whether the widget is enabled
  * @param state true if the widget is enable, false otherwise.
  */
    void setEnabled (bool state){
        groupBox->setEnabled(state);
        nbSamplesLineEdit->setEnabled(state);
        waveformLabel->setEnabled(state);
        peakIndexLineEdit->setEnabled(state);
        peakLabel->setEnabled(state);
    }

private:
    QIntValidator intValidator;
};

#endif
