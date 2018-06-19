/*********************                                                        */
/*! \file CSRMatrix.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **/

#ifndef __CSRMatrix_h__
#define __CSRMatrix_h__

#include "SparseMatrix.h"

/*
  This class provides supprot for sparse matrices in
  Compressed Sparse Row (CSR) format.

  Let nnz represent the number of non-zero entries in a matrix M
  of dimensions m x n.

  The CSR format is comprised of 3 arrays:

  - Array A of length nnz that holds M's non-zero elements in
    row-major order (left-to-right, top-to-bottom).

  - Array IA of length m + 1 that is defined as follows:

    IA[0] = 0
    IA[i] = IA[i-1] + number of non-zero elements in the i'th row
            of M

  - Array JA of length nnz that holds the column index of every element.
*/

class CSRMatrix : public SparseMatrix
{
public:
    /*
      Initialize a CSR matrix from a given matrix M of dimensions
      m x n, or create an empty object and then initialize it separately.
    */
    CSRMatrix( const double *M, unsigned m, unsigned n );
    CSRMatrix();
    ~CSRMatrix();
    void initialize( const double *M, unsigned m, unsigned n );

    /*
      Obtain a single element/row/column of the matrix.
    */
    double get( unsigned row, unsigned column ) const;
    void getRow( unsigned row, SparseVector *result ) const;
    void getColumn( unsigned column, SparseVector *result ) const;
    void getColumnDense( unsigned column, double *result ) const;

    /*
      Add a row/column to the end of the matrix.
      The new row/column is provided in dense format.
    */
    void addLastRow( double *row );
    void addLastColumn( double *column );

    /*
      This function increments n, the number of columns in the
      matrix. It assumes the new column is all zeroes.
     */
    void addEmptyColumn();

    /*
      For debugging purposes.
    */
    void dump() const;

    /*
      Storing and restoring the sparse matrix
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

private:
    enum {
        // Initial estimate: each row has average density 1 / ROW_DENSITY_ESTIMATE
        ROW_DENSITY_ESTIMATE = 5,
    };

    unsigned _m;
    unsigned _n;

    double *_A;
    unsigned *_IA;
    unsigned *_JA;

    unsigned _nnz;

    /*
      An estimate of nnz, used to allocate space for _A.
      If the real nnz exceeds this value, it needs to be
      increased.
    */
    unsigned _estimatedNnz;

    /*
      If too many elements are stored for the current
      arrays' capacity, increase their size.
    */
    void increaseCapacity();

    /*
      Release allocated memory
    */
    void freeMemoryIfNeeded();
};

#endif // __CSRMatrix_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
