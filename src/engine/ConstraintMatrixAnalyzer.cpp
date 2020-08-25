/*********************                                                        */
/*! \file ConstraintMatrixAnalyzer.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz, Shantanu Thakoor
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

 **/

#include "ConstraintMatrixAnalyzer.h"
#include "FloatUtils.h"
#include "List.h"
#include "MStringf.h"

ConstraintMatrixAnalyzer::ConstraintMatrixAnalyzer()
    : _numRowElements( NULL )
    , _numColumnElements( NULL )
    , _workRow( NULL )
    , _workRow2( NULL )
    , _rowHeaders( NULL )
    , _columnHeaders( NULL )
    , _rowHeadersInverse( NULL )
    , _columnHeadersInverse( NULL )
{
}

ConstraintMatrixAnalyzer::~ConstraintMatrixAnalyzer()
{
    freeMemoryIfNeeded();
}

void ConstraintMatrixAnalyzer::freeMemoryIfNeeded()
{
    if ( _rowHeaders )
    {
        delete[] _rowHeaders;
        _rowHeaders = NULL;
    }

    if ( _columnHeaders )
    {
        delete[] _columnHeaders;
        _columnHeaders = NULL;
    }

    if ( _rowHeadersInverse )
    {
        delete[] _rowHeadersInverse;
        _rowHeadersInverse = NULL;
    }

    if ( _columnHeadersInverse )
    {
        delete[] _columnHeadersInverse;
        _columnHeadersInverse = NULL;
    }

    if ( _workRow )
    {
        delete[] _workRow;
        _workRow = NULL;
    }

    if ( _workRow2 )
    {
        delete[] _workRow2;
        _workRow2 = NULL;
    }

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

void ConstraintMatrixAnalyzer::analyze( const double *matrix, unsigned m, unsigned n )
{
    freeMemoryIfNeeded();

    _m = m;
    _n = n;

    // Create a sparse version of the matrix
    _A.initialize( matrix, m, n );
    _A.transposeIntoOther( &_At );

    allocateMemory();

    // Perform the actual Gaussian elimination
    gaussianElimination();
}

void ConstraintMatrixAnalyzer::analyze( const SparseUnsortedList **matrix,
                                        unsigned m,
                                        unsigned n )
{
    freeMemoryIfNeeded();

    _m = m;
    _n = n;

    _A.initialize( matrix, m, n );
    _A.transposeIntoOther( &_At );

    allocateMemory();

    // Perform the actual Gaussian elimination
    gaussianElimination();
}

void ConstraintMatrixAnalyzer::allocateMemory()
{
    // Initialize the row and column headers
    _rowHeaders = new unsigned[_m];
    _columnHeaders = new unsigned[_n];
    _rowHeadersInverse = new unsigned[_m];
    _columnHeadersInverse = new unsigned[_n];

    for ( unsigned i = 0; i < _m; ++i )
    {
        _rowHeaders[i] = i;
        _rowHeadersInverse[i] = i;
    }

    for ( unsigned i = 0; i < _n; ++i )
    {
        _columnHeaders[i] = i;
        _columnHeadersInverse[i] = i;
    }

    // Initialize the row and column counters
    _numRowElements = new unsigned[_m];
    _numColumnElements = new unsigned[_n];

    for ( unsigned i = 0; i < _m; ++i )
        _numRowElements[i] = _A.getRow( i )->getNnz();

    for ( unsigned i = 0; i < _n; ++i )
        _numColumnElements[i] = _At.getRow( i )->getNnz();

    // Work memory
    _workRow = new double[_n];
    _workRow2 = new double[_n];
}

void ConstraintMatrixAnalyzer::gaussianElimination()
{
    /*
      We work column-by-column, until m columns are found
      that are non-zeroes, or until we run out of columns.
    */
    for ( _eliminationStep = 0; _eliminationStep < _m; ++_eliminationStep )
    {
        printf( "GE starting, elimination step %u\n", _eliminationStep );

        /*
          Step 1:
          -------
          Choose a pivot element from the active submatrix of U. This
          can be any non-zero entry. If no pivot exists, we are done.
        */
        if ( !choosePivot() )
            return;

        /*
          Step 2:
          -------
          Element <p,q> has been selected as the pivot. Move it to
          position [k,k], where k is the current elimination step.
        */
        permute();

        /*
          Step 3:
          -------
          Perform the actual elimination of lower rows in the active
          submatrix.
        */
        eliminate();
    }
}

bool ConstraintMatrixAnalyzer::choosePivot()
{
    printf( "\tChoose pivot invoked\n" );

    /*
      Apply the Markowitz rule: in the active sub-matrix,
      let p_i denote the number of non-zero elements in the i'th
      equation, and let q_j denote the number of non-zero elements
      in the q'th column.

      We pick a pivot a_ij \neq 0 that minimizes (p_i - 1)(q_i - 1).
    */

    const SparseUnsortedArray *sparseRow;
    const SparseUnsortedArray *sparseColumn;
    const SparseUnsortedArray::Entry *entry;
    unsigned nnz;

    // If there's a singleton row, use it as the pivot row
    for ( unsigned i = _eliminationStep; i < _m; ++i )
    {
        if ( _numRowElements[i] == 1 )
        {
            _pivotRow = i;

            // Get the singleton element
            sparseRow = _A.getRow( _rowHeaders[i] );
            ASSERT( sparseRow->getNnz() == 1U );
            entry = sparseRow->getArray();

            _pivotColumn = _columnHeadersInverse[entry->_index];
            _pivotElement = entry->_value;

            printf( "\tChoose pivot selected a pivot (singleton row): <%u,%u> = %lf\n",
                    _pivotRow,
                    _pivotColumn,
                    _pivotElement );
            return true;
        }
    }

    // If there's a singleton column, use it as the pivot column
    for ( unsigned i = _eliminationStep; i < _n; ++i )
    {
        if ( _numColumnElements[i] == 1 )
        {
            _pivotColumn = i;

            // Get the singleton element
            sparseColumn = _At.getRow( _columnHeaders[i] );
            entry = sparseColumn->getArray();
            nnz = sparseColumn->getNnz();

            // There may be some elements in higher rows - we need just the one
            // in the active submatrix.

            DEBUG( bool found = false; );

            for ( unsigned i = 0; i < nnz; ++i )
            {
                unsigned row = _rowHeadersInverse[entry[i]._index];

                if ( row >= _eliminationStep )
                {
                    DEBUG( found = true; );

                    _pivotRow = row;
                    _pivotElement = entry[i]._value;
                    break;
                }
            }

            ASSERT( found );

            printf( "\tChoose pivot selected a pivot (singleton column): <%u,%u> = %lf\n",
                    _pivotRow,
                    _pivotColumn,
                    _pivotElement );
            return true;
        }
    }

    // No singletons, apply the Markowitz rule. Find the element with
    // acceptable magnitude that has the smallet Markowitz value.
    // Fail if no elements exists that are within acceptable magnitude

    unsigned minimalCost = _m * _n;
    _pivotElement = 0.0;
    double absPivotElement = 0.0;

    bool found = false;
    for ( unsigned column = _eliminationStep; column < _n; ++column )
    {
        sparseColumn = _At.getRow( _columnHeaders[column] );

        double maxInColumn = 0;
        nnz = sparseColumn->getNnz();
        entry = sparseColumn->getArray();

        for ( unsigned i = 0; i < nnz; ++i )
        {
            // Ignore entries that are not in the active submatrix
            unsigned row = _rowHeadersInverse[entry[i]._index];
            if ( row < _eliminationStep )
                continue;

            double contender = FloatUtils::abs( entry[i]._value );
            if ( contender > maxInColumn )
                maxInColumn = contender;
        }

        if ( FloatUtils::isZero( maxInColumn ) )
        {
            // This is a zero column, not useful to us
            continue;
        }

        for ( unsigned i = 0; i < nnz; ++i )
        {
            unsigned row = _rowHeadersInverse[entry[i]._index];

            // Ignore entries that are not in the active submatrix
            if ( row < _eliminationStep )
                continue;

            double contender = entry[i]._value;
            double absContender = FloatUtils::abs( contender );

            // Only consider large-enough elements
            if ( absContender >
                 maxInColumn * GlobalConfiguration::GAUSSIAN_ELIMINATION_PIVOT_SCALE_THRESHOLD )
            {
                unsigned cost = ( _numRowElements[row] - 1 ) * ( _numColumnElements[column] - 1 );

                ASSERT( ( cost != minimalCost ) || found );

                if ( ( cost < minimalCost ) ||
                     ( ( cost == minimalCost ) && ( absContender > absPivotElement ) ) )
                {
                    minimalCost = cost;
                    _pivotRow = row;
                    _pivotColumn = column;
                    _pivotElement = contender;
                    absPivotElement = absContender;

                    found = true;
                }
            }
        }
    }

    if ( !found )
    {
        // Not more pivots, we are done
        return false;
    }

    printf( "\tChoose pivot selected a pivot: <%u,%u> = %lf (cost %u)\n",
            _pivotRow,
            _pivotColumn,
            _pivotElement,
            minimalCost );

    return true;
}

void ConstraintMatrixAnalyzer::permute()
{
    // Permute the rows
    unsigned temp = _rowHeaders[_eliminationStep];
    _rowHeaders[_eliminationStep] = _rowHeaders[_pivotRow];
    _rowHeaders[_pivotRow] = temp;

    _rowHeadersInverse[_rowHeaders[_eliminationStep]] = _eliminationStep;
    _rowHeadersInverse[_rowHeaders[_pivotRow]] = _pivotRow;

    // Permute the columns
    temp = _columnHeaders[_eliminationStep];
    _columnHeaders[_eliminationStep] = _columnHeaders[_pivotColumn];
    _columnHeaders[_pivotColumn] = temp;

    _columnHeadersInverse[_columnHeaders[_eliminationStep]] = _eliminationStep;
    _columnHeadersInverse[_columnHeaders[_pivotColumn]] = _pivotColumn;

    // Permute the element counters
    temp = _numRowElements[_eliminationStep];
    _numRowElements[_eliminationStep] = _numRowElements[_pivotRow];
    _numRowElements[_pivotRow] = temp;

    temp = _numColumnElements[_eliminationStep];
    _numColumnElements[_eliminationStep] = _numColumnElements[_pivotColumn];
    _numColumnElements[_pivotColumn] = temp;
}

void ConstraintMatrixAnalyzer::eliminate()
{
    /*
      Eliminate all entries below the pivot element A[k,k]
    */

    // Get the pivot row in dense format, due to repeated access
    _A.getRowDense( _rowHeaders[_eliminationStep], _workRow );

    /*
      The pivot row is not eliminated per se, but it is excluded
      from the active submatrix, so we adjust the element counters
    */
    _numRowElements[_eliminationStep] = 0;
    for ( unsigned column = _eliminationStep; column < _n; ++column )
    {
        if ( !FloatUtils::isZero( _workRow[_columnHeaders[column]] ) )
            --_numColumnElements[column];
    }

    // Process all rows below the pivot row
    SparseUnsortedArray *sparseColumn = _At.getRow( _columnHeaders[_eliminationStep] );
    unsigned index = 0;

    const SparseUnsortedArray::Entry *entry = sparseColumn->getArray();

    while ( index < sparseColumn->getNnz() )
    {
        unsigned row = _rowHeadersInverse[entry[index]._index];
        if ( row <= _eliminationStep )
        {
            ++index;
            continue;
        }

        /*
          Compute the Gaussian row multiplier for this row.
          The multiplier is: - U[row,k] / pivotElement
        */
        double rowMultiplier = -entry[index]._value / _pivotElement;

        // Get the row being eliminated in dense format
        _A.getRowDense( _rowHeaders[row], _workRow2 );

        // Eliminate the sub-diagonal entry
        --_numColumnElements[_eliminationStep];
        --_numRowElements[row];
        sparseColumn->erase( index );
        _workRow2[_columnHeaders[_eliminationStep]] = 0;

        // Handle the rest of the row
        for ( unsigned column = _eliminationStep + 1; column < _n; ++column )
        {
            unsigned columnLocation = _columnHeaders[column];

            // If the value does not change, skip
            if ( FloatUtils::isZero( _workRow[columnLocation] ) )
                continue;

            // Value will change
            double oldValue = _workRow2[columnLocation];
            bool wasZero = FloatUtils::isZero( oldValue );
            double newValue = oldValue + ( rowMultiplier * _workRow[columnLocation] );
            bool isZero = FloatUtils::isZero( newValue );

            if ( !wasZero && isZero )
            {
                newValue = 0;
                --_numColumnElements[column];
                --_numRowElements[row];
            }
            else if ( wasZero && !isZero )
            {
                ++_numColumnElements[column];
                ++_numRowElements[row];
            }

            _workRow2[columnLocation] = newValue;

            // Transposed matrix is updated immediately, regular matrix will
            // be updated when entire row has been processed
            if ( !FloatUtils::areEqual( newValue, oldValue ) )
                _At.set( columnLocation, _rowHeaders[row], newValue );
        }

        _A.updateSingleRow( _rowHeaders[row], _workRow2 );
    }
}

List<unsigned> ConstraintMatrixAnalyzer::getIndependentColumns() const
{
    List<unsigned> result;
    for ( unsigned i = 0; i < _eliminationStep; ++i )
        result.append( _columnHeaders[i] );
    return result;
}

Set<unsigned> ConstraintMatrixAnalyzer::getRedundantRows() const
{
    Set<unsigned> result;
    for ( unsigned i = _eliminationStep; i < _m; ++i )
        result.insert( _rowHeaders[i] );
    return result;
}

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
