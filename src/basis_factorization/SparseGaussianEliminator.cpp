/********************                                                        */
/*! \file SparseGaussianEliminator.cpp
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
#include "GlobalConfiguration.h"
#include "MStringf.h"
#include "MalformedBasisException.h"
#include "SparseGaussianEliminator.h"
#include "SparseVector.h"

#include "SparseVectors.h"

#include <cstdio>

SparseGaussianEliminator::SparseGaussianEliminator( unsigned m )
    : _m( m )
    , _work( NULL )
    , _numURowElements( NULL )
    , _numUColumnElements( NULL )

{
    _work = new double[_m];
    if ( !_work )
        throw BasisFactorizationError( BasisFactorizationError::ALLOCATION_FAILED,
                                       "SparseGaussianEliminator::work" );

    _numURowElements = new unsigned[_m];
    if ( !_numURowElements )
        throw BasisFactorizationError( BasisFactorizationError::ALLOCATION_FAILED,
                                       "SparseGaussianEliminator::numURowElements" );

    _numUColumnElements = new unsigned[_m];
    if ( !_numUColumnElements )
        throw BasisFactorizationError( BasisFactorizationError::ALLOCATION_FAILED,
                                       "SparseGaussianEliminator::numUColumnElements" );
}

SparseGaussianEliminator::~SparseGaussianEliminator()
{
    if ( _work )
    {
        delete[] _work;
        _work = NULL;
    }

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

void SparseGaussianEliminator::initializeFactorization( const SparseColumnsOfBasis *A, SparseLUFactors *sparseLUFactors )
{
    // Allocate the work space
    _sparseLUFactors = sparseLUFactors;

    /*
      Initially:

        P = Q = I
        V = U = A
        F = L = I

        In the sparse representation of F, the diagonal is implicity 1,
        so we just leave it empty for now.
    */

    ((SparseVectors *)_sparseLUFactors->_Vt)->initialize( A->_columns, _m, _m );
    _sparseLUFactors->_Vt->transposeIntoOther( _sparseLUFactors->_V );

    _sparseLUFactors->_F->initializeToEmpty( _m, _m );
    _sparseLUFactors->_P.resetToIdentity();
    _sparseLUFactors->_Q.resetToIdentity();

    // Count number of non-zeros in U ( = V )
    _sparseLUFactors->_V->countElements( _numURowElements, _numUColumnElements );

    // Use same matrix P for L and V
    _sparseLUFactors->_usePForF = false;
}

void SparseGaussianEliminator::permute()
{
    /*
      The element selected for pivoting is U[p,q],
      We want to update P and Q to move u[p,q] to position [k,k] in U (= P'VQ'), where k is the current
      eliminiation step.
    */

    _sparseLUFactors->_P.swapColumns( _uPivotRow, _eliminationStep );
    _sparseLUFactors->_Q.swapRows( _uPivotColumn, _eliminationStep );

    // Adjust the element counters
    unsigned temp;
    temp = _numURowElements[_uPivotRow];
    _numURowElements[_uPivotRow] = _numURowElements[_eliminationStep];
    _numURowElements[_eliminationStep] = temp;

    temp = _numUColumnElements[_uPivotColumn];
    _numUColumnElements[_uPivotColumn] = _numUColumnElements[_eliminationStep];
    _numUColumnElements[_eliminationStep] = temp;
}

void SparseGaussianEliminator::run( const SparseColumnsOfBasis *A, SparseLUFactors *sparseLUFactors )
{
    // Initialize the LU factors
    initializeFactorization( A, sparseLUFactors );

    // Do the work
    factorize();
}

void SparseGaussianEliminator::factorize()
{
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

    // Execute the changes in F, compute its transpose
    _sparseLUFactors->_F->executeChanges();
    _sparseLUFactors->_F->transposeIntoOther( _sparseLUFactors->_Ft );

    // DEBUG({
    //         // Check that the factorization is correct
    //         double *product = new double[_m * _m];
    //         std::fill_n( product, _m * _m, 0 );

    //         for ( unsigned i = 0; i < _m; ++i )
    //             for ( unsigned j = 0; j < _m; ++j )
    //                 for ( unsigned k = 0; k < _m; ++k )
    //                 {
    //                     double fValue = ( i == k ) ? 1.0 : _sparseLUFactors->_F->get( i, k );
    //                     double vValue = _sparseLUFactors->_V->get( k, j );
    //                     product[i*_m + j] += fValue * vValue;
    //                 }

    //         for ( unsigned i = 0; i < _m; ++i )
    //             for ( unsigned j = 0; j < _m; ++j )
    //             {
    //                 ASSERT( FloatUtils::areEqual( product[i*_m+j], A->get( i, j ) ) );
    //             }

    //         delete[] product;
    //     });
}

void SparseGaussianEliminator::choosePivot()
{
    log( "Choose pivot invoked" );

    /*
      Apply the Markowitz rule: in the active sub-matrix,
      let p_i denote the number of non-zero elements in the i'th
      equation, and let q_j denote the number of non-zero elements
      in the q'th column.

      We pick a pivot a_ij \neq 0 that minimizes (p_i - 1)(q_i - 1).
    */

    SparseVector sparseRow( _m );
    SparseVector sparseColumn( _m );

    // If there's a singleton row, use it as the pivot row
    for ( unsigned i = _eliminationStep; i < _m; ++i )
    {
        if ( _numURowElements[i] == 1 )
        {
            _uPivotRow = i;
            _vPivotRow = _sparseLUFactors->_P._columnOrdering[i];

            // Get the singleton element
            _sparseLUFactors->_V->getRow( _vPivotRow, &sparseRow );

            ASSERT( sparseRow.getNnz() == 1U );

            _vPivotColumn = sparseRow.getIndexOfEntry( 0 );
            _uPivotColumn = _sparseLUFactors->_Q._columnOrdering[_vPivotColumn];
            _pivotElement = sparseRow.getValueOfEntry( 0 );

            log( Stringf( "Choose pivot selected a pivot (singleton row): V[%u,%u] = %lf",
                          _vPivotRow,
                          _vPivotColumn,
                          _pivotElement ) );
            return;
        }
    }

    // If there's a singleton column, use it as the pivot column
    for ( unsigned i = _eliminationStep; i < _m; ++i )
    {
        if ( _numUColumnElements[i] == 1 )
        {
            _uPivotColumn = i;
            _vPivotColumn = _sparseLUFactors->_Q._rowOrdering[i];

            // Get the singleton element
            _sparseLUFactors->_Vt->getRow( _vPivotColumn, &sparseColumn );

            // There may be some elements in higher rows - we need just the one
            // in the active submatrix.

            DEBUG( bool found = false; );

            for ( unsigned entry = 0; entry < sparseColumn.getNnz(); ++entry )
            {
                unsigned vRow = sparseColumn.getIndexOfEntry( entry );
                unsigned uRow = _sparseLUFactors->_P._rowOrdering[vRow];

                if ( uRow >= _eliminationStep )
                {
                    DEBUG( found = true; );

                    _vPivotRow = vRow;
                    _uPivotRow = uRow;
                    _pivotElement = sparseColumn.getValueOfEntry( entry );

                    break;
                }
            }

            ASSERT( found );

            log( Stringf( "Choose pivot selected a pivot (singleton column): V[%u,%u] = %lf",
                          _vPivotRow,
                          _vPivotColumn,
                          _pivotElement ) );
            return;
        }
    }

    // No singletons, apply the Markowitz rule. Find the element with acceptable
    // magnitude that has the smallet Markowitz rule.
    // Fail if no elements exists that are within acceptable magnitude

    // Todo: more clever heuristics to reduce the search space
    unsigned minimalCost = _m * _m;
    _pivotElement = 0.0;
    double absPivotElement = 0.0;

    bool found = false;
    for ( unsigned uColumn = _eliminationStep; uColumn < _m; ++uColumn )
    {
        unsigned vColumn = _sparseLUFactors->_Q._rowOrdering[uColumn];
        _sparseLUFactors->_Vt->getRow( vColumn, &sparseColumn );

        double maxInColumn = 0;
        for ( unsigned entry = 0; entry < sparseColumn.getNnz(); ++entry )
        {
            // Ignore entrying that are not in the active submatrix
            unsigned vRow = sparseColumn.getIndexOfEntry( entry );
            unsigned uRow = _sparseLUFactors->_P._rowOrdering[vRow];
            if ( uRow < _eliminationStep )
                continue;

            double contender = FloatUtils::abs( sparseColumn.getValueOfEntry( entry ) );
            if ( FloatUtils::gt( contender, maxInColumn ) )
                maxInColumn = contender;
        }

        if ( FloatUtils::isZero( maxInColumn ) )
        {
            throw BasisFactorizationError( BasisFactorizationError::GAUSSIAN_ELIMINATION_FAILED,
                                           "Have a zero column" );
        }

        for ( unsigned entry = 0; entry < sparseColumn.getNnz(); ++entry )
        {
            unsigned vRow = sparseColumn.getIndexOfEntry( entry );
            unsigned uRow = _sparseLUFactors->_P._rowOrdering[vRow];

            // Ignore entrying that are not in the active submatrix
            if ( uRow < _eliminationStep )
                continue;

            double contender = sparseColumn.getValueOfEntry( entry );
            double absContender = FloatUtils::abs( contender );

            // Only consider large-enough elements
            if ( FloatUtils::gt( absContender,
                                 maxInColumn * GlobalConfiguration::GAUSSIAN_ELIMINATION_PIVOT_SCALE_THRESHOLD ) )
            {
                unsigned cost = ( _numURowElements[uRow] - 1 ) * ( _numUColumnElements[uColumn] - 1 );

                ASSERT( ( cost != minimalCost ) || found );

                if ( ( cost < minimalCost ) ||
                     ( ( cost == minimalCost ) && FloatUtils::gt( absContender, absPivotElement ) ) )
                {
                    minimalCost = cost;
                    _uPivotRow = uRow;
                    _uPivotColumn = uColumn;
                    _vPivotRow = vRow;
                    _vPivotColumn = vColumn;
                    _pivotElement = contender;
                    absPivotElement = absContender;

                    found = true;
                }
            }
        }
    }

    if ( !found )
        throw BasisFactorizationError( BasisFactorizationError::GAUSSIAN_ELIMINATION_FAILED,
                                       "Couldn't find a pivot" );

    log( Stringf( "Choose pivot selected a pivot: V[%u,%u] = %lf (cost %u)", _vPivotRow, _vPivotColumn, _pivotElement, minimalCost ) );
}

void SparseGaussianEliminator::eliminate()
{
    unsigned fColumn = _sparseLUFactors->_P._columnOrdering[_eliminationStep];
    SparseVector sparseRow( _m );
    SparseVector sparseColumn( _m );

    /*
      Eliminate all entries below the pivot element U[k,k]
      We know that V[_vPivotRow, _vPivotColumn] = U[k,k].
    */
    _sparseLUFactors->_Vt->getRow( _vPivotColumn, &sparseColumn );

    // Get the pivot row in dense format, due to repeated access
    _sparseLUFactors->_V->getRowDense( _vPivotRow, _work );

    /*
      The pivot row is not eliminated per se, but it is excluded
      from the active submatrix, so we adjust the element counters
    */
    _numURowElements[_eliminationStep] = 0;
    for ( unsigned uColumn = _eliminationStep; uColumn < _m; ++uColumn )
    {
        unsigned vColumn = _sparseLUFactors->_Q._rowOrdering[uColumn];
        if ( !FloatUtils::isZero( _work[vColumn] ) )
            --_numUColumnElements[uColumn];
    }

    // Process all rows below the pivot row
    for ( unsigned entry = 0; entry < sparseColumn.getNnz(); ++entry )
    {
        unsigned vRow = sparseColumn.getIndexOfEntry( entry );
        unsigned uRow = _sparseLUFactors->_P._rowOrdering[vRow];

        if ( uRow <= _eliminationStep )
            continue;

        /*
          Compute the Gaussian row multiplier for this row.
          The multiplier is: - U[row,k] / pivotElement
        */
        double rowMultiplier = - sparseColumn.getValueOfEntry( entry ) / _pivotElement;
        log( Stringf( "\tWorking on V row: %u. Multiplier: %lf", vRow, rowMultiplier ) );

        // Eliminate the row
        --_numUColumnElements[_eliminationStep];
        --_numURowElements[uRow];
        _sparseLUFactors->_V->commitChange( vRow, _vPivotColumn, 0.0 );
        _sparseLUFactors->_Vt->commitChange( _vPivotColumn, vRow, 0.0 );
        _sparseLUFactors->_V->getRow( vRow, &sparseRow );

        Set<unsigned> columnsAlreadyHandled;
        // First, handle non-zero entries in the row being eliminated
        for ( unsigned rowEntry = 0; rowEntry < sparseRow.getNnz(); ++rowEntry )
        {
            unsigned vColumnIndex = sparseRow.getIndexOfEntry( rowEntry );
            unsigned uColumnIndex = _sparseLUFactors->_Q._columnOrdering[vColumnIndex];

            columnsAlreadyHandled.insert( vColumnIndex );

            // Only care about the active submatirx
            if ( uColumnIndex <= _eliminationStep )
                continue;

            // If the value does not change, skip
            if ( FloatUtils::isZero( _work[vColumnIndex] ) )
                continue;

            // Value will change
            double newValue = sparseRow.getValueOfEntry( rowEntry ) + ( rowMultiplier * _work[vColumnIndex] );
            if ( FloatUtils::isZero( newValue ) )
            {
                newValue = 0;
                --_numUColumnElements[uColumnIndex];
                --_numURowElements[uRow];
            }

            _sparseLUFactors->_V->commitChange( vRow, vColumnIndex, newValue );
            _sparseLUFactors->_Vt->commitChange( vColumnIndex, vRow, newValue );
        }

        // Next, handle entries that were zero in the eliminated row
        for ( unsigned uColumnIndex = _eliminationStep + 1; uColumnIndex < _m; ++uColumnIndex )
        {
            unsigned vColumnIndex = _sparseLUFactors->_Q._rowOrdering[uColumnIndex];

            if ( columnsAlreadyHandled.exists( vColumnIndex ) )
                continue;

            if ( FloatUtils::isZero( _work[vColumnIndex] ) )
                continue;

            ++_numUColumnElements[uColumnIndex];
            ++_numURowElements[uRow];
            double newVal = rowMultiplier * _work[vColumnIndex];
            _sparseLUFactors->_V->commitChange( vRow, vColumnIndex, newVal );
            _sparseLUFactors->_Vt->commitChange( vColumnIndex, vRow, newVal );
        }

        /*
          Store the row multiplier in matrix F, using F = PLP'.
          F's rows are ordered same as V's
        */
        _sparseLUFactors->_F->commitChange( vRow, fColumn, -rowMultiplier );
    }

    // Execute the changes in V
    _sparseLUFactors->_V->executeChanges();
    _sparseLUFactors->_Vt->executeChanges();
}

void SparseGaussianEliminator::log( const String &message )
{
    if ( GlobalConfiguration::GAUSSIAN_ELIMINATION_LOGGING )
        printf( "SparseGaussianEliminator: %s\n", message.ascii() );
}

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
