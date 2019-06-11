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

DnCWorker::DnCWorker( WorkerQueue* workload, std::shared_ptr<Engine> engine,
                      std::atomic_uint& numUnsolvedSubqueries,
                      std::atomic_bool& foundSolutionInSomeThread,
                      unsigned threadId, unsigned onlineDivides,
                      float timeoutFactor, DivideStrategy divideStrategy )
  : _workload ( workload )
  , _engine ( engine )
  , _numUnsolvedSubqueries ( &numUnsolvedSubqueries )
  , _foundSolutionInSomeThread ( &foundSolutionInSomeThread )
  , _threadId ( threadId )
  , _onlineDivides ( onlineDivides )
  , _timeoutFactor ( timeoutFactor )
{
  setQueryDivider( divideStrategy );

  // Obtain the current state of the engine
  _initialState = std::make_shared<EngineState>();
  _initialState->_stateId = _threadId;
  _engine->storeState( *_initialState, true );
}

void DnCWorker::setQueryDivider( DivideStrategy divideStrategy )
{
  // For now, there is only one strategy
  assert( divideStrategy == DivideStrategy::LargestInterval );
  const List<unsigned>& inputVariables = _engine->getInputVariables();
  _queryDivider = std::unique_ptr<LargestIntervalDivider>
    ( new LargestIntervalDivider( inputVariables,  _timeoutFactor ) );
}

void DnCWorker::run()
{
  while ( _numUnsolvedSubqueries->load() > 0 )
    {
      std::unique_ptr<SubQuery> subquery = nullptr;
      // Boost queue stores the next element into the passed-in pointer
      // and returns true if the pop is successful (aka, the queue is not empty
      // in most cases)
      if ( _workload->pop( subquery ) )
        {
          String queryId =  subquery->_queryId;
          PiecewiseLinearCaseSplit split = *( subquery->_split );
          unsigned timeoutInSeconds = subquery->_timeoutInSeconds;

          // Create a new statistics object for each subquery
          Statistics* statistics = new Statistics();
          _engine->resetStatistics( *statistics );

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
              SubQueries subqueries;
              _queryDivider->createSubQueries( pow( 2, _onlineDivides ),
                                               *subquery, subqueries );
              bool pushed = false;
              for ( auto& subquery : subqueries )
                {
                  pushed = _workload->push( std::move( subquery ) );
                  assert ( pushed );
                  *_numUnsolvedSubqueries += 1;
                }
              *_numUnsolvedSubqueries -= 1;
            }
          else if ( result == Engine::SAT )
            {
              // If SAT, set the corresponding flag to true,
              // and quit the thread
              *_foundSolutionInSomeThread = true;
              *_numUnsolvedSubqueries -= 1;
              return;
            }
          else if ( result == Engine::ERROR )
            {
              // If ERROR, quit solving
              std::cout << "Error!" << std::endl;
              *_numUnsolvedSubqueries -= 1;
              return;
            }
          else if ( result == Engine::QUIT_REQUESTED )
            {
              // If engine was asked to quit
              std::cout << "Quit requested by manager!" << std::endl;
              *_numUnsolvedSubqueries -= 1;
              return;
            }
          else if ( result == Engine::NOT_DONE )
            {
              std::cout << "Not done! This should not happen" << std::endl;
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
          std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );
        }
    }
}

void DnCWorker::printProgress( const String& queryId,
                               const Engine::ExitCode result ) const
{
  printf( "Query %s %s, %d tasks remaining\n", queryId.ascii(),
          exitCodeToString( result ).c_str(), _numUnsolvedSubqueries->load() );
}

const std::string DnCWorker::exitCodeToString( const Engine::ExitCode result )
{
  switch ( result ){
  case  Engine::UNSAT:
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
