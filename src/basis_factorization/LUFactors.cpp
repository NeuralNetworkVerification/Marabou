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
    , _invF( NULL )
    , _invV( NULL )
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

    _invF = new double[m*m];
    if ( !_invF )
        throw BasisFactorizationError( BasisFactorizationError::ALLOCATION_FAILED, "LUFactors::invF" );

    _invV = new double[m*m];
    if ( !_invV )
        throw BasisFactorizationError( BasisFactorizationError::ALLOCATION_FAILED, "LUFactors::invV" );

    _workMatrix = new double[m*m];
    if ( !_invV )
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

    if ( _invF )
    {
        delete[] _invF;
        _invF = NULL;
    }

    if ( _invV )
    {
        delete[] _invV;
        _invV = NULL;
    }

    if ( _workMatrix )
    {
        delete[] _invV;
        _invV = NULL;
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

    /*
      A = FV

      inv(A) = inv(V) * inv(F)
     */

    /*
      Step 1: Compute invV.
      Go over U, and translate each entry to its corresponding
      entry in invV.

      V = PUQ
      U = P'VQ'

      inv(V) = Q' inv(U) P'
      inv(U) = Q  inv(V) P
    */
    std::fill_n( _invV, _m * _m, 0 );

    // Handle U row by row, from top to bottom
    for ( int uRow = _m - 1; uRow >= 0; --uRow )
    {
        /*
          Start with the diagonal element of this row: invert
          and place it in invV.
        */
        double invUDiagonalEntry = 1 / _V[_P._columnOrdering[uRow]*_m + _Q._rowOrdering[uRow]];
        _invV[_Q._rowOrdering[uRow]*_m + _P._columnOrdering[uRow]] = invUDiagonalEntry;

        /*
          Next, any remaining entries on this row are computed directly,
          by considering the product of the corresponding row of U and the
          corresponding (partially-computed) column of invV.
        */
        for ( unsigned uColumn = uRow + 1; uColumn < _m; ++uColumn )
        {
            // Compute inv(U)[uRow, uColumn]
            for ( unsigned i = uRow + 1; i < _m; ++i )
            {
                // invU[uRow, uColumn] -= U[uRow, i] * invU[i, uColumn]
                _invV[_Q._rowOrdering[uRow]*_m + _P._columnOrdering[uColumn]] -=
                      ( _V[_P._columnOrdering[uRow]*_m + _Q._rowOrdering[i]] *
                        _invV[_Q._rowOrdering[i]*_m + _P._columnOrdering[uColumn]] );
            }

            _invV[_Q._rowOrdering[uRow]*_m + _P._columnOrdering[uColumn]] *= invUDiagonalEntry;
        }
    }

    /*
      Step 2: Compute invF.
      Go over L, and translate each entry to its corresponding
      entry in invF.

         F = PLP'
         L = P'FP

         inv(F) = P * inv(L) * P'
         inv(L) = P' * inv(F) * P
    */
    std::fill_n( _invF, _m * _m, 0 );

    // Handle L row by row, from top to bottom
    for ( unsigned lRow = 0; lRow < _m; ++lRow )
    {
        // L's diagonal entry is always 1. Place it in inv(F)
        _invF[_P._columnOrdering[lRow]*_m + _P._columnOrdering[lRow]] = 1;

        /*
          The remaining elements on row i (to the left of the
          diagonal) are computed by taking the i'th row of L and
          multiplying it by each of the partially-computed columns
          of inv(L).
        */
        for ( unsigned lColumn = 0; lColumn < lRow; ++lColumn )
        {
            // Compute inv(L)[lRow, lColumn]
            for ( unsigned i = 0; i < lRow; ++i )
            {
                // invL[lRow,lColumn] -= L[lRow,i] * invL[i,lColumn]
                _invF[_P._columnOrdering[lRow]*_m + _P._columnOrdering[lColumn]] -=
                    _F[_P._columnOrdering[lRow]*_m + _P._columnOrdering[i]] *
                    _invF[_P._columnOrdering[i]*_m + _P._columnOrdering[lColumn]];
            }
        }
    }

    /*
      Step 3: Compute inv(A), using

          inv(A) = inv(V) * inv(F)
    */
    std::fill_n( result, _m * _m, 0.0 );
    for ( unsigned i = 0; i < _m; ++i )
        for ( unsigned j = 0; j < _m; ++j )
            for ( unsigned k = 0; k < _m; ++k )
                result[i*_m + j] += _invV[i*_m + k] * _invF[k*_m + j];
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
