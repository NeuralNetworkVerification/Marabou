/*********************                                                        */
/*! \file MockColumnOracle.h
** \verbatim
** Top contributors (to current version):
**   Guy Katz
** This file is part of the Marabou project.
** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
** in the top-level source directory) and their institutional affiliations.
** All rights reserved. See the file COPYING in the top-level source
** directory for licensing information.\endverbatim
**/

#ifndef __MockColumnOracle_h__
#define __MockColumnOracle_h__

#include "IBasisFactorization.h"

class MockColumnOracle : public IBasisFactorization::BasisColumnOracle
{
public:
    MockColumnOracle()
        : _basis( NULL )
    {
    }

    ~MockColumnOracle()
    {
        if ( _basis )
        {
            delete[] _basis;
            _basis = NULL;
        }
    }

    void storeBasis( unsigned m, const double *basis )
    {
        _m = m;
        _basis = new double[_m * _m];

        for ( unsigned row = 0; row < _m; ++row )
        {
            for ( unsigned column = 0; column < _m; ++column )
            {
                _basis[column * _m + row] = basis[row * _m + column];
            }
        }
    }

    double *_basis;
    unsigned _m;
    void getColumnOfBasis( unsigned column, double *result ) const
    {
        memcpy( result, _basis + ( _m * column ), sizeof(double) * _m );
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
