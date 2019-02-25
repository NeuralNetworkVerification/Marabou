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
#include "FactTracker.h"
#include "FloatUtils.h"
#include "ITableau.h"
#include "MStringf.h"
#include "PiecewiseLinearCaseSplit.h"
#include "ReluConstraint.h"
#include "ReluplexError.h"
#include "Statistics.h"
#include "TableauRow.h"
#include "assert.h"

ReluConstraint::ReluConstraint( unsigned b, unsigned f, unsigned id )
    // Guy: since _id is a member of the parent class, initialize it in the parent class
    : PiecewiseLinearConstraint( id )
    , _b( b )
    , _f( f )
    , _haveEliminatedVariables( false )
{
    // Both of these moved to parent
    // _id = id;
    // _factTracker = NULL;
    setPhaseStatus( PhaseStatus::PHASE_NOT_FIXED );
}

ReluConstraint::ReluConstraint( unsigned b, unsigned f )
{
    // Guy: lets make 0 the default ID, maybe define it in the parent class, and make all numbering start from 1.
    // Alternatively, make the default id (unsigned)-1.
    (*this) = ReluConstraint( b, f, 0 );
}

ReluConstraint::ReluConstraint( const String &serializedRelu )
    : _haveEliminatedVariables( false )
{
    String constraintType = serializedRelu.substring(0, 4);
    ASSERT(constraintType == String("relu"));

    // remove the constraint type in serialized form
    String serializedValues = serializedRelu.substring( 5, serializedRelu.length() - 5 );
    List<String> values = serializedValues.tokenize( "," );
    auto valuesIter = values.begin();
    _id = atoi( valuesIter->ascii() );
    ++valuesIter;
    _f = atoi( valuesIter->ascii() );
    ++valuesIter;
    _b = atoi( valuesIter->ascii() );
    _factTracker = NULL;

    setPhaseStatus( PhaseStatus::PHASE_NOT_FIXED );
}

PiecewiseLinearConstraint *ReluConstraint::duplicateConstraint() const
{
    ReluConstraint *clone = new ReluConstraint( _b, _f, _id );
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

    if ( isActive() && _constraintBoundTightener )
    {
        Fact* explanation = NULL;

        if( _factTracker )
        {
            assert( _factTracker->hasFactAffectingBound( variable, FactTracker::LB ) );
            explanation = const_cast<Fact*>(_factTracker->getFactAffectingBound( variable, FactTracker::LB ));
        }

        // A positive lower bound is always propagated between the two variables
        if ( bound > 0 )
        {
            unsigned partner = ( variable == _f ) ? _b : _f;

            if ( _lowerBounds.exists( partner ) )
            {
                double otherLowerBound = _lowerBounds[partner];
                if ( bound > otherLowerBound )
                    _constraintBoundTightener->registerTighterLowerBound( partner, bound, explanation );
            }
            else
            {
                _constraintBoundTightener->registerTighterLowerBound( partner, bound, explanation );
            }
        }

        // Also, if for some reason we only know a negative lower bound for f,
        // we attempt to tighten it to 0
        if ( bound < 0 && variable == _f )
        {
            _constraintBoundTightener->registerTighterLowerBound( _f, 0, explanation );
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

    if ( isActive() && _constraintBoundTightener )
    {
        Fact* explanation = NULL;

        if( _factTracker )
        {
            assert( _factTracker->hasFactAffectingBound( variable, FactTracker::UB ) );
            explanation = const_cast<Fact*>(_factTracker->getFactAffectingBound( variable, FactTracker::UB ));
        }

        if ( variable == _f )
        {
            // Any bound that we learned of f should be propagated to b
            // Junyao: check exists first before retrieving value from map
            if ( _upperBounds.exists(_b) )
            {
                if ( bound < _upperBounds[_b] )
                    _constraintBoundTightener->registerTighterUpperBound( _b, bound, explanation );
            }
            else
            {
                _constraintBoundTightener->registerTighterUpperBound( _b, bound, explanation );
            }
        }
        else
        {
            // If b has a negative upper bound, we f's upper bound is 0
            double adjustedUpperBound = FloatUtils::max( bound, 0 );
            // Junyao: check exists first before retrieving value from map
            if ( _upperBounds.exists(_f) )
            {
                if ( adjustedUpperBound < _upperBounds[_f] )
                    _constraintBoundTightener->registerTighterUpperBound( _f, adjustedUpperBound, explanation );
            }
            else
            {
                _constraintBoundTightener->registerTighterUpperBound( _f, adjustedUpperBound, explanation );   
            }
        }
    }
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

PiecewiseLinearCaseSplit ReluConstraint::getSplitFromID( unsigned splitID, bool impliedSplit/*=false*/ ) const
{
    // Write a comment somewhere in the h file, saying that split 0 is the inactive, 1 is the active.
    ASSERT( splitID == 0 || splitID == 1 );
    return splitID == 0 ? getInactiveSplit(impliedSplit) : getActiveSplit(impliedSplit);
}

PiecewiseLinearCaseSplit ReluConstraint::getInactiveSplit(bool impliedSplit/*=false*/) const
{
    assert( !_factTracker || _statistics );
    unsigned nextSplitLevel = 0;
    if (_statistics)
      nextSplitLevel = _statistics->getCurrentStackDepth() + 1;
    // Inactive phase: b <= 0, f = 0
    PiecewiseLinearCaseSplit inactivePhase;
    inactivePhase.setConstraintAndSplitID( _id, 0 );
    Tightening bound1 = Tightening( _b, 0.0, Tightening::UB );
    if(!impliedSplit)
      bound1.setCausingSplitInfo( _id, 0, nextSplitLevel );
    Tightening bound2 = Tightening( _f, 0.0, Tightening::UB );
    if(!impliedSplit)
      bound2.setCausingSplitInfo( _id, 0, nextSplitLevel );
    inactivePhase.storeBoundTightening( bound1 );
    inactivePhase.storeBoundTightening( bound2 );
    return inactivePhase;
}

PiecewiseLinearCaseSplit ReluConstraint::getActiveSplit(bool impliedSplit/*=false*/) const
{
    assert( !_factTracker || _statistics );
    unsigned nextSplitLevel = 0;
    if (_statistics)
      nextSplitLevel = _statistics->getCurrentStackDepth() + 1;
    // Active phase: b >= 0, b - f = 0
    PiecewiseLinearCaseSplit activePhase;
    activePhase.setConstraintAndSplitID( _id, 1 );
    Tightening bound = Tightening( _b, 0.0, Tightening::LB );
    // this fact will be caused by next split
    if(!impliedSplit)
      bound.setCausingSplitInfo( _id, 1, nextSplitLevel );
    activePhase.storeBoundTightening( bound );
    Equation activeEquation( Equation::EQ );
    activeEquation.addAddend( 1, _b );
    activeEquation.addAddend( -1, _f );
    activeEquation.setScalar( 0 );
    if(!impliedSplit)
      activeEquation.setCausingSplitInfo( _id, 1, nextSplitLevel );
    activePhase.addEquation( activeEquation );
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
    {
        PiecewiseLinearCaseSplit activeSplit = getActiveSplit(true);

        /*
          Guy: just to make sure I got this right:
          We're storing in advance, in the case split, the ID of the facts that participated in setting the lower
          bound of f to its current value?

          and likewise for the inactive case, below?

          Also, the logic here is that this is justified because f.lb > 0? Lets add an ASSERT statement to explain
          this. Likewise for the inactive case, and in MaxConstraint.
        */
        if ( _factTracker && _factTracker->hasFactAffectingBound( _f, FactTracker::LB ) )
            activeSplit.addExplanation( _factTracker->getFactAffectingBound( _f, FactTracker::LB ) );

        return activeSplit;
    }

    PiecewiseLinearCaseSplit inactiveSplit = getInactiveSplit(true);
    if ( _factTracker && _factTracker->hasFactAffectingBound( _b, FactTracker::UB ) )
        inactiveSplit.addExplanation( _factTracker->getFactAffectingBound( _b, FactTracker::UB ) );
    return inactiveSplit;
}

void ReluConstraint::dump( String &output ) const
{
    output = Stringf( "ReluConstraint ID = %u: x%u = ReLU( x%u ). Active? %s. PhaseStatus = %u (%s). "
                      "b in [%lf, %lf]. f in [%lf, %lf]",
                      _id,
                      _f, _b,
                      _constraintActive ? "Yes" : "No",
                      _phaseStatus, phaseToString( _phaseStatus ).ascii(),
                      _lowerBounds[_b], _upperBounds[_b], _lowerBounds[_f], _upperBounds[_f]
                      );
}

void ReluConstraint::updateVariableIndex( unsigned oldIndex, unsigned newIndex )
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

void ReluConstraint::eliminateVariable( __attribute__((unused)) unsigned variable,
                                        __attribute__((unused)) double fixedValue )
{
	ASSERT( variable == _b || variable == _f );

    DEBUG({
            if ( variable == _f )
            {
                ASSERT( FloatUtils::gte( fixedValue, 0.0 ) );
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

    // Upper bounds
    double bUpperBound = _upperBounds[_b];
    double fUpperBound = _upperBounds[_f];

    double minUpperBound =
        FloatUtils::lt( bUpperBound, fUpperBound ) ? bUpperBound : fUpperBound;

    if ( !FloatUtils::isNegative( minUpperBound ) )
    {
        // The minimal bound is non-negative. Should match for both f and b.
        if ( FloatUtils::lt( minUpperBound, bUpperBound ) )
            tightenings.append( Tightening( _b, minUpperBound, Tightening::UB ) );
        else if ( FloatUtils::lt( minUpperBound, fUpperBound ) )
            tightenings.append( Tightening( _f, minUpperBound, Tightening::UB ) );
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

    // F's lower bound should always be non-negative
    if ( FloatUtils::isNegative( fLowerBound ) )
        tightenings.append( Tightening( _f, 0.0, Tightening::LB ) );

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

void ReluConstraint::setPhaseStatus( PhaseStatus phaseStatus )
{
    _phaseStatus = phaseStatus;
}

void ReluConstraint::getAuxiliaryEquations( List<Equation> &newEquations ) const
{
    // Add the equation: f >= b, or f - b >= 0
    Equation equation( Equation::GE );
    equation.addAddend( 1.0, _f );
    equation.addAddend( -1.0, _b );
    equation.setScalar( 0 );

    newEquations.append( equation );
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
    // Output format is: relu,id,f,b
    return Stringf( "relu,%u,%u,%u", _id, _f, _b );
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

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
