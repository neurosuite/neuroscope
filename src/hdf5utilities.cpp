/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "hdf5utilities.h"


///
/// \brief HDF5Utilities::bTypesMatch returns true if the calling pType matches the dataset type_class.
/// For example, if the user wants a NATIVE_INT, but the dataset holds H5T_FLOAT, the function returns false.
/// \param pType: the desired type to read.
/// \param type_class: the type found in the dataset.
/// \return true if the types match and false if they do not.
///
bool HDF5Utilities::bTypesMatch(PredType pType, H5T_class_t type_class)
{
    if (pType == PredType::NATIVE_INT)
    {
        return (type_class == H5T_INTEGER);
    }
    else if (pType == PredType::NATIVE_FLOAT)
    {
        return (type_class == H5T_FLOAT);
    }
    else if (pType == PredType::NATIVE_DOUBLE)
    {
        return (type_class == H5T_FLOAT); // Is this correct? !!!! RHM
    }
    else if (pType == PredType::STD_REF_OBJ)
    {
        return (type_class == H5T_REFERENCE);
    }
    else {
        return false; // Programmer has not yet mapped this case
    }
}




///
/// \brief HDF5Utilities::Read1DArrayStr read an array of string (character) data
/// \param data_out: the string array that is allocated, filled, and returned.
/// \param lLength: length of the string array.
/// \param hsFileName: name of the HDF5 NWB file
/// \param DSN: location within the file to read the string array
/// \return 0 on success and non-zero on error
///
int HDF5Utilities::Read1DArrayStr(std::string **data_out, long &lLength, std::string hsFileName, std::string DSN)
{
    H5File file = H5File(hsFileName.c_str(), H5F_ACC_RDONLY);
    DataSet dataset = file.openDataSet(DSN.c_str()); 
    DataSpace dataspace = dataset.getSpace();

    // Get the dimension size of each dimension in the dataspace
    hsize_t dims_out[2];
    int ndims = dataspace.getSimpleExtentDims(dims_out, nullptr);
    if (ndims < 1)
    {
        cout << "number of dimension is too low in Read1DArrayStr()" << endl;
        lLength = 0;
        return 2; // wrong number of dimensions
    }
    lLength = static_cast<long>(dims_out[0]);

    *data_out = new std::string[static_cast<unsigned long long>(lLength)];

    // Get class of datatype that should be H5T_REFERENCE
    H5T_class_t type_class = dataset.getTypeClass();
    //cout << type_class << endl;
    if (type_class == H5T_REFERENCE)
    {
        hobj_ref_t* rbuf = new hobj_ref_t[static_cast<unsigned long long>(lLength)];

        try {
            // Read selection from disk
            dataset.read(rbuf, PredType::STD_REF_OBJ);
        }
        catch (...) {
            cout << "exception in Read1DArrayStr";
            return 3;
        }

        char sz180[180];
        for (int ii=0; ii< lLength; ++ii)
        {
            H5Rget_name(file.getLocId(), H5R_OBJECT, &rbuf[ii], sz180, 180);
            (*data_out)[ii] = sz180;
            //cout << sz180 << endl;
        }
        return 0;
    }
    else {
        lLength = 0;
        return 1; // wrong type in file
    }

}

///
/// \brief HDF5Utilities::GetDataSetTypeNResolution :Get size of the data element stored in file and use it to guess the resolution.
/// \param resolution
/// \param type_class
/// \param dataset
/// \return
///
int HDF5Utilities::GetDataSetTypeNResolution(int &resolution, H5T_class_t &type_class, DataSet *dataset)
{
    // Get size of the data element stored in file and use it to guess the resolution.
    type_class = dataset->getTypeClass();
    //cout << type_class << endl;
    if (type_class == H5T_INTEGER)
    {
        IntType intype = dataset->getIntType();
        size_t size = intype.getSize();
        resolution = static_cast<int>(8 * size);
    }
    else if (type_class == H5T_FLOAT)
    {
        FloatType floattype = dataset->getFloatType();
        size_t size = floattype.getSize();
        resolution = static_cast<int>(8 * size);
    }
    else if (type_class == H5T_REFERENCE)
    {
        // Not sure if we would use the size of a reference type, so zero it.
        resolution = 0;
    }
    else {
        cout << "programmer needs to handle more types in GetDataSetSizes()" << endl;
        resolution = 0;
    }

    return 0;
}

///
/// \brief HDF5Utilities::GetDataSet2DSizes: Get the # rows and columns of a 2D data set.
/// \param lDim0
/// \param lDim1
/// \param dataset
/// \return
///
int HDF5Utilities::GetDataSet2DSizes(long &lDim0, long &lDim1, DataSet *dataset)
{
    DataSpace dataspace = dataset->getSpace();

    // Get the dimension size of each dimension in the dataspace
    //int rank = dataspace.getSimpleExtentNdims();
    //cout << "rank of dataspace: " << rank  << endl;
    hsize_t dims_out[2];
    int ndims = dataspace.getSimpleExtentDims(dims_out, nullptr);
    if (ndims < 2)
    {
        cout << "number of dimension is too low in GetDataSetSizes()" << endl;
        lDim0 = lDim1 = 0;
        return 1;
    }
    lDim0 = static_cast<long>(dims_out[0]);  // length
    lDim1 = static_cast<long>(dims_out[1]);  // nbChannels

    return 0;
}

///
/// \brief HDF5Utilities::GetDataSetSizes: read the size and resolution of the 2D data set.
/// \param lDim0
/// \param lDim1
/// \param resolution
/// \param type_class
/// \param dataset
/// \return o on success and non-zero on failure
///
int HDF5Utilities::GetDataSetSizes(long &lDim0, long &lDim1, int &resolution, H5T_class_t &type_class, DataSet *dataset)
{
    // Get size of the data element stored in file and use it to guess the resolution.
    GetDataSetTypeNResolution(resolution, type_class, dataset);

    // Get the dimension size of each dimension in the dataspace
    GetDataSet2DSizes(lDim0, lDim1, dataset);

    return 0;
}


///
/// \brief HDF5Utilities::GetDataSetSizes: read the size and resolution of the 2D data set within a file at a location
/// \param lDim0
/// \param lDim1
/// \param resolution
/// \param type_class
/// \param file: the name of the file to read
/// \param DSN: the location within the file to read a dataset.
/// \return zero on success and non-zero otherwise.
///
int HDF5Utilities::GetDataSetSizes(long &lDim0, long &lDim1, int &resolution, H5T_class_t &type_class, H5::H5File* file, std::string DSN)
{
    DataSet *dataset = new DataSet(file->openDataSet(DSN.c_str()));
    int iRet = GetDataSetSizes(lDim0, lDim1, resolution, type_class, dataset);
    delete dataset;
    return iRet;
}


///
/// \brief HDF5Utilities::GetAttributeDouble Helper function to read in a double precision number from a attribute label.
/// \param dAttr : the double precision answer
/// \param file: the name of the file to read
/// \param DSN: the location within the file to for the dataset
/// \param attrName: the attribite or label to read
/// \return 0 on success.
///
int HDF5Utilities::GetAttributeDouble(double &dAttr, H5::H5File* file, std::string DSN, std::string attrName)
{
    DataSet *startSet = new DataSet(file->openDataSet(DSN.c_str()));
    Attribute attr = startSet->openAttribute(attrName);
    DataType dtype = attr.getDataType();
    //std::cout << "attribute type " << dtype << std::endl;
    attr.read(dtype, &dAttr);
    delete startSet;

    return 0;
}






