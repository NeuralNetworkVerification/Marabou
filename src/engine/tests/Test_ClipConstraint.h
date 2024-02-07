/*********************                                                        */
/*! \file Test_ClipConstraint.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Andrew Wu
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

**/

#include <cxxtest/TestSuite.h>

#include "ClipConstraint.h"
#include "InputQuery.h"
#include "MarabouError.h"
#include "MockErrno.h"
#include "MockTableau.h"
#include "PiecewiseLinearCaseSplit.h"
#include <iostream>
#include <string.h>

class MockForClipConstraint : public MockErrno
{
public:
};

/*
   Exposes protected members of ClipConstraint for testing.
 */
class TestClipConstraint : public ClipConstraint
{
public:
    TestClipConstraint( unsigned b, unsigned f )
        : ClipConstraint( b, f, 1, 5 )
    {}

    using ClipConstraint::getPhaseStatus;
};

using namespace CVC4::context;

class ClipConstraintTestSuite : public CxxTest::TestSuite
{
public:
    MockForClipConstraint *mock;

    void setUp()
    {
        TS_ASSERT( mock = new MockForClipConstraint );
    }

    void tearDown()
    {
        TS_ASSERT_THROWS_NOTHING( delete mock );
    }

    void test_clip_duplicate_and_restore()
    {
        TestClipConstraint clip1( 4, 6 );
        MockTableau tableau;
        clip1.registerTableau( &tableau );

        clip1.setActiveConstraint( false );
        tableau.setValue( 4, 1.0 );
        tableau.setValue( 6, 1.0 );

        clip1.notifyLowerBound( 4, -8.0 );
        clip1.notifyUpperBound( 4, 8.0 );

        clip1.notifyLowerBound( 6, 0.0 );
        clip1.notifyUpperBound( 6, 4.0 );

        PiecewiseLinearConstraint *clip2 = clip1.duplicateConstraint();

        TS_ASSERT( clip1.satisfied() );
        TS_ASSERT( !clip1.isActive() );
        TS_ASSERT( !clip2->isActive() );
        TS_ASSERT( clip2->satisfied() );

        clip1.setActiveConstraint( true );
        clip2->restoreState( &clip1 );
        TS_ASSERT( clip2->isActive() );
        TS_ASSERT( clip2->satisfied() );

        clip1.setActiveConstraint( false );
        TS_ASSERT( !clip1.isActive() );
        TS_ASSERT( clip2->isActive() );

        TS_ASSERT_THROWS_NOTHING( delete clip2 );
    }

    void test_register_and_unregister_as_watcher()
    {
        unsigned b = 1;
        unsigned f = 4;

        MockTableau tableau;

        TestClipConstraint clip( b, f );

        TS_ASSERT_THROWS_NOTHING( clip.registerAsWatcher( &tableau ) );

        TS_ASSERT_EQUALS( tableau.lastRegisteredVariableToWatcher.size(), 2U );
        TS_ASSERT( tableau.lastUnregisteredVariableToWatcher.empty() );
        TS_ASSERT_EQUALS( tableau.lastRegisteredVariableToWatcher[b].size(), 1U );
        TS_ASSERT( tableau.lastRegisteredVariableToWatcher[b].exists( &clip ) );
        TS_ASSERT_EQUALS( tableau.lastRegisteredVariableToWatcher[f].size(), 1U );
        TS_ASSERT( tableau.lastRegisteredVariableToWatcher[f].exists( &clip ) );

        tableau.lastRegisteredVariableToWatcher.clear();

        TS_ASSERT_THROWS_NOTHING( clip.unregisterAsWatcher( &tableau ) );

        TS_ASSERT( tableau.lastRegisteredVariableToWatcher.empty() );
        TS_ASSERT_EQUALS( tableau.lastUnregisteredVariableToWatcher.size(), 2U );
        TS_ASSERT_EQUALS( tableau.lastUnregisteredVariableToWatcher[b].size(), 1U );
        TS_ASSERT( tableau.lastUnregisteredVariableToWatcher[b].exists( &clip ) );
        TS_ASSERT_EQUALS( tableau.lastUnregisteredVariableToWatcher[f].size(), 1U );
        TS_ASSERT( tableau.lastUnregisteredVariableToWatcher[f].exists( &clip ) );
    }

    void test_participating_variables()
    {
        unsigned b = 1;
        unsigned f = 4;

        TestClipConstraint clip( b, f );

        List<unsigned> participatingVariables;

        TS_ASSERT_THROWS_NOTHING( participatingVariables = clip.getParticipatingVariables() );
        TS_ASSERT_EQUALS( participatingVariables.size(), 2U );
        auto it = participatingVariables.begin();
        TS_ASSERT_EQUALS( *it, b );
        ++it;
        TS_ASSERT_EQUALS( *it, f );

        TS_ASSERT_EQUALS( clip.getParticipatingVariables(), List<unsigned>( { 1, 4 } ) );

        TS_ASSERT( clip.participatingVariable( b ) );
        TS_ASSERT( clip.participatingVariable( f ) );
        TS_ASSERT( !clip.participatingVariable( 2 ) );
        TS_ASSERT( !clip.participatingVariable( 0 ) );
        TS_ASSERT( !clip.participatingVariable( 5 ) );
    }

    void test_clip_notify_variable_value_and_satisfied()
    {
        unsigned b = 1;
        unsigned f = 4;

        TestClipConstraint clip( b, f );
        MockTableau tableau;
        clip.registerTableau( &tableau );

        tableau.setValue( b, 5 );
        tableau.setValue( f, 5 );
        TS_ASSERT( clip.satisfied() );

        tableau.setValue( b, 8 );
        tableau.setValue( f, 5 );
        TS_ASSERT( clip.satisfied() );

        tableau.setValue( b, 3 );
        tableau.setValue( f, 5 );
        TS_ASSERT( !clip.satisfied() );

        tableau.setValue( b, -1 );
        tableau.setValue( f, -1 );
        TS_ASSERT( !clip.satisfied() );

        tableau.setValue( b, -1 );
        tableau.setValue( f, 1 );
        TS_ASSERT( clip.satisfied() );

        tableau.setValue( b, 1 );
        tableau.setValue( f, 1 );
        TS_ASSERT( clip.satisfied() );

        tableau.setValue( b, 5 );
        tableau.setValue( f, 5 );
        TS_ASSERT( clip.satisfied() );
    }

    void test_clip_update_variable_index()
    {
        unsigned b = 1;
        unsigned f = 4;

        TestClipConstraint clip( b, f );
        MockTableau tableau;
        clip.registerTableau( &tableau );

        // Changing variable indices
        tableau.setValue( b, 1 );
        tableau.setValue( f, 1 );
        TS_ASSERT( clip.satisfied() );

        unsigned newB = 12;
        unsigned newF = 14;

        TS_ASSERT_THROWS_NOTHING( clip.updateVariableIndex( b, newB ) );
        TS_ASSERT_THROWS_NOTHING( clip.updateVariableIndex( f, newF ) );

        tableau.setValue( newB, 1 );
        tableau.setValue( newF, 1 );

        TS_ASSERT( clip.satisfied() );

        tableau.setValue( newF, 2 );

        TS_ASSERT( !clip.satisfied() );

        tableau.setValue( newB, 2 );

        TS_ASSERT( clip.satisfied() );
    }

    void test_eliminate_variable_b()
    {
        unsigned b = 1;
        unsigned f = 4;

        MockTableau tableau;

        TestClipConstraint clip( b, f );

        clip.registerAsWatcher( &tableau );

        TS_ASSERT( !clip.constraintObsolete() );
        TS_ASSERT_THROWS_NOTHING( clip.eliminateVariable( b, 5 ) );
        TS_ASSERT( clip.constraintObsolete() );
    }

    void test_eliminate_variable_f()
    {
        unsigned b = 1;
        unsigned f = 4;

        MockTableau tableau;

        TestClipConstraint clip( b, f );

        clip.registerAsWatcher( &tableau );

        TS_ASSERT( !clip.constraintObsolete() );
        TS_ASSERT_THROWS_NOTHING( clip.eliminateVariable( f, 5 ) );
        TS_ASSERT( clip.constraintObsolete() );
    }

    void existsBound( unsigned variable, double lowerBound, double upperBound,
                      List<Tightening> entailedTightenings )
    {
        TS_ASSERT( entailedTightenings.exists( Tightening( variable, lowerBound, Tightening::LB ) ) );
        TS_ASSERT( entailedTightenings.exists( Tightening( variable, upperBound, Tightening::UB ) ) );
    }

    void test_serialize_and_unserialize()
    {
        unsigned b = 42;
        unsigned f = 7;

        TestClipConstraint originalClip( b, f );
        originalClip.notifyLowerBound( b, -10 );
        originalClip.notifyUpperBound( f, 5 );
        originalClip.notifyUpperBound( f, 5 );

        String originalSerialized = originalClip.serializeToString();
        ClipConstraint recoveredClip( originalSerialized );

        TS_ASSERT_EQUALS( originalClip.serializeToString(),
                          recoveredClip.serializeToString() );
    }

    void test_initialization_of_CDOs()
    {
        Context context;
        TestClipConstraint *clip1 = new TestClipConstraint( 4, 6 );

        TS_ASSERT_EQUALS( clip1->getContext(), static_cast<Context*>( nullptr ) );
        TS_ASSERT_EQUALS( clip1->getActiveStatusCDO(), static_cast<CDO<bool>*>( nullptr ) );
        TS_ASSERT_EQUALS( clip1->getPhaseStatusCDO(), static_cast<CDO<PhaseStatus>*>( nullptr ) );
        TS_ASSERT_EQUALS( clip1->getInfeasibleCasesCDList(), static_cast<CDList<PhaseStatus>*>( nullptr ) );
        TS_ASSERT_THROWS_NOTHING( clip1->initializeCDOs( &context ) );
        TS_ASSERT_EQUALS( clip1->getContext(), &context );
        TS_ASSERT_DIFFERS( clip1->getActiveStatusCDO(), static_cast<CDO<bool>*>( nullptr ) );
        TS_ASSERT_DIFFERS( clip1->getPhaseStatusCDO(), static_cast<CDO<PhaseStatus>*>( nullptr ) );
        TS_ASSERT_DIFFERS( clip1->getInfeasibleCasesCDList(), static_cast<CDList<PhaseStatus>*>( nullptr ) );

        bool active = false;
        TS_ASSERT_THROWS_NOTHING( active = clip1->isActive() );
        TS_ASSERT_EQUALS( active, true );

        bool phaseFixed = true;
        TS_ASSERT_THROWS_NOTHING( phaseFixed = clip1->phaseFixed() );
        TS_ASSERT_EQUALS( phaseFixed, PHASE_NOT_FIXED );
        TS_ASSERT_EQUALS( clip1->numFeasibleCases(), 3u );


        TS_ASSERT_THROWS_NOTHING( delete clip1 );
    }

    void test_clip_get_cases()
    {
        TestClipConstraint clip( 4, 6 );

        List<PhaseStatus> cases = clip.getAllCases();

        TS_ASSERT_EQUALS( cases.size(), 3u );

        TS_ASSERT( cases.exists( CLIP_PHASE_FLOOR ) );
        TS_ASSERT( cases.exists( CLIP_PHASE_CEILING ) );
        TS_ASSERT( cases.exists( CLIP_PHASE_MIDDLE ) );
    }

    void test_get_cost_function_component()
    {
        /* Test the add cost function component methods */

        unsigned b = 0;
        unsigned f = 1;

        // The clip is fixed, do not add cost term.
        TestClipConstraint clip1 = TestClipConstraint( b, f );
        MockTableau tableau;
        clip1.registerTableau( &tableau );

        clip1.notifyLowerBound( b, 1.1 );
        clip1.notifyUpperBound( b, 2 );
        clip1.notifyLowerBound( f, 1 );
        clip1.notifyUpperBound( f, 5 );
        tableau.setValue( b, 1.5 );
        tableau.setValue( f, 1.5 );

        TS_ASSERT( clip1.phaseFixed() );
        LinearExpression cost1;
        TS_ASSERT_THROWS_NOTHING( clip1.getCostFunctionComponent( cost1, CLIP_PHASE_CEILING ) );
        TS_ASSERT_EQUALS( cost1._addends.size(), 0u );
        TS_ASSERT_EQUALS( cost1._constant, 0 );


        // The clip is not fixed and add active cost term
        TestClipConstraint clip2 = TestClipConstraint( b, f );
        clip2.registerTableau( &tableau );

        LinearExpression cost2;
        clip2.notifyLowerBound( b, -1 );
        clip2.notifyUpperBound( b, 4 );
        clip2.notifyLowerBound( f, 1 );
        clip2.notifyUpperBound( f, 5 );
        TS_ASSERT( !clip2.phaseFixed() );
        TS_ASSERT_THROWS_NOTHING( clip2.getCostFunctionComponent( cost2, CLIP_PHASE_FLOOR ) );
        TS_ASSERT_EQUALS( cost2._addends.size(), 1u );
        TS_ASSERT_EQUALS( cost2._addends[f], 1 );
        TS_ASSERT_EQUALS( cost2._constant, -1 );

        // The clip is not fixed and add inactive cost term
        TestClipConstraint clip3 = TestClipConstraint( b, f );
        clip3.registerTableau( &tableau );

        LinearExpression cost3;
        clip3.notifyLowerBound( b, -1 );
        clip3.notifyUpperBound( b, 6 );
        clip3.notifyLowerBound( f, 1 );
        clip3.notifyUpperBound( f, 5 );
        TS_ASSERT_THROWS_NOTHING( clip3.getCostFunctionComponent( cost3, CLIP_PHASE_CEILING ) );
        TS_ASSERT_EQUALS( cost3._addends.size(), 1u );
        TS_ASSERT_EQUALS( cost3._addends[f], -1 );
        TS_ASSERT_EQUALS( cost3._constant, 5 );
    }

    void test_get_phase_in_assignment()
    {
        unsigned b = 0;
        unsigned f = 1;

        // The clip is fixed, do not add cost term.
        TestClipConstraint clip = TestClipConstraint( b, f );
        MockTableau tableau;
        clip.registerTableau( &tableau );
        tableau.setValue( b, 1.5 );
        tableau.setValue( f, 2 );

        Map<unsigned, double> assignment;
        assignment[b] = -1;
        TS_ASSERT_EQUALS( clip.getPhaseStatusInAssignment( assignment ),
                          CLIP_PHASE_FLOOR );

        assignment[b] = 15;
        TS_ASSERT_EQUALS( clip.getPhaseStatusInAssignment( assignment ),
                          CLIP_PHASE_CEILING );

        assignment[b] = 3;
        TS_ASSERT_EQUALS( clip.getPhaseStatusInAssignment( assignment ),
                          CLIP_PHASE_MIDDLE );
    }

};
