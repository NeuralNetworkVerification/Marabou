/*********************                                                        */
/*! \file ReluConstraint.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Shiran Aziz, Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]
 **/

#include "AbsError.h"
#include "AbsoluteValueConstraint.h"
#include "ConstraintBoundTightener.h"
#include "Debug.h"
#include "FloatUtils.h"
#include "ITableau.h"
#include "MStringf.h"
#include "PiecewiseLinearCaseSplit.h"
#include "Statistics.h"

AbsoluteValueConstraint::AbsoluteValueConstraint( unsigned b, unsigned f )
    : _b( b )
    , _f( f )
    , _haveEliminatedVariables( false )
{
    setPhaseStatus( PhaseStatus::PHASE_NOT_FIXED );
}

PiecewiseLinearConstraint *AbsoluteValueConstraint::duplicateConstraint() const
{
    AbsoluteValueConstraint *clone = new AbsoluteValueConstraint( _b, _f );
    *clone = *this;
    return clone;
}

void AbsoluteValueConstraint::restoreState( const PiecewiseLinearConstraint *state )
{
    const AbsoluteValueConstraint *abs = dynamic_cast<const AbsoluteValueConstraint *>( state );
    *this = *abs;
}

void AbsoluteValueConstraint::registerAsWatcher( ITableau *tableau )
{
    tableau->registerToWatchVariable( this, _b );
    tableau->registerToWatchVariable( this, _f );
}

void AbsoluteValueConstraint::unregisterAsWatcher( ITableau *tableau )
{
    tableau->unregisterToWatchVariable( this, _b );
    tableau->unregisterToWatchVariable( this, _f );
}

void AbsoluteValueConstraint::notifyVariableValue( unsigned variable, double value )
{
    _assignment[variable] = value;
}

/*
  The variable watcher notification callbacks, about a change in a variable's value or bounds.
  suppose A < x_b < B, C < x_f < D
  if variable == x_b then:
    A < 0 & B < 0 then: max{|B|, C} < x_f < min{|A|, D}
    A < 0 & B > 0 then: max{0, C} < x_f < min{max{|A|, B}, D}
    A > 0 & B < 0 then: ------
    A > 0 & B > 0 then: max{A, C} < x_f < min{B, D}
  if variable == x_f then:
    C > 0 & D > 0 then: min{max{-D, A}, max{A, C}} < x_b < max{min{-C, A}, min{B, D}}
*/

void AbsoluteValueConstraint::notifyLowerBound( unsigned variable, double bound )
{
    if ( _statistics )
        _statistics->incNumBoundNotificationsPlConstraints();

    if ( _lowerBounds.exists( variable ) &&
         !FloatUtils::gt( bound, _lowerBounds[variable] ) )
        return;

    _lowerBounds[variable] = bound;

    // If b has a non-negative lower bound, phase is fixed
    if ( ( variable == _b ) && bound >= 0 )
        setPhaseStatus( PhaseStatus::PHASE_POSITIVE );

    // Update partner's bound
    if ( isActive() && _constraintBoundTightener )
    {
        if ( variable == _b )
        {
            if ( bound < 0 )
            {
                double newUpperBound = FloatUtils::abs( bound );
                if ( _upperBounds.exists( _f ) )
                    newUpperBound = FloatUtils::min( _upperBounds[_f], newUpperBound );
                _constraintBoundTightener->registerTighterUpperBound( _f, newUpperBound );
            }
            else
            {
                // Phase is fixed, don't care about this case
            }
        }
        else
        {
            if ( bound > 0 )
            {
                // F has a strictly positive lower bound

                // TODO: review this
                double newUpperBound = -1 * bound;
                double newLowerBound = bound;

                if ( _lowerBounds.exists( _b ) )
                {
                    newUpperBound = FloatUtils::min( _upperBounds[_b], newUpperBound );
                    newLowerBound = FloatUtils::max( _lowerBounds[_b], newLowerBound );
                }
                _constraintBoundTightener->registerTighterLowerBound( _b, newLowerBound );
                _constraintBoundTightener->registerTighterUpperBound( _b, newUpperBound );
            }
        }

        // Also, if for some reason we only know a negative lower bound for f,
        // we attempt to tighten it to 0
        if ( bound < 0 && variable == _f )
            _constraintBoundTightener->registerTighterLowerBound( _f, 0 );
    }
}

void AbsoluteValueConstraint::notifyUpperBound( unsigned variable, double bound )
{
    if ( _statistics )
        _statistics->incNumBoundNotificationsPlConstraints();

    if ( _upperBounds.exists( variable ) && !FloatUtils::lt( bound, _upperBounds[variable] ) )
        return;

    _upperBounds[variable] = bound;

    // If b has a non-positive upper bound, phase is fixed
    if ( ( variable == _b ) && bound <= 0 )
        setPhaseStatus( PhaseStatus::PHASE_NEGATIVE );

    // Update partner's bound
    if ( isActive() && _constraintBoundTightener )
    {
        if ( variable == _b )
        {
            if ( bound > 0 )
            {
                double newUpperBound = bound;
                if ( _upperBounds.exists( _f ) )
                    newUpperBound = FloatUtils::min( _upperBounds[_f], newUpperBound );
                _constraintBoundTightener->registerTighterUpperBound( _f, newUpperBound );
            }
            else
            {
                // Phase is fixed, don't care about this case
            }
        }
        else
        {
            if ( bound > 0 )
            {
                // F has a strictly positive upper bound

                // TODO: review this
                double newUpperBound = bound;
                double newLowerBound = -1 * bound;
                if ( _lowerBounds.exists( _b ) )
                {
                    newUpperBound = FloatUtils::min( _upperBounds[_b], newUpperBound );
                    newLowerBound = FloatUtils::max( _lowerBounds[_b], newLowerBound );
                }
                _constraintBoundTightener->registerTighterLowerBound( _b, newLowerBound );
                _constraintBoundTightener->registerTighterUpperBound( _b, newUpperBound );
            }
        }
    }
}

bool AbsoluteValueConstraint::participatingVariable(unsigned variable ) const
{
    return ( variable == _b ) || ( variable == _f );
}

List<unsigned> AbsoluteValueConstraint::getParticipatingVariables() const
{
    return List<unsigned>( { _b, _f } );
}

bool AbsoluteValueConstraint::satisfied() const
{
    if ( !( _assignment.exists( _b ) && _assignment.exists( _f ) ) )
        throw AbsError( AbsError::PARTICIPATING_VARIABLES_ABSENT );

    double bValue = _assignment.get( _b );
    double fValue = _assignment.get( _f );

    // Possible violations:
    //   1. f is negative
    //   2. f is positive, abs(b) and f are not equal

    if ( fValue < 0 )
        return false;

    return FloatUtils::areEqual( FloatUtils::abs( bValue ),
                                 fValue,
                                 GlobalConfiguration::ABS_CONSTRAINT_COMPARISON_TOLERANCE );
}

List<PiecewiseLinearConstraint::Fix> AbsoluteValueConstraint::getPossibleFixes() const
{
    ASSERT( !satisfied() );
    ASSERT( _assignment.exists( _b ) );
    ASSERT( _assignment.exists( _f ) );

    double bValue = _assignment.get( _b );
    double fValue = _assignment.get( _f );

    ASSERT( !FloatUtils::isNegative( fValue ) );

    List<PiecewiseLinearConstraint::Fix> fixes;

    // Possible violations:
    //   1. f is positive, b is positive, b and f are unequal
    //   2. f is positive, b is negative, -b and f are unequal

    fixes.append( PiecewiseLinearConstraint::Fix( _b, fValue ) );
    fixes.append( PiecewiseLinearConstraint::Fix( _b, -fValue ) );
    fixes.append( PiecewiseLinearConstraint::Fix( _f, abs( bValue ) ) );

    return fixes;
}

List<PiecewiseLinearConstraint::Fix> AbsoluteValueConstraint::getSmartFixes( __attribute__((unused)) ITableau *tableau ) const
{
    return getPossibleFixes();
}

List<PiecewiseLinearCaseSplit> AbsoluteValueConstraint::getCaseSplits() const
{
    if ( _phaseStatus != PhaseStatus::PHASE_NOT_FIXED )
        throw AbsError( AbsError::REQUESTED_CASE_SPLITS_FROM_FIXED_CONSTRAINT );

    List<PiecewiseLinearCaseSplit> splits;
    splits.append( getNegativeSplit() );
    splits.append( getPositiveSplit() );

    return splits;
}

PiecewiseLinearCaseSplit AbsoluteValueConstraint::getNegativeSplit() const
{
    PiecewiseLinearCaseSplit negativePhase;

    // Negative phase: b <= 0, b + f = 0
    negativePhase.storeBoundTightening( Tightening( _b, 0.0, Tightening::UB ) );

    Equation negativeEquation( Equation::EQ );
    negativeEquation.addAddend( 1, _b );
    negativeEquation.addAddend( 1, _f );
    negativeEquation.setScalar( 0 );
    negativePhase.addEquation( negativeEquation );

    return negativePhase;
}

PiecewiseLinearCaseSplit AbsoluteValueConstraint::getPositiveSplit() const {
    PiecewiseLinearCaseSplit positivePhase;

    // Positive phase: b >= 0, b - f = 0
    positivePhase.storeBoundTightening( Tightening( _b, 0.0, Tightening::LB ) );

    //b - f = 0
    Equation positiveEquation( Equation::EQ );
    positiveEquation.addAddend( 1, _b );
    positiveEquation.addAddend( -1, _f );
    positiveEquation.setScalar( 0 );
    positivePhase.addEquation( positiveEquation );

    return positivePhase;
}

bool AbsoluteValueConstraint::phaseFixed() const
{
    return _phaseStatus != PhaseStatus::PHASE_NOT_FIXED;
}

PiecewiseLinearCaseSplit AbsoluteValueConstraint::getValidCaseSplit() const
{
    ASSERT( _phaseStatus != PhaseStatus::PHASE_NOT_FIXED );

    if ( _phaseStatus == PhaseStatus::PHASE_POSITIVE )
        return getPositiveSplit();

    return getNegativeSplit();
}

void AbsoluteValueConstraint::eliminateVariable(__attribute__((unused)) unsigned variable,
                                                __attribute__((unused)) double fixedValue )
{
    // In an absolute value constraint, if a variable is removed the
    // entire constraint can be discarded
    _haveEliminatedVariables = true;
}

void AbsoluteValueConstraint::updateVariableIndex( unsigned oldIndex, unsigned newIndex )
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

bool AbsoluteValueConstraint::constraintObsolete() const
{
    return _haveEliminatedVariables;
}

void AbsoluteValueConstraint::getEntailedTightenings( List<Tightening> &tightenings ) const
{
    ASSERT( _lowerBounds.exists( _b ) && _lowerBounds.exists( _f ) &&
            _upperBounds.exists( _b ) && _upperBounds.exists( _f ) );

    // Upper bounds
    double bUpperBound = _upperBounds[_b];
    double fUpperBound = _upperBounds[_f];
    // Lower bounds
    double bLowerBound = _lowerBounds[_b];
    double fLowerBound = _lowerBounds[_f];

    // F's lower bound should always be non-negative
    if ( fLowerBound < 0 )
    {
        tightenings.append( Tightening( _f, 0.0, Tightening::LB ) );
        fLowerBound = 0;
    }

    if ( bLowerBound >= 0 )
    {
        // Positive phase, all bounds much match
        tightenings.append( Tightening( _f, bLowerBound, Tightening::LB ) );
        tightenings.append( Tightening( _b, fLowerBound, Tightening::LB ) );

        tightenings.append( Tightening( _f, bUpperBound, Tightening::UB ) );
        tightenings.append( Tightening( _b, fUpperBound, Tightening::UB ) );
    }

    else if ( bUpperBound <= 0 )
    {
        // Negative phase, all bounds must match

        tightenings.append( Tightening( _f, -bUpperBound, Tightening::LB ) );
        tightenings.append( Tightening( _b, -fUpperBound, Tightening::LB ) );

        tightenings.append( Tightening( _f, -bLowerBound, Tightening::UB ) );
        tightenings.append( Tightening( _b, -fLowerBound, Tightening::UB ) );
    }

    else if ( bLowerBound < 0 && bUpperBound >= 0 && FloatUtils::isZero( fLowerBound ) )
    {
        // Phase undetermined, b can be either positive or negative, f can be 0
        tightenings.append( Tightening( _b, -fUpperBound , Tightening::LB ) );
        tightenings.append( Tightening( _b, fUpperBound, Tightening::UB ) );
        tightenings.append( Tightening( _f, FloatUtils::max( -bLowerBound , bUpperBound ), Tightening::UB ) );
    }

    else if ( bLowerBound < 0 && bUpperBound >= 0 && fLowerBound > 0 )
    {
        // Phase undetermined, b can be either positive or negative, f strictly positive
        tightenings.append( Tightening( _b, -fUpperBound, Tightening::LB ) );

        if ( fLowerBound > -bLowerBound )
        {
            // Constraint is actually in the positive phsae
            tightenings.append( Tightening( _b, fLowerBound, Tightening::LB ) );
        }

        if ( fLowerBound > bUpperBound )
        {
            // Constraint is actually in the negative phsae
            tightenings.append( Tightening( _b, -fLowerBound, Tightening::UB ) );
        }

        tightenings.append( Tightening( _b, fUpperBound, Tightening::UB ) );
        tightenings.append( Tightening( _f, FloatUtils::max( -bLowerBound, bUpperBound ), Tightening::UB ) );
    }
}

void AbsoluteValueConstraint::getAuxiliaryEquations( __attribute__((unused)) List<Equation> &newEquations ) const
{
    // Currently unsupported
}

String AbsoluteValueConstraint::serializeToString() const
{
    // Output format is: Abs,f,b
    return Stringf( "absoluteValue,%u,%u", _f, _b );
}

void AbsoluteValueConstraint::setPhaseStatus( PhaseStatus phaseStatus )
{
    _phaseStatus = phaseStatus;
}

bool AbsoluteValueConstraint::supportsSymbolicBoundTightening() const
{
    return false;
}

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
