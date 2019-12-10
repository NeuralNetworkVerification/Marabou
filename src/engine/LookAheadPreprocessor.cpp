/*********************                                                        */
/*! \file LookAheadPreprocessor.cpp
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

#include "GetCPUData.h"
#include <InputQuery.h>
#include <LookAheadPreprocessor.h>

#include <thread>

void LookAheadPreprocessor::preprocessWorker( LookAheadPreprocessor::WorkerQueue
                                              *workload, std::shared_ptr<Engine>
                                              engine, unsigned threadId,
                                              List<PiecewiseLinearCaseSplit>
                                              &impliedCaseSplits )
{
    unsigned cpuId = 0;
    getCPUId( cpuId );
    printf( "Thread #%u on CPU %u\n", threadId, cpuId );

    while ( !workload->empty() )
    {
        List<PiecewiseLinearCaseSplit> *caseSplits = NULL;
        workload->pop( caseSplits );

        EngineState *stateBeforeSplit = new EngineState();
        engine->storeState( *stateBeforeSplit, true );
        Vector<List<PiecewiseLinearCaseSplit>> feasibleImpliedSplits;
        for ( const auto &caseSplit : *caseSplits )
        {
            engine->applySplit( caseSplit );
            engine->quickSolve();
            if ( engine->_exitCode != IEngine::UNSAT )
            {
                List<PiecewiseLinearCaseSplit> temp = engine->_smtCore._impliedValidSplitsAtRoot;
                feasibleImpliedSplits.append( temp );
            }
            engine->reset();
            engine->restoreState( *stateBeforeSplit );
        }
        if ( feasibleImpliedSplits.size() == 0 )
        {
            engine->_exitCode = IEngine::UNSAT;
            std::cout << "Finished preprocessing early!" << std::endl;
            return;
        }
        else if ( feasibleImpliedSplits.size() == 1 )
        {
            std::cout << "Fixed " << feasibleImpliedSplits[0].size() << " ReLUs" << std::endl;
            for ( const auto &feasibleImpliedSplit : feasibleImpliedSplits[0] )
            {
                engine->applySplit( feasibleImpliedSplit );
                impliedCaseSplits.append( feasibleImpliedSplit );
            }
        }
        engine->applyAllValidConstraintCaseSplits();
    }
}

LookAheadPreprocessor::LookAheadPreprocessor( unsigned numWorkers,
                                              const InputQuery &inputQuery )
    : _numWorkers ( numWorkers )
    , _baseInputQuery( inputQuery )
{
    createEngines();
    _workload = new LookAheadPreprocessor::WorkerQueue( 0 );
    for ( const auto plConstraint : inputQuery.getPiecewiseLinearConstraints() )
    {
        _allPiecewiseLinearCaseSplits.append( plConstraint->getCaseSplits() );
    }
}

void LookAheadPreprocessor::run( Map<unsigned, unsigned> &bToPhase )
{
    bool progressMade = true;
    while ( progressMade )
    {
        for ( auto &caseSplits : _allPiecewiseLinearCaseSplits )
            if ( !_workload->push( &caseSplits ) )
                std::cout << "Pushed failed!" << std::endl;

        List<List<PiecewiseLinearCaseSplit>> allImpliedCaseSplits;
        // Spawn threads and start solving
        std::list<std::thread> threads;
        for ( unsigned threadId = 0; threadId < _numWorkers; ++threadId )
        {
            List<PiecewiseLinearCaseSplit> impliedCaseSplits;
            allImpliedCaseSplits.append( impliedCaseSplits );
            threads.push_back( std::thread( preprocessWorker, _workload,
                                            _engines[ threadId ], threadId,
                                            std::ref( impliedCaseSplits ) ) );
        }

        for ( auto &thread : threads )
            thread.join();

        for ( const auto &impliedCaseSplits : allImpliedCaseSplits )
        {
            for ( const auto &split : impliedCaseSplits )
                split.dump();
        }
        std::cout << bToPhase.size() << std::endl;
        break;
    }
}

void LookAheadPreprocessor::createEngines()
{
    // Create engines for each thread
    for ( unsigned i = 0; i < _numWorkers; ++i )
    {
        auto engine = std::make_shared<Engine>( 0 );
        InputQuery *inputQuery = new InputQuery();
        *inputQuery = _baseInputQuery;
        engine->processInputQuery( *inputQuery );

        _engines.append( engine );
    }
}

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
