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

#include "Engine.h"
#include "FloatUtils.h"
#include "InputQuery.h"
#include "MaxConstraint.h"
#include "MockErrno.h"
#include "Preprocessor.h"
#include "ReluConstraint.h"
#include "ReluplexError.h"

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

	void test_tighten_equation_bounds()
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

        InputQuery processed = Preprocessor().preprocess( inputQuery );

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

        inputQuery.setLowerBound( 0, 0 );
        inputQuery.setUpperBound( 0, FloatUtils::infinity() );
        inputQuery.setLowerBound( 1, 0 );
        inputQuery.setUpperBound( 1, 1 );
        inputQuery.setLowerBound( 2, FloatUtils::negativeInfinity() );
        inputQuery.setUpperBound( 2, 3 );
        inputQuery.setLowerBound( 3, 0 );
        inputQuery.setUpperBound( 3, 0 );

        // x0.lb = 10 - x1.ub + x2.lb --> Unbounded
        // x0.ub = 10 - x1.lb + x2.ub = 10 - 0 + 3 = 13

        // x2.lb = -10 + x0.lb + x1.lb = -10 + 0 + 0 = -10
        // x2.ub = -10 + x0.ub + x1.ub = -10 + 13 + 1 = 4
        // 3 is a tighter bound than 4, keep 3 as the upper bound

        processed = Preprocessor().preprocess( inputQuery );

        TS_ASSERT_EQUALS( processed.getLowerBound( 0 ), 0 );
        TS_ASSERT_EQUALS( processed.getUpperBound( 0 ), 13 );
        TS_ASSERT_EQUALS( processed.getLowerBound( 2 ), -10 );
		TS_ASSERT_EQUALS( processed.getUpperBound( 2 ), 3 );

        inputQuery.setLowerBound( 0, FloatUtils::negativeInfinity() );
        inputQuery.setUpperBound( 0, 15 );
        inputQuery.setLowerBound( 1, 3 );
        inputQuery.setUpperBound( 1, 3 );
        inputQuery.setLowerBound( 2, 2 );
        inputQuery.setUpperBound( 2, FloatUtils::infinity() );
        inputQuery.setLowerBound( 3, 0 );
        inputQuery.setUpperBound( 3, 0 );

        // x0.lb = 10 - x1.ub + x2.lb = 10 - 3 + 2 = 9
        // x0.ub = 10 - x1.lb + x2.ub --> Unbounded

        // x2.lb = -10 + x0.lb + x1.lb = -10 + 9 + 3 = 2
        // x2.ub = -10 + x0.ub + x1.ub = -10 + 15 + 3 = 8

        processed = Preprocessor().preprocess( inputQuery );

        TS_ASSERT_EQUALS( processed.getLowerBound( 0 ), 9 );
        TS_ASSERT_EQUALS( processed.getUpperBound( 0 ), 15 );
        TS_ASSERT_EQUALS( processed.getLowerBound( 2 ), 2 );
        TS_ASSERT_EQUALS( processed.getUpperBound( 2 ), 8 );

        inputQuery.setLowerBound( 0, FloatUtils::negativeInfinity() );
        inputQuery.setUpperBound( 0, 15 );
        inputQuery.setLowerBound( 1, -3 );
        inputQuery.setUpperBound( 1, -2 );
        inputQuery.setLowerBound( 2, FloatUtils::negativeInfinity() );
        inputQuery.setUpperBound( 2, 6 );
        inputQuery.setLowerBound( 3, 0 );
        inputQuery.setUpperBound( 3, 0 );

        // x0.lb = 10 - x1.ub + x2.lb --> Unbounded
        // x0.ub = 10 - x1.lb + x2.ub = 10 - -3 + 6 = 19
        // 15 is a tighter bound, keep 15 as the upper bound

        // x2.lb = -10 + x0.lb + x1.lb = Unbounded
        // x2.ub = -10 + x0.ub + x1.ub = -10 + 15 -2 = 3

        processed = Preprocessor().preprocess( inputQuery );

        TS_ASSERT_EQUALS( processed.getLowerBound( 0 ), FloatUtils::negativeInfinity() );
        TS_ASSERT_EQUALS( processed.getUpperBound( 0 ), 15 );
        TS_ASSERT_EQUALS( processed.getLowerBound( 2 ), FloatUtils::negativeInfinity() );
        TS_ASSERT_EQUALS( processed.getUpperBound( 2 ), 3 );

        inputQuery.setLowerBound( 0, FloatUtils::negativeInfinity() );
        inputQuery.setUpperBound( 0, 5 );
        inputQuery.setLowerBound( 1, -3 );
        inputQuery.setUpperBound( 1, -2 );
        inputQuery.setLowerBound( 2, 0 );
        inputQuery.setUpperBound( 2, 6 );
        inputQuery.setLowerBound( 3, 0 );
        inputQuery.setUpperBound( 3, 0 );

        // x0.lb = 10 - x1.ub + x2.lb --> Unbounded
        // x0.ub = 10 - x1.lb + x2.ub = 10 - -3 + 6 = 19
        // 5 is a tighter bound, keep 5 as the upper bound

        // x2.lb = -10 + x0.lb + x1.lb = Unbounded
        // 0 is a tighter bound, keep 0 as the lower bound
        // x2.ub = -10 + x0.ub + x1.ub = -10 + 5 -2 = -7
        // x2 = [0, -7] -> throw error

        TS_ASSERT_THROWS_EQUALS( Preprocessor().preprocess( inputQuery ),
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

        processed = Preprocessor().preprocess( inputQuery2 );

        TS_ASSERT_EQUALS( processed.getLowerBound( 0 ), 5.5 );
        TS_ASSERT_EQUALS( processed.getUpperBound( 0 ), 6.5 );
	}

    void test_tighten_bounds_using_constraints()
    {
		 InputQuery inputQuery;

		 inputQuery.setNumberOfVariables( 17 );
		 inputQuery.setLowerBound( 1, 0 );
		 inputQuery.setUpperBound( 1, 10 );
         inputQuery.setLowerBound( 2, 5 );
         inputQuery.setUpperBound( 2, 13 );
         inputQuery.setUpperBound( 3, 10 );
   		 inputQuery.setLowerBound( 4, 4 );
		 inputQuery.setLowerBound( 5, 0 );
		 inputQuery.setUpperBound( 5, 1 );
		 inputQuery.setLowerBound( 6, 0 );
		 inputQuery.setUpperBound( 6, 2 );
		 inputQuery.setLowerBound( 7, -1 );
		 inputQuery.setUpperBound( 7, 0 );
		 inputQuery.setLowerBound( 8, 0 );
		 inputQuery.setUpperBound( 8, 2 );
		 inputQuery.setLowerBound( 9, 0 );
		 inputQuery.setUpperBound( 9, 1 );
		 inputQuery.setLowerBound( 10, -1 );
		 inputQuery.setUpperBound( 10, 0 );

		 ReluConstraint *relu = new ReluConstraint( 0, 1 );
		 MaxConstraint *max1 = new MaxConstraint( 2, Set<unsigned>( { 4, 3 } ) );
		 MaxConstraint *max2 = new MaxConstraint( 5, Set<unsigned>( { 6, 7 } ) );
		 MaxConstraint *max3 = new MaxConstraint( 8, Set<unsigned>( { 9, 10 } ) );

		 inputQuery.addPiecewiseLinearConstraint( relu );
		 inputQuery.addPiecewiseLinearConstraint( max1 );
		 inputQuery.addPiecewiseLinearConstraint( max2 );
		 inputQuery.addPiecewiseLinearConstraint( max3 );

         InputQuery processed = Preprocessor().preprocess( inputQuery );

		 // x1 = Relu( x0 ) = max( 0, x0 )
		 // x1 \in [0, 10]
		 // x0 \in [-inf, +inf]
		 // x0.lb = -inf
		 // x0.ub = 10
		 TS_ASSERT( FloatUtils::areEqual( processed.getLowerBound( 1 ), 0 ) );
		 TS_ASSERT( FloatUtils::areEqual( processed.getUpperBound( 1 ), 10 ) );
		 TS_ASSERT( FloatUtils::areEqual( processed.getLowerBound( 0 ), FloatUtils::negativeInfinity() ) );
		 TS_ASSERT( FloatUtils::areEqual( processed.getUpperBound( 0 ), 10 ) );

         // x2 = Max( x3, x4 )
		 // x2 \in [5, 13]
		 // x3 \in [-inf, 10]
		 // x4 \in [4, inf]
		 // x2.lb = 5
		 // x2.ub = 13
		 // x3.lb = -inf
		 // x3.ub = 10
		 // x4.lb = 4
		 // x4.ub = 13
		 TS_ASSERT( FloatUtils::areEqual( processed.getLowerBound( 2 ), 5 ) );
		 TS_ASSERT( FloatUtils::areEqual( processed.getUpperBound( 2 ), 13 ) );
		 TS_ASSERT( FloatUtils::areEqual( processed.getLowerBound( 3 ), FloatUtils::negativeInfinity() ) );
		 TS_ASSERT( FloatUtils::areEqual( processed.getUpperBound( 3 ), 10 ) );
		 TS_ASSERT( FloatUtils::areEqual( processed.getLowerBound( 4 ), 4 ) );
		 TS_ASSERT( FloatUtils::areEqual( processed.getUpperBound( 4 ), 13 ) );

         // x5 = Max( x6, x7 )
		 // x5 \in [0, 1]
		 // x6 \in [0, 2]
		 // x7 \in [-1, 0]
		 // x5.lb = 0
		 // x5.ub = 1
		 // x6.lb = 0
		 // x6.ub = 1
		 TS_ASSERT( FloatUtils::areEqual( processed.getLowerBound( 5 ), 0 ) );
		 TS_ASSERT( FloatUtils::areEqual( processed.getUpperBound( 5 ), 1 ) );
		 TS_ASSERT( FloatUtils::areEqual( processed.getLowerBound( 6 ), 0 ) );
		 TS_ASSERT( FloatUtils::areEqual( processed.getUpperBound( 6 ), 1 ) );
		 TS_ASSERT( FloatUtils::areEqual( processed.getLowerBound( 7 ), -1 ) );
		 TS_ASSERT( FloatUtils::areEqual( processed.getUpperBound( 7 ), 0 ) );

         // x8 = Max( x9, x10 )
		 // x8 \in [0, 2]
		 // x9 \in [0, 1]
		 // x10 \in [-1, 0]
		 // x8.lb = 0
		 // x8.ub = 1
		 TS_ASSERT( FloatUtils::areEqual( processed.getLowerBound( 8 ), 0 ) );
		 TS_ASSERT( FloatUtils::areEqual( processed.getUpperBound( 8 ), 1 ) );
		 TS_ASSERT( FloatUtils::areEqual( processed.getLowerBound( 9 ), 0 ) );
		 TS_ASSERT( FloatUtils::areEqual( processed.getUpperBound( 9 ), 1 ) );
		 TS_ASSERT( FloatUtils::areEqual( processed.getLowerBound( 10 ), -1 ) );
		 TS_ASSERT( FloatUtils::areEqual( processed.getUpperBound( 10 ), 0 ) );
    }

	void test_tighten_bounds_using_equations_and_constraints()
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

		 InputQuery processed = Preprocessor().preprocess( inputQuery );

         TS_ASSERT( FloatUtils::areEqual( processed.getLowerBound( 0 ), 5.5 ) );
         TS_ASSERT( FloatUtils::areEqual( processed.getUpperBound( 0 ), 6.5 ) );
		 TS_ASSERT( FloatUtils::areEqual( processed.getLowerBound( 5 ), 5.5 ) );
		 TS_ASSERT( FloatUtils::areEqual( processed.getUpperBound( 5 ), 6.5 ) );
		 TS_ASSERT( FloatUtils::areEqual( processed.getLowerBound( 6 ), 5.5 ) );
		 TS_ASSERT( FloatUtils::areEqual( processed.getUpperBound( 6 ), 7 ) );
    }


	// void test_eliminate_variables()
	// {
	// 	InputQuery inputQuery;

    //     inputQuery.setNumberOfVariables( 10 );
	// 	inputQuery.setLowerBound( 0, 1 );
	// 	inputQuery.setUpperBound( 0, 1 );
    //     inputQuery.setLowerBound( 1, 0 );
    //     inputQuery.setUpperBound( 1, 1 );
    //     inputQuery.setLowerBound( 2, 2 );
    //     inputQuery.setUpperBound( 2, 3 );
	// 	inputQuery.setLowerBound( 3, 5 );
	// 	inputQuery.setUpperBound( 3, 5 );
	// 	inputQuery.setLowerBound( 4, 0 );
	// 	inputQuery.setUpperBound( 4, 10 );
	// 	inputQuery.setLowerBound( 5, 0 );
	// 	inputQuery.setUpperBound( 5, 10 );
	// 	inputQuery.setLowerBound( 6, 5 );
	// 	inputQuery.setUpperBound( 6, 5 );
	// 	inputQuery.setLowerBound( 7, 0 );
	// 	inputQuery.setUpperBound( 7, 9 );
	// 	inputQuery.setLowerBound( 8, 0 );
	// 	inputQuery.setUpperBound( 8, 9 );
	// 	inputQuery.setLowerBound( 9, 0 );
	// 	inputQuery.setUpperBound( 9, 9 );

    //     // x0 + x1 + x3 = 10
    //     Equation equation1;
    //     equation1.addAddend( 1, 0 );
    //     equation1.addAddend( 1, 1 );
    //     equation1.addAddend( 1, 3 );
    //     equation1.setScalar( 10 );
    //     inputQuery.addEquation( equation1 );

	// 	// x2 + x3 = 6
	// 	Equation equation2;
	// 	equation2.addAddend( 1, 3 );
	// 	equation2.addAddend( 1, 2 );
	// 	equation2.setScalar( 6 );
	// 	inputQuery.addEquation( equation2 );

	// 	MaxConstraint *max = new MaxConstraint( 4, Set<unsigned>( {5, 6, 7 } ) );
	// 	ReluConstraint *relu = new ReluConstraint( 4, 5 );
	// 	inputQuery.addPiecewiseLinearConstraint( max );
	// 	inputQuery.addPiecewiseLinearConstraint( relu );

	// 	Preprocessor preprocess( inputQuery );
	// 	preprocess.preprocess();
    //     preprocess.eliminateVariables();
	// 	InputQuery processed = preprocess.getInputQuery();

	// 	//eliminate variables x0, x3 because UB = LB
	// 	TS_ASSERT_EQUALS( processed.getNumberOfVariables(), 7U );

	// 	//	x0 + x1 + x3 = 10
	// 	//	x1 + x3 = 10 - x0 = 9
	// 	//	x0 + x3 = 9
	// 	//	x0 = 9 - x3 = 4
	// 	//	1 addend: x0
	// 	//	scalar = 4
	// 	TS_ASSERT( FloatUtils::areEqual( processed.getEquations().front()._scalar, 4 ) );
	// 	TS_ASSERT( FloatUtils::areEqual( processed.getEquations().front()._addends.size(), 1 ) );
	// 	TS_ASSERT( FloatUtils::areEqual( processed.getEquations().front()._addends.front()._variable, 0 ) );
	// 	//	x2 + x3 = 6
	// 	//	x1 + x3 = 6
	// 	//	x1 = 6 - x3 = 1
	// 	//	1 addend: x1
	// 	//	scalar = 1
	// 	TS_ASSERT( FloatUtils::areEqual( processed.getEquations().back()._scalar, 1 ) );
	// 	TS_ASSERT( FloatUtils::areEqual( processed.getEquations().back()._addends.size(), 1 ) );
	// 	TS_ASSERT( FloatUtils::areEqual( processed.getEquations().back()._addends.front()._variable, 1 ) );

	// 	TS_ASSERT_EQUALS( processed.getPiecewiseLinearConstraints().front()->getParticipatingVariables().size(), 3U );
	// 	TS_ASSERT_EQUALS( processed.getPiecewiseLinearConstraints().back()->getParticipatingVariables().size(), 2U );

	// 	TS_ASSERT( FloatUtils::areEqual( processed.getLowerBound( 2 ), 5 ) );
	// 	TS_ASSERT( FloatUtils::areEqual( processed.getUpperBound( 2 ), 10 ) );
	// 	TS_ASSERT( FloatUtils::areEqual( processed.getLowerBound( 3 ), 5 ) );
	// 	TS_ASSERT( FloatUtils::areEqual( processed.getUpperBound( 3 ), 10 ) );
	// 	TS_ASSERT( FloatUtils::areEqual( processed.getLowerBound( 4 ), 0 ) );
	// 	TS_ASSERT( FloatUtils::areEqual( processed.getUpperBound( 4 ), 9 ) );
	// }

	// void xtest_saturation()
	// {
	// 	InputQuery inputQuery;

	// 	inputQuery.setNumberOfVariables( 5 );
	// 	inputQuery.setLowerBound( 1, 0 );
	// 	inputQuery.setUpperBound( 1, 5 );
	// 	inputQuery.setUpperBound( 2, 3 );
	// 	inputQuery.setLowerBound( 3, 1 );
	// 	inputQuery.setUpperBound( 3, 5 );

	// 	MaxConstraint *max = new MaxConstraint( 1, Set<unsigned>( { 0, 3 } ) );
	// 	inputQuery.addPiecewiseLinearConstraint( max );
	// 	Equation equation1;
	// 	equation1.addAddend( 1, 0 );
	// 	equation1.addAddend( 1, 1 );
	// 	equation1.addAddend( 1, 2 );
	// 	equation1.setScalar( 10 );
	// 	inputQuery.addEquation( equation1 );

	// 	// x0 + x1 + x2 = 10
	// 	// x1 = Max( x0, x3 )
	// 	// x0 = [-inf, inf]
	// 	// x1 = [0, 5]
	// 	// x2 = [-inf, 3]
	// 	// x3 = [1, 5]
	// 	//
	// 	// tighten equation bounds
	// 	// x0 = 10 - x1 - x2
	// 	// x0 = [2, inf]
	// 	//
	// 	// tighten max bounds
	// 	// [0, 5] = Max( [2, inf], [1, 5] )
	// 	// result
	// 	// [2, 5] = Max( [2, 5], [1, 5] )
	// 	//
	// 	// tighten equation bounds
	// 	// [2, 5] + [2, 5] + [-inf, 3] = 10
	// 	// x2 = 10 - [2, 5] - [2, 5]
	// 	// x2 = [0, 3]
	// 	// 3 is tighter than 6, keep 3
	// 	//
	// 	// x0 = [2, 5]
	// 	// x1 = [2, 5]
	// 	// x2 = [0, 3]
	// 	// x3 = [1. 5]

	// 	Preprocessor preprocess( inputQuery );
	// 	InputQuery project = preprocess.preprocess();

	// 	TS_ASSERT( FloatUtils::areEqual( project.getLowerBound( 0 ), 2 ) );
	// 	TS_ASSERT( FloatUtils::areEqual( project.getUpperBound( 0 ), 5 ) );
	// 	TS_ASSERT( FloatUtils::areEqual( project.getLowerBound( 1 ), 2 ) );
	// 	TS_ASSERT( FloatUtils::areEqual( project.getUpperBound( 1 ), 5 ) );
	// 	TS_ASSERT( FloatUtils::areEqual( project.getLowerBound( 2 ), 0 ) );
	// 	TS_ASSERT( FloatUtils::areEqual( project.getUpperBound( 2 ), 3 ) );
	// 	TS_ASSERT( FloatUtils::areEqual( project.getLowerBound( 3 ), 1 ) );
	// 	TS_ASSERT( FloatUtils::areEqual( project.getUpperBound( 3 ), 5 ) );

	// 	InputQuery inputQuery2;

	// 	inputQuery2.setNumberOfVariables( 5 );
	// 	inputQuery2.setLowerBound( 1, 0 );
	// 	inputQuery2.setUpperBound( 1, 6 );
	// 	inputQuery2.setUpperBound( 2, 3 );

	// 	Equation equation;
	// 	equation.addAddend( 1, 0 );
	// 	equation.addAddend( 1, 1 );
	// 	equation.addAddend( 1, 2 );
	// 	equation.setScalar( 10 );
	// 	inputQuery2.addEquation( equation1 );

	// 	ReluConstraint *relu = new ReluConstraint( 1, 0 );
	// 	inputQuery2.addPiecewiseLinearConstraint( relu );

	// 	// x0 + x1 + x2 = 10
	// 	// x1 = Relu( 0, x0 )
	// 	// x0 = [-inf, inf]
	// 	// x1 = [0, 6]
	// 	// x2 = [-inf, 3]
	// 	//
	// 	// tighten equation bounds
	// 	// x0 = 10 - x1 - x2
	// 	// x0 = [1, inf]
	// 	//
	// 	// tighten relu bounds
	// 	// [0, 6] = Relu( 0, [1, inf] )
	// 	// result
	// 	// [1, 6] = Relu( 0, [1, 6] )
	// 	//
	// 	// tighten equation bounds
	// 	// [1, 6] + [1, 6] + [-inf, 3] = 10
	// 	// x2 = 10 - [1, 6] - [1, 6]
	// 	// x2 = [-2, 3]
	// 	// 3 is tighter than 8, keep 3
	// 	//
	// 	// x0 = [1, 6]
	// 	// x1 = [1, 6]
	// 	// x2 = [-2, 3]

	// 	Preprocessor preprocess2( inputQuery2 );
	// 	InputQuery reluInput = preprocess2.preprocess();

	// 	TS_ASSERT( FloatUtils::areEqual( reluInput.getLowerBound( 0 ), 1 ) );
	// 	TS_ASSERT( FloatUtils::areEqual( reluInput.getUpperBound( 0 ), 6 ) );
	// 	TS_ASSERT( FloatUtils::areEqual( reluInput.getLowerBound( 1 ), 1 ) );
	// 	TS_ASSERT( FloatUtils::areEqual( reluInput.getUpperBound( 1 ), 6 ) );
	// 	TS_ASSERT( FloatUtils::areEqual( reluInput.getLowerBound( 2 ), -2 ) );
	// 	TS_ASSERT( FloatUtils::areEqual( reluInput.getUpperBound( 2 ), 3 ) );
	// }

    // void test_todo()
    // {
    //     TS_TRACE( "Reinstate and revise tests" );
    //     TS_TRACE( "Revise Preprocessor interface, maybe make some things private" );
    //     TS_TRACE( "Revise & invoke variable elimination" );
    // }
};

//
// Local Variables:
// compile-command: "make -C ../../.. "
// tags-file-name: "../../../TAGS"
// c-basic-offset: 4
// End:
//
