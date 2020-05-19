
/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#include <stdio.h> /* fabs() */
#include "nwbreader.h"

#include <QDebug>

// getGenericText(QString qsTag, QString defaultTextIn)
// <nwb_dataset_name>       /acquisition/test_ephys_data/data       </nwb_dataset_name>
// ***<nwb_voltage_timestamps> /acquisition/test_ephys_data/timestamps </nwb_voltage_timestamps>
// <nwb_voltage_electrodes> /acquisition/test_ephys_data/electrodes </nwb_voltage_electrodes>

// <nwb_dataset_name>/processing/ecephys/LFP/lfp/data</nwb_dataset_name>
// ***<nwb_sampling_name>/processing/ecephys/LFP/lfp/starting_time</nwb_sampling_name>
// <nwb_voltage_electrodes>/processing/ecephys/LFP/lfp/electrodes</nwb_voltage_electrodes>


VoltageSpecs::VoltageSpecs(int voltageRange, int resolution, int amplification)
{
   this->voltageRange = voltageRange;
   this->resolution = resolution;
   this->amplification = amplification;
}




///
/// \brief NWBReader::bTracesUseTimeStamps checks if timestamps are used.
/// \return true if voltages use time stamps
///
bool NWBReader::bTracesUseTimeStamps()
{
    std::string strTimeStampLocation = NWB_Locations.getGenericText("nwb_voltage_timestamps", "");
    return (strTimeStampLocation.length() > 0)? true: false;
}

///
/// \brief NWBReader::dGetSamplingRateFromTimeStamps estimates sampling rate in Hz based on 1st 2 timestamps
/// \return sampling rate in Hz based on first 2 time stamps
///
double NWBReader::dGetSamplingRateFromTimeStamps()
{
    double dSamplingRate = 0.0;

    std::string strTimeStampLocation = NWB_Locations.getGenericText("nwb_voltage_timestamps", "");
    if (strTimeStampLocation.length() > 0)
    {
        double *voltageTimes = nullptr;
        long lLengthTS;
        HDF5_Utilities.Read1DArray<double>(&voltageTimes, PredType::NATIVE_DOUBLE, lLengthTS, hsFileName, strTimeStampLocation);
        if (lLengthTS > 1)
        {
            double dDiff =  voltageTimes[1] - voltageTimes[0];
            if (dDiff > 0.00000001) {
                dSamplingRate = 1.0 / dDiff;
            }
        }
    }

    return dSamplingRate;
}





NWBReader::NWBReader(std::string hsFileName)
    :NWB_Locations(hsFileName)
{
    this->hsFileName = hsFileName;
    this->HDF5_Utilities = HDF5Utilities();
}

NWBReader::NWBReader()
    :NWB_Locations("")
{
}

///
/// \brief NWBReader::ReadNWBAttribs returns some basic information about our trace data
/// \param nbChannels -How many channels of trace data
/// \param resolution -Number of bits of information in the data
/// \param samplingRate -How many samples per second
/// \param offset -Does the trace start at zero
/// \param length -How many points are in the trace
///
void NWBReader::ReadNWBAttribs(int &nbChannels, int &resolution, double &samplingRate, int &offset, long &length)
{
    try
    {
        std::cout << "Start ReadNWBAttribs " << std::endl;

        // Turn off the auto-printing when failure occurs so that we can handle the errors appropriately
        H5::Exception::dontPrint();

        H5::H5File* file = new H5::H5File(hsFileName.c_str(), H5F_ACC_RDONLY);

        std::string DSN = NWB_Locations.getVoltageDataSetName();
        DataSet *dataset = new DataSet(file->openDataSet(DSN.c_str()));

        long lDim1 = 0;
        H5T_class_t type_class;
        HDF5_Utilities.GetDataSetSizes(length, lDim1, resolution, type_class, dataset);
        // At this point, length is the number of readings
        // !!! We may need to convert this to milliseconds, depending on the sampling rate
        nbChannels = static_cast<int>(lDim1);
        delete dataset;

        offset = 0; // Did not see this field in NWB, so we placed it here

        // Get the sampling rate from either the time stamps or an attribute
        samplingRate = getSamplingRate();
        // !!! Possibly do this in all cases, and not just time stamps
        if (bTracesUseTimeStamps())
        {
            length = static_cast<long>(length * 1000.0 / samplingRate); // Convert to milliseconds in length, assuming that sampling rate is in Hz
        }

        std::cout << "Done ReadNWBAttribs " << std::endl;

        delete file;
    }  // end of try block
    // catch failure caused by the H5File operations
    catch (FileIException error)
    {
        std::cout << "FileIException " << std::endl;
        //error.printError();
        return;
    }
    // catch failure caused by the DataSet operations
    catch (DataSetIException error)
    {
        std::cout << "DataSetIException " << std::endl;
        //error.printError();
        return;
    }
    // catch failure caused by the DataSpace operations
    catch (DataSpaceIException error)
    {
        std::cout << "DataSpaceIException " << std::endl;
        //error.printError();
        return;
    }
    // catch failure caused by the DataSpace operations
    catch (DataTypeIException error)
    {
        std::cout << "DataTypeIException " << std::endl;
        //error.printError();
        return;
    }

}

double NWBReader::getSamplingRate()
{
    double samplingRate = 0.0;

    if (bTracesUseTimeStamps())
    {
        samplingRate = dGetSamplingRateFromTimeStamps();
    }
    else {
        // Get the sampling rate from an attribute
        std::string DS = NWB_Locations.getSamplingName();
        H5::H5File* file = new H5::H5File(hsFileName.c_str(), H5F_ACC_RDONLY);
        HDF5_Utilities.GetAttributeDouble(samplingRate, file, DS, "rate");
        delete file;
    }

    return samplingRate;
}

long NWBReader::GetNWBLength( )
{
    //std::cout << "Start GetNWBLength " << std::endl;
    int nbChannels = 0;
    int resolution = 16;
    double samplingRate = 0;
    int offset = 0;
    long length = 0;

    ReadNWBAttribs(nbChannels, resolution, samplingRate, offset, length);
    //std::cout << "Done GetNWBLength " << std::endl;
    return length;
}



// Read all channels for a given time range
int NWBReader::iReadVoltageTraces(Array<short> &retrieveData, int iStart, long nLength, int nChannels, VoltageSpecs **pVS )
{
    *pVS = nullptr;

    //std::cout << "Start ReadBlockData " << std::endl;
    long nbValues = nLength * nChannels;
    //std::cout << "nbValues  " << nbValues << std::endl;

    std::string DSN = NWB_Locations.getVoltageDataSetName();
    int *data_out = new int[static_cast<unsigned long long>(nbValues)];
    HDF5_Utilities.Read2DBlockDataNamed(data_out, PredType::NATIVE_INT, iStart, nLength, nChannels, hsFileName, DSN);

    long k = 0;
    for (int i = 0; i < nLength; ++i) {
        for (int j = 0; j < nChannels; ++j) {
            retrieveData[k] = static_cast<short>(data_out[k]);
            ++k;
        }
    }

    delete[] data_out;

    //std::cout << "Done ReadBlockData " << std::endl;
    return 0;
}


void NWBReader::ReadBlockData2A(Array<short> &retrieveData, int iStart, long nLength, int nChannels, std::string DSN)
{
    //std::cout << "Start ReadBlockData " << std::endl;
    long nbValues = nLength * nChannels;
    //std::cout << "nbValues  " << nbValues << std::endl;

    // Modified by RHM to read Neurodata Without Borders format
    int *data_out = new int[static_cast<unsigned long long>(nbValues)];

    HDF5_Utilities.Read2DBlockDataNamed<int>(data_out, PredType::NATIVE_INT, iStart, nLength, nChannels, hsFileName, DSN);

    long k = 0;
    for (int i = 0; i < nLength; ++i)
        for (int j = 0; j < nChannels; ++j) {
            //qDebug() << "data_out " << data_out[k] << " " << k << "\n";
            //std::cout << "data_out " << data_out[k] << std::endl;
            // RHM thinks that retrieveData is base-1 of 2D and base-0 for 1D
            retrieveData[k] = static_cast<short>(data_out[k]);
            ++k;
        }
    delete[] data_out;

    //std::cout << "Done ReadBlockData " << std::endl;
}



int NWBReader::getVoltageGroups(Array<short>& indexData, Array<short>& groupData, int channelNb)
{
    //std::cout << "Start ReadBlockData " << std::endl;

    std::string DSNIndex = NWB_Locations.getGenericText("nwb_voltage_electrodes", "/processing/ecephys/LFP/lfp/electrodes");
    std::string DSNGroup = NWB_Locations.getGenericText("nwb_voltage_electrodes_shanks", "general/extracellular_ephys/electrodes/shank_electrode_number");

    ReadBlockData2A(indexData, 0, channelNb, 1, DSNIndex);
    ReadBlockData2A(groupData, 0, channelNb, 1, DSNGroup);

    // Let's hack a possible bug fix to keep from future array overruns
    for (int i=0; i < indexData.nbOfColumns(); ++i)
        if (indexData[i] >= channelNb || indexData[i] < -1)
            indexData[i] = static_cast<short>(channelNb-1);

    for (int i=0; i < groupData.nbOfColumns(); ++i)
        if (groupData[i] >= channelNb || groupData[i] < -1)
            groupData[i] = static_cast<short>(channelNb-1);
    // End of safety hack


    return 0;
}

// An example XML event location is: //<nwb_event_times>/stimulus/presentation/PulseStim_0V_10001ms_LD0/timestamps</nwb_event_times>
NamedArray<double> *  NWBReader::ReadEvents(int iPos)
{
    NamedArray<double> *nad = nullptr;

    //std::string DSN = NWB_Locations.getGenericText("nwb_event_times", "");
    //if (DSN.length() < 1)
    //{
    //    std::cout << "could not find event label in xml file." << std::endl;
    //    return nad;
    //}

    QList<std::string> lstDSNs = NWB_Locations.getListGenericTexts("nwb_event_times", "");
    if (lstDSNs.count() <= iPos)
    {
        std::cout << "could not find event label in xml file." << std::endl;
        return nad;
    }

    try {
        H5::H5File* file = new H5::H5File(hsFileName.c_str(), H5F_ACC_RDONLY);
        nad = ReadOneEvent(lstDSNs[iPos] /*DSN*/, file);
        delete file;
    }
    // catch failure caused by the H5File operations
    catch( FileIException error )
    {
      //error.printError();
      return nullptr;
    }
    // catch failure caused by the DataSet operations
    catch( DataSetIException error )
    {
      //error.printError();
      return nullptr;
    }
    // catch failure caused by the DataSpace operations
    catch( DataSpaceIException error )
    {
      //error.printError();
      return nullptr;
    }
    // catch failure caused by the DataSpace operations
    catch( DataTypeIException error )
    {
      //error.printError();
      return nullptr;
    }

    return nad;
}

NamedArray<double> *  NWBReader::ReadOneEvent(std::string DSN, H5::H5File* file)
{
    NamedArray<double> *nad = nullptr;

    DataSet *dataset = new DataSet(file->openDataSet(DSN.c_str()));

    // It would be easier to simply strip off the timestamps label and take the name for this event.
    // Here is how to get the name from HDF5.
    char sz180[180];
    H5Iget_name(dataset->getId(), sz180, 180);
    std::cout << "data set name: " << sz180 << std::endl;
    // data set name example: /stimulus/presentation/PulseStim_0V_10001ms_LD0/timestamps

    // The event times should be double which is H5T_FLOAT
    H5T_class_t type_class = dataset->getTypeClass();
    //std::cout << type_class << std::endl;

    // Confirm we have the correct datatype and read the data.
    if (type_class == H5T_FLOAT)
    {
        // Get the floating point datatype
        FloatType floattype = dataset->getFloatType();

        // Get the dimension size of each dimension in the dataspace
        DataSpace dataspace = dataset->getSpace();
        hsize_t dims_out[2];
        int ndims = dataspace.getSimpleExtentDims(dims_out, nullptr);
        if (ndims <1)
        {
            std::cout << "NWBReader::ReadOneEvent() number of dimensions error " << std::endl;
            delete dataset;
            return nullptr; //nad;
        }

        long length = static_cast<long>(dims_out[0]);       /*qlonglong*/
        std::cout << " length " << length <<  std::endl;

        double *data_out = new double[static_cast<unsigned long long>(length)];
        HDF5_Utilities.Read2DBlockDataNamed<double>(data_out, PredType::NATIVE_DOUBLE, 0, length, 1, hsFileName, DSN);
        nad  = new NamedArray<double>();
        for (int i=0; i<length; ++i)
        {
            //std::cout << i << " data: " << data_out[i] << std::endl;
            nad->arrayData.append(data_out[i]);
        }
        nad->strName = sz180;
        delete [] data_out;
    }
    delete dataset;

    return nad;
}




QList<NamedArray<double>>  NWBReader::ReadSpikeShank(std::string nwb_spike_times, std::string nwb_spike_times_index, std::string nwb_units_electrode_group)
{
    // Some example XML locations are:
    //nwb_spike_times = "units/spike_times";
    //nwb_spike_times_index = "units/spike_times_index";
    //nwb_units_electrode_group = "units/electrode_group";

    double *spikeTimes = nullptr;
    long lLengthST;
    HDF5_Utilities.Read1DArray<double>(&spikeTimes, PredType::NATIVE_DOUBLE, lLengthST, hsFileName, nwb_spike_times);

    int *spikeIndices = nullptr;
    long lLengthSI;
    HDF5_Utilities.Read1DArray<int>(&spikeIndices, PredType::NATIVE_INT, lLengthSI, hsFileName, nwb_spike_times_index);

    std::string *spikeNames = nullptr;
    long lLengthSN;
    HDF5_Utilities.Read1DArrayStr(&spikeNames, lLengthSN, hsFileName, nwb_units_electrode_group);

    QList<NamedArray<double>> nad;
    for (int idx=0; idx < lLengthSI; ++idx)
    {
        int ndxLower = (idx > 0) ? spikeIndices[idx-1] : 0;
        int ndxUpperP1 = spikeIndices[idx];
        int nLen = ndxUpperP1 - ndxLower;

        NamedArray<double> OneNad;
        for (int ii=0; ii < nLen; ++ii)
        {
            OneNad.arrayData.append(spikeTimes[ndxLower + ii]);
        }
        OneNad.strName = spikeNames[idx];
        nad.append(OneNad);

        // print debugging information
        //std::cout << nad[idx].strName << " ";
        //for (int jj=0; jj < 8; ++jj)
        //        std::cout << nad[idx].arrayData[jj] << " ";
        //std::cout << std::endl;
    }

    if (spikeTimes)
        delete [] spikeTimes;
    if (spikeIndices)
        delete [] spikeIndices;
    if (spikeNames)
        delete [] spikeNames;

    return nad;
}



QList<NamedArray<double>> NWBReader::ReadSpikeShank()
{
    // Read the locations from the XML file
    // Here some examples:
    //<nwb_spike_times_values>units/spike_times</nwb_spike_times_values>
    //<nwb_spike_times_indices>units/spike_times_index</nwb_spike_times_indices>
    //<nwb_spike_times_shanks>units/electrode_group</nwb_spike_times_shanks>

    std::string DS_STV = NWB_Locations.getGenericText("nwb_spike_times_values", "");
    std::string DS_STI = NWB_Locations.getGenericText("nwb_spike_times_indices", "");
    std::string DS_STS = NWB_Locations.getGenericText("nwb_spike_times_shanks", "");

    return ReadSpikeShank(DS_STV, DS_STI, DS_STS);
}







// Read all channels for a given time range
int NWBReader::fReadVoltageTraces(Array<short> &retrieveData, int iStart, long nLength, int nChannels, VoltageSpecs **pVS)
{
    long nbValues = nLength * nChannels;

    double *data_out = new double[static_cast<unsigned long long>(nbValues)];
    std::string DSN = NWB_Locations.getVoltageDataSetName();
    HDF5_Utilities.Read2DBlockDataNamed(data_out, PredType::NATIVE_DOUBLE, iStart, nLength, nChannels, hsFileName, DSN);


    // Find limits of this floating point data
    double dMin = data_out[0];
    double dMax = data_out[0];
    for (long ll = 1; ll < nbValues; ++ll)
    {
        if (data_out[ll] > dMax)
            dMax = data_out[ll];
        if (data_out[ll] < dMin)
            dMin = data_out[ll];
    }
    double dRange = dMax - dMin;
    //double dFact = (fabs(dMax) > fabs(dMin))? fabs(dMax) : fabs(dMin);
    //dFact *= 0.032;
    //if (dFact < 1)
    //    dFact = 1.0;

    *pVS = new VoltageSpecs(10, 16, 1000);
    //VoltageSpecs(int voltageRange, int resolution, int amplification);


    // Convert the floating point data to short integers to make it compatible with Neuroscope
    // !!! See what neuroscope attributes we must update to make clear that we converted. say scale.
    // short integers range from -32,768 to 32,767, so their range is 65535.
    double dFact = (fabs(dRange)< 0.00001) ? 1.0 : 65535.0/dRange;
    double dTemp= 0.0;
    long k = 0;
    for (int i = 0; i < nLength; ++i) {
        for (int j = 0; j < nChannels; ++j) {
            dTemp = -32768.0 + dFact * (data_out[k]-dMin);
            //dTemp = 0.0 + dFact * (data_out[k]-dMin)/2000.0;
            retrieveData[k] = static_cast<short>(dTemp);
            ++k;
        }
    }

    delete[] data_out;

    return 0;
}






int NWBReader::getDataSetDataTypeFromLabel(H5T_class_t &type_class, std::string DSN)
{
    try {
        H5::H5File* file = new H5::H5File(hsFileName.c_str(), H5F_ACC_RDONLY);

        DataSet *dataset = new DataSet(file->openDataSet(DSN.c_str()));
        type_class = dataset->getTypeClass();
        delete dataset;

        delete file;
        return 0;
    }
    catch(...) {
        type_class = H5T_class_t::H5T_NO_CLASS;
        std::cout << " Error determining data set type " << std::endl;
        return 1;
    }
}

int NWBReader::ReadVoltageTraces(Array<short> &retrieveData, int iStart, long nLength, int nChannels, VoltageSpecs **pVS)
{
    H5T_class_t type_class;
    std::string DSN = NWB_Locations.getVoltageDataSetName();
    int iRet = getDataSetDataTypeFromLabel(type_class, DSN);
    if (iRet == 0)
    {
        if (type_class == H5T_INTEGER)
            return iReadVoltageTraces(retrieveData, iStart, nLength, nChannels, pVS);
        else if (type_class == H5T_FLOAT)
            return fReadVoltageTraces(retrieveData, iStart, nLength, nChannels, pVS);
    }
    std::cout << " Error determining data set type for reading voltage traces." << std::endl;
    return 1;
}



