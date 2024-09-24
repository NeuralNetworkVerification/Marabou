/*********************                                                        */
/*! \file SignConstraint.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Amir, Haoze Wu, Aleksandar Zeljic
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2024 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** See the description of the class in SignConstraint.h.
 **/

#include "SignConstraint.h"

#include "Debug.h"
#include "FloatUtils.h"
#include "GlobalConfiguration.h"
#include "ITableau.h"
#include "InfeasibleQueryException.h"
#include "MStringf.h"
#include "MarabouError.h"
#include "PiecewiseLinearCaseSplit.h"
#include "Query.h"
#include "SignConstraint.h"
#include "Statistics.h"

#ifdef _WIN32
#define __attribute__( x )
#endif

SignConstraint::SignConstraint( unsigned b, unsigned f )
    : PiecewiseLinearConstraint( TWO_PHASE_PIECEWISE_LINEAR_CONSTRAINT )
    , _b( b )
    , _f( f )
    , _direction( PHASE_NOT_FIXED )
    , _haveEliminatedVariables( false )
{
}

SignConstraint::SignConstraint( const String &serializedSign )
    : _haveEliminatedVariables( false )
{
    String constraintType = serializedSign.substring( 0, 4 );
    ASSERT( constraintType == String( "sign" ) );

    // Remove the constraint type in serialized form
    String serializedValues = serializedSign.substring( 5, serializedSign.length() - 5 );
    List<String> values = serializedValues.tokenize( "," );

    ASSERT( values.size() == 2 );

    auto var = values.begin();
    _f = atoi( var->ascii() );
    ++var;
    _b = atoi( var->ascii() );
}

PiecewiseLinearFunctionType SignConstraint::getType() const
{
    return PiecewiseLinearFunctionType::SIGN;
}

PiecewiseLinearConstraint *SignConstraint::duplicateConstraint() const
{
    SignConstraint *clone = new SignConstraint( _b, _f );
    *clone = *this;
    this->initializeDuplicateCDOs( clone );
    return clone;
}

void SignConstraint::restoreState( const PiecewiseLinearConstraint *state )
{
    const SignConstraint *sign = dynamic_cast<const SignConstraint *>( state );

    CVC4::context::CDO<bool> *activeStatus = _cdConstraintActive;
    CVC4::context::CDO<PhaseStatus> *phaseStatus = _cdPhaseStatus;
    CVC4::context::CDList<PhaseStatus> *infeasibleCases = _cdInfeasibleCases;
    *this = *sign;
    _cdConstraintActive = activeStatus;
    _cdPhaseStatus = phaseStatus;
    _cdInfeasibleCases = infeasibleCases;
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
    if ( !( existsAssignment( _b ) && existsAssignment( _f ) ) )
        throw MarabouError( MarabouError::PARTICIPATING_VARIABLE_MISSING_ASSIGNMENT );

    double bValue = getAssignment( _b );
    double fValue = getAssignment( _f );

    // if bValue is negative, f should be -1
    if ( FloatUtils::isNegative( bValue ) )
        return FloatUtils::areEqual( fValue, -1 );

    // bValue is non-negative, f should be 1
    return FloatUtils::areEqual( fValue, 1 );
}

List<PiecewiseLinearCaseSplit> SignConstraint::getCaseSplits() const
{
    if ( _phaseStatus != PHASE_NOT_FIXED )
        throw MarabouError( MarabouError::REQUESTED_CASE_SPLITS_FROM_FIXED_CONSTRAINT );

    List<PiecewiseLinearCaseSplit> splits;

    if ( _direction == SIGN_PHASE_NEGATIVE )
    {
        splits.append( getNegativeSplit() );
        splits.append( getPositiveSplit() );
        return splits;
    }
    if ( _direction == SIGN_PHASE_POSITIVE )
    {
        splits.append( getPositiveSplit() );
        splits.append( getNegativeSplit() );
        return splits;
    }

    // If we have existing knowledge about the assignment, use it to
    // influence the order of splits
    if ( existsAssignment( _f ) )
    {
        if ( FloatUtils::isPositive( getAssignment( _f ) ) )
        {
            splits.append( getPositiveSplit() );
            splits.append( getNegativeSplit() );
        }
        else
        {
            splits.append( getNegativeSplit() );
            splits.append( getPositiveSplit() );
        }
    }
    else
    {
        // Default
        splits.append( getNegativeSplit() );
        splits.append( getPositiveSplit() );
    }

    return splits;
}

List<PhaseStatus> SignConstraint::getAllCases() const
{
    if ( _phaseStatus != PHASE_NOT_FIXED )
        throw MarabouError( MarabouError::REQUESTED_CASE_SPLITS_FROM_FIXED_CONSTRAINT );

    if ( _direction == SIGN_PHASE_NEGATIVE )
        return { SIGN_PHASE_NEGATIVE, SIGN_PHASE_POSITIVE };

    if ( _direction == SIGN_PHASE_POSITIVE )
        return { SIGN_PHASE_POSITIVE, SIGN_PHASE_NEGATIVE };

    // If we have existing knowledge about the assignment, use it to
    // influence the order of splits
    if ( existsAssignment( _f ) )
    {
        if ( FloatUtils::isPositive( getAssignment( _f ) ) )
            return { SIGN_PHASE_POSITIVE, SIGN_PHASE_NEGATIVE };
        else
            return { SIGN_PHASE_NEGATIVE, SIGN_PHASE_POSITIVE };
    }

    return { SIGN_PHASE_NEGATIVE, SIGN_PHASE_POSITIVE };
}

PiecewiseLinearCaseSplit SignConstraint::getCaseSplit( PhaseStatus phase ) const
{
    if ( phase == SIGN_PHASE_NEGATIVE )
        return getNegativeSplit();
    else if ( phase == SIGN_PHASE_POSITIVE )
        return getPositiveSplit();
    else
        throw MarabouError( MarabouError::REQUESTED_NONEXISTENT_CASE_SPLIT );
}

PiecewiseLinearCaseSplit SignConstraint::getNegativeSplit() const
{
    // Negative phase: b < 0, f = -1
    PiecewiseLinearCaseSplit negativePhase;
    negativePhase.storeBoundTightening( Tightening( _b, 0.0, Tightening::UB ) );
    negativePhase.storeBoundTightening( Tightening( _f, -1.0, Tightening::UB ) );
    return negativePhase;
}

PiecewiseLinearCaseSplit SignConstraint::getPositiveSplit() const
{
    // Positive phase: b >= 0, f = 1
    PiecewiseLinearCaseSplit positivePhase;
    positivePhase.storeBoundTightening( Tightening( _b, 0.0, Tightening::LB ) );
    positivePhase.storeBoundTightening( Tightening( _f, 1.0, Tightening::LB ) );
    return positivePhase;
}

bool SignConstraint::phaseFixed() const
{
    return _phaseStatus != PHASE_NOT_FIXED;
}

void SignConstraint::addAuxiliaryEquationsAfterPreprocessing( Query &inputQuery )
{
    /*
      If the phase is not fixed, add _f <= -2/lb_b * _b + 1
      and  _f >= 2/ub_b * _b - 1
      which becomes,
      _f + 2/lb_b * _b + aux_ub = 1, 0 <= aux_ub <= 1 - lb_f - 2 * ub_b/lb_b
      _f - 2/ub_b * _b + aux2_lb = -1, -1 - ub_f + 2 * lb_b/ub_b  <= aux_lb <= 0
    */

    if ( isActive() && !phaseFixed() )
    {
        double lowerBound = inputQuery.getLowerBound( _b );
        double upperBound = inputQuery.getUpperBound( _b );

        ASSERT( FloatUtils::lt( lowerBound, 0 ) && FloatUtils::gte( upperBound, 0 ) );

        // Create the aux variable
        unsigned auxUpper = inputQuery.getNumberOfVariables();
        inputQuery.setNumberOfVariables( auxUpper + 1 );

        // Create and add the equation
        Equation equation( Equation::EQ );
        equation.addAddend( 1.0, _f );
        equation.addAddend( 2 / lowerBound, _b );
        equation.addAddend( 1.0, auxUpper );
        equation.setScalar( 1 );
        inputQuery.addEquation( equation );

        inputQuery.setLowerBound( auxUpper, 0 );
        inputQuery.setUpperBound( auxUpper, 2 - 2 * upperBound / lowerBound );

        if ( FloatUtils::isPositive( upperBound ) )
        {
            // Create the aux variable
            unsigned auxLower = inputQuery.getNumberOfVariables();
            inputQuery.setNumberOfVariables( auxLower + 1 );
            // Create and add the equation
            Equation equation2( Equation::EQ );
            equation2.addAddend( 1.0, _f );
            equation2.addAddend( -2 / upperBound, _b );
            equation2.addAddend( 1.0, auxLower );
            equation2.setScalar( -1 );
            inputQuery.addEquation( equation2 );

            inputQuery.setLowerBound( auxLower, -2 + 2 * lowerBound / upperBound );
            inputQuery.setUpperBound( auxLower, 0 );
        }
    }
}

PiecewiseLinearCaseSplit SignConstraint::getImpliedCaseSplit() const
{
    ASSERT( _phaseStatus != PHASE_NOT_FIXED );

    if ( _phaseStatus == PhaseStatus::SIGN_PHASE_POSITIVE )
        return getPositiveSplit();

    return getNegativeSplit();
}

PiecewiseLinearCaseSplit SignConstraint::getValidCaseSplit() const
{
    return getImpliedCaseSplit();
}

bool SignConstraint::constraintObsolete() const
{
    return _haveEliminatedVariables;
}

String SignConstraint::serializeToString() const
{
    // Output format is: sign,f,b
    return Stringf( "sign,%u,%u", _f, _b );
}

void SignConstraint::getCostFunctionComponent( LinearExpression &cost, PhaseStatus phase ) const
{
    // If the constraint is not active or is fixed, it contributes nothing
    if ( !isActive() || phaseFixed() )
        return;

    ASSERT( phase == SIGN_PHASE_NEGATIVE || phase == SIGN_PHASE_POSITIVE );

    // The SoI cost term is sound iff the aux equations are added.
    ASSERT( GlobalConfiguration::PL_CONSTRAINTS_ADD_AUX_EQUATIONS_AFTER_PREPROCESSING );

    if ( phase == SIGN_PHASE_NEGATIVE )
    {
        // The cost term corresponding to the negative phase is 1 + f.
        // The SignConstraint is satisfied iff 1 + f is minimal and 0.
        if ( !cost._addends.exists( _f ) )
            cost._addends[_f] = 0;
        cost._addends[_f] += 1;
        cost._constant += 1;
    }
    else
    {
        // The cost term corresponding to the negative phase is 1 - f.
        // The Sign constraint is satisfied, iff 1 - f must be minimal
        // and 0.
        if ( !cost._addends.exists( _f ) )
            cost._addends[_f] = 0;
        cost._addends[_f] -= 1;
        cost._constant += 1;
    }
}

PhaseStatus
SignConstraint::getPhaseStatusInAssignment( const Map<unsigned, double> &assignment ) const
{
    ASSERT( assignment.exists( _b ) );
    return FloatUtils::isNegative( assignment[_b] ) ? SIGN_PHASE_NEGATIVE : SIGN_PHASE_POSITIVE;
}

bool SignConstraint::haveOutOfBoundVariables() const
{
    double bValue = getAssignment( _b );
    double fValue = getAssignment( _f );

    if ( FloatUtils::gt(
             getLowerBound( _b ), bValue, GlobalConfiguration::CONSTRAINT_COMPARISON_TOLERANCE ) ||
         FloatUtils::lt(
             getUpperBound( _b ), bValue, GlobalConfiguration::CONSTRAINT_COMPARISON_TOLERANCE ) )
        return true;

    if ( FloatUtils::gt(
             getLowerBound( _f ), fValue, GlobalConfiguration::CONSTRAINT_COMPARISON_TOLERANCE ) ||
         FloatUtils::lt(
             getUpperBound( _f ), fValue, GlobalConfiguration::CONSTRAINT_COMPARISON_TOLERANCE ) )
        return true;

    return false;
}

String SignConstraint::phaseToString( PhaseStatus phase )
{
    switch ( phase )
    {
    case PHASE_NOT_FIXED:
        return "PHASE_NOT_FIXED";

    case SIGN_PHASE_POSITIVE:
        return "SIGN_PHASE_POSITIVE";

    case SIGN_PHASE_NEGATIVE:
        return "SIGN_PHASE_NEGATIVE";

    default:
        return "UNKNOWN";
    }
};

void SignConstraint::notifyLowerBound( unsigned variable, double bound )
{
    if ( _statistics )
        _statistics->incLongAttribute( Statistics::NUM_BOUND_NOTIFICATIONS_TO_PL_CONSTRAINTS );

    // If there's an already-stored tighter bound, return
    if ( _boundManager == nullptr && existsLowerBound( variable ) &&
         !FloatUtils::gt( bound, getLowerBound( variable ) ) )
        return;

    // Otherwise - update bound
    setLowerBound( variable, bound );

    if ( variable == _f && FloatUtils::gt( bound, -1 ) )
    {
        setPhaseStatus( PhaseStatus::SIGN_PHASE_POSITIVE );

        if ( _boundManager != nullptr )
        {
            if ( _boundManager->shouldProduceProofs() )
            {
                // If lb of f is > 1, we have a contradiction
                if ( FloatUtils::gt( bound, 1 ) )
                    throw InfeasibleQueryException();

                _boundManager->addLemmaExplanationAndTightenBound(
                    _f, 1, Tightening::LB, { variable }, Tightening::LB, getType() );
                _boundManager->addLemmaExplanationAndTightenBound(
                    _b, 0, Tightening::LB, { variable }, Tightening::LB, getType() );
            }
            else
            {
                _boundManager->tightenLowerBound( _f, 1 );
                _boundManager->tightenLowerBound( _b, 0 );
            }
        }
    }
    else if ( variable == _b && !FloatUtils::isNegative( bound ) )
    {
        setPhaseStatus( PhaseStatus::SIGN_PHASE_POSITIVE );
        if ( _boundManager != nullptr )
        {
            if ( _boundManager->shouldProduceProofs() )
                _boundManager->addLemmaExplanationAndTightenBound(
                    _f, 1, Tightening::LB, { variable }, Tightening::LB, getType() );
            else
                _boundManager->tightenLowerBound( _f, 1 );
        }
    }
}

void SignConstraint::notifyUpperBound( unsigned variable, double bound )
{
    if ( _statistics )
        _statistics->incLongAttribute( Statistics::NUM_BOUND_NOTIFICATIONS_TO_PL_CONSTRAINTS );

    // If there's an already-stored tighter bound, return
    if ( _boundManager == nullptr && existsUpperBound( variable ) &&
         !FloatUtils::lt( bound, getUpperBound( variable ) ) )
        return;

    // Otherwise - update bound
    setUpperBound( variable, bound );

    if ( variable == _f && FloatUtils::lt( bound, 1 ) )
    {
        setPhaseStatus( PhaseStatus::SIGN_PHASE_NEGATIVE );
        if ( _boundManager != nullptr )
        {
            if ( _boundManager->shouldProduceProofs() )
            {
                // If ub of f is < -1, we have a contradiction
                if ( FloatUtils::lt( bound, -1 ) )
                    throw InfeasibleQueryException();

                _boundManager->addLemmaExplanationAndTightenBound(
                    _f, -1, Tightening::UB, { variable }, Tightening::UB, getType() );
                _boundManager->addLemmaExplanationAndTightenBound(
                    _b, 0, Tightening::UB, { variable }, Tightening::UB, getType() );
            }
            else
            {
                _boundManager->tightenUpperBound( _f, -1 );
                _boundManager->tightenUpperBound( _b, 0 );
            }
        }
    }
    else if ( variable == _b && FloatUtils::isNegative( bound ) )
    {
        setPhaseStatus( PhaseStatus::SIGN_PHASE_NEGATIVE );
        if ( _boundManager != nullptr )
        {
            if ( _boundManager->shouldProduceProofs() )
                _boundManager->addLemmaExplanationAndTightenBound(
                    _f, -1, Tightening::UB, { variable }, Tightening::UB, getType() );
            else
                _boundManager->tightenUpperBound( _f, -1 );
        }
    }
}

List<PiecewiseLinearConstraint::Fix> SignConstraint::getPossibleFixes() const
{
    // This should never be called when we are using Gurobi to solve LPs.
    ASSERT( _gurobi == NULL );

    ASSERT( !satisfied() );
    ASSERT( existsAssignment( _b ) );
    ASSERT( existsAssignment( _f ) );

    double bValue = getAssignment( _b );
    double newFValue = FloatUtils::isNegative( bValue ) ? -1 : 1;

    List<PiecewiseLinearConstraint::Fix> fixes;
    fixes.append( PiecewiseLinearConstraint::Fix( _f, newFValue ) );
    return fixes;
}

List<PiecewiseLinearConstraint::Fix> SignConstraint::getSmartFixes( ITableau * ) const
{
    return getPossibleFixes();
}

void SignConstraint::getEntailedTightenings( List<Tightening> &tightenings ) const
{
    ASSERT( existsLowerBound( _b ) && existsLowerBound( _f ) && existsUpperBound( _b ) &&
            existsUpperBound( _f ) );

    double bLowerBound = getLowerBound( _b );
    double fLowerBound = getLowerBound( _f );

    double bUpperBound = getUpperBound( _b );
    double fUpperBound = getUpperBound( _f );

    // Always make f between -1 and 1
    tightenings.append( Tightening( _f, -1, Tightening::LB ) );
    tightenings.append( Tightening( _f, 1, Tightening::UB ) );

    // Additional bounds can only be propagated if we are in the POSITIVE or NEGATIVE phases
    if ( !FloatUtils::isNegative( bLowerBound ) || FloatUtils::gt( fLowerBound, -1 ) )
    {
        // Positive case
        tightenings.append( Tightening( _b, 0, Tightening::LB ) );
        tightenings.append( Tightening( _f, 1, Tightening::LB ) );
    }
    else if ( FloatUtils::isNegative( bUpperBound ) || FloatUtils::lt( fUpperBound, 1 ) )
    {
        // Negative case
        tightenings.append( Tightening( _b, 0, Tightening::UB ) );
        tightenings.append( Tightening( _f, -1, Tightening::UB ) );
    }
}

void SignConstraint::updateVariableIndex( unsigned oldIndex, unsigned newIndex )
{
    // Variable reindexing can only occur in preprocessing before Gurobi is
    // registered.
    ASSERT( _gurobi == NULL );

    ASSERT( oldIndex == _b || oldIndex == _f );
    ASSERT( !_boundManager );
    ASSERT( !_lowerBounds.exists( newIndex ) && !_upperBounds.exists( newIndex ) &&
            newIndex != _b && newIndex != _f );

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
}

void SignConstraint::eliminateVariable( __attribute__( ( unused ) ) unsigned variable,
                                        __attribute__( ( unused ) ) double fixedValue )
{
    ASSERT( variable == _b || variable == _f );

    DEBUG( {
        if ( variable == _f )
        {
            ASSERT( ( FloatUtils::areEqual( fixedValue, 1 ) ) ||
                    ( FloatUtils::areEqual( fixedValue, -1 ) ) );

            if ( FloatUtils::areEqual( fixedValue, 1 ) )
            {
                ASSERT( _phaseStatus != SIGN_PHASE_NEGATIVE );
            }
            else if ( FloatUtils::areEqual( fixedValue, -1 ) )
            {
                ASSERT( _phaseStatus != SIGN_PHASE_POSITIVE );
            }
        }
        else if ( variable == _b )
        {
            if ( FloatUtils::gte( fixedValue, 0 ) )
            {
                ASSERT( _phaseStatus != SIGN_PHASE_NEGATIVE );
            }
            else if ( FloatUtils::lt( fixedValue, 0 ) )
            {
                ASSERT( _phaseStatus != SIGN_PHASE_POSITIVE );
            }
        }
    } );

    // In a Sign constraint, if a variable is removed the entire constraint can be discarded.
    _haveEliminatedVariables = true;
}

unsigned SignConstraint::getB() const
{
    return _b;
}

unsigned SignConstraint::getF() const
{
    return _f;
}

void SignConstraint::dump( String &output ) const
{
    output = Stringf( "SignConstraint: x%u = Sign( x%u ). Active? %s. PhaseStatus = %u (%s). ",
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
        Stringf( "f in [%s, %s]\n",
                 existsLowerBound( _f ) ? Stringf( "%lf", getLowerBound( _f ) ).ascii() : "-inf",
                 existsUpperBound( _f ) ? Stringf( "%lf", getUpperBound( _f ) ).ascii() : "inf" );
}

double SignConstraint::computePolarity() const
{
    double currentLb = getLowerBound( _b );
    double currentUb = getUpperBound( _b );
    if ( !FloatUtils::isNegative( currentLb ) )
        return 1;
    if ( FloatUtils::isNegative( currentUb ) )
        return -1;
    double width = currentUb - currentLb;
    double sum = currentUb + currentLb;
    return sum / width;
}

void SignConstraint::updateDirection()
{
    _direction =
        ( FloatUtils::isNegative( computePolarity() ) ) ? SIGN_PHASE_NEGATIVE : SIGN_PHASE_POSITIVE;
}

PhaseStatus SignConstraint::getDirection() const
{
    return _direction;
}

void SignConstraint::updateScoreBasedOnPolarity()
{
    _score = std::abs( computePolarity() );
}

bool SignConstraint::supportPolarity() const
{
    return true;
}

// No aux vars in Sign constraint, so the function is suppressed
void SignConstraint::addTableauAuxVar( unsigned /* tableauAuxVar */,
                                       unsigned /* constraintAuxVar */ )
{
}
