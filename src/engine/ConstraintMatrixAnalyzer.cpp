/*********************                                                        */
/*! \file ConstraintMatrixAnalyzer.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **/

#include "ConstraintMatrixAnalyzer.h"
#include "FloatUtils.h"
#include "List.h"
#include "MStringf.h"

ConstraintMatrixAnalyzer::ConstraintMatrixAnalyzer()
    : _matrix( NULL )
    , _work( NULL )
    , _logging( false )
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
}

void ConstraintMatrixAnalyzer::analyze( const double *matrix, unsigned m, unsigned n )
{
    freeMemoryIfNeeded();

    _m = m;
    _n = n;
    _matrix = new double[m * n];
    _work = new double[n];

    // Initialize the local copy of the matrix. Switch from column-major
    // to row-major.
    for ( unsigned i = 0; i < n*m; ++i )
    {
        unsigned row = i % m;
        unsigned col = i / m;
        _matrix[row * n + col] = matrix[i];
    }

    gaussianElimination();
}

void ConstraintMatrixAnalyzer::gaussianElimination()
{
    /*
      We work column-by-column, until m columns are found
      that are non-zeroes, or until we run out of columns.
    */
    _rank = 0;
    unsigned currentColumn = 0;
    while ( currentColumn < _n && _rank < _m )
    {
        dumpMatrix( Stringf( "Begin iteration %u", currentColumn ) );
        unsigned largestPivotRow = _rank;
        double largestPivot = FloatUtils::abs( _matrix[largestPivotRow * _n + currentColumn] );

        for ( unsigned i = _rank + 1; i < _m; ++i )
        {
            double contender = FloatUtils::abs( _matrix[i * _n + currentColumn] );
            if ( FloatUtils::gt( contender, largestPivot ) )
            {
                largestPivotRow = i;
                largestPivot = contender;
            }
        }

        // If the entire column is zeros. Nothing to be done, move on to next column.
        if ( FloatUtils::isZero( largestPivot ) )
        {
            ++currentColumn;
            continue;
        }

        // Move the pivot row to its new index
        if ( largestPivotRow != _rank )
            swapRows( largestPivotRow, _rank );

        // Eliminate the rows below the pivot row
        for ( unsigned i = _rank + 1; i < _m; ++i )
        {
            double factor =
                - _matrix[i * _n + currentColumn] / _matrix[_rank * _n + currentColumn];
            _matrix[i * _n + currentColumn] = 0;

            for ( unsigned j = currentColumn + 1; j < _n; ++j )
                _matrix[i * _n + j] += (factor * _matrix[_rank * _n + j]);
        }

        _independentColumns.append( currentColumn );
        ++currentColumn;
        ++_rank;
    }

    dumpMatrix( "Elimination finished" );
}

List<unsigned> ConstraintMatrixAnalyzer::getIndependentColumns() const
{
    return _independentColumns;
}

void ConstraintMatrixAnalyzer::swapRows( unsigned i, unsigned j )
{
    memcpy( _work, _matrix + (i * _n), sizeof(double) * _n );
    memcpy( _matrix + (i * _n), _matrix + ( j * _n ), sizeof(double) * _n );
    memcpy( _matrix + (j * _n), _work, sizeof(double) * _n );
}

const double *ConstraintMatrixAnalyzer::getCanonicalForm()
{
    return _matrix;
}

unsigned ConstraintMatrixAnalyzer::getRank() const
{
    return _rank;
}

void ConstraintMatrixAnalyzer::dumpMatrix( const String &message )
{
    if ( _logging )
    {
        printf( "\nConstraintMatrixAnalyzer::Dumping constraint matrix ");

        if ( message.length() > 0 )
            printf( "(%s)\n", message.ascii() );
        else
            printf( "\n" );

        for ( unsigned i = 0; i < _m; ++i )
        {
            printf( "\t" );
            for ( unsigned j = 0; j < _n; ++j )
                printf( "%.2lf ", _matrix[i*_n + j] );
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
