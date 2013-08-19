/***************************************************************************
                          clustersprovider.cpp  -  description
                             -------------------
    begin                : Wed Apr 14 2004
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
//QT include files
#include <QFile>
#include <QStringList>
#include <QFileInfo>

#include <QList>
#include <QMap> 
#include <QDebug>

//Unix include file
#include <unistd.h>

//include files for the application
#include "clustersprovider.h"
#include "timer.h"
#include "utilities.h"


ClustersProvider::ClustersProvider(const QString &fileUrl, double samplingRate, double currentSamplingRate, dataType fileMaxTime, int position)
    : DataProvider(fileUrl),timeFileUrl(fileUrl),
    samplingRate(samplingRate),nbSpikes(0),nbClusters(0),clusterPosition(static_cast<float>(position) / 100.0)
{

    dataCurrentRatio = static_cast<float>(samplingRate / currentSamplingRate);
    dataFileMaxTime = fileMaxTime * dataCurrentRatio;

    //Find the cluster file number and use it as the name for the provider
    //the file name can be X.clu.n or X.n.clu
    QString fileName = fileUrl;
    int startingIndex = fileName.lastIndexOf("clu");
    if(startingIndex == static_cast<int>(fileName.length()) - 3){//X.n.clu
        int nBStartingIndex = fileName.lastIndexOf(".",startingIndex - 2);
        name = fileName.mid(nBStartingIndex + 1,(startingIndex - 1) - (nBStartingIndex + 1));
    } else {//X.clu.n
        int nBStartingIndex = fileName.lastIndexOf(".");
        name = fileName.right(static_cast<int>(fileName.length()) - (nBStartingIndex + 1));
    }

    fileName = fileName.replace(startingIndex,3,"res" );
    timeFileUrl=(fileName);
}

ClustersProvider::~ClustersProvider(){
}

int ClustersProvider::loadData(){

    //Fist check if the time file (.res) exists
    QString timeFilePath = timeFileUrl;
    if(!QFile(timeFileUrl).exists()){
        clusters.setSize(0,0);
        return MISSING_FILE;
    }

    //Get the number of spikes
    nbSpikes = Utilities::getNbLines(timeFilePath);

    qDebug()<<"nbSpikes "<<nbSpikes;

    if(nbSpikes == -1){
        clusters.setSize(0,0);
        return COUNT_ERROR;
    }

    //should not happen, but just in case
    if(nbSpikes == 0){
        clusters.setSize(0,0);

        //Initialize the variables
        previousStartTime = 0;
        previousStartIndex = 1;
        previousEndIndex = 1;
        previousEndTime = 0;
        fileMaxTime = 0;

        return OK;
    }

    RestartTimer();

    //Create a reader on the clusterFile
    QFile clusterFile(fileName);
    bool status = clusterFile.open(QIODevice::ReadOnly);
    if(!status){
        clusters.setSize(0,0);
        return OPEN_ERROR;
    }

    //Set the size of the Array containing the spikes and clusters ids.
    clusters.setSize(2,nbSpikes);

    //Get the number of clusters stored on the first line.
    QString sNbClusters;
    QByteArray buf;
    buf.resize(255);
    int ret=clusterFile.readLine(buf.data(), 255);
    sNbClusters = QString::fromLatin1(buf, ret);


    nbClusters = sNbClusters.toInt();
    //This map is used as an easy way to compute the unique list of cluster ids
    QMap<int,long> ids;
    QByteArray buffer = clusterFile.readAll();
    uint size = buffer.size();

    //The buffer is read and each dataType is build char by char into a string. When the char read
    //is not [1-9] (<=> blank space or a new line), the string is converted into a dataType and store
    //into the first row of clusters.
    //string of character which will contains the current seek dataType
    dataType k = 0;
    int l = 0;
    char clusterID[255];
    for(uint i = 0 ; i < size ; ++i){
        if(buffer[i] >= '0' && buffer[i] <= '9')  {
            clusterID[l++] = buffer[i];
        } else if(l) {
            clusterID[l] = '\0';
            long id = atol(clusterID);
            clusters[k++] = id;//Warning if the typedef dataType changes, change will have to be made here.
            ids.insert(static_cast<int>(id),id);
            l = 0;
        }
    }

    clusterFile.close();
    clusterIds = ids.keys();

    //The number of spikes read has to be coherent with the number of spikes computed by wc -l (via Utilities::getNbLines).
    if(k != nbSpikes){
        clusters.setSize(0,0);
        return INCORRECT_CONTENT;
    }

    //Get the spikes time

    //Create a reader on the spikeFile
    QFile spikeFile(timeFilePath);
    status = spikeFile.open(QIODevice::ReadOnly);
    if(!status){
        clusters.setSize(0,0);
        return OPEN_ERROR;
    }


    QByteArray spikeBuffer = spikeFile.readAll();
    size = spikeBuffer.size();

    //The buffer is read and each dataType is build char by char into a string. When the char read
    //is not [1-9] (<=> blank space or a new line), the string is converted into a dataType and store
    //into the second row of clusters.
    //string of character which will contains the current seek dataType
    l = 0;
    char time[255];
    for(uint i = 0 ; i < size ; ++i){
        if(spikeBuffer[i] >= '0' && spikeBuffer[i] <= '9') time[l++] = spikeBuffer[i];
        else if(l){
            time[l] = '\0';
            clusters[k++] = atol(time);//Warning if the typedef dataType changes, change will have to be made here.
            l = 0;
        }
    }

    spikeFile.close();

    //The number of spikes read has to be coherent with the number of spikes computed by wc -l (via Utilities::getNbLines).
    if(k != (2 * nbSpikes)){
        clusters.setSize(0,0);
        return INCORRECT_CONTENT;
    }

    qDebug() << "Loading clu file into memory: "<<Timer() << endl;

    //Initialize the variables
    previousStartTime = 0;
    previousStartIndex = 1;
    previousEndIndex = nbSpikes;
    double maxTime =  static_cast<double>(static_cast<double>(clusters(2,nbSpikes)) * static_cast<double>(1000) / static_cast<double>(samplingRate));
    previousEndTime = static_cast<dataType>(floor(0.5 + maxTime));
    fileMaxTime = previousEndTime;

    return OK;
}

void ClustersProvider::requestData(long startTime,long endTime,QObject* initiator,long startTimeInRecordingUnits){
    retrieveData(startTime,endTime,initiator,startTimeInRecordingUnits);
}

void ClustersProvider::retrieveData(long startTime,long endTime,QObject* initiator,long startTimeInRecordingUnits){
    Array<dataType> data;

    //qDebug()<<" in retrieveData, startTime " <<startTime<<" endTime " <<endTime<<" startTimeInRecordingUnits "<<startTimeInRecordingUnits;

    //qDebug()<<" in retrieveData, fileMaxTime " <<fileMaxTime<<" samplingRate " <<samplingRate;


    if(startTime > fileMaxTime){
        //Send the information to the receiver.
        emit dataReady(data,initiator,name);
        return;
    }
    if(endTime > fileMaxTime)
        endTime = fileMaxTime;


    //Convert the time in miliseconds to time in recording units if need it.
    dataType startInRecordingUnits;
    //startTimeInRecordingUnits has been computed in a previous call to a browsing function. It has to be used insted of computing
    //the value from startTime because of the rounding which has been applied to it.
    if(startTimeInRecordingUnits != 0)
        startInRecordingUnits = startTimeInRecordingUnits;
    else
        startInRecordingUnits = static_cast<dataType>(startTime * static_cast<double>(static_cast<double>(samplingRate) / static_cast<double>(1000)));
    dataType endInRecordingUnits;
    //Hack to be sure not to forget a spike due to conversion rounding
    if(endTime == fileMaxTime)
        endInRecordingUnits = clusters(2,nbSpikes);
    else
        endInRecordingUnits =  static_cast<dataType>(endTime * static_cast<double>(static_cast<double>(samplingRate) / static_cast<double>(1000)));

    long startIndex;
    long endIndex;
    long dicotomyBreak = 1000;
    long time;

    //Look up for the closest starting index to the one corresponding to startTime
    //Dicotomy will be used with a stop at dicotomyBreak.
    if((startTime != previousStartTime) && (startTime != previousEndTime)){
        if(startTime == 0){
            startIndex = 1;
            if(endTime <= previousStartTime) endIndex = previousStartIndex;
            else if(endTime <= previousEndTime) endIndex = previousEndIndex;
            else if(endTime > previousEndTime) endIndex = nbSpikes;
        }
        if(startTime < previousStartTime){
            startIndex = static_cast<int>(previousStartIndex / 2);
            if(startIndex <= 0)
                startIndex = 1;

            if(endTime <= previousStartTime)
                endIndex = previousStartIndex;
            else if(endTime <= previousEndTime)
                endIndex = previousEndIndex;
            else if(endTime > previousEndTime)
                endIndex = nbSpikes;
        }
        else if(startTime < previousEndTime && startTime > previousStartTime){
            startIndex = previousStartIndex + static_cast<int>((previousEndIndex - previousStartIndex + 1)/ 2);
            if(endTime <= previousEndTime)
                endIndex = previousEndIndex;
            else if(endTime > previousEndTime)
                endIndex = nbSpikes;
        }
        else if(startTime > previousEndTime){
            startIndex = previousEndIndex;
            endIndex = nbSpikes;
        }

        long newStartIndex = startIndex;
        long newEndIndex = endIndex;
        //dicotomy
        while((newEndIndex - newStartIndex + 1) > dicotomyBreak){
            time = clusters(2,newStartIndex);
            if(time == startInRecordingUnits) break;
            else if(time > startInRecordingUnits){
                long previousStart = newStartIndex;
                newStartIndex = previousStart - ((newEndIndex - previousStart + 1) / 2);
                if(newStartIndex <= 0){
                    newStartIndex = 1;
                    break;
                }
                newEndIndex = previousStart;
            }
            else{
                newStartIndex = newStartIndex + ((newEndIndex - newStartIndex + 1) / 2);
                if(newStartIndex > nbSpikes){
                    newStartIndex = nbSpikes;
                    break;
                }
            }
        }

        //look up for the startIndex index by index
        time = clusters(2,newStartIndex);

        if(time < startInRecordingUnits && (newStartIndex < nbSpikes)){
            while(time < startInRecordingUnits){
                newStartIndex++;
                if(newStartIndex > nbSpikes){
                    newStartIndex = nbSpikes;
                    time =  clusters(2,newStartIndex);
                    break;
                }
                else{
                    time =  clusters(2,newStartIndex);
                }
            }
        }
        else if(time > startInRecordingUnits && (newStartIndex > 1)){
            while(time > startInRecordingUnits){
                newStartIndex--;
                if(newStartIndex <= 0){
                    newStartIndex = 1;
                    time =  clusters(2,newStartIndex);
                    break;
                }
                else{
                    time =  clusters(2,newStartIndex);
                    if(time < startInRecordingUnits){
                        newStartIndex++;
                        time =  clusters(2,newStartIndex);
                        break;
                    }
                }
            }
        }

        startIndex = newStartIndex;
    }//end (startTime != previousStartTime) || (startTime != previousEndTime)

    else{
        if(startTime == previousStartTime) startIndex = previousStartIndex;
        else if(startTime == previousEndTime){
            if(clusters(2,previousEndIndex) < startInRecordingUnits){
                startIndex = previousEndIndex + 1;
                if(startIndex > nbSpikes) startIndex = nbSpikes;
            }
            else startIndex = previousEndIndex;
        }

        if(endTime <= previousEndTime) endIndex = previousEndIndex;
        else if(endTime > previousEndTime) endIndex = nbSpikes;
    }

    //Store the information for the next request
    previousStartTime = startTime;
    previousStartIndex = startIndex;

    //look up for the cluster ids and indexes.
    //The exact size (<=> number of spikes is not known yet, so the size of data is set to the maximum possible)
    data.setSize(2,(endIndex - startIndex + 1));//line 1 sample index,line 2 clusterId
    time = clusters(2,startIndex);

    long count = 0;
    if(dataCurrentRatio != 1){
        while(time <= endInRecordingUnits && startIndex <= nbSpikes){
            double currentTime = static_cast<double>(time - startInRecordingUnits) / static_cast<double>(dataCurrentRatio);
            data(1,count + 1) = static_cast<dataType>(floor(0.5 + currentTime));
            data(2,count + 1) = clusters(1,startIndex);

            //if(data(2,count + 1) ==9 || data(2,count + 1) == 8)qDebug()<<" in retrieveData, data(1,count + 1) " <<data(1,count + 1)<<" data(2,count + 1) " <<data(2,count + 1)<<endl;


            count++;
            startIndex++;
            time = clusters(2,startIndex);
        }
    }
    else{
        while(time <= endInRecordingUnits && startIndex <= nbSpikes){
            data(1,count + 1) = time - startInRecordingUnits;
            data(2,count + 1) = clusters(1,startIndex);
            count++;
            startIndex++;
            if(startIndex > nbSpikes) break;
            time = clusters(2,startIndex);
        }
    }


    qDebug()<<" in retrieveData, count " <<count<<" startInRecordingUnits " <<startInRecordingUnits<<" endInRecordingUnits " <<endInRecordingUnits<<" endTime " <<endTime<<endl;


    //Store the data in a array of the good size
    Array<dataType> finalData;
    finalData.setSize(2,count);
    finalData.copySubset(data,count);

    //Store the information for the next request
    previousEndTime = endTime;
    previousEndIndex = startIndex - 1;

    //Send the information to the receiver.
    emit dataReady(finalData,initiator,name);
}

void ClustersProvider::requestNextClusterData(long startTime, long timeFrame, const QList<int> &selectedIds, QObject* initiator, long startTimeInRecordingUnits){
    long initialStartTime = startTime;
    //Compute the start time for the spike look up
    startTime = initialStartTime + static_cast<long>(timeFrame * clusterPosition);

    qDebug()<<"timeFrame " <<timeFrame<<" clusterPosition " <<clusterPosition<<" initialStartTime " <<initialStartTime<<" startTime " <<startTime<<" startTime " <<startTime<<" startTimeInRecordingUnits " <<startTimeInRecordingUnits;

    //first look up for the index corresponding to the startTime and then for the first valid spike (contained in selectedIds)
    //after that time
    Array<dataType> data;

    if(startTime > fileMaxTime){
        //Send the information to the receiver.
        emit dataReady(data,initiator,name);
        return;
    }

    long startIndex = previousStartIndex;
    //By default we will go up to the end of the file
    long endIndex = nbSpikes;
    long dicotomyBreak = 1000;
    long time;


    //Convert the time in miliseconds to time in recording units (to compare it with the data from the file) if need it.
    dataType startInRecordingUnits;
    //startTimeInRecordingUnits has been computed in a previous call to a browsing function. It has to be used insted of computing
    //the value from startTime because of the rounding which has been applied to it.
    if(startTimeInRecordingUnits != 0){
        //the found spike will be placed at clusterPosition*100 % of the timeFrame
        dataType timeFrameInRecordingUnits = static_cast<dataType>(timeFrame * static_cast<double>(static_cast<double>(samplingRate) / static_cast<double>(1000)));
        float position = static_cast<float>(timeFrameInRecordingUnits) * clusterPosition;
        //startTimeInRecordingUnits is given in recording units computed with the current sampling rate, it has to be converted in
        //the recording units corresponding to the acquisiton system sampling rate (unit used in the cluster file).
        double start = static_cast<double>(startTimeInRecordingUnits) * static_cast<double>(dataCurrentRatio);
        startInRecordingUnits = static_cast<dataType>(floor(0.5 + start)) + static_cast<long>(position);
    }
    else startInRecordingUnits = static_cast<dataType>(startTime * static_cast<double>(static_cast<double>(samplingRate) / static_cast<double>(1000)));

    //Look up for the closest starting index to the one corresponding to startTime
    //Dicotomy will be used with a stop at dicotomyBreak.
    if((startTime != previousStartTime) && (startTime != previousEndTime)){
        if(startTime == 0){
            startIndex = 1;
        }
        if(startTime < previousStartTime){
            startIndex = static_cast<int>(previousStartIndex / 2);
            if(startIndex <= 0) startIndex = 1;
        }
        else if(startTime < previousEndTime && startTime > previousStartTime){
            startIndex = previousStartIndex + static_cast<int>((previousEndIndex - previousStartIndex + 1)/ 2);
        }
        else if(startTime > previousEndTime){
            startIndex = previousEndIndex;
        }

        long newStartIndex = startIndex;
        long newEndIndex = endIndex;
        //dicotomy
        while((newEndIndex - newStartIndex + 1) > dicotomyBreak){
            time = clusters(2,newStartIndex);
            if(time == startInRecordingUnits) break;
            else if(time > startInRecordingUnits){
                long previousStart = newStartIndex;
                newStartIndex = previousStart - ((newEndIndex - previousStart + 1) / 2);
                if(newStartIndex <= 0){
                    newStartIndex = 1;
                    break;
                }
                newEndIndex = previousStart;
            }
            else{
                newStartIndex = newStartIndex + ((newEndIndex - newStartIndex + 1) / 2);
                if(newStartIndex > nbSpikes){
                    newStartIndex = nbSpikes;
                    break;
                }
            }
        }


        //look up for the startIndex index by index
        time = clusters(2,newStartIndex);

        qDebug()<<"newStartIndex " <<newStartIndex<<" time " <<time<<" nbSpikes " <<nbSpikes <<endl;

        if(time < startInRecordingUnits && (newStartIndex < nbSpikes)){
            while(time < startInRecordingUnits){
                newStartIndex++;
                if(newStartIndex > nbSpikes){
                    newStartIndex = nbSpikes;
                    time =clusters(2,newStartIndex);
                    break;
                }
                else{
                    time = clusters(2,newStartIndex);
                }
            }
        }
        else if(time > startInRecordingUnits && (newStartIndex > 1)){
            while(time > startInRecordingUnits){
                newStartIndex--;
                if(newStartIndex <= 0){
                    newStartIndex = 1;
                    time =  clusters(2,newStartIndex);
                    break;
                }
                else{
                    time =  clusters(2,newStartIndex);
                    if(time < startInRecordingUnits){
                        newStartIndex++;
                        time =  clusters(2,newStartIndex);
                        break;
                    }
                }
            }
        }

        startIndex = newStartIndex;
    }//end (startTime != previousStartTime) || (startTime != previousEndTime)

    else{
        if(startTime == previousStartTime) startIndex = previousStartIndex;
        else if(startTime == previousEndTime){
            if(clusters(2,previousEndIndex) < startTime){
                startIndex = previousEndIndex + 1;
                if(startIndex > nbSpikes) startIndex = nbSpikes;
            }
            else startIndex = previousEndIndex;
        }
    }


    time = clusters(2,startIndex);

    dataType previousTime = previousStartTime + static_cast<long>(timeFrame * clusterPosition);
    double computeTime = static_cast<double>(static_cast<double>(time) * static_cast<double>(1000) / static_cast<double>(samplingRate));
    dataType timeInMiliseconds = static_cast<dataType>(floor(0.5 + computeTime));
    //check that the spike corresponding to the current startIndex
    //is not the one already at clusterPosition, if so take the following start index.
    if(((timeInMiliseconds == previousTime) || (time == startInRecordingUnits)) && (startIndex < nbSpikes))
        startIndex++;

    //look up for the first spike contained in selectedIds which exist after startTime
    while(true){
        int id = clusters(1,startIndex);
        if(selectedIds.contains(id)) break;
        startIndex++;
        if(startIndex > nbSpikes){
            startIndex = nbSpikes;
            break;
        }
    }

    //check that a valid index has been found (the startIndex corresponds to a spike included in selectedIds)
    //if that is not the case return startTime as the startingTime => no change will be done in the view, and startTimeInRecordingUnits
    int id = clusters(1,startIndex);
    if(!selectedIds.contains(id)){
        Array<dataType> data;
        emit nextClusterDataReady(data,initiator,name,initialStartTime,startTimeInRecordingUnits);
        return;
    }

    time = clusters(2,startIndex);

    //the found spike will be placed at clusterPosition*100 % of the timeFrame
    dataType timeFrameInRecordingUnits = static_cast<dataType>(timeFrame * static_cast<double>(static_cast<double>(samplingRate) / static_cast<double>(1000)));

    //compute the final starting time
    float position = static_cast<float>(timeFrameInRecordingUnits) * clusterPosition;
    dataType startingInRecordingUnits = qMax(time - static_cast<long>(position),0L);

    //compute the final index corresponding to the final starting time
    long newStartIndex = startIndex;
    while(time > startingInRecordingUnits){
        newStartIndex--;
        if(newStartIndex <= 0){
            newStartIndex = 1;
            time =  clusters(2,newStartIndex);
            break;
        }
        else{
            time =  clusters(2,newStartIndex);
            if(time < startingInRecordingUnits){
                newStartIndex++;
                time =  clusters(2,newStartIndex);
                break;
            }
        }
    }

    startIndex = newStartIndex;

    dataType endInRecordingUnits = startingInRecordingUnits + timeFrameInRecordingUnits;

    //Always keep the same timeFrame
    if(endInRecordingUnits > dataFileMaxTime){
        startingInRecordingUnits = dataFileMaxTime - timeFrameInRecordingUnits;
        endInRecordingUnits = dataFileMaxTime;
    }

    //Store the information for the next request
    double computeStartingTime = static_cast<double>(static_cast<double>(startingInRecordingUnits) * static_cast<double>(1000) / static_cast<double>(samplingRate));
    previousStartTime = static_cast<dataType>(floor(0.5 + computeStartingTime));
    previousStartIndex = startIndex;

    //look up for the spike ids and indexes.
    //The exact size (<=> number of spikes is not known yet, so the size of data is set to the maximum possible)

    data.setSize(2,(endIndex - startIndex + 1));//line 1 sample index,line 2 clusterId
    time = clusters(2,startIndex);

    long count = 0;
    if(dataCurrentRatio != 1){
        while(time <= endInRecordingUnits && startIndex <= nbSpikes){
            double currentTime = static_cast<double>(time - startingInRecordingUnits) / static_cast<double>(dataCurrentRatio);
            data(1,count + 1) = static_cast<dataType>(floor(0.5 + currentTime));
            data(2,count + 1) = clusters(1,startIndex);

            count++;
            startIndex++;
            if(startIndex > nbSpikes) break;
            time = clusters(2,startIndex);
        }
    }
    else{
        while(time <= endInRecordingUnits && startIndex <= nbSpikes){
            data(1,count + 1) = (time - startingInRecordingUnits);
            data(2,count + 1) = clusters(1,startIndex);
            count++;
            startIndex++;
            if(startIndex > nbSpikes) break;
            time = clusters(2,startIndex);
        }
    }


    qDebug()<<" count " <<count <<endl;


    //Store the data in a array of the good size
    Array<dataType> finalData;
    finalData.setSize(2,count);
    finalData.copySubset(data,count);

    //Store the information for the next request
    double computeEndTime = static_cast<double>(static_cast<double>(endInRecordingUnits) * static_cast<double>(1000) / static_cast<double>(samplingRate));
    previousEndTime = static_cast<dataType>(computeEndTime) + 1;
    previousEndIndex = startIndex - 1;

    //the data provider needs the startingInRecordingUnits computed with the current sampling rate and not the one use to record
    //the data (if the currently open file is a .dat file the sampling rates are equals)
    double startingInCurrentRecordingUnits = static_cast<double>(startingInRecordingUnits) / static_cast<double>(dataCurrentRatio);
    dataType startingInCurrentRecordingUnits2 = static_cast<dataType>(floor(0.5 + startingInCurrentRecordingUnits));

    //Send the information to the receiver.
    emit nextClusterDataReady(finalData,initiator,name,previousStartTime,startingInCurrentRecordingUnits2);
}


void ClustersProvider::requestPreviousClusterData(long startTime,long timeFrame,QList<int> selectedIds,QObject* initiator,long startTimeInRecordingUnits){

    long initialStartTime = startTime;
    //Compute the start time for the spike look up
    startTime = initialStartTime + static_cast<long>(timeFrame * clusterPosition);

    //first look up for the index corresponding to the startTime and then for the first valid spike (contained in selectedIds)
    //before that time.
    Array<dataType> data;

    long startIndex = previousStartIndex;
    //By default we will go up to the end of the file
    long endIndex = nbSpikes;
    long dicotomyBreak = 1000;
    long time;

    //Convert the time in miliseconds to time in recording units (to compare it with the data from the file) if need it.
    dataType startInRecordingUnits;
    //startTimeInRecordingUnits has been computed in a previous call to a browsing function. It has to be used insted of computing
    //the value from startTime because of the rounding which has been applied to it.
    if(startTimeInRecordingUnits != 0){
        //the found spike will be placed at clusterPosition*100 % of the timeFrame
        dataType timeFrameInRecordingUnits = static_cast<dataType>(timeFrame * static_cast<double>(static_cast<double>(samplingRate) / static_cast<double>(1000)));
        float position = static_cast<float>(timeFrameInRecordingUnits) * clusterPosition;

        //startTimeInRecordingUnits is given in recording units computed with the current sampling rate, it has to be converted in
        //the recording units corresponding to the acquisiton system sampling rate (unit used in the cluster file).
        double start = static_cast<double>(startTimeInRecordingUnits) * static_cast<double>(dataCurrentRatio);
        startInRecordingUnits = static_cast<dataType>(floor(0.5 + start)) + static_cast<long>(position);
    }
    else startInRecordingUnits = static_cast<dataType>(startTime * static_cast<double>(static_cast<double>(samplingRate) / static_cast<double>(1000)));

    //Look up for the closest starting index to the one corresponding to startTime
    //Dicotomy will be used with a stop at dicotomyBreak.
    if((startTime != previousStartTime) && (startTime != previousEndTime)){
        if(startTime == 0){
            startIndex = 1;
        }
        if(startTime < previousStartTime){
            startIndex = static_cast<int>(previousStartIndex / 2);
            if(startIndex <= 0) startIndex = 1;
        }
        else if(startTime < previousEndTime && startTime > previousStartTime){
            startIndex = previousStartIndex + static_cast<int>((previousEndIndex - previousStartIndex + 1)/ 2);
        }
        else if(startTime > previousEndTime){
            startIndex = previousEndIndex;
        }

        long newStartIndex = startIndex;
        long newEndIndex = endIndex;

        //dicotomy
        while((newEndIndex - newStartIndex + 1) > dicotomyBreak){
            time = clusters(2,newStartIndex);
            if(time == startInRecordingUnits) break;
            else if(time > startInRecordingUnits){
                long previousStart = newStartIndex;
                newStartIndex = previousStart - ((newEndIndex - previousStart + 1) / 2);
                if(newStartIndex <= 0){
                    newStartIndex = 1;
                    break;
                }
                newEndIndex = previousStart;
            }
            else{
                newStartIndex = newStartIndex + ((newEndIndex - newStartIndex + 1) / 2);
                if(newStartIndex > nbSpikes){
                    newStartIndex = nbSpikes;
                    break;
                }
            }
        }


        //look up for the startIndex index by index
        time = clusters(2,newStartIndex);

        if(time < startInRecordingUnits && (newStartIndex < nbSpikes)){
            while(time < startInRecordingUnits){
                newStartIndex++;
                if(newStartIndex > nbSpikes){
                    newStartIndex = nbSpikes;
                    time = clusters(2,newStartIndex);
                    break;
                }
                else{
                    time = clusters(2,newStartIndex);
                    if(time > startInRecordingUnits){
                        newStartIndex--;
                        time =  clusters(2,newStartIndex);
                        break;
                    }
                }
            }
        }
        else if(time > startInRecordingUnits && (newStartIndex > 1)){
            while(time > startInRecordingUnits){
                newStartIndex--;
                if(newStartIndex <= 0){
                    newStartIndex = 1;
                    time =  clusters(2,newStartIndex);
                    break;
                }
                else{
                    time =  clusters(2,newStartIndex);
                    if(time < startInRecordingUnits) break;
                }
            }
        }

        startIndex = newStartIndex;
    }//end (startTime != previousStartTime) || (startTime != previousEndTime)

    else{
        if(startTime == previousStartTime) startIndex = previousStartIndex;
        else if(startTime == previousEndTime){
            if(clusters(2,previousEndIndex) < startTime){
                startIndex = previousEndIndex + 1;
                if(startIndex > nbSpikes) startIndex = nbSpikes;
            }
            else startIndex = previousEndIndex;
        }
    }


    //check that the spike corresponding to the startIndex
    //is not the one already at clusterPosition, if so take the previous index
    time = clusters(2,startIndex);

    dataType previousTime = previousStartTime + static_cast<long>(timeFrame * clusterPosition);
    double computeTime = static_cast<double>(static_cast<double>(time) * static_cast<double>(1000) / static_cast<double>(samplingRate));
    dataType timeInMiliseconds = static_cast<dataType>(floor(0.5 + computeTime));
    if((time == startInRecordingUnits || timeInMiliseconds == previousTime) && (startIndex > 1)) startIndex--;

    //now, look up for the first spike contained in selectedIds which exist before endTime
    while(true){
        int id = clusters(1,startIndex);
        if(selectedIds.contains(id)) break;
        startIndex--;
        if(startIndex <= 0){
            startIndex = 1;
            break;
        }
    }

    //check that a valid index has been found: the startIndex corresponds to n spike included in selectedIds
    //and the corresponding time in before the startTime
    //if that is not the case return initialStartTime as the startingTime => no change will be done in the view, and startTimeInRecordingUnits
    int id = clusters(1,startIndex);
    time = clusters(2,startIndex);
    if(!selectedIds.contains(id) || time > startInRecordingUnits){
        Array<dataType> data;
        emit previousClusterDataReady(data,initiator,name,initialStartTime,startTimeInRecordingUnits);
        return;
    }

    //the found spike will be placed at clusterPosition*100 % of the timeFrame
    dataType timeFrameInRecordingUnits = static_cast<dataType>(timeFrame * static_cast<double>(static_cast<double>(samplingRate) / static_cast<double>(1000)));
    //compute the final starting time and the corresponding index
    float position = static_cast<float>(timeFrameInRecordingUnits) * clusterPosition;
    dataType startingInRecordingUnits = qMax(time - static_cast<long>(position),0L);

    //compute the final index corresponding to the final starting time
    long newStartIndex = startIndex;
    while(time > startingInRecordingUnits){
        newStartIndex--;
        if(newStartIndex <= 0){
            newStartIndex = 1;
            time =  clusters(2,newStartIndex);
            break;
        }
        else{
            time =  clusters(2,newStartIndex);
            if(time < startingInRecordingUnits){
                newStartIndex++;
                time =  clusters(2,newStartIndex);
                break;
            }
        }
    }

    startIndex = newStartIndex;

    //Store the information for the next request
    double computeStartingTime = static_cast<double>(static_cast<double>(startingInRecordingUnits) * static_cast<double>(1000) / static_cast<double>(samplingRate));
    previousStartTime = static_cast<dataType>(floor(0.5 + computeStartingTime));
    previousStartIndex = startIndex;

    //look up for the cluster ids and indexes.
    //The exact size (<=> number of spikes is not known yet, so the size of data is set to the maximum possible)
    data.setSize(2,(endIndex - startIndex + 1));//line 1 sample index,line 2 clusterId
    time = clusters(2,startIndex);
    dataType endInRecordingUnits = startingInRecordingUnits + timeFrameInRecordingUnits;

    long count = 0;
    if(dataCurrentRatio != 1){
        while(time <= endInRecordingUnits && startIndex <= nbSpikes){
            double currentTime = static_cast<double>(time - startingInRecordingUnits) / static_cast<double>(dataCurrentRatio);
            data(1,count + 1) = static_cast<dataType>(floor(0.5 + currentTime));
            data(2,count + 1) = clusters(1,startIndex);
            count++;
            startIndex++;
            if(startIndex > nbSpikes) break;
            time = clusters(2,startIndex);
        }
    }
    else{
        while(time <= endInRecordingUnits && startIndex <= nbSpikes){
            data(1,count + 1) = (time - startingInRecordingUnits);
            data(2,count + 1) = clusters(1,startIndex);

            count++;
            startIndex++;
            if(startIndex > nbSpikes) break;
            time = clusters(2,startIndex);
        }
    }

    //Store the data in a array of the good size
    Array<dataType> finalData;
    finalData.setSize(2,count);
    finalData.copySubset(data,count);

    //Store the information for the next request
    double computeEndTime = static_cast<double>(static_cast<double>(endInRecordingUnits) * static_cast<double>(1000) / static_cast<double>(samplingRate));
    previousEndTime = static_cast<dataType>(computeEndTime) + 1;
    previousEndIndex = startIndex - 1;

    //the data provider needs the startingInRecordingUnits computed with the current sampling rate and not the one use to record
    //the data (if the currently open file is a .dat file the sampling rates are equals)
    double startingInCurrentRecordingUnits = static_cast<double>(startingInRecordingUnits) / static_cast<double>(dataCurrentRatio);
    dataType startingInCurrentRecordingUnits2 = static_cast<dataType>(floor(0.5 + startingInCurrentRecordingUnits));


    //Send the information to the receiver.
    emit previousClusterDataReady(finalData,initiator,name,previousStartTime,startingInCurrentRecordingUnits2);
}
