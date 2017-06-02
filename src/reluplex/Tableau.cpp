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

#include "Debug.h"
#include "FloatUtils.h"
#include "ReluplexError.h"
#include "Tableau.h"

#include <string.h>

Tableau::Tableau()
    : _A( NULL )
    , _B( NULL )
    , _AN( NULL )
    , _a( NULL )
    , _b( NULL )
    , _costFunction( NULL )
    , _basicIndexToVariable( NULL )
    , _nonBasicIndexToVariable( NULL )
    , _variableToIndex( NULL )
    , _nonBasicAtUpper( NULL )
    , _lowerBounds( NULL )
    , _upperBounds( NULL )
    , _assignment( NULL )
    , _basicStatus( NULL )
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

    if ( _AN )
    {
        delete[] _AN;
        _AN = NULL;
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

    if ( _costFunction )
    {
        delete[] _costFunction;
        _costFunction = NULL;
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

    if ( _basicStatus )
    {
        delete[] _basicStatus;
        _basicStatus = NULL;
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

    _AN = new double[m * (n-m)];
    if ( !_AN )
        throw ReluplexError( ReluplexError::ALLOCATION_FAILED, "AN" );

    _a = new double[m];
    if ( !_a )
        throw ReluplexError( ReluplexError::ALLOCATION_FAILED, "a" );

    _b = new double[m];
    if ( !_b )
        throw ReluplexError( ReluplexError::ALLOCATION_FAILED, "b" );

    _costFunction = new double[n-m];
    if ( !_costFunction )
        throw ReluplexError( ReluplexError::ALLOCATION_FAILED, "costFunction" );

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

    _basicStatus = new unsigned[m];
    if ( !_basicStatus )
        throw ReluplexError( ReluplexError::ALLOCATION_FAILED, "basicStatus" );
}

void Tableau::setEntryValue( unsigned row, unsigned column, double value )
{
    _A[(column * _m) + row] = value;
}

void Tableau::initializeBasis( const Set<unsigned> &basicVariables )
{
    unsigned basicIndex = 0;
    unsigned nonBasicIndex = 0;

    _basicVariables = basicVariables;

    // Assign variable indices, grab basic columns from A into B
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
            memcpy( _AN + nonBasicIndex * _m, _A + i * _m, sizeof(double) * _m );
            ++nonBasicIndex;
        }
    }
    ASSERT( basicIndex + nonBasicIndex == _n );

    // Set non-basics to lower bounds
    std::fill( _nonBasicAtUpper, _nonBasicAtUpper + _n - _m, false );

    // Recompute assignment
    computeAssignment();
}

void Tableau::computeAssignment()
{
    for ( unsigned i = 0; i < _m; ++i )
    {
        double result = _b[i];
        for ( unsigned j = 0; j < _n - _m; ++j )
        {
            double nonBasicValue = _nonBasicAtUpper[j] ?
                _upperBounds[_nonBasicIndexToVariable[j]] :
                _lowerBounds[_nonBasicIndexToVariable[j]];

            result -= ( ( _AN[(j * _m) + i] * nonBasicValue ) / _B[(i * _m) + i] );
        }

        _assignment[i] = result;
        computeBasicStatus( i );
    }
}

void Tableau::computeBasicStatus( unsigned basic )
{
    double ub = _upperBounds[_basicIndexToVariable[basic]];
    double lb = _lowerBounds[_basicIndexToVariable[basic]];
    double value = _assignment[basic];

    if ( FloatUtils::gt( value , ub ) )
        _basicStatus[basic] = Tableau::ABOVE_UB;
    else if ( FloatUtils::lt( value , lb ) )
        _basicStatus[basic] = Tableau::BELOW_LB;
    else if ( FloatUtils::areEqual( lb, ub ) )
        _basicStatus[basic] = Tableau::FIXED;
    else if ( FloatUtils::areEqual( ub, value ) )
        _basicStatus[basic] = Tableau::AT_UB;
    else if ( FloatUtils::areEqual( lb, value ) )
        _basicStatus[basic] = Tableau::AT_LB;
    else
        _basicStatus[basic] = Tableau::BETWEEN;
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
    ASSERT( variable < _n );
    _lowerBounds[variable] = value;
}

void Tableau::setUpperBound( unsigned variable, double value )
{
    ASSERT( variable < _n );
    _upperBounds[variable] = value;
}

double Tableau::getValue( unsigned variable )
{
    if ( _basicVariables.exists( variable ) )
        return _assignment[_variableToIndex[variable]];

    unsigned index = _variableToIndex[variable];
    return _nonBasicAtUpper[index] ? _upperBounds[variable] : _lowerBounds[variable];
}


void Tableau::setRightHandSide( const double *b )
{
    memcpy( _b, b, sizeof(double) * _m );
}

const double *Tableau::getCostFunction()
{
    computeCostFunction();
    return _costFunction;
}

bool Tableau::basicOutOfBounds( unsigned basic )
{
    return basicTooHigh( basic ) || basicTooLow( basic );
}

bool Tableau::basicTooLow( unsigned basic )
{
    return _basicStatus[basic] == Tableau::BELOW_LB;
}

bool Tableau::basicTooHigh( unsigned basic )
{
    return _basicStatus[basic] == Tableau::ABOVE_UB;
}

void Tableau::computeCostFunction()
{
    std::fill( _costFunction, _costFunction + _n - _m, 0.0 );

    for ( unsigned i = 0; i < _m; ++i )
    {
        // Currently assume the basic matrix is diagonal.
        // If the variable is too low, We want to add -row, but the
        // equation is given as x = -An*xn so the two negations cancel
        // out. The too high case is symmetrical.
        if ( basicTooLow( i ) )
            addRowToCostFunction( i, 1 );
        else if ( basicTooHigh( i ) )
            addRowToCostFunction( i, -1 );
    }
}

void Tableau::addRowToCostFunction( unsigned row, double weight )
{
    // TODO: we assume B is the identity matrix, otherwise might need
    // to divide by the coefficient from B.
    for ( unsigned i = 0; i < _n - _m; ++i )
        _costFunction[i] += ( _AN[i * _m + row] * weight );
}

unsigned Tableau::getBasicStatus( unsigned basic )
{
    return _basicStatus[_variableToIndex[basic]];
}

unsigned Tableau::pickEnteringVariable()
{
    computeCostFunction();

    // Dantzig's rule
    unsigned maxIndex = 0;
    double maxValue = FloatUtils::abs( _costFunction[maxIndex] );

    for ( unsigned i = 1; i < _n - _m; ++i )
    {
        double contender = FloatUtils::abs( _costFunction[i] );
        if ( FloatUtils::gt( contender, maxValue ) )
        {
            maxIndex = i;
            maxValue = contender;
        }
    }

    return _nonBasicIndexToVariable[maxIndex];
}

//
// Local Variables:
// compile-command: "make -C . "
// tags-file-name: "./TAGS"
// c-basic-offset: 4
// End:
//
