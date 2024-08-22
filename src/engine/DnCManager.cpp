/*********************                                                        */
/*! \file DnCManager.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Haoze Wu
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2024 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

 **/

#include "DnCManager.h"

#include "Debug.h"
#include "DnCWorker.h"
#include "GetCPUData.h"
#include "GlobalConfiguration.h"
#include "LargestIntervalDivider.h"
#include "MStringf.h"
#include "MarabouError.h"
#include "Options.h"
#include "PiecewiseLinearCaseSplit.h"
#include "PolarityBasedDivider.h"
#include "Query.h"
#include "QueryDivider.h"
#include "SnCDivideStrategy.h"
#include "TimeUtils.h"
#include "Vector.h"

#include <atomic>
#include <chrono>
#include <cmath>
#include <thread>

#ifdef ENABLE_OPENBLAS
#include "cblas.h"
#endif

void DnCManager::dncSolve( WorkerQueue *workload,
                           std::shared_ptr<Engine> engine,
                           std::unique_ptr<Query> inputQuery,
                           std::atomic_int &numUnsolvedSubQueries,
                           std::atomic_bool &shouldQuitSolving,
                           unsigned threadId,
                           unsigned onlineDivides,
                           float timeoutFactor,
                           SnCDivideStrategy divideStrategy,
                           bool restoreTreeStates,
                           unsigned verbosity,
                           unsigned seed,
                           bool parallelDeepSoI )
{
    unsigned cpuId = 0;
    (void)threadId;
    (void)cpuId;

    getCPUId( cpuId );
    DNC_MANAGER_LOG( Stringf( "Thread #%u on CPU %u", threadId, cpuId ).ascii() );

    engine->setRandomSeed( seed );
    if ( threadId != 0 )
        engine->processInputQuery( *inputQuery, false );

    DnCWorker worker( workload,
                      engine,
                      std::ref( numUnsolvedSubQueries ),
                      std::ref( shouldQuitSolving ),
                      threadId,
                      onlineDivides,
                      timeoutFactor,
                      divideStrategy,
                      verbosity,
                      parallelDeepSoI );
    while ( !shouldQuitSolving.load() )
    {
        worker.popOneSubQueryAndSolve( restoreTreeStates );
    }
}

DnCManager::DnCManager( IQuery *inputQuery )
    : _baseQuery( inputQuery )
    , _exitCode( DnCManager::NOT_DONE )
    , _workload( NULL )
    , _timeoutReached( false )
    , _numUnsolvedSubQueries( 0 )
    , _verbosity( Options::get()->getInt( Options::VERBOSITY ) )
    , _runParallelDeepSoI( Options::get()->getBool( Options::PARALLEL_DEEPSOI ) )
    , _sncSplittingStrategy( Options::get()->getSnCDivideStrategy() )
{
}

DnCManager::~DnCManager()
{
    freeMemoryIfNeeded();
}

void DnCManager::freeMemoryIfNeeded()
{
    if ( _workload )
    {
        SubQuery *subQuery = NULL;
        while ( !_workload->empty() )
        {
            _workload->pop( subQuery );
            delete subQuery;
        }

        delete _workload;
        _workload = NULL;
    }
}

void DnCManager::solve()
{
    enum {
        MICROSECONDS_IN_SECOND = 1000000
    };

    unsigned timeoutInSeconds = Options::get()->getInt( Options::TIMEOUT );
    unsigned long long timeoutInMicroSeconds =
        (unsigned long long)timeoutInSeconds * (unsigned long long)MICROSECONDS_IN_SECOND;
    DNC_MANAGER_LOG( Stringf( "timeout in micro seconds: %llu", timeoutInMicroSeconds ).ascii() );

    struct timespec startTime = TimeUtils::sampleMicro();

    unsigned numWorkers = Options::get()->getInt( Options::NUM_WORKERS );

#ifdef ENABLE_OPENBLAS
    // When preprocess the input query with SBT, we leverage multi-threading.
    openblas_set_num_threads( numWorkers );
#endif

    // Preprocess the input query and create an engine for each of the threads
    if ( !createEngines( numWorkers ) )
    {
        _exitCode = DnCManager::UNSAT;
        return;
    }

#ifdef ENABLE_OPENBLAS
    // Now each worker occupies one thread. So SBT performed during the search
    // will be single-threaded.
    openblas_set_num_threads( 1 );
#endif

    // Prepare the mechanism through which we can ask the engines to quit
    List<std::atomic_bool *> quitThreads;
    for ( unsigned i = 0; i < numWorkers; ++i )
        quitThreads.append( _engines[i]->getQuitRequested() );

    // Partition the input query into initial subqueries, and place these
    // queries in the queue
    _workload = new WorkerQueue( 0 );
    if ( !_workload )
        throw MarabouError( MarabouError::ALLOCATION_FAILED, "DnCManager::workload" );

    SubQueries subQueries;
    if ( !_runParallelDeepSoI )
        initialDivide( subQueries );
    else
    {
        for ( unsigned i = 0; i < numWorkers; ++i )
        {
            // Create empty case splits to get each worker started.
            SubQuery *subQuery = new SubQuery;
            subQuery->_queryId = Stringf( "%u", i );
            auto split = std::unique_ptr<PiecewiseLinearCaseSplit>( new PiecewiseLinearCaseSplit );
            subQuery->_split = std::move( split );
            subQuery->_timeoutInSeconds = timeoutInSeconds;
            subQuery->_depth = 0;
            subQueries.append( subQuery );
        }
    }

    // Create objects shared across workers
    _numUnsolvedSubQueries = _runParallelDeepSoI ? 1 : subQueries.size();
    std::atomic_bool shouldQuitSolving( false );
    WorkerQueue *workload = new WorkerQueue( 0 );
    for ( auto &subQuery : subQueries )
    {
        if ( !workload->push( subQuery ) )
        {
            // This should never happen
            ASSERT( false );
        }
    }

    unsigned onlineDivides = Options::get()->getInt( Options::NUM_ONLINE_DIVIDES );
    float timeoutFactor = Options::get()->getFloat( Options::TIMEOUT_FACTOR );
    bool restoreTreeStates = Options::get()->getBool( Options::RESTORE_TREE_STATES );
    unsigned seed = Options::get()->getInt( Options::SEED );

    auto baseQuery = std::unique_ptr<Query>( new Query( *( _baseEngine->getQuery() ) ) );

    // Spawn threads and start solving
    std::list<std::thread> threads;
    for ( unsigned threadId = 0; threadId < numWorkers; ++threadId )
    {
        std::unique_ptr<Query> inputQuery = nullptr;
        if ( threadId != 0 )
            // Get the processed input query from the base engine
            inputQuery = std::unique_ptr<Query>( new Query( *( baseQuery ) ) );

        threads.push_back( std::thread( dncSolve,
                                        workload,
                                        _engines[threadId],
                                        threadId != 0 ? std::move( inputQuery ) : nullptr,
                                        std::ref( _numUnsolvedSubQueries ),
                                        std::ref( shouldQuitSolving ),
                                        threadId,
                                        onlineDivides,
                                        timeoutFactor,
                                        _sncSplittingStrategy,
                                        restoreTreeStates,
                                        _verbosity,
                                        _runParallelDeepSoI ? seed + threadId : seed,
                                        _runParallelDeepSoI ) );
    }

    // Wait until either all subQueries are solved or a satisfying assignment is
    // found by some worker
    while ( !shouldQuitSolving.load() )
    {
        updateTimeoutReached( startTime, timeoutInMicroSeconds );
        if ( _timeoutReached )
            shouldQuitSolving = true;
        else
            std::this_thread::sleep_for( std::chrono::milliseconds( numWorkers ) );
    }


    // Now that we are done, tell all workers to quit
    for ( auto &quitThread : quitThreads )
        *quitThread = true;

    for ( auto &thread : threads )
        thread.join();

    updateDnCExitCode();
    return;
}

DnCManager::DnCExitCode DnCManager::getExitCode() const
{
    return _exitCode;
}

void DnCManager::updateDnCExitCode()
{
    bool hasSat = false;
    bool hasError = false;
    bool hasQuitRequested = false;
    for ( auto &engine : _engines )
    {
        Engine::ExitCode result = engine->getExitCode();
        if ( result == Engine::SAT )
        {
            _engineWithSATAssignment = engine;
            hasSat = true;
            break;
        }
        else if ( result == Engine::ERROR )
            hasError = true;
        else if ( result == Engine::QUIT_REQUESTED )
            hasQuitRequested = true;
    }
    if ( hasSat )
        _exitCode = DnCManager::SAT;
    else if ( _timeoutReached )
        _exitCode = DnCManager::TIMEOUT;
    else if ( _numUnsolvedSubQueries.load() <= 0 )
        _exitCode = DnCManager::UNSAT;
    else if ( hasQuitRequested )
        _exitCode = DnCManager::QUIT_REQUESTED;
    else if ( hasError )
        _exitCode = DnCManager::ERROR;
    else
    {
        ASSERT( false ); // This should never happen
        _exitCode = DnCManager::NOT_DONE;
    }
}

String DnCManager::getResultString()
{
    switch ( _exitCode )
    {
    case DnCManager::SAT:
        return "sat";
    case DnCManager::UNSAT:
        return "unsat";
    case DnCManager::ERROR:
        return "ERROR";
    case DnCManager::NOT_DONE:
        return "NOT_DONE";
    case DnCManager::QUIT_REQUESTED:
        return "QUIT_REQUESTED";
    case DnCManager::TIMEOUT:
        return "TIMEOUT";
    default:
        ASSERT( false );
        return "";
    }
}

void DnCManager::extractSolution( IQuery &inputQuery )
{
    ASSERT( _engineWithSATAssignment != nullptr );
    _engineWithSATAssignment->extractSolution( inputQuery, _baseEngine->getPreprocessor() );
}

void DnCManager::getSolution( std::map<int, double> &ret, IQuery &inputQuery )
{
    extractSolution( inputQuery );
    for ( unsigned i = 0; i < inputQuery.getNumberOfVariables(); ++i )
        ret[i] = inputQuery.getSolutionValue( i );
}

void DnCManager::printResult()
{
    std::cout << std::endl;
    switch ( _exitCode )
    {
    case DnCManager::SAT:
    {
        std::cout << "sat\n" << std::endl;

        // TODO: update the variable assignment using NLR if possible and double-check that all the
        // constraints are indeed satisfied.
        extractSolution( *_baseQuery );

        printf( "Input assignment:\n" );
        for ( unsigned i = 0; i < _baseQuery->getNumInputVariables(); ++i )
        {
            printf( "\tx%u = %lf\n",
                    i,
                    _baseQuery->getSolutionValue( _baseQuery->inputVariableByIndex( i ) ) );
        }

        printf( "\n" );
        printf( "Output:\n" );
        for ( unsigned i = 0; i < _baseQuery->getNumOutputVariables(); ++i )
        {
            printf( "\ty%u = %lf\n",
                    i,
                    _baseQuery->getSolutionValue( _baseQuery->outputVariableByIndex( i ) ) );
        }
        printf( "\n" );
        break;
    }
    case DnCManager::UNSAT:
        std::cout << "unsat" << std::endl;
        break;
    case DnCManager::ERROR:
        std::cout << "ERROR" << std::endl;
        break;
    case DnCManager::NOT_DONE:
        std::cout << "NOT_DONE" << std::endl;
        break;
    case DnCManager::QUIT_REQUESTED:
        std::cout << "QUIT_REQUESTED" << std::endl;
        break;
    case DnCManager::TIMEOUT:
        std::cout << "TIMEOUT" << std::endl;
        break;
    default:
        ASSERT( false );
    }
}

bool DnCManager::createEngines( unsigned numberOfEngines )
{
    // Create the base engine
    _baseEngine = std::make_shared<Engine>();
    _engines.append( _baseEngine );
    if ( !_baseEngine->processInputQuery( *_baseQuery ) )
        // Solved by preprocessing, we are done!
        return false;

    _baseEngine->setVerbosity( 0 );

    // Create engines for each thread
    for ( unsigned i = 1; i < numberOfEngines; ++i )
    {
        auto engine = std::make_shared<Engine>();
        engine->setVerbosity( 0 );
        _engines.append( engine );
    }

    return true;
}

void DnCManager::initialDivide( SubQueries &subQueries )
{
    if ( _sncSplittingStrategy == SnCDivideStrategy::Auto )
    {
        DNC_MANAGER_LOG( Stringf( "Deciding splitting strategy automatically...\n" ).ascii() );
        Query *processedQuery = _baseEngine->getQuery();
        if ( processedQuery->getNumInputVariables() <
                 GlobalConfiguration::INTERVAL_SPLITTING_THRESHOLD ||
             processedQuery->getPiecewiseLinearConstraints().empty() )
        {
            DNC_MANAGER_LOG( Stringf( "\tUsing Largest Interval Heuristics\n" ).ascii() );
            _sncSplittingStrategy = SnCDivideStrategy::LargestInterval;
        }
        else
        {
            DNC_MANAGER_LOG( Stringf( "\tUsing Polarity-based Heuristics\n" ).ascii() );
            _sncSplittingStrategy = SnCDivideStrategy::Polarity;
        }
    }

    auto split = std::unique_ptr<PiecewiseLinearCaseSplit>( new PiecewiseLinearCaseSplit() );
    std::unique_ptr<QueryDivider> queryDivider = nullptr;
    if ( _sncSplittingStrategy == SnCDivideStrategy::Polarity )
    {
        queryDivider = std::unique_ptr<QueryDivider>( new PolarityBasedDivider( _baseEngine ) );
    }
    else // Default is LargestInterval
    {
        const List<unsigned> inputVariables( _baseEngine->getInputVariables() );
        queryDivider =
            std::unique_ptr<QueryDivider>( new LargestIntervalDivider( inputVariables ) );
        Query *inputQuery = _baseEngine->getQuery();
        // Add bound as equations for each input variable
        for ( const auto &variable : inputVariables )
        {
            double lb = inputQuery->getLowerBounds()[variable];
            double ub = inputQuery->getUpperBounds()[variable];
            split->storeBoundTightening( Tightening( variable, lb, Tightening::LB ) );
            split->storeBoundTightening( Tightening( variable, ub, Tightening::UB ) );
        }
    }

    unsigned initialDivides = Options::get()->getInt( Options::NUM_INITIAL_DIVIDES );
    unsigned initialTimeout = Options::get()->getInt( Options::INITIAL_TIMEOUT );

    String queryId;

    // Create subqueries
    queryDivider->createSubQueries(
        pow( 2, initialDivides ), queryId, 0, *split, initialTimeout, subQueries );
}

void DnCManager::updateTimeoutReached( timespec startTime,
                                       unsigned long long timeoutInMicroSeconds )
{
    if ( timeoutInMicroSeconds == 0 )
        return;
    struct timespec now = TimeUtils::sampleMicro();
    _timeoutReached = TimeUtils::timePassed( startTime, now ) >= timeoutInMicroSeconds;
}
