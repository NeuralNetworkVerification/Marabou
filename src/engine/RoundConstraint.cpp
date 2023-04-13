/*********************                                                        */
/*! \file RoundConstraint.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz, Parth Shah, Derek Huang
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** See the description of the class in RoundConstraint.h.
 **/

#include "RoundConstraint.h"

#include "PiecewiseLinearConstraint.h"
#include "Debug.h"
#include "DivideStrategy.h"
#include "FloatUtils.h"
#include "GlobalConfiguration.h"
#include "InfeasibleQueryException.h"
#include "InputQuery.h"
#include "ITableau.h"
#include "MStringf.h"
#include "MarabouError.h"
#include "PiecewiseLinearCaseSplit.h"
#include "Statistics.h"
#include "TableauRow.h"

#ifdef _WIN32
#define __attribute__(x)
#endif

RoundConstraint::RoundConstraint( unsigned b, unsigned f )
    : PiecewiseLinearConstraint( TWO_PHASE_PIECEWISE_LINEAR_CONSTRAINT )
    , _b( b )
    , _f( f )
    , _direction( PHASE_NOT_FIXED )
    , _haveEliminatedVariables( false )
{
}

RoundConstraint::RoundConstraint( const String &serializedRound )
    : _haveEliminatedVariables( false )
{
    String constraintType = serializedRound.substring( 0, 5 );
    ASSERT( constraintType == String( "round" ) );

    // Remove the constraint type in serialized form
    String serializedValues = serializedRound.substring( 6, serializedRound.length() - 6 );
    List<String> values = serializedValues.tokenize( "," );

    ASSERT( values.size() == 2 );

    auto var = values.begin();
    _f = atoi( var->ascii() );
    ++var;
    _b = atoi( var->ascii() );
}

PiecewiseLinearFunctionType RoundConstraint::getType() const
{
    return PiecewiseLinearFunctionType::ROUND;
}

PiecewiseLinearConstraint *RoundConstraint::duplicateConstraint() const
{
    RoundConstraint *clone = new RoundConstraint( _b, _f );
    *clone = *this;
    this->initializeDuplicateCDOs( clone );
    return clone;
}

void RoundConstraint::restoreState( const PiecewiseLinearConstraint *state )
{
    const RoundConstraint *round = dynamic_cast<const RoundConstraint *>( state );

    CVC4::context::CDO<bool> *activeStatus = _cdConstraintActive;
    CVC4::context::CDO<PhaseStatus> *phaseStatus = _cdPhaseStatus;
    CVC4::context::CDList<PhaseStatus> *infeasibleCases = _cdInfeasibleCases;
    *this = *round;
    _cdConstraintActive = activeStatus;
    _cdPhaseStatus = phaseStatus;
    _cdInfeasibleCases = infeasibleCases;
}

void RoundConstraint::registerAsWatcher( ITableau *tableau )
{
    tableau->registerToWatchVariable( this, _b );
    tableau->registerToWatchVariable( this, _f );
}

void RoundConstraint::unregisterAsWatcher( ITableau *tableau )
{
    tableau->unregisterToWatchVariable( this, _b );
    tableau->unregisterToWatchVariable( this, _f );
}

void RoundConstraint::checkIfLowerBoundUpdateFixesPhase( unsigned /*variable*/, double /*bound*/ )
{
}

void RoundConstraint::checkIfUpperBoundUpdateFixesPhase( unsigned /*variable*/, double /*bound*/ )
{
}

void RoundConstraint::notifyLowerBound( unsigned /*variable*/, double /*newBound*/ )
{
}

void RoundConstraint::notifyUpperBound( unsigned /*variable*/, double /*newBound*/ )
{
}

bool RoundConstraint::participatingVariable( unsigned variable ) const
{
    return ( variable == _b ) || ( variable == _f );
}

List<unsigned> RoundConstraint::getParticipatingVariables() const
{
    return List<unsigned>( { _b, _f } );
}

bool RoundConstraint::satisfied() const
{
    return false;
}

List<PiecewiseLinearConstraint::Fix> RoundConstraint::getPossibleFixes() const
{
    throw MarabouError(MarabouError::FEATURE_NOT_YET_SUPPORTED);
}

List<PiecewiseLinearConstraint::Fix> RoundConstraint::getSmartFixes( ITableau */*tableau*/ ) const
{
    throw MarabouError(MarabouError::FEATURE_NOT_YET_SUPPORTED);
}

List<PiecewiseLinearCaseSplit> RoundConstraint::getCaseSplits() const
{
    throw MarabouError(MarabouError::FEATURE_NOT_YET_SUPPORTED);
}

List<PhaseStatus> RoundConstraint::getAllCases() const
{
    throw MarabouError(MarabouError::FEATURE_NOT_YET_SUPPORTED);
}

PiecewiseLinearCaseSplit RoundConstraint::getCaseSplit( PhaseStatus /*phase*/ ) const
{
    throw MarabouError(MarabouError::FEATURE_NOT_YET_SUPPORTED);
}

PiecewiseLinearCaseSplit RoundConstraint::getInactiveSplit() const
{
    throw MarabouError(MarabouError::FEATURE_NOT_YET_SUPPORTED);
}

PiecewiseLinearCaseSplit RoundConstraint::getActiveSplit() const
{
    throw MarabouError(MarabouError::FEATURE_NOT_YET_SUPPORTED);
}

bool RoundConstraint::phaseFixed() const
{
    return _phaseStatus != PHASE_NOT_FIXED;
}

PiecewiseLinearCaseSplit RoundConstraint::getImpliedCaseSplit() const
{
    throw MarabouError(MarabouError::FEATURE_NOT_YET_SUPPORTED);
}

PiecewiseLinearCaseSplit RoundConstraint::getValidCaseSplit() const
{
    throw MarabouError(MarabouError::FEATURE_NOT_YET_SUPPORTED);
}

void RoundConstraint::dump( String &output ) const
{
    output = Stringf( "RoundConstraint: x%u = Round( x%u ). Active? %s. \n",
                      _f, _b,
                      _constraintActive ? "Yes" : "No"
                      );
}

void RoundConstraint::updateVariableIndex( unsigned oldIndex, unsigned newIndex )
{
    // Variable reindexing can only occur in preprocessing before Gurobi is
    // registered.
    ASSERT( _gurobi == NULL );

    ASSERT( oldIndex == _b || oldIndex == _f );

    if ( oldIndex == _b )
        _b = newIndex;
    else if ( oldIndex == _f )
        _f = newIndex;
}

void RoundConstraint::eliminateVariable( __attribute__((unused)) unsigned variable,
                                        __attribute__((unused)) double fixedValue )
{
    throw MarabouError(MarabouError::FEATURE_NOT_YET_SUPPORTED);
}

bool RoundConstraint::constraintObsolete() const
{
    return false;
}

void RoundConstraint::getEntailedTightenings( List<Tightening> &/*tightenings*/ ) const
{
}

void RoundConstraint::transformToUseAuxVariables( InputQuery &inputQuery )
{
    /*
      We want to add the equation

          f - b >= -0.5
          f - b <= 0.5

      Which actually becomes

          f - b - aux1 = -0.5
          f - b + aux2 = 0.5

      where aux1, aux2 >= 0
    */

    // Create the aux variable
    unsigned aux = inputQuery.getNumberOfVariables();
    inputQuery.setNumberOfVariables( aux + 1 );

    // Create and add the equation
    Equation equation( Equation::EQ );
    equation.addAddend( 1.0, _f );
    equation.addAddend( -1.0, _b );
    equation.addAddend( -1.0, aux );
    equation.setScalar( -0.5 );
    inputQuery.addEquation( equation );

    // Adjust the bounds for the new variable
    inputQuery.setLowerBound( aux, 0 );

    aux = inputQuery.getNumberOfVariables();
    inputQuery.setNumberOfVariables( aux + 1 );
    Equation equation2( Equation::EQ );
    equation2.addAddend( 1.0, _f );
    equation2.addAddend( -1.0, _b );
    equation2.addAddend( 1.0, aux );
    equation2.setScalar( 0.5 );
    inputQuery.addEquation( equation2 );

    // Adjust the bounds for the new variable
    inputQuery.setLowerBound( aux, 0 );
}

void RoundConstraint::getCostFunctionComponent( LinearExpression &/*cost*/,
                                                PhaseStatus /*phase*/ ) const
{
    throw MarabouError(MarabouError::FEATURE_NOT_YET_SUPPORTED);
}

PhaseStatus RoundConstraint::getPhaseStatusInAssignment( const Map<unsigned, double>
                                                         &/*assignment*/ ) const
{
    throw MarabouError(MarabouError::FEATURE_NOT_YET_SUPPORTED);
}

bool RoundConstraint::haveOutOfBoundVariables() const
{
    throw MarabouError(MarabouError::FEATURE_NOT_YET_SUPPORTED);
}

String RoundConstraint::serializeToString() const
{
    // Output format is: round,f,b
    return Stringf( "round,%u,%u", _f, _b );
}

unsigned RoundConstraint::getB() const
{
    return _b;
}

unsigned RoundConstraint::getF() const
{
    return _f;
}

bool RoundConstraint::supportPolarity() const
{
    return false;
}

double RoundConstraint::computePolarity() const
{
    throw MarabouError(MarabouError::FEATURE_NOT_YET_SUPPORTED);
}

void RoundConstraint::updateDirection()
{
    throw MarabouError(MarabouError::FEATURE_NOT_YET_SUPPORTED);
}

PhaseStatus RoundConstraint::getDirection() const
{
    throw MarabouError(MarabouError::FEATURE_NOT_YET_SUPPORTED);
}

void RoundConstraint::updateScoreBasedOnPolarity()
{
    throw MarabouError(MarabouError::FEATURE_NOT_YET_SUPPORTED);
}

void RoundConstraint::createTighteningRow()
{
    throw MarabouError(MarabouError::FEATURE_NOT_YET_SUPPORTED);
}
