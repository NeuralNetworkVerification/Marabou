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
                                              *workload, Engine *engine,
                                              InputQuery *inputQuery,
                                              unsigned threadId,
                                              Map<unsigned, unsigned>
                                              &idToPhase,
                                              std::atomic_bool
                                              &shouldQuitPreprocessing )
{
    unsigned cpuId = 0;
    getCPUId( cpuId );
    printf( "Thread #%u on CPU %u\n", threadId, cpuId );

    if ( !engine )
    {
        engine = new Engine( 0 );
        engine->processInputQuery( *inputQuery, false );
        std::cout << "Engine created!" << std::endl;
    }
    // Apply all splits
    engine->applySplits( idToPhase );
    idToPhase.clear();

    do
    {
        engine->performSymbolicBoundTightening();
    } while ( engine->applyAllValidConstraintCaseSplits() );
    idToPhase = engine->_smtCore._impliedIdToPhaseAtRoot;

    // Repeatedly pop from queue
    while ( !workload->empty() )
    {
        unsigned id = 0;
        workload->pop( id );

        std::cout << id << std::endl;

        PiecewiseLinearConstraint *plConstraint = engine->
            getConstraintFromId( id );

        if ( (!plConstraint->isActive()) || plConstraint->phaseFixed() )
            continue;
        auto caseSplits = plConstraint->getCaseSplits();

        EngineState *stateBeforeSplit = new EngineState();
        engine->storeState( *stateBeforeSplit, true );
        Map<unsigned, unsigned> commonImpliedIdToPhase;
        Map<unsigned, unsigned> idToCount;
        Vector<List<PiecewiseLinearCaseSplit>> feasibleImpliedSplits;
        Vector<Map<unsigned, unsigned>> feasibleImpliedIdToPhase;
        Vector<unsigned> feasibleStatus;
        for ( const auto &caseSplit : caseSplits )
        {
            engine->applySplit( caseSplit );
            engine->quickSolve();
            if ( engine->_exitCode != IEngine::UNSAT )
            {
                List<PiecewiseLinearCaseSplit> temp = engine->
                    _smtCore._impliedValidSplitsAtRoot;
                feasibleImpliedSplits.append( temp );

                Map<unsigned, unsigned> tempMap = engine->
                    _smtCore._impliedIdToPhaseAtRoot;
                feasibleImpliedIdToPhase.append( tempMap );

                for ( const auto &entry : tempMap )
                {
                    if ( !commonImpliedIdToPhase.exists( entry.first ) )
                    {
                        commonImpliedIdToPhase[entry.first] = entry.second;
                        idToCount[entry.first] = 0;
                    }
                    if ( commonImpliedIdToPhase[entry.first] == entry.second )
                        idToCount[entry.first] += 1;
                }
                feasibleStatus.append( ( ( ReluConstraint * ) plConstraint )
                                       ->getPhaseStatus() );
            }
            engine->reset();
            engine->restoreState( *stateBeforeSplit );
        }
        if ( feasibleImpliedSplits.size() == 0 )
        {
            engine->_exitCode = IEngine::UNSAT;
            std::cout << "Finished preprocessing early!" << std::endl;
            shouldQuitPreprocessing = true;
            return;
        }
        else if ( feasibleImpliedSplits.size() == 1 )
        {
            for ( const auto &feasibleImpliedSplit : feasibleImpliedSplits[0] )
                engine->applySplit( feasibleImpliedSplit );
            for ( const auto &entry : feasibleImpliedIdToPhase[0] )
                idToPhase[entry.first] = entry.second;
            idToPhase[plConstraint->getId()] = feasibleStatus[0];
        }
        else
        {
            unsigned commonCount = 0;
            for ( const auto &entry : commonImpliedIdToPhase )
            {
                if ( idToCount[entry.first] == caseSplits.size() )
                {
                    idToPhase[entry.first] = entry.second;
                    commonCount += 1;
                }
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
}

bool LookAheadPreprocessor::run( Map<unsigned, unsigned> &idToPhase )
{
    bool progressMade = true;
    Vector<Map<unsigned, unsigned>> allIdToPhase;

    // Prepare the mechanism through which we can ask the engines to quit
    List<std::atomic_bool *> quitThreads;
    for ( unsigned i = 0; i < _numWorkers; ++i )
        quitThreads.append( _engines[i]->getQuitRequested() );

    std::atomic_bool shouldQuitPreprocessing( false );

    while ( progressMade )
    {
        allIdToPhase.clear();
        for ( unsigned i = 0; i < _numWorkers; ++i )
        {
            allIdToPhase.append( idToPhase );
        }
        for ( const auto plConstraint : _baseInputQuery.getPiecewiseLinearConstraints() )
            _allPiecewiseLinearConstraints.append( plConstraint->getId() );
        for ( auto id : _allPiecewiseLinearConstraints )
            if ( !_workload->push( id ) )
                std::cout << "Pushed failed!" << std::endl;
        // Spawn threads and start solving
        std::list<std::thread> threads;
        for ( unsigned threadId = 0; threadId < _numWorkers; ++threadId )
        {
            threads.push_back( std::thread( preprocessWorker, _workload,
                                            _engines[threadId],
                                            _inputQueries[threadId],
                                            threadId,
                                            std::ref( allIdToPhase[threadId] ),
                                            std::ref( shouldQuitPreprocessing )
                                            ) );
        }

        while ( (!shouldQuitPreprocessing.load()) && (!_workload->empty()) )
        {
            std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );
        }

        if ( shouldQuitPreprocessing.load() )
            for ( auto &quitThread : quitThreads )
                *quitThread =true;

        for ( auto &thread : threads )
            thread.join();

        if ( shouldQuitPreprocessing.load() )
            return false;

        unsigned previousSize = idToPhase.size();
        for ( const auto &_idToPhase : allIdToPhase )
            for ( auto &entry : _idToPhase )
                idToPhase[entry.first] = entry.second;

        if ( idToPhase.size() > previousSize )
            progressMade = true;
        else
            progressMade = false;
        std::cout << "Number of fixed Relus: " << idToPhase.size() << std::endl;
    }
    std::cout << "Preprocessing done!" << std::endl;
    std::cout << "Number of fixed Relus: " << idToPhase.size() << std::endl;
    return true;
}

void LookAheadPreprocessor::createEngines()
{
    // Create engines for each thread
    for ( unsigned i = 0; i < _numWorkers; ++i )
    {
        Engine *engine = NULL;
        InputQuery *inputQuery = new InputQuery();
        *inputQuery = _baseInputQuery;
        _engines.append( engine );
        _inputQueries.append( inputQuery );

    }
}

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
