/*********************                                                        */
/*! \file Test_ConstraintBoundTightener.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

**/

#include <cxxtest/TestSuite.h>

#include "MockTableau.h"
#include "ConstraintBoundTightener.h"

class MockForConstraintBoundTightener
{
public:
};

class ConstraintBoundTightenerTestSuite : public CxxTest::TestSuite
{
public:
    MockForConstraintBoundTightener *mock;
    MockTableau *tableau;

    void setUp()
    {
        TS_ASSERT( mock = new MockForConstraintBoundTightener );
        TS_ASSERT( tableau = new MockTableau );
    }

    void tearDown()
    {
        TS_ASSERT_THROWS_NOTHING( delete tableau );
        TS_ASSERT_THROWS_NOTHING( delete mock );
    }

    void test_bounds_tightened()
    {
        ConstraintBoundTightener tightener( *tableau );

        tableau->setDimensions( 2, 5, 2, 5 );

        // Current bounds:
        //  0 <= x0 <= 0
        //    5  <= x1 <= 10
        //    -2 <= x2 <= 3
        //  -100 <= x4 <= 100
        tableau->setLowerBound( 0, -200 );
        tableau->setUpperBound( 0, 200 );
        tableau->setLowerBound( 1, 5 );
        tableau->setUpperBound( 1, 10 );
        tableau->setLowerBound( 2, -2 );
        tableau->setUpperBound( 2, 3 );
        tableau->setLowerBound( 3, -5 );
        tableau->setUpperBound( 3, 5 );
        tableau->setLowerBound( 4, -100 );
        tableau->setUpperBound( 4, 100 );

        tightener.setDimensions();

        TS_ASSERT_THROWS_NOTHING( tightener.registerTighterLowerBound( 1, 7 ) );
        TS_ASSERT_THROWS_NOTHING( tightener.registerTighterUpperBound( 3, 2 ) );

        List<Tightening> tightenings;
        TS_ASSERT_THROWS_NOTHING( tightener.getConstraintTightenings( tightenings ) );

        // Lower and upper bounds should have been tightened
        TS_ASSERT_EQUALS( tightenings.size(), 2U );

        auto lower = tightenings.begin();
        while ( ( lower != tightenings.end() ) && !( ( lower->_variable == 1 ) && ( lower->_type == Tightening::LB ) ) )
            ++lower;
        TS_ASSERT( lower != tightenings.end() );

        auto upper = tightenings.begin();
        while ( ( upper != tightenings.end() ) && !( ( upper->_variable == 3 ) && ( upper->_type == Tightening::UB ) ) )
            ++upper;
        TS_ASSERT( upper != tightenings.end() );

        TS_ASSERT_EQUALS( lower->_value, 7 );
        TS_ASSERT_EQUALS( upper->_value, 2 );

        TS_ASSERT_THROWS_NOTHING( tightener.notifyLowerBound( 1, 8 ) );

        tightenings.clear();

        TS_ASSERT_THROWS_NOTHING( tightener.getConstraintTightenings( tightenings ) );
        TS_ASSERT_EQUALS( tightenings.size(), 1U );

        upper = tightenings.begin();

        TS_ASSERT_EQUALS( upper->_type, Tightening::UB );
        TS_ASSERT_EQUALS( upper->_variable, 3U );
        TS_ASSERT_EQUALS( upper->_value, 2 );
    }
};

//
// Local Variables:
// compile-command: "make -C ../../.. "
// tags-file-name: "../../../TAGS"
// c-basic-offset: 4
// End:
//
