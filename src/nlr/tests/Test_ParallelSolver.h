/**********************/
/*! \file Test_WsLayerElimination.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Haoze (Andrew) Wu
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

**/

#include <cxxtest/TestSuite.h>
#include "GurobiWrapper.h"
#include "IterativePropagator.h"
#include "NetworkLevelReasoner.h"
#include "ParallelSolver.h"

class MockForNetworkLevelReasoner
{
public:
};

class NetworkLevelReasonerTestSuite : public CxxTest::TestSuite
{
public:

    void setUp()
    {
    }

    void tearDown()
    {
    }

    void test_enqueueSolver()
    {
        NLR::NetworkLevelReasoner nlr;
        NLR::IterativePropagator mock = NLR::IterativePropagator( &nlr );
        unsigned numberOfWorkers = 4;
        NLR::ParallelSolver::SolverQueue solvers ( numberOfWorkers );
        GurobiWrapper *gurobi = new GurobiWrapper();
        TS_ASSERT_THROWS_NOTHING( mock.enqueueSolver( solvers, gurobi) );
        TS_ASSERT( !solvers.empty() );
        GurobiWrapper *gurobiPtr = NULL;
        TS_ASSERT_THROWS_NOTHING( solvers.pop( gurobiPtr ) );
        TS_ASSERT( solvers.empty() );
        delete gurobiPtr;
    }

    void test_clear_solver_queue()
    {
        NLR::NetworkLevelReasoner nlr;
        NLR::IterativePropagator mock = NLR::IterativePropagator( &nlr );
        unsigned numberOfWorkers = 4;
        NLR::ParallelSolver::SolverQueue solvers ( numberOfWorkers );
        for ( unsigned i = 0; i < numberOfWorkers; ++i )
        {
            GurobiWrapper *gurobi = new GurobiWrapper();
            TS_ASSERT_THROWS_NOTHING( mock.enqueueSolver( solvers, gurobi) );
        }
        TS_ASSERT( !solvers.empty() );
        TS_ASSERT_THROWS_NOTHING( mock.clearSolverQueue( solvers ) );
        TS_ASSERT( solvers.empty() );
    }
};
