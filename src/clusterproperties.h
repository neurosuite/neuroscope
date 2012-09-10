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
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef CLUSTERPROPERTIES_H
#define CLUSTERPROPERTIES_H

// include files for QT
#include <qwidget.h>
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
    ClusterProperties(QWidget *parent=0);
    ~ClusterProperties();


    /**Sets the number of samples per spike waveform.*/
    inline void setNbSamples(int nb){nbSamplesLineEdit->setText(QString::fromLatin1("%1").arg(nb));}

    /**Sets the index of the peak sample in the spike waveform.*/
    inline void setPeakIndex(int index){peakIndexLineEdit->setText(QString::fromLatin1("%1").arg(index));}

    /**Returns the number of samples per spike waveform.*/
    inline int getNbSamples()const{return nbSamplesLineEdit->text().toInt();}

    /**Returns the index of the peak sample in the spike waveform.*/
    inline int getPeakIndex()const{return peakIndexLineEdit->text().toInt();}

public slots:
    /**Sets whether the widget is enabled
  * @param state true if the widget is enable, false otherwise.
  */
    inline void setEnabled (bool state){
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
