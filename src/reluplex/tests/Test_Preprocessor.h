 /*********************                                                        */
/*! \file Test_Preprocessor.h
** \verbatim
** Top contributors (to current version):
**   Derek Huang
** This file is part of the Marabou project.
** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
** in the top-level source directory) and their institutional affiliations.
** All rights reserved. See the file COPYING in the top-level source
** directory for licensing information.\endverbatim
**/

#include <cxxtest/TestSuite.h>

#include "Preprocessor.h"
#include "InputQuery.h"
#include "MockErrno.h"
#include "ReluplexError.h"
#include "Engine.h"

#include <cfloat>
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
		Preprocessor preprocess;

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

        preprocess.tightenBounds( inputQuery );

        // x0 = 10 - x1 + x2
        //
        // x0.lb = 10 - x1.ub + x2.lb = 10 - 1 + 2 = 11
        // x0.ub = 10 - x1.lb + x2.ub = 10 - 0 + 3 = 13
        //
        // x2 = -10 + x0 + x1
        //
        // x2.lb = -10 + x0.lb + x1.lb
        // x2.ub = -10 + x0.ub + x1.ub

        TS_ASSERT_EQUALS( inputQuery.getLowerBound( 0 ), 11 );
        TS_ASSERT_EQUALS( inputQuery.getUpperBound( 0 ), 13 );

        inputQuery.setLowerBound( 0, 0 );
        inputQuery.setUpperBound( 0, DBL_MAX );
        inputQuery.setLowerBound( 1, 0 );
        inputQuery.setUpperBound( 1, 1 );
        inputQuery.setLowerBound( 2, -DBL_MAX );
        inputQuery.setUpperBound( 2, 3 );
        inputQuery.setLowerBound( 3, 0 );
        inputQuery.setUpperBound( 3, 0 );

        // x0.lb = 10 - x1.ub + x2.lb --> Unbounded
        // x0.ub = 10 - x1.lb + x2.ub = 10 - 0 + 3 = 13

        // x2.lb = -10 + x0.lb + x1.lb = -10 + 0 + 0 = -10
        // x2.ub = -10 + x0.ub + x1.ub = -10 + 13 + 1 = 4
        // 3 is a tighter bound than 4, keep 3 as the upper bound

        preprocess.tightenBounds( inputQuery );

        TS_ASSERT_EQUALS( inputQuery.getLowerBound( 0 ), 0 );
        TS_ASSERT_EQUALS( inputQuery.getUpperBound( 0 ), 13 );
        TS_ASSERT_EQUALS( inputQuery.getLowerBound( 2 ), -10 );
		TS_ASSERT_EQUALS( inputQuery.getUpperBound( 2 ), 3 );

        inputQuery.setLowerBound( 0, -DBL_MAX );
        inputQuery.setUpperBound( 0, 15 );
        inputQuery.setLowerBound( 1, 3 );
        inputQuery.setUpperBound( 1, 3 );
        inputQuery.setLowerBound( 2, 2 );
        inputQuery.setUpperBound( 2, DBL_MAX );
        inputQuery.setLowerBound( 3, 0 );
        inputQuery.setUpperBound( 3, 0 );

        // x0.lb = 10 - x1.ub + x2.lb = 10 - 3 + 2 = 9
        // x0.ub = 10 - x1.lb + x2.ub --> Unbounded

        // x2.lb = -10 + x0.lb + x1.lb = -10 + 9 + 3 = 2
        // x2.ub = -10 + x0.ub + x1.ub = -10 + 15 + 3 = 8

        preprocess.tightenBounds( inputQuery );

        TS_ASSERT_EQUALS( inputQuery.getLowerBound( 0 ), 9 );
        TS_ASSERT_EQUALS( inputQuery.getUpperBound( 0 ), 15 );
        TS_ASSERT_EQUALS( inputQuery.getLowerBound( 2 ), 2 );
        TS_ASSERT_EQUALS( inputQuery.getUpperBound( 2 ), 8 );

        inputQuery.setLowerBound( 0, -DBL_MAX );
        inputQuery.setUpperBound( 0, 15 );
        inputQuery.setLowerBound( 1, -3 );
        inputQuery.setUpperBound( 1, -2 );
        inputQuery.setLowerBound( 2, -DBL_MAX );
        inputQuery.setUpperBound( 2, 6 );
        inputQuery.setLowerBound( 3, 0 );
        inputQuery.setUpperBound( 3, 0 );

        // x0.lb = 10 - x1.ub + x2.lb --> Unbounded
        // x0.ub = 10 - x1.lb + x2.ub = 10 - -3 + 6 = 19
        // 15 is a tighter bound, keep 15 as the upper bound

        // x2.lb = -10 + x0.lb + x1.lb = Unbounded
        // x2.ub = -10 + x0.ub + x1.ub = -10 + 15 -2 = 3

        preprocess.tightenBounds( inputQuery );

        TS_ASSERT_EQUALS( inputQuery.getLowerBound( 0 ), -DBL_MAX );
        TS_ASSERT_EQUALS( inputQuery.getUpperBound( 0 ), 15 );
        TS_ASSERT_EQUALS( inputQuery.getLowerBound( 2 ), -DBL_MAX );
        TS_ASSERT_EQUALS( inputQuery.getUpperBound( 2 ), 3 );

        inputQuery.setLowerBound( 0, -DBL_MAX );
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

        TS_ASSERT_THROWS_EQUALS( preprocess.tightenBounds( inputQuery ),
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

        preprocess.tightenBounds( inputQuery2 );

        TS_ASSERT_EQUALS( inputQuery2.getLowerBound( 0 ), 5.5 );
        TS_ASSERT_EQUALS( inputQuery2.getUpperBound( 0 ), 6.5 );
	}
};

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
