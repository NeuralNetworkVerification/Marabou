/*********************                                                        */
/*! \file ExponentialConstraint.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Haoze Wu
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** See the description of the class in ExponentialConstraint.h.
 **/

#include "ExponentialConstraint.h"

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

ExponentialConstraint::ExponentialConstraint( unsigned b, unsigned f )
    : NonlinearConstraint()
    , _b( b )
    , _f( f )
    , _haveEliminatedVariables( false )
{
}

ExponentialConstraint::ExponentialConstraint( const String & )
{
    throw MarabouError( MarabouError::FEATURE_NOT_YET_SUPPORTED );
}

NonlinearFunctionType ExponentialConstraint::getType() const
{
    return NonlinearFunctionType::EXP;
}

NonlinearConstraint *ExponentialConstraint::duplicateConstraint() const
{
    ExponentialConstraint *clone = new ExponentialConstraint( _b, _f );
    *clone = *this;
    return clone;
}

void ExponentialConstraint::restoreState( const NonlinearConstraint *state )
{
    const ExponentialConstraint *exp = dynamic_cast<const ExponentialConstraint *>( state );
    *this = *exp;
}

void ExponentialConstraint::registerAsWatcher( ITableau *tableau )
{
    tableau->registerToWatchVariable( this, _b );
    tableau->registerToWatchVariable( this, _f );
}

void ExponentialConstraint::unregisterAsWatcher( ITableau *tableau )
{
    tableau->unregisterToWatchVariable( this, _b );
    tableau->unregisterToWatchVariable( this, _f );
}

void ExponentialConstraint::notifyLowerBound( unsigned variable, double bound )
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
        if ( variable == _f )
            _boundManager->tightenLowerBound( _b, inverse( bound ) );
        else if ( variable == _b )
            _boundManager->tightenLowerBound( _f, evaluate( bound ) );
    }
}

void ExponentialConstraint::notifyUpperBound( unsigned variable, double bound )
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
        if ( variable == _f )
            _boundManager->tightenUpperBound( _b, inverse( bound ) );
        else if ( variable == _b )
            _boundManager->tightenUpperBound( _f, evaluate( bound ) );
    }
}

bool ExponentialConstraint::participatingVariable( unsigned variable ) const
{
    return ( variable == _b ) || ( variable == _f );
}

List<unsigned> ExponentialConstraint::getParticipatingVariables() const
{
    return List<unsigned>( { _b, _f } );
}

void ExponentialConstraint::dump( String &output ) const
{
    output = Stringf( "ExponentialConstraint: x%u = Exponential( x%u ).\n", _f, _b );

    output += Stringf( "b in [%s, %s], ",
                       existsLowerBound( _b ) ? Stringf( "%lf", getLowerBound( _b ) ).ascii() : "-inf",
                       existsUpperBound( _b ) ? Stringf( "%lf", getUpperBound( _b ) ).ascii() : "inf" );

    output += Stringf( "f in [%s, %s]",
                       existsLowerBound( _f ) ? Stringf( "%lf", getLowerBound( _f ) ).ascii() : "1",
                       existsUpperBound( _f ) ? Stringf( "%lf", getUpperBound( _f ) ).ascii() : "0" );
}

void ExponentialConstraint::updateVariableIndex( unsigned oldIndex, unsigned newIndex )
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

void ExponentialConstraint::eliminateVariable( __attribute__((unused)) unsigned variable,
                                        __attribute__((unused)) double fixedValue )
{
    ASSERT( variable == _b || variable == _f );

    // In a Exponential constraint, if a variable is removed the entire constraint can be discarded.
    _haveEliminatedVariables = true;
}

bool ExponentialConstraint::constraintObsolete() const
{
    return _haveEliminatedVariables;
}

void ExponentialConstraint::getEntailedTightenings( List<Tightening> &tightenings ) const
{
    tightenings.append( Tightening( _f, 0, Tightening::LB ) );

    if ( existsLowerBound( _b ) )
        tightenings.append( Tightening( _f, evaluate( getLowerBound( _b ) ),
                                        Tightening::LB ) );
    if ( existsLowerBound( _f ) )
        tightenings.append( Tightening( _b, inverse( getLowerBound( _f ) ),
                                        Tightening::LB ) );
    if ( existsUpperBound( _b ) )
        tightenings.append( Tightening( _f, evaluate( getUpperBound( _b ) ),
                                        Tightening::UB ) );
    if ( existsUpperBound( _f ) )
        tightenings.append( Tightening( _b, inverse( getUpperBound( _f ) ),
                                        Tightening::UB ) );
}

String ExponentialConstraint::serializeToString() const
{
    throw MarabouError( MarabouError::FEATURE_NOT_YET_SUPPORTED );
}

unsigned ExponentialConstraint::getB() const
{
    return _b;
}

unsigned ExponentialConstraint::getF() const
{
    return _f;
}

double ExponentialConstraint::evaluate( double x ) const
{
    return std::exp( x );
}

double ExponentialConstraint::inverse( double y ) const
{
    return log( y );
}

double ExponentialConstraint::derivative( double x ) const
{
    return evaluate( x );
}
