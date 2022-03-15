/*********************                                                        */
/*! \file BoundExplainer.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Omri Isac, Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2022 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]
 **/

#include "BoundExplainer.h"

BoundExplainer::BoundExplainer( unsigned numberOfVariables, unsigned numberOfRows )
    : _numberOfVariables( numberOfVariables )
    , _numberOfRows( numberOfRows )
    , _upperBoundExplanations( _numberOfVariables, Vector<double>( 0 ) )
    , _lowerBoundExplanations( _numberOfVariables, Vector<double>( 0 ) )
{
}

unsigned BoundExplainer::getNumberOfRows() const
{
    return _numberOfRows;
}

unsigned BoundExplainer::getNumberOfVariables() const
{
    return _numberOfVariables;
}

const Vector<double> &BoundExplainer::getExplanation( unsigned var, bool isUpper )
{
    ASSERT ( var < _numberOfVariables );
    return isUpper ? _upperBoundExplanations[var] : _lowerBoundExplanations[var];
}

void BoundExplainer::updateBoundExplanation( const TableauRow &row, bool isUpper )
{
    if ( row._size == 0 )
        return;

    updateBoundExplanation( row, isUpper, row._lhs );
}

void BoundExplainer::updateBoundExplanation( const TableauRow &row, bool isUpper, unsigned var )
{
    if ( row._size == 0 )
        return;

    ASSERT ( var < _numberOfVariables );
    double curCoefficient;
    double ci = 0;
    double realCoefficient;
    bool tempUpper;
    unsigned curVar;

    if ( var != row._lhs )
    {
        for ( unsigned i = 0; i < row._size; ++i )
        {
            if ( row._row[i]._var == var )
            {
                ci = row._row[i]._coefficient;
                break;
            }
        }
    }
    else
        ci = -1;

    ASSERT( !FloatUtils::isZero( ci ) );
    Vector<double> rowCoefficients = Vector<double>( _numberOfRows, 0 );
    Vector<double> sum = Vector<double>( _numberOfRows, 0 );
    Vector<double> tempBound;

    for ( unsigned i = 0; i < row._size; ++i )
    {
        curVar = row._row[i]._var;
        curCoefficient = row._row[i]._coefficient;

        // If the coefficient of the variable is zero then nothing to add to the sum, also skip var
        if ( FloatUtils::isZero( curCoefficient ) || curVar == var )
            continue;

        realCoefficient = curCoefficient / -ci;
        if ( FloatUtils::isZero( realCoefficient ) )
            continue;

        // If we're currently explaining an upper bound, we use upper bound explanation iff variable's coefficient is positive
        // If we're currently explaining a lower bound, we use upper bound explanation iff variable's coefficient is negative
        tempUpper = ( isUpper && realCoefficient > 0 ) || ( !isUpper && realCoefficient < 0 );
        tempBound = tempUpper ? _upperBoundExplanations[curVar] : _lowerBoundExplanations[curVar];
        addVecTimesScalar( sum, tempBound, realCoefficient );
    }

    // Include lhs as well, if needed
    if ( var != row._lhs )
    {
        realCoefficient = 1 / ci;
        if ( !FloatUtils::isZero( realCoefficient ) )
        {
            tempUpper = ( isUpper && realCoefficient > 0 ) || ( !isUpper && realCoefficient < 0 );
            tempBound = tempUpper ? _upperBoundExplanations[row._lhs] : _lowerBoundExplanations[row._lhs];
            addVecTimesScalar( sum, tempBound, realCoefficient );
        }
    }

    // Update according to row coefficients
    extractRowCoefficients( row, rowCoefficients, ci );
    addVecTimesScalar( sum, rowCoefficients, 1 );

    setExplanation( sum, var, isUpper );
}

void BoundExplainer::updateBoundExplanationSparse( const SparseUnsortedList &row, bool isUpper, unsigned var )
{
    if ( row.empty() )
        return;

    ASSERT( var < _numberOfVariables );
    bool tempUpper;
    double curCoefficient;
    double ci = 0;
    double realCoefficient;

    for ( const auto &entry : row )
    {
        if ( entry._index == var )
        {
            ci = entry._value;
            break;
        }
    }

    ASSERT( !FloatUtils::isZero( ci ) );
    Vector<double> rowCoefficients = Vector<double>( _numberOfRows, 0 );
    Vector<double> sum = Vector<double>( _numberOfRows, 0 );
    Vector<double> tempBound;

    for ( const auto &entry : row )
    {
        curCoefficient = entry._value;
        // If coefficient is zero then nothing to add to the sum, also skip var
        if ( FloatUtils::isZero( curCoefficient ) || entry._index == var )
            continue;

        realCoefficient = curCoefficient / -ci;
        if ( FloatUtils::isZero( realCoefficient ) )
            continue;

        // If we're currently explaining an upper bound, we use upper bound explanation iff variable's coefficient is positive
        // If we're currently explaining a lower bound, we use upper bound explanation iff variable's coefficient is negative
        tempUpper = ( isUpper && realCoefficient > 0 ) || ( !isUpper && realCoefficient < 0 );
        tempBound = tempUpper ? _upperBoundExplanations[entry._index] : _lowerBoundExplanations[entry._index];
        addVecTimesScalar( sum, tempBound, realCoefficient );
    }

    // Update according to row coefficients
    extractSparseRowCoefficients( row, rowCoefficients, ci );
    addVecTimesScalar( sum, rowCoefficients, 1 );

    setExplanation( sum, var, isUpper );
}

void BoundExplainer::addVecTimesScalar( Vector<double> &sum, const Vector<double> &input,  double scalar ) const
{
    if ( input.empty() || FloatUtils::isZero( scalar ) )
        return;

    ASSERT( sum.size() == _numberOfRows && input.size() == _numberOfRows );

    for ( unsigned i = 0; i < _numberOfRows; ++i )
        sum[i] += scalar * input[i];
}

void BoundExplainer::extractRowCoefficients( const TableauRow &row, Vector<double> &coefficients, double ci ) const
{
    ASSERT( coefficients.size() == _numberOfRows && row._size <= _numberOfVariables );
    ASSERT( !FloatUtils::isZero( ci ) );

    // The coefficients of the row m highest-indices vars are the coefficients of slack variables
    for ( unsigned i = 0; i < row._size; ++i )
    {
        if ( row._row[i]._var >= _numberOfVariables - _numberOfRows && !FloatUtils::isZero( row._row[i]._coefficient ) )
            coefficients[row._row[i]._var - _numberOfVariables + _numberOfRows] = row._row[i]._coefficient / ci;
    }

    // If the lhs was part of original basis, its coefficient is -1 / ci
    if ( row._lhs >= _numberOfVariables - _numberOfRows )
        coefficients[row._lhs - _numberOfVariables + _numberOfRows] = -1 / ci;
}

void BoundExplainer::extractSparseRowCoefficients( const SparseUnsortedList &row, Vector<double> &coefficients, double ci ) const
{
    ASSERT( coefficients.size() == _numberOfRows );
    ASSERT( !FloatUtils::isZero( ci ) );

    // The coefficients of the row m highest-indices vars are the coefficients of slack variables
    for ( const auto &entry : row )
    {
        if ( entry._index >= _numberOfVariables - _numberOfRows && !FloatUtils::isZero( entry._value ) )
            coefficients[entry._index - _numberOfVariables + _numberOfRows] = entry._value / ci;
    }
}

void BoundExplainer::addVariable()
{
    ++_numberOfRows;
    ++_numberOfVariables;
    _upperBoundExplanations.append( Vector<double>( 0 ) );
    _lowerBoundExplanations.append( Vector<double>( 0 ) );

    for ( unsigned i = 0; i < _numberOfVariables; ++i )
    {
        if ( !_upperBoundExplanations[i].empty() )
            _upperBoundExplanations[i].append( 0 );

        if ( !_lowerBoundExplanations[i].empty() )
            _lowerBoundExplanations[i].append( 0 );
    }
}

void BoundExplainer::resetExplanation( unsigned var, bool isUpper )
{
    ASSERT( var < _numberOfVariables );
    isUpper ? _upperBoundExplanations[var].clear() : _lowerBoundExplanations[var].clear();
}

void BoundExplainer::setExplanation( const Vector<double> &explanation, unsigned var, bool isUpper )
{
    ASSERT( var < _numberOfVariables && (explanation.empty() || explanation.size() == _numberOfRows ) );
    Vector<double> *temp = isUpper ? &_upperBoundExplanations[var] : &_lowerBoundExplanations[var];
    *temp = explanation;
}
