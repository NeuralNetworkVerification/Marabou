/*********************                                                        */
/*! \file BasisDecomposition.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Reluplex project.
 ** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **/

#include "BasisDecomposition.h"
#include "EtaMatrix.h"
#include "ReluplexError.h"

#include <cstdlib>
#include <cstring>

BasisDecomposition::BasisDecomposition( unsigned m )
    : _m( m )
    , _B0( NULL )
{
    _B0 = new double[m*m];
    if ( !_B0 )
        throw ReluplexError( ReluplexError::ALLOCATION_FAILED, "BasisDecomposition::B0" );
}

BasisDecomposition::~BasisDecomposition()
{
    if ( _B0 )
    {
        delete[] _B0;
        _B0 = NULL;
    }

    for ( const auto &it : _etas )
        delete it;
}

void BasisDecomposition::pushEtaMatrix( unsigned columnIndex, double *column )
{
    EtaMatrix *matrix = new EtaMatrix( _m, columnIndex, column );
    _etas.append( matrix );
}

void BasisDecomposition::forwardTransformation( const double *a, double *result )
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

//
// Local Variables:
// compile-command: "make -C . "
// tags-file-name: "./TAGS"
// c-basic-offset: 4
// End:
//
