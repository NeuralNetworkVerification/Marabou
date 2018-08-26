/*********************                                                        */
/*! \file SparseUnsortedVector.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **/

#ifndef __SparseUnsortedVector_h__
#define __SparseUnsortedVector_h__

#include "HashMap.h"
#include "SparseMatrix.h"
#include "SparseVector.h"

class SparseUnsortedVector
{
public:
    /*
      Initialization: the size determines the dimension of the
      underlying storage.

      A unsortedVector can be initialized from a dense unsortedVector, or it
      can remain empty.
    */
    SparseUnsortedVector();
    ~SparseUnsortedVector();
    SparseUnsortedVector( unsigned size );
    SparseUnsortedVector( const double *V, unsigned size );
    void initialize( const double *V, unsigned size );
    void initializeToEmpty();

    /*
      Remove the unsortedVector's elements, without touching the
      allocated memory
    */
    void clear();

    /*
      Set a value
    */
    void set( unsigned index, double value );

    /*
      The number of non-zero elements in the unsortedVector
    */
    unsigned getNnz() const;
    bool empty() const;

    /*
      Retrieve an element
    */
    double get( unsigned entry ) const;

    /*
      Convert the unsortedVector to dense format
    */
    void toDense( double *result ) const;

    /*
      Increase vector size and add new entry
    */
    void addLastEntry( double entry );
    void incrementSize();

    /*
      Cloning
    */
    SparseUnsortedVector &operator=( const SparseUnsortedVector &other );
    void storeIntoOther( SparseUnsortedVector *other ) const;

    /*
      Retrieve entries
    */
    HashMap<unsigned, double>::const_iterator begin() const;
    HashMap<unsigned, double>::const_iterator end() const;

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
    unsigned _size;
    HashMap<unsigned, double> _vector;
    Map<unsigned, double> _committedChanges;
};

#endif // __SparseUnsortedVector_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
