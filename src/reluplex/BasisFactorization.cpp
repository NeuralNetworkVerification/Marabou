/*********************                                                        */
/*! \file BasisFactorization.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **/

#include "BasisFactorization.h"
#include "Debug.h"
#include "EtaMatrix.h"
#include "FloatUtils.h"
#include "ReluplexError.h"

#include <cstdio>
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
    freeIfNeeded();
}

void BasisFactorization::freeIfNeeded()
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

void BasisFactorization::forwardTransformation( const double *y, double *x )
{
    if ( _etas.empty() )
    {
        memcpy( x, y, sizeof(double) * _m );
        return;
    }

    double *tempY = new double[_m];
    memcpy( tempY, y, sizeof(double) * _m );
    std::fill( x, x + _m, 0.0 );

    for ( const auto &eta : _etas )
    {
        x[eta->_columnIndex] = tempY[eta->_columnIndex] / eta->_column[eta->_columnIndex];

        // Solve the rows above columnIndex
        for ( unsigned i = eta->_columnIndex + 1; i < _m; ++i )
            x[i] = tempY[i] - ( x[eta->_columnIndex] * eta->_column[i] );

        // Solve the rows below columnIndex
        for ( int i = eta->_columnIndex - 1; i >= 0; --i )
            x[i] = tempY[i] - ( x[eta->_columnIndex] * eta->_column[i] );

        // The x from this iteration becomes a for the next iteration
        memcpy( tempY, x, sizeof(double) *_m );
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
        x[columnIndex] = tempY[columnIndex];
        for ( unsigned i = 0; i < _m; ++i )
        {
            if ( i != columnIndex )
                x[columnIndex] -= (x[i] * (*eta)->_column[i]);
        }
        x[columnIndex] = x[columnIndex] / (*eta)->_column[columnIndex];

        // To handle -0.0
        if ( FloatUtils::isZero( x[columnIndex] ) )
            x[columnIndex] = 0.0;

        // The x from this iteration becomes y for the next iteration
        memcpy( tempY, x, sizeof(double) *_m );
    }
}

void BasisFactorization::storeFactorization( BasisFactorization *other ) const
{
    ASSERT( _m == other->_m );
    ASSERT( other->_etas.size() == 0 );

    memcpy( other->_B0, _B0, sizeof(double) * _m * _m );

    for ( const auto &eta : _etas )
        other->_etas.append( new EtaMatrix( eta->_m, eta->_columnIndex, eta->_column ) );
}

void BasisFactorization::restoreFactorization( const BasisFactorization *other )
{
    ASSERT( _m == other->_m );

    for ( const auto &it : _etas )
        delete it;

    _etas.clear();

    memcpy( _B0, other->_B0, sizeof(double) * _m * _m );

    for ( const auto &eta : other->_etas )
        _etas.append( new EtaMatrix( eta->_m, eta->_columnIndex, eta->_column ) );
}

//
// Local Variables:
// compile-command: "make -C .. "
// tags-file-name: "../TAGS"
// c-basic-offset: 4
// End:
//
