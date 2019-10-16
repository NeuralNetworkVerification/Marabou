/*********************                                                        */
/*! \file Test_RowBoundTightener.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

**/

#include <cxxtest/TestSuite.h>

#include "Engine.h"
#include "InputQuery.h"
#include "MpsParser.h"
/* #include <cstdio> */


class MpsTestSuite : public CxxTest::TestSuite
{
public:

    void setUp()
    {
    }

    void tearDown()
    {
    }

    void test_infesiable() 
    {
        const char *filename = "./lp_infeasible_1.mps";

        // Extract an input query from the network
        InputQuery inputQuery;

        MpsParser mpsParser( filename );
        mpsParser.generateQuery( inputQuery );
        Engine engine;
        if ( ! engine.processInputQuery( inputQuery ) )
        {
            // got infeasible in preprocess stage
            TS_ASSERT( 1 );
        }
        else {
            TS_ASSERT_THROWS_NOTHING ( ! engine.solve() );
        }
    }

    void test_fesiable() 
    {
        const char *filename = "./lp_feasible_1.mps";

        // Extract an input query from the network
        InputQuery inputQuery;

        MpsParser mpsParser( filename );
        mpsParser.generateQuery( inputQuery );
        Engine engine;
        TS_ASSERT_THROWS_NOTHING ( engine.processInputQuery( inputQuery ) );
        TS_ASSERT_THROWS_NOTHING ( engine.solve() );
        engine.extractSolution( inputQuery );

        // Sanity test
        double value = 0;

        double value0 = inputQuery.getSolutionValue( 0 );
        double value1 = inputQuery.getSolutionValue( 1 );
        double value2 = inputQuery.getSolutionValue( 2 );

        value += 1  * value0;
        value += 2  * value1;
        value += -1 * value2;

        TS_ASSERT( FloatUtils::lte( value, 11 ) )

        TS_ASSERT( value0 >= 0 ); 
        TS_ASSERT( value0 <= 2 ); 
        TS_ASSERT( value1 >= -3 ); 
        TS_ASSERT( value1 <= 3 ); 
        TS_ASSERT( value2 >= 4 ); 
        TS_ASSERT( value2 <= 6 ); 
    }
};

//
// Local Variables:
// compile-command: "make -C ../../.. "
// tags-file-name: "../../../TAGS"
// c-basic-offset: 4
// End:
//
