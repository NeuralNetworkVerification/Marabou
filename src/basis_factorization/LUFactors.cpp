/*********************                                                        */
/*! \file LUFactors.cpp
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

#include "BasisFactorizationError.h"
#include "FloatUtils.h"
#include "LUFactors.h"
#include "MString.h"

LUFactors::LUFactors( unsigned m )
    : _m( m )
    , _P( m )
    , _Q( m )
{
    _F = new double[m*m];
    if ( !_F )
        throw BasisFactorizationError( BasisFactorizationError::ALLOCATION_FAILED, "LUFactors::F" );

    _V = new double[m*m];
    if ( !_V )
        throw BasisFactorizationError( BasisFactorizationError::ALLOCATION_FAILED, "LUFactors::V" );
}

LUFactors::~LUFactors()
{
    if ( _F )
    {
        delete[] _F;
        _F = NULL;
    }

    if ( _V )
    {
        delete[] _V;
        _V = NULL;
    }
}

void LUFactors::dump() const
{
    printf( "\nDumping LU factos:\n" );

    printf( "\tDumping F:\n" );
    for ( unsigned i = 0; i < _m; ++i )
    {
        printf( "\t" );
        for ( unsigned j = 0; j < _m; ++j )
        {
            printf( "%8.2lf ", _F[i*_m + j] );
        }
        printf( "\n" );
    }

    printf( "\tDumping V:\n" );
    for ( unsigned i = 0; i < _m; ++i )
    {
        printf( "\t" );
        for ( unsigned j = 0; j < _m; ++j )
        {
            printf( "%8.2lf ", _V[i*_m + j] );
        }
        printf( "\n" );
    }

    printf( "\tDumping product F*V:\n" );

    double *result = new double[_m*_m];

    std::fill_n( result, _m * _m, 0.0 );
    for ( unsigned i = 0; i < _m; ++i )
    {
        for ( unsigned j = 0; j < _m; ++j )
        {
            result[i*_m + j] = 0;
            for ( unsigned k = 0; k < _m; ++k )
            {
                result[i*_m + j] += _F[i*_m + k] * _V[k*_m + j];
            }
        }
    }

    for ( unsigned i = 0; i < _m; ++i )
    {
        printf( "\t" );
        for ( unsigned j = 0; j < _m; ++j )
        {
            printf( "%8.2lf ", result[i*_m + j] );
        }
        printf( "\n" );
    }

    printf( "\tDumping the implied U:\n" );
    for ( unsigned i = 0; i < _m; ++i )
    {
        unsigned uRow = _P._columnOrdering[i];

        printf( "\t" );
        for ( unsigned j = 0; j < _m; ++j )
        {
            unsigned uCol = _Q._rowOrdering[j];
            printf( "%8.2lf ", _V[uRow*_m + uCol] );
        }
        printf( "\n" );
    }

    delete[] result;
}

void LUFactors::fForwardTransformation( double *x )
{
    /*
      Example for solving for a lower triansgular matrix:

      | 1 0 0 |   | x1 |   | y1 |
      | 2 1 0 | * | x2 | = | y2 |
      | 3 4 1 |   | x3 |   | y3 |

      Solution:

       x1 = y1 (unchanged)
       x2 = y2 - 2y1
       x3 = y3 - 3y1 - 4y2

      However, F is not lower triangular, but rather F = PLP',
      or L = P'FP.
      Observe that the i'th column of L becomes the j'th column of F,
      for j = P._rowOrdering[i]. Also observe that the diagonal elements
      of L remain diagonal elements in F, i.e. F has a diagonal of 1s.
    */

    for ( unsigned lColumn = 0; lColumn < _m; ++lColumn )
    {
        unsigned fColumn = _P._columnOrdering[lColumn];

        printf( "\nfColumn = %u (x[%u] is the fixed element, %lf)\n", fColumn, fColumn, x[fColumn] );

        /*
          x[fColumn] has already been computed at this point,
          so we can substitute it into all remaining equations.
        */
        if ( FloatUtils::isZero( x[fColumn] ) )
            continue;

        for ( unsigned lRow = lColumn + 1; lRow < _m; ++lRow )
        {
            unsigned fRow = _P._columnOrdering[lRow];

            printf( "\tx[%u] -= %lf * %lf\n", fRow, _F[fRow*_m + fColumn], x[fColumn] );

            x[fRow] -= ( _F[fRow*_m + fColumn] * x[fColumn] );
        }
    }
}

// fBackwardTransformation: find x such that xF = y
      // vForwardTransformation:  find x such that Vx = y
      // vBackwardTransformation: find x such that xV = y

void LUFactors::fBackwardTransformation( double * )
{
}

void LUFactors::vForwardTransformation( double * )
{
}

void LUFactors::vBackwardTransformation( double * )
{
}

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
