/*********************                                                        */
/*! \file Test_Engine.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz, Shantanu Thakoor, Derek Huang
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

**/

#include <cxxtest/TestSuite.h>

#include "Engine.h"
#include "InputQuery.h"
#include "MockConstraintBoundTightenerFactory.h"
#include "MockConstraintMatrixAnalyzerFactory.h"
#include "MockCostFunctionManagerFactory.h"
#include "MockErrno.h"
#include "MockProjectedSteepestEdgeFactory.h"
#include "MockRowBoundTightenerFactory.h"
#include "MockTableauFactory.h"
#include "ReluConstraint.h"

#include <string.h>

class MockForEngine :
    public MockTableauFactory,
    public MockProjectedSteepestEdgeRuleFactory,
    public MockRowBoundTightenerFactory,
    public MockConstraintBoundTightenerFactory,
    public MockCostFunctionManagerFactory,
    public MockConstraintMatrixAnalyzerFactory
{
public:
};

class EngineTestSuite : public CxxTest::TestSuite
{
public:
    MockForEngine *mock;
    MockTableau *tableau;
    MockCostFunctionManager *costFunctionManager;
    MockRowBoundTightener *rowTightener;
    MockConstraintBoundTightener *constraintTightener;
    MockConstraintMatrixAnalyzer *constraintMatrixAnalyzer;

    void setUp()
    {
        TS_ASSERT( mock = new MockForEngine );

        tableau = &( mock->mockTableau );
        costFunctionManager = &( mock->mockCostFunctionManager );
        rowTightener = &( mock->mockRowBoundTightener );
        constraintTightener = &( mock->mockConstraintBoundTightener );
        constraintMatrixAnalyzer = &( mock->mockConstraintMatrixAnalyzer );
    }

    void tearDown()
    {
        TS_ASSERT_THROWS_NOTHING( delete mock );
    }

    void test_constructor_destructor()
    {
        Engine *engine = NULL;

        TS_ASSERT_THROWS_NOTHING( engine = new Engine() );

        TS_ASSERT( tableau->wasCreated );
        TS_ASSERT( costFunctionManager->wasCreated );

        TS_ASSERT_THROWS_NOTHING( delete engine );

        TS_ASSERT( tableau->wasDiscarded );
        TS_ASSERT( costFunctionManager->wasDiscarded );
    }

    void test_process_input_query()
    {
        //   0  <= x0 <= 2
        //   -3 <= x1 <= 3
        //   4  <= x2 <= 6
        //
        //  x0 + 2x1 -x2 <= 11 --> x0 + 2x1 - x2 + x3 = 11, 100 >= x3 >= 0
        //  -3x0 + 3x1  >= -5 --> -3x0 + 3x1 + x4 = -5, -100 <= x4 <= 0
        //
        //  aux var x6 added to equation 1, x7 to equation 2

        InputQuery inputQuery;
        inputQuery.setNumberOfVariables( 5 );

        inputQuery.setLowerBound( 0, 0 );
        inputQuery.setUpperBound( 0, 2 );

        inputQuery.setLowerBound( 1, -3 );
        inputQuery.setUpperBound( 1, 3 );

        inputQuery.setLowerBound( 2, 4 );
        inputQuery.setUpperBound( 2, 6 );

        inputQuery.setLowerBound( 3, 0 );
        inputQuery.setUpperBound( 3, 100 );

        inputQuery.setLowerBound( 4, -100 );
        inputQuery.setUpperBound( 4, 0 );

        Equation equation1;
        equation1.addAddend( 1, 0 );
        equation1.addAddend( 2, 1 );
        equation1.addAddend( -1, 2 );
        equation1.addAddend( 1, 3 );
        equation1.setScalar( 11 );
        inputQuery.addEquation( equation1 );

        Equation equation2;
        equation2.addAddend( -3, 0 );
        equation2.addAddend( 3, 1 );
        equation2.addAddend( 1, 4 );
        equation2.setScalar( -5 );
        inputQuery.addEquation( equation2 );

        ReluConstraint *relu1 = new ReluConstraint( 1, 2 );
        ReluConstraint *relu2 = new ReluConstraint( 2, 4 );

        inputQuery.addPiecewiseLinearConstraint( relu1 );
        inputQuery.addPiecewiseLinearConstraint( relu2 );

        constraintMatrixAnalyzer->nextIndependentColumns.append( 0 );
        constraintMatrixAnalyzer->nextIndependentColumns.append( 1 );

        Engine engine;

        TS_ASSERT_THROWS_NOTHING( engine.processInputQuery( inputQuery, false ) );

        TS_ASSERT( tableau->initializeTableauCalled );
        TS_ASSERT( costFunctionManager->initializeWasCalled );
        TS_ASSERT( rowTightener->setDimensionsWasCalled );

        TS_ASSERT_EQUALS( tableau->lastResizeWatchers.size(), 2U );
        TS_ASSERT_EQUALS( *( tableau->lastResizeWatchers.begin() ), rowTightener );
        TS_ASSERT_EQUALS( *( tableau->lastResizeWatchers.rbegin() ), constraintTightener );

        TS_ASSERT_EQUALS( tableau->lastCostFunctionManager, costFunctionManager );

        TS_ASSERT_EQUALS( tableau->lastM, 2U );
        TS_ASSERT_EQUALS( tableau->lastN, 7U );

        // Two basic variables
        TS_ASSERT_EQUALS( tableau->lastBasicVariables.size(), 2U );

        // Right hand side scalars
        TS_ASSERT( tableau->lastRightHandSide );
        TS_ASSERT_EQUALS( tableau->lastRightHandSide[0], 0 );
        TS_ASSERT_EQUALS( tableau->lastRightHandSide[1], 0 );

        // Tableau entries
        TS_ASSERT_EQUALS( tableau->lastEntries[(0*7) + 0], 1 );
        TS_ASSERT_EQUALS( tableau->lastEntries[(0*7) + 1], 2 );
        TS_ASSERT_EQUALS( tableau->lastEntries[(0*7) + 2], -1 );
        TS_ASSERT_EQUALS( tableau->lastEntries[(0*7) + 3], 1 );
        TS_ASSERT_EQUALS( tableau->lastEntries[(0*7) + 4], 0 );
        TS_ASSERT_EQUALS( tableau->lastEntries[(0*7) + 5], -1 );
        TS_ASSERT_EQUALS( tableau->lastEntries[(0*7) + 6], 0 );

        TS_ASSERT_EQUALS( tableau->lastEntries[(1*7) + 0], -3 );
        TS_ASSERT_EQUALS( tableau->lastEntries[(1*7) + 1], 3 );
        TS_ASSERT_EQUALS( tableau->lastEntries[(1*7) + 2], 0 );
        TS_ASSERT_EQUALS( tableau->lastEntries[(1*7) + 3], 0 );
        TS_ASSERT_EQUALS( tableau->lastEntries[(1*7) + 4], 1 );
        TS_ASSERT_EQUALS( tableau->lastEntries[(1*7) + 5], 0 );
        TS_ASSERT_EQUALS( tableau->lastEntries[(1*7) + 6], -1 );

        // Lower and upper bounds
        TS_ASSERT( tableau->lowerBounds.exists( 0 ) );
        TS_ASSERT_EQUALS( tableau->lowerBounds[0], 0.0 );

        TS_ASSERT( tableau->lowerBounds.exists( 1 ) );
        TS_ASSERT_EQUALS( tableau->lowerBounds[1], -3.0 );

        TS_ASSERT( tableau->lowerBounds.exists( 2 ) );
        TS_ASSERT_EQUALS( tableau->lowerBounds[2], 4.0 );

        TS_ASSERT( tableau->lowerBounds.exists( 3 ) );
        TS_ASSERT_EQUALS( tableau->lowerBounds[3], 0.0 );

        TS_ASSERT( tableau->lowerBounds.exists( 4 ) );
        TS_ASSERT_EQUALS( tableau->lowerBounds[4], -100.0 );

        TS_ASSERT( tableau->lowerBounds.exists( 5 ) );
        TS_ASSERT_EQUALS( tableau->lowerBounds[5], 11.0 );

        TS_ASSERT( tableau->lowerBounds.exists( 6 ) );
        TS_ASSERT_EQUALS( tableau->lowerBounds[6], -5.0 );

        TS_ASSERT( tableau->upperBounds.exists( 0 ) );
        TS_ASSERT_EQUALS( tableau->upperBounds[0], 2.0 );

        TS_ASSERT( tableau->upperBounds.exists( 1 ) );
        TS_ASSERT_EQUALS( tableau->upperBounds[1], 3.0 );

        TS_ASSERT( tableau->upperBounds.exists( 2 ) );
        TS_ASSERT_EQUALS( tableau->upperBounds[2], 6.0 );

        TS_ASSERT( tableau->upperBounds.exists( 3 ) );
        TS_ASSERT_EQUALS( tableau->upperBounds[3], 100.0 );

        TS_ASSERT( tableau->upperBounds.exists( 4 ) );
        TS_ASSERT_EQUALS( tableau->upperBounds[4], 0.0 );

        TS_ASSERT( tableau->upperBounds.exists( 5 ) );
        TS_ASSERT_EQUALS( tableau->upperBounds[5], 11.0 );

        TS_ASSERT( tableau->upperBounds.exists( 6 ) );
        TS_ASSERT_EQUALS( tableau->upperBounds[6], -5.0 );

        TS_ASSERT_EQUALS( tableau->lastRegisteredVariableToWatcher.size(), 3U );
        TS_ASSERT( tableau->lastRegisteredVariableToWatcher.exists( 1 ) );
        TS_ASSERT( tableau->lastRegisteredVariableToWatcher.exists( 2 ) );
        TS_ASSERT( tableau->lastRegisteredVariableToWatcher.exists( 4 ) );

        TS_ASSERT_EQUALS( tableau->lastRegisteredVariableToWatcher[1].size(), 1U );
        auto it = tableau->lastRegisteredVariableToWatcher[1].begin();
        auto watcher1 = ( ( ReluConstraint * ) ( * it ) )->getParticipatingVariables();
       	TS_ASSERT( watcher1 == relu1->getParticipatingVariables() );

        TS_ASSERT_EQUALS( tableau->lastRegisteredVariableToWatcher[2].size(), 2U );
        it = tableau->lastRegisteredVariableToWatcher[2].begin();
		auto watcher2 = ( ( ReluConstraint * ) ( * it  ) )->getParticipatingVariables();
		it++;
		auto watcher3 = ( ( ReluConstraint * ) ( * it  ) )->getParticipatingVariables();

		if ( watcher2 == relu1->getParticipatingVariables() )
		{
			TS_ASSERT( watcher3 == relu2->getParticipatingVariables() );
		}
		else if ( watcher2 == relu2->getParticipatingVariables() )
		{
			TS_ASSERT( watcher3 == relu1->getParticipatingVariables() );
		}
		else
		{
			TS_ASSERT( false );
		}

        TS_ASSERT_EQUALS( tableau->lastRegisteredVariableToWatcher[4].size(), 1U );
        it = tableau->lastRegisteredVariableToWatcher[4].begin();
		auto watcher4 = ( ( ReluConstraint * ) ( * it ) )->getParticipatingVariables();

       	TS_ASSERT( watcher4 == relu2->getParticipatingVariables() );
    }

    void test_pick_split_pl_constraint_polarity()
    {
        // x0 --> relu(x2,x4) --> relu(x6,x8)
        //    x               x
        // x1 --> relu(x3,x5) --> relu(x7,x9)

        // x0 \in [-1, 1]
        // x1 \in [-1, 2]
        // x2 = 2 x0 - 3 x1    ( x2 \in [-8, 5] )
        // x3 = 2 x0 - x1    ( x2 \in [-4, 3] )
        // x4 \in [0, 5]
        // x5 \in [0, 3]
        // x6 = -x4 + x5  ( x6 \in [-2, 8] )
        // x7 = -x4 + x5 - 3 ( x7 \in [-5, 5] )
        // Based on the polarity, relu(x7, x9) should be picked
        // And the picked ReLU must have |polarity| not larger than the other unfixed ReLUs

        InputQuery inputQuery;
        inputQuery.setNumberOfVariables( 10 );
        inputQuery.markInputVariable( 0, 0 );
        inputQuery.markInputVariable( 1, 1 );

        Equation equation;
        equation.addAddend( 2, 0 );
        equation.addAddend( -3, 1 );
        equation.addAddend( -1, 2 );
        equation.setScalar( 0 );
        inputQuery.addEquation( equation );

        Equation equation1;
        equation1.addAddend( 2, 0 );
        equation1.addAddend( -1, 1 );
        equation1.addAddend( -1, 3 );
        equation1.setScalar( 0 );
        inputQuery.addEquation( equation1 );

        Equation equation3;
        equation3.addAddend( -1, 4 );
        equation3.addAddend( 1, 5 );
        equation3.addAddend( -1, 6 );
        equation3.setScalar( 0 );
        inputQuery.addEquation( equation3 );

        Equation equation4;
        equation4.addAddend( -1, 4 );
        equation4.addAddend( 1, 5 );
        equation4.addAddend( -1, 7 );
        equation4.setScalar( 0 );
        inputQuery.addEquation( equation4 );

        ReluConstraint *relu1 = new ReluConstraint( 2, 4 );
        ReluConstraint *relu2 = new ReluConstraint( 3, 5 );
        ReluConstraint *relu3 = new ReluConstraint( 6, 8 );
        ReluConstraint *relu4 = new ReluConstraint( 7, 9 );

        inputQuery.addPiecewiseLinearConstraint( relu1 );
        inputQuery.addPiecewiseLinearConstraint( relu2 );
        inputQuery.addPiecewiseLinearConstraint( relu3 );
        inputQuery.addPiecewiseLinearConstraint( relu4 );

        inputQuery.setLowerBound( 0, -1 );
        inputQuery.setUpperBound( 0, 1 );
        inputQuery.setLowerBound( 1, -1 );
        inputQuery.setUpperBound( 1, 2 );
        inputQuery.setLowerBound( 2, -8 );
        inputQuery.setUpperBound( 2, 5 );
        inputQuery.setLowerBound( 3, -4 );
        inputQuery.setUpperBound( 3, 3 );
        inputQuery.setLowerBound( 4, 0 );
        inputQuery.setUpperBound( 4, 5 );
        inputQuery.setLowerBound( 5, 0 );
        inputQuery.setUpperBound( 5, 3 );
        inputQuery.setLowerBound( 6, -2 );
        inputQuery.setUpperBound( 6, 8 );
        inputQuery.setLowerBound( 7, -5 );
        inputQuery.setUpperBound( 7, 5 );
        inputQuery.setLowerBound( 8, 0 );
        inputQuery.setUpperBound( 8, 8 );
        inputQuery.setLowerBound( 9, 0 );
        inputQuery.setUpperBound( 9, 5 );

        Engine engine;
        TS_ASSERT( inputQuery.constructNetworkLevelReasoner() );
        engine.processInputQuery( inputQuery, false );
        PiecewiseLinearConstraint *constraintToSplit;
        PiecewiseLinearConstraint *constraintToSplitSnC;
        constraintToSplit = engine.pickSplitPLConstraint( DivideStrategy::Polarity );
        constraintToSplitSnC = engine.pickSplitPLConstraintSnC( SnCDivideStrategy::Polarity );
        TS_ASSERT_EQUALS( constraintToSplitSnC, constraintToSplit );

        TS_ASSERT( constraintToSplit );
        TS_ASSERT( !constraintToSplit->phaseFixed() );
        TS_ASSERT( constraintToSplit->isActive() );
        for ( const auto &constraint : engine.getInputQuery()->
                  getNetworkLevelReasoner()->getConstraintsInTopologicalOrder() )
        {
            TS_ASSERT( std::abs( ( ( ReluConstraint * ) constraintToSplit )->computePolarity() ) <=
                       std::abs( ( ( ReluConstraint * ) constraint )->computePolarity() ) );
        }

        TS_ASSERT_EQUALS( 0, ( ( ReluConstraint * ) constraintToSplit )->computePolarity() );
    }

    void test_pick_split_pl_constraint_earliest_relu()
    {
        // x0 --> relu(x2,x4) --> relu(x6,x8)
        //    x               x
        // x1 --> relu(x3,x5) --> relu(x7,x9)

        // x0 \in [-1, 1]
        // x1 \in [-1, 2]
        // x2 = 2 x0 - 3 x1    ( x2 \in [-8, 5] )
        // x3 = 2 x0 - x1    ( x2 \in [-4, 3] )
        // x4 \in [0, 5]
        // x5 \in [0, 3]
        // x6 = -x4 + x5  ( x6 \in [-2, 8] )
        // x7 = -x4 + x5 - 3 ( x7 \in [-5, 5] )

        InputQuery inputQuery;
        inputQuery.setNumberOfVariables( 10 );
        inputQuery.markInputVariable( 0, 0 );
        inputQuery.markInputVariable( 1, 1 );

        Equation equation;
        equation.addAddend( 2, 0 );
        equation.addAddend( -3, 1 );
        equation.addAddend( -1, 2 );
        equation.setScalar( 0 );
        inputQuery.addEquation( equation );

        Equation equation1;
        equation1.addAddend( 2, 0 );
        equation1.addAddend( -1, 1 );
        equation1.addAddend( -1, 3 );
        equation1.setScalar( 0 );
        inputQuery.addEquation( equation1 );

        Equation equation3;
        equation3.addAddend( -1, 4 );
        equation3.addAddend( 1, 5 );
        equation3.addAddend( -1, 6 );
        equation3.setScalar( 0 );
        inputQuery.addEquation( equation3 );

        Equation equation4;
        equation4.addAddend( -1, 4 );
        equation4.addAddend( 1, 5 );
        equation4.addAddend( -1, 7 );
        equation4.setScalar( 0 );
        inputQuery.addEquation( equation4 );

        ReluConstraint *relu1 = new ReluConstraint( 2, 4 );
        ReluConstraint *relu2 = new ReluConstraint( 3, 5 );
        ReluConstraint *relu3 = new ReluConstraint( 6, 8 );
        ReluConstraint *relu4 = new ReluConstraint( 7, 9 );

        inputQuery.addPiecewiseLinearConstraint( relu1 );
        inputQuery.addPiecewiseLinearConstraint( relu2 );
        inputQuery.addPiecewiseLinearConstraint( relu3 );
        inputQuery.addPiecewiseLinearConstraint( relu4 );

        inputQuery.setLowerBound( 0, -1 );
        inputQuery.setUpperBound( 0, 1 );
        inputQuery.setLowerBound( 1, -1 );
        inputQuery.setUpperBound( 1, 2 );
        inputQuery.setLowerBound( 2, -8 );
        inputQuery.setUpperBound( 2, 5 );
        inputQuery.setLowerBound( 3, -4 );
        inputQuery.setUpperBound( 3, 3 );
        inputQuery.setLowerBound( 4, 0 );
        inputQuery.setUpperBound( 4, 5 );
        inputQuery.setLowerBound( 5, 0 );
        inputQuery.setUpperBound( 5, 3 );
        inputQuery.setLowerBound( 6, -2 );
        inputQuery.setUpperBound( 6, 8 );
        inputQuery.setLowerBound( 7, -5 );
        inputQuery.setUpperBound( 7, 5 );
        inputQuery.setLowerBound( 8, 0 );
        inputQuery.setUpperBound( 8, 8 );
        inputQuery.setLowerBound( 9, 0 );
        inputQuery.setUpperBound( 9, 5 );

        Engine engine;
        TS_ASSERT( inputQuery.constructNetworkLevelReasoner() );
        engine.processInputQuery( inputQuery, false );
        PiecewiseLinearConstraint *constraintToSplit;
        PiecewiseLinearConstraint *constraintToSplitSnC;
        constraintToSplit = engine.pickSplitPLConstraint( DivideStrategy::EarliestReLU );
        constraintToSplitSnC = engine.pickSplitPLConstraintSnC( SnCDivideStrategy::EarliestReLU );
        TS_ASSERT_EQUALS( constraintToSplitSnC, constraintToSplit );

        PiecewiseLinearConstraint *firstConstraintInTopologicalOrder;
        firstConstraintInTopologicalOrder = *(engine.getInputQuery()->
                                              getNetworkLevelReasoner()->
                                              getConstraintsInTopologicalOrder().begin());

        TS_ASSERT( firstConstraintInTopologicalOrder );
        TS_ASSERT( !firstConstraintInTopologicalOrder->phaseFixed() );
        TS_ASSERT( firstConstraintInTopologicalOrder->isActive() );
        TS_ASSERT_EQUALS( firstConstraintInTopologicalOrder, constraintToSplit );
    }

    void test_todo()
    {
        TS_TRACE( "Future work: Guarantee correct behavior even when some variable is unbounded\n" );
    }
};

//
// Local Variables:
// compile-command: "make -C ../../.. "
// tags-file-name: "../../../TAGS"
// c-basic-offset: 4
// End:
//
