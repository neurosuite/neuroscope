/***************************************************************************
                          positionsprovider.cpp  -  description
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
//include files for the application
#include "positionsprovider.h"
#include "timer.h"
#include "utilities.h"

// include files for QT
#include <QStringList>
#include <QFileInfo>
#include <QDebug>



PositionsProvider::PositionsProvider(const QString &fileUrl, double samplingRate, int width, int height, int rotation, int flip):
    DataProvider(fileUrl),
    samplingRate(samplingRate),
    width(width),
    height(height),
    rotation(rotation),
    flip(flip)
{

    name = fileUrl;
}

PositionsProvider::~PositionsProvider(){
}

void PositionsProvider::requestData(long startTime,long endTime,QObject* initiator){
    retrieveData(startTime,endTime,initiator);
}

int PositionsProvider::loadData(){

    //Get the number of positions
    nbPositions = Utilities::getNbLines(fileName);
    
    if(nbPositions == -1){
        positions.setSize(0,0);
        return COUNT_ERROR;
    }

    if(nbPositions == 0){
        positions.setSize(0,0);
        return OK;
    }

    //Create a reader on the eventFile
    QFile positionFile(fileName);
    bool status = positionFile.open(QIODevice::ReadOnly);
    if(!status){
        positions.setSize(0,0);
        return OPEN_ERROR;
    }

    RestartTimer();

    QString firstLine;

    QByteArray buf;
    buf.resize(255);
    int ret=positionFile.readLine(buf.data(), 255);
    firstLine = QString::fromLatin1(buf, ret);


    //Set the size of the Arrays containing the positions using the first line.
    firstLine = firstLine.simplified();
    QStringList lineParts = firstLine.split(QLatin1String(" "), QString::SkipEmptyParts);
    nbCoordinates = lineParts.count();
    dataType k = nbCoordinates;
    positions.setSize(nbPositions,nbCoordinates);
    for(int i = 0;i<nbCoordinates;++i){
        positions(i+1,1) = static_cast<dataType>(floor(0.5 + lineParts[i].toDouble()));
    }


    QByteArray buffer = positionFile.readAll();
    uint size = buffer.size();

    //The buffer is read and each dataType is build char by char into a string. When the char read
    //is not [1-9],e or + (<=> blank space or a new line), the string is converted into a dataType and store
    //into positions.
    //string of character which will contains the current seek dataType
    int l = 0;
    char clusterID[255];
    for(uint i = 0 ; i < size ; ++i){
        if(buffer[i] >= '0' && buffer[i] <= '9' || buffer[i] == 'e'| buffer[i] == 'E' || buffer[i] == '+' || buffer[i] == '-' || buffer[i] == '.')
            clusterID[l++] = buffer[i];
        else if(l){
            clusterID[l] = '\0';
            double pos = atof(clusterID);
            //The precision is lost
            positions[k++] = static_cast<dataType>(floor(0.5 + pos));//Warning if the typedef dataType changes, change will have to be make here.
            l = 0;
        }
    }

    positionFile.close();
    qDebug() << "Loading pos file into memory: "<<Timer() << endl;


    //The number of positions read has to be coherent with the number of positions read.
    if(k != nbPositions*nbCoordinates){
        positions.setSize(0,0);
        return INCORRECT_CONTENT;
    }

    return OK;
}

void PositionsProvider::retrieveAllData(QObject* initiator){
    Array<dataType> data;
    dataType startInRecordingUnits = 1;
    dataType endInRecordingUnits = nbPositions;

    dataType nbSamples = static_cast<dataType>(endInRecordingUnits - startInRecordingUnits) + 1;

    //data will contain the final values.
    data.setSize(nbSamples,nbCoordinates);

    //No transformation
    long count = 1;
    for(int i = startInRecordingUnits; i<= endInRecordingUnits; ++i){
        for(int j = 1;j<=nbCoordinates;++j){
            data(count,j) = positions(i,j);
        }
        count++;
    }

    //Send the information to the receiver.
    emit dataReady(data,initiator);
    return;

}

void PositionsProvider::retrieveData(long startTime,long endTime,QObject* initiator){
    Array<dataType> data;

    //Search what is the number of samples in the given time frame.
    //Convert the time in miliseconds to time in recording units.
    dataType startInRecordingUnits = static_cast<dataType>(ceil(static_cast<float>(static_cast<double>(startTime) * static_cast<double>(static_cast<double>(samplingRate) / static_cast<double>(1000)))));
    dataType endInRecordingUnits =  static_cast<dataType>(floor(static_cast<float>(0.5 + static_cast<double>(endTime) * static_cast<double>(static_cast<double>(samplingRate) / static_cast<double>(1000)))));

    //The start time is over the maximum of the position file
    if(startInRecordingUnits > nbPositions){
        //Send the information to the receiver.
        emit dataReady(data,initiator);
        return;
    }
    //The end time is over the maximum of the position file
    if(endInRecordingUnits > nbPositions) endInRecordingUnits = nbPositions;
    if(startInRecordingUnits == 0) startInRecordingUnits = 1;//the arrary positions starts at 1

    dataType nbSamples = static_cast<dataType>(endInRecordingUnits - startInRecordingUnits) + 1;

    //data will contain the final values.
    data.setSize(nbSamples,nbCoordinates);

    //Transform the data if need it.
    //No transformation
    if(rotation == 0 && flip == 0){
        long count = 1;
        for(int i = startInRecordingUnits; i<= endInRecordingUnits; ++i){
            for(int j = 1;j<=nbCoordinates;++j){
                data(count,j) = positions(i,j);
            }
            count++;
        }

        //Send the information to the receiver.
        emit dataReady(data,initiator);
        return;
    }

    //vertical flip x = x;y = Y - y;X = width; Y = height
    if(rotation == 0 && flip == 1){
        long count = 1;
        for(int i = startInRecordingUnits; i<= endInRecordingUnits; ++i){
            for(int j = 1;j<=nbCoordinates;j=j+2){
                data(count,j) = positions(i,j);//x
                data(count,j+1) = height - positions(i,j+1);//y
            }
            count++;
        }

        //Send the information to the receiver.
        emit dataReady(data,initiator);
        return;
    }

    //horizontal flip, x = X - x;y = y;X = width; Y = height
    if(rotation == 0 && flip == 2){
        long count = 1;
        for(int i = startInRecordingUnits; i<= endInRecordingUnits; ++i){
            for(int j = 1;j<=nbCoordinates;j=j+2){
                data(count,j) = width - positions(i,j);//x
                data(count,j+1) = positions(i,j+1);//y
            }
            count++;
        }

        //Send the information to the receiver.
        emit dataReady(data,initiator);
        return;
    }
    
    //rotation of 90 degrees, x = Y - y;y = x; Y = height
    if(rotation == 90 && flip == 0){
        long count = 1;
        for(int i = startInRecordingUnits; i<= endInRecordingUnits; ++i){
            for(int j = 1;j<=nbCoordinates;j=j+2){
                data(count,j) =  height - positions(i,j+1);//x
                data(count,j+1) = positions(i,j);//y
            }
            count++;
        }

        //Send the information to the receiver.
        emit dataReady(data,initiator);
        return;
    }

    //rotation of 90 degrees and vertical flip, x = Y -y;y = X - x;X = width; Y = height
    if(rotation == 90 && flip == 1){
        long count = 1;
        for(int i = startInRecordingUnits; i<= endInRecordingUnits; ++i){
            for(int j = 1;j<=nbCoordinates;j=j+2){
                data(count,j) = height - positions(i,j+1);//x
                data(count,j+1) = width - positions(i,j);//y
            }
            count++;
        }

        //Send the information to the receiver.
        emit dataReady(data,initiator);
        return;
    }

    //rotation of 90 degrees and horizontal flip, x = y;y = x
    if(rotation == 90 && flip == 2){
        long count = 1;
        for(int i = startInRecordingUnits; i<= endInRecordingUnits; ++i){
            for(int j = 1;j<=nbCoordinates;j=j+2){
                data(count,j) = positions(i,j+1);//x
                data(count,j+1) = positions(i,j);//y
            }
            count++;
        }

        //Send the information to the receiver.
        emit dataReady(data,initiator);
        return;
    }

    //rotation of 180 degrees, x = X - x;y = Y - y;X = width; Y = height
    if(rotation == 180 && flip == 0){
        long count = 1;
        for(int i = startInRecordingUnits; i<= endInRecordingUnits; ++i){
            for(int j = 1;j<=nbCoordinates;j=j+2){
                data(count,j) = width - positions(i,j);//x
                data(count,j+1) = height - positions(i,j+1);//y
            }
            count++;
        }

        //Send the information to the receiver.
        emit dataReady(data,initiator);
        return;
    }

    //rotation of 180 degrees and vertical flip, x = X - x;y = y;X = width; Y = height
    if(rotation == 180 && flip == 1){
        long count = 1;
        for(int i = startInRecordingUnits; i<= endInRecordingUnits; ++i){
            for(int j = 1;j<=nbCoordinates;j=j+2){
                data(count,j) = width - positions(i,j);//x
                data(count,j+1) = positions(i,j+1);//y
            }
            count++;
        }

        //Send the information to the receiver.
        emit dataReady(data,initiator);
        return;
    }

    //rotation of 180 degrees and horizontal flip, x = x;y = Y - y;X = width; Y = height
    if(rotation == 180 && flip == 2){
        long count = 1;
        for(int i = startInRecordingUnits; i<= endInRecordingUnits; ++i){
            for(int j = 1;j<=nbCoordinates;j=j+2){
                data(count,j) = positions(i,j);//x
                data(count,j+1) = height - positions(i,j+1);//y
            }
            count++;
        }

        //Send the information to the receiver.
        emit dataReady(data,initiator);
        return;
    }

    //rotation of 270 degrees, x = y;y = X - x;X = width; Y = height
    if(rotation == 270 && flip == 0){
        long count = 1;
        for(int i = startInRecordingUnits; i<= endInRecordingUnits; ++i){
            for(int j = 1;j<=nbCoordinates;j=j+2){
                data(count,j) = positions(i,j+1);//x
                data(count,j+1) = width - positions(i,j);//y
            }
            count++;
        }

        //Send the information to the receiver.
        emit dataReady(data,initiator);
        return;
    }

    //rotation of 270 degrees and vertical flip, x = y;y = x
    if(rotation == 270 && flip == 1){
        long count = 1;
        for(int i = startInRecordingUnits; i<= endInRecordingUnits; ++i){
            for(int j = 1;j<=nbCoordinates;j=j+2){
                data(count,j) = positions(i,j+1);//x
                data(count,j+1) = positions(i,j);//y
            }
            count++;
        }

        //Send the information to the receiver.
        emit dataReady(data,initiator);
        return;
    }

    //rotation of 270 degrees and horizontal flip, x = Y -y;y = X - x;X = width; Y = height
    if(rotation == 270 && flip == 2){
        long count = 1;
        for(int i = startInRecordingUnits; i<= endInRecordingUnits; ++i){
            for(int j = 1;j<=nbCoordinates;j=j+2){
                data(count,j) = height - positions(i,j+1);//x
                data(count,j+1) = width - positions(i,j);//y
            }
            count++;
        }

        //Send the information to the receiver.
        emit dataReady(data,initiator);
        return;
    }

}

