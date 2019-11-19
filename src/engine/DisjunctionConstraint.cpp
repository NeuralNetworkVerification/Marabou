/*********************                                                        */
/*! \file DisjunctionConstraint.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz, Parth Shah, Derek Huang
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]
 **/

#include "DisjunctionConstraint.h"
#include "Statistics.h"

DisjunctionConstraint::DisjunctionConstraint( const List<PiecewiseLinearCaseSplit> &disjuncts )
    : _disjuncts( disjuncts )
{
    extractParticipatingVariables();
}

DisjunctionConstraint::DisjunctionConstraint( const String &// serializedDisjunction
                                              )
{
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

void DisjunctionConstraint::registerAsWatcher( ITableau *// tableau
                                               )
{
}

void DisjunctionConstraint::unregisterAsWatcher( ITableau *// tableau
                                                 )
{
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
}

void DisjunctionConstraint::notifyUpperBound( unsigned variable, double bound )
{
    if ( _statistics )
        _statistics->incNumBoundNotificationsPlConstraints();

    if ( _upperBounds.exists( variable ) && !FloatUtils::lt( bound, _upperBounds[variable] ) )
        return;

    _upperBounds[variable] = bound;
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
    List<PiecewiseLinearConstraint::Fix> fixes;
    return fixes;
}

List<PiecewiseLinearConstraint::Fix> DisjunctionConstraint::getSmartFixes( ITableau *// tableau
                                                                           ) const
{
    return getPossibleFixes();
}

List<PiecewiseLinearCaseSplit> DisjunctionConstraint::getCaseSplits() const
{
    return _disjuncts;
}

bool DisjunctionConstraint::phaseFixed() const
{
    return false;
}

PiecewiseLinearCaseSplit DisjunctionConstraint::getValidCaseSplit() const
{
    PiecewiseLinearCaseSplit split;
    return split;
}

void DisjunctionConstraint::dump( String &// output
                                  ) const
{
}

void DisjunctionConstraint::updateVariableIndex( unsigned oldIndex, unsigned newIndex )
{
	// ASSERT( oldIndex == _b || oldIndex == _f || ( _auxVarInUse && oldIndex == _aux ) );
    // ASSERT( !_assignment.exists( newIndex ) &&
    //         !_lowerBounds.exists( newIndex ) &&
    //         !_upperBounds.exists( newIndex ) &&
    //         newIndex != _b && newIndex != _f && ( !_auxVarInUse || newIndex != _aux ) );

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
}

void DisjunctionConstraint::eliminateVariable( unsigned // variable
                                               , double // fixedValue
                                               )
{
}

bool DisjunctionConstraint::constraintObsolete() const
{
    return false;
}

void DisjunctionConstraint::getEntailedTightenings( List<Tightening> &// tightenings
                                                    ) const
{
}

void DisjunctionConstraint::addAuxiliaryEquations( InputQuery &// inputQuery
                                                   )
{
}

void DisjunctionConstraint::getCostFunctionComponent( Map<unsigned, double> &// cost
                                                      ) const
{
}

String DisjunctionConstraint::serializeToString() const
{
    return String();
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
        printf( "Processing equation: \n");
        equation.dump();

        double result = 0;

        for ( const auto &addend : equation._addends )
            result += addend._coefficient * _assignment[addend._variable];

        printf( "\tresult: %lf\n", result );

        if ( !FloatUtils::areEqual( result, equation._scalar ) )
            return false;
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
