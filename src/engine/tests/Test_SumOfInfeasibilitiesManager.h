/*********************                                                        */
/*! \file Test_SumOfInfeasibilitiesManager.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Haoze Wu
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

**/

#include <cxxtest/TestSuite.h>

#include "InputQuery.h"
#include "LinearExpression.h"
#include "MaxConstraint.h"
#include "MockTableau.h"
#include "Options.h"
#include "ReluConstraint.h"
#include "SumOfInfeasibilitiesManager.h"

#include "Vector.h"

class SumOfInfeasibilitiesManagerTestSuite : public CxxTest::TestSuite
{
public:

    void setUp()
    {
    }

    void tearDown()
    {
    }

    void createInputQuery( InputQuery &ipq,
                           Vector<PiecewiseLinearConstraint *> &plConstraints )
    {
        /*  R
          0 -- 1
            R      \
          2 -- 3 ---  6
            R      /
          4 -- 5
        */
        ipq.setNumberOfVariables(6);
        ReluConstraint *relu1 = new ReluConstraint(0,1);
        ReluConstraint *relu2 = new ReluConstraint(2,3);
        ReluConstraint *relu3 = new ReluConstraint(4,5);
        MaxConstraint *max1 = new MaxConstraint(6, {1,3,5});

        ipq.addPiecewiseLinearConstraint(relu1);
        ipq.addPiecewiseLinearConstraint(relu2);
        ipq.addPiecewiseLinearConstraint(relu3);
        ipq.addPiecewiseLinearConstraint(max1);

        for ( unsigned i = 0; i < 6; ++i )
        {
            ipq.setLowerBound( i, -2 );
            ipq.setUpperBound( i, 2 );
        }

        ipq.setLowerBound( 1, 0 );
        ipq.setLowerBound( 3, 0 );
        ipq.setLowerBound( 5, 0 );

        ipq.markInputVariable( 0, 0 );
        ipq.markInputVariable( 2, 1 );
        ipq.markInputVariable( 4, 2 );

        plConstraints = {relu1, relu2, relu3, max1};

        for ( const auto &c : plConstraints )
        {
            for ( const auto &var : c->getParticipatingVariables() )
            {
                c->notifyLowerBound( var, -2 );
                c->notifyUpperBound( var, 2 );
            }
        }
        relu1->notifyLowerBound( 1, 0 );
        relu2->notifyLowerBound( 3, 0 );
        relu3->notifyLowerBound( 5, 0 );

        TS_ASSERT( ipq.constructNetworkLevelReasoner() );
        ipq.getNetworkLevelReasoner()->dumpTopology();
    }

    void test_initialize_phase_pattern_with_input_assignment1()
    {
        InputQuery ipq;
        Vector<PiecewiseLinearConstraint *> plConstraints;
        createInputQuery( ipq, plConstraints );
        MockTableau tableau;
        ipq.getNetworkLevelReasoner()->setTableau( &tableau );

        Options::get()->setString
            ( Options::SOI_INITIALIZATION_STRATEGY, "input-assignment" );

        std::unique_ptr<SumOfInfeasibilitiesManager> soiManager;
        TS_ASSERT_THROWS_NOTHING
            ( soiManager =
              std::unique_ptr<SumOfInfeasibilitiesManager>
              ( new SumOfInfeasibilitiesManager( ipq ) ) );

        tableau.nextValues[0] = -1;
        tableau.nextValues[2] = 1;
        tableau.nextValues[4] = 2;
        plConstraints[0]->notifyVariableValue( 0, -1 );
        plConstraints[0]->notifyVariableValue( 1, 0 );
        plConstraints[1]->notifyVariableValue( 2, 1 );
        plConstraints[1]->notifyVariableValue( 3, 1 );
        plConstraints[2]->notifyVariableValue( 4, 2 );
        plConstraints[2]->notifyVariableValue( 5, 2 );
        plConstraints[3]->notifyVariableValue( 1, 0 );
        plConstraints[3]->notifyVariableValue( 3, 1 );
        plConstraints[3]->notifyVariableValue( 5, 2 );
        plConstraints[3]->notifyVariableValue( 6, 2 );

        // The input assignment is [-1, 1, 2], the output of the max should be 2
        TS_ASSERT_THROWS_NOTHING
            (soiManager->initializePhasePattern() );

        LinearExpression cost;
        TS_ASSERT_THROWS_NOTHING( plConstraints[0]->getCostFunctionComponent
                                  ( cost, RELU_PHASE_INACTIVE ) );
        TS_ASSERT_THROWS_NOTHING( plConstraints[1]->getCostFunctionComponent
                                  ( cost, RELU_PHASE_ACTIVE ) );
        TS_ASSERT_THROWS_NOTHING( plConstraints[2]->getCostFunctionComponent
                                  ( cost, RELU_PHASE_ACTIVE ) );
        List<PhaseStatus> phases = plConstraints[3]->getAllCases();
        TS_ASSERT_THROWS_NOTHING( plConstraints[3]->getCostFunctionComponent
                                  ( cost, *( ++( ++phases.begin() ) ) ) );
        cost.dump();
        TS_ASSERT_EQUALS( cost, soiManager->getSoIPhasePattern() );
    }

    void test_initialize_phase_pattern_with_input_assignment2()
    {
        InputQuery ipq;
        Vector<PiecewiseLinearConstraint *> plConstraints;
        createInputQuery( ipq, plConstraints );
        MockTableau tableau;
        ipq.getNetworkLevelReasoner()->setTableau( &tableau );

        Options::get()->setString
            ( Options::SOI_INITIALIZATION_STRATEGY, "input-assignment" );

        std::unique_ptr<SumOfInfeasibilitiesManager> soiManager;
        TS_ASSERT_THROWS_NOTHING
            ( soiManager =
              std::unique_ptr<SumOfInfeasibilitiesManager>
              ( new SumOfInfeasibilitiesManager( ipq ) ) );

        tableau.nextValues[0] = 1;
        tableau.nextValues[2] = 2;
        tableau.nextValues[4] = -1;
        plConstraints[0]->notifyVariableValue( 0, 1 );
        plConstraints[0]->notifyVariableValue( 1, 1 );

        // Phase is fixed, won't add the second relu to SoI
        plConstraints[1]->notifyLowerBound( 2, 2 );
        plConstraints[1]->notifyVariableValue( 2, 2 );
        plConstraints[1]->notifyVariableValue( 3, 2 );

        plConstraints[2]->notifyVariableValue( 4, -1 );
        plConstraints[2]->notifyVariableValue( 5, 0 );

        // Eliminate the variable from the max constraint
        plConstraints[3]->eliminateVariable( 3, 2 );
        ipq.getNetworkLevelReasoner()->eliminateVariable( 3, 2 );

        plConstraints[3]->notifyVariableValue( 1, 1 );
        plConstraints[3]->notifyVariableValue( 5, 0 );
        plConstraints[3]->notifyVariableValue( 6, 2 );

        // The input assignment is [-1, 1, 2], the output of the max should be 2
        TS_ASSERT_THROWS_NOTHING
            (soiManager->initializePhasePattern() );

        LinearExpression cost;
        TS_ASSERT_THROWS_NOTHING( plConstraints[0]->getCostFunctionComponent
                                  ( cost, RELU_PHASE_ACTIVE ) );
        TS_ASSERT_THROWS_NOTHING( plConstraints[2]->getCostFunctionComponent
                                  ( cost, RELU_PHASE_INACTIVE ) );
        TS_ASSERT_THROWS_NOTHING( plConstraints[3]->getCostFunctionComponent
                                  ( cost, MAX_PHASE_ELIMINATED ) );
        cost.dump();
        TS_ASSERT_EQUALS( cost, soiManager->getSoIPhasePattern() );
    }
};
