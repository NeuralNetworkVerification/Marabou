/*********************                                                        */
/*! \file CDSmtCore.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz, Aleksandar Zeljic, Haoze Wu, Parth Shah
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2024 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** See the description of the class in CDSmtCore.h.
 **/

#include "CDSmtCore.h"

#include "Debug.h"
#include "DivideStrategy.h"
#include "EngineState.h"
#include "FloatUtils.h"
#include "GlobalConfiguration.h"
#include "IEngine.h"
#include "MStringf.h"
#include "MarabouError.h"
#include "PseudoImpactTracker.h"
#include "ReluConstraint.h"

using namespace CVC4::context;

CDSmtCore::CDSmtCore( IEngine *engine, Context &ctx )
    : _statistics( NULL )
    , _context( ctx )
    , _trail( &_context )
    , _decisions( &_context )
    , _engine( engine )
    , _needToSplit( false )
    , _constraintForSplitting( NULL )
    , _constraintViolationThreshold( Options::CONSTRAINT_VIOLATION_THRESHOLD )
    , _deepSoIRejectionThreshold( Options::get()->getInt( Options::DEEP_SOI_REJECTION_THRESHOLD ) )
    , _branchingHeuristic( Options::get()->getDivideStrategy() )
    , _scoreTracker( nullptr )
    , _numRejectedPhasePatternProposal( 0 )
{
}

CDSmtCore::~CDSmtCore()
{
}

void CDSmtCore::reportViolatedConstraint( PiecewiseLinearConstraint *constraint )
{
    ASSERT( !constraint->phaseFixed() );

    if ( !_constraintToViolationCount.exists( constraint ) )
        _constraintToViolationCount[constraint] = 0;

    ++_constraintToViolationCount[constraint];

    if ( _constraintToViolationCount[constraint] >= _constraintViolationThreshold )
    {
        _needToSplit = true;
        if ( GlobalConfiguration::SPLITTING_HEURISTICS == DivideStrategy::ReLUViolation ||
             !pickSplitPLConstraint() )
            // If pickSplitConstraint failed to pick one, use the native
            // relu-violation based splitting heuristic.
            _constraintForSplitting = constraint;
        ASSERT( !_constraintForSplitting->phaseFixed() );
    }
}

unsigned CDSmtCore::getViolationCounts( PiecewiseLinearConstraint *constraint ) const
{
    if ( !_constraintToViolationCount.exists( constraint ) )
        return 0;

    return _constraintToViolationCount[constraint];
}

void CDSmtCore::initializeScoreTrackerIfNeeded(
    const List<PiecewiseLinearConstraint *> &plConstraints )
{
    if ( GlobalConfiguration::USE_DEEPSOI_LOCAL_SEARCH )
    {
        _scoreTracker = std::unique_ptr<PseudoImpactTracker>( new PseudoImpactTracker() );
        _scoreTracker->initialize( plConstraints );

        SMT_LOG( "\tTracking Pseudo Impact..." );
    }
}

void CDSmtCore::reportRejectedPhasePatternProposal()
{
    ++_numRejectedPhasePatternProposal;

    if ( _numRejectedPhasePatternProposal >= _deepSoIRejectionThreshold )
    {
        _needToSplit = true;
        if ( !pickSplitPLConstraint() )
            // If pickSplitConstraint failed to pick one, use the native
            // relu-violation based splitting heuristic.
            _constraintForSplitting = _scoreTracker->topUnfixed();
    }
}

bool CDSmtCore::needToSplit() const
{
    return _needToSplit;
}

void CDSmtCore::pushDecision( PiecewiseLinearConstraint *constraint, PhaseStatus decision )
{
    SMT_LOG( Stringf( "Decision @ %d )", _context.getLevel() + 1 ).ascii() );
    TrailEntry te( constraint, decision );
    applyTrailEntry( te, true );
    SMT_LOG( Stringf( "Decision push @ %d DONE", _context.getLevel() ).ascii() );
}

void CDSmtCore::pushImplication( PiecewiseLinearConstraint *constraint )
{
    ASSERT( constraint->isImplication() );
    SMT_LOG( Stringf( "Implication @ %d ... ", _context.getLevel() ).ascii() );
    TrailEntry te( constraint, constraint->nextFeasibleCase() );
    applyTrailEntry( te, false );
    SMT_LOG( Stringf( "Implication @ %d DONE", _context.getLevel() ).ascii() );
}

void CDSmtCore::applyTrailEntry( TrailEntry &te, bool isDecision )
{
    if ( isDecision )
    {
        _context.push();
        _decisions.push_back( te );
    }

    _trail.push_back( te );
    _engine->applySplit( te.getPiecewiseLinearCaseSplit() );
}

void CDSmtCore::decide()
{
    ASSERT( _needToSplit );
    SMT_LOG( "Performing a ReLU split" );

    _numRejectedPhasePatternProposal = 0;
    // Maybe the constraint has already become inactive - if so, ignore
    // TODO: Ideally we will not ever reach this point
    // TODO: Maintain a vector of constraints above the threshold
    //       Iterate until we find an active one
    if ( !_constraintForSplitting->isActive() )
    {
        _needToSplit = false;
        _constraintToViolationCount[_constraintForSplitting] = 0;
        _constraintForSplitting = nullptr;
        return;
    }

    ASSERT( _constraintForSplitting->isActive() );
    _needToSplit = false;
    _constraintForSplitting->setActiveConstraint( false );

    decideSplit( _constraintForSplitting );
}

void CDSmtCore::decideSplit( PiecewiseLinearConstraint *constraint )
{
    struct timespec start = TimeUtils::sampleMicro();

    if ( _statistics )
    {
        _statistics->incUnsignedAttribute( Statistics::NUM_SPLITS );
        _statistics->incUnsignedAttribute( Statistics::NUM_VISITED_TREE_STATES );
    }

    if ( !constraint->isFeasible() )
        throw MarabouError( MarabouError::DEBUGGING_ERROR );
    ASSERT( constraint->isFeasible() );
    ASSERT( !constraint->isImplication() );

    PhaseStatus decision = constraint->nextFeasibleCase();
    pushDecision( constraint, decision );

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
    SMT_LOG( "Performing a ReLU split - DONE" );
}


unsigned CDSmtCore::getDecisionLevel() const
{
    return _decisions.size();
}

bool CDSmtCore::popDecisionLevel( TrailEntry &lastDecision )
{
    // ASSERT( static_cast<size_t>( _context.getLevel() ) == _decisions.size() );
    if ( _decisions.empty() )
        return false;

    SMT_LOG( "Popping trail ..." );
    lastDecision = _decisions.back();
    _context.pop();
    _engine->postContextPopHook();
    SMT_LOG( Stringf( "to %d DONE", _context.getLevel() ).ascii() );
    return true;
}

void CDSmtCore::interruptIfCompliantWithDebugSolution()
{
    if ( checkSkewFromDebuggingSolution() )
    {
        SMT_LOG( "Error! Popping from a compliant stack\n" );
        throw MarabouError( MarabouError::DEBUGGING_ERROR );
    }
}

PiecewiseLinearCaseSplit CDSmtCore::getDecision( unsigned decisionLevel ) const
{
    ASSERT( decisionLevel <= getDecisionLevel() );
    ASSERT( decisionLevel > 0 );
    return _decisions[decisionLevel - 1].getPiecewiseLinearCaseSplit();
}

bool CDSmtCore::backtrackToFeasibleDecision( TrailEntry &lastDecision )
{
    SMT_LOG( "Backtracking to a feasible decision..." );

    if ( getDecisionLevel() == 0 )
        return false;

    popDecisionLevel( lastDecision );
    lastDecision.markInfeasible();

    while ( !lastDecision.isFeasible() )
    {
        interruptIfCompliantWithDebugSolution();

        if ( !popDecisionLevel( lastDecision ) )
            return false;

        lastDecision.markInfeasible();
    }

    interruptIfCompliantWithDebugSolution();

    return true;
}

bool CDSmtCore::backtrackAndContinueSearch()
{
    TrailEntry feasibleDecision( nullptr, CONSTRAINT_INFEASIBLE );
    struct timespec start = TimeUtils::sampleMicro();

    if ( !backtrackToFeasibleDecision( feasibleDecision ) )
        return false;

    ASSERT( feasibleDecision.isFeasible() );

    if ( _statistics )
        _statistics->incUnsignedAttribute( Statistics::NUM_VISITED_TREE_STATES );

    PiecewiseLinearConstraint *pwlc = feasibleDecision._pwlConstraint;
    if ( pwlc->isImplication() )
        pushImplication( pwlc );
    else
        decideSplit( pwlc );

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

    checkSkewFromDebuggingSolution();
    return true;
}

void CDSmtCore::resetReportedViolations()
{
    _constraintToViolationCount.clear();
    _numRejectedPhasePatternProposal = 0;
    _needToSplit = false;
}

void CDSmtCore::allSplitsSoFar( List<PiecewiseLinearCaseSplit> &result ) const
{
    result.clear();

    for ( auto trailEntry : _trail )
        result.append( trailEntry.getPiecewiseLinearCaseSplit() );
}

void CDSmtCore::setStatistics( Statistics *statistics )
{
    _statistics = statistics;
}

void CDSmtCore::storeDebuggingSolution( const Map<unsigned, double> &debuggingSolution )
{
    _debuggingSolution = debuggingSolution;
}

// Return true if stack is currently compliant, false otherwise
// If there is no stored solution, return false --- incompliant.
bool CDSmtCore::checkSkewFromDebuggingSolution()
{
    if ( _debuggingSolution.empty() )
        return false;

    String error;

    int decisionLevel = 0;
    bool isDecision = false;
    // First check that the valid splits implied at the root level are okay
    for ( const auto &trailEntry : _trail )
    {
        if ( trailEntry._pwlConstraint != _decisions[decisionLevel]._pwlConstraint )
            isDecision = false;
        else
        {
            ASSERT( trailEntry._phase == _decisions[decisionLevel]._phase );
            isDecision = true;
            ++decisionLevel;
        }

        PiecewiseLinearCaseSplit caseSplit = trailEntry.getPiecewiseLinearCaseSplit();
        if ( decisionLevel == 0 )
        {
            if ( !splitAllowsStoredSolution( caseSplit, error ) )
            {
                printf( "Error with one of the splits implied at root level:\n\t%s\n",
                        error.ascii() );
                throw MarabouError( MarabouError::DEBUGGING_ERROR );
            }
        }
        else
        {
            // If the active split is non-compliant but there are alternatives,
            // i.e. it was a decision, that's fine
            if ( isDecision && !splitAllowsStoredSolution( caseSplit, error ) )
            {
                // Active split is non-compliant but this is fine, because there
                // are alternatives. We're done.
                return false;
            }
            else // Implied split
            {
                if ( !splitAllowsStoredSolution( caseSplit, error ) )
                {
                    printf( "Error with one of the splits implied at this stack level:\n\t%s\n",
                            error.ascii() );
                    throw MarabouError( MarabouError::DEBUGGING_ERROR );
                }
            }
        }
    }

    // No problems were detected, the stack is compliant with the stored solution
    return true;
}

bool CDSmtCore::splitAllowsStoredSolution( const PiecewiseLinearCaseSplit &split,
                                           String &error ) const
{
    // False if the split prevents one of the values in the stored solution, true otherwise.
    error = "";
    if ( _debuggingSolution.empty() )
        return true;

    for ( const auto bound : split.getBoundTightenings() )
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

void CDSmtCore::setConstraintViolationThreshold( unsigned threshold )
{
    _constraintViolationThreshold = threshold;
}

PiecewiseLinearConstraint *CDSmtCore::chooseViolatedConstraintForFixing(
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

bool CDSmtCore::pickSplitPLConstraint()
{
    if ( _needToSplit )
        _constraintForSplitting = _engine->pickSplitPLConstraint( _branchingHeuristic );
    return _constraintForSplitting != NULL;
}

void CDSmtCore::reset()
{
    _context.popto( 0 );
    _engine->postContextPopHook();
    _needToSplit = false;
    _constraintForSplitting = NULL;
    _constraintToViolationCount.clear();
    _numRejectedPhasePatternProposal = 0;
}
