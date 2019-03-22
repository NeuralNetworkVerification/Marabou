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
#include "Tableau.h"

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
        tightener.nullifyInternalFactTracker();
        
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
        tightener.nullifyInternalFactTracker();
        
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
        tightener.nullifyInternalFactTracker();
        
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
        tightener.nullifyInternalFactTracker();
        
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
        Tableau *tableau = new Tableau;
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
        tightener.nullifyInternalFactTracker();
        
        double A[] = { 1, -2, 0, 1, 2 };
        double b[] = { 1 };

        // Remember to initialize sparse rows, because now RBT uses these sparse rows.
        // But this is not implemented in mock yet.
        tableau->setConstraintMatrix( A );
        tableau->setRightHandSide( b );
        
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

        delete tableau;
    }

    void test_examine_constraint_matrix_multiple_equations()
    {
        Tableau *tableau = new Tableau;
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
        tightener.nullifyInternalFactTracker();
        
        double A[] = {
            1, -2, 0, 1, 2,
            0, -2, 1, 0, 0,
        };
        
        double b[] = { 1, -2 };
        
        // Remember to initialize sparse rows, because now RBT uses these sparse rows.
        // But this is not implemented in mock yet.
        tableau->setConstraintMatrix( A );
        tableau->setRightHandSide( b );
        
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

        delete tableau;
    }

    void test_examine_inverted_row_bounds_tightened_explanation()
    {
        FactTracker* factTracker = new FactTracker();

        RowBoundTightener tightener( *tableau );

        tightener.setFactTracker( factTracker );

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

        factTracker->initializeFromTableau( *tableau );

        const Fact* fact_x1_lb = factTracker->getFactAffectingBound( 1, FactTracker::LB );
        const Fact* fact_x1_ub = factTracker->getFactAffectingBound( 1, FactTracker::UB );
        const Fact* fact_x2_lb = factTracker->getFactAffectingBound( 2, FactTracker::LB );
        const Fact* fact_x2_ub = factTracker->getFactAffectingBound( 2, FactTracker::UB );
        const Fact* fact_x4_lb = factTracker->getFactAffectingBound( 4, FactTracker::LB );
        const Fact* fact_x4_ub = factTracker->getFactAffectingBound( 4, FactTracker::UB );

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

        List<const Fact*> lowerBoundExplanations = lower->getExplanations();
        Set<const Fact*> lowerBoundExplanationSet;

        for ( const Fact* explanation : lowerBoundExplanations )
        {
            TS_ASSERT( factTracker->hasFact(explanation) );
        }

        for ( const Fact* explanation : lowerBoundExplanations )
        {
            lowerBoundExplanationSet.insert( explanation );
        }

        TS_ASSERT_EQUALS( lowerBoundExplanationSet.size(), 3U );
        TS_ASSERT( lowerBoundExplanationSet.exists( fact_x2_lb ) && lowerBoundExplanationSet.exists( fact_x4_ub )
            && lowerBoundExplanationSet.exists( fact_x1_ub ) );

        List<const Fact*> upperBoundExplanations = upper->getExplanations();
        Set<const Fact*> upperBoundExplanationSet;

        for ( const Fact* explanation : upperBoundExplanations )
        {
            TS_ASSERT( factTracker->hasFact(explanation) );
        }

        for ( const Fact* explanation: upperBoundExplanations )
        {
            upperBoundExplanationSet.insert( explanation );
        }

        TS_ASSERT_EQUALS( upperBoundExplanationSet.size(), 3U );
        TS_ASSERT( upperBoundExplanationSet.exists( fact_x2_ub ) && upperBoundExplanationSet.exists( fact_x4_lb )
            && upperBoundExplanationSet.exists( fact_x1_lb ) );

        delete factTracker;
    }

    void test_examine_constraint_matrix_row_bound_tightened_explanation()
    {
        Tableau* tableau = new Tableau;
        RowBoundTightener tightener( *tableau );

        tableau->setDimensions( 2, 5 );

        FactTracker* factTracker = new FactTracker();

        tightener.setFactTracker( factTracker );

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

        factTracker->initializeFromTableau( *tableau );

        const Fact* fact_x1_ub = factTracker->getFactAffectingBound( 1, FactTracker::UB );
        const Fact* fact_x4_lb = factTracker->getFactAffectingBound( 4, FactTracker::LB );
        const Fact* fact_x3_lb = factTracker->getFactAffectingBound( 3, FactTracker::LB );
        const Fact* fact_x0_lb = factTracker->getFactAffectingBound( 0, FactTracker::LB );

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

        // Remember to initialize sparse rows, because now RBT uses these sparse rows.
        // But this is not implemented in mock yet.
        tableau->setConstraintMatrix( A );
        tableau->setRightHandSide( b );

        Equation fakeEq1, fakeEq2;
        fakeEq1.setCausingSplitInfo(0, 0, 0);
        fakeEq2.setCausingSplitInfo(0, 0, 0);
        factTracker->addEquationFact( 0, fakeEq1 );
        factTracker->addEquationFact( 1, fakeEq2 );
        const Fact* fact_eq1 = factTracker->getFactAffectingEquation( 0 );
        const Fact* fact_eq2 = factTracker->getFactAffectingEquation( 1 ); 
        TS_ASSERT( fact_eq1 != NULL && fact_eq2 != NULL );

        TS_ASSERT_THROWS_NOTHING( tightener.examineConstraintMatrix( false ) );

        List<Tightening> tightenings;
        TS_ASSERT_THROWS_NOTHING( tightener.getRowTightenings( tightenings ) );
        TS_ASSERT_EQUALS( tightenings.size(), 4U );

        auto it = tightenings.begin();

        TS_ASSERT_EQUALS( it->_variable, 0U );
        TS_ASSERT( FloatUtils::areEqual( it->_value, 1.0 ) );
        TS_ASSERT_EQUALS( it->_type, Tightening::UB );

        List<const Fact*> explanations = it->getExplanations();
        Set<const Fact*> explanationSet;
        for ( List<const Fact*>::iterator explanation_iter = explanations.begin(); explanation_iter != explanations.end(); ++explanation_iter )
            explanationSet.insert( *explanation_iter );

        TS_ASSERT_EQUALS( explanations.size(), 4U );
        TS_ASSERT( explanationSet.exists( fact_x4_lb ) && explanationSet.exists( fact_x1_ub )
            && explanationSet.exists( fact_x3_lb ) && explanationSet.exists( fact_eq1 ) );
        
        ++it;

        TS_ASSERT_EQUALS( it->_variable, 1U );
        TS_ASSERT( FloatUtils::areEqual( it->_value, 1.5 ) );
        TS_ASSERT_EQUALS( it->_type, Tightening::LB );

        explanations = it->getExplanations();
        explanationSet.clear();
        for ( List<const Fact*>::iterator explanation_iter = explanations.begin(); explanation_iter != explanations.end(); ++explanation_iter )
            explanationSet.insert( *explanation_iter );

        TS_ASSERT_EQUALS( explanations.size(), 4U );
        TS_ASSERT( explanationSet.exists( fact_x0_lb ) && explanationSet.exists( fact_x3_lb )
            && explanationSet.exists( fact_x4_lb ) && explanationSet.exists( fact_eq1 ) );

        ++it;

        TS_ASSERT_EQUALS( it->_variable, 2U );
        TS_ASSERT( FloatUtils::areEqual( it->_value, 1 ) );
        TS_ASSERT_EQUALS( it->_type, Tightening::LB );

        explanations = it->getExplanations();
        explanationSet.clear();
        for ( List<const Fact*>::iterator explanation_iter = explanations.begin(); explanation_iter != explanations.end(); ++explanation_iter )
            explanationSet.insert( *explanation_iter );

        TS_ASSERT_EQUALS( explanations.size(), 5U );
        TS_ASSERT( explanationSet.exists( fact_x0_lb ) && explanationSet.exists( fact_x3_lb )
            && explanationSet.exists( fact_x4_lb ) && explanationSet.exists( fact_eq1 ) && explanationSet.exists( fact_eq2 ) );      

        ++it;

        TS_ASSERT_EQUALS( it->_variable, 2U );
        TS_ASSERT( FloatUtils::areEqual( it->_value, 2 ) );
        TS_ASSERT_EQUALS( it->_type, Tightening::UB );

        explanations = it->getExplanations();
        explanationSet.clear();
        for ( List<const Fact*>::iterator explanation_iter = explanations.begin(); explanation_iter != explanations.end(); ++explanation_iter )
            explanationSet.insert( *explanation_iter );

        TS_ASSERT_EQUALS( explanations.size(), 2U );
        TS_ASSERT( explanationSet.exists( fact_x1_ub ) && explanationSet.exists( fact_eq2 ) );

        delete factTracker;
        delete tableau;
    }

};

//
// Local Variables:
// compile-command: "make -C ../../.. "
// tags-file-name: "../../../TAGS"
// c-basic-offset: 4
// End:
//