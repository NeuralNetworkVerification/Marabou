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

class SparseVector
{
public:
    SparseVector();
    SparseVector( unsigned size );
    SparseVector( const double *V, unsigned size );

    void initialize( const double *V, unsigned size );

    void initializeToEmpty( unsigned size );

    void clear();

    unsigned getNnz() const;

    bool empty() const;

    double get( unsigned index );

    void dump() const;

    void toDense( double *result ) const;

    SparseVector &operator=( const SparseVector &other );

    CSRMatrix *getInternalMatrix();

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
