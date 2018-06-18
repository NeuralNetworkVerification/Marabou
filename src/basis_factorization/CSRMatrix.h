/*********************                                                        */
/*! \file CSRMatrix.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Derek Huang
 **   Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **/

#ifndef __CSRMatrix_h__
#define __CSRMatrix_h__

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

class CSRMatrix
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
    void getRow( unsigned row, double *result ) const;
    void getColumn( unsigned column, double *result ) const;

    /*
      Add a row to the end of the matrix.
      The new row is provided in dense format.
    */
    void addLastRow( double *row );

    /*
      For debugging purposes.
    */
    void dump() const;

    /*
      Storing and restoring the sparse matrix
    */
    void storeIntoOther( CSRMatrix *other ) const;

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
