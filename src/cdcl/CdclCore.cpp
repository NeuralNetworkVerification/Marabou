/*********************                                                        */
/*! \file CdclCore.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Idan Refaeli, Omri Isac
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2024 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

**/

#include "CdclCore.h"

#include "NetworkLevelReasoner.h"
#include "Options.h"
#include "TimeUtils.h"
#include "TimeoutException.h"

CdclCore::CdclCore( IEngine *engine )
    : _engine( engine )
    , _context( _engine->getContext() )
    , _statistics( nullptr )
    , _satSolverWrapper( new CadicalWrapper( this, this, this ) )
    , _cadicalVarToPlc()
    , _literalsToPropagate()
    , _assignedLiterals( &_context )
    , _reasonClauseLiterals()
    , _isReasonClauseInitialized( false )
    , _fixedCadicalVars()
    , _timeoutInSeconds( 0 )
    , _numOfClauses( 0 )
    , _satisfiedClauses( &_context )
    , _literalToClauses()
    , _vsidsDecayThreshold( 0 )
    , _vsidsDecayCounter( 0 )
    , _restarts( 1 )
    , _restartLimit( 512 * luby( 1 ) )
    , _numOfConflictClauses( 0 )
    , _shouldRestart( false )
    , _tableauState( nullptr )
    , _initialClauses()
{
    _cadicalVarToPlc.insert( 0, NULL );
    _tableauState = std::make_shared<TableauState>();
}

CdclCore::~CdclCore()
{
    delete _satSolverWrapper;
}

void CdclCore::initBooleanAbstraction( PiecewiseLinearConstraint *plc )
{
    struct timespec start = TimeUtils::sampleMicro();

    plc->booleanAbstraction( _cadicalVarToPlc );

    if ( _statistics )
    {
        struct timespec end = TimeUtils::sampleMicro();
        _statistics->incLongAttribute( Statistics::TOTAL_TIME_CDCL_CORE_MAIN_LOOP_MICRO,
                                       TimeUtils::timePassed( start, end ) );
    }
}

bool CdclCore::isLiteralAssigned( int literal ) const
{
    if ( _assignedLiterals.count( literal ) > 0 )
    {
        ASSERT( _cadicalVarToPlc.at( abs( literal ) )->phaseFixed() ||
                !_cadicalVarToPlc.at( abs( literal ) )->isActive() )
        return true;
    }

    return false;
}

void CdclCore::notify_assignment( const std::vector<int> &lits )
{
    if ( _engine->getExitCode() != ExitCode::NOT_DONE )
        return;

    checkIfShouldExitDueToTimeout();

    //    if ( !_externalClauseToAdd.empty() )
    //    {
    //        SEARCH_TREE_LOG( "Skipping notification due to conflict clause" )
    //        return;
    //    }

    struct timespec start = TimeUtils::sampleMicro();

    CDCL_LOG( "Notifying assignments:" )

    for ( int lit : lits )
    {
        CDCL_LOG( Stringf( "\tNotified assignment %d; is decision: %d",
                           lit,
                           _satSolverWrapper->isDecision( lit ) )
                      .ascii() )

        if ( !isLiteralAssigned( lit ) )
            notifySingleAssignment( lit, false );
    }

    if ( _statistics )
    {
        struct timespec end = TimeUtils::sampleMicro();
        _statistics->incLongAttribute( Statistics::TOTAL_TIME_CDCL_CORE_CALLBACKS_MICRO,
                                       TimeUtils::timePassed( start, end ) );
        _statistics->incLongAttribute( Statistics::TOTAL_TIME_CDCL_CORE_NOTIFY_ASSIGNMENT_MICRO,
                                       TimeUtils::timePassed( start, end ) );
    }
}

void CdclCore::notify_new_decision_level()
{
    if ( _engine->getExitCode() != ExitCode::NOT_DONE )
        return;

    checkIfShouldExitDueToTimeout();
    struct timespec start = TimeUtils::sampleMicro();
    CDCL_LOG( "Notified new decision level" )

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
        _statistics->incLongAttribute( Statistics::TOTAL_TIME_CDCL_CORE_CALLBACKS_MICRO,
                                       TimeUtils::timePassed( start, end ) );
        _statistics->incLongAttribute(
            Statistics::TOTAL_TIME_CDCL_CORE_NOTIFY_NEW_DECISION_LEVEL_MICRO,
            TimeUtils::timePassed( start, end ) );
    }
}

void CdclCore::notify_backtrack( size_t new_level )
{
    if ( _engine->getExitCode() != ExitCode::NOT_DONE )
        return;

    checkIfShouldExitDueToTimeout();
    struct timespec start = TimeUtils::sampleMicro();
    CDCL_LOG( Stringf( "Backtracking to level %d", new_level ).ascii() )

    //    struct timespec start = TimeUtils::sampleMicro();
    unsigned oldLevel = _context.getLevel();

    if ( _shouldRestart )
    {
        if ( _statistics )
            _statistics->incUnsignedAttribute( Statistics::NUM_RESTARTS );

        _shouldRestart = false;
        _numOfConflictClauses = 0;
        _restartLimit = 512 * luby( ++_restarts );
        _engine->restoreInitialEngineState();
        _largestAssignmentSoFar.clear();
    }

    popContextTo( new_level );
    _engine->postContextPopHook();

    // Maintain literals to propagate learned before the decision level
    List<Pair<int, int>> currentPropagations = _literalsToPropagate;
    _literalsToPropagate.clear();

    for ( const Pair<int, int> &propagation : currentPropagations )
        if ( propagation.second() <= (int)new_level )
            _literalsToPropagate.append( propagation );

    for ( int lit : _fixedCadicalVars )
        if ( !isLiteralAssigned( lit ) )
            notifySingleAssignment( lit, true );

    struct timespec end = TimeUtils::sampleMicro();

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

        _statistics->incLongAttribute( Statistics::TOTAL_TIME_CDCL_CORE_CALLBACKS_MICRO,
                                       TimeUtils::timePassed( start, end ) );
        _statistics->incLongAttribute( Statistics::TOTAL_TIME_CDCL_CORE_NOTIFY_BACKTRACK_MICRO,
                                       TimeUtils::timePassed( start, end ) );
    }
}

bool CdclCore::cb_check_found_model( const std::vector<int> &model )
{
    if ( _engine->getExitCode() != ExitCode::NOT_DONE )
        return false;

    checkIfShouldExitDueToTimeout();

    if ( _statistics )
        _statistics->incUnsignedAttribute( Statistics::NUM_VISITED_TREE_STATES );
    CDCL_LOG( "Checking model found by SAT solver" )
    ASSERT( _externalClauseToAdd.empty() )
    notify_assignment( model );

    bool result;

    if ( _engine->getLpSolverType() == LPSolverType::NATIVE )
    {
        // Quickly try to notify constraints for bounds, which raises exception in case of
        // infeasibility
        if ( !_engine->propagateBoundManagerTightenings() )
            return false;

        // If external clause learned, no need to call solve
        if ( !_externalClauseToAdd.empty() )
            return false;

        result = _engine->solve( 0 );

        // In cases where Marabou fails to provide a conflict clause, add the trivial possibility
        if ( !result && _externalClauseToAdd.empty() )
            addTrivialConflictClause();

        CDCL_LOG( Stringf( "\tResult is %u", result ).ascii() )
        result = result && _externalClauseToAdd.empty();
    }
    else
        result = _engine->solve( 0 );

    return result;
}

int CdclCore::cb_decide()
{
    if ( _engine->getExitCode() != ExitCode::NOT_DONE )
        return 0;

    checkIfShouldExitDueToTimeout();
    struct timespec start = TimeUtils::sampleMicro();
    CDCL_LOG( "Callback for decision:" )

    if ( _shouldRestart )
    {
        _satSolverWrapper->forceBacktrack( 0 );
        return 0;
    }

    unsigned variableToDecide = 0;

    NLR::NetworkLevelReasoner *networkLevelReasoner = _engine->getNetworkLevelReasoner();
    ASSERT( networkLevelReasoner )

    List<PiecewiseLinearConstraint *> constraints =
        networkLevelReasoner->getConstraintsInTopologicalOrder();

    Map<double, PiecewiseLinearConstraint *> polarityScoreToConstraint;
    for ( auto &plConstraint : constraints )
    {
        if ( _largestAssignmentSoFar.exists( plConstraint->getVariableForDecision() ) )
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
            unsigned var = polarityScoreToConstraint[polarityScore]->getVariableForDecision();
            double score = ( getVariableVSIDSScore( var ) + 1 ) * polarityScore;
            if ( score > maxScore )
            {
                variableToDecide = var;
                maxScore = score;
            }
        }
    }

    int literalToDecide = 0;

    if ( variableToDecide )
        literalToDecide = _cadicalVarToPlc[variableToDecide]->getLiteralForDecision();

    if ( literalToDecide )
    {
        ASSERT( !isLiteralAssigned( -literalToDecide ) && !isLiteralAssigned( literalToDecide ) )
        ASSERT( FloatUtils::abs( literalToDecide ) <= _satSolverWrapper->vars() )
        CDCL_LOG( Stringf( "Decided literal %d", literalToDecide ).ascii() )
        if ( _statistics )
            _statistics->incUnsignedAttribute( Statistics::NUM_MARABOU_DECISIONS );
    }
    else
    {
        CDCL_LOG( "No decision made" )
        if ( _statistics )
            _statistics->incUnsignedAttribute( Statistics::NUM_SAT_SOLVER_DECISIONS );
    }

    if ( _statistics )
    {
        struct timespec end = TimeUtils::sampleMicro();
        _statistics->incLongAttribute( Statistics::TOTAL_TIME_CDCL_CORE_CALLBACKS_MICRO,
                                       TimeUtils::timePassed( start, end ) );
        _statistics->incLongAttribute( Statistics::TOTAL_TIME_CDCL_CORE_CB_DECIDE_MICRO,
                                       TimeUtils::timePassed( start, end ) );
    }

    return literalToDecide;
}

int CdclCore::cb_propagate()
{
    if ( _engine->getExitCode() != ExitCode::NOT_DONE )
        return 0;

    checkIfShouldExitDueToTimeout();

    if ( _engine->getLpSolverType() == LPSolverType::GUROBI )
    {
        if ( _engine->solve( 0 ) )
        {
            bool allInitialClausesSatisfied = true;
            for ( const Set<int> &clause : _initialClauses )
                if ( !_engine->checkAssignmentComplianceWithClause( clause ) )
                {
                    allInitialClausesSatisfied = false;
                    break;
                }

            if ( allInitialClausesSatisfied )
            {
                _engine->setExitCode( ExitCode::SAT );
                return 0;
            }
        }
    }

    ASSERT( _engine->getLpSolverType() == LPSolverType::NATIVE )

    if ( _literalsToPropagate.empty() )
    {
        if ( _statistics )
            _statistics->incUnsignedAttribute( Statistics::NUM_VISITED_TREE_STATES );

        // If no literals left to propagate, and no clause already found, attempt solving
        if ( _externalClauseToAdd.empty() )
        {
            if ( _engine->solve( 0 ) )
            {
                bool allInitialClausesSatisfied = true;
                for ( const Set<int> &clause : _initialClauses )
                    if ( !_engine->checkAssignmentComplianceWithClause( clause ) )
                    {
                        allInitialClausesSatisfied = false;
                        break;
                    }

                if ( allInitialClausesSatisfied )
                {
                    _engine->setExitCode( ExitCode::SAT );
                    return 0;
                }
            }
        }

        // Try learning a conflict clause if possible
        if ( _externalClauseToAdd.empty() )
        {
            _engine->propagateBoundManagerTightenings();
            if ( _externalClauseToAdd.empty() )
            {
                if ( _assignedLiterals.size() + _literalsToPropagate.size() >
                     _largestAssignmentSoFar.size() )
                {
                    _largestAssignmentSoFar.clear();

                    for ( const auto &p : _assignedLiterals )
                    {
                        int lit = p.first;
                        if ( lit > 0 )
                            _largestAssignmentSoFar[lit] = true;
                        else
                            _largestAssignmentSoFar[-lit] = false;
                    }

                    for ( const auto &p : _literalsToPropagate )
                    {
                        int lit = p.first();
                        if ( lit > 0 )
                            _largestAssignmentSoFar[lit] = true;
                        else
                            _largestAssignmentSoFar[-lit] = false;
                    }
                }
            }
        }

        // Add the zero literal at the end
        _literalsToPropagate.append( Pair<int, int>( 0, _context.getLevel() ) );
    }

    int lit = _literalsToPropagate.popFront().first();

    // In case of assigned boolean variable with opposite assignment, find a conflict clause and
    // terminate propagating
    if ( lit )
        if ( isLiteralAssigned( -lit ) ) // TODO: currently not counted for smt core time, maybe add
        {
            if ( _externalClauseToAdd.empty() )
                _engine->explainSimplexFailure();
            ASSERT( !_externalClauseToAdd.empty() )
            _literalsToPropagate.clear();
            _literalsToPropagate.append( Pair<int, int>( 0, _context.getLevel() ) );
        }

    CDCL_LOG( Stringf( "Propagating literal %d", lit ).ascii() )
    ASSERT( FloatUtils::abs( lit ) <= _satSolverWrapper->vars() )
    return lit;
}

int CdclCore::cb_add_reason_clause_lit( int propagated_lit )
{
    if ( _engine->getExitCode() != ExitCode::NOT_DONE )
    {
        return 0;
    }

    checkIfShouldExitDueToTimeout();

    ASSERT( _engine->getLpSolverType() == LPSolverType::NATIVE )
    struct timespec start = TimeUtils::sampleMicro();
    ASSERT( propagated_lit )
    ASSERT( !_satSolverWrapper->isDecision( propagated_lit ) )

    if ( !_isReasonClauseInitialized )
    {
        _reasonClauseLiterals.clear();
        if ( _numOfClauses == _vsidsDecayThreshold )
        {
            _numOfClauses = 0;
            _vsidsDecayThreshold = 512 * luby( ++_vsidsDecayCounter );
            _literalToClauses.clear();
        }

        CDCL_LOG( Stringf( "Adding reason clause for literal %d", propagated_lit ).ascii() )

        if ( !_fixedCadicalVars.exists( propagated_lit ) )
        {
            Vector<int> toAdd = _engine->explainPhase( _cadicalVarToPlc[abs( propagated_lit )] );

            for ( int lit : toAdd )
            {
                // Make sure all clause literals were fixed before the literal to explain
                ASSERT( _cadicalVarToPlc[abs( propagated_lit )]->getPhaseFixingEntry()->id >
                        _cadicalVarToPlc[abs( lit )]->getPhaseFixingEntry()->id )

                // Remove fixed literals from clause, as they are redundant
                if ( !_fixedCadicalVars.exists( -lit ) )
                {
                    _reasonClauseLiterals.append( -lit );
                    _literalToClauses[-lit].insert( _numOfClauses );
                }
            }
        }

        ASSERT( !_reasonClauseLiterals.exists( -propagated_lit ) )
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
        ASSERT( FloatUtils::abs( lit ) <= _satSolverWrapper->vars() )
        CDCL_LOG( Stringf( "\tAdding Literal %d for Reason Clause", lit ).ascii() )
    }
    else
        _isReasonClauseInitialized = false;

    if ( _statistics )
    {
        struct timespec end = TimeUtils::sampleMicro();
        _statistics->incLongAttribute( Statistics::TOTAL_TIME_CDCL_CORE_CALLBACKS_MICRO,
                                       TimeUtils::timePassed( start, end ) );
        _statistics->incLongAttribute(
            Statistics::TOTAL_TIME_CDCL_CORE_CB_ADD_REASON_CLAUSE_LIT_MICRO,
            TimeUtils::timePassed( start, end ) );
    }

    return lit;
}

bool CdclCore::cb_has_external_clause( bool & /*is_forgettable*/ )
{
    if ( _engine->getExitCode() != ExitCode::NOT_DONE )
        return false;

    checkIfShouldExitDueToTimeout();
    CDCL_LOG( Stringf( "Checking if there is a Conflict Clause to add: %d",
                       !_externalClauseToAdd.empty() )
                  .ascii() )
    return !_externalClauseToAdd.empty();
}

int CdclCore::cb_add_external_clause_lit()
{
    if ( _engine->getExitCode() != ExitCode::NOT_DONE )
        return 0;

    checkIfShouldExitDueToTimeout();
    struct timespec start = TimeUtils::sampleMicro();

    ASSERT( !_externalClauseToAdd.empty() )

    // Add literal from the last conflict clause learned
    int lit = _externalClauseToAdd.pop();
    ASSERT( FloatUtils::abs( lit ) <= _satSolverWrapper->vars() )
    CDCL_LOG( Stringf( "\tAdding Literal %d to Conflict Clause", lit ).ascii() )

    if ( _statistics )
    {
        struct timespec end = TimeUtils::sampleMicro();
        _statistics->incLongAttribute( Statistics::TOTAL_TIME_CDCL_CORE_CALLBACKS_MICRO,
                                       TimeUtils::timePassed( start, end ) );
        _statistics->incLongAttribute(
            Statistics::TOTAL_TIME_CDCL_CORE_CB_ADD_EXTERNAL_CLAUSE_LIT_MICRO,
            TimeUtils::timePassed( start, end ) );
    }

    return lit;
}

void CdclCore::addExternalClause( const Set<int> &clause )
{
    CDCL_LOG( "Add External Clause" )
    struct timespec start = TimeUtils::sampleMicro();

    ASSERT( !clause.exists( 0 ) )
    if ( _numOfClauses == _vsidsDecayThreshold )
    {
        _numOfClauses = 0;
        _vsidsDecayThreshold = 512 * luby( ++_vsidsDecayCounter );
        _literalToClauses.clear();
    }

    _externalClauseToAdd.append( 0 );

    // Remove fixed literals as they are redundant
    for ( int lit : clause )
    {
        _externalClauseToAdd.append( -lit );
        if ( !_fixedCadicalVars.exists( -lit ) )
            _literalToClauses[-lit].insert( _numOfClauses );
    }

    ++_numOfClauses;

    ++_numOfConflictClauses;
    if ( _numOfConflictClauses == _restartLimit )
        _shouldRestart = true;

    if ( _statistics )
    {
        struct timespec end = TimeUtils::sampleMicro();
        _statistics->incLongAttribute( Statistics::TOTAL_TIME_CDCL_CORE_MAIN_LOOP_MICRO,
                                       TimeUtils::timePassed( start, end ) );
    }
}

const PiecewiseLinearConstraint *CdclCore::getConstraintFromLit( int lit ) const
{
    if ( _cadicalVarToPlc.exists( (unsigned)FloatUtils::abs( lit ) ) )
        return _cadicalVarToPlc.at( (unsigned)FloatUtils::abs( lit ) );
    return nullptr;
}

bool CdclCore::solveWithCDCL( double timeoutInSeconds )
{
    try
    {
        _timeoutInSeconds = timeoutInSeconds;

        // Maybe query detected as UNSAT in processInputQuery
        if ( _engine->getExitCode() == ExitCode::UNSAT )
            return false;

        // TODO: Update the following fast-sat code to reflect the new changes of solving
        //        if ( Options::get()->getString( Options::NAP_EXTERNAL_CLAUSE_FILE_PATH ) == "" &&
        //             Options::get()->getString( Options::NAP_EXTERNAL_CLAUSE_FILE_PATH2 ) == "" )
        //            if ( _engine->solve() )
        //            {
        //                _exitCode = SAT;
        //                return true;
        //            }

        if ( _engine->getLpSolverType() == LPSolverType::NATIVE )
            _engine->propagateBoundManagerTightenings();

        // Add the zero literal at the end
        if ( !_literalsToPropagate.empty() )
            _literalsToPropagate.append( Pair<int, int>( 0, _context.getLevel() ) );

        if ( !_externalClauseToAdd.empty() )
        {
            _engine->setExitCode( ExitCode::UNSAT );
            return false;
        }

        for ( unsigned var : _cadicalVarToPlc.keys() )
            if ( var != 0 )
                _satSolverWrapper->addObservedVar( (int)var );

        Set<int> externalClause;

        // TODO: think if to include, and how to integrate code for NAP clauses in a clean way
        //        externalClause = _satSolverWrapper->addExternalNAPClause(
        //            Options::get()->getString( Options::NAP_EXTERNAL_CLAUSE_FILE_PATH ) );
        //        if ( !externalClause.empty() )
        //            _initialClauses.append( externalClause );
        //
        //        externalClause = _satSolverWrapper->addExternalNAPClause(
        //            Options::get()->getString( Options::NAP_EXTERNAL_CLAUSE_FILE_PATH2 ) );
        //        if ( !externalClause.empty() )
        //            _initialClauses.append( externalClause );

        int result = _satSolverWrapper->solve();

        if ( _statistics && _engine->getVerbosity() )
        {
            printf( "\nCdclCore::Final statistics:\n" );
            _statistics->print();
        }

        if ( result == 0 )
        {
            if ( _engine->getExitCode() == ExitCode::SAT )
                return true;
            else
                return false;
        }
        else if ( result == 10 )
        {
            _engine->setExitCode( ExitCode::SAT );
            return true;
        }
        else if ( result == 20 )
        {
            _engine->setExitCode( ExitCode::UNSAT );
            return false;
        }
        else
        {
            ASSERT( false )
        }
    }
    catch ( const TimeoutException & )
    {
        if ( _statistics )
        {
            if ( _engine->getVerbosity() > 0 )
            {
                printf( "\n\nCdclCore: quitting due to timeout...\n\n" );
                printf( "Final statistics:\n" );
                _statistics->print();
            }
            _statistics->timeout();
        }

        _engine->setExitCode( ExitCode::TIMEOUT );
        return false;
    }
}

void CdclCore::addLiteralToPropagate( int literal )
{
    struct timespec start = TimeUtils::sampleMicro();

    ASSERT( literal )
    if ( !isLiteralAssigned( literal ) && !isLiteralToBePropagated( literal ) )
    {
        ASSERT( !isLiteralAssigned( -literal ) && !isLiteralToBePropagated( -literal ) )
        _literalsToPropagate.append( Pair<int, int>( literal, _context.getLevel() ) );
    }

    if ( _statistics )
    {
        struct timespec end = TimeUtils::sampleMicro();
        _statistics->incLongAttribute( Statistics::TOTAL_TIME_CDCL_CORE_MAIN_LOOP_MICRO,
                                       TimeUtils::timePassed( start, end ) );
    }
}

bool CdclCore::isLiteralToBePropagated( int literal ) const
{
    for ( const Pair<int, int> &pair : _literalsToPropagate )
        if ( pair.first() == literal )
            return true;

    return false;
}

Set<int> CdclCore::addTrivialConflictClause()
{
    struct timespec start = TimeUtils::sampleMicro();

    Set<int> clause = Set<int>();
    for ( const auto &pair : _assignedLiterals )
    {
        int lit = pair.first;
        if ( _satSolverWrapper->isDecision( lit ) && !_fixedCadicalVars.exists( lit ) )
            clause.insert( lit );
    }

    if ( _statistics )
    {
        struct timespec end = TimeUtils::sampleMicro();
        _statistics->incLongAttribute( Statistics::TOTAL_TIME_CDCL_CORE_MAIN_LOOP_MICRO,
                                       TimeUtils::timePassed( start, end ) );
    }

    addExternalClause( clause );

    return clause;
}

void CdclCore::removeLiteralFromPropagations( int literal )
{
    _literalsToPropagate.erase( Pair<int, int>( literal, _context.getLevel() ) );
}

void CdclCore::phase( int literal )
{
    CDCL_LOG( Stringf( "Phasing literal %d", literal ).ascii() )
    _satSolverWrapper->phase( literal );
    _literalsToPropagate.append( Pair<int, int>( literal, 0 ) );
    _fixedCadicalVars.insert( literal );
}

void CdclCore::checkIfShouldExitDueToTimeout()
{
    if ( _engine->shouldExitDueToTimeout( _timeoutInSeconds ) )
    {
        throw TimeoutException();
    }
}

bool CdclCore::terminate()
{
    CDCL_LOG( Stringf( "Callback for terminate: %d", _engine->getExitCode() != ExitCode::NOT_DONE )
                  .ascii() )
    return _engine->getExitCode() != ExitCode::NOT_DONE;
}

unsigned CdclCore::getLiteralAssignmentIndex( int literal )
{
    struct timespec start = TimeUtils::sampleMicro();

    if ( _assignedLiterals.count( literal ) > 0 )
        return _assignedLiterals[literal].get();

    if ( _statistics )
    {
        struct timespec end = TimeUtils::sampleMicro();
        _statistics->incLongAttribute( Statistics::TOTAL_TIME_CDCL_CORE_MAIN_LOOP_MICRO,
                                       TimeUtils::timePassed( start, end ) );
    }

    return _assignedLiterals.size();
}

bool CdclCore::isLiteralFixed( int literal ) const
{
    return _fixedCadicalVars.exists( literal );
}

bool CdclCore::isClauseSatisfied( unsigned int clause ) const
{
    return _satisfiedClauses.contains( clause );
}

unsigned int CdclCore::getLiteralVSIDSScore( int literal ) const
{
    unsigned numOfClausesSatisfiedByLiteral = 0;
    if ( _literalToClauses.exists( literal ) )
        for ( unsigned clause : _literalToClauses[literal] )
            if ( !isClauseSatisfied( clause ) )
                ++numOfClausesSatisfiedByLiteral;
    return numOfClausesSatisfiedByLiteral;
}

unsigned int CdclCore::getVariableVSIDSScore( unsigned int var ) const
{
    return getLiteralVSIDSScore( (int)var ) + getLiteralVSIDSScore( -(int)var );
}

unsigned CdclCore::luby( unsigned int i )
{
    unsigned k;
    for ( k = 1; k < 32; ++k )
        if ( i == (unsigned)( ( 1 << k ) - 1 ) )
            return 1 << ( k - 1 );

    for ( k = 1;; ++k )
        if ( (unsigned)( 1 << ( k - 1 ) ) <= i && i < (unsigned)( ( 1 << k ) - 1 ) )
            return luby( i - ( 1 << ( k - 1 ) ) + 1 );
}

void CdclCore::notify_fixed_assignment( int lit )
{
    if ( _engine->getExitCode() != ExitCode::NOT_DONE )
        return;

    checkIfShouldExitDueToTimeout();
    struct timespec start = TimeUtils::sampleMicro();

    CDCL_LOG( Stringf( "Notified fixed assignment: %d", lit ).ascii() )
    if ( !isLiteralAssigned( lit ) )
        notifySingleAssignment( lit, true );
    else
        _fixedCadicalVars.insert( lit );

    if ( _statistics )
    {
        struct timespec end = TimeUtils::sampleMicro();
        _statistics->incLongAttribute( Statistics::TOTAL_TIME_CDCL_CORE_CALLBACKS_MICRO,
                                       TimeUtils::timePassed( start, end ) );
        _statistics->incLongAttribute(
            Statistics::TOTAL_TIME_CDCL_CORE_NOTIFY_FIXED_ASSIGNMENT_MICRO,
            TimeUtils::timePassed( start, end ) );
    }
}

bool CdclCore::hasConflictClause() const
{
    return !_externalClauseToAdd.empty();
}

void CdclCore::notifySingleAssignment( int lit, bool isFixed )
{
    if ( isLiteralToBePropagated( -lit ) || isLiteralAssigned( -lit ) )
        return;

    // Allow notifying a negation of assigned literal only when a conflict is already discovered
    ASSERT( !isLiteralAssigned( -lit ) || !_externalClauseToAdd.empty() )

    if ( isFixed )
        _fixedCadicalVars.insert( lit );

    // Pick the split to perform
    PiecewiseLinearConstraint *plc = _cadicalVarToPlc.at( (unsigned)FloatUtils::abs( lit ) );
    DEBUG( PhaseStatus originalPlcPhase = plc->getPhaseStatus() );

    plc->propagateLitAsSplit( lit );
    _engine->applyPlcPhaseFixingTightenings( *plc );
    plc->setActiveConstraint( false );

    ASSERT( !isLiteralAssigned( lit ) )

    _assignedLiterals.insert( lit, _assignedLiterals.size() );
    for ( unsigned clause : _literalToClauses[lit] )
        if ( !isClauseSatisfied( clause ) )
            _satisfiedClauses.insert( clause );

    ASSERT( originalPlcPhase == PHASE_NOT_FIXED || plc->getPhaseStatus() == originalPlcPhase )
}

void CdclCore::setStatistics( Statistics *statistics )
{
    _statistics = statistics;
}

void CdclCore::pushContext()
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

void CdclCore::popContextTo( unsigned int level )
{
    struct timespec start = TimeUtils::sampleMicro();
    unsigned int prevLevel = _context.getLevel();
    _context.popto( (int)level );
    struct timespec end = TimeUtils::sampleMicro();

    if ( _statistics )
    {
        _statistics->incUnsignedAttribute( Statistics::NUM_CONTEXT_POPS, prevLevel - level );
        _statistics->incLongAttribute( Statistics::TIME_CONTEXT_POP,
                                       TimeUtils::timePassed( start, end ) );
    }
}

void CdclCore::addLiteral( int lit )
{
    _satSolverWrapper->addLiteral( lit );
}
