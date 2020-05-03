/*********************                                                        */
/*! \file Test_QueryLoader.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Kyle Julian
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

**/

#include <cxxtest/TestSuite.h>

#include "AutoFile.h"
#include "Equation.h"
#include "InputQuery.h"
#include "MockFileFactory.h"
#include "QueryLoader.h"
#include "ReluConstraint.h"
#include "T/unistd.h"

const String QUERY_TEST_FILE( "QueryTest.txt" );

class MockForQueryLoader
    : public MockFileFactory
    , public T::Base_stat
{
public:
    int stat( const char */* path */, StructStat */* buf */ )
    {
        // 0 means file exists
        return 0;
    }
};

class QueryLoaderTestSuite : public CxxTest::TestSuite
{
public:
    MockForQueryLoader *mock;

    void setUp()
    {
        TS_ASSERT( mock = new MockForQueryLoader );
    }

    void tearDown()
    {
        TS_ASSERT_THROWS_NOTHING( delete mock );
    }

    void test_load_query()
    {
        // Set up simple query as a test
        InputQuery inputQuery;
        inputQuery.setNumberOfVariables( 6 );

        // Input layer with one variable
        inputQuery.markInputVariable( 0, 0 );
        inputQuery.setLowerBound( 0, 0.0 );
        inputQuery.setUpperBound( 0, 1.0 );

        // Hidden layer with two nodes
        inputQuery.setLowerBound( 3, 0.0 );
        inputQuery.setLowerBound( 4, 0.0 );

        // Output layer with one variable
        inputQuery.markOutputVariable( 5, 0 );
        inputQuery.setUpperBound( 5, 3.0 );

        // Equations
        // First equation, input to first ReLU
        Equation equation0;
        equation0.addAddend( -1.0, 1 ); // Equation output
        equation0.addAddend( 1.0, 0 );  // Weighted equation input
        equation0.setScalar( 0.5 );     // Equation bias
        inputQuery.addEquation( equation0 );

        // Second equation, input to second ReLU
        Equation equation1;
        equation1.addAddend( -1.0, 2 ); // Equation output
        equation1.addAddend( -1.0, 0 ); // Weighted equation input
        equation1.setScalar( -0.5 );    // Equation bias
        inputQuery.addEquation( equation1 );

        // ReLU Constraints
        inputQuery.addPiecewiseLinearConstraint( new ReluConstraint( 1, 3 ) );
        inputQuery.addPiecewiseLinearConstraint( new ReluConstraint( 2, 4 ) );

        // Third equation, hidden layer to output
        Equation equation2;
        equation2.addAddend( -1.0, 5 ); // Equation output
        equation2.addAddend( -1.0, 3 ); // Weighted equation input
        equation2.addAddend( 1.0, 4 );  // Weighted equation input
        equation2.setScalar( 0.5 );     // Equation bias
        inputQuery.addEquation( equation2 );

        // Save the query and then reload the query
        inputQuery.saveQuery( QUERY_TEST_FILE );

        mock->mockFile.wasCreated = false;
        mock->mockFile.wasDiscarded = false;

        InputQuery inputQuery2 = QueryLoader::loadQuery( QUERY_TEST_FILE );

        // Check that inputQuery is unchanged when saving and loading the query
        // Number of variables unchanged
        TS_ASSERT( inputQuery.getNumberOfVariables() == inputQuery2.getNumberOfVariables() );
        TS_ASSERT( inputQuery.getNumInputVariables() == inputQuery2.getNumInputVariables() );
        TS_ASSERT( inputQuery.getNumOutputVariables() == inputQuery2.getNumOutputVariables() );

        // Input and output variables unchanged
        TS_ASSERT( inputQuery.getInputVariables() == inputQuery2.getInputVariables() );
        TS_ASSERT( inputQuery.getOutputVariables() == inputQuery2.getOutputVariables() );

        // Bounds unchanged
        TS_ASSERT( inputQuery.getLowerBounds() == inputQuery2.getLowerBounds() );
        TS_ASSERT( inputQuery.getUpperBounds() == inputQuery2.getUpperBounds() );

        // Number of infinite bounds unchanged
        TS_ASSERT( inputQuery.countInfiniteBounds() == inputQuery2.countInfiniteBounds() );

        // Equations unchanged
        TS_ASSERT( inputQuery.getEquations() == inputQuery2.getEquations() );

        // Constraints unchanged
        TS_ASSERT( inputQuery.getPiecewiseLinearConstraints() == inputQuery.getPiecewiseLinearConstraints() );
    }
};

//
// Local Variables:
// compile-command: "make -C ../../.. "
// tags-file-name: "../../../TAGS"
// c-basic-offset: 4
// End:
//
