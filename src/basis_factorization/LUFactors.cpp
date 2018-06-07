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
      Solve F*x = y

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
        /*
          x[fColumn] has already been computed at this point,
          so we can substitute it into all remaining equations.
        */
        if ( FloatUtils::isZero( x[fColumn] ) )
            continue;

        for ( unsigned lRow = lColumn + 1; lRow < _m; ++lRow )
        {
            unsigned fRow = _P._columnOrdering[lRow];
            x[fRow] -= ( _F[fRow*_m + fColumn] * x[fColumn] );
        }
    }
}

void LUFactors::fBackwardTransformation( double *x )
{
    /*
      Solve x*F = y

      Example for solving for a lower triansgular matrix:

                     | 1 0 0 |   | y1 |
      | x1 x2 x3 | * | 2 1 0 | = | y2 |
                     | 3 4 1 |   | y3 |

      Solution:

       x3 = y3
       x2 = y2 - 4x3
       x1 = y1 - 2x2 - 3x3

      However, F is not lower triangular, but rather F = PLP',
      or L = P'FP.
      Observe that the i'th column of L becomes the j'th column of F,
      for j = P._rowOrdering[i]. Also observe that the diagonal elements
      of L remain diagonal elements in F, i.e. F has a diagonal of 1s.
    */

    for ( int lRow = _m - 1; lRow >= 0; --lRow )
    {
        unsigned fRow = _P._columnOrdering[lRow];
        /*
          x[fRow] has already been computed at this point,
          so we can substitute it into all remaining equations.
        */
        if ( FloatUtils::isZero( x[fRow] ) )
            continue;

        for ( int lColumn = lRow - 1; lColumn >= 0; --lColumn )
        {
            unsigned fColumn = _P._columnOrdering[lColumn];
            x[fColumn] -= ( _F[fRow*_m + fColumn] * x[fRow] );
        }
    }
}

void LUFactors::vForwardTransformation( const double *y, double *x )
{
    /*
      Solve V*x = y

      Example for solving for an upper triansgular matrix:

      | 2 2  3 |   | x1 |   | y1 |
      | 0 -1 4 | * | x2 | = | y2 |
      | 0 0  4 |   | x3 |   | y3 |

      Solution:

       x3 = 1/4 y3
       x2 = 1/-1 ( y2 - 4x3 )
       x3 = 1/2 ( y1 - 2x2 - 3x3 )

      However, V is not lower triangular, but rather V = PUQ,
      or U = P'VQ'.
    */

    for ( int uRow = _m - 1; uRow >= 0; --uRow )
    {
        unsigned vRow = _P._columnOrdering[uRow];
        unsigned xBeingSolved = _Q._rowOrdering[uRow];
        x[xBeingSolved] = y[vRow];

        for ( unsigned uColumn = uRow + 1; uColumn < _m; ++uColumn )
        {
            unsigned vColumn = _Q._rowOrdering[uColumn];
            x[xBeingSolved] -= ( _V[vRow*_m + vColumn] * x[vColumn] );
        }

        x[xBeingSolved] *= ( 1.0 / _V[vRow*_m + _Q._rowOrdering[uRow]] );
    }
}

void LUFactors::vBackwardTransformation( const double *y, double *x )
{
    /*
      Solve x*V = y

      Example for solving for an upper triansgular matrix:

                     | 2 2  3 |   | y1 |
      | x1 x2 x3 | * | 0 -1 4 | = | y2 |
                     | 0 0  4 |   | y3 |

      Solution:

       x1 = 1/2 y1
       x2 = 1/-1 ( y2 - 2x1 )
       x3 = 1/4 ( y3 - 4x2 -3x1 )

      However, V is not lower triangular, but rather V = PUQ,
      or U = P'VQ'.
    */

    for ( unsigned uColumn = 0; uColumn < _m; ++uColumn )
    {
        unsigned vColumn = _Q._rowOrdering[uColumn];
        unsigned xBeingSolved = _P._columnOrdering[uColumn];
        x[xBeingSolved] = y[vColumn];

        for ( unsigned uRow = 0; uRow < uColumn; ++uRow )
        {
            unsigned vRow = _P._columnOrdering[uRow];
            x[xBeingSolved] -= ( _V[vRow*_m + vColumn] * x[vRow] );
        }

        x[xBeingSolved] *= ( 1.0 / _V[ _P._columnOrdering[uColumn]*_m + vColumn] );
    }
}

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
