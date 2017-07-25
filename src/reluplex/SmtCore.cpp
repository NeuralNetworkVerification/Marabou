/*********************                                                        */
/*! \file SmtCore.cpp
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
#include "IEngine.h"
#include "SmtCore.h"
#include "TableauState.h"

SmtCore::SmtCore( IEngine *engine )
    : _statistics( NULL )
    , _engine( engine )
    , _needToSplit( false )
{
}

void SmtCore::reportViolatedConstraint( PiecewiseLinearConstraint *constraint )
{
    if ( !_constraintToViolationCount.exists( constraint ) )
        _constraintToViolationCount[constraint] = 0;

    ++_constraintToViolationCount[constraint];

    if ( _constraintToViolationCount[constraint] >= SPLIT_THRESHOLD )
    {
        _needToSplit = true;
        _constraintForSplitting = constraint;
    }
}

bool SmtCore::needToSplit() const
{
    return _needToSplit;
}

void SmtCore::performSplit()
{
    ASSERT( _needToSplit );
    _needToSplit = false;

    if ( _statistics )
        _statistics->incNumSplits();

    // First, obtain the current state of the tableau
    TableauState *stateBeforeSplits = new TableauState;
    _engine->storeTableauState( *stateBeforeSplits );

    // Obtain the splits
    List<PiecewiseLinearCaseSplit> splits = _constraintForSplitting->getCaseSplits();

    // Perform the first split: add bounds and equations
    List<PiecewiseLinearCaseSplit>::iterator split = splits.begin();
    applySplit( *split );

    // Store the remaining splits on the stack, for later
    StackEntry stackEntry;
    stackEntry._tableauState = stateBeforeSplits;
    ++split;
    while ( split != splits.end() )
    {
        stackEntry._splits.append( *split );
        ++split;
    }

    _stack.push( stackEntry );
    if ( _statistics )
        _statistics->setCurrentStackDepth( getStackDepth() );
}

unsigned SmtCore::getStackDepth() const
{
    return _stack.size();
}

bool SmtCore::popSplit()
{
    if ( _stack.empty() )
        return false;

    if ( _statistics )
        _statistics->incNumPops();

    StackEntry &stackEntry( _stack.top() );

    // Restore the state of the tableau
    _engine->restoreTableauState( *stackEntry._tableauState );

    // Apply the new split and erase it from the list
    auto split = stackEntry._splits.begin();
    applySplit( *split );
    stackEntry._splits.erase( split );

    // If there are no splits left, pop this entry
    if ( stackEntry._splits.size() == 0 )
        _stack.pop();

    if ( _statistics )
        _statistics->setCurrentStackDepth( getStackDepth() );

    return true;
}

void SmtCore::applySplit( const PiecewiseLinearCaseSplit &split )
{
    // Guy: if a split includes more than one equation, we want to add all of them, not just one.
    // Please adjust also the test for smtCore to check this.
    _engine->addNewEquation( split.getEquations().front() );
    List<PiecewiseLinearCaseSplit::Bound> bounds = split.getBoundTightenings();
    for ( const auto &bound : bounds )
    {
        if ( bound._boundType == PiecewiseLinearCaseSplit::Bound::LOWER )
            _engine->tightenLowerBound( bound._variable, bound._newBound );
        else
            _engine->tightenUpperBound( bound._variable, bound._newBound );
    }
}

void SmtCore::setStatistics( Statistics *statistics )
{
    _statistics = statistics;
}

//
// Local Variables:
// compile-command: "make -C .. "
// tags-file-name: "../TAGS"
// c-basic-offset: 4
// End:
//
