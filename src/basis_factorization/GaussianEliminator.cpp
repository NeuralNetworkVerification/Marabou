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
    , _luFactors( NULL )
{
}

GaussianEliminator::~GaussianEliminator()
{
    // if ( _pivotColumn )
    // {
    //     delete[] _pivotColumn;
    //     _pivotColumn = NULL;
    // }

    // if ( _LCol )
    // {
    //     delete[] _LCol;
    //     _LCol = NULL;
    // }
}

void GaussianEliminator::initializeFactorization()
{
    // Allocate the space
    _luFactors = new LUFactors( _m );
    if ( !_luFactors )
        throw BasisFactorizationError( BasisFactorizationError::ALLOCATION_FAILED,
                                       "GaussianEliminator::luFactors" );

    // In the beginning:
    //
    //   1. P = Q = I
    //   2. V = U = A
    //   3. F = L = I

    memcpy( _luFactors->_V, _A, sizeof(double) * _m * _m );

    std::fill_n( _luFactors->_F, _m * _m, 0 );
    for ( unsigned i = 0; i < _m; ++i )
        _luFactors->_F[i*_m +i] = 1;
}

void GaussianEliminator::permute()
{
    /*
      The element selected for pivoting is V[p,q],
      where p = _pivotRow and _q = _pivotColumn. Because

         V = PUQ

      this translates into some u[i,j]. We want to update P and Q to
      move u[i,j] to position [k,k] in V, where k is the current
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
          Choose a pivot element from the active submatrix of V. This
          can be any non-zero coefficient. Store the result in
          _pivotRow, _pivotColumn.
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

    if ( !found )
        throw BasisFactorizationError( BasisFactorizationError::GAUSSIAN_ELIMINATION_FAILED,
                                       "Couldn't find a pivot" );
}

void GaussianEliminator::eliminate()
{
    /*
      Eliminate all entries below the pivot element U[k,k]
      We know that V[_pivotRow, _pivotColumn] = U[k,k].
    */
}

// unsigned GaussianEliminator::choosePivotElement( unsigned columnIndex, FactorizationStrategy factorizationStrategy )
// {
//     if ( factorizationStrategy != PARTIAL_PIVOTING )
//     {
//         printf( "Error! Only strategy currently supported is partial pivoting!\n" );
//         exit( 1 );
//     }

//     double largestElement = FloatUtils::abs( _pivotColumn[columnIndex] );
//     unsigned bestRowIndex = columnIndex;

//     for ( unsigned i = columnIndex + 1; i < _m; ++i )
//     {
//         double contender = FloatUtils::abs( _pivotColumn[i] );
//         if ( FloatUtils::gt( contender, largestElement ) )
//         {
//             largestElement = contender;
//             bestRowIndex = i;
//         }
//     }

//     // No non-zero pivot has been found, matrix cannot be factorized
//     if ( FloatUtils::isZero( largestElement ) )
//         throw MalformedBasisException();

//     return bestRowIndex;
// }

// void GaussianEliminator::factorize( List<LPElement *> *lp,
//                                     double *U,
//                                     unsigned *rowHeaders,
//                                     FactorizationStrategy factorizationStrategy )
// {
//     // Initialize U to the stored A
// 	memcpy( U, _A, sizeof(double) * _m * _m );

//     // Reset the row headers. The i'th row is stored as the rowHeaders[i]'th row in memory
//     for ( unsigned i = 0; i < _m; ++i )
//         rowHeaders[i] = i;

//     // Iterate over the columns of U
//     for ( unsigned pivotColumnIndex = 0; pivotColumnIndex < _m; ++pivotColumnIndex )
//     {
//         // Extract the current column
//         for ( unsigned i = pivotColumnIndex; i < _m; ++i )
//             _pivotColumn[i] = U[rowHeaders[i] * _m + pivotColumnIndex];

//         // Select the new pivot element according to the factorization strategy
//         unsigned bestRowIndex = choosePivotElement( pivotColumnIndex, factorizationStrategy );

//         // Swap the pivot row to the top of the column if needed
//         if ( bestRowIndex != pivotColumnIndex )
//         {
//             // Swap the rows through the headers
//             unsigned tempIndex = rowHeaders[pivotColumnIndex];
//             rowHeaders[pivotColumnIndex] = rowHeaders[bestRowIndex];
//             rowHeaders[bestRowIndex] = tempIndex;

//             // Also swap in the work vector
//             double tempVal = _pivotColumn[pivotColumnIndex];
//             _pivotColumn[pivotColumnIndex] = _pivotColumn[bestRowIndex];
//             _pivotColumn[bestRowIndex] = tempVal;

//             std::pair<unsigned, unsigned> *P = new std::pair<unsigned, unsigned>( pivotColumnIndex, bestRowIndex );
//             lp->appendHead( new LPElement( NULL, P ) );
//         }

//         // The matrix now has a non-zero value at entry (pivotColumnIndex, pivotColumnIndex),
//         // so we can perform Gaussian elimination for the subsequent rows

//         // TODO: don't need the full LCol, can switch to a dedicated lower-triangular matrix datatype
//         bool eliminationRequired = !FloatUtils::areEqual( _pivotColumn[pivotColumnIndex], 1.0 );
//         std::fill_n( _LCol, _m, 0 );
//         _LCol[pivotColumnIndex] = 1 / _pivotColumn[pivotColumnIndex];
// 		for ( unsigned j = pivotColumnIndex + 1; j < _m; ++j )
//         {
//             if ( !FloatUtils::isZero( _pivotColumn[j] ) )
//             {
//                 eliminationRequired = true;
//                 _LCol[j] = -_pivotColumn[j] * _LCol[pivotColumnIndex];
//             }
//         }

//         if ( eliminationRequired )
//         {
//             // Store the resulting lower-triangular eta matrix
//             EtaMatrix *L = new EtaMatrix( _m, pivotColumnIndex, _LCol );
//             lp->appendHead( new LPElement( L, NULL ) );

//             // Perform the actual elimination step on U. First, perform in-place multiplication
//             // for all rows below the pivot row
//             for ( unsigned row = pivotColumnIndex + 1; row < _m; ++row )
//             {
//                 U[rowHeaders[row] * _m + pivotColumnIndex] = 0.0;
//                 for ( unsigned col = pivotColumnIndex + 1; col < _m; ++col )
//                     U[rowHeaders[row] * _m + col] += L->_column[row] * U[rowHeaders[pivotColumnIndex] * _m + col];
//             }

//             // Finally, perform the multiplication for the pivot row
//             // itself. We change this row last because it is required for all
//             // previous multiplication operations.
//             for ( unsigned i = pivotColumnIndex + 1; i < _m; ++i )
//                 U[rowHeaders[pivotColumnIndex] * _m + i] *= L->_column[pivotColumnIndex];

//             U[rowHeaders[pivotColumnIndex] * _m + pivotColumnIndex] = 1.0;
//         }
// 	}
// }

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
