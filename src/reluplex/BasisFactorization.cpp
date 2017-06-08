/*********************                                                        */
/*! \file BasisFactorization.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Reluplex project.
 ** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **/

#include "BasisFactorization.h"
#include "EtaMatrix.h"
#include "ReluplexError.h"

#include <cstdlib>
#include <cstring>

BasisFactorization::BasisFactorization( unsigned m )
    : _m( m )
    , _B0( NULL )
{
    _B0 = new double[m*m];
    if ( !_B0 )
        throw ReluplexError( ReluplexError::ALLOCATION_FAILED, "BasisFactorization::B0" );
}

BasisFactorization::~BasisFactorization()
{
    if ( _B0 )
    {
        delete[] _B0;
        _B0 = NULL;
    }

    for ( const auto &it : _etas )
        delete it;
}

void BasisFactorization::pushEtaMatrix( unsigned columnIndex, double *column )
{
    EtaMatrix *matrix = new EtaMatrix( _m, columnIndex, column );
    _etas.append( matrix );
}

void BasisFactorization::forwardTransformation( const double *a, double *result )
{
    if ( _etas.empty() )
    {
        memcpy( result, a, sizeof(double) * _m );
        return;
    }

    double *tempA = new double[_m];
    memcpy( tempA, a, sizeof(double) * _m );
    std::fill( result, result + _m, 0.0 );

    for ( const auto &eta : _etas )
    {
        result[eta->_columnIndex] = tempA[eta->_columnIndex] / eta->_column[eta->_columnIndex];

        // Solve the rows above columnIndex
        for ( unsigned i = eta->_columnIndex + 1; i < _m; ++i )
            result[i] = tempA[i] - ( result[eta->_columnIndex] * eta->_column[i] );

        // Solve the rows below columnIndex
        for ( int i = eta->_columnIndex - 1; i >= 0; --i )
            result[i] = tempA[i] - ( result[eta->_columnIndex] * eta->_column[i] );

        // The result from this iteration becomes a for the next iteration
        memcpy( tempA, result, sizeof(double) *_m );
    }
}

void BasisFactorization::backwardTransformation( const double *y, double *x )
{
    if ( _etas.empty() )
    {
        memcpy( x, y, sizeof(double) * _m );
        return;
    }

    double *tempY = new double[_m];
    memcpy( tempY, y, sizeof(double) * _m );
    std::fill( x, x + _m, 0.0 );

    for ( auto eta = _etas.rbegin(); eta != _etas.rend(); ++eta )
    {
        // x is going to equal y in all entries except columnIndex
        memcpy( x, tempY, sizeof(double) * _m );

        // Compute the special column
        unsigned columnIndex = (*eta)->_columnIndex;
        x[columnIndex] = y[columnIndex];
        for ( unsigned i = 0; i < _m; ++i )
        {
            if ( i != columnIndex )
                x[columnIndex] -= x[i] * (*eta)->_column[i];
        }
        x[columnIndex] = x[columnIndex] / (*eta)->_column[columnIndex];

        // The x from this iteration becomes y for the next iteration
        memcpy( tempY, x, sizeof(double) *_m );
    }
}

//
// Local Variables:
// compile-command: "make -C . "
// tags-file-name: "./TAGS"
// c-basic-offset: 4
// End:
//
