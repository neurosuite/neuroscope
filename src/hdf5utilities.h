#ifndef HDF5UTILITIES_H
#define HDF5UTILITIES_H

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifdef OLD_HEADER_FILENAME
#include <iostream.h>
#else
#include <iostream>
#endif
using std::cout;
using std::endl;
#include <string>
#include "H5Cpp.h"
#include "hdf5.h"
#include "H5File.h"
using namespace H5;


///
/// \brief The HDF5Utilities class provides a few convenient functions to read data from an HDF5 file.
///
class HDF5Utilities
{
public:

    bool bTypesMatch(PredType pType, H5T_class_t type_class);

    int Read1DArrayStr(std::string **data_out, long &lLength, std::string hsFileName, std::string DSN);

    int GetDataSet2DSizes(long &lDim0, long &lDim1, DataSet *dataset);
    int GetDataSetTypeNResolution(int &resolution, H5T_class_t &type_class, DataSet *dataset);
    int GetDataSetSizes(long &lDim0, long &lDim1, int &resolution, H5T_class_t &type_class, DataSet *dataset);
    int GetDataSetSizes(long &lDim0, long &lDim1, int &resolution, H5T_class_t &type_class, H5::H5File* file, std::string DSN);

    int GetAttributeDouble(double &dAttr, H5::H5File* file, std::string DSN, std::string attrName);

    // General HDF5 function to read a multi-dimensional hyperslab
    // data_out holds the read data.
    // pType is the data type, such as some form of integer or floating point.
    // startOffsets is the beginning point of the data.
    // count is the dimensions of the data to be read.
    // hsFileName is the name of the HDF5 file
    // DSN is the location of the dataset.
    template <typename T>
    int ReadHyperSlab(T *data_out, PredType pType, hsize_t *startOffsets, hsize_t *count, std::string hsFileName, std::string DSN )
    {
        try
        {
            //std::cout << "ReadHyperSlab " << std::endl;

            //Turn off the auto-printing when failure occurs so that we can handle the errors appropriately
            H5::Exception::dontPrint();


            // The stride array allows you to sample elements along a dimension.For example, a stride of one(or NULL) will select every element
            //     along a dimension, a stride of two will select every other element, and a stride of three will select an element after every
            //     two elements.
            //The block array determines the size of the element block selected from a dataspace.If the block size is one or NULL then the block
            //     size is a single element in that dimension.
            hsize_t stride[2]{ 1,1 };  // Stride of hyperslab
            hsize_t block[2]{ 1,1 };  // Block sizes

            // Open the file and dataset and get the dataspace.
            H5File* file = new H5File(hsFileName.c_str(), H5F_ACC_RDONLY);
            DataSet* dataset = new DataSet(file->openDataSet(DSN.c_str()));

            DataSpace fspace = dataset->getSpace();
            fspace.selectHyperslab(H5S_SELECT_SET, count, startOffsets, stride, block);

            // Create memory dataspace.
            //Select hyperslab in memory. Hyperslab has the same size and shape as the selected hyperslab for the file dataspace.
            DataSpace mspace(2, count);
            hsize_t startMemOffsets[2]{0,0};   // Start offsets of hyperslab row, column
            mspace.selectHyperslab(H5S_SELECT_SET, count, startMemOffsets, stride, block);

            // Read data back to the buffer matrix.
            dataset->read(data_out, pType, mspace, fspace);

            // Close the dataset and the file.
            delete dataset;
            delete file;
            //std::cout << "Done ReadHyperSlab " << std::endl;

        }  // end of try block
        // catch failure caused by the H5File operations
        catch (FileIException error)
        {
            std::cout << "FileIException " << std::endl;
            //error.printError();
            return -1;
        }
        // catch failure caused by the DataSet operations
        catch (DataSetIException error)
        {
            std::cout << "DataSetIException " << std::endl;
            //error.printError();
            return -1;
        }
        // catch failure caused by the DataSpace operations
        catch (DataSpaceIException error)
        {
            std::cout << "DataSpaceIException " << std::endl;
            //error.printError();
            return -1;
        }
        // catch failure caused by the DataSpace operations
        catch (DataTypeIException error)
        {
            std::cout << "DataTypeIException " << std::endl;
            //error.printError();
            return -1;
        }
        catch (...)
        {
            std::cout << " RHS exception error RHS " << std::endl;
            //error.printError();
            return -1;
        }
        return 0;
    }

    // Function to read a 2D block of data from a HDF5 file at a named location.
    //                    1D if nChannels = 1.
    // There is a slight convenience with using this function.
    // For trace voltages, for example, we may want to read in all the channels, but look at 1250 points at one time.
    //     Thus, we use a starting offset and a length of 1250, but the channels will be zero to nChannels-1.
    //
    // data_out holds the data that was read from the file.
    // pType is the type of data we want, say some form of integer.
    // iStart is an index of when to start grabbing data.
    // nLength tells us how many data points to read, say 1250.
    // nChannels is the number of channels we collected, say 64.
    // hsFileName is the name of the HDF5 file.
    // DSN is the location within the file that holds teh dataset.
    template <typename T>
    void Read2DBlockDataNamed(T *data_out, PredType pType, int iStart, long nLength, int nChannels, std::string hsFileName, std::string DSN)
    {
        hsize_t startOffsets[2]{ static_cast<unsigned long> (iStart), 0 };   // Start offsets of hyperslab row, column
        hsize_t count[2]{ static_cast<unsigned long> (nLength), static_cast<unsigned long>(nChannels) };  // Block count
        ReadHyperSlab<T>(data_out, pType, startOffsets, count, hsFileName, DSN);
    }

    // Helper function to read in a 1D array.
    // The array memory (data_out) is allocated and the lLength is filled in with the length of the data.
    // pType is the type of data that we expect from the file, say some type of integer.
    // hsFileName is the name of the HDF5 file.
    // DSN is the location within the file where the dataset resides.
    template <typename T>
    int Read1DArray(T **data_out, PredType pType, long &lLength, std::string hsFileName, std::string DSN)
    {
        H5File file = H5File(hsFileName.c_str(), H5F_ACC_RDONLY);
        DataSet dataset = file.openDataSet(DSN.c_str());

        // Make sure we have the correct data type
        H5T_class_t type_class = dataset.getTypeClass();
        if (!bTypesMatch( pType, type_class))
        {
            std::cout << "type classes do not match in Read1DArray" << type_class << std::endl;
            lLength=0;
            return 1;
        }

        DataSpace dataspace = dataset.getSpace();

        // Get the dimension size of each dimension in the dataspace
        hsize_t dims_out[2];
        int ndims = dataspace.getSimpleExtentDims(dims_out, nullptr );
        if (ndims < 1)
        {
            lLength=0;
            return 1;
        }
        lLength = static_cast<long>(dims_out[0]);

        *data_out = new T[lLength];
        Read2DBlockDataNamed<T>(*data_out, pType, 0, lLength, 1, hsFileName, DSN);

        // Print the 1D array for debugging
        //for (int i=0; i<10 /*lLength*/; ++i)
        //{
        //  std::cout << i << " data: " << (*data_out)[i] << std::endl;
        //}

        return 0;
    }

};


#endif // HDF5UTILITIES_H
