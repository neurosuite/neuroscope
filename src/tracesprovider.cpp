/***************************************************************************
                          tracesprovider.cpp  -  description
                             -------------------
    begin                : Mon Mar 1 2004
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

//C include files
//#define _LARGEFILE_SOURCE already defined in /usr/include/features.h
#define _FILE_OFFSET_BITS 64

//General C++ include files
#include <iostream>
#include <fstream>
#include <stdlib.h>
using namespace std;

//include files for the application
#include "tracesprovider.h"

#include <qfile.h>
#include <qregexp.h>

TracesProvider::TracesProvider(KURL fileUrl,int nbChannels,int resolution,double samplingRate,int offset):
  DataProvider(fileUrl),nbChannels(nbChannels),resolution(resolution),samplingRate(samplingRate),offset(offset){
  computeRecordingLength();
}

TracesProvider::~TracesProvider(){
}



dataType TracesProvider::getNbSamples(long startTime,long endTime,long startTimeInRecordingUnits){
 //Search what is the number of samples in the given time frame.
 //Convert the time in miliseconds to time in recording units.

 //Convert the time in miliseconds to time in recording units if need it.
 dataType startInRecordingUnits;
 //startTimeInRecordingUnits has been computed in a previous call to a clustersProvider browsing function. It has to be used insted of computing
 //the value from startTime because of the rounding which has been applied to it.
 if(startTimeInRecordingUnits != 0) startInRecordingUnits = startTimeInRecordingUnits;
 else startInRecordingUnits = static_cast<dataType>(startTime * static_cast<double>(static_cast<double>(samplingRate) / static_cast<double>(1000)));
 dataType endInRecordingUnits =  static_cast<dataType>(endTime * static_cast<double>(static_cast<double>(samplingRate) / static_cast<double>(1000)));


 //The caller should have check that we do not go over the end of the file.
 //The recording starts at time equals 0 and ends at length of the file minus one.
 //Therefore if the end time requested equals the length of the file, endInRecordingUnits
 // should be diminish by one sample.
 if(endTime == length) endInRecordingUnits--;

 dataType nbSamples = static_cast<dataType>(endInRecordingUnits - startInRecordingUnits) + 1;

 return nbSamples;
}

void TracesProvider::requestData(long startTime,long endTime,QObject* initiator,long startTimeInRecordingUnits){
  retrieveData(startTime,endTime,initiator,startTimeInRecordingUnits);
}

void TracesProvider::retrieveData(long startTime,long endTime,QObject* initiator,long startTimeInRecordingUnits){
  Array<dataType> data;
 //When the bug in gcc will be corrected for the 64 bits, the c++ code will be use
 //[alex@slut]/home/alex/src/sizetest > ./sizetest-2.95.3
 //  sizeof(std::streamoff) = 8 bytes (64 bits)
 //  sizeof(std::streampos) = 8 bytes (64 bits)

 // [alex@slut]/home/alex/src/sizetest > ./sizetest-3.2.2
 //  sizeof(std::streamoff) = 4 bytes (32 bits)
 //  sizeof(std::streampos) = 12 bytes (96 bits)

/*  ifstream dataFile;

 //Open the file containing the data
 dataFile.open(fileName,ifstream::in|ifstream::binary);
 if(dataFile.fail()){
  //emit the signal with an empty array, the reciever will take care of it, given a message to the user.
  data.setSize(0,0);
  emit dataReady(data,initiator);
  return;
 }*/

 //Search what is the number of samples in the given time frame.

 //Convert the time in miliseconds to time in recording units if need it.
 dataType startInRecordingUnits;
 //startTimeInRecordingUnits has been computed in a previous call to a clustersProvider browsing function. It has to be used insted of computing
 //the value from startTime because of the rounding which has been applied to it.
 if(startTimeInRecordingUnits != 0) startInRecordingUnits = startTimeInRecordingUnits;
 else startInRecordingUnits = static_cast<dataType>(startTime * static_cast<double>(static_cast<double>(samplingRate) / static_cast<double>(1000)));
 dataType endInRecordingUnits =  static_cast<dataType>(endTime * static_cast<double>(static_cast<double>(samplingRate) / static_cast<double>(1000)));

 //The caller should have check that we do not go over the end of the file.
 //The recording starts at time equals 0 and ends at length of the file minus one.
 //Therefore if the end time requested equals the length of the file, endInRecordingUnits
 // should be diminish by one sample.
 if(endTime == length) endInRecordingUnits--;

 dataType nbSamples = static_cast<dataType>(endInRecordingUnits - startInRecordingUnits) + 1;

 //data will contain the final values.
 data.setSize(nbSamples,nbChannels);

 //Depending on the acquisition system resolution, the data are store as short or long
 if((resolution == 12) | (resolution == 14) | (resolution == 16)){
  Array<short> retrieveData(nbSamples,nbChannels);
  off_t nbValues = nbSamples * nbChannels;

	// Is this a Neuralynx file?
	int p = fileName.findRev(".ncs");
	if ( p != -1 )
	{
		/// Modified by M.Zugaro to read Neuralynx ncs format

		// Neuralynx headers
		char			fileHeader[16*1024];
		char			recordHeader[20];
		const int	nSamplesPerRecord = 512;
		int16_t		buffer[nSamplesPerRecord];
		int			recordSize = sizeof(recordHeader)+sizeof(buffer);

		// Determine offset in ncs file
		int64_t	firstRecord = startInRecordingUnits/nSamplesPerRecord;
		int		offsetInFirstRecord = startInRecordingUnits-firstRecord*nSamplesPerRecord;
		off_t		position = (off_t)(sizeof(fileHeader)+firstRecord*recordSize+sizeof(recordHeader)+offsetInFirstRecord*sizeof(int16_t));

		// Data will be read starting somewhere in the first record (not necessarily at the beginning), proceeding with
		// a number of complete records, and ending somwhere in the last record (not necessarily at the end)
		int		inFirstRecord = nSamplesPerRecord-offsetInFirstRecord;
		int64_t	nRecords = (nbSamples-inFirstRecord)/nSamplesPerRecord;
		int		inLastRecord = nbSamples-nRecords*nSamplesPerRecord-inFirstRecord-1;
		off_t		nRead;

		int p = fileName.findRev(".");
		QString baseName = fileName;
		baseName.truncate(p-1);
		p = baseName.findRev(QRegExp("[^0-9]"));
		baseName.truncate(p+1);

		for ( int channel = 1 ; channel <= nbChannels ; ++channel )
		{
			// Open CSC file
			QString cscFileName;
			FILE* dataFile;
			for ( int i = 0 ; i <= 3 ; ++i )
			{
				// Files are numbered 1...N but we do not know if they are zero-padded,
				// so we try different padding lengths (from 0 to 3 digits)
				QString pad;
				for ( int j = 0 ; j < i ; ++j ) pad += "0";
				cscFileName = baseName + pad + QString("%1.ncs").arg(channel);
				dataFile = fopen(cscFileName,"rb");
				if (dataFile != NULL) break;
			}
			if (dataFile == NULL)
			{
				// Emit the signal with an empty array, let the receiver handle the error (user message).
				data.setSize(0,0);
				emit dataReady(data,initiator);
				return;
			}

			// Start at appropriate offset
			fseeko(dataFile,position,SEEK_SET);

			// 1) Read the rest of the first record
			nRead = fread((char*)buffer,sizeof(int16_t),inFirstRecord,dataFile);
			if (nRead != inFirstRecord)
			{
				// Do not report an error, because Neuralynx files do not necessarily all have the same number of
				// records (sometimes the last record is missing...)

				// Emit the signal with an empty array, let the receiver handle the error (user message).
				//data.setSize(0,0);
				//fclose(dataFile);
				//emit dataReady(data,initiator);
				//return;
			}
			for ( int i = 0 ; i < inFirstRecord ; ++i ) retrieveData[i*nbChannels+channel-1] = buffer[i];

			// 2) Read N full records
			for ( int r = 0 ; r < nRecords ; ++r )
			{
				fread(recordHeader,sizeof(recordHeader),1,dataFile); // skip header
				nRead = fread((char*)buffer,sizeof(int16_t),nSamplesPerRecord,dataFile);
				if (nRead != nSamplesPerRecord)
				{
					// Do not report an error, because Neuralynx files do not necessarily all have the same number of
					// records (sometimes the last record is missing...)

					// Emit the signal with an empty array, let the receiver handle the error (user message).
					//data.setSize(0,0);
					//fclose(dataFile);
					//emit dataReady(data,initiator);
					//return;
				}
				for ( int i = 0 ; i < nSamplesPerRecord ; ++i ) retrieveData[(i+inFirstRecord+r*nSamplesPerRecord)*nbChannels+channel-1] = buffer[i];
			}

			// 3) Read the beginning of the last record
			if ( inLastRecord > 0 )
				{
				fread(recordHeader,sizeof(recordHeader),1,dataFile); // skip header
				nRead = fread((char*)buffer,sizeof(int16_t),inLastRecord,dataFile);
				if (nRead != inLastRecord)
				{
					// Do not report an error, because Neuralynx files do not necessarily all have the same number of
					// records (sometimes the last record is missing...)

					// Emit the signal with an empty array, let the receiver handle the error (user message).
					//data.setSize(0,0);
					//fclose(dataFile);
					//emit dataReady(data,initiator);
					//return;
				}
				for ( int i = 0 ; i < inLastRecord ; ++i )  retrieveData[(i+inFirstRecord+nRecords*nSamplesPerRecord)*nbChannels+channel-1] = buffer[i];
			}
			fclose(dataFile);
		}
		/// (end of code modified by M.Zugaro)
	}
	else
	{
		FILE* dataFile = fopen(fileName,"rb");
		if(dataFile == NULL){
			//emit the signal with an empty array, the reciever will take care of it, given a message to the user.
			data.setSize(0,0);
			emit dataReady(data,initiator);
			return;
		}
		//go to the startInRecordingUnits position
		off_t position = static_cast<off_t>(static_cast<off_t>(startInRecordingUnits)* static_cast<off_t>(nbChannels));
		//C++ code:  dataFile.seekg(static_cast<off_t>(position * sizeof(short)),ios::beg);
		fseeko(dataFile,static_cast<off_t>(position * sizeof(short)),SEEK_SET);
		// copy the data into retrieveData.
		//C++ code: dataFile.read((char*)(&(retrieveData[0])),sizeof(short) * nbValues);
		off_t nbRead = fread((char*)(&(retrieveData[0])),sizeof(short),nbValues,dataFile);

		if(nbRead != nbValues){
			//emit the signal with an empty array, the reciever will take care of it, given a message to the user.
			data.setSize(0,0);
			fclose(dataFile);
			emit dataReady(data,initiator);
			return;
  		}
  		fclose(dataFile);
	}
  //Apply the offset if need it,convert to dataType and store the values in data.
  if(offset != 0){
   for(off_t i = 0; i < nbValues; ++i){
    data[i] = static_cast<dataType>(retrieveData[i]) - static_cast<dataType>(offset);
   }
  }
  else{
   for(off_t i = 0; i < nbValues; ++i){
    data[i] = static_cast<dataType>(retrieveData[i]);
   }
  }
 }
 else if(resolution == 32){
  Array<dataType> retrieveData(nbSamples,nbChannels);
  FILE* dataFile = fopen(fileName,"rb");
  if(dataFile == NULL){
   //emit the signal with an empty array, the reciever will take care of it, given a message to the user.
   data.setSize(0,0);
   emit dataReady(data,initiator);
   return;
  }
  //go to the startInRecordingUnits position
  off_t position = static_cast<off_t>(static_cast<off_t>(startInRecordingUnits)* static_cast<off_t>(nbChannels));
  //C++ code: dataFile.seekg(static_cast<off_t>(position * sizeof(dataType)),ios::beg);
  fseeko(dataFile,static_cast<off_t>(position * sizeof(dataType)),SEEK_SET);
  // copy the data into retrieveData.
  off_t nbValues = nbSamples * nbChannels;
  //C++ code: dataFile.read((char*)&(retrieveData[0]),sizeof(dataType) * nbValues);
  off_t nbRead = fread((char*)(&(retrieveData[0])),sizeof(dataType),nbValues,dataFile);

  if(nbRead != nbValues){
   //emit the signal with an empty array, the reciever will take care of it, given a message to the user.
   data.setSize(0,0);
   fclose(dataFile);
   emit dataReady(data,initiator);
   return;
  }

  //Apply the offset if need it and store the values in data.
  if(offset != 0){
   for(off_t i = 0; i < nbValues; ++i)
    data[i] = retrieveData[i] - static_cast<dataType>(offset);
  }
  else{
   for(off_t i = 0; i < nbValues; ++i){
    data[i] = static_cast<dataType>(retrieveData[i]);
   }
  }
  //The data have been retrieve, close the file.
  fclose(dataFile);//dataFile.close();
 }


 //Send the information to the receiver.
 emit dataReady(data,initiator);
}

void TracesProvider::computeRecordingLength(){
 //When the bug in gcc will be corrected for the 64 bits the c++ code will be use
 //[alex@slut]/home/alex/src/sizetest > ./sizetest-2.95.3
 //  sizeof(std::streamoff) = 8 bytes (64 bits)
 //  sizeof(std::streampos) = 8 bytes (64 bits)

 // [alex@slut]/home/alex/src/sizetest > ./sizetest-3.2.2
 //  sizeof(std::streamoff) = 4 bytes (32 bits)
 //  sizeof(std::streampos) = 12 bytes (96 bits)

 /*ifstream dataFile;

 //Open the file containing the data
 dataFile.open(fileName,ifstream::in|ifstream::binary);
 if(dataFile.fail()) return 0;

 // get the length of the file:
 dataFile.seekg (0, ios::end);
 off_t fileLength = dataFile.tellg();
 dataFile.close();*/

 FILE* file = fopen(fileName,"rb");
 if(file == NULL){
  length = 0;
  return;
 }
 // obtain file size.
 fseeko(file,0,SEEK_END);
 off_t fileLength = ftello(file);
 fclose(file);

 int dataSize = 0;
 if((resolution == 12) | (resolution == 14) | (resolution == 16)) dataSize = 2;
 else if(resolution == 32) dataSize = 4;

	// Is this a Neuralynx file?
	int p = fileName.findRev(".ncs");
	if ( p != -1 )
	{
		/// Modified by M.Zugaro to read Neuralynx ncs format

		// Neuralynx headers
		char			fileHeader[16*1024];
		char			recordHeader[20];
		const int	nSamplesPerRecord = 512;
		int			recordSize = sizeof(recordHeader)+nSamplesPerRecord*dataSize;

		// Determine number of complete records in file, + amount of extra data (last record may be incomplete)
		int64_t		nRecords = (fileLength-sizeof(fileHeader))/recordSize;
		int			extraData = (fileLength-sizeof(fileHeader)) - nRecords*recordSize - sizeof(recordHeader);
		if ( extraData < 0 ) extraData = 0;
		int64_t		dataLength = nRecords*nSamplesPerRecord*dataSize+extraData;

		length = static_cast<long long>(
						static_cast<float>(
							dataLength / static_cast<float>(samplingRate * dataSize)  // Only one channel per file!
						) * 1000
				);
		/// (end of code modified by M.Zugaro)
	}
	else
	{
		length = static_cast<long long>(
						static_cast<float>(
							fileLength / static_cast<float>(nbChannels * samplingRate * dataSize)
						) * 1000
				);
	}
}

long TracesProvider::getTotalNbSamples(){
 return static_cast<long>((length * samplingRate) / 1000);
}



#include "tracesprovider.moc"
