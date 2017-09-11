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
#include "Statistics.h"

ReluConstraint::ReluConstraint( unsigned b, unsigned f )
    : _b( b )
    , _f( f )
    , _haveEliminatedVariables( false )
{
    setPhaseStatue( PhaseStatus::PHASE_NOT_FIXED );
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
    if ( _statistics )
        _statistics->incNumBoundNotificationsPlConstraints();

    if ( _lowerBounds.exists( variable ) && !FloatUtils::gt( bound, _lowerBounds[variable] ) )
        return;

    _lowerBounds[variable] = bound;

    if ( variable == _f && FloatUtils::isPositive( bound ) )
        setPhaseStatue( PhaseStatus::PHASE_ACTIVE );
    else if ( variable == _b && !FloatUtils::isNegative( bound ) )
        setPhaseStatue( PhaseStatus::PHASE_ACTIVE );
 }

void ReluConstraint::notifyUpperBound( unsigned variable, double bound )
{
    if ( _statistics )
        _statistics->incNumBoundNotificationsPlConstraints();

    if ( _upperBounds.exists( variable ) && !FloatUtils::lt( bound, _upperBounds[variable] ) )
        return;

    _upperBounds[variable] = bound;

    if ( ( variable == _f || variable == _b ) && !FloatUtils::isPositive( bound ) )
        setPhaseStatue( PhaseStatus::PHASE_INACTIVE );
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

    if ( FloatUtils::isNegative( fValue ) )
        return false;

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

    List<PiecewiseLinearCaseSplit> splits;

    // If we have existing knowledge about the assignment, use it to
    // influence the order of splits
    if ( _assignment.exists( _f ) )
    {
        if ( FloatUtils::isPositive( _assignment[_f] ) )
        {
            splits.append( getActiveSplit() );
            splits.append( getInactiveSplit() );
        }
        else
        {
            splits.append( getInactiveSplit() );
            splits.append( getActiveSplit() );
        }
    }
    else
    {
        // Default: start with the inactive case, because it doesn't
        // introduce a new equation and is hence computationally cheaper.
        splits.append( getInactiveSplit() );
        splits.append( getActiveSplit() );
    }

    return splits;
}

PiecewiseLinearCaseSplit ReluConstraint::getInactiveSplit() const
{
    // Inactive phase: b <= 0, f = 0
    PiecewiseLinearCaseSplit inactivePhase;
    inactivePhase.storeBoundTightening( Tightening( _b, 0.0, Tightening::UB ) );
    inactivePhase.storeBoundTightening( Tightening( _f, 0.0, Tightening::UB ) );
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
    output = Stringf( "ReluConstraint: x%u = ReLU( x%u ). Active? %s. PhaseStatus = %u (%s). "
                      "b in [%lf, %lf]. f in [%lf, %lf]",
                      _f, _b,
                      _constraintActive ? "Yes" : "No",
                      _phaseStatus, phaseToString( _phaseStatus ).ascii(),
                      _lowerBounds[_b], _upperBounds[_b], _lowerBounds[_f], _upperBounds[_f]
                      );
}

void ReluConstraint::updateVariableIndex( unsigned oldIndex, unsigned newIndex )
{
	ASSERT( oldIndex == _b || oldIndex == _f );

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
	else
		_f = newIndex;
}

void ReluConstraint::eliminateVariable( __attribute__((unused)) unsigned variable, double fixedValue )
{
	ASSERT( variable == _b || variable == _f );

    DEBUG({
            if ( variable == _f )
            {
                ASSERT( FloatUtils::gte( fixedValue, 0 ) );
            }

            if ( FloatUtils::gt( fixedValue, 0 ) )
            {
                ASSERT( _phaseStatus != PHASE_INACTIVE );
            }
            else
            {
                ASSERT( _phaseStatus != PHASE_ACTIVE );
            }
        });

	if ( FloatUtils::gt( fixedValue, 0 ) )
        setPhaseStatue( PhaseStatus::PHASE_ACTIVE );
	else
        setPhaseStatue( PhaseStatus::PHASE_INACTIVE );

    _haveEliminatedVariables = true;
}

void ReluConstraint::getEntailedTightenings( List<Tightening> &tightenings ) const
{
    ASSERT( _lowerBounds.exists( _b ) && _lowerBounds.exists( _f ) &&
            _upperBounds.exists( _b ) && _upperBounds.exists( _f ) );

    // Upper bounds
    double bUpperBound = _upperBounds[_b];
    double fUpperBound = _upperBounds[_f];

    double minBound =
        FloatUtils::lt( bUpperBound, fUpperBound ) ? bUpperBound : fUpperBound;

    if ( !FloatUtils::isNegative( minBound ) )
    {
        // The minimal bound is non-negative. Should match for both f and b.
        if ( FloatUtils::lt( minBound, bUpperBound ) )
            tightenings.append( Tightening( _b, minBound, Tightening::UB ) );
        else if ( FloatUtils::lt( minBound, fUpperBound ) )
            tightenings.append( Tightening( _f, minBound, Tightening::UB ) );
    }
    else
    {
        // The minimal bound is negative. This has to be b's upper bound.
        if ( !FloatUtils::isZero( fUpperBound ) )
            tightenings.append( Tightening( _f, 0.0, Tightening::UB ) );
    }

    // Lower bounds
    double bLowerBound = _lowerBounds[_b];
    double fLowerBound = _lowerBounds[_f];

    // Lower bounds are entailed between f and b only if they are strictly positive, and otherwise ignored.
    if ( FloatUtils::isPositive( fLowerBound ) )
    {
        if ( FloatUtils::lt( bLowerBound, fLowerBound ) )
            tightenings.append( Tightening( _b, fLowerBound, Tightening::LB ) );
    }

    if ( FloatUtils::isPositive( bLowerBound ) )
    {
        if ( FloatUtils::lt( fLowerBound, bLowerBound ) )
            tightenings.append( Tightening( _f, bLowerBound, Tightening::LB ) );
    }
}

String ReluConstraint::phaseToString( PhaseStatus phase )
{
    switch ( phase )
    {
    case PHASE_NOT_FIXED:
        return "PHASE_NOT_FIXED";

    case PHASE_ACTIVE:
        return "PHASE_ACTIVE";

    case PHASE_INACTIVE:
        return "PHASE_INACTIVE";

    default:
        return "UNKNOWN";
    }
};

void ReluConstraint::setPhaseStatue( PhaseStatus phaseStatus )
{
    _phaseStatus = phaseStatus;
}

bool ReluConstraint::constraintDisabledByVariableElimination() const
{
    return _haveEliminatedVariables;
}

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
