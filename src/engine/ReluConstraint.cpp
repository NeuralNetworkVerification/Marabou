/*********************                                                        */
/*! \file ReluConstraint.cpp
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

#include "ConstraintBoundTightener.h"
#include "Debug.h"
#include "DivideStrategy.h"
#include "FloatUtils.h"
#include "GlobalConfiguration.h"
#include "ITableau.h"
#include "InputQuery.h"
#include "MStringf.h"
#include "PiecewiseLinearCaseSplit.h"
#include "ReluConstraint.h"
#include "MarabouError.h"
#include "Statistics.h"
#include "TableauRow.h"

#ifdef _WIN32
#define __attribute__(x)
#endif

ReluConstraint::ReluConstraint( unsigned b, unsigned f )
    : _b( b )
    , _f( f )
    , _auxVarInUse( false )
    , _direction( PhaseStatus::PHASE_NOT_FIXED )
    , _haveEliminatedVariables( false )
{
    setPhaseStatus( PhaseStatus::PHASE_NOT_FIXED );
}

ReluConstraint::ReluConstraint( const String &serializedRelu )
    : _haveEliminatedVariables( false )
{
    String constraintType = serializedRelu.substring( 0, 4 );
    ASSERT( constraintType == String( "relu" ) );

    // Remove the constraint type in serialized form
    String serializedValues = serializedRelu.substring( 5, serializedRelu.length() - 5 );
    List<String> values = serializedValues.tokenize( "," );

    ASSERT( values.size() >= 2 && values.size() <= 3 );

    if ( values.size() == 2 )
    {
        auto var = values.begin();
        _f = atoi( var->ascii() );
        ++var;
        _b = atoi( var->ascii() );

        _auxVarInUse = false;
    }
    else
    {
        auto var = values.begin();
        _f = atoi( var->ascii() );
        ++var;
        _b = atoi( var->ascii() );
        ++var;
        _aux = atoi( var->ascii() );

        _auxVarInUse = true;
    }

    setPhaseStatus( PhaseStatus::PHASE_NOT_FIXED );
}

PiecewiseLinearFunctionType ReluConstraint::getType() const
{
    return PiecewiseLinearFunctionType::RELU;
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

    if ( _auxVarInUse )
        tableau->registerToWatchVariable( this, _aux );
}

void ReluConstraint::unregisterAsWatcher( ITableau *tableau )
{
    tableau->unregisterToWatchVariable( this, _b );
    tableau->unregisterToWatchVariable( this, _f );

    if ( _auxVarInUse )
        tableau->unregisterToWatchVariable( this, _aux );
}

void ReluConstraint::notifyVariableValue( unsigned variable, double value )
{
    if ( FloatUtils::isZero( value, GlobalConfiguration::RELU_CONSTRAINT_COMPARISON_TOLERANCE ) )
        value = 0.0;

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
        setPhaseStatus( PhaseStatus::PHASE_ACTIVE );
    else if ( variable == _b && !FloatUtils::isNegative( bound ) )
        setPhaseStatus( PhaseStatus::PHASE_ACTIVE );
    else if ( variable == _aux && FloatUtils::isPositive( bound ) )
        setPhaseStatus( PhaseStatus::PHASE_INACTIVE );

    if ( isActive() && _constraintBoundTightener )
    {
        // A positive lower bound is always propagated between f and b
        if ( ( variable == _f || variable == _b ) && bound > 0 )
        {
            unsigned partner = ( variable == _f ) ? _b : _f;
            _constraintBoundTightener->registerTighterLowerBound( partner, bound );

            // If we're in the active phase, aux should be 0
            if ( _auxVarInUse )
                _constraintBoundTightener->registerTighterUpperBound( _aux, 0 );
        }

        // If b is non-negative, we're in the active phase
        else if ( _auxVarInUse && variable == _b && FloatUtils::isZero( bound ) )
        {
            _constraintBoundTightener->registerTighterUpperBound( _aux, 0 );
        }

        // A positive lower bound for aux means we're inactive: f is 0, b is non-positive
        // When inactive, b = -aux
        else if ( _auxVarInUse && variable == _aux && bound > 0 )
        {
            _constraintBoundTightener->registerTighterUpperBound( _b, -bound );
            _constraintBoundTightener->registerTighterUpperBound( _f, 0 );
        }

        // A negative lower bound for b could tighten aux's upper bound
        else if ( _auxVarInUse && variable == _b && bound < 0 )
        {
            _constraintBoundTightener->registerTighterUpperBound( _aux, -bound );
        }

        // Also, if for some reason we only know a negative lower bound for f,
        // we attempt to tighten it to 0
        else if ( bound < 0 && variable == _f )
        {
            _constraintBoundTightener->registerTighterLowerBound( _f, 0 );
        }
    }
}

void ReluConstraint::notifyUpperBound( unsigned variable, double bound )
{
    if ( _statistics )
        _statistics->incNumBoundNotificationsPlConstraints();

    if ( _upperBounds.exists( variable ) && !FloatUtils::lt( bound, _upperBounds[variable] ) )
        return;

    _upperBounds[variable] = bound;

    if ( ( variable == _f || variable == _b ) && !FloatUtils::isPositive( bound ) )
        setPhaseStatus( PhaseStatus::PHASE_INACTIVE );

    if ( _auxVarInUse && variable == _aux && FloatUtils::isZero( bound ) )
        setPhaseStatus( PhaseStatus::PHASE_ACTIVE );

    if ( isActive() && _constraintBoundTightener )
    {
        if ( variable == _f )
        {
            // Any bound that we learned of f should be propagated to b
            _constraintBoundTightener->registerTighterUpperBound( _b, bound );
        }
        else if ( variable == _b )
        {
            if ( !FloatUtils::isPositive( bound ) )
            {
                // If b has a non-positive upper bound, f's upper bound is 0
                _constraintBoundTightener->registerTighterUpperBound( _f, 0 );

                if ( _auxVarInUse )
                {
                    // Aux's range is minus the range of b
                    _constraintBoundTightener->registerTighterLowerBound( _aux, -bound );
                }
            }
            else
            {
                // b has a positive upper bound, propagate to f
                _constraintBoundTightener->registerTighterUpperBound( _f, bound );
            }
        }
        else if ( _auxVarInUse && variable == _aux )
        {
            _constraintBoundTightener->registerTighterLowerBound( _b, -bound );
        }
    }
}

bool ReluConstraint::participatingVariable( unsigned variable ) const
{
    return ( variable == _b ) || ( variable == _f ) || ( _auxVarInUse && variable == _aux );
}

List<unsigned> ReluConstraint::getParticipatingVariables() const
{
    return _auxVarInUse?
        List<unsigned>( { _b, _f, _aux } ) :
        List<unsigned>( { _b, _f } );
}

bool ReluConstraint::satisfied() const
{
    if ( !( _assignment.exists( _b ) && _assignment.exists( _f ) ) )
        throw MarabouError( MarabouError::PARTICIPATING_VARIABLES_ABSENT );

    double bValue = _assignment.get( _b );
    double fValue = _assignment.get( _f );

    if ( FloatUtils::isNegative( fValue ) )
        return false;

    if ( FloatUtils::isPositive( fValue ) )
        return FloatUtils::areEqual( bValue, fValue, GlobalConfiguration::RELU_CONSTRAINT_COMPARISON_TOLERANCE );
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
            if ( _direction == PHASE_INACTIVE )
            {
                fixes.append( PiecewiseLinearConstraint::Fix( _f, 0 ) );
                fixes.append( PiecewiseLinearConstraint::Fix( _b, fValue ) );
            }
            else
            {
                fixes.append( PiecewiseLinearConstraint::Fix( _b, fValue ) );
                fixes.append( PiecewiseLinearConstraint::Fix( _f, 0 ) );
            }
        }
    }
    else
    {
        if ( _direction == PHASE_ACTIVE )
        {
            fixes.append( PiecewiseLinearConstraint::Fix( _f, bValue ) );
            fixes.append( PiecewiseLinearConstraint::Fix( _b, 0 ) );
        }
        else
        {
            fixes.append( PiecewiseLinearConstraint::Fix( _b, 0 ) );
            fixes.append( PiecewiseLinearConstraint::Fix( _f, bValue ) );
        }
    }

    return fixes;
}

List<PiecewiseLinearConstraint::Fix> ReluConstraint::getSmartFixes( ITableau *tableau ) const
{
    ASSERT( !satisfied() );
    ASSERT( _assignment.exists( _f ) && _assignment.size() > 1 );

    double bDeltaToFDelta;
    double fDeltaToBDelta;
    bool linearlyDependent = tableau->areLinearlyDependent( _b, _f, bDeltaToFDelta, fDeltaToBDelta );

    /*
      If b and f are linearly independent, there's nothing clever to be done -
      just return the "non-smart" fixes.

      We could potentially do something if both are basic, but for now we
      return the non-smart fixes. Some dependency may be created when f or b are
      pivoted out of the base; in which case we hope getSmartFixes will be called
      again later, where we will be able to produce smart fixes.
    */
    if ( !linearlyDependent )
        return getPossibleFixes();

    bool fIsBasic = tableau->isBasic( _f );
    bool bIsBasic = tableau->isBasic( _b );
    ASSERT( bIsBasic != fIsBasic );

    List<PiecewiseLinearConstraint::Fix> fixes;
    /*
      We know b and f are linearly dependent. This means that one of them
      is basic, the other non basic, and that coefficient is not 0.

      We know that:

        _f = ... + coefficient * _b + ...

      Next, we want to compute by how much we need to change b and/or f in order to
      repair the violation. For example, if we have:

        b = 0, f = 6

      and

        b = ... -2f ...

      And we want to repair so that f=b, we do the following computation:

        f' = f - x
        b' = b +2x
        f' = b'
        -------->
        0 + 2x = 6 - x
        -------->
        x = 2

      Giving us that we need to decrease f by 2, which will cause b to be increased
      by 4, repairing the violation. Of course, there may be multiple options for repair.
    */

    double bValue = _assignment.get( _b );
    double fValue = _assignment.get( _f );

    /*
      Repair option number 1: the active fix. We want to set f = b > 0.
    */

    if ( !bIsBasic )
    {
        /*
          bValue + delta = fValue + bDeltaToFDelta * delta
          delta = ( bValue - fValue ) / ( bDeltaToFDelta - 1 );
        */

        if ( !FloatUtils::areEqual( bDeltaToFDelta, 1.0 ) )
        {
            double activeFixDelta = ( bValue - fValue )  / ( bDeltaToFDelta - 1 );
            double activeFix = bValue + activeFixDelta;
            fixes.append( PiecewiseLinearConstraint::Fix( _b, activeFix ) );
        }
    }
    else
    {
        /*
          fValue + delta = bValue + fDeltaToBDelta * delta
          delta = ( fValue - bValue ) / ( fDeltaToBDelta - 1 );
        */
        if ( !FloatUtils::areEqual( fDeltaToBDelta, 1.0 ) )
        {
            double activeFixDelta = ( fValue - bValue )  / ( fDeltaToBDelta - 1 );
            double activeFix = fValue + activeFixDelta;
            fixes.append( PiecewiseLinearConstraint::Fix( _f, activeFix ) );
        }
    }

    /*
      Repair option number 2: the inactive fix. We want to set f = 0, b < 0.
    */

    if ( !fIsBasic )
    {
        double newBValue = bValue + fDeltaToBDelta * (-fValue);
        if ( newBValue <= 0 )
            fixes.append( PiecewiseLinearConstraint::Fix( _f, 0 ) );
    }
    else
    {
        /*
          By how much should we change b to make f zero?

          fValue + bDeltaToFDelta * delta = 0
          delta = fValue / ( -bDeltaToFDelta )
        */
        double nonactiveFixDelta = fValue / ( -bDeltaToFDelta );
        double nonactiveFix = bValue + nonactiveFixDelta;

        if ( nonactiveFix <= 0 )
            fixes.append( PiecewiseLinearConstraint::Fix( _b, nonactiveFix ) );
    }

    return fixes;
}

List<PiecewiseLinearCaseSplit> ReluConstraint::getCaseSplits() const
{
    if ( _phaseStatus != PhaseStatus::PHASE_NOT_FIXED )
        throw MarabouError( MarabouError::REQUESTED_CASE_SPLITS_FROM_FIXED_CONSTRAINT );

    List<PiecewiseLinearCaseSplit> splits;

    if ( _direction == PHASE_INACTIVE )
    {
        splits.append( getInactiveSplit() );
        splits.append( getActiveSplit() );
        return splits;
    }
    if ( _direction == PHASE_ACTIVE )
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

    if ( _auxVarInUse )
    {
        // Special case: aux var in use.
        // Because aux = f - b and aux >= 0, we just add that aux <= 0.
        activePhase.storeBoundTightening( Tightening( _aux, 0.0, Tightening::UB ) );
    }
    else
    {
        Equation activeEquation( Equation::EQ );
        activeEquation.addAddend( 1, _b );
        activeEquation.addAddend( -1, _f );
        activeEquation.setScalar( 0 );
        activePhase.addEquation( activeEquation );
    }

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
    output = Stringf( "ReluConstraint: x%u = ReLU( x%u ). Active? %s. PhaseStatus = %u (%s).\n",
                      _f, _b,
                      _constraintActive ? "Yes" : "No",
                      _phaseStatus, phaseToString( _phaseStatus ).ascii()
                      );

    output += Stringf( "b in [%s, %s], ",
                       _lowerBounds.exists( _b ) ? Stringf( "%lf", _lowerBounds[_b] ).ascii() : "-inf",
                       _upperBounds.exists( _b ) ? Stringf( "%lf", _upperBounds[_b] ).ascii() : "inf" );

    output += Stringf( "f in [%s, %s]",
                       _lowerBounds.exists( _f ) ? Stringf( "%lf", _lowerBounds[_f] ).ascii() : "-inf",
                       _upperBounds.exists( _f ) ? Stringf( "%lf", _upperBounds[_f] ).ascii() : "inf" );

    if ( _auxVarInUse )
    {
        output += Stringf( ". Aux var: %u. Range: [%s, %s]\n",
                           _aux,
                           _lowerBounds.exists( _aux ) ? Stringf( "%lf", _lowerBounds[_aux] ).ascii() : "-inf",
                           _upperBounds.exists( _aux ) ? Stringf( "%lf", _upperBounds[_aux] ).ascii() : "inf" );
    }
}

void ReluConstraint::updateVariableIndex( unsigned oldIndex, unsigned newIndex )
{
	ASSERT( oldIndex == _b || oldIndex == _f || ( _auxVarInUse && oldIndex == _aux ) );
    ASSERT( !_assignment.exists( newIndex ) &&
            !_lowerBounds.exists( newIndex ) &&
            !_upperBounds.exists( newIndex ) &&
            newIndex != _b && newIndex != _f && ( !_auxVarInUse || newIndex != _aux ) );

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
    else
        _aux = newIndex;
}

void ReluConstraint::eliminateVariable( __attribute__((unused)) unsigned variable,
                                        __attribute__((unused)) double fixedValue )
{
    ASSERT( variable == _b || variable == _f || ( _auxVarInUse && variable == _aux ) );

    DEBUG({
            if ( variable == _f )
            {
                ASSERT( FloatUtils::gte( fixedValue, 0.0 ) );
            }

            if ( variable == _f || variable == _b )
            {
                if ( FloatUtils::gt( fixedValue, 0 ) )
                {
                    ASSERT( _phaseStatus != PHASE_INACTIVE );
                }
                else if ( FloatUtils::lt( fixedValue, 0 ) )
                {
                    ASSERT( _phaseStatus != PHASE_ACTIVE );
                }
            }
            else
            {
                // This is the aux variable
                if ( FloatUtils::isPositive( fixedValue ) )
                {
                    ASSERT( _phaseStatus != PHASE_ACTIVE );
                }
            }
        });

    // In a ReLU constraint, if a variable is removed the entire constraint can be discarded.
    _haveEliminatedVariables = true;
}

bool ReluConstraint::constraintObsolete() const
{
    return _haveEliminatedVariables;
}

void ReluConstraint::getEntailedTightenings( List<Tightening> &tightenings ) const
{
    ASSERT( _lowerBounds.exists( _b ) && _lowerBounds.exists( _f ) &&
            _upperBounds.exists( _b ) && _upperBounds.exists( _f ) );

    ASSERT( !_auxVarInUse || ( _lowerBounds.exists( _aux ) && _upperBounds.exists( _aux ) ) );

    double bLowerBound = _lowerBounds[_b];
    double fLowerBound = _lowerBounds[_f];

    double bUpperBound = _upperBounds[_b];
    double fUpperBound = _upperBounds[_f];

    double auxLowerBound = 0;
    double auxUpperBound = 0;

    if ( _auxVarInUse )
    {
        auxLowerBound = _lowerBounds[_aux];
        auxUpperBound = _upperBounds[_aux];
    }

    // Determine if we are in the active phase, inactive phase or unknown phase
    if ( !FloatUtils::isNegative( bLowerBound ) ||
         FloatUtils::isPositive( fLowerBound ) ||
         ( _auxVarInUse && FloatUtils::isZero( auxUpperBound ) ) )
    {
        // Active case;

        // All bounds are propagated between b and f
        tightenings.append( Tightening( _b, fLowerBound, Tightening::LB ) );
        tightenings.append( Tightening( _f, bLowerBound, Tightening::LB ) );

        tightenings.append( Tightening( _b, fUpperBound, Tightening::UB ) );
        tightenings.append( Tightening( _f, bUpperBound, Tightening::UB ) );

        // Aux is zero
        if ( _auxVarInUse )
        {
            tightenings.append( Tightening( _aux, 0, Tightening::LB ) );
            tightenings.append( Tightening( _aux, 0, Tightening::UB ) );
        }

        tightenings.append( Tightening( _b, 0, Tightening::LB ) );
        tightenings.append( Tightening( _f, 0, Tightening::LB ) );
    }
    else if ( FloatUtils::isNegative( bUpperBound ) ||
              FloatUtils::isZero( fUpperBound ) ||
              ( _auxVarInUse && FloatUtils::isPositive( auxLowerBound ) ) )
    {
        // Inactive case

        // f is zero
        tightenings.append( Tightening( _f, 0, Tightening::LB ) );
        tightenings.append( Tightening( _f, 0, Tightening::UB ) );

        // b is non-positive
        tightenings.append( Tightening( _b, 0, Tightening::UB ) );

        // aux = -b, aux is non-negative
        if ( _auxVarInUse )
        {
            tightenings.append( Tightening( _aux, -bLowerBound, Tightening::UB ) );
            tightenings.append( Tightening( _aux, -bUpperBound, Tightening::LB ) );

            tightenings.append( Tightening( _b, -auxLowerBound, Tightening::UB ) );
            tightenings.append( Tightening( _b, -auxUpperBound, Tightening::LB ) );

            tightenings.append( Tightening( _aux, 0, Tightening::LB ) );
        }
    }
    else
    {
        // Unknown case

        // b and f share upper bounds
        tightenings.append( Tightening( _b, fUpperBound, Tightening::UB ) );
        tightenings.append( Tightening( _f, bUpperBound, Tightening::UB ) );

        // aux upper bound is -b lower bound
        if ( _auxVarInUse )
        {
            tightenings.append( Tightening( _b, -auxUpperBound, Tightening::LB ) );
            tightenings.append( Tightening( _aux, -bLowerBound, Tightening::UB ) );
        }

        // f and aux are always non negative
        tightenings.append( Tightening( _f, 0, Tightening::LB ) );
        if ( _auxVarInUse )
            tightenings.append( Tightening( _aux, 0, Tightening::LB ) );
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

void ReluConstraint::setPhaseStatus( PhaseStatus phaseStatus )
{
    _phaseStatus = phaseStatus;
}

void ReluConstraint::addAuxiliaryEquations( InputQuery &inputQuery )
{
    /*
      We want to add the equation

          f >= b

      Which actually becomes

          f - b - aux = 0

      Lower bound: always non-negative
      Upper bound: when f = 0 and b is minimal, i.e. -b.lb
    */

    // Create the aux variable
    _aux = inputQuery.getNumberOfVariables();
    inputQuery.setNumberOfVariables( _aux + 1 );

    // Create and add the equation
    Equation equation( Equation::EQ );
    equation.addAddend( 1.0, _f );
    equation.addAddend( -1.0, _b );
    equation.addAddend( -1.0, _aux );
    equation.setScalar( 0 );
    inputQuery.addEquation( equation );

    // Adjust the bounds for the new variable
    ASSERT( _lowerBounds.exists( _b ) );
    inputQuery.setLowerBound( _aux, 0 );

    // Generally, aux.ub = -b.lb. However, if b.lb is positive (active
    // phase), then aux.ub needs to be 0
    double auxUpperBound =
        _lowerBounds[_b] > 0 ? 0 : -_lowerBounds[_b];
    inputQuery.setUpperBound( _aux, auxUpperBound );

    // We now care about the auxiliary variable, as well
    _auxVarInUse = true;
}

void ReluConstraint::getCostFunctionComponent( Map<unsigned, double> &cost ) const
{
    // This should not be called for inactive constraints
    ASSERT( isActive() );

    // If the constraint is satisfied, fixed or has OOB components,
    // it contributes nothing
    if ( satisfied() || phaseFixed() || haveOutOfBoundVariables() )
        return;

    // Both variables are within bounds and the constraint is not
    // satisfied or fixed.
    double bValue = _assignment.get( _b );
    double fValue = _assignment.get( _f );

    if ( !cost.exists( _f ) )
        cost[_f] = 0;

    // Case 1: b is non-positive, f is not zero. Cost: f
    if ( !FloatUtils::isPositive( bValue ) )
    {
        ASSERT( !FloatUtils::isZero( fValue ) );
        cost[_f] = cost[_f] + 1;
        return;
    }

    ASSERT( !FloatUtils::isNegative( bValue ) );
    ASSERT( !FloatUtils::isNegative( fValue ) );

    if ( !cost.exists( _b ) )
        cost[_b] = 0;

    // Case 2: both non-negative, not equal, b > f. Cost: b - f
    if ( FloatUtils::gt( bValue, fValue ) )
    {
        cost[_b] = cost[_b] + 1;
        cost[_f] = cost[_f] - 1;
        return;
    }

    // Case 3: both non-negative, not equal, f > b. Cost: f - b
    cost[_b] = cost[_b] - 1;
    cost[_f] = cost[_f] + 1;
    return;
}

bool ReluConstraint::haveOutOfBoundVariables() const
{
    double bValue = _assignment.get( _b );
    double fValue = _assignment.get( _f );

    if ( FloatUtils::gt( _lowerBounds[_b], bValue ) || FloatUtils::lt( _upperBounds[_b], bValue ) )
        return true;

    if ( FloatUtils::gt( _lowerBounds[_f], fValue ) || FloatUtils::lt( _upperBounds[_f], fValue ) )
        return true;

    return false;
}

String ReluConstraint::serializeToString() const
{
    // Output format is: relu,f,b,aux
    if ( _auxVarInUse )
        return Stringf( "relu,%u,%u,%u", _f, _b, _aux );

    return Stringf( "relu,%u,%u", _f, _b );
}

unsigned ReluConstraint::getB() const
{
    return _b;
}

ReluConstraint::PhaseStatus ReluConstraint::getPhaseStatus() const
{
    return _phaseStatus;
}

bool ReluConstraint::supportsSymbolicBoundTightening() const
{
    return true;
}

bool ReluConstraint::supportPolarity() const
{
    return true;
}

bool ReluConstraint::auxVariableInUse() const
{
    return _auxVarInUse;
}

unsigned ReluConstraint::getAux() const
{
    return _aux;
}

double ReluConstraint::computePolarity() const
{
    double currentLb = _lowerBounds[_b];
    double currentUb = _upperBounds[_b];
    if ( currentLb >= 0 ) return 1;
    if ( currentUb <= 0 ) return -1;
    double width = currentUb - currentLb;
    double sum = currentUb + currentLb;
    return sum / width;
}

void ReluConstraint::updateDirection()
{
    _direction = ( computePolarity() > 0 ) ? PHASE_ACTIVE : PHASE_INACTIVE;
}

ReluConstraint::PhaseStatus ReluConstraint::getDirection() const
{
    return _direction;
}

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
