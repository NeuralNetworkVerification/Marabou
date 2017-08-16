/*********************                                                        */
/*! \file ReluConstraint.cpp
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
#include "FreshVariables.h"
#include "ITableau.h"
#include "MStringf.h"
#include "PiecewiseLinearCaseSplit.h"
#include "ReluConstraint.h"
#include "ReluplexError.h"

ReluConstraint::ReluConstraint( unsigned b, unsigned f )
    : PiecewiseLinearConstraint( f )
    , _b( b )
    , _phaseStatus( PhaseStatus::PHASE_NOT_FIXED )
{
}

PiecewiseLinearConstraint *ReluConstraint::duplicateConstraint() const
{
    ReluConstraint *clone = new ReluConstraint( _b, _f );
    *clone = *this;
    return clone;
}

void ReluConstraint::restoreState( const PiecewiseLinearConstraint *state )
{
    const ReluConstraint *relu = dynamic_cast<const ReluConstraint *>( state );
    *this = *relu;
}

void ReluConstraint::registerAsWatcher( ITableau *tableau )
{
    tableau->registerToWatchVariable( this, _b );
    tableau->registerToWatchVariable( this, _f );
}

void ReluConstraint::unregisterAsWatcher( ITableau *tableau )
{
    tableau->unregisterToWatchVariable( this, _b );
    tableau->unregisterToWatchVariable( this, _f );
}

void ReluConstraint::notifyVariableValue( unsigned variable, double value )
{
    _assignment[variable] = value;
}

void ReluConstraint::notifyLowerBound( unsigned variable, double bound )
{
    if ( _lowerBounds.exists( variable ) && !FloatUtils::gt( bound, _lowerBounds[variable] ) )
        return;

    _lowerBounds[variable] = bound;

    // f >= c implies b >= c for any c > 0
    // b >= c implies f >= c for any c >= 0, but the c = 0 case is unneeded
    if ( variable == _f && FloatUtils::isPositive( bound ) )
    {
        _phaseStatus = PhaseStatus::PHASE_ACTIVE;
        _entailedTightenings.push( Tightening( _b, bound, Tightening::LB ) );
    }
    else if ( variable == _b && !FloatUtils::isNegative( bound ) )
    {
        _phaseStatus = PhaseStatus::PHASE_ACTIVE;
        if ( FloatUtils::isPositive( bound ) )
            _entailedTightenings.push( Tightening( _f, bound, Tightening::LB ) );
    }
}

void ReluConstraint::notifyUpperBound( unsigned variable, double bound )
{
    if ( _upperBounds.exists( variable ) && !FloatUtils::lt( bound, _upperBounds[variable] ) )
        return;

    _upperBounds[variable] = bound;

    // b <= c implies f <= c for any c >= 0
    // f <= c implies b <= c for any c >= 0
    if ( variable == _b && !FloatUtils::isNegative( bound ) )
        _entailedTightenings.push( Tightening( _f, bound, Tightening::UB ) );
    else if ( variable == _f && !FloatUtils::isNegative( bound ) )
        _entailedTightenings.push( Tightening( _b, bound, Tightening::UB ) );

    // b <= c implies f <= 0 for any c < 0
    if ( variable == _b && FloatUtils::isNegative( bound ) )
        _entailedTightenings.push( Tightening( _f, 0.0, Tightening::UB ) );

    if ( ( variable == _f || variable == _b ) && !FloatUtils::isPositive( bound ) )
        _phaseStatus = PhaseStatus::PHASE_INACTIVE;
}

bool ReluConstraint::participatingVariable( unsigned variable ) const
{
    return ( variable == _b ) || ( variable == _f );
}

List<unsigned> ReluConstraint::getParticipatingVariables() const
{
    return List<unsigned>( { _b, _f } );
}

bool ReluConstraint::satisfied() const
{
    if ( !( _assignment.exists( _b ) && _assignment.exists( _f ) ) )
        throw ReluplexError( ReluplexError::PARTICIPATING_VARIABLES_ABSENT );

    double bValue = _assignment.get( _b );
    double fValue = _assignment.get( _f );

    ASSERT( !FloatUtils::isNegative( fValue ) );

    if ( FloatUtils::isPositive( fValue ) )
        return FloatUtils::areEqual( bValue, fValue );
    else
        return !FloatUtils::isPositive( bValue );
}

List<PiecewiseLinearConstraint::Fix> ReluConstraint::getPossibleFixes() const
{
    ASSERT( !satisfied() );
    ASSERT( _assignment.exists( _b ) );
    ASSERT( _assignment.exists( _f ) );

    double bValue = _assignment.get( _b );
    double fValue = _assignment.get( _f );

    ASSERT( !FloatUtils::isNegative( fValue ) );

    List<PiecewiseLinearConstraint::Fix> fixes;

    // Possible violations:
    //   1. f is positive, b is positive, b and f are disequal
    //   2. f is positive, b is non-positive
    //   3. f is zero, b is positive
    if ( FloatUtils::isPositive( fValue ) )
    {
        if ( FloatUtils::isPositive( bValue ) )
        {
            fixes.append( PiecewiseLinearConstraint::Fix( _b, fValue ) );
            fixes.append( PiecewiseLinearConstraint::Fix( _f, bValue ) );
        }
        else
        {
            fixes.append( PiecewiseLinearConstraint::Fix( _b, fValue ) );
            fixes.append( PiecewiseLinearConstraint::Fix( _f, 0 ) );
        }
    }
    else
    {
        fixes.append( PiecewiseLinearConstraint::Fix( _b, 0 ) );
        fixes.append( PiecewiseLinearConstraint::Fix( _f, bValue ) );
    }

    return fixes;
}

List<PiecewiseLinearCaseSplit> ReluConstraint::getCaseSplits() const
{
    if ( _phaseStatus != PhaseStatus::PHASE_NOT_FIXED )
        throw ReluplexError( ReluplexError::REQUESTED_CASE_SPLITS_FROM_FIXED_CONSTRAINT );

    // Auxiliary variable bound, needed for either phase
    List<PiecewiseLinearCaseSplit> splits;

    splits.append( getActiveSplit() );
    splits.append( getInactiveSplit() );

    return splits;
}

PiecewiseLinearCaseSplit ReluConstraint::getInactiveSplit() const
{
    // Inactive phase: b <= 0, f = 0
    // Need to fix f because the bound might not be a tightening
    PiecewiseLinearCaseSplit inactivePhase;
    inactivePhase.storeBoundTightening( Tightening( _b, 0.0, Tightening::UB ) );
    Equation inactiveEquation;
    inactiveEquation.addAddend( 1, _f );
    inactiveEquation.setScalar( 0 );
    inactivePhase.addEquation( inactiveEquation, PiecewiseLinearCaseSplit::EQ );
    return inactivePhase;
}

PiecewiseLinearCaseSplit ReluConstraint::getActiveSplit() const
{
    // Active phase: b >= 0, b - f = 0
    PiecewiseLinearCaseSplit activePhase;
    activePhase.storeBoundTightening( Tightening( _b, 0.0, Tightening::LB ) );
    Equation activeEquation;
    activeEquation.addAddend( 1, _b );
    activeEquation.addAddend( -1, _f );
    activeEquation.setScalar( 0 );
    activePhase.addEquation( activeEquation, PiecewiseLinearCaseSplit::EQ );
    return activePhase;
}

bool ReluConstraint::phaseFixed() const
{
    return _phaseStatus != PhaseStatus::PHASE_NOT_FIXED;
}

PiecewiseLinearCaseSplit ReluConstraint::getValidCaseSplit() const
{
    ASSERT( _phaseStatus != PhaseStatus::PHASE_NOT_FIXED );

    if ( _phaseStatus == PhaseStatus::PHASE_ACTIVE )
        return getActiveSplit();

    return getInactiveSplit();
}

void ReluConstraint::dump( String &output ) const
{
    output = Stringf( "ReluConstraint: x%u = ReLU( x%u ). Active? %s. PhaseStatus = %u",
                      _f, _b,
                      _constraintActive ? "Yes" : "No",
                      _phaseStatus );
}

void ReluConstraint::updateVarIndex( unsigned prevVar, unsigned newVar )
{
	ASSERT( prevVar == _b || prevVar == _f );

	if ( _assignment.exists( prevVar ) )
	{
		_assignment[newVar] = _assignment.get( prevVar );
		_assignment.erase( prevVar );
	}

	if ( prevVar == _b )
		_b = newVar;
	else
		_f = newVar;
}

void ReluConstraint::eliminateVar( unsigned var, double val )
{
	ASSERT( var == _b || var == _f );

	if ( var == _f )
	{
	    ASSERT( FloatUtils::gte( val, 0 ) );
	}

	if ( FloatUtils::gt( val, 0 ) )
	{
		_phaseStatus = PhaseStatus::PHASE_ACTIVE;
	}
	else
	{
		_phaseStatus = PhaseStatus::PHASE_INACTIVE;
	}
}

//
// Local Variables:
// compile-command: "make -C .. "
// tags-file-name: "../TAGS"
// c-basic-offset: 4
// End:
//
