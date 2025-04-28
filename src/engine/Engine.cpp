/*********************                                                        */
/*! \file Engine.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz, Duligur Ibeling, Andrew Wu, Omri Isac
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2024 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** \brief [[ Add one-line brief description here ]]
 **
 ** [[ Add lengthier description here ]]
 **/

#include "Engine.h"

#include "AutoConstraintMatrixAnalyzer.h"
#include "Debug.h"
#include "DisjunctionConstraint.h"
#include "EngineState.h"
#include "InfeasibleQueryException.h"
#include "MStringf.h"
#include "MalformedBasisException.h"
#include "MarabouError.h"
#include "NLRError.h"
#include "PiecewiseLinearConstraint.h"
#include "Preprocessor.h"
#include "Query.h"
#include "QuitFromPrecisionRestorationException.h"
#include "SearchTreeHandler.h"
#include "TableauRow.h"
#include "TimeUtils.h"
#include "VariableOutOfBoundDuringOptimizationException.h"
#include "Vector.h"

#include <random>

Engine::Engine()
    : _exitCode( ExitCode::NOT_DONE )
    , _context()
    , _boundManager( _context )
    , _tableau( _boundManager )
    , _preprocessedQuery( nullptr )
    , _rowBoundTightener( *_tableau )
    , _searchTreeHandler( this )
#ifdef BUILD_CADICAL
    , _cdclCore( this )
#endif
    , _numPlConstraintsDisabledByValidSplits( 0 )
    , _preprocessingEnabled( false )
    , _initialStateStored( false )
    , _work( NULL )
    , _basisRestorationRequired( Engine::RESTORATION_NOT_NEEDED )
    , _basisRestorationPerformed( Engine::NO_RESTORATION_PERFORMED )
    , _costFunctionManager( _tableau )
    , _quitRequested( false )
    , _numVisitedStatesAtPreviousRestoration( 0 )
    , _networkLevelReasoner( NULL )
    , _verbosity( Options::get()->getInt( Options::VERBOSITY ) )
    , _lastNumVisitedStates( 0 )
    , _lastIterationWithProgress( 0 )
    , _symbolicBoundTighteningType( Options::get()->getSymbolicBoundTighteningType() )
    , _solveWithMILP( Options::get()->getBool( Options::SOLVE_WITH_MILP ) )
    , _lpSolverType( Options::get()->getLPSolverType() )
    , _gurobi( nullptr )
    , _milpEncoder( nullptr )
    , _soiManager( nullptr )
    , _simulationSize( Options::get()->getInt( Options::NUMBER_OF_SIMULATIONS ) )
    , _isGurobyEnabled( Options::get()->gurobiEnabled() )
    , _performLpTighteningAfterSplit(
          Options::get()->getBool( Options::PERFORM_LP_TIGHTENING_AFTER_SPLIT ) )
    , _milpSolverBoundTighteningType( Options::get()->getMILPSolverBoundTighteningType() )
    , _sncMode( false )
    , _queryId( "" )
    , _produceUNSATProofs( Options::get()->getBool( Options::PRODUCE_PROOFS ) )
    , _groundBoundManager( _context )
    , _UNSATCertificate( NULL )
#ifdef BUILD_CADICAL
    , _solveWithCDCL( Options::get()->getBool( Options::SOLVE_WITH_CDCL ) )
#else
    , _solveWithCDCL( false )
#endif
    , _initialized( false )
{
    _searchTreeHandler.setStatistics( &_statistics );
#ifdef BUILD_CADICAL
    _cdclCore.setStatistics( &_statistics );
#endif
    _tableau->setStatistics( &_statistics );
    _rowBoundTightener->setStatistics( &_statistics );
    _preprocessor.setStatistics( &_statistics );

    _activeEntryStrategy = _projectedSteepestEdgeRule;
    _activeEntryStrategy->setStatistics( &_statistics );
    _statistics.stampStartingTime();
    setRandomSeed( Options::get()->getInt( Options::SEED ) );

    _boundManager.registerEngine( this );
    _statisticsPrintingFrequency = ( _lpSolverType == LPSolverType::NATIVE )
                                     ? GlobalConfiguration::STATISTICS_PRINTING_FREQUENCY
                                     : GlobalConfiguration::STATISTICS_PRINTING_FREQUENCY_GUROBI;

    _UNSATCertificateCurrentPointer =
        _produceUNSATProofs ? new ( true )
                                  CVC4::context::CDO<UnsatCertificateNode *>( &_context, NULL )
                            : NULL;
}

Engine::~Engine()
{
    if ( _work )
    {
        delete[] _work;
        _work = NULL;
    }

    if ( _UNSATCertificate )
    {
        delete _UNSATCertificate;
        _UNSATCertificate = NULL;
    }

    if ( _produceUNSATProofs && _UNSATCertificateCurrentPointer )
        _UNSATCertificateCurrentPointer->deleteSelf();
}

void Engine::setVerbosity( unsigned verbosity )
{
    _verbosity = verbosity;
}

void Engine::adjustWorkMemorySize()
{
    if ( _work )
    {
        delete[] _work;
        _work = NULL;
    }

    _work = new double[_tableau->getM()];
    if ( !_work )
        throw MarabouError( MarabouError::ALLOCATION_FAILED, "Engine::work" );
}

void Engine::applySnCSplit( PiecewiseLinearCaseSplit sncSplit, String queryId )
{
    _sncMode = true;
    _sncSplit = sncSplit;
    _queryId = queryId;
    preContextPushHook();
    _searchTreeHandler.pushContext();
    applySplit( sncSplit );
    _boundManager.propagateTightenings();
}

bool Engine::inSnCMode() const
{
    return _sncMode;
}

void Engine::setRandomSeed( unsigned seed )
{
    srand( seed );
}

Query Engine::prepareSnCQuery()
{
    List<Tightening> bounds = _sncSplit.getBoundTightenings();
    List<Equation> equations = _sncSplit.getEquations();

    Query sncIPQ = *_preprocessedQuery;
    for ( auto &equation : equations )
        sncIPQ.addEquation( equation );

    for ( auto &bound : bounds )
    {
        switch ( bound._type )
        {
        case Tightening::LB:
            sncIPQ.setLowerBound( bound._variable, bound._value );
            break;

        case Tightening::UB:
            sncIPQ.setUpperBound( bound._variable, bound._value );
        }
    }

    return sncIPQ;
}

void Engine::exportQueryWithError( String errorMessage )
{
    String ipqFileName = ( _queryId.length() > 0 ) ? _queryId + ".ipq" : "failedInputQuery.ipq";
    prepareSnCQuery().saveQuery( ipqFileName );
    printf( "Engine: %s!\nInput query has been saved as %s. Please attach the input query when you "
            "open the issue on GitHub.\n",
            errorMessage.ascii(),
            ipqFileName.ascii() );
}

void Engine::initializeSolver()
{
    SignalHandler::getInstance()->initialize();
    SignalHandler::getInstance()->registerClient( this );

    // Register the boundManager with all the PL constraints
    for ( auto &plConstraint : _plConstraints )
        plConstraint->registerBoundManager( &_boundManager );
    for ( auto &nlConstraint : _nlConstraints )
        nlConstraint->registerBoundManager( &_boundManager );

    // Before encoding, make sure all valid constraints are applied.
    applyAllValidConstraintCaseSplits();

    if ( _solveWithMILP )
        return;

#ifdef BUILD_CADICAL
    if ( _solveWithCDCL )
        for ( const auto plConstraint : _plConstraints )
            if ( plConstraint->phaseFixed() )
                _cdclCore.phase( plConstraint->propagatePhaseAsLit() );
#endif

    updateDirections();
    if ( _lpSolverType == LPSolverType::NATIVE )
        storeInitialEngineState();
    else if ( _lpSolverType == LPSolverType::GUROBI )
    {
        ENGINE_LOG( "Encoding convex relaxation into Gurobi..." );
        _gurobi = std::unique_ptr<GurobiWrapper>( new GurobiWrapper() );
        _tableau->setGurobi( &( *_gurobi ) );
        _milpEncoder = std::unique_ptr<MILPEncoder>( new MILPEncoder( *_tableau ) );
        _milpEncoder->setStatistics( &_statistics );
        _milpEncoder->encodeQuery( *_gurobi, *_preprocessedQuery, true );
        ENGINE_LOG( "Encoding convex relaxation into Gurobi - done" );
    }

    mainLoopStatistics();
    if ( _verbosity > 0 )
    {
        printf( "\nEngine::solve: Initial statistics\n" );
        _statistics.print();
        printf( "\n---\n" );
    }
}

bool Engine::solve( double timeoutInSeconds )
{
    if ( !_initialized )
    {
        initializeSolver();
        _initialized = true;
    }

    bool splitJustPerformed = true;
    struct timespec mainLoopStart = TimeUtils::sampleMicro();
    if ( _solveWithCDCL )
    {
        _searchTreeHandler.resetSplitConditions();
        applyAllBoundTightenings();

        if ( _lpSolverType == LPSolverType::NATIVE && !propagateBoundManagerTightenings() )
            return false;
    }

    while ( true )
    {
        struct timespec mainLoopEnd = TimeUtils::sampleMicro();
        _statistics.incLongAttribute( Statistics::TIME_MAIN_LOOP_MICRO,
                                      TimeUtils::timePassed( mainLoopStart, mainLoopEnd ) );
        mainLoopStart = mainLoopEnd;

        if ( shouldExitDueToTimeout( timeoutInSeconds ) )
        {
            if ( _verbosity > 0 )
            {
                printf( "\n\nEngine: quitting due to timeout...\n\n" );
                printf( "Final statistics:\n" );
                _statistics.print();
            }

            setExitCode( ExitCode::TIMEOUT );
            _statistics.timeout();
            return false;
        }

        if ( _quitRequested )
        {
            if ( _verbosity > 0 )
            {
                printf( "\n\nEngine: quitting due to external request...\n\n" );
                printf( "Final statistics:\n" );
                _statistics.print();
            }

            setExitCode( ExitCode::QUIT_REQUESTED );
            return false;
        }

        try
        {
            DEBUG( _tableau->verifyInvariants() );

            mainLoopStatistics();
            if ( _verbosity > 1 &&
                 _statistics.getLongAttribute( Statistics::NUM_MAIN_LOOP_ITERATIONS ) %
                         _statisticsPrintingFrequency ==
                     0 )
                _statistics.print();

            // If solving with CDCL, and _searchTreeHandler demands splitting, stop the loop before
            // performing other actions
            if ( _solveWithCDCL && _searchTreeHandler.needToSplit() )
            {
                if ( std::any_of(
                         _plConstraints.begin(),
                         _plConstraints.end(),
                         []( PiecewiseLinearConstraint *p ) { return !p->phaseFixed(); } ) )
                {
                    _boundManager.propagateTightenings();
                    return false;
                }
                else
                    _searchTreeHandler.setNeedToSplit( false );
            }

            if ( _lpSolverType == LPSolverType::NATIVE )
            {
                checkOverallProgress();
                // Check whether progress has been made recently

                if ( performPrecisionRestorationIfNeeded() )
                    continue;

                if ( _tableau->basisMatrixAvailable() )
                {
                    explicitBasisBoundTightening();
                    _boundManager.propagateTightenings();
                    applyAllValidConstraintCaseSplits();
                }
            }

            // If true, we just entered a new sub-problem
            if ( splitJustPerformed )
            {
                performBoundTighteningAfterCaseSplit();
                informLPSolverOfBounds();
                splitJustPerformed = false;
            }

            if ( !_solveWithCDCL && _searchTreeHandler.needToSplit() )
            {
                _searchTreeHandler.performSplit();
                splitJustPerformed = true;
                continue;
            }

            if ( !_tableau->allBoundsValid() )
            {
                // Some variable bounds are invalid, so the query is unsat
                throw InfeasibleQueryException();
            }

            if ( allVarsWithinBounds() )
            {
                // It's possible that a disjunction constraint is fixed and additional constraints
                // are introduced, making the linear portion unsatisfied. So we need to make sure
                // there are no valid case splits that we do not know of.
                applyAllBoundTightenings();
                if ( applyAllValidConstraintCaseSplits() )
                    continue;

                // The linear portion of the problem has been solved.
                // Check the status of the PL constraints
                bool solutionFound = adjustAssignmentToSatisfyNonLinearConstraints();
                if ( solutionFound )
                {
                    if ( allNonlinearConstraintsHold() )
                    {
                        DEBUG( for ( unsigned int v = 0; v < _tableau->getN(); ++v ) {
                            ASSERT( FloatUtils::areEqual( _tableau->getLowerBound( v ),
                                                          _boundManager.getLowerBound( v ) ) );

                            ASSERT( FloatUtils::areEqual( _tableau->getUpperBound( v ),
                                                          _boundManager.getUpperBound( v ) ) );
                        } );


                        mainLoopEnd = TimeUtils::sampleMicro();
                        _statistics.incLongAttribute(
                            Statistics::TIME_MAIN_LOOP_MICRO,
                            TimeUtils::timePassed( mainLoopStart, mainLoopEnd ) );
                        if ( _verbosity > 0 )
                        {
                            printf( "\nEngine::solve: sat assignment found\n" );
                            _statistics.print();
                        }

                        // Allows checking proofs produced for UNSAT leaves of satisfiable query
                        // search tree
                        if ( !_solveWithCDCL && _produceUNSATProofs )
                        {
                            ASSERT( _UNSATCertificateCurrentPointer );
                            ( **_UNSATCertificateCurrentPointer ).setSATSolutionFlag();
                        }
                        setExitCode( ExitCode::SAT );
                        return true;
                    }
                    else if ( !hasBranchingCandidate() )
                    {
                        mainLoopEnd = TimeUtils::sampleMicro();
                        _statistics.incLongAttribute(
                            Statistics::TIME_MAIN_LOOP_MICRO,
                            TimeUtils::timePassed( mainLoopStart, mainLoopEnd ) );
                        if ( _verbosity > 0 )
                        {
                            printf( "\nEngine::solve: at leaf node but solving inconclusive\n" );
                            _statistics.print();
                        }
                        setExitCode( ExitCode::UNKNOWN );
                        return false;
                    }
                    else
                    {
                        while ( !_searchTreeHandler.needToSplit() )
                            _searchTreeHandler.reportRejectedPhasePatternProposal();
                        continue;
                    }
                }
                else
                {
                    continue;
                }
            }

            // We have out-of-bounds variables.
            if ( _lpSolverType == LPSolverType::NATIVE )
                performSimplexStep();
            else
            {
                ENGINE_LOG( "Checking LP feasibility with Gurobi..." );
                DEBUG( { checkGurobiBoundConsistency(); } );
                ASSERT( _lpSolverType == LPSolverType::GUROBI );
                LinearExpression dontCare;
                minimizeCostWithGurobi( dontCare );
            }
            continue;
        }
        catch ( const MalformedBasisException & )
        {
            _tableau->toggleOptimization( false );
            if ( !handleMalformedBasisException() )
            {
                ASSERT( _lpSolverType == LPSolverType::NATIVE );
                setExitCode( ExitCode::ERROR );
                exportQueryWithError( "Cannot restore tableau" );
                mainLoopEnd = TimeUtils::sampleMicro();
                _statistics.incLongAttribute( Statistics::TIME_MAIN_LOOP_MICRO,
                                              TimeUtils::timePassed( mainLoopStart, mainLoopEnd ) );
                return false;
            }
        }
        catch ( const InfeasibleQueryException & )
        {
            _tableau->toggleOptimization( false );
            // The current query is unsat, and we need to pop.
            // If we're at level 0, the whole query is unsat.
            if ( _produceUNSATProofs )
            {
                if ( _lpSolverType == LPSolverType::NATIVE )
                    explainSimplexFailure();
#ifdef BUILD_CADICAL
                else
                    explainGurobiFailure();
#endif
            }

            if ( !_solveWithCDCL )
            {
                if ( !_searchTreeHandler.popSplit() )
                {
                    mainLoopEnd = TimeUtils::sampleMicro();
                    _statistics.incLongAttribute(
                        Statistics::TIME_MAIN_LOOP_MICRO,
                        TimeUtils::timePassed( mainLoopStart, mainLoopEnd ) );
                    if ( _verbosity > 0 )
                    {
                        printf( "\nEngine::solve: unsat query\n" );
                        _statistics.print();
                    }
                    setExitCode( ExitCode::UNSAT );
                    return false;
                }
                else
                {
                    splitJustPerformed = true;
                }
            }
            else
            {
#ifdef BUILD_CADICAL
                if ( !GlobalConfiguration::CDCL_USE_PROOF_BASED_CLAUSES )
                    _cdclCore.addDecisionBasedConflictClause();
#endif
                return false;
            }
        }
        catch ( const VariableOutOfBoundDuringOptimizationException & )
        {
            _tableau->toggleOptimization( false );
            continue;
        }
        catch ( const QuitFromPrecisionRestorationException & )
        {
            _tableau->toggleOptimization( false );
            return false;
        }
        catch ( MarabouError &e )
        {
            String message = Stringf(
                "Caught a MarabouError. Code: %u. Message: %s ", e.getCode(), e.getUserMessage() );
            setExitCode( ExitCode::ERROR );
            exportQueryWithError( message );
            mainLoopEnd = TimeUtils::sampleMicro();
            _statistics.incLongAttribute( Statistics::TIME_MAIN_LOOP_MICRO,
                                          TimeUtils::timePassed( mainLoopStart, mainLoopEnd ) );
            return false;
        }
        catch ( ... )
        {
            setExitCode( ExitCode::ERROR );
            exportQueryWithError( "Unknown error" );
            mainLoopEnd = TimeUtils::sampleMicro();
            _statistics.incLongAttribute( Statistics::TIME_MAIN_LOOP_MICRO,
                                          TimeUtils::timePassed( mainLoopStart, mainLoopEnd ) );
            return false;
        }
    }
}

void Engine::mainLoopStatistics()
{
    struct timespec start = TimeUtils::sampleMicro();

    unsigned activeConstraints = 0;
    for ( const auto &constraint : _plConstraints )
        if ( constraint->isActive() )
            ++activeConstraints;

    _statistics.setUnsignedAttribute( Statistics::NUM_ACTIVE_PL_CONSTRAINTS, activeConstraints );
    _statistics.setUnsignedAttribute( Statistics::NUM_PL_VALID_SPLITS,
                                      _numPlConstraintsDisabledByValidSplits );
    _statistics.setUnsignedAttribute( Statistics::NUM_PL_SEARCH_TREE_ORIGINATED_SPLITS,
                                      _plConstraints.size() - activeConstraints -
                                          _numPlConstraintsDisabledByValidSplits );

    _statistics.incLongAttribute( Statistics::NUM_MAIN_LOOP_ITERATIONS );

    struct timespec end = TimeUtils::sampleMicro();
    _statistics.incLongAttribute( Statistics::TOTAL_TIME_HANDLING_STATISTICS_MICRO,
                                  TimeUtils::timePassed( start, end ) );
}

void Engine::performBoundTighteningAfterCaseSplit()
{
    // Tighten bounds of a first hidden layer with MILP solver
    performMILPSolverBoundedTighteningForSingleLayer( 1 );
    do
    {
        performSymbolicBoundTightening();
    }
    while ( applyAllValidConstraintCaseSplits() );

    // Tighten bounds of an output layer with MILP solver
    if ( _networkLevelReasoner ) // to avoid failing of system test.
        performMILPSolverBoundedTighteningForSingleLayer(
            _networkLevelReasoner->getLayerIndexToLayer().size() - 1 );
}

bool Engine::adjustAssignmentToSatisfyNonLinearConstraints()
{
    ENGINE_LOG( "Linear constraints satisfied. Now trying to satisfy non-linear"
                " constraints..." );
    collectViolatedPlConstraints();

    // If all constraints are satisfied, we are possibly done
    if ( allPlConstraintsHold() )
    {
        if ( _lpSolverType == LPSolverType::NATIVE &&
             _tableau->getBasicAssignmentStatus() != ITableau::BASIC_ASSIGNMENT_JUST_COMPUTED )
        {
            if ( _verbosity > 0 )
            {
                printf( "Before declaring sat, recomputing...\n" );
            }
            // Make sure that the assignment is precise before declaring success
            _tableau->computeAssignment();
            // If we actually have a real satisfying assignment,
            return false;
        }
        else
            return true;
    }
    else if ( !GlobalConfiguration::USE_DEEPSOI_LOCAL_SEARCH )
    {
        // We have violated piecewise-linear constraints.
        performConstraintFixingStep();

        // Finally, take this opporunity to tighten any bounds
        // and perform any valid case splits.
        tightenBoundsOnConstraintMatrix();
        _boundManager.propagateTightenings();

        // For debugging purposes
        checkBoundCompliancyWithDebugSolution();

        while ( applyAllValidConstraintCaseSplits() )
            performSymbolicBoundTightening();
        return false;
    }
    else
    {
        return performDeepSoILocalSearch();
    }
}

bool Engine::performPrecisionRestorationIfNeeded()
{
    // If the basis has become malformed, we need to restore it
    if ( basisRestorationNeeded() )
    {
        if ( _basisRestorationRequired == Engine::STRONG_RESTORATION_NEEDED )
        {
            performPrecisionRestoration( PrecisionRestorer::RESTORE_BASICS );
            _basisRestorationPerformed = Engine::PERFORMED_STRONG_RESTORATION;
        }
        else
        {
            performPrecisionRestoration( PrecisionRestorer::DO_NOT_RESTORE_BASICS );
            _basisRestorationPerformed = Engine::PERFORMED_WEAK_RESTORATION;
        }

        _numVisitedStatesAtPreviousRestoration =
            _statistics.getUnsignedAttribute( Statistics::NUM_VISITED_TREE_STATES );
        _basisRestorationRequired = Engine::RESTORATION_NOT_NEEDED;
        return true;
    }

    // Restoration is not required
    _basisRestorationPerformed = Engine::NO_RESTORATION_PERFORMED;

    // Possible restoration due to precision degradation
    if ( shouldCheckDegradation() && highDegradation() )
    {
        performPrecisionRestoration( PrecisionRestorer::RESTORE_BASICS );
        return true;
    }

    return false;
}

bool Engine::handleMalformedBasisException()
{
    // Debug
    printf( "MalformedBasisException caught!\n" );
    //

    if ( _basisRestorationPerformed == Engine::NO_RESTORATION_PERFORMED )
    {
        if ( _numVisitedStatesAtPreviousRestoration !=
             _statistics.getUnsignedAttribute( Statistics::NUM_VISITED_TREE_STATES ) )
        {
            // We've tried a strong restoration before, and it didn't work. Do a weak restoration
            _basisRestorationRequired = Engine::WEAK_RESTORATION_NEEDED;
        }
        else
        {
            _basisRestorationRequired = Engine::STRONG_RESTORATION_NEEDED;
        }
        return true;
    }
    else if ( _basisRestorationPerformed == Engine::PERFORMED_STRONG_RESTORATION )
    {
        _basisRestorationRequired = Engine::WEAK_RESTORATION_NEEDED;
        return true;
    }
    else
        return false;
}

void Engine::performConstraintFixingStep()
{
    // Statistics
    _statistics.incLongAttribute( Statistics::NUM_CONSTRAINT_FIXING_STEPS );
    struct timespec start = TimeUtils::sampleMicro();

    // Select a violated constraint as the target
    selectViolatedPlConstraint();

    // Report the violated constraint to the Search Tree engine
    reportPlViolation();

    // Attempt to fix the constraint
    fixViolatedPlConstraintIfPossible();

    struct timespec end = TimeUtils::sampleMicro();
    _statistics.incLongAttribute( Statistics::TIME_CONSTRAINT_FIXING_STEPS_MICRO,
                                  TimeUtils::timePassed( start, end ) );
}

bool Engine::performSimplexStep()
{
    // Statistics
    _statistics.incLongAttribute( Statistics::NUM_SIMPLEX_STEPS );
    struct timespec start = TimeUtils::sampleMicro();

    /*
      In order to increase numerical stability, we attempt to pick a
      "good" entering/leaving combination, by trying to avoid tiny pivot
      values. We do this as follows:

      1. Pick an entering variable according to the strategy in use.
      2. Find the entailed leaving variable.
      3. If the combination is bad, go back to (1) and find the
         next-best entering variable.
    */

    if ( _tableau->isOptimizing() )
        _costFunctionManager->computeGivenCostFunction( _heuristicCost._addends );
    if ( _costFunctionManager->costFunctionInvalid() )
        _costFunctionManager->computeCoreCostFunction();
    else
        _costFunctionManager->adjustBasicCostAccuracy();

    DEBUG( {
        // Since we're performing a simplex step, there are out-of-bounds variables.
        // Therefore, if the cost function is fresh, it should not be zero.
        if ( _costFunctionManager->costFunctionJustComputed() )
        {
            const double *costFunction = _costFunctionManager->getCostFunction();
            unsigned size = _tableau->getN() - _tableau->getM();
            bool found = false;
            for ( unsigned i = 0; i < size; ++i )
            {
                if ( !FloatUtils::isZero( costFunction[i] ) )
                {
                    found = true;
                    break;
                }
            }

            if ( !found )
            {
                printf( "Error! Have OOB vars but cost function is zero.\n"
                        "Recomputing cost function. New one is:\n" );
                _costFunctionManager->computeCoreCostFunction();
                _costFunctionManager->dumpCostFunction();
                throw MarabouError( MarabouError::DEBUGGING_ERROR,
                                    "Have OOB vars but cost function is zero" );
            }
        }
    } );

    // Obtain all eligible entering variables
    List<unsigned> enteringVariableCandidates;
    _tableau->getEntryCandidates( enteringVariableCandidates );

    unsigned bestLeaving = 0;
    double bestChangeRatio = 0.0;
    Set<unsigned> excludedEnteringVariables;
    bool haveCandidate = false;
    unsigned bestEntering = 0;
    double bestPivotEntry = 0.0;
    unsigned tries = GlobalConfiguration::MAX_SIMPLEX_PIVOT_SEARCH_ITERATIONS;

    while ( tries > 0 )
    {
        --tries;

        // Attempt to pick the best entering variable from the available candidates
        if ( !_activeEntryStrategy->select(
                 _tableau, enteringVariableCandidates, excludedEnteringVariables ) )
        {
            // No additional candidates can be found.
            break;
        }

        // We have a candidate!
        haveCandidate = true;

        // We don't want to re-consider this candidate in future
        // iterations
        excludedEnteringVariables.insert( _tableau->getEnteringVariableIndex() );

        // Pick a leaving variable
        _tableau->computeChangeColumn();
        _tableau->pickLeavingVariable();

        // A fake pivot always wins
        if ( _tableau->performingFakePivot() )
        {
            bestEntering = _tableau->getEnteringVariableIndex();
            bestLeaving = _tableau->getLeavingVariableIndex();
            bestChangeRatio = _tableau->getChangeRatio();
            memcpy( _work, _tableau->getChangeColumn(), sizeof( double ) * _tableau->getM() );
            break;
        }

        // Is the newly found pivot better than the stored one?
        unsigned leavingIndex = _tableau->getLeavingVariableIndex();
        double pivotEntry = FloatUtils::abs( _tableau->getChangeColumn()[leavingIndex] );
        if ( pivotEntry > bestPivotEntry )
        {
            bestEntering = _tableau->getEnteringVariableIndex();
            bestPivotEntry = pivotEntry;
            bestLeaving = leavingIndex;
            bestChangeRatio = _tableau->getChangeRatio();
            memcpy( _work, _tableau->getChangeColumn(), sizeof( double ) * _tableau->getM() );
        }

        // If the pivot is greater than the sought-after threshold, we
        // are done.
        if ( bestPivotEntry >= GlobalConfiguration::ACCEPTABLE_SIMPLEX_PIVOT_THRESHOLD )
            break;
        else
            _statistics.incLongAttribute(
                Statistics::NUM_SIMPLEX_PIVOT_SELECTIONS_IGNORED_FOR_STABILITY );
    }

    // If we don't have any candidates, this simplex step has failed.
    if ( !haveCandidate )
    {
        if ( _tableau->getBasicAssignmentStatus() != ITableau::BASIC_ASSIGNMENT_JUST_COMPUTED )
        {
            // This failure might have resulted from a corrupt basic assignment.
            _tableau->computeAssignment();
            struct timespec end = TimeUtils::sampleMicro();
            _statistics.incLongAttribute( Statistics::TIME_SIMPLEX_STEPS_MICRO,
                                          TimeUtils::timePassed( start, end ) );
            return false;
        }
        else if ( !_costFunctionManager->costFunctionJustComputed() )
        {
            // This failure might have resulted from a corrupt cost function.
            ASSERT( _costFunctionManager->getCostFunctionStatus() ==
                    ICostFunctionManager::COST_FUNCTION_UPDATED );
            _costFunctionManager->invalidateCostFunction();
            struct timespec end = TimeUtils::sampleMicro();
            _statistics.incLongAttribute( Statistics::TIME_SIMPLEX_STEPS_MICRO,
                                          TimeUtils::timePassed( start, end ) );
            return false;
        }
        else
        {
            // Cost function is fresh --- failure is real.
            struct timespec end = TimeUtils::sampleMicro();
            _statistics.incLongAttribute( Statistics::TIME_SIMPLEX_STEPS_MICRO,
                                          TimeUtils::timePassed( start, end ) );
            if ( _tableau->isOptimizing() )
            {
                // The current solution is optimal.
                return true;
            }
            else
                throw InfeasibleQueryException();
        }
    }

    // Set the best choice in the tableau
    _tableau->setEnteringVariableIndex( bestEntering );
    _tableau->setLeavingVariableIndex( bestLeaving );
    _tableau->setChangeColumn( _work );
    _tableau->setChangeRatio( bestChangeRatio );

    bool fakePivot = _tableau->performingFakePivot();

    if ( !fakePivot && bestPivotEntry < GlobalConfiguration::ACCEPTABLE_SIMPLEX_PIVOT_THRESHOLD )
    {
        /*
          Despite our efforts, we are stuck with a small pivot. If basis factorization
          isn't fresh, refresh it and terminate this step - perhaps in the next iteration
          a better pivot will be found
        */
        if ( !_tableau->basisMatrixAvailable() )
        {
            _tableau->refreshBasisFactorization();
            return false;
        }

        _statistics.incLongAttribute( Statistics::NUM_SIMPLEX_UNSTABLE_PIVOTS );
    }

    if ( !fakePivot )
    {
        _tableau->computePivotRow();
        _rowBoundTightener->examinePivotRow();
    }

    // Perform the actual pivot
    _activeEntryStrategy->prePivotHook( _tableau, fakePivot );
    _tableau->performPivot();
    _activeEntryStrategy->postPivotHook( _tableau, fakePivot );
    _boundManager.propagateTightenings();
    _costFunctionManager->invalidateCostFunction();

    struct timespec end = TimeUtils::sampleMicro();
    _statistics.incLongAttribute( Statistics::TIME_SIMPLEX_STEPS_MICRO,
                                  TimeUtils::timePassed( start, end ) );
    return false;
}

void Engine::fixViolatedPlConstraintIfPossible()
{
    List<PiecewiseLinearConstraint::Fix> fixes;

    if ( GlobalConfiguration::USE_SMART_FIX )
        fixes = _plConstraintToFix->getSmartFixes( _tableau );
    else
        fixes = _plConstraintToFix->getPossibleFixes();

    // First, see if we can fix without pivoting. We are looking for a fix concerning a
    // non-basic variable, that doesn't set that variable out-of-bounds.
    for ( const auto &fix : fixes )
    {
        if ( !_tableau->isBasic( fix._variable ) )
        {
            if ( _tableau->checkValueWithinBounds( fix._variable, fix._value ) )
            {
                _tableau->setNonBasicAssignment( fix._variable, fix._value, true );
                return;
            }
        }
    }

    // No choice, have to pivot. Look for a fix concerning a basic variable, that
    // doesn't set that variable out-of-bounds. If smart-fix is enabled and implemented,
    // we should probably not reach this point.
    bool found = false;
    auto it = fixes.begin();
    while ( !found && it != fixes.end() )
    {
        if ( _tableau->isBasic( it->_variable ) )
        {
            if ( _tableau->checkValueWithinBounds( it->_variable, it->_value ) )
            {
                found = true;
            }
        }
        if ( !found )
        {
            ++it;
        }
    }

    // If we couldn't find an eligible fix, give up
    if ( !found )
        return;

    PiecewiseLinearConstraint::Fix fix = *it;
    ASSERT( _tableau->isBasic( fix._variable ) );

    TableauRow row( _tableau->getN() - _tableau->getM() );
    _tableau->getTableauRow( _tableau->variableToIndex( fix._variable ), &row );

    // Pick the variable with the largest coefficient in this row for pivoting,
    // to increase numerical stability.
    unsigned bestCandidate = row._row[0]._var;
    double bestValue = FloatUtils::abs( row._row[0]._coefficient );

    unsigned n = _tableau->getN();
    unsigned m = _tableau->getM();
    for ( unsigned i = 1; i < n - m; ++i )
    {
        double contenderValue = FloatUtils::abs( row._row[i]._coefficient );
        if ( FloatUtils::gt( contenderValue, bestValue ) )
        {
            bestValue = contenderValue;
            bestCandidate = row._row[i]._var;
        }
    }

    if ( FloatUtils::isZero( bestValue ) )
    {
        // This can happen, e.g., if we have an equation x = 5, and is legal behavior.
        return;
    }

    // Switch between nonBasic and the variable we need to fix
    _tableau->setEnteringVariableIndex( _tableau->variableToIndex( bestCandidate ) );
    _tableau->setLeavingVariableIndex( _tableau->variableToIndex( fix._variable ) );

    // Make sure the change column and pivot row are up-to-date - strategies
    // such as projected steepest edge need these for their internal updates.
    _tableau->computeChangeColumn();
    _tableau->computePivotRow();

    _activeEntryStrategy->prePivotHook( _tableau, false );
    _tableau->performDegeneratePivot();
    _activeEntryStrategy->postPivotHook( _tableau, false );

    ASSERT( !_tableau->isBasic( fix._variable ) );
    _tableau->setNonBasicAssignment( fix._variable, fix._value, true );
}

bool Engine::processInputQuery( const IQuery &inputQuery )
{
    return processInputQuery( inputQuery, GlobalConfiguration::PREPROCESS_INPUT_QUERY );
}

bool Engine::calculateBounds( const IQuery &inputQuery )
{
    ENGINE_LOG( "calculateBounds starting\n" );
    struct timespec start = TimeUtils::sampleMicro();

    try
    {
        invokePreprocessor( inputQuery, true );
        if ( _verbosity > 1 )
            printInputBounds( inputQuery );

        initializeNetworkLevelReasoning();

        performSymbolicBoundTightening( &( *_preprocessedQuery ) );
        performSimulation();
        performMILPSolverBoundedTightening( &( *_preprocessedQuery ) );
        performAdditionalBackwardAnalysisIfNeeded();

        if ( _networkLevelReasoner && Options::get()->getBool( Options::DUMP_BOUNDS ) )
            _networkLevelReasoner->dumpBounds();

        struct timespec end = TimeUtils::sampleMicro();
        _statistics.setLongAttribute( Statistics::CALCULATE_BOUNDS_TIME_MICRO,
                                      TimeUtils::timePassed( start, end ) );
        if ( !_tableau->allBoundsValid() )
        {
            // Some variable bounds are invalid, so the query is unsat
            throw InfeasibleQueryException();
        }
    }
    catch ( const InfeasibleQueryException & )
    {
        ENGINE_LOG( "calculateBounds done\n" );

        struct timespec end = TimeUtils::sampleMicro();
        _statistics.setLongAttribute( Statistics::CALCULATE_BOUNDS_TIME_MICRO,
                                      TimeUtils::timePassed( start, end ) );

        printf( "unsat\n" );

        _exitCode = ExitCode::UNSAT;
        return false;
    }

    ENGINE_LOG( "calculateBounds done\n" );

    return true;
}

void Engine::invokePreprocessor( const IQuery &inputQuery, bool preprocess )
{
    if ( _verbosity > 0 )
        printf( "Engine::processInputQuery: Input query (before preprocessing): "
                "%u equations, %u variables\n",
                inputQuery.getNumberOfEquations(),
                inputQuery.getNumberOfVariables() );

    // If processing is enabled, invoke the preprocessor
    _preprocessingEnabled = preprocess;
    if ( _preprocessingEnabled )
        _preprocessedQuery = _preprocessor.preprocess(
            inputQuery, GlobalConfiguration::PREPROCESSOR_ELIMINATE_VARIABLES );
    else
    {
        _preprocessedQuery = std::unique_ptr<Query>( inputQuery.generateQuery() );
        Preprocessor().informConstraintsOfInitialBounds( *_preprocessedQuery );
    }

    if ( _verbosity > 0 )
        printf( "Engine::processInputQuery: Input query (after preprocessing): "
                "%u equations, %u variables\n\n",
                _preprocessedQuery->getNumberOfEquations(),
                _preprocessedQuery->getNumberOfVariables() );

    unsigned infiniteBounds = _preprocessedQuery->countInfiniteBounds();
    if ( infiniteBounds != 0 )
    {
        _exitCode = ExitCode::ERROR;
        throw MarabouError( MarabouError::UNBOUNDED_VARIABLES_NOT_YET_SUPPORTED,
                            Stringf( "Error! Have %u infinite bounds", infiniteBounds ).ascii() );
    }
}

void Engine::printInputBounds( const IQuery &inputQuery ) const
{
    printf( "Input bounds:\n" );
    for ( unsigned i = 0; i < inputQuery.getNumInputVariables(); ++i )
    {
        unsigned variable = inputQuery.inputVariableByIndex( i );
        double lb, ub;
        bool fixed = false;
        if ( _preprocessingEnabled )
        {
            // Fixed variables are easy: return the value they've been fixed to.
            if ( _preprocessor.variableIsFixed( variable ) )
            {
                fixed = true;
                lb = _preprocessor.getFixedValue( variable );
                ub = lb;
            }
            else
            {
                // Has the variable been merged into another?
                while ( _preprocessor.variableIsMerged( variable ) )
                    variable = _preprocessor.getMergedIndex( variable );

                // We know which variable to look for, but it may have been assigned
                // a new index, due to variable elimination
                variable = _preprocessor.getNewIndex( variable );

                lb = _preprocessedQuery->getLowerBound( variable );
                ub = _preprocessedQuery->getUpperBound( variable );
            }
        }
        else
        {
            lb = inputQuery.getLowerBound( variable );
            ub = inputQuery.getUpperBound( variable );
        }

        printf( "\tx%u: [%8.4lf, %8.4lf] %s\n", i, lb, ub, fixed ? "[FIXED]" : "" );
    }
    printf( "\n" );
}

void Engine::storeEquationsInDegradationChecker()
{
    _degradationChecker.storeEquations( *_preprocessedQuery );
}

double *Engine::createConstraintMatrix()
{
    const List<Equation> &equations( _preprocessedQuery->getEquations() );
    unsigned m = equations.size();
    unsigned n = _preprocessedQuery->getNumberOfVariables();

    // Step 1: create a constraint matrix from the equations
    double *constraintMatrix = new double[n * m];
    if ( !constraintMatrix )
        throw MarabouError( MarabouError::ALLOCATION_FAILED, "Engine::constraintMatrix" );
    std::fill_n( constraintMatrix, n * m, 0.0 );

    unsigned equationIndex = 0;
    for ( const auto &equation : equations )
    {
        if ( equation._type != Equation::EQ )
        {
            _exitCode = ExitCode::ERROR;
            throw MarabouError( MarabouError::NON_EQUALITY_INPUT_EQUATION_DISCOVERED );
        }

        for ( const auto &addend : equation._addends )
            constraintMatrix[equationIndex * n + addend._variable] = addend._coefficient;

        ++equationIndex;
    }

    return constraintMatrix;
}

void Engine::removeRedundantEquations( const double *constraintMatrix )
{
    const List<Equation> &equations( _preprocessedQuery->getEquations() );
    unsigned m = equations.size();
    unsigned n = _preprocessedQuery->getNumberOfVariables();

    // Step 1: analyze the matrix to identify redundant rows
    AutoConstraintMatrixAnalyzer analyzer;
    analyzer->analyze( constraintMatrix, m, n );

    ENGINE_LOG(
        Stringf( "Number of redundant rows: %u out of %u", analyzer->getRedundantRows().size(), m )
            .ascii() );

    // Step 2: remove any equations corresponding to redundant rows
    Set<unsigned> redundantRows = analyzer->getRedundantRows();

    if ( !redundantRows.empty() )
    {
        _preprocessedQuery->removeEquationsByIndex( redundantRows );
        m = equations.size();
    }
}

void Engine::selectInitialVariablesForBasis( const double *constraintMatrix,
                                             List<unsigned> &initialBasis,
                                             List<unsigned> &basicRows )
{
    /*
      This method permutes rows and columns in the constraint matrix (prior
      to the addition of auxiliary variables), in order to obtain a set of
      column that constitute a lower triangular matrix. The variables
      corresponding to the columns of this matrix join the initial basis.

      (It is possible that not enough variables are obtained this way, in which
      case the initial basis will have to be augmented later).
    */

    const List<Equation> &equations( _preprocessedQuery->getEquations() );

    unsigned m = equations.size();
    unsigned n = _preprocessedQuery->getNumberOfVariables();

    // Trivial case, or if a trivial basis is requested
    if ( ( m == 0 ) || ( n == 0 ) || GlobalConfiguration::ONLY_AUX_INITIAL_BASIS )
    {
        for ( unsigned i = 0; i < m; ++i )
            basicRows.append( i );

        return;
    }

    unsigned *nnzInRow = new unsigned[m];
    unsigned *nnzInColumn = new unsigned[n];

    std::fill_n( nnzInRow, m, 0 );
    std::fill_n( nnzInColumn, n, 0 );

    unsigned *columnOrdering = new unsigned[n];
    unsigned *rowOrdering = new unsigned[m];

    for ( unsigned i = 0; i < m; ++i )
        rowOrdering[i] = i;

    for ( unsigned i = 0; i < n; ++i )
        columnOrdering[i] = i;

    // Initialize the counters
    for ( unsigned i = 0; i < m; ++i )
    {
        for ( unsigned j = 0; j < n; ++j )
        {
            if ( !FloatUtils::isZero( constraintMatrix[i * n + j] ) )
            {
                ++nnzInRow[i];
                ++nnzInColumn[j];
            }
        }
    }

    DEBUG( {
        for ( unsigned i = 0; i < m; ++i )
        {
            ASSERT( nnzInRow[i] > 0 );
        }
    } );

    unsigned numExcluded = 0;
    unsigned numTriangularRows = 0;
    unsigned temp;

    while ( numExcluded + numTriangularRows < n )
    {
        // Do we have a singleton row?
        unsigned singletonRow = m;
        for ( unsigned i = numTriangularRows; i < m; ++i )
        {
            if ( nnzInRow[i] == 1 )
            {
                singletonRow = i;
                break;
            }
        }

        if ( singletonRow < m )
        {
            // Have a singleton row! Swap it to the top and update counters
            temp = rowOrdering[singletonRow];
            rowOrdering[singletonRow] = rowOrdering[numTriangularRows];
            rowOrdering[numTriangularRows] = temp;

            temp = nnzInRow[numTriangularRows];
            nnzInRow[numTriangularRows] = nnzInRow[singletonRow];
            nnzInRow[singletonRow] = temp;

            // Find the non-zero entry in the row and swap it to the diagonal
            DEBUG( bool foundNonZero = false );
            for ( unsigned i = numTriangularRows; i < n - numExcluded; ++i )
            {
                if ( !FloatUtils::isZero( constraintMatrix[rowOrdering[numTriangularRows] * n +
                                                           columnOrdering[i]] ) )
                {
                    temp = columnOrdering[i];
                    columnOrdering[i] = columnOrdering[numTriangularRows];
                    columnOrdering[numTriangularRows] = temp;

                    temp = nnzInColumn[numTriangularRows];
                    nnzInColumn[numTriangularRows] = nnzInColumn[i];
                    nnzInColumn[i] = temp;

                    DEBUG( foundNonZero = true );
                    break;
                }
            }

            ASSERT( foundNonZero );

            // Remove all entries under the diagonal entry from the row counters
            for ( unsigned i = numTriangularRows + 1; i < m; ++i )
            {
                if ( !FloatUtils::isZero( constraintMatrix[rowOrdering[i] * n +
                                                           columnOrdering[numTriangularRows]] ) )
                    --nnzInRow[i];
            }

            ++numTriangularRows;
        }
        else
        {
            // No singleton rows. Exclude the densest column
            unsigned maxDensity = nnzInColumn[numTriangularRows];
            unsigned column = numTriangularRows;

            for ( unsigned i = numTriangularRows; i < n - numExcluded; ++i )
            {
                if ( nnzInColumn[i] > maxDensity )
                {
                    maxDensity = nnzInColumn[i];
                    column = i;
                }
            }

            // Update the row counters to account for the excluded column
            for ( unsigned i = numTriangularRows; i < m; ++i )
            {
                double element = constraintMatrix[rowOrdering[i] * n + columnOrdering[column]];
                if ( !FloatUtils::isZero( element ) )
                {
                    ASSERT( nnzInRow[i] > 1 );
                    --nnzInRow[i];
                }
            }

            columnOrdering[column] = columnOrdering[n - 1 - numExcluded];
            nnzInColumn[column] = nnzInColumn[n - 1 - numExcluded];
            ++numExcluded;
        }
    }

    // Final basis: diagonalized columns + non-diagonalized rows
    List<unsigned> result;

    for ( unsigned i = 0; i < numTriangularRows; ++i )
    {
        initialBasis.append( columnOrdering[i] );
    }

    for ( unsigned i = numTriangularRows; i < m; ++i )
    {
        basicRows.append( rowOrdering[i] );
    }

    // Cleanup
    delete[] nnzInRow;
    delete[] nnzInColumn;
    delete[] columnOrdering;
    delete[] rowOrdering;
}

void Engine::addAuxiliaryVariables()
{
    List<Equation> &equations( _preprocessedQuery->getEquations() );

    unsigned m = equations.size();
    unsigned originalN = _preprocessedQuery->getNumberOfVariables();
    unsigned n = originalN + m;

    _preprocessedQuery->setNumberOfVariables( n );

    // Add auxiliary variables to the equations and set their bounds
    unsigned count = 0;
    for ( auto &eq : equations )
    {
        unsigned auxVar = originalN + count;
        if ( _produceUNSATProofs )
            _preprocessedQuery->_lastAddendToAux.insert( eq._addends.back()._variable, auxVar );
        eq.addAddend( -1, auxVar );
        _preprocessedQuery->setLowerBound( auxVar, eq._scalar );
        _preprocessedQuery->setUpperBound( auxVar, eq._scalar );
        eq.setScalar( 0 );

        ++count;
    }
}

void Engine::augmentInitialBasisIfNeeded( List<unsigned> &initialBasis,
                                          const List<unsigned> &basicRows )
{
    unsigned m = _preprocessedQuery->getEquations().size();
    unsigned n = _preprocessedQuery->getNumberOfVariables();
    unsigned originalN = n - m;

    if ( initialBasis.size() != m )
    {
        for ( const auto &basicRow : basicRows )
            initialBasis.append( basicRow + originalN );
    }
}

void Engine::initializeTableau( const double *constraintMatrix, const List<unsigned> &initialBasis )
{
    const List<Equation> &equations( _preprocessedQuery->getEquations() );
    unsigned m = equations.size();
    unsigned n = _preprocessedQuery->getNumberOfVariables();

    _tableau->setDimensions( m, n );

    adjustWorkMemorySize();

    unsigned equationIndex = 0;
    for ( const auto &equation : equations )
    {
        _tableau->setRightHandSide( equationIndex, equation._scalar );
        ++equationIndex;
    }

    // Populate constraint matrix
    _tableau->setConstraintMatrix( constraintMatrix );

    _tableau->registerToWatchAllVariables( _rowBoundTightener );
    _tableau->registerResizeWatcher( _rowBoundTightener );

    _rowBoundTightener->setDimensions();

    initializeBoundsAndConstraintWatchersInTableau( n );

    _tableau->initializeTableau( initialBasis );

    _costFunctionManager->initialize();
    _tableau->registerCostFunctionManager( _costFunctionManager );
    _activeEntryStrategy->initialize( _tableau );
}

void Engine::initializeBoundsAndConstraintWatchersInTableau( unsigned numberOfVariables )
{
    _plConstraints = _preprocessedQuery->getPiecewiseLinearConstraints();
    for ( const auto &constraint : _plConstraints )
    {
        constraint->registerAsWatcher( _tableau );
        constraint->setStatistics( &_statistics );

        // Assuming aux var is use, add the constraint's auxiliary variable assigned to it in the
        // tableau, to the constraint
        if ( _produceUNSATProofs )
            for ( unsigned var : constraint->getNativeAuxVars() )
                if ( _preprocessedQuery->_lastAddendToAux.exists( var ) )
                    constraint->addTableauAuxVar( _preprocessedQuery->_lastAddendToAux.at( var ),
                                                  var );
    }

    _nlConstraints = _preprocessedQuery->getNonlinearConstraints();
    for ( const auto &constraint : _nlConstraints )
    {
        constraint->registerAsWatcher( _tableau );
        constraint->setStatistics( &_statistics );
    }

    for ( unsigned i = 0; i < numberOfVariables; ++i )
    {
        _tableau->setLowerBound( i, _preprocessedQuery->getLowerBound( i ) );
        _tableau->setUpperBound( i, _preprocessedQuery->getUpperBound( i ) );
    }
    _boundManager.storeLocalBounds();

    _statistics.setUnsignedAttribute( Statistics::NUM_PL_CONSTRAINTS, _plConstraints.size() );
}

void Engine::initializeNetworkLevelReasoning()
{
    _networkLevelReasoner = _preprocessedQuery->getNetworkLevelReasoner();

    if ( _networkLevelReasoner )
    {
        _networkLevelReasoner->computeSuccessorLayers();
        _networkLevelReasoner->setTableau( _tableau );
        if ( Options::get()->getBool( Options::DUMP_TOPOLOGY ) )
        {
            _networkLevelReasoner->dumpTopology( false );
            std::cout << std::endl;
        }
    }
}

bool Engine::processInputQuery( const IQuery &inputQuery, bool preprocess )
{
    ENGINE_LOG( "processInputQuery starting\n" );
    struct timespec start = TimeUtils::sampleMicro();

    try
    {
        invokePreprocessor( inputQuery, preprocess );
        if ( _verbosity > 1 )
            printInputBounds( inputQuery );
        initializeNetworkLevelReasoning();
        if ( preprocess )
        {
            performSymbolicBoundTightening( &( *_preprocessedQuery ) );
            performSimulation();
            performMILPSolverBoundedTightening( &( *_preprocessedQuery ) );
            performAdditionalBackwardAnalysisIfNeeded();
        }

        if ( GlobalConfiguration::PL_CONSTRAINTS_ADD_AUX_EQUATIONS_AFTER_PREPROCESSING )
            for ( auto &plConstraint : _preprocessedQuery->getPiecewiseLinearConstraints() )
                plConstraint->addAuxiliaryEquationsAfterPreprocessing( *_preprocessedQuery );

        if ( GlobalConfiguration::NL_CONSTRAINTS_ADD_AUX_EQUATIONS_AFTER_PREPROCESSING )
            for ( auto &nlConstraint : _preprocessedQuery->getNonlinearConstraints() )
                nlConstraint->addAuxiliaryEquationsAfterPreprocessing( *_preprocessedQuery );

        if ( _produceUNSATProofs )
        {
            for ( auto &plConstraint : _preprocessedQuery->getPiecewiseLinearConstraints() )
            {
                if ( !UNSATCertificateUtils::getSupportedActivations().exists(
                         plConstraint->getType() ) )
                {
                    _produceUNSATProofs = false;
                    Options::get()->setBool( Options::PRODUCE_PROOFS, false );
                    String activationType =
                        plConstraint->serializeToString().tokenize( "," ).back();
                    printf(
                        "Turning off proof production since activation %s is not yet supported\n",
                        activationType.ascii() );
                    break;
                }
            }
        }

        if ( _lpSolverType == LPSolverType::NATIVE )
        {
            double *constraintMatrix = createConstraintMatrix();
            removeRedundantEquations( constraintMatrix );

            // The equations have changed, recreate the constraint matrix
            delete[] constraintMatrix;
            constraintMatrix = createConstraintMatrix();

            List<unsigned> initialBasis;
            List<unsigned> basicRows;
            selectInitialVariablesForBasis( constraintMatrix, initialBasis, basicRows );
            addAuxiliaryVariables();
            augmentInitialBasisIfNeeded( initialBasis, basicRows );

            storeEquationsInDegradationChecker();

            // The equations have changed, recreate the constraint matrix
            delete[] constraintMatrix;
            constraintMatrix = createConstraintMatrix();

            unsigned n = _preprocessedQuery->getNumberOfVariables();
            _boundManager.initialize( n );

            initializeTableau( constraintMatrix, initialBasis );
            _boundManager.initializeBoundExplainer( n, _tableau->getM() );
            delete[] constraintMatrix;

            if ( _produceUNSATProofs )
            {
                _UNSATCertificate = new UnsatCertificateNode( NULL, PiecewiseLinearCaseSplit() );
                _UNSATCertificateCurrentPointer->set( _UNSATCertificate );
                _UNSATCertificate->setVisited();
                _groundBoundManager.initialize( n );

                for ( unsigned i = 0; i < n; ++i )
                {
                    _groundBoundManager.addGroundBound(
                        i, _preprocessedQuery->getUpperBound( i ), Tightening::UB, false );
                    _groundBoundManager.addGroundBound(
                        i, _preprocessedQuery->getLowerBound( i ), Tightening::LB, false );
                }
            }
        }
        else
        {
            ASSERT( _lpSolverType == LPSolverType::GUROBI );

            ASSERT( GlobalConfiguration::USE_DEEPSOI_LOCAL_SEARCH == true );

            if ( _verbosity > 0 )
                printf( "Using Gurobi to solve LP...\n" );

            unsigned n = _preprocessedQuery->getNumberOfVariables();
            unsigned m = _preprocessedQuery->getEquations().size();
            // Only use BoundManager to store the bounds.
            _boundManager.initialize( n );
            _tableau->setDimensions( m, n );
            initializeBoundsAndConstraintWatchersInTableau( n );
        }

        for ( const auto &constraint : _plConstraints )
        {
            constraint->registerTableau( _tableau );
#ifdef BUILD_CADICAL
            constraint->registerCdclCore( &_cdclCore );
#endif
            if ( !Options::get()->getBool( Options::DNC_MODE ) )
                constraint->initializeCDOs( &_context );
        }
        for ( const auto &constraint : _nlConstraints )
            constraint->registerTableau( _tableau );

        if ( _networkLevelReasoner && Options::get()->getBool( Options::DUMP_BOUNDS ) )
            _networkLevelReasoner->dumpBounds();

        if ( GlobalConfiguration::USE_DEEPSOI_LOCAL_SEARCH )
        {
            _soiManager = std::unique_ptr<SumOfInfeasibilitiesManager>(
                new SumOfInfeasibilitiesManager( *_preprocessedQuery, *_tableau ) );
            _soiManager->setStatistics( &_statistics );
        }

        if ( GlobalConfiguration::WARM_START )
            warmStart();

        decideBranchingHeuristics();

        struct timespec end = TimeUtils::sampleMicro();
        _statistics.setLongAttribute( Statistics::PREPROCESSING_TIME_MICRO,
                                      TimeUtils::timePassed( start, end ) );

        if ( !_tableau->allBoundsValid() )
        {
            // Some variable bounds are invalid, so the query is unsat
            throw InfeasibleQueryException();
        }

#ifdef BUILD_CADICAL
        if ( _solveWithCDCL )
        {
            if ( !_nlConstraints.empty() )
                throw MarabouError( MarabouError::FEATURE_NOT_YET_SUPPORTED,
                                    "The network contains constraints currently "
                                    "unsupported by CDCL" );

            for ( auto *constraint : _plConstraints )
            {
                if ( !CdclCore::isSupported( constraint ) )
                    throw MarabouError( MarabouError::FEATURE_NOT_YET_SUPPORTED,
                                        "The network contains constraints currently "
                                        "unsupported by CDCL" );

                _cdclCore.initBooleanAbstraction( constraint );
            }
        }
#endif
    }
    catch ( const InfeasibleQueryException & )
    {
        ENGINE_LOG( "processInputQuery done\n" );

        struct timespec end = TimeUtils::sampleMicro();
        _statistics.setLongAttribute( Statistics::PREPROCESSING_TIME_MICRO,
                                      TimeUtils::timePassed( start, end ) );

        _exitCode = ExitCode::UNSAT;
        return false;
    }

    ENGINE_LOG( "processInputQuery done\n" );

    DEBUG( {
        // Initially, all constraints should be active
        for ( const auto &plc : _plConstraints )
        {
            ASSERT( plc->isActive() );
        }
    } );

    _searchTreeHandler.storeDebuggingSolution( _preprocessedQuery->_debuggingSolution );
    return true;
}

void Engine::performMILPSolverBoundedTightening( Query *inputQuery )
{
    if ( _networkLevelReasoner && Options::get()->gurobiEnabled() )
    {
        // Obtain from and store bounds into inputQuery if it is not null.
        if ( inputQuery )
            _networkLevelReasoner->obtainCurrentBounds( *inputQuery );
        else
            _networkLevelReasoner->obtainCurrentBounds();

        // TODO: Remove this block after getting ready to support sigmoid with MILP Bound
        // Tightening.
        if ( ( _milpSolverBoundTighteningType == MILPSolverBoundTighteningType::MILP_ENCODING ||
               _milpSolverBoundTighteningType ==
                   MILPSolverBoundTighteningType::MILP_ENCODING_INCREMENTAL ||
               _milpSolverBoundTighteningType ==
                   MILPSolverBoundTighteningType::ITERATIVE_PROPAGATION ) &&
             _preprocessedQuery->getNonlinearConstraints().size() > 0 )
            throw MarabouError( MarabouError::FEATURE_NOT_YET_SUPPORTED,
                                "Marabou doesn't support sigmoid with MILP Bound Tightening" );

        switch ( _milpSolverBoundTighteningType )
        {
        case MILPSolverBoundTighteningType::LP_RELAXATION:
        case MILPSolverBoundTighteningType::LP_RELAXATION_INCREMENTAL:
        case MILPSolverBoundTighteningType::BACKWARD_ANALYSIS_ONCE:
        case MILPSolverBoundTighteningType::BACKWARD_ANALYSIS_CONVERGE:
            _networkLevelReasoner->lpRelaxationPropagation();
            break;
        case MILPSolverBoundTighteningType::MILP_ENCODING:
        case MILPSolverBoundTighteningType::MILP_ENCODING_INCREMENTAL:
            _networkLevelReasoner->MILPPropagation();
            break;
        case MILPSolverBoundTighteningType::ITERATIVE_PROPAGATION:
            _networkLevelReasoner->iterativePropagation();
            break;
        case MILPSolverBoundTighteningType::NONE:
            return;
        }
        List<Tightening> tightenings;
        _networkLevelReasoner->getConstraintTightenings( tightenings );


        if ( inputQuery )
        {
            for ( const auto &tightening : tightenings )
            {
                if ( tightening._type == Tightening::LB &&
                     FloatUtils::gt( tightening._value,
                                     inputQuery->getLowerBound( tightening._variable ) ) )
                    inputQuery->setLowerBound( tightening._variable, tightening._value );
                if ( tightening._type == Tightening::UB &&
                     FloatUtils::lt( tightening._value,
                                     inputQuery->getUpperBound( tightening._variable ) ) )
                    inputQuery->setUpperBound( tightening._variable, tightening._value );
            }
        }
        else
        {
            for ( const auto &tightening : tightenings )
            {
                if ( tightening._type == Tightening::LB )
                    _tableau->tightenLowerBound( tightening._variable, tightening._value );

                else if ( tightening._type == Tightening::UB )
                    _tableau->tightenUpperBound( tightening._variable, tightening._value );
            }
        }
    }
}

void Engine::performAdditionalBackwardAnalysisIfNeeded()
{
    if ( _milpSolverBoundTighteningType == MILPSolverBoundTighteningType::BACKWARD_ANALYSIS_ONCE ||
         _milpSolverBoundTighteningType ==
             MILPSolverBoundTighteningType::BACKWARD_ANALYSIS_CONVERGE )
    {
        unsigned tightened = performSymbolicBoundTightening( &( *_preprocessedQuery ) );
        if ( _verbosity > 0 )
            printf( "Backward analysis tightened %u bounds\n", tightened );
        while ( tightened &&
                _milpSolverBoundTighteningType ==
                    MILPSolverBoundTighteningType::BACKWARD_ANALYSIS_CONVERGE &&
                GlobalConfiguration::MAX_ROUNDS_OF_BACKWARD_ANALYSIS )
        {
            performMILPSolverBoundedTightening( &( *_preprocessedQuery ) );
            tightened = performSymbolicBoundTightening( &( *_preprocessedQuery ) );
            if ( _verbosity > 0 )
                printf( "Backward analysis tightened %u bounds\n", tightened );
        }
    }
}

void Engine::performMILPSolverBoundedTighteningForSingleLayer( unsigned targetIndex )
{
    if ( _produceUNSATProofs )
        return;

    if ( _networkLevelReasoner && _isGurobyEnabled && _performLpTighteningAfterSplit &&
         _milpSolverBoundTighteningType != MILPSolverBoundTighteningType::NONE )
    {
        _networkLevelReasoner->obtainCurrentBounds();
        _networkLevelReasoner->clearConstraintTightenings();

        switch ( _milpSolverBoundTighteningType )
        {
        case MILPSolverBoundTighteningType::LP_RELAXATION:
            _networkLevelReasoner->LPTighteningForOneLayer( targetIndex );
            break;
        case MILPSolverBoundTighteningType::LP_RELAXATION_INCREMENTAL:
            return;
        case MILPSolverBoundTighteningType::MILP_ENCODING:
            _networkLevelReasoner->MILPTighteningForOneLayer( targetIndex );
            break;
        case MILPSolverBoundTighteningType::MILP_ENCODING_INCREMENTAL:
            return;
        case MILPSolverBoundTighteningType::BACKWARD_ANALYSIS_ONCE:
        case MILPSolverBoundTighteningType::BACKWARD_ANALYSIS_CONVERGE:
        case MILPSolverBoundTighteningType::ITERATIVE_PROPAGATION:
        case MILPSolverBoundTighteningType::NONE:
            return;
        }
        List<Tightening> tightenings;
        _networkLevelReasoner->getConstraintTightenings( tightenings );

        for ( const auto &tightening : tightenings )
        {
            if ( tightening._type == Tightening::LB )
                _tableau->tightenLowerBound( tightening._variable, tightening._value );

            else if ( tightening._type == Tightening::UB )
                _tableau->tightenUpperBound( tightening._variable, tightening._value );
        }
    }
}

void Engine::extractSolution( IQuery &inputQuery, Preprocessor *preprocessor )
{
    Preprocessor *preprocessorInUse = nullptr;
    if ( preprocessor != nullptr )
        preprocessorInUse = preprocessor;
    else if ( _preprocessingEnabled )
        preprocessorInUse = &_preprocessor;

    for ( unsigned i = 0; i < inputQuery.getNumberOfVariables(); ++i )
    {
        if ( preprocessorInUse )
        {
            // Symbolically fixed variables are skipped. They will be re-constructed in the end.
            if ( preprocessorInUse->variableIsUnusedAndSymbolicallyFixed( i ) )
                continue;

            // Has the variable been merged into another?
            unsigned variable = i;
            while ( preprocessorInUse->variableIsMerged( variable ) )
                variable = preprocessorInUse->getMergedIndex( variable );

            // Fixed variables are easy: return the value they've been fixed to.
            if ( preprocessorInUse->variableIsFixed( variable ) )
            {
                inputQuery.setSolutionValue( i, preprocessorInUse->getFixedValue( variable ) );
                continue;
            }

            // We know which variable to look for, but it may have been assigned
            // a new index, due to variable elimination
            variable = preprocessorInUse->getNewIndex( variable );

            // Finally, set the assigned value
            inputQuery.setSolutionValue( i, _tableau->getValue( variable ) );
        }
        else
        {
            inputQuery.setSolutionValue( i, _tableau->getValue( i ) );
        }
    }

    if ( preprocessorInUse )
    {
        /*
          Recover the assignment of eliminated neurons (e.g., due to merging of WS layers)
        */
        preprocessorInUse->setSolutionValuesOfEliminatedNeurons( inputQuery );
    }
}

bool Engine::allVarsWithinBounds() const
{
    if ( _lpSolverType == LPSolverType::GUROBI )
    {
        ASSERT( _gurobi );
        return _gurobi->haveFeasibleSolution();
    }
    else
        return !_tableau->existsBasicOutOfBounds();
}

void Engine::collectViolatedPlConstraints()
{
    _violatedPlConstraints.clear();
    for ( const auto &constraint : _plConstraints )
    {
        if ( constraint->isActive() && !constraint->satisfied() )
            _violatedPlConstraints.append( constraint );
    }
}

bool Engine::allPlConstraintsHold()
{
    return _violatedPlConstraints.empty();
}

bool Engine::allNonlinearConstraintsHold()
{
    for ( const auto &constraint : _nlConstraints )
    {
        if ( !constraint->satisfied() )
            return false;
    }
    return true;
}

bool Engine::hasBranchingCandidate()
{
    for ( const auto &constraint : _plConstraints )
    {
        if ( constraint->isActive() && !constraint->phaseFixed() )
            return true;
    }
    return false;
}

void Engine::selectViolatedPlConstraint()
{
    ASSERT( !_violatedPlConstraints.empty() );

    _plConstraintToFix =
        _searchTreeHandler.chooseViolatedConstraintForFixing( _violatedPlConstraints );

    ASSERT( _plConstraintToFix );
}

void Engine::reportPlViolation()
{
    _searchTreeHandler.reportViolatedConstraint( _plConstraintToFix );
}

void Engine::storeState( EngineState &state, TableauStateStorageLevel level ) const
{
    _tableau->storeState( state._tableauState, level );
    state._tableauStateStorageLevel = level;

    for ( const auto &constraint : _plConstraints )
        if ( !constraint->getContext() )
            state._plConstraintToState[constraint] = constraint->duplicateConstraint();

    state._numPlConstraintsDisabledByValidSplits = _numPlConstraintsDisabledByValidSplits;
}

void Engine::restoreState( const EngineState &state )
{
    ENGINE_LOG( "Restore state starting" );

    if ( state._tableauStateStorageLevel == TableauStateStorageLevel::STORE_NONE )
        throw MarabouError( MarabouError::RESTORING_ENGINE_FROM_INVALID_STATE );

    ENGINE_LOG( "\tRestoring tableau state" );
    _tableau->restoreState( state._tableauState, state._tableauStateStorageLevel );

    ENGINE_LOG( "\tRestoring constraint states" );
    for ( auto &constraint : _plConstraints )
    {
        if ( !state._plConstraintToState.exists( constraint ) && !constraint->getContext() )
            throw MarabouError( MarabouError::MISSING_PL_CONSTRAINT_STATE );

        if ( !constraint->getContext() )
            constraint->restoreState( state._plConstraintToState[constraint] );
    }

    _numPlConstraintsDisabledByValidSplits = state._numPlConstraintsDisabledByValidSplits;

    if ( _lpSolverType == LPSolverType::NATIVE )
    {
        // Make sure the data structures are initialized to the correct size
        _rowBoundTightener->setDimensions();
        adjustWorkMemorySize();
        _activeEntryStrategy->resizeHook( _tableau );
        _costFunctionManager->initialize();
    }

    // Reset the violation counts in the Search Tree handler
    _searchTreeHandler.resetSplitConditions();
}

void Engine::setNumPlConstraintsDisabledByValidSplits( unsigned numConstraints )
{
    _numPlConstraintsDisabledByValidSplits = numConstraints;
}

bool Engine::attemptToMergeVariables( unsigned x1, unsigned x2 )
{
    /*
      First, we need to ensure that the variables are both non-basic.
    */

    unsigned n = _tableau->getN();
    unsigned m = _tableau->getM();

    if ( _tableau->isBasic( x1 ) )
    {
        TableauRow x1Row( n - m );
        _tableau->getTableauRow( _tableau->variableToIndex( x1 ), &x1Row );

        bool found = false;
        double bestCoefficient = 0.0;
        unsigned nonBasic = 0;
        for ( unsigned i = 0; i < n - m; ++i )
        {
            if ( x1Row._row[i]._var != x2 )
            {
                double contender = FloatUtils::abs( x1Row._row[i]._coefficient );
                if ( FloatUtils::gt( contender, bestCoefficient ) )
                {
                    found = true;
                    nonBasic = x1Row._row[i]._var;
                    bestCoefficient = contender;
                }
            }
        }

        if ( !found )
            return false;

        _tableau->setEnteringVariableIndex( _tableau->variableToIndex( nonBasic ) );
        _tableau->setLeavingVariableIndex( _tableau->variableToIndex( x1 ) );

        // Make sure the change column and pivot row are up-to-date - strategies
        // such as projected steepest edge need these for their internal updates.
        _tableau->computeChangeColumn();
        _tableau->computePivotRow();

        _activeEntryStrategy->prePivotHook( _tableau, false );
        _tableau->performDegeneratePivot();
        _activeEntryStrategy->postPivotHook( _tableau, false );
    }

    if ( _tableau->isBasic( x2 ) )
    {
        TableauRow x2Row( n - m );
        _tableau->getTableauRow( _tableau->variableToIndex( x2 ), &x2Row );

        bool found = false;
        double bestCoefficient = 0.0;
        unsigned nonBasic = 0;
        for ( unsigned i = 0; i < n - m; ++i )
        {
            if ( x2Row._row[i]._var != x1 )
            {
                double contender = FloatUtils::abs( x2Row._row[i]._coefficient );
                if ( FloatUtils::gt( contender, bestCoefficient ) )
                {
                    found = true;
                    nonBasic = x2Row._row[i]._var;
                    bestCoefficient = contender;
                }
            }
        }

        if ( !found )
            return false;

        _tableau->setEnteringVariableIndex( _tableau->variableToIndex( nonBasic ) );
        _tableau->setLeavingVariableIndex( _tableau->variableToIndex( x2 ) );

        // Make sure the change column and pivot row are up-to-date - strategies
        // such as projected steepest edge need these for their internal updates.
        _tableau->computeChangeColumn();
        _tableau->computePivotRow();

        _activeEntryStrategy->prePivotHook( _tableau, false );
        _tableau->performDegeneratePivot();
        _activeEntryStrategy->postPivotHook( _tableau, false );
    }

    // Both variables are now non-basic, so we can merge their columns
    _tableau->mergeColumns( x1, x2 );
    DEBUG( _tableau->verifyInvariants() );

    // Reset the entry strategy
    _activeEntryStrategy->initialize( _tableau );

    return true;
}

void Engine::applySplit( const PiecewiseLinearCaseSplit &split )
{
    ENGINE_LOG( "" );
    ENGINE_LOG( "Applying a split. " );

    DEBUG( _tableau->verifyInvariants() );

    List<Tightening> bounds = split.getBoundTightenings();
    List<Equation> equations = split.getEquations();

    // We assume that case splits only apply new bounds but do not apply
    // new equations. This can always be made possible.
    if ( _lpSolverType != LPSolverType::NATIVE && equations.size() > 0 )
        throw MarabouError( MarabouError::FEATURE_NOT_YET_SUPPORTED,
                            "Can only update bounds when using non-native"
                            "simplex engine!" );

    for ( auto &equation : equations )
    {
        /*
          First, adjust the equation if any variables have been merged.
          E.g., if the equation is x1 + x2 + x3 = 0, and x1 and x2 have been
          merged, the equation becomes 2x1 + x3 = 0
        */
        for ( auto &addend : equation._addends )
            addend._variable = _tableau->getVariableAfterMerging( addend._variable );

        List<Equation::Addend>::iterator addend;
        List<Equation::Addend>::iterator otherAddend;

        addend = equation._addends.begin();
        while ( addend != equation._addends.end() )
        {
            otherAddend = addend;
            ++otherAddend;

            while ( otherAddend != equation._addends.end() )
            {
                if ( otherAddend->_variable == addend->_variable )
                {
                    addend->_coefficient += otherAddend->_coefficient;
                    otherAddend = equation._addends.erase( otherAddend );
                }
                else
                    ++otherAddend;
            }

            if ( FloatUtils::isZero( addend->_coefficient ) )
                addend = equation._addends.erase( addend );
            else
                ++addend;
        }

        /*
          In the general case, we just add the new equation to the tableau.
          However, we also support a very common case: equations of the form
          x1 = x2, which are common, e.g., with ReLUs. For these equations we
          may be able to merge two columns of the tableau.
        */
        unsigned x1, x2;
        bool canMergeColumns =
            // Only if the flag is on
            GlobalConfiguration::USE_COLUMN_MERGING_EQUATIONS &&
            // Only if the equation has the correct form
            equation.isVariableMergingEquation( x1, x2 ) &&
            // And only if the variables are not out of bounds
            ( !_tableau->isBasic( x1 ) ||
              !_tableau->basicOutOfBounds( _tableau->variableToIndex( x1 ) ) ) &&
            ( !_tableau->isBasic( x2 ) ||
              !_tableau->basicOutOfBounds( _tableau->variableToIndex( x2 ) ) );

        bool columnsSuccessfullyMerged = false;
        if ( canMergeColumns )
            columnsSuccessfullyMerged = attemptToMergeVariables( x1, x2 );

        if ( !columnsSuccessfullyMerged )
        {
            // General case: add a new equation to the tableau
            unsigned auxVariable = _tableau->addEquation( equation );
            _activeEntryStrategy->resizeHook( _tableau );

            switch ( equation._type )
            {
            case Equation::GE:
                bounds.append( Tightening( auxVariable, 0.0, Tightening::UB ) );
                break;

            case Equation::LE:
                bounds.append( Tightening( auxVariable, 0.0, Tightening::LB ) );
                break;

            case Equation::EQ:
                bounds.append( Tightening( auxVariable, 0.0, Tightening::LB ) );
                bounds.append( Tightening( auxVariable, 0.0, Tightening::UB ) );
                break;

            default:
                ASSERT( false );
                break;
            }
        }
    }

    if ( _lpSolverType == LPSolverType::NATIVE )
    {
        adjustWorkMemorySize();
    }
    for ( auto &bound : bounds )
    {
        unsigned variable = _tableau->getVariableAfterMerging( bound._variable );

        if ( bound._type == Tightening::LB )
        {
            ENGINE_LOG(
                Stringf( "x%u: lower bound set to %.3lf", variable, bound._value ).ascii() );
            if ( _produceUNSATProofs &&
                 FloatUtils::gt( bound._value, _boundManager.getLowerBound( bound._variable ) ) )
            {
                _boundManager.resetExplanation( variable, Tightening::LB );
                _groundBoundManager.addGroundBound( variable, bound._value, Tightening::LB, true );
                _boundManager.tightenLowerBound( variable, bound._value );
            }
            else if ( !_produceUNSATProofs )
                _boundManager.tightenLowerBound( variable, bound._value );
        }
        else
        {
            ENGINE_LOG(
                Stringf( "x%u: upper bound set to %.3lf", variable, bound._value ).ascii() );
            if ( _produceUNSATProofs &&
                 FloatUtils::lt( bound._value, _boundManager.getUpperBound( bound._variable ) ) )
            {
                _boundManager.resetExplanation( variable, Tightening::UB );
                _groundBoundManager.addGroundBound( variable, bound._value, Tightening::UB, true );
                _boundManager.tightenUpperBound( variable, bound._value );
            }
            else if ( !_produceUNSATProofs )
                _boundManager.tightenUpperBound( variable, bound._value );
        }
    }

    if ( _produceUNSATProofs && _UNSATCertificateCurrentPointer )
        ( **_UNSATCertificateCurrentPointer ).setVisited();

    DEBUG( _tableau->verifyInvariants() );
    ENGINE_LOG( "Done with split\n" );
}

void Engine::applyPlcPhaseFixingTightenings( PiecewiseLinearConstraint &constraint )
{
    ASSERT( constraint.phaseFixed() );
    ASSERT( constraint.getValidCaseSplit().getEquations().empty() );

    List<Tightening> bounds = constraint.getValidCaseSplit().getBoundTightenings();

    for ( auto &bound : bounds )
    {
        unsigned variable = _tableau->getVariableAfterMerging( bound._variable );

        if ( bound._type == Tightening::LB )
        {
            ENGINE_LOG(
                Stringf( "x%u: lower bound set to %.3lf", variable, bound._value ).ascii() );
            if ( _produceUNSATProofs &&
                 FloatUtils::gt( bound._value, _boundManager.getLowerBound( bound._variable ) ) &&
                 _lpSolverType == LPSolverType::NATIVE )
            {
                _boundManager.resetExplanation( variable, Tightening::LB );
                _groundBoundManager.addGroundBound( variable, bound._value, Tightening::LB, true );
                _boundManager.tightenLowerBound( variable, bound._value );
                if ( !constraint.getPhaseFixingEntry() )
                    constraint.setPhaseFixingEntry(
                        getGroundBoundEntry( variable, Tightening::LB ) );
            }
            else if ( !_produceUNSATProofs || _lpSolverType == LPSolverType::GUROBI )
                _boundManager.tightenLowerBound( variable, bound._value );
        }
        else
        {
            ENGINE_LOG(
                Stringf( "x%u: upper bound set to %.3lf", variable, bound._value ).ascii() );
            if ( _produceUNSATProofs &&
                 FloatUtils::lt( bound._value, _boundManager.getUpperBound( bound._variable ) ) &&
                 _lpSolverType == LPSolverType::NATIVE )
            {
                _boundManager.resetExplanation( variable, Tightening::UB );
                _groundBoundManager.addGroundBound( variable, bound._value, Tightening::UB, true );
                _boundManager.tightenUpperBound( variable, bound._value );
                if ( !constraint.getPhaseFixingEntry() )
                    constraint.setPhaseFixingEntry(
                        getGroundBoundEntry( variable, Tightening::UB ) );
            }
            else if ( !_produceUNSATProofs || _lpSolverType == LPSolverType::GUROBI )
                _boundManager.tightenUpperBound( variable, bound._value );
        }
    }

    DEBUG( _tableau->verifyInvariants() );
    ENGINE_LOG( "Done with split\n" );
}

void Engine::applyBoundTightenings()
{
    List<Tightening> tightenings;
    _boundManager.getTightenings( tightenings );

    for ( const auto &tightening : tightenings )
    {
        if ( tightening._type == Tightening::LB )
            _tableau->tightenLowerBound( tightening._variable, tightening._value );
        else
            _tableau->tightenUpperBound( tightening._variable, tightening._value );
    }
}

void Engine::applyAllRowTightenings()
{
    applyBoundTightenings();
}

void Engine::applyAllConstraintTightenings()
{
    applyBoundTightenings();
}

void Engine::applyAllBoundTightenings()
{
    struct timespec start = TimeUtils::sampleMicro();

    if ( _lpSolverType == LPSolverType::NATIVE )
        applyAllRowTightenings();
    applyAllConstraintTightenings();

    struct timespec end = TimeUtils::sampleMicro();
    _statistics.incLongAttribute( Statistics::TOTAL_TIME_APPLYING_STORED_TIGHTENINGS_MICRO,
                                  TimeUtils::timePassed( start, end ) );
}

bool Engine::applyAllValidConstraintCaseSplits()
{
    struct timespec start = TimeUtils::sampleMicro();

    bool appliedSplit = false;
    for ( auto &constraint : _plConstraints )
        if ( applyValidConstraintCaseSplit( constraint ) )
            appliedSplit = true;

    struct timespec end = TimeUtils::sampleMicro();
    _statistics.incLongAttribute( Statistics::TOTAL_TIME_PERFORMING_VALID_CASE_SPLITS_MICRO,
                                  TimeUtils::timePassed( start, end ) );

    return appliedSplit;
}

bool Engine::applyValidConstraintCaseSplit( PiecewiseLinearConstraint *constraint )
{
    if ( constraint->isActive() && constraint->phaseFixed() )
    {
        String constraintString;
        constraint->dump( constraintString );
        ENGINE_LOG( Stringf( "A constraint has become valid. Dumping constraint: %s",
                             constraintString.ascii() )
                        .ascii() );

        constraint->setActiveConstraint( false );
        applyPlcPhaseFixingTightenings( *constraint );

        if ( _soiManager )
            _soiManager->removeCostComponentFromHeuristicCost( constraint );
        ++_numPlConstraintsDisabledByValidSplits;

        return true;
    }

    return false;
}

bool Engine::shouldCheckDegradation()
{
    return _statistics.getLongAttribute( Statistics::NUM_MAIN_LOOP_ITERATIONS ) %
               GlobalConfiguration::DEGRADATION_CHECKING_FREQUENCY ==
           0;
}

bool Engine::highDegradation()
{
    struct timespec start = TimeUtils::sampleMicro();

    double degradation = _degradationChecker.computeDegradation( *_tableau );
    _statistics.setDoubleAttribute( Statistics::CURRENT_DEGRADATION, degradation );
    if ( FloatUtils::gt( degradation,
                         _statistics.getDoubleAttribute( Statistics::MAX_DEGRADATION ) ) )
        _statistics.setDoubleAttribute( Statistics::MAX_DEGRADATION, degradation );

    bool result = FloatUtils::gt( degradation, GlobalConfiguration::DEGRADATION_THRESHOLD );

    struct timespec end = TimeUtils::sampleMicro();
    _statistics.incLongAttribute( Statistics::TOTAL_TIME_DEGRADATION_CHECKING,
                                  TimeUtils::timePassed( start, end ) );

    // Debug
    if ( result )
        printf( "High degradation found!\n" );
    //

    return result;
}

void Engine::tightenBoundsOnConstraintMatrix()
{
    struct timespec start = TimeUtils::sampleMicro();

    if ( _statistics.getLongAttribute( Statistics::NUM_MAIN_LOOP_ITERATIONS ) %
             GlobalConfiguration::BOUND_TIGHTENING_ON_CONSTRAINT_MATRIX_FREQUENCY ==
         0 )
    {
        _rowBoundTightener->examineConstraintMatrix( true );
        _statistics.incLongAttribute( Statistics::NUM_BOUND_TIGHTENINGS_ON_CONSTRAINT_MATRIX );
    }

    struct timespec end = TimeUtils::sampleMicro();
    _statistics.incLongAttribute( Statistics::TOTAL_TIME_CONSTRAINT_MATRIX_BOUND_TIGHTENING_MICRO,
                                  TimeUtils::timePassed( start, end ) );
}

void Engine::explicitBasisBoundTightening()
{
    struct timespec start = TimeUtils::sampleMicro();

    bool saturation = GlobalConfiguration::EXPLICIT_BOUND_TIGHTENING_UNTIL_SATURATION;

    _statistics.incLongAttribute( Statistics::NUM_BOUND_TIGHTENINGS_ON_EXPLICIT_BASIS );

    switch ( GlobalConfiguration::EXPLICIT_BASIS_BOUND_TIGHTENING_TYPE )
    {
    case GlobalConfiguration::COMPUTE_INVERTED_BASIS_MATRIX:
        _rowBoundTightener->examineInvertedBasisMatrix( saturation );
        break;

    case GlobalConfiguration::USE_IMPLICIT_INVERTED_BASIS_MATRIX:
        _rowBoundTightener->examineImplicitInvertedBasisMatrix( saturation );
        break;

    case GlobalConfiguration::DISABLE_EXPLICIT_BASIS_TIGHTENING:
        break;
    }

    struct timespec end = TimeUtils::sampleMicro();
    _statistics.incLongAttribute( Statistics::TOTAL_TIME_EXPLICIT_BASIS_BOUND_TIGHTENING_MICRO,
                                  TimeUtils::timePassed( start, end ) );
}

void Engine::performPrecisionRestoration( PrecisionRestorer::RestoreBasics restoreBasics )
{
    struct timespec start = TimeUtils::sampleMicro();

    // debug
    double before = _degradationChecker.computeDegradation( *_tableau );
    //

    _precisionRestorer.restorePrecision( *this, *_tableau, restoreBasics );
    struct timespec end = TimeUtils::sampleMicro();
    _statistics.incLongAttribute( Statistics::TOTAL_TIME_PRECISION_RESTORATION,
                                  TimeUtils::timePassed( start, end ) );

    _statistics.incUnsignedAttribute( Statistics::NUM_PRECISION_RESTORATIONS );

    // debug
    double after = _degradationChecker.computeDegradation( *_tableau );
    if ( _verbosity > 0 )
        printf( "Performing precision restoration. Degradation before: %.15lf. After: %.15lf\n",
                before,
                after );
    //

    if ( highDegradation() && ( restoreBasics == PrecisionRestorer::RESTORE_BASICS ) )
    {
        // First round, with basic restoration, still resulted in high degradation.
        // Try again!
        start = TimeUtils::sampleMicro();
        _precisionRestorer.restorePrecision(
            *this, *_tableau, PrecisionRestorer::DO_NOT_RESTORE_BASICS );
        end = TimeUtils::sampleMicro();
        _statistics.incLongAttribute( Statistics::TOTAL_TIME_PRECISION_RESTORATION,
                                      TimeUtils::timePassed( start, end ) );
        _statistics.incUnsignedAttribute( Statistics::NUM_PRECISION_RESTORATIONS );

        // debug
        double afterSecond = _degradationChecker.computeDegradation( *_tableau );
        if ( _verbosity > 0 )
            printf(
                "Performing 2nd precision restoration. Degradation before: %.15lf. After: %.15lf\n",
                after,
                afterSecond );

        if ( highDegradation() )
            throw MarabouError( MarabouError::RESTORATION_FAILED_TO_RESTORE_PRECISION );
    }
}

void Engine::storeInitialEngineState()
{
    if ( !_initialStateStored )
    {
        _precisionRestorer.storeInitialEngineState( *this );
        _initialStateStored = true;
    }
}

void Engine::restoreInitialEngineState()
{
    if ( _initialStateStored )
        _precisionRestorer.restoreInitialEngineState( *this );
}

bool Engine::basisRestorationNeeded() const
{
    return _basisRestorationRequired == Engine::STRONG_RESTORATION_NEEDED ||
           _basisRestorationRequired == Engine::WEAK_RESTORATION_NEEDED;
}

const Statistics *Engine::getStatistics() const
{
    return &_statistics;
}

Query *Engine::getQuery()
{
    return &( *_preprocessedQuery );
}

void Engine::checkBoundCompliancyWithDebugSolution()
{
    if ( _searchTreeHandler.checkSkewFromDebuggingSolution() )
    {
        // The stack is compliant, we should not have learned any non-compliant bounds
        for ( const auto &var : _preprocessedQuery->_debuggingSolution )
        {
            // printf( "Looking at var %u\n", var.first );

            if ( FloatUtils::gt( _tableau->getLowerBound( var.first ), var.second, 1e-5 ) )
            {
                printf( "Error! The stack is compliant, but learned an non-compliant bound: "
                        "Solution for x%u is %.15lf, but learned lower bound %.15lf\n",
                        var.first,
                        var.second,
                        _tableau->getLowerBound( var.first ) );

                throw MarabouError( MarabouError::DEBUGGING_ERROR );
            }

            if ( FloatUtils::lt( _tableau->getUpperBound( var.first ), var.second, 1e-5 ) )
            {
                printf( "Error! The stack is compliant, but learned an non-compliant bound: "
                        "Solution for %u is %.15lf, but learned upper bound %.15lf\n",
                        var.first,
                        var.second,
                        _tableau->getUpperBound( var.first ) );

                throw MarabouError( MarabouError::DEBUGGING_ERROR );
            }
        }
    }
}

void Engine::quitSignal()
{
    _quitRequested = true;
}

std::atomic_bool *Engine::getQuitRequested()
{
    return &_quitRequested;
}

List<unsigned> Engine::getInputVariables() const
{
    return _preprocessedQuery->getInputVariables();
}

void Engine::performSimulation()
{
    if ( _simulationSize == 0 || !_networkLevelReasoner ||
         _milpSolverBoundTighteningType == MILPSolverBoundTighteningType::NONE ||
         _produceUNSATProofs )
    {
        ENGINE_LOG( Stringf( "Skip simulation..." ).ascii() );
        return;
    }

    // outer vector is for neuron
    // inner vector is for simulation value
    Vector<Vector<double>> simulations;

    std::mt19937 mt( GlobalConfiguration::SIMULATION_RANDOM_SEED );

    for ( unsigned i = 0; i < _networkLevelReasoner->getLayer( 0 )->getSize(); ++i )
    {
        std::uniform_real_distribution<double> distribution(
            _networkLevelReasoner->getLayer( 0 )->getLb( i ),
            _networkLevelReasoner->getLayer( 0 )->getUb( i ) );
        Vector<double> simulationInput( _simulationSize );

        for ( unsigned j = 0; j < _simulationSize; ++j )
            simulationInput[j] = distribution( mt );
        simulations.append( simulationInput );
    }
    _networkLevelReasoner->simulate( &simulations );
}

unsigned Engine::performSymbolicBoundTightening( Query *inputQuery )
{
    if ( _symbolicBoundTighteningType == SymbolicBoundTighteningType::NONE ||
         ( !_networkLevelReasoner ) || _produceUNSATProofs )
        return 0;

    struct timespec start = TimeUtils::sampleMicro();

    unsigned numTightenedBounds = 0;

    // Step 1: tell the NLR about the current bounds
    if ( inputQuery )
    {
        // Obtain from and store bounds into inputQuery if it is not null.
        _networkLevelReasoner->obtainCurrentBounds( *inputQuery );
    }
    else
    {
        // Get bounds from Tableau.
        _networkLevelReasoner->obtainCurrentBounds();
    }

    // Step 2: perform SBT
    if ( _symbolicBoundTighteningType == SymbolicBoundTighteningType::SYMBOLIC_BOUND_TIGHTENING )
        _networkLevelReasoner->symbolicBoundPropagation();
    else if ( _symbolicBoundTighteningType == SymbolicBoundTighteningType::DEEP_POLY )
        _networkLevelReasoner->deepPolyPropagation();

    // Step 3: Extract the bounds
    List<Tightening> tightenings;
    _networkLevelReasoner->getConstraintTightenings( tightenings );

    if ( inputQuery )
    {
        for ( const auto &tightening : tightenings )
        {
            if ( tightening._type == Tightening::LB &&
                 FloatUtils::gt( tightening._value,
                                 inputQuery->getLowerBound( tightening._variable ) ) )
            {
                inputQuery->setLowerBound( tightening._variable, tightening._value );
                ++numTightenedBounds;
            }

            if ( tightening._type == Tightening::UB &&
                 FloatUtils::lt( tightening._value,
                                 inputQuery->getUpperBound( tightening._variable ) ) )
            {
                inputQuery->setUpperBound( tightening._variable, tightening._value );
                ++numTightenedBounds;
            }
        }
    }
    else
    {
        for ( const auto &tightening : tightenings )
        {
            if ( tightening._type == Tightening::LB &&
                 FloatUtils::gt( tightening._value,
                                 _tableau->getLowerBound( tightening._variable ) ) )
            {
                _tableau->tightenLowerBound( tightening._variable, tightening._value );
                ++numTightenedBounds;
            }

            if ( tightening._type == Tightening::UB &&
                 FloatUtils::lt( tightening._value,
                                 _tableau->getUpperBound( tightening._variable ) ) )
            {
                _tableau->tightenUpperBound( tightening._variable, tightening._value );
                ++numTightenedBounds;
            }
        }
    }

    struct timespec end = TimeUtils::sampleMicro();
    _statistics.incLongAttribute( Statistics::TOTAL_TIME_PERFORMING_SYMBOLIC_BOUND_TIGHTENING,
                                  TimeUtils::timePassed( start, end ) );
    _statistics.incLongAttribute( Statistics::NUM_TIGHTENINGS_FROM_SYMBOLIC_BOUND_TIGHTENING,
                                  numTightenedBounds );
    return numTightenedBounds;
}

bool Engine::shouldExitDueToTimeout( double timeout ) const
{
    // A timeout value of 0 means no time limit
    if ( timeout == 0 )
        return false;

    return static_cast<long double>( _statistics.getTotalTimeInMicro() ) / MICROSECONDS_TO_SECONDS >
           timeout;
}

void Engine::preContextPushHook()
{
    struct timespec start = TimeUtils::sampleMicro();
    _boundManager.storeLocalBounds();
    struct timespec end = TimeUtils::sampleMicro();

    _statistics.incLongAttribute( Statistics::TIME_CONTEXT_PUSH_HOOK,
                                  TimeUtils::timePassed( start, end ) );
}

void Engine::postContextPopHook()
{
    struct timespec start = TimeUtils::sampleMicro();

    _boundManager.restoreLocalBounds();
    if ( getLpSolverType() == LPSolverType::NATIVE )
    {
        _tableau->postContextPopHook();
        _costFunctionManager->computeCoreCostFunction();
    }

    struct timespec end = TimeUtils::sampleMicro();
    _statistics.incLongAttribute( Statistics::TIME_CONTEXT_POP_HOOK,
                                  TimeUtils::timePassed( start, end ) );
}

void Engine::reset()
{
    resetStatistics();
    _sncMode = false;
    clearViolatedPLConstraints();
    resetSearchTreeHandler();
    resetBoundTighteners();
}

void Engine::resetStatistics()
{
    Statistics statistics;
    _statistics = statistics;
    _searchTreeHandler.setStatistics( &_statistics );
    _tableau->setStatistics( &_statistics );
    _rowBoundTightener->setStatistics( &_statistics );
    _preprocessor.setStatistics( &_statistics );
    _activeEntryStrategy->setStatistics( &_statistics );

    _statistics.stampStartingTime();
}

void Engine::clearViolatedPLConstraints()
{
    _violatedPlConstraints.clear();
    _plConstraintToFix = NULL;
}

void Engine::resetSearchTreeHandler()
{
    _searchTreeHandler.reset();
#ifdef BUILD_CADICAL
    _searchTreeHandler.initializeScoreTrackerIfNeeded( _plConstraints, &_cdclCore );
#else
    _searchTreeHandler.initializeScoreTrackerIfNeeded( _plConstraints );
#endif
}

void Engine::resetBoundTighteners()
{
}

void Engine::warmStart()
{
    // An NLR is required for a warm start
    if ( !_networkLevelReasoner )
        return;

    // First, choose an arbitrary assignment for the input variables
    unsigned numInputVariables = _preprocessedQuery->getNumInputVariables();
    unsigned numOutputVariables = _preprocessedQuery->getNumOutputVariables();

    if ( numInputVariables == 0 )
    {
        // Trivial case: all inputs are fixed, nothing to evaluate
        return;
    }

    double *inputAssignment = new double[numInputVariables];
    double *outputAssignment = new double[numOutputVariables];

    for ( unsigned i = 0; i < numInputVariables; ++i )
    {
        unsigned variable = _preprocessedQuery->inputVariableByIndex( i );
        inputAssignment[i] = _tableau->getLowerBound( variable );
    }

    // Evaluate the network for this assignment
    _networkLevelReasoner->evaluate( inputAssignment, outputAssignment );

    // Try to update as many variables as possible to match their assignment
    for ( unsigned i = 0; i < _networkLevelReasoner->getNumberOfLayers(); ++i )
    {
        const NLR::Layer *layer = _networkLevelReasoner->getLayer( i );
        unsigned layerSize = layer->getSize();
        const double *assignment = layer->getAssignment();

        for ( unsigned j = 0; j < layerSize; ++j )
        {
            if ( layer->neuronHasVariable( j ) )
            {
                unsigned variable = layer->neuronToVariable( j );
                if ( !_tableau->isBasic( variable ) )
                    _tableau->setNonBasicAssignment( variable, assignment[j], false );
            }
        }
    }

    // We did what we could for the non-basics; now let the tableau compute
    // the basic assignment
    _tableau->computeAssignment();

    delete[] outputAssignment;
    delete[] inputAssignment;
}

void Engine::checkOverallProgress()
{
    // Get fresh statistics
    unsigned numVisitedStates =
        _statistics.getUnsignedAttribute( Statistics::NUM_VISITED_TREE_STATES );
    unsigned long long currentIteration =
        _statistics.getLongAttribute( Statistics::NUM_MAIN_LOOP_ITERATIONS );

    if ( numVisitedStates > _lastNumVisitedStates )
    {
        // Progress has been made
        _lastNumVisitedStates = numVisitedStates;
        _lastIterationWithProgress = currentIteration;
    }
    else
    {
        // No progress has been made. If it's been too long, request a restoration
        if ( currentIteration >
             _lastIterationWithProgress + GlobalConfiguration::MAX_ITERATIONS_WITHOUT_PROGRESS )
        {
            ENGINE_LOG(
                "checkOverallProgress detected cycling. Requesting a precision restoration" );
            _basisRestorationRequired = Engine::STRONG_RESTORATION_NEEDED;
            _lastIterationWithProgress = currentIteration;
        }
    }
}

void Engine::updateDirections()
{
    if ( GlobalConfiguration::USE_POLARITY_BASED_DIRECTION_HEURISTICS )
        for ( const auto &constraint : _plConstraints )
            if ( constraint->supportPolarity() && constraint->isActive() &&
                 !constraint->phaseFixed() )
                constraint->updateDirection();
}

void Engine::decideBranchingHeuristics()
{
    DivideStrategy divideStrategy = Options::get()->getDivideStrategy();
    if ( divideStrategy == DivideStrategy::Auto )
    {
        if ( !_produceUNSATProofs && !_preprocessedQuery->getInputVariables().empty() &&
             _preprocessedQuery->getInputVariables().size() <
                 GlobalConfiguration::INTERVAL_SPLITTING_THRESHOLD )
        {
            // NOTE: the benefit of input splitting is minimal with abstract interpretation
            // disabled. Therefore, since the proof production mode does not currently support that,
            // we do not perform input-splitting in proof production mode.
            divideStrategy = DivideStrategy::LargestInterval;
            if ( _verbosity >= 2 )
                printf( "Branching heuristics set to LargestInterval\n" );
        }
        else
        {
            if ( GlobalConfiguration::USE_DEEPSOI_LOCAL_SEARCH )
            {
                divideStrategy = DivideStrategy::PseudoImpact;
                if ( _verbosity >= 2 )
                    printf( "Branching heuristics set to PseudoImpact\n" );
            }
            else
            {
                divideStrategy = DivideStrategy::Polarity;
                if ( _verbosity >= 2 )
                    printf( "Branching heuristics set to Polarity\n" );
            }
        }
    }
    ASSERT( divideStrategy != DivideStrategy::Auto );
    _searchTreeHandler.setBranchingHeuristics( divideStrategy );
#ifdef BUILD_CADICAL
    _searchTreeHandler.initializeScoreTrackerIfNeeded( _plConstraints, &_cdclCore );
#else
    _searchTreeHandler.initializeScoreTrackerIfNeeded( _plConstraints );
#endif
}

PiecewiseLinearConstraint *Engine::pickSplitPLConstraintBasedOnBaBsrHeuristic()
{
    ENGINE_LOG( Stringf( "Using BaBsr heuristic with normalized scores..." ).ascii() );

    if ( !_networkLevelReasoner )
        return NULL;

    // Get constraints from NLR
    List<PiecewiseLinearConstraint *> constraints =
        _networkLevelReasoner->getConstraintsInTopologicalOrder();

    // Temporary vector to store raw scores for normalization
    Vector<double> rawScores;
    Map<double, PiecewiseLinearConstraint *> scoreToConstraint;

    // Filter for ReLU constraints
    for ( auto &plConstraint : constraints )
    {
        if ( plConstraint->supportBaBsr() && plConstraint->isActive() &&
             !plConstraint->phaseFixed() )
        {
            ReluConstraint *reluConstraint = dynamic_cast<ReluConstraint *>( plConstraint );
            if ( reluConstraint )
            {
                // Set NLR if not already set
                reluConstraint->initializeNLRForBaBSR( _networkLevelReasoner );

                // Calculate heuristic score - bias calculation happens inside computeBaBsr
                plConstraint->updateScoreBasedOnBaBsr();

                // Collect raw scores
                rawScores.append( plConstraint->getScore() );
            }
        }
    }

    // Normalize scores
    if ( !rawScores.empty() )
    {
        double minScore = *std::min_element( rawScores.begin(), rawScores.end() );
        double maxScore = *std::max_element( rawScores.begin(), rawScores.end() );
        double range = maxScore - minScore;

        for ( auto &plConstraint : constraints )
        {
            if ( plConstraint->supportBaBsr() && plConstraint->isActive() &&
                 !plConstraint->phaseFixed() )
            {
                double rawScore = plConstraint->getScore();
                double normalizedScore = range > 0 ? ( rawScore - minScore ) / range : 0;

                // Store normalized score in the map
                scoreToConstraint[normalizedScore] = plConstraint;
                if ( scoreToConstraint.size() >= GlobalConfiguration::BABSR_CANDIDATES_THRESHOLD )
                    break;
            }
        }
    }

    // Split on neuron or constraint with highest normalized score
    if ( !scoreToConstraint.empty() )
    {
        ENGINE_LOG( Stringf( "Normalized score of the picked ReLU: %f",
                             ( *scoreToConstraint.begin() ).first )
                        .ascii() );
        return ( *scoreToConstraint.begin() ).second;
    }
    else
        return NULL;
}

PiecewiseLinearConstraint *Engine::pickSplitPLConstraintBasedOnPolarity()
{
    ENGINE_LOG( Stringf( "Using Polarity-based heuristics..." ).ascii() );

    if ( !_networkLevelReasoner )
        return NULL;

    List<PiecewiseLinearConstraint *> constraints =
        _networkLevelReasoner->getConstraintsInTopologicalOrder();

    Map<double, PiecewiseLinearConstraint *> scoreToConstraint;
    for ( auto &plConstraint : constraints )
    {
        if ( plConstraint->supportPolarity() && plConstraint->isActive() &&
             !plConstraint->phaseFixed() )
        {
            plConstraint->updateScoreBasedOnPolarity();
            scoreToConstraint[plConstraint->getScore()] = plConstraint;
            if ( scoreToConstraint.size() >= GlobalConfiguration::POLARITY_CANDIDATES_THRESHOLD )
                break;
        }
    }
    if ( scoreToConstraint.size() > 0 )
    {
        ENGINE_LOG( Stringf( "Score of the picked ReLU: %f", ( *scoreToConstraint.begin() ).first )
                        .ascii() );
        return ( *scoreToConstraint.begin() ).second;
    }
    else
        return NULL;
}

PiecewiseLinearConstraint *Engine::pickSplitPLConstraintBasedOnTopology()
{
    // We push the first unfixed ReLU in the topology order to the _candidatePlConstraints
    ENGINE_LOG( Stringf( "Using EarliestReLU heuristics..." ).ascii() );

    if ( !_networkLevelReasoner )
        throw MarabouError( MarabouError::NETWORK_LEVEL_REASONER_NOT_AVAILABLE );

    List<PiecewiseLinearConstraint *> constraints =
        _networkLevelReasoner->getConstraintsInTopologicalOrder();

    for ( auto &plConstraint : constraints )
    {
        if ( plConstraint->isActive() && !plConstraint->phaseFixed() )
            return plConstraint;
    }
    return NULL;
}

PiecewiseLinearConstraint *Engine::pickSplitPLConstraintBasedOnIntervalWidth()
{
    // We push the first unfixed ReLU in the topology order to the _candidatePlConstraints
    ENGINE_LOG( Stringf( "Using LargestInterval heuristics..." ).ascii() );

    unsigned inputVariableWithLargestInterval = 0;
    double largestIntervalSoFar = 0;
    for ( const auto &variable : _preprocessedQuery->getInputVariables() )
    {
        double interval = _tableau->getUpperBound( variable ) - _tableau->getLowerBound( variable );
        if ( interval > largestIntervalSoFar )
        {
            inputVariableWithLargestInterval = variable;
            largestIntervalSoFar = interval;
        }
    }

    if ( largestIntervalSoFar == 0 )
        return NULL;
    else
    {
        double mid = ( _tableau->getLowerBound( inputVariableWithLargestInterval ) +
                       _tableau->getUpperBound( inputVariableWithLargestInterval ) ) /
                     2;
        PiecewiseLinearCaseSplit s1;
        s1.storeBoundTightening(
            Tightening( inputVariableWithLargestInterval, mid, Tightening::UB ) );
        PiecewiseLinearCaseSplit s2;
        s2.storeBoundTightening(
            Tightening( inputVariableWithLargestInterval, mid, Tightening::LB ) );

        List<PiecewiseLinearCaseSplit> splits;
        splits.append( s1 );
        splits.append( s2 );
        _disjunctionForSplitting =
            std::unique_ptr<DisjunctionConstraint>( new DisjunctionConstraint( splits ) );
        return _disjunctionForSplitting.get();
    }
}

PiecewiseLinearConstraint *Engine::pickSplitPLConstraint( DivideStrategy strategy )
{
    ENGINE_LOG( Stringf( "Picking a split PLConstraint..." ).ascii() );

    PiecewiseLinearConstraint *candidatePLConstraint = NULL;
    if ( strategy == DivideStrategy::PseudoImpact )
    {
        if ( _context.getLevel() > 3 )
            candidatePLConstraint = _searchTreeHandler.getConstraintsWithHighestScore();
        else if ( !_preprocessedQuery->getInputVariables().empty() &&
                  _preprocessedQuery->getInputVariables().size() <
                      GlobalConfiguration::INTERVAL_SPLITTING_THRESHOLD )
            candidatePLConstraint = pickSplitPLConstraintBasedOnIntervalWidth();
        else
        {
            candidatePLConstraint = pickSplitPLConstraintBasedOnPolarity();
            if ( candidatePLConstraint == NULL )
                candidatePLConstraint = _searchTreeHandler.getConstraintsWithHighestScore();
        }
    }
    else if ( strategy == DivideStrategy::BaBSR )
        candidatePLConstraint = pickSplitPLConstraintBasedOnBaBsrHeuristic();
    else if ( strategy == DivideStrategy::Polarity )
        candidatePLConstraint = pickSplitPLConstraintBasedOnPolarity();
    else if ( strategy == DivideStrategy::EarliestReLU )
        candidatePLConstraint = pickSplitPLConstraintBasedOnTopology();
    else if ( strategy == DivideStrategy::LargestInterval &&
              ( ( _context.getLevel() + 1 ) % GlobalConfiguration::INTERVAL_SPLITTING_FREQUENCY !=
                0 ) )
    {
        // Conduct interval splitting periodically.
        candidatePLConstraint = pickSplitPLConstraintBasedOnIntervalWidth();
    }
    ENGINE_LOG(
        Stringf( ( candidatePLConstraint ? "Picked..."
                                         : "Unable to pick using the current strategy..." ) )
            .ascii() );
    return candidatePLConstraint;
}

PiecewiseLinearConstraint *Engine::pickSplitPLConstraintSnC( SnCDivideStrategy strategy )
{
    PiecewiseLinearConstraint *candidatePLConstraint = NULL;
    if ( strategy == SnCDivideStrategy::Polarity )
        candidatePLConstraint = pickSplitPLConstraintBasedOnPolarity();
    else if ( strategy == SnCDivideStrategy::EarliestReLU )
        candidatePLConstraint = pickSplitPLConstraintBasedOnTopology();

    ENGINE_LOG( Stringf( "Done updating scores..." ).ascii() );
    ENGINE_LOG(
        Stringf( ( candidatePLConstraint ? "Picked..."
                                         : "Unable to pick using the current strategy..." ) )
            .ascii() );
    return candidatePLConstraint;
}

bool Engine::restoreSearchTreeState( SearchTreeState &searchTreeState )
{
    // TODO: this method should be updated as well, when integrating snc with cdcl
    try
    {
        ASSERT( _searchTreeHandler.getStackDepth() == 0 );

        // Step 1: all implied valid splits at root
        for ( auto &validSplit : searchTreeState._impliedValidSplitsAtRoot )
        {
            applySplit( validSplit );
            _searchTreeHandler.recordImpliedValidSplit( validSplit );
        }

        tightenBoundsOnConstraintMatrix();
        _boundManager.propagateTightenings();
        // For debugging purposes
        checkBoundCompliancyWithDebugSolution();
        do
            performSymbolicBoundTightening();
        while ( applyAllValidConstraintCaseSplits() );

        // Step 2: replay the stack
        for ( auto &stackEntry : searchTreeState._stack )
        {
            _searchTreeHandler.replaySearchTreeStackEntry( stackEntry );
            // Do all the bound propagation, and set ReLU constraints to inactive (at
            // least the one corresponding to the _activeSplit applied above).
            tightenBoundsOnConstraintMatrix();

            // For debugging purposes
            checkBoundCompliancyWithDebugSolution();
            do
                performSymbolicBoundTightening();
            while ( applyAllValidConstraintCaseSplits() );
        }
        _boundManager.propagateTightenings();
    }
    catch ( const InfeasibleQueryException & )
    {
        // The current query is unsat, and we need to pop.
        // If we're at level 0, the whole query is unsat.
        if ( _produceUNSATProofs )
        {
            if ( _lpSolverType == LPSolverType::NATIVE )
                explainSimplexFailure();
#ifdef BUILD_CADICAL
            else
                explainGurobiFailure();
#endif
        }

#ifdef BUILD_CADICAL
        if ( _solveWithCDCL && !GlobalConfiguration::CDCL_USE_PROOF_BASED_CLAUSES )
            _cdclCore.addDecisionBasedConflictClause();
#endif

        if ( !_searchTreeHandler.popSplit() )
        {
            if ( _verbosity > 0 )
            {
                printf( "\nEngine::solve: UNSAT query\n" );
                _statistics.print();
            }
            _exitCode = ExitCode::UNSAT;
            for ( PiecewiseLinearConstraint *p : _plConstraints )
                p->setActiveConstraint( true );
            return false;
        }
    }
    return true;
}

void Engine::storeSearchTreeState( SearchTreeState &searchTreeState )
{
    _searchTreeHandler.storeSearchTreeState( searchTreeState );
}

bool Engine::solveWithMILPEncoding( double timeoutInSeconds )
{
    // TODO: this method should be updated when integrating milp with cdcl
    if ( !_initialized )
    {
        initializeSolver();
        _initialized = true;
    }

    try
    {
        if ( _lpSolverType == LPSolverType::NATIVE && _tableau->basisMatrixAvailable() )
        {
            explicitBasisBoundTightening();
            applyAllBoundTightenings();
            applyAllValidConstraintCaseSplits();
        }

        while ( applyAllValidConstraintCaseSplits() )
        {
            performSymbolicBoundTightening();
        }
    }
    catch ( const InfeasibleQueryException & )
    {
        _exitCode = ExitCode::UNSAT;
        return false;
    }

    ENGINE_LOG( "Encoding the input query with Gurobi...\n" );
    _gurobi = std::unique_ptr<GurobiWrapper>( new GurobiWrapper() );
    _tableau->setGurobi( &( *_gurobi ) );
    _milpEncoder = std::unique_ptr<MILPEncoder>( new MILPEncoder( *_tableau ) );
    _milpEncoder->encodeQuery( *_gurobi, *_preprocessedQuery );
    ENGINE_LOG( "Query encoded in Gurobi...\n" );

    double timeoutForGurobi = ( timeoutInSeconds == 0 ? FloatUtils::infinity() : timeoutInSeconds );
    ENGINE_LOG( Stringf( "Gurobi timeout set to %f\n", timeoutForGurobi ).ascii() )
    _gurobi->setTimeLimit( timeoutForGurobi );
    if ( !_sncMode )
        _gurobi->setNumberOfThreads( Options::get()->getInt( Options::NUM_WORKERS ) );
    _gurobi->setVerbosity( _verbosity > 0 );
    _gurobi->solve();

    if ( _gurobi->haveFeasibleSolution() )
    {
        if ( allNonlinearConstraintsHold() )
        {
            _exitCode = ExitCode::SAT;
            return true;
        }
        else
        {
            _exitCode = ExitCode::UNKNOWN;
            return false;
        }
    }
    else if ( _gurobi->infeasible() )
        _exitCode = ExitCode::UNSAT;
    else if ( _gurobi->timeout() )
        _exitCode = ExitCode::TIMEOUT;
    else
        throw NLRError( NLRError::UNEXPECTED_RETURN_STATUS_FROM_GUROBI );
    return false;
}

bool Engine::preprocessingEnabled() const
{
    return _preprocessingEnabled;
}

Preprocessor *Engine::getPreprocessor()
{
    return &_preprocessor;
}

bool Engine::performDeepSoILocalSearch()
{
    ENGINE_LOG( "Performing local search..." );
    struct timespec start = TimeUtils::sampleMicro();
    ASSERT( allVarsWithinBounds() );

    // All the linear constraints have been satisfied at this point.
    // Update the cost function
    _soiManager->initializePhasePattern();

    LinearExpression initialPhasePattern = _soiManager->getCurrentSoIPhasePattern();

    if ( initialPhasePattern.isZero() )
    {
        if ( hasBranchingCandidate() )
            while ( !_searchTreeHandler.needToSplit() )
                _searchTreeHandler.reportRejectedPhasePatternProposal();
        return false;
    }

    if ( _lpSolverType == LPSolverType::NATIVE )
        _costFunctionManager->computeCoreCostFunction(); // TODO ask Andrew
    minimizeHeuristicCost( initialPhasePattern );
    ASSERT( allVarsWithinBounds() );
    _soiManager->updateCurrentPhasePatternForSatisfiedPLConstraints();
    // Always accept the first phase pattern.
    _soiManager->acceptCurrentPhasePattern();
    double costOfLastAcceptedPhasePattern =
        computeHeuristicCost( _soiManager->getCurrentSoIPhasePattern() );

    double costOfProposedPhasePattern = FloatUtils::infinity();
    bool lastProposalAccepted = true;
    while ( !_searchTreeHandler.needToSplit() )
    {
        struct timespec end = TimeUtils::sampleMicro();
        _statistics.incLongAttribute( Statistics::TOTAL_TIME_LOCAL_SEARCH_MICRO,
                                      TimeUtils::timePassed( start, end ) );
        start = end;

        if ( lastProposalAccepted )
        {
            /*
              Check whether the optimal solution to the last accepted phase
              is a real solution. We only check this when the last proposal
              was accepted, because rejected phase pattern must have resulted in
              increase in the SoI cost.

              HW: Another option is to only do this check when
              costOfLastAcceptedPhasePattern is 0, but this might be too strict.
              The overhead is low anyway.
            */
            collectViolatedPlConstraints();
            if ( allPlConstraintsHold() )
            {
                if ( _lpSolverType == LPSolverType::NATIVE &&
                     _tableau->getBasicAssignmentStatus() !=
                         ITableau::BASIC_ASSIGNMENT_JUST_COMPUTED )
                {
                    if ( _verbosity > 0 )
                    {
                        printf( "Before declaring sat, recomputing...\n" );
                    }
                    // Make sure that the assignment is precise before declaring success
                    _tableau->computeAssignment();
                    // If we actually have a real satisfying assignment,
                    return false;
                }
                else
                {
                    ENGINE_LOG( "Performing local search - done." );
                    return true;
                }
            }
            else if ( FloatUtils::isZero( costOfLastAcceptedPhasePattern -
                                          costOfProposedPhasePattern ) ) // TODO: check this fix
                                                                         // with Andrew, and
                                                                         // maybe PR into main
                                                                         // branch of Marabou
            {
                // Corner case: the SoI is minimal but there are still some PL
                // constraints (those not in the SoI) unsatisfied.
                // In this case, we bump up the score of PLConstraints not in
                // the SoI with the hope to branch on them early.
                bumpUpPseudoImpactOfPLConstraintsNotInSoI();
                if ( hasBranchingCandidate() )
                    while ( !_searchTreeHandler.needToSplit() )
                        _searchTreeHandler.reportRejectedPhasePatternProposal();
                return false;
            }
        }

        // No satisfying assignment found for the last accepted phase pattern,
        // propose an update to it.
        _soiManager->proposePhasePatternUpdate();
        minimizeHeuristicCost( _soiManager->getCurrentSoIPhasePattern() );
        _soiManager->updateCurrentPhasePatternForSatisfiedPLConstraints();
        costOfProposedPhasePattern =
            computeHeuristicCost( _soiManager->getCurrentSoIPhasePattern() );

        // We have the "local" effect of change the cost term of some
        // PLConstraints in the phase pattern. Use this information to influence
        // the branching decision.
        updatePseudoImpactWithSoICosts( costOfLastAcceptedPhasePattern,
                                        costOfProposedPhasePattern );

        // Decide whether to accept the last proposal.
        if ( _soiManager->decideToAcceptCurrentProposal( costOfLastAcceptedPhasePattern,
                                                         costOfProposedPhasePattern ) )
        {
            _soiManager->acceptCurrentPhasePattern();
            costOfLastAcceptedPhasePattern = costOfProposedPhasePattern;
            lastProposalAccepted = true;
        }
        else
        {
            _searchTreeHandler.reportRejectedPhasePatternProposal();
            lastProposalAccepted = false;
        }
    }

    ENGINE_LOG( "Performing local search - done" );
    return false;
}

void Engine::minimizeHeuristicCost( const LinearExpression &heuristicCost )
{
    ENGINE_LOG( "Optimizing w.r.t. the current heuristic cost..." );

    if ( _lpSolverType == LPSolverType::GUROBI )
    {
        minimizeCostWithGurobi( heuristicCost );

        ENGINE_LOG(
            Stringf( "Current heuristic cost: %f", _gurobi->getOptimalCostOrObjective() ).ascii() );
    }
    else
    {
        _tableau->toggleOptimization( true );

        _heuristicCost = heuristicCost;

        bool localOptimumReached = false;
        while ( !localOptimumReached )
        {
            DEBUG( _tableau->verifyInvariants() );

            mainLoopStatistics();
            if ( _verbosity > 1 &&
                 _statistics.getLongAttribute( Statistics::NUM_MAIN_LOOP_ITERATIONS ) %
                         _statisticsPrintingFrequency ==
                     0 )
                _statistics.print();

            if ( !allVarsWithinBounds() )
                throw VariableOutOfBoundDuringOptimizationException();

            if ( performPrecisionRestorationIfNeeded() )
                continue;

            ASSERT( allVarsWithinBounds() );

            localOptimumReached = performSimplexStep();
        }
        _tableau->toggleOptimization( false );
        ENGINE_LOG( Stringf( "Current heuristic cost: %f", computeHeuristicCost( heuristicCost ) )
                        .ascii() );
    }

    ENGINE_LOG( "Optimizing w.r.t. the current heuristic cost - done\n" );
}

double Engine::computeHeuristicCost( const LinearExpression &heuristicCost )
{
    return ( _costFunctionManager->computeGivenCostFunctionDirectly( heuristicCost._addends ) +
             heuristicCost._constant );
}

void Engine::updatePseudoImpactWithSoICosts( double costOfLastAcceptedPhasePattern,
                                             double costOfProposedPhasePattern )
{
    ASSERT( _soiManager );

    const List<PiecewiseLinearConstraint *> &constraintsUpdated =
        _soiManager->getConstraintsUpdatedInLastProposal();
    // Score is divided by the number of updated constraints in the last
    // proposal. In the Sum of Infeasibilities paper, only one constraint
    // is updated each time. But we might consider alternative proposal
    // strategy in the future.
    double score = ( fabs( costOfLastAcceptedPhasePattern - costOfProposedPhasePattern ) /
                     constraintsUpdated.size() );

    ASSERT( constraintsUpdated.size() > 0 );
    // Update the Pseudo-Impact estimation.
    for ( const auto &constraint : constraintsUpdated )
        _searchTreeHandler.updatePLConstraintScore( constraint, score );
}

void Engine::bumpUpPseudoImpactOfPLConstraintsNotInSoI()
{
    ASSERT( _soiManager );
    for ( const auto &plConstraint : _plConstraints )
    {
        if ( plConstraint->isActive() && !plConstraint->supportSoI() &&
             !plConstraint->phaseFixed() && !plConstraint->satisfied() )
            _searchTreeHandler.updatePLConstraintScore(
                plConstraint, GlobalConfiguration::SCORE_BUMP_FOR_PL_CONSTRAINTS_NOT_IN_SOI );
    }
}

void Engine::informLPSolverOfBounds()
{
    if ( _lpSolverType == LPSolverType::GUROBI )
    {
        struct timespec start = TimeUtils::sampleMicro();
        for ( unsigned i = 0; i < _preprocessedQuery->getNumberOfVariables(); ++i )
        {
            String variableName = _milpEncoder->getVariableNameFromVariable( i );
            _gurobi->setLowerBound( variableName, _tableau->getLowerBound( i ) );
            _gurobi->setUpperBound( variableName, _tableau->getUpperBound( i ) );
        }
        _gurobi->updateModel();
        struct timespec end = TimeUtils::sampleMicro();
        _statistics.incLongAttribute( Statistics::TIME_ADDING_CONSTRAINTS_TO_MILP_SOLVER_MICRO,
                                      TimeUtils::timePassed( start, end ) );
    }
    else
    {
        // Bounds are already up-to-date in Tableau when using native Simplex.
        return;
    }
}

bool Engine::minimizeCostWithGurobi( const LinearExpression &costFunction )
{
    ASSERT( _gurobi && _milpEncoder );

    struct timespec simplexStart = TimeUtils::sampleMicro();

    _milpEncoder->encodeCostFunction( *_gurobi, costFunction );
    _gurobi->setTimeLimit( FloatUtils::infinity() );
    _gurobi->solve();

    struct timespec simplexEnd = TimeUtils::sampleMicro();

    _statistics.incLongAttribute( Statistics::TIME_SIMPLEX_STEPS_MICRO,
                                  TimeUtils::timePassed( simplexStart, simplexEnd ) );
    _statistics.incLongAttribute( Statistics::NUM_SIMPLEX_STEPS,
                                  _gurobi->getNumberOfSimplexIterations() );

    if ( _gurobi->infeasible() )
        throw InfeasibleQueryException();
    else if ( _gurobi->optimal() )
        return true;
    else
        throw CommonError( CommonError::UNEXPECTED_GUROBI_STATUS,
                           Stringf( "Current status: %u", _gurobi->getStatusCode() ).ascii() );

    return false;
}

void Engine::checkGurobiBoundConsistency() const
{
    if ( _gurobi && _milpEncoder )
    {
        for ( unsigned i = 0; i < _preprocessedQuery->getNumberOfVariables(); ++i )
        {
            String iName = _milpEncoder->getVariableNameFromVariable( i );
            double gurobiLowerBound = _gurobi->getLowerBound( iName );
            double lowerBound = _tableau->getLowerBound( i );
            if ( !FloatUtils::areEqual( gurobiLowerBound, lowerBound ) )
            {
                throw MarabouError( MarabouError::BOUNDS_NOT_UP_TO_DATE_IN_LP_SOLVER,
                                    Stringf( "x%u lower bound inconsistent!"
                                             " Gurobi: %f, Tableau: %f",
                                             i,
                                             gurobiLowerBound,
                                             lowerBound )
                                        .ascii() );
            }
            double gurobiUpperBound = _gurobi->getUpperBound( iName );
            double upperBound = _tableau->getUpperBound( i );

            if ( !FloatUtils::areEqual( gurobiUpperBound, upperBound ) )
            {
                throw MarabouError( MarabouError::BOUNDS_NOT_UP_TO_DATE_IN_LP_SOLVER,
                                    Stringf( "x%u upper bound inconsistent!"
                                             " Gurobi: %f, Tableau: %f",
                                             i,
                                             gurobiUpperBound,
                                             upperBound )
                                        .ascii() );
            }
        }
    }
}

bool Engine::consistentBounds() const
{
    return _boundManager.consistentBounds();
}

Query Engine::buildQueryFromCurrentState() const
{
    Query query = *_preprocessedQuery;
    for ( unsigned i = 0; i < query.getNumberOfVariables(); ++i )
    {
        query.setLowerBound( i, _tableau->getLowerBound( i ) );
        query.setUpperBound( i, _tableau->getUpperBound( i ) );
    }
    return query;
}

double Engine::getGroundBound( unsigned var, bool isUpper ) const
{
    ASSERT( var < _tableau->getN() && _produceUNSATProofs );
    return _groundBoundManager.getGroundBound( var, isUpper ? Tightening::UB : Tightening::LB );
}

std::shared_ptr<GroundBoundManager::GroundBoundEntry>
Engine::getGroundBoundEntry( unsigned var, bool isUpper ) const
{
    ASSERT( var < _tableau->getN() && _produceUNSATProofs );
    return _groundBoundManager.getGroundBoundEntry( var,
                                                    isUpper ? Tightening::UB : Tightening::LB );
}

bool Engine::shouldProduceProofs() const
{
    return _produceUNSATProofs;
}

void Engine::explainSimplexFailure()
{
    ASSERT( _produceUNSATProofs && _lpSolverType == LPSolverType::NATIVE );
#ifdef BUILD_CADICAL
    ASSERT( !_solveWithCDCL || !_cdclCore.hasConflictClause() );
#endif
    DEBUG( checkGroundBounds() );

    unsigned infeasibleVar = _boundManager.getInconsistentVariable();

    if ( infeasibleVar == IBoundManager::NO_VARIABLE_FOUND ||
         !certifyInfeasibility( infeasibleVar ) )
        infeasibleVar = explainFailureWithTableau();

    if ( infeasibleVar == IBoundManager::NO_VARIABLE_FOUND )
        infeasibleVar = explainFailureWithCostFunction();

    if ( infeasibleVar == IBoundManager::NO_VARIABLE_FOUND )
    {
        _costFunctionManager->computeCoreCostFunction();
        infeasibleVar = explainFailureWithCostFunction();
    }

    if ( infeasibleVar == IBoundManager::NO_VARIABLE_FOUND )
    {
        markLeafToDelegate();
#ifdef BUILD_CADICAL
        if ( _solveWithCDCL )
            _cdclCore.addDecisionBasedConflictClause();
#endif
        return;
    }

    ASSERT( infeasibleVar < _tableau->getN() );
    if ( !_solveWithCDCL )
        ASSERT( _UNSATCertificateCurrentPointer &&
                !( **_UNSATCertificateCurrentPointer ).getContradiction() );

    _statistics.incUnsignedAttribute( Statistics::NUM_CERTIFIED_LEAVES );

    Vector<double> leafContradictionVec = computeContradiction( infeasibleVar );

    if ( !_solveWithCDCL )
    {
        writeContradictionToCertificate( leafContradictionVec, infeasibleVar );

        ( **_UNSATCertificateCurrentPointer ).makeLeaf();
        SparseUnsortedList sparseContradictionToAnalyse = SparseUnsortedList();


        leafContradictionVec.empty()
            ? sparseContradictionToAnalyse.initializeToEmpty()
            : sparseContradictionToAnalyse.initialize( leafContradictionVec.data(),
                                                       leafContradictionVec.size() );

        analyseExplanationDependencies(
            sparseContradictionToAnalyse, _groundBoundManager.getCounter(), -1, true );

        return;
    }
#ifdef BUILD_CADICAL
    // If both bounds are ground bounds, explanation would be empty and the clause trivial
    if ( _boundManager.getUpperBound( infeasibleVar ) ==
             getGroundBound( infeasibleVar, Tightening::UB ) &&
         _boundManager.getLowerBound( infeasibleVar ) ==
             getGroundBound( infeasibleVar, Tightening::LB ) )
    {
        _cdclCore.addDecisionBasedConflictClause();
        return;
    }
    if ( GlobalConfiguration::CDCL_USE_PROOF_BASED_CLAUSES )
    {
        SparseUnsortedList sparseContradiction( leafContradictionVec.data(),
                                                leafContradictionVec.size() );
        Set<int> clause = clauseFromContradictionVector(
            sparseContradiction, _groundBoundManager.getCounter(), -1, true );

        if ( GlobalConfiguration::CDCL_REDUCE_CLAUSE_SIZE_WITH_PROOF )
        {
            // If possible, attempt to reduce the clause size
            if ( clause.size() > 1 && checkClauseWithProof( sparseContradiction, clause, NULL ) )
                clause = reduceClauseSizeWithProof(
                    sparseContradiction, Vector<int>( clause.begin(), clause.end() ), NULL );
        }

        _cdclCore.addExternalClause( clause );
    }
    else
        _cdclCore.addDecisionBasedConflictClause();
#endif
}

bool Engine::certifyInfeasibility( unsigned var ) const
{
    ASSERT( _produceUNSATProofs );

    Vector<double> contradiction = computeContradiction( var );

    if ( contradiction.empty() )
        return FloatUtils::isNegative( _groundBoundManager.getGroundBound( var, Tightening::UB ) -
                                       _groundBoundManager.getGroundBound( var, Tightening::LB ) );

    SparseUnsortedList sparseContradiction = SparseUnsortedList();

    contradiction.empty()
        ? sparseContradiction.initializeToEmpty()
        : sparseContradiction.initialize( contradiction.data(), contradiction.size() );

    // In case contradiction is a vector of zeros
    if ( sparseContradiction.empty() )
        return FloatUtils::isNegative( _groundBoundManager.getGroundBound( var, Tightening::UB ) -
                                       _groundBoundManager.getGroundBound( var, Tightening::LB ) );

    double derivedBound = UNSATCertificateUtils::computeCombinationUpperBound(
        sparseContradiction,
        _tableau->getSparseA(),
        _groundBoundManager.getAllGroundBounds( Tightening::UB ).data(),
        _groundBoundManager.getAllGroundBounds( Tightening::LB ).data(),
        _tableau->getN() );
    return FloatUtils::isNegative( derivedBound );
}

double Engine::explainBound( unsigned var, bool isUpper ) const
{
    ASSERT( _produceUNSATProofs );

    SparseUnsortedList explanation( 0 );

    if ( !_boundManager.isExplanationTrivial( var, isUpper ) )
        explanation = _boundManager.getExplanation( var, isUpper );

    if ( explanation.empty() )
        return _groundBoundManager.getGroundBound( var, isUpper ? Tightening::UB : Tightening::LB );

    return UNSATCertificateUtils::computeBound(
        var,
        isUpper,
        explanation,
        _tableau->getSparseA(),
        _groundBoundManager.getAllGroundBounds( Tightening::UB ).data(),
        _groundBoundManager.getAllGroundBounds( Tightening::LB ).data(),
        _tableau->getN() );
}

bool Engine::validateBounds( unsigned var, double epsilon, bool isUpper ) const
{
    ASSERT( _produceUNSATProofs );

    double explained, real;
    explained = explainBound( var, isUpper );
    if ( isUpper )
    {
        real = _boundManager.getUpperBound( var );
        if ( explained - real > epsilon )
        {
            ENGINE_LOG( "Var %d. Computed Upper %.5lf, real %.5lf. Difference is %.10lf\n",
                        var,
                        explained,
                        real,
                        abs( explained - real ) );
            return false;
        }
    }
    else
    {
        real = _boundManager.getLowerBound( var );
        if ( explained - real < -epsilon )
        {
            ENGINE_LOG( "Var %d. Computed Lower  %.5lf, real %.5lf. Difference is %.10lf\n",
                        var,
                        explained,
                        real,
                        abs( explained - real ) );
            return false;
        }
    }
    return true;
}

bool Engine::validateAllBounds( double epsilon ) const
{
    ASSERT( _produceUNSATProofs );

    bool res = true;

    for ( unsigned var = 0; var < _tableau->getN(); ++var )
        if ( !validateBounds( var, epsilon, Tightening::UB ) ||
             !validateBounds( var, epsilon, Tightening::LB ) )
            res = false;

    return res;
}

bool Engine::checkGroundBounds() const
{
    ASSERT( _produceUNSATProofs );

    for ( unsigned i = 0; i < _tableau->getN(); ++i )
    {
        if ( FloatUtils::gt( _groundBoundManager.getGroundBound( i, Tightening::UB ),
                             _boundManager.getLowerBound( i ) ) ||
             FloatUtils::lt( _groundBoundManager.getGroundBound( i, Tightening::LB ),
                             _boundManager.getUpperBound( i ) ) )
            return false;
    }
    return true;
}

unsigned Engine::explainFailureWithTableau()
{
    ASSERT( _produceUNSATProofs );

    // Failure of a simplex step implies infeasible bounds imposed by the row
    TableauRow boundUpdateRow = TableauRow( _tableau->getN() );

    //  For every basic, check that is has no slack and its explanations indeed prove a
    //  contradiction
    unsigned basicVar;

    for ( unsigned i = 0; i < _tableau->getM(); ++i )
    {
        if ( _tableau->basicOutOfBounds( i ) )
        {
            _tableau->getTableauRow( i, &boundUpdateRow );
            basicVar = boundUpdateRow._lhs;

            if ( FloatUtils::gt( _boundManager.computeRowBound( boundUpdateRow, Tightening::LB ),
                                 _boundManager.getUpperBound( basicVar ) ) &&
                 explainAndCheckContradiction( basicVar, Tightening::LB, &boundUpdateRow ) )
                return basicVar;

            if ( FloatUtils::lt( _boundManager.computeRowBound( boundUpdateRow, Tightening::UB ),
                                 _boundManager.getLowerBound( basicVar ) ) &&
                 explainAndCheckContradiction( basicVar, Tightening::UB, &boundUpdateRow ) )
                return basicVar;
        }
    }

    return IBoundManager::NO_VARIABLE_FOUND;
}

unsigned Engine::explainFailureWithCostFunction()
{
    ASSERT( _produceUNSATProofs );

    // Failure of a simplex step might imply infeasible bounds imposed by the cost function
    unsigned curBasicVar;
    unsigned infVar = IBoundManager::NO_VARIABLE_FOUND;
    double curCost;
    bool curUpper;
    const SparseUnsortedList *costRow = _costFunctionManager->createRowOfCostFunction();

    for ( unsigned i = 0; i < _tableau->getM(); ++i )
    {
        curBasicVar = _tableau->basicIndexToVariable( i );
        curCost = _costFunctionManager->getBasicCost( i );

        if ( FloatUtils::isZero( curCost ) )
            continue;

        curUpper = ( curCost < 0 );

        // Check the basic variable has no slack
        if ( !( !curUpper && FloatUtils::gt( _boundManager.computeSparseRowBound(
                                                 *costRow, Tightening::LB, curBasicVar ),
                                             _boundManager.getUpperBound( curBasicVar ) ) ) &&
             !( curUpper && FloatUtils::lt( _boundManager.computeSparseRowBound(
                                                *costRow, Tightening::UB, curBasicVar ),
                                            _boundManager.getLowerBound( curBasicVar ) ) ) )

            continue;

        // Check the explanation indeed proves a contradiction
        if ( explainAndCheckContradiction( curBasicVar, curUpper, costRow ) )
        {
            infVar = curBasicVar;
            break;
        }
    }

    delete costRow;
    return infVar;
}

bool Engine::explainAndCheckContradiction( unsigned var, bool isUpper, const TableauRow *row )
{
    ASSERT( _produceUNSATProofs );

    SparseUnsortedList backup( 0 );
    backup = _boundManager.getExplanation( var, isUpper );

    _boundManager.updateBoundExplanation( *row, isUpper, var );

    // Ensure the proof is correct
    if ( certifyInfeasibility( var ) )
        return true;

    // If not, restores previous certificate if the proof is wrong
    _boundManager.setExplanation( backup, var, isUpper );

    return false;
}

bool Engine::explainAndCheckContradiction( unsigned var,
                                           bool isUpper,
                                           const SparseUnsortedList *row )
{
    ASSERT( _produceUNSATProofs );

    SparseUnsortedList backup( 0 );
    backup = _boundManager.getExplanation( var, isUpper );

    _boundManager.updateBoundExplanationSparse( *row, isUpper, var );

    // Ensure the proof is correct
    if ( certifyInfeasibility( var ) )
        return true;

    // If not, restores previous certificate if the proof is wrong
    _boundManager.setExplanation( backup, var, isUpper );

    return false;
}

UnsatCertificateNode *Engine::getUNSATCertificateCurrentPointer() const
{
    return _UNSATCertificateCurrentPointer->get();
}

void Engine::setUNSATCertificateCurrentPointer( UnsatCertificateNode *node )
{
    _UNSATCertificateCurrentPointer->set( node );
}

const UnsatCertificateNode *Engine::getUNSATCertificateRoot() const
{
    return _UNSATCertificate;
}

bool Engine::certifyUNSATCertificate()
{
    ASSERT( _produceUNSATProofs && _UNSATCertificate && !_searchTreeHandler.getStackDepth() &&
            !_solveWithCDCL );

    for ( auto &constraint : _plConstraints )
    {
        if ( !UNSATCertificateUtils::getSupportedActivations().exists( constraint->getType() ) )
        {
            String activationType = constraint->serializeToString().tokenize( "," ).back();
            printf( "Certification Error! Network contains activation function %s, that is not yet "
                    "supported by Marabou certification.\n",
                    activationType.ascii() );
            return false;
        }
    }

    struct timespec certificationStart = TimeUtils::sampleMicro();
    _precisionRestorer.restoreInitialEngineState( *this );

    Vector<double> groundUpperBounds( _tableau->getN(), 0 );
    Vector<double> groundLowerBounds( _tableau->getN(), 0 );

    for ( unsigned i = 0; i < _tableau->getN(); ++i )
    {
        groundUpperBounds[i] = _groundBoundManager.getGroundBound( i, Tightening::UB );
        groundLowerBounds[i] = _groundBoundManager.getGroundBound( i, Tightening::LB );
    }

    if ( GlobalConfiguration::WRITE_JSON_PROOF )
    {
        File file( JsonWriter::PROOF_FILENAME );
        JsonWriter::writeProofToJson( _UNSATCertificate,
                                      _tableau->getM(),
                                      _tableau->getSparseA(),
                                      groundUpperBounds,
                                      groundLowerBounds,
                                      _plConstraints,
                                      file );
    }

    Checker unsatCertificateChecker( _UNSATCertificate,
                                     _tableau->getM(),
                                     _tableau->getSparseA(),
                                     groundUpperBounds,
                                     groundLowerBounds,
                                     _plConstraints );
    bool certificationSucceeded = unsatCertificateChecker.check();

    _statistics.setLongAttribute(
        Statistics::TOTAL_CERTIFICATION_TIME,
        TimeUtils::timePassed( certificationStart, TimeUtils::sampleMicro() ) );
    printf( "Certification time: " );
    _statistics.printLongAttributeAsTime(
        _statistics.getLongAttribute( Statistics::TOTAL_CERTIFICATION_TIME ) );

    if ( certificationSucceeded )
    {
        printf( "Certified\n" );
        _statistics.incUnsignedAttribute( Statistics::CERTIFIED_UNSAT );
        if ( _statistics.getUnsignedAttribute( Statistics::NUM_DELEGATED_LEAVES ) )
            printf( "Some leaves were delegated and need to be certified separately by an SMT "
                    "solver\n" );
    }
    else
        printf( "Error certifying UNSAT certificate\n" );

    DEBUG( {
        ASSERT( certificationSucceeded );
        if ( _statistics.getUnsignedAttribute( Statistics::NUM_POPS ) )
        {
            double delegationRatio =
                _statistics.getUnsignedAttribute( Statistics::NUM_DELEGATED_LEAVES ) /
                _statistics.getUnsignedAttribute( Statistics::NUM_CERTIFIED_LEAVES );
            ASSERT( FloatUtils::lt( delegationRatio, 0.01 ) );
        }
    } );

    return certificationSucceeded;
}

void Engine::markLeafToDelegate()
{
    ASSERT( _produceUNSATProofs );
    UnsatCertificateNode *currentUnsatCertificateNode = NULL;
    if ( !_solveWithCDCL )
    {
        // Mark leaf with toDelegate Flag
        currentUnsatCertificateNode = _UNSATCertificateCurrentPointer->get();
        ASSERT( _UNSATCertificateCurrentPointer &&
                !currentUnsatCertificateNode->getContradiction() );
        currentUnsatCertificateNode->setDelegationStatus( DelegationStatus::DELEGATE_DONT_SAVE );
        currentUnsatCertificateNode->deletePLCExplanations();
    }

    _statistics.incUnsignedAttribute( Statistics::NUM_DELEGATED_LEAVES );

    if ( !_solveWithCDCL && currentUnsatCertificateNode->getChildren().empty() )
        currentUnsatCertificateNode->makeLeaf();
}

const Vector<double> Engine::computeContradiction( unsigned infeasibleVar ) const
{
    ASSERT( _produceUNSATProofs );

    unsigned m = _tableau->getM();
    SparseUnsortedList upperBoundExplanation( 0 );
    SparseUnsortedList lowerBoundExplanation( 0 );

    if ( !_boundManager.isExplanationTrivial( infeasibleVar, Tightening::UB ) )
        upperBoundExplanation = _boundManager.getExplanation( infeasibleVar, Tightening::UB );

    if ( !_boundManager.isExplanationTrivial( infeasibleVar, Tightening::LB ) )
        lowerBoundExplanation = _boundManager.getExplanation( infeasibleVar, Tightening::LB );

    if ( upperBoundExplanation.empty() && lowerBoundExplanation.empty() )
        return Vector<double>( 0 );

    Vector<double> contradiction = Vector<double>( m, 0 );

    if ( !upperBoundExplanation.empty() )
        for ( const auto &entry : upperBoundExplanation )
            contradiction[entry._index] = entry._value;

    if ( !lowerBoundExplanation.empty() )
        for ( const auto &entry : lowerBoundExplanation )
            contradiction[entry._index] -= entry._value;

    return contradiction;
}

void Engine::writeContradictionToCertificate( const Vector<double> &contradiction,
                                              unsigned infeasibleVar ) const
{
    ASSERT( _produceUNSATProofs );

    Contradiction *leafContradiction = contradiction.empty() ? new Contradiction( infeasibleVar )
                                                             : new Contradiction( contradiction );
    ( **_UNSATCertificateCurrentPointer ).setContradiction( leafContradiction );
}

const BoundExplainer *Engine::getBoundExplainer() const
{
    return _boundManager.getBoundExplainer();
}

void Engine::setBoundExplainerContent( BoundExplainer *boundExplainer )
{
    _boundManager.copyBoundExplainerContent( boundExplainer );
}

bool Engine::propagateBoundManagerTightenings()
{
    ASSERT( _lpSolverType == LPSolverType::NATIVE );
    try
    {
        _boundManager.propagateTightenings();
        if ( !consistentBounds() )
        {
            if ( _produceUNSATProofs )
                explainSimplexFailure();
#ifdef BUILD_CADICAL
            else if ( _solveWithCDCL && !GlobalConfiguration::CDCL_USE_PROOF_BASED_CLAUSES )
                _cdclCore.addDecisionBasedConflictClause();
#endif
        }

        return consistentBounds();
    }
    catch ( InfeasibleQueryException )
    {
        if ( _produceUNSATProofs )
            explainSimplexFailure();
#ifdef BUILD_CADICAL
        else if ( _solveWithCDCL && !GlobalConfiguration::CDCL_USE_PROOF_BASED_CLAUSES )
            _cdclCore.addDecisionBasedConflictClause();
#endif
        return false;
    }
}

void Engine::extractBounds( IQuery &inputQuery )
{
    for ( unsigned i = 0; i < inputQuery.getNumberOfVariables(); ++i )
    {
        if ( _preprocessingEnabled )
        {
            // Has the variable been merged into another?
            unsigned variable = i;
            while ( _preprocessor.variableIsMerged( variable ) )
                variable = _preprocessor.getMergedIndex( variable );

            // Symbolically fixed variables are ignored
            if ( _preprocessor.variableIsUnusedAndSymbolicallyFixed( i ) )
            {
                inputQuery.tightenLowerBound( i, FloatUtils::negativeInfinity() );
                inputQuery.tightenUpperBound( i, FloatUtils::infinity() );
                continue;
            }

            // Fixed variables are easy: return the value they've been fixed to.
            if ( _preprocessor.variableIsFixed( variable ) )
            {
                inputQuery.tightenLowerBound( i, _preprocessor.getFixedValue( variable ) );
                inputQuery.tightenUpperBound( i, _preprocessor.getFixedValue( variable ) );
                continue;
            }

            // We know which variable to look for, but it may have been assigned
            // a new index, due to variable elimination
            variable = _preprocessor.getNewIndex( variable );

            inputQuery.tightenLowerBound( i, _preprocessedQuery->getLowerBound( variable ) );
            inputQuery.tightenUpperBound( i, _preprocessedQuery->getUpperBound( variable ) );
        }
        else
        {
            inputQuery.tightenLowerBound( i, _preprocessedQuery->getLowerBound( i ) );
            inputQuery.tightenUpperBound( i, _preprocessedQuery->getUpperBound( i ) );
        }
    }
}

void Engine::incNumOfLemmas()
{
    if ( !_produceUNSATProofs )
        return;

    ASSERT( _UNSATCertificate && _UNSATCertificateCurrentPointer )
    _statistics.incUnsignedAttribute( Statistics::NUM_LEMMAS );
}

void Engine::assertEngineBoundsForSplit( const PiecewiseLinearCaseSplit &split )
{
    for ( const auto &bound : split.getBoundTightenings() )
    {
        if ( bound._type == Tightening::UB )
            ASSERT(
                FloatUtils::lte( _boundManager.getUpperBound( bound._variable ), bound._value ) );

        if ( bound._type == Tightening::LB )
            ASSERT(
                FloatUtils::gte( _boundManager.getLowerBound( bound._variable ), bound._value ) );
    }
}

unsigned Engine::getVerbosity() const
{
    return _verbosity;
}

ExitCode Engine::getExitCode() const
{
    return _exitCode;
}

void Engine::setExitCode( ExitCode exitCode )
{
    _exitCode = exitCode;
}

const List<PiecewiseLinearConstraint *> *Engine::getPiecewiseLinearConstraints() const
{
    return &_plConstraints;
}

LPSolverType Engine::getLpSolverType() const
{
    return _lpSolverType;
}

NLR::NetworkLevelReasoner *Engine::getNetworkLevelReasoner() const
{
    return _networkLevelReasoner;
}

bool Engine::shouldSolveWithMILP() const
{
    return _solveWithMILP;
}

std::shared_ptr<GroundBoundManager::GroundBoundEntry>
Engine::setGroundBoundFromLemma( const std::shared_ptr<PLCLemma> lemma, bool isPhaseFixing )
{
    return _groundBoundManager.addGroundBound( lemma, isPhaseFixing );
}

bool Engine::shouldSolveWithCDCL() const
{
    return _solveWithCDCL;
}

Set<std::shared_ptr<GroundBoundManager::GroundBoundEntry>>
Engine::analyseExplanationDependencies( const SparseUnsortedList &explanation,
                                        unsigned id,
                                        int explainedVar,
                                        bool isUpper )
{
    Vector<double> linearCombination( 0 );
    UNSATCertificateUtils::getExplanationRowCombination(
        explanation, linearCombination, _tableau->getSparseA(), _tableau->getN() );

    if ( explainedVar >= 0 )
        linearCombination[explainedVar]++;

    Set<std::shared_ptr<GroundBoundManager::GroundBoundEntry>> entries =
        Set<std::shared_ptr<GroundBoundManager::GroundBoundEntry>>();

    Vector<std::tuple<double, std::shared_ptr<GroundBoundManager::GroundBoundEntry>>>
        contributions =
            Vector<std::tuple<double, std::shared_ptr<GroundBoundManager::GroundBoundEntry>>>();

    Vector<double> gub = Vector<double>( _tableau->getN(), 0 );
    Vector<double> glb = Vector<double>( _tableau->getN(), 0 );


    for ( unsigned i = 0; i < _tableau->getN(); ++i )
    {
        gub[i] = _groundBoundManager.getGroundBoundUpToId( i, Tightening::UB, id );
        glb[i] = _groundBoundManager.getGroundBoundUpToId( i, Tightening::LB, id );
    }

    // Iterate through all lemmas learned, check which participated in the explanation
    for ( unsigned var = 0; var < linearCombination.size(); ++var )
    {
        if ( !FloatUtils::isZero( linearCombination[var] ) )
        {
            Tightening::BoundType btype = ( ( linearCombination[var] > 0 ) && isUpper ) ||
                                                  ( ( linearCombination[var] < 0 ) && !isUpper )
                                            ? Tightening::UB
                                            : Tightening::LB;
            std::shared_ptr<GroundBoundManager::GroundBoundEntry> entry =
                _groundBoundManager.getGroundBoundEntryUpToId( var, btype, id );
            if ( entry->isPhaseFixing )
            {
                double contribution =
                    ( entry->val - _groundBoundManager.getGroundBoundUpToId( var, btype, 0 ) ) *
                    linearCombination[var];

                entries.insert( entry );

                contributions.append( std::make_tuple( contribution, entry ) );
            }
        }
    }

    double explanationBound = isUpper
                                ? UNSATCertificateUtils::computeCombinationUpperBound(
                                      linearCombination, gub.data(), glb.data(), _tableau->getN() )
                                : UNSATCertificateUtils::computeCombinationLowerBound(
                                      linearCombination, gub.data(), glb.data(), _tableau->getN() );

    std::sort( contributions.begin(),
               contributions.end(),
               []( std::tuple<double, std::shared_ptr<GroundBoundManager::GroundBoundEntry>> a,
                   std::tuple<double, std::shared_ptr<GroundBoundManager::GroundBoundEntry>> b ) {
                   return abs( std::get<0>( a ) ) < abs( std::get<0>( b ) );
               } );

    double overallContributions = 0;
    for ( const auto &contribution : contributions )
    {
        overallContributions += abs( std::get<0>( contribution ) );
        if ( FloatUtils::lt( overallContributions,
                             abs( explanationBound ),
                             GlobalConfiguration::LEMMA_CERTIFICATION_TOLERANCE ) )
            entries.erase( std::get<1>( contribution ) );
        else
            break;
    }

    if ( _solveWithCDCL )
        return entries;

    for ( const auto &entry : entries )
    {
        ASSERT( entry->id < id );

        Set<std::shared_ptr<GroundBoundManager::GroundBoundEntry>> minorDepList;
        if ( entry->lemma && !entry->lemma->getExplanations().empty() &&
             !entry->lemma->getExplanations().front().empty() && entry->depList.empty() &&
             !entry->lemma->getToCheck() )
        {
            _statistics.incUnsignedAttribute( Statistics::NUM_LEMMAS_USED );

            minorDepList = analyseExplanationDependencies( entry->lemma->getExplanations().back(),
                                                           entry->id,
                                                           entry->lemma->getCausingVars().back(),
                                                           entry->lemma->getCausingVarBound() ==
                                                               Tightening::UB );

            _groundBoundManager.addDepListToGroundBoundEntry( entry, minorDepList );
            entry->lemma->setToCheck();
        }
        else
            minorDepList = entry->depList;

        for ( const auto &minorEntry : minorDepList )
            entries.insert( minorEntry );
    }

    return entries;
}
#ifdef BUILD_CADICAL

bool Engine::solveWithCDCL( double timeoutInSeconds )
{
    return _cdclCore.solveWithCDCL( timeoutInSeconds );
}

Set<int> Engine::clauseFromContradictionVector( const SparseUnsortedList &explanation,
                                                unsigned id,
                                                int explainedVar,
                                                bool isUpper )
{
    ASSERT( _solveWithCDCL );
    ASSERT( _nlConstraints.empty() && !explanation.empty() );
    Set<int> clause = Set<int>();

    Vector<double> linearCombination( 0 );
    UNSATCertificateUtils::getExplanationRowCombination(
        explanation, linearCombination, _tableau->getSparseA(), _tableau->getN() );

    if ( explainedVar >= 0 )
        linearCombination[explainedVar]++;

    int lit;
    int decisionCounter = 0;

    // Iterate through all constraints, check whether their phase was involved in the explanation
    // Propagate literals accordingly
    // Currently works only for ReLU constraints
    for ( const auto &constraint : _plConstraints )
    {
        lit = 0;
        // TODO support max and disjunction
        ASSERT( constraint->getType() != DISJUNCTION && constraint->getType() != MAX );
        if ( constraint->getPhaseFixingEntry() && constraint->getPhaseFixingEntry()->id < id )
        {
            for ( unsigned var : constraint->getParticipatingVariables() )
                if ( !FloatUtils::isZero( linearCombination[var] ) )
                {
                    Tightening::BoundType boundTypeParticipating =
                        ( ( linearCombination[var] > 0 ) && isUpper ) ||
                                ( ( linearCombination[var] < 0 ) && !isUpper )
                            ? Tightening::UB
                            : Tightening::LB;
                    std::shared_ptr<GroundBoundManager::GroundBoundEntry> entry =
                        _groundBoundManager.getGroundBoundEntryUpToId(
                            var, boundTypeParticipating, id );

                    if ( entry->isPhaseFixing &&
                         constraint->isBoundFixingPhase(
                             var, entry->val, boundTypeParticipating ) &&
                         !entry->lemma )
                    {
                        ++decisionCounter;
                        lit = constraint->propagatePhaseAsLit();
                        break;
                    }
                }
        }

        if ( lit )
        {
            ASSERT( !clause.exists( -lit ) );
            ASSERT( constraint->phaseFixed() || !constraint->isActive() )
            clause.insert( lit );
        }
    }

    if ( decisionCounter >= _context.getLevel() )
        return clause;

    Set<std::shared_ptr<GroundBoundManager::GroundBoundEntry>> entries =
        analyseExplanationDependencies( explanation, id, explainedVar, isUpper );

    for ( const auto &entry : entries )
    {
        ASSERT( entry->id < id );
        Set<int> minorClause;
        if ( entry->lemma && !entry->lemma->getExplanations().empty() &&
             !entry->lemma->getExplanations().front().empty() && entry->clause.empty() )
        {
            minorClause = clauseFromContradictionVector( entry->lemma->getExplanations().back(),
                                                         entry->id,
                                                         entry->lemma->getCausingVars().back(),
                                                         entry->lemma->getCausingVarBound() ==
                                                             Tightening::UB );

            _groundBoundManager.addClauseToGroundBoundEntry( entry, minorClause );
            _statistics.incUnsignedAttribute( Statistics::NUM_LEMMAS_USED );
        }
        else
            minorClause = entry->clause;

        decisionCounter = 0;
        for ( int literal : minorClause )
        {
            ASSERT( literal && !clause.exists( -literal ) );
            clause.insert( literal );
            if ( _cdclCore.isDecision( literal ) )
                ++decisionCounter;
            if ( decisionCounter >= _context.getLevel() )
                return minorClause;
        }

        decisionCounter = 0;
        for ( const auto &clauseLit : clause )
            if ( _cdclCore.isDecision( clauseLit ) )
                ++decisionCounter;

        if ( decisionCounter >= _context.getLevel() )
            return clause;
    }

    return clause;
}

Vector<int> Engine::explainPhase( const PiecewiseLinearConstraint *litConstraint )
{
    ASSERT( _solveWithCDCL );
    ASSERT( litConstraint );
    ASSERT( litConstraint->phaseFixed() || !litConstraint->isActive() );

    // Get corresponding constraints, and its participating variables
    std::shared_ptr<GroundBoundManager::GroundBoundEntry> phaseFixingEntry =
        litConstraint->getPhaseFixingEntry();

    // Return a clause explaining the phase-fixing GroundBound entry
    ASSERT( phaseFixingEntry && phaseFixingEntry->lemma && phaseFixingEntry->isPhaseFixing );

    SparseUnsortedList tempExpl = phaseFixingEntry->lemma->getExplanations().back();
    _statistics.incUnsignedAttribute( Statistics::NUM_LEMMAS_USED );
    Set clause = clauseFromContradictionVector( tempExpl,
                                                phaseFixingEntry->id,
                                                phaseFixingEntry->lemma->getCausingVars().back(),
                                                phaseFixingEntry->lemma->getCausingVarBound() );

    if ( GlobalConfiguration::CDCL_REDUCE_CLAUSE_SIZE_WITH_PROOF )
        if ( clause.size() > 1 &&
             checkClauseWithProof( tempExpl, clause, phaseFixingEntry->lemma ) )
            clause = reduceClauseSizeWithProof(
                tempExpl, Vector<int>( clause.begin(), clause.end() ), phaseFixingEntry->lemma );

    return Vector<int>( clause.begin(), clause.end() );
}

Set<int> Engine::reduceClauseSizeWithProof( const SparseUnsortedList &explanation,
                                            const Vector<int> &clause,
                                            const std::shared_ptr<PLCLemma> lemma )
{
    ASSERT( _solveWithCDCL );
    ASSERT( !clause.empty() && !explanation.empty() );

    // Order clause by appearance in assigned constraints of the SearchTreeHandler
    // if not assigned yet then should be last
    Vector<int> toReturn = clause;
    std::sort( toReturn.begin(), toReturn.end(), [this]( int a, int b ) {
        unsigned aVal = _cdclCore.isLiteralFixed( a ) ? _plConstraints.size()
                                                      : _cdclCore.getLiteralAssignmentIndex( a );
        unsigned bVal = _cdclCore.isLiteralFixed( b ) ? _plConstraints.size()
                                                      : _cdclCore.getLiteralAssignmentIndex( b );
        return aVal > bVal;
    } );

    Vector<double> explanationLinearCombination( 0 );
    UNSATCertificateUtils::getExplanationRowCombination(
        explanation, explanationLinearCombination, _tableau->getSparseA(), _tableau->getN() );

    if ( lemma )
    {
        ASSERT( lemma->getCausingVars().size() == 1 );
        explanationLinearCombination[lemma->getCausingVars().front()]++;
    }

    Vector<double> gub = _groundBoundManager.getAllInitialGroundBounds( Tightening::UB );
    Vector<double> glb = _groundBoundManager.getAllInitialGroundBounds( Tightening::LB );

    Vector<int> support( 0 );
    toReturn = reduceClauseSizeWithLinearCombination(
        explanationLinearCombination, gub, glb, support, toReturn, lemma );

    ASSERT( !toReturn.empty() );
    Set<int> newClause = Set<int>();
    for ( int lit : toReturn )
        newClause.insert( lit );

    DEBUG( {
        if ( !lemma || lemma->getConstraintType() == RELU )
            ASSERT( checkClauseWithProof( explanation, newClause, lemma ) )
    } );

    return newClause;
}

Vector<int>
Engine::reduceClauseSizeWithLinearCombination( const Vector<double> &linearCombination,
                                               const Vector<double> &groundUpperBounds,
                                               const Vector<double> &groundLowerBounds,
                                               Vector<int> &support,
                                               const Vector<int> &clause,
                                               const std::shared_ptr<PLCLemma> lemma ) const
{
    ASSERT( _solveWithCDCL );
    ASSERT( !clause.empty() );

    if ( clause.size() == 1 )
        return clause;

    unsigned size = clause.size() / 2;

    Vector<int> l = Vector<int>( clause.begin(), clause.begin() + size );
    Vector<int> r = Vector<int>( clause.begin() + size, clause.end() );
    ASSERT( l.size() + r.size() == clause.size() );

    if ( checkLinearCombinationForClause(
             linearCombination, groundUpperBounds, groundLowerBounds, support + l, lemma ) )
        return reduceClauseSizeWithLinearCombination(
            linearCombination, groundUpperBounds, groundLowerBounds, support, l, lemma );

    // Try eliminating elements from the right half of the clause
    Vector<int> can1 = support + l;
    Vector<int> newR = reduceClauseSizeWithLinearCombination(
        linearCombination, groundUpperBounds, groundLowerBounds, can1, r, lemma );

    // Then try eliminating elements from the left half of the clause
    Vector<int> can2 = support + newR;
    Vector<int> newL = reduceClauseSizeWithLinearCombination(
        linearCombination, groundUpperBounds, groundLowerBounds, can2, l, lemma );

    return newL + newR;
}

bool Engine::checkLinearCombinationForClause( const Vector<double> &linearCombination,
                                              Vector<double> groundUpperBounds,
                                              Vector<double> groundLowerBounds,
                                              const Vector<int> &clause,
                                              const std::shared_ptr<PLCLemma> lemma ) const
{
    ASSERT( _solveWithCDCL );

    const PiecewiseLinearConstraint *constraint;
    for ( int lit : clause )
    {
        constraint = _cdclCore.getConstraintFromLit( lit );
        ASSERT( constraint );
        ASSERT( constraint->phaseFixed() || !constraint->isActive() );
        PiecewiseLinearCaseSplit litSplit;
        if ( lit > 0 )
        {
            if ( constraint->getType() == RELU || constraint->getType() == LEAKY_RELU )
                litSplit = constraint->getCaseSplit( RELU_PHASE_ACTIVE );
            else if ( constraint->getType() == ABSOLUTE_VALUE )
                litSplit = constraint->getCaseSplit( ABS_PHASE_POSITIVE );
            else if ( constraint->getType() == SIGN )
                litSplit = constraint->getCaseSplit( SIGN_PHASE_POSITIVE );
        }
        else
        {
            if ( constraint->getType() == RELU || constraint->getType() == LEAKY_RELU )
                litSplit = constraint->getCaseSplit( RELU_PHASE_INACTIVE );
            else if ( constraint->getType() == ABSOLUTE_VALUE )
                litSplit = constraint->getCaseSplit( ABS_PHASE_NEGATIVE );
            else if ( constraint->getType() == SIGN )
                litSplit = constraint->getCaseSplit( SIGN_PHASE_NEGATIVE );
        }
        ASSERT( !litSplit.getBoundTightenings().empty() )

        for ( const auto tightening : litSplit.getBoundTightenings() )
        {
            Vector<double> &temp =
                tightening._type == Tightening::UB ? groundUpperBounds : groundLowerBounds;
            temp[tightening._variable] = tightening._value;
        }
    }

    if ( !lemma )
        return FloatUtils::isNegative(
            UNSATCertificateUtils::computeCombinationUpperBound( linearCombination,
                                                                 groundUpperBounds.data(),
                                                                 groundLowerBounds.data(),
                                                                 _tableau->getN() ) );

    // Currently works for bound lemmas only
    ASSERT( lemma->getCausingVars().size() == 1 );
    double explainedBound =
        lemma->getCausingVarBound() == Tightening::UB
            ? UNSATCertificateUtils::computeCombinationUpperBound( linearCombination,
                                                                   groundUpperBounds.data(),
                                                                   groundLowerBounds.data(),
                                                                   _tableau->getN() )
            : UNSATCertificateUtils::computeCombinationLowerBound( linearCombination,
                                                                   groundUpperBounds.data(),
                                                                   groundLowerBounds.data(),
                                                                   _tableau->getN() );

    return lemma->getCausingVarBound() == Tightening::UB
             ? FloatUtils::lt( explainedBound, lemma->getBound() )
             : FloatUtils::gt( explainedBound, lemma->getBound() );
}

bool Engine::checkClauseWithProof( const SparseUnsortedList &explanation,
                                   const Set<int> &clause,
                                   const std::shared_ptr<PLCLemma> lemma ) const
{
    ASSERT( _solveWithCDCL );
    ASSERT( !explanation.empty() && !clause.empty() );
    Vector<double> explanationLinearCombination( 0 );
    UNSATCertificateUtils::getExplanationRowCombination(
        explanation, explanationLinearCombination, _tableau->getSparseA(), _tableau->getN() );
    Vector<double> gub = _groundBoundManager.getAllInitialGroundBounds( Tightening::UB );
    Vector<double> glb = _groundBoundManager.getAllInitialGroundBounds( Tightening::LB );

    if ( lemma )
        explanationLinearCombination[lemma->getCausingVars().front()]++;

    Vector<int> clauseVec( clause.begin(), clause.end() );

    return checkLinearCombinationForClause(
        explanationLinearCombination, gub, glb, clauseVec, lemma );
}

void Engine::removeLiteralFromPropagations( int literal )
{
    ASSERT( _solveWithCDCL );
    _cdclCore.removeLiteralFromPropagations( literal );
}

void Engine::explainGurobiFailure()
{
    ASSERT( _lpSolverType == LPSolverType::GUROBI );
    ASSERT( _milpEncoder && _gurobi && _gurobi->infeasible() );
    ASSERT( _produceUNSATProofs );

    ENGINE_LOG( "Extracting theory explanation..." );
    _gurobi->computeIIS();

    Map<String, GurobiWrapper::IISBoundType> bounds;
    List<String> dontCare;
    List<String> names;
    _gurobi->extractIIS( bounds, dontCare, names );

    Set<int> clause;
    for ( const auto &plc : _plConstraints )
    {
        if ( !plc->phaseFixed() )
            continue;

        for ( unsigned variable : plc->getParticipatingVariables() )
        {
            String variableName = Stringf( "x%u", variable );
            if ( ( ( bounds[variableName] == GurobiWrapper::IIS_UB ||
                     bounds[variableName] == GurobiWrapper::IIS_BOTH ) &&
                   plc->isBoundFixingPhase(
                       variable, _boundManager.getUpperBound( variable ), Tightening::UB ) ) ||
                 ( ( bounds[variableName] == GurobiWrapper::IIS_LB ||
                     bounds[variableName] == GurobiWrapper::IIS_BOTH ) &&
                   plc->isBoundFixingPhase(
                       variable, _boundManager.getLowerBound( variable ), Tightening::LB ) ) )
            {
                clause.insert( plc->propagatePhaseAsLit() );
                break;
            }
        }
    }

    if ( _solveWithCDCL )
        _cdclCore.addExternalClause( clause );

    ENGINE_LOG( Stringf( "Conflict analysis - done, conflict length %u, level %u",
                         clause.size(),
                         _context.getLevel() )
                    .ascii() );
}

bool Engine::checkAssignmentComplianceWithClause( const Set<int> &clause ) const
{
    ASSERT( _solveWithCDCL );

    bool compliant = false;
    for ( const auto &lit : clause )
    {
        const PiecewiseLinearConstraint *plc = _cdclCore.getConstraintFromLit( lit );
        PiecewiseLinearCaseSplit litSplit = lit > 0 ? plc->getCaseSplit( RELU_PHASE_ACTIVE )
                                                    : plc->getCaseSplit( RELU_PHASE_INACTIVE );
        bool compliantWithSplit = true;
        for ( const auto &tightening : litSplit.getBoundTightenings() )
        {
            if ( tightening._type == Tightening::UB )
            {
                if ( FloatUtils::gt( _tableau->getValue( tightening._variable ),
                                     tightening._value ) )
                    compliantWithSplit = false;
            }
            else
            {
                if ( FloatUtils::lt( _tableau->getValue( tightening._variable ),
                                     tightening._value ) )
                    compliantWithSplit = false;
            }
        }

        if ( compliantWithSplit )
        {
            compliant = true;
            break;
        }
    }
    return compliant;
}

void Engine::configureForCDCL()
{
    GlobalConfiguration::USE_DEEPSOI_LOCAL_SEARCH = false;
    _solveWithCDCL = true;
    _produceUNSATProofs = true;
    _solveWithMILP = false;
    _sncMode = false;

    _UNSATCertificateCurrentPointer =
        new ( true ) CVC4::context::CDO<UnsatCertificateNode *>( &_context, NULL );
}
#endif