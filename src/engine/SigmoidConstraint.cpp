/*********************                                                        */
/*! \file SigmoidConstraint.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Teruhiro Tagomori
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** See the description of the class in SigmoidConstraint.h.
 **/

#include "SigmoidConstraint.h"

#include "ConstraintBoundTightener.h"
#include "TranscendentalConstraint.h"
#include "Debug.h"
#include "DivideStrategy.h"
#include "FloatUtils.h"
#include "GlobalConfiguration.h"
#include "ITableau.h"
#include "InputQuery.h"
#include "MStringf.h"
#include "MarabouError.h"
#include "Statistics.h"
#include "TableauRow.h"

#ifdef _WIN32
#define __attribute__(x)
#endif

SigmoidConstraint::SigmoidConstraint( unsigned b, unsigned f )
    : TranscendentalConstraint()
    , _b( b )
    , _f( f )
    , _haveEliminatedVariables( false )
{
}

SigmoidConstraint::SigmoidConstraint( const String &serializedSigmoid )
    : _haveEliminatedVariables( false )
{
    String constraintType = serializedSigmoid.substring( 0, 7 );
    ASSERT( constraintType == String( "sigmoid" ) );

    // Remove the constraint type in serialized form
    String serializedValues = serializedSigmoid.substring( 8, serializedSigmoid.length() - 5 );
    List<String> values = serializedValues.tokenize( "," );

    ASSERT( values.size() == 2 );

    auto var = values.begin();
    _f = atoi( var->ascii() );
    ++var;
    _b = atoi( var->ascii() );
}

TranscendentalFunctionType SigmoidConstraint::getType() const
{
    return TranscendentalFunctionType::SIGMOID;
}

TranscendentalConstraint *SigmoidConstraint::duplicateConstraint() const
{
    SigmoidConstraint *clone = new SigmoidConstraint( _b, _f );
    *clone = *this;
    return clone;
}

void SigmoidConstraint::restoreState( const TranscendentalConstraint *state )
{
    const SigmoidConstraint *sigmoid = dynamic_cast<const SigmoidConstraint *>( state );
    *this = *sigmoid;
}

void SigmoidConstraint::registerAsWatcher( ITableau *tableau )
{
    tableau->registerToWatchVariable( this, _b );
    tableau->registerToWatchVariable( this, _f );
}

void SigmoidConstraint::unregisterAsWatcher( ITableau *tableau )
{
    tableau->unregisterToWatchVariable( this, _b );
    tableau->unregisterToWatchVariable( this, _f );
}

void SigmoidConstraint::notifyVariableValue( unsigned variable, double value )
{
    ASSERT( variable == _b || variable == _f );

    _assignment[variable] = value;
}

void SigmoidConstraint::notifyLowerBound( unsigned variable, double bound )
{
    ASSERT( variable == _b || variable == _f );

    if ( _statistics )
        _statistics->incLongAttribute( Statistics::NUM_BOUND_NOTIFICATIONS_TO_TRANSCENDENTAL_CONSTRAINTS );

    if ( existsLowerBound( variable ) && !FloatUtils::gt( bound, getLowerBound( variable ) ) )
        return;

    setLowerBound( variable, bound );

    if ( _constraintBoundTightener )
    {
        if ( variable == _f )
            _constraintBoundTightener->registerTighterLowerBound( _b, sigmoidInverse(bound) );
        else if ( variable == _b )
            _constraintBoundTightener->registerTighterLowerBound( _f, sigmoid(bound) );
    }
}

void SigmoidConstraint::notifyUpperBound( unsigned variable, double bound )
{
    ASSERT( variable == _b || variable == _f );

    if ( _statistics )
        _statistics->incLongAttribute( Statistics::NUM_BOUND_NOTIFICATIONS_TO_TRANSCENDENTAL_CONSTRAINTS );

    if ( existsUpperBound( variable ) && !FloatUtils::lt( bound, getUpperBound( variable ) ) )
        return;

    setUpperBound( variable, bound );

    if ( _constraintBoundTightener )
    {
        if ( variable == _f )
            _constraintBoundTightener->registerTighterUpperBound( _b, sigmoidInverse(bound) );
        else if ( variable == _b )
            _constraintBoundTightener->registerTighterUpperBound( _f, sigmoid(bound) );
    }
}

bool SigmoidConstraint::participatingVariable( unsigned variable ) const
{
    return ( variable == _b ) || ( variable == _f );
}

List<unsigned> SigmoidConstraint::getParticipatingVariables() const
{
    return List<unsigned>( { _b, _f } );
}

void SigmoidConstraint::dump( String &output ) const
{
    output = Stringf( "SigmoidConstraint: x%u = Sigmoid( x%u ).\n", _f, _b );

    output += Stringf( "b in [%s, %s], ",
                       existsLowerBound( _b ) ? Stringf( "%lf", getLowerBound( _b ) ).ascii() : "-inf",
                       existsUpperBound( _b ) ? Stringf( "%lf", getUpperBound( _b ) ).ascii() : "inf" );

    output += Stringf( "f in [%s, %s]",
                       existsLowerBound( _f ) ? Stringf( "%lf", getLowerBound( _f ) ).ascii() : "1",
                       existsUpperBound( _f ) ? Stringf( "%lf", getUpperBound( _f ) ).ascii() : "0" );
}

void SigmoidConstraint::updateVariableIndex( unsigned oldIndex, unsigned newIndex )
{
	ASSERT( oldIndex == _b || oldIndex == _f );
    ASSERT( !_assignment.exists( newIndex ) &&
            !_lowerBounds.exists( newIndex ) &&
            !_upperBounds.exists( newIndex ) &&
            newIndex != _b && newIndex != _f );

    if ( _assignment.exists( oldIndex ) )
    {
        _assignment[newIndex] = _assignment.get( oldIndex );
        _assignment.erase( oldIndex );
    }

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

void SigmoidConstraint::eliminateVariable( __attribute__((unused)) unsigned variable,
                                        __attribute__((unused)) double fixedValue )
{
    ASSERT( variable == _b || variable == _f );

    // In a Sigmoid constraint, if a variable is removed the entire constraint can be discarded.
    _haveEliminatedVariables = true;
}

bool SigmoidConstraint::constraintObsolete() const
{
    return _haveEliminatedVariables;
}

void SigmoidConstraint::getEntailedTightenings( List<Tightening> &tightenings ) const
{ 
    ASSERT( existsLowerBound( _b ) && existsLowerBound( _f ) &&
            existsUpperBound( _b ) && existsUpperBound( _f ) );

    double bLowerBound = getLowerBound( _b );
    double fLowerBound = getLowerBound( _f );
    double bUpperBound = getUpperBound( _b );
    double fUpperBound = getUpperBound( _f );

    tightenings.append( Tightening( _b, bLowerBound, Tightening::LB ) );
    tightenings.append( Tightening( _f, fLowerBound, Tightening::LB ) );

    tightenings.append( Tightening( _b, bUpperBound, Tightening::UB ) );
    tightenings.append( Tightening( _f, fUpperBound, Tightening::UB ) );
}

String SigmoidConstraint::serializeToString() const
{
    return Stringf( "sigmoid,%u,%u", _f, _b );
}

unsigned SigmoidConstraint::getB() const
{
    return _b;
}

unsigned SigmoidConstraint::getF() const
{
    return _f;
}

double SigmoidConstraint::sigmoid( double x ) const
{
    return 1 / ( 1 + std::exp( -x ) );
}

double SigmoidConstraint::sigmoidInverse( double y ) const
{
    ASSERT( y != 1 );
    return log( y / ( 1 - y ) );
}

double SigmoidConstraint::sigmoidDerivative( double x ) const
{
    return sigmoid( x ) * ( 1 - sigmoid( x ) );
}
