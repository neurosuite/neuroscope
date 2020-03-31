
/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "nwbreader.h"

#include <QDebug>

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
        nbChannels = static_cast<int>(lDim1);
        delete dataset;

        offset = 0; // Did not see this field in NWB, so we placed it here

        // Get the sampling rate
        std::string DS = NWB_Locations.getSamplingName();
        HDF5_Utilities.GetAttributeDouble(samplingRate, file, DS, "rate");

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
    // Get the sampling rate
    std::string DS = NWB_Locations.getSamplingName();
    H5::H5File* file = new H5::H5File(hsFileName.c_str(), H5F_ACC_RDONLY);
    HDF5_Utilities.GetAttributeDouble(samplingRate, file, DS, "rate");
    delete file;
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
int NWBReader::ReadVoltageTraces(int *data_out, int iStart, long nLength, int nChannels)
{
    std::string DSN = NWB_Locations.getVoltageDataSetName();
    HDF5_Utilities.Read2DBlockDataNamed(data_out, PredType::NATIVE_INT, iStart, nLength, nChannels, hsFileName, DSN);
    return 0;
}



int NWBReader::ReadVoltageTraces(Array<short> &retrieveData, int iStart, long nLength, int nChannels)
{
    //std::cout << "Start ReadBlockData " << std::endl;
    long nbValues = nLength * nChannels;
    //std::cout << "nbValues  " << nbValues << std::endl;


    int *data_out = new int[static_cast<unsigned long long>(nbValues)];

    ReadVoltageTraces(data_out, iStart, nLength, nChannels);

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



