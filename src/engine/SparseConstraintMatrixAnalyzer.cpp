/*********************                                                        */
/*! \file SparseConstraintMatrixAnalyzer.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz, Duligur Ibeling
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
#include "EtaMatrix.h"
#include "FloatUtils.h"
#include "GlobalConfiguration.h"
#include "MStringf.h"
#include "MalformedBasisException.h"
#include "SparseConstraintMatrixAnalyzer.h"

#include <cstdio>

SparseConstraintMatrixAnalyzer::SparseConstraintMatrixAnalyzer( const double *constraintMatrix, unsigned m, unsigned n )
    : _A( NULL )
    , _m( m )
    , _n( n )
    , _rowHeaders( NULL )
    , _columnHeaders( NULL )
    , _inverseRowHeaders( NULL )
    , _inverseColumnHeaders( NULL )
    , _work( NULL )
    , _work2( NULL )
{
    // Construct a sparse matrix

    _A = new SparseUnsortedList *[_m];
    for ( unsigned i = 0; i < _m; ++i )
        _A[i] = new SparseUnsortedList( constraintMatrix + ( i * _n ), _n );

    _rowHeaders = new unsigned[_m];
    _inverseRowHeaders = new unsigned[_m];

    _columnHeaders = new unsigned[_n];
    _inverseColumnHeaders = new unsigned[_n];

    _work = new double[_n];
    _work2 = new double[_n];
}

SparseConstraintMatrixAnalyzer::~SparseConstraintMatrixAnalyzer()
{
    if ( _A )
    {
        for ( unsigned i = 0; i < _m; ++i )
        {
            if ( _A[i] )
            {
                delete _A[i];
                _A[i] = NULL;
            }
        }

        delete[] _A;
        _A = NULL;
    }

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

    if ( _inverseRowHeaders )
    {
        delete[] _inverseRowHeaders;
        _inverseRowHeaders = NULL;
    }

    if ( _inverseColumnHeaders )
    {
        delete[] _inverseColumnHeaders;
        _inverseColumnHeaders = NULL;
    }

    if ( _work )
    {
        delete[] _work;
        _work = NULL;
    }

    if ( _work2 )
    {
        delete[] _work2;
        _work2 = NULL;
    }
}

void SparseConstraintMatrixAnalyzer::analyze()
{
    // Initialize the row and column headers
    for ( unsigned i = 0; i < _m; ++i )
    {
        _rowHeaders[i] = i;
        _inverseRowHeaders[i] = i;
    }

    for ( unsigned i = 0; i < _n; ++i )
    {
        _columnHeaders[i] = i;
        _inverseColumnHeaders[i] = i;
    }

    gaussianElimination();
}

void SparseConstraintMatrixAnalyzer::dumpMatrix()
{
    printf( "\nDumping matrix (%u x %u):\n\n", _m, _n );
    double row[_n];

    for ( unsigned i = 0; i < _m; ++i )
    {
        printf( "\t" );
        _A[i]->toDense( row );
        for ( unsigned j = 0; j < _n; ++j )
            printf( "%5.2lf ", row[j] );
        printf( "\n" );
    }

    printf( "\nNow dumping permuted matrix:\n\n" );
    for ( unsigned i = 0; i < _m; ++i )
    {
        printf( "\t" );
        _A[_rowHeaders[i]]->toDense( row );
        for ( unsigned j = 0; j < _n; ++j )
            printf( "%5.2lf ", row[_columnHeaders[j]] );
        printf( "\n" );
    }

    printf( "\n" );
}

void SparseConstraintMatrixAnalyzer::swapRows( unsigned i, unsigned j )
{
    unsigned temp;

    temp = _inverseRowHeaders[_rowHeaders[i]];
    _inverseRowHeaders[_rowHeaders[i]] = _inverseRowHeaders[_rowHeaders[j]];
    _inverseRowHeaders[_rowHeaders[j]] = temp;

    temp = _rowHeaders[i];
    _rowHeaders[i] = _rowHeaders[j];
    _rowHeaders[j] = temp;
}

void SparseConstraintMatrixAnalyzer::swapColumns( unsigned i, unsigned j )
{
    unsigned temp;

    temp = _inverseColumnHeaders[_columnHeaders[i]];
    _inverseColumnHeaders[_columnHeaders[i]] = _inverseColumnHeaders[_columnHeaders[j]];
    _inverseColumnHeaders[_columnHeaders[j]] = temp;

    temp = _columnHeaders[i];
    _columnHeaders[i] = _columnHeaders[j];
    _columnHeaders[j] = temp;
}

void SparseConstraintMatrixAnalyzer::gaussianElimination()
{
    /*
      We work column-by-column, until m columns are found
      that are non-zeroes, or until we run out of columns.
    */
    _eliminationStep = 0;

    while ( _eliminationStep < _m )
    {
        printf( "\nWorking on elimintation step %u\n", _eliminationStep );

        // Find largest pivot in active submatrix
        double largestPivot = 0.0;
        unsigned bestRow = 0, bestColumn = 0;
        for ( unsigned i = _eliminationStep; i < _m; ++i )
        {
            SparseUnsortedList *row = _A[_rowHeaders[i]];

            for ( const auto &entry : *row )
            {
                unsigned j = entry._index;
                double contender = FloatUtils::abs( entry._value );

                if ( contender > largestPivot )
                {
                    largestPivot = contender;
                    bestRow = i;
                    bestColumn = _inverseColumnHeaders[j];
                }
            }
        }

        // If we couldn't find a non-zero pivot, elimination is complete
        if ( FloatUtils::isZero( largestPivot ) )
            return;

        // Move the pivot row to the top
        if ( bestRow != _eliminationStep )
            swapRows( bestRow, _eliminationStep );

        // Move the pivot column to the top
        if ( bestColumn != _eliminationStep )
            swapColumns( bestColumn, _eliminationStep );

        /*
          Eliminate the rows below the pivot row
        */

        // First, extract the dense pivot row
        _A[_rowHeaders[_eliminationStep]]->toDense( _work );

        // Compute the inverse pivot entry
        double invPivotEntry = 1 / _work[_columnHeaders[_eliminationStep]];

        // Eliminate each row below the pivot row
        for ( unsigned i = _eliminationStep + 1; i < _m; ++i )
        {
            SparseUnsortedList *rowBeingEliminated = _A[_rowHeaders[i]];
            double subPivotEntry = rowBeingEliminated->get( _columnHeaders[_eliminationStep] );

            if ( FloatUtils::isZero( subPivotEntry ) )
                continue;

            double factor = - invPivotEntry * subPivotEntry;

            rowBeingEliminated->toDense( _work2 );
            for ( unsigned j = 0; j < _n; ++j )
                _work2[j] += _work[j] * factor;
            rowBeingEliminated->initialize( _work2, _n );
        }

        _independentColumns.append( _columnHeaders[_eliminationStep] );
        ++_eliminationStep;
    }

    // dumpMatrix( "Elimination finished" );
}

void SparseConstraintMatrixAnalyzer::getCanonicalForm( double *matrix )
{
    for ( unsigned i = 0; i < _m; ++i )
    {
        SparseUnsortedList *row = _A[_rowHeaders[i]];
        row->toDense( _work );
        for ( unsigned j = 0; j < _n; ++j )
            matrix[i*_n + j] = _work[_columnHeaders[j]];
    }
}

unsigned SparseConstraintMatrixAnalyzer::getRank() const
{
    return _eliminationStep;
}

List<unsigned> SparseConstraintMatrixAnalyzer::getIndependentColumns() const
{
    return _independentColumns;
}

Set<unsigned> SparseConstraintMatrixAnalyzer::getRedundantRows() const
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
