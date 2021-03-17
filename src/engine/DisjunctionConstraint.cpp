/*********************                                                        */
/*! \file DisjunctionConstraint.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]
 **/

#include "Debug.h"
#include "DisjunctionConstraint.h"
#include "MStringf.h"
#include "MarabouError.h"
#include "Statistics.h"

DisjunctionConstraint::DisjunctionConstraint( const List<PiecewiseLinearCaseSplit> &disjuncts )
    : ContextDependentPiecewiseLinearConstraint( disjuncts.size() )
    , _disjuncts( disjuncts.begin(), disjuncts.end() )
    , _feasibleDisjuncts( disjuncts.size(), 0 )
{
    for ( unsigned ind = 0;  ind < disjuncts.size();  ++ind )
        _feasibleDisjuncts.append( ind );

    extractParticipatingVariables();
}

DisjunctionConstraint::DisjunctionConstraint( const Vector<PiecewiseLinearCaseSplit> &disjuncts )
    : ContextDependentPiecewiseLinearConstraint( disjuncts.size() )
    , _disjuncts( disjuncts )
    , _feasibleDisjuncts( disjuncts.size(), 0 )
{
    for ( unsigned ind = 0;  ind < disjuncts.size();  ++ind )
        _feasibleDisjuncts.append( ind );

    extractParticipatingVariables();
}

DisjunctionConstraint::DisjunctionConstraint( const String &/* serializedDisjunction */ )
{
    throw MarabouError( MarabouError::FEATURE_NOT_YET_SUPPORTED,
                        "Construct DisjunctionConstraint from String" );
}

PiecewiseLinearFunctionType DisjunctionConstraint::getType() const
{
    return PiecewiseLinearFunctionType::DISJUNCTION;
}

ContextDependentPiecewiseLinearConstraint *DisjunctionConstraint::duplicateConstraint() const
{
    DisjunctionConstraint *clone = new DisjunctionConstraint( _disjuncts );
    *clone = *this;
    initializeDuplicatesCDOs( clone );
    return clone;
}

void DisjunctionConstraint::restoreState( const PiecewiseLinearConstraint *state )
{
    const DisjunctionConstraint *disjunction = dynamic_cast<const DisjunctionConstraint *>( state );

    CVC4::context::CDO<bool> *activeStatus = _cdConstraintActive;
    CVC4::context::CDO<PhaseStatus> *phaseStatus = _cdPhaseStatus;
    CVC4::context::CDList<PhaseStatus> *infeasibleCases = _cdInfeasibleCases;
    *this = *disjunction;
    _cdConstraintActive = activeStatus;
    _cdPhaseStatus = phaseStatus;
    _cdInfeasibleCases = infeasibleCases;
}

void DisjunctionConstraint::registerAsWatcher( ITableau *tableau )
{
    for ( const auto &variable : _participatingVariables )
        tableau->registerToWatchVariable( this, variable );
}

void DisjunctionConstraint::unregisterAsWatcher( ITableau *tableau )
{
    for ( const auto &variable : _participatingVariables )
        tableau->unregisterToWatchVariable( this, variable );
}

void DisjunctionConstraint::notifyVariableValue( unsigned variable, double value )
{
    _assignment[variable] = value;
}

void DisjunctionConstraint::notifyLowerBound( unsigned variable, double bound )
{
    if ( _statistics )
        _statistics->incNumBoundNotificationsPlConstraints();

    if ( _lowerBounds.exists( variable ) && !FloatUtils::gt( bound, _lowerBounds[variable] ) )
        return;

    _lowerBounds[variable] = bound;

    updateFeasibleDisjuncts();
}

void DisjunctionConstraint::notifyUpperBound( unsigned variable, double bound )
{
    if ( _statistics )
        _statistics->incNumBoundNotificationsPlConstraints();

    if ( _upperBounds.exists( variable ) && !FloatUtils::lt( bound, _upperBounds[variable] ) )
        return;

    _upperBounds[variable] = bound;

    updateFeasibleDisjuncts();
}

bool DisjunctionConstraint::participatingVariable( unsigned variable ) const
{
    return _participatingVariables.exists( variable );
}

List<unsigned> DisjunctionConstraint::getParticipatingVariables() const
{
    List<unsigned> variables;
    for ( const auto &var : _participatingVariables )
        variables.append( var );

    return variables;
}

bool DisjunctionConstraint::satisfied() const
{
    for ( const auto &disjunct : _disjuncts )
        if ( disjunctSatisfied( disjunct ) )
            return true;

    return false;
}

List<PiecewiseLinearConstraint::Fix> DisjunctionConstraint::getPossibleFixes() const
{
    return List<PiecewiseLinearConstraint::Fix>();
}

List<PiecewiseLinearConstraint::Fix> DisjunctionConstraint::getSmartFixes( ITableau */* tableau */ ) const
{
    return getPossibleFixes();
}

List<PiecewiseLinearCaseSplit> DisjunctionConstraint::getCaseSplits() const
{
    return List<PiecewiseLinearCaseSplit>( _disjuncts.begin(), _disjuncts.end() );
}

List<PhaseStatus> DisjunctionConstraint::getAllCases() const
{
    List<PhaseStatus> cases;
    for ( unsigned i = 0; i < _disjuncts.size(); ++i  )
        cases.append( indToPhaseStatus( i ) );
    return cases;
}

PiecewiseLinearCaseSplit DisjunctionConstraint::getCaseSplit( PhaseStatus phase ) const
{
    return _disjuncts.get( phaseStatusToInd( phase ) );
}

bool DisjunctionConstraint::phaseFixed() const
{
    return _feasibleDisjuncts.size() == 1;
}

PiecewiseLinearCaseSplit DisjunctionConstraint::getImpliedCaseSplit() const
{
    return _disjuncts.get( *_feasibleDisjuncts.begin() );
}

PiecewiseLinearCaseSplit DisjunctionConstraint::getValidCaseSplit() const
{
    return getImpliedCaseSplit();
}

void DisjunctionConstraint::dump( String &output ) const
{
    output = Stringf( "DisjunctionConstraint:\n" );

    for ( const auto &disjunct : _disjuncts )
    {
        String disjunctOutput;
        disjunct.dump( disjunctOutput );
        output += Stringf( "\t%s\n", disjunctOutput.ascii() );
    }

    output += Stringf( "Active? %s.", _constraintActive ? "Yes" : "No" );
}

void DisjunctionConstraint::updateVariableIndex( unsigned oldIndex, unsigned newIndex )
{
    ASSERT( !participatingVariable( newIndex ) );

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

    for ( auto &disjunct : _disjuncts )
        disjunct.updateVariableIndex( oldIndex, newIndex );

    extractParticipatingVariables();
}

void DisjunctionConstraint::eliminateVariable( unsigned /* variable */, double /* fixedValue */ )
{
    throw MarabouError( MarabouError::FEATURE_NOT_YET_SUPPORTED,
                        "Eliminate variable from a DisjunctionConstraint" );
}

bool DisjunctionConstraint::constraintObsolete() const
{
    return _feasibleDisjuncts.empty();
}

void DisjunctionConstraint::getEntailedTightenings( List<Tightening> &/* tightenings */ ) const
{
}

void DisjunctionConstraint::addAuxiliaryEquations( InputQuery &/* inputQuery */ )
{
}

void DisjunctionConstraint::getCostFunctionComponent( Map<unsigned, double> &/* cost */ ) const
{
}

String DisjunctionConstraint::serializeToString() const
{
    throw MarabouError( MarabouError::FEATURE_NOT_YET_SUPPORTED,
                        "Serialize DisjunctionConstraint to String" );
}

void DisjunctionConstraint::extractParticipatingVariables()
{
    _participatingVariables.clear();

    for ( const auto &disjunct : _disjuncts )
    {
        // Extract from bounds
        for ( const auto &bound : disjunct.getBoundTightenings() )
            _participatingVariables.insert( bound._variable );

        // Extract from equations
        for ( const auto &equation : disjunct.getEquations() )
        {
            for ( const auto &addend : equation._addends )
                _participatingVariables.insert( addend._variable );
        }
    }
}

bool DisjunctionConstraint::disjunctSatisfied( const PiecewiseLinearCaseSplit &disjunct ) const
{
    // Check whether the bounds are satisfied
    for ( const auto &bound : disjunct.getBoundTightenings() )
    {
        if ( bound._type == Tightening::LB )
        {
            if ( _assignment[bound._variable] < bound._value )
                return false;
        }
        else
        {
            if ( _assignment[bound._variable] > bound._value )
                return false;
        }
    }

    // Check whether the equations are satisfied
    for ( const auto &equation : disjunct.getEquations() )
    {
        double result = 0;
        for ( const auto &addend : equation._addends )
            result += addend._coefficient * _assignment[addend._variable];

        if ( !FloatUtils::areEqual( result, equation._scalar ) )
            return false;
    }

    return true;
}

void DisjunctionConstraint::updateFeasibleDisjuncts()
{
    _feasibleDisjuncts.clear();

    for ( unsigned ind = 0; ind < _disjuncts.size(); ++ind )
    {
        if ( disjunctIsFeasible( ind ) )
            _feasibleDisjuncts.append( ind );
        else if ( _cdInfeasibleCases && !isCaseInfeasible( indToPhaseStatus( ind ) ) )
            markInfeasible( indToPhaseStatus( ind ) );
    }
}

bool DisjunctionConstraint::disjunctIsFeasible( unsigned ind ) const
{
    if ( _cdInfeasibleCases && isCaseInfeasible( indToPhaseStatus( ind ) ) )
        return false;

    return caseSplitIsFeasible( _disjuncts.get( ind ) );
}

bool DisjunctionConstraint::caseSplitIsFeasible( const PiecewiseLinearCaseSplit &disjunct ) const
{
    for ( const auto &bound : disjunct.getBoundTightenings() )
    {
        if ( bound._type == Tightening::LB )
        {
            if ( _upperBounds.exists( bound._variable ) &&
                 _upperBounds[bound._variable] < bound._value )
                return false;
        }
        else
        {
            if ( _lowerBounds.exists( bound._variable ) &&
                 _lowerBounds[bound._variable] > bound._value )
                return false;
        }
    }

    return true;
}

