/*********************                                                        */
/*! \file SparseVector.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **/

#ifndef __SparseVector_h__
#define __SparseVector_h__

#include "CSRMatrix.h"

/*
  This class provides supprot for sparse vectors in
  Compressed Sparse Row (CSR) format. The vector is
  simply stored as a 1 x n matrix in CSR format.
*/

class SparseVector
{
public:
    /*
      Initialization: the size determines the dimension of the
      underlying 1 x n matrix, and also influences the number
      of memory allocated initially.

      A vector can be initialized from a dense vector, or it
      can remain empty.
    */
    SparseVector();
    SparseVector( unsigned size );
    SparseVector( const double *V, unsigned size );
    void initialize( const double *V, unsigned size );
    void initializeToEmpty( unsigned size );

    /*
      Remove the vector's elements, without touching the
      allocated memory
    */
    void clear();

    /*
      The number of non-zero elements in the vector
    */
    unsigned getNnz() const;
    bool empty() const;

    /*
      Retrieve an element
    */
    double get( unsigned index );

    /*
      Convert the vector to dense format
    */
    void toDense( double *result ) const;

    /*
      Cloning
    */
    SparseVector &operator=( const SparseVector &other );

    /*
      For some delicate changes, we can get access to the
      internal CSR matrix
    */
    CSRMatrix *getInternalMatrix();

    /*
      Retrieve an entry by its index in the vector.
      This is not like "get" - the entry index is a number
      between 0 and _nnz.
    */
    unsigned getIndexOfEntry( unsigned entry ) const;
    double getValueOfEntry( unsigned entry ) const;

    /*
      This actually increase the diemsnion of the vector
    */
    void addEmptyLastEntry();
    void addLastEntry( double value );

    /*
      Changing values
    */
    void commitChange( unsigned index, double newValue );
    void executeChanges();

    /*
      For debugging
    */
    void dump() const;
    void dumpDense() const;

private:
    CSRMatrix _V;
};

#endif // __SparseVector_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
