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
        nbChannels = lDim1;
        delete dataset;

        offset = 0; // Did not see this field in NWB

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
    std::cout << "Start GetNWBLength " << std::endl;
    int nbChannels = 0;
    int resolution = 16;
    double samplingRate = 0;
    int offset = 0;
    long length = 0;

    ReadNWBAttribs(nbChannels, resolution, samplingRate, offset, length);
    std::cout << "Done GetNWBLength " << std::endl;
    return length;
}


// Read all channels for a given time range
int NWBReader::ReadVoltageTraces(int *data_out, int iStart, long nLength, int nChannels)
{
    // !!! Put these offsets back
    //if (iStart > 0)
     //       iStart = 0;
    hsize_t startOffsets[2]{static_cast<unsigned long>(iStart),0};   // Start offsets of hyperslab row, column
    hsize_t count[2]{static_cast<unsigned long>(nLength), static_cast<unsigned long>(nChannels) };  // Block count

    std::string DSN = NWB_Locations.getVoltageDataSetName();
// Exception here
    try
    {
        HDF5_Utilities.ReadHyperSlab<int>(data_out, PredType::NATIVE_INT, startOffsets, count, hsFileName, DSN);
    }
    catch (int e)
    {
        cout << "An exception occurred. (ReadVoltageTraces) Exception Nr. " << e << '\n';
    }

    return 0;
}


int NWBReader::ReadVoltageTraces(Array<short> &retrieveData, int iStart, long nLength, int nChannels)
{
    std::cout << "Start ReadBlockData " << std::endl;
    long nbValues = nLength * nChannels;
    std::cout << "nbValues  " << nbValues << std::endl;


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

    std::cout << "Done ReadBlockData " << std::endl;
    return 0;
}


void NWBReader::ReadBlockData2A(Array<short> &retrieveData, int iStart, long nLength, int nChannels, std::string DSN)
{
    std::cout << "Start ReadBlockData " << std::endl;
    long nbValues = nLength * nChannels;
    std::cout << "nbValues  " << nbValues << std::endl;

    // Modified by RHM to read Neurodata Without Borders format
    int *data_out = new int[static_cast<unsigned long long>(nbValues)];

    HDF5_Utilities.ReadBlockDataNamed<int>(data_out, PredType::NATIVE_INT, iStart, nLength, nChannels, hsFileName, DSN);

    long k = 0;
    for (int i = 0; i < nLength; ++i)
        for (int j = 0; j < nChannels; ++j) {
            qDebug() << "data_out " << data_out[k] << " " << k << "\n";
            //std::cout << "data_out " << data_out[k] << std::endl;
            // RHM thinks that retrieveData is base-1 of 2D and base-0 for 1D
            retrieveData[k] = static_cast<short>(data_out[k]);
            ++k;
        }
    delete[] data_out;

    std::cout << "Done ReadBlockData " << std::endl;
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


NamedArray<double> *  NWBReader::ReadEvents()
{
    NamedArray<double> *nad = nullptr;

    //double dS = getSamplingRate();

    //<nwb_event_times>/stimulus/presentation/PulseStim_0V_10001ms_LD0/timestamps</nwb_event_times>
    std::string DSN = NWB_Locations.getGenericText("nwb_event_times", "");
    if (DSN.length() < 1)
    {
        std::cout << "could not find event label in xml file." << std::endl;
        return nad;
    }


    // !!! Look at EventsProvider.cpp line 58 for an entry point to mimic for integrating this function.
try {
    H5::H5File* file = new H5::H5File(hsFileName.c_str(), H5F_ACC_RDONLY);

    DataSet *dataset = new DataSet(file->openDataSet(DSN.c_str()));


    //H5File file = H5File(hsFileName.c_str(), H5F_ACC_RDONLY);
    //DataSet dataset = file.openDataSet(DSN.c_str());


    // It would be easier to simply strip off the timestamps label and take the name for this event.
    // Here is how to get the name from HDF5.
    char sz180[180];
    H5Iget_name(dataset->getId(), sz180, 180);
    std::cout << "data set name: " << sz180 << std::endl;
    // data set name: /stimulus/presentation/PulseStim_0V_10001ms_LD0/timestamps



    // Should be double which is H5T_FLOAT
    H5T_class_t type_class = dataset->getTypeClass();
    std::cout << type_class << std::endl;



    // Get class of datatype and print message if it's an integer.
    if (type_class == H5T_FLOAT)
    {
        // Get the floating point datatype
        FloatType floattype = dataset->getFloatType(); // .getIntType();

        // Get order of datatype and print message if it's a little endian.
        //H5std_string order_string;
        //H5T_order_t order = floattype.getOrder(order_string);

        // Get size of the data element stored in file and print it.
        size_t size = floattype.getSize();

        DataSpace dataspace = dataset->getSpace();

        // Get the number of dimensions in the dataspace.
        //int rank = dataspace.getSimpleExtentNdims();

        // Get the dimension size of each dimension in the dataspace
        hsize_t dims_out[2];
        int ndims = dataspace.getSimpleExtentDims(dims_out, nullptr);

        int nbChannels = static_cast<int>(dims_out[1]);
        long length = static_cast<long>(dims_out[0]);       /*qlonglong*/
        int resolution = static_cast<int>(8 * size);

        std::cout << "nbChannels " << nbChannels << " length " << length << " resolution " << resolution << std::endl;

        double *data_out = new double[static_cast<unsigned long long>(length)];
        HDF5_Utilities.ReadBlockDataNamed<double>(data_out, PredType::NATIVE_DOUBLE, 0, length, 1, hsFileName, DSN);
        nad  = new NamedArray<double>();
        for (int i=0; i<length; ++i)
        {
            //std::cout << i << " data: " << data_out[i] << std::endl;
            nad->arrayData.append(data_out[i]);
        }
        nad->strName = sz180;

        delete [] data_out;

        // !!!! do we delete or close the file and link handles?
        // !!!! Do we have links to better labels?

        delete dataset;
        delete file;
    }
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



QList<NamedArray<double>>  NWBReader::ReadSpikeShank(std::string nwb_spike_times, std::string nwb_spike_times_index, std::string nwb_units_electrode_group)
{
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

        // !!! RHM, check if the copy is deep enough
        nad.append(OneNad);


        // print debugging information
        std::cout << nad[idx].strName << " ";
        for (int jj=0; jj < 8; ++jj)
                std::cout << nad[idx].arrayData[jj] << " ";
        std::cout << std::endl;

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
    //<nwb_spike_times_values>units/spike_times</nwb_spike_times_values>
    //<nwb_spike_times_indices>units/spike_times_index</nwb_spike_times_indices>
    //<nwb_spike_times_shanks>units/electrode_group</nwb_spike_times_shanks>

    std::string DS_STV = NWB_Locations.getGenericText("nwb_spike_times_values", "");
    std::string DS_STI = NWB_Locations.getGenericText("nwb_spike_times_indices", "");
    std::string DS_STS = NWB_Locations.getGenericText("nwb_spike_times_shanks", "");

    return ReadSpikeShank(DS_STV, DS_STI, DS_STS);
}



