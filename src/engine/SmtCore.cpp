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

#include "Debug.h"
#include "DivideStrategy.h"
#include "EngineState.h"
#include "FloatUtils.h"
#include "GlobalConfiguration.h"
#include "IEngine.h"
#include "InfeasibleQueryException.h"
#include "MStringf.h"
#include "MarabouError.h"
#include "Options.h"
#include "PseudoImpactTracker.h"
#include "ReluConstraint.h"
#include "UnsatCertificateNode.h"

SmtCore::SmtCore( IEngine *engine )
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
    , _cadicalWrapper()
    , _cadicalVarToPlc()
    , _literalsToPropagate()
    , _assignedLiterals( &_engine->getContext() )
    , _reasonClauseLiterals()
    , _isReasonClauseInitialized( false )
    , _fixedCadicalVars()
{
    _cadicalVarToPlc.insert( 0, NULL );
}

SmtCore::~SmtCore()
{
    freeMemory();

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
}

void SmtCore::reportViolatedConstraint( PiecewiseLinearConstraint *constraint )
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

    if ( _statistics )
    {
        unsigned level = getStackDepth();
        _statistics->setUnsignedAttribute( Statistics::CURRENT_DECISION_LEVEL, level );
        if ( level > _statistics->getUnsignedAttribute( Statistics::MAX_DECISION_LEVEL ) )
            _statistics->setUnsignedAttribute( Statistics::MAX_DECISION_LEVEL, level );
        struct timespec end = TimeUtils::sampleMicro();
        _statistics->incLongAttribute( Statistics::TOTAL_TIME_SMT_CORE_MICRO,
                                       TimeUtils::timePassed( start, end ) );
    }

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

    if ( _statistics )
    {
        unsigned level = getStackDepth();
        _statistics->setUnsignedAttribute( Statistics::CURRENT_DECISION_LEVEL, level );
        if ( level > _statistics->getUnsignedAttribute( Statistics::MAX_DECISION_LEVEL ) )
            _statistics->setUnsignedAttribute( Statistics::MAX_DECISION_LEVEL, level );
        struct timespec end = TimeUtils::sampleMicro();
        _statistics->incLongAttribute( Statistics::TOTAL_TIME_SMT_CORE_MICRO,
                                       TimeUtils::timePassed( start, end ) );
    }

    checkSkewFromDebuggingSolution();

    return true;
}

void SmtCore::resetSplitConditions()
{
    _constraintToViolationCount.clear();
    _numRejectedPhasePatternProposal = 0;
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
        _statistics->incLongAttribute( Statistics::TOTAL_TIME_SMT_CORE_MICRO,
                                       TimeUtils::timePassed( start, end ) );
    }
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
    plc->booleanAbstraction( _cadicalWrapper, _cadicalVarToPlc );
}

bool SmtCore::isLiteralAssigned( int literal ) const
{
    for ( int curLiteral : _assignedLiterals )
        if ( curLiteral == literal )
        {
            ASSERT( _cadicalVarToPlc.at( abs( literal ) )->phaseFixed() ||
                    !_cadicalVarToPlc.at( abs( literal ) )->isActive() )
            return true;
        }
    return false;
}

void SmtCore::notify_assignment( int lit, bool is_fixed )
{
    SMT_LOG( "Notified assignment %d", lit );
    ASSERT( !isLiteralAssigned( -lit ) );
    if ( is_fixed )
        _fixedCadicalVars.insert( lit );

    // Pick the split to perform
    PiecewiseLinearConstraint *plc = _cadicalVarToPlc.at( FloatUtils::abs( lit ) );
    PiecewiseLinearCaseSplit split = plc->propagateLitAsSplit( lit );

    // TODO perform within applySplit, and setPhaseFixing entry according to the first bound tightened
    // Apply split, and add the first bound of the split as a phase fixing ground bound entry,
    // if needed and split actually tightened a bound
    bool tightened = _engine->applySplit( split );
    if ( !plc->getPhaseFixingEntry() && tightened )
        plc->setPhaseFixingEntry(
            _engine->getGroundBoundEntry( split.getBoundTightenings().back()._variable,
                                          split.getBoundTightenings().back()._type ) );
    plc->setActiveConstraint( false );
    _assignedLiterals.push_back( lit );
}

void SmtCore::notify_new_decision_level()
{
    SMT_LOG( "Notified new decision level" );
    _numRejectedPhasePatternProposal = 0;

    struct timespec start = TimeUtils::sampleMicro();

    // TODO change name to num decisions. Is NUM_VISITED_TREE_STATES relevant?
    if ( _statistics )
    {
        _statistics->incUnsignedAttribute( Statistics::NUM_SPLITS );
        _statistics->incUnsignedAttribute( Statistics::NUM_VISITED_TREE_STATES );
    }

    _needToSplit = false;
    if ( _constraintForSplitting && !_constraintForSplitting->isActive() )
    {
        _constraintToViolationCount[_constraintForSplitting] = 0;
        _constraintForSplitting = NULL;
        return;
    }

    _engine->preContextPushHook();
    pushContext();

    for ( const auto &lit : _fixedCadicalVars )
        notify_assignment( lit, true );

    if ( _statistics )
    {
        unsigned level = _context.getLevel();
        _statistics->setUnsignedAttribute( Statistics::CURRENT_DECISION_LEVEL, level );
        if ( level > _statistics->getUnsignedAttribute( Statistics::MAX_DECISION_LEVEL ) )
            _statistics->setUnsignedAttribute( Statistics::MAX_DECISION_LEVEL, level );
        struct timespec end = TimeUtils::sampleMicro();
        _statistics->incLongAttribute( Statistics::TOTAL_TIME_SMT_CORE_MICRO,
                                       TimeUtils::timePassed( start, end ) );
    }
}

void SmtCore::notify_backtrack( size_t new_level )
{
    SMT_LOG( "Backtracking to level %d", new_level );
    struct timespec start = TimeUtils::sampleMicro();

    if ( _statistics )
        // A pop always sends us to a state that we haven't seen before - whether
        // from a sibling split, or from a lower level of the tree.
        _statistics->incUnsignedAttribute( Statistics::NUM_VISITED_TREE_STATES );

    popContextTo( new_level );
    _engine->postContextPopHook();

    // Maintain literals to propagate learned before the decision level
    List<Pair<int, int>> currentPropagations = _literalsToPropagate;
    _literalsToPropagate.clear();

    for ( const Pair<int, int> &propagation : currentPropagations )
        if ( propagation.second() <= (int)new_level )
            _literalsToPropagate.append( propagation );

    if ( _statistics )
    {
        unsigned level = _context.getLevel();
        _statistics->setUnsignedAttribute( Statistics::CURRENT_DECISION_LEVEL, level );
        if ( level > _statistics->getUnsignedAttribute( Statistics::MAX_DECISION_LEVEL ) )
            _statistics->setUnsignedAttribute( Statistics::MAX_DECISION_LEVEL, level );
        struct timespec end = TimeUtils::sampleMicro();
        _statistics->incLongAttribute( Statistics::TOTAL_TIME_SMT_CORE_MICRO,
                                       TimeUtils::timePassed( start, end ) );
    }
}

bool SmtCore::cb_check_found_model( const std::vector<int> &model )
{
    SMT_LOG( "Checking model found by SAT solver" );
    ASSERT( _externalClausesToAdd.empty() );
    for ( const auto &lit : model )
        notify_assignment( lit, false );

    // Quickly try to notify constraints for bounds, which raises exception in case of infeasibility
    try
    {
        _engine->propagateBoundManagerTightenings();
    }
    catch ( const InfeasibleQueryException & )
    {
        _engine->explainSimplexFailure();
    }

    // If external clause learned, no need to call solve
    if ( cb_has_external_clause() )
        return false;

    bool result = _engine->solve( 0 );
    // In cases where Marabou fails to provide a conflict clause, add the trivial possibility
    if ( !result && !cb_has_external_clause() )
        addTrivialConflictClause();

    return result && !cb_has_external_clause();
}

int SmtCore::cb_decide()
{
    SMT_LOG( "Callback for decision:" );
    // First, try to decide according to Marabou heuristics
    if ( pickSplitPLConstraint() )
    {
        _constraintForSplitting->setActiveConstraint( false );
        int lit = _constraintForSplitting->propagatePhaseAsLit();
        ASSERT( !isLiteralAssigned( -lit ) );
        if ( lit )
            _assignedLiterals.push_back( lit );
        _constraintForSplitting = NULL;
        ASSERT( FloatUtils::abs( lit ) <= _cadicalWrapper.vars() )
        SMT_LOG( "Decided literal %d", lit );
        return lit;
    }

    return 0;
}

int SmtCore::cb_propagate()
{
    SMT_LOG( "Propagating:" );
    if ( _literalsToPropagate.empty() )
    {
        // If no literals left to propagate, attempt solving
        _engine->solve( 0 );
        if ( _externalClausesToAdd.empty() )
            // Try learning a conflict clause if possible
            try
            {
                _engine->propagateBoundManagerTightenings();
            }
            catch ( const InfeasibleQueryException & )
            {
                _engine->explainSimplexFailure();
            }
        // Add the zero literal at the end
        _literalsToPropagate.append( Pair<int, int>( 0, _context.getLevel() ) );
    }

    int lit = _literalsToPropagate.popFront().first();
    if ( lit )
        _assignedLiterals.push_back( lit );
    ASSERT( FloatUtils::abs( lit ) <= _cadicalWrapper.vars() )
    SMT_LOG( "propagating literal %d", lit );
    return lit;
}

int SmtCore::cb_add_reason_clause_lit( int propagated_lit )
{
    SMT_LOG( "Adding reason clause for literal %d", propagated_lit );
    ASSERT( propagated_lit )
    ASSERT( !_cadicalWrapper.isDecision( propagated_lit ) );

    if ( !_isReasonClauseInitialized )
    {
        Vector<int> toAdd = _engine->explainPhase( _cadicalVarToPlc[abs( propagated_lit )] );

        for ( int lit : toAdd )
        {
            // Make sure all clause literals were fixed before the literal to explain
            ASSERT( _cadicalVarToPlc[abs( propagated_lit )]->getPhaseFixingEntry()->id >
                    _cadicalVarToPlc[abs( lit )]->getPhaseFixingEntry()->id );

            // Remove fixed literals from clause, as they are redundant
            if ( !_fixedCadicalVars.exists( lit ) )
                _reasonClauseLiterals.append( -lit );
        }

        ASSERT( !_reasonClauseLiterals.exists( -propagated_lit ) );
        ASSERT( !_fixedCadicalVars.exists( propagated_lit ) );
        _reasonClauseLiterals.append( propagated_lit );
        _isReasonClauseInitialized = true;
    }

    if ( _reasonClauseLiterals.empty() )
    {
        _isReasonClauseInitialized = false;
        return 0;
    }

    int lit = _reasonClauseLiterals.pop();
    ASSERT( FloatUtils::abs( lit ) <= _cadicalWrapper.vars() )
    return lit;
}

bool SmtCore::cb_has_external_clause()
{
    return !_externalClausesToAdd.empty();
}

int SmtCore::cb_add_external_clause_lit()
{
    ASSERT( !_externalClausesToAdd.empty() );

    // Add literal from the last conflict clause learned
    Vector<int> &currentClause = _externalClausesToAdd[_externalClausesToAdd.size() - 1];
    if ( !currentClause.empty() )
    {
        int lit = currentClause.pop();
        ASSERT( FloatUtils::abs( lit ) <= _cadicalWrapper.vars() )
        return lit;
    }

    _externalClausesToAdd.pop();
    return 0;
}

void SmtCore::addExternalClause( const Set<int> &clause )
{
    ASSERT( !clause.exists( 0 ) )
    Vector<int> toAdd( 0 );

    // Remove fixed literals as they are redundant
    for ( int lit : clause )
        if ( !_fixedCadicalVars.exists( lit ) )
            toAdd.append( -lit );
    _externalClausesToAdd.append( toAdd );
}

const PiecewiseLinearConstraint *SmtCore::getConstraintFromLit( int lit ) const
{
    if ( _cadicalVarToPlc.exists( FloatUtils::abs( lit ) ) )
        return _cadicalVarToPlc.at( FloatUtils::abs( lit ) );
    return NULL;
}

void SmtCore::solveWithCadical()
{
    _cadicalWrapper.connectTheorySolver( this );
    for ( unsigned var : _cadicalVarToPlc.keys() )
        if ( var != 0 )
            _cadicalWrapper.addObservedVar( var );

    _engine->preSolve();
    int result = _cadicalWrapper.solve();

    if ( result == 0 )
        _engine->setExitCode( IEngine::ExitCode::UNKNOWN );
    else if ( result == 10 )
        _engine->setExitCode( IEngine::ExitCode::SAT );
    else if ( result == 20 )
        _engine->setExitCode( IEngine::ExitCode::UNSAT );
    else
    {
        ASSERT( false );
    }
}

void SmtCore::addLiteralToPropagate( int literal )
{
    ASSERT( literal );
    if ( !isLiteralAssigned( literal ) && !isLiteralToBePropagated( literal ) )
    {
        ASSERT( !isLiteralAssigned( -literal ) && !isLiteralToBePropagated( -literal ) );
        _literalsToPropagate.append( Pair<int, int>( literal, _context.getLevel() ) );
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
    Set<int> clause = Set<int>();
    for ( int lit : _assignedLiterals )
        if ( _cadicalWrapper.isDecision( lit ) )
            clause.insert( lit );

    addExternalClause( clause );

    return clause;
}

void SmtCore::turnNeedToSplitOff()
{
    _needToSplit = false;
}

void SmtCore::removeLiteralFromPropagations( int literal )
{
    _literalsToPropagate.erase( Pair<int, int>( literal, _context.getLevel() ) );
}
