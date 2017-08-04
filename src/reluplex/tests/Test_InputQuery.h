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

#include "InputQuery.h"
#include "MockErrno.h"
#include "ReluplexError.h"
#include "Engine.h"

#include <cfloat>
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
        TS_ASSERT_EQUALS( inputQuery.getLowerBound( 3 ), -DBL_MAX );
        TS_ASSERT_THROWS_NOTHING( inputQuery.setLowerBound( 3, -3 ) );
        TS_ASSERT_EQUALS( inputQuery.getLowerBound( 3 ), -3 );
        TS_ASSERT_THROWS_NOTHING( inputQuery.setLowerBound( 3, 5 ) );
        TS_ASSERT_EQUALS( inputQuery.getLowerBound( 3 ), 5 );

        TS_ASSERT_EQUALS( inputQuery.getLowerBound( 2 ), -DBL_MAX );
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
        TS_ASSERT_EQUALS( inputQuery.getUpperBound( 2 ), DBL_MAX );
        TS_ASSERT_THROWS_NOTHING( inputQuery.setUpperBound( 2, -4 ) );
        TS_ASSERT_EQUALS( inputQuery.getUpperBound( 2 ), -4 );
        TS_ASSERT_THROWS_NOTHING( inputQuery.setUpperBound( 2, 55 ) );
        TS_ASSERT_EQUALS( inputQuery.getUpperBound( 2 ), 55 );

        TS_ASSERT_EQUALS( inputQuery.getUpperBound( 0 ), DBL_MAX );
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
	void test_preprocess()
	{
		InputQuery inputQuery;

		inputQuery.setNumberOfVariables( 4 );
		inputQuery.setLowerBound( 1, 0 );
		inputQuery.setUpperBound( 1, 1 );
		inputQuery.setLowerBound( 2, 2 );
		inputQuery.setUpperBound( 2, 3 );
		inputQuery.setLowerBound( 3, 0 );
		inputQuery.setUpperBound( 3, 0 );

		Equation equation1;
		equation1.addAddend( 1, 0 );
		equation1.addAddend( 1, 1 );
		equation1.addAddend( -1, 2 );
		equation1.setScalar( 10 );
		equation1.markAuxiliaryVariable( 3 );
		inputQuery.addEquation( equation1 );
		
		inputQuery.preprocessBounds();
		
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
		
		inputQuery.preprocessBounds();
		
		TS_ASSERT_EQUALS( inputQuery.getLowerBound( 0 ), 0 );
		TS_ASSERT_EQUALS( inputQuery.getUpperBound( 0 ), 13 );
		TS_ASSERT_EQUALS( inputQuery.getLowerBound( 2 ), -4 );
		TS_ASSERT_EQUALS( inputQuery.getUpperBound( 2 ), 3 );
		
		inputQuery.setLowerBound( 0, -DBL_MAX );
		inputQuery.setUpperBound( 0, 15 );
		inputQuery.setLowerBound( 1, -3 );
		inputQuery.setUpperBound( 1, 1 );
		inputQuery.setLowerBound( 2, 2 );
		inputQuery.setUpperBound( 2, DBL_MAX );
		inputQuery.setLowerBound( 3, 0 );
		inputQuery.setUpperBound( 3, 0 );

		inputQuery.preprocessBounds();

		TS_ASSERT_EQUALS( inputQuery.getLowerBound( 0 ), 11 );
		TS_ASSERT_EQUALS( inputQuery.getUpperBound( 0 ), 15 );
		TS_ASSERT_EQUALS( inputQuery.getLowerBound( 2 ), 2 );
		TS_ASSERT_EQUALS( inputQuery.getUpperBound( 2 ), 2 );

		inputQuery.setLowerBound( 0, -DBL_MAX );
		inputQuery.setUpperBound( 0, 15 );
		inputQuery.setLowerBound( 1, 0 );
		inputQuery.setUpperBound( 1, 1 );
		inputQuery.setLowerBound( 2, -DBL_MAX );
		inputQuery.setUpperBound( 2, 3 );
		inputQuery.setLowerBound( 3, 0 );
		inputQuery.setUpperBound( 3, 0 );

		inputQuery.preprocessBounds();

		TS_ASSERT_EQUALS( inputQuery.getLowerBound( 0 ), -DBL_MAX );
		TS_ASSERT_EQUALS( inputQuery.getUpperBound( 0 ), 13 );
		TS_ASSERT_EQUALS( inputQuery.getLowerBound( 2 ), -4 );
		TS_ASSERT_EQUALS( inputQuery.getUpperBound( 2 ), 3 );


		

	}
};

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
