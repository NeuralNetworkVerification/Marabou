/*********************                                                        */
/*! \file Engine.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **/

#include "Debug.h"
#include "Engine.h"
#include "InputQuery.h"
#include "PiecewiseLinearConstraint.h"
#include "TableauRow.h"

Engine::Engine()
{
}

Engine::~Engine()
{
}

bool Engine::solve()
{
    // Todo: If l >= u for some var, fail immediately

    while ( true )
    {
        _tableau->computeAssignment();
        _tableau->computeBasicStatus();

        _tableau->dumpAssignment();

        if ( allVarsWithinBounds() )
        {
            // If all variables are within bounds and all PL
            // constraints hold, we're done
            extractPlAssignment();
            if ( allPlConstraintsHold() )
                return true;
            else if ( !fixViolatedPlConstraint() )
                return false;
        }
        else
        {
            // We have out-of-bounds variables.

            // If a simplex step fails, the query is unsat
            if ( !performSimplexStep() )
                return false;
        }
    }
}

bool Engine::performSimplexStep()
{
    // // Debug
    // for ( unsigned i = 0; i < _tableau->getM(); ++i )
    // {
    //     printf( "Extracting tableau row %u\n", i );
    //     TableauRow row( _tableau->getN() - _tableau->getM() );
    //     _tableau->getTableauRow( i, &row );
    //     row.dump();
    // }
    // //

    _tableau->computeCostFunction();
    _tableau->dumpCostFunction();

    if ( !_tableau->pickEnteringVariable( &_dantzigsRule ) )
        return false;

    _tableau->computeD();
    _tableau->pickLeavingVariable();
    _tableau->performPivot();

    return true;
}

bool Engine::fixViolatedPlConstraint()
{
    PiecewiseLinearConstraint *violated = NULL;
    for ( const auto &constraint : _plConstraints )
    {
        if ( !constraint->satisfied( _plVarAssignment ) )
            violated = constraint;
    }

    ASSERT( violated );

    List<PiecewiseLinearConstraint::Fix> fixes = violated->getPossibleFixes( _plVarAssignment );

    // First, see if we can fix without pivoting
    for ( const auto &fix : fixes )
    {
        if ( !_tableau->isBasic( fix._variable ) )
        {
            _tableau->setNonBasicAssignment( fix._variable, fix._value );
            return true;
        }
    }

    // No choice, have to pivot
    PiecewiseLinearConstraint::Fix fix = *fixes.begin();
    ASSERT( _tableau->isBasic( fix._variable ) );

    TableauRow row( _tableau->getN() - _tableau->getM() );
    _tableau->getTableauRow( fix._variable, &row );

    unsigned nonBasic;
    bool done = false;
    unsigned i = 0;
    while ( !done && ( i < _tableau->getN() - _tableau->getM() ) )
    {
        // TODO: numerical stability. Pick a good candidate.
        // TODO: guarantee that candidate does not participate in the
        // same PL constraint?
        if ( !FloatUtils::isZero( row._row->_coefficient ) )
        {
            done = true;
            nonBasic = row._row->_var;
        }

        ++i;
    }

    ASSERT( done );
    // Switch between nonBasic and the variable we need to fix
    _tableau->performDegeneratePivot( _tableau->variableToIndex( nonBasic ),
                                      _tableau->variableToIndex( fix._variable ) );

    ASSERT( !_tableau->isBasic( fix._variable ) );
    _tableau->setNonBasicAssignment( fix._variable, fix._value );
    return true;

    // printf( "Could not fix a violated PL constraint\n" );
    // return false;
}

void Engine::processInputQuery( const InputQuery &inputQuery )
{
    const List<Equation> equations( inputQuery.getEquations() );

    unsigned m = equations.size();
    unsigned n = inputQuery.getNumberOfVariables();
    _tableau->setDimensions( m, n );

    unsigned equationIndex = 0;
    for ( const auto &equation : equations )
    {
        _tableau->markAsBasic( equation._auxVariable );
        _tableau->setRightHandSide( equationIndex, equation._scalar );

        for ( const auto &addend : equation._addends )
            _tableau->setEntryValue( equationIndex, addend._variable, addend._coefficient );

        ++equationIndex;
    }

    for ( unsigned i = 0; i < n; ++i )
    {
        _tableau->setLowerBound( i, inputQuery.getLowerBound( i ) );
        _tableau->setUpperBound( i, inputQuery.getUpperBound( i ) );
    }

    _tableau->initializeTableau();

    _plConstraints = inputQuery.getPiecewiseLinearConstraints();
    for ( const auto &constraint : _plConstraints )
    {
        List<unsigned> participatingVariables = constraint->getParticiatingVariables();
        for ( const auto &var : participatingVariables )
            _plVarAssignment[var] = 0;
    }
}

void Engine::extractSolution( InputQuery &inputQuery )
{
    for ( unsigned i = 0; i < inputQuery.getNumberOfVariables(); ++i )
        inputQuery.setSolutionValue( i, _tableau->getValue( i ) );
}

bool Engine::allVarsWithinBounds() const
{
    return !_tableau->existsBasicOutOfBounds();
}

bool Engine::allPlConstraintsHold()
{
    for ( const auto &constraint : _plConstraints )
        if ( !constraint->satisfied( _plVarAssignment ) )
            return false;

    return true;
}

void Engine::extractPlAssignment()
{
    for ( auto it : _plVarAssignment )
        _plVarAssignment[it.first] = _tableau->getValue( it.first );
}

const Set<unsigned> Engine::getVarsInPlConstraints()
{
    return _plVarAssignment.keys();
}

//
// Local Variables:
// compile-command: "make -C .. "
// tags-file-name: "../TAGS"
// c-basic-offset: 4
// End:
//
