/***************************************************************************
                          propertiesdialog.h  -  description
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

#ifndef PROPERTIESDIALOG_H
#define PROPERTIESDIALOG_H

// include files for QT
#include <QWidget>

#include <QDialog>

//include files for the application
#include "properties.h"  
#include "positionproperties.h"
#include "clusterproperties.h"


/** Class representing the Neuroscope properties dialog.
  *@author Lynn Hazan
  */

class Properties;
class QTabWidget;
class PropertiesDialog : public QDialog  {
    Q_OBJECT
public:
    //Constructor
    explicit PropertiesDialog(QWidget *parent);
    ~PropertiesDialog();

    /** Updates the dialog.
   * @param channelNb number of channels.
   * @param SR sampling rate of the current file.
   * @param resolution resolution of the acquisition system.
   * @param offset initial offset for all the traces.
   * @param screenGain initial screen gain in milivolts by centimeters used to display the field potentiels.
   * @param voltageRange initial voltage range of the acquisition system in volts.
   * @param amplification initial amplification of the acquisition system.
   * @param nbSamples number of samples per spike waveform.
   * @param peakIndex sample index corresponding to the peak of a spike waveform.
   * @param videoSamplingRate video acquisition sampling rate.
   * @param width video image width.
   * @param height video image height.
   * @param backgroundImage image used as a background for the position view.
   * @param rotation video image rotation angle.
   * @param flip video image flip orientation, 0 stands for none, 1 for vertical and 2 for horizontal.
   * @param acquisitionSystemSamplingRate sampling rate of the acquisition system.
   * @param positionsBackground true if the all the positions contain in the position file have to be drawn on the background, false otherwise.
   * @param traceBackgroundImage image used as a background for the trace view.
  */
    void updateDialog(int channelNb,double SR, int resolution,int offset,float screenGain,int voltageRange,int amplification,
                      int nbSamples,int peakIndex,double videoSamplingRate, int width, int height, const QString& backgroundImage,
                      int rotation,int flip,double acquisitionSystemSamplingRate,bool positionsBackground, const QString& traceBackgroundImage);

    /**Returns the screen gain in milivolts by centimeters used to display the field potentiels.
  */
    float getScreenGain() const{
        return properties->getScreenGain();
    }

    /**Returns the voltage range of the acquisition system in volts.
  */
    int getVoltageRange() const{
        return properties->getVoltageRange();
    }

    /**Returns the amplification of the acquisition system.
  */
    int getAmplification() const{
        return properties->getAmplification();
    }

    /**Returns the number of channels.*/
    int getNbChannels() const{return properties->getNbChannels();}

    /**Returns the sampling rate for the current file.*/
    double getSamplingRate() const{return properties->getSamplingRate();}

    /**Returns the sampling rate of the acquisition system.*/
    double getAcquisitionSystemSamplingRate(){return properties->getAcquisitionSystemSamplingRate();}

    /**Returns the initial offset for all the field potentials.*/
    int getOffset() const{return properties->getOffset();}

    /**Returns the resolution of the acquisition system.*/
    int getResolution()const{return properties->getResolution();}

    /**Returns the background image for the TraceView.*/
    QString getTraceBackgroundImage()const{return properties->getTraceBackgroundImage();}

    /**True if at least one property has been modified, false otherwise.*/
    bool isModified()const{return modified;}

    void openState(bool init){atStartUp = init;}

    /**Returns the number of samples per spike waveform.*/
    int getNbSamples()const{return clusterProperties->getNbSamples();}

    /**Returns the index of the peak sample in the spike waveform.*/
    int getPeakIndex()const{return clusterProperties->getPeakIndex();}

    /**Returns the video acquisition sampling rate.*/
    double getVideoSamplingRate()const{return positionProperties->getSamplingRate();}

    /**Returns the video image width.*/
    int getWidth()const{return positionProperties->getWidth();}

    /**Returns the video image height.*/
    int getHeight()const{return positionProperties->getHeight();}

    /**Returns the background image for the PositionView.*/
    QString getBackgroundImage()const{return positionProperties->getBackgroundImage();}

    /**All the positions contained in a position file can be used to create a background image for the PositionView.
  * The value return by this function tells if such background has to be created.
  * @return true if the all the positions contain in the position file have to be drawn on the background, false otherwise.
  */
    bool getPositionsBackground()const{return positionProperties->getPositionsBackground();}

    /**Returns the video image rotation angle.*/
    int getRotation()const{return positionProperties->getRotation();}

    /**Returns the video image flip orientation.
  * 0 stands for none, 1 for vertical and 2 for horizontal.
  */
    int getFlip()const{return positionProperties->getFlip();}

    /**Sets whether the widgets of the cluster tab are enabled.
  * @param state true if enable, false otherwise.
  */
    void setEnabledCluster(bool state){clusterProperties->setEnabled(state);}

    /**Sets whether the widgets of the position tab are enabled.
  * @param state true enable, false otherwise.
  */
    void setEnabledPosition(bool state){positionProperties->setEnabled(state);}

    /**Sets whether the sampling rate for the current file is enabled.
  * @param state true enable, false otherwise.
  */
    void setEnabledCurrentSamplingRate(bool state){properties->setCurrentSamplingRateEnabled(state);}

    /**Shows the page containing the position information in front.*/
    void showPositionPage();

public Q_SLOTS:
    /** Will be called when any properties except the number of channels has been modified.*/
    void propertyModified(){modified = true;}

    /**Makes the verifications before returning from the dialog.*/
    void slotVerify();

protected:
    void accept(){
        if(oops){
            oops = false;
        }
        else
            QDialog::accept();
    }

private Q_SLOTS:
    /** Will be called when the number of channels has been modified.*/
     void channelNbModified(){nbChannelsModified = true;}
     void slotHelp();
private:
    Properties* properties;
    PositionProperties* positionProperties;
    ClusterProperties* clusterProperties;
    bool modified;
    bool nbChannelsModified;
    int nbChannels;
    bool oops;
    bool atStartUp;
    QTabWidget *mTabWidget;
};

#endif
