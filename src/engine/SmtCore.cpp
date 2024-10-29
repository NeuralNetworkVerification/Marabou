/*********************                                                        */
/*! \file SmtCore.cpp
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

#include "SmtCore.h"

#include "AutoConstraintMatrixAnalyzer.h"
#include "Debug.h"
#include "Engine.h"
#include "EngineState.h"
#include "FloatUtils.h"
#include "GlobalConfiguration.h"
#include "IEngine.h"
#include "InputQuery.h"
#include "MStringf.h"
#include "MarabouError.h"
#include "Options.h"
#include "PiecewiseLinearConstraint.h"
#include "PseudoImpactTracker.h"
#include "TimeUtils.h"
#include "UnsatCertificateNode.h"
#include "Vector.h"

#include <random>

SmtCore::SmtCore( IEngine *engine )
    : _exitCode( ExitCode::NOT_DONE )
    , _statistics( NULL )
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
    , _cadicalWrapper()
    , _cadicalVarToPlc()
    , _literalsToPropagate()
    , _assignedLiterals( &_engine->getContext() )
    , _reasonClauseLiterals()
    , _isReasonClauseInitialized( false )
    , _fixedCadicalVars()
    , _timeoutInSeconds( 0 )
    , _numOfClauses( 0 )
    , _satisfiedClauses( &_engine->getContext() )
    , _literalToClauses()
    , _vsidsDecayThreshold( 0 )
    , _vsidsDecayCounter( 0 )
{
    _cadicalVarToPlc.insert( 0, NULL );
}

SmtCore::~SmtCore()
{
    freeMemory();

    _cadicalWrapper.disconnectTerminator();
    _cadicalWrapper.disconnectTheorySolver();
}

void SmtCore::freeMemory()
{
    for ( const auto &stackEntry : _stack )
    {
        delete stackEntry->_engineState;
        delete stackEntry;
    }

    _stack.clear();
}

void SmtCore::reset()
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
    resetExitCode();
}

ExitCode SmtCore::getExitCode() const
{
    return _exitCode;
}

void SmtCore::setExitCode( ExitCode exitCode )
{
    _exitCode = exitCode;
}

void SmtCore::reportViolatedConstraint( PiecewiseLinearConstraint *constraint )
{
    if ( !_constraintToViolationCount.exists( constraint ) )
        _constraintToViolationCount[constraint] = 0;

    ++_constraintToViolationCount[constraint];

    if ( _constraintToViolationCount[constraint] >= _constraintViolationThreshold )
    {
        _needToSplit = true;
        assert( !constraint->phaseFixed() );
    }
}

unsigned SmtCore::getViolationCounts( PiecewiseLinearConstraint *constraint ) const
{
    if ( !_constraintToViolationCount.exists( constraint ) )
        return 0;

    return _constraintToViolationCount[constraint];
}

void SmtCore::initializeScoreTrackerIfNeeded(
    const List<PiecewiseLinearConstraint *> &plConstraints )
{
    if ( GlobalConfiguration::USE_DEEPSOI_LOCAL_SEARCH )
    {
        _scoreTracker = std::unique_ptr<PseudoImpactTracker>( new PseudoImpactTracker() );
        _scoreTracker->initialize( plConstraints );

        SMT_LOG( "\tTracking Pseudo Impact..." );
    }
}

void SmtCore::reportRejectedPhasePatternProposal()
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

bool SmtCore::needToSplit() const
{
    return _needToSplit;
}

void SmtCore::setNeedToSplit( bool needToSplit )
{
    _needToSplit = needToSplit;
}

void SmtCore::performSplit()
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

    //    struct timespec start = TimeUtils::sampleMicro();

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

    SmtStackEntry *stackEntry = new SmtStackEntry;
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

    _constraintForSplitting = NULL;
}

unsigned SmtCore::getStackDepth() const
{
    ASSERT(
        ( _engine->inSnCMode() || _stack.size() == static_cast<unsigned>( _context.getLevel() ) ) );
    return _stack.size();
}

void SmtCore::popContext()
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

void SmtCore::popContextTo( unsigned int level )
{
    struct timespec start = TimeUtils::sampleMicro();
    unsigned int prevLevel = _context.getLevel();
    _context.popto( level );
    struct timespec end = TimeUtils::sampleMicro();

    if ( _statistics )
    {
        _statistics->incUnsignedAttribute( Statistics::NUM_CONTEXT_POPS, prevLevel - level );
        _statistics->incLongAttribute( Statistics::TIME_CONTEXT_POP,
                                       TimeUtils::timePassed( start, end ) );
    }
}

void SmtCore::pushContext()
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

bool SmtCore::popSplit()
{
    SMT_LOG( "Performing a pop" );

    if ( _stack.empty() )
        return false;

    //    struct timespec start = TimeUtils::sampleMicro();

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

        SmtStackEntry *stackEntry = _stack.back();

        popContext();
        _engine->postContextPopHook();
        // Restore the state of the engine
        SMT_LOG( "\tRestoring engine state..." );
        _engine->restoreState( *( stackEntry->_engineState ) );
        SMT_LOG( "\tRestoring engine state - DONE" );

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
                certificateNode = certificateNode->getParent();
                ASSERT( certificateNode );
                splitChild = certificateNode->getChildBySplit( *split );
            }
            ASSERT( splitChild );
            _engine->setUNSATCertificateCurrentPointer( splitChild );
            ASSERT( _engine->getUNSATCertificateCurrentPointer()->getSplit() == *split );
        }

        SMT_LOG( "\tApplying new split..." );
        ASSERT( split->getEquations().size() == 0 );
        _engine->preContextPushHook();
        pushContext();
        _engine->applySplit( *split );
        SMT_LOG( "\tApplying new split - DONE" );

        stackEntry->_activeSplit = *split;
        stackEntry->_alternativeSplits.erase( split );

        inconsistent = !_engine->consistentBounds();

        if ( _engine->shouldProduceProofs() && inconsistent )
            _engine->explainSimplexFailure();
    }

    checkSkewFromDebuggingSolution();

    return true;
}

void SmtCore::resetSplitConditions()
{
    _constraintToViolationCount.clear();
    _numRejectedPhasePatternProposal = 0;
    _constraintForSplitting = NULL;
    _needToSplit = false;
}

void SmtCore::recordImpliedValidSplit( PiecewiseLinearCaseSplit &validSplit )
{
    if ( _stack.empty() )
        _impliedValidSplitsAtRoot.append( validSplit );
    else
        _stack.back()->_impliedValidSplits.append( validSplit );

    checkSkewFromDebuggingSolution();
}

void SmtCore::allSplitsSoFar( List<PiecewiseLinearCaseSplit> &result ) const
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

void SmtCore::setStatistics( Statistics *statistics )
{
    _statistics = statistics;
}

void SmtCore::storeDebuggingSolution( const Map<unsigned, double> &debuggingSolution )
{
    _debuggingSolution = debuggingSolution;
}

// Return true if stack is currently compliant, false otherwise
// If there is no stored solution, return false --- incompliant.
bool SmtCore::checkSkewFromDebuggingSolution()
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

bool SmtCore::splitAllowsStoredSolution( const PiecewiseLinearCaseSplit &split,
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

PiecewiseLinearConstraint *SmtCore::chooseViolatedConstraintForFixing(
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

void SmtCore::replaySmtStackEntry( SmtStackEntry *stackEntry )
{
    //    struct timespec start = TimeUtils::sampleMicro();

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

    //    if ( _statistics )
    //    {
    //        unsigned level = getStackDepth();
    //        _statistics->setUnsignedAttribute( Statistics::CURRENT_DECISION_LEVEL, level );
    //        if ( level > _statistics->getUnsignedAttribute( Statistics::MAX_DECISION_LEVEL ) )
    //            _statistics->setUnsignedAttribute( Statistics::MAX_DECISION_LEVEL, level );
    //        struct timespec end = TimeUtils::sampleMicro();
    //        _statistics->incLongAttribute( Statistics::TIME_SMT_CORE_CALLBACKS_MICRO,
    //                                       TimeUtils::timePassed( start, end ) );
    //    }
}

void SmtCore::storeSmtState( SmtState &smtState )
{
    smtState._impliedValidSplitsAtRoot = _impliedValidSplitsAtRoot;

    for ( auto &stackEntry : _stack )
        smtState._stack.append( stackEntry->duplicateSmtStackEntry() );

    smtState._stateId = _stateId;
}

bool SmtCore::pickSplitPLConstraint()
{
    if ( _needToSplit )
    {
        _constraintForSplitting = _engine->pickSplitPLConstraint( _branchingHeuristic );
    }
    return _constraintForSplitting != NULL;
}

void SmtCore::initBooleanAbstraction( PiecewiseLinearConstraint *plc )
{
    struct timespec start = TimeUtils::sampleMicro();

    plc->booleanAbstraction( _cadicalWrapper, _cadicalVarToPlc );

    if ( _statistics )
    {
        struct timespec end = TimeUtils::sampleMicro();
        _statistics->incLongAttribute( Statistics::TOTAL_TIME_SMT_CORE_MAIN_LOOP_MICRO,
                                       TimeUtils::timePassed( start, end ) );
    }
}

bool SmtCore::isLiteralAssigned( int literal ) const
{
    if ( _assignedLiterals.count( literal ) > 0 )
    {
        ASSERT( _cadicalVarToPlc.at( abs( literal ) )->phaseFixed() ||
                !_cadicalVarToPlc.at( abs( literal ) )->isActive() )
        return true;
    }

    return false;
}

void SmtCore::notify_assignment( int lit, bool is_fixed )
{
    if ( _exitCode != NOT_DONE )
        return;

    checkIfShouldExitDueToTimeout();
    SMT_LOG( Stringf( "Notified assignment %d; is decision: %d; is fixed: %d",
                      lit,
                      _cadicalWrapper.isDecision( lit ),
                      is_fixed )
                 .ascii() );

    struct timespec start = TimeUtils::sampleMicro();

    if ( isLiteralToBePropagated( -lit ) || isLiteralAssigned( -lit ) )
    {
        if ( _statistics )
        {
            struct timespec end = TimeUtils::sampleMicro();
            _statistics->incLongAttribute( Statistics::TIME_SMT_CORE_CALLBACKS_MICRO,
                                           TimeUtils::timePassed( start, end ) );
            _statistics->incLongAttribute( Statistics::TOTAL_TIME_SMT_CORE_NOTIFY_ASSIGNMENT_MICRO,
                                           TimeUtils::timePassed( start, end ) );
        }
        return;
    }

    // Allow notifying a negation of assigned literal only when a conflict is already discovered
    ASSERT( !isLiteralAssigned( -lit ) || cb_has_external_clause() );

    if ( is_fixed )
        _fixedCadicalVars.insert( lit );

    // TODO: notify_assignment may be called on already assigned literals (to notify they are
    //  fixed), maybe the following code should not be executed in this case

    // Pick the split to perform
    PiecewiseLinearConstraint *plc = _cadicalVarToPlc.at( FloatUtils::abs( lit ) );
    PhaseStatus originalPlcPhase = plc->getPhaseStatus();
    plc->propagateLitAsSplit( lit );

    _engine->applyPlcPhaseFixingTightenings( *plc );
    plc->setActiveConstraint( false );
    if ( !isLiteralAssigned( lit ) )
    {
        _assignedLiterals.insert( lit, _assignedLiterals.size() );
        for ( unsigned clause : _literalToClauses[lit] )
            if ( !isClauseSatisfied( clause ) )
                _satisfiedClauses.insert( clause );
    }

    ASSERT( originalPlcPhase == PHASE_NOT_FIXED || plc->getPhaseStatus() == originalPlcPhase );

    if ( _statistics )
    {
        struct timespec end = TimeUtils::sampleMicro();
        _statistics->incLongAttribute( Statistics::TIME_SMT_CORE_CALLBACKS_MICRO,
                                       TimeUtils::timePassed( start, end ) );
        _statistics->incLongAttribute( Statistics::TOTAL_TIME_SMT_CORE_NOTIFY_ASSIGNMENT_MICRO,
                                       TimeUtils::timePassed( start, end ) );
    }
}

void SmtCore::notify_new_decision_level()
{
    if ( _exitCode != NOT_DONE )
        return;

    checkIfShouldExitDueToTimeout();
    struct timespec start = TimeUtils::sampleMicro();
    SMT_LOG( "Notified new decision level" );

    _numRejectedPhasePatternProposal = 0;

    _needToSplit = false;
    if ( _constraintForSplitting && !_constraintForSplitting->isActive() )
    {
        if ( _branchingHeuristic == DivideStrategy::ReLUViolation )
            _constraintToViolationCount[_constraintForSplitting] = 0;
        _constraintForSplitting = NULL;
    }

    _engine->preContextPushHook();
    pushContext();


    if ( _statistics )
    {
        _statistics->incUnsignedAttribute( Statistics::NUM_SPLITS );

        unsigned level = _context.getLevel();
        _statistics->setUnsignedAttribute( Statistics::CURRENT_DECISION_LEVEL, level );
        if ( level > _statistics->getUnsignedAttribute( Statistics::MAX_DECISION_LEVEL ) )
            _statistics->setUnsignedAttribute( Statistics::MAX_DECISION_LEVEL, level );
        _statistics->incUnsignedAttribute( Statistics::NUM_DECISION_LEVELS );
        _statistics->incUnsignedAttribute( Statistics::SUM_DECISION_LEVELS, level );

        struct timespec end = TimeUtils::sampleMicro();
        _statistics->incLongAttribute( Statistics::TIME_SMT_CORE_CALLBACKS_MICRO,
                                       TimeUtils::timePassed( start, end ) );
        _statistics->incLongAttribute(
            Statistics::TOTAL_TIME_SMT_CORE_NOTIFY_NEW_DECISION_LEVEL_MICRO,
            TimeUtils::timePassed( start, end ) );
    }
}

void SmtCore::notify_backtrack( size_t new_level )
{
    if ( _exitCode != NOT_DONE )
        return;

    checkIfShouldExitDueToTimeout();
    struct timespec start = TimeUtils::sampleMicro();
    SMT_LOG( Stringf( "Backtracking to level %d", new_level ).ascii() );

    //    struct timespec start = TimeUtils::sampleMicro();
    unsigned oldLevel = _context.getLevel();

    popContextTo( new_level );
    _engine->postContextPopHook();

    // Maintain literals to propagate learned before the decision level
    List<Pair<int, int>> currentPropagations = _literalsToPropagate;
    _literalsToPropagate.clear();

    for ( const Pair<int, int> &propagation : currentPropagations )
        if ( propagation.second() <= (int)new_level )
            _literalsToPropagate.append( propagation );

    struct timespec end = TimeUtils::sampleMicro();

    for ( const auto &lit : _fixedCadicalVars )
        notify_assignment( lit, true );

    if ( _statistics )
    {
        unsigned jumpSize = oldLevel - new_level;

        _statistics->setUnsignedAttribute( Statistics::CURRENT_DECISION_LEVEL, new_level );
        _statistics->incUnsignedAttribute( Statistics::NUM_DECISION_LEVELS );
        _statistics->incUnsignedAttribute( Statistics::SUM_DECISION_LEVELS, new_level );

        _statistics->incUnsignedAttribute( Statistics::NUM_BACKJUMPS );
        _statistics->incUnsignedAttribute( Statistics::SUM_BACKJUMPS, jumpSize );
        if ( jumpSize > _statistics->getUnsignedAttribute( Statistics::MAX_BACKJUMP ) )
            _statistics->setUnsignedAttribute( Statistics::MAX_BACKJUMP, jumpSize );

        _statistics->incLongAttribute( Statistics::TIME_SMT_CORE_CALLBACKS_MICRO,
                                       TimeUtils::timePassed( start, end ) );
        _statistics->incLongAttribute( Statistics::TOTAL_TIME_SMT_CORE_NOTIFY_BACKTRACK_MICRO,
                                       TimeUtils::timePassed( start, end ) );
    }
}

bool SmtCore::cb_check_found_model( const std::vector<int> &model )
{
    if ( _exitCode != NOT_DONE )
        return false;

    checkIfShouldExitDueToTimeout();

    if ( _statistics )
    {
        _statistics->incUnsignedAttribute( Statistics::NUM_VISITED_TREE_STATES );
        //        printCurrentState();
    }
    SMT_LOG( "Checking model found by SAT solver" );
    ASSERT( _externalClausesToAdd.empty() );
    for ( const auto &lit : model )
        notify_assignment( lit, false );

    if ( _engine->getLpSolverType() == LPSolverType::NATIVE )
    {
        // Quickly try to notify constraints for bounds, which raises exception in case of
        // infeasibility
        if ( !_engine->propagateBoundManagerTightenings() )
            return false;

        // If external clause learned, no need to call solve
        if ( cb_has_external_clause() )
            return false;

        bool result = _engine->solve();
        // In cases where Marabou fails to provide a conflict clause, add the trivial possibility
        if ( !result && !cb_has_external_clause() )
            addTrivialConflictClause();

        SMT_LOG( Stringf( "\tResult is %u", result ).ascii() );
        return result && !cb_has_external_clause();
    }
    else
        return _engine->solve();
}

int SmtCore::cb_decide()
{
    if ( _exitCode != NOT_DONE )
        return 0;

    checkIfShouldExitDueToTimeout();
    struct timespec start = TimeUtils::sampleMicro();
    SMT_LOG( "Callback for decision:" );

    int literalToDecide = 0;

    if ( _branchingHeuristic == DivideStrategy::ReLUViolation )
    {
        Map<int, double> scores;
        for ( int literal : _literalToClauses.keys() )
        {
            ASSERT( literal != 0 );
            if ( isLiteralAssigned( literal ) || isLiteralAssigned( -literal ) )
                continue;

            // For stability
            if ( _cadicalVarToPlc[abs( literal )]->getLiteralForDecision() != literal )
                continue;

            double numOfClausesSatisfiedByLiteral = getVSIDSScore( literal );

            scores[literal] = numOfClausesSatisfiedByLiteral;
        }

        for ( PiecewiseLinearConstraint *plc : _constraintToViolationCount.keys() )
        {
            if ( plc->getPhaseStatus() != PHASE_NOT_FIXED )
                continue;

            int literal = plc->getLiteralForDecision();
            scores[literal] += _constraintToViolationCount[plc];
        }

        double maxScore = 0;

        for ( const auto &pair : scores )
        {
            int literal = pair.first;
            double score = pair.second;

            if ( score > maxScore )
            {
                literalToDecide = literal;
                maxScore = score;
            }
        }
    }
    else
    {
        NLR::NetworkLevelReasoner *networkLevelReasoner = _engine->getNetworkLevelReasoner();
        ASSERT( networkLevelReasoner );

        List<PiecewiseLinearConstraint *> constraints =
            networkLevelReasoner->getConstraintsInTopologicalOrder();

        Map<double, PiecewiseLinearConstraint *> polarityScoreToConstraint;
        for ( auto &plConstraint : constraints )
        {
            if ( plConstraint->supportPolarity() && plConstraint->isActive() &&
                 !plConstraint->phaseFixed() )
            {
                plConstraint->updateScoreBasedOnPolarity();
                polarityScoreToConstraint[plConstraint->getScore()] = plConstraint;
                if ( polarityScoreToConstraint.size() >=
                     GlobalConfiguration::POLARITY_CANDIDATES_THRESHOLD )
                    break;
            }
        }

        if ( !polarityScoreToConstraint.empty() )
        {
            double maxScore = 0;
            for ( double polarityScore : polarityScoreToConstraint.keys() )
            {
                int literal = polarityScoreToConstraint[polarityScore]->getLiteralForDecision();
                double score = ( getVSIDSScore( literal ) + 1 ) * polarityScore;
                if ( score > maxScore )
                {
                    literalToDecide = literal;
                    maxScore = score;
                }
            }
        }
    }

    if ( literalToDecide )
    {
        ASSERT( !isLiteralAssigned( -literalToDecide ) && !isLiteralAssigned( literalToDecide ) );
        ASSERT( FloatUtils::abs( literalToDecide ) <= _cadicalWrapper.vars() )
        SMT_LOG( Stringf( "Decided literal %d", literalToDecide ).ascii() );
        if ( _statistics )
            _statistics->incUnsignedAttribute( Statistics::NUM_MARABOU_DECISIONS );
        _constraintForSplitting = NULL;
    }
    else
    {
        SMT_LOG( "No decision made" );
        if ( _statistics )
            _statistics->incUnsignedAttribute( Statistics::NUM_SAT_SOLVER_DECISIONS );
    }

    if ( _statistics )
    {
        struct timespec end = TimeUtils::sampleMicro();
        _statistics->incLongAttribute( Statistics::TIME_SMT_CORE_CALLBACKS_MICRO,
                                       TimeUtils::timePassed( start, end ) );
        _statistics->incLongAttribute( Statistics::TOTAL_TIME_SMT_CORE_CB_DECIDE_MICRO,
                                       TimeUtils::timePassed( start, end ) );
    }

    return literalToDecide;
}

int SmtCore::cb_propagate()
{
    if ( _exitCode != NOT_DONE )
        return 0;

    checkIfShouldExitDueToTimeout();

    if ( _engine->getLpSolverType() == LPSolverType::GUROBI )
    {
        if ( _engine->solve() )
            _exitCode = SAT;

        return 0;
    }

    ASSERT( _engine->getLpSolverType() == LPSolverType::NATIVE );

    if ( _literalsToPropagate.empty() )
    {
        if ( _statistics )
        {
            _statistics->incUnsignedAttribute( Statistics::NUM_VISITED_TREE_STATES );
            //            printCurrentState();
        }

        // If no literals left to propagate, and no clause already found, attempt solving
        if ( _externalClausesToAdd.empty() )
            if ( _engine->solve() )
            {
                _exitCode = SAT;
                return 0;
            }

        // Try learning a conflict clause if possible
        if ( _externalClausesToAdd.empty() )
            _engine->propagateBoundManagerTightenings();

        // Add the zero literal at the end
        _literalsToPropagate.append( Pair<int, int>( 0, _context.getLevel() ) );
    }

    int lit = _literalsToPropagate.popFront().first();

    // In case of assigned boolean variable with opposite assignment, find a conflict clause and
    // terminate propagating
    if ( lit )
    {
        if ( isLiteralAssigned( -lit ) ) // TODO: currently not counted for smt core time, maybe add
        {
            if ( !cb_has_external_clause() )
                _engine->explainSimplexFailure();
            ASSERT( cb_has_external_clause() );
            _literalsToPropagate.clear();
            _literalsToPropagate.append( Pair<int, int>( 0, _context.getLevel() ) );
        }
    }

    SMT_LOG( Stringf( "Propagating literal %d", lit ).ascii() );
    ASSERT( FloatUtils::abs( lit ) <= _cadicalWrapper.vars() )
    return lit;
}

int SmtCore::cb_add_reason_clause_lit( int propagated_lit )
{
    ASSERT( _engine->getLpSolverType() == LPSolverType::NATIVE );
    if ( _exitCode != NOT_DONE )
        return 0;

    checkIfShouldExitDueToTimeout();
    struct timespec start = TimeUtils::sampleMicro();
    ASSERT( propagated_lit )
    ASSERT( !_cadicalWrapper.isDecision( propagated_lit ) );

    if ( !_isReasonClauseInitialized )
    {
        _reasonClauseLiterals.clear();
        if ( _numOfClauses == _vsidsDecayThreshold )
        {
            _numOfClauses = 0;
            _vsidsDecayThreshold = 512 * luby( ++_vsidsDecayCounter );
            _literalToClauses.clear();
        }
        SMT_LOG( Stringf( "Adding reason clause for literal %d", propagated_lit ).ascii() );

        if ( !_fixedCadicalVars.exists( propagated_lit ) )
        {
            Vector<int> toAdd = _engine->explainPhase( _cadicalVarToPlc[abs( propagated_lit )] );

            for ( int lit : toAdd )
            {
                // Make sure all clause literals were fixed before the literal to explain
                ASSERT( _cadicalVarToPlc[abs( propagated_lit )]->getPhaseFixingEntry()->id >
                        _cadicalVarToPlc[abs( lit )]->getPhaseFixingEntry()->id );

                // Remove fixed literals from clause, as they are redundant
                if ( !_fixedCadicalVars.exists( -lit ) )
                {
                    _reasonClauseLiterals.append( -lit );
                    _literalToClauses[-lit].insert( _numOfClauses );
                }
            }
        }

        ASSERT( !_reasonClauseLiterals.exists( -propagated_lit ) );
        _reasonClauseLiterals.append( propagated_lit );
        _literalToClauses[propagated_lit].insert( _numOfClauses );
        ++_numOfClauses;
        _isReasonClauseInitialized = true;

        // Unit clause fixes the propagated literal
        if ( _reasonClauseLiterals.size() == 1 )
            _fixedCadicalVars.insert( propagated_lit );
    }

    int lit = 0;
    if ( !_reasonClauseLiterals.empty() )
    {
        lit = _reasonClauseLiterals.pop();
        ASSERT( FloatUtils::abs( lit ) <= _cadicalWrapper.vars() )
        SMT_LOG( Stringf( "\tAdding Literal %d for Reason Clause", lit ).ascii() )
    }
    else
        _isReasonClauseInitialized = false;

    if ( _statistics )
    {
        struct timespec end = TimeUtils::sampleMicro();
        _statistics->incLongAttribute( Statistics::TIME_SMT_CORE_CALLBACKS_MICRO,
                                       TimeUtils::timePassed( start, end ) );
        _statistics->incLongAttribute(
            Statistics::TOTAL_TIME_SMT_CORE_CB_ADD_REASON_CLAUSE_LIT_MICRO,
            TimeUtils::timePassed( start, end ) );
    }

    return lit;
}

bool SmtCore::cb_has_external_clause()
{
    if ( _exitCode != NOT_DONE )
        return false;

    checkIfShouldExitDueToTimeout();
    SMT_LOG( Stringf( "Checking if there is a Conflict Clause to add: %d",
                      !_externalClausesToAdd.empty() )
                 .ascii() );
    return !_externalClausesToAdd.empty();
}

int SmtCore::cb_add_external_clause_lit()
{
    if ( _exitCode != NOT_DONE )
        return 0;

    checkIfShouldExitDueToTimeout();
    struct timespec start = TimeUtils::sampleMicro();

    ASSERT( !_externalClausesToAdd.empty() );

    // Add literal from the last conflict clause learned
    Vector<int> &currentClause = _externalClausesToAdd[_externalClausesToAdd.size() - 1];
    int lit = 0;
    if ( !currentClause.empty() )
    {
        lit = currentClause.pop();
        ASSERT( FloatUtils::abs( lit ) <= _cadicalWrapper.vars() )
        SMT_LOG( Stringf( "\tAdding Literal %d to Conflict Clause", lit ).ascii() )
    }
    else
        _externalClausesToAdd.pop();

    if ( _statistics )
    {
        struct timespec end = TimeUtils::sampleMicro();
        _statistics->incLongAttribute( Statistics::TIME_SMT_CORE_CALLBACKS_MICRO,
                                       TimeUtils::timePassed( start, end ) );
        _statistics->incLongAttribute(
            Statistics::TOTAL_TIME_SMT_CORE_CB_ADD_EXTERNAL_CLAUSE_LIT_MICRO,
            TimeUtils::timePassed( start, end ) );
    }

    return lit;
}

void SmtCore::addExternalClause( const Set<int> &clause )
{
    struct timespec start = TimeUtils::sampleMicro();

    ASSERT( !clause.exists( 0 ) )
    if ( _numOfClauses == _vsidsDecayThreshold )
    {
        _numOfClauses = 0;
        _vsidsDecayThreshold = 512 * luby( ++_vsidsDecayCounter );
        _literalToClauses.clear();
    }
    Vector<int> toAdd( 0 );

    // Remove fixed literals as they are redundant
    for ( int lit : clause )
        if ( !_fixedCadicalVars.exists( -lit ) )
        {
            toAdd.append( -lit );
            _literalToClauses[-lit].insert( _numOfClauses );
        }

    ++_numOfClauses;
    _externalClausesToAdd.append( toAdd );

    if ( _statistics )
    {
        struct timespec end = TimeUtils::sampleMicro();
        _statistics->incLongAttribute( Statistics::TOTAL_TIME_SMT_CORE_MAIN_LOOP_MICRO,
                                       TimeUtils::timePassed( start, end ) );
    }
}

const PiecewiseLinearConstraint *SmtCore::getConstraintFromLit( int lit ) const
{
    if ( _cadicalVarToPlc.exists( FloatUtils::abs( lit ) ) )
        return _cadicalVarToPlc.at( FloatUtils::abs( lit ) );
    return NULL;
}

bool SmtCore::solveWithCadical( double timeoutInSeconds )
{
    try
    {
        _timeoutInSeconds = timeoutInSeconds;

        // Maybe query detected as UNSAT in processInputQuery
        if ( _exitCode == UNSAT )
            return false;

        _cadicalWrapper.connectTheorySolver( this );
        _cadicalWrapper.connectTerminator( this );
        for ( unsigned var : _cadicalVarToPlc.keys() )
            if ( var != 0 )
                _cadicalWrapper.addObservedVar( var );

        _engine->preSolve();
        //        printCurrentState();
        if ( _engine->solve() )
        {
            _exitCode = SAT;
            return true;
        }
        if ( _engine->getLpSolverType() == LPSolverType::NATIVE )
            _engine->propagateBoundManagerTightenings();

        if ( !_externalClausesToAdd.empty() )
        {
            _exitCode = UNSAT;
            return false;
        }

        // Add the zero literal at the end
        if ( !_literalsToPropagate.empty() )
            _literalsToPropagate.append( Pair<int, int>( 0, _context.getLevel() ) );

        int result = _cadicalWrapper.solve();

        if ( _statistics && _engine->getVerbosity() )
        {
            printf( "\nSmtCore::Final statistics:\n" );
            _statistics->print();
        }


        if ( result == 0 )
        {
            if ( _exitCode != NOT_DONE )
                return true;

            _exitCode = UNKNOWN;
            return false;
        }
        else if ( result == 10 )
        {
            _exitCode = SAT;
            return true;
        }
        else if ( result == 20 )
        {
            _exitCode = UNSAT;
            return false;
        }
        else
        {
            ASSERT( false );
            return false;
        }
    }
    catch ( const TimeoutException & )
    {
        if ( _engine->getVerbosity() > 0 )
        {
            printf( "\n\nSmtCore: quitting due to timeout...\n\n" );
            printf( "Final statistics:\n" );
            _statistics->print();
        }

        _exitCode = TIMEOUT;
        _statistics->timeout();
        return false;
    }
    catch ( MarabouError &e )
    {
        String message = Stringf(
            "Caught a MarabouError. Code: %u. Message: %s ", e.getCode(), e.getUserMessage() );
        _exitCode = ExitCode::ERROR;
        _engine->exportInputQueryWithError( message );

        // TODO: uncomment after handling statistics
        //        mainLoopEnd = TimeUtils::sampleMicro();
        //        _statistics.incLongAttribute( Statistics::TIME_MAIN_LOOP_MICRO,
        //                                      TimeUtils::timePassed( mainLoopStart, mainLoopEnd )
        //                                      );
        return false;
    }
    catch ( ... )
    {
        _exitCode = ERROR;
        _engine->exportInputQueryWithError( "Unknown error" );

        // TODO: uncomment after handling statistics
        //        mainLoopEnd = TimeUtils::sampleMicro();
        //        _statistics.incLongAttribute( Statistics::TIME_MAIN_LOOP_MICRO,
        //                                      TimeUtils::timePassed( mainLoopStart, mainLoopEnd )
        //                                      );
        return false;
    }
}

void SmtCore::addLiteralToPropagate( int literal )
{
    struct timespec start = TimeUtils::sampleMicro();

    ASSERT( literal );
    if ( !isLiteralAssigned( literal ) && !isLiteralToBePropagated( literal ) )
    {
        ASSERT( !isLiteralAssigned( -literal ) && !isLiteralToBePropagated( -literal ) );
        _literalsToPropagate.append( Pair<int, int>( literal, _context.getLevel() ) );
    }

    if ( _statistics )
    {
        struct timespec end = TimeUtils::sampleMicro();
        _statistics->incLongAttribute( Statistics::TOTAL_TIME_SMT_CORE_MAIN_LOOP_MICRO,
                                       TimeUtils::timePassed( start, end ) );
    }
}

bool SmtCore::isLiteralToBePropagated( int literal ) const
{
    for ( const Pair<int, int> &pair : _literalsToPropagate )
        if ( pair.first() == literal )
            return true;

    return false;
}

Set<int> SmtCore::addTrivialConflictClause()
{
    struct timespec start = TimeUtils::sampleMicro();

    Set<int> clause = Set<int>();
    for ( const auto &pair : _assignedLiterals )
    {
        int lit = pair.first;
        if ( _cadicalWrapper.isDecision( lit ) && !_fixedCadicalVars.exists( lit ) )
            clause.insert( lit );
    }

    if ( _statistics )
    {
        struct timespec end = TimeUtils::sampleMicro();
        _statistics->incLongAttribute( Statistics::TOTAL_TIME_SMT_CORE_MAIN_LOOP_MICRO,
                                       TimeUtils::timePassed( start, end ) );
    }

    addExternalClause( clause );

    return clause;
}

void SmtCore::removeLiteralFromPropagations( int literal )
{
    _literalsToPropagate.erase( Pair<int, int>( literal, _context.getLevel() ) );
}

void SmtCore::phase( int literal )
{
    SMT_LOG( Stringf( "Phasing literal %d", literal ).ascii() );
    _cadicalWrapper.phase( literal );
    _literalsToPropagate.append( Pair<int, int>( literal, 0 ) );
    _fixedCadicalVars.insert( literal );
}

void SmtCore::checkIfShouldExitDueToTimeout()
{
    if ( _engine->shouldExitDueToTimeout( _timeoutInSeconds ) )
    {
        throw TimeoutException();
    }
}

void SmtCore::resetExitCode()
{
    _exitCode = NOT_DONE;
}

bool SmtCore::terminate()
{
    SMT_LOG( Stringf( "Callback for terminate: %d", _exitCode != NOT_DONE ).ascii() );
    return _exitCode != NOT_DONE;
}

unsigned SmtCore::getLiteralAssignmentIndex( int literal )
{
    struct timespec start = TimeUtils::sampleMicro();

    if ( _assignedLiterals.count( literal ) > 0 )
        return _assignedLiterals[literal].get();

    if ( _statistics )
    {
        struct timespec end = TimeUtils::sampleMicro();
        _statistics->incLongAttribute( Statistics::TOTAL_TIME_SMT_CORE_MAIN_LOOP_MICRO,
                                       TimeUtils::timePassed( start, end ) );
    }

    return _assignedLiterals.size();
}

bool SmtCore::isLiteralFixed( int literal ) const
{
    return _fixedCadicalVars.exists( literal );
}

void SmtCore::printCurrentState() const
{
    // TODO: remove this method
    unsigned size = _cadicalVarToPlc.size();
    std::cout << "State "
              << _statistics->getUnsignedAttribute( Statistics::NUM_VISITED_TREE_STATES )
              << " Time " << _statistics->getTotalTimeInMicro() << " : ";
    for ( unsigned v = size - 1; v > 0; --v )
    {
        if ( _cadicalVarToPlc[v]->getType() == RELU ||
             _cadicalVarToPlc[v]->getType() == LEAKY_RELU )
        {
            if ( _cadicalVarToPlc[v]->getPhaseStatus() == RELU_PHASE_ACTIVE )
                std::cout << size - v << " ";
            else if ( _cadicalVarToPlc[v]->getPhaseStatus() == RELU_PHASE_INACTIVE )
                std::cout << -(int)( size - v ) << " ";
        }
        else if ( _cadicalVarToPlc[v]->getType() == SIGN )
        {
            if ( _cadicalVarToPlc[v]->getPhaseStatus() == SIGN_PHASE_POSITIVE )
                std::cout << size - v << " ";
            else if ( _cadicalVarToPlc[v]->getPhaseStatus() == SIGN_PHASE_NEGATIVE )
                std::cout << -(int)( size - v ) << " ";
        }
        else if ( _cadicalVarToPlc[v]->getType() == ABSOLUTE_VALUE )
        {
            if ( _cadicalVarToPlc[v]->getPhaseStatus() == ABS_PHASE_POSITIVE )
                std::cout << size - v << " ";
            else if ( _cadicalVarToPlc[v]->getPhaseStatus() == ABS_PHASE_NEGATIVE )
                std::cout << -(int)( size - v ) << " ";
        }
    }
    std::cout << std::endl;
}

bool SmtCore::isClauseSatisfied( unsigned int clause ) const
{
    return _satisfiedClauses.contains( clause );
}

double SmtCore::getVSIDSScore( int literal ) const
{
    double numOfClausesSatisfiedByLiteral = 0;
    if ( _literalToClauses.exists( literal ) )
        for ( unsigned clause : _literalToClauses[literal] )
            if ( !isClauseSatisfied( clause ) )
                ++numOfClausesSatisfiedByLiteral;
    return numOfClausesSatisfiedByLiteral;
}

unsigned SmtCore::luby( unsigned int i )
{
    unsigned k;
    for ( k = 1; k < 32; ++k )
        if ( i == (unsigned)( ( 1 << k ) - 1 ) )
            return 1 << ( k - 1 );

    for ( k = 1;; ++k )
        if ( (unsigned)( 1 << ( k - 1 ) ) <= i && i < (unsigned)( ( 1 << k ) - 1 ) )
            return luby( i - ( 1 << ( k - 1 ) ) + 1 );
}
