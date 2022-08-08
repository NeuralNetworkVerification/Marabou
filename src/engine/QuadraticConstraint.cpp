/*********************                                                        */
/*! \file QuadraticConstraint.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Haoze Wu
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** See the description of the class in QuadraticConstraint.h.
 **/

#include "QuadraticConstraint.h"

#include "NonlinearConstraint.h"
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

QuadraticConstraint::QuadraticConstraint( unsigned b1, unsigned b2, unsigned f )
    : NonlinearConstraint()
    , _b1( b1 )
    , _b2( b2 )
    , _f( f )
    , _haveEliminatedVariables( false )
{
}

QuadraticConstraint::QuadraticConstraint( const String & )
{
    throw MarabouError( MarabouError::FEATURE_NOT_YET_SUPPORTED );
}

NonlinearFunctionType QuadraticConstraint::getType() const
{
    return NonlinearFunctionType::QUADRATIC;
}

NonlinearConstraint *QuadraticConstraint::duplicateConstraint() const
{
    QuadraticConstraint *clone = new QuadraticConstraint( _b1, _b2, _f );
    *clone = *this;
    return clone;
}

void QuadraticConstraint::restoreState( const NonlinearConstraint *state )
{
    const QuadraticConstraint *quad = dynamic_cast<const QuadraticConstraint *>( state );
    *this = *quad;
}

void QuadraticConstraint::registerAsWatcher( ITableau *tableau )
{
    tableau->registerToWatchVariable( this, _b1 );
    tableau->registerToWatchVariable( this, _b2 );
    tableau->registerToWatchVariable( this, _f );
}

void QuadraticConstraint::unregisterAsWatcher( ITableau *tableau )
{
    tableau->unregisterToWatchVariable( this, _b1 );
    tableau->unregisterToWatchVariable( this, _b2 );
    tableau->unregisterToWatchVariable( this, _f );
}

void QuadraticConstraint::notifyLowerBound( unsigned variable, double bound )
{
    ASSERT( variable == _b1 || variable == _b2 || variable == _f );

    if ( _statistics )
        _statistics->incLongAttribute(
            Statistics::NUM_BOUND_NOTIFICATIONS_TO_NONLINEAR_CONSTRAINTS );

    if ( _boundManager == nullptr )
    {
        if ( existsLowerBound( variable ) &&
             !FloatUtils::gt( bound, getLowerBound( variable ) ) )
            return;

        setLowerBound( variable, bound );
    }
    else
    {
    }
}

void QuadraticConstraint::notifyUpperBound( unsigned variable, double bound )
{
    ASSERT( variable == _b1 || variable == _b2 || variable == _f );

    if ( _statistics )
        _statistics->incLongAttribute(
            Statistics::NUM_BOUND_NOTIFICATIONS_TO_NONLINEAR_CONSTRAINTS );

    if ( _boundManager == nullptr )
    {
        if ( existsUpperBound( variable ) &&
             !FloatUtils::lt( bound, getUpperBound( variable ) ) )
            return;

        setUpperBound( variable, bound );
    }
    else
    {
    }
}

bool QuadraticConstraint::participatingVariable( unsigned variable ) const
{
  return variable == _b1 || variable == _b2 || variable == _f;
}

List<unsigned> QuadraticConstraint::getParticipatingVariables() const
{
  return List<unsigned>( { _b1, _b2, _f } );
}

void QuadraticConstraint::dump( String &output ) const
{
  output = Stringf( "QuadraticConstraint: x%u = Quadratic( x%u, x%u ).\n", _f, _b1, _b2 );

    output += Stringf( "b1 in [%s, %s], ",
                       existsLowerBound( _b1 ) ? Stringf( "%lf", getLowerBound( _b1 ) ).ascii() : "-inf",
                       existsUpperBound( _b1 ) ? Stringf( "%lf", getUpperBound( _b1 ) ).ascii() : "inf" );

    output += Stringf( "b2 in [%s, %s], ",
                       existsLowerBound( _b2 ) ? Stringf( "%lf", getLowerBound( _b2 ) ).ascii() : "-inf",
                       existsUpperBound( _b2 ) ? Stringf( "%lf", getUpperBound( _b2 ) ).ascii() : "inf" );

    output += Stringf( "f in [%s, %s]",
                       existsLowerBound( _f ) ? Stringf( "%lf", getLowerBound( _f ) ).ascii() : "1",
                       existsUpperBound( _f ) ? Stringf( "%lf", getUpperBound( _f ) ).ascii() : "0" );
}

void QuadraticConstraint::updateVariableIndex( unsigned oldIndex, unsigned newIndex )
{
    ASSERT( oldIndex == _b1 || oldIndex == _b2 || oldIndex == _f );

    ASSERT( !_lowerBounds.exists( newIndex ) &&
            !_upperBounds.exists( newIndex ) &&
            newIndex != _b1 && newIndex != _b2 && newIndex != _f );

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

    if ( oldIndex == _b1 )
        _b1 = newIndex;
    if ( oldIndex == _b2 )
      _b2 = newIndex;
    else if ( oldIndex == _f )
        _f = newIndex;
}

void QuadraticConstraint::eliminateVariable( __attribute__((unused)) unsigned variable,
                                        __attribute__((unused)) double fixedValue )
{
    ASSERT( variable == _b1 || variable == _b2 || variable == _f );

    // In a Quadratic constraint, if a variable is removed the entire constraint can be discarded.
    _haveEliminatedVariables = true;
}

bool QuadraticConstraint::constraintObsolete() const
{
    return _haveEliminatedVariables;
}

void QuadraticConstraint::getEntailedTightenings( List<Tightening> & ) const
{
}

String QuadraticConstraint::serializeToString() const
{
    throw MarabouError( MarabouError::FEATURE_NOT_YET_SUPPORTED );
}

List<unsigned> QuadraticConstraint::getBs() const
{
  return {_b1, _b2};
}

unsigned QuadraticConstraint::getF() const
{
    return _f;
}

double QuadraticConstraint::evaluate( double x, double y ) const
{
  return x * y;
}
