/*********************                                                        */
/*! \file SparseLUFactors.cpp
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
#include "MString.h"
#include "SparseLUFactors.h"

SparseLUFactors::SparseLUFactors( unsigned m )
    : _m( m )
    , _F( NULL )
    , _V( NULL )
    , _P( m )
    , _Q( m )
    , _usePForF( false )
    , _PForF( m )
    , _Ft( NULL )
    , _Vt( NULL )
    , _vDiagonalElements( NULL )
    , _z( NULL )
    , _workMatrix( NULL )
    , _workVector( NULL )
{
    _F = new SparseUnsortedLists();
    if ( !_F )
        throw BasisFactorizationError( BasisFactorizationError::ALLOCATION_FAILED, "SparseLUFactors::F" );

    _V = new SparseUnsortedLists();
    if ( !_V )
        throw BasisFactorizationError( BasisFactorizationError::ALLOCATION_FAILED, "SparseLUFactors::V" );

    _Ft = new SparseUnsortedLists();
    if ( !_Ft )
        throw BasisFactorizationError( BasisFactorizationError::ALLOCATION_FAILED, "SparseLUFactors::Ft" );

    _Vt = new SparseUnsortedLists();
    if ( !_Vt )
        throw BasisFactorizationError( BasisFactorizationError::ALLOCATION_FAILED, "SparseLUFactors::Vt" );

    _vDiagonalElements = new double[m];
    if ( !_vDiagonalElements )
        throw BasisFactorizationError( BasisFactorizationError::ALLOCATION_FAILED, "SparseLUFactors::vDiagonalElements" );

    _z = new double[m];
    if ( !_z )
        throw BasisFactorizationError( BasisFactorizationError::ALLOCATION_FAILED, "SparseLUFactors::z" );

    _workMatrix = new double[m*m];
    if ( !_workMatrix )
        throw BasisFactorizationError( BasisFactorizationError::ALLOCATION_FAILED, "SparseLUFactors::workMatrix" );

    _workVector = new double[m];
    if ( !_workVector )
        throw BasisFactorizationError( BasisFactorizationError::ALLOCATION_FAILED, "SparseLUFactors::workVector" );
}

SparseLUFactors::~SparseLUFactors()
{
    if ( _F )
    {
        delete _F;
        _F = NULL;
    }

    if ( _V )
    {
        delete _V;
        _V = NULL;
    }

    if ( _Ft )
    {
        delete _Ft;
        _Ft = NULL;
    }

    if ( _Vt )
    {
        delete _Vt;
        _Vt = NULL;
    }

    if ( _vDiagonalElements )
    {
        delete[] _vDiagonalElements;
        _vDiagonalElements = NULL;
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

    if ( _workVector )
    {
        delete[] _workVector;
        _workVector = NULL;
    }
}

void SparseLUFactors::dump() const
{
    printf( "\nDumping LU factos:\n" );

    printf( "\tDumping F:\n" );
    _F->dumpDense();

    printf( "\tDumping V:\n" );
    _V->dumpDense();

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
                result[i*_m + j] += ( i == k ? 1.0 : _F->get( i, k ) ) * _V->get( k, j );
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
            printf( "%8.2lf ", _V->get( uRow, uCol ) );
        }
        printf( "\n" );
    }

    printf( "\tDumping the stored V diagonal:\n" );
    for ( unsigned i = 0; i < _m; ++i )
    {
        printf( "\t%8.2lf\n", _vDiagonalElements[i] );
    }
    printf( "\n" );

    printf( "\tDumping the implied L:\n" );
    for ( unsigned i = 0; i < _m; ++i )
    {
        const PermutationMatrix *p = ( _usePForF ) ? &_PForF : &_P;
        unsigned lRow = p->_columnOrdering[i];
        printf( "\t" );
        for ( unsigned j = 0; j < _m; ++j )
        {
            unsigned lCol = p->_columnOrdering[j];
            printf( "%8.2lf ", ( lRow == lCol ? 1.0 : _F->get( lRow, lCol ) ) );
        }
        printf( "\n" );
    }

    delete[] result;
}

void SparseLUFactors::fForwardTransformation( const double *y, double *x ) const
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
      Observe that the diagonal elements of L remain diagonal
      elements in F, i.e. F has a diagonal of 1s.
    */

    memcpy( x, y, sizeof(double) * _m );

    const PermutationMatrix *p = ( _usePForF ) ? &_PForF : &_P;
    for ( unsigned lRow = 0; lRow < _m; ++lRow )
    {
        unsigned fRow = p->_columnOrdering[lRow];
        const SparseUnsortedList *sparseRow = _F->getRow( fRow );

        for ( const auto &entry : *sparseRow )
        {
            unsigned fColumn = entry._index;
            x[fRow] -= x[fColumn] * entry._value;
        }
    }
}

void SparseLUFactors::fBackwardTransformation( const double *y, double *x ) const
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
      Observe that the diagonal elements of L remain diagonal
      elements in F, i.e. F has a diagonal of 1s.
    */

    memcpy( x, y, sizeof(double) * _m );

    const PermutationMatrix *p = ( _usePForF ) ? &_PForF : &_P;
    for ( int lColumn = _m - 1; lColumn >= 0; --lColumn )
    {
        unsigned fColumn = p->_columnOrdering[lColumn];
        const SparseUnsortedList *sparseColumn = _Ft->getRow( fColumn );

        for ( const auto &entry : *sparseColumn )
        {
            unsigned fRow = entry._index;
            x[fColumn] -= ( entry._value * x[fRow] );
        }
    }
}

void SparseLUFactors::vForwardTransformation( const double *y, double *x ) const
{
    /*
      Solve V*x = y

      Example for solving for an upper triangular matrix:

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

    const SparseUnsortedList *sparseColumn;
    double diagonalElement;
    double xElement;
    unsigned vRow;
    unsigned vColumn;

    memcpy( _workVector, y, sizeof(double) * _m );

    for ( int uRow = _m - 1; uRow >= 0; --uRow )
    {
        vRow = _P._columnOrdering[uRow];
        vColumn = _Q._rowOrdering[uRow];

        // Extract the pivot elemnt
        diagonalElement = _vDiagonalElements[vRow];

        // Set the (final) value of this entry of x
        xElement = x[vColumn] = ( _workVector[vRow] / diagonalElement );

        // Eliminate this entry from all other entries
        if ( xElement != 0.0 )
        {
            sparseColumn = _Vt->getRow( vColumn );
            for ( const auto &entry : *sparseColumn )
                _workVector[entry._index] -= xElement * entry._value;
        }
    }
}

void SparseLUFactors::vBackwardTransformation( const double *y, double *x ) const
{
    /*
      Solve x*V = y.

      Example for solving for an upper triansgular matrix:

                     | 2 2  3 |
      | x1 x2 x3 | * | 0 -1 4 | = | y1 y2 y3 |
                     | 0 0  4 |

      Solution:

       x1 = 1/2 y1
       x2 = 1/-1 ( y2 - 2x1 )
       x3 = 1/4 ( y3 - 4x2 -3x1 )

      However, V is not lower triangular, but rather V = PUQ,
      or U = P'VQ'.

      It is also useful to remember that xV = y is equivalent to
      V'x' = y'.
    */

    const SparseUnsortedList *sparseRow;
    double diagonalElement;
    double xElement;
    unsigned vRow;
    unsigned vColumn;

    memcpy( _workVector, y, sizeof(double) * _m );

    for ( unsigned utIndex = 0; utIndex < _m; ++utIndex )
    {
        // The utIndex row of u' is the k-th column of u
        vRow = _P._columnOrdering[utIndex];
        // The utIndex column of u' is the k-th row of u
        vColumn = _Q._rowOrdering[utIndex];

        // Extract the pivot elemnt
        diagonalElement = _vDiagonalElements[vRow];

        // Set the (final) value of this entry of x
        xElement = x[vRow] = ( _workVector[vColumn] / diagonalElement );

        // Eliminate this entry from all other entries
        if ( xElement != 0.0 )
        {
            sparseRow = _V->getRow( vRow );
            for ( const auto &entry : *sparseRow )
                _workVector[entry._index] -= xElement * entry._value;
        }
    }
}

void SparseLUFactors::forwardTransformation( const double *y, double *x ) const
{
    /*
      Solve Ax = FV x = y.

      First we find z such that Fz = y
      And then we find x such that Vx = z
    */

    fForwardTransformation( y, _z );
    vForwardTransformation( _z, x );
}

void SparseLUFactors::backwardTransformation( const double *y, double *x ) const
{
    /*
      Solve xA = x FV = y.

      First we find z such that zV = y
      And then we find x such that xF = z
    */

    vBackwardTransformation( y, _z );
    fBackwardTransformation( _z, x );
}

void SparseLUFactors::invertBasis( double *result )
{
    ASSERT( result );

    if ( _usePForF )
    {
        printf( "SparseLUFactors::invertBasis not supported when _usePForF is set!\n" );
        exit( 1 );
    }

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
      Remember that L's diagonal is all 1s, and that L = P'FP, F = PLP'
    */
    for ( unsigned lColumn = 0; lColumn < _m - 1; ++lColumn )
    {
        unsigned fColumn = _P._columnOrdering[lColumn];
        const SparseUnsortedList *sparseColumn = _Ft->getRow( fColumn );
        for ( const auto &entry : *sparseColumn )
        {
            unsigned fRow = entry._index;
            unsigned lRow = _P._rowOrdering[fRow];

            if ( lRow > lColumn )
            {
                for ( unsigned i = 0; i <= lColumn; ++i )
                    _workMatrix[lRow*_m + i] += _workMatrix[lColumn*_m + i] * ( - entry._value );
            }
        }
    }

    /*
      Step 2: Multiply inv(L) on the left by inv(U) using
      elementary row operations.

      Go over U's columns from right to left and eliminate rows.
      Remember that U's diagonal are not necessarily 1s, and that U = P'VQ', V = PUQ
    */
    for ( int uColumn = _m - 1; uColumn >= 0; --uColumn )
    {
        unsigned vColumn = _Q._rowOrdering[uColumn];
        const SparseUnsortedList *sparseColumn = _Vt->getRow( vColumn );

        double invUDiagonalEntry =
            1 / sparseColumn->get( _P._columnOrdering[uColumn] );

        for ( const auto &entry : *sparseColumn )
        {
            unsigned vRow = entry._index;
            unsigned uRow = _P._rowOrdering[vRow];

            if ( (int)uRow < uColumn )
            {
                double multiplier = ( -entry._value * invUDiagonalEntry );
                for ( unsigned i = 0; i < _m; ++i )
                    _workMatrix[uRow*_m + i] += _workMatrix[uColumn*_m +i] * multiplier;
            }

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

void SparseLUFactors::storeToOther( SparseLUFactors *other ) const
{
    ASSERT( _m == other->_m );
    ASSERT( !_usePForF );

    _F->storeIntoOther( other->_F );
    _V->storeIntoOther( other->_V );

    _Ft->storeIntoOther( other->_Ft );
    _Vt->storeIntoOther( other->_Vt );

    _P.storeToOther( &other->_P );
    _Q.storeToOther( &other->_Q );

    memcpy( other->_vDiagonalElements, _vDiagonalElements, sizeof(double) * _m );

    other->_usePForF = false;
}

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
