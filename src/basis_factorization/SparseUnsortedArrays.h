/*********************                                                        */
/*! \file SparseUnsortedArrays.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

 **/

#ifndef __SparseUnsortedArrays_h__
#define __SparseUnsortedArrays_h__

#include "HashMap.h"
#include "SparseUnsortedArray.h"
#include "SparseUnsortedList.h"

class SparseUnsortedArrays
{
public:
    SparseUnsortedArrays();
    ~SparseUnsortedArrays();

    /*
      Initialize the otherSparse matrix.
    */
    void initializeToEmpty( unsigned m, unsigned n );
    void initialize( const double *M, unsigned m, unsigned n );
    void initialize( const SparseUnsortedArray **V, unsigned m, unsigned n );
    void initialize( const SparseUnsortedList **V, unsigned m, unsigned n );

    /*
      Update a single row from a dense vector
    */
    void updateSingleRow( unsigned row, const double *dense );

    /*
      Obtain a single element/row/column of the matrix.
    */
    double get( unsigned row, unsigned column ) const;
    void set( unsigned row, unsigned column, double value );
    const SparseUnsortedArray *getRow( unsigned row ) const;
    SparseUnsortedArray *getRow( unsigned row );
    void getRowDense( unsigned row, double *result ) const;
    void getColumn( unsigned column, SparseUnsortedArray *result ) const;
    void getColumnDense( unsigned column, double *result ) const;

    /*
      Add a row/column to the end of the matrix.
      The new row/column is provided in dense format.
    */
    void addLastRow( const double *row );
    void addLastColumn( const double *column );

    /*
      This function increments n, the number of columns in the
      matrix. It assumes the new column is all zeroes.
     */
    void addEmptyColumn();

    /*
      Count the number of non-zero elements in each row and column
    */
    void countElements( unsigned *numRowElements, unsigned *numColumnElements );

    /*
      Transpose the matrix and store it in another matrix
    */
    void transposeIntoOther( SparseUnsortedArrays *other );

    /*
      For debugging purposes.
    */
    void dump() const;
    void dumpDense() const;

    /*
      Storing and restoring the otherSparse matrix.
    */
    void storeIntoOther( SparseUnsortedArrays *other ) const;

    /*
      Get the number of non-zero elements
    */
    unsigned getNnz() const;

    /*
      Produce a dense version of the matrix
    */
    void toDense( double *result ) const;

    /*
      Empty the matrix without changing its dimensions,
      or clear a specific row
    */
    void clear();
    void clear( unsigned row );

    /*
      Append an element to one of the lists. Element must be
      non-zero, and must not already exist in that list
    */
    void append( unsigned row, unsigned column, double value );

private:
    // Actual rows
    SparseUnsortedArray **_rows;

    // Number of rows
    unsigned _m;

    // Number of columns
    unsigned _n;

    void freeMemoryIfNeeded();
};

#endif // __SparseUnsortedArrays_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
