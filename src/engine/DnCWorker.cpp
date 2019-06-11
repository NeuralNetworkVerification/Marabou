/*********************                                                        */
/*! \file DnCWorker.cpp
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

#include "DivideStrategy.h"
#include "DnCWorker.h"
#include "Engine.h"
#include "EngineState.h"
#include "LargestIntervalDivider.h"
#include "MStringf.h"
#include "PiecewiseLinearCaseSplit.h"
#include "SubQuery.h"

#include <atomic>
#include <chrono>
#include <cmath>
#include <thread>

DnCWorker::DnCWorker( WorkerQueue *workload, std::shared_ptr<Engine> engine,
                      std::atomic_uint &numUnsolvedSubqueries,
                      std::atomic_bool &shouldQuitSolving,
                      unsigned threadId, unsigned onlineDivides,
                      float timeoutFactor, DivideStrategy divideStrategy )
    : _workload( workload )
    , _engine( engine )
    , _numUnsolvedSubqueries( &numUnsolvedSubqueries ) // subqueries --> subQueries?
    , _shouldQuitSolving( &shouldQuitSolving )
    , _threadId( threadId )
    , _onlineDivides( onlineDivides )
    , _timeoutFactor( timeoutFactor )
{
    setQueryDivider( divideStrategy );

    // Obtain the current state of the engine
    _initialState = std::make_shared<EngineState>();
    _initialState->_stateId = _threadId; // State ids were originally added solely for debugging. Are they used for something else, now?
    _engine->storeState( *_initialState, true );
}

void DnCWorker::setQueryDivider( DivideStrategy divideStrategy )
{
    // For now, there is only one strategy
    ASSERT( divideStrategy == DivideStrategy::LargestInterval );
    const List<unsigned> &inputVariables = _engine->getInputVariables();
    _queryDivider = std::unique_ptr<LargestIntervalDivider>
        ( new LargestIntervalDivider( inputVariables, _timeoutFactor ) );
}

void DnCWorker::run()
{
    while ( _numUnsolvedSubqueries->load() > 0 )
    {
        std::unique_ptr<SubQuery> subquery = nullptr; // subquery --> subQuery?
        // Boost queue stores the next element into the passed-in pointer
        // and returns true if the pop is successful (aka, the queue is not empty
        // in most cases)
        if ( _workload->pop( subquery ) )
        {
            String queryId = subquery->_queryId;
            PiecewiseLinearCaseSplit split = *( subquery->_split );
            unsigned timeoutInSeconds = subquery->_timeoutInSeconds;

            // Create a new statistics object for each subquery
            Statistics *statistics = new Statistics();
            _engine->resetStatistics( *statistics ); // What is our policy for statistics in DnC mode? is there some global statistics, also?

            // Apply the split and solve
            _engine->applySplit( split );
            _engine->solve( timeoutInSeconds );

            Engine::ExitCode result = _engine->getExitCode();
            printProgress( queryId, result );
            // Switch on the result
            if ( result == Engine::UNSAT )
            {
                // If UNSAT, continue to solve
                *_numUnsolvedSubqueries -= 1;
            }
            else if ( result == Engine::TIMEOUT )
            {
                // If TIMEOUT, split the current input region and add the
                // new subqueries to the current queue
                SubQueries subqueries; // subQueries
                _queryDivider->createSubQueries( pow( 2, _onlineDivides ),
                                                 *subquery, subqueries );
                bool pushed = false;
                for ( auto &subquery : subqueries )
                {
                    pushed = _workload->push( std::move( subquery ) );
                    ASSERT( pushed );
                    *_numUnsolvedSubqueries += 1;
                }
                *_numUnsolvedSubqueries -= 1;
            }
            else if ( result == Engine::SAT )
            {
                // If SAT, set the shouldQuitSolving flag to true, so that the
                // DnCManager will kill all the DnCWorkers
                *_shouldQuitSolving = true;
                *_numUnsolvedSubqueries -= 1;
                return;
            }
            else if ( result == Engine::QUIT_REQUESTED )
            {
                // If engine was asked to quit, quit
                std::cout << "Quit requested by manager!" << std::endl;
                *_numUnsolvedSubqueries -= 1;
                return;
            }
            else if ( result == Engine::ERROR )
            {
                // If ERROR, set the shouldQuitSolving flag to true and quit
                std::cout << "Error!" << std::endl;
                *_shouldQuitSolving = true;
                *_numUnsolvedSubqueries -= 1;
                return;
            }
            else if ( result == Engine::NOT_DONE )
            {
                // If NOT_DONE, set the shouldQuitSolving flag to true and quit
                std::cout << "Not done! This should not happen." << std::endl;
                // If this should not happen, add ASSERT( false )?
                *_shouldQuitSolving = true;
                *_numUnsolvedSubqueries -= 1;
                return;
            }

            // reset the engine state
            _engine->restoreState( *_initialState );
            _engine->clearViolatedPLConstraints();
            _engine->resetSmtCore();
            _engine->resetExitCode();
            _engine->resetBoundTighteners();
        }
        else
        {
            // If the queue is empty but the pop fails, wait and retry
            std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );
        }
    }
}

void DnCWorker::printProgress( const String &queryId,
                               const Engine::ExitCode result ) const // no need to add "const" for pass-by-value
{
    printf( "Query %s %s, %d tasks remaining\n", queryId.ascii(),
            exitCodeToString( result ).ascii(), _numUnsolvedSubqueries->load() );
}

const String DnCWorker::exitCodeToString( const Engine::ExitCode result ) // no need to add "const" for pass-by-value, it's a local copy anyway. Likewise for return value
{
    switch ( result )
    {
    case Engine::UNSAT:
        return "UNSAT";
    case Engine::SAT:
        return "SAT";
    case Engine::ERROR:
        return "ERROR";
    case Engine::TIMEOUT:
        return "TIMEOUT";
    case Engine::QUIT_REQUESTED:
        return "QUIT_REQUESTED";
    default:
        // Add ASSERT( false ) for unreachable code, so that it's harded to overlook.
        return "UNKNOWN (this should never happen)";
    }
}

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
