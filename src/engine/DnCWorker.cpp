/*********************                                                        */
/*! \file DnCWorker.cpp
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

#include "DnCWorker.h"

#include "Debug.h"
#include "EngineState.h"
#include "IEngine.h"
#include "LargestIntervalDivider.h"
#include "MStringf.h"
#include "MarabouError.h"
#include "PiecewiseLinearCaseSplit.h"
#include "PolarityBasedDivider.h"
#include "SnCDivideStrategy.h"
#include "SubQuery.h"
#include "TableauStateStorageLevel.h"

#include <atomic>
#include <chrono>
#include <cmath>
#include <thread>

DnCWorker::DnCWorker( WorkerQueue *workload,
                      std::shared_ptr<IEngine> engine,
                      std::atomic_int &numUnsolvedSubQueries,
                      std::atomic_bool &shouldQuitSolving,
                      unsigned threadId,
                      unsigned onlineDivides,
                      float timeoutFactor,
                      SnCDivideStrategy divideStrategy,
                      unsigned verbosity,
                      bool parallelDeepSoI )
    : _workload( workload )
    , _engine( engine )
    , _numUnsolvedSubQueries( &numUnsolvedSubQueries )
    , _shouldQuitSolving( &shouldQuitSolving )
    , _threadId( threadId )
    , _onlineDivides( onlineDivides )
    , _timeoutFactor( timeoutFactor )
    , _verbosity( verbosity )
    , _parallelDeepSoI( parallelDeepSoI )
{
    setQueryDivider( divideStrategy );

    // Obtain the current state of the engine
    if ( !_parallelDeepSoI )
    {
        _initialState = std::make_shared<EngineState>();
        _engine->storeState( *_initialState, TableauStateStorageLevel::STORE_ENTIRE_TABLEAU_STATE );
    }
}

void DnCWorker::setQueryDivider( SnCDivideStrategy divideStrategy )
{
    if ( divideStrategy == SnCDivideStrategy::Polarity )
        _queryDivider = std::unique_ptr<QueryDivider>( new PolarityBasedDivider( _engine ) );
    else
    {
        const List<unsigned> &inputVariables = _engine->getInputVariables();
        _queryDivider =
            std::unique_ptr<LargestIntervalDivider>( new LargestIntervalDivider( inputVariables ) );
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
        unsigned depth = subQuery->_depth;
        auto split = std::move( subQuery->_split );
        std::unique_ptr<SearchTreeState> searchTreeState = nullptr;
        if ( restoreTreeStates && subQuery->_searchTreeState )
            searchTreeState = std::move( subQuery->_searchTreeState );
        unsigned timeoutInSeconds = subQuery->_timeoutInSeconds;

        // Reset the engine state
        _engine->restoreState( *_initialState );
        _engine->reset();

        // TODO: each worker is going to keep a map from *CaseSplit to an
        // object of class DnCStatistics, which contains some basic
        // statistics. The maps are owned by the DnCManager.

        // Apply the split and solve
        _engine->applySnCSplit( *split, queryId );

        bool fullSolveNeeded = true; // denotes whether we need to solve the subquery
        if ( restoreTreeStates && searchTreeState )
            fullSolveNeeded = _engine->restoreSearchTreeState( *searchTreeState );
        ExitCode result = ExitCode::NOT_DONE;
        if ( fullSolveNeeded )
        {
            _engine->solve( timeoutInSeconds );
            result = _engine->getExitCode();
        }
        else
        {
            // UNSAT is proven when replaying stack-entries
            result = ExitCode::UNSAT;
        }

        if ( _verbosity > 0 )
            printProgress( queryId, result );
        // Switch on the result
        if ( result == ExitCode::UNSAT )
        {
            // If UNSAT, continue to solve
            *_numUnsolvedSubQueries -= 1;
            if ( _numUnsolvedSubQueries->load() == 0 || _parallelDeepSoI )
                *_shouldQuitSolving = true;
            delete subQuery;
        }
        else if ( result == ExitCode::TIMEOUT )
        {
            // If TIMEOUT, split the current input region and add the
            // new subQueries to the current queue
            SubQueries subQueries;
            unsigned newTimeout = ( depth >= GlobalConfiguration::DNC_DEPTH_THRESHOLD - 1
                                        ? 0
                                        : (unsigned)timeoutInSeconds * _timeoutFactor );
            unsigned numNewSubQueries = pow( 2, _onlineDivides );
            std::vector<std::unique_ptr<SearchTreeState>> newSearchTreeStates;
            if ( restoreTreeStates )
            {
                // create |numNewSubQueries| copies of the current SearchTreeState
                for ( unsigned i = 0; i < numNewSubQueries; ++i )
                {
                    newSearchTreeStates.push_back( std::unique_ptr<SearchTreeState>( new SearchTreeState() ) );
                    _engine->storeSearchTreeState( *( newSearchTreeStates[i] ) );
                }
            }

            _queryDivider->createSubQueries(
                numNewSubQueries, queryId, depth, *split, newTimeout, subQueries );

            unsigned i = 0;
            for ( auto &newSubQuery : subQueries )
            {
                // Store the SearchTreeHandler state
                if ( restoreTreeStates )
                {
                    newSubQuery->_searchTreeState = std::move( newSearchTreeStates[i++] );
                }

                if ( !_workload->push( std::move( newSubQuery ) ) )
                {
                    throw MarabouError( MarabouError::UNSUCCESSFUL_QUEUE_PUSH );
                }

                *_numUnsolvedSubQueries += 1;
            }
            *_numUnsolvedSubQueries -= 1;
            delete subQuery;
        }
        else if ( result == ExitCode::QUIT_REQUESTED )
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
            if ( result == ExitCode::SAT )
            {
                // case SAT
                *_numUnsolvedSubQueries -= 1;
                delete subQuery;
            }
            else if ( result == ExitCode::ERROR )
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
        std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );
    }
}

void DnCWorker::printProgress( String queryId, ExitCode result ) const
{
    printf( "Worker %d: Query %s %s, %d tasks remaining\n",
            _threadId,
            queryId.ascii(),
            exitCodeToString( result ).ascii(),
            _numUnsolvedSubQueries->load() );
}

String DnCWorker::exitCodeToString( ExitCode result )
{
    switch ( result )
    {
    case ExitCode::UNSAT:
        return "unsat";
    case ExitCode::SAT:
        return "sat";
    case ExitCode::ERROR:
        return "ERROR";
    case ExitCode::TIMEOUT:
        return "TIMEOUT";
    case ExitCode::QUIT_REQUESTED:
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
