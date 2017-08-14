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

    clone->_constraintActive = _constraintActive;
    clone->_assignment = _assignment;
    clone->_phaseStatus = _phaseStatus;

    return clone;
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

List<unsigned> ReluConstraint::getParticiatingVariables() const
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
    unsigned auxVariable = FreshVariables::getNextVariable();

    splits.append( getActiveSplit( auxVariable ) );
    splits.append( getInactiveSplit( auxVariable ) );

    return splits;
}

void ReluConstraint::storeState( PiecewiseLinearConstraintState &state ) const
{
    PiecewiseLinearConstraint::storeState( state );
    ReluConstraintState *reluState = dynamic_cast<ReluConstraintState *>( &state );
    reluState->_phaseStatus = _phaseStatus;
}

void ReluConstraint::restoreState( const PiecewiseLinearConstraintState &state )
{
    PiecewiseLinearConstraint::restoreState( state );
    const ReluConstraintState *reluState = dynamic_cast<const ReluConstraintState *>( &state );
    _phaseStatus = reluState->_phaseStatus;
}

PiecewiseLinearConstraintState *ReluConstraint::allocateState() const
{
    return new ReluConstraintState;
}

PiecewiseLinearCaseSplit ReluConstraint::getInactiveSplit( unsigned auxVariable ) const
{
    // Inactive phase: b <= 0, f = 0
    PiecewiseLinearCaseSplit inactivePhase;
    Tightening inactiveBound( _b, 0.0, Tightening::UB );
    inactivePhase.storeBoundTightening( inactiveBound );
    Equation inactiveEquation;
    inactiveEquation.addAddend( 1, _f );
    inactiveEquation.addAddend( 1, auxVariable );
    inactiveEquation.markAuxiliaryVariable( auxVariable );
    inactiveEquation.setScalar( 0 );
    inactivePhase.addEquation( inactiveEquation );
    Tightening auxUpperBound( auxVariable, 0.0, Tightening::UB );
    Tightening auxLowerBound( auxVariable, 0.0, Tightening::LB );
    inactivePhase.storeBoundTightening( auxUpperBound );
    inactivePhase.storeBoundTightening( auxLowerBound );
    return inactivePhase;
}

PiecewiseLinearCaseSplit ReluConstraint::getActiveSplit( unsigned auxVariable ) const
{
    // Active phase: b >= 0, b - f = 0
    PiecewiseLinearCaseSplit activePhase;
    Tightening activeBound( _b, 0.0, Tightening::LB );
    activePhase.storeBoundTightening( activeBound );
    Equation activeEquation;
    activeEquation.addAddend( 1, _b );
    activeEquation.addAddend( -1, _f );
    activeEquation.addAddend( 1, auxVariable );
    activeEquation.markAuxiliaryVariable( auxVariable );
    activeEquation.setScalar( 0 );
    activePhase.addEquation( activeEquation );
    Tightening auxUpperBound( auxVariable, 0.0, Tightening::UB );
    Tightening auxLowerBound( auxVariable, 0.0, Tightening::LB );
    activePhase.storeBoundTightening( auxUpperBound );
    activePhase.storeBoundTightening( auxLowerBound );
    return activePhase;
}

bool ReluConstraint::phaseFixed() const
{
    return _phaseStatus != PhaseStatus::PHASE_NOT_FIXED;
}

PiecewiseLinearCaseSplit ReluConstraint::getValidCaseSplit() const
{
    ASSERT( _phaseStatus != PhaseStatus::PHASE_NOT_FIXED );

    unsigned auxVariable = FreshVariables::getNextVariable();

    if ( _phaseStatus == PhaseStatus::PHASE_ACTIVE )
        return getActiveSplit( auxVariable );

    return getInactiveSplit( auxVariable );
}

void ReluConstraint::dump( String &output ) const
{
    output = Stringf( "ReluConstraint: x%u = ReLU( x%u ). Active? %s. PhaseStatus = %u",
                      _f, _b,
                      _constraintActive ? "Yes" : "No",
                      _phaseStatus );
}

void ReluConstraint::changeVarAssign( unsigned prevVar, unsigned newVar )
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
		_assignment[_f] = val;
	}
	else
	{
		_phaseStatus = PhaseStatus::PHASE_INACTIVE;
		_assignment[_f] = 0;
	}
}

//
// Local Variables:
// compile-command: "make -C .. "
// tags-file-name: "../TAGS"
// c-basic-offset: 4
// End:
//
