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
#include "ReluConstraint.h"
#include "MaxConstraint.h"

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
		inputQuery2.setNumberOfVariables( 5 );
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
	void test_tighten_pl()
	{
        InputQuery inputQuery;
		inputQuery.setNumberOfVariables( 7 );
        inputQuery.setLowerBound( 1, 0 );
        inputQuery.setUpperBound( 1, 1 );
        inputQuery.setLowerBound( 2, 2 );
        inputQuery.setUpperBound( 2, 3 );
        inputQuery.setLowerBound( 3, 0 );
        inputQuery.setUpperBound( 3, 0 );
		inputQuery.setLowerBound( 4, 5 );
		inputQuery.setUpperBound( 4, 7 );

		ReluConstraint *relu = new ReluConstraint( 5, 0 );
		MaxConstraint *max = new MaxConstraint( 6, Set<unsigned>( { 4, 0 } ) );
		inputQuery.addPiecewiseLinearConstraint( relu );
		inputQuery.addPiecewiseLinearConstraint( max );


        // 2 * x0 + x1 - x2 = 10
        //
        // x0 = 5 - 1/2 x1 + 1/2 x2
        //
        // x0.lb = 5 - 1/2 x1.ub + 1/2 x2.lb = 5 - 1/2 + 1 = 5.5
        // x0.ub = 5 - 1/2 x1.lb + 1/2 x2.ub = 5 - 0 + 1.5 = 6.5
		// 
		// x5 = Relu( 0, x0 )
		// x5.lb = 5.5
		// x5.ub = 6.5
		//
		// x6 = Max( x0, x4 )
		// x6.lb = 5.5
		// x6.ub = 7

        Equation equation;
        equation.addAddend( 2, 0 );
        equation.addAddend( 1, 1 );
        equation.addAddend( -1, 2 );
        equation.setScalar( 10 );
        inputQuery.addEquation( equation );

		Preprocessor preprocess( inputQuery );
		preprocess.tightenBounds();
		preprocess.tightenPL();

		InputQuery processed = preprocess.getInputQuery();
        TS_ASSERT( FloatUtils::areEqual( processed.getLowerBound( 0 ), 5.5 ) );
        TS_ASSERT( FloatUtils::areEqual( processed.getUpperBound( 0 ), 6.5 ) );
		TS_ASSERT( FloatUtils::areEqual( processed.getLowerBound( 5 ), 5.5 ) );
		TS_ASSERT( FloatUtils::areEqual( processed.getUpperBound( 5 ), 6.5 ) );
		TS_ASSERT( FloatUtils::areEqual( processed.getLowerBound( 6 ), 5.5 ) );
		TS_ASSERT( FloatUtils::areEqual( processed.getUpperBound( 6 ), 7 ) );

		InputQuery inputQuery2;
		inputQuery2.setNumberOfVariables( 17 );
		inputQuery2.setLowerBound( 0, 0 );
		inputQuery2.setUpperBound( 0, 10 );
        inputQuery2.setLowerBound( 2, 5 );
        inputQuery2.setUpperBound( 2, 13 );
        inputQuery2.setUpperBound( 3, 10 );
   		inputQuery2.setLowerBound( 4, 4 );
		inputQuery2.setLowerBound( 5, 0 );
		inputQuery2.setUpperBound( 5, 1 );
		inputQuery2.setLowerBound( 6, 0 );
		inputQuery2.setUpperBound( 6, 2 );
		inputQuery2.setLowerBound( 7, -1 );
		inputQuery2.setUpperBound( 7, 0 );
		inputQuery2.setLowerBound( 8, 0 );
		inputQuery2.setUpperBound( 8, 2 );
		inputQuery2.setLowerBound( 9, 0 );
		inputQuery2.setUpperBound( 9, 1 );
		inputQuery2.setLowerBound( 10, -1 );
		inputQuery2.setUpperBound( 10, 0 );

		ReluConstraint *relu2 = new ReluConstraint( 0, 1 );
		MaxConstraint *max2 = new MaxConstraint( 2, Set<unsigned>( { 4, 3 } ) );
		MaxConstraint *max3 = new MaxConstraint( 5, Set<unsigned>( { 6, 7 } ) );
		MaxConstraint *max4 = new MaxConstraint( 8, Set<unsigned>( { 9, 10} ) );
		inputQuery2.addPiecewiseLinearConstraint( relu2 );
		inputQuery2.addPiecewiseLinearConstraint( max2 );
		inputQuery2.addPiecewiseLinearConstraint( max3 );
		inputQuery2.addPiecewiseLinearConstraint( max4 );

		Preprocessor preprocess2( inputQuery2 );
		preprocess2.tightenPL();
		InputQuery processed2 = preprocess2.getInputQuery();

		// x0 = Relu( 0, x1 ) 
		// x0 = [0, 10]
		// x1 = [-inf, +inf]
		// x1.lb = -inf
		// x1.ub = 10
		//
		// x2 = Max( x3, x4 )
		// x2 = [5, 13]
		// x3 = [-inf, 10]
		// x4 = [4, inf]
		// x2.lb = 5
		// x2.ub = 13
		// x3.lb = -inf
		// x3.ub = 10
		// x4.lb = 4
		// x4.ub = 13
		//
		// x5 = Max( x6, x7 )
		// x5 = [0, 1]
		// x6 = [0, 2]
		// x7 = [-1, 0]
		// x5.lb = 0
		// x5.ub = 1
		// x6.lb = 0
		// x6.ub = 1
		//
		// x8 = Max( x9, x10 )
		// x8 = [0, 2]
		// x9 = [0, 1]
		// x10 = [-1, 0]
		// x8.lb = 0
		// x8.ub = 1
		TS_ASSERT( FloatUtils::areEqual( processed2.getLowerBound( 0 ), 0 ) );
		TS_ASSERT( FloatUtils::areEqual( processed2.getUpperBound( 0  ), 10 ) );
		TS_ASSERT( FloatUtils::areEqual( processed2.getLowerBound( 1 ), FloatUtils::negativeInfinity() ) );
		TS_ASSERT( FloatUtils::areEqual( processed2.getUpperBound( 1 ), 10 ) );
		TS_ASSERT( FloatUtils::areEqual( processed2.getLowerBound( 2 ), 5 ) );
		TS_ASSERT( FloatUtils::areEqual( processed2.getUpperBound( 2 ), 13 ) );
		TS_ASSERT( FloatUtils::areEqual( processed2.getLowerBound( 3 ), FloatUtils::negativeInfinity() ) );
		TS_ASSERT( FloatUtils::areEqual( processed2.getUpperBound( 3 ), 10 ) );
		TS_ASSERT( FloatUtils::areEqual( processed2.getLowerBound( 4 ), 4 ) );
		TS_ASSERT( FloatUtils::areEqual( processed2.getUpperBound( 4 ), 13 ) );
		TS_ASSERT( FloatUtils::areEqual( processed2.getLowerBound( 5 ), 0 ) );
		TS_ASSERT( FloatUtils::areEqual( processed2.getUpperBound( 5 ), 1 ) );
		TS_ASSERT( FloatUtils::areEqual( processed2.getLowerBound( 6 ), 0 ) );
		TS_ASSERT( FloatUtils::areEqual( processed2.getUpperBound( 6 ), 1 ) );
		TS_ASSERT( FloatUtils::areEqual( processed2.getLowerBound( 7 ), -1 ) );
		TS_ASSERT( FloatUtils::areEqual( processed2.getUpperBound( 7 ), 0 ) );
		TS_ASSERT( FloatUtils::areEqual( processed2.getLowerBound( 8 ), 0 ) );
		TS_ASSERT( FloatUtils::areEqual( processed2.getUpperBound( 8 ), 1 ) );
		TS_ASSERT( FloatUtils::areEqual( processed2.getLowerBound( 9 ), 0 ) );
		TS_ASSERT( FloatUtils::areEqual( processed2.getUpperBound( 9 ), 1 ) );
		TS_ASSERT( FloatUtils::areEqual( processed2.getLowerBound( 10 ), -1 ) );
		TS_ASSERT( FloatUtils::areEqual( processed2.getUpperBound( 10 ), 0 ) );

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
