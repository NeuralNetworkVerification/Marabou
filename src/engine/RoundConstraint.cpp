/*********************                                                        */
/*! \file RoundConstraint.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Andrew Wu
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2024 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** See the description of the class in RoundConstraint.h.
 **/

#include "RoundConstraint.h"

#include "Debug.h"
#include "FloatUtils.h"
#include "GlobalConfiguration.h"
#include "ITableau.h"
#include "MStringf.h"
#include "MarabouError.h"
#include "NonlinearConstraint.h"
#include "Query.h"
#include "Statistics.h"

#ifdef _WIN32
#define __attribute__( x )
#endif

RoundConstraint::RoundConstraint( unsigned b, unsigned f )
    : NonlinearConstraint()
    , _b( b )
    , _f( f )
    , _haveEliminatedVariables( false )
{
}

RoundConstraint::RoundConstraint( const String &serializedRound )
    : _haveEliminatedVariables( false )
{
    String constraintType = serializedRound.substring( 0, 5 );
    ASSERT( constraintType == String( "round" ) );

    // Remove the constraint type in serialized form
    String serializedValues = serializedRound.substring( 6, serializedRound.length() - 6 );
    List<String> values = serializedValues.tokenize( "," );

    ASSERT( values.size() == 2 );

    auto var = values.begin();
    _f = atoi( var->ascii() );
    ++var;
    _b = atoi( var->ascii() );
}

NonlinearFunctionType RoundConstraint::getType() const
{
    return NonlinearFunctionType::ROUND;
}

NonlinearConstraint *RoundConstraint::duplicateConstraint() const
{
    RoundConstraint *clone = new RoundConstraint( _b, _f );
    *clone = *this;
    return clone;
}

void RoundConstraint::restoreState( const NonlinearConstraint *state )
{
    const RoundConstraint *round = dynamic_cast<const RoundConstraint *>( state );
    *this = *round;
}

void RoundConstraint::registerAsWatcher( ITableau *tableau )
{
    tableau->registerToWatchVariable( this, _b );
    tableau->registerToWatchVariable( this, _f );
}

void RoundConstraint::unregisterAsWatcher( ITableau *tableau )
{
    tableau->unregisterToWatchVariable( this, _b );
    tableau->unregisterToWatchVariable( this, _f );
}

void RoundConstraint::notifyLowerBound( unsigned variable, double newBound )
{
    ASSERT( variable == _b || variable == _f );

    if ( _statistics )
        _statistics->incLongAttribute(
            Statistics::NUM_BOUND_NOTIFICATIONS_TO_TRANSCENDENTAL_CONSTRAINTS );

    if ( tightenLowerBound( variable, newBound ) )
    {
        if ( variable == _f )
        {
            double val = FloatUtils::round( newBound );
            if ( FloatUtils::lt( val, newBound ) )
                val = std::ceil( newBound );
            tightenLowerBound( _f, val );
            tightenLowerBound( _b, val - 0.5 );
        }
        else if ( variable == _b )
        {
            tightenLowerBound( _f, FloatUtils::round( newBound ) );
        }
    }
}

void RoundConstraint::notifyUpperBound( unsigned variable, double newBound )
{
    ASSERT( variable == _b || variable == _f );

    if ( _statistics )
        _statistics->incLongAttribute(
            Statistics::NUM_BOUND_NOTIFICATIONS_TO_TRANSCENDENTAL_CONSTRAINTS );

    if ( tightenUpperBound( variable, newBound ) )
    {
        if ( variable == _f )
        {
            double val = std::round( newBound );
            if ( FloatUtils::gt( val, newBound ) )
                val = std::floor( newBound );
            tightenUpperBound( _f, val );
            tightenUpperBound( _b, val + 0.5 );
        }
        else if ( variable == _b )
            tightenUpperBound( _f, FloatUtils::round( newBound ) );
    }
}

bool RoundConstraint::participatingVariable( unsigned variable ) const
{
    return ( variable == _b ) || ( variable == _f );
}

List<unsigned> RoundConstraint::getParticipatingVariables() const
{
    return List<unsigned>( { _b, _f } );
}

void RoundConstraint::addAuxiliaryEquationsAfterPreprocessing( Query &inputQuery )
{
    // Since at this point we can only encode equality,
    // we encode the following:
    // f - b - aux = 0 (i.e. aux = f - b)
    // aux <= 0.5
    // aux >= -0.5

    // Create the aux variable
    unsigned aux = inputQuery.getNewVariable();

    // Create and add the equation
    Equation equation( Equation::EQ );
    equation.addAddend( 1.0, _f );
    equation.addAddend( -1.0, _b );
    equation.addAddend( -1.0, aux );
    equation.setScalar( 0 );
    inputQuery.addEquation( equation );

    inputQuery.setLowerBound( aux, -0.5 );
    inputQuery.setUpperBound( aux, 0.5 );
}

void RoundConstraint::dump( String &output ) const
{
    output = Stringf( "RoundConstraint: x%u = Round( x%u ).\n", _f, _b );

    output +=
        Stringf( "b in [%s, %s], ",
                 existsLowerBound( _b ) ? Stringf( "%lf", getLowerBound( _b ) ).ascii() : "-inf",
                 existsUpperBound( _b ) ? Stringf( "%lf", getUpperBound( _b ) ).ascii() : "inf" );

    output +=
        Stringf( "f in [%s, %s]",
                 existsLowerBound( _f ) ? Stringf( "%lf", getLowerBound( _f ) ).ascii() : "1",
                 existsUpperBound( _f ) ? Stringf( "%lf", getUpperBound( _f ) ).ascii() : "0" );
}

void RoundConstraint::updateVariableIndex( unsigned oldIndex, unsigned newIndex )
{
    ASSERT( oldIndex == _b || oldIndex == _f );
    ASSERT( !_lowerBounds.exists( newIndex ) && !_upperBounds.exists( newIndex ) &&
            newIndex != _b && newIndex != _f );

    if ( _lowerBounds.exists( oldIndex ) )
    {
        _lowerBounds[newIndex] = _lowerBounds.get( oldIndex );
        _lowerBounds.erase( oldIndex );
    }

    if ( _upperBounds.exists( oldIndex ) )
    {
        _upperBounds[newIndex] = _upperBounds.get( oldIndex );
        _upperBounds.erase( oldIndex );
    }

    if ( oldIndex == _b )
        _b = newIndex;
    else if ( oldIndex == _f )
        _f = newIndex;
}

void RoundConstraint::eliminateVariable( __attribute__( ( unused ) ) unsigned variable,
                                         __attribute__( ( unused ) ) double fixedValue )
{
    ASSERT( variable == _b || variable == _f );

    // In a Round constraint, if a variable is removed the entire constraint can be discarded.
    _haveEliminatedVariables = true;
}

bool RoundConstraint::constraintObsolete() const
{
    return _haveEliminatedVariables;
}

void RoundConstraint::getEntailedTightenings( List<Tightening> &tightenings ) const
{
    ASSERT( existsLowerBound( _b ) && existsLowerBound( _f ) && existsUpperBound( _b ) &&
            existsUpperBound( _f ) );

    double bLowerBound = getLowerBound( _b );
    double fLowerBound = getLowerBound( _f );
    double bUpperBound = getUpperBound( _b );
    double fUpperBound = getUpperBound( _f );

    tightenings.append( Tightening( _b, bLowerBound, Tightening::LB ) );
    tightenings.append( Tightening( _f, fLowerBound, Tightening::LB ) );

    tightenings.append( Tightening( _b, bUpperBound, Tightening::UB ) );
    tightenings.append( Tightening( _f, fUpperBound, Tightening::UB ) );
}

bool RoundConstraint::satisfied() const
{
    if ( !( existsAssignment( _b ) && existsAssignment( _f ) ) )
        throw MarabouError( MarabouError::PARTICIPATING_VARIABLE_MISSING_ASSIGNMENT );

    double bValue = getAssignment( _b );
    double fValue = getAssignment( _f );

    return FloatUtils::areEqual(
        FloatUtils::round( bValue ), fValue, GlobalConfiguration::CONSTRAINT_COMPARISON_TOLERANCE );
}

String RoundConstraint::serializeToString() const
{
    // Output format is: round,f,b
    return Stringf( "round,%u,%u", _f, _b );
}

unsigned RoundConstraint::getB() const
{
    return _b;
}

unsigned RoundConstraint::getF() const
{
    return _f;
}
