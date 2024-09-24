/*********************                                                        */
/*! \file AbsoluteValueConstraint.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Shiran Aziz, Guy Katz, Haoze Wu, Aleksandar Zeljic
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2024 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** See the description of the class in AbsoluteValueConstraint.h.
 **/

#include "AbsoluteValueConstraint.h"

#include "Debug.h"
#include "FloatUtils.h"
#include "ITableau.h"
#include "MStringf.h"
#include "MarabouError.h"
#include "PiecewiseLinearCaseSplit.h"
#include "Query.h"
#include "Statistics.h"

AbsoluteValueConstraint::AbsoluteValueConstraint( unsigned b, unsigned f )
    : PiecewiseLinearConstraint( TWO_PHASE_PIECEWISE_LINEAR_CONSTRAINT )
    , _b( b )
    , _f( f )
    , _auxVarsInUse( false )
    , _haveEliminatedVariables( false )
{
}

AbsoluteValueConstraint::AbsoluteValueConstraint( const String &serializedAbs )
    : _auxVarsInUse( false )
    , _haveEliminatedVariables( false )
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
}

PiecewiseLinearFunctionType AbsoluteValueConstraint::getType() const
{
    return PiecewiseLinearFunctionType::ABSOLUTE_VALUE;
}

PiecewiseLinearConstraint *AbsoluteValueConstraint::duplicateConstraint() const
{
    AbsoluteValueConstraint *clone = new AbsoluteValueConstraint( _b, _f );
    *clone = *this;
    this->initializeDuplicateCDOs( clone );
    return clone;
}

void AbsoluteValueConstraint::restoreState( const PiecewiseLinearConstraint *state )
{
    const AbsoluteValueConstraint *abs = dynamic_cast<const AbsoluteValueConstraint *>( state );

    CVC4::context::CDO<bool> *activeStatus = _cdConstraintActive;
    CVC4::context::CDO<PhaseStatus> *phaseStatus = _cdPhaseStatus;
    CVC4::context::CDList<PhaseStatus> *infeasibleCases = _cdInfeasibleCases;
    *this = *abs;
    _cdConstraintActive = activeStatus;
    _cdPhaseStatus = phaseStatus;
    _cdInfeasibleCases = infeasibleCases;
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

void AbsoluteValueConstraint::notifyLowerBound( unsigned variable, double bound )
{
    if ( _statistics )
        _statistics->incLongAttribute( Statistics::NUM_BOUND_NOTIFICATIONS_TO_PL_CONSTRAINTS );

    if ( _boundManager == nullptr && existsLowerBound( variable ) &&
         !FloatUtils::gt( bound, getLowerBound( variable ) ) )
        return;

    setLowerBound( variable, bound );

    // Check whether the phase has become fixed
    fixPhaseIfNeeded();

    // Update partner's bound
    if ( isActive() && _boundManager )
    {
        bool proofs = _boundManager->shouldProduceProofs();

        if ( proofs )
            createPosTighteningRow();

        if ( variable == _b )
        {
            if ( bound < 0 )
            {
                double fUpperBound = FloatUtils::max( -bound, getUpperBound( _b ) );
                // If phase is not fixed, both bounds are stored and checker should check the max of
                // the two
                if ( proofs && !phaseFixed() )
                    _boundManager->addLemmaExplanationAndTightenBound( _f,
                                                                       fUpperBound,
                                                                       Tightening::UB,
                                                                       { variable, variable },
                                                                       Tightening::UB,
                                                                       getType() );
                else if ( proofs && phaseFixed() )
                {
                    std::shared_ptr<TableauRow> tighteningRow =
                        fUpperBound == getUpperBound( _b ) ? _posTighteningRow : _negTighteningRow;
                    _boundManager->tightenUpperBound( _f, fUpperBound, *tighteningRow );
                }
                else
                    _boundManager->tightenUpperBound( _f, fUpperBound );

                if ( _auxVarsInUse )
                {
                    if ( proofs )
                        _boundManager->tightenUpperBound(
                            _posAux, fUpperBound - bound, *_posTighteningRow );
                    else
                        _boundManager->tightenUpperBound( _posAux, fUpperBound - bound );
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
            {
                if ( !proofs )
                    _boundManager->tightenLowerBound( _f, 0 );
            }
        }

        // Any lower bound tightening on the aux variables, if they
        // are used, must have already fixed the phase, and needs not
        // be considered
    }
}

void AbsoluteValueConstraint::notifyUpperBound( unsigned variable, double bound )
{
    if ( _statistics )
        _statistics->incLongAttribute( Statistics::NUM_BOUND_NOTIFICATIONS_TO_PL_CONSTRAINTS );

    if ( _boundManager == nullptr && existsUpperBound( variable ) &&
         !FloatUtils::lt( bound, getUpperBound( variable ) ) )
        return;

    setUpperBound( variable, bound );
    // Check whether the phase has become fixed
    fixPhaseIfNeeded();

    // Update partner's bound
    if ( isActive() && _boundManager )
    {
        bool proofs = _boundManager->shouldProduceProofs();

        if ( proofs )
        {
            createPosTighteningRow();
            createNegTighteningRow();
        }

        if ( variable == _b )
        {
            if ( bound > 0 )
            {
                double fUpperBound = FloatUtils::max( bound, -getLowerBound( _b ) );
                // If phase is not fixed, both bonds are stored and checker should check the max of
                // the two
                if ( proofs && !phaseFixed() )
                    _boundManager->addLemmaExplanationAndTightenBound( _f,
                                                                       fUpperBound,
                                                                       Tightening::UB,
                                                                       { variable, variable },
                                                                       Tightening::UB,
                                                                       getType() );
                else if ( proofs && phaseFixed() )
                {
                    std::shared_ptr<TableauRow> tighteningRow =
                        fUpperBound == bound ? _posTighteningRow : _negTighteningRow;
                    _boundManager->tightenUpperBound( _f, fUpperBound, *tighteningRow );
                }
                else
                    _boundManager->tightenUpperBound( _f, fUpperBound );

                if ( _auxVarsInUse )
                {
                    if ( proofs )
                        _boundManager->tightenUpperBound(
                            _negAux, fUpperBound + bound, *_negTighteningRow );
                    else
                        _boundManager->tightenUpperBound( _negAux, fUpperBound + bound );
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
            if ( FloatUtils::lt( bound, getUpperBound( _b ) ) )
            {
                if ( proofs )
                    _boundManager->tightenUpperBound( _b, bound, *_posTighteningRow );
                else
                    _boundManager->tightenUpperBound( _b, bound );
            }

            if ( FloatUtils::gt( -bound, getLowerBound( _b ) ) )
            {
                if ( proofs )
                    _boundManager->tightenLowerBound( _b, -bound, *_negTighteningRow );
                else
                    _boundManager->tightenLowerBound( _b, -bound );
            }

            if ( _auxVarsInUse )
            {
                if ( existsLowerBound( _b ) )
                {
                    if ( proofs )
                        _boundManager->tightenUpperBound(
                            _posAux, bound - getLowerBound( _b ), *_posTighteningRow );
                    else
                        _boundManager->tightenUpperBound( _posAux, bound - getLowerBound( _b ) );
                }

                if ( existsUpperBound( _b ) )
                {
                    if ( proofs )
                        _boundManager->tightenUpperBound(
                            _negAux, bound + getUpperBound( _b ), *_negTighteningRow );
                    else
                        _boundManager->tightenUpperBound( _negAux, bound + getUpperBound( _b ) );
                }
            }
        }
        else if ( _auxVarsInUse )
        {
            if ( variable == _posAux )
            {
                if ( existsUpperBound( _b ) )
                {
                    if ( proofs )
                        _boundManager->tightenUpperBound(
                            _f, getUpperBound( _b ) + bound, *_posTighteningRow );
                    else
                        _boundManager->tightenUpperBound( _f, getUpperBound( _b ) + bound );
                }

                if ( existsLowerBound( _f ) )
                {
                    if ( proofs )
                        _boundManager->tightenLowerBound(
                            _b, getLowerBound( _f ) - bound, *_posTighteningRow );
                    else
                        _boundManager->tightenLowerBound( _b, getLowerBound( _f ) - bound );
                }
            }
            else if ( variable == _negAux )
            {
                if ( existsLowerBound( _b ) )
                {
                    if ( proofs )
                        _boundManager->tightenUpperBound(
                            _f, bound - getLowerBound( _b ), *_negTighteningRow );
                    else
                        _boundManager->tightenUpperBound( _f, bound - getLowerBound( _b ) );
                }

                if ( existsLowerBound( _f ) )
                {
                    if ( proofs )
                        _boundManager->tightenUpperBound(
                            _b, bound - getLowerBound( _f ), *_negTighteningRow );
                    else
                        _boundManager->tightenUpperBound( _b, bound - getLowerBound( _f ) );
                }
            }
        }
    }
}

bool AbsoluteValueConstraint::participatingVariable( unsigned variable ) const
{
    return ( variable == _b ) || ( variable == _f ) ||
           ( _auxVarsInUse && ( variable == _posAux || variable == _negAux ) );
}

List<unsigned> AbsoluteValueConstraint::getParticipatingVariables() const
{
    return _auxVarsInUse ? List<unsigned>( { _b, _f, _posAux, _negAux } )
                         : List<unsigned>( { _b, _f } );
}

bool AbsoluteValueConstraint::satisfied() const
{
    if ( !( existsAssignment( _b ) && existsAssignment( _f ) ) )
        throw MarabouError( MarabouError::PARTICIPATING_VARIABLE_MISSING_ASSIGNMENT );

    double bValue = getAssignment( _b );
    double fValue = getAssignment( _f );

    // Possible violations:
    //   1. f is negative
    //   2. f is positive, abs(b) and f are not equal

    if ( fValue < 0 )
        return false;

    return FloatUtils::areEqual(
        FloatUtils::abs( bValue ), fValue, GlobalConfiguration::CONSTRAINT_COMPARISON_TOLERANCE );
}

List<PiecewiseLinearConstraint::Fix> AbsoluteValueConstraint::getPossibleFixes() const
{
    // Reluplex does not currently work with Gurobi.
    ASSERT( _gurobi == NULL );

    ASSERT( !satisfied() );

    double bValue = getAssignment( _b );
    double fValue = getAssignment( _f );

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

List<PiecewiseLinearConstraint::Fix>
AbsoluteValueConstraint::getSmartFixes( ITableau * /* tableau */ ) const
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

List<PhaseStatus> AbsoluteValueConstraint::getAllCases() const
{
    return { ABS_PHASE_NEGATIVE, ABS_PHASE_POSITIVE };
}

PiecewiseLinearCaseSplit AbsoluteValueConstraint::getCaseSplit( PhaseStatus phase ) const
{
    if ( phase == ABS_PHASE_NEGATIVE )
        return getNegativeSplit();
    else if ( phase == ABS_PHASE_POSITIVE )
        return getPositiveSplit();
    else
        throw MarabouError( MarabouError::REQUESTED_NONEXISTENT_CASE_SPLIT );
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

PiecewiseLinearCaseSplit AbsoluteValueConstraint::getImpliedCaseSplit() const
{
    ASSERT( _phaseStatus != PHASE_NOT_FIXED );

    if ( _phaseStatus == ABS_PHASE_POSITIVE )
        return getPositiveSplit();

    return getNegativeSplit();
}

PiecewiseLinearCaseSplit AbsoluteValueConstraint::getValidCaseSplit() const
{
    return getImpliedCaseSplit();
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
    output =
        Stringf( "AbsoluteValueCosntraint: x%u = Abs( x%u ). Active? %s. PhaseStatus = %u (%s).\n",
                 _f,
                 _b,
                 _constraintActive ? "Yes" : "No",
                 _phaseStatus,
                 phaseToString( _phaseStatus ).ascii() );

    output +=
        Stringf( "b in [%s, %s], ",
                 existsLowerBound( _b ) ? Stringf( "%lf", getLowerBound( _b ) ).ascii() : "-inf",
                 existsUpperBound( _b ) ? Stringf( "%lf", getUpperBound( _b ) ).ascii() : "inf" );

    output +=
        Stringf( "f in [%s, %s]",
                 existsLowerBound( _f ) ? Stringf( "%lf", getLowerBound( _f ) ).ascii() : "-inf",
                 existsUpperBound( _f ) ? Stringf( "%lf", getUpperBound( _f ) ).ascii() : "inf" );

    if ( _auxVarsInUse )
    {
        output += Stringf(
            ". PosAux: %u. Range: [%s, %s]",
            _posAux,
            existsLowerBound( _posAux ) ? Stringf( "%lf", getLowerBound( _posAux ) ).ascii()
                                        : "-inf",
            existsUpperBound( _posAux ) ? Stringf( "%lf", getUpperBound( _posAux ) ).ascii()
                                        : "inf" );

        output += Stringf(
            ". NegAux: %u. Range: [%s, %s]",
            _negAux,
            existsLowerBound( _negAux ) ? Stringf( "%lf", getLowerBound( _negAux ) ).ascii()
                                        : "-inf",
            existsUpperBound( _negAux ) ? Stringf( "%lf", getUpperBound( _negAux ) ).ascii()
                                        : "inf" );
    }
}

void AbsoluteValueConstraint::updateVariableIndex( unsigned oldIndex, unsigned newIndex )
{
    // Variable reindexing can only occur in preprocessing before Gurobi is
    // registered.
    ASSERT( _gurobi == NULL );

    ASSERT( oldIndex == _b || oldIndex == _f ||
            ( _auxVarsInUse && ( oldIndex == _posAux || oldIndex == _negAux ) ) );

    ASSERT( !existsLowerBound( newIndex ) && !existsUpperBound( newIndex ) && newIndex != _b &&
            newIndex != _f &&
            ( !_auxVarsInUse || ( newIndex != _posAux && newIndex != _negAux ) ) );

    if ( existsLowerBound( oldIndex ) )
    {
        _lowerBounds[newIndex] = _lowerBounds.get( oldIndex );
        _lowerBounds.erase( oldIndex );
    }

    if ( existsUpperBound( oldIndex ) )
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
    ASSERT( existsLowerBound( _b ) && existsLowerBound( _f ) && existsUpperBound( _b ) &&
            existsUpperBound( _f ) );

    // Upper bounds
    double bUpperBound = getUpperBound( _b );
    double fUpperBound = getUpperBound( _f );
    // Lower bounds
    double bLowerBound = getLowerBound( _b );
    double fLowerBound = getLowerBound( _f );

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
        tightenings.append( Tightening( _b, -fUpperBound, Tightening::LB ) );
        tightenings.append( Tightening( _b, fUpperBound, Tightening::UB ) );
        tightenings.append(
            Tightening( _f, FloatUtils::max( -bLowerBound, bUpperBound ), Tightening::UB ) );

        if ( _auxVarsInUse )
        {
            tightenings.append( Tightening( _posAux, fUpperBound - bLowerBound, Tightening::UB ) );
            tightenings.append( Tightening( _negAux, fUpperBound + bUpperBound, Tightening::UB ) );
        }
    }

    else if ( bLowerBound < 0 && bUpperBound >= 0 && fLowerBound > 0 )
    {
        // Phase undetermined, b can be either positive or negative, f strictly positive
        tightenings.append( Tightening( _b, -fUpperBound, Tightening::LB ) );
        tightenings.append( Tightening( _b, fUpperBound, Tightening::UB ) );
        tightenings.append(
            Tightening( _f, FloatUtils::max( -bLowerBound, bUpperBound ), Tightening::UB ) );

        if ( _auxVarsInUse )
        {
            tightenings.append( Tightening( _posAux, fUpperBound - bLowerBound, Tightening::UB ) );
            tightenings.append( Tightening( _negAux, fUpperBound + bUpperBound, Tightening::UB ) );
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

void AbsoluteValueConstraint::transformToUseAuxVariables( Query &inputQuery )
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
    if ( _auxVarsInUse )
        return;

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

    setLowerBound( _posAux, 0 );
    setLowerBound( _negAux, 0 );
    setUpperBound( _posAux, FloatUtils::infinity() );
    setUpperBound( _negAux, FloatUtils::infinity() );

    // Mark that the aux vars are in use
    _auxVarsInUse = true;
}

void AbsoluteValueConstraint::getCostFunctionComponent( LinearExpression &cost,
                                                        PhaseStatus phase ) const
{
    // If the constraint is not active or is fixed, it contributes nothing
    if ( !isActive() || phaseFixed() )
        return;

    ASSERT( phase == ABS_PHASE_NEGATIVE || phase == ABS_PHASE_POSITIVE );

    if ( phase == ABS_PHASE_NEGATIVE )
    {
        // The cost term corresponding to the negative phase is f + b,
        // since the Abs is negative and satisfied iff f + b is 0
        // and minimal. This is true when we added the constraint
        // that f >= b and f >= -b.
        if ( !cost._addends.exists( _f ) )
            cost._addends[_f] = 0;
        if ( !cost._addends.exists( _b ) )
            cost._addends[_b] = 0;
        cost._addends[_f] += 1;
        cost._addends[_b] += 1;
    }
    else
    {
        // The cost term corresponding to the positive phase is f - b,
        // since the Abs is non-negative and satisfied iff f - b is 0 and
        // minimal. This is true when we added the constraint
        // that f >= b and f >= -b.
        if ( !cost._addends.exists( _f ) )
            cost._addends[_f] = 0;
        if ( !cost._addends.exists( _b ) )
            cost._addends[_b] = 0;
        cost._addends[_f] += 1;
        cost._addends[_b] -= 1;
    }
}

PhaseStatus
AbsoluteValueConstraint::getPhaseStatusInAssignment( const Map<unsigned, double> &assignment ) const
{
    ASSERT( assignment.exists( _b ) );
    return FloatUtils::isNegative( assignment[_b] ) ? ABS_PHASE_NEGATIVE : ABS_PHASE_POSITIVE;
}

bool AbsoluteValueConstraint::haveOutOfBoundVariables() const
{
    double bValue = getAssignment( _b );
    double fValue = getAssignment( _f );

    if ( FloatUtils::gt( getLowerBound( _b ), bValue ) ||
         FloatUtils::lt( getUpperBound( _b ), bValue ) )
        return true;

    if ( FloatUtils::gt( getLowerBound( _f ), fValue ) ||
         FloatUtils::lt( getUpperBound( _f ), fValue ) )
        return true;

    return false;
}

String AbsoluteValueConstraint::serializeToString() const
{
    // Output format is: Abs,f,b,posAux,NegAux
    return _auxVarsInUse ? Stringf( "absoluteValue,%u,%u,%u,%u", _f, _b, _posAux, _negAux )
                         : Stringf( "absoluteValue,%u,%u", _f, _b );
}

void AbsoluteValueConstraint::fixPhaseIfNeeded()
{
    if ( phaseFixed() )
        return;

    bool proofs = _boundManager && _boundManager->shouldProduceProofs();

    if ( proofs )
    {
        createPosTighteningRow();
        createNegTighteningRow();
    }

    // Option 1: b's range is strictly positive
    if ( existsLowerBound( _b ) && getLowerBound( _b ) >= 0 )
    {
        setPhaseStatus( ABS_PHASE_POSITIVE );
        if ( proofs )
            _boundManager->addLemmaExplanationAndTightenBound(
                _posAux, 0, Tightening::UB, { _b }, Tightening::LB, getType() );
        return;
    }

    // Option 2: b's range is strictly negative:
    if ( existsUpperBound( _b ) && getUpperBound( _b ) <= 0 )
    {
        setPhaseStatus( ABS_PHASE_NEGATIVE );
        if ( proofs )
            _boundManager->addLemmaExplanationAndTightenBound(
                _negAux, 0, Tightening::UB, { _b }, Tightening::UB, getType() );
        return;
    }

    if ( !existsLowerBound( _f ) )
        return;

    // Option 3: f's range is strictly disjoint from b's positive
    // range
    if ( existsUpperBound( _b ) && getLowerBound( _f ) > getUpperBound( _b ) )
    {
        setPhaseStatus( ABS_PHASE_NEGATIVE );
        if ( proofs )
            _boundManager->addLemmaExplanationAndTightenBound(
                _negAux, 0, Tightening::UB, { _b, _f }, Tightening::UB, getType() );
        return;
    }

    // Option 4: f's range is strictly disjoint from b's negative
    // range, in absolute value
    if ( existsLowerBound( _b ) && getLowerBound( _f ) > -getLowerBound( _b ) )
    {
        setPhaseStatus( ABS_PHASE_POSITIVE );
        if ( proofs )
            _boundManager->addLemmaExplanationAndTightenBound(
                _posAux, 0, Tightening::UB, { _b, _f }, Tightening::LB, getType() );
        return;
    }

    if ( _auxVarsInUse )
    {
        // Option 5: posAux has become zero, phase is positive
        if ( existsUpperBound( _posAux ) && FloatUtils::isZero( getUpperBound( _posAux ) ) )
        {
            setPhaseStatus( ABS_PHASE_POSITIVE );
            return;
        }

        // Option 6: posAux can never be zero, phase is negative
        if ( existsLowerBound( _posAux ) && FloatUtils::isPositive( getLowerBound( _posAux ) ) )
        {
            setPhaseStatus( ABS_PHASE_NEGATIVE );
            if ( proofs )
                _boundManager->addLemmaExplanationAndTightenBound(
                    _negAux, 0, Tightening::UB, { _posAux }, Tightening::LB, getType() );
            return;
        }

        // Option 7: negAux has become zero, phase is negative
        if ( existsUpperBound( _negAux ) && FloatUtils::isZero( getUpperBound( _negAux ) ) )
        {
            setPhaseStatus( ABS_PHASE_NEGATIVE );
            return;
        }

        // Option 8: negAux can never be zero, phase is positive
        if ( existsLowerBound( _negAux ) && FloatUtils::isPositive( getLowerBound( _negAux ) ) )
        {
            setPhaseStatus( ABS_PHASE_POSITIVE );
            if ( proofs )
                _boundManager->addLemmaExplanationAndTightenBound(
                    _posAux, 0, Tightening::UB, { _negAux }, Tightening::LB, getType() );
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

    case ABS_PHASE_POSITIVE:
        return "ABS_PHASE_POSITIVE";

    case ABS_PHASE_NEGATIVE:
        return "ABS_PHASE_NEGATIVE";

    default:
        return "UNKNOWN";
    }
};

void AbsoluteValueConstraint::createPosTighteningRow()
{
    // Create the row only when needed and when not already created
    if ( !_boundManager->getBoundExplainer() || _posTighteningRow || !_auxVarsInUse ||
         _tableauAuxVars.empty() )
        return;
    _posTighteningRow = std::unique_ptr<TableauRow>( new TableauRow( 3 ) );

    // f = b + aux + counterpart (an additional aux variable of tableau)
    _posTighteningRow->_lhs = _f;
    _posTighteningRow->_row[0] = TableauRow::Entry( _b, 1 );
    _posTighteningRow->_row[1] = TableauRow::Entry( _posAux, 1 );
    _posTighteningRow->_row[2] = TableauRow::Entry( _tableauAuxVars.front(), 1 );
    _posTighteningRow->_scalar = 0;
}

void AbsoluteValueConstraint::createNegTighteningRow()
{
    // Create the row only when needed and when not already created
    if ( !_boundManager->getBoundExplainer() || _negTighteningRow || !_auxVarsInUse ||
         _tableauAuxVars.empty() )
        return;

    _negTighteningRow = std::unique_ptr<TableauRow>( new TableauRow( 3 ) );

    // f = b + aux + counterpart (an additional aux variable of tableau)
    _negTighteningRow->_lhs = _f;
    _negTighteningRow->_row[0] = TableauRow::Entry( _b, -1 );
    _negTighteningRow->_row[1] = TableauRow::Entry( _negAux, 1 );
    _negTighteningRow->_row[2] = TableauRow::Entry( _tableauAuxVars.back(), 1 );
    _negTighteningRow->_scalar = 0;
}

const List<unsigned> AbsoluteValueConstraint::getNativeAuxVars() const
{
    if ( _auxVarsInUse )
        return { _posAux, _negAux };
    return {};
}

void AbsoluteValueConstraint::addTableauAuxVar( unsigned tableauAuxVar, unsigned constraintAuxVar )
{
    ASSERT( constraintAuxVar == _negAux || constraintAuxVar == _posAux );
    if ( _tableauAuxVars.size() == 2 )
        return;

    if ( constraintAuxVar == _negAux )
    {
        _tableauAuxVars.append( tableauAuxVar );
        ASSERT( _tableauAuxVars.back() == tableauAuxVar );
    }
    else
    {
        _tableauAuxVars.appendHead( tableauAuxVar );
        ASSERT( _tableauAuxVars.front() == tableauAuxVar );
    }
}
