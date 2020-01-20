#include "hdf5utilities.h"



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





int HDF5Utilities::Read1DArrayStr(std::string **data_out, long &lLength, std::string hsFileName, std::string DSN)
{
    H5File file = H5File(hsFileName.c_str(), H5F_ACC_RDONLY);
    DataSet dataset = file.openDataSet(DSN.c_str());

    // Should be H5T_REFERENCE
    H5T_class_t type_class = dataset.getTypeClass();
    cout << type_class << endl;

    DataSpace dataspace = dataset.getSpace();


    // Get the dimension size of each dimension in the dataspace
    hsize_t dims_out[2];
    int ndims = dataspace.getSimpleExtentDims(dims_out, nullptr);
    if (ndims < 1)
        cout << "number of dimension is too low in Read1DArrayStr()" << endl;
    lLength = static_cast<long>(dims_out[0]);       /*qlonglong*/

    *data_out = new std::string[static_cast<unsigned long long>(lLength)];

    // Get class of datatype and print message if it's a reference.
    if (type_class == H5T_REFERENCE)
    {
        hobj_ref_t* rbuf = new hobj_ref_t[static_cast<unsigned long long>(lLength)];

        try {
            // Read selection from disk
            dataset.read(rbuf, PredType::STD_REF_OBJ);
        }
        catch (...) {
            cout << "exception in Read1DArrayStr";
        }

        char sz180[180];
        for (int ii=0; ii< lLength; ++ii)
        {
            H5Rget_name(file.getLocId(), H5R_OBJECT, &rbuf[ii], sz180, 180);
            (*data_out)[ii] = sz180;
            cout << sz180 << endl;
        }
    }
    return 0;
}

int HDF5Utilities::GetDataSetSizes(long &lDim0, long &lDim1, int &resolution, H5T_class_t &type_class, DataSet *dataset)
{

    type_class = dataset->getTypeClass();
    //cout << type_class << endl;


    // Get size of the data element stored in file.
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



    DataSpace dataspace = dataset->getSpace();

    // Get the dimension size of each dimension in the dataspace
    int rank = dataspace.getSimpleExtentNdims();
    cout << "rank of dataspace: " << rank  << endl;
    hsize_t dims_out[2];
    int ndims = dataspace.getSimpleExtentDims(dims_out, nullptr);
    if (ndims < 2)
        cout << "number of dimension is too low in GetDataSetSizes()" << endl;


    lDim0 = static_cast<long>(dims_out[0]);   //length    /*qlonglong*/
    lDim1 = static_cast<long>(dims_out[1]); //nbChannels


    return 0;
}



int HDF5Utilities::GetDataSetSizes(long &lDim0, long &lDim1, int &resolution, H5T_class_t &type_class, H5::H5File* file, std::string DSN)
{
    DataSet *dataset = new DataSet(file->openDataSet(DSN.c_str()));
    GetDataSetSizes(lDim0, lDim1, resolution, type_class, dataset);
    delete dataset;
    return 0;
}



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






