#ifndef NWBREADER_H
#define NWBREADER_H

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#include "nwblocations.h"
#include "hdf5utilities.h"

// Helper class to attach a label to a list/array of information.
template <typename T>
class NamedArray {
public:
    NamedArray(){}
    std::string strName;
    QList<T> arrayData;
    int length() {return arrayData.length();}
};

class VoltageSpecs {
public:
    int voltageRange;
    int resolution;
    int amplification;
    VoltageSpecs(int voltageRange, int resolution, int amplification);
};

///
/// \brief The NWBReader class ties together other classes so that NWB data can be sent to Neuroscope.
/// The HDF5 file details are handled using HDF5Utilities that knows little or nothing about Neuroscope.
/// The data locations within HDF5 files are handled using class: NWBLocations.
/// Thus, NWBReader is the glue between Neuroscope and HDF5.
///
class NWBReader
{
public:
    NWBReader(std::string hsFileName);

    void ReadNWBAttribs(int &nbChannels, int &resolution, double &samplingRate, int &offset, long &length);
    long GetNWBLength();

    int ReadVoltageTraces(Array<short> &retrieveData, int iStart, long nLength, int nChannels, VoltageSpecs **pVS);

    int getVoltageGroups(Array<short>& indexData, Array<short>& groupData, int channelNb);

    QList<NamedArray<double>> ReadSpikeShank(std::string nwb_spike_times, std::string nwb_spike_times_index, std::string nwb_units_electrode_group);
    QList<NamedArray<double>> ReadSpikeShank();

    NamedArray<double> *  ReadEvents(int iPos);

    void ReadBlockData2A(Array<short> &retrieveData, int iStart, long nLength, int nChannels, std::string DSN);

    double getSamplingRate();
    bool bTracesUseTimeStamps();
    double dGetSamplingRateFromTimeStamps();

private:
    NWBReader(); // defensive move
    std::string hsFileName;
    NWBLocations NWB_Locations;
    HDF5Utilities HDF5_Utilities;

    //int ReadVoltageTraces(int *data_out, int iStart, long nLength, int nChannels);
    NamedArray<double> *  ReadOneEvent(std::string DSN, H5::H5File* file);

    int getDataSetDataTypeFromLabel(H5T_class_t &type_class, std::string DSN);
    int iReadVoltageTraces(Array<short> &retrieveData, int iStart, long nLength, int nChannels, VoltageSpecs **pVS);
    int fReadVoltageTraces(Array<short> &retrieveData, int iStart, long nLength, int nChannels, VoltageSpecs **pVS);
};


#endif // NWBREADER_H
