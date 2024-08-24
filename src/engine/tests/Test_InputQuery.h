/*********************                                                        */
/*! \file Test_InputQuery.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Andrew Wu
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2024 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

**/

#include "Engine.h"
#include "InputQuery.h"
#include "MarabouError.h"
#include "MockErrno.h"
#include "Query.h"
#include "ReluConstraint.h"
#include "SigmoidConstraint.h"

#include <cxxtest/TestSuite.h>
#include <string.h>

class MockForInputQuery : public MockErrno
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

    void test_push_and_pop()
    {
        InputQuery inputQuery;
        TS_ASSERT_EQUALS( inputQuery.getLevel(), 0u );
        TS_ASSERT_THROWS_NOTHING( inputQuery.push() );
        TS_ASSERT_EQUALS( inputQuery.getLevel(), 1u );
        TS_ASSERT_THROWS_NOTHING( inputQuery.push() );
        TS_ASSERT_EQUALS( inputQuery.getLevel(), 2u );
        TS_ASSERT_THROWS_NOTHING( inputQuery.pop() );
        TS_ASSERT_EQUALS( inputQuery.getLevel(), 1u );
        TS_ASSERT_THROWS_NOTHING( inputQuery.push() );
        TS_ASSERT_EQUALS( inputQuery.getLevel(), 2u );
        TS_ASSERT_THROWS_NOTHING( inputQuery.popTo( 0 ) );
        TS_ASSERT_EQUALS( inputQuery.getLevel(), 0u );
    }

    void test_set_and_get_new_variable()
    {
        InputQuery inputQuery;
        TS_ASSERT_THROWS_NOTHING( inputQuery.setNumberOfVariables( 1 ) );
        TS_ASSERT_EQUALS( inputQuery.getNumberOfVariables(), 1u );
        TS_ASSERT_EQUALS( inputQuery.getNewVariable(), 1u );
        TS_ASSERT_EQUALS( inputQuery.getNumberOfVariables(), 2u );
        TS_ASSERT_EQUALS( inputQuery.getNewVariable(), 2u );
        TS_ASSERT_EQUALS( inputQuery.getNumberOfVariables(), 3u );

        inputQuery.push();
        TS_ASSERT_EQUALS( inputQuery.getNumberOfVariables(), 3u );
        TS_ASSERT_THROWS_NOTHING( inputQuery.setNumberOfVariables( 5u ) );
        TS_ASSERT_EQUALS( inputQuery.getNumberOfVariables(), 5u );
        inputQuery.pop();
        TS_ASSERT_EQUALS( inputQuery.getNumberOfVariables(), 3u );
    }

    void test_set_and_tighten_bounds()
    {
        InputQuery inputQuery;
        TS_ASSERT_THROWS_NOTHING( inputQuery.setNumberOfVariables( 3 ) );
        inputQuery.setLowerBound( 0, 0.5 );
        inputQuery.setUpperBound( 0, 2.5 );
        TS_ASSERT( inputQuery.tightenLowerBound( 1, 0.5 ) );
        TS_ASSERT( inputQuery.tightenUpperBound( 1, 2.5 ) );
        TS_ASSERT_EQUALS( inputQuery.getLowerBound( 0 ), 0.5 );
        TS_ASSERT_EQUALS( inputQuery.getUpperBound( 0 ), 2.5 );
        TS_ASSERT_EQUALS( inputQuery.getLowerBound( 1 ), 0.5 );
        TS_ASSERT_EQUALS( inputQuery.getUpperBound( 1 ), 2.5 );
        TS_ASSERT_EQUALS( inputQuery.getLowerBound( 2 ), FloatUtils::negativeInfinity() );
        TS_ASSERT_EQUALS( inputQuery.getUpperBound( 2 ), FloatUtils::infinity() );

        inputQuery.setLowerBound( 0, -1.5 );
        TS_ASSERT_EQUALS( inputQuery.getLowerBound( 0 ), -1.5 );

        TS_ASSERT( !inputQuery.tightenLowerBound( 1, -1.5 ) );
        TS_ASSERT_EQUALS( inputQuery.getLowerBound( 1 ), 0.5 );

        inputQuery.setUpperBound( 0, 3 );
        TS_ASSERT_EQUALS( inputQuery.getUpperBound( 0 ), 3 );

        TS_ASSERT( !inputQuery.tightenUpperBound( 1, 3 ) );
        TS_ASSERT_EQUALS( inputQuery.getUpperBound( 1 ), 2.5 );

        inputQuery.push();
        TS_ASSERT_EQUALS( inputQuery.getLowerBound( 0 ), -1.5 );
        TS_ASSERT_EQUALS( inputQuery.getUpperBound( 0 ), 3 );
        TS_ASSERT_EQUALS( inputQuery.getLowerBound( 1 ), 0.5 );
        TS_ASSERT_EQUALS( inputQuery.getUpperBound( 1 ), 2.5 );
        TS_ASSERT_EQUALS( inputQuery.getLowerBound( 2 ), FloatUtils::negativeInfinity() );
        TS_ASSERT_EQUALS( inputQuery.getUpperBound( 2 ), FloatUtils::infinity() );

        // Cannot set non-infinite variable bound when context level is non-zero
        TS_ASSERT_THROWS_EQUALS( inputQuery.setLowerBound( 0, 1 ),
                                 const MarabouError &e,
                                 e.getCode(),
                                 MarabouError::INPUT_QUERY_VARIABLE_BOUND_ALREADY_SET );
        TS_ASSERT_THROWS_EQUALS( inputQuery.setUpperBound( 0, 1 ),
                                 const MarabouError &e,
                                 e.getCode(),
                                 MarabouError::INPUT_QUERY_VARIABLE_BOUND_ALREADY_SET );
        TS_ASSERT_THROWS_NOTHING( inputQuery.setLowerBound( 2, 0.5 ) );
        TS_ASSERT_THROWS_NOTHING( inputQuery.setUpperBound( 2, 2.5 ) );

        TS_ASSERT_EQUALS( inputQuery.getLowerBound( 2 ), 0.5 );
        TS_ASSERT_EQUALS( inputQuery.getUpperBound( 2 ), 2.5 );

        inputQuery.push();
        TS_ASSERT_EQUALS( inputQuery.getLowerBound( 0 ), -1.5 );
        TS_ASSERT_EQUALS( inputQuery.getUpperBound( 0 ), 3 );
        TS_ASSERT_EQUALS( inputQuery.getLowerBound( 1 ), 0.5 );
        TS_ASSERT_EQUALS( inputQuery.getUpperBound( 1 ), 2.5 );
        TS_ASSERT_EQUALS( inputQuery.getLowerBound( 2 ), 0.5 );
        TS_ASSERT_EQUALS( inputQuery.getUpperBound( 2 ), 2.5 );

        TS_ASSERT( inputQuery.tightenLowerBound( 0, 0.5 ) );
        TS_ASSERT( inputQuery.tightenUpperBound( 0, 2.5 ) );

        TS_ASSERT_EQUALS( inputQuery.getLowerBound( 0 ), 0.5 );
        TS_ASSERT_EQUALS( inputQuery.getUpperBound( 0 ), 2.5 );

        // Pop to context level 0
        TS_ASSERT_THROWS_NOTHING( inputQuery.popTo( 0 ) );
        TS_ASSERT_EQUALS( inputQuery.getLowerBound( 0 ), -1.5 );
        TS_ASSERT_EQUALS( inputQuery.getUpperBound( 0 ), 3 );
        TS_ASSERT_EQUALS( inputQuery.getLowerBound( 1 ), 0.5 );
        TS_ASSERT_EQUALS( inputQuery.getUpperBound( 1 ), 2.5 );
        TS_ASSERT_EQUALS( inputQuery.getLowerBound( 2 ), FloatUtils::negativeInfinity() );
        TS_ASSERT_EQUALS( inputQuery.getUpperBound( 2 ), FloatUtils::infinity() );
    }

    void encodeQuery( IQuery &query, bool pushContext = false )
    {
        query.markInputVariable( 2, 0 );
        query.setNumberOfVariables( 5 );
        query.setLowerBound( 1, 0.5 );
        query.setUpperBound( 1, 1.5 );
        // x2 - x1 = -1
        Equation e;
        e.addAddend( 1, 2 );
        e.addAddend( -1, 1 );
        e.setScalar( -1 );
        query.addEquation( e );

        if ( pushContext )
            ( (InputQuery &)query ).push();

        // x1 = Clip(x0, 1, 2);
        query.addClipConstraint( 0, 1, 1, 2 );

        // x3 = ReLU(x1)
        query.addPiecewiseLinearConstraint( new ReluConstraint( 1, 3 ) );

        // x4 = Sigmoid(x3)
        query.addNonlinearConstraint( new SigmoidConstraint( 3, 4 ) );

        if ( pushContext )
        {
            InputQuery &inputQuery = (InputQuery &)query;
            unsigned numEquations = inputQuery.getNumberOfEquations();
            inputQuery.push();
            inputQuery.markOutputVariable( 2, 0 );
            unsigned newVar = inputQuery.getNewVariable();
            unsigned newVar2 = inputQuery.getNewVariable();
            // newVar + 0.5 * x4 <= 1
            // newVar = ReLU(x4)
            // newVar2 = Sigmoid(x4)
            Equation e1( Equation::LE );
            e1.addAddend( 1, newVar );
            e1.addAddend( 0.5, 4 );
            e1.setScalar( 1 );
            inputQuery.addEquation( e1 );
            inputQuery.addPiecewiseLinearConstraint( new ReluConstraint( 4, newVar ) );
            inputQuery.addNonlinearConstraint( new SigmoidConstraint( 4, newVar2 ) );
            TS_ASSERT_EQUALS( inputQuery.getNumberOfEquations(), numEquations + 1 );
            inputQuery.pop();
        }

        query.markOutputVariable( 4, 0 );
    }

    void test_generate_query()
    {
        InputQuery inputQuery;
        Query query;

        TS_ASSERT_THROWS_NOTHING( encodeQuery( inputQuery, true ) );
        TS_ASSERT_THROWS_NOTHING( encodeQuery( query ) );

        Query *queryGeneratedFromInputQuery = inputQuery.generateQuery();

        compare_query( query, *queryGeneratedFromInputQuery );
        delete queryGeneratedFromInputQuery;
    }

    void compare_query( const Query &query1, const Query &query2 )
    {
        TS_ASSERT_EQUALS( query1.getNumberOfVariables(), query2.getNumberOfVariables() );
        TS_ASSERT_EQUALS( query1.getEquations(), query2.getEquations() );
        TS_ASSERT_EQUALS( query1.getLowerBounds(), query2.getLowerBounds() );
        TS_ASSERT_EQUALS( query1.getUpperBounds(), query2.getUpperBounds() );
        TS_ASSERT_EQUALS( query1._variableToInputIndex, query2._variableToInputIndex );
        TS_ASSERT_EQUALS( query1._inputIndexToVariable, query2._inputIndexToVariable );
        TS_ASSERT_EQUALS( query1._variableToOutputIndex, query2._variableToOutputIndex );
        TS_ASSERT_EQUALS( query1._inputIndexToVariable, query2._inputIndexToVariable );
        TS_ASSERT_EQUALS( query1.getInputVariables(), query2.getInputVariables() );

        TS_ASSERT_EQUALS( query1.getPiecewiseLinearConstraints().size(),
                          query2.getPiecewiseLinearConstraints().size() );
        {
            auto c1 = query1.getPiecewiseLinearConstraints().begin();
            auto c2 = query2.getPiecewiseLinearConstraints().begin();
            while ( c1 != query1.getPiecewiseLinearConstraints().end() )
            {
                String s1, s2;
                ( *c1 )->dump( s1 );
                ( *c2 )->dump( s2 );
                TS_ASSERT_EQUALS( s1, s2 );
                ++c1;
                ++c2;
            }
        }
        {
            TS_ASSERT_EQUALS( query1.getNonlinearConstraints().size(),
                              query2.getNonlinearConstraints().size() );
            auto c1 = query1.getNonlinearConstraints().begin();
            auto c2 = query2.getNonlinearConstraints().begin();
            while ( c1 != query1.getNonlinearConstraints().end() )
            {
                String s1, s2;
                ( *c1 )->dump( s1 );
                ( *c2 )->dump( s2 );
                TS_ASSERT_EQUALS( s1, s2 );
                ++c1;
                ++c2;
            }
        }
    }
};
