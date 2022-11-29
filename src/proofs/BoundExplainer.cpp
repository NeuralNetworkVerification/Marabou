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

using namespace CVC4::context;

BoundExplainer::BoundExplainer( unsigned numberOfVariables, unsigned numberOfRows, Context &ctx )
    : _context( ctx ),
      _numberOfVariables( numberOfVariables )
    , _numberOfRows( numberOfRows )
    , _upperBoundExplanations( _numberOfVariables, Vector<CDO<double> *>( 0 ) )
    , _lowerBoundExplanations( _numberOfVariables, Vector<CDO<double> *>( 0 ) )
    , _trivialUpperBoundExplanation( 0 )
    , _trivialLowerBoundExplanation( 0 )
{
     for ( unsigned i = 0; i < _numberOfVariables; ++i )
     {
         for ( unsigned j = 0; j < _numberOfRows; ++j )
         {
            _upperBoundExplanations[i].append( new ( true ) CDO<double>( &ctx, 0 ) );
            _lowerBoundExplanations[i].append( new ( true ) CDO<double>( &ctx, 0 ) );
         }

         _trivialUpperBoundExplanation.append( new ( true ) CDO<bool>( &ctx, true ) ) ;
         _trivialLowerBoundExplanation.append( new ( true ) CDO<bool>( &ctx, true ) );
     }
}

BoundExplainer::~BoundExplainer()
{
    for ( unsigned i = 0; i < _numberOfVariables; ++i )
    {
        for ( unsigned j = 0; j < _numberOfRows; ++j )
        {
            _upperBoundExplanations[i][j]->deleteSelf();
            _lowerBoundExplanations[i][j]->deleteSelf();
        }

        _trivialUpperBoundExplanation[i]->deleteSelf();
        _trivialLowerBoundExplanation[i]->deleteSelf();
    }
}

BoundExplainer &BoundExplainer::operator=( const BoundExplainer &other )
{
    if ( this == &other )
        return *this;

    _numberOfRows = other._numberOfRows;
    _numberOfVariables = other._numberOfVariables;

    for ( unsigned i = 0; i < _numberOfVariables; ++i )
    {
        for ( unsigned j = 0; j < _numberOfRows; ++j )
        {
            _upperBoundExplanations[i][j]->set( other._upperBoundExplanations[i][j]->get() );
            _lowerBoundExplanations[i][j]->set( other._lowerBoundExplanations[i][j]->get() );
        }
        _trivialUpperBoundExplanation[i]->set( other._trivialUpperBoundExplanation[i]->get() );
        _trivialLowerBoundExplanation[i]->set( other._trivialLowerBoundExplanation[i]->get() );
    }

    return *this;
}

unsigned BoundExplainer::getNumberOfRows() const
{
    return _numberOfRows;
}

unsigned BoundExplainer::getNumberOfVariables() const
{
    return _numberOfVariables;
}

const Vector<CDO<double> *> &BoundExplainer::getExplanation( unsigned var, bool isUpper )
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
    Vector<CDO<double> *> tempBound;

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

        if ( ( tempUpper && *_trivialUpperBoundExplanation[curVar] ) || ( !tempUpper && *_trivialLowerBoundExplanation[curVar] ) )
            continue;

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
            if ( !( tempUpper && *_trivialUpperBoundExplanation[row._lhs] ) && !( !tempUpper && *_trivialLowerBoundExplanation[row._lhs] ) )
            {
                tempBound = tempUpper ? _upperBoundExplanations[row._lhs] : _lowerBoundExplanations[row._lhs];
                addVecTimesScalar( sum, tempBound, realCoefficient );
            }
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
    Vector<CDO<double> *> tempBound;

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

        if ( ( tempUpper && *_trivialUpperBoundExplanation[entry._index] ) || ( !tempUpper && *_trivialLowerBoundExplanation[entry._index] ) )
            continue;

        tempBound = tempUpper ? _upperBoundExplanations[entry._index] : _lowerBoundExplanations[entry._index];
        addVecTimesScalar( sum, tempBound, realCoefficient );
    }

    // Update according to row coefficients
    extractSparseRowCoefficients( row, rowCoefficients, ci );
    addVecTimesScalar( sum, rowCoefficients, 1 );

    setExplanation( sum, var, isUpper );
}

void BoundExplainer::addVecTimesScalar( Vector<double> &sum, const Vector<CVC4::context::CDO<double> *> &input,  double scalar ) const
{
    if ( input.empty() || FloatUtils::isZero( scalar ) )
        return;

    ASSERT( sum.size() == _numberOfRows && input.size() == _numberOfRows );

    for ( unsigned i = 0; i < _numberOfRows; ++i )
        sum[i] += scalar * ( *input[i] );
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

    // Add info to all current explanations
    for ( unsigned i = 0; i < _numberOfVariables - 1; ++i )
    {
        _upperBoundExplanations[i].append( new ( true ) CDO<double>( &_context, 0 ) );
        _lowerBoundExplanations[i].append( new ( true ) CDO<double>( &_context, 0 ) );
    }

    // Add a new explanation for the new variable
    _trivialUpperBoundExplanation.append( new ( true ) CDO<bool>( &_context, true ) );
    _trivialLowerBoundExplanation.append( new ( true ) CDO<bool>( &_context, true ) );

    _upperBoundExplanations.append( Vector<CDO<double> *>( 0 ) );
    _lowerBoundExplanations.append( Vector<CDO<double> *>( 0 ) );

    for ( unsigned i = 0; i < _numberOfRows; ++i )
    {
        _upperBoundExplanations[_numberOfVariables - 1].append( new ( true ) CDO<double>( &_context, 0 ) );
        _lowerBoundExplanations[_numberOfVariables - 1].append( new ( true ) CDO<double>( &_context, 0 ) );
    }

    ASSERT( _upperBoundExplanations.size() == _numberOfVariables );
    ASSERT( _upperBoundExplanations[0].size() == _numberOfRows );
    ASSERT( _trivialUpperBoundExplanation.size() == _numberOfVariables );
}

void BoundExplainer::resetExplanation( unsigned var, bool isUpper )
{
    ASSERT( var < _numberOfVariables );
    Vector<CDO<double> *> temp = isUpper ? _upperBoundExplanations[var] : _lowerBoundExplanations[var];

    for ( unsigned i = 0; i < _numberOfRows; ++i )
        temp[i]->set( 0 );

    isUpper ? _trivialUpperBoundExplanation[var]->set( true ) : _trivialLowerBoundExplanation[var]->set( true );
}

void BoundExplainer::setExplanation( const Vector<double> &explanation, unsigned var, bool isUpper )
{
    ASSERT( var < _numberOfVariables && ( explanation.empty() || explanation.size() == _numberOfRows ) );
    Vector<CDO<double> *> temp = isUpper ? _upperBoundExplanations[var] : _lowerBoundExplanations[var];
    for ( unsigned i = 0; i < _numberOfRows; ++i )
        temp[i]->set( explanation[i] );

    isUpper ? _trivialUpperBoundExplanation[var]->set( false ) : _trivialLowerBoundExplanation[var]->set( false );
}

bool BoundExplainer::isExplanationTrivial( unsigned var, bool isUpper ) const
{
    return isUpper ? *_trivialUpperBoundExplanation[var] : *_trivialLowerBoundExplanation[var];
}