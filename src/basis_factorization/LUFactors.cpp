/*********************                                                        */
/*! \file LUFactors.cpp
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

#include "BasisFactorizationError.h"
#include "Debug.h"
#include "FloatUtils.h"
#include "LUFactors.h"
#include "MString.h"

LUFactors::LUFactors( unsigned m )
    : _m( m )
    , _F( NULL )
    , _V( NULL )
    , _P( m )
    , _Q( m )
    , _z( NULL )
    , _workMatrix( NULL )
{
    _F = new double[m*m];
    if ( !_F )
        throw BasisFactorizationError( BasisFactorizationError::ALLOCATION_FAILED, "LUFactors::F" );

    _V = new double[m*m];
    if ( !_V )
        throw BasisFactorizationError( BasisFactorizationError::ALLOCATION_FAILED, "LUFactors::V" );

    _z = new double[m];
    if ( !_z )
        throw BasisFactorizationError( BasisFactorizationError::ALLOCATION_FAILED, "LUFactors::z" );

    _workMatrix = new double[m*m];
    if ( !_workMatrix )
        throw BasisFactorizationError( BasisFactorizationError::ALLOCATION_FAILED, "LUFactors::workMatrix" );
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

    if ( _z )
    {
        delete[] _z;
        _z = NULL;
    }

    if ( _workMatrix )
    {
        delete[] _workMatrix;
        _workMatrix = NULL;
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

    printf( "\tDumping the implied L:\n" );
    for ( unsigned i = 0; i < _m; ++i )
    {
        unsigned lRow = _P._columnOrdering[i];

        printf( "\t" );
        for ( unsigned j = 0; j < _m; ++j )
        {
            unsigned lCol = _P._columnOrdering[j];
            printf( "%8.2lf ", _F[lRow*_m + lCol] );
        }
        printf( "\n" );
    }

    delete[] result;
}

void LUFactors::fForwardTransformation( const double *y, double *x ) const
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
    memcpy( x, y, sizeof(double) * _m );

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

void LUFactors::fBackwardTransformation( const double *y, double *x ) const
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

    memcpy( x, y, sizeof(double) * _m );

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

void LUFactors::vForwardTransformation( const double *y, double *x ) const
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

        if ( FloatUtils::isZero( x[xBeingSolved] ) )
            x[xBeingSolved] = 0.0;
        else
            x[xBeingSolved] *= ( 1.0 / _V[vRow*_m + _Q._rowOrdering[uRow]] );
    }
}

void LUFactors::vBackwardTransformation( const double *y, double *x ) const
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

        if ( FloatUtils::isZero( x[xBeingSolved] ) )
            x[xBeingSolved] = 0.0;
        else
            x[xBeingSolved] *= ( 1.0 / _V[ _P._columnOrdering[uColumn]*_m + vColumn] );
    }
}

void LUFactors::forwardTransformation( const double *y, double *x ) const
{
    /*
      Solve Ax = FV x = y.

      First we find z such that Fz = y
      And then we find x such that Vx = z
    */

    fForwardTransformation( y, _z );
    vForwardTransformation( _z, x );
}

void LUFactors::backwardTransformation( const double *y, double *x ) const
{
    /*
      Solve xA = x FV = y.

      First we find z such that zV = y
      And then we find x such that xF = z
    */

    vBackwardTransformation( y, _z );
    fBackwardTransformation( _z, x );
}

void LUFactors::invertBasis( double *result )
{
    ASSERT( result );

    // Corner case - empty Tableau
    if ( _m == 0 )
        return;

    /*
      A = F * V = P * L * U * Q

      P' * A * Q' = LU
      Q * inv(A) * P = inv(LU)

      So, first we compute inv(LU). We do this by applying elementary
      row operations to the identity matrix.
    */

    // Initialize _workMatrix to the identity
    std::fill_n( _workMatrix, _m * _m, 0 );
    for ( unsigned i = 0; i < _m; ++i )
        _workMatrix[i*_m + i] = 1;

    /*
      Step 1: Multiply I on the left by inv(L) using
      elementary row operations.

      Go over L's columns from left to right and eliminate rows.
      Remember that L's diagonal is all 1s, and that L = P'FP
    */
    for ( unsigned lColumn = 0; lColumn < _m - 1; ++lColumn )
    {
        for ( unsigned lRow = lColumn + 1; lRow < _m; ++lRow )
        {
            double multiplier = -_F[_P._columnOrdering[lRow]*_m + _P._columnOrdering[lColumn]];

            for ( unsigned i = 0; i <= lColumn; ++i )
                _workMatrix[lRow*_m + i] += _workMatrix[lColumn*_m + i] * multiplier;
        }
    }

    /*
      Step 2: Multiply inv(L) on the left by inv(U) using
      elementary row operations.

      Go over U's columns from right to left and eliminate rows.
      Remember that U's diagonal are not necessarily 1s, and that U = P'VQ'
    */
    for ( int uColumn = _m - 1; uColumn >= 0; --uColumn )
    {
        double invUDiagonalEntry =
            1 / _V[_P._columnOrdering[uColumn]*_m + _Q._rowOrdering[uColumn]];

        for ( int uRow = 0; uRow < uColumn; ++uRow )
        {
            double multiplier = -_V[_P._columnOrdering[uRow]*_m + _Q._rowOrdering[uColumn]] * invUDiagonalEntry;

            for ( unsigned i = 0; i < _m; ++i )
                _workMatrix[uRow*_m + i] += _workMatrix[uColumn*_m +i] * multiplier;
        }

        for ( unsigned i = 0; i < _m; ++i )
            _workMatrix[uColumn*_m + i] *= invUDiagonalEntry;
    }

    /*
      Step 3: Compute inv(A), using

         Q * inv(A) * P = inv(LU)
         inv(A) = Q' * inv(LU) * P'
    */
    unsigned invLURow, invLUColumn;
    for ( unsigned i = 0; i < _m; ++i )
    {
        for ( unsigned j = 0; j < _m; ++j )
        {
            invLURow = _Q._columnOrdering[i];
            invLUColumn = _P._rowOrdering[j];

            result[i*_m + j] = _workMatrix[invLURow*_m + invLUColumn];
        }
    }
}

void LUFactors::storeToOther( LUFactors *other ) const
{
    ASSERT( _m == other->_m );

    memcpy( other->_F, _F, sizeof(double) * _m * _m );
    memcpy( other->_V, _V, sizeof(double) * _m * _m );

    _P.storeToOther( &other->_P );
    _Q.storeToOther( &other->_Q );
}

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
