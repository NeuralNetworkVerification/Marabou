/*********************                                                        */
/*! \file Test_InputQuery.h
** \verbatim
** Top contributors (to current version):
**   Guy Katz
** This file is part of the Marabou project.
** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
** in the top-level source directory) and their institutional affiliations.
** All rights reserved. See the file COPYING in the top-level source
** directory for licensing information.\endverbatim
**/

#include <cxxtest/TestSuite.h>

#include "Engine.h"
#include "FloatUtils.h"
#include "InputQuery.h"
#include "MockErrno.h"
#include "ReluConstraint.h"
#include "ReluplexError.h"

#include <string.h>

class MockForInputQuery
{
public:
};

class InputQueryTestSuite : public CxxTest::TestSuite
{
public:
    MockForInputQuery *mock;

    void setUp()
    {
        TS_ASSERT( mock = new MockForInputQuery );
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
                                 const ReluplexError &e,
                                 e.getCode(),
                                 ReluplexError::VARIABLE_INDEX_OUT_OF_RANGE );

        TS_ASSERT_THROWS_EQUALS( inputQuery.setLowerBound( 6, 1 ),
                                 const ReluplexError &e,
                                 e.getCode(),
                                 ReluplexError::VARIABLE_INDEX_OUT_OF_RANGE );
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
                                 const ReluplexError &e,
                                 e.getCode(),
                                 ReluplexError::VARIABLE_INDEX_OUT_OF_RANGE );

        TS_ASSERT_THROWS_EQUALS( inputQuery.setUpperBound( 6, 1 ),
                                 const ReluplexError &e,
                                 e.getCode(),
                                 ReluplexError::VARIABLE_INDEX_OUT_OF_RANGE );
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
    }
};

//
// Local Variables:
// compile-command: "make -C ../../.. "
// tags-file-name: "../../../TAGS"
// c-basic-offset: 4
// End:
//
