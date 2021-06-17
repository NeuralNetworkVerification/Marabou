/*********************                                                        */
/*! \file Test_InputQuery.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz, Christopher Lazarus, Shantanu Thakoor
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
#include "FloatUtils.h"
#include "InputQuery.h"
#include "MockErrno.h"
#include "MockFileFactory.h"
#include "LeakyReluConstraint.h"
#include "ReluConstraint.h"
#include "MarabouError.h"
#include "NetworkLevelReasoner.h"

#include <string.h>

class MockForInputQuery
    : public MockFileFactory
    , public MockErrno
{
public:
};

class InputQueryTestSuite : public CxxTest::TestSuite
{
public:
    MockForInputQuery *mock;
    MockFile *file;

    void setUp()
    {
        TS_ASSERT( mock = new MockForInputQuery );

        file = &( mock->mockFile );
    }

    void tearDown()
    {
        TS_ASSERT_THROWS_NOTHING( delete mock );
    }

    void test_lower_bounds()
    {
        InputQuery inputQuery;

        TS_ASSERT_THROWS_NOTHING( inputQuery.setNumberOfVariables( 5 ) );
        TS_ASSERT_EQUALS( inputQuery.getLowerBound( 3 ), FloatUtils::negativeInfinity() );
        TS_ASSERT_THROWS_NOTHING( inputQuery.setLowerBound( 3, -3 ) );
        TS_ASSERT_EQUALS( inputQuery.getLowerBound( 3 ), -3 );
        TS_ASSERT_THROWS_NOTHING( inputQuery.setLowerBound( 3, 5 ) );
        TS_ASSERT_EQUALS( inputQuery.getLowerBound( 3 ), 5 );

        TS_ASSERT_EQUALS( inputQuery.getLowerBound( 2 ), FloatUtils::negativeInfinity() );
        TS_ASSERT_THROWS_NOTHING( inputQuery.setLowerBound( 2, 4 ) );
        TS_ASSERT_EQUALS( inputQuery.getLowerBound( 2 ), 4 );

        TS_ASSERT_THROWS_EQUALS( inputQuery.getLowerBound( 5 ),
                                 const MarabouError &e,
                                 e.getCode(),
                                 MarabouError::VARIABLE_INDEX_OUT_OF_RANGE );

        TS_ASSERT_THROWS_EQUALS( inputQuery.setLowerBound( 6, 1 ),
                                 const MarabouError &e,
                                 e.getCode(),
                                 MarabouError::VARIABLE_INDEX_OUT_OF_RANGE );
    }

    void test_upper_bounds()
    {
        InputQuery inputQuery;

        TS_ASSERT_THROWS_NOTHING( inputQuery.setNumberOfVariables( 5 ) );
        TS_ASSERT_EQUALS( inputQuery.getUpperBound( 2 ), FloatUtils::infinity() );
        TS_ASSERT_THROWS_NOTHING( inputQuery.setUpperBound( 2, -4 ) );
        TS_ASSERT_EQUALS( inputQuery.getUpperBound( 2 ), -4 );
        TS_ASSERT_THROWS_NOTHING( inputQuery.setUpperBound( 2, 55 ) );
        TS_ASSERT_EQUALS( inputQuery.getUpperBound( 2 ), 55 );

        TS_ASSERT_EQUALS( inputQuery.getUpperBound( 0 ), FloatUtils::infinity() );
        TS_ASSERT_THROWS_NOTHING( inputQuery.setUpperBound( 0, 1 ) );
        TS_ASSERT_EQUALS( inputQuery.getUpperBound( 0 ), 1 );

        TS_ASSERT_THROWS_EQUALS( inputQuery.getUpperBound( 5 ),
                                 const MarabouError &e,
                                 e.getCode(),
                                 MarabouError::VARIABLE_INDEX_OUT_OF_RANGE );

        TS_ASSERT_THROWS_EQUALS( inputQuery.setUpperBound( 6, 1 ),
                                 const MarabouError &e,
                                 e.getCode(),
                                 MarabouError::VARIABLE_INDEX_OUT_OF_RANGE );
    }

    void test_equality_operator()
    {
        ReluConstraint *relu1 = new ReluConstraint( 3, 5 );

        InputQuery *inputQuery = new InputQuery;

        inputQuery->setNumberOfVariables( 5 );
        inputQuery->setLowerBound( 2, -4 );
        inputQuery->setUpperBound( 2, 55 );
        inputQuery->addPiecewiseLinearConstraint( relu1 );

        InputQuery inputQuery2 = *inputQuery;

        TS_ASSERT_EQUALS( inputQuery2.getNumberOfVariables(), 5U );
        TS_ASSERT_EQUALS( inputQuery2.getLowerBound( 2 ), -4 );
        TS_ASSERT_EQUALS( inputQuery2.getUpperBound( 2 ), 55 );

        auto constraints = inputQuery2.getPiecewiseLinearConstraints();

        TS_ASSERT_EQUALS( constraints.size(), 1U );
        ReluConstraint *constraint = (ReluConstraint *)*constraints.begin();

        TS_ASSERT_DIFFERS( constraint, relu1 ); // Different pointers

        TS_ASSERT( constraint->participatingVariable( 3 ) );
        TS_ASSERT( constraint->participatingVariable( 5 ) );

        inputQuery2 = *inputQuery; // Repeat the assignment

        delete inputQuery;

        TS_ASSERT_EQUALS( inputQuery2.getNumberOfVariables(), 5U );
        TS_ASSERT_EQUALS( inputQuery2.getLowerBound( 2 ), -4 );
        TS_ASSERT_EQUALS( inputQuery2.getUpperBound( 2 ), 55 );

        constraints = inputQuery2.getPiecewiseLinearConstraints();

        TS_ASSERT_EQUALS( constraints.size(), 1U );
        constraint = (ReluConstraint *)*constraints.begin();

        TS_ASSERT_DIFFERS( constraint, relu1 ); // Different pointers

        TS_ASSERT( constraint->participatingVariable( 3 ) );
        TS_ASSERT( constraint->participatingVariable( 5 ) );
    }

    void test_infinite_bounds()
    {
        InputQuery *inputQuery = new InputQuery;

        inputQuery->setNumberOfVariables( 5 );
        inputQuery->setLowerBound( 2, -4 );
        inputQuery->setUpperBound( 2, 55 );
        inputQuery->setUpperBound( 3, FloatUtils::infinity() );

        TS_ASSERT_EQUALS( inputQuery->countInfiniteBounds(), 8U );

        delete inputQuery;
    }

    void test_save_query()
    {
        TS_TRACE( "TODO" );

        InputQuery *inputQuery = new InputQuery;

        // Todo: Load some stuff into the input query

        TS_ASSERT_THROWS_NOTHING( inputQuery->saveQuery( "query.dump" ) );

        // Todo: after saveQuery(), all the relevant information
        // should have been written to the mockFile. Specifically, we should
        // have mockFile's write() store everythign that's been written, and then
        // check that it is as expected here.

        TS_ASSERT_EQUALS( file->lastPath, "query.dump" );

        delete inputQuery;
    }

    void test_construct_leaky_relu_nlr()
    {
        // x2 = x0 + x1
        // x3 = x0 - x1
        // x4 = lRelu(x2)
        // x5 = lRelu(x3)
        // x6 = x2 + x3 + x4
        InputQuery *inputQuery = new InputQuery;
        inputQuery->setNumberOfVariables( 7 );
        Equation eq1;
        eq1.addAddend(1, 0);
        eq1.addAddend(1, 1);
        eq1.addAddend(-1, 2);
        inputQuery->addEquation(eq1);
        Equation eq2;
        eq2.addAddend(1, 0);
        eq2.addAddend(-1, 1);
        eq2.addAddend(-1, 3);
        inputQuery->addEquation(eq2);
        LeakyReluConstraint *r1 = new LeakyReluConstraint(2,4, 0.1);
        LeakyReluConstraint *r2 = new LeakyReluConstraint(3,5, 0.1);
        inputQuery->addPiecewiseLinearConstraint(r1);
        inputQuery->addPiecewiseLinearConstraint(r2);
        Equation eq3;
        eq3.addAddend(1, 2);
        eq3.addAddend(1, 3);
        eq3.addAddend(1, 4);
        eq3.addAddend(-1, 6);
        inputQuery->addEquation(eq3);
        inputQuery->markInputVariable(0, 0);
        inputQuery->markInputVariable(1, 1);
        TS_ASSERT( inputQuery->constructNetworkLevelReasoner() );
        NLR::NetworkLevelReasoner *nlr = inputQuery->getNetworkLevelReasoner();
        TS_ASSERT( nlr->getNumberOfLayers() == 4 );
        NLR::Layer *layer = nlr->getLayer( 2 );
        TS_ASSERT( layer->getLayerType() == NLR::Layer::LEAKY_RELU );
        layer = nlr->getLayer( 3 );
        TS_ASSERT( layer->getLayerType() == NLR::Layer::WEIGHTED_SUM );
        TS_ASSERT( layer->getSourceLayers().size() == 2 );

        double input[2];
        double output[1];

        input[0] = 2;
        input[1] = -3;
        double result = 2 - 3 + 2 + 3 - 0.1;
        nlr->evaluate(input, output);
        TS_ASSERT_EQUALS( output[0], result );
        delete inputQuery;
    }
};

//
// Local Variables:
// compile-command: "make -C ../../.. "
// tags-file-name: "../../../TAGS"
// c-basic-offset: 4
// End:
//
