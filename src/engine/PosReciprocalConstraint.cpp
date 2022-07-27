/*********************                                                        */
/*! \file PosReciprocalConstraint.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Haoze Wu
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** See the description of the class in PosReciprocalConstraint.h.
 **/

#include "PosReciprocalConstraint.h"

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

PosReciprocalConstraint::PosReciprocalConstraint( unsigned b, unsigned f )
    : NonlinearConstraint()
    , _b( b )
    , _f( f )
    , _haveEliminatedVariables( false )
{
}

PosReciprocalConstraint::PosReciprocalConstraint( const String & )
{
    throw MarabouError( MarabouError::FEATURE_NOT_YET_SUPPORTED );
}

NonlinearFunctionType PosReciprocalConstraint::getType() const
{
    return NonlinearFunctionType::POS_RECIPROCAL;
}

NonlinearConstraint *PosReciprocalConstraint::duplicateConstraint() const
{
    PosReciprocalConstraint *clone = new PosReciprocalConstraint( _b, _f );
    *clone = *this;
    return clone;
}

void PosReciprocalConstraint::restoreState( const NonlinearConstraint *state )
{
    const PosReciprocalConstraint *recip = dynamic_cast<const PosReciprocalConstraint *>( state );
    *this = *recip;
}

void PosReciprocalConstraint::registerAsWatcher( ITableau *tableau )
{
    tableau->registerToWatchVariable( this, _b );
    tableau->registerToWatchVariable( this, _f );
}

void PosReciprocalConstraint::unregisterAsWatcher( ITableau *tableau )
{
    tableau->unregisterToWatchVariable( this, _b );
    tableau->unregisterToWatchVariable( this, _f );
}

void PosReciprocalConstraint::notifyLowerBound( unsigned variable, double bound )
{
    ASSERT( variable == _b || variable == _f );

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
        if ( variable == _f && FloatUtils::isPositive( bound ) )
            _boundManager->tightenUpperBound( _b, evaluate( bound ) );
        else if ( variable == _b && FloatUtils::isPositive( bound ) )
            _boundManager->tightenUpperBound( _f, evaluate( bound ) );
    }
}

void PosReciprocalConstraint::notifyUpperBound( unsigned variable, double bound )
{
    ASSERT( variable == _b || variable == _f );

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
        if ( variable == _f && FloatUtils::isPositive( bound ) )
            _boundManager->tightenLowerBound( _b, evaluate( bound ) );
        else if ( variable == _b && FloatUtils::isPositive( bound ) )
            _boundManager->tightenLowerBound( _f, evaluate( bound ) );
    }
}

bool PosReciprocalConstraint::participatingVariable( unsigned variable ) const
{
    return ( variable == _b ) || ( variable == _f );
}

List<unsigned> PosReciprocalConstraint::getParticipatingVariables() const
{
    return List<unsigned>( { _b, _f } );
}

void PosReciprocalConstraint::dump( String &output ) const
{
    output = Stringf( "PosReciprocalConstraint: x%u = PosReciprocal( x%u ).\n", _f, _b );

    output += Stringf( "b in [%s, %s], ",
                       existsLowerBound( _b ) ? Stringf( "%lf", getLowerBound( _b ) ).ascii() : "0",
                       existsUpperBound( _b ) ? Stringf( "%lf", getUpperBound( _b ) ).ascii() : "inf" );

    output += Stringf( "f in [%s, %s]",
                       existsLowerBound( _f ) ? Stringf( "%lf", getLowerBound( _f ) ).ascii() : "0",
                       existsUpperBound( _f ) ? Stringf( "%lf", getUpperBound( _f ) ).ascii() : "inf" );
}

void PosReciprocalConstraint::updateVariableIndex( unsigned oldIndex, unsigned newIndex )
{
    ASSERT( oldIndex == _b || oldIndex == _f );
    ASSERT( !_lowerBounds.exists( newIndex ) &&
            !_upperBounds.exists( newIndex ) &&
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

void PosReciprocalConstraint::eliminateVariable( __attribute__((unused)) unsigned variable,
                                        __attribute__((unused)) double fixedValue )
{
    ASSERT( variable == _b || variable == _f );

    // In a Reciprocal constraint, if a variable is removed the entire constraint can be discarded.
    _haveEliminatedVariables = true;
}

bool PosReciprocalConstraint::constraintObsolete() const
{
    return _haveEliminatedVariables;
}

void PosReciprocalConstraint::getEntailedTightenings( List<Tightening> &tightenings ) const
{ 
    tightenings.append( Tightening( _b, 0, Tightening::LB ) );
    tightenings.append( Tightening( _f, 0, Tightening::LB ) );

    if ( existsLowerBound( _b ) && getLowerBound( _b ) > 0 )
        tightenings.append( Tightening( _f, evaluate( getLowerBound( _b ) ),
                                        Tightening::UB ) );
    if ( existsLowerBound( _f ) && getLowerBound( _f ) > 0  )
        tightenings.append( Tightening( _b, evaluate( getLowerBound( _f ) ),
                                        Tightening::UB ) );
    if ( existsUpperBound( _b ) && getUpperBound( _b ) > 0 )
        tightenings.append( Tightening( _f, evaluate( getUpperBound( _b ) ),
                                        Tightening::LB ) );
    if ( existsUpperBound( _f ) && getUpperBound( _f ) > 0 )
        tightenings.append( Tightening( _b, evaluate( getUpperBound( _f ) ),
                                        Tightening::LB ) );
}

String PosReciprocalConstraint::serializeToString() const
{
    throw MarabouError( MarabouError::FEATURE_NOT_YET_SUPPORTED );
}

unsigned PosReciprocalConstraint::getB() const
{
    return _b;
}

unsigned PosReciprocalConstraint::getF() const
{
    return _f;
}

double PosReciprocalConstraint::evaluate( double x ) const
{
  ASSERT( x >= 0 );
  if ( !FloatUtils::isFinite(x) )
    return 0;
  else if ( FloatUtils::isZero(x) )
    return FloatUtils::infinity();
  else
    return 1/x;
}

double PosReciprocalConstraint::inverse( double y ) const
{
  return evaluate(y);
}

double PosReciprocalConstraint::derivative( double x ) const
{
    return -1 / (x * x);
}
