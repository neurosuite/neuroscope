/***************************************************************************
                          array.h  -  description
                             -------------------
    begin                : Mond Dec 29 2003
    copyright            : (C) 2003 by Lynn Hazan
    email                : lynn.hazan@myrealbox.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef ARRAY_H
#define ARRAY_H

//C include files
#include <cstring>

//General C++ include files
#include <iostream>
using namespace std;

/**
  * This Array class is a template class that provides arrays of simple types.
  * Array stores the array elements directly in the array. It can only deal with simple types.
  * Array provides an easy way to acess the elements through the parenthesis operator. 
  *@author Lynn Hazan
  */
  
template <class T>
class Array {
public:
	inline Array(long nbOfRows,long nbOfColumns){
    nbColumns = nbOfColumns;
    nbRows = nbOfRows;
    array = new T[nbRows * nbColumns];
  };
  
  inline Array():nbColumns(0),nbRows(0),array(0L){};
  
 	inline ~Array(){
    delete []array;
  };
  /**
  *
  */
  inline void setSize(long nbOfRows,long nbOfColumns){
    nbColumns = nbOfColumns;
    nbRows = nbOfRows;
    if(array) delete[]array;
    array = new T[nbRows * nbColumns];
  };
  
  /**
    Accessor @p Array(i,j). The rows and columns starts at 1.

    @param      row     index
    @param      column  index
    @return     the element (by value)
  */
  inline T operator()(long row,long column) const{
    return array[(row - 1)*nbColumns + (column - 1)];
  };
  /**
    Mutator @p Array(i,j). The rows and columns starts at 1.

    @param      row      index
    @param      column   index
    @return     the element (by value)
  */
  inline T& operator()(long row,long column){
    return array[(row - 1)*nbColumns + (column - 1)];
  };
  /**
    Return the i element of the internal array by reference.
    The position starts at 0.

    @param      position      index
    @return     the element (by value)
  */
  inline T& operator[](long position){
    return array[position];
  };
  /**
  * Returns the number of rows in the array
  * @return number of rows
  */
  inline long nbOfRows() const{
    return nbRows;
  };
  /**
  * Returns the number of columns in the array
  * @return number of columns
  */
  inline long nbOfColumns() const{
    return nbColumns;
  };


  /**
  * Copies data from @p source to the current object prepending an empty column.
  * @param source Array containing the data to put in the current Array.
  */
  inline void copyAndPrependColumn(Array& source){
    for(long i = 0;i < nbRows;++i)
      memcpy(&array[i * nbColumns + 1],&(source.array[i * source.nbColumns]),
             source.nbColumns * sizeof(T));
  };

  /**
  * Copies a subset of data from @p source to the current object. Copy from the first column
  * to the @p lastColumnToCopy. Assumes that the Array has the correct size.
  * @param source Array containing the data to put in the current Array.  
  * @param lastColumnToCopy last column containing data to copy, start at 1.
  */
  inline void copySubset(Array& source,long lastColumnToCopy){   
    for(long i = 0;i < nbRows;++i)
      memcpy(&array[i * lastColumnToCopy],&(source.array[i * source.nbColumns]),
             lastColumnToCopy * sizeof(T));
  };

  /**
  * Copies a subset of data from @p source to the current object. Copy from @p firstColumnToCopy
  * to the @p lastColumnToCopy inserting them after @p startingColumn. Assumes that the Array has the correct size.
  * @param source Array containing the data to put in the current Array.
  * @param firstColumnToCopy first column containing data to copy, start at 1.
  * @param lastColumnToCopy last column containing data to copy, start at 1.
  * @param startingColumn column from where to start copying to.
  */
  inline void copySubset(Array& source,long firstColumnToCopy,long lastColumnToCopy,long startingColumn){
    long nbColumnsToCopy = lastColumnToCopy - firstColumnToCopy + 1;
    for(long i = 0;i < nbRows;++i)
      memcpy(&array[i * nbColumns + startingColumn - 1],&(source.array[i * source.nbColumns + (firstColumnToCopy - 1)]),
             nbColumnsToCopy * sizeof(T));
  };
  
  /**
  * Fills the array with zeros.
  */
  inline void fillWithZeros(){
   memset(array,0,static_cast<unsigned int>(nbRows * nbColumns) * sizeof(T));   
  };


  /**Overloading of the operator=.*/
  inline Array<T>& operator=(const Array<T>& source){
   if(&source != this){
    nbColumns = source.nbColumns;
    nbRows = source.nbRows;   
    if(array) delete[]array;
    array = new T[nbRows * nbColumns];
    memcpy(array,source.array,nbRows * nbColumns * sizeof(T));
   } 
    return *this;
  };
    
protected:
  /**Number of columns in the array.*/
  long nbColumns;
  /**Number of rows in the array.*/
  long nbRows;
  T* array;

};


/**
  * This pArray class is a template class that provides arrays of classes containing pointers.
  * pArray stores the array elements directly in the array.
  * Array provides an easy way to acess the elements through the parenthesis operator.
  *@author Lynn Hazan
  */
template <class T>
class pArray : public Array<T>{

protected:
  using Array<T>::nbColumns;
  using Array<T>::nbRows;
  using Array<T>::array;

public:
	inline pArray(){};
	inline ~pArray(){};


  /**
  * Copies data from @p source to the current object prepending an empty column.
  * @param source pArray containing the data to put in the current pArray.
  */
  inline void copyAndPrependColumn(pArray& source){
   for(long i = 0;i < nbRows;++i){
    for(int j = 0;j<nbColumns;++j){
     array[i*nbColumns + (j + 1)] = source.array[i*source.nbColumns + j];
    }
   }
  };

  /**
  * Copies a subset of data from @p source to the current object. Copy from the first column
  * to the @p lastColumnToCopy.
  * @param source pArray containing the data to put in the current pArray.
  * @param lastColumnToCopy last column containing data to copy, start at 1.
  */
  inline void copySubset(pArray& source,long lastColumnToCopy){
 for(long i = 0;i < nbRows;++i){
    for(int j = 0;j<lastColumnToCopy;++j){
     array[i*lastColumnToCopy + j] = source.array[i*source.nbColumns + j];
    }
   }
  };

  /**
  * Copies a subset of data from @p source to the current object. Copy from @p firstColumnToCopy
  * to the @p lastColumnToCopy inserting them after @p startingColumn. Assumes that the Array has the correct size.
  * @param source pArray containing the data to put in the current Array.
  * @param firstColumnToCopy first column containing data to copy, start at 1.
  * @param lastColumnToCopy last column containing data to copy, start at 1.
  * @param startingColumn column from where to start copying to.
  */
  inline void copySubset(pArray& source,long firstColumnToCopy,long lastColumnToCopy,long startingColumn){
   long nbColumnsToCopy = lastColumnToCopy - firstColumnToCopy + 1;
    for(long i = 0;i < nbRows;++i){
    for(int j = 0;j< nbColumnsToCopy;++j){
     array[i*nbColumns + (startingColumn - 1) + j] = source.array[i*source.nbColumns + (firstColumnToCopy - 1) + j];
    }
   }
  };
  
   
  inline pArray<T>& operator=(pArray<T>& source){
    nbColumns = source.nbColumns;
    nbRows = source.nbRows;
    if(array) delete[]array;
    array = new T[nbRows * nbColumns];
    for(int i = 0;i<nbRows;++i){
      for(int j = 0;j<nbColumns;++j){
      array[i*nbColumns + j] = source.array[i*nbColumns + j];
     } 
    }
    return *this;
  };

  inline pArray<T> operator=(pArray<T>& source) const{
    nbColumns = source.nbColumns;
    nbRows = source.nbRows;
    if(array) delete[]array;
    array = new T[nbRows * nbColumns];
    for(int i = 0;i<nbRows;++i){
     for(int j = 0;j<nbColumns;++j){
      array[i*nbColumns + j] = source.array[i*nbColumns + j];
     }
    }
    return *this;
  };
};

#endif
