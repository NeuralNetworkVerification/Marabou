 /*********************                                                        */
/*! \file Test_Preprocessor.h
** \verbatim
** Top contributors (to current version):
**   Derek Huang
** This file is part of the Marabou project.
** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
** in the top-level source directory) and their institutional affiliations.
** All rights reserved. See the file COPYING in the top-level source
** directory for licensing information.\endverbatim **/

#include <cxxtest/TestSuite.h>

#include "Preprocessor.h"
#include "InputQuery.h"
#include "MockErrno.h"
#include "ReluplexError.h"
#include "Engine.h"

#include <string.h>

class MockForPreprocessor
{
public:
};

class PreprocessorTestSuite : public CxxTest::TestSuite
{
public:
	MockForPreprocessor *mock;

	void setUp()
	{
		TS_ASSERT( mock = new MockForPreprocessor );
	}

	void tearDown()
	{
		TS_ASSERT_THROWS_NOTHING( delete mock );
	}

	void test_tighten_bounds()
	{
		InputQuery inputQuery;

        inputQuery.setNumberOfVariables( 4 );
        inputQuery.setLowerBound( 1, 0 );
        inputQuery.setUpperBound( 1, 1 );
        inputQuery.setLowerBound( 2, 2 );
        inputQuery.setUpperBound( 2, 3 );
        inputQuery.setLowerBound( 3, 0 );
        inputQuery.setUpperBound( 3, 0 );

        // x0 + x1 - x2 = 10
        Equation equation1;
        equation1.addAddend( 1, 0 );
        equation1.addAddend( 1, 1 );
        equation1.addAddend( -1, 2 );
        equation1.setScalar( 10 );
        inputQuery.addEquation( equation1 );

		Preprocessor preprocess( inputQuery );
        preprocess.tightenBounds();
		InputQuery processed = preprocess.getInputQuery();

        // x0 = 10 - x1 + x2
        //
        // x0.lb = 10 - x1.ub + x2.lb = 10 - 1 + 2 = 11
        // x0.ub = 10 - x1.lb + x2.ub = 10 - 0 + 3 = 13
        //
        // x2 = -10 + x0 + x1
        //
        // x2.lb = -10 + x0.lb + x1.lb
        // x2.ub = -10 + x0.ub + x1.ub

        TS_ASSERT_EQUALS( processed.getLowerBound( 0 ), 11 );
        TS_ASSERT_EQUALS( processed.getUpperBound( 0 ), 13 );

        preprocess._input.setLowerBound( 0, 0 );
        preprocess._input.setUpperBound( 0, FloatUtils::infinity() );
        preprocess._input.setLowerBound( 1, 0 );
        preprocess._input.setUpperBound( 1, 1 );
        preprocess._input.setLowerBound( 2, FloatUtils::negativeInfinity() );
        preprocess._input.setUpperBound( 2, 3 );
        preprocess._input.setLowerBound( 3, 0 );
        preprocess._input.setUpperBound( 3, 0 );

        // x0.lb = 10 - x1.ub + x2.lb --> Unbounded
        // x0.ub = 10 - x1.lb + x2.ub = 10 - 0 + 3 = 13

        // x2.lb = -10 + x0.lb + x1.lb = -10 + 0 + 0 = -10
        // x2.ub = -10 + x0.ub + x1.ub = -10 + 13 + 1 = 4
        // 3 is a tighter bound than 4, keep 3 as the upper bound

        preprocess.tightenBounds();
		processed = preprocess.getInputQuery();

        TS_ASSERT_EQUALS( processed.getLowerBound( 0 ), 0 );
        TS_ASSERT_EQUALS( processed.getUpperBound( 0 ), 13 );
        TS_ASSERT_EQUALS( processed.getLowerBound( 2 ), -10 );
		TS_ASSERT_EQUALS( processed.getUpperBound( 2 ), 3 );

        preprocess._input.setLowerBound( 0, FloatUtils::negativeInfinity() );
        preprocess._input.setUpperBound( 0, 15 );
        preprocess._input.setLowerBound( 1, 3 );
        preprocess._input.setUpperBound( 1, 3 );
        preprocess._input.setLowerBound( 2, 2 );
        preprocess._input.setUpperBound( 2, FloatUtils::infinity() );
        preprocess._input.setLowerBound( 3, 0 );
        preprocess._input.setUpperBound( 3, 0 );

        // x0.lb = 10 - x1.ub + x2.lb = 10 - 3 + 2 = 9
        // x0.ub = 10 - x1.lb + x2.ub --> Unbounded

        // x2.lb = -10 + x0.lb + x1.lb = -10 + 9 + 3 = 2
        // x2.ub = -10 + x0.ub + x1.ub = -10 + 15 + 3 = 8

        preprocess.tightenBounds();
		processed = preprocess.getInputQuery();

        TS_ASSERT_EQUALS( processed.getLowerBound( 0 ), 9 );
        TS_ASSERT_EQUALS( processed.getUpperBound( 0 ), 15 );
        TS_ASSERT_EQUALS( processed.getLowerBound( 2 ), 2 );
        TS_ASSERT_EQUALS( processed.getUpperBound( 2 ), 8 );

        preprocess._input.setLowerBound( 0, FloatUtils::negativeInfinity() );
        preprocess._input.setUpperBound( 0, 15 );
        preprocess._input.setLowerBound( 1, -3 );
        preprocess._input.setUpperBound( 1, -2 );
        preprocess._input.setLowerBound( 2, FloatUtils::negativeInfinity() );
        preprocess._input.setUpperBound( 2, 6 );
        preprocess._input.setLowerBound( 3, 0 );
        preprocess._input.setUpperBound( 3, 0 );

        // x0.lb = 10 - x1.ub + x2.lb --> Unbounded
        // x0.ub = 10 - x1.lb + x2.ub = 10 - -3 + 6 = 19
        // 15 is a tighter bound, keep 15 as the upper bound

        // x2.lb = -10 + x0.lb + x1.lb = Unbounded
        // x2.ub = -10 + x0.ub + x1.ub = -10 + 15 -2 = 3

        preprocess.tightenBounds();
		processed = preprocess.getInputQuery();

        TS_ASSERT_EQUALS( processed.getLowerBound( 0 ), FloatUtils::negativeInfinity() );
        TS_ASSERT_EQUALS( processed.getUpperBound( 0 ), 15 );
        TS_ASSERT_EQUALS( processed.getLowerBound( 2 ), FloatUtils::negativeInfinity() );
        TS_ASSERT_EQUALS( processed.getUpperBound( 2 ), 3 );

        preprocess._input.setLowerBound( 0, FloatUtils::negativeInfinity() );
        preprocess._input.setUpperBound( 0, 5 );
        preprocess._input.setLowerBound( 1, -3 );
        preprocess._input.setUpperBound( 1, -2 );
        preprocess._input.setLowerBound( 2, 0 );
        preprocess._input.setUpperBound( 2, 6 );
        preprocess._input.setLowerBound( 3, 0 );
        preprocess._input.setUpperBound( 3, 0 );

        // x0.lb = 10 - x1.ub + x2.lb --> Unbounded
        // x0.ub = 10 - x1.lb + x2.ub = 10 - -3 + 6 = 19
        // 5 is a tighter bound, keep 5 as the upper bound

        // x2.lb = -10 + x0.lb + x1.lb = Unbounded
        // 0 is a tighter bound, keep 0 as the lower bound
        // x2.ub = -10 + x0.ub + x1.ub = -10 + 5 -2 = -7
        // x2 = [0, -7] -> throw error

        TS_ASSERT_THROWS_EQUALS( preprocess.tightenBounds(),
                                 const ReluplexError &e,
                                 e.getCode(),
                                 ReluplexError::INVALID_BOUND_TIGHTENING );

        InputQuery inputQuery2;
		inputQuery2.setNumberOfVariables( 4 );
        inputQuery2.setLowerBound( 1, 0 );
        inputQuery2.setUpperBound( 1, 1 );
        inputQuery2.setLowerBound( 2, 2 );
        inputQuery2.setUpperBound( 2, 3 );
        inputQuery2.setLowerBound( 3, 0 );
        inputQuery2.setUpperBound( 3, 0 );

        // 2 * x0 + x1 - x2 = 10
        //
        // x0 = 5 - 1/2 x1 + 1/2 x2
        //
        // x0.lb = 5 - 1/2 x1.ub + 1/2 x2.lb = 5 - 1/2 + 1 = 5.5
        // x0.ub = 5 - 1/2 x1.lb + 1/2 x2.ub = 5 - 0 + 1.5 = 6.5

        Equation equation2;
        equation2.addAddend( 2, 0 );
        equation2.addAddend( 1, 1 );
        equation2.addAddend( -1, 2 );
        equation2.setScalar( 10 );
        inputQuery2.addEquation( equation2 );
		
		Preprocessor preprocess2( inputQuery2 );
		preprocess2.tightenBounds();
		InputQuery processed2 = preprocess2.getInputQuery();

        TS_ASSERT_EQUALS( processed2.getLowerBound( 0 ), 5.5 );
        TS_ASSERT_EQUALS( processed2.getUpperBound( 0 ), 6.5 );
	}

	void test_eliminate_variables()
	{
		InputQuery inputQuery;

        inputQuery.setNumberOfVariables( 4 );
		inputQuery.setLowerBound( 0, 1 );
		inputQuery.setUpperBound( 0, 1 );
        inputQuery.setLowerBound( 1, 0 );
        inputQuery.setUpperBound( 1, 1 );
        inputQuery.setLowerBound( 2, 2 );
        inputQuery.setUpperBound( 2, 3 );
		inputQuery.setLowerBound( 3, 5 );
		inputQuery.setUpperBound( 3, 5 );

        // x0 + x1 + x3 = 10
        Equation equation1;
        equation1.addAddend( 1, 0 );
        equation1.addAddend( 1, 1 );
        equation1.addAddend( 1, 3 );
        equation1.setScalar( 10 );
        inputQuery.addEquation( equation1 );

		// x2 + x3 = 6
		Equation equation2;
		equation2.addAddend( 1, 2 );
		equation2.addAddend( 1, 3 );
		equation2.setScalar( 6 );
		inputQuery.addEquation( equation2 );

		Preprocessor preprocess( inputQuery );

        preprocess.eliminateVariables();

		InputQuery processed = preprocess.getInputQuery();

		//eliminate variables x0, x3 because UB = LB
		TS_ASSERT_EQUALS( processed.getNumberOfVariables(), 2U );

		//	x0 + x1 + x3 = 10
		//	x1 + x3 = 10 - x0 = 9
		//	x0 + x3 = 9
		//	x0 = 9 - x3 = 4
		//	1 addend: x0
		//	scalar = 4
		TS_ASSERT( FloatUtils::areEqual( processed.getEquations().front()._scalar, 4 ) );
		TS_ASSERT( FloatUtils::areEqual( processed.getEquations().front()._addends.size(), 1 ) );
		TS_ASSERT( FloatUtils::areEqual( processed.getEquations().front()._addends.front()._variable, 0 ) );

		//	x2 + x3 = 6
		//	x1 + x3 = 6
		//	x1 = 6 - x3 = 1
		//	1 addend: x1
		//	scalar = 1
		TS_ASSERT( FloatUtils::areEqual( processed.getEquations().back()._scalar, 1 ) );
		TS_ASSERT( FloatUtils::areEqual( processed.getEquations().back()._addends.size(), 1 ) );
		TS_ASSERT( FloatUtils::areEqual( processed.getEquations().back()._addends.front()._variable, 1 ) );
	}

};

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
