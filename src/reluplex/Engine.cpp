/*********************                                                        */
/*! \file Engine.cpp
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
#include "FreshVariables.h"
#include "InputQuery.h"
#include "PiecewiseLinearConstraint.h"
#include "TableauRow.h"
#include "Time.h"

Engine::Engine()
    : _smtCore( this )
{
    _smtCore.setStatistics( &_statistics );
}

Engine::~Engine()
{
}

bool Engine::solve()
{
    // Todo: If l >= u for some var, fail immediately

    while ( true )
    {
        if ( _statistics.getNumMainLoopIterations() % GlobalConfiguration::STATISTICS_PRINTING_FREQUENCY == 0 )
            _statistics.print();
        _statistics.incNumMainLoopIterations();

        _tableau->computeAssignment();
        _tableau->computeBasicStatus();

        // TODO: tighten bounds
        // TODO: split if necessary

        // _tableau->dumpAssignment();

        if ( allVarsWithinBounds() )
        {
            // Check the status of the PL constraints
            collectViolatedPlConstraints();

            // If all constraints are satisfied, we are done
            if ( allPlConstraintsHold() )
                return true;

            // We have violated piecewise-linear constraints.
            _statistics.incNumConstraintFixingSteps();

            // Select a violated constraint as the target
            selectViolatedPlConstraint();

            // Report the violated constraint to the SMT engine
            reportPlViolation();

            // Attempt to fix the constraint
            if ( !fixViolatedPlConstraint() )
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

    // Statistics
    _statistics.incNumSimplexSteps();
    timeval start = Time::sampleMicro();

    if ( !(_nestedDantzigsRule.select( _tableau )) )
    {
        timeval end = Time::sampleMicro();
        _statistics.addTimeSimplexSteps( Time::timePassed( start, end ) );
        return false;
    }

    // If you use the full pricing Dantzig's rule, need to calculate entire cost function
    // _tableau->computeCostFunction();
    // _tableau->dumpCostFunction();

    // if ( !_tableau->pickEnteringVariable( &_dantzigsRule ) )
    //     return false;

    _tableau->computeD();
    _tableau->pickLeavingVariable();
    _tableau->performPivot();

    timeval end = Time::sampleMicro();
    _statistics.addTimeSimplexSteps( Time::timePassed( start, end ) );
    return true;
}

bool Engine::fixViolatedPlConstraint()
{
    PiecewiseLinearConstraint *violated = NULL;
    for ( const auto &constraint : _plConstraints )
    {
        if ( !constraint->satisfied() )
            violated = constraint;
    }

    ASSERT( violated );

    List<PiecewiseLinearConstraint::Fix> fixes = violated->getPossibleFixes();

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

    // Current variables are [0,..,n-1], so the next variable is n.
    FreshVariables::setNextVariable( n );

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

    _plConstraints = inputQuery.getPiecewiseLinearConstraints();
    for ( const auto &constraint : _plConstraints )
        constraint->registerAsWatcher( _tableau );

    _tableau->initializeTableau();
    _nestedDantzigsRule.initialize(_tableau);

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

void Engine::collectViolatedPlConstraints()
{
    _violatedPlConstraints.clear();
    for ( const auto &constraint : _plConstraints )
        if ( !constraint->satisfied() )
            _violatedPlConstraints.append( constraint );
}

bool Engine::allPlConstraintsHold()
{
    return _violatedPlConstraints.empty();
}

void Engine::selectViolatedPlConstraint()
{
    _plConstraintToFix = *_violatedPlConstraints.begin();
}

void Engine::reportPlViolation()
{
    _smtCore.reportViolatedConstraint( _plConstraintToFix );
}

void Engine::tightenLowerBound( unsigned variable, double bound )
{
    _tableau->tightenLowerBound( variable, bound );
}

void Engine::tightenUpperBound( unsigned variable, double bound )
{
    _tableau->tightenUpperBound( variable, bound );
}

void Engine::addNewEquation( const Equation &equation )
{
    _tableau->addEquation( equation );
}

void Engine::storeTableauState( TableauState &state ) const
{
    _tableau->storeState( state );
}

void Engine::restoreTableauState( const TableauState &state )
{
    _tableau->restoreState( state );
}

void Engine::log( const String &line ) const
{
    printf( "Engine: %s\n", line.ascii() );
}

//
// Local Variables:
// compile-command: "make -C .. "
// tags-file-name: "../TAGS"
// c-basic-offset: 4
// End:
//
