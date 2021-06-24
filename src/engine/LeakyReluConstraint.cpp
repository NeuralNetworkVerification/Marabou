/*********************                                                        */
/*! \file LeakyReluConstraint.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz, Parth Shah, Derek Huang
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]
 **/

#include "LeakyReluConstraint.h"

#include "ConstraintBoundTightener.h"
#include "ContextDependentPiecewiseLinearConstraint.h"
#include "Debug.h"
#include "DivideStrategy.h"
#include "FloatUtils.h"
#include "GlobalConfiguration.h"
#include "ITableau.h"
#include "InputQuery.h"
#include "MStringf.h"
#include "PiecewiseLinearCaseSplit.h"
#include "MarabouError.h"
#include "Statistics.h"
#include "TableauRow.h"

#ifdef _WIN32
#define __attribute__(x)
#endif

LeakyReluConstraint::LeakyReluConstraint( unsigned b, unsigned f )
    : ContextDependentPiecewiseLinearConstraint( TWO_PHASE_PIECEWISE_LINEAR_CONSTRAINT )
    , _b( b )
    , _f( f )
    , _slope( GlobalConfiguration::LEAKY_RELU_SLOPE )
    , _direction( PHASE_NOT_FIXED )
    , _haveEliminatedVariables( false )
{
    if ( _slope <= 0 || _slope > 1 )
        throw MarabouError( MarabouError::INVALID_LEAKY_RELU_SLOPE );
}

LeakyReluConstraint::LeakyReluConstraint( unsigned b, unsigned f, double slope )
    : ContextDependentPiecewiseLinearConstraint( TWO_PHASE_PIECEWISE_LINEAR_CONSTRAINT )
    , _b( b )
    , _f( f )
    , _slope( slope )
    , _direction( PHASE_NOT_FIXED )
    , _haveEliminatedVariables( false )
{
    if ( _slope <= 0 )
        throw MarabouError( MarabouError::INVALID_LEAKY_RELU_SLOPE );
}

LeakyReluConstraint::LeakyReluConstraint( const String &serializedLeakyRelu )
    : _haveEliminatedVariables( false )
{
    String constraintType = serializedLeakyRelu.substring( 0, 10 );
    ASSERT( constraintType == String( "leaky_relu" ) );

    // Remove the constraint type in serialized form
    String serializedValues = serializedLeakyRelu.substring( 11, serializedLeakyRelu.length() - 11 );
    List<String> values = serializedValues.tokenize( "," );

    ASSERT( values.size() == 3 );

    auto var = values.begin();
    _f = atoi( var->ascii() );
    ++var;
    _b = atoi( var->ascii() );
    ++var;
    _slope = atof( var->ascii() );
    if ( _slope <= 0 )
        throw MarabouError( MarabouError::INVALID_LEAKY_RELU_SLOPE );

    _direction = PHASE_NOT_FIXED;
}

PiecewiseLinearFunctionType LeakyReluConstraint::getType() const
{
    return PiecewiseLinearFunctionType::LEAKY_RELU;
}

ContextDependentPiecewiseLinearConstraint *LeakyReluConstraint::duplicateConstraint() const
{
    LeakyReluConstraint *clone = new LeakyReluConstraint( _b, _f, _slope );
    *clone = *this;
    this->initializeDuplicateCDOs( clone );
    return clone;
}

void LeakyReluConstraint::restoreState( const PiecewiseLinearConstraint *state )
{
    const LeakyReluConstraint *relu = dynamic_cast<const LeakyReluConstraint *>( state );

    CVC4::context::CDO<bool> *activeStatus = _cdConstraintActive;
    CVC4::context::CDO<PhaseStatus> *phaseStatus = _cdPhaseStatus;
    CVC4::context::CDList<PhaseStatus> *infeasibleCases = _cdInfeasibleCases;
    *this = *relu;
    _cdConstraintActive = activeStatus;
    _cdPhaseStatus = phaseStatus;
    _cdInfeasibleCases = infeasibleCases;
}

void LeakyReluConstraint::registerAsWatcher( ITableau *tableau )
{
    tableau->registerToWatchVariable( this, _b );
    tableau->registerToWatchVariable( this, _f );
}

void LeakyReluConstraint::unregisterAsWatcher( ITableau *tableau )
{
    tableau->unregisterToWatchVariable( this, _b );
    tableau->unregisterToWatchVariable( this, _f );
}

void LeakyReluConstraint::notifyVariableValue( unsigned variable, double value )
{
    if ( FloatUtils::isZero( value, GlobalConfiguration::RELU_CONSTRAINT_COMPARISON_TOLERANCE ) )
        value = 0.0;

    _assignment[variable] = value;
}

void LeakyReluConstraint::notifyLowerBound( unsigned variable, double bound )
{
    ASSERT( variable == _f || variable == _b );
    if ( _statistics )
        _statistics->incNumBoundNotificationsPlConstraints();

    if ( _lowerBounds.exists( variable ) && !FloatUtils::gt( bound, _lowerBounds[variable] ) )
        return;

    _lowerBounds[variable] = bound;

    if ( !FloatUtils::isNegative( bound ) )
        setPhaseStatus( RELU_PHASE_ACTIVE );

    if ( isActive() && _constraintBoundTightener )
    {
        // A positive lower bound is always propagated between f and b
        if ( !FloatUtils::isNegative( bound ) )
        {
            unsigned partner = ( variable == _f ) ? _b : _f;
            _constraintBoundTightener->registerTighterLowerBound( partner, bound );
        }
        else if ( variable == _b )
        {
            _constraintBoundTightener->registerTighterLowerBound( _f, _slope * bound );
        }
        else if ( variable == _f )
        {
            _constraintBoundTightener->registerTighterLowerBound( _b, bound / _slope );
        }
    }
}

void LeakyReluConstraint::notifyUpperBound( unsigned variable, double bound )
{
    ASSERT( variable == _f || variable == _b );
    if ( _statistics )
        _statistics->incNumBoundNotificationsPlConstraints();

    if ( _upperBounds.exists( variable ) && !FloatUtils::lt( bound, _upperBounds[variable] ) )
        return;

    _upperBounds[variable] = bound;

    if ( !FloatUtils::isPositive( bound ) )
        setPhaseStatus( RELU_PHASE_INACTIVE );

    if ( isActive() && _constraintBoundTightener )
    {
        if ( !FloatUtils::isNegative( bound ) )
        {
            unsigned partner = ( variable == _f ) ? _b : _f;
            _constraintBoundTightener->registerTighterUpperBound( partner, bound );
        }
        else if ( variable == _b )
        {
            _constraintBoundTightener->registerTighterUpperBound( _f, _slope * bound );
        }
        else if ( variable == _f )
        {
            _constraintBoundTightener->registerTighterUpperBound( _b, bound / _slope );
        }
    }
}

bool LeakyReluConstraint::participatingVariable( unsigned variable ) const
{
    return ( variable == _b ) || ( variable == _f );
}

List<unsigned> LeakyReluConstraint::getParticipatingVariables() const
{
    return List<unsigned>( { _b, _f } );
}

bool LeakyReluConstraint::satisfied() const
{
    if ( !( _assignment.exists( _b ) && _assignment.exists( _f ) ) )
        throw MarabouError( MarabouError::PARTICIPATING_VARIABLES_ABSENT );

    double bValue = _assignment.get( _b );
    double fValue = _assignment.get( _f );

    if ( FloatUtils::isPositive( fValue ) )
        return FloatUtils::areEqual( bValue, fValue, GlobalConfiguration::RELU_CONSTRAINT_COMPARISON_TOLERANCE );
    else
        return FloatUtils::areEqual( _slope * bValue, fValue, GlobalConfiguration::RELU_CONSTRAINT_COMPARISON_TOLERANCE );
}

List<PiecewiseLinearConstraint::Fix> LeakyReluConstraint::getPossibleFixes() const
{
    return List<PiecewiseLinearConstraint::Fix>();
}

List<PiecewiseLinearConstraint::Fix> LeakyReluConstraint::getSmartFixes( ITableau * ) const
{
    return List<PiecewiseLinearConstraint::Fix>();
}

List<PiecewiseLinearCaseSplit> LeakyReluConstraint::getCaseSplits() const
{
    if ( _phaseStatus != PHASE_NOT_FIXED )
        throw MarabouError( MarabouError::REQUESTED_CASE_SPLITS_FROM_FIXED_CONSTRAINT );

    List<PiecewiseLinearCaseSplit> splits;

    if ( _direction == RELU_PHASE_INACTIVE )
    {
        splits.append( getInactiveSplit() );
        splits.append( getActiveSplit() );
        return splits;
    }
    if ( _direction == RELU_PHASE_ACTIVE )
    {
        splits.append( getActiveSplit() );
        splits.append( getInactiveSplit() );
        return splits;
    }

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

List<PhaseStatus> LeakyReluConstraint::getAllCases() const
{
    if ( _direction == RELU_PHASE_INACTIVE )
        return { RELU_PHASE_INACTIVE, RELU_PHASE_ACTIVE };

    if ( _direction == RELU_PHASE_ACTIVE )
        return { RELU_PHASE_ACTIVE, RELU_PHASE_INACTIVE };

    // If we have existing knowledge about the assignment, use it to
    // influence the order of splits
    if ( _assignment.exists( _f ) )
    {
        if ( FloatUtils::isPositive( _assignment[_f] ) )
            return { RELU_PHASE_ACTIVE, RELU_PHASE_INACTIVE };
        else
            return { RELU_PHASE_INACTIVE, RELU_PHASE_ACTIVE };
    }
    else
        return { RELU_PHASE_INACTIVE, RELU_PHASE_ACTIVE };
}

PiecewiseLinearCaseSplit LeakyReluConstraint::getCaseSplit( PhaseStatus phase ) const
{
    if ( phase == RELU_PHASE_INACTIVE )
        return getInactiveSplit();
    else if ( phase == RELU_PHASE_ACTIVE )
        return getActiveSplit();
    else
        throw MarabouError( MarabouError::REQUESTED_NONEXISTENT_CASE_SPLIT );
}

PiecewiseLinearCaseSplit LeakyReluConstraint::getInactiveSplit() const
{
    // Inactive phase: b <= 0, f = slope * b
    PiecewiseLinearCaseSplit inactivePhase;
    inactivePhase.storeBoundTightening( Tightening( _b, 0.0, Tightening::UB ) );
    inactivePhase.storeBoundTightening( Tightening( _f, 0.0, Tightening::UB ) );

    Equation inactiveEquation( Equation::EQ );
    inactiveEquation.addAddend( _slope, _b );
    inactiveEquation.addAddend( -1, _f );
    inactiveEquation.setScalar( 0 );
    inactivePhase.addEquation( inactiveEquation );
    return inactivePhase;
}

PiecewiseLinearCaseSplit LeakyReluConstraint::getActiveSplit() const
{
    // Active phase: b >= 0, f = b
    PiecewiseLinearCaseSplit activePhase;
    activePhase.storeBoundTightening( Tightening( _b, 0.0, Tightening::LB ) );
    activePhase.storeBoundTightening( Tightening( _f, 0.0, Tightening::LB ) );

    Equation activeEquation( Equation::EQ );
    activeEquation.addAddend( 1, _b );
    activeEquation.addAddend( -1, _f );
    activeEquation.setScalar( 0 );
    activePhase.addEquation( activeEquation );

    return activePhase;
}

bool LeakyReluConstraint::phaseFixed() const
{
    return _phaseStatus != PHASE_NOT_FIXED;
}

PiecewiseLinearCaseSplit LeakyReluConstraint::getImpliedCaseSplit() const
{
    ASSERT( _phaseStatus != PHASE_NOT_FIXED );

    if ( _phaseStatus == RELU_PHASE_ACTIVE )
        return getActiveSplit();

    return getInactiveSplit();
}

PiecewiseLinearCaseSplit LeakyReluConstraint::getValidCaseSplit() const
{
    return getImpliedCaseSplit();
}

void LeakyReluConstraint::dump( String &output ) const
{
    output = Stringf( "LeakyReluConstraint: x%u = LeakyReLU( x%u ), slope = %lf. Active? %s. PhaseStatus = %u (%s).\n",
                      _f, _b, _slope,
                      _constraintActive ? "Yes" : "No",
                      _phaseStatus, phaseToString( _phaseStatus ).ascii()
                      );

    output += Stringf( "b in [%s, %s], ",
                       _lowerBounds.exists( _b ) ? Stringf( "%lf", _lowerBounds[_b] ).ascii() : "-inf",
                       _upperBounds.exists( _b ) ? Stringf( "%lf", _upperBounds[_b] ).ascii() : "inf" );

    output += Stringf( "f in [%s, %s]",
                       _lowerBounds.exists( _f ) ? Stringf( "%lf", _lowerBounds[_f] ).ascii() : "-inf",
                       _upperBounds.exists( _f ) ? Stringf( "%lf", _upperBounds[_f] ).ascii() : "inf" );
}

void LeakyReluConstraint::updateVariableIndex( unsigned oldIndex, unsigned newIndex )
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
    else
        _f = newIndex;
}

void LeakyReluConstraint::eliminateVariable( __attribute__((unused)) unsigned variable,
                                        __attribute__((unused)) double fixedValue )
{
    ASSERT( variable == _b || variable == _f );
    DEBUG({
            if ( variable == _f || variable == _b )
            {
                if ( FloatUtils::gt( fixedValue, 0 ) )
                {
                    ASSERT( _phaseStatus != RELU_PHASE_INACTIVE );
                }
                else if ( FloatUtils::lt( fixedValue, 0 ) )
                {
                    ASSERT( _phaseStatus != RELU_PHASE_ACTIVE );
                }
            }
        });

    // In a ReLU constraint, if a variable is removed the entire constraint can be discarded.
    _haveEliminatedVariables = true;
}

bool LeakyReluConstraint::constraintObsolete() const
{
    return _haveEliminatedVariables;
}

void LeakyReluConstraint::getEntailedTightenings( List<Tightening> &tightenings ) const
{
    ASSERT( _lowerBounds.exists( _b ) && _lowerBounds.exists( _f ) &&
            _upperBounds.exists( _b ) && _upperBounds.exists( _f ) );

    double bLowerBound = _lowerBounds[_b];
    double fLowerBound = _lowerBounds[_f];

    double bUpperBound = _upperBounds[_b];
    double fUpperBound = _upperBounds[_f];

    // Determine if we are in the active phase, inactive phase or unknown phase
    if ( FloatUtils::isPositive( bLowerBound ) ||
         FloatUtils::isPositive( fLowerBound ) )
    {
        // Active case;
        if ( FloatUtils::isPositive( fLowerBound ) )
            tightenings.append( Tightening( _b, fLowerBound, Tightening::LB ) );
        if ( FloatUtils::isPositive( bLowerBound ) )
            tightenings.append( Tightening( _f, bLowerBound, Tightening::LB ) );

        tightenings.append( Tightening( _b, fUpperBound, Tightening::UB ) );
        tightenings.append( Tightening( _f, bUpperBound, Tightening::UB ) );

        tightenings.append( Tightening( _b, 0, Tightening::LB ) );
        tightenings.append( Tightening( _f, 0, Tightening::LB ) );
    }
    else if ( !FloatUtils::isPositive( bUpperBound ) ||
              !FloatUtils::isPositive( fUpperBound ) )
    {
        // Inactive case
        tightenings.append( Tightening( _b, fLowerBound / _slope, Tightening::LB ) );
        tightenings.append( Tightening( _f, _slope * bLowerBound, Tightening::LB ) );

        if ( !FloatUtils::isPositive( fUpperBound ) )
            tightenings.append( Tightening( _b, fUpperBound / _slope, Tightening::UB ) );
        if ( !FloatUtils::isPositive( bUpperBound ) )
            tightenings.append( Tightening( _f, _slope * bUpperBound, Tightening::UB ) );

        tightenings.append( Tightening( _f, 0, Tightening::UB ) );
        tightenings.append( Tightening( _b, 0, Tightening::UB ) );
    }
    else
    {
        // Unknown case

        // b and f share upper bounds
        tightenings.append( Tightening( _b, fUpperBound, Tightening::UB ) );
        tightenings.append( Tightening( _f, bUpperBound, Tightening::UB ) );

        tightenings.append( Tightening( _b, fLowerBound / _slope, Tightening::LB ) );
        tightenings.append( Tightening( _f, _slope * bLowerBound, Tightening::LB ) );
    }
}

String LeakyReluConstraint::phaseToString( PhaseStatus phase )
{
    switch ( phase )
    {
    case PHASE_NOT_FIXED:
        return "PHASE_NOT_FIXED";

    case RELU_PHASE_ACTIVE:
        return "RELU_PHASE_ACTIVE";

    case RELU_PHASE_INACTIVE:
        return "RELU_PHASE_INACTIVE";

    default:
        return "UNKNOWN";
    }
};

void LeakyReluConstraint::addAuxiliaryEquations( InputQuery & )
{
}

void LeakyReluConstraint::getCostFunctionComponent( Map<unsigned, double> & ) const
{
}

bool LeakyReluConstraint::haveOutOfBoundVariables() const
{
    double bValue = _assignment.get( _b );
    double fValue = _assignment.get( _f );

    if ( FloatUtils::gt( _lowerBounds[_b], bValue ) || FloatUtils::lt( _upperBounds[_b], bValue ) )
        return true;

    if ( FloatUtils::gt( _lowerBounds[_f], fValue ) || FloatUtils::lt( _upperBounds[_f], fValue ) )
        return true;

    return false;
}

String LeakyReluConstraint::serializeToString() const
{
    return Stringf( "leaky_relu,%u,%u", _f, _b, _slope );
}

unsigned LeakyReluConstraint::getB() const
{
    return _b;
}

unsigned LeakyReluConstraint::getF() const
{
    return _f;
}

double LeakyReluConstraint::getSlope() const
{
    return _slope;
}

bool LeakyReluConstraint::supportPolarity() const
{
    return true;
}

double LeakyReluConstraint::computePolarity() const
{
    double currentLb = _lowerBounds[_b];
    double currentUb = _upperBounds[_b];
    if ( currentLb >= 0 ) return 1;
    if ( currentUb <= 0 ) return -1;
    double width = currentUb - currentLb;
    double sum = currentUb + currentLb;
    return sum / width;
}

void LeakyReluConstraint::updateDirection()
{
    _direction = ( computePolarity() > 0 ) ? RELU_PHASE_ACTIVE : RELU_PHASE_INACTIVE;
}

PhaseStatus LeakyReluConstraint::getDirection() const
{
    return _direction;
}

void LeakyReluConstraint::updateScoreBasedOnPolarity()
{
    _score = std::abs( computePolarity() );
}
