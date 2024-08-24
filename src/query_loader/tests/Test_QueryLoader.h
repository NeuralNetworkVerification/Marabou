/*********************                                                        */
/*! \file Test_QueryLoader.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Kyle Julian, Teruhiro Tagomori
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2024 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

**/

#include "AutoFile.h"
#include "Equation.h"
#include "MockFileFactory.h"
#include "Query.h"
#include "QueryLoader.h"
#include "ReluConstraint.h"
#include "T/unistd.h"

#include <cxxtest/TestSuite.h>

const String QUERY_TEST_FILE( "QueryTest.txt" );

class MockForQueryLoader
    : public MockFileFactory
    , public T::Base_stat
{
public:
    int stat( const char * /* path */, StructStat * /* buf */ )
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
        Query inputQuery;
        inputQuery.setNumberOfVariables( 10 );

        // Input layer with one variable
        inputQuery.markInputVariable( 0, 0 );
        inputQuery.setLowerBound( 0, 0.0 );
        inputQuery.setUpperBound( 0, 1.0 );

        // First hidden layer with two nodes
        inputQuery.setLowerBound( 3, 0.0 );
        inputQuery.setLowerBound( 4, 0.0 );

        // Second hidden layer with two nodes
        inputQuery.setLowerBound( 7, 0.0 );
        inputQuery.setLowerBound( 8, 0.0 );

        // Output layer with one variable
        inputQuery.markOutputVariable( 9, 0 );
        inputQuery.setUpperBound( 9, 3.0 );

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

        // Third equation, first hidden layer to first node of second hidden layer
        Equation equation2;
        equation2.addAddend( -1.0, 5 ); // Equation output
        equation2.addAddend( -1.0, 3 ); // Weighted equation input
        equation2.addAddend( 1.0, 4 );  // Weighted equation input
        equation2.setScalar( 0.5 );     // Equation bias
        inputQuery.addEquation( equation2 );

        // Forth equation, first hidden layer to second node of second hidden layer
        Equation equation3;
        equation3.addAddend( -1.0, 6 ); // Equation output
        equation3.addAddend( -1.0, 3 ); // Weighted equation input
        equation3.addAddend( 1.0, 4 );  // Weighted equation input
        equation3.setScalar( 0.5 );     // Equation bias
        inputQuery.addEquation( equation3 );

        // Sigmoid Constraints
        inputQuery.addNonlinearConstraint( new SigmoidConstraint( 5, 7 ) );
        inputQuery.addNonlinearConstraint( new SigmoidConstraint( 6, 8 ) );

        // Fifth equation, second hidden layer to output
        Equation equation4;
        equation4.addAddend( -1.0, 9 ); // Equation output
        equation4.addAddend( -1.0, 7 ); // Weighted equation input
        equation4.addAddend( 1.0, 8 );  // Weighted equation input
        equation4.setScalar( 0.5 );     // Equation bias
        inputQuery.addEquation( equation4 );

        // Save the query and then reload the query
        inputQuery.saveQuery( QUERY_TEST_FILE );

        mock->mockFile.wasCreated = false;
        mock->mockFile.wasDiscarded = false;

        Query inputQuery2;
        QueryLoader::loadQuery( QUERY_TEST_FILE, inputQuery2 );

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

        // Piecewise Constraints unchanged
        TS_ASSERT( inputQuery.getPiecewiseLinearConstraints().size() == 2U );
        TS_ASSERT( inputQuery2.getPiecewiseLinearConstraints().size() == 2U );
        auto it = inputQuery.getPiecewiseLinearConstraints().begin();
        ReluConstraint *constraint = (ReluConstraint *)*it;
        auto it2 = inputQuery2.getPiecewiseLinearConstraints().begin();
        ReluConstraint *constraint2 = (ReluConstraint *)*it2;
        TS_ASSERT( constraint->serializeToString() == constraint2->serializeToString() );
        ++it;
        ++it2;
        constraint = (ReluConstraint *)*it;
        constraint2 = (ReluConstraint *)*it2;
        TS_ASSERT( constraint->serializeToString() == constraint2->serializeToString() );

        // Nonlinear Constraints unchanged
        TS_ASSERT( inputQuery.getNonlinearConstraints().size() == 2U );
        TS_ASSERT( inputQuery2.getNonlinearConstraints().size() == 2U );
        auto tsIt = inputQuery.getNonlinearConstraints().begin();
        SigmoidConstraint *nlConstraint = (SigmoidConstraint *)*tsIt;
        auto tsIt2 = inputQuery2.getNonlinearConstraints().begin();
        SigmoidConstraint *nlConstraint2 = (SigmoidConstraint *)*tsIt2;
        TS_ASSERT( nlConstraint->serializeToString() == nlConstraint2->serializeToString() );
        ++tsIt;
        ++tsIt2;
        nlConstraint = (SigmoidConstraint *)*tsIt;
        nlConstraint2 = (SigmoidConstraint *)*tsIt2;
        TS_ASSERT( nlConstraint->serializeToString() == nlConstraint2->serializeToString() );
    }
};

//
// Local Variables:
// compile-command: "make -C ../../.. "
// tags-file-name: "../../../TAGS"
// c-basic-offset: 4
// End:
//
