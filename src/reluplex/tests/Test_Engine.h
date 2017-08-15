/*********************                                                        */
/*! \file Test_Engine.h
** \verbatim
** Top contributors (to current version):
**   Guy Katz
** This file is part of the Marabou project.
** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
** in the top-level source directory) and their institutional affiliations.
** All rights reserved. See the file COPYING in the top-level source
** directory for licensing information.\endverbatim
**/

#include <cxxtest/TestSuite.h>

#include "Engine.h"
#include "FreshVariables.h"
#include "InputQuery.h"
#include "MockErrno.h"
#include "MockTableauFactory.h"
#include "ReluConstraint.h"

#include <cfloat>
#include <string.h>

class MockForEngine :
    public MockTableauFactory
{
public:
};

class EngineTestSuite : public CxxTest::TestSuite
{
public:
    MockForEngine *mock;
    MockTableau *tableau;

    void setUp()
    {
        TS_ASSERT( mock = new MockForEngine );

        tableau = &( mock->mockTableau );
    }

    void tearDown()
    {
        TS_ASSERT_THROWS_NOTHING( delete mock );
    }

    void test_constructor_destructor()
    {
        Engine *engine;

        TS_ASSERT_THROWS_NOTHING( engine = new Engine() );

        TS_ASSERT( tableau->wasCreated );

        TS_ASSERT_THROWS_NOTHING( delete engine );

        TS_ASSERT( tableau->wasDiscarded );
    }

    void test_process_input_query()
    {
        //   0  <= x0 <= 2
        //   -3 <= x1 <= 3
        //   4  <= x2 <= 6
        //
        //  x0 + 2x1 -x2 <= 11 --> x0 + 2x1 - x2 + x3 = 11, x3 >= 0
        //  -3x0 + 3x1  >= -5 --> -3x0 + 3x1 + x4 = -5, x4 <= 0

        InputQuery inputQuery;
        inputQuery.setNumberOfVariables( 5 );

        inputQuery.setLowerBound( 0, 0 );
        inputQuery.setUpperBound( 0, 2 );

        inputQuery.setLowerBound( 1, -3 );
        inputQuery.setUpperBound( 1, 3 );

        inputQuery.setLowerBound( 2, 4 );
        inputQuery.setUpperBound( 2, 6 );

        inputQuery.setLowerBound( 3, 0 );
        inputQuery.setUpperBound( 4, 0 );

        Equation equation1;
        equation1.addAddend( 1, 0 );
        equation1.addAddend( 2, 1 );
        equation1.addAddend( -1, 2 );
        equation1.addAuxAddend( 1 );
        equation1.setScalar( 11 );
        equation1.markAuxiliaryVariable( 3 );
        inputQuery.addEquation( equation1 );

        Equation equation2;
        equation2.addAddend( -3, 0 );
        equation2.addAddend( 3, 1 );
        equation2.addAuxAddend( 1 );
        equation2.setScalar( -5 );
        equation2.markAuxiliaryVariable( 4 );        
        inputQuery.addEquation( equation2 );

        ReluConstraint *relu1 = new ReluConstraint( 1, 2 );
        ReluConstraint *relu2 = new ReluConstraint( 2, 4 );

        inputQuery.addPiecewiseLinearConstraint( relu1 );
        inputQuery.addPiecewiseLinearConstraint( relu2 );

        Engine engine;

        TS_ASSERT_THROWS_NOTHING( engine.processInputQuery( inputQuery, false ) );
        TS_ASSERT( tableau->initializeTableauCalled );

        TS_ASSERT_EQUALS( tableau->lastM, 2U );
        TS_ASSERT_EQUALS( tableau->lastN, 5U );

        // Variables 3 and 4 should be marked as basic
        TS_ASSERT_EQUALS( tableau->lastBasicVariables.size(), 2U );
        TS_ASSERT( tableau->lastBasicVariables.exists( 3 ) );
        TS_ASSERT( tableau->lastBasicVariables.exists( 4 ) );

        // Right hand side scalars
        TS_ASSERT( tableau->lastRightHandSide );
        TS_ASSERT_EQUALS( tableau->lastRightHandSide[0], 11.0 );
        TS_ASSERT_EQUALS( tableau->lastRightHandSide[1], -5.0 );

        // Tableau entries
        TS_ASSERT_EQUALS( tableau->lastEntries[(0*5) + 0], 1 );
        TS_ASSERT_EQUALS( tableau->lastEntries[(0*5) + 1], 2 );
        TS_ASSERT_EQUALS( tableau->lastEntries[(0*5) + 2], -1 );
        TS_ASSERT_EQUALS( tableau->lastEntries[(0*5) + 3], 1 );
        TS_ASSERT_EQUALS( tableau->lastEntries[(0*5) + 4], 0 );

        TS_ASSERT_EQUALS( tableau->lastEntries[(1*5) + 0], -3 );
        TS_ASSERT_EQUALS( tableau->lastEntries[(1*5) + 1], 3 );
        TS_ASSERT_EQUALS( tableau->lastEntries[(1*5) + 2], 0 );
        TS_ASSERT_EQUALS( tableau->lastEntries[(1*5) + 3], 0 );
        TS_ASSERT_EQUALS( tableau->lastEntries[(1*5) + 4], 1 );

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
        TS_ASSERT_EQUALS( tableau->lowerBounds[4], -DBL_MAX );

        TS_ASSERT( tableau->upperBounds.exists( 0 ) );
        TS_ASSERT_EQUALS( tableau->upperBounds[0], 2.0 );

        TS_ASSERT( tableau->upperBounds.exists( 1 ) );
        TS_ASSERT_EQUALS( tableau->upperBounds[1], 3.0 );

        TS_ASSERT( tableau->upperBounds.exists( 2 ) );
        TS_ASSERT_EQUALS( tableau->upperBounds[2], 6.0 );

        TS_ASSERT( tableau->upperBounds.exists( 3 ) );
        TS_ASSERT_EQUALS( tableau->upperBounds[3], DBL_MAX );

        TS_ASSERT( tableau->upperBounds.exists( 4 ) );
        TS_ASSERT_EQUALS( tableau->upperBounds[4], 0.0 );

        TS_ASSERT_EQUALS( tableau->lastRegisteredVariableToWatcher.size(), 3U );
        TS_ASSERT( tableau->lastRegisteredVariableToWatcher.exists( 1 ) );
        TS_ASSERT( tableau->lastRegisteredVariableToWatcher.exists( 2 ) );
        TS_ASSERT( tableau->lastRegisteredVariableToWatcher.exists( 4 ) );

        TS_ASSERT_EQUALS( tableau->lastRegisteredVariableToWatcher[1].size(), 1U );
        TS_ASSERT( tableau->lastRegisteredVariableToWatcher[1].exists( relu1 ) );

        TS_ASSERT_EQUALS( tableau->lastRegisteredVariableToWatcher[2].size(), 2U );
        TS_ASSERT( tableau->lastRegisteredVariableToWatcher[2].exists( relu1 ) );
        TS_ASSERT( tableau->lastRegisteredVariableToWatcher[2].exists( relu2 ) );

        TS_ASSERT_EQUALS( tableau->lastRegisteredVariableToWatcher[4].size(), 1U );
        TS_ASSERT( tableau->lastRegisteredVariableToWatcher[4].exists( relu2 ) );

        TS_ASSERT_EQUALS( FreshVariables::getNextVariable(), 5U );
    }

    void test_todo()
    {
        TS_TRACE( "Future work: Guarantee correct behavior even when some variable is unbounded\n" );
    }
};

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
