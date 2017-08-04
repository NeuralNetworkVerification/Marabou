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
#include "EngineState.h"
#include "IEngine.h"
#include "SmtCore.h"

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

    // First, obtain the current state of the engine
    EngineState *stateBeforeSplits = new EngineState;
    _engine->storeState( *stateBeforeSplits );

    // Obtain the splits
    List<PiecewiseLinearCaseSplit> splits = _constraintForSplitting->getCaseSplits();
    ASSERT( !splits.empty() );

    // Perform the first split: add bounds and equations
    List<PiecewiseLinearCaseSplit>::iterator split = splits.begin();
    _constraintForSplitting->setActiveConstraint( false );
    _engine->applySplit( *split );

    // Store the remaining splits on the stack, for later
    StackEntry stackEntry;
    stackEntry._engineState = stateBeforeSplits;
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

    // Restore the state of the engine
    _engine->restoreState( *stackEntry._engineState );

    // Apply the new split and erase it from the list
    auto split = stackEntry._splits.begin();
    _engine->applySplit( *split );
    stackEntry._splits.erase( split );

    // If there are no splits left, pop this entry
    if ( stackEntry._splits.size() == 0 )
    {
        delete stackEntry._engineState;
        _stack.pop();
    }

    if ( _statistics )
        _statistics->setCurrentStackDepth( getStackDepth() );

    return true;
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
