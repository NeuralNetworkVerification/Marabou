/*********************                                                        */
/*! \file Test_CostFunctionManager.h
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

#include <cxxtest/TestSuite.h>

#include "CostFunctionManager.h"
#include "MockErrno.h"
#include "MockTableau.h"
#include "MarabouError.h"

#include <string.h>

class MockForCostFunctionManager
{
public:
};

class CostFunctionManagerTestSuite : public CxxTest::TestSuite
{
public:
    MockForCostFunctionManager *mock;

    void setUp()
    {
        TS_ASSERT( mock = new MockForCostFunctionManager );
    }

    void tearDown()
    {
        TS_ASSERT_THROWS_NOTHING( delete mock );
    }

    void test_constructor()
    {
        CostFunctionManager *manager = NULL;
        MockTableau tableau;
        TS_ASSERT( manager = new CostFunctionManager( &tableau ) );

        TS_ASSERT_EQUALS( manager->getCostFunctionStatus(),
                          CostFunctionManager::COST_FUNCTION_INVALID );

        TS_ASSERT_THROWS_NOTHING( delete manager );
    }

    void test_compute_core_cost_function()
    {
        CostFunctionManager *manager = NULL;
        MockTableau tableau;

        unsigned n = 5;
        unsigned m = 3;
        tableau.setDimensions( m, n, m, n );

        TS_ASSERT( manager = new CostFunctionManager( &tableau ) );
        TS_ASSERT_THROWS_NOTHING( manager->initialize() );

        // Prepare the intermediate information needed for cost function computation:
        // 1. The multipliers
        double multipliers[3] = { 0, 2, -3 };
        memcpy( tableau.nextBtranOutput, multipliers, sizeof(double) * 3 );
        // 2. Mapping from non-basic indices to original variables
        tableau.nextNonBasicIndexToVariable[0] = 2;
        tableau.nextNonBasicIndexToVariable[1] = 0;
        // 3. Constraint matrix columns for non basic variables
        double columnZero[] = { 1, -1, 2 };
        double columnTwo[] = { 3, 1, 0 };
        tableau.nextAColumn[0] = columnZero;
        tableau.nextAColumn[2] = columnTwo;

        // We have 3 basic variables. One is too high, one too low, one within bounds.

        tableau.nextBasicIndexToVariable[0] = 5;
        tableau.nextBasicIndexToVariable[1] = 6;
        tableau.nextBasicIndexToVariable[2] = 7;

        tableau.lowerBounds[5] = 0;
        tableau.upperBounds[5] = 1;
        tableau.lowerBounds[6] = 0;
        tableau.upperBounds[6] = 1;
        tableau.lowerBounds[7] = 0;
        tableau.upperBounds[7] = 1;

        tableau.nextValues[5] = 10; // Too high
        tableau.nextValues[6] = -10; // Too low
        tableau.nextValues[7] = 0.5; // Okay

        TS_ASSERT_THROWS_NOTHING( manager->computeCoreCostFunction() );

        // Basic costs should be [ 1, -1, 0 ], and this should have been sent to BTRAN.
        double expectedBTranInput[] = { 1, -1, 0 };
        TS_ASSERT_SAME_DATA( tableau.lastBtranInput, expectedBTranInput, sizeof(double) * m );

        // Each entry of the cost function is a negated dot product of the multiplier vector
        // and the appropriate column from the constraint matrix
        const double *costFucntion = manager->getCostFunction();

        // Entry 0: multipliers * columnTwo
        TS_ASSERT_EQUALS( costFucntion[0], -( 0 + 2 + 0 ) );
        // Entry 1: multipliers * columnZero
        TS_ASSERT_EQUALS( costFucntion[1], -( 0 - 2 - 6 ) );

        TS_ASSERT_THROWS_NOTHING( delete manager );
    }

    void test_compute_cost_function()
    {
        CostFunctionManager *manager = NULL;
        MockTableau tableau;

        unsigned n = 5;
        unsigned m = 3;
        tableau.setDimensions( m, n, m, n );

        TS_ASSERT( manager = new CostFunctionManager( &tableau ) );
        TS_ASSERT_THROWS_NOTHING( manager->initialize() );

        // Prepare the intermediate information needed for cost function computation:
        // 1. The multipliers
        double multipliers[3] = { 0, 2, -3 };
        memcpy( tableau.nextBtranOutput, multipliers, sizeof(double) * 3 );
        // 2. Mapping from non-basic indices to original variables
        tableau.nextNonBasicIndexToVariable[0] = 2;
        tableau.nextNonBasicIndexToVariable[1] = 0;
        // 3. Constraint matrix columns for non basic variables
        double columnZero[] = { 1, -1, 2 };
        double columnTwo[] = { 3, 1, 0 };
        tableau.nextAColumn[0] = columnZero;
        tableau.nextAColumn[2] = columnTwo;

        // We have 3 basic variables. One is too high, one too low, one within bounds.
        tableau.nextBasicIndexToVariable[0] = 5;
        tableau.nextBasicIndexToVariable[1] = 6;
        tableau.nextBasicIndexToVariable[2] = 7;

        tableau.lowerBounds[5] = 0;
        tableau.upperBounds[5] = 1;
        tableau.lowerBounds[6] = 0;
        tableau.upperBounds[6] = 1;
        tableau.lowerBounds[7] = 0;
        tableau.upperBounds[7] = 1;

        tableau.nextValues[5] = 10; // Too high
        tableau.nextValues[6] = -10; // Too low
        tableau.nextValues[7] = 0.5; // Okay

        // The heuristic costs change a basic var and a non-basic variable
        Map<unsigned, double> heuristicCost;
        // Variable 2 is basic #0
        tableau.nextVariableToIndex[2] = 0;
        heuristicCost[2] = 1;
        tableau.nextIsBasic.insert( 2 );
        // Variable 3 is non-basic #1
        tableau.nextVariableToIndex[3] = 1;
        heuristicCost[3] = 4;

        TS_ASSERT_THROWS_NOTHING( manager->computeCostFunction( heuristicCost ) );

        // Basic costs should be [ 2, -1, 0 ], and this should have been sent to BTRAN.
        double expectedBTranInput[] = { 2, -1, 0 };
        TS_ASSERT_SAME_DATA( tableau.lastBtranInput, expectedBTranInput, sizeof(double) * m );

        // Each entry of the cost function is a negated dot product of the multiplier vector
        // and the appropriate column from the constraint matrix
        const double *costFucntion = manager->getCostFunction();

        // Entry 0: multipliers * columnTwo
        TS_ASSERT_EQUALS( costFucntion[0], -( 0 + 2 + 0 ) );
        // Entry 1: multipliers * columnZero + heuristic cost
        TS_ASSERT_EQUALS( costFucntion[1], -( 0 - 2 - 6 ) + 4 );

        TS_ASSERT_THROWS_NOTHING( delete manager );
    }
};

//
// Local Variables:
// compile-command: "make -C ../../.. "
// tags-file-name: "../../../TAGS"
// c-basic-offset: 4
// End:
//
