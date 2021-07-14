/*********************                                                        */
/*! \file SignConstraint.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Amir, Aleksandar Zeljic
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]
 **/

#include "SignConstraint.h"

#include "ConstraintBoundTightener.h"
#include "Debug.h"
#include "FloatUtils.h"
#include "ITableau.h"
#include "MStringf.h"
#include "MarabouError.h"
#include "PiecewiseLinearCaseSplit.h"
#include "SignConstraint.h"
#include "Statistics.h"

#ifdef _WIN32
#define __attribute__(x)
#endif

SignConstraint::SignConstraint( unsigned b, unsigned f )
    : ContextDependentPiecewiseLinearConstraint( TWO_PHASE_PIECEWISE_LINEAR_CONSTRAINT )
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

ContextDependentPiecewiseLinearConstraint *SignConstraint::duplicateConstraint() const
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
    if ( !( _assignment.exists( _b ) && _assignment.exists( _f ) ) )
        throw MarabouError( MarabouError::PARTICIPATING_VARIABLES_ABSENT );

    double bValue = _assignment.get( _b );
    double fValue = _assignment.get( _f );

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

    List <PiecewiseLinearCaseSplit> splits;

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
    if ( _assignment.exists( _f ) )
    {
        if ( FloatUtils::isPositive( _assignment[_f] ) )
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
    if ( _assignment.exists( _f ) )
    {
        if ( FloatUtils::isPositive( _assignment[_f] ) )
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

bool SignConstraint::haveOutOfBoundVariables() const
{
    double bValue = _assignment.get( _b );
    double fValue = _assignment.get( _f );

    if ( FloatUtils::gt( getLowerBound( _b ), bValue ) || FloatUtils::lt( getUpperBound( _b ), bValue ) )
        return true;

    if ( FloatUtils::gt( getLowerBound( _f ), fValue ) || FloatUtils::lt( getUpperBound( _f ), fValue ) )
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

void SignConstraint::notifyVariableValue( unsigned variable, double value )
{
    if ( FloatUtils::isZero( value ) )
        value = 0.0;

    _assignment[variable] = value;
}

void SignConstraint::notifyLowerBound( unsigned variable, double bound )
{
    if ( _statistics )
        _statistics->incNumBoundNotificationsPlConstraints();

    // If there's an already-stored tighter bound, return
    if ( existsLowerBound( variable ) && !FloatUtils::gt( bound, getLowerBound( variable ) ) )
        return;

    // Otherwise - update bound
    setLowerBound( variable, bound );

    if ( variable == _f && FloatUtils::gt( bound, -1 ) )
    {
        setPhaseStatus( PhaseStatus::SIGN_PHASE_POSITIVE );
        if ( _constraintBoundTightener )
        {
            _constraintBoundTightener->registerTighterLowerBound( _f, 1 );
            _constraintBoundTightener->registerTighterLowerBound( _b, 0 );
        }
    }
    else if ( variable == _b && !FloatUtils::isNegative( bound ) )
    {
        setPhaseStatus( PhaseStatus::SIGN_PHASE_POSITIVE );
        if ( _constraintBoundTightener )
        {
            _constraintBoundTightener->registerTighterLowerBound( _f, 1 );
        }
    }
}

void SignConstraint::notifyUpperBound( unsigned variable, double bound )
{
    if ( _statistics )
        _statistics->incNumBoundNotificationsPlConstraints();

    // If there's an already-stored tighter bound, return
    if ( existsUpperBound( variable ) && !FloatUtils::lt( bound, getUpperBound( variable ) ) )
        return;

    // Otherwise - update bound
    setUpperBound( variable, bound );

    if ( variable == _f && FloatUtils::lt( bound, 1 ) )
    {
        setPhaseStatus( PhaseStatus::SIGN_PHASE_NEGATIVE );
        if ( _constraintBoundTightener )
        {
            _constraintBoundTightener->registerTighterUpperBound( _f, -1 );
            _constraintBoundTightener->registerTighterUpperBound( _b, 0 );
        }
    }
    else if ( variable == _b && FloatUtils::isNegative( bound ) )
    {
        setPhaseStatus( PhaseStatus::SIGN_PHASE_NEGATIVE );
        if ( _constraintBoundTightener )
        {
            _constraintBoundTightener->registerTighterUpperBound( _f, -1 );
        }
    }
}

List<PiecewiseLinearConstraint::Fix> SignConstraint::getPossibleFixes() const
{
    ASSERT( !satisfied() );
    ASSERT( _assignment.exists( _b ) );
    ASSERT( _assignment.exists( _f ) );

    double bValue = _assignment.get( _b );
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
    ASSERT( existsLowerBound( _b ) && existsLowerBound( _f ) &&
            existsUpperBound( _b ) && existsUpperBound( _f ) );

    double bLowerBound = getLowerBound( _b );
    double fLowerBound = getLowerBound( _f );

    double bUpperBound = getUpperBound( _b );
    double fUpperBound = getUpperBound( _f );

    // Always make f between -1 and 1
    tightenings.append( Tightening( _f, -1, Tightening::LB ) );
    tightenings.append( Tightening( _f, 1, Tightening::UB ) );

    // Additional bounds can only be propagated if we are in the POSITIVE or NEGATIVE phases
    if ( !FloatUtils::isNegative( bLowerBound ) ||
         FloatUtils::gt( fLowerBound, -1 ) )
    {
        // Positive case
        tightenings.append( Tightening( _b, 0, Tightening::LB ) );
        tightenings.append( Tightening( _f, 1, Tightening::LB ) );
    }
    else if ( FloatUtils::isNegative( bUpperBound ) ||
              FloatUtils::lt( fUpperBound, 1 ) )
    {
        // Negative case
        tightenings.append( Tightening( _b, 0, Tightening::UB ) );
        tightenings.append( Tightening( _f, -1, Tightening::UB ) );
    }
}

void SignConstraint::updateVariableIndex( unsigned oldIndex, unsigned newIndex )
{
    ASSERT( oldIndex == _b || oldIndex == _f  );
    ASSERT( !_boundManager );
    ASSERT( !_assignment.exists( newIndex ) &&
            !_lowerBounds.exists( newIndex ) &&
            !_upperBounds.exists( newIndex ) &&
            newIndex != _b && newIndex != _f );

    if ( _assignment.exists( oldIndex ) )
    {
        _assignment[newIndex] = _assignment.get( oldIndex );
        _assignment.erase( oldIndex );
    }

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

void SignConstraint::eliminateVariable( __attribute__((unused)) unsigned variable,
                                        __attribute__((unused)) double fixedValue )
{
    ASSERT( variable == _b || variable == _f );

    DEBUG({
              if ( variable == _f )
              {
                  ASSERT( ( FloatUtils::areEqual( fixedValue, 1 ) ) ||
                          ( FloatUtils::areEqual( fixedValue,-1 ) ) );

                  if ( FloatUtils::areEqual( fixedValue, 1 ) )
                  {
                      ASSERT( _phaseStatus != SIGN_PHASE_NEGATIVE );
                  }
                  else if (FloatUtils::areEqual( fixedValue, -1 ) )
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
        });

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
                      _f, _b,
                      _constraintActive ? "Yes" : "No",
                      _phaseStatus, phaseToString( _phaseStatus ).ascii()
                      );

    output += Stringf( "b in [%s, %s], ",
                       existsLowerBound( _b ) ? Stringf( "%lf", getLowerBound( _b ) ).ascii() : "-inf",
                       existsUpperBound( _b ) ? Stringf( "%lf", getUpperBound( _b ) ).ascii() : "inf" );

    output += Stringf( "f in [%s, %s]\n",
                       existsLowerBound( _f ) ? Stringf( "%lf", getLowerBound( _f ) ).ascii() : "-inf",
                       existsUpperBound( _f ) ? Stringf( "%lf", getUpperBound( _f ) ).ascii() : "inf" );
}

double SignConstraint::computePolarity() const
{
  double currentLb = getLowerBound( _b );
  double currentUb = getUpperBound( _b );
  if ( !FloatUtils::isNegative( currentLb ) ) return 1;
  if ( FloatUtils::isNegative( currentUb ) ) return -1;
  double width = currentUb - currentLb;
  double sum = currentUb + currentLb;
  return sum / width;
}

void SignConstraint::updateDirection()
{
    _direction = ( FloatUtils::isNegative( computePolarity() ) ) ?
        SIGN_PHASE_NEGATIVE : SIGN_PHASE_POSITIVE;
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
