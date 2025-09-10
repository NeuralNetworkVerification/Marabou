/*********************                                                        */
/*! \file SearchTreeHandler.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz, Parth Shah, Duligur Ibeling
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2024 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

**/

#include "SearchTreeHandler.h"

#include "Debug.h"
#include "EngineState.h"
#include "FloatUtils.h"
#include "GlobalConfiguration.h"
#include "IEngine.h"
#include "MStringf.h"
#include "MarabouError.h"
#include "Options.h"
#include "PseudoImpactTracker.h"
#include "UnsatCertificateNode.h"

SearchTreeHandler::SearchTreeHandler( IEngine *engine )
    : _statistics( NULL )
    , _engine( engine )
    , _context( _engine->getContext() )
    , _needToSplit( false )
    , _constraintForSplitting( NULL )
    , _stateId( 0 )
    , _constraintViolationThreshold(
          Options::get()->getInt( Options::CONSTRAINT_VIOLATION_THRESHOLD ) )
    , _deepSoIRejectionThreshold( Options::get()->getInt( Options::DEEP_SOI_REJECTION_THRESHOLD ) )
    , _branchingHeuristic( Options::get()->getDivideStrategy() )
    , _scoreTracker( nullptr )
    , _numRejectedPhasePatternProposal( 0 )
{
}

SearchTreeHandler::~SearchTreeHandler()
{
    freeMemory();
}

void SearchTreeHandler::freeMemory()
{
    for ( const auto &stackEntry : _stack )
    {
        delete stackEntry->_engineState;
        delete stackEntry;
    }

    _stack.clear();
}

void SearchTreeHandler::reset()
{
    _context.popto( 0 );
    _engine->postContextPopHook();
    freeMemory();
    _impliedValidSplitsAtRoot.clear();
    _needToSplit = false;
    _constraintForSplitting = NULL;
    _stateId = 0;
    _constraintToViolationCount.clear();
    _numRejectedPhasePatternProposal = 0;
}

void SearchTreeHandler::reportViolatedConstraint( PiecewiseLinearConstraint *constraint )
{
    if ( !_constraintToViolationCount.exists( constraint ) )
        _constraintToViolationCount[constraint] = 0;

    ++_constraintToViolationCount[constraint];

    if ( _constraintToViolationCount[constraint] >= _constraintViolationThreshold )
    {
        _needToSplit = true;
        if ( !pickSplitPLConstraint() )
            // If pickSplitConstraint failed to pick one, use the native
            // relu-violation based splitting heuristic.
            _constraintForSplitting = constraint;
    }
}

unsigned SearchTreeHandler::getViolationCounts( PiecewiseLinearConstraint *constraint ) const
{
    if ( !_constraintToViolationCount.exists( constraint ) )
        return 0;

    return _constraintToViolationCount[constraint];
}

void SearchTreeHandler::initializeScoreTrackerIfNeeded(
    const List<PiecewiseLinearConstraint *> &plConstraints )
{
    if ( GlobalConfiguration::USE_DEEPSOI_LOCAL_SEARCH )
    {
        _scoreTracker = std::unique_ptr<PseudoImpactTracker>( new PseudoImpactTracker() );
        _scoreTracker->initialize( plConstraints );

        SEARCH_TREE_LOG( "\tTracking Pseudo Impact..." );
    }
}

void SearchTreeHandler::reportRejectedPhasePatternProposal()
{
    ++_numRejectedPhasePatternProposal;

    if ( _numRejectedPhasePatternProposal >= _deepSoIRejectionThreshold )
    {
        _needToSplit = true;
        _engine->applyAllBoundTightenings();
        _engine->applyAllValidConstraintCaseSplits();
        if ( !pickSplitPLConstraint() )
            // If pickSplitConstraint failed to pick one, use the native
            // relu-violation based splitting heuristic.
            _constraintForSplitting = _scoreTracker->topUnfixed();
    }
}

bool SearchTreeHandler::needToSplit() const
{
    return _needToSplit;
}

void SearchTreeHandler::performSplit()
{
    ASSERT( _needToSplit );

    _numRejectedPhasePatternProposal = 0;
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
        _statistics->incUnsignedAttribute( Statistics::NUM_SPLITS );
        _statistics->incUnsignedAttribute( Statistics::NUM_VISITED_TREE_STATES );
    }

    // Before storing the state of the engine, we:
    //   1. Obtain the splits.
    //   2. Disable the constraint, so that it is marked as disbaled in the EngineState.
    List<PiecewiseLinearCaseSplit> splits = _constraintForSplitting->getCaseSplits();
    ASSERT( !splits.empty() );
    ASSERT( splits.size() >= 2 ); // Not really necessary, can add code to handle this case.
    _constraintForSplitting->setActiveConstraint( false );

    // Obtain the current state of the engine
    EngineState *stateBeforeSplits = new EngineState;
    stateBeforeSplits->_stateId = _stateId;
    ++_stateId;
    _engine->storeState( *stateBeforeSplits, TableauStateStorageLevel::STORE_BOUNDS_ONLY );
    _engine->preContextPushHook();
    pushContext();

    UnsatCertificateNode *certificateNode = NULL;
    if ( _engine->shouldProduceProofs() && _engine->getUNSATCertificateRoot() )
    {
        certificateNode = _engine->getUNSATCertificateCurrentPointer();
        // Create children for UNSATCertificate current node, and assign a split to each of them
        ASSERT( certificateNode );
        for ( PiecewiseLinearCaseSplit &childSplit : splits )
            new UnsatCertificateNode( certificateNode, childSplit );
    }

    SearchTreeStackEntry *stackEntry = new SearchTreeStackEntry;
    // Perform the first split: add bounds and equations
    List<PiecewiseLinearCaseSplit>::iterator split = splits.begin();
    ASSERT( split->getEquations().size() == 0 );

    if ( _engine->shouldProduceProofs() && _engine->getUNSATCertificateRoot() )
    {
        // Set the current node of the UNSAT certificate to be the child corresponding to the first
        // split
        UnsatCertificateNode *firstSplitChild = certificateNode->getChildBySplit( *split );
        ASSERT( firstSplitChild );
        _engine->setUNSATCertificateCurrentPointer( firstSplitChild );
        ASSERT( _engine->getUNSATCertificateCurrentPointer()->getSplit() == *split );
    }

    _engine->applySplit( *split );
    stackEntry->_activeSplit = *split;

    // Store the remaining splits on the stack, for later
    stackEntry->_engineState = stateBeforeSplits;
    ++split;
    while ( split != splits.end() )
    {
        stackEntry->_alternativeSplits.append( *split );
        ++split;
    }

    _stack.append( stackEntry );

    if ( _statistics )
    {
        unsigned level = getStackDepth();
        _statistics->setUnsignedAttribute( Statistics::CURRENT_DECISION_LEVEL, level );
        if ( level > _statistics->getUnsignedAttribute( Statistics::MAX_DECISION_LEVEL ) )
            _statistics->setUnsignedAttribute( Statistics::MAX_DECISION_LEVEL, level );
        struct timespec end = TimeUtils::sampleMicro();
        _statistics->incLongAttribute( Statistics::TOTAL_TIME_SEARCH_TREE_HANDLER_MICRO,
                                       TimeUtils::timePassed( start, end ) );
    }

    _constraintForSplitting = NULL;
}

unsigned SearchTreeHandler::getStackDepth() const
{
    ASSERT(
        ( _engine->inSnCMode() || _stack.size() == static_cast<unsigned>( _context.getLevel() ) ) );
    return _stack.size();
}

void SearchTreeHandler::popContext()
{
    struct timespec start = TimeUtils::sampleMicro();
    _context.pop();
    struct timespec end = TimeUtils::sampleMicro();

    if ( _statistics )
    {
        _statistics->incUnsignedAttribute( Statistics::NUM_CONTEXT_POPS );
        _statistics->incLongAttribute( Statistics::TIME_CONTEXT_POP,
                                       TimeUtils::timePassed( start, end ) );
    }
}

void SearchTreeHandler::pushContext()
{
    struct timespec start = TimeUtils::sampleMicro();
    _context.push();
    struct timespec end = TimeUtils::sampleMicro();

    if ( _statistics )
    {
        _statistics->incUnsignedAttribute( Statistics::NUM_CONTEXT_PUSHES );
        _statistics->incLongAttribute( Statistics::TIME_CONTEXT_PUSH,
                                       TimeUtils::timePassed( start, end ) );
    }
}

bool SearchTreeHandler::popSplit()
{
    SEARCH_TREE_LOG( "Performing a pop" );

    if ( _stack.empty() )
        return false;

    struct timespec start = TimeUtils::sampleMicro();

    if ( _statistics )
    {
        _statistics->incUnsignedAttribute( Statistics::NUM_POPS );
        // A pop always sends us to a state that we haven't seen before - whether
        // from a sibling split, or from a lower level of the tree.
        _statistics->incUnsignedAttribute( Statistics::NUM_VISITED_TREE_STATES );
    }

    bool inconsistent = true;
    while ( inconsistent )
    {
        // Remove any entries that have no alternatives
        String error;
        while ( _stack.back()->_alternativeSplits.empty() )
        {
            if ( checkSkewFromDebuggingSolution() )
            {
                // Pops should not occur from a compliant stack!
                printf( "Error! Popping from a compliant stack\n" );
                throw MarabouError( MarabouError::DEBUGGING_ERROR );
            }

            delete _stack.back()->_engineState;
            delete _stack.back();
            _stack.popBack();
            popContext();

            if ( _engine->shouldProduceProofs() && _engine->getUNSATCertificateCurrentPointer() )
            {
                UnsatCertificateNode *certificateNode =
                    _engine->getUNSATCertificateCurrentPointer();
                certificateNode->deleteUnusedLemmas();
                _engine->setUNSATCertificateCurrentPointer( certificateNode->getParent() );
            }

            if ( _stack.empty() )
                return false;
        }

        if ( checkSkewFromDebuggingSolution() )
        {
            // Pops should not occur from a compliant stack!
            printf( "Error! Popping from a compliant stack\n" );
            throw MarabouError( MarabouError::DEBUGGING_ERROR );
        }

        SearchTreeStackEntry *stackEntry = _stack.back();

        if ( _engine->shouldProduceProofs() && _engine->getUNSATCertificateCurrentPointer() )
            _engine->getUNSATCertificateCurrentPointer()->deleteUnusedLemmas();

        popContext();
        _engine->postContextPopHook();
        // Restore the state of the engine
        SEARCH_TREE_LOG( "\tRestoring engine state..." );
        _engine->restoreState( *( stackEntry->_engineState ) );
        SEARCH_TREE_LOG( "\tRestoring engine state - DONE" );

        // Apply the new split and erase it from the list
        auto split = stackEntry->_alternativeSplits.begin();

        // Erase any valid splits that were learned using the split we just
        // popped
        stackEntry->_impliedValidSplits.clear();

        // Set the current node of the UNSAT certificate to be the child corresponding to the chosen
        // split
        if ( _engine->shouldProduceProofs() && _engine->getUNSATCertificateCurrentPointer() )
        {
            UnsatCertificateNode *certificateNode = _engine->getUNSATCertificateCurrentPointer();
            ASSERT( certificateNode );
            UnsatCertificateNode *splitChild = certificateNode->getChildBySplit( *split );
            while ( !splitChild )
            {
                certificateNode->deleteUnusedLemmas();
                certificateNode = certificateNode->getParent();
                ASSERT( certificateNode );
                splitChild = certificateNode->getChildBySplit( *split );
            }
            ASSERT( splitChild );
            _engine->setUNSATCertificateCurrentPointer( splitChild );
            ASSERT( _engine->getUNSATCertificateCurrentPointer()->getSplit() == *split );
        }

        SEARCH_TREE_LOG( "\tApplying new split..." );
        ASSERT( split->getEquations().size() == 0 );
        _engine->preContextPushHook();
        pushContext();
        _engine->applySplit( *split );
        SEARCH_TREE_LOG( "\tApplying new split - DONE" );

        stackEntry->_activeSplit = *split;
        stackEntry->_alternativeSplits.erase( split );

        inconsistent = !_engine->consistentBounds();

        if ( _engine->shouldProduceProofs() && inconsistent )
            _engine->explainSimplexFailure();
    }

    if ( _statistics )
    {
        unsigned level = getStackDepth();
        _statistics->setUnsignedAttribute( Statistics::CURRENT_DECISION_LEVEL, level );
        if ( level > _statistics->getUnsignedAttribute( Statistics::MAX_DECISION_LEVEL ) )
            _statistics->setUnsignedAttribute( Statistics::MAX_DECISION_LEVEL, level );
        struct timespec end = TimeUtils::sampleMicro();
        _statistics->incLongAttribute( Statistics::TOTAL_TIME_SEARCH_TREE_HANDLER_MICRO,
                                       TimeUtils::timePassed( start, end ) );
    }

    checkSkewFromDebuggingSolution();

    return true;
}

void SearchTreeHandler::resetSplitConditions()
{
    _constraintToViolationCount.clear();
    _numRejectedPhasePatternProposal = 0;
    _needToSplit = false;
}

void SearchTreeHandler::recordImpliedValidSplit( PiecewiseLinearCaseSplit &validSplit )
{
    if ( _stack.empty() )
        _impliedValidSplitsAtRoot.append( validSplit );
    else
        _stack.back()->_impliedValidSplits.append( validSplit );

    checkSkewFromDebuggingSolution();
}

void SearchTreeHandler::allSplitsSoFar( List<PiecewiseLinearCaseSplit> &result ) const
{
    result.clear();

    for ( const auto &it : _impliedValidSplitsAtRoot )
        result.append( it );

    for ( const auto &it : _stack )
    {
        result.append( it->_activeSplit );
        for ( const auto &impliedSplit : it->_impliedValidSplits )
            result.append( impliedSplit );
    }
}

void SearchTreeHandler::setStatistics( Statistics *statistics )
{
    _statistics = statistics;
}

void SearchTreeHandler::storeDebuggingSolution( const Map<unsigned, double> &debuggingSolution )
{
    _debuggingSolution = debuggingSolution;
}

// Return true if stack is currently compliant, false otherwise
// If there is no stored solution, return false --- incompliant.
bool SearchTreeHandler::checkSkewFromDebuggingSolution()
{
    if ( _debuggingSolution.empty() )
        return false;

    String error;

    // First check that the valid splits implied at the root level are okay
    for ( const auto &split : _impliedValidSplitsAtRoot )
    {
        if ( !splitAllowsStoredSolution( split, error ) )
        {
            printf( "Error with one of the splits implied at root level:\n\t%s\n", error.ascii() );
            throw MarabouError( MarabouError::DEBUGGING_ERROR );
        }
    }

    // Now go over the stack from oldest to newest and check that each level is compliant
    for ( const auto &stackEntry : _stack )
    {
        // If the active split is non-compliant but there are alternatives, that's fine
        if ( !splitAllowsStoredSolution( stackEntry->_activeSplit, error ) )
        {
            if ( stackEntry->_alternativeSplits.empty() )
            {
                printf( "Error! Have a split that is non-compliant with the stored solution, "
                        "without alternatives:\n\t%s\n",
                        error.ascii() );
                throw MarabouError( MarabouError::DEBUGGING_ERROR );
            }

            // Active split is non-compliant but this is fine, because there are alternatives. We're
            // done.
            return false;
        }

        // Did we learn any valid splits that are non-compliant?
        for ( auto const &split : stackEntry->_impliedValidSplits )
        {
            if ( !splitAllowsStoredSolution( split, error ) )
            {
                printf( "Error with one of the splits implied at this stack level:\n\t%s\n",
                        error.ascii() );
                throw MarabouError( MarabouError::DEBUGGING_ERROR );
            }
        }
    }

    // No problems were detected, the stack is compliant with the stored solution
    return true;
}

bool SearchTreeHandler::splitAllowsStoredSolution( const PiecewiseLinearCaseSplit &split,
                                                   String &error ) const
{
    // False if the split prevents one of the values in the stored solution, true otherwise.
    error = "";
    if ( _debuggingSolution.empty() )
        return true;

    for ( const auto &bound : split.getBoundTightenings() )
    {
        unsigned variable = bound._variable;

        // If the possible solution doesn't care about this variable,
        // ignore it
        if ( !_debuggingSolution.exists( variable ) )
            continue;

        // Otherwise, check that the bound is consistent with the solution
        double solutionValue = _debuggingSolution[variable];
        double boundValue = bound._value;

        if ( ( bound._type == Tightening::LB ) && FloatUtils::gt( boundValue, solutionValue ) )
        {
            error =
                Stringf( "Variable %u: new LB is %.5lf, which contradicts possible solution %.5lf",
                         variable,
                         boundValue,
                         solutionValue );
            return false;
        }
        else if ( ( bound._type == Tightening::UB ) && FloatUtils::lt( boundValue, solutionValue ) )
        {
            error =
                Stringf( "Variable %u: new UB is %.5lf, which contradicts possible solution %.5lf",
                         variable,
                         boundValue,
                         solutionValue );
            return false;
        }
    }

    return true;
}

PiecewiseLinearConstraint *SearchTreeHandler::chooseViolatedConstraintForFixing(
    List<PiecewiseLinearConstraint *> &_violatedPlConstraints ) const
{
    ASSERT( !_violatedPlConstraints.empty() );

    if ( !GlobalConfiguration::USE_LEAST_FIX )
        return *_violatedPlConstraints.begin();

    PiecewiseLinearConstraint *candidate;

    // Apply the least fix heuristic
    auto it = _violatedPlConstraints.begin();

    candidate = *it;
    unsigned minFixes = getViolationCounts( candidate );

    PiecewiseLinearConstraint *contender;
    unsigned contenderFixes;
    while ( it != _violatedPlConstraints.end() )
    {
        contender = *it;
        contenderFixes = getViolationCounts( contender );
        if ( contenderFixes < minFixes )
        {
            minFixes = contenderFixes;
            candidate = contender;
        }

        ++it;
    }

    return candidate;
}

void SearchTreeHandler::replaySearchTreeStackEntry( SearchTreeStackEntry *stackEntry )
{
    struct timespec start = TimeUtils::sampleMicro();

    if ( _statistics )
    {
        _statistics->incUnsignedAttribute( Statistics::NUM_SPLITS );
        _statistics->incUnsignedAttribute( Statistics::NUM_VISITED_TREE_STATES );
    }

    // Obtain the current state of the engine
    EngineState *stateBeforeSplits = new EngineState;
    stateBeforeSplits->_stateId = _stateId;
    ++_stateId;
    _engine->storeState( *stateBeforeSplits, TableauStateStorageLevel::STORE_ENTIRE_TABLEAU_STATE );
    stackEntry->_engineState = stateBeforeSplits;

    // Apply all the splits
    _engine->applySplit( stackEntry->_activeSplit );
    for ( const auto &impliedSplit : stackEntry->_impliedValidSplits )
        _engine->applySplit( impliedSplit );

    _stack.append( stackEntry );

    if ( _statistics )
    {
        unsigned level = getStackDepth();
        _statistics->setUnsignedAttribute( Statistics::CURRENT_DECISION_LEVEL, level );
        if ( level > _statistics->getUnsignedAttribute( Statistics::MAX_DECISION_LEVEL ) )
            _statistics->setUnsignedAttribute( Statistics::MAX_DECISION_LEVEL, level );
        struct timespec end = TimeUtils::sampleMicro();
        _statistics->incLongAttribute( Statistics::TOTAL_TIME_SEARCH_TREE_HANDLER_MICRO,
                                       TimeUtils::timePassed( start, end ) );
    }
}

void SearchTreeHandler::storeSearchTreeState( SearchTreeState &searchTreeState )
{
    searchTreeState._impliedValidSplitsAtRoot = _impliedValidSplitsAtRoot;

    for ( auto &stackEntry : _stack )
        searchTreeState._stack.append( stackEntry->duplicateSearchTreeStackEntry() );

    searchTreeState._stateId = _stateId;
}

bool SearchTreeHandler::pickSplitPLConstraint()
{
    if ( _needToSplit )
    {
        _constraintForSplitting = _engine->pickSplitPLConstraint( _branchingHeuristic );
    }
    return _constraintForSplitting != NULL;
}
