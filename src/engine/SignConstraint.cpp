//
// Created by guyam on 5/20/20.
//

#include "SignConstraint.h"

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




SignConstraint::SignConstraint( unsigned b, unsigned f )
        : _b( b )
        , _f( f )
        , _direction( PhaseStatus::PHASE_NOT_FIXED )
        , _haveEliminatedVariables( false )
{
    setPhaseStatus( PhaseStatus::PHASE_NOT_FIXED );
}




PiecewiseLinearConstraint *SignConstraint::duplicateConstraint() const
{
    SignConstraint *clone = new SignConstraint( _b, _f );
    *clone = *this;
    return clone;
}

void SignConstraint::restoreState( const PiecewiseLinearConstraint *state )
{
    const SignConstraint *sign = dynamic_cast<const SignConstraint *>( state );
    *this = *sign;
}


void SignConstraint::registerAsWatcher( ITableau *tableau )
{
    tableau->registerToWatchVariable( this, _b );
    tableau->registerToWatchVariable( this, _f );


}

void SignConstraint::unregisterAsWatcher( ITableau *tableau )
{
    tableau->unregisterToWatchVariable( this, _b );
    tableau->unregisterToWatchVariable( this, _f );

}


bool SignConstraint::participatingVariable( unsigned variable ) const
{
    return ( variable == _b ) || ( variable == _f );
}

List<unsigned> SignConstraint::getParticipatingVariables() const
{
    return List<unsigned>( { _b, _f } );
}



bool SignConstraint::satisfied() const
{
    if ( !( _assignment.exists( _b ) && _assignment.exists( _f ) ) )
        throw MarabouError( MarabouError::PARTICIPATING_VARIABLES_ABSENT );

    double bValue = _assignment.get( _b );
    double fValue = _assignment.get( _f );

    if ( FloatUtils::isNegative( bValue ) && FloatUtils::areEqual( fValue, -1 ) )
        return true;

    if ( FloatUtils::isPositive( bValue ) && FloatUtils::areEqual( fValue, 1 ) )
        return true;

    else
        return false;
}


List<PiecewiseLinearCaseSplit> SignConstraint::getCaseSplits() const {
    if (_phaseStatus != PhaseStatus::PHASE_NOT_FIXED)
        throw MarabouError(MarabouError::REQUESTED_CASE_SPLITS_FROM_FIXED_CONSTRAINT);

    List <PiecewiseLinearCaseSplit> splits;

    if (_direction == PHASE_NEGATIVE) {
        splits.append(getNegativeSplit());
        splits.append(getPositiveSplit());
        return splits;
    }
    if (_direction == PHASE_POSITIVE) {
        splits.append(getPositiveSplit());
        splits.append(getNegativeSplit());
        return splits;
    }

    // If we have existing knowledge about the assignment, use it to
    // influence the order of splits
    if (_assignment.exists(_f)) {
        if (FloatUtils::isPositive(_assignment[_f])) {
            splits.append(getPositiveSplit());
            splits.append(getNegativeSplit());
        } else {
            splits.append(getNegativeSplit());
            splits.append(getPositiveSplit());
        }
    } else {
        // Default: start with the inactive case, because it doesn't
        // introduce a new equation and is hence computationally cheaper.
        splits.append(getNegativeSplit()); // notice no change from ReLU function
        splits.append(getPositiveSplit()); // notice no change from ReLU function
    }

    return splits;
}


PiecewiseLinearCaseSplit SignConstraint::getNegativeSplit() const
{
    // Negative phase: b <= 0, f = -1
    PiecewiseLinearCaseSplit negativePhase;
    negativePhase.storeBoundTightening( Tightening( _b, 0.0, Tightening::UB ) );
    negativePhase.storeBoundTightening( Tightening( _f, -1.0, Tightening::UB ) );
    return negativePhase;
}

PiecewiseLinearCaseSplit SignConstraint::getPositiveSplit() const {
    // Positive phase: b >= 0, b - f = 0
    PiecewiseLinearCaseSplit positivePhase;
    positivePhase.storeBoundTightening(Tightening(_b, 0.0, Tightening::LB));
    positivePhase.storeBoundTightening(Tightening(_f, 1.0, Tightening::LB));
    return positivePhase;
}






bool SignConstraint::phaseFixed() const
{
    return _phaseStatus != PhaseStatus::PHASE_NOT_FIXED;
}


PiecewiseLinearCaseSplit SignConstraint::getValidCaseSplit() const
{
    ASSERT( _phaseStatus != PhaseStatus::PHASE_NOT_FIXED );

    if ( _phaseStatus == PhaseStatus::PHASE_POSITIVE )
        return getPositiveSplit();

    return getNegativeSplit();
}



bool SignConstraint::constraintObsolete() const
{
    return _haveEliminatedVariables;
}





String SignConstraint::serializeToString() const
{
    // Output format is: relu,f,b
    return Stringf( "sign,%u,%u", _f, _b );
}



bool SignConstraint::haveOutOfBoundVariables() const
{
    double bValue = _assignment.get( _b );
    double fValue = _assignment.get( _f );

    if ( FloatUtils::gt( _lowerBounds[_b], bValue ) || FloatUtils::lt( _upperBounds[_b], bValue ) )
        return true;

    if ( FloatUtils::gt( _lowerBounds[_f], fValue ) || FloatUtils::lt( _upperBounds[_f], fValue ) )
        return true;

    return false;
}


String SignConstraint::phaseToString( PhaseStatus phase )
{
    switch ( phase )
    {
        case PHASE_NOT_FIXED:
            return "PHASE_NOT_FIXED";

        case PHASE_POSITIVE:
            return "PHASE_POSITIVE";

        case PHASE_NEGATIVE:
            return "PHASE_NEGATIVE";

        default:
            return "UNKNOWN";
    }
};


void SignConstraint::notifyVariableValue( unsigned variable, double value )
{
    if ( FloatUtils::isZero( value, GlobalConfiguration::RELU_CONSTRAINT_COMPARISON_TOLERANCE ) )
        value = 0.0;

    _assignment[variable] = value;
}


void SignConstraint::notifyLowerBound( unsigned variable, double bound )
{
    if ( _statistics )
        _statistics->incNumBoundNotificationsPlConstraints();

    // if lower bound exists and better than suggested 'bound' input - return
    if ( _lowerBounds.exists( variable ) && !FloatUtils::gt( bound, _lowerBounds[variable] ) )
        return;
    // otherwise - update bound
    _lowerBounds[variable] = bound;

    if ( variable == _f && FloatUtils::gt( bound, -1 ))
        setPhaseStatus( PhaseStatus::PHASE_POSITIVE );
    else if ( variable == _b && !FloatUtils::isNegative( bound ) )  // if b (input) is >= 0
        setPhaseStatus( PhaseStatus::PHASE_POSITIVE );
// todo - eventually unmask code for _constraintBoundTightener
//    if ( isActive() && _constraintBoundTightener )
//    {
//        // A positive lower bound is always propagated between f and b
//        if ( bound > 0 )
//        {
//            unsigned partner = ( variable == _f ) ? _b : _f;
//            _constraintBoundTightener->registerTighterLowerBound( partner, bound );
//        }
//
//            // Also, if for some reason we only know a negative lower bound for f,
//            // we attempt to tighten it to 0
//        else if ( bound < 0 && variable == _f )
//        {
//            _constraintBoundTightener->registerTighterLowerBound( _f, 0 );
//        }
//    }
}


void SignConstraint::notifyUpperBound(unsigned variable, double bound)
{
    if ( _statistics )
        _statistics->incNumBoundNotificationsPlConstraints();

    if ( _upperBounds.exists( variable ) && !FloatUtils::lt( bound, _upperBounds[variable] ) )
        return;

    _upperBounds[variable] = bound;

    if ( variable == _f && FloatUtils::lt( bound, 1 ))
        setPhaseStatus( PhaseStatus::PHASE_NEGATIVE );
    else if ( variable == _b && FloatUtils::isNegative( bound ) )  // if b (input) is < 0
        setPhaseStatus( PhaseStatus::PHASE_NEGATIVE );
// todo - eventually unmask code for _constraintBoundTightener
//    if ( isActive() && _constraintBoundTightener )
//    {
//        // A positive lower bound is always propagated between f and b
//        if ( bound > 0 )
//        {
//            unsigned partner = ( variable == _f ) ? _b : _f;
//            _constraintBoundTightener->registerTighterLowerBound( partner, bound );
//        }
//
//            // Also, if for some reason we only know a negative lower bound for f,
//            // we attempt to tighten it to 0
//        else if ( bound < 0 && variable == _f )
//        {
//            _constraintBoundTightener->registerTighterLowerBound( _f, 0 );
//        }
//    }
}



// todo - check thoroughly!
// todo - Guy - do I need also conditions of phases?
List<PiecewiseLinearConstraint::Fix> SignConstraint::getPossibleFixes() const
{
    ASSERT( !satisfied() );
    ASSERT( _assignment.exists( _b ) );
    ASSERT( _assignment.exists( _f ) );

    double bValue = _assignment.get( _b );
    double fValue = _assignment.get( _f );

    List<PiecewiseLinearConstraint::Fix> fixes;

    // Possible violations:
    //   1. f is NOT +1, b is >= 0
    //   2. f is  NOT -1, b is < 0
    if ( !FloatUtils::isNegative( bValue ) ) // if b >= 0 -> make f = 1
    {
        if (!FloatUtils::areEqual(fValue, 1)) {
            fixes.append(PiecewiseLinearConstraint::Fix(_f, 1));
        }
    }
     else  // if b < 0 -> make f = (-1)
    {
        if (!FloatUtils::areEqual(fValue, -1)) {
            fixes.append(PiecewiseLinearConstraint::Fix(_f, -1));
        }
    }
    return fixes;
}




List<PiecewiseLinearConstraint::Fix> SignConstraint::getSmartFixes( ITableau *tableau ) const
{
    ASSERT(!satisfied());
    ASSERT(_assignment.exists(_f) && _assignment.size() > 1);
    (void) tableau; // todo - void line added so compiler will see var used

    return getPossibleFixes();
}



void SignConstraint::getEntailedTightenings( List<Tightening> &tightenings ) const
{
    ASSERT( _lowerBounds.exists( _b ) && _lowerBounds.exists( _f ) &&
            _upperBounds.exists( _b ) && _upperBounds.exists( _f ) );


    double bLowerBound = _lowerBounds[_b];
    double fLowerBound = _lowerBounds[_f];

    double bUpperBound = _upperBounds[_b];
    double fUpperBound = _upperBounds[_f];

    // always make f between -1 and 1
    tightenings.append( Tightening( _f, -1, Tightening::LB ) );
    tightenings.append( Tightening( _f, 1, Tightening::UB ) );

    // any other update can be done only if we are in the POSITIVE phase or the NEGATIVE phase

    // Determine if we are in the positive phase, negative phase or unknown phase
    if ( !FloatUtils::isNegative( bLowerBound ) ||
         FloatUtils::gt(fLowerBound, -1))
    {
        // positive case

        tightenings.append( Tightening( _f, 1, Tightening::LB ) );
    }
    else if ( FloatUtils::isNegative( bUpperBound ) ||
              FloatUtils::lt(fUpperBound, 1) )
    {
        // negative case

        tightenings.append( Tightening( _f, -1, Tightening::UB ) );
    }

}




void SignConstraint::setPhaseStatus( PhaseStatus phaseStatus )
{
    _phaseStatus = phaseStatus;
}




void SignConstraint::updateVariableIndex( unsigned oldIndex, unsigned newIndex )
{
    ASSERT( oldIndex == _b || oldIndex == _f  );
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

void SignConstraint::eliminateVariable( __attribute__((unused)) unsigned variable,
                                        __attribute__((unused)) double fixedValue )
{
    ASSERT( variable == _b || variable == _f );

    DEBUG({
              if ( variable == _f )
              {
                  ASSERT((FloatUtils::areEqual(fixedValue,1))||(FloatUtils::areEqual(fixedValue,-1) ));
                  if ( FloatUtils::areEqual(fixedValue, 1 ) )
                  {
                      ASSERT( _phaseStatus != PHASE_NEGATIVE );
                  }
                  else if (FloatUtils::areEqual(fixedValue, -1 )  )
                  {
                      ASSERT( _phaseStatus != PHASE_POSITIVE );
                  }
              }
              else if (variable == _b)
              {
                  if ( FloatUtils::gte( fixedValue, 0 ) )
                  {
                      ASSERT( _phaseStatus != PHASE_NEGATIVE );
                  }
                  else if ( FloatUtils::lt( fixedValue, 0 ) )
                  {
                      ASSERT( _phaseStatus != PHASE_POSITIVE );
                  }
              }
          };
    // In a Sign constraint, if a variable is removed the entire constraint can be discarded.
    _haveEliminatedVariables = true;
}