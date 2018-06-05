/********************                                                        */
/*! \file GaussianEliminator.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **/

#include "BasisFactorizationError.h"
#include "EtaMatrix.h"
#include "FloatUtils.h"
#include "GaussianEliminator.h"
#include "GlobalConfiguration.h"
#include "MStringf.h"
#include "MalformedBasisException.h"

#include <cstdio>

GaussianEliminator::GaussianEliminator( const double *A, unsigned m )
    : _A( A )
    , _m( m )
    , _luFactors( NULL )
    , _numRowElements( NULL )
    , _numColumnElements( NULL )
{
}

GaussianEliminator::~GaussianEliminator()
{
    if ( _numRowElements )
    {
        delete[] _numRowElements;
        _numRowElements = NULL;
    }

    if ( _numColumnElements )
    {
        delete[] _numColumnElements;
        _numColumnElements = NULL;
    }
}

void GaussianEliminator::initializeFactorization()
{
    // Allocate the work space
    _luFactors = new LUFactors( _m );
    if ( !_luFactors )
        throw BasisFactorizationError( BasisFactorizationError::ALLOCATION_FAILED,
                                       "GaussianEliminator::luFactors" );

    _numRowElements = new unsigned[_m];
    if ( !_numRowElements )
        throw BasisFactorizationError( BasisFactorizationError::ALLOCATION_FAILED,
                                       "GaussianEliminator::numRowElements" );

    _numColumnElements = new unsigned[_m];
    if ( !_numColumnElements )
        throw BasisFactorizationError( BasisFactorizationError::ALLOCATION_FAILED,
                                       "GaussianEliminator::numColumnElements" );

    /*
      Initially:

        P = Q = I
        V = U = A
        F = L = I
    */
    memcpy( _luFactors->_V, _A, sizeof(double) * _m * _m );
    std::fill_n( _luFactors->_F, _m * _m, 0 );
    for ( unsigned i = 0; i < _m; ++i )
        _luFactors->_F[i*_m +i] = 1;

    // Count number of non-zeros in A
    std::fill_n( _numRowElements, _m, 0 );
    std::fill_n( _numColumnElements, _m, 0 );
    for ( unsigned i = 0; i < _m; ++i )
    {
        for ( unsigned j = 0; j < _m; ++j )
        {
            if ( !FloatUtils::isZero( _A[i*_m + j] ) )
            {
                ++_numRowElements[i];
                ++_numColumnElements[i];
            }
        }
    }
}

void GaussianEliminator::permute()
{
    /*
      The element selected for pivoting is V[p,q],
      where p = _pivotRow and _q = _pivotColumn. Because

         V = PUQ

      this translates into some u[i,j]. We want to update P and Q to
      move u[i,j] to position [k,k] in U (= P'VQ'), where k is the current
      eliminiation step.
    */

    // Translate V-indices to U-indices
    unsigned uRow = _luFactors->_P._rowOrdering[_pivotRow];
    unsigned uColumn = _luFactors->_Q._columnOrdering[_pivotColumn];

    // Move U[uRow, uColumn] to U[k,k] (because U = P'VQ')
    _luFactors->_P.swapColumns( uRow, _eliminationStep );
    _luFactors->_Q.swapRows( uColumn, _eliminationStep );
}

LUFactors *GaussianEliminator::run()
{
    // Initialize the LU factors
    initializeFactorization();

    // Main factorization loop
    for ( _eliminationStep = 0; _eliminationStep < _m; ++_eliminationStep )
    {
        /*
          Step 1:
          -------
          Choose a pivot element from the active submatrix. This
          can be any non-zero coefficient. Store the result in
          _pivotRow, _pivotColumn. These indices refer to matrix V.
        */
        choosePivot();

        /*
          Step 2:
          -------
          Element V[p,q] has been selected as the pivot. Find the
          corresponding element of U and move it to position [k,k],
          where k is the current elimination step.
        */
        permute();

        /*
          Step 3:
          -------
          Perform the actual elimination on U, while maintaining the
          equation A = FV.
        */
        eliminate();
    }

    return _luFactors;
}

void GaussianEliminator::choosePivot()
{
    log( "Choose pivot invoked" );

    /*
      Apply the Markowitz rule: in the active sub-matrix,
      let p_i denote the number of non-zero elements in the i'th
      equation, and let q_j denote the number of non-zero elements
      in the q'th column.

      We pick a pivot a_ij \neq 0 that minimizes (p_i - 1)(q_i - 1).

      Although the selection should be performed on U, it is actually
      performed on V, which is explicitly stored. We know that

          V = P * U * Q,

      And since P and Q only rearrange elements in rows and columns,
      the Markowitz cost of each entry remains the same in V as in U.
    */

    /*
      Todo:
      If there's a singleton column, select that element as pivot
      If there's a singleton row, select thta element as pivot
      Only if both fail, use Markowitz
    */

    unsigned *rowCounters = new unsigned[_m];
    unsigned *columnCounters = new unsigned[_m];

    std::fill_n( rowCounters, _m, 0.0 );
    std::fill_n( columnCounters, _m, 0.0 );

    for ( unsigned i = _eliminationStep; i < _m; ++i )
    {
        for ( unsigned j = _eliminationStep; j < _m; ++j )
        {
            if ( !FloatUtils::isZero( _luFactors->_V[i*_m + j] ) )
            {
                ++rowCounters[i];
                ++columnCounters[j];
            }
        }
    }

    unsigned minimum = _m * _m;
    bool found = false;
    for ( unsigned i = _eliminationStep; i < _m; ++i )
    {
        for ( unsigned j = _eliminationStep; j < _m; ++j )
        {
            if ( !FloatUtils::isZero( _luFactors->_V[i*_m + j] ) )
            {
                found = true;

                double candidate = ( rowCounters[i] - 1 ) * ( columnCounters[j] - 1 );
                if ( candidate < minimum )
                {
                    minimum = candidate;
                    _pivotRow = i;
                    _pivotColumn = j;
                }
            }
        }
    }

    log( Stringf( "Choose pivot selected a pivot: V[%u,%u] = %lf (cost %u)", _pivotRow, _pivotColumn, _luFactors->_V[_pivotRow*_m + _pivotColumn], minimum ) );

    if ( !found )
        throw BasisFactorizationError( BasisFactorizationError::GAUSSIAN_ELIMINATION_FAILED,
                                       "Couldn't find a pivot" );
}

void GaussianEliminator::eliminate()
{
    unsigned fColumnIndex = _luFactors->_P._rowOrdering[_eliminationStep];

    /*
      Eliminate all entries below the pivot element U[k,k]
      We know that V[_pivotRow, _pivotColumn] = U[k,k].
    */
    double pivotElement = _luFactors->_V[_pivotRow*_m + _pivotColumn];

    log( Stringf( "Eliminate called. Pivot element: %lf", pivotElement ) );

    // Process all rows below the pivot row.
    for ( unsigned row = _eliminationStep + 1; row < _m; ++row )
    {
        /*
          Compute the Gaussian row multiplier for this row.
          The multiplier is: - U[row,k] / pivotElement
          We compute it in terms of V
        */
        unsigned vRowIndex = _luFactors->_P._rowOrdering[row];
        double rowMultiplier = -_luFactors->_V[vRowIndex*_m + _pivotColumn] / pivotElement;

        log( Stringf( "\tWorking on V row: %u. Multiplier: %lf", vRowIndex, rowMultiplier ) );

        /*
          And now use it to eliminate the row
        */
        _luFactors->_V[vRowIndex*_m + _pivotColumn] = 0;
        for ( unsigned column = _eliminationStep + 1; column < _m; ++column )
        {
            unsigned vColIndex = _luFactors->_Q._columnOrdering[column];
            _luFactors->_V[vRowIndex*_m + vColIndex] +=
                rowMultiplier * _luFactors->_V[_pivotRow*_m + vColIndex];
        }

        /*
          Store the row multiplier in matrix F, using F = PLP'.
          F's rows are ordered same as V's
        */
        _luFactors->_F[vRowIndex*_m + fColumnIndex] = -rowMultiplier;
    }

    /*
      Put 1 in L's diagonal.
      TODO: This can be made implicit
    */
    _luFactors->_F[fColumnIndex*_m + fColumnIndex] = 1;
}

void GaussianEliminator::log( const String &message )
{
    if ( GlobalConfiguration::GAUSSIAN_ELIMINATION_LOGGING )
        printf( "GaussianEliminator: %s\n", message.ascii() );
}

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
