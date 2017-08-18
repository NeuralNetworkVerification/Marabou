/*********************                                                        */
/*! \file Test_BoundTightener.h
** \verbatim
** Top contributors (to current version):
**   Duligur Ibeling
** This file is part of the Marabou project.
** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
** in the top-level source directory) and their institutional affiliations.
** All rights reserved. See the file COPYING in the top-level source
** directory for licensing information.\endverbatim
**/

#include <cxxtest/TestSuite.h>

#include "BoundTightener.h"
#include "MockTableau.h"

class MockForBoundTightener
{
public:
};

class BoundTightenerTestSuite : public CxxTest::TestSuite
{
public:
    MockForBoundTightener *mock;
    MockTableau *tableau;

    void setUp()
    {
        TS_ASSERT( mock = new MockForBoundTightener );
        TS_ASSERT( tableau = new MockTableau );
    }

    void tearDown()
    {
        TS_ASSERT_THROWS_NOTHING( delete tableau );
        TS_ASSERT_THROWS_NOTHING( delete mock );
    }

    void test_both_bounds_tightened()
    {
        BoundTightener tightener;

        tableau->setDimensions( 2, 5 );

        TableauRow row( 3 );
        // 1 - x0 - x1 + 2x2
        row._row[0] = TableauRow::Entry( 0, -1 );
        row._row[1] = TableauRow::Entry( 1, -1 );
        row._row[2] = TableauRow::Entry( 2, 2 );
        row._scalar = 1;
        tableau->nextRow = &row;

        // Current bounds:
        //    0  <= x0 <= 0
        //    5  <= x1 <= 10
        //    -2 <= x2 <= 3
        //  -100 <= x4 <= 100
        tableau->setLowerBound( 0, 0 );
        tableau->setUpperBound( 0, 0 );
        tableau->setLowerBound( 1, 5 );
        tableau->setUpperBound( 1, 10 );
        tableau->setLowerBound( 2, -2 );
        tableau->setUpperBound( 2, 3 );
        tableau->setLowerBound( 4, -100 );
        tableau->setUpperBound( 4, 100 );

        tableau->nextPivotRow = &row;
        tableau->mockLeavingVariable = 4;

        TS_ASSERT_THROWS_NOTHING( tightener.deriveTightenings( *tableau, 4, false ) );
        TS_ASSERT_THROWS_NOTHING( tightener.tighten( *tableau ) );

        // Lower and upper bounds should have been tightened
        TS_ASSERT_EQUALS( tableau->tightenedLowerBounds.size(), 1U );
        TS_ASSERT_EQUALS( tableau->tightenedUpperBounds.size(), 1U );
        TS_ASSERT( tableau->tightenedLowerBounds.exists( 4U ) );
        TS_ASSERT( tableau->tightenedUpperBounds.exists( 4U ) );

        // Lower: 1 - 10 - 4, Upper: 1 - 5 + 6
        TS_ASSERT_EQUALS( tableau->tightenedLowerBounds[4], -13 );
        TS_ASSERT_EQUALS( tableau->tightenedUpperBounds[4], 2 );
    }

    void test_just_upper_tightend()
    {
        BoundTightener tightener;

        tableau->setDimensions( 2, 5 );

        TableauRow row( 3 );
        // 1 - x0 - x1 + 2x2
        row._row[0] = TableauRow::Entry( 0, -1 );
        row._row[1] = TableauRow::Entry( 1, -1 );
        row._row[2] = TableauRow::Entry( 2, 2 );
        row._scalar = 1;
        tableau->nextRow = &row;

        // Current bounds:
        //    0  <= x0 <= 0
        //    5  <= x1 <= 10
        //    -2 <= x2 <= 3
        //    -10 <= x4 <= 100
        tableau->setLowerBound( 0, 0 );
        tableau->setUpperBound( 0, 0 );
        tableau->setLowerBound( 1, 5 );
        tableau->setUpperBound( 1, 10 );
        tableau->setLowerBound( 2, -2 );
        tableau->setUpperBound( 2, 3 );
        tableau->setLowerBound( 4, -10 );
        tableau->setUpperBound( 4, 100 );

        tableau->nextPivotRow = &row;
        tableau->mockLeavingVariable = 4;

        TS_ASSERT_THROWS_NOTHING( tightener.deriveTightenings( *tableau, 4, false ) );
        TS_ASSERT_THROWS_NOTHING( tightener.tighten( *tableau ) );

        // Lower and upper bounds should have been tightened
        TS_ASSERT( tableau->tightenedLowerBounds.empty() );
        TS_ASSERT_EQUALS( tableau->tightenedUpperBounds.size(), 1U );
        TS_ASSERT( tableau->tightenedUpperBounds.exists( 4U ) );

        // Lower: 1 - 10 - 4, Upper: 1 - 5 + 6
        TS_ASSERT_EQUALS( tableau->tightenedUpperBounds[4], 2 );
    }

    void test_just_lower_tightend()
    {
        BoundTightener tightener;

        tableau->setDimensions( 2, 5 );

        TableauRow row( 3 );
        // 1 - x0 - x1 + 2x2
        row._row[0] = TableauRow::Entry( 0, -1 );
        row._row[1] = TableauRow::Entry( 1, -1 );
        row._row[2] = TableauRow::Entry( 2, 2 );
        row._scalar = 1;
        tableau->nextRow = &row;

        // Current bounds:
        //    0  <= x0 <= 0
        //    5  <= x1 <= 10
        //    -2 <= x2 <= 3
        //  -100 <= x4 <= -1
        tableau->setLowerBound( 0, 0 );
        tableau->setUpperBound( 0, 0 );
        tableau->setLowerBound( 1, 5 );
        tableau->setUpperBound( 1, 10 );
        tableau->setLowerBound( 2, -2 );
        tableau->setUpperBound( 2, 3 );
        tableau->setLowerBound( 4, -100 );
        tableau->setUpperBound( 4, -1 );

        tableau->setLeavingVariable( 4 );

        tableau->nextPivotRow = &row;
        tableau->mockLeavingVariable = 4;

        TS_ASSERT_THROWS_NOTHING( tightener.deriveTightenings( *tableau, 4, false ) );
        TS_ASSERT_THROWS_NOTHING( tightener.tighten( *tableau ) );

        // Lower and upper bounds should have been tightened
        TS_ASSERT_EQUALS( tableau->tightenedLowerBounds.size(), 1U );
        TS_ASSERT( tableau->tightenedUpperBounds.empty() );
        TS_ASSERT( tableau->tightenedLowerBounds.exists( 4U ) );

        // Lower: 1 - 10 - 4, Upper: 1 - 5 + 6
        TS_ASSERT_EQUALS( tableau->tightenedLowerBounds[4], -13 );
    }

    void test_nothing_tightened()
    {
        BoundTightener tightener;

        tableau->setDimensions( 2, 5 );

        TableauRow row( 3 );
        // 1 - x0 - x1 + 2x2
        row._row[0] = TableauRow::Entry( 0, -1 );
        row._row[1] = TableauRow::Entry( 1, -1 );
        row._row[2] = TableauRow::Entry( 2, 2 );
        row._scalar = 1;
        tableau->nextRow = &row;

        // Current bounds:
        //    0  <= x0 <= 0
        //    5  <= x1 <= 10
        //    -2 <= x2 <= 3
        //  -100 <= x4 <= -1
        tableau->setLowerBound( 0, 0 );
        tableau->setUpperBound( 0, 0 );
        tableau->setLowerBound( 1, 5 );
        tableau->setUpperBound( 1, 10 );
        tableau->setLowerBound( 2, -2 );
        tableau->setUpperBound( 2, 3 );
        tableau->setLowerBound( 4, -10 );
        tableau->setUpperBound( 4, -1 );

        tableau->nextPivotRow = &row;
        tableau->mockLeavingVariable = 4;

        TS_ASSERT_THROWS_NOTHING( tightener.deriveTightenings( *tableau, 4, false ) );
        TS_ASSERT_THROWS_NOTHING( tightener.tighten( *tableau ) );

        // Lower and upper bounds should have been tightened
        TS_ASSERT( tableau->tightenedLowerBounds.empty() );
        TS_ASSERT( tableau->tightenedUpperBounds.empty() );
    }
};

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
