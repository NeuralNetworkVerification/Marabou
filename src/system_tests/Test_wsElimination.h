/*********************                                                        */
/*! \file Test_wsElimination.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Amir
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2020 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

**/


#ifndef MARABOU_TEST_WSELIMINATION_H
#define MARABOU_TEST_WSELIMINATION_H



#include <cxxtest/TestSuite.h>
#include "AcasParser.h"
#include "Engine.h"
#include "FloatUtils.h"
#include "GlobalConfiguration.h"
#include "InputQuery.h"
#include "Preprocessor.h"
#include "QueryLoader.h"
#include "MarabouError.h"

class wsEliminationTestSuite : public CxxTest::TestSuite
{
public:

    void setUp()
    {
    }

    void tearDown()
    {
    }

    void test_nlr_to_query_and_back_1()
    {
        Engine engine;
        InputQuery inputQuery = QueryLoader::loadQuery (
                RESOURCES_DIR "/bnn_queries/smallBNN_original" );

        // Fix the input
        for ( unsigned inputVariable = 0; inputVariable  < 784; ++ inputVariable )
        {
            auto pixel = 0.5;

            inputQuery.setLowerBound( inputVariable, pixel );
            inputQuery.setUpperBound( inputVariable, pixel );
        }

        TS_ASSERT_THROWS_NOTHING( engine.processInputQuery( inputQuery ) );
        TS_ASSERT_THROWS_NOTHING( engine.solve() );

        engine.extractSolution( inputQuery );

        // The output vector is - [-2.0, -8.0, 10.0, 16.0, -18.0, 2.0, 6.0, 0.0, 2.0, 2.0]
        TS_ASSERT_EQUALS( inputQuery.getSolutionValue( 784 ), -2 );
        TS_ASSERT_EQUALS( inputQuery.getSolutionValue( 785 ), -8 );
        TS_ASSERT_EQUALS( inputQuery.getSolutionValue( 786 ), 10 );
        TS_ASSERT_EQUALS( inputQuery.getSolutionValue( 787 ), 16 );
        TS_ASSERT_EQUALS( inputQuery.getSolutionValue( 788 ), -18 );
        TS_ASSERT_EQUALS( inputQuery.getSolutionValue( 789 ), 2 );
        TS_ASSERT_EQUALS( inputQuery.getSolutionValue( 790 ), 6 );
        TS_ASSERT_EQUALS( inputQuery.getSolutionValue( 791 ), 0 );
        TS_ASSERT_EQUALS( inputQuery.getSolutionValue( 792 ), 2 );
        TS_ASSERT_EQUALS( inputQuery.getSolutionValue( 793 ), 2 );
    }


    void test_nlr_to_query_and_back_2()
    {
        Engine engine;
        InputQuery inputQuery = QueryLoader::loadQuery (
                RESOURCES_DIR "/bnn_queries/smallBNN_parsed" );

        // Fix the input
        for ( unsigned inputVariable = 0; inputVariable  < 784; ++ inputVariable )
        {
            auto pixel = 0.5;

            inputQuery.setLowerBound( inputVariable, pixel );
            inputQuery.setUpperBound( inputVariable, pixel );
        }

        TS_ASSERT_THROWS_NOTHING( engine.processInputQuery( inputQuery ) );
        TS_ASSERT_THROWS_NOTHING( engine.solve() );

        engine.extractSolution( inputQuery );

        // The output vector is - [-2.0, -8.0, 10.0, 16.0, -18.0, 2.0, 6.0, 0.0, 2.0, 2.0]
        TS_ASSERT_EQUALS( inputQuery.getSolutionValue( 784 ), -2 );
        TS_ASSERT_EQUALS( inputQuery.getSolutionValue( 785 ), -8 );
        TS_ASSERT_EQUALS( inputQuery.getSolutionValue( 786 ), 10 );
        TS_ASSERT_EQUALS( inputQuery.getSolutionValue( 787 ), 16 );
        TS_ASSERT_EQUALS( inputQuery.getSolutionValue( 788 ), -18 );
        TS_ASSERT_EQUALS( inputQuery.getSolutionValue( 789 ), 2 );
        TS_ASSERT_EQUALS( inputQuery.getSolutionValue( 790 ), 6 );
        TS_ASSERT_EQUALS( inputQuery.getSolutionValue( 791 ), 0 );
        TS_ASSERT_EQUALS( inputQuery.getSolutionValue( 792 ), 2 );
        TS_ASSERT_EQUALS( inputQuery.getSolutionValue( 793 ), 2 );
    }


    void test_nlr_to_query_and_back_3()
    {
        Engine engine;
        InputQuery inputQuery = QueryLoader::loadQuery (
                RESOURCES_DIR "/bnn_queries/smallBNN_original" );

        // Fix the input
        for ( unsigned inputVariable = 0; inputVariable  < 784; ++ inputVariable )
        {
            auto pixel = 0.123;

            inputQuery.setLowerBound( inputVariable, pixel );
            inputQuery.setUpperBound( inputVariable, pixel );
        }

        TS_ASSERT_THROWS_NOTHING( engine.processInputQuery( inputQuery ) );
        TS_ASSERT_THROWS_NOTHING( engine.solve() );

        engine.extractSolution( inputQuery );

        // The output vector is - [0.0, -2.0, 8.0, 14.0, -24.0, 4.0, 0.0, 6.0, 0.0, 0.0]
        TS_ASSERT_EQUALS( inputQuery.getSolutionValue( 784 ), 0 );
        TS_ASSERT_EQUALS( inputQuery.getSolutionValue( 785 ), -2 );
        TS_ASSERT_EQUALS( inputQuery.getSolutionValue( 786 ), 8 );
        TS_ASSERT_EQUALS( inputQuery.getSolutionValue( 787 ), 14 );
        TS_ASSERT_EQUALS( inputQuery.getSolutionValue( 788 ), -24 );
        TS_ASSERT_EQUALS( inputQuery.getSolutionValue( 789 ), 4 );
        TS_ASSERT_EQUALS( inputQuery.getSolutionValue( 790 ), 0 );
        TS_ASSERT_EQUALS( inputQuery.getSolutionValue( 791 ), 6 );
        TS_ASSERT_EQUALS( inputQuery.getSolutionValue( 792 ), 0 );
        TS_ASSERT_EQUALS( inputQuery.getSolutionValue( 793 ), 0 );
    }

    void test_nlr_to_query_and_back_4()
    {
        Engine engine;
        InputQuery inputQuery = QueryLoader::loadQuery (
                RESOURCES_DIR "/bnn_queries/smallBNN_parsed" );

        // Fix the input
        for ( unsigned inputVariable = 0; inputVariable  < 784; ++ inputVariable )
        {
            auto pixel = 0.123;

            inputQuery.setLowerBound( inputVariable, pixel );
            inputQuery.setUpperBound( inputVariable, pixel );
        }

        TS_ASSERT_THROWS_NOTHING( engine.processInputQuery( inputQuery ) );
        TS_ASSERT_THROWS_NOTHING( engine.solve() );

        engine.extractSolution( inputQuery );

        // The output vector is - [0.0, -2.0, 8.0, 14.0, -24.0, 4.0, 0.0, 6.0, 0.0, 0.0]
        TS_ASSERT_EQUALS( inputQuery.getSolutionValue( 784 ), 0 );
        TS_ASSERT_EQUALS( inputQuery.getSolutionValue( 785 ), -2 );
        TS_ASSERT_EQUALS( inputQuery.getSolutionValue( 786 ), 8 );
        TS_ASSERT_EQUALS( inputQuery.getSolutionValue( 787 ), 14 );
        TS_ASSERT_EQUALS( inputQuery.getSolutionValue( 788 ), -24 );
        TS_ASSERT_EQUALS( inputQuery.getSolutionValue( 789 ), 4 );
        TS_ASSERT_EQUALS( inputQuery.getSolutionValue( 790 ), 0 );
        TS_ASSERT_EQUALS( inputQuery.getSolutionValue( 791 ), 6 );
        TS_ASSERT_EQUALS( inputQuery.getSolutionValue( 792 ), 0 );
        TS_ASSERT_EQUALS( inputQuery.getSolutionValue( 793 ), 0 );
    }
};

//
// Local Variables:
// compile-command: "make -C ../../.. "
// tags-file-name: "../../../TAGS"
// c-basic-offset: 4
// End:
//



#endif //MARABOU_TEST_WSELIMINATION_H
