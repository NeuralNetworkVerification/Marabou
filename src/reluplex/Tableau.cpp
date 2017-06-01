/*********************                                                        */
/*! \file Tableau.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Reluplex project.
 ** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **/

#include "ReluplexError.h"
#include "Tableau.h"

#include <assert.h>
#include <string.h>

Tableau::Tableau()
    : _A( NULL )
    , _b( NULL )
{
}

Tableau::~Tableau()
{
    if ( _A )
    {
        delete[] _A;
        _A = NULL;
    }

    if ( _B )
    {
        delete[] _B;
        _B = NULL;
    }

    if ( _a )
    {
        delete[] _A;
        _A = NULL;
    }

    if ( _b )
    {
        delete[] _b;
        _b = NULL;
    }

    if ( _basicIndexToVariable )
    {
        delete[] _basicIndexToVariable;
        _basicIndexToVariable = NULL;
    }

    if ( _variableToIndex )
    {
        delete[] _variableToIndex;
        _variableToIndex = NULL;
    }

    if ( _nonBasicIndexToVariable )
    {
        delete[] _nonBasicIndexToVariable;
        _nonBasicIndexToVariable = NULL;
    }

    if ( _nonBasicAtUpper )
    {
        delete[] _nonBasicAtUpper;
        _nonBasicAtUpper = NULL;
    }

    if ( _lowerBounds )
    {
        delete[] _lowerBounds;
        _lowerBounds = NULL;
    }

    if ( _upperBounds )
    {
        delete[] _upperBounds;
        _upperBounds = NULL;
    }

    if ( _assignment )
    {
        delete[] _assignment;
        _assignment = NULL;
    }
}

void Tableau::setDimensions( unsigned m, unsigned n )
{
    _m = m;
    _n = n;

    _A = new double[n*m];
    if ( !_A )
        throw ReluplexError( ReluplexError::ALLOCATION_FAILED, "A" );

    _B = new double[m*m];
    if ( !_B )
        throw ReluplexError( ReluplexError::ALLOCATION_FAILED, "B" );

    _a = new double[m];
    if ( !_a )
        throw ReluplexError( ReluplexError::ALLOCATION_FAILED, "a" );

    _basicIndexToVariable = new unsigned[m];
    if ( !_basicIndexToVariable )
        throw ReluplexError( ReluplexError::ALLOCATION_FAILED, "basicIndexToVariable" );

    _variableToIndex = new unsigned[n];
    if ( !_variableToIndex )
        throw ReluplexError( ReluplexError::ALLOCATION_FAILED, "variableToIndex" );

    _nonBasicIndexToVariable = new unsigned[n-m];
    if ( !_nonBasicIndexToVariable )
        throw ReluplexError( ReluplexError::ALLOCATION_FAILED, "nonBasicIndexToVariable" );

    _nonBasicAtUpper = new bool[n-m];
    if ( !_nonBasicAtUpper )
        throw ReluplexError( ReluplexError::ALLOCATION_FAILED, "nonBasicValues" );

    _lowerBounds = new double[n];
    if ( !_lowerBounds )
        throw ReluplexError( ReluplexError::ALLOCATION_FAILED, "lowerBounds" );

    _upperBounds = new double[n];
    if ( !_upperBounds )
        throw ReluplexError( ReluplexError::ALLOCATION_FAILED, "upperBounds" );

    _assignment = new double[m];
    if ( !_assignment )
        throw ReluplexError( ReluplexError::ALLOCATION_FAILED, "assignment" );
}

void Tableau::setEntryValue( unsigned row, unsigned column, double value )
{
    _A[(column * _m) + row] = value;
}

void Tableau::initializeBasis( const Set<unsigned> &basicVariables )
{
    unsigned basicIndex = 0;
    unsigned nonBasicIndex = 0;

    for ( unsigned i = 0; i < _n; ++i )
    {
        if ( basicVariables.exists( i ) )
        {
            _basicIndexToVariable[basicIndex] = i;
            _variableToIndex[i] = basicIndex;
            memcpy( _B + basicIndex * _m, _A + i * _m, sizeof(double) * _m );
            ++basicIndex;
        }
        else
        {
            _nonBasicIndexToVariable[nonBasicIndex] = i;
            _variableToIndex[i] = nonBasicIndex;
            ++nonBasicIndex;
        }
    }

    _basicVariables = basicVariables;
    assert( basicIndex + nonBasicIndex == _n );
}



    /*
      Perform a backward transformation, i.e. find d such that d = inv(B) * a
      where a is the column of the matrix A that corresponds to the entering variable.

      The solution is found by solving Bd = a.

      Bd = (B_0 * E_1 * E_2 ... * E_n) d
         = B_0 ( E_1 ( ... ( E_n d ) ) ) = a
                           -- u_n --
                     ------- u_1 -------
               --------- u_0 -----------


      And the equation is solved iteratively:
      B_0     * u_0   =   a   --> obtain u_0
      E_1     * u_1   =  u_0  --> obtain u_1
      ...
      E_n     * d     =  u_n  --> obtain d


      result needs to be of size m.
    */

void Tableau::backwardTransformation( unsigned enteringVariable, double *result )
{
    // Obtain column a from A, put it in _a
    memcpy( _a, _A + enteringVariable * _m, sizeof(double) * _m );

    // Perform the backward transformation, step by step.
    // For now, assume no eta functions, so just solve B * d = a.

    solveForwardMultiplication( _B, result, _a );
}


/* Extract d for the equation M * d = a. Assume M is upper triangular, and is stored column-wise. */
void Tableau::solveForwardMultiplication( const double *M, double *d, const double *a )
{
    printf( "solveForwardMultiplication: vector a is: " );
    for ( unsigned i = 0; i < _m; ++i )
        printf( "%5.1lf ", a[i] );
    printf( "\n" );

    for ( int i = _m - 1; i >= 0; --i )
    {
        printf( "Starting iteration for: i = %i\n", i );

        d[i] = a[i];
        printf( "\tInitializing: d[i] = a[i] = %5.1lf\n", d[i] );
        for ( unsigned j = i + 1; j < _m; ++j )
        {
            d[i] = d[i] - ( d[j] * M[j * _m + i] );
        }
        printf( "\tAfter summation: d[i] = %5.1lf\n", d[i] );
        d[i] = d[i] / M[i * _m + i];
        printf( "\tAfter division: d[i] = %5.1lf\n", d[i] );
    }

}

void Tableau::setLowerBound( unsigned variable, double value )
{
    assert( variable < _n );
    _lowerBounds[variable] = value;
}

void Tableau::setUpperBound( unsigned variable, double value )
{
    assert( variable < _n );
    _upperBounds[variable] = value;
}

double Tableau::getValue( unsigned variable )
{
    if ( _basicVariables.exists( variable ) )
        return 0.0;

    unsigned index = _variableToIndex[variable];
    if ( _nonBasicAtUpper[index] )
        return _upperBounds[variable];
    else
        return _lowerBounds[variable];
}


// 1  2  3     x1        4
// 0  2  2     x2   =    0
// 0  0 -1     x3        2

//     x3 = 2 / -1 = -2
//     2x2 + 2x3 = 0 -->
//     x2 = 0 - 2(-2)
// private:
//     /* The matrix */
//     double **_A;

//     /* The result vector */
//     double *_b;


// };

//
// Local Variables:
// compile-command: "make -C . "
// tags-file-name: "./TAGS"
// c-basic-offset: 4
// End:
//
