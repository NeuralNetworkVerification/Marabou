/*********************                                                        */
/*! \file Test_RowBoundTightener.h
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
#include "FactTracker.h"
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
        RowBoundTightener tightener( *tableau );

        tableau->setDimensions( 2, 5 );

        // Current bounds:
        //  -200 <= x0 <= 200
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

        TableauRow row( 3 );
        // 1 - x0 - x1 + 2x2
        row._row[0] = TableauRow::Entry( 0, -1 );
        row._row[1] = TableauRow::Entry( 1, -1 );
        row._row[2] = TableauRow::Entry( 2, 2 );
        row._scalar = 1;
        row._lhs = 4;

        tableau->nextPivotRow = &row;

        // 1 - x0 - x1 + 2x2 = x4 (pre pivot)
        // x0 entering, x4 leaving
        // x0 = 1 - x1 + 2 x2 - x4

        TS_ASSERT_THROWS_NOTHING( tightener.examinePivotRow() );

        List<Tightening> tightenings;
        TS_ASSERT_THROWS_NOTHING( tightener.getRowTightenings( tightenings ) );

        // Lower and upper bounds should have been tightened
        TS_ASSERT_EQUALS( tightenings.size(), 2U );

        auto lower = tightenings.begin();
        while ( ( lower != tightenings.end() ) && !( ( lower->_variable == 0 ) && ( lower->_type == Tightening::LB ) ) )
            ++lower;
        TS_ASSERT( lower != tightenings.end() );

        auto upper = tightenings.begin();
        while ( ( upper != tightenings.end() ) && !( ( upper->_variable == 0 ) && ( upper->_type == Tightening::UB ) ) )
            ++upper;
        TS_ASSERT( upper != tightenings.end() );

        // LB -> 1 - 10 - 4 -100
        // UB -> 1 - 5 + 6 + 100
        TS_ASSERT_EQUALS( lower->_value, -113 );
        TS_ASSERT_EQUALS( upper->_value, 102 );
    }

    void test_pivot_row__just_upper_tightend()
    {
        RowBoundTightener tightener( *tableau );

        tableau->setDimensions( 2, 5 );

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

        tightener.setDimensions();

        TableauRow row( 3 );
        // 1 - x0 - x1 + 2x2
        row._row[0] = TableauRow::Entry( 0, -1 );
        row._row[1] = TableauRow::Entry( 1, -1 );
        row._row[2] = TableauRow::Entry( 2, 2 );
        row._scalar = 1;
        row._lhs = 4;

        tableau->nextPivotRow = &row;

        TS_ASSERT_THROWS_NOTHING( tightener.examinePivotRow() );

        List<Tightening> tightenings;
        TS_ASSERT_THROWS_NOTHING( tightener.getRowTightenings( tightenings ) );

        auto upper = tightenings.begin();
        while ( ( upper != tightenings.end() ) && !( ( upper->_variable == 0 ) && ( upper->_type == Tightening::UB ) ) )
            ++upper;
        TS_ASSERT( upper != tightenings.end() );

        TS_ASSERT_EQUALS( upper->_variable, 0U );
        TS_ASSERT_EQUALS( upper->_type, Tightening::UB );

        // Lower: 1 - 10 - 4, Upper: 1 - 5 + 6
        TS_ASSERT_EQUALS( upper->_value, 2 );
    }

    void test_pivot_row__just_lower_tightend()
    {
        RowBoundTightener tightener( *tableau );

        tableau->setDimensions( 2, 5 );

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

        tightener.setDimensions();

        TableauRow row( 3 );
        // 1 - x0 - x1 + 2x2
        row._row[0] = TableauRow::Entry( 0, -1 );
        row._row[1] = TableauRow::Entry( 1, -1 );
        row._row[2] = TableauRow::Entry( 2, 2 );
        row._scalar = 1;
        row._lhs = 4;

        tableau->nextPivotRow = &row;

        TS_ASSERT_THROWS_NOTHING( tightener.examinePivotRow() );

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
        RowBoundTightener tightener( *tableau );

        tableau->setDimensions( 2, 5 );

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

        tightener.setDimensions();

        TableauRow row( 3 );
        // 1 - x0 - x1 + 2x2
        row._row[0] = TableauRow::Entry( 0, -1 );
        row._row[1] = TableauRow::Entry( 1, -1 );
        row._row[2] = TableauRow::Entry( 2, 2 );
        row._scalar = 1;
        row._lhs = 4;

        tableau->nextPivotRow = &row;

        TS_ASSERT_THROWS_NOTHING( tightener.examinePivotRow() );

        List<Tightening> tightenings;
        TS_ASSERT_THROWS_NOTHING( tightener.getRowTightenings( tightenings ) );

        TS_ASSERT( tightenings.empty() );
    }

    void test_examine_constraint_matrix_single_equation()
    {
        RowBoundTightener tightener( *tableau );

        tableau->setDimensions( 1, 5 );

        tableau->setLowerBound( 0, 0 );
        tableau->setUpperBound( 0, 3 );
        tableau->setLowerBound( 1, -1 );
        tableau->setUpperBound( 1, 2 );
        tableau->setLowerBound( 2, 4 );
        tableau->setUpperBound( 2, 5 );
        tableau->setLowerBound( 3, 0 );
        tableau->setUpperBound( 3, 1 );
        tableau->setLowerBound( 4, 2 );
        tableau->setUpperBound( 4, 2 );

        /*
           A = | 1 -2 0  1 2 | , b = | 1  |

           Equation:
                x0 -2x1     +x3  +2x4 = 1

           Ranges:
                x0: [0, 3]
                x1: [-1, 2]
                x2: [4, 5]
                x3: [0, 1]
                x4: [2, 2]

           The equation gives us that x0 <= 1
                                      x1 >= 1.5
        */

        tightener.setDimensions();

        double A[] = { 1, -2, 0, 1, 2 };
        double b[] = { 1 };

        tableau->A = A;
        tableau->b = b;

        TS_ASSERT_THROWS_NOTHING( tightener.examineConstraintMatrix( false ) );

        List<Tightening> tightenings;
        TS_ASSERT_THROWS_NOTHING( tightener.getRowTightenings( tightenings ) );
        TS_ASSERT_EQUALS( tightenings.size(), 2U );

        auto it = tightenings.begin();

        TS_ASSERT_EQUALS( it->_variable, 0U );
        TS_ASSERT( FloatUtils::areEqual( it->_value, 1.0 ) );
        TS_ASSERT_EQUALS( it->_type, Tightening::UB );

        ++it;

        TS_ASSERT_EQUALS( it->_variable, 1U );
        TS_ASSERT( FloatUtils::areEqual( it->_value, 1.5 ) );
        TS_ASSERT_EQUALS( it->_type, Tightening::LB );
    }

    void test_examine_constraint_matrix_multiple_equations()
    {
        RowBoundTightener tightener( *tableau );

        tableau->setDimensions( 2, 5 );

        tableau->setLowerBound( 0, 0 );
        tableau->setUpperBound( 0, 3 );
        tableau->setLowerBound( 1, -1 );
        tableau->setUpperBound( 1, 2 );
        tableau->setLowerBound( 2, -10 );
        tableau->setUpperBound( 2, 10 );
        tableau->setLowerBound( 3, 0 );
        tableau->setUpperBound( 3, 1 );
        tableau->setLowerBound( 4, 2 );
        tableau->setUpperBound( 4, 2 );

        /*
               | 1 -2 0  1 2 | ,     | 1  |
           A = | 0 -2 1  0 0 | , b = | -2 |


           Equations:
                x0 -2x1      +x3  +2x4 = 1
                   -2x1 + x2           = -2

           Ranges:
                x0: [0, 3]
                x1: [-1, 2]
                x2: [-10, 10]
                x3: [0, 1]
                x4: [2, 2]

           Equation 1 gives us that x0 <= 1
                                    x1 >= 1.5
           Equation 2 gives us that x2 <= 2, >= 1
        */

        tightener.setDimensions();

        double A[] = {
            1, -2, 0, 1, 2,
            0, -2, 1, 0, 0,
        };

        double b[] = { 1, -2 };

        tableau->A = A;
        tableau->b = b;

        TS_ASSERT_THROWS_NOTHING( tightener.examineConstraintMatrix( false ) );

        List<Tightening> tightenings;
        TS_ASSERT_THROWS_NOTHING( tightener.getRowTightenings( tightenings ) );
        TS_ASSERT_EQUALS( tightenings.size(), 4U );

        auto it = tightenings.begin();

        TS_ASSERT_EQUALS( it->_variable, 0U );
        TS_ASSERT( FloatUtils::areEqual( it->_value, 1.0 ) );
        TS_ASSERT_EQUALS( it->_type, Tightening::UB );

        ++it;

        TS_ASSERT_EQUALS( it->_variable, 1U );
        TS_ASSERT( FloatUtils::areEqual( it->_value, 1.5 ) );
        TS_ASSERT_EQUALS( it->_type, Tightening::LB );

        ++it;

        TS_ASSERT_EQUALS( it->_variable, 2U );
        TS_ASSERT( FloatUtils::areEqual( it->_value, 1 ) );
        TS_ASSERT_EQUALS( it->_type, Tightening::LB );

        ++it;

        TS_ASSERT_EQUALS( it->_variable, 2U );
        TS_ASSERT( FloatUtils::areEqual( it->_value, 2 ) );
        TS_ASSERT_EQUALS( it->_type, Tightening::UB );
    }

    void test_examine_inverted_row_bounds_tightened_explanation()
    {
        FactTracker* factTracker = new FactTracker();

        RowBoundTightener tightener( *tableau );

        tightener.setFactTracker( factTracker );

        tableau->setDimensions( 2, 5 );

        // Add external facts for x2 >= -2, x2 <= 3, x4 >= -100, x4 <= 100
        Tightening x2_lb( 2, -2, Tightening::LB );
        Tightening x2_ub( 2, 3, Tightening::UB );
        Tightening x4_lb( 4, -100, Tightening::LB );
        Tightening x4_ub( 4, 100, Tightening::UB );

        factTracker->addBoundFact( 2, x2_lb );
        factTracker->addBoundFact( 2, x2_ub );
        factTracker->addBoundFact( 4, x4_lb );
        factTracker->addBoundFact( 4, x4_ub );

        // Current bounds:
        //  -200 <= x0 <= 200
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

        TableauRow row( 3 );
        // 1 - x0 - x1 + 2x2
        row._row[0] = TableauRow::Entry( 0, -1 );
        row._row[1] = TableauRow::Entry( 1, -1 );
        row._row[2] = TableauRow::Entry( 2, 2 );
        row._scalar = 1;
        row._lhs = 4;

        tableau->nextPivotRow = &row;

        // 1 - x0 - x1 + 2x2 = x4 (pre pivot)
        // x0 entering, x4 leaving
        // x0 = 1 - x1 + 2 x2 - x4

        TS_ASSERT_THROWS_NOTHING( tightener.examinePivotRow() );

        List<Tightening> tightenings;
        TS_ASSERT_THROWS_NOTHING( tightener.getRowTightenings( tightenings ) );

        // Lower and upper bounds should have been tightened
        TS_ASSERT_EQUALS( tightenings.size(), 2U );

        auto lower = tightenings.begin();
        while ( ( lower != tightenings.end() ) && !( ( lower->_variable == 0 ) && ( lower->_type == Tightening::LB ) ) )
            ++lower;
        TS_ASSERT( lower != tightenings.end() );

        auto upper = tightenings.begin();
        while ( ( upper != tightenings.end() ) && !( ( upper->_variable == 0 ) && ( upper->_type == Tightening::UB ) ) )
            ++upper;
        TS_ASSERT( upper != tightenings.end() );

        // LB -> 1 - 10 - 4 -100
        // UB -> 1 - 5 + 6 + 100
        TS_ASSERT_EQUALS( lower->_value, -113 );
        TS_ASSERT_EQUALS( upper->_value, 102 );

        // LB should have explanations 1, 4
        List<unsigned> lowerBoundExplanations = lower->getExplanations();
        List<bool> lowerBoundExplanationIsInternal = lower->getExplanationIsInternal();
        Set<unsigned> lowerBoundExplanationSet;

        for ( bool explanationIsInternal : lowerBoundExplanationIsInternal )
        {
            TS_ASSERT_EQUALS( explanationIsInternal, false );
        }

        for ( unsigned explanation : lowerBoundExplanations )
        {
            lowerBoundExplanationSet.insert( explanation );
        }

        TS_ASSERT_EQUALS( lowerBoundExplanationSet.size(), 2U );
        TS_ASSERT( lowerBoundExplanationSet.exists( 1U ) && lowerBoundExplanationSet.exists( 4U ) );

        // UB should have explanations 2, 3 
        List<unsigned> upperBoundExplanations = upper->getExplanations();
        List<bool> upperBoundExplanationIsInternal = upper->getExplanationIsInternal();
        Set<unsigned> upperBoundExplanationSet;

        for ( bool explanationIsInternal : upperBoundExplanationIsInternal )
        {
            TS_ASSERT_EQUALS( explanationIsInternal, false );
        }

        for ( unsigned explanation: upperBoundExplanations )
        {
            upperBoundExplanationSet.insert( explanation );
        }

        TS_ASSERT_EQUALS( upperBoundExplanationSet.size(), 2U );
        TS_ASSERT( upperBoundExplanationSet.exists( 2U ) && upperBoundExplanationSet.exists( 3U ) );

        delete factTracker;
        factTracker = NULL;
    }    
};

//
// Local Variables:
// compile-command: "make -C ../../.. "
// tags-file-name: "../../../TAGS"
// c-basic-offset: 4
// End:
//
