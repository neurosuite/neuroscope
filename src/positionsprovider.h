/***************************************************************************
                          positionsprovider.h  -  description
                             -------------------
    begin                : Tue Jul 20 2004
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

#ifndef POSITIONSPROVIDER_H
#define POSITIONSPROVIDER_H

//include files for the application
#include <dataprovider.h>
#include <array.h>
#include <types.h>

// include files for QT
#include <QWidget>

//include files for c/c++ libraries
#include <math.h>

/**Class storing and providing the position information.
  *@author Lynn Hazan
  */

class PositionsProvider : public DataProvider  {
    Q_OBJECT
public: 
    /**Constructor.
  * @param fileUrl the url of the position file containing the positions.
  * @param samplingRate sampling rate used to record the data.
  * @param width video image width.
  * @param height video image height.
  * @param rotation video image rotation angle.
  * @param flip video image flip orientation, 0 stands for none, 1 for vertical and 2 for horizontal.
  */
    explicit PositionsProvider(const QString& fileUrl,double samplingRate,int width,int height,int rotation,int flip);

    ~PositionsProvider();

    /**Information retun after a call to openFile/saveDocument/createFeatureFile*/
    enum loadReturnMessage {OK=0,OPEN_ERROR=1,MISSING_FILE=3,COUNT_ERROR=4,INCORRECT_CONTENT=5};
    
    /**Triggers the retrieve of the position information included in the time interval given by @p startTime and @p endTime.
  * @param startTime begining of the time interval from which to retrieve the data.
  * @param endTime end of the time interval from which to retrieve the data.
  * @param initiator instance requesting the data.
  */
    void requestData(long startTime,long endTime,QObject* initiator);


    /**Triggers the retrieve of all the positions.
  * @param initiator instance requesting the data.
  */
    void retrieveAllData(QObject* initiator);

    /**Loads the positions.
  * @return an loadReturnMessage enum giving the load status
  */
    int loadData();

    /**Returns the name of the provider which is the position file name.
  * @return provider'name.
  */
    QString getName() const {return name;}

    /**Returns the path to the position file.
  * @return path.
  */
    QString getFilePath() const {return fileName;}

    void updateVideoInformation(double videoSamplingRate,int rotation,int flip,int videoWidth,int videoHeight){
        samplingRate = videoSamplingRate;
        width = videoWidth;
        height= videoHeight;
        this->flip = flip;
        this->rotation = rotation;
    }

    /**Returns the number of spots for each animal position recorded. It is either 1 or 2.
  * @return number of spots.
  */
    int getNbSpots()const {return nbCoordinates / 2;}

    /**Returns the sampling rate used to record the data contained in the file identified by fileUrl.
  */
    double getSamplingRate() const {return samplingRate;}

Q_SIGNALS:
    /**Signals that the data have been retrieved.
  * @param data n column array containing the position of the animal. The two first columns contain
  * the position of the first spot and the following optional pair of columns contain the position of optional spots.
  * @param initiator instance requesting the data.
  */
    void dataReady(Array<dataType>& data,QObject* initiator);


private:

    /**Provider's name.*/
    QString name;

    /**Sampling rate used to record the data.*/
    double samplingRate;

    /* n column array containing the position of the animal. The two first columns contain
  * the position of the first spot and following optional pair of columns two contain the position of optional spots.*/
    Array<dataType> positions;

    /**The start time for the previously requested data.*/
    long previousStartTime;

    /**The end time for the previously requested data.*/
    long previousEndTime;

    /**The start index for the previously requested data.*/
    long previousStartIndex;

    /**The end index for the previously requested data.*/
    long previousEndIndex;

    /**Number of positions.*/
    long nbPositions;

    /**The maximum time, in miliseconds, contained in the file.*/
    long fileMaxTime;

    /**Video image width.*/
    int width;
    /**Video image height.*/
    int height;

    /**Number of coordinates per line contained in the position file.*/
    int nbCoordinates;

    /**Angle of rotation of the video records.*/
    int rotation;

    /**Flip orientation of the video records. 0 stands for none, 1 for vertical flip and 2 for horizontal flip.*/
    int flip;

    //Functions

    /**Retrieves the peak index of each spike included in the time frame given by @p startTime and @p endTime.
  * @param startTime begining of the time frame from which to retrieve the data, given in milisecond.
  * @param endTime end of the time frame from which to retrieve the data, given in milisecond.
  * @param initiator instance requesting the data.
  */
    void retrieveData(long startTime,long endTime,QObject* initiator);

};

#endif
