/*********************                                                        */
/*! \file Test_SumOfInfeasibilitiesManager.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Haoze Wu
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2024 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

**/

#include "LinearExpression.h"
#include "MaxConstraint.h"
#include "MockErrno.h"
#include "MockTableau.h"
#include "Options.h"
#include "Query.h"
#include "ReluConstraint.h"
#include "SumOfInfeasibilitiesManager.h"
#include "Vector.h"

#include <cxxtest/TestSuite.h>

class MockForSoI : public T::Base_rand
{
public:
    MockForSoI()
    {
        randWasCalled = 0;
    }

    unsigned randWasCalled;
    int nextRandValue;

    int rand()
    {
        ++randWasCalled;
        return nextRandValue;
    }
};

class SumOfInfeasibilitiesManagerTestSuite : public CxxTest::TestSuite
{
public:
    MockForSoI *mock; // random

    void setUp()
    {
        TS_ASSERT( mock = new MockForSoI );
    }

    void tearDown()
    {
        TS_ASSERT_THROWS_NOTHING( delete mock );
    }

    void
    createQuery( Query &ipq, Vector<PiecewiseLinearConstraint *> &plConstraints, ITableau &tableau )
    {
        /*  R
          0 -- 1
            R      \
          2 -- 3 ---  6
            R      /
          4 -- 5
        */
        ipq.setNumberOfVariables( 7 );
        ReluConstraint *relu1 = new ReluConstraint( 0, 1 );
        ReluConstraint *relu2 = new ReluConstraint( 2, 3 );
        ReluConstraint *relu3 = new ReluConstraint( 4, 5 );
        MaxConstraint *max1 = new MaxConstraint( 6, { 1, 3, 5 } );

        max1->transformToUseAuxVariables( ipq );

        ipq.addPiecewiseLinearConstraint( relu1 );
        ipq.addPiecewiseLinearConstraint( relu2 );
        ipq.addPiecewiseLinearConstraint( relu3 );
        ipq.addPiecewiseLinearConstraint( max1 );

        for ( unsigned i = 0; i <= 6; ++i )
        {
            ipq.setLowerBound( i, -3 );
            ipq.setUpperBound( i, 3 );
        }

        ipq.setLowerBound( 1, 0 );
        ipq.setLowerBound( 3, 0 );
        ipq.setLowerBound( 5, 0 );

        ipq.markInputVariable( 0, 0 );
        ipq.markInputVariable( 2, 1 );
        ipq.markInputVariable( 4, 2 );

        plConstraints = { relu1, relu2, relu3, max1 };

        for ( const auto &c : plConstraints )
        {
            c->registerTableau( &tableau );
            for ( const auto &var : c->getParticipatingVariables() )
            {
                c->notifyLowerBound( var, -3 );
                c->notifyUpperBound( var, 3 );
            }
        }
        relu1->notifyLowerBound( 1, 0 );
        relu2->notifyLowerBound( 3, 0 );
        relu3->notifyLowerBound( 5, 0 );

        // Aux vars are 7, 8, 9
        for ( unsigned aux = 7; aux <= 9; ++aux )
        {
            max1->notifyLowerBound( aux, 0 );
            max1->notifyUpperBound( aux, 3 );
        }

        List<Equation> unhandledEquations;
        Set<unsigned> varsInUnhandledConstraints;
        TS_ASSERT(
            ipq.constructNetworkLevelReasoner( unhandledEquations, varsInUnhandledConstraints ) );
        TS_ASSERT( unhandledEquations.empty() );
        TS_ASSERT( varsInUnhandledConstraints.empty() );
    }

    void test_initialize_phase_pattern_with_input_assignment1()
    {
        Query ipq;
        Vector<PiecewiseLinearConstraint *> plConstraints;
        MockTableau tableau;
        createQuery( ipq, plConstraints, tableau );
        ipq.getNetworkLevelReasoner()->setTableau( &tableau );

        Options::get()->setString( Options::SOI_INITIALIZATION_STRATEGY, "input-assignment" );

        std::unique_ptr<SumOfInfeasibilitiesManager> soiManager;
        TS_ASSERT_THROWS_NOTHING( soiManager = std::unique_ptr<SumOfInfeasibilitiesManager>(
                                      new SumOfInfeasibilitiesManager( ipq, tableau ) ) );

        tableau.setValue( 0, -1 );
        tableau.setValue( 1, 0 );
        tableau.setValue( 2, 1 );
        tableau.setValue( 3, 1 );
        tableau.setValue( 4, 2 );
        tableau.setValue( 5, 2 );
        tableau.setValue( 6, 2 );
        tableau.setValue( 7, 2 );
        tableau.setValue( 8, 1 );
        tableau.setValue( 9, 0 );

        // The input assignment is [-1, 1, 2], the output of the max should be 2
        TS_ASSERT_THROWS_NOTHING( soiManager->initializePhasePattern() );

        // So the cost compoenents in the SoI should be:
        // relu1: inactive, relu2: active, relu3: active, max: third max input
        LinearExpression cost;
        TS_ASSERT_THROWS_NOTHING(
            plConstraints[0]->getCostFunctionComponent( cost, RELU_PHASE_INACTIVE ) );
        TS_ASSERT_THROWS_NOTHING(
            plConstraints[1]->getCostFunctionComponent( cost, RELU_PHASE_ACTIVE ) );
        TS_ASSERT_THROWS_NOTHING(
            plConstraints[2]->getCostFunctionComponent( cost, RELU_PHASE_ACTIVE ) );
        List<PhaseStatus> phases = plConstraints[3]->getAllCases();
        TS_ASSERT_THROWS_NOTHING(
            plConstraints[3]->getCostFunctionComponent( cost, *( ++( ++phases.begin() ) ) ) );
        TS_ASSERT_EQUALS( cost, soiManager->getCurrentSoIPhasePattern() );
        TS_ASSERT_EQUALS( cost, soiManager->getLastAcceptedSoIPhasePattern() );
        TS_ASSERT_EQUALS( soiManager->getConstraintsUpdatedInLastProposal().size(), 0u );
    }

    void test_initialize_phase_pattern_with_input_assignment2()
    {
        Query ipq;
        Vector<PiecewiseLinearConstraint *> plConstraints;
        MockTableau tableau;
        createQuery( ipq, plConstraints, tableau );
        ipq.getNetworkLevelReasoner()->setTableau( &tableau );

        Options::get()->setString( Options::SOI_INITIALIZATION_STRATEGY, "input-assignment" );

        std::unique_ptr<SumOfInfeasibilitiesManager> soiManager;
        TS_ASSERT_THROWS_NOTHING( soiManager = std::unique_ptr<SumOfInfeasibilitiesManager>(
                                      new SumOfInfeasibilitiesManager( ipq, tableau ) ) );

        // Phase is fixed, won't add the second relu to SoI
        tableau.setValue( 0, 1 );
        tableau.setValue( 1, 1 );
        tableau.setValue( 2, 2 );
        tableau.setValue( 3, 2 );
        tableau.setValue( 4, -1 );
        tableau.setValue( 5, 0 );
        tableau.setValue( 6, 2 );
        tableau.setValue( 7, 1 );
        tableau.setValue( 9, 0 );

        plConstraints[1]->notifyLowerBound( 2, 2 );

        // Eliminate the variable from the max constraint
        plConstraints[3]->eliminateVariable( 3, 2 );
        ipq.getNetworkLevelReasoner()->eliminateVariable( 3, 2 );

        // The input assignment is [1, 2, -1], the output of the max constraint
        // should be 2
        // So the cost compoenents in the SoI should be:
        // relu1: active, relu3: inactive, max: PHASE_ELIMINATED
        TS_ASSERT_THROWS_NOTHING( soiManager->initializePhasePattern() );

        LinearExpression cost;
        TS_ASSERT_THROWS_NOTHING(
            plConstraints[0]->getCostFunctionComponent( cost, RELU_PHASE_ACTIVE ) );
        TS_ASSERT_THROWS_NOTHING(
            plConstraints[2]->getCostFunctionComponent( cost, RELU_PHASE_INACTIVE ) );
        TS_ASSERT_THROWS_NOTHING(
            plConstraints[3]->getCostFunctionComponent( cost, MAX_PHASE_ELIMINATED ) );

        TS_ASSERT_EQUALS( cost, soiManager->getCurrentSoIPhasePattern() );
    }

    void test_initialize_phase_pattern_with_current_assignment()
    {
        Query ipq;
        Vector<PiecewiseLinearConstraint *> plConstraints;
        MockTableau tableau;
        createQuery( ipq, plConstraints, tableau );
        ipq.getNetworkLevelReasoner()->setTableau( &tableau );

        Options::get()->setString( Options::SOI_INITIALIZATION_STRATEGY, "current-assignment" );

        std::unique_ptr<SumOfInfeasibilitiesManager> soiManager;
        TS_ASSERT_THROWS_NOTHING( soiManager = std::unique_ptr<SumOfInfeasibilitiesManager>(
                                      new SumOfInfeasibilitiesManager( ipq, tableau ) ) );

        tableau.nextValues[0] = -1;
        tableau.nextValues[1] = 0;
        tableau.nextValues[2] = 1;
        tableau.nextValues[3] = 2;
        tableau.nextValues[4] = 2;
        tableau.nextValues[5] = 2;
        tableau.nextValues[6] = 2;
        tableau.nextValues[7] = 2;
        tableau.nextValues[8] = 1;
        tableau.nextValues[9] = 0;

        // The input assignment is [-1, 1, 2], the output of the max should be 2
        TS_ASSERT_THROWS_NOTHING( soiManager->initializePhasePattern() );

        // So the cost compoenents in the SoI should be:
        // relu1: inactive, relu2: active, relu3: active, max: third max input
        LinearExpression cost;
        TS_ASSERT_THROWS_NOTHING(
            plConstraints[0]->getCostFunctionComponent( cost, RELU_PHASE_INACTIVE ) );
        TS_ASSERT_THROWS_NOTHING(
            plConstraints[1]->getCostFunctionComponent( cost, RELU_PHASE_ACTIVE ) );
        TS_ASSERT_THROWS_NOTHING(
            plConstraints[2]->getCostFunctionComponent( cost, RELU_PHASE_ACTIVE ) );
        List<PhaseStatus> phases = plConstraints[3]->getAllCases();
        TS_ASSERT_THROWS_NOTHING(
            plConstraints[3]->getCostFunctionComponent( cost, *( ++phases.begin() ) ) );
        TS_ASSERT_EQUALS( cost, soiManager->getCurrentSoIPhasePattern() );
        TS_ASSERT_EQUALS( cost, soiManager->getLastAcceptedSoIPhasePattern() );
        TS_ASSERT_EQUALS( soiManager->getConstraintsUpdatedInLastProposal().size(), 0u );
    }

    void test_propose_phase_pattern_update_randomly()
    {
        Query ipq;
        Vector<PiecewiseLinearConstraint *> plConstraints;
        MockTableau tableau;
        createQuery( ipq, plConstraints, tableau );
        ipq.getNetworkLevelReasoner()->setTableau( &tableau );
        tableau.nextValues[0] = -1;
        tableau.nextValues[1] = 1;
        tableau.nextValues[2] = 1;
        tableau.nextValues[3] = 2;
        tableau.nextValues[4] = 2;
        tableau.nextValues[5] = 2;
        tableau.nextValues[6] = 2;
        tableau.nextValues[7] = 2;
        tableau.nextValues[8] = 0;
        tableau.nextValues[9] = 0;

        Options::get()->setString( Options::SOI_INITIALIZATION_STRATEGY, "input-assignment" );
        Options::get()->setString( Options::SOI_SEARCH_STRATEGY, "mcmc" );

        std::unique_ptr<SumOfInfeasibilitiesManager> soiManager;
        TS_ASSERT_THROWS_NOTHING( soiManager = std::unique_ptr<SumOfInfeasibilitiesManager>(
                                      new SumOfInfeasibilitiesManager( ipq, tableau ) ) );

        TS_ASSERT_THROWS_NOTHING( soiManager->initializePhasePattern() );
        TS_ASSERT_THROWS_NOTHING(
            soiManager->setPLConstraintsInCurrentPhasePattern( plConstraints ) );

        for ( const auto &plConstraint : plConstraints )
        {
            soiManager->setPhaseStatusInLastAcceptedPhasePattern(
                plConstraint, *( plConstraint->getAllCases().begin() ) );
        }

        mock->randWasCalled = 0;
        mock->nextRandValue = 1;
        TS_ASSERT_THROWS_NOTHING( soiManager->proposePhasePatternUpdate() );
        TS_ASSERT_EQUALS( mock->randWasCalled, 1u );

        // The cost term of the second relu is flipped.
        LinearExpression cost1;
        TS_ASSERT_THROWS_NOTHING( plConstraints[0]->getCostFunctionComponent(
            cost1, *( plConstraints[0]->getAllCases().begin() ) ) );
        TS_ASSERT_THROWS_NOTHING( plConstraints[1]->getCostFunctionComponent(
            cost1, *( ++( plConstraints[1]->getAllCases().begin() ) ) ) );
        TS_ASSERT_THROWS_NOTHING( plConstraints[2]->getCostFunctionComponent(
            cost1, *( plConstraints[2]->getAllCases().begin() ) ) );
        TS_ASSERT_THROWS_NOTHING( plConstraints[3]->getCostFunctionComponent(
            cost1, *( plConstraints[3]->getAllCases().begin() ) ) );


        TS_ASSERT_EQUALS( soiManager->getConstraintsUpdatedInLastProposal().size(), 1u );
        TS_ASSERT_EQUALS( *soiManager->getConstraintsUpdatedInLastProposal().begin(),
                          plConstraints[1] );

        TS_ASSERT_EQUALS( cost1, soiManager->getCurrentSoIPhasePattern() );

        mock->nextRandValue = 7;
        TS_ASSERT_THROWS_NOTHING( soiManager->proposePhasePatternUpdate() );
        TS_ASSERT_EQUALS( mock->randWasCalled, 3u );

        // The cost term of the third constraint (max) is updated,
        // because 7 % 4 = 3. The updated phase status corresponds to the third
        // input variable to max, because there are two alternative phase
        // statuses and 7 % 2 = 1.

        LinearExpression cost2;
        TS_ASSERT_THROWS_NOTHING( plConstraints[0]->getCostFunctionComponent(
            cost2, *( plConstraints[0]->getAllCases().begin() ) ) );
        TS_ASSERT_THROWS_NOTHING( plConstraints[1]->getCostFunctionComponent(
            cost2, *( plConstraints[1]->getAllCases().begin() ) ) );
        TS_ASSERT_THROWS_NOTHING( plConstraints[2]->getCostFunctionComponent(
            cost2, *( plConstraints[2]->getAllCases().begin() ) ) );
        TS_ASSERT_THROWS_NOTHING( plConstraints[3]->getCostFunctionComponent(
            cost2, *( ++( ++( plConstraints[3]->getAllCases().begin() ) ) ) ) );

        TS_ASSERT_EQUALS( cost2, soiManager->getCurrentSoIPhasePattern() );

        TS_ASSERT_EQUALS( soiManager->getConstraintsUpdatedInLastProposal().size(), 1u );
        TS_ASSERT_EQUALS( *soiManager->getConstraintsUpdatedInLastProposal().begin(),
                          plConstraints[3] );

        TS_ASSERT_THROWS_NOTHING( soiManager->acceptCurrentPhasePattern() );

        TS_ASSERT_EQUALS( cost2, soiManager->getLastAcceptedSoIPhasePattern() );
        TS_ASSERT_EQUALS( cost2, soiManager->getCurrentSoIPhasePattern() );

        TS_ASSERT_EQUALS( soiManager->getConstraintsUpdatedInLastProposal().size(), 0u );
    }

    void test_propose_phase_pattern_update_walksat()
    {
        Query ipq;
        Vector<PiecewiseLinearConstraint *> plConstraints;
        MockTableau tableau;
        createQuery( ipq, plConstraints, tableau );
        ipq.getNetworkLevelReasoner()->setTableau( &tableau );
        tableau.nextValues[0] = -2;
        tableau.nextValues[1] = 0.5;
        tableau.nextValues[2] = 1;
        tableau.nextValues[3] = 2;
        tableau.nextValues[4] = 2;
        tableau.nextValues[5] = 2;
        tableau.nextValues[6] = 2.5;
        tableau.nextValues[7] = 2;
        tableau.nextValues[8] = 0.5;
        tableau.nextValues[9] = 0.5;

        Options::get()->setString( Options::SOI_INITIALIZATION_STRATEGY, "input-assignment" );
        Options::get()->setString( Options::SOI_SEARCH_STRATEGY, "walksat" );

        std::unique_ptr<SumOfInfeasibilitiesManager> soiManager;
        TS_ASSERT_THROWS_NOTHING( soiManager = std::unique_ptr<SumOfInfeasibilitiesManager>(
                                      new SumOfInfeasibilitiesManager( ipq, tableau ) ) );

        TS_ASSERT_THROWS_NOTHING( soiManager->initializePhasePattern() );
        TS_ASSERT_THROWS_NOTHING( soiManager->obtainCurrentAssignment() );

        soiManager->setPhaseStatusInLastAcceptedPhasePattern( plConstraints[0], RELU_PHASE_ACTIVE );
        soiManager->setPhaseStatusInLastAcceptedPhasePattern( plConstraints[1],
                                                              RELU_PHASE_INACTIVE );
        soiManager->setPhaseStatusInLastAcceptedPhasePattern( plConstraints[2], RELU_PHASE_ACTIVE );
        soiManager->setPhaseStatusInLastAcceptedPhasePattern(
            plConstraints[3], *( plConstraints[3]->getAllCases().begin() ) );

        // Reduced cost for relu1: 2, for relu2: 1, for relu3: -2,
        // for max: 1.5. So pick relu1.
        TS_ASSERT_THROWS_NOTHING( soiManager->proposePhasePatternUpdate() );

        // The cost term of the second relu is flipped.
        LinearExpression cost1;
        TS_ASSERT_THROWS_NOTHING(
            plConstraints[0]->getCostFunctionComponent( cost1, RELU_PHASE_INACTIVE ) );
        TS_ASSERT_THROWS_NOTHING(
            plConstraints[1]->getCostFunctionComponent( cost1, RELU_PHASE_INACTIVE ) );
        TS_ASSERT_THROWS_NOTHING(
            plConstraints[2]->getCostFunctionComponent( cost1, RELU_PHASE_ACTIVE ) );
        TS_ASSERT_THROWS_NOTHING( plConstraints[3]->getCostFunctionComponent(
            cost1, *( plConstraints[3]->getAllCases().begin() ) ) );

        TS_ASSERT_EQUALS( cost1, soiManager->getCurrentSoIPhasePattern() );

        TS_ASSERT_EQUALS( soiManager->getConstraintsUpdatedInLastProposal().size(), 1u );
        TS_ASSERT_EQUALS( *soiManager->getConstraintsUpdatedInLastProposal().begin(),
                          plConstraints[0] );


        tableau.setValue( 0, 0 );

        // Reduced cost for relu1: 0, for relu2: 1, for relu3: -2,
        // for max: 1.5. So pick max with phase corresponding to the second input.
        TS_ASSERT_THROWS_NOTHING( soiManager->proposePhasePatternUpdate() );

        // The cost term of the second relu is flipped.
        LinearExpression cost2;
        TS_ASSERT_THROWS_NOTHING(
            plConstraints[0]->getCostFunctionComponent( cost2, RELU_PHASE_ACTIVE ) );
        TS_ASSERT_THROWS_NOTHING(
            plConstraints[1]->getCostFunctionComponent( cost2, RELU_PHASE_INACTIVE ) );
        TS_ASSERT_THROWS_NOTHING(
            plConstraints[2]->getCostFunctionComponent( cost2, RELU_PHASE_ACTIVE ) );
        TS_ASSERT_THROWS_NOTHING( plConstraints[3]->getCostFunctionComponent(
            cost2, *( ++plConstraints[3]->getAllCases().begin() ) ) );

        TS_ASSERT_EQUALS( cost2, soiManager->getCurrentSoIPhasePattern() );

        TS_ASSERT_EQUALS( soiManager->getConstraintsUpdatedInLastProposal().size(), 1u );
        TS_ASSERT_EQUALS( *soiManager->getConstraintsUpdatedInLastProposal().begin(),
                          plConstraints[3] );
    }

    void test_decide_to_accept_current_proposal()
    {
        Query ipq;
        MockTableau tableau;

        // Set beta to 5.
        Options::get()->setFloat( Options::PROBABILITY_DENSITY_PARAMETER, 5 );

        std::unique_ptr<SumOfInfeasibilitiesManager> soiManager;
        TS_ASSERT_THROWS_NOTHING( soiManager = std::unique_ptr<SumOfInfeasibilitiesManager>(
                                      new SumOfInfeasibilitiesManager( ipq, tableau ) ) );

        mock->randWasCalled = 0;
        // Only accept if the probability to accept is larger than 80%.
        mock->nextRandValue = (int)RAND_MAX * 0.8;
        double costOfLastAcceptedPhasePattern = 10;
        double costOfProposedPhasePattern = 9;
        TS_ASSERT( soiManager->decideToAcceptCurrentProposal( costOfLastAcceptedPhasePattern,
                                                              costOfProposedPhasePattern ) );
        // Always accept if the new cost is lower.
        TS_ASSERT_EQUALS( mock->randWasCalled, 0u );

        costOfProposedPhasePattern = 10.1;
        // Prob. to accept is e^( -beta * (10.5 - 10)) ~= 60%, thus rejected.
        TS_ASSERT( !soiManager->decideToAcceptCurrentProposal( costOfLastAcceptedPhasePattern,
                                                               costOfProposedPhasePattern ) );
        TS_ASSERT_EQUALS( mock->randWasCalled, 1u );

        // Only accept if the probability to accept is larger than 40%.
        mock->nextRandValue = (int)RAND_MAX * 0.4;

        // Prob. to accept is still ~60%, thus accepted.
        TS_ASSERT( soiManager->decideToAcceptCurrentProposal( costOfLastAcceptedPhasePattern,
                                                              costOfProposedPhasePattern ) );
        TS_ASSERT_EQUALS( mock->randWasCalled, 2u );

        costOfProposedPhasePattern = 10.5;
        // Accept with prob. e^( -beta * (10.5 - 10)) ~= 8.2%, thus rejected.
        TS_ASSERT( !soiManager->decideToAcceptCurrentProposal( costOfLastAcceptedPhasePattern,
                                                               costOfProposedPhasePattern ) );
        TS_ASSERT_EQUALS( mock->randWasCalled, 3u );
    }

    void test_update_current_phase_pattern_for_satisfied_pl_constraints()
    {
        Query ipq;
        Vector<PiecewiseLinearConstraint *> plConstraints;
        MockTableau tableau;
        createQuery( ipq, plConstraints, tableau );
        ipq.getNetworkLevelReasoner()->setTableau( &tableau );

        Options::get()->setString( Options::SOI_INITIALIZATION_STRATEGY, "input-assignment" );

        std::unique_ptr<SumOfInfeasibilitiesManager> soiManager;
        TS_ASSERT_THROWS_NOTHING( soiManager = std::unique_ptr<SumOfInfeasibilitiesManager>(
                                      new SumOfInfeasibilitiesManager( ipq, tableau ) ) );

        // relu1, relu2 satisfied, relu3 not satisfied, max not satisfied.
        tableau.nextValues[0] = -1;
        tableau.nextValues[1] = 0;
        tableau.nextValues[2] = 1;
        tableau.nextValues[3] = 1;
        tableau.nextValues[4] = 1;
        tableau.nextValues[5] = 1.5;
        tableau.nextValues[6] = 2.5;
        tableau.nextValues[7] = 2.5;
        tableau.nextValues[8] = 1.5;
        tableau.nextValues[9] = 1;


        TS_ASSERT_THROWS_NOTHING( soiManager->initializePhasePattern() );

        for ( const auto &plConstraint : plConstraints )
        {
            soiManager->setPhaseStatusInCurrentPhasePattern(
                plConstraint, *( plConstraint->getAllCases().begin() ) );
        }

        TS_ASSERT_THROWS_NOTHING(
            soiManager->updateCurrentPhasePatternForSatisfiedPLConstraints() );

        LinearExpression cost;
        TS_ASSERT_THROWS_NOTHING(
            plConstraints[0]->getCostFunctionComponent( cost, RELU_PHASE_INACTIVE ) );
        TS_ASSERT_THROWS_NOTHING(
            plConstraints[1]->getCostFunctionComponent( cost, RELU_PHASE_ACTIVE ) );
        TS_ASSERT_THROWS_NOTHING( plConstraints[2]->getCostFunctionComponent(
            cost, *( plConstraints[2]->getAllCases().begin() ) ) );
        TS_ASSERT_THROWS_NOTHING( plConstraints[3]->getCostFunctionComponent(
            cost, *( plConstraints[3]->getAllCases().begin() ) ) );

        TS_ASSERT_EQUALS( cost, soiManager->getCurrentSoIPhasePattern() );
    }

    void test_remove_cost_component_from_heuristic_cost()
    {
        Query ipq;
        Vector<PiecewiseLinearConstraint *> plConstraints;
        MockTableau tableau;
        createQuery( ipq, plConstraints, tableau );
        ipq.getNetworkLevelReasoner()->setTableau( &tableau );

        Options::get()->setString( Options::SOI_INITIALIZATION_STRATEGY, "input-assignment" );

        std::unique_ptr<SumOfInfeasibilitiesManager> soiManager;
        TS_ASSERT_THROWS_NOTHING( soiManager = std::unique_ptr<SumOfInfeasibilitiesManager>(
                                      new SumOfInfeasibilitiesManager( ipq, tableau ) ) );

        // relu1, relu2 satisfied, relu3 not satisfied, max not satisfied.
        tableau.setValue( 0, -1 );
        tableau.setValue( 1, 0 );
        tableau.setValue( 2, 1 );
        tableau.setValue( 3, 1 );
        tableau.setValue( 4, 1 );
        tableau.setValue( 5, 1.5 );
        tableau.setValue( 6, 2.5 );
        tableau.setValue( 7, 2.5 );
        tableau.setValue( 8, 1.5 );
        tableau.setValue( 9, 1 );

        TS_ASSERT_THROWS_NOTHING( soiManager->initializePhasePattern() );

        for ( const auto &plConstraint : plConstraints )
        {
            soiManager->setPhaseStatusInCurrentPhasePattern(
                plConstraint, *( plConstraint->getAllCases().begin() ) );
        }

        TS_ASSERT_THROWS_NOTHING(
            soiManager->removeCostComponentFromHeuristicCost( plConstraints[0] ) );
        TS_ASSERT_THROWS_NOTHING(
            soiManager->removeCostComponentFromHeuristicCost( plConstraints[3] ) );


        LinearExpression cost;
        TS_ASSERT_THROWS_NOTHING( plConstraints[1]->getCostFunctionComponent(
            cost, *( plConstraints[1]->getAllCases().begin() ) ) );
        TS_ASSERT_THROWS_NOTHING( plConstraints[2]->getCostFunctionComponent(
            cost, *( plConstraints[2]->getAllCases().begin() ) ) );
        TS_ASSERT_EQUALS( cost, soiManager->getCurrentSoIPhasePattern() );

        // Reinitialize.
        TS_ASSERT_THROWS_NOTHING( soiManager->initializePhasePattern() );
        for ( const auto &plConstraint : plConstraints )
        {
            soiManager->setPhaseStatusInCurrentPhasePattern(
                plConstraint, *( plConstraint->getAllCases().begin() ) );
        }

        TS_ASSERT_THROWS_NOTHING(
            soiManager->removeCostComponentFromHeuristicCost( plConstraints[0] ) );
        TS_ASSERT_THROWS_NOTHING(
            soiManager->removeCostComponentFromHeuristicCost( plConstraints[3] ) );

        TS_ASSERT_EQUALS( cost, soiManager->getCurrentSoIPhasePattern() );
    }
};
