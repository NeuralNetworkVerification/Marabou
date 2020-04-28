/*********************                                                        */
/*! \file Test_Preprocessor.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz, Derek Huang, Shantanu Thakoor
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** \brief [[ Add one-line brief description here ]]
 **
 ** [[ Add lengthier description here ]]
 **/

#include <cxxtest/TestSuite.h>

#include "Engine.h"
#include "FloatUtils.h"
#include "InfeasibleQueryException.h"
#include "InputQuery.h"
#include "MaxConstraint.h"
#include "MockErrno.h"
#include "Preprocessor.h"
#include "ReluConstraint.h"
#include "MarabouError.h"

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

        processed = Preprocessor().preprocess( inputQuery, false );

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

        processed = Preprocessor().preprocess( inputQuery, false );

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

        TS_ASSERT_THROWS( Preprocessor().preprocess( inputQuery ), const InfeasibleQueryException &e );

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

        relu->notifyLowerBound( 0, FloatUtils::negativeInfinity() );

        inputQuery.addPiecewiseLinearConstraint( relu );
        inputQuery.addPiecewiseLinearConstraint( max1 );
        inputQuery.addPiecewiseLinearConstraint( max2 );
        inputQuery.addPiecewiseLinearConstraint( max3 );

        InputQuery processed = Preprocessor().preprocess( inputQuery, false );

        // x1 = Relu( x0 ) = max( 0, x0 )
        // x1 \in [0, 10]
        // x0 \in [-inf, +inf]
        // x0.lb = -inf
        // x0.ub = 10
        TS_ASSERT( FloatUtils::areEqual( processed.getLowerBound( 1 ), 0 ) );
        TS_ASSERT( FloatUtils::areEqual( processed.getUpperBound( 1 ), 10 ) );

        TS_ASSERT( FloatUtils::areEqual( processed.getLowerBound( 0 ), FloatUtils::negativeInfinity() ) ); //
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

        ReluConstraint *relu = new ReluConstraint( 0, 5 );
        MaxConstraint *max = new MaxConstraint( 6, Set<unsigned>( { 4, 0 } ) );
        inputQuery.addPiecewiseLinearConstraint( relu );
        inputQuery.addPiecewiseLinearConstraint( max );

        relu->notifyLowerBound( 0, FloatUtils::negativeInfinity() );

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

        InputQuery processed = Preprocessor().preprocess( inputQuery, false );

        TS_ASSERT( FloatUtils::areEqual( processed.getLowerBound( 0 ), 5.5 ) );
        TS_ASSERT( FloatUtils::areEqual( processed.getUpperBound( 0 ), 6.5 ) );
        TS_ASSERT( FloatUtils::areEqual( processed.getLowerBound( 5 ), 5.5 ) );
        TS_ASSERT( FloatUtils::areEqual( processed.getUpperBound( 5 ), 6.5 ) );
        TS_ASSERT( FloatUtils::areEqual( processed.getLowerBound( 6 ), 5.5 ) );
        TS_ASSERT( FloatUtils::areEqual( processed.getUpperBound( 6 ), 7 ) );
    }

	void test_variable_elimination()
	{
		InputQuery inputQuery;

        inputQuery.setNumberOfVariables( 10 );
        inputQuery.setLowerBound( 0, 1 ); // fixed
        inputQuery.setUpperBound( 0, 1 );
        inputQuery.setLowerBound( 1, 0 ); // fixed
        inputQuery.setUpperBound( 1, 5 );
        inputQuery.setLowerBound( 2, 2 ); // unused
        inputQuery.setUpperBound( 2, 3 );
        inputQuery.setLowerBound( 3, 5 ); // fixed
        inputQuery.setUpperBound( 3, 5 );
        inputQuery.setLowerBound( 4, 0 ); // unused
        inputQuery.setUpperBound( 4, 10 );
        inputQuery.setLowerBound( 5, 0 );  // unused
        inputQuery.setUpperBound( 5, 10 );
        inputQuery.setLowerBound( 6, 5 ); // fxied
        inputQuery.setUpperBound( 6, 5 );
        inputQuery.setLowerBound( 7, 0 ); // normal
        inputQuery.setUpperBound( 7, 9 );
        inputQuery.setLowerBound( 8, 0 ); // normal
        inputQuery.setUpperBound( 8, 9 );
        inputQuery.setLowerBound( 9, 0 ); // unused
        inputQuery.setUpperBound( 9, 9 );

        // x0 + x1 + x3 = 10
        Equation equation1;
        equation1.addAddend( 1, 0 );
        equation1.addAddend( 1, 1 );
        equation1.addAddend( 1, 3 );
        equation1.setScalar( 10 );
        inputQuery.addEquation( equation1 );

		// x7 + x8 = 12
		Equation equation2;
		equation2.addAddend( 1, 7 );
		equation2.addAddend( 1, 8 );
		equation2.setScalar( 12 );
		inputQuery.addEquation( equation2 );

		InputQuery processed = Preprocessor().preprocess( inputQuery, true );

        // Variables 2, 4, 5 and 9 are unused and should be eliminated.
        // Variables 0, 3 and 6 were fixed and should be eliminated.
        // Because of equation1 variable 1 should become fixed at 4 and be eliminated too.
        // This only leaves variables 7 and 8.
        TS_ASSERT_EQUALS( processed.getNumberOfVariables(), 2U );

        // Equation 1 should have been eliminated
        TS_ASSERT_EQUALS( processed.getEquations().size(), 1U );

        // Check that equation 2 has been updated as needed
        Equation preprocessedEquation = *processed.getEquations().begin();
        List<Equation::Addend>::iterator addend = preprocessedEquation._addends.begin();
        TS_ASSERT_EQUALS( addend->_coefficient, 1.0 );
        TS_ASSERT_EQUALS( addend->_variable, 0U );
        ++addend;
        TS_ASSERT_EQUALS( addend->_coefficient, 1.0 );
        TS_ASSERT_EQUALS( addend->_variable, 1U );

        TS_ASSERT_EQUALS( preprocessedEquation._scalar, 12.0 );
	}

    void test_all_equations_become_equalities()
    {
        InputQuery inputQuery;

        inputQuery.setNumberOfVariables( 3 );
        inputQuery.setLowerBound( 0, 1 );
        inputQuery.setUpperBound( 0, 1 );
        inputQuery.setLowerBound( 1, 0 );
        inputQuery.setUpperBound( 1, 5 );
        inputQuery.setLowerBound( 2, 2 );
        inputQuery.setUpperBound( 2, 3 );

        // x0 + -3x1 + 4x2 >= 10
        Equation equation1( Equation::GE );
        equation1.addAddend( 1, 0 );
        equation1.addAddend( -3, 1 );
        equation1.addAddend( 4, 2 );
        equation1.setScalar( 10 );
        inputQuery.addEquation( equation1 );

        InputQuery processed = Preprocessor().preprocess( inputQuery, false );

        TS_ASSERT_EQUALS( processed.getNumberOfVariables(), 4U );

        Equation eq = processed.getEquations().front();

        TS_ASSERT_EQUALS( eq._type, Equation::EQ );
        TS_ASSERT_EQUALS( eq._scalar, 10.0 );

        TS_ASSERT_EQUALS( eq._addends.size(), 4U );

        auto it = eq._addends.begin();
        TS_ASSERT_EQUALS( it->_coefficient, 1 );
        TS_ASSERT_EQUALS( it->_variable, 0U );
        ++it;
        TS_ASSERT_EQUALS( it->_coefficient, -3 );
        TS_ASSERT_EQUALS( it->_variable, 1U );
        ++it;
        TS_ASSERT_EQUALS( it->_coefficient, 4 );
        TS_ASSERT_EQUALS( it->_variable, 2U );
        ++it;
        TS_ASSERT_EQUALS( it->_coefficient, 1 );
        TS_ASSERT_EQUALS( it->_variable, 3U );
    }

    void test_identical_variable_elimination()
    {
        InputQuery inputQuery;

        inputQuery.setNumberOfVariables( 4 );
        for ( unsigned i = 0; i < 4; ++i )
        {
            inputQuery.setLowerBound( i, -3 );
            inputQuery.setUpperBound( i, 5 );
        }

        // 2x0 - 2x1 = 0
        Equation equation1;
        equation1.addAddend(  2, 0 );
        equation1.addAddend( -2, 1 );
        equation1.setScalar( 0 );
        inputQuery.addEquation( equation1 );

        // x0 + x1 + x2 = 1
        Equation equation2;
        equation2.addAddend( 1, 0 );
        equation2.addAddend( 1, 1 );
        equation2.addAddend( 1, 2 );
        equation2.setScalar( 1 );
        inputQuery.addEquation( equation2 );

        ReluConstraint *relu1 = new ReluConstraint( 1, 3 );
        relu1->notifyLowerBound( 1, -3 );

        inputQuery.addPiecewiseLinearConstraint( relu1 );

        Preprocessor preprocessor;
        InputQuery processed = preprocessor.preprocess( inputQuery, true );

        if ( !GlobalConfiguration::PREPROCESSOR_PL_CONSTRAINTS_ADD_AUX_EQUATIONS )
        {
            TS_ASSERT_EQUALS( processed.getEquations().size(), 1U );
        }
        else
        {
            TS_ASSERT( processed.getEquations().size() > 1U );
        }

        // Check that equation has been updated as needed
        Equation preprocessedEquation = *processed.getEquations().begin();

        // Make sure that variable x0 has been merged into x1
        TS_ASSERT( preprocessor.variableIsMerged( 0 ) );
        TS_ASSERT_EQUALS( preprocessor.getMergedIndex( 0 ), 1U );

        TS_ASSERT_EQUALS( preprocessor.getNewIndex( 1 ), 0U );
        TS_ASSERT_EQUALS( preprocessor.getNewIndex( 2 ), 1U );
        TS_ASSERT_EQUALS( preprocessor.getNewIndex( 3 ), 2U );

        // Variables have been renamed, so we should have 2x0 + x1 (x0 is the new x1)
        List<Equation::Addend>::iterator addend = preprocessedEquation._addends.begin();
        TS_ASSERT_EQUALS( addend->_coefficient, 2.0 );
        TS_ASSERT_EQUALS( addend->_variable, 0U );
        ++addend;
        TS_ASSERT_EQUALS( addend->_coefficient, 1.0 );
        TS_ASSERT_EQUALS( addend->_variable, 1U );
        TS_ASSERT_EQUALS( preprocessedEquation._scalar, 1.0 );

        for ( const auto &plConstraint: processed.getPiecewiseLinearConstraints() )
        {
            TS_ASSERT( plConstraint->participatingVariable( 0 ));
            TS_ASSERT( plConstraint->participatingVariable( 2 ));
        }
    }

    void test_merge_and_fix_disjoint()
    {
		InputQuery inputQuery;
        inputQuery.setNumberOfVariables( 20 );

        /* Input */
        inputQuery.setLowerBound( 0, 0.1 );
        inputQuery.setUpperBound( 0, 0.1 );
        inputQuery.setLowerBound( 1, 0.2 );
        inputQuery.setUpperBound( 1, 0.2 );
        inputQuery.setLowerBound( 2, 0.3 );
        inputQuery.setUpperBound( 2, 0.3 );
        inputQuery.setLowerBound( 3, 0.4 );
        inputQuery.setUpperBound( 3, 0.4 );

        // RNN cell 1
        inputQuery.setLowerBound( 4, 0 );
        inputQuery.setUpperBound( 4, 10 );
        inputQuery.setLowerBound( 5, 0 );
        inputQuery.setUpperBound( 5, 5000 );
        inputQuery.setLowerBound( 6, -5000 );
        inputQuery.setUpperBound( 6, 5000 );
        inputQuery.setLowerBound( 7, 0 );
        inputQuery.setUpperBound( 7, 5000 );

        // RNN cell 2
        inputQuery.setLowerBound( 8, 0 );
        inputQuery.setUpperBound( 8, 10 );
        inputQuery.setLowerBound( 9, 0 );
        inputQuery.setUpperBound( 9, 500000 );
        inputQuery.setLowerBound( 10, -5000 );
        inputQuery.setUpperBound( 10, 500000 );
        inputQuery.setLowerBound( 11, 0 );
        inputQuery.setUpperBound( 11, 500000 );

        // RNN cell 3
        inputQuery.setLowerBound( 12, 0 );
        inputQuery.setUpperBound( 12, 10 );
        inputQuery.setLowerBound( 13, 0 );
        inputQuery.setUpperBound( 13, 500000 );
        inputQuery.setLowerBound( 14, -500000 );
        inputQuery.setUpperBound( 14, 500000 );
        inputQuery.setLowerBound( 15, 0 );
        inputQuery.setUpperBound( 15, 500000 );

        // RNN cell 4
        inputQuery.setLowerBound( 16, 0 );
        inputQuery.setUpperBound( 16, 10 );
        inputQuery.setLowerBound( 17, 0 );
        inputQuery.setUpperBound( 17, 500000 );
        inputQuery.setLowerBound( 18, -500000 );
        inputQuery.setUpperBound( 18, 500000 );
        inputQuery.setLowerBound( 19, 0 );
        inputQuery.setUpperBound( 19, 500000 );

        ReluConstraint *relu1 = new ReluConstraint( 6, 7 );
        relu1->notifyLowerBound( 6, FloatUtils::negativeInfinity() );
        ReluConstraint *relu2 = new ReluConstraint( 10, 11 );
        relu2->notifyLowerBound( 10, FloatUtils::negativeInfinity() );
        ReluConstraint *relu3 = new ReluConstraint( 14, 15 );
        relu3->notifyLowerBound( 14, FloatUtils::negativeInfinity() );
        ReluConstraint *relu4 = new ReluConstraint( 18, 19 );
        relu4->notifyLowerBound( 18, FloatUtils::negativeInfinity() );

        inputQuery.addPiecewiseLinearConstraint( relu1 );
        inputQuery.addPiecewiseLinearConstraint( relu2 );
        inputQuery.addPiecewiseLinearConstraint( relu3 );
        inputQuery.addPiecewiseLinearConstraint( relu4 );

        Equation equation1;
        equation1.addAddend( -0.03, 0 );
        equation1.addAddend( -1, 6 );
        inputQuery.addEquation( equation1 );

        Equation equation2;
        equation2.addAddend( -0.06, 0 );
        equation2.addAddend( -0.05, 1 );
        equation2.addAddend( -1, 10 );
        inputQuery.addEquation( equation2 );

        Equation equation3;
        equation3.addAddend( -0.01, 0 );
        equation3.addAddend( -1, 14 );
        inputQuery.addEquation( equation3 );

        Equation equation4;
        equation4.addAddend( -0.0006, 1 );
        equation4.addAddend( -1, 18 );
        inputQuery.addEquation( equation4 );

        Equation *eq_eq = new Equation();
        eq_eq->addAddend( 1, 4 );
        eq_eq->addAddend( -1, 8 );
        eq_eq->setScalar( 0 );
        inputQuery.addEquation( *eq_eq );
        delete eq_eq;

        eq_eq = new Equation();
        eq_eq->addAddend( 1, 4 );
        eq_eq->addAddend( -1, 12 );
        eq_eq->setScalar( 0 );
        inputQuery.addEquation( *eq_eq );
        delete eq_eq;

        eq_eq = new Equation();
        eq_eq->addAddend( 1, 8 );
        eq_eq->setScalar( 0 );
        inputQuery.addEquation( *eq_eq);
        delete eq_eq;

        eq_eq = new Equation();
        eq_eq->addAddend( 1, 16 );
        eq_eq->setScalar( 0 );
        inputQuery.addEquation( *eq_eq);
        delete eq_eq;

        eq_eq = new Equation();
        eq_eq->addAddend( 1, 5 );
        eq_eq->setScalar( 0 );
        inputQuery.addEquation( *eq_eq);
        delete eq_eq;

        eq_eq = new Equation();
        eq_eq->addAddend( 1, 9 );
        eq_eq->setScalar( 0 );
        inputQuery.addEquation( *eq_eq);
        delete eq_eq;

        eq_eq = new Equation();
        eq_eq->addAddend( 1, 13 );
        eq_eq->setScalar( 0 );
        inputQuery.addEquation( *eq_eq);
        delete eq_eq;

        eq_eq = new Equation();
        eq_eq->addAddend( 1, 17 );
        eq_eq->setScalar( 0 );
        inputQuery.addEquation( *eq_eq);
        delete eq_eq;

        InputQuery processed = Preprocessor().preprocess( inputQuery );
        TS_ASSERT_EQUALS( processed.getNumberOfVariables(), 0U );
    }

    void test_construction_of_network_level_reasoner()
    {
        /*
              2      R       1
          x0 --- x2 ---> x4 --- x6
            \    /              /
           1 \  /              /
              \/           -1 /
              /\             /
           3 /  \           /
            /    \   R     /
          x1 --- x3 ---> x5
              1
        */
        InputQuery inputQuery;

        inputQuery.setNumberOfVariables( 7 );

        // Mark inputs and outputs
        inputQuery.markInputVariable( 0, 0 );
        inputQuery.markInputVariable( 1, 1 );
        inputQuery.markOutputVariable( 6, 0 );

        // Specify bounds for all variables
        for ( unsigned i = 0; i < 7; ++i )
        {
            inputQuery.setLowerBound( i, -10 );
            inputQuery.setUpperBound( i, 10 );
        }

        // Specify activation functions
        ReluConstraint *relu1 = new ReluConstraint( 2, 4 );
        ReluConstraint *relu2 = new ReluConstraint( 3, 5 );
        inputQuery.addPiecewiseLinearConstraint( relu1 );
        inputQuery.addPiecewiseLinearConstraint( relu2 );
        relu1->notifyLowerBound( 2, -10 );
        relu2->notifyLowerBound( 3, -10 );

        // Specify equations
        Equation equation1;
        equation1.addAddend( 2, 0 );
        equation1.addAddend( 3, 1 );
        equation1.addAddend( -1, 2 );
        equation1.setScalar( 0 );
        inputQuery.addEquation( equation1 );

        Equation equation2;
        equation2.addAddend( 1, 0 );
        equation2.addAddend( 1, 1 );
        equation2.addAddend( -1, 3 );
        equation2.setScalar( 0 );
        inputQuery.addEquation( equation2 );

        Equation equation3;
        equation3.addAddend( 1, 4 );
        equation3.addAddend( -1, 5 );
        equation3.addAddend( -1, 6 );
        equation3.setScalar( 0 );
        inputQuery.addEquation( equation3 );

        // Invoke preprocessor
        TS_ASSERT( !inputQuery._networkLevelReasoner );
        InputQuery processed = Preprocessor().preprocess( inputQuery );
        TS_ASSERT( processed._networkLevelReasoner );

        NetworkLevelReasoner *nlr = processed._networkLevelReasoner;

        double inputs1[2] = { 1, -2 };
        double inputs2[2] = { -4, 3 };
        double output;

        TS_ASSERT_THROWS_NOTHING( nlr->evaluate( inputs1, &output ) );
        TS_ASSERT_EQUALS( output, 0 );

        TS_ASSERT_THROWS_NOTHING( nlr->evaluate( inputs2, &output ) );
        TS_ASSERT_EQUALS( output, 1 );
    }

    void test_todo()
    {
        TS_TRACE( "In test_variable_elimination, test something about updated bounds and updated PL constraints" );
    }
};

//
// Local Variables:
// compile-command: "make -C ../../.. "
// tags-file-name: "../../../TAGS"
// c-basic-offset: 4
// End:
//
