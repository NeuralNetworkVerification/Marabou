/*********************                                                        */
/*! \file SparseEtaMatrix.h
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

#ifndef __SparseEtaMatrix_h__
#define __SparseEtaMatrix_h__

#include "List.h"

class SparseEtaMatrix
{
public:
    SparseEtaMatrix( unsigned m, unsigned index, const double *column );

    /*
      Initializees the matrix to the identity matrix
    */
    SparseEtaMatrix( unsigned m, unsigned index );

    SparseEtaMatrix( const SparseEtaMatrix &other );
    SparseEtaMatrix &operator=( const SparseEtaMatrix &other );

    ~SparseEtaMatrix();
    void dump() const;
    void dumpDenseTransposed() const;
    void toMatrix( double *A ) const;

    bool operator==( const SparseEtaMatrix &other ) const;

    void addEntry( unsigned index, double value );

    struct Entry
    {
        Entry( unsigned index, double value )
            : _index( index )
            , _value( value )
        {
        }

        unsigned _index;
        double _value;
    };

    unsigned _m;
    unsigned _columnIndex;

    List<Entry> _sparseColumn;
    double _diagonalElement;
};

#endif // __SparseEtaMatrix_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
