/*********************                                                        */
/*! \file Test_DegradationChecker.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
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

 /*********************                                                        */
/*! \file Test_DegradationChecker.h
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

#include "InputQuery.h"
#include "DegradationChecker.h"
#include "MockTableau.h"

class MockForDegradationChecker
{
public:
};

class DegradationCheckerTestSuite : public CxxTest::TestSuite
{
public:
	MockForDegradationChecker *mock;
    MockTableau *tableau;

	void setUp()
	{
		TS_ASSERT( mock = new MockForDegradationChecker );
        TS_ASSERT( tableau = new MockTableau );
	}

	void tearDown()
	{
		TS_ASSERT_THROWS_NOTHING( delete tableau );
		TS_ASSERT_THROWS_NOTHING( delete mock );
	}

    void test_single_equation()
    {
        InputQuery inputQuery;

        // x0 + x1 - x2 = 10
        Equation equation1;
        equation1.addAddend( 1, 0 );
        equation1.addAddend( 1, 1 );
        equation1.addAddend( -1, 2 );
        equation1.setScalar( 10 );
        inputQuery.addEquation( equation1 );

        DegradationChecker checker;
        TS_ASSERT_THROWS_NOTHING( checker.storeEquations( inputQuery ) );

        tableau->nextValues[0] = 5;
        tableau->nextValues[1] = 7;
        tableau->nextValues[2] = 2;

        TS_ASSERT( FloatUtils::areEqual( checker.computeDegradation( *tableau ), 0.0 ) );

        tableau->nextValues[2] = -2;

        TS_ASSERT( FloatUtils::areEqual( checker.computeDegradation( *tableau ), 4.0 ) );

        tableau->nextValues[2] = 5;

        TS_ASSERT( FloatUtils::areEqual( checker.computeDegradation( *tableau ), 3.0 ) );
    }

    void test_two_equation()
    {
        InputQuery inputQuery;

        // x0 + x1 - x2 = 10
        Equation equation1;
        equation1.addAddend( 1, 0 );
        equation1.addAddend( 1, 1 );
        equation1.addAddend( -1, 2 );
        equation1.setScalar( 10 );
        inputQuery.addEquation( equation1 );

        // -3x0 + 3x1 + x4 = -5
        Equation equation2;
        equation2.addAddend( -3, 0 );
        equation2.addAddend( 3, 1 );
        equation2.addAddend( 1, 4 );
        equation2.setScalar( -5 );
        inputQuery.addEquation( equation2 );

        DegradationChecker checker;
        TS_ASSERT_THROWS_NOTHING( checker.storeEquations( inputQuery ) );

        tableau->nextValues[0] = 5;
        tableau->nextValues[1] = 0;
        tableau->nextValues[2] = 2;
        tableau->nextValues[3] = -2;
        tableau->nextValues[4] = 1;

        // x0 + x1 - x2    = 3   --> contribute 7
        // -3x0 + 3x1 + x4 = -14 --> contribute 9

        TS_ASSERT( FloatUtils::areEqual( checker.computeDegradation( *tableau ), 16.0 ) );

        tableau->nextValues[1] = 0.5;

        // x0 + x1 - x2    = 3.5    --> contribute 6.5
        // -3x0 + 3x1 + x4 = -12.5  --> contribute 7.5

        TS_ASSERT( FloatUtils::areEqual( checker.computeDegradation( *tableau ), 14.0 ) );
    }
};

//
// Local Variables:
// compile-command: "make -C ../../.. "
// tags-file-name: "../../../TAGS"
// c-basic-offset: 4
// End:
//
