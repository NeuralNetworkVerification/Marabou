/*********************                                                        */
/*! \file MockColumnOracle.h
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

#ifndef __MockColumnOracle_h__
#define __MockColumnOracle_h__

#include "IBasisFactorization.h"
#include "SparseColumnsOfBasis.h"
#include "SparseUnsortedList.h"

class MockColumnOracle : public IBasisFactorization::BasisColumnOracle
{
public:
    MockColumnOracle()
        : _basis( NULL )
        , _sparseBasis( NULL )
    {
    }

    void freeMemoryIfNeeded()
    {
        if ( _basis )
        {
            delete[] _basis;
            _basis = NULL;
        }

        if ( _sparseBasis )
        {
            for ( unsigned i = 0; i < _m; ++i )
            {
                if ( _sparseBasis->_columns[i] )
                {
                    delete _sparseBasis->_columns[i];
                    _sparseBasis->_columns[i] = NULL;
                }
            }

            delete _sparseBasis;
            _sparseBasis = NULL;
        }
    }

    ~MockColumnOracle()
    {
        freeMemoryIfNeeded();
    }

    void storeBasis( unsigned m, const double *basis )
    {
        // In some tests we might call storeBasis twice and we will lose the
        // pointers
        freeMemoryIfNeeded();
        _m = m;
        _basis = new double[_m * _m];
        _sparseBasis = new SparseColumnsOfBasis( _m );

        for ( unsigned row = 0; row < _m; ++row )
        {
            for ( unsigned column = 0; column < _m; ++column )
            {
                _basis[column * _m + row] = basis[row * _m + column];
            }
        }

        for ( unsigned i = 0; i < _m; ++i )
            _sparseBasis->_columns[i] = new SparseUnsortedList( _basis + ( i * _m ), _m );
    }

    double *_basis;
    unsigned _m;
    void getColumnOfBasis( unsigned column, double *result ) const
    {
        memcpy( result, _basis + ( _m * column ), sizeof(double) * _m );
    }

    void getColumnOfBasis( unsigned column, SparseUnsortedList *result ) const
    {
        result->initialize( _basis + ( _m * column ), _m );
    }

    SparseColumnsOfBasis *_sparseBasis;
    void getSparseBasis( SparseColumnsOfBasis &basis ) const
    {
        for ( unsigned i = 0; i < _m; ++i )
        {
            basis._columns[i] = _sparseBasis->_columns[i];
            if ( !basis._columns[i] )
                TS_ASSERT( 0 );
        }
    }
};

#endif // __MockColumnOracle_h__

//
// Local Variables:
// compile-command: "make -C ../../.. "
// tags-file-name: "../../../TAGS"
// c-basic-offset: 4
// End:
//
