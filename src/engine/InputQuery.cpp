/*********************                                                        */
/*! \file InputQuery.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Andrew Wu
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2024 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

 **/

#include "InputQuery.h"

#include "AutoFile.h"
#include "BilinearConstraint.h"
#include "Debug.h"
#include "FloatUtils.h"
#include "LeakyReluConstraint.h"
#include "MStringf.h"
#include "MarabouError.h"
#include "MaxConstraint.h"
#include "Options.h"
#include "Query.h"
#include "RoundConstraint.h"
#include "SoftmaxConstraint.h"

#define INPUT_QUERY_LOG( x, ... )                                                                  \
    LOG( GlobalConfiguration::INPUT_QUERY_LOGGING, "Marabou Query: %s\n", x )

using namespace CVC4::context;

InputQuery::InputQuery()
    : _userContext()
    , _numberOfVariables( &_userContext )
    , _equations( &_userContext )
    , _lowerBounds( &_userContext )
    , _upperBounds( &_userContext )
    , _plConstraints( &_userContext )
    , _nlConstraints( &_userContext )
    , _variableToInputIndex( &_userContext )
    , _inputIndexToVariable( &_userContext )
    , _variableToOutputIndex( &_userContext )
    , _outputIndexToVariable( &_userContext )
    , _solution( &_userContext )
    , _debuggingSolution( &_userContext )
{
}

InputQuery::~InputQuery()
{
}

void InputQuery::setNumberOfVariables( unsigned numberOfVariables )
{
    _numberOfVariables = numberOfVariables;
}

unsigned InputQuery::getNumberOfVariables() const
{
    return _numberOfVariables;
}

unsigned InputQuery::getNewVariable()
{
    _numberOfVariables = _numberOfVariables + 1;
    return _numberOfVariables - 1;
}

void InputQuery::setLowerBound( unsigned variable, double bound )
{
    if ( variable >= _numberOfVariables )
    {
        throw MarabouError( MarabouError::VARIABLE_INDEX_OUT_OF_RANGE,
                            Stringf( "Variable = %u, number of variables = %u (setLowerBound)",
                                     variable,
                                     getNumberOfVariables() )
                                .ascii() );
    }

    if ( getLevel() > 0 && _lowerBounds.exists( variable ) )
        throw MarabouError(
            MarabouError::INPUT_QUERY_VARIABLE_BOUND_ALREADY_SET,
            Stringf( "Lower bound of variable %u has already been set, consider using "
                     "the tightenLowerBound method",
                     variable )
                .ascii() );

    _lowerBounds[variable] = bound;
}

void InputQuery::setUpperBound( unsigned variable, double bound )
{
    if ( variable >= _numberOfVariables )
    {
        throw MarabouError( MarabouError::VARIABLE_INDEX_OUT_OF_RANGE,
                            Stringf( "Variable = %u, number of variables = %u (setUpperBound)",
                                     variable,
                                     getNumberOfVariables() )
                                .ascii() );
    }

    if ( getLevel() > 0 && _upperBounds.exists( variable ) )
        throw MarabouError(
            MarabouError::INPUT_QUERY_VARIABLE_BOUND_ALREADY_SET,
            Stringf( "Upper bound of variable %u has already been set, consider using "
                     "the tightenUpperBound method",
                     variable )
                .ascii() );

    _upperBounds[variable] = bound;
}

bool InputQuery::tightenLowerBound( unsigned variable, double bound )
{
    if ( variable >= _numberOfVariables )
    {
        throw MarabouError( MarabouError::VARIABLE_INDEX_OUT_OF_RANGE,
                            Stringf( "Variable = %u, number of variables = %u (tightenLowerBound)",
                                     variable,
                                     getNumberOfVariables() )
                                .ascii() );
    }

    if ( !_lowerBounds.exists( variable ) || _lowerBounds[variable] < bound )
    {
        _lowerBounds[variable] = bound;
        return true;
    }
    return false;
}

bool InputQuery::tightenUpperBound( unsigned variable, double bound )
{
    if ( variable >= _numberOfVariables )
    {
        throw MarabouError( MarabouError::VARIABLE_INDEX_OUT_OF_RANGE,
                            Stringf( "Variable = %u, number of variables = %u (tightenUpperBound)",
                                     variable,
                                     getNumberOfVariables() )
                                .ascii() );
    }

    if ( !_upperBounds.exists( variable ) || _upperBounds[variable] > bound )
    {
        _upperBounds[variable] = bound;
        return true;
    }
    return false;
}

double InputQuery::getLowerBound( unsigned variable ) const
{
    if ( variable >= _numberOfVariables )
    {
        throw MarabouError( MarabouError::VARIABLE_INDEX_OUT_OF_RANGE,
                            Stringf( "Variable = %u, number of variables = %u (getLowerBound)",
                                     variable,
                                     getNumberOfVariables() )
                                .ascii() );
    }

    if ( !_lowerBounds.exists( variable ) )
        return FloatUtils::negativeInfinity();

    return _lowerBounds.get( variable );
}

double InputQuery::getUpperBound( unsigned variable ) const
{
    if ( variable >= _numberOfVariables )
    {
        throw MarabouError( MarabouError::VARIABLE_INDEX_OUT_OF_RANGE,
                            Stringf( "Variable = %u, number of variables = %u (getUpperBound)",
                                     variable,
                                     getNumberOfVariables() )
                                .ascii() );
    }

    if ( !_upperBounds.exists( variable ) )
        return FloatUtils::infinity();

    return _upperBounds.get( variable );
}

void InputQuery::addEquation( const Equation &equation )
{
    _equations.push_back( new Equation( equation ) );
}

unsigned InputQuery::getNumberOfEquations() const
{
    return _equations.size();
}

void InputQuery::addPiecewiseLinearConstraint( PiecewiseLinearConstraint *constraint )
{
    _plConstraints.push_back( constraint );
}

void InputQuery::addClipConstraint( unsigned b, unsigned f, double floor, double ceiling )
{
    /*
      f = clip(b, floor, ceiling)
      -
      aux1 = b - floor
      aux2 = relu(aux1)
      aux2.5 = aux2 + floor
      aux3 = -aux2.5 + ceiling
      aux4 = relu(aux3)
      f = -aux4 + ceiling
    */

    // aux1 = var1 - floor
    unsigned aux1 = getNewVariable();
    Equation eq1( Equation::EQ );
    eq1.addAddend( 1.0, b );
    eq1.addAddend( -1.0, aux1 );
    eq1.setScalar( floor );
    addEquation( eq1 );
    unsigned aux2 = getNewVariable();
    PiecewiseLinearConstraint *r1 = new ReluConstraint( aux1, aux2 );
    addPiecewiseLinearConstraint( r1 );
    // aux2.5 = aux2 + floor
    // aux3 = -aux2.5 + ceiling
    // So aux3 = -aux2 - floor + ceiling
    unsigned aux3 = getNewVariable();
    Equation eq2( Equation::EQ );
    eq2.addAddend( -1.0, aux2 );
    eq2.addAddend( -1.0, aux3 );
    eq2.setScalar( floor - ceiling );
    addEquation( eq2 );

    unsigned aux4 = getNewVariable();
    PiecewiseLinearConstraint *r2 = new ReluConstraint( aux3, aux4 );
    addPiecewiseLinearConstraint( r2 );

    // aux4.5 = aux4 - ceiling
    // f = -aux4.5
    // f = -aux4 + ceiling
    Equation eq3( Equation::EQ );
    eq3.addAddend( -1.0, aux4 );
    eq3.addAddend( -1.0, f );
    eq3.setScalar( -ceiling );
    addEquation( eq3 );
}

void InputQuery::addNonlinearConstraint( NonlinearConstraint *constraint )
{
    _nlConstraints.push_back( constraint );
}

void InputQuery::getNonlinearConstraints( Vector<NonlinearConstraint *> &constraints ) const
{
    for ( const auto &constraint : _nlConstraints )
    {
        constraints.append( constraint );
    }
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

List<unsigned> InputQuery::getInputVariables() const
{
    List<unsigned> result;
    for ( const auto &pair : _variableToInputIndex )
        result.append( pair.first );

    return result;
}

List<unsigned> InputQuery::getOutputVariables() const
{
    List<unsigned> result;
    for ( const auto &pair : _variableToOutputIndex )
        result.append( pair.first );

    return result;
}

void InputQuery::setSolutionValue( unsigned variable, double value )
{
    _solution[variable] = value;
}

double InputQuery::getSolutionValue( unsigned variable ) const
{
    if ( !_solution.exists( variable ) )
        throw MarabouError( MarabouError::VARIABLE_DOESNT_EXIST_IN_SOLUTION,
                            Stringf( "Variable: %u", variable ).ascii() );

    return _solution.get( variable );
}

void InputQuery::storeDebuggingSolution( unsigned variable, double value )
{
    _debuggingSolution[variable] = value;
}

void InputQuery::saveQuery( const String &fileName )
{
    Query *query = generateQuery();
    query->saveQuery( fileName );
    delete query;
}

void InputQuery::saveQueryAsSmtLib( const String &fileName ) const
{
    Query *query = generateQuery();
    query->saveQueryAsSmtLib( fileName );
    delete query;
}

Query *InputQuery::generateQuery() const
{
    Query *query = new Query();

    query->setNumberOfVariables( _numberOfVariables );

    for ( const auto &equation : _equations )
        query->addEquation( *equation );

    for ( const auto &pair : _lowerBounds )
        query->setLowerBound( pair.first, pair.second );

    for ( const auto &pair : _upperBounds )
        query->setUpperBound( pair.first, pair.second );

    for ( const auto &c : _plConstraints )
        query->addPiecewiseLinearConstraint( c->duplicateConstraint() );

    for ( const auto &c : _nlConstraints )
        query->addNonlinearConstraint( c->duplicateConstraint() );

    for ( const auto &pair : _variableToInputIndex )
    {
        query->markInputVariable( pair.first, pair.second );
    }

    for ( const auto &pair : _variableToOutputIndex )
    {
        query->markOutputVariable( pair.first, pair.second );
    }

    for ( const auto &pair : _solution )
    {
        query->setSolutionValue( pair.first, pair.second );
    }

    for ( const auto &pair : _debuggingSolution )
    {
        query->storeDebuggingSolution( pair.first, pair.second );
    }

    return query;
}

void InputQuery::dump() const
{
    Query *query = generateQuery();
    query->dump();
    delete query;
}
