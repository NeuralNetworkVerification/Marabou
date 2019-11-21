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

#include "Debug.h"
#include "DivideStrategy.h"
#include "DnCWorker.h"
#include "IEngine.h"
#include "EngineState.h"
#include "LargestIntervalDivider.h"
#include "MarabouError.h"
#include "MStringf.h"
#include "PiecewiseLinearCaseSplit.h"
#include "QueryDivider.h"
#include "ReluDivider.h"
#include "SubQuery.h"

#include <atomic>
#include <chrono>
#include <cmath>
#include <thread>

DnCWorker::DnCWorker( WorkerQueue *workload, std::shared_ptr<IEngine> engine,
                      std::atomic_uint &numUnsolvedSubQueries,
                      std::atomic_bool &shouldQuitSolving,
                      unsigned threadId, unsigned onlineDivides,
                      float timeoutFactor, DivideStrategy divideStrategy )
    : _workload( workload )
    , _engine( engine )
    , _numUnsolvedSubQueries( &numUnsolvedSubQueries )
    , _shouldQuitSolving( &shouldQuitSolving )
    , _threadId( threadId )
    , _onlineDivides( onlineDivides )
    , _timeoutFactor( timeoutFactor )
{
    setQueryDivider( divideStrategy );

    // Obtain the current state of the engine
    _initialState = std::make_shared<EngineState>();
    _engine->storeState( *_initialState, true );
}

void DnCWorker::setQueryDivider( DivideStrategy divideStrategy )
{
    // For now, there is only one strategy
    if ( divideStrategy == DivideStrategy::LargestInterval )
    {
        const List<unsigned> &inputVariables = _engine->getInputVariables();
        _queryDivider = std::unique_ptr<LargestIntervalDivider>
            ( new LargestIntervalDivider( inputVariables ) );
    }
    else
    {
        _queryDivider = std::unique_ptr<QueryDivider>
            ( new ReluDivider( _engine ) );
    }
}

void DnCWorker::popOneSubQueryAndSolve( bool restoreTreeStates )
{
    SubQuery *subQuery = NULL;
    // Boost queue stores the next element into the passed-in pointer
    // and returns true if the pop is successful (aka, the queue is not empty
    // in most cases)
    if ( _workload->pop( subQuery ) )
    {
        String queryId = subQuery->_queryId;
        auto split = std::move( subQuery->_split );
        unsigned timeoutInSeconds = subQuery->_timeoutInSeconds;

        // Reset the engine state
        _engine->restoreState( *_initialState );
        _engine->reset();

        // TODO: each worker is going to keep a map from *CaseSplit to an
        // object of class DnCStatistics, which contains some basic
        // statistics. The maps are owned by the DnCManager.

        // Apply the split and solve
        _engine->applySplit( *split );
        _engine->solve( timeoutInSeconds );

        IEngine::ExitCode result = _engine->getExitCode();
        printProgress( queryId, result );
        // Switch on the result
        if ( result == IEngine::UNSAT )
        {
            // If UNSAT, continue to solve
            *_numUnsolvedSubQueries -= 1;
            if ( _numUnsolvedSubQueries->load() == 0 )
                *_shouldQuitSolving = true;
            delete subQuery;
        }
        else if ( result == IEngine::TIMEOUT )
        {
            // If TIMEOUT, split the current input region and add the
            // new subQueries to the current queue
            SubQueries subQueries;
	    _engine->restoreState( *_initialState );
            _queryDivider->createSubQueries( pow( 2, _onlineDivides ),
                                             queryId, *split,
                                             (unsigned)timeoutInSeconds *
                                             _timeoutFactor, subQueries );
            for ( auto &newSubQuery : subQueries )
            {
                if ( !_workload->push( std::move( newSubQuery ) ) )
                {
                    throw MarabouError( MarabouError::UNSUCCESSFUL_QUEUE_PUSH );
                }

                *_numUnsolvedSubQueries += 1;
            }
            *_numUnsolvedSubQueries -= 1;
            delete subQuery;
        }
        else if ( result == IEngine::QUIT_REQUESTED )
        {
            // If engine was asked to quit, quit
            std::cout << "Quit requested by manager!" << std::endl;
            delete subQuery;
            ASSERT( _shouldQuitSolving->load() );
        }
        else
        {
            // We must set the quit flag to true  if the result is not UNSAT or
            // TIMEOUT. This way, the DnCManager will kill all the DnCWorkers.

            *_shouldQuitSolving = true;
            if ( result == IEngine::SAT )
            {
                // case SAT
                *_numUnsolvedSubQueries -= 1;
                delete subQuery;
            }
            else if ( result == IEngine::ERROR )
            {
                // case ERROR
                std::cout << "Error!" << std::endl;
                delete subQuery;
            }
            else // result == IEngine::NOT_DONE
            {
                // case NOT_DONE
                ASSERT( false );
                std::cout << "Not done! This should not happen." << std::endl;
                delete subQuery;
            }
        }
    }
    else
    {
        // If the queue is empty but the pop fails, wait and retry
        std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );
    }
}

void DnCWorker::printProgress( String queryId, IEngine::ExitCode result ) const
{
    printf( "Worker %d: Query %s %s, %d tasks remaining\n", _threadId,
            queryId.ascii(), exitCodeToString( result ).ascii(),
            _numUnsolvedSubQueries->load() );
}

String DnCWorker::exitCodeToString( IEngine::ExitCode result )
{
    switch ( result )
    {
    case IEngine::UNSAT:
        return "UNSAT";
    case IEngine::SAT:
        return "SAT";
    case IEngine::ERROR:
        return "ERROR";
    case IEngine::TIMEOUT:
        return "TIMEOUT";
    case IEngine::QUIT_REQUESTED:
        return "QUIT_REQUESTED";
    default:
        ASSERT( false );
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
