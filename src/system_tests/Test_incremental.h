/*********************                                                        */
/*! \file Test_RowBoundTightener.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2024 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

**/

#include "Engine.h"
#include "FloatUtils.h"
#include "InputQuery.h"

#include <cxxtest/TestSuite.h>

class IncrementalTestSuite : public CxxTest::TestSuite
{
public:
    void setUp()
    {
    }

    void tearDown()
    {
    }


    void test_incremental_solving()
    {
        InputQuery inputQuery;
        // x0 + x1 = 0
        // x1 + x2 = 0
        // -1 <= x1 <= 1
        inputQuery.setNumberOfVariables( 3 );

        {
            Equation equation;
            equation.addAddend( 1, 0 );
            equation.addAddend( 1, 1 );
            inputQuery.addEquation( equation );
        }

        {
            Equation equation;
            equation.addAddend( 1, 1 );
            equation.addAddend( 1, 2 );
            inputQuery.addEquation( equation );
        }

        inputQuery.setLowerBound( 1, -1 );
        inputQuery.setUpperBound( 1, 1 );

        {
            Engine engine;
            TS_ASSERT_THROWS_NOTHING( engine.processInputQuery( inputQuery ) );
            TS_ASSERT_THROWS_NOTHING( engine.solve() );

            TS_ASSERT_EQUALS( engine.getExitCode(), IEngine::SAT );
            TS_ASSERT_THROWS_NOTHING( engine.extractSolution( inputQuery ) );
            inputQuery.push();
        }

        {
            // Now additionally, x2 >= 2
            Engine engine;
            inputQuery.setLowerBound( 2, 2 );
            TS_ASSERT( !engine.processInputQuery( inputQuery ) );
            TS_ASSERT_EQUALS( engine.getExitCode(), IEngine::UNSAT );
            inputQuery.pop();
        }

        {
            // Now additionally, x2 >= 0
            Engine engine;
            inputQuery.setLowerBound( 2, 0 );
            TS_ASSERT( engine.processInputQuery( inputQuery ) );
            TS_ASSERT_THROWS_NOTHING( engine.solve() );

            TS_ASSERT_EQUALS( engine.getExitCode(), IEngine::SAT );
            TS_ASSERT_THROWS_NOTHING( engine.extractSolution( inputQuery ) );
            inputQuery.push();
        }
    }
};
