/*********************                                                        */
/*! \file DnCManager.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Haoze Wu
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

 **/

#include "AcasParser.h"
#include "Debug.h"
#include "DivideStrategy.h"
#include "DnCManager.h"
#include "DnCWorker.h"
#include "LargestIntervalDivider.h"
#include "MStringf.h"
#include "PiecewiseLinearCaseSplit.h"
#include "PropertyParser.h"
#include "QueryDivider.h"
#include "ReluplexError.h"

#include <atomic>
#include <chrono>
#include <cmath>
#include <thread>

static void dncSolve( WorkerQueue *workload, std::shared_ptr<Engine> engine,
                      std::atomic_uint &numUnsolvedSubQueries,
                      std::atomic_bool &shouldQuitSolving,
                      unsigned threadId, unsigned onlineDivides,
                      float timeoutFactor, DivideStrategy divideStrategy )
{
    DnCWorker worker( workload, engine, std::ref( numUnsolvedSubQueries ),
                      std::ref( shouldQuitSolving ), threadId, onlineDivides,
                      timeoutFactor, divideStrategy );
    worker.run();
}

DnCManager::DnCManager( unsigned numWorkers, unsigned initialDivides,
                        unsigned initialTimeout, unsigned onlineDivides,
                        float timeoutFactor, DivideStrategy divideStrategy,
                        String networkFilePath, String propertyFilePath )
    : _numWorkers( numWorkers )
    , _initialDivides( initialDivides )
    , _initialTimeout( initialTimeout )
    , _onlineDivides( onlineDivides )
    , _timeoutFactor( timeoutFactor )
    , _divideStrategy( divideStrategy )
    , _networkFilePath( networkFilePath )
    , _propertyFilePath( propertyFilePath )
    , _exitCode( DnCManager::NOT_DONE )
    , _workload( NULL )
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
        SubQuery *subQuery;
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
    if ( !createEngines() )
    {
        _exitCode = DnCManager::UNSAT;
        printResult();
        return;
    }

    List<std::atomic_bool *> quitThreads;
    for ( unsigned i = 0; i < _numWorkers; ++i )
        quitThreads.append( _engines[i]->getQuitRequested() );

    // Conduct the initial division of the input region
    _workload = new WorkerQueue( 0 );
    if ( !_workload )
        throw ReluplexError( ReluplexError::ALLOCATION_FAILED, "DnCManager::workload" );

    SubQueries subQueries;
    initialDivide( subQueries );

    // Create objects shared across workers
    std::atomic_uint numUnsolvedSubqueries( subQueries.size() );
    std::atomic_bool shouldQuitSolving( false );
    WorkerQueue *workload = new WorkerQueue( 0 );
    bool pushed = false;
    (void)pushed;
    for ( auto &subQuery : subQueries )
    {
        pushed = workload->push( subQuery );
        ASSERT( pushed );
    }

    // Spawn threads and start solving
    std::list<std::thread> threads; // TODO: change this to List (compliation error)
    for ( unsigned threadId = 0; threadId < _numWorkers; ++threadId )
    {
        threads.push_back( std::thread( dncSolve, workload,
                                        _engines[ threadId ],
                                        std::ref( numUnsolvedSubqueries ),
                                        std::ref( shouldQuitSolving ),
                                        threadId, _onlineDivides,
                                        _timeoutFactor, _divideStrategy ) );
    }

    // Wait until either all subqueries are solved or a satisfying assignment is
    // found by some worker
    while ( numUnsolvedSubqueries.load() > 0 &&
            !shouldQuitSolving.load() )
        std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );

    // Now that we are done, quit all workers
    for ( auto &quitThread : quitThreads )
        *quitThread = true;

    for ( auto &thread : threads )
        thread.join();

    updateDnCExitCode();
    printResult();
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
    bool hasNotDone = false;
    bool hasQuitRequested = false;
    for ( auto &engine : _engines )
    {
        Engine::ExitCode result = engine->getExitCode( );
        if ( result == Engine::SAT )
        {
            hasSat = true;
            break;
        }
        else if ( result == Engine::ERROR )
            hasError = true;
        else if ( result == Engine::QUIT_REQUESTED )
            hasQuitRequested = true;
        else if ( result == Engine::NOT_DONE )
            hasNotDone = true;
    }
    if ( hasSat )
        _exitCode = DnCManager::SAT;
    else if ( hasError )
        _exitCode = DnCManager::ERROR;
    else if ( hasQuitRequested )
        _exitCode = DnCManager::QUIT_REQUESTED;
    else if ( hasNotDone )
        _exitCode = DnCManager::NOT_DONE;
    else
        _exitCode = DnCManager::UNSAT;
}

void DnCManager::printResult()
{
    switch ( _exitCode )
    {
    case DnCManager::SAT:
        std::cout << "DnCManager::solve SAT query" << std::endl;
        break;
    case DnCManager::UNSAT:
        std::cout << "DnCManager::solve UNSAT query" << std::endl;
        break;
    case DnCManager::ERROR:
        std::cout << "DnCManager::solve ERROR" << std::endl;
        break;
    case DnCManager::NOT_DONE:
        std::cout << "DnCManager::solve NOT_DONE" << std::endl;
        break;
    case DnCManager::QUIT_REQUESTED:
        std::cout << "DnCManager::solve QUIT_REQUESTED" << std::endl;
        break;
    case DnCManager::TIMEOUT:
        std::cout << "DnCManager::solve TIMEOUT" << std::endl;
        break;
    default:
        ASSERT( false );
    }
}

bool DnCManager::createEngines()
{
    // Create the base engine
    _baseEngine = std::make_shared<Engine>();
    InputQuery *baseInputQuery = new InputQuery();
    // inputQuery is owned by engine
    AcasParser acasParser( _networkFilePath );
    acasParser.generateQuery( *baseInputQuery );

    if ( _propertyFilePath != "" )
        PropertyParser( ).parse( _propertyFilePath, *baseInputQuery );

    if ( !_baseEngine->processInputQuery( *baseInputQuery ) )
        // Solved by preprocessing, we are done!
        return false;

    // Create engines for each thread
    for ( unsigned i = 0; i < _numWorkers; ++i )
    {
        auto engine = std::make_shared<Engine>();
        InputQuery *inputQuery = new InputQuery();
        *inputQuery = *baseInputQuery;
        engine->processInputQuery( *inputQuery );
        _engines.append( engine );
    }
    return true;
}

void DnCManager::initialDivide( SubQueries &subQueries )
{
    const List<unsigned> inputVariables( _baseEngine->getInputVariables() );
    std::unique_ptr<QueryDivider> queryDivider = nullptr;
    if ( _divideStrategy == DivideStrategy::LargestInterval )
    {
        queryDivider = std::unique_ptr<QueryDivider>
            ( new LargestIntervalDivider( inputVariables, _timeoutFactor ) );
    }
    else
    {
        // Default
        queryDivider = std::unique_ptr<QueryDivider>
            ( new LargestIntervalDivider( inputVariables, _timeoutFactor ) );
    }

    String queryId = "";
    // Create a new case split
    QueryDivider::InputRegion initialRegion;
    InputQuery *inputQuery = _baseEngine->getInputQuery();
    for ( const auto &variable : inputVariables )
    {
        initialRegion._lowerBounds[variable] =
            inputQuery->getLowerBounds()[variable];
        initialRegion._upperBounds[variable] =
            inputQuery->getUpperBounds()[variable];
    }

    auto split = std::unique_ptr<PiecewiseLinearCaseSplit>
        ( new PiecewiseLinearCaseSplit() );

    // Add bound as equations for each input variable
    for ( const auto &variable : inputVariables )
    {
        double lb = initialRegion._lowerBounds[variable];
        double ub = initialRegion._upperBounds[variable];
        split->storeBoundTightening( Tightening( variable, lb,
                                                 Tightening::LB ) );
        split->storeBoundTightening( Tightening( variable, ub,
                                                 Tightening::UB ) );
    }

    // Construct the new subquery and add it to subqueries
    SubQuery subQuery( queryId, split, _initialTimeout );

    queryDivider->createSubQueries( pow( 2, _initialDivides ), subQuery,
                                    subQueries );
}

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
