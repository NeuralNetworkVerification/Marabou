/*********************                                                        */
/*! \file Test_ProjectedSteepestEdge.h
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
#include "ProjectedSteepestEdge.h"

class MockForProjectedSteepestEdge
{
public:
};

class ProjectedSteepestEdgeTestSuite : public CxxTest::TestSuite
{
public:
    MockForProjectedSteepestEdge *mock;

    void setUp()
    {
        TS_ASSERT( mock = new MockForProjectedSteepestEdge );
    }

    void tearDown()
    {
        TS_ASSERT_THROWS_NOTHING( delete mock );
    }

    void test_variable_selection()
    {
        MockTableau tableau;
        tableau.setDimensions( 2, 5 );

        ProjectedSteepestEdgeRule pse;

        // Non basics are {x0, x1, x2}, basics are {x4, x5}
        tableau.nextNonBasicIndexToVariable[0] = 0;
        tableau.nextNonBasicIndexToVariable[1] = 1;
        tableau.nextNonBasicIndexToVariable[2] = 2;

        TS_ASSERT_THROWS_NOTHING( pse.initialize( tableau ) );

        // After the first initialization, gamma should be 1 for all variables.
        for ( unsigned i = 0; i < 3; ++i )
            TS_ASSERT_EQUALS( pse.getGamma( i ), 1.0 );

        // Next, we're going to ask pse to pick an entering variable.
        // All variables are eligible, none are excluded
        Set<unsigned> excluded;

        List<unsigned> candidates = { 0, 1, 2 };
        double costFunction[] = { -5.0, -3.0, -7.0 };

        memcpy( tableau.nextCostFunction, costFunction, sizeof(costFunction) );

        TS_ASSERT_THROWS_NOTHING( pse.select( tableau, candidates, excluded ) );

        // The largest cost^2/gamma belongs to variable #2, so it should enter
        TS_ASSERT_EQUALS( tableau.mockEnteringVariable, 2U );

        // Assume variable #2 is hopping to opposite variable. Both hooks should not
        // change gamma.
        tableau.mockLeavingVariable = 2;
        bool fakePivot = true;

        TS_ASSERT_THROWS_NOTHING( pse.prePivotHook( tableau, fakePivot ) );
        for ( unsigned i = 0; i < 3; ++i )
            TS_ASSERT_EQUALS( pse.getGamma( i ), 1.0 );

        TS_ASSERT_THROWS_NOTHING( pse.postPivotHook( tableau, fakePivot ) );
        for ( unsigned i = 0; i < 3; ++i )
            TS_ASSERT_EQUALS( pse.getGamma( i ), 1.0 );

        // Second iteration is another fake pivot, with variable #0 hopping to opposite bound.
        candidates.clear();
        candidates.append( 0 );
        candidates.append( 1 );

        TS_ASSERT_THROWS_NOTHING( pse.select( tableau, candidates, excluded ) );
        TS_ASSERT_EQUALS( tableau.mockEnteringVariable, 0U );

        tableau.mockLeavingVariable = 2;
        TS_ASSERT_THROWS_NOTHING( pse.prePivotHook( tableau, fakePivot ) );
        for ( unsigned i = 0; i < 3; ++i )
            TS_ASSERT_EQUALS( pse.getGamma( i ), 1.0 );

        TS_ASSERT_THROWS_NOTHING( pse.postPivotHook( tableau, fakePivot ) );
        for ( unsigned i = 0; i < 3; ++i )
            TS_ASSERT_EQUALS( pse.getGamma( i ), 1.0 );

        // The third iteration has a real pivot.
        candidates.clear();
        candidates.append( 1 );

        TS_ASSERT_THROWS_NOTHING( pse.select( tableau, candidates, excluded ) );
        TS_ASSERT_EQUALS( tableau.mockEnteringVariable, 1U );

        // The entering variable is 1 (index 1), leaving variable is 3 (index 0)
        tableau.nextEnteringVariable = 1;
        tableau.mockLeavingVariable = 3;
        tableau.nextVariableToIndex[1] = 1;
        tableau.nextVariableToIndex[3] = 0;

        // Set basic varaibles' indices
        tableau.nextBasicIndexToVariable[0] = 3;
        tableau.nextBasicIndexToVariable[1] = 4;

        // Prepare the information that pse needs for updating gamme
        double changeColumn[] = { 1, 2 };
        tableau.nextChangeColumn = changeColumn;
        double initialU[] = { 0, 0 };
        double finalU[] = { 0, 0 };
        memcpy( tableau.nextBtranOutput, finalU, sizeof(double) * 2 );

        TableauRow pivotRow( 3 );
        pivotRow._row[0]._coefficient = 3;
        pivotRow._row[1]._coefficient = 1;
        pivotRow._row[2]._coefficient = 1;
        tableau.nextPivotRow = &pivotRow;

        double nextAColumn0[] = { 3, 2 };
        double nextAColumn1[] = { 1, 2 };
        double nextAColumn2[] = { 1, 6 };

        tableau.nextAColumn[0] = nextAColumn0;
        tableau.nextAColumn[1] = nextAColumn1;
        tableau.nextAColumn[2] = nextAColumn2;

        // Check that the gamma values reflect the recent (real) pivot
        fakePivot = false;
        TS_ASSERT_THROWS_NOTHING( pse.prePivotHook( tableau, fakePivot ) );

        TS_ASSERT( FloatUtils::areEqual( pse.getGamma( 0 ), 10.0 ) );
        TS_ASSERT( FloatUtils::areEqual( pse.getGamma( 1 ), 1.0 ) );
        TS_ASSERT( FloatUtils::areEqual( pse.getGamma( 2 ), 2.0 ) );

        TS_ASSERT_SAME_DATA( tableau.lastBtranInput, initialU, sizeof(double) * 2 );

        TS_ASSERT_THROWS_NOTHING( pse.postPivotHook( tableau, fakePivot ) );
        TS_ASSERT( FloatUtils::areEqual( pse.getGamma( 0 ), 10.0 ) );
        TS_ASSERT( FloatUtils::areEqual( pse.getGamma( 1 ), 1.0 ) );
        TS_ASSERT( FloatUtils::areEqual( pse.getGamma( 2 ), 2.0 ) );

        // Fourth iteration, also with a real pivot
        candidates.clear();
        candidates.append( 0 );
        candidates.append( 1 );

        costFunction[0] = 4.0; costFunction[1] = -2.0; costFunction[2] = -4.0;
        memcpy( tableau.nextCostFunction, costFunction, sizeof(costFunction) );

        TS_ASSERT_THROWS_NOTHING( pse.select( tableau, candidates, excluded ) );
        TS_ASSERT_EQUALS( tableau.mockEnteringVariable, 1U );

        // The entering variable is 3 (index 1), leaving variable is 4 (index 1)
        tableau.nextEnteringVariable = 3;
        tableau.mockLeavingVariable = 4;
        tableau.nextVariableToIndex[4] = 1;
        tableau.nextVariableToIndex[3] = 1;

        // Set basic varaibles' indices
        tableau.nextBasicIndexToVariable[0] = 1;
        tableau.nextBasicIndexToVariable[1] = 4;

        // Prepare the information that pse needs for updating gamme
        changeColumn[0] = 1; changeColumn[1] = 2;
        tableau.nextChangeColumn = changeColumn;
        initialU[0] = 1; initialU[1] = 0;
        finalU[0] = -1; finalU[1] = 0;
        memcpy( tableau.nextBtranOutput, finalU, sizeof(double) * 2 );

        pivotRow._row[0]._coefficient = -4;
        pivotRow._row[1]._coefficient = 2;
        pivotRow._row[2]._coefficient = 4;
        tableau.nextPivotRow = &pivotRow;

        nextAColumn1[0] = 1; nextAColumn1[1] = 0;

        // Check that the gamma values reflect the recent (real) pivot
        fakePivot = false;
        TS_ASSERT_THROWS_NOTHING( pse.prePivotHook( tableau, fakePivot ) );

        TS_ASSERT( FloatUtils::areEqual( pse.getGamma( 0 ), 2.0 ) );
        TS_ASSERT( FloatUtils::areEqual( pse.getGamma( 1 ), 0.25 ) );
        TS_ASSERT( FloatUtils::areEqual( pse.getGamma( 2 ), 10.0 ) );

        TS_ASSERT( FloatUtils::areEqual( tableau.lastBtranInput[0], -1 ) );
        TS_ASSERT( FloatUtils::areEqual( tableau.lastBtranInput[1], 0 ) );

        TS_ASSERT_THROWS_NOTHING( pse.postPivotHook( tableau, fakePivot ) );
        TS_ASSERT( FloatUtils::areEqual( pse.getGamma( 0 ), 2.0 ) );
        TS_ASSERT( FloatUtils::areEqual( pse.getGamma( 1 ), 0.25 ) );
        TS_ASSERT( FloatUtils::areEqual( pse.getGamma( 2 ), 10.0 ) );
    }
};

//
// Local Variables:
// compile-command: "make -C ../../.. "
// tags-file-name: "../../../TAGS"
// c-basic-offset: 4
// End:
//
