/*********************                                                        */
/*! \file Test_RowBoundTightener.h
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

#include "MockTableau.h"
#include "RowBoundTightener.h"

class MockForRowBoundTightener
{
public:
};

class RowBoundTightenerTestSuite : public CxxTest::TestSuite
{
public:
    MockForRowBoundTightener *mock;
    MockTableau *tableau;

    void setUp()
    {
        TS_ASSERT( mock = new MockForRowBoundTightener );
        TS_ASSERT( tableau = new MockTableau );
    }

    void tearDown()
    {
        TS_ASSERT_THROWS_NOTHING( delete tableau );
        TS_ASSERT_THROWS_NOTHING( delete mock );
    }

    void test_pivot_row__both_bounds_tightened()
    {
        RowBoundTightener tightener;

        tableau->setDimensions( 2, 5 );
        tightener.initialize( *tableau );

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

        tightener.reset( *tableau );

        TableauRow row( 3 );
        // 1 - x0 - x1 + 2x2
        row._row[0] = TableauRow::Entry( 0, -1 );
        row._row[1] = TableauRow::Entry( 1, -1 );
        row._row[2] = TableauRow::Entry( 2, 2 );
        row._scalar = 1;
        tableau->nextRow = &row;

        tableau->nextPivotRow = &row;
        tableau->mockLeavingVariable = 0;
        tableau->nextEnteringVariable = 4;
        tableau->nextEnteringVariableIndex = 0;

        // 1 - x0 - x1 + 2x2 = x4 (pre pivot)
        // x0 entering, x4 leaving
        // x0 = 1 - x1 + 2 x2 - x4

        TS_ASSERT_THROWS_NOTHING( tightener.examinePivotRow( *tableau ) );

        List<Tightening> tightenings;
        TS_ASSERT_THROWS_NOTHING( tightener.getRowTightenings( tightenings ) );

        // Lower and upper bounds should have been tightened
        TS_ASSERT_EQUALS( tightenings.size(), 2U );
        auto lower = *tightenings.begin();
        auto upper = *tightenings.rbegin();

        TS_ASSERT_EQUALS( lower._variable, 0U );
        TS_ASSERT_EQUALS( lower._type, Tightening::LB );
        TS_ASSERT_EQUALS( upper._variable, 0U );
        TS_ASSERT_EQUALS( upper._type, Tightening::UB );

        // LB -> 1 - 10 - 4 -100
        // UB -> 1 - 5 + 6 + 100
        TS_ASSERT_EQUALS( lower._value, -113 );
        TS_ASSERT_EQUALS( upper._value, 102 );
    }

    void test_pivot_row__just_upper_tightend()
    {
        RowBoundTightener tightener;

        tableau->setDimensions( 2, 5 );
        tightener.initialize( *tableau );

        // Current bounds:
        //    0  <= x0 <= 0
        //    5  <= x1 <= 10
        //    -2 <= x2 <= 3
        //    -10 <= x4 <= 100
        tableau->setLowerBound( 0, 0 );
        tableau->setUpperBound( 0, 200 );
        tableau->setLowerBound( 1, 5 );
        tableau->setUpperBound( 1, 10 );
        tableau->setLowerBound( 2, -2 );
        tableau->setUpperBound( 2, 3 );
        tableau->setLowerBound( 3, -5 );
        tableau->setUpperBound( 3, 5 );
        tableau->setLowerBound( 4, 0 );
        tableau->setUpperBound( 4, 0 );

        tightener.reset( *tableau );

        TableauRow row( 3 );
        // 1 - x0 - x1 + 2x2
        row._row[0] = TableauRow::Entry( 0, -1 );
        row._row[1] = TableauRow::Entry( 1, -1 );
        row._row[2] = TableauRow::Entry( 2, 2 );
        row._scalar = 1;
        tableau->nextRow = &row;

        tableau->nextPivotRow = &row;
        tableau->mockLeavingVariable = 0;
        tableau->nextEnteringVariable = 4;
        tableau->nextEnteringVariableIndex = 0;

        TS_ASSERT_THROWS_NOTHING( tightener.examinePivotRow( *tableau ) );

        List<Tightening> tightenings;
        TS_ASSERT_THROWS_NOTHING( tightener.getRowTightenings( tightenings ) );

        // Lower and upper bounds should have been tightened
        TS_ASSERT_EQUALS( tightenings.size(), 1U );
        auto upper = *tightenings.begin();

        TS_ASSERT_EQUALS( upper._variable, 0U );
        TS_ASSERT_EQUALS( upper._type, Tightening::UB );

        // Lower: 1 - 10 - 4, Upper: 1 - 5 + 6
        TS_ASSERT_EQUALS( upper._value, 2 );
    }

    void test_pivot_row__just_lower_tightend()
    {
        RowBoundTightener tightener;

        tableau->setDimensions( 2, 5 );
        tightener.initialize( *tableau );

        tableau->setLowerBound( 0, -200 );
        tableau->setUpperBound( 0, 0 );
        tableau->setLowerBound( 1, 5 );
        tableau->setUpperBound( 1, 10 );
        tableau->setLowerBound( 2, -2 );
        tableau->setUpperBound( 2, 3 );
        tableau->setLowerBound( 3, -5 );
        tableau->setUpperBound( 3, 5 );
        tableau->setLowerBound( 4, 0 );
        tableau->setUpperBound( 4, 0 );

        tightener.reset( *tableau );

        TableauRow row( 3 );
        // 1 - x0 - x1 + 2x2
        row._row[0] = TableauRow::Entry( 0, -1 );
        row._row[1] = TableauRow::Entry( 1, -1 );
        row._row[2] = TableauRow::Entry( 2, 2 );
        row._scalar = 1;
        tableau->nextRow = &row;

        tableau->setLeavingVariableIndex( 4 );

        tableau->nextPivotRow = &row;
        tableau->mockLeavingVariable = 0;
        tableau->nextEnteringVariable = 4;
        tableau->nextEnteringVariableIndex = 0;

        TS_ASSERT_THROWS_NOTHING( tightener.examinePivotRow( *tableau ) );

        List<Tightening> tightenings;
        TS_ASSERT_THROWS_NOTHING( tightener.getRowTightenings( tightenings ) );

        // Lower and upper bounds should have been tightened
        TS_ASSERT_EQUALS( tightenings.size(), 1U );
        auto lower = *tightenings.begin();

        TS_ASSERT_EQUALS( lower._variable, 0U );
        TS_ASSERT_EQUALS( lower._type, Tightening::LB );

        // Lower: 1 - 10 - 4, Lower: 1 - 5 + 6
        TS_ASSERT_EQUALS( lower._value, -13 );
    }

    void test_pivot_row__nothing_tightened()
    {
        RowBoundTightener tightener;

        tableau->setDimensions( 2, 5 );
        tightener.initialize( *tableau );

        tableau->setLowerBound( 0, -112 );
        tableau->setUpperBound( 0, 101 );
        tableau->setLowerBound( 1, 5 );
        tableau->setUpperBound( 1, 10 );
        tableau->setLowerBound( 2, -2 );
        tableau->setUpperBound( 2, 3 );
        tableau->setLowerBound( 3, -5 );
        tableau->setUpperBound( 3, 5 );
        tableau->setLowerBound( 4, -100 );
        tableau->setUpperBound( 4, 100 );

        tightener.reset( *tableau );

        TableauRow row( 3 );
        // 1 - x0 - x1 + 2x2
        row._row[0] = TableauRow::Entry( 0, -1 );
        row._row[1] = TableauRow::Entry( 1, -1 );
        row._row[2] = TableauRow::Entry( 2, 2 );
        row._scalar = 1;
        tableau->nextRow = &row;

        tableau->nextPivotRow = &row;
        tableau->mockLeavingVariable = 0;
        tableau->nextEnteringVariable = 4;
        tableau->nextEnteringVariableIndex = 0;

        TS_ASSERT_THROWS_NOTHING( tightener.examinePivotRow( *tableau ) );

        List<Tightening> tightenings;
        TS_ASSERT_THROWS_NOTHING( tightener.getRowTightenings( tightenings ) );

        TS_ASSERT( tightenings.empty() );
    }
};

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
