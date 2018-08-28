/*********************                                                        */
/*! \file SparseUnsortedLists.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **/

#ifndef __SparseUnsortedLists_h__
#define __SparseUnsortedLists_h__

#include "HashMap.h"
#include "SparseUnsortedList.h"

class SparseUnsortedLists
{
public:
    SparseUnsortedLists();
    ~SparseUnsortedLists();

    /*
      Initialize the sparse matrix.
    */
    void initializeToEmpty( unsigned m, unsigned n );
    void initialize( const double *M, unsigned m, unsigned n );
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
    const SparseUnsortedList *getRow( unsigned row ) const;
    SparseUnsortedList *getRow( unsigned row );
    void getRowDense( unsigned row, double *result ) const;
    void getColumn( unsigned column, SparseUnsortedList *result ) const;
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
      Count the number of elements in each row and column
    */
    void countElements( unsigned *numRowElements, unsigned *numColumnElements );

    /*
      Transpose the matrix and store it in another matrix
    */
    void transposeIntoOther( SparseUnsortedLists *other );

    /*
      For debugging purposes.
    */
    void dump() const;
    void dumpDense() const;

    /*
      Storing and restoring the sparse matrix.
    */
    void storeIntoOther( SparseUnsortedLists *other ) const;

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
    SparseUnsortedList **_rows;

    // Number of rows
    unsigned _m;

    // Number of columns
    unsigned _n;

    void freeMemoryIfNeeded();
};

#endif // __SparseUnsortedLists_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
