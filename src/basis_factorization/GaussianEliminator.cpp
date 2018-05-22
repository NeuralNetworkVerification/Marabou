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
#include "MalformedBasisException.h"

#include <cstdio>

GaussianEliminator::GaussianEliminator( const double *A, unsigned m )
    : _A( A )
    , _m( m )
    , _pivotColumn( NULL )
    , _LCol( NULL )
{
    _pivotColumn = new double[_m];
    if ( !_pivotColumn )
        throw BasisFactorizationError( BasisFactorizationError::ALLOCATION_FAILED,
                                       "GaussianEliminator::pivotColumn" );

    _LCol = new double[_m];
    if ( !_LCol )
        throw BasisFactorizationError( BasisFactorizationError::ALLOCATION_FAILED, "LUFactorization::LCol" );
}

GaussianEliminator::~GaussianEliminator()
{
    if ( _pivotColumn )
    {
        delete[] _pivotColumn;
        _pivotColumn = NULL;
    }

    if ( _LCol )
    {
        delete[] _LCol;
        _LCol = NULL;
    }
}

unsigned GaussianEliminator::choosePivotElement( unsigned columnIndex, FactorizationStrategy factorizationStrategy )
{
    if ( factorizationStrategy != PARTIAL_PIVOTING )
    {
        printf( "Error! Only strategy currently supported is partial pivoting!\n" );
        exit( 1 );
    }

    double largestElement = FloatUtils::abs( _pivotColumn[columnIndex] );
    unsigned bestRowIndex = columnIndex;

    for ( unsigned i = columnIndex + 1; i < _m; ++i )
    {
        double contender = FloatUtils::abs( _pivotColumn[i] );
        if ( FloatUtils::gt( contender, largestElement ) )
        {
            largestElement = contender;
            bestRowIndex = i;
        }
    }

    // No non-zero pivot has been found, matrix cannot be factorized
    if ( FloatUtils::isZero( largestElement ) )
        throw MalformedBasisException();

    return bestRowIndex;
}

void GaussianEliminator::factorize( List<LPElement *> *lp,
                                    double *U,
                                    unsigned *rowHeaders,
                                    FactorizationStrategy factorizationStrategy )
{
    // Initialize U to the stored A
	memcpy( U, _A, sizeof(double) * _m * _m );

    // Reset the row headers. The i'th row is stored as the rowHeaders[i]'th row in memory
    for ( unsigned i = 0; i < _m; ++i )
        rowHeaders[i] = i;

    // Iterate over the columns of U
    for ( unsigned pivotColumnIndex = 0; pivotColumnIndex < _m; ++pivotColumnIndex )
    {
        // Extract the current column
        for ( unsigned i = pivotColumnIndex; i < _m; ++i )
            _pivotColumn[i] = U[rowHeaders[i] * _m + pivotColumnIndex];

        // Select the new pivot element according to the factorization strategy
        unsigned bestRowIndex = choosePivotElement( pivotColumnIndex, factorizationStrategy );

        // Swap the pivot row to the top of the column if needed
        if ( bestRowIndex != pivotColumnIndex )
        {
            // Swap the rows through the headers
            unsigned tempIndex = rowHeaders[pivotColumnIndex];
            rowHeaders[pivotColumnIndex] = rowHeaders[bestRowIndex];
            rowHeaders[bestRowIndex] = tempIndex;

            // Also swap in the work vector
            double tempVal = _pivotColumn[pivotColumnIndex];
            _pivotColumn[pivotColumnIndex] = _pivotColumn[bestRowIndex];
            _pivotColumn[bestRowIndex] = tempVal;

            std::pair<unsigned, unsigned> *P = new std::pair<unsigned, unsigned>( pivotColumnIndex, bestRowIndex );
            lp->appendHead( new LPElement( NULL, P ) );
        }

        // The matrix now has a non-zero value at entry (pivotColumnIndex, pivotColumnIndex),
        // so we can perform Gaussian elimination for the subsequent rows

        // TODO: don't need the full LCol, can switch to a dedicated lower-triangular matrix datatype
        bool eliminationRequired = !FloatUtils::areEqual( _pivotColumn[pivotColumnIndex], 1.0 );
        std::fill_n( _LCol, _m, 0 );
        _LCol[pivotColumnIndex] = 1 / _pivotColumn[pivotColumnIndex];
		for ( unsigned j = pivotColumnIndex + 1; j < _m; ++j )
        {
            if ( !FloatUtils::isZero( _pivotColumn[j] ) )
            {
                eliminationRequired = true;
                _LCol[j] = -_pivotColumn[j] * _LCol[pivotColumnIndex];
            }
        }

        if ( eliminationRequired )
        {
            // Store the resulting lower-triangular eta matrix
            EtaMatrix *L = new EtaMatrix( _m, pivotColumnIndex, _LCol );
            lp->appendHead( new LPElement( L, NULL ) );

            // Perform the actual elimination step on U. First, perform in-place multiplication
            // for all rows below the pivot row
            for ( unsigned row = pivotColumnIndex + 1; row < _m; ++row )
            {
                U[rowHeaders[row] * _m + pivotColumnIndex] = 0.0;
                for ( unsigned col = pivotColumnIndex + 1; col < _m; ++col )
                    U[rowHeaders[row] * _m + col] += L->_column[row] * U[rowHeaders[pivotColumnIndex] * _m + col];
            }

            // Finally, perform the multiplication for the pivot row
            // itself. We change this row last because it is required for all
            // previous multiplication operations.
            for ( unsigned i = pivotColumnIndex + 1; i < _m; ++i )
                U[rowHeaders[pivotColumnIndex] * _m + i] *= L->_column[pivotColumnIndex];

            U[rowHeaders[pivotColumnIndex] * _m + pivotColumnIndex] = 1.0;
        }
	}
}

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
