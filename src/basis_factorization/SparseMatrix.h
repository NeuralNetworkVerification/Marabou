/*********************                                                        */
/*! \file SparseMatrix.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **/

#ifndef __SparseMatrix_h__
#define __SparseMatrix_h__

#include "SparseVector.h"

class SparseMatrix
{
public:
    virtual ~SparseMatrix() {}

    /*
      Initialize the sparse matrix from a given dense matrix
      M of dimensions m x n.
    */
    virtual void initialize( const double *M, unsigned m, unsigned n ) = 0;

    /*
      Obtain a single element/row/column of the matrix.
    */
    virtual double get( unsigned row, unsigned column ) const = 0;
    virtual void getRow( unsigned row, SparseVector *result ) const = 0;
    virtual void getColumn( unsigned column, SparseVector *result ) const = 0;

    /*
      Add a row/column to the end of the matrix.
      The new row/column is provided in dense format.
    */
    virtual void addLastRow( double *row ) = 0;
    virtual void addLastColumn( double *column ) = 0;

    /*
      This function increments n, the number of columns in the
      matrix. It assumes the new column is all zeroes.
     */
    virtual void addEmptyColumn() = 0;

    /*
      For debugging purposes.
    */
    virtual void dump() const {};

    /*
      Storing and restoring the sparse matrix. This assumes both
      matrices are of the same child class.
    */
    virtual void storeIntoOther( SparseMatrix *other ) const = 0;

    /*
      Merge column x2 into column x1, and zero x2 out
    */
    virtual void mergeColumns( unsigned x1, unsigned x2 ) = 0;

    /*
      Get the number of non-zero elements
    */
    virtual unsigned getNnz() const = 0;
};

#endif // __SparseMatrix_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
