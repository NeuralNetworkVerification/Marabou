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
    : _disjuncts( disjuncts )
    , _feasibleDisjuncts( disjuncts )
{
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

PiecewiseLinearConstraint *DisjunctionConstraint::duplicateConstraint() const
{
    DisjunctionConstraint *clone = new DisjunctionConstraint( _disjuncts );
    *clone = *this;
    return clone;
}

void DisjunctionConstraint::restoreState( const PiecewiseLinearConstraint *state )
{
    const DisjunctionConstraint *disjunction = dynamic_cast<const DisjunctionConstraint *>( state );
    *this = *disjunction;
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
    return _disjuncts;
}

bool DisjunctionConstraint::phaseFixed() const
{
    return _feasibleDisjuncts.size() == 1;
}

PiecewiseLinearCaseSplit DisjunctionConstraint::getValidCaseSplit() const
{
    return *_feasibleDisjuncts.begin();
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

bool DisjunctionConstraint::supportsSymbolicBoundTightening() const
{
    return false;
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

    for ( const auto &disjunct : _disjuncts )
    {
        if ( disjunctIsFeasible( disjunct ) )
            _feasibleDisjuncts.append( disjunct );
    }
}

bool DisjunctionConstraint::disjunctIsFeasible( const PiecewiseLinearCaseSplit &disjunct ) const
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

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
