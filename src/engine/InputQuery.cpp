/*********************                                                        */
/*! \file InputQuery.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **/

#include "Debug.h"
#include "FloatUtils.h"
#include "InputQuery.h"
#include "MStringf.h"
#include "ReluplexError.h"

InputQuery::InputQuery()
{
}

InputQuery::~InputQuery()
{
    freeConstraintsIfNeeded();
}

void InputQuery::setNumberOfVariables( unsigned numberOfVariables )
{
    _numberOfVariables = numberOfVariables;
}

void InputQuery::setLowerBound( unsigned variable, double bound )
{
    if ( variable >= _numberOfVariables )
    {
        throw ReluplexError( ReluplexError::VARIABLE_INDEX_OUT_OF_RANGE,
                             Stringf( "Variable = %u, number of variables = %u (setLowerBound)",
                                      variable, _numberOfVariables ).ascii() );
    }

    _lowerBounds[variable] = bound;
}

void InputQuery::setUpperBound( unsigned variable, double bound )
{
    if ( variable >= _numberOfVariables )
    {
        throw ReluplexError( ReluplexError::VARIABLE_INDEX_OUT_OF_RANGE,
                             Stringf( "Variable = %u, number of variables = %u (setUpperBound)",
                                      variable, _numberOfVariables ).ascii() );
    }

    _upperBounds[variable] = bound;
}

void InputQuery::addEquation( const Equation &equation )
{
    _equations.append( equation );
}

unsigned InputQuery::getNumberOfVariables() const
{
    return _numberOfVariables;
}

double InputQuery::getLowerBound( unsigned variable ) const
{
    if ( variable >= _numberOfVariables )
    {
        throw ReluplexError( ReluplexError::VARIABLE_INDEX_OUT_OF_RANGE,
                             Stringf( "Variable = %u, number of variables = %u (getLowerBound)",
                                      variable, _numberOfVariables ).ascii() );
    }

    if ( !_lowerBounds.exists( variable ) )
        return FloatUtils::negativeInfinity();

    return _lowerBounds.get( variable );
}

double InputQuery::getUpperBound( unsigned variable ) const
{
    if ( variable >= _numberOfVariables )
    {
        throw ReluplexError( ReluplexError::VARIABLE_INDEX_OUT_OF_RANGE,
                             Stringf( "Variable = %u, number of variables = %u (getUpperBound)",
                                      variable, _numberOfVariables ).ascii() );
    }

    if ( !_upperBounds.exists( variable ) )
        return FloatUtils::infinity();

    return _upperBounds.get( variable );
}

List<Equation> &InputQuery::getEquations()
{
    return _equations;
}

const List<Equation> &InputQuery::getEquations() const
{
    return _equations;
}

void InputQuery::setSolutionValue( unsigned variable, double value )
{
    _solution[variable] = value;
}

double InputQuery::getSolutionValue( unsigned variable ) const
{
    if ( !_solution.exists( variable ) )
        throw ReluplexError( ReluplexError::VARIABLE_DOESNT_EXIST_IN_SOLUTION,
                             Stringf( "Variable: %u", variable ).ascii() );

    return _solution.get( variable );
}

void InputQuery::addPiecewiseLinearConstraint( PiecewiseLinearConstraint *constraint )
{
    _plConstraints.append( constraint );
}

List<PiecewiseLinearConstraint *> &InputQuery::getPiecewiseLinearConstraints()
{
    return _plConstraints;
}

const List<PiecewiseLinearConstraint *> &InputQuery::getPiecewiseLinearConstraints() const
{
    return _plConstraints;
}

unsigned InputQuery::countInfiniteBounds()
{
    unsigned result = 0;

    for ( const auto &lowerBound : _lowerBounds )
        if ( lowerBound.second == FloatUtils::negativeInfinity() )
            ++result;

    for ( const auto &upperBound : _upperBounds )
        if ( upperBound.second == FloatUtils::infinity() )
            ++result;

    for ( unsigned i = 0; i < _numberOfVariables; ++i )
    {
        if ( !_lowerBounds.exists( i ) )
            ++result;
        if ( !_upperBounds.exists( i ) )
            ++result;
    }

    return result;
}

void InputQuery::mergeIdenticalVariables( unsigned v1, unsigned v2 )
{
    // Handle equations
    for ( auto &equation : getEquations() )
        equation.updateVariableIndex( v1, v2 );

    // Handle PL constraints
    for ( auto &plConstraint : getPiecewiseLinearConstraints() )
    {
        if ( plConstraint->participatingVariable( v1 ) )
        {
            ASSERT( !plConstraint->participatingVariable( v2 ) );
            plConstraint->updateVariableIndex( v1, v2 );
        }
    }

    // TODO: update lower and upper bounds
}

void InputQuery::removeEquation( Equation e )
{
    _equations.erase( e );
}

InputQuery &InputQuery::operator=( const InputQuery &other )
{
    _numberOfVariables = other._numberOfVariables;
    _equations = other._equations;
    _lowerBounds = other._lowerBounds;
    _upperBounds = other._upperBounds;
    _solution = other._solution;
    _debuggingSolution = other._debuggingSolution;

    freeConstraintsIfNeeded();
    for ( const auto &constraint : other._plConstraints )
        _plConstraints.append( constraint->duplicateConstraint() );

    return *this;
}

InputQuery::InputQuery( const InputQuery &other )
{
    *this = other;
}

void InputQuery::freeConstraintsIfNeeded()
{
    for ( auto &it : _plConstraints )
        delete it;

    _plConstraints.clear();
}

const Map<unsigned, double> &InputQuery::getLowerBounds() const
{
    return _lowerBounds;
}

const Map<unsigned, double> &InputQuery::getUpperBounds() const
{
    return _upperBounds;
}

void InputQuery::storeDebuggingSolution( unsigned variable, double value )
{
    _debuggingSolution[variable] = value;
}

void InputQuery::markInputVariable( unsigned variable, unsigned inputIndex )
{
    _variableToInputIndex[variable] = inputIndex;
    _inputIndexToVariable[inputIndex] = variable;
}

void InputQuery::markOutputVariable( unsigned variable, unsigned outputIndex )
{
    _variableToOutputIndex[variable] = outputIndex;
    _outputIndexToVariable[outputIndex] = variable;
}

unsigned InputQuery::inputVariableByIndex( unsigned index ) const
{
    ASSERT( _inputIndexToVariable.exists( index ) );
    return _inputIndexToVariable.get( index );
}

unsigned InputQuery::outputVariableByIndex( unsigned index ) const
{
    ASSERT( _outputIndexToVariable.exists( index ) );
    return _outputIndexToVariable.get( index );
}

unsigned InputQuery::getNumInputVariables() const
{
    return _inputIndexToVariable.size();
}

unsigned InputQuery::getNumOutputVariables() const
{
    return _outputIndexToVariable.size();
}

void InputQuery::printInputOutputBounds() const
{
    printf( "Dumping bounds of the input and output variables:\n" );

    for ( const auto &pair : _variableToInputIndex )
    {
        printf( "\tInput %u (var %u): [%lf, %lf]\n",
                pair.second,
                pair.first,
                _lowerBounds[pair.first],
                _upperBounds[pair.first] );
    }

    for ( const auto &pair : _variableToOutputIndex )
    {
        printf( "\tOutput %u (var %u): [%lf, %lf]\n",
                pair.second,
                pair.first,
                _lowerBounds[pair.first],
                _upperBounds[pair.first] );
    }
}

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
