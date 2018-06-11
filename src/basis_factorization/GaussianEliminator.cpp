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
#include "Debug.h"
#include "EtaMatrix.h"
#include "FloatUtils.h"
#include "GaussianEliminator.h"
#include "GlobalConfiguration.h"
#include "MStringf.h"
#include "MalformedBasisException.h"

#include <cstdio>

GaussianEliminator::GaussianEliminator( unsigned m )
    : _m( m )
    , _numURowElements( NULL )
    , _numUColumnElements( NULL )
{
    _numURowElements = new unsigned[_m];
    if ( !_numURowElements )
        throw BasisFactorizationError( BasisFactorizationError::ALLOCATION_FAILED,
                                       "GaussianEliminator::numURowElements" );

    _numUColumnElements = new unsigned[_m];
    if ( !_numUColumnElements )
        throw BasisFactorizationError( BasisFactorizationError::ALLOCATION_FAILED,
                                       "GaussianEliminator::numUColumnElements" );
}

GaussianEliminator::~GaussianEliminator()
{
    if ( _numURowElements )
    {
        delete[] _numURowElements;
        _numURowElements = NULL;
    }

    if ( _numUColumnElements )
    {
        delete[] _numUColumnElements;
        _numUColumnElements = NULL;
    }
}

void GaussianEliminator::initializeFactorization( const double *A, LUFactors *luFactors )
{
    // Allocate the work space
    _luFactors = luFactors;

    /*
      Initially:

        P = Q = I
        V = U = A
        F = L = I
    */
    memcpy( _luFactors->_V, A, sizeof(double) * _m * _m );
    std::fill_n( _luFactors->_F, _m * _m, 0 );
    for ( unsigned i = 0; i < _m; ++i )
        _luFactors->_F[i*_m +i] = 1;

    _luFactors->_P.resetToIdentity();
    _luFactors->_Q.resetToIdentity();

    // Count number of non-zeros in U ( = A )
    std::fill_n( _numURowElements, _m, 0 );
    std::fill_n( _numUColumnElements, _m, 0 );
    for ( unsigned i = 0; i < _m; ++i )
    {
        for ( unsigned j = 0; j < _m; ++j )
        {
            if ( !FloatUtils::isZero( A[i*_m + j] ) )
            {
                ++_numURowElements[i];
                ++_numUColumnElements[j];
            }
        }
    }
}

void GaussianEliminator::permute()
{
    /*
      The element selected for pivoting is U[p,q],
      We want to update P and Q to move u[p,q] to position [k,k] in U (= P'VQ'), where k is the current
      eliminiation step.
    */

    _luFactors->_P.swapColumns( _uPivotRow, _eliminationStep );
    _luFactors->_Q.swapRows( _uPivotColumn, _eliminationStep );

    // Adjust the element counters
    unsigned temp;
    temp = _numURowElements[_uPivotRow];
    _numURowElements[_uPivotRow] = _numURowElements[_eliminationStep];
    _numURowElements[_eliminationStep] = temp;

    temp = _numUColumnElements[_uPivotColumn];
    _numUColumnElements[_uPivotColumn] = _numUColumnElements[_eliminationStep];
    _numUColumnElements[_eliminationStep] = temp;
}

void GaussianEliminator::run( const double *A, LUFactors *luFactors )
{
    // Initialize the LU factors
    initializeFactorization( A, luFactors );

    // Main factorization loop
    for ( _eliminationStep = 0; _eliminationStep < _m; ++_eliminationStep )
    {
        /*
          Step 1:
          -------
          Choose a pivot element from the active submatrix of U. This
          can be any non-zero coefficient. Store the result in:
             _uPivotRow, _uPivotColumn (indices in U)
             _vPivotRow, _vPivotColumn (indices in V)
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
    */

    bool found = false;

    // If there's a singleton row, use it as the pivot row
    for ( unsigned i = _eliminationStep; i < _m; ++i )
    {
        if ( _numURowElements[i] == 1 )
        {
            _uPivotRow = i;
            _vPivotRow = _luFactors->_P._columnOrdering[i];

            // Locate the singleton element
            for ( unsigned j = _eliminationStep; j < _m; ++j )
            {
                unsigned vCol = _luFactors->_Q._rowOrdering[j];
                if ( !FloatUtils::isZero( _luFactors->_V[_vPivotRow*_m + vCol] ) )
                {
                    _vPivotColumn = vCol;
                    _uPivotColumn = j;

                    found = true;
                    break;
                }
            }

            ASSERT( found );

            log( Stringf( "Choose pivot selected a pivot (singleton row): V[%u,%u] = %lf",
                          _vPivotRow,
                          _vPivotColumn,
                          _luFactors->_V[_vPivotRow*_m + _vPivotColumn] ) );

            return;
        }
    }

    // If there's a singleton column, use it as the pivot column
    for ( unsigned i = _eliminationStep; i < _m; ++i )
    {
        if ( _numUColumnElements[i] == 1 )
        {
            _uPivotColumn = i;
            _vPivotColumn = _luFactors->_Q._rowOrdering[i];

            // Locate the singleton element
            for ( unsigned j = _eliminationStep; j < _m; ++j )
            {
                unsigned vRow = _luFactors->_P._columnOrdering[j];
                if ( !FloatUtils::isZero( _luFactors->_V[vRow*_m + _vPivotColumn] ) )
                {
                    _vPivotRow = vRow;
                    _uPivotRow = j;

                    found = true;
                    break;
                }
            }

            ASSERT( found );

            log( Stringf( "Choose pivot selected a pivot (singleton column): V[%u,%u] = %lf",
                          _vPivotRow,
                          _vPivotColumn,
                          _luFactors->_V[_vPivotRow*_m + _vPivotColumn] ) );
            return;
        }
    }

    // No singletons, apply the Markowitz rule. Find the element with acceptable
    // magnitude that has the smallet Markowitz rule.
    // Fail if no elements exists that are within acceptable magnitude

    // Todo: more clever heuristics to reduce the search space
    unsigned minimalCost = _m * _m;
    double pivotEntry = 0.0;
    for ( unsigned column = _eliminationStep; column < _m; ++column )
    {
        // First find the maximal element in the column
        double maxInColumn = 0;
        for ( unsigned row = _eliminationStep; row < _m; ++row )
        {
            unsigned vRow = _luFactors->_P._columnOrdering[row];
            unsigned vColumn = _luFactors->_Q._rowOrdering[column];
            double contender = FloatUtils::abs( _luFactors->_V[vRow*_m + vColumn] );

            if ( FloatUtils::gt( contender, maxInColumn ) )
                maxInColumn = contender;
        }

        if ( FloatUtils::isZero( maxInColumn ) )
        {
            if ( !found )
                throw BasisFactorizationError( BasisFactorizationError::GAUSSIAN_ELIMINATION_FAILED,
                                               "Have a zero column" );

        }

        // Now scan the column for candidates
        for ( unsigned row = _eliminationStep; row < _m; ++row )
        {
            unsigned vRow = _luFactors->_P._columnOrdering[row];
            unsigned vColumn = _luFactors->_Q._rowOrdering[column];
            double contender = FloatUtils::abs( _luFactors->_V[vRow*_m + vColumn] );

            // Only consider large-enough elements
            if ( FloatUtils::gt( contender,
                                 maxInColumn * GlobalConfiguration::GAUSSIAN_ELIMINATION_PIVOT_SCALE_THRESHOLD ) )
            {
                unsigned cost = ( _numURowElements[row] - 1 ) * ( _numUColumnElements[column] - 1 );

                ASSERT( ( cost != minimalCost ) || found );

                if ( ( cost < minimalCost ) ||
                     ( ( cost == minimalCost ) && FloatUtils::gt( contender, pivotEntry ) ) )
                {
                    minimalCost = cost;
                    _uPivotRow = row;
                    _uPivotColumn = column;
                    _vPivotRow = vRow;
                    _vPivotColumn = vColumn;
                    pivotEntry = contender;
                    found = true;
                }
            }
        }
    }

    log( Stringf( "Choose pivot selected a pivot: V[%u,%u] = %lf (cost %u)", _vPivotRow, _vPivotColumn, _luFactors->_V[_vPivotRow*_m + _vPivotColumn], minimalCost ) );

    if ( !found )
        throw BasisFactorizationError( BasisFactorizationError::GAUSSIAN_ELIMINATION_FAILED,
                                       "Couldn't find a pivot" );
}

void GaussianEliminator::eliminate()
{
    unsigned fColumnIndex = _luFactors->_P._columnOrdering[_eliminationStep];

    /*
      Eliminate all entries below the pivot element U[k,k]
      We know that V[_pivotRow, _pivotColumn] = U[k,k].
    */
    double pivotElement = _luFactors->_V[_vPivotRow*_m + _vPivotColumn];

    log( Stringf( "Eliminate called. Pivot element: %lf", pivotElement ) );

    // Process all rows below the pivot row.
    for ( unsigned row = _eliminationStep + 1; row < _m; ++row )
    {
        /*
          Compute the Gaussian row multiplier for this row.
          The multiplier is: - U[row,k] / pivotElement
          We compute it in terms of V
        */
        unsigned vRowIndex = _luFactors->_P._columnOrdering[row];
        double subDiagonalEntry = _luFactors->_V[vRowIndex*_m + _vPivotColumn];

        // Ignore zero entries
        if ( FloatUtils::isZero( subDiagonalEntry ) )
            continue;

        double rowMultiplier = -subDiagonalEntry / pivotElement;
        log( Stringf( "\tWorking on V row: %u. Multiplier: %lf", vRowIndex, rowMultiplier ) );

        // Eliminate the row
        _luFactors->_V[vRowIndex*_m + _vPivotColumn] = 0;
        --_numUColumnElements[_eliminationStep];
        --_numURowElements[row];

        for ( unsigned column = _eliminationStep + 1; column < _m; ++column )
        {
            unsigned vColIndex = _luFactors->_Q._rowOrdering[column];

            bool wasZero = FloatUtils::isZero( _luFactors->_V[vRowIndex*_m + vColIndex] );

            _luFactors->_V[vRowIndex*_m + vColIndex] +=
                rowMultiplier * _luFactors->_V[_vPivotRow*_m + vColIndex];

            bool isZero = FloatUtils::isZero( _luFactors->_V[vRowIndex*_m + vColIndex] );

            if ( wasZero != isZero )
            {
                if ( wasZero )
                {
                    ++_numUColumnElements[column];
                    ++_numURowElements[row];
                }
                else
                {
                    --_numUColumnElements[column];
                    --_numURowElements[row];
                }
            }

            if ( isZero )
                _luFactors->_V[vRowIndex*_m + vColIndex] = 0;
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
