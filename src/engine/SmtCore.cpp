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
#include "GlobalConfiguration.h"
#include "IEngine.h"
#include "MString.h"
#include "SmtCore.h"

SmtCore::SmtCore( IEngine *engine )
    : _statistics( NULL )
    , _engine( engine )
    , _needToSplit( false )
{
}

SmtCore::~SmtCore()
{
    while ( !_stack.empty() )
    {
        StackEntry &stackEntry( _stack.top() );
        delete stackEntry._engineState;
        stackEntry._engineState = NULL;
        _stack.pop();
    }
}

void SmtCore::reportViolatedConstraint( PiecewiseLinearConstraint *constraint )
{
    if ( !_constraintToViolationCount.exists( constraint ) )
        _constraintToViolationCount[constraint] = 0;

    ++_constraintToViolationCount[constraint];

    if ( _constraintToViolationCount[constraint] >= GlobalConfiguration::CONSTRAINT_VIOLATION_THRESHOLD )
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

    // Maybe the constraint has already become inactive - if so, ignore
    if ( !_constraintForSplitting->isActive() )
    {
        _needToSplit = false;
        _constraintToViolationCount[_constraintForSplitting] = 0;
        _constraintForSplitting = NULL;
        return;
    }

    struct timespec start = TimeUtils::sampleMicro();

    ASSERT( _constraintForSplitting->isActive() );
    _needToSplit = false;

    if ( _statistics )
    {
        _statistics->incNumSplits();
        _statistics->incNumVisitedTreeStates();
    }

    // Before storing the state of the engine, we:
    //   1. Obtain the splits. This increments the AuxVariable counter before it
    //      is stored as part of the EngineState.
    //   2. Disable the constraint, so that it is marked as disbaled in the EngineState.
    List<PiecewiseLinearCaseSplit> splits = _constraintForSplitting->getCaseSplits();
    ASSERT( !splits.empty() );
    ASSERT( splits.size() >= 2 ); // Not really necessary, can add code to handle this case.
    _constraintForSplitting->setActiveConstraint( false );

    // Obtain the current state of the engine
    EngineState *stateBeforeSplits = new EngineState;
    _engine->storeState( *stateBeforeSplits );

    // Perform the first split: add bounds and equations
    List<PiecewiseLinearCaseSplit>::iterator split = splits.begin();
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
    {
        _statistics->setCurrentStackDepth( getStackDepth() );
        struct timespec end = TimeUtils::sampleMicro();
        _statistics->addTimeSmtCore( TimeUtils::timePassed( start, end ) );
    }
}

unsigned SmtCore::getStackDepth() const
{
    return _stack.size();
}

bool SmtCore::popSplit()
{
    log( "Performing a pop" );

    if ( _stack.empty() )
        return false;

    struct timespec start = TimeUtils::sampleMicro();

    if ( _statistics )
    {
        _statistics->incNumPops();
        // A pop always sends us to a state that we haven't seen before - whether
        // from a sibling split, or from a lower level of the tree.
        _statistics->incNumVisitedTreeStates();
    }

    StackEntry &stackEntry( _stack.top() );

    // Restore the state of the engine
    log( "\tRestoring engine state..." );
    _engine->restoreState( *stackEntry._engineState );
    log( "\tRestoring engine state - DONE" );

    // Apply the new split and erase it from the list
    auto split = stackEntry._splits.begin();

    log( "\tApplying new split..." );
    _engine->applySplit( *split );
    log( "\tApplying new split - DONE" );

    stackEntry._splits.erase( split );

    // If there are no splits left, pop this entry
    if ( stackEntry._splits.size() == 0 )
    {
        delete stackEntry._engineState;
        _stack.pop();
    }

    if ( _statistics )
    {
        _statistics->setCurrentStackDepth( getStackDepth() );
        struct timespec end = TimeUtils::sampleMicro();
        _statistics->addTimeSmtCore( TimeUtils::timePassed( start, end ) );
    }

    return true;
}

void SmtCore::setStatistics( Statistics *statistics )
{
    _statistics = statistics;
}

void SmtCore::log( const String &message )
{
    if ( GlobalConfiguration::SMT_CORE_LOGGING )
        printf( "SmtCore: %s\n", message.ascii() );
}

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
