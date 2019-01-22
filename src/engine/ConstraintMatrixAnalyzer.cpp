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
    : _matrix( NULL )
    , _work( NULL )
    , _logging( false )
    , _rowHeaders( NULL )
    , _columnHeaders( NULL )
{
}

ConstraintMatrixAnalyzer::~ConstraintMatrixAnalyzer()
{
    freeMemoryIfNeeded();
}

void ConstraintMatrixAnalyzer::freeMemoryIfNeeded()
{
    if ( _matrix )
    {
        delete[] _matrix;
        _matrix = NULL;
    }

    if ( _work )
    {
        delete[] _work;
        _work = NULL;
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
}

void ConstraintMatrixAnalyzer::analyze( const SparseMatrix *matrix, unsigned m, unsigned n )
{
    freeMemoryIfNeeded();

    _m = m;
    _n = n;

    _matrix = new double[m * n];
    _work = new double[n];
    _rowHeaders = new unsigned[m];
    _columnHeaders = new unsigned[n];

    matrix->toDense( _matrix );

    // Initialize the row and column headers
    for ( unsigned i = 0; i < _m; ++i )
        _rowHeaders[i] = i;

    for ( unsigned i = 0; i < _n; ++i )
        _columnHeaders[i] = i;

    gaussianElimination();
}

void ConstraintMatrixAnalyzer::analyze( const double *matrix, unsigned m, unsigned n )
{
    freeMemoryIfNeeded();

    _m = m;
    _n = n;

    _matrix = new double[m * n];
    _work = new double[n];
    _rowHeaders = new unsigned[m];
    _columnHeaders = new unsigned[n];

    // Initialize the local copy of the matrix. Switch from column-major
    // to row-major.
    for ( unsigned row = 0; row < _m; ++row )
    {
        for ( unsigned column = 0; column < _n; ++column )
        {
            _matrix[row * n + column] = matrix[column * _m + row];
        }
    }

    // Initialize the row and column headers
    for ( unsigned i = 0; i < _m; ++i )
        _rowHeaders[i] = i;

    for ( unsigned i = 0; i < _n; ++i )
        _columnHeaders[i] = i;

    gaussianElimination();
}

void ConstraintMatrixAnalyzer::gaussianElimination()
{
    /*
      We work column-by-column, until m columns are found
      that are non-zeroes, or until we run out of columns.
    */
    _eliminationStep = 0;

    while ( _eliminationStep < _m )
    {
        // Find largest pivot in active submatrix
        double largestPivot = 0.0;
        unsigned bestRow = 0, bestColumn = 0;
        for ( unsigned i = _eliminationStep; i < _m; ++i )
        {
            for ( unsigned j = _eliminationStep; j < _n; ++j )
            {
                double contender = FloatUtils::abs( _matrix[_rowHeaders[i]*_n + _columnHeaders[j]] );
                if ( FloatUtils::gt( contender, largestPivot ) )
                {
                    largestPivot = contender;
                    bestRow = i;
                    bestColumn = j;
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

        // Eliminate the rows below the pivot row
        double invPivotEntry = 1 / _matrix[_rowHeaders[_eliminationStep]*_n + _columnHeaders[_eliminationStep]];

        for ( unsigned i = _eliminationStep + 1; i < _m; ++i )
        {
            double factor =
                -_matrix[_rowHeaders[i]*_n + _columnHeaders[_eliminationStep]] * invPivotEntry ;
            _matrix[_rowHeaders[i]*_n + _columnHeaders[_eliminationStep]] = 0;

            for ( unsigned j = _eliminationStep + 1; j < _n; ++j )
                _matrix[_rowHeaders[i]*_n + _columnHeaders[j]] +=
                    (factor * _matrix[_rowHeaders[_eliminationStep]*_n + _columnHeaders[j]]);
        }

        _independentColumns.append( _columnHeaders[_eliminationStep] );
        ++_eliminationStep;
    }

    dumpMatrix( "Elimination finished" );
}

List<unsigned> ConstraintMatrixAnalyzer::getIndependentColumns() const
{
    return _independentColumns;
}

void ConstraintMatrixAnalyzer::swapRows( unsigned i, unsigned j )
{
    unsigned temp = _rowHeaders[i];
    _rowHeaders[i] = _rowHeaders[j];
    _rowHeaders[j] = temp;
}

void ConstraintMatrixAnalyzer::swapColumns( unsigned i, unsigned j )
{
    unsigned temp = _columnHeaders[i];
    _columnHeaders[i] = _columnHeaders[j];
    _columnHeaders[j] = temp;
}

void ConstraintMatrixAnalyzer::getCanonicalForm( double *matrix )
{
    for ( unsigned i = 0; i < _m; ++i )
        for ( unsigned j = 0; j < _n; ++j )
            matrix[i*_n + j] = _matrix[_rowHeaders[i]*_n + _columnHeaders[j]];
}

unsigned ConstraintMatrixAnalyzer::getRank() const
{
    return _eliminationStep;
}

void ConstraintMatrixAnalyzer::dumpMatrix( const String &message )
{
    if ( _logging )
    {
        printf( "\nConstraintMatrixAnalyzer::Dumping constraint matrix" );

        if ( message.length() > 0 )
            printf( "(%s)\n", message.ascii() );
        else
            printf( "\n" );

        for ( unsigned i = 0; i < _m; ++i )
        {
            printf( "\t" );
            for ( unsigned j = 0; j < _n; ++j )
                printf( "%.2lf ", _matrix[_rowHeaders[i]*_n + _columnHeaders[j]] );
            printf( "\n" );
        }

        printf( "\n" );
    }
}

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
