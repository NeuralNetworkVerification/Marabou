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
    _inputVariables = other._inputVariables;

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

void InputQuery::markInputVariable( unsigned variable )
{
    _inputVariables.append( variable );
}

List<unsigned> InputQuery::getInputVariables() const
{
    return _inputVariables;
}

void InputQuery::clear()
{
    _numberOfVariables = 0;
    _equations.clear();
    _lowerBounds.clear();
    _upperBounds.clear();
    _solution.clear();
    _inputVariables.clear();

    freeConstraintsIfNeeded();
}

void InputQuery::copyEquatiosnAndBounds( const InputQuery &other )
{
    _equations = other._equations;
    _lowerBounds = other._lowerBounds;
    _upperBounds = other._upperBounds;
}

void InputQuery::dump() const
{
    printf( "Dumping input query:\n" );
    printf( "\tNumber of varibales: %u. Equations: %u. Constraints: %u\n",
            _numberOfVariables, _equations.size(), _plConstraints.size() );
    printf( "\tVariable bounds:\n" );
    for ( unsigned i = 0; i < _numberOfVariables; ++i )
    {
        printf( "\t\tx%u: [", i );

        if ( _lowerBounds[i] == FloatUtils::negativeInfinity() )
            printf( "-INFTY, " );
        else
            printf( "%5.3lf, ", _lowerBounds[i] );

        if ( _upperBounds[i] == FloatUtils::infinity() )
            printf( "+INFTY] " );
        else
            printf( "%5.3lf]", _upperBounds[i] );

        printf( "\n" );
    }

    printf( "\tEquations:\n" );
    for ( const auto &eq : _equations )
    {
        printf( "\t\t" );
        eq.dump();
    }

    printf( "\tConstraints:\n" );
    for ( const auto &constraint : _plConstraints )
    {
        String asString;
        constraint->dump( asString );
        printf( "\t\t%s\n", asString.ascii() );
    }
}

void InputQuery::dumpSolution() const
{
    printf( "Dumping input query's stored solution:\n" );
    for ( unsigned i = 0; i < _numberOfVariables; ++i )
    {
        printf( "\tx%u = %5.3lf\n", i, getSolutionValue( i ) );
    }
}

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
