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

#include "AbsoluteValueConstraint.h"
#include "ConstraintBoundTightener.h"
#include "Debug.h"
#include "FloatUtils.h"
#include "ITableau.h"
#include "InputQuery.h"
#include "MStringf.h"
#include "MarabouError.h"
#include "PiecewiseLinearCaseSplit.h"
#include "Statistics.h"

AbsoluteValueConstraint::AbsoluteValueConstraint( unsigned b, unsigned f )
    : _b( b )
    , _f( f )
    , _auxVarsInUse( false )
    , _haveEliminatedVariables( false )
{
    setPhaseStatus( PhaseStatus::PHASE_NOT_FIXED );
}

AbsoluteValueConstraint::AbsoluteValueConstraint( const String &serializedAbs )
    : _haveEliminatedVariables( false )
{
    String constraintType = serializedAbs.substring( 0, 13 );
    ASSERT( constraintType == String( "absoluteValue" ) );

    // Remove the constraint type in serialized form
    String serializedValues = serializedAbs.substring( 14, serializedAbs.length() - 14 );
    List<String> values = serializedValues.tokenize( "," );

    ASSERT( values.size() >= 2 || values.size() <= 4 );

    if ( values.size() == 2 )
    {
        auto var = values.begin();
        _f = atoi( var->ascii() );
        ++var;
        _b = atoi( var->ascii() );
    }
    else
    {
        auto var = values.begin();
        _f = atoi( var->ascii() );
        ++var;
        _b = atoi( var->ascii() );
        ++var;
        _posAux = atoi( var->ascii() );
        ++var;
        _negAux = atoi( var->ascii() );

        _auxVarsInUse = true;
    }

    setPhaseStatus( PhaseStatus::PHASE_NOT_FIXED );
}

PiecewiseLinearFunctionType AbsoluteValueConstraint::getType() const
{
    return PiecewiseLinearFunctionType::ABSOLUTE_VALUE;
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

    if ( _auxVarsInUse )
    {
        tableau->registerToWatchVariable( this, _posAux );
        tableau->registerToWatchVariable( this, _negAux );
    }
}

void AbsoluteValueConstraint::unregisterAsWatcher( ITableau *tableau )
{
    tableau->unregisterToWatchVariable( this, _b );
    tableau->unregisterToWatchVariable( this, _f );

    if ( _auxVarsInUse )
    {
        tableau->unregisterToWatchVariable( this, _posAux );
        tableau->unregisterToWatchVariable( this, _negAux );
    }
}

void AbsoluteValueConstraint::notifyVariableValue( unsigned variable, double value )
{
    _assignment[variable] = value;
}

void AbsoluteValueConstraint::notifyLowerBound( unsigned variable, double bound )
{
    if ( _statistics )
        _statistics->incNumBoundNotificationsPlConstraints();

    if ( _lowerBounds.exists( variable ) &&
         !FloatUtils::gt( bound, _lowerBounds[variable] ) )
        return;

    _lowerBounds[variable] = bound;

    // Check whether the phase has become fixed
    fixPhaseIfNeeded();

    // Update partner's bound
    if ( isActive() && _constraintBoundTightener )
    {
        if ( variable == _b )
        {
            if ( bound < 0 )
            {
                double fUpperBound = FloatUtils::max( -bound, _upperBounds[_b] );
                _constraintBoundTightener->registerTighterUpperBound( _f, fUpperBound );

                if ( _auxVarsInUse )
                {
                    _constraintBoundTightener->
                        registerTighterUpperBound( _posAux, fUpperBound - bound );
                }
            }
            else
            {
                // Phase is fixed, don't care about this case
            }
        }
        else if ( variable == _f )
        {
            // F's lower bound can only cause bound propagations if it
            // fixes the phase of the constraint, so no need to
            // bother. The only exception is if the lower bound is,
            // for some reason, negative
            if ( bound < 0 )
                _constraintBoundTightener->registerTighterLowerBound( _f, 0 );
        }

        // Any lower bound tightneing on the aux variables, if they
        // are used, must have already fixed the phase, and needs not
        // be considered
    }
}

void AbsoluteValueConstraint::notifyUpperBound( unsigned variable, double bound )
{
    if ( _statistics )
        _statistics->incNumBoundNotificationsPlConstraints();

    if ( _upperBounds.exists( variable ) && !FloatUtils::lt( bound, _upperBounds[variable] ) )
        return;

    _upperBounds[variable] = bound;

    // Check whether the phase has become fixed
    fixPhaseIfNeeded();

    // Update partner's bound
    if ( isActive() && _constraintBoundTightener )
    {
        if ( variable == _b )
        {
            if ( bound > 0 )
            {
                double fUpperBound = FloatUtils::max( bound, -_lowerBounds[_b] );
                _constraintBoundTightener->registerTighterUpperBound( _f, fUpperBound );

                if ( _auxVarsInUse )
                {
                    _constraintBoundTightener->
                        registerTighterUpperBound( _negAux, fUpperBound + bound );
                }
            }
            else
            {
                // Phase is fixed, don't care about this case
            }
        }
        else if ( variable == _f )
        {
            // F's upper bound can restrict both bounds of B
            if ( bound < _upperBounds[_b] )
                _constraintBoundTightener->registerTighterUpperBound( _b, bound );

            if ( -bound > _lowerBounds[_b] )
                _constraintBoundTightener->registerTighterLowerBound( _b, -bound );

            if ( _auxVarsInUse )
            {
                // And also the upper bounds of both aux variables
                _constraintBoundTightener->
                    registerTighterUpperBound( _posAux, bound - _lowerBounds[_b] );

                _constraintBoundTightener->
                    registerTighterUpperBound( _negAux, bound + _lowerBounds[_b] );
            }
        }
        else if ( _auxVarsInUse )
        {
            if ( variable == _posAux )
            {
                // posAux.ub = f.ub - b.lb, and so this tightening can cause:
                //    1. f.ub = b.lb + posAux.ub
                //    2. b.lb = f.ub - posAux.ub
                _constraintBoundTightener->
                    registerTighterUpperBound( _f, _lowerBounds[_b] + bound );
                _constraintBoundTightener->
                    registerTighterLowerBound( _b, _upperBounds[_f] - bound );
            }
            else if ( variable == _negAux )
            {
                // negAux.ub = f.ub + b.ub, and so this tightening can cause:
                //    1. f.ub = negAux.ub - b.ub
                //    2. b.ub = negAux.ub - f.ub
                _constraintBoundTightener->
                    registerTighterUpperBound( _f, bound - _upperBounds[_b] );
                _constraintBoundTightener->
                    registerTighterUpperBound( _b, bound - _upperBounds[_f] );
            }
        }
    }
}

bool AbsoluteValueConstraint::participatingVariable(unsigned variable ) const
{
    return ( variable == _b ) || ( variable == _f )
        || ( _auxVarsInUse && ( variable == _posAux || variable == _negAux ) );
}

List<unsigned> AbsoluteValueConstraint::getParticipatingVariables() const
{
    return _auxVarsInUse ?
        List<unsigned>( { _b, _f, _posAux, _negAux } ) :
        List<unsigned>( { _b, _f } );
}

bool AbsoluteValueConstraint::satisfied() const
{
    if ( !( _assignment.exists( _b ) && _assignment.exists( _f ) ) )
        throw MarabouError( MarabouError::PARTICIPATING_VARIABLES_ABSENT );

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
    fixes.append( PiecewiseLinearConstraint::Fix( _f, FloatUtils::abs( bValue ) ) );

    return fixes;
}

List<PiecewiseLinearConstraint::Fix> AbsoluteValueConstraint::getSmartFixes( ITableau */* tableau */ ) const
{
    return getPossibleFixes();
}

List<PiecewiseLinearCaseSplit> AbsoluteValueConstraint::getCaseSplits() const
{
    ASSERT( _phaseStatus == PhaseStatus::PHASE_NOT_FIXED );

    List<PiecewiseLinearCaseSplit> splits;
    splits.append( getNegativeSplit() );
    splits.append( getPositiveSplit() );

    return splits;
}

PiecewiseLinearCaseSplit AbsoluteValueConstraint::getNegativeSplit() const
{
    // Negative phase: b <= 0, b + f = 0
    PiecewiseLinearCaseSplit negativePhase;
    negativePhase.storeBoundTightening( Tightening( _b, 0.0, Tightening::UB ) );

    if ( _auxVarsInUse )
    {
        negativePhase.storeBoundTightening( Tightening( _negAux, 0.0, Tightening::UB ) );
    }
    else
    {
        Equation negativeEquation( Equation::EQ );
        negativeEquation.addAddend( 1, _b );
        negativeEquation.addAddend( 1, _f );
        negativeEquation.setScalar( 0 );
        negativePhase.addEquation( negativeEquation );
    }

    return negativePhase;
}

PiecewiseLinearCaseSplit AbsoluteValueConstraint::getPositiveSplit() const
{
    // Positive phase: b >= 0, b - f = 0
    PiecewiseLinearCaseSplit positivePhase;
    positivePhase.storeBoundTightening( Tightening( _b, 0.0, Tightening::LB ) );

    if ( _auxVarsInUse )
    {
        positivePhase.storeBoundTightening( Tightening( _posAux, 0.0, Tightening::UB ) );
    }
    else
    {
        Equation positiveEquation( Equation::EQ );
        positiveEquation.addAddend( 1, _b );
        positiveEquation.addAddend( -1, _f );
        positiveEquation.setScalar( 0 );
        positivePhase.addEquation( positiveEquation );
    }

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

void AbsoluteValueConstraint::eliminateVariable( unsigned variable, double /* fixedValue */ )
{
    (void)variable;
    ASSERT( ( variable == _f ) || ( variable == _b ) ||
            ( _auxVarsInUse && ( variable == _posAux || variable == _negAux ) ) );

    // In an absolute value constraint, if a variable is removed the
    // entire constraint can be discarded
    _haveEliminatedVariables = true;
}

void AbsoluteValueConstraint::dump( String &output ) const
{
    output = Stringf( "AbsoluteValueCosntraint: x%u = Abs( x%u ). Active? %s. PhaseStatus = %u (%s).\n",
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

    if ( _auxVarsInUse )
    {
        output += Stringf( ". PosAux: %u. Range: [%s, %s]\n",
                           _posAux,
                           _lowerBounds.exists( _posAux ) ? Stringf( "%lf", _lowerBounds[_posAux] ).ascii() : "-inf",
                           _upperBounds.exists( _posAux ) ? Stringf( "%lf", _upperBounds[_posAux] ).ascii() : "inf" );

        output += Stringf( ". NegAux: %u. Range: [%s, %s]\n",
                           _negAux,
                           _lowerBounds.exists( _negAux ) ? Stringf( "%lf", _lowerBounds[_negAux] ).ascii() : "-inf",
                           _upperBounds.exists( _negAux ) ? Stringf( "%lf", _upperBounds[_negAux] ).ascii() : "inf" );
    }
}

void AbsoluteValueConstraint::updateVariableIndex( unsigned oldIndex, unsigned newIndex )
{
    ASSERT( oldIndex == _b || oldIndex == _f ||
            ( _auxVarsInUse && ( oldIndex == _posAux || oldIndex == _negAux ) ) );

    ASSERT( !_assignment.exists( newIndex ) &&
            !_lowerBounds.exists( newIndex ) &&
            !_upperBounds.exists( newIndex ) &&
            newIndex != _b && newIndex != _f && ( !_auxVarsInUse || ( newIndex != _posAux && newIndex != _negAux ) ) );

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
    else if ( oldIndex == _posAux )
        _posAux = newIndex;
    else
        _negAux = newIndex;
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

    // Aux vars should always be non-negative
    if ( _auxVarsInUse )
    {
        tightenings.append( Tightening( _posAux, 0.0, Tightening::LB ) );
        tightenings.append( Tightening( _negAux, 0.0, Tightening::LB ) );
    }

    if ( bLowerBound >= 0 )
    {
        // Positive phase, all bounds much match
        tightenings.append( Tightening( _f, bLowerBound, Tightening::LB ) );
        tightenings.append( Tightening( _b, fLowerBound, Tightening::LB ) );

        tightenings.append( Tightening( _f, bUpperBound, Tightening::UB ) );
        tightenings.append( Tightening( _b, fUpperBound, Tightening::UB ) );

        if ( _auxVarsInUse )
            tightenings.append( Tightening( _posAux, 0.0, Tightening::UB ) );
    }

    else if ( bUpperBound <= 0 )
    {
        // Negative phase, all bounds must match
        tightenings.append( Tightening( _f, -bUpperBound, Tightening::LB ) );
        tightenings.append( Tightening( _b, -fUpperBound, Tightening::LB ) );

        tightenings.append( Tightening( _f, -bLowerBound, Tightening::UB ) );
        tightenings.append( Tightening( _b, -fLowerBound, Tightening::UB ) );

        if ( _auxVarsInUse )
            tightenings.append( Tightening( _negAux, 0.0, Tightening::UB ) );
    }

    else if ( bLowerBound < 0 && bUpperBound >= 0 && FloatUtils::isZero( fLowerBound ) )
    {
        // Phase undetermined, b can be either positive or negative, f can be 0
        tightenings.append( Tightening( _b, -fUpperBound , Tightening::LB ) );
        tightenings.append( Tightening( _b, fUpperBound, Tightening::UB ) );
        tightenings.append( Tightening( _f, FloatUtils::max( -bLowerBound , bUpperBound ), Tightening::UB ) );

        if ( _auxVarsInUse )
        {
            tightenings.append( Tightening( _posAux,
                                            fUpperBound - bLowerBound,
                                            Tightening::UB ) );
            tightenings.append( Tightening( _negAux,
                                            fUpperBound + bUpperBound,
                                            Tightening::UB ) );
        }
    }

    else if ( bLowerBound < 0 && bUpperBound >= 0 && fLowerBound > 0 )
    {
        // Phase undetermined, b can be either positive or negative, f strictly positive
        tightenings.append( Tightening( _b, -fUpperBound, Tightening::LB ) );
        tightenings.append( Tightening( _b, fUpperBound, Tightening::UB ) );
        tightenings.append( Tightening( _f, FloatUtils::max( -bLowerBound, bUpperBound ), Tightening::UB ) );

        if ( _auxVarsInUse )
        {
            tightenings.append( Tightening( _posAux,
                                            fUpperBound - bLowerBound,
                                            Tightening::UB ) );
            tightenings.append( Tightening( _negAux,
                                            fUpperBound + bUpperBound,
                                            Tightening::UB ) );
        }

        // Below we test if the phase has actually become fixed
        if ( fLowerBound > -bLowerBound )
        {
            // Positive phase
            tightenings.append( Tightening( _b, fLowerBound, Tightening::LB ) );
            if ( _auxVarsInUse )
                tightenings.append( Tightening( _posAux, 0.0, Tightening::UB ) );
        }

        if ( fLowerBound > bUpperBound )
        {
            // Negative phase
            tightenings.append( Tightening( _b, -fLowerBound, Tightening::UB ) );
            if ( _auxVarsInUse )
                tightenings.append( Tightening( _negAux, 0.0, Tightening::UB ) );
        }
    }
}

void AbsoluteValueConstraint::addAuxiliaryEquations( InputQuery &inputQuery )
{
    /*
      We want to add the two equations

          f >= b
          f >= -b

      Which actually becomes

          f - b - posAux = 0
          f + b - negAux = 0

      posAux is non-negative, and 0 if in the positive phase
      its upper bound is (f.ub - b.lb)

      negAux is also non-negative, and 0 if in the negative phase
      its upper bound is (f.ub + b.ub)
    */

    _posAux = inputQuery.getNumberOfVariables();
    _negAux = _posAux + 1;
    inputQuery.setNumberOfVariables( _posAux + 2 );

    // Create and add the pos equation
    Equation posEquation( Equation::EQ );
    posEquation.addAddend( 1.0, _f );
    posEquation.addAddend( -1.0, _b );
    posEquation.addAddend( -1.0, _posAux );
    posEquation.setScalar( 0 );
    inputQuery.addEquation( posEquation );

    // Create and add the neg equation
    Equation negEquation( Equation::EQ );
    negEquation.addAddend( 1.0, _f );
    negEquation.addAddend( 1.0, _b );
    negEquation.addAddend( -1.0, _negAux );
    negEquation.setScalar( 0 );
    inputQuery.addEquation( negEquation );

    // Both aux variables are non-negative
    inputQuery.setLowerBound( _posAux, 0 );
    inputQuery.setLowerBound( _negAux, 0 );

    // Set their upper bounds
    inputQuery.setUpperBound( _posAux, _upperBounds[_f] - _lowerBounds[_b] );
    inputQuery.setUpperBound( _negAux, _upperBounds[_f] + _lowerBounds[_b] );

    // Mark that the aux vars are in use
    _auxVarsInUse = true;
}

String AbsoluteValueConstraint::serializeToString() const
{
    // Output format is: Abs,f,b,posAux,NegAux
    return _auxVarsInUse ?
        Stringf( "absoluteValue,%u,%u", _f, _b, _posAux, _negAux ) :
        Stringf( "absoluteValue,%u,%u", _f, _b );
}

void AbsoluteValueConstraint::fixPhaseIfNeeded()
{
    // Option 1: b's range is strictly positive
    if ( _lowerBounds.exists( _b ) && _lowerBounds[_b] >= 0 )
    {
        setPhaseStatus( PHASE_POSITIVE );
        return;
    }

    // Option 2: b's range is strictly negative:
    if ( _upperBounds.exists( _b ) && _upperBounds[_b] <= 0 )
    {
        setPhaseStatus( PHASE_NEGATIVE );
        return;
    }

    if ( !_lowerBounds.exists( _f ) )
        return;

    // Option 3: f's range is strictly disjoint from b's positive
    // range
    if ( _upperBounds.exists( _b ) && _lowerBounds[_f] > _upperBounds[_b] )
    {
        setPhaseStatus( PHASE_NEGATIVE );
        return;
    }

    // Option 4: f's range is strictly disjoint from b's negative
    // range, in absolute value
    if ( _lowerBounds.exists( _b ) && _lowerBounds[_f] > -_lowerBounds[_b] )
    {
        setPhaseStatus( PHASE_POSITIVE );
        return;
    }

    if ( _auxVarsInUse )
    {
        // Option 5: posAux has become zero, phase is positive
        if ( _upperBounds.exists( _posAux )
             && FloatUtils::isZero( _upperBounds[_posAux] ) )
        {
            setPhaseStatus( PHASE_POSITIVE );
            return;
        }

        // Option 6: posAux can never be zero, phase is negative
        if ( _lowerBounds.exists( _posAux )
             && FloatUtils::isPositive( _upperBounds[_posAux] ) )
        {
            setPhaseStatus( PHASE_NEGATIVE );
            return;
        }

        // Option 7: negAux has become zero, phase is negative
        if ( _upperBounds.exists( _negAux )
             && FloatUtils::isZero( _upperBounds[_negAux] ) )
        {
            setPhaseStatus( PHASE_NEGATIVE );
            return;
        }

        // Option 8: negAux can never be zero, phase is positive
        if ( _lowerBounds.exists( _negAux )
             && FloatUtils::isPositive( _lowerBounds[_negAux] ) )
        {
            setPhaseStatus( PHASE_POSITIVE );
            return;
        }
    }
}

String AbsoluteValueConstraint::phaseToString( PhaseStatus phase )
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

void AbsoluteValueConstraint::setPhaseStatus( PhaseStatus phaseStatus )
{
    _phaseStatus = phaseStatus;
}

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
