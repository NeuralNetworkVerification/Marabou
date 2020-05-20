//
// Created by guyam on 5/20/20.
//

#include "SignConstraint.hpp"

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
        , _auxVarInUse( false )
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

    if ( _auxVarInUse )
        tableau->registerToWatchVariable( this, _aux );
}

void SignConstraint::unregisterAsWatcher( ITableau *tableau )
{
    tableau->unregisterToWatchVariable( this, _b );
    tableau->unregisterToWatchVariable( this, _f );

    if ( _auxVarInUse ) // todo - check if line is relevant? (originally from ReLU)
        tableau->unregisterToWatchVariable( this, _aux );
}


bool SignConstraint::participatingVariable( unsigned variable ) const
{
    return ( variable == _b ) || ( variable == _f ) || ( _auxVarInUse && variable == _aux );
}

List<unsigned> SignConstraint::getParticipatingVariables() const
{
    return _auxVarInUse?
           List<unsigned>( { _b, _f, _aux } ) :
           List<unsigned>( { _b, _f } );
}



bool SignConstraint::satisfied() const // todo check
{
    if ( !( _assignment.exists( _b ) && _assignment.exists( _f ) ) )
        throw MarabouError( MarabouError::PARTICIPATING_VARIABLES_ABSENT );

    double bValue = _assignment.get( _b );
    double fValue = _assignment.get( _f );

    if ( FloatUtils::isNegative( bValue ) && fValue == -1 )
        return true;

    if ( FloatUtils::isPositive( bValue ) && fValue == 1 )
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


List<PiecewiseLinearConstraint::Fix> SignConstraint::getPossibleFixes() const {
    ASSERT(!satisfied());
    ASSERT(_assignment.exists(_b));
    ASSERT(_assignment.exists(_f));

    double bValue = _assignment.get(_b);
    double fValue = _assignment.get(_f);

    ASSERT(!FloatUtils::isNegative(fValue));

    List <PiecewiseLinearConstraint::Fix> fixes;

    // todo fil - after consulting with Guy - TO IMPLEMENT

    return fixes;
}



List<PiecewiseLinearConstraint::Fix> SignConstraint::getSmartFixes( ITableau *tableau ) const {
    ASSERT(!satisfied());
    ASSERT(_assignment.exists(_f) && _assignment.size() > 1);

    double bDeltaToFDelta;
    double fDeltaToBDelta;
    bool linearlyDependent = tableau->areLinearlyDependent(_b, _f, bDeltaToFDelta, fDeltaToBDelta);

    /*
      If b and f are linearly independent, there's nothing clever to be done -
      just return the "non-smart" fixes.

      We could potentially do something if both are basic, but for now we
      return the non-smart fixes. Some dependency may be created when f or b are
      pivoted out of the base; in which case we hope getSmartFixes will be called
      again later, where we will be able to produce smart fixes.
    */
    if (!linearlyDependent)
        return getPossibleFixes();

    bool fIsBasic = tableau->isBasic(_f);
    bool bIsBasic = tableau->isBasic(_b);
    ASSERT(bIsBasic != fIsBasic);

    List <PiecewiseLinearConstraint::Fix> fixes;

    // todo fil - after consulting with Guy - TO IMPLEMENT

    return fixes;
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



void SignConstraint::getEntailedTightenings( List<Tightening> &tightenings ) const
{

    // todo fil - after consulting with Guy - TO IMPLEMENT

    return;
}



String SignConstraint::serializeToString() const
{
    // Output format is: relu,f,b,aux
    if ( _auxVarInUse ) // todo - check if AUX variable used?
        return Stringf( "sign,%u,%u,%u", _f, _b, _aux );

    return Stringf( "sign,%u,%u", _f, _b );
}
