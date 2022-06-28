/*********************                                                        */
/*! \file ReciprocalConstraint.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Haoze Wu
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** See the description of the class in ReciprocalConstraint.h.
 **/

#include "ReciprocalConstraint.h"

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

ReciprocalConstraint::ReciprocalConstraint( unsigned b, unsigned f )
    : NonlinearConstraint()
    , _b( b )
    , _f( f )
    , _haveEliminatedVariables( false )
{
}

ReciprocalConstraint::ReciprocalConstraint( const String & )
{
    throw MarabouError( MarabouError::FEATURE_NOT_YET_SUPPORTED );
}

NonlinearFunctionType ReciprocalConstraint::getType() const
{
    return NonlinearFunctionType::RECIPROCAL;
}

NonlinearConstraint *ReciprocalConstraint::duplicateConstraint() const
{
    ReciprocalConstraint *clone = new ReciprocalConstraint( _b, _f );
    *clone = *this;
    return clone;
}

void ReciprocalConstraint::restoreState( const NonlinearConstraint *state )
{
    const ReciprocalConstraint *recip = dynamic_cast<const ReciprocalConstraint *>( state );
    *this = *recip;
}

void ReciprocalConstraint::registerAsWatcher( ITableau *tableau )
{
    tableau->registerToWatchVariable( this, _b );
    tableau->registerToWatchVariable( this, _f );
}

void ReciprocalConstraint::unregisterAsWatcher( ITableau *tableau )
{
    tableau->unregisterToWatchVariable( this, _b );
    tableau->unregisterToWatchVariable( this, _f );
}

void ReciprocalConstraint::notifyLowerBound( unsigned variable, double bound )
{
    ASSERT( variable == _b || variable == _f );

    if ( _statistics )
        _statistics->incLongAttribute(
            Statistics::NUM_BOUND_NOTIFICATIONS_TO_TRANSCENDENTAL_CONSTRAINTS );

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
            _boundManager->tightenUpperBound( _b,  );
        else if ( variable == _b )
            _boundManager->tightenLowerBound( _f, recip( bound ) );
    }
}

void ReciprocalConstraint::notifyUpperBound( unsigned variable, double bound )
{
    ASSERT( variable == _b || variable == _f );

    if ( _statistics )
        _statistics->incLongAttribute(
            Statistics::NUM_BOUND_NOTIFICATIONS_TO_TRANSCENDENTAL_CONSTRAINTS );

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
            _boundManager->tightenUpperBound( _b, recipInverse( bound ) );
        else if ( variable == _b )
            _boundManager->tightenUpperBound( _f, recip( bound ) );
    }
}

bool ReciprocalConstraint::participatingVariable( unsigned variable ) const
{
    return ( variable == _b ) || ( variable == _f );
}

List<unsigned> ReciprocalConstraint::getParticipatingVariables() const
{
    return List<unsigned>( { _b, _f } );
}

void ReciprocalConstraint::dump( String &output ) const
{
    output = Stringf( "ReciprocalConstraint: x%u = Reciprocal( x%u ).\n", _f, _b );

    output += Stringf( "b in [%s, %s], ",
                       existsLowerBound( _b ) ? Stringf( "%lf", getLowerBound( _b ) ).ascii() : "-inf",
                       existsUpperBound( _b ) ? Stringf( "%lf", getUpperBound( _b ) ).ascii() : "inf" );

    output += Stringf( "f in [%s, %s]",
                       existsLowerBound( _f ) ? Stringf( "%lf", getLowerBound( _f ) ).ascii() : "1",
                       existsUpperBound( _f ) ? Stringf( "%lf", getUpperBound( _f ) ).ascii() : "0" );
}

void ReciprocalConstraint::updateVariableIndex( unsigned oldIndex, unsigned newIndex )
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

void ReciprocalConstraint::eliminateVariable( __attribute__((unused)) unsigned variable,
                                        __attribute__((unused)) double fixedValue )
{
    ASSERT( variable == _b || variable == _f );

    // In a Reciprocal constraint, if a variable is removed the entire constraint can be discarded.
    _haveEliminatedVariables = true;
}

bool ReciprocalConstraint::constraintObsolete() const
{
    return _haveEliminatedVariables;
}

void ReciprocalConstraint::getEntailedTightenings( List<Tightening> &tightenings ) const
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

String ReciprocalConstraint::serializeToString() const
{
    throw MarabouError( MarabouError::FEATURE_NOT_YET_SUPPORTED );
}

unsigned ReciprocalConstraint::getB() const
{
    return _b;
}

unsigned ReciprocalConstraint::getF() const
{
    return _f;
}
