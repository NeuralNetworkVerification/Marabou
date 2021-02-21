/*********************                                                        */
/*! \file Test_Tableau.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz, Parth Shah, Duligur Ibeling
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

#include "Equation.h"
#include "MockCostFunctionManager.h"
#include "MockErrno.h"
#include "MarabouError.h"
#include "Tableau.h"
#include "TableauRow.h"
#include "TableauState.h"

#include <string.h>

class MockForTableau
{
public:
};

class MockVariableWatcher : public ITableau::VariableWatcher
{
public:
    Map<unsigned, double> lastNotifiedValues;
    void notifyVariableValue( unsigned variable, double value )
    {
        lastNotifiedValues[variable] = value;
    }

    Map<unsigned, double> lastNotifiedLowerBounds;
    void notifyLowerBound( unsigned variable, double bound )
    {
        lastNotifiedLowerBounds[variable] = bound;
    }

    Map<unsigned, double> lastNotifiedUpperBounds;
    void notifyUpperBound( unsigned variable, double bound )
    {
        lastNotifiedUpperBounds[variable] = bound;
    }
};

class TableauTestSuite : public CxxTest::TestSuite
{
public:
    MockForTableau *mock;

    void setUp()
    {
        TS_ASSERT( mock = new MockForTableau );
    }

    void tearDown()
    {
        TS_ASSERT_THROWS_NOTHING( delete mock );
    }

    bool hasCandidates( const ITableau &tableau ) const
    {
        List<unsigned> candidates;
        tableau.getEntryCandidates( candidates );
        return !candidates.empty();
    }

    void initializeTableauValues( Tableau &tableau )
    {
        /*
               | 3 2 1 2 1 0 0 | | x1 |   | 225 |
          Ax = | 1 1 1 1 0 1 0 | | x2 | = | 117 | = b
               | 4 3 3 4 0 0 1 | | x3 |   | 420 |
                                 | x4 |
                                 | x5 |
                                 | x6 |
                                 | x7 |

           x5 = 225 - 3x1 - 2x2 - x3  - 2x4
           x6 = 117 -  x1 -  x2 - x3  -  x4
           x7 = 420 - 4x1 - 3x2 - 3x3 - 4x4

        */

        double A[] = {
            3, 2, 1, 2, 1, 0, 0,
            1, 1, 1, 1, 0, 1, 0,
            4, 3, 3, 4, 0, 0, 1,
        };

        tableau.setConstraintMatrix( A );

        tableau.assignIndexToBasicVariable( 4, 0 );
        tableau.assignIndexToBasicVariable( 5, 1 );
        tableau.assignIndexToBasicVariable( 6, 2 );

        double b[3] = { 225, 117, 420 };
        tableau.setRightHandSide( b );
    }

    void test_initialize_bounds()
    {
        Tableau *tableau = NULL;

        TS_ASSERT( tableau = new Tableau );

        TS_ASSERT_THROWS_NOTHING( tableau->setDimensions( 3, 7, 3, 7 ) );
        initializeTableauValues( *tableau );

        for ( unsigned i = 0; i < tableau->getN(); ++i )
        {
            TS_ASSERT_EQUALS( tableau->getLowerBound( i ), FloatUtils::negativeInfinity() );
            TS_ASSERT_EQUALS( tableau->getUpperBound( i ), FloatUtils::infinity() );
        }

        TS_ASSERT_THROWS_NOTHING( delete tableau );
    }

    void test_initalize_basis_get_value()
    {
        Tableau *tableau = NULL;

        TS_ASSERT( tableau = new Tableau );

        TS_ASSERT_THROWS_NOTHING( tableau->setDimensions( 3, 7, 3, 7 ) );
        initializeTableauValues( *tableau );

        for ( unsigned i = 0; i < 4; ++i )
        {
            TS_ASSERT_THROWS_NOTHING( tableau->setLowerBound( i, 1 ) );
            TS_ASSERT_THROWS_NOTHING( tableau->setUpperBound( i, 2 ) );
        }

        TS_ASSERT_THROWS_NOTHING( tableau->setLowerBound( 4, 218 ) );
        TS_ASSERT_THROWS_NOTHING( tableau->setUpperBound( 4, 228 ) );

        TS_ASSERT_THROWS_NOTHING( tableau->setLowerBound( 5, 112 ) );
        TS_ASSERT_THROWS_NOTHING( tableau->setUpperBound( 5, 114 ) );

        TS_ASSERT_THROWS_NOTHING( tableau->setLowerBound( 6, 400 ) );
        TS_ASSERT_THROWS_NOTHING( tableau->setUpperBound( 6, 402 ) );

        List<unsigned> basics = { 4, 5, 6 };
        TS_ASSERT_THROWS_NOTHING( tableau->initializeTableau( basics ) );

        // All non-basics are set to lower bounds.
        TS_ASSERT_EQUALS( tableau->getValue( 0 ), 1.0 );
        TS_ASSERT_EQUALS( tableau->getValue( 1 ), 1.0 );
        TS_ASSERT_EQUALS( tableau->getValue( 2 ), 1.0 );
        TS_ASSERT_EQUALS( tableau->getValue( 3 ), 1.0 );

        // Expect:
        // x5 = 225 - 3x1 - 2x2 - x3  - 2x4 = 225 - 8  = 217
        // x6 = 117 -  x1 -  x2 - x3  -  x4 = 117 - 4  = 113
        // x7 = 420 - 4x1 - 3x2 - 3x3 - 4x4 = 420 - 14 = 406

        TS_ASSERT_EQUALS( tableau->getValue( 4 ), 217.0 );
        TS_ASSERT_EQUALS( tableau->getValue( 5 ), 113.0 );
        TS_ASSERT_EQUALS( tableau->getValue( 6 ), 406.0 );

        TS_ASSERT_THROWS_NOTHING( delete tableau );
    }

    void test_watcher__value_changes()
    {
        Tableau *tableau = NULL;

        TS_ASSERT( tableau = new Tableau );

        TS_ASSERT_THROWS_NOTHING( tableau->setDimensions( 3, 7, 3, 7 ) );
        initializeTableauValues( *tableau );

        for ( unsigned i = 0; i < 4; ++i )
        {
            TS_ASSERT_THROWS_NOTHING( tableau->setLowerBound( i, 1 ) );
            TS_ASSERT_THROWS_NOTHING( tableau->setUpperBound( i, 2 ) );
        }

        TS_ASSERT_THROWS_NOTHING( tableau->setLowerBound( 4, 218 ) );
        TS_ASSERT_THROWS_NOTHING( tableau->setUpperBound( 4, 228 ) );

        TS_ASSERT_THROWS_NOTHING( tableau->setLowerBound( 5, 112 ) );
        TS_ASSERT_THROWS_NOTHING( tableau->setUpperBound( 5, 114 ) );

        TS_ASSERT_THROWS_NOTHING( tableau->setLowerBound( 6, 400 ) );
        TS_ASSERT_THROWS_NOTHING( tableau->setUpperBound( 6, 402 ) );

        MockVariableWatcher watcher1;
        MockVariableWatcher watcher2;

        TS_ASSERT_THROWS_NOTHING( tableau->registerToWatchVariable( &watcher1, 4 ) );
        TS_ASSERT_THROWS_NOTHING( tableau->registerToWatchVariable( &watcher1, 5 ) );
        TS_ASSERT_THROWS_NOTHING( tableau->registerToWatchVariable( &watcher2, 5 ) );

        List<unsigned> basics = { 4, 5, 6 };
        TS_ASSERT_THROWS_NOTHING( tableau->initializeTableau( basics ) );

        // The basic values get computed, so the watchers should be called

        TS_ASSERT_EQUALS( watcher1.lastNotifiedValues[4], 217.0 );
        TS_ASSERT_EQUALS( watcher1.lastNotifiedValues[5], 113.0 );
        TS_ASSERT_EQUALS( watcher2.lastNotifiedValues[5], 113.0 );

        MockVariableWatcher watcher3;
        TS_ASSERT_THROWS_NOTHING( tableau->registerToWatchVariable( &watcher3, 3 ) );

        TS_ASSERT_THROWS_NOTHING( tableau->setNonBasicAssignment( 3, 2, false ) );

        TS_ASSERT_EQUALS( watcher3.lastNotifiedValues[3], 2.0 );

        watcher3.lastNotifiedValues.clear();
        TS_ASSERT_THROWS_NOTHING( tableau->unregisterToWatchVariable( &watcher3, 3 ) );
        TS_ASSERT_THROWS_NOTHING( tableau->setNonBasicAssignment( 3, 1, false ) );
        //TS_ASSERT_EQUALS( watcher3.lastNotifiedValues[3], 1.0 );
        TS_ASSERT( watcher3.lastNotifiedValues.empty() );

        TS_ASSERT_THROWS_NOTHING( delete tableau );
    }

    void test_get_entering_variable__have_eligible_variables()
    {
        Tableau *tableau = NULL;
        MockCostFunctionManager costFunctionManager;

        TS_ASSERT( tableau = new Tableau );

        TS_ASSERT_THROWS_NOTHING( tableau->setDimensions( 3, 7, 3, 7 ) );
        tableau->registerCostFunctionManager( &costFunctionManager );
        initializeTableauValues( *tableau );

        for ( unsigned i = 0; i < 4; ++i )
        {
            TS_ASSERT_THROWS_NOTHING( tableau->setLowerBound( i, 1 ) );
            TS_ASSERT_THROWS_NOTHING( tableau->setUpperBound( i, 2 ) );
        }

        TS_ASSERT_THROWS_NOTHING( tableau->setLowerBound( 4, 218 ) );
        TS_ASSERT_THROWS_NOTHING( tableau->setUpperBound( 4, 228 ) );

        TS_ASSERT_THROWS_NOTHING( tableau->setLowerBound( 5, 112 ) );
        TS_ASSERT_THROWS_NOTHING( tableau->setUpperBound( 5, 114 ) );

        TS_ASSERT_THROWS_NOTHING( tableau->setLowerBound( 6, 400 ) );
        TS_ASSERT_THROWS_NOTHING( tableau->setUpperBound( 6, 402 ) );

        List<unsigned> basics = { 4, 5, 6 };
        TS_ASSERT_THROWS_NOTHING( tableau->initializeTableau( basics ) );

        TS_ASSERT_EQUALS( tableau->getBasicStatus( 4 ), Tableau::BELOW_LB );
        TS_ASSERT_EQUALS( tableau->getBasicStatus( 5 ), Tableau::BETWEEN );
        TS_ASSERT_EQUALS( tableau->getBasicStatus( 6 ), Tableau::ABOVE_UB );

        TS_ASSERT_THROWS_NOTHING( tableau->computeCostFunction() );
        TS_ASSERT( costFunctionManager.computeCoreCostFunctionCalled );

        // Cost function is: - x1 -  x2 - 2x3 - 2x4
        costFunctionManager.nextCostFunction = new double[4];
        costFunctionManager.nextCostFunction[0] = -1;
        costFunctionManager.nextCostFunction[1] = -1;
        costFunctionManager.nextCostFunction[2] = -2;
        costFunctionManager.nextCostFunction[3] = -2;

        tableau->setEnteringVariableIndex( 2u );

        TS_ASSERT( hasCandidates( *tableau ) );

        TS_ASSERT_EQUALS( tableau->getEnteringVariable(), 2u );

        TS_ASSERT_THROWS_NOTHING( delete tableau );
    }

    void test_get_entering_variable__no_eligible_variables()
    {
        Tableau *tableau = NULL;
        MockCostFunctionManager costFunctionManager;

        TS_ASSERT( tableau = new Tableau );

        TS_ASSERT_THROWS_NOTHING( tableau->setDimensions( 3, 7, 3, 7 ) );
        tableau->registerCostFunctionManager( &costFunctionManager );
        initializeTableauValues( *tableau );

        for ( unsigned i = 0; i < 4; ++i )
        {
            TS_ASSERT_THROWS_NOTHING( tableau->setLowerBound( i, 1 ) );
            TS_ASSERT_THROWS_NOTHING( tableau->setUpperBound( i, 2 ) );
        }

        TS_ASSERT_THROWS_NOTHING( tableau->setLowerBound( 4, 216 ) );
        TS_ASSERT_THROWS_NOTHING( tableau->setUpperBound( 4, 228 ) );

        TS_ASSERT_THROWS_NOTHING( tableau->setLowerBound( 5, 120 ) );
        TS_ASSERT_THROWS_NOTHING( tableau->setUpperBound( 5, 122 ) );

        TS_ASSERT_THROWS_NOTHING( tableau->setLowerBound( 6, 400 ) );
        TS_ASSERT_THROWS_NOTHING( tableau->setUpperBound( 6, 412 ) );

        List<unsigned> basics = { 4, 5, 6 };
        TS_ASSERT_THROWS_NOTHING( tableau->initializeTableau( basics ) );

        TS_ASSERT_EQUALS( tableau->getBasicStatus( 4 ), Tableau::BETWEEN );
        TS_ASSERT_EQUALS( tableau->getBasicStatus( 5 ), Tableau::BELOW_LB );
        TS_ASSERT_EQUALS( tableau->getBasicStatus( 6 ), Tableau::BETWEEN );

        TS_ASSERT_THROWS_NOTHING( tableau->computeCostFunction() );

        // Cost function is: + x1 + x2 + x3 + x4
        // All these variables are at their lower bounds, so cannot decrease.
        costFunctionManager.nextCostFunction = new double[4];
        costFunctionManager.nextCostFunction[0] = 1;
        costFunctionManager.nextCostFunction[1] = 1;
        costFunctionManager.nextCostFunction[2] = 1;
        costFunctionManager.nextCostFunction[3] = 1;

        tableau->setEnteringVariableIndex( 2u );

        TS_ASSERT( !hasCandidates( *tableau ) );

        TS_ASSERT_EQUALS( tableau->getEnteringVariable(), 2u );

        TS_ASSERT_THROWS_NOTHING( delete tableau );
    }

    void test_get_get_leaving_variable()
    {
        Tableau *tableau = NULL;
        MockCostFunctionManager costFunctionManager;

        TS_ASSERT( tableau = new Tableau );

        TS_ASSERT_THROWS_NOTHING( tableau->setDimensions( 3, 7, 3, 7 ) );
        tableau->registerCostFunctionManager( &costFunctionManager );
        initializeTableauValues( *tableau );

        for ( unsigned i = 0; i < 4; ++i )
        {
            TS_ASSERT_THROWS_NOTHING( tableau->setLowerBound( i, 1 ) );
            TS_ASSERT_THROWS_NOTHING( tableau->setUpperBound( i, 10 ) );
        }

        TS_ASSERT_THROWS_NOTHING( tableau->setLowerBound( 4, 219 ) );
        TS_ASSERT_THROWS_NOTHING( tableau->setUpperBound( 4, 228 ) );

        TS_ASSERT_THROWS_NOTHING( tableau->setLowerBound( 5, 112 ) );
        TS_ASSERT_THROWS_NOTHING( tableau->setUpperBound( 5, 114 ) );

        TS_ASSERT_THROWS_NOTHING( tableau->setLowerBound( 6, 400 ) );
        TS_ASSERT_THROWS_NOTHING( tableau->setUpperBound( 6, 402 ) );

        List<unsigned> basics = { 4, 5, 6 };
        TS_ASSERT_THROWS_NOTHING( tableau->initializeTableau( basics ) );

        TS_ASSERT_EQUALS( tableau->getBasicStatus( 4 ), Tableau::BELOW_LB );
        TS_ASSERT_EQUALS( tableau->getBasicStatus( 5 ), Tableau::BETWEEN );
        TS_ASSERT_EQUALS( tableau->getBasicStatus( 6 ), Tableau::ABOVE_UB );

        TS_ASSERT_THROWS_NOTHING( tableau->computeCostFunction() );

        costFunctionManager.nextCostFunction = new double[4];
        costFunctionManager.nextCostFunction[0] = -1;
        costFunctionManager.nextCostFunction[1] = -1;
        costFunctionManager.nextCostFunction[2] = -1;
        costFunctionManager.nextCostFunction[3] = -1;

        costFunctionManager.nextBasicCost[0] = -1;
        costFunctionManager.nextBasicCost[1] =  0;
        costFunctionManager.nextBasicCost[2] = +1;

        tableau->setEnteringVariableIndex( 2u );
        TS_ASSERT( hasCandidates( *tableau ) );
        TS_ASSERT_EQUALS( tableau->getEnteringVariable(), 2u );

        // Entering variable is 2, and it needs to increase
        // Current basic values are: 217, 113, 406
        TS_ASSERT_EQUALS( tableau->getValue( 4 ), 217.0 );
        TS_ASSERT_EQUALS( tableau->getValue( 5 ), 113.0 );
        TS_ASSERT_EQUALS( tableau->getValue( 6 ), 406.0 );

        double d1[] = { -1, -1, -1 };
        // Var 4 will hit its lower bound: constraint is 2
        // Var 5 will hit its upper bound: constraint is 1
        // Var 6 poses no constraint
        TS_ASSERT_THROWS_NOTHING( tableau->pickLeavingVariable( d1 ) );
        TS_ASSERT_EQUALS( tableau->getLeavingVariable(), 5u );
        TS_ASSERT_EQUALS( tableau->getChangeRatio(), 1.0 );

        double d2[] = { -0.5, -0.5, -0.5 };
        // d1 scaled by 1/2
        TS_ASSERT_THROWS_NOTHING( tableau->pickLeavingVariable( d2 ) );
        TS_ASSERT_EQUALS( tableau->getLeavingVariable(), 5u );
        TS_ASSERT_EQUALS( tableau->getChangeRatio(), 2.0 );

        double d3[] = { 1, 1, 1 };
        // Var 4 poses no constraint
        // Var 5 will hit its lower bound: constraint is 1
        // Var 6 will hit its upper bound: constraint is 4
        TS_ASSERT_THROWS_NOTHING( tableau->pickLeavingVariable( d3 ) );
        TS_ASSERT_EQUALS( tableau->getLeavingVariable(), 5u );
        TS_ASSERT_EQUALS( tableau->getChangeRatio(), 1.0 );

        double d4[] = { 1, 0.1, 2 };
        // Var 4 poses no constraint
        // Var 5 will hit its lower bound: constraint is 10
        // Var 6 will hit its upper bound: constraint is 2
        TS_ASSERT_THROWS_NOTHING( tableau->pickLeavingVariable( d4 ) );
        TS_ASSERT_EQUALS( tableau->getLeavingVariable(), 6u );
        TS_ASSERT_EQUALS( tableau->getChangeRatio(), 2.0 );

        double d5[] = { 1, 0, 0.5 };
        // Var 4 poses no constraint
        // Var 5 poses no constraint
        // Var 6 will hit its upper bound: constraint is 8
        TS_ASSERT_THROWS_NOTHING( tableau->pickLeavingVariable( d5 ) );
        TS_ASSERT_EQUALS( tableau->getLeavingVariable(), 6u );
        TS_ASSERT_EQUALS( tableau->getChangeRatio(), 8.0 );

        double d6[] = { -0.5, 0, 0.1 };
        // Var 4 will hit its lower bound: constraint is 4
        // Var 5 poses no constraint
        // Var 6 will hit its upper bound: constraint is 40
        TS_ASSERT_THROWS_NOTHING( tableau->pickLeavingVariable( d6 ) );
        TS_ASSERT_EQUALS( tableau->getLeavingVariable(), 4u );
        TS_ASSERT_EQUALS( tableau->getChangeRatio(), 4.0 );

        double d7[] = { 1, 0, 0.00001 };
        // The entering variable (2) can change by 9 at most. Here
        // this will be the bound.
        TS_ASSERT_THROWS_NOTHING( tableau->pickLeavingVariable( d7 ) );
        TS_ASSERT_EQUALS( tableau->getLeavingVariable(), 2u );
        TS_ASSERT_EQUALS( tableau->getChangeRatio(), 9.0 );

        TS_ASSERT_THROWS_NOTHING( delete tableau );
    }

    void test_perform_pivot_nonbasic_goes_to_opposite_bound()
    {
        Tableau *tableau = NULL;
        MockCostFunctionManager costFunctionManager;

        TS_ASSERT( tableau = new Tableau );

        TS_ASSERT_THROWS_NOTHING( tableau->setDimensions( 3, 7, 3, 7 ) );
        tableau->registerCostFunctionManager( &costFunctionManager );
        initializeTableauValues( *tableau );

        for ( unsigned i = 0; i < 4; ++i )
        {
            TS_ASSERT_THROWS_NOTHING( tableau->setLowerBound( i, 1 ) );
            TS_ASSERT_THROWS_NOTHING( tableau->setUpperBound( i, 10 ) );
        }

        TS_ASSERT_THROWS_NOTHING( tableau->setLowerBound( 4, 219 ) );
        TS_ASSERT_THROWS_NOTHING( tableau->setUpperBound( 4, 228 ) );

        TS_ASSERT_THROWS_NOTHING( tableau->setLowerBound( 5, 112 ) );
        TS_ASSERT_THROWS_NOTHING( tableau->setUpperBound( 5, 114 ) );

        TS_ASSERT_THROWS_NOTHING( tableau->setLowerBound( 6, 400 ) );
        TS_ASSERT_THROWS_NOTHING( tableau->setUpperBound( 6, 402 ) );

        List<unsigned> basics = { 4, 5, 6 };
        TS_ASSERT_THROWS_NOTHING( tableau->initializeTableau( basics ) );

        TS_ASSERT_THROWS_NOTHING( tableau->computeCostFunction() );
        costFunctionManager.nextCostFunction = new double[4];
        costFunctionManager.nextCostFunction[0] = -1;
        costFunctionManager.nextCostFunction[1] = -1;
        costFunctionManager.nextCostFunction[2] = -1;
        costFunctionManager.nextCostFunction[3] = -1;

        costFunctionManager.nextBasicCost[0] = -1;
        costFunctionManager.nextBasicCost[1] =  0;
        costFunctionManager.nextBasicCost[2] = +1;

        tableau->setEnteringVariableIndex( 2u );
        TS_ASSERT( hasCandidates( *tableau ) );
        TS_ASSERT_EQUALS( tableau->getEnteringVariable(), 2u );

        double d[] = { 1, 0, 0.00001 };
        TS_ASSERT_THROWS_NOTHING( tableau->pickLeavingVariable( d ) );
        TS_ASSERT_EQUALS( tableau->getLeavingVariable(), 2u );
        TS_ASSERT_EQUALS( tableau->getEnteringVariable(), 2u );

        TS_ASSERT( !tableau->isBasic( 2u ) );

        // Before the pivot, the variable is at the lower bound
        TS_ASSERT_EQUALS( tableau->getValue( 2u ), 1.0 );
        TS_ASSERT_THROWS_NOTHING( tableau->performPivot() );
        // After the pivot, the variable is at the upper bound
        TS_ASSERT_EQUALS( tableau->getValue( 2u ), 10.0 );

        TS_ASSERT( !tableau->isBasic( 2u ) );

        TS_ASSERT_THROWS_NOTHING( delete tableau );
    }

    void test_perform_pivot_nonbasic_becomes_basic()
    {
        Tableau *tableau = NULL;
        MockCostFunctionManager costFunctionManager;

        TS_ASSERT( tableau = new Tableau );

        TS_ASSERT_THROWS_NOTHING( tableau->setDimensions( 3, 7, 3, 7 ) );
        tableau->registerCostFunctionManager( &costFunctionManager );
        initializeTableauValues( *tableau );

        for ( unsigned i = 0; i < 4; ++i )
        {
            TS_ASSERT_THROWS_NOTHING( tableau->setLowerBound( i, 1 ) );
            TS_ASSERT_THROWS_NOTHING( tableau->setUpperBound( i, 10 ) );
        }

        TS_ASSERT_THROWS_NOTHING( tableau->setLowerBound( 4, 219 ) );
        TS_ASSERT_THROWS_NOTHING( tableau->setUpperBound( 4, 228 ) );

        TS_ASSERT_THROWS_NOTHING( tableau->setLowerBound( 5, 112 ) );
        TS_ASSERT_THROWS_NOTHING( tableau->setUpperBound( 5, 114 ) );

        TS_ASSERT_THROWS_NOTHING( tableau->setLowerBound( 6, 400 ) );
        TS_ASSERT_THROWS_NOTHING( tableau->setUpperBound( 6, 402 ) );

        List<unsigned> basics = { 4, 5, 6 };
        TS_ASSERT_THROWS_NOTHING( tableau->initializeTableau( basics ) );

        TS_ASSERT_THROWS_NOTHING( tableau->computeCostFunction() );
        costFunctionManager.nextCostFunction = new double[4];
        costFunctionManager.nextCostFunction[0] = -1;
        costFunctionManager.nextCostFunction[1] = -1;
        costFunctionManager.nextCostFunction[2] = -1;
        costFunctionManager.nextCostFunction[3] = -1;

        costFunctionManager.nextBasicCost[0] = -1;
        costFunctionManager.nextBasicCost[1] =  0;
        costFunctionManager.nextBasicCost[2] = +1;

        tableau->setEnteringVariableIndex( 2u );
        TS_ASSERT( hasCandidates( *tableau ) );
        TS_ASSERT_EQUALS( tableau->getEnteringVariable(), 2u );

        double d[] = { -1, -1, -1 };
        // Var 4 will hit its lower bound: constraint is 2
        // Var 5 will hit its upper bound: constraint is 1
        // Var 6 poses no constraint
        TS_ASSERT_THROWS_NOTHING( tableau->pickLeavingVariable( d ) );
        TS_ASSERT_EQUALS( tableau->getEnteringVariable(), 2u );
        TS_ASSERT_EQUALS( tableau->getLeavingVariable(), 5u );

        TS_ASSERT( !tableau->isBasic( 2u ) );
        TS_ASSERT( tableau->isBasic( 5u ) );

        TS_ASSERT_EQUALS( tableau->getValue( 2u ), 1.0 );
        TS_ASSERT_EQUALS( tableau->getValue( 5u ), 113.0 );

        TS_ASSERT_THROWS_NOTHING( tableau->computePivotRow() );
        const TableauRow &pivotRow = *tableau->getPivotRow();
        // x3 = 117 -x1 -x2 -x6 -x4
        for ( unsigned i = 0; i < 4; ++i )
            TS_ASSERT( FloatUtils::areEqual( pivotRow[i], -1.0 ) );
        TS_ASSERT( FloatUtils::areEqual( pivotRow._scalar, 117.0 ) );

        TS_ASSERT_THROWS_NOTHING( tableau->computeChangeColumn() );

        TS_ASSERT_THROWS_NOTHING( tableau->performPivot() );

        TS_ASSERT( tableau->isBasic( 2u ) );
        TS_ASSERT( !tableau->isBasic( 5u ) );

        // The new non-basic variable is at its upper bound
        TS_ASSERT_EQUALS( tableau->getValue( 5u ), 114.0 );

        TS_ASSERT_THROWS_NOTHING( delete tableau );
    }

    void test_get_row()
    {
        Tableau *tableau = NULL;
        MockCostFunctionManager costFunctionManager;

        TS_ASSERT( tableau = new Tableau );

        TS_ASSERT_THROWS_NOTHING( tableau->setDimensions( 3, 7, 3, 7 ) );
        tableau->registerCostFunctionManager( &costFunctionManager );
        initializeTableauValues( *tableau );

        for ( unsigned i = 0; i < 4; ++i )
        {
            TS_ASSERT_THROWS_NOTHING( tableau->setLowerBound( i, 1 ) );
            TS_ASSERT_THROWS_NOTHING( tableau->setUpperBound( i, 10 ) );
        }

        TS_ASSERT_THROWS_NOTHING( tableau->setLowerBound( 4, 219 ) );
        TS_ASSERT_THROWS_NOTHING( tableau->setUpperBound( 4, 228 ) );

        TS_ASSERT_THROWS_NOTHING( tableau->setLowerBound( 5, 112 ) );
        TS_ASSERT_THROWS_NOTHING( tableau->setUpperBound( 5, 114 ) );

        TS_ASSERT_THROWS_NOTHING( tableau->setLowerBound( 6, 400 ) );
        TS_ASSERT_THROWS_NOTHING( tableau->setUpperBound( 6, 402 ) );

        List<unsigned> basics = { 4, 5, 6 };
        TS_ASSERT_THROWS_NOTHING( tableau->initializeTableau( basics ) );

        TableauRow row( 4 );

           // x5 = 225 - 3x1 - 2x2 - x3  - 2x4
           // x6 = 117 -  x1 -  x2 - x3  -  x4
           // x7 = 420 - 4x1 - 3x2 - 3x3 - 4x4

        TS_ASSERT_THROWS_NOTHING( tableau->getTableauRow( 0, &row ) );

        TableauRow::Entry entry;
        entry = row._row[0];
        TS_ASSERT_EQUALS( entry._var, 0U );
        TS_ASSERT_EQUALS( entry._coefficient, -3 );

        entry = row._row[1];
        TS_ASSERT_EQUALS( entry._var, 1U );
        TS_ASSERT_EQUALS( entry._coefficient, -2 );

        entry = row._row[2];
        TS_ASSERT_EQUALS( entry._var, 2U );
        TS_ASSERT_EQUALS( entry._coefficient, -1 );

        entry = row._row[3];
        TS_ASSERT_EQUALS( entry._var, 3U );
        TS_ASSERT_EQUALS( entry._coefficient, -2 );

        TS_ASSERT_EQUALS( row._scalar, 225.0 );

        TS_ASSERT_THROWS_NOTHING( tableau->getTableauRow( 1, &row ) );

        entry = row._row[0];
        TS_ASSERT_EQUALS( entry._var, 0U );
        TS_ASSERT_EQUALS( entry._coefficient, -1 );

        entry = row._row[1];
        TS_ASSERT_EQUALS( entry._var, 1U );
        TS_ASSERT_EQUALS( entry._coefficient, -1 );

        entry = row._row[2];
        TS_ASSERT_EQUALS( entry._var, 2U );
        TS_ASSERT_EQUALS( entry._coefficient, -1 );

        entry = row._row[3];
        TS_ASSERT_EQUALS( entry._var, 3U );
        TS_ASSERT_EQUALS( entry._coefficient, -1 );

        TS_ASSERT_EQUALS( row._scalar, 117.0 );

        TS_ASSERT_THROWS_NOTHING( tableau->getTableauRow( 2, &row ) );

        entry = row._row[0];
        TS_ASSERT_EQUALS( entry._var, 0U );
        TS_ASSERT_EQUALS( entry._coefficient, -4 );

        entry = row._row[1];
        TS_ASSERT_EQUALS( entry._var, 1U );
        TS_ASSERT_EQUALS( entry._coefficient, -3 );

        entry = row._row[2];
        TS_ASSERT_EQUALS( entry._var, 2U );
        TS_ASSERT_EQUALS( entry._coefficient, -3 );

        entry = row._row[3];
        TS_ASSERT_EQUALS( entry._var, 3U );
        TS_ASSERT_EQUALS( entry._coefficient, -4 );

        TS_ASSERT_EQUALS( row._scalar, 420.0 );

        TS_ASSERT_THROWS_NOTHING( tableau->computeCostFunction() );
        costFunctionManager.nextCostFunction = new double[4];
        costFunctionManager.nextCostFunction[0] = -1;
        costFunctionManager.nextCostFunction[1] = -1;
        costFunctionManager.nextCostFunction[2] = -1;
        costFunctionManager.nextCostFunction[3] = -1;

        costFunctionManager.nextBasicCost[0] = -1;
        costFunctionManager.nextBasicCost[1] =  0;
        costFunctionManager.nextBasicCost[2] = +1;

        tableau->setEnteringVariableIndex( 2u );
        TS_ASSERT( hasCandidates( *tableau ) );
        TS_ASSERT_EQUALS( tableau->getEnteringVariable(), 2u );

        TS_ASSERT_THROWS_NOTHING( tableau->computeChangeColumn() );

        const double expectedChangeColumn[] = { 1.0, 1.0, 3.0 };
        for ( unsigned i = 0; i < 3; ++i )
            TS_ASSERT( FloatUtils::areEqual( tableau->getChangeColumn()[i], expectedChangeColumn[i] ) );

        TS_ASSERT_THROWS_NOTHING( tableau->pickLeavingVariable() );
        TS_ASSERT_EQUALS( tableau->getEnteringVariable(), 2u );
        TS_ASSERT_EQUALS( tableau->getLeavingVariable(), 5u );

        TS_ASSERT( !tableau->isBasic( 2u ) );
        TS_ASSERT( tableau->isBasic( 5u ) );

        TS_ASSERT_EQUALS( tableau->getValue( 2u ), 1.0 );
        TS_ASSERT_EQUALS( tableau->getValue( 5u ), 113.0 );

        TS_ASSERT_THROWS_NOTHING( tableau->computeChangeColumn() );

        TS_ASSERT_THROWS_NOTHING( tableau->computePivotRow() );

        TS_ASSERT_THROWS_NOTHING( tableau->performPivot() );
        TS_ASSERT( tableau->isBasic( 2u ) );
        TS_ASSERT( !tableau->isBasic( 5u ) );

        // Old equations are:

           // x5 = 225 - 3x1 - 2x2 - x3  - 2x4
           // x6 = 117 -  x1 -  x2 - x3  -  x4
           // x7 = 420 - 4x1 - 3x2 - 3x3 - 4x4

        // New equations are:

           // x5 = 108 - 2x1 -  x2 + x6  -  x4
           // x3 = 117 -  x1 -  x2 - x6  -  x4
           // x7 =  69 -  x1       + 3x6 -  x4

        TS_ASSERT_THROWS_NOTHING( tableau->getTableauRow( 0, &row ) );

        entry = row._row[0];
        TS_ASSERT_EQUALS( entry._var, 0U );
        TS_ASSERT( FloatUtils::areEqual( entry._coefficient, -2 ) );

        entry = row._row[1];
        TS_ASSERT_EQUALS( entry._var, 1U );
        TS_ASSERT( FloatUtils::areEqual( entry._coefficient, -1 ) );

        entry = row._row[2];
        TS_ASSERT_EQUALS( entry._var, 5U );
        TS_ASSERT( FloatUtils::areEqual( entry._coefficient, 1 ) );

        entry = row._row[3];
        TS_ASSERT_EQUALS( entry._var, 3U );
        TS_ASSERT( FloatUtils::areEqual( entry._coefficient, -1 ) );

        TS_ASSERT_EQUALS( row._scalar, 108.0 );

        TS_ASSERT_THROWS_NOTHING( tableau->getTableauRow( 1, &row ) );

        entry = row._row[0];
        TS_ASSERT_EQUALS( entry._var, 0U );
        TS_ASSERT( FloatUtils::areEqual( entry._coefficient, -1 ) );

        entry = row._row[1];
        TS_ASSERT_EQUALS( entry._var, 1U );
        TS_ASSERT( FloatUtils::areEqual( entry._coefficient, -1 ) );

        entry = row._row[2];
        TS_ASSERT_EQUALS( entry._var, 5U );
        TS_ASSERT( FloatUtils::areEqual( entry._coefficient, -1 ) );

        entry = row._row[3];
        TS_ASSERT_EQUALS( entry._var, 3U );
        TS_ASSERT( FloatUtils::areEqual( entry._coefficient, -1 ) );

        TS_ASSERT_EQUALS( row._scalar, 117.0 );

        TS_ASSERT_THROWS_NOTHING( tableau->getTableauRow( 2, &row ) );

        entry = row._row[0];
        TS_ASSERT_EQUALS( entry._var, 0U );
        TS_ASSERT( FloatUtils::areEqual( entry._coefficient, -1 ) );

        entry = row._row[1];
        TS_ASSERT_EQUALS( entry._var, 1U );
        TS_ASSERT( FloatUtils::areEqual( entry._coefficient, 0 ) );

        entry = row._row[2];
        TS_ASSERT_EQUALS( entry._var, 5U );
        TS_ASSERT( FloatUtils::areEqual( entry._coefficient, 3 ) );

        entry = row._row[3];
        TS_ASSERT_EQUALS( entry._var, 3U );
        TS_ASSERT( FloatUtils::areEqual( entry._coefficient, -1 ) );

        TS_ASSERT_EQUALS( row._scalar, 69 );
        TS_ASSERT_THROWS_NOTHING( delete tableau );
    }

    void test_degenerate_pivot()
    {
        Tableau *tableau = NULL;
        MockCostFunctionManager costFunctionManager;

        TS_ASSERT( tableau = new Tableau );

        TS_ASSERT_THROWS_NOTHING( tableau->setDimensions( 3, 7, 3, 7 ) );
        tableau->registerCostFunctionManager( &costFunctionManager );
        initializeTableauValues( *tableau );

        for ( unsigned i = 0; i < 4; ++i )
        {
            TS_ASSERT_THROWS_NOTHING( tableau->setLowerBound( i, 1 ) );
            TS_ASSERT_THROWS_NOTHING( tableau->setUpperBound( i, 10 ) );
        }

        TS_ASSERT_THROWS_NOTHING( tableau->setLowerBound( 4, 219 ) );
        TS_ASSERT_THROWS_NOTHING( tableau->setUpperBound( 4, 228 ) );

        TS_ASSERT_THROWS_NOTHING( tableau->setLowerBound( 5, 112 ) );
        TS_ASSERT_THROWS_NOTHING( tableau->setUpperBound( 5, 114 ) );

        TS_ASSERT_THROWS_NOTHING( tableau->setLowerBound( 6, 400 ) );
        TS_ASSERT_THROWS_NOTHING( tableau->setUpperBound( 6, 402 ) );

        List<unsigned> basics = { 4, 5, 6 };
        TS_ASSERT_THROWS_NOTHING( tableau->initializeTableau( basics ) );

        TS_ASSERT_THROWS_NOTHING( tableau->computeAssignment() );

        TS_ASSERT( !tableau->isBasic( 2 ) );
        TS_ASSERT( tableau->isBasic( 5 ) );

        // Check equations before the pivot
        TableauRow row( 4 );
        TableauRow::Entry entry;

        TS_ASSERT_THROWS_NOTHING( tableau->getTableauRow( 0, &row ) );

        entry = row._row[0];
        TS_ASSERT_EQUALS( entry._var, 0U );
        TS_ASSERT_EQUALS( entry._coefficient, -3 );

        entry = row._row[1];
        TS_ASSERT_EQUALS( entry._var, 1U );
        TS_ASSERT_EQUALS( entry._coefficient, -2 );

        entry = row._row[2];
        TS_ASSERT_EQUALS( entry._var, 2U );
        TS_ASSERT_EQUALS( entry._coefficient, -1 );

        entry = row._row[3];
        TS_ASSERT_EQUALS( entry._var, 3U );
        TS_ASSERT_EQUALS( entry._coefficient, -2 );

        TS_ASSERT_THROWS_NOTHING( tableau->getTableauRow( 1, &row ) );

        entry = row._row[0];
        TS_ASSERT_EQUALS( entry._var, 0U );
        TS_ASSERT_EQUALS( entry._coefficient, -1 );

        entry = row._row[1];
        TS_ASSERT_EQUALS( entry._var, 1U );
        TS_ASSERT_EQUALS( entry._coefficient, -1 );

        entry = row._row[2];
        TS_ASSERT_EQUALS( entry._var, 2U );
        TS_ASSERT_EQUALS( entry._coefficient, -1 );

        entry = row._row[3];
        TS_ASSERT_EQUALS( entry._var, 3U );
        TS_ASSERT_EQUALS( entry._coefficient, -1 );

        TS_ASSERT_THROWS_NOTHING( tableau->getTableauRow( 2, &row ) );

        entry = row._row[0];
        TS_ASSERT_EQUALS( entry._var, 0U );
        TS_ASSERT_EQUALS( entry._coefficient, -4 );

        entry = row._row[1];
        TS_ASSERT_EQUALS( entry._var, 1U );
        TS_ASSERT_EQUALS( entry._coefficient, -3 );

        entry = row._row[2];
        TS_ASSERT_EQUALS( entry._var, 2U );
        TS_ASSERT_EQUALS( entry._coefficient, -3 );

        entry = row._row[3];
        TS_ASSERT_EQUALS( entry._var, 3U );
        TS_ASSERT_EQUALS( entry._coefficient, -4 );

        // Values before the degenerate pivot
        TS_ASSERT_EQUALS( tableau->getValue( 0 ), 1.0 );
        TS_ASSERT_EQUALS( tableau->getValue( 1 ), 1.0 );
        TS_ASSERT_EQUALS( tableau->getValue( 2 ), 1.0 );
        TS_ASSERT_EQUALS( tableau->getValue( 3 ), 1.0 );
        TS_ASSERT_EQUALS( tableau->getValue( 4 ), 217.0 );
        TS_ASSERT_EQUALS( tableau->getValue( 5 ), 113.0 );
        TS_ASSERT_EQUALS( tableau->getValue( 6 ), 406.0 );

        // Nonbasic index #2 --> variable 2
        // Basic index #1 --> variable 5
        tableau->setEnteringVariableIndex( 2 );
        tableau->setLeavingVariableIndex( 1 );
        tableau->computeChangeColumn();

        TS_ASSERT_THROWS_NOTHING( tableau->performDegeneratePivot() );
        TS_ASSERT( tableau->isBasic( 2 ) );
        TS_ASSERT( !tableau->isBasic( 5 ) );

        // Values should be unchanged after the degenerate pivot
        TS_ASSERT_EQUALS( tableau->getValue( 0 ), 1.0 );
        TS_ASSERT_EQUALS( tableau->getValue( 1 ), 1.0 );
        TS_ASSERT_EQUALS( tableau->getValue( 2 ), 1.0 );
        TS_ASSERT_EQUALS( tableau->getValue( 3 ), 1.0 );
        TS_ASSERT_EQUALS( tableau->getValue( 4 ), 217.0 );
        TS_ASSERT_EQUALS( tableau->getValue( 5 ), 113.0 );
        TS_ASSERT_EQUALS( tableau->getValue( 6 ), 406.0 );

        // Recompute the assignment, see that values are still unchanged
        TS_ASSERT_THROWS_NOTHING( tableau->computeAssignment() );

        TS_ASSERT_EQUALS( tableau->getValue( 0 ), 1.0 );
        TS_ASSERT_EQUALS( tableau->getValue( 1 ), 1.0 );
        TS_ASSERT_EQUALS( tableau->getValue( 2 ), 1.0 );
        TS_ASSERT_EQUALS( tableau->getValue( 3 ), 1.0 );
        TS_ASSERT_EQUALS( tableau->getValue( 4 ), 217.0 );
        TS_ASSERT_EQUALS( tableau->getValue( 5 ), 113.0 );
        TS_ASSERT_EQUALS( tableau->getValue( 6 ), 406.0 );

        // Old equations are:

           // x5 = 225 - 3x1 - 2x2 - x3  - 2x4
           // x6 = 117 -  x1 -  x2 - x3  -  x4
           // x7 = 420 - 4x1 - 3x2 - 3x3 - 4x4

        // New equations are:

           // x5 = 108 - 2x1 -  x2 + x6  -  x4
           // x3 = 117 -  x1 -  x2 - x6  -  x4
           // x7 = 69  -  x1       + 3x6 -  x4

        // Check equations after the pivot
        TS_ASSERT_THROWS_NOTHING( tableau->getTableauRow( 0, &row ) );

        entry = row._row[0];
        TS_ASSERT_EQUALS( entry._var, 0U );
        TS_ASSERT_EQUALS( entry._coefficient, -2 );

        entry = row._row[1];
        TS_ASSERT_EQUALS( entry._var, 1U );
        TS_ASSERT_EQUALS( entry._coefficient, -1 );

        entry = row._row[2];
        TS_ASSERT_EQUALS( entry._var, 5U );
        TS_ASSERT_EQUALS( entry._coefficient, 1 );

        entry = row._row[3];
        TS_ASSERT_EQUALS( entry._var, 3U );
        TS_ASSERT_EQUALS( entry._coefficient, -1 );

        TS_ASSERT_THROWS_NOTHING( tableau->getTableauRow( 1, &row ) );

        entry = row._row[0];
        TS_ASSERT_EQUALS( entry._var, 0U );
        TS_ASSERT_EQUALS( entry._coefficient, -1 );

        entry = row._row[1];
        TS_ASSERT_EQUALS( entry._var, 1U );
        TS_ASSERT_EQUALS( entry._coefficient, -1 );

        entry = row._row[2];
        TS_ASSERT_EQUALS( entry._var, 5U );
        TS_ASSERT_EQUALS( entry._coefficient, -1 );

        entry = row._row[3];
        TS_ASSERT_EQUALS( entry._var, 3U );
        TS_ASSERT_EQUALS( entry._coefficient, -1 );

        TS_ASSERT_THROWS_NOTHING( tableau->getTableauRow( 2, &row ) );

        entry = row._row[0];
        TS_ASSERT_EQUALS( entry._var, 0U );
        TS_ASSERT_EQUALS( entry._coefficient, -1 );

        entry = row._row[1];
        TS_ASSERT_EQUALS( entry._var, 1U );
        TS_ASSERT_EQUALS( entry._coefficient, 0 );

        entry = row._row[2];
        TS_ASSERT_EQUALS( entry._var, 5U );
        TS_ASSERT_EQUALS( entry._coefficient, 3 );

        entry = row._row[3];
        TS_ASSERT_EQUALS( entry._var, 3U );
        TS_ASSERT_EQUALS( entry._coefficient, -1 );

        TS_ASSERT_THROWS_NOTHING( delete tableau );
    }

    void test_store_and_restore()
    {
        Tableau *tableau = NULL;
        MockCostFunctionManager costFunctionManager;

        TS_ASSERT( tableau = new Tableau );

        // Initialization steps

        TS_ASSERT_THROWS_NOTHING( tableau->setDimensions( 3, 7, 3, 7 ) );
        tableau->registerCostFunctionManager( &costFunctionManager );
        initializeTableauValues( *tableau );

        for ( unsigned i = 0; i < 4; ++i )
        {
            TS_ASSERT_THROWS_NOTHING( tableau->setLowerBound( i, 1 ) );
            TS_ASSERT_THROWS_NOTHING( tableau->setUpperBound( i, 2 ) );
        }

        TS_ASSERT_THROWS_NOTHING( tableau->setLowerBound( 4, 219 ) );
        TS_ASSERT_THROWS_NOTHING( tableau->setUpperBound( 4, 228 ) );

        TS_ASSERT_THROWS_NOTHING( tableau->setLowerBound( 5, 111.5 ) );
        TS_ASSERT_THROWS_NOTHING( tableau->setUpperBound( 5, 114 ) );

        TS_ASSERT_THROWS_NOTHING( tableau->setLowerBound( 6, 200 ) );
        TS_ASSERT_THROWS_NOTHING( tableau->setUpperBound( 6, 202 ) );

        List<unsigned> basics = { 4, 5, 6 };
        TS_ASSERT_THROWS_NOTHING( tableau->initializeTableau( basics ) );

        TS_ASSERT_THROWS_NOTHING( tableau->computeCostFunction() );
        costFunctionManager.nextCostFunction = new double[4];
        costFunctionManager.nextCostFunction[0] = -1;
        costFunctionManager.nextCostFunction[1] = -1;
        costFunctionManager.nextCostFunction[2] = -1;
        costFunctionManager.nextCostFunction[3] = -1;

        costFunctionManager.nextBasicCost[0] = -1;
        costFunctionManager.nextBasicCost[1] =  0;
        costFunctionManager.nextBasicCost[2] = +1;

        TS_ASSERT( hasCandidates( *tableau ) );
        tableau->setEnteringVariableIndex( 3u );
        tableau->computeChangeColumn();

        tableau->pickLeavingVariable();

        TS_ASSERT_EQUALS( tableau->getEnteringVariable(), 3u );
        TS_ASSERT_EQUALS( tableau->getLeavingVariable(), 3u );

        TS_ASSERT( !tableau->isBasic( 3u ) );

        TS_ASSERT_EQUALS( tableau->getValue( 3u ), 1.0 );

        TS_ASSERT_THROWS_NOTHING( tableau->performPivot() );

        // Check some stuff

        TS_ASSERT( !tableau->isBasic( 3u ) );

        TS_ASSERT_EQUALS( tableau->getValue( 3u ), 2.0 );

        TS_ASSERT_THROWS_NOTHING( tableau->computeAssignment() );

        // Store the tableau
        TableauState *tableauState = NULL;
        TS_ASSERT( tableauState = new TableauState );

        TS_ASSERT_THROWS_NOTHING( tableau->storeState( *tableauState ) );

        // Do some more stuff
        TS_ASSERT_THROWS_NOTHING( tableau->computeCostFunction() );
        tableau->setEnteringVariableIndex( 2u );

        tableau->computeChangeColumn();

        TS_ASSERT_THROWS_NOTHING( tableau->pickLeavingVariable() );
        TS_ASSERT_EQUALS( tableau->getEnteringVariable(), 2u );
        TS_ASSERT_EQUALS( tableau->getLeavingVariable(), 5u );

        TS_ASSERT( !tableau->isBasic( 2u ) );
        TS_ASSERT( tableau->isBasic( 5u ) );

        TS_ASSERT_EQUALS( tableau->getValue( 2u ), 1.0 );
        TS_ASSERT_EQUALS( tableau->getValue( 5u ), 112.0 );

        TS_ASSERT_THROWS_NOTHING( tableau->computePivotRow() );

        TS_ASSERT_THROWS_NOTHING( tableau->performPivot() );

        TS_ASSERT_DIFFERS( tableau->getValue( 2u ), 1.0 );
        TS_ASSERT_DIFFERS( tableau->getValue( 5u ), 112.0 );

        TS_ASSERT( tableau->isBasic( 2u ) );
        TS_ASSERT( !tableau->isBasic( 5u ) );

        // Now restore the tableau
        TS_ASSERT_THROWS_NOTHING( tableau->restoreState( *tableauState ) );

        // Do some more stuff again
        TS_ASSERT_THROWS_NOTHING( tableau->computeCostFunction() );
        tableau->setEnteringVariableIndex( 2u );
        tableau->computeChangeColumn();

        TS_ASSERT_THROWS_NOTHING( tableau->pickLeavingVariable() );

        TS_ASSERT_EQUALS( tableau->getEnteringVariable(), 2u );
        TS_ASSERT_EQUALS( tableau->getLeavingVariable(), 5u );

        TS_ASSERT( !tableau->isBasic( 2u ) );
        TS_ASSERT( tableau->isBasic( 5u ) );

        TS_ASSERT_EQUALS( tableau->getValue( 2u ), 1.0 );
        TS_ASSERT_EQUALS( tableau->getValue( 5u ), 112.0 );

        TS_ASSERT_THROWS_NOTHING( tableau->computePivotRow() );

        TS_ASSERT_THROWS_NOTHING( tableau->performPivot() );

        TS_ASSERT_DIFFERS( tableau->getValue( 2u ), 1.0 );
        TS_ASSERT_DIFFERS( tableau->getValue( 5u ), 112.0 );

        TS_ASSERT( tableau->isBasic( 2u ) );
        TS_ASSERT( !tableau->isBasic( 5u ) );

        TS_ASSERT_THROWS_NOTHING( delete tableauState );
        TS_ASSERT_THROWS_NOTHING( delete tableau );
    }

    void test_add_equation()
    {
        Tableau *tableau = NULL;
        MockCostFunctionManager costFunctionManager;

        TS_ASSERT( tableau = new Tableau );

        TS_ASSERT_THROWS_NOTHING( tableau->setDimensions( 3, 7, 3, 7 ) );
        tableau->registerCostFunctionManager( &costFunctionManager );
        initializeTableauValues( *tableau );

        for ( unsigned i = 0; i < 4; ++i )
        {
            TS_ASSERT_THROWS_NOTHING( tableau->setLowerBound( i, 1 ) );
            TS_ASSERT_THROWS_NOTHING( tableau->setUpperBound( i, 10 ) );
        }

        TS_ASSERT_THROWS_NOTHING( tableau->setLowerBound( 4, 219 ) );
        TS_ASSERT_THROWS_NOTHING( tableau->setUpperBound( 4, 228 ) );

        TS_ASSERT_THROWS_NOTHING( tableau->setLowerBound( 5, 112 ) );
        TS_ASSERT_THROWS_NOTHING( tableau->setUpperBound( 5, 114 ) );

        TS_ASSERT_THROWS_NOTHING( tableau->setLowerBound( 6, 400 ) );
        TS_ASSERT_THROWS_NOTHING( tableau->setUpperBound( 6, 402 ) );

        List<unsigned> basics = { 4, 5, 6 };
        TS_ASSERT_THROWS_NOTHING( tableau->initializeTableau( basics ) );

        // Do a pivot to shuffle the basis
        tableau->setEnteringVariableIndex( 2u );
        TS_ASSERT_THROWS_NOTHING( tableau->computeCostFunction() );
        costFunctionManager.nextCostFunction = new double[4];
        costFunctionManager.nextCostFunction[0] = -1;
        costFunctionManager.nextCostFunction[1] = -1;
        costFunctionManager.nextCostFunction[2] = -1;
        costFunctionManager.nextCostFunction[3] = -1;

        costFunctionManager.nextBasicCost[0] = -1;
        costFunctionManager.nextBasicCost[1] =  0;
        costFunctionManager.nextBasicCost[2] = +1;

        TS_ASSERT_THROWS_NOTHING( tableau->computeChangeColumn() );
        tableau->pickLeavingVariable();

        TS_ASSERT_THROWS_NOTHING( tableau->computePivotRow() );

        TS_ASSERT_THROWS_NOTHING( tableau->performPivot() );

        // Variables x3 and x6 have been pivoted
        TS_ASSERT( tableau->isBasic( 2u ) );
        TS_ASSERT( !tableau->isBasic( 5u ) );

        /*
          Current basic variables are: x5, x3, x7
          Current basis matrix B0 is:

                | 1 1   |
           B0 = |   1   |
                |   1 1 |

           Now add a new new equation:

               2x2 - 4x3 + x8 = 5

           Where x8 is a new basic variable
        */

        Equation equation;
        equation.addAddend( 2, 1 );
        equation.addAddend( -4, 2 );
        equation.setScalar( 5 );
        TS_ASSERT_THROWS_NOTHING( tableau->addEquation( equation ) );

        TS_ASSERT( tableau->isBasic( 7u ) );

        /*
          Test that an old row is still computed correctly, with 0
          entry for the new variable.
          Test that the new row is also computed correctly.
        */

        TableauRow row( 4 );
        tableau->getTableauRow( 0, &row );

        // Row 0 is the row for x5. Due to the earlier pivot, we expect:
        //     x5 = 225 - 2x1 - x2 + x6 - x4
        TableauRow::Entry entry;
        entry = row._row[0];
        TS_ASSERT_EQUALS( entry._var, 0U );
        TS_ASSERT_EQUALS( entry._coefficient, -2.0 );

        entry = row._row[1];
        TS_ASSERT_EQUALS( entry._var, 1U );
        TS_ASSERT_EQUALS( entry._coefficient, -1.0 );

        entry = row._row[2];
        TS_ASSERT_EQUALS( entry._var, 5U );
        TS_ASSERT_EQUALS( entry._coefficient, 1.0 );

        entry = row._row[3];
        TS_ASSERT_EQUALS( entry._var, 3U );
        TS_ASSERT_EQUALS( entry._coefficient, -1.0 );

        // Row 3 is the new row.
        //   Originally,  x8 = 5 - 2x2 + 4x3
        //   But due to the pivot operation from before, x3 is basic.
        //   It is given by the equation:
        //      x3 = -x1 -x2 -x6 -x4 + 117
        //   And so, we expect:
        //      x8 = 473 - 4x1 - 6x2 -4x6 -4x4

        tableau->getTableauRow( 3, &row );

        entry = row._row[0];
        TS_ASSERT_EQUALS( entry._var, 0U );
        TS_ASSERT_EQUALS( entry._coefficient, -4.0 );

        entry = row._row[1];
        TS_ASSERT_EQUALS( entry._var, 1U );
        TS_ASSERT_EQUALS( entry._coefficient, -6.0 );

        entry = row._row[2];
        TS_ASSERT_EQUALS( entry._var, 5U );
        TS_ASSERT_EQUALS( entry._coefficient, -4.0 );

        entry = row._row[3];
        TS_ASSERT_EQUALS( entry._var, 3U );
        TS_ASSERT_EQUALS( entry._coefficient, -4.0 );

        /*
          At this point, the equations are:

             x5 = -2x1 - x2 + x6 - x4 + 108
             x3 =  -x1 - x2 - x6 - x4 + 117
             x7 =  -x1      +3x6 - x4 + 69
             x8 = -4x1 -6x2 -4x6 -4x4 + 473
        */

        // Check the assignment
        tableau->computeAssignment();

        // Non-Basics
        TS_ASSERT_EQUALS( tableau->getValue( 0 ), 1.0 );
        TS_ASSERT_EQUALS( tableau->getValue( 1 ), 1.0 );
        TS_ASSERT_EQUALS( tableau->getValue( 5 ), 112.0 );
        TS_ASSERT_EQUALS( tableau->getValue( 3 ), 1.0 );

        // Basics
        TS_ASSERT_EQUALS( tableau->getValue( 4 ), 216.0 );
        TS_ASSERT_EQUALS( tableau->getValue( 2 ), 2.0 );
        TS_ASSERT_EQUALS( tableau->getValue( 6 ), 403.0 );
        TS_ASSERT_EQUALS( tableau->getValue( 7 ), 11.0 ); // 473 - 4 - 6 - 4 - 4*112

        TS_ASSERT_THROWS_NOTHING( delete tableau );
    }

    void test_tighten_bounds()
    {
        Tableau *tableau = NULL;
        MockCostFunctionManager costFunctionManager;

        TS_ASSERT( tableau = new Tableau );

        TS_ASSERT_THROWS_NOTHING( tableau->setDimensions( 3, 7, 3, 7 ) );
        tableau->registerCostFunctionManager( &costFunctionManager );
        initializeTableauValues( *tableau );

        for ( unsigned i = 0; i < 4; ++i )
        {
            TS_ASSERT_THROWS_NOTHING( tableau->setLowerBound( i, 1 ) );
            TS_ASSERT_THROWS_NOTHING( tableau->setUpperBound( i, 10 ) );
        }

        TS_ASSERT_THROWS_NOTHING( tableau->setLowerBound( 4, 218 ) );
        TS_ASSERT_THROWS_NOTHING( tableau->setUpperBound( 4, 228 ) );

        TS_ASSERT_THROWS_NOTHING( tableau->setLowerBound( 5, 100 ) );
        TS_ASSERT_THROWS_NOTHING( tableau->setUpperBound( 5, 114 ) );

        TS_ASSERT_THROWS_NOTHING( tableau->setLowerBound( 6, 400 ) );
        TS_ASSERT_THROWS_NOTHING( tableau->setUpperBound( 6, 402 ) );

        List<unsigned> basics = { 4, 5, 6 };
        TS_ASSERT_THROWS_NOTHING( tableau->initializeTableau( basics ) );

        for ( unsigned i = 0; i < 4; ++i )
        {
            TS_ASSERT_EQUALS( tableau->getLowerBound( i ), 1 );
            TS_ASSERT_EQUALS( tableau->getUpperBound( i ), 10 );
            TS_ASSERT_EQUALS( tableau->getValue( i ), 1.0 );
        }

        TS_ASSERT_EQUALS( tableau->getValue( 4 ), 217.0 );
        TS_ASSERT_EQUALS( tableau->getValue( 5 ), 113.0 );
        TS_ASSERT_EQUALS( tableau->getValue( 6 ), 406.0 );

        TS_ASSERT_THROWS_NOTHING( tableau->tightenLowerBound( 1, 4 ) );

        for ( unsigned i = 0; i < 4; ++i )
        {
            if ( i == 1 )
                continue;

            TS_ASSERT_EQUALS( tableau->getLowerBound( i ), 1 );
            TS_ASSERT_EQUALS( tableau->getUpperBound( i ), 10 );
            TS_ASSERT_EQUALS( tableau->getValue( i ), 1.0 );
        }

        TS_ASSERT_EQUALS( tableau->getLowerBound( 1 ), 4 );
        TS_ASSERT_EQUALS( tableau->getUpperBound( 1 ), 10 );
        TS_ASSERT_EQUALS( tableau->getValue( 1 ), 4.0 );

        TS_ASSERT_THROWS_NOTHING( tableau->tightenLowerBound( 1, 2 ) );
        TS_ASSERT_THROWS_NOTHING( tableau->tightenUpperBound( 1, 22 ) );

        TS_ASSERT_EQUALS( tableau->getLowerBound( 1 ), 4 );
        TS_ASSERT_EQUALS( tableau->getUpperBound( 1 ), 10 );
        TS_ASSERT_EQUALS( tableau->getValue( 1 ), 4.0 );

        TS_ASSERT_THROWS_NOTHING( tableau->tightenUpperBound( 1, 8 ) );

        TS_ASSERT_EQUALS( tableau->getLowerBound( 1 ), 4 );
        TS_ASSERT_EQUALS( tableau->getUpperBound( 1 ), 8 );
        TS_ASSERT_EQUALS( tableau->getValue( 1 ), 4.0 );

        // Tightening the bounds of basic variables doesn't change their values
        TS_ASSERT_EQUALS( tableau->getValue( 5 ), 110.0 );
        TS_ASSERT_THROWS_NOTHING( tableau->tightenLowerBound( 5, 111 ) );
        TS_ASSERT_EQUALS( tableau->getValue( 5 ), 110.0 );

        TS_ASSERT_THROWS_NOTHING( delete tableau );
    }

    void test_are_dependent()
    {
        Tableau *tableau = NULL;
        MockCostFunctionManager costFunctionManager;

        TS_ASSERT( tableau = new Tableau );

        TS_ASSERT_THROWS_NOTHING( tableau->setDimensions( 3, 7, 3, 7 ) );
        tableau->registerCostFunctionManager( &costFunctionManager );
        initializeTableauValues( *tableau );

        for ( unsigned i = 0; i < 4; ++i )
        {
            TS_ASSERT_THROWS_NOTHING( tableau->setLowerBound( i, 1 ) );
            TS_ASSERT_THROWS_NOTHING( tableau->setUpperBound( i, 2 ) );
        }

        TS_ASSERT_THROWS_NOTHING( tableau->setLowerBound( 4, 219 ) );
        TS_ASSERT_THROWS_NOTHING( tableau->setUpperBound( 4, 228 ) );

        TS_ASSERT_THROWS_NOTHING( tableau->setLowerBound( 5, 111.5 ) );
        TS_ASSERT_THROWS_NOTHING( tableau->setUpperBound( 5, 114 ) );

        TS_ASSERT_THROWS_NOTHING( tableau->setLowerBound( 6, 200 ) );
        TS_ASSERT_THROWS_NOTHING( tableau->setUpperBound( 6, 202 ) );

        List<unsigned> basics = { 4, 5, 6 };
        TS_ASSERT_THROWS_NOTHING( tableau->initializeTableau( basics ) );

        double coefficient, inverseCoefficient;

        // Basics are independet, non-basics are independet
        TS_ASSERT( !tableau->areLinearlyDependent( 0, 1, coefficient, inverseCoefficient ) );
        TS_ASSERT( !tableau->areLinearlyDependent( 1, 0, coefficient, inverseCoefficient ) );
        TS_ASSERT( !tableau->areLinearlyDependent( 0, 2, coefficient, inverseCoefficient ) );
        TS_ASSERT( !tableau->areLinearlyDependent( 2, 0, coefficient, inverseCoefficient ) );
        TS_ASSERT( !tableau->areLinearlyDependent( 0, 3, coefficient, inverseCoefficient ) );
        TS_ASSERT( !tableau->areLinearlyDependent( 3, 0, coefficient, inverseCoefficient ) );
        TS_ASSERT( !tableau->areLinearlyDependent( 1, 2, coefficient, inverseCoefficient ) );
        TS_ASSERT( !tableau->areLinearlyDependent( 2, 1, coefficient, inverseCoefficient ) );
        TS_ASSERT( !tableau->areLinearlyDependent( 1, 3, coefficient, inverseCoefficient ) );
        TS_ASSERT( !tableau->areLinearlyDependent( 3, 1, coefficient, inverseCoefficient ) );
        TS_ASSERT( !tableau->areLinearlyDependent( 2, 3, coefficient, inverseCoefficient ) );
        TS_ASSERT( !tableau->areLinearlyDependent( 3, 2, coefficient, inverseCoefficient ) );

        TS_ASSERT( !tableau->areLinearlyDependent( 4, 5, coefficient, inverseCoefficient ) );
        TS_ASSERT( !tableau->areLinearlyDependent( 5, 4, coefficient, inverseCoefficient ) );
        TS_ASSERT( !tableau->areLinearlyDependent( 4, 6, coefficient, inverseCoefficient ) );
        TS_ASSERT( !tableau->areLinearlyDependent( 6, 4, coefficient, inverseCoefficient ) );
        TS_ASSERT( !tableau->areLinearlyDependent( 5, 6, coefficient, inverseCoefficient ) );
        TS_ASSERT( !tableau->areLinearlyDependent( 6, 5, coefficient, inverseCoefficient ) );

        // All basics depend on all non-basics

        /*
           x5 = 225 - 3x1 - 2x2 - x3  - 2x4
           x6 = 117 -  x1 -  x2 - x3  -  x4
           x7 = 420 - 4x1 - 3x2 - 3x3 - 4x4
        */

        TS_ASSERT( tableau->areLinearlyDependent( 0, 4, coefficient, inverseCoefficient ) );
        TS_ASSERT( FloatUtils::areEqual( coefficient, -3 ) );
        TS_ASSERT( FloatUtils::areEqual( inverseCoefficient, -1.0/3 ) );

        TS_ASSERT( tableau->areLinearlyDependent( 1, 4, coefficient, inverseCoefficient ) );
        TS_ASSERT( FloatUtils::areEqual( coefficient, -2 ) );
        TS_ASSERT( FloatUtils::areEqual( inverseCoefficient, -1.0/2 ) );

        TS_ASSERT( tableau->areLinearlyDependent( 2, 4, coefficient, inverseCoefficient ) );
        TS_ASSERT( FloatUtils::areEqual( coefficient, -1 ) );
        TS_ASSERT( FloatUtils::areEqual( inverseCoefficient, -1 ) );

        TS_ASSERT( tableau->areLinearlyDependent( 3, 4, coefficient, inverseCoefficient ) );
        TS_ASSERT( FloatUtils::areEqual( coefficient, -2 ) );
        TS_ASSERT( FloatUtils::areEqual( inverseCoefficient, -1.0/2 ) );

        TS_ASSERT( tableau->areLinearlyDependent( 0, 5, coefficient, inverseCoefficient ) );
        TS_ASSERT( FloatUtils::areEqual( coefficient, -1 ) );
        TS_ASSERT( FloatUtils::areEqual( inverseCoefficient, -1 ) );

        TS_ASSERT( tableau->areLinearlyDependent( 1, 5, coefficient, inverseCoefficient ) );
        TS_ASSERT( FloatUtils::areEqual( coefficient, -1 ) );
        TS_ASSERT( FloatUtils::areEqual( inverseCoefficient, -1 ) );

        TS_ASSERT( tableau->areLinearlyDependent( 2, 5, coefficient, inverseCoefficient ) );
        TS_ASSERT( FloatUtils::areEqual( coefficient, -1 ) );
        TS_ASSERT( FloatUtils::areEqual( inverseCoefficient, -1 ) );

        TS_ASSERT( tableau->areLinearlyDependent( 3, 5, coefficient, inverseCoefficient ) );
        TS_ASSERT( FloatUtils::areEqual( coefficient, -1 ) );
        TS_ASSERT( FloatUtils::areEqual( inverseCoefficient, -1 ) );

        TS_ASSERT( tableau->areLinearlyDependent( 0, 6, coefficient, inverseCoefficient ) );
        TS_ASSERT( FloatUtils::areEqual( coefficient, -4 ) );
        TS_ASSERT( FloatUtils::areEqual( inverseCoefficient, -1.0/4 ) );

        TS_ASSERT( tableau->areLinearlyDependent( 1, 6, coefficient, inverseCoefficient ) );
        TS_ASSERT( FloatUtils::areEqual( coefficient, -3 ) );
        TS_ASSERT( FloatUtils::areEqual( inverseCoefficient, -1.0/3 ) );

        TS_ASSERT( tableau->areLinearlyDependent( 2, 6, coefficient, inverseCoefficient ) );
        TS_ASSERT( FloatUtils::areEqual( coefficient, -3 ) );
        TS_ASSERT( FloatUtils::areEqual( inverseCoefficient, -1.0/3 ) );

        TS_ASSERT( tableau->areLinearlyDependent( 3, 6, coefficient, inverseCoefficient ) );
        TS_ASSERT( FloatUtils::areEqual( coefficient, -4 ) );
        TS_ASSERT( FloatUtils::areEqual( inverseCoefficient, -1.0/4 ) );

        // Now perform a pivot, and see that things still make sense afterwards

        // Nonbasic index #2 --> variable 2
        // Basic index #1 --> variable 5
        tableau->setEnteringVariableIndex( 2 );
        tableau->setLeavingVariableIndex( 1 );
        tableau->computeChangeColumn();

        TS_ASSERT_THROWS_NOTHING( tableau->performDegeneratePivot() );
        TS_ASSERT( tableau->isBasic( 2 ) );
        TS_ASSERT( !tableau->isBasic( 5 ) );

        /*

        Old equations are:

           x5 = 225 - 3x1 - 2x2 - x3  - 2x4
           x6 = 117 -  x1 -  x2 - x3  -  x4
           x7 = 420 - 4x1 - 3x2 - 3x3 - 4x4

        New equations are:

           x5 = 108 - 2x1 -  x2 + x6  -  x4
           x3 = 117 -  x1 -  x2 - x6  -  x4
           x7 = 69  -  x1       + 3x6 -  x4

        */

        TS_ASSERT( !tableau->areLinearlyDependent( 0, 1, coefficient, inverseCoefficient ) );
        TS_ASSERT( !tableau->areLinearlyDependent( 1, 0, coefficient, inverseCoefficient ) );
        TS_ASSERT( !tableau->areLinearlyDependent( 0, 5, coefficient, inverseCoefficient ) );
        TS_ASSERT( !tableau->areLinearlyDependent( 5, 0, coefficient, inverseCoefficient ) );
        TS_ASSERT( !tableau->areLinearlyDependent( 0, 3, coefficient, inverseCoefficient ) );
        TS_ASSERT( !tableau->areLinearlyDependent( 3, 0, coefficient, inverseCoefficient ) );
        TS_ASSERT( !tableau->areLinearlyDependent( 1, 5, coefficient, inverseCoefficient ) );
        TS_ASSERT( !tableau->areLinearlyDependent( 5, 1, coefficient, inverseCoefficient ) );
        TS_ASSERT( !tableau->areLinearlyDependent( 1, 3, coefficient, inverseCoefficient ) );
        TS_ASSERT( !tableau->areLinearlyDependent( 3, 1, coefficient, inverseCoefficient ) );
        TS_ASSERT( !tableau->areLinearlyDependent( 5, 3, coefficient, inverseCoefficient ) );
        TS_ASSERT( !tableau->areLinearlyDependent( 3, 5, coefficient, inverseCoefficient ) );

        TS_ASSERT( !tableau->areLinearlyDependent( 4, 2, coefficient, inverseCoefficient ) );
        TS_ASSERT( !tableau->areLinearlyDependent( 2, 4, coefficient, inverseCoefficient ) );
        TS_ASSERT( !tableau->areLinearlyDependent( 4, 6, coefficient, inverseCoefficient ) );
        TS_ASSERT( !tableau->areLinearlyDependent( 6, 4, coefficient, inverseCoefficient ) );
        TS_ASSERT( !tableau->areLinearlyDependent( 2, 6, coefficient, inverseCoefficient ) );
        TS_ASSERT( !tableau->areLinearlyDependent( 6, 2, coefficient, inverseCoefficient ) );

        TS_ASSERT( tableau->areLinearlyDependent( 0, 2, coefficient, inverseCoefficient ) );
        TS_ASSERT( FloatUtils::areEqual( coefficient, -1 ) );
        TS_ASSERT( FloatUtils::areEqual( inverseCoefficient, -1 ) );

        TS_ASSERT( tableau->areLinearlyDependent( 1, 2, coefficient, inverseCoefficient ) );
        TS_ASSERT( FloatUtils::areEqual( coefficient, -1 ) );
        TS_ASSERT( FloatUtils::areEqual( inverseCoefficient, -1 ) );

        TS_ASSERT( tableau->areLinearlyDependent( 5, 2, coefficient, inverseCoefficient ) );
        TS_ASSERT( FloatUtils::areEqual( coefficient, -1 ) );
        TS_ASSERT( FloatUtils::areEqual( inverseCoefficient, -1 ) );

        TS_ASSERT( tableau->areLinearlyDependent( 3, 2, coefficient, inverseCoefficient ) );
        TS_ASSERT( FloatUtils::areEqual( coefficient, -1 ) );
        TS_ASSERT( FloatUtils::areEqual( inverseCoefficient, -1 ) );

        TS_ASSERT_THROWS_NOTHING( delete tableau );
    }

    void test_todo()
    {
        TS_TRACE( "When resizing the talbeau, allocate a larger size and only use part of it, "
                  "instead of increasing it one row at a time?" );
        TS_TRACE( "Make sure all watchers are properply informed when restoring a tabealu" );
        TS_TRACE( "Recomputing the cost function: more clever handling for row addition and "
                  "the setNonBasic() case?" );
    }
};

//
// Local Variables:
// compile-command: "make -C ../../.. "
// tags-file-name: "../../../TAGS"
// c-basic-offset: 4
// End:
//
