/*********************                                                        */
/*! \file Test_IncrementalLinearization.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Teruhiro Tagomori
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2024 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

**/

#include "FloatUtils.h"
#include "IncrementalLinearization.h"
#include "MILPEncoder.h"
#include "MarabouError.h"
#include "MockTableau.h"
#include "Query.h"

#include <cxxtest/TestSuite.h>
#include <string.h>

class IncrementalLinearizationTestSuite : public CxxTest::TestSuite
{
public:
    void setUp()
    {
    }

    void tearDown()
    {
    }

    void test_solve_with_incremental_linearization()
    {
#ifdef ENABLE_GUROBI

        /*
         * Unknown case
         */
        GurobiWrapper gurobi1;

        Query inputQuery1 = Query();
        inputQuery1.setNumberOfVariables( 2 );

        MockTableau tableau1 = MockTableau();
        tableau1.setDimensions( 2, 2 );

        // 0 <= x0 <= 1
        inputQuery1.setLowerBound( 0, 0 );
        inputQuery1.setUpperBound( 0, 1 );
        tableau1.setLowerBound( 0, 0 );
        tableau1.setUpperBound( 0, 1 );

        // x1 = sigmoid( x0 )
        SigmoidConstraint *sigmoid1 = new SigmoidConstraint( 0, 1 );
        inputQuery1.addTranscendentalConstraint( sigmoid1 );
        inputQuery1.setLowerBound( 1, sigmoid1->sigmoid( 0 ) );
        inputQuery1.setUpperBound( 1, sigmoid1->sigmoid( 1 ) );
        tableau1.setLowerBound( 1, sigmoid1->sigmoid( 0 ) );
        tableau1.setUpperBound( 1, sigmoid1->sigmoid( 1 ) );

        MILPEncoder milp1( tableau1 );
        milp1.encodeQuery( gurobi1, inputQuery1 );

        TS_ASSERT_THROWS_NOTHING( gurobi1.solve() );

        TS_ASSERT( gurobi1.haveFeasibleSolution() );

        Map<String, double> solution1;
        double costValue1;

        TS_ASSERT_THROWS_NOTHING( gurobi1.extractSolution( solution1, costValue1 ) );

        TS_ASSERT( solution1.exists( "x0" ) );
        TS_ASSERT( solution1.exists( "x1" ) );

        IncrementalLinearization *incrLinear = new IncrementalLinearization( milp1 );
        IEngine::ExitCode exitCode = incrLinear->solveWithIncrementalLinearization(
            gurobi1, inputQuery1.getTranscendentalConstraints(), 1000000000000 );
        ASSERT( exitCode == IEngine::UNKNOWN );
#else
        TS_ASSERT( true );
#endif // ENABLE_GUROBI
    }
};