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
#include "InputQuery.h"
#include "LookAheadPreprocessor.h"
#include "GlobalConfiguration.h"

#include <thread>

LookAheadPreprocessor::LookAheadPreprocessor( unsigned numWorkers,
                                              const InputQuery &inputQuery,
                                              unsigned splitThreshold )
    : _workload( 0 )
    , _numWorkers ( numWorkers )
    , _baseInputQuery( inputQuery )
    , _initialTimeout( splitThreshold )
{
    createEngines();
}

LookAheadPreprocessor::~LookAheadPreprocessor()
{
    for ( const auto &engine : _engines )
        delete engine;
}

void LookAheadPreprocessor::preprocessWorker( LookAheadPreprocessor::WorkerQueue
                                              &workload, Engine *engine,
                                              InputQuery *inputQuery,
                                              unsigned threadId,
                                              Map<unsigned, unsigned>
                                              &idToPhase,
                                              std::atomic_bool
                                              &shouldQuitPreprocessing,
                                              std::mutex &mtx,
                                              std::atomic_int &lastFixed,
                                              std::atomic_int &maxTime,
                                              unsigned splitThreshold )
{

    printf("Worker started");
    if ( !engine->_processed )
    {
        engine->processInputQuery( *inputQuery, false );
	engine->setConstraintViolationThreshold( splitThreshold );
    }

    // Apply all splits
    engine->applySplits( idToPhase );
    engine->propagate();
    do
    {
        engine->performSymbolicBoundTightening();
    } while ( engine->applyAllValidConstraintCaseSplits() );

    Map<unsigned, double> balanceEstimates;
    Map<unsigned, double> runtimeEstimates;
    engine->getEstimatesReal( balanceEstimates, runtimeEstimates );

    mtx.lock();
    for ( const auto &entry : engine->_smtCore._impliedIdToPhaseAtRoot )
        idToPhase[entry.first] = entry.second;
    mtx.unlock();

    unsigned prevSize = idToPhase.size();

    // Repeatedly pop from queue
    while ( !workload.empty() )
    {
        unsigned id = 0;
        workload.pop( id );
        if ( (int) id == lastFixed.load() )
        {
            std::cout << "No new info for subsequent constraints!" << std::endl;
            shouldQuitPreprocessing = true;
            return;
        }

        // Sync up
        if ( idToPhase.size() > prevSize )
        {
            prevSize = idToPhase.size();
            engine->applySplits( idToPhase );
        }
        PiecewiseLinearConstraint *plConstraint = engine->
            getConstraintFromId( id );


        float balanceEstimate = balanceEstimates[id];
        if ( (!plConstraint->isActive()) || plConstraint->phaseFixed()
             || runtimeEstimates[id] > runtimeEstimates.size() )
            continue;

        engine->storeInitialEngineState();

        // Try to propagate
        PiecewiseLinearCaseSplit _caseSplit;
        if ( balanceEstimate > 0 )
            _caseSplit = ((ReluConstraint *)plConstraint)->getInactiveSplit();
        else
            _caseSplit = ((ReluConstraint *)plConstraint)->getActiveSplit();
        EngineState *stateBeforeSplit = new EngineState();
        engine->storeState( *stateBeforeSplit, true );
        auto caseSplit = PiecewiseLinearCaseSplit();
        for ( const auto &bound : _caseSplit.getBoundTightenings() )
            caseSplit.storeBoundTightening( bound );

        engine->applySplit( caseSplit );

        struct timespec start = TimeUtils::sampleMicro();
        engine->solve( 2 );
        struct timespec end = TimeUtils::sampleMicro();
        unsigned long long totalElapsed = TimeUtils::timePassed( start, end );
        unsigned time =  totalElapsed / 1000000;
        if ( static_cast<int>(time) > maxTime.load() )
            maxTime = static_cast<int>(time);

        if ( engine->_exitCode == IEngine::UNSAT )
        {
            printf("Thread %u fixed relu %u\n", threadId, plConstraint->getId() );
            lastFixed = plConstraint->getId();
            mtx.lock();
            idToPhase[plConstraint->getId()] = ( balanceEstimate > 0 ?
                                                 ReluConstraint::PHASE_ACTIVE :
                                                 ReluConstraint::PHASE_INACTIVE );
            prevSize += 1;
            mtx.unlock();
        }
        if ( engine->_exitCode == IEngine::QUIT_REQUESTED )
            return;
        if ( engine->_exitCode == IEngine::ERROR )
            return;

        engine->reset();
        engine->restoreState( *stateBeforeSplit );

        engine->applyAllValidConstraintCaseSplits();
    }
}

bool LookAheadPreprocessor::run( Map<unsigned, unsigned> &idToPhase,
				 List<unsigned> &maxTimes )
{
    bool progressMade = true;
    //Vector<Map<unsigned, unsigned>> allIdToPhase;

    // Prepare the mechanism through which we can ask the engines to quit
    List<std::atomic_bool *> quitThreads;
    for ( unsigned i = 0; i < _numWorkers; ++i )
        quitThreads.append( _engines[i]->getQuitRequested() );

    std::atomic_bool shouldQuitPreprocessing( false );

    for ( const auto plConstraint : _baseInputQuery.getPiecewiseLinearConstraints() )
        _allPiecewiseLinearConstraints.append( plConstraint->getId() );

    std::mutex mtx;
    std::atomic_int lastFixed ( -1 );

    while ( progressMade )
    {
	std::atomic_int maxTime ( 0 );
        std::cout << "new look ahead preprocessing iteration" << std::endl;
        unsigned previousSize = idToPhase.size();
        for ( auto id : _allPiecewiseLinearConstraints )
            if ( !_workload.push( id ) )
                std::cout << "Pushed failed!" << std::endl;
        // Spawn threads and start solving
        std::list<std::thread> threads;
        for ( unsigned threadId = 0; threadId < _numWorkers; ++threadId )
        {
            threads.push_back( std::thread( preprocessWorker,
                                            std::ref( _workload ),
                                            _engines[threadId],
                                            _inputQueries[threadId],
                                            threadId,
                                            std::ref( idToPhase ),
                                            std::ref( shouldQuitPreprocessing ),
                                            std::ref( mtx ),
                                            std::ref( lastFixed ),
                                            std::ref( maxTime ),
                                            _initialTimeout ) );
        }

        while ( (!shouldQuitPreprocessing.load()) && (!_workload.empty()) )
        {
            std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );
        }

        if ( shouldQuitPreprocessing.load() )
            for ( auto &quitThread : quitThreads )
                *quitThread =true;

        for ( auto &thread : threads )
            thread.join();

	maxTimes.append( maxTime.load() );
	    
        if ( shouldQuitPreprocessing.load() )
        {
            std::cout << "Preprocessing done!" << std::endl;
            std::cout << "Number of fixed Relus: " << idToPhase.size() << std::endl;
            return lastFixed.load() != -2;
        }

        if ( idToPhase.size() > previousSize && lastFixed.load() != -1 )
        {
            progressMade = true;
        }
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
        Engine *engine = new Engine( 0 );
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
