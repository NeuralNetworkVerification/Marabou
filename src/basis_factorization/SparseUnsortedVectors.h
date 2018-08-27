/*********************                                                        */
/*! \file SparseUnsortedVectors.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **/

#ifndef __SparseUnsortedVectors_h__
#define __SparseUnsortedVectors_h__

#include "HashMap.h"
#include "SparseMatrix.h"
#include "SparseUnsortedVector.h"

class SparseUnsortedVectors : public SparseMatrix
{
public:
    SparseUnsortedVectors();
    ~SparseUnsortedVectors();

    /*
      Initialize the sparse matrix from a given dense matrix
      M of dimensions m x n, or an empty matrix
    */
    void initialize( const double *M, unsigned m, unsigned n );
    void initializeToEmpty( unsigned m, unsigned n );
    void initialize( const SparseUnsortedVector **V, unsigned m, unsigned n );

    /*
      Obtain a single element/row/column of the matrix.
    */
    double get( unsigned row, unsigned column ) const;
    void set( unsigned row, unsigned column, double value );
    void getRow( unsigned row, SparseUnsortedVector *result ) const;
    const SparseUnsortedVector *getRow( unsigned row ) const;
    void getRowDense( unsigned row, double *result ) const;
    void getColumn( unsigned column, SparseUnsortedVector *result ) const;
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
      A mechanism for storing a set of changes to the matrix,
      and then executing them all at once to reduce overhead
    */
    void commitChange( unsigned row, unsigned column, double newValue );
    void executeChanges();

    /*
      Count the number of elements in each row and column
    */
    void countElements( unsigned *numRowElements, unsigned *numColumnElements );

    /*
      Transpose the matrix and store it in another matrix
    */
    void transposeIntoOther( SparseMatrix *other );

    /*
      For debugging purposes.
    */
    void dump() const;
    void dumpDense() const;

    /*
      Storing and restoring the sparse matrix. This assumes both
      matrices are of the same child class.
    */
    void storeIntoOther( SparseMatrix *other ) const;

    /*
      Merge column x2 into column x1, and zero x2 out
    */
    void mergeColumns( unsigned x1, unsigned x2 );

    /*
      Get the number of non-zero elements
    */
    unsigned getNnz() const;

    /*
      Produce a dense version of the matrix
    */
    void toDense( double *result ) const;

    /*
      Empty the matrix without changing its dimensions
    */
    void clear();

private:
    // Actual rows
    SparseUnsortedVector **_rows;

    // Number of rows
    unsigned _m;

    // Number of columns
    unsigned _n;

    void freeMemoryIfNeeded();
};

#endif // __SparseUnsortedVectors_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
