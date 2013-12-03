/***************************************************************************
                          prefdialog.h  -  description
                             -------------------
    begin                : Fri Feb 27 2004
    copyright            : (C) 2003 by Lynn Hazan
    email                :
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef PREFDIALOG_H
#define PREFDIALOG_H

// include files for QT
#include <QWidget>

#include <qpagedialog.h>

class PrefGeneral;
class PrefDefaults;
class PositionProperties;
class ClusterProperties;


/**
  * Class representing the Neuroscope preferences dialog.
  *@author Lynn Hazan
  */

class PrefDialog : public QPageDialog {
    Q_OBJECT
public:
    /**Constructor*/
    explicit PrefDialog(QWidget *parent);

    /** Transfers the settings from the configuration object to the dialog.*/
    void updateDialog();
    /** Transfers the settings from the dialog to the configuration object.*/
    void updateConfiguration();
    /** */
    bool isApplyEnable() const {return applyEnable;}
    
public Q_SLOTS:
    /**Will be called when the "Default" button has been clicked.*/
    void slotDefault();
    /**Will be called when the "Apply" button has been clicked.*/
    void slotApply();
    /**Will be called whenever a setting was changed.*/
    void enableApply();

    void slotHelp();

Q_SIGNALS:
    /// Will be emitted when the new settings should be applied.
    void settingsChanged();


private:
    PrefGeneral* prefGeneral;
    PrefDefaults* prefDefaults;
    PositionProperties* positionProperties;
    ClusterProperties* clusterProperties;
    bool applyEnable;
};

#endif  // PREFDIALOG_H
