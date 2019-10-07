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


    void test_merge_and_fix_disjoint()
    {
        printf("test_merge_and_fix_disjoint\n");
		InputQuery inputQuery;
        inputQuery.setNumberOfVariables( 20 );

        /* /1* Input *1/ */
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

        printf("test");
        InputQuery processed = Preprocessor().preprocess( inputQuery );
        TS_ASSERT_EQUALS( processed.getNumberOfVariables(), 0U );
        printf("assert pass proceesed: %p , query: %p\n", &processed, &inputQuery);
    }
};

//
// Local Variables:
// compile-command: "make -C ../../.. "
// tags-file-name: "../../../TAGS"
// c-basic-offset: 4
// End:
