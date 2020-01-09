/*********************                                                        */
/*! \file Test_DnCWorker.h
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

#include <cxxtest/TestSuite.h>

#include "DnCWorker.h"
#include "MockEngine.h"

#include <string.h>

class DnCWorkerTestSuite : public CxxTest::TestSuite
{
public:

    WorkerQueue *_workload;
    std::shared_ptr<MockEngine> _engine;

    DnCWorkerTestSuite()
        : _workload( NULL )
    {
    }

    void setUp()
    {
        _workload = new WorkerQueue( 0 );

        // Initialize the mockEngine
        _engine = std::make_shared<MockEngine>();
        List<unsigned> inputVariables = { 1, 2, 3 };
        _engine->setInputVariables( inputVariables );
    }

    void tearDown()
    {
        if ( _workload )
        {
            clearSubQueries();
            delete _workload;
            _workload = NULL;
        }
    }

    // Clear workload and return the number of removed subQueries
    unsigned clearSubQueries()
    {
        unsigned counter = 0;
        SubQuery *subQuery;
        while ( !_workload->empty() )
        {
            _workload->pop( subQuery );
            delete subQuery;
            ++counter;
        }

        return counter;
    }

    void createPlaceHolderSubQuery()
    {
        // Add a subQuery to workload.
        // This subQuery serves only as a placeholder, as the exitCode of the
        // mock engine after solving this subQuery does not depend on it.

        SubQuery *subQuery = new SubQuery;

        auto split = std::unique_ptr<PiecewiseLinearCaseSplit>
            ( new PiecewiseLinearCaseSplit );
        Tightening bound1( 1, -2.0, Tightening::LB );
        Tightening bound2( 1, 2.0, Tightening::UB );
        Tightening bound3( 2, 3.0, Tightening::LB );
        Tightening bound4( 2, 5.0, Tightening::UB );
        Tightening bound5( 3, 2.0, Tightening::LB );
        Tightening bound6( 3, 5.0, Tightening::UB );
        split->storeBoundTightening( bound1 );
        split->storeBoundTightening( bound2 );
        split->storeBoundTightening( bound3 );
        split->storeBoundTightening( bound4 );
        split->storeBoundTightening( bound5 );
        split->storeBoundTightening( bound6 );

        subQuery->_queryId = "";
        subQuery->_split = std::move( split );
        subQuery->_timeoutInSeconds = 5;
        TS_ASSERT( _workload->push( std::move( subQuery ) ) );
    }

    // Test different branches of DnCWorker.popOneSubQueryAndSolve()
    void test_pop_one_sub_query_and_solve()
    {
        //  Pop a subQuery from the workload, set the mock engine to report
        //  timeout on solving the subQuery.
        //  In this case, we must further split the subQuery
        //
        //  1. The size of workload should become 4
        //  2. The value of numUnsolvedSubQueries should become 4
        //  3. The value of shouldQuitSolving should be false

        // Create the subQuery to be added to workLoad
        TS_ASSERT( clearSubQueries() == 0 );

        createPlaceHolderSubQuery();
        _engine->setTimeToSolve( 10 );
        _engine->setExitCode( IEngine::TIMEOUT );
        std::atomic_uint numUnsolvedSubQueries( 1 );
        std::atomic_bool shouldQuitSolving( false );
        unsigned threadId = 0;
        unsigned onlineDivides = 2;
        float timeoutFactor = 1;
        DivideStrategy divideStrategy = DivideStrategy::LargestInterval;
        DnCWorker dncWorker( _workload, _engine, numUnsolvedSubQueries,
                             shouldQuitSolving, threadId, onlineDivides,
                             timeoutFactor, divideStrategy, 10 );

        dncWorker.popOneSubQueryAndSolve();
        TS_ASSERT( _engine->getExitCode() == IEngine::TIMEOUT );
        TS_ASSERT( clearSubQueries() == 4 );
        TS_ASSERT( numUnsolvedSubQueries.load() == 4 );
        TS_ASSERT( !shouldQuitSolving.load() );

        //  Pop 2 subQueries from the workload, set the mock engine to report
        //  unsat on solving it.
        //  In this case, we only decrement the value of numUnsolvedSubQueries
        //
        //  1. The size of workload should become 1
        //  2. The value of numUnsolvedSubQueries should become 1
        //  3. The value of shouldQuitSolving should still be false
        TS_ASSERT( clearSubQueries() == 0 );

        createPlaceHolderSubQuery();
        createPlaceHolderSubQuery();
        _engine->setExitCode( IEngine::UNSAT );
        numUnsolvedSubQueries= 2;
        shouldQuitSolving= false;
		dncWorker = DnCWorker( _workload, _engine, numUnsolvedSubQueries,
                               shouldQuitSolving, threadId, onlineDivides,
                               timeoutFactor, divideStrategy, 10 );

        dncWorker.popOneSubQueryAndSolve();
        TS_ASSERT( _engine->getExitCode() == IEngine::UNSAT );
        TS_ASSERT( clearSubQueries() == 1 );
        TS_ASSERT( numUnsolvedSubQueries.load() == 1 );
        TS_ASSERT( !shouldQuitSolving.load() );

        //  Pop a subQuery from the workload, set the mock engine to report
        //  unsat on solving it.
        //  In this case, we only decrement the value of numUnsolvedSubQueries
        //
        //  1. The size of workload should become 0
        //  2. The value of numUnsolvedSubQueries should become 0
        //  3. The value of shouldQuitSolving should still be false

        // Create the subQuery to be added to workLoad
        TS_ASSERT( clearSubQueries() == 0 );

        createPlaceHolderSubQuery();
        _engine->setExitCode( IEngine::UNSAT );
        numUnsolvedSubQueries= 1;
        shouldQuitSolving= false;

        dncWorker = DnCWorker( _workload, _engine, numUnsolvedSubQueries,
                               shouldQuitSolving, threadId, onlineDivides,
                               timeoutFactor, divideStrategy, 10 );

        dncWorker.popOneSubQueryAndSolve();
        TS_ASSERT( _engine->getExitCode() == IEngine::UNSAT );
        TS_ASSERT( clearSubQueries() == 0 );
        TS_ASSERT( numUnsolvedSubQueries.load() == 0 );
        TS_ASSERT( shouldQuitSolving.load() );

        //  Pop a subQuery from the workload, set the mock engine to report
        //  sat on solving it.
        //  In this case, we decrement the value of numUnsolvedSubQueries, and
        //  set shouldQuitSolving to be true.
        //
        //  1. The size of workload should become 0
        //  2. The value of numUnsolvedSubQueries should become 0
        //  3. The value of shouldQuitSolving should become true
        TS_ASSERT( clearSubQueries() == 0 );

        createPlaceHolderSubQuery();
        _engine->setExitCode( IEngine::SAT );
        numUnsolvedSubQueries=( 1 );
        shouldQuitSolving=( false );
		dncWorker = DnCWorker( _workload, _engine, numUnsolvedSubQueries,
                               shouldQuitSolving, threadId, onlineDivides,
                               timeoutFactor, divideStrategy, 10 );

        dncWorker.popOneSubQueryAndSolve();
        TS_ASSERT( _engine->getExitCode() == IEngine::SAT );
        TS_ASSERT( clearSubQueries() == 0 );
        TS_ASSERT( numUnsolvedSubQueries.load() == 0 );
        TS_ASSERT( shouldQuitSolving.load() );

        //  Pop a subQuery from the workload, set the mock engine's exitCode
        //  to be QuitRequested, and shouldQuitSolving to be true.
        //  In this case, the DnCWorker should quit.
        //
        //  1. There should still be 1 unsolved subQuery

        // Create the subQuery to be added to workLoad
        TS_ASSERT( clearSubQueries() == 0 );

        createPlaceHolderSubQuery();
        _engine->setExitCode( IEngine::QUIT_REQUESTED );
        numUnsolvedSubQueries= 1;
        shouldQuitSolving =  true;
		dncWorker = DnCWorker( _workload, _engine, numUnsolvedSubQueries,
                               shouldQuitSolving, threadId, onlineDivides,
                               timeoutFactor, divideStrategy, 10 );
        dncWorker.popOneSubQueryAndSolve();
        TS_ASSERT( _engine->getExitCode() == IEngine::QUIT_REQUESTED );
        TS_ASSERT( numUnsolvedSubQueries.load() == 1 );

        //  Pop a subQuery from the workload, set the mock engine's exitCode
        //  to be error
        //  In this case, the DnCWorker should quit.
        //
        //  1. The value of numUnsolvedSubQueries should become 0
        //  2. The value of shouldQuitSolving should become true

        // Create the subQuery to be added to workLoad
        TS_ASSERT( clearSubQueries() == 0 );

        createPlaceHolderSubQuery();
        _engine->setExitCode( IEngine::ERROR );
         numUnsolvedSubQueries= 1;
        shouldQuitSolving = false;
		dncWorker = DnCWorker( _workload, _engine, numUnsolvedSubQueries,
                               shouldQuitSolving, threadId, onlineDivides,
                               timeoutFactor, divideStrategy, 10 );

        dncWorker.popOneSubQueryAndSolve();
        TS_ASSERT( _engine->getExitCode() == IEngine::ERROR );
        TS_ASSERT( numUnsolvedSubQueries.load() == 1 );
        TS_ASSERT( shouldQuitSolving.load() );
    }
};

//
// Local Variables:
// compile-command: "make -C ../../.. "
// tags-file-name: "../../../TAGS"
// c-basic-offset: 4
// End:
//
