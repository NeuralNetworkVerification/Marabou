/*********************                                                        */
/*! \file Test_SmtCore.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz, Duligur Ibeling, Derek Huang
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
#include "MockEngine.h"
#include "MockErrno.h"
#include "Options.h"
#include "PiecewiseLinearConstraint.h"
#include "ReluConstraint.h"
#include "SmtCore.h"
#include "context/context.h"

#include <string.h>

class MockForSmtCore
{
public:
};

class SmtCoreTestSuite : public CxxTest::TestSuite
{
public:
    MockForSmtCore *mock;
    MockEngine *engine;

    class MockConstraint : public PiecewiseLinearConstraint
    {
    public:
        MockConstraint()
            : setActiveWasCalled( false )
        {
            nextIsActive = true;
        }

        PiecewiseLinearFunctionType getType() const
        {
            return (PiecewiseLinearFunctionType)999;
        }

        PiecewiseLinearConstraint *duplicateConstraint() const
        {
            return NULL;
        }

        void restoreState( const PiecewiseLinearConstraint */* state */ )
        {
        }

        void registerAsWatcher( ITableau * )
        {
        }

        void unregisterAsWatcher( ITableau * )
        {
        }

        bool setActiveWasCalled;
        void setActiveConstraint( bool active )
        {
            TS_ASSERT( active == false );
            setActiveWasCalled = true;
        }

        bool nextIsActive;
        bool isActive() const
        {
            return nextIsActive;
        }

        bool participatingVariable( unsigned ) const
        {
            return true;
        }

        List<unsigned> getParticipatingVariables() const
        {
            return List<unsigned>();
        }


        bool satisfied() const
        {
            return true;
        }

        List<PiecewiseLinearConstraint::Fix> getPossibleFixes() const
        {
            return List<PiecewiseLinearConstraint::Fix>();
        }

        List<PiecewiseLinearConstraint::Fix> getSmartFixes( ITableau * ) const
        {
            return List<PiecewiseLinearConstraint::Fix>();
        }

        List<PiecewiseLinearCaseSplit> nextSplits;
        void registerCaseSplit( PiecewiseLinearCaseSplit caseSplit )
        {
            nextSplits.append( caseSplit );
            allCases.append( (PhaseStatus)(++_numCases) );
        }

        List<PiecewiseLinearCaseSplit> getCaseSplits() const
        {
            return nextSplits;
        }

        List<PhaseStatus> allCases;
        List<PhaseStatus> getAllCases() const
        {
            return allCases;
        }

        bool phaseFixed() const
        {
            return false;
        }

        PiecewiseLinearCaseSplit getCaseSplit( PhaseStatus caseId ) const
        {
            auto it = nextSplits.begin();
            for ( int i = 0; i < (int)caseId - 1; ++i )
                ++it;

            return *it;
        }

        PiecewiseLinearCaseSplit getValidCaseSplit() const
        {
            PiecewiseLinearCaseSplit dontCare;
            return dontCare;
        }

        PiecewiseLinearCaseSplit getImpliedCaseSplit() const
        {
            PiecewiseLinearCaseSplit dontCare;
            return dontCare;
        }

        void updateVariableIndex( unsigned, unsigned )
        {
        }

        void eliminateVariable( unsigned, double )
        {
        }

        bool constraintObsolete() const
        {
            return false;
        }

        void preprocessBounds( unsigned, double, Tightening::BoundType )
        {
        }

        void getEntailedTightenings( List<Tightening> & ) const
        {
        }

        void getAuxiliaryEquations( List<Equation> &/* newEquations */ ) const
        {
        }

        String serializeToString() const
        {
            return "";
        }

    };

    void setUp()
    {
        TS_ASSERT( mock = new MockForSmtCore );
        TS_ASSERT( engine = new MockEngine );
    }

    void tearDown()
    {
        TS_ASSERT_THROWS_NOTHING( delete engine );
        TS_ASSERT_THROWS_NOTHING( delete mock );
    }

    void test_need_to_split()
    {
        ReluConstraint constraint1( 1, 2 );
        ReluConstraint constraint2( 3, 4 );

        CVC4::context::Context context;
        SmtCore smtCore( engine, context );

        TS_ASSERT( !smtCore.needToSplit() );
        for ( unsigned i = 0; i < ( unsigned ) Options::get()->getInt( Options::CONSTRAINT_VIOLATION_THRESHOLD ) - 1; ++i )
        {
            smtCore.reportViolatedConstraint( &constraint1 );
            TS_ASSERT( !smtCore.needToSplit() );
            smtCore.reportViolatedConstraint( &constraint2 );
            TS_ASSERT( !smtCore.needToSplit() );
        }

        smtCore.reportViolatedConstraint( &constraint2 );
        TS_ASSERT( smtCore.needToSplit() );
    }

    void test_perform_split()
    {
        CVC4::context::Context context;
        SmtCore smtCore( engine, context );

        MockConstraint constraint;

        // Split 1
        PiecewiseLinearCaseSplit split1;
        Tightening bound1( 1, 3.0, Tightening::LB );
        Tightening bound2( 1, 5.0, Tightening::UB );

        split1.storeBoundTightening( bound1 );
        split1.storeBoundTightening( bound2 );

        // Split 2
        PiecewiseLinearCaseSplit split2;
        Tightening bound3( 2, 13.0, Tightening::UB );
        Tightening bound4( 3, 25.0, Tightening::UB );

        split2.storeBoundTightening( bound3 );
        split2.storeBoundTightening( bound4 );

        // Split 3
        PiecewiseLinearCaseSplit split3;
        Tightening bound5( 14, 2.3, Tightening::LB );

        split3.storeBoundTightening( bound5 );

        // Store the splits
        constraint.registerCaseSplit( split1 );
        constraint.registerCaseSplit( split2 );
        constraint.registerCaseSplit( split3 );

        constraint.initializeCDOs( &context );

        for ( unsigned i = 0; i < ( unsigned ) Options::get()->getInt( Options::CONSTRAINT_VIOLATION_THRESHOLD ); ++i )
            smtCore.reportViolatedConstraint( &constraint );

        engine->lastStoredState = NULL;
        engine->lastRestoredState = NULL;

        TS_ASSERT( smtCore.needToSplit() );
        TS_ASSERT_EQUALS( smtCore.getDecisionLevel(), 0U );
        TS_ASSERT( !constraint.setActiveWasCalled );
        TS_ASSERT_THROWS_NOTHING( smtCore.decide() );
        TS_ASSERT( constraint.setActiveWasCalled );
        TS_ASSERT( !smtCore.needToSplit() );
        TS_ASSERT_EQUALS( smtCore.getDecisionLevel(), 1U );

        // Pop Split1  and check that Split2 was decided
        TS_ASSERT( smtCore.backtrackAndContinueSearch() );
        TS_ASSERT_EQUALS( smtCore.getDecisionLevel(), 1U );

        // Pop Split2, check that Split3 was implied
        TS_ASSERT( smtCore.backtrackAndContinueSearch() );
        TS_ASSERT_EQUALS( smtCore.getDecisionLevel(), 0U );

        // Final pop
        TS_ASSERT( !smtCore.backtrackAndContinueSearch() );
        TS_ASSERT_EQUALS( smtCore.getDecisionLevel(), 0U );
    }

    void test_perform_split__inactive_constraint()
    {
        CVC4::context::Context context;
        SmtCore smtCore( engine, context );

        MockConstraint constraint;

        // Split 1
        PiecewiseLinearCaseSplit split1;
        Tightening bound1( 1, 3.0, Tightening::LB );
        Tightening bound2( 1, 5.0, Tightening::UB );

        split1.storeBoundTightening( bound1 );
        split1.storeBoundTightening( bound2 );

        // Split 2
        PiecewiseLinearCaseSplit split2;
        Tightening bound3( 2, 13.0, Tightening::UB );
        Tightening bound4( 3, 25.0, Tightening::UB );

        split2.storeBoundTightening( bound3 );
        split2.storeBoundTightening( bound4 );

        // Split 3
        PiecewiseLinearCaseSplit split3;
        Tightening bound5( 14, 2.3, Tightening::LB );

        split3.storeBoundTightening( bound5 );

        // Store the splits
        constraint.registerCaseSplit( split1 );
        constraint.registerCaseSplit( split2 );
        constraint.registerCaseSplit( split3 );

        for ( unsigned i = 0; i < ( unsigned ) Options::get()->getInt( Options::CONSTRAINT_VIOLATION_THRESHOLD ); ++i )
            smtCore.reportViolatedConstraint( &constraint );

        constraint.nextIsActive = false;

        TS_ASSERT( smtCore.needToSplit() );
        TS_ASSERT_THROWS_NOTHING( smtCore.decide() );
        TS_ASSERT( !smtCore.needToSplit() );

        // Check that no split was performed

        TS_ASSERT( engine->lastLowerBounds.empty() );
        TS_ASSERT( engine->lastUpperBounds.empty() );
        TS_ASSERT( engine->lastEquations.empty() );
        TS_ASSERT( !engine->lastStoredState );
    }

    void test_all_splits_so_far()
    {
        CVC4::context::Context context;
        SmtCore smtCore( engine, context );

        MockConstraint constraint;

        // Split 1
        PiecewiseLinearCaseSplit split1;
        Tightening bound1( 1, 3.0, Tightening::LB );
        Tightening bound2( 1, 5.0, Tightening::UB );

        split1.storeBoundTightening( bound1 );
        split1.storeBoundTightening( bound2 );

        // Split 2
        PiecewiseLinearCaseSplit split2;
        Tightening bound3( 2, 13.0, Tightening::UB );
        Tightening bound4( 3, 25.0, Tightening::UB );

        split2.storeBoundTightening( bound3 );
        split2.storeBoundTightening( bound4 );

        // Store the splits
        constraint.registerCaseSplit( split1 );
        constraint.registerCaseSplit( split2 );
        constraint.initializeCDOs( &context );

        for ( unsigned i = 0; i < ( unsigned ) Options::get()->getInt( Options::CONSTRAINT_VIOLATION_THRESHOLD ); ++i )
            smtCore.reportViolatedConstraint( &constraint );

        constraint.nextIsActive = true;

        TS_ASSERT( smtCore.needToSplit() );
        TS_ASSERT_THROWS_NOTHING( smtCore.decide() );
        TS_ASSERT( !smtCore.needToSplit() );

        // Register a valid split

        // Split 3
        MockConstraint implication;
        PiecewiseLinearCaseSplit split3;
        Tightening bound5( 14, 2.3, Tightening::LB );
        split3.storeBoundTightening( bound5 );
        implication.registerCaseSplit( split3 );
        implication.initializeCDOs( &context );

        TS_ASSERT_THROWS_NOTHING( smtCore.pushImplication( &implication ) );

        // Do another real split

        MockConstraint constraint2;

        // Split 4
        PiecewiseLinearCaseSplit split4;
        Tightening bound6( 7, 3.0, Tightening::LB );
        split4.storeBoundTightening( bound6 );

        PiecewiseLinearCaseSplit split5;
        Tightening bound7( 8, 13.0, Tightening::UB );
        split5.storeBoundTightening( bound7 );

        constraint2.registerCaseSplit( split4 );
        constraint2.registerCaseSplit( split5 );

        constraint2.initializeCDOs( &context );

        for ( unsigned i = 0; i < ( unsigned ) Options::get()->getInt( Options::CONSTRAINT_VIOLATION_THRESHOLD ); ++i )
            smtCore.reportViolatedConstraint( &constraint2 );

        constraint2.nextIsActive = true;

        TS_ASSERT( smtCore.needToSplit() );
        TS_ASSERT_THROWS_NOTHING( smtCore.decide() );
        TS_ASSERT( !smtCore.needToSplit() );

        // Check that everything is received in the correct order
        List<PiecewiseLinearCaseSplit> allSplitsSoFar;
        TS_ASSERT_THROWS_NOTHING( smtCore.allSplitsSoFar( allSplitsSoFar ) );

        TS_ASSERT_EQUALS( allSplitsSoFar.size(), 3U );

        auto it = allSplitsSoFar.begin();
        TS_ASSERT_EQUALS( *it, split1 );

        ++it;
        TS_ASSERT_EQUALS( *it, split3 );

        ++it;
        TS_ASSERT_EQUALS( *it, split4 );
    }

    /* void _test_store_smt_state() */
    /* { */
    /*     // ReLU(x0, x1) */
    /*     // ReLU(x2, x3) */
    /*     // ReLU(x4, x5) */

    /*     InputQuery inputQuery; */
    /*     inputQuery.setNumberOfVariables( 6 ); */

    /*     ReluConstraint relu1 = ReluConstraint( 0, 1 ); */
    /*     ReluConstraint relu2 = ReluConstraint( 2, 3 ); */

    /*     relu1.transformToUseAuxVariables( inputQuery ); */
    /*     relu2.transformToUseAuxVariables( inputQuery ); */

    /*     CVC4::context::Context context; */
    /*     SmtCore smtCore( engine, context ); */

    /*     MockConstraint implication; */
    /*     PiecewiseLinearCaseSplit split1; */
    /*     Tightening bound1( 1, 0.5, Tightening::LB ); */
    /*     split1.storeBoundTightening( bound1 ); */
    /*     implication.registerCaseSplit( split1 ); */
    /*     TS_ASSERT_THROWS_NOTHING( smtCore.pushImplication( &implication ) ); */

    /*     for ( unsigned i = 0; i < ( unsigned ) Options::get()->getInt( Options::CONSTRAINT_VIOLATION_THRESHOLD ); ++i ) */
    /*         smtCore.reportViolatedConstraint( &relu1 ); */

    /*     TS_ASSERT( smtCore.needToSplit() ); */
    /*     TS_ASSERT_THROWS_NOTHING( smtCore.decide() ); */
    /*     TS_ASSERT( !smtCore.needToSplit() ); */

    /*     MockConstraint implication2; */
    /*     PiecewiseLinearCaseSplit split2; */
    /*     Tightening bound2( 3, 2.3, Tightening::UB ); */
    /*     split2.storeBoundTightening( bound2 ); */
    /*     implication2.registerCaseSplit( split2 ); */
    /*     TS_ASSERT_THROWS_NOTHING( smtCore.pushImplication( &implication2 ) ); */

    /*     for ( unsigned i = 0; i < ( unsigned ) Options::get()->getInt( Options::CONSTRAINT_VIOLATION_THRESHOLD ); ++i ) */
    /*         smtCore.reportViolatedConstraint( &relu2 ); */

    /*     TS_ASSERT( smtCore.needToSplit() ); */
    /*     TS_ASSERT_THROWS_NOTHING( smtCore.decide() ); */
    /*     TS_ASSERT( !smtCore.needToSplit() ); */

    /*     SmtState smtState; */
    /*     smtCore.storeSmtState( smtState ); */
    /*     TS_ASSERT( smtState._impliedValidSplitsAtRoot.size() == 1 ); */
    /*     TS_ASSERT( *smtState._impliedValidSplitsAtRoot.begin() == split1 ); */

    /*     TS_ASSERT( smtState._stack.size() == 2 ); */
    /*     // Examine the first stackEntry */
    /*     SmtStackEntry *stackEntry = *( smtState._stack.begin() ); */
    /*     TS_ASSERT( stackEntry->_activeSplit == *( relu1.getCaseSplits().begin() ) ); */
    /*     TS_ASSERT( *( stackEntry->_alternativeSplits.begin() ) == *( ++relu1.getCaseSplits().begin() ) ); */
    /*     TS_ASSERT( stackEntry->_impliedValidSplits.size() == 1 ); */
    /*     TS_ASSERT( *( stackEntry->_impliedValidSplits.begin() ) == split2 ); */
    /*     // Examine the second stackEntry */
    /*     stackEntry = *( ++smtState._stack.begin() ); */
    /*     TS_ASSERT( stackEntry->_activeSplit == *( relu2.getCaseSplits().begin() ) ); */
    /*     TS_ASSERT( *( stackEntry->_alternativeSplits.begin() ) == *( ++relu2.getCaseSplits().begin() ) ); */
    /*     TS_ASSERT( stackEntry->_impliedValidSplits.size() == 0 ); */

    /*     clearSmtState( smtState ); */

    /*     TS_ASSERT_THROWS_NOTHING( smtCore.popSplit() ); */

    /*     smtCore.storeSmtState( smtState ); */
    /*     TS_ASSERT( smtState._impliedValidSplitsAtRoot.size() == 1 ); */
    /*     TS_ASSERT( *smtState._impliedValidSplitsAtRoot.begin() == split1 ); */

    /*     TS_ASSERT( smtState._stack.size() == 2 ); */
    /*     // Examine the first stackEntry */
    /*     stackEntry = *( smtState._stack.begin() ); */
    /*     TS_ASSERT( stackEntry->_activeSplit == *( relu1.getCaseSplits().begin() ) ); */
    /*     TS_ASSERT( *( stackEntry->_alternativeSplits.begin() ) == *( ++relu1.getCaseSplits().begin() ) ); */
    /*     TS_ASSERT( stackEntry->_impliedValidSplits.size() == 1 ); */
    /*     TS_ASSERT( *( stackEntry->_impliedValidSplits.begin() ) == split2 ); */
    /*     // Examine the second stackEntry */
    /*     stackEntry = *( ++smtState._stack.begin() ); */
    /*     TS_ASSERT( stackEntry->_activeSplit == *( ++relu2.getCaseSplits().begin() ) ); */
    /*     TS_ASSERT( stackEntry->_alternativeSplits.empty() ); */
    /*     TS_ASSERT( stackEntry->_impliedValidSplits.size() == 0 ); */

    /*     clearSmtState( smtState ); */

    /*     TS_ASSERT_THROWS_NOTHING( smtCore.popSplit() ); */
    /* } */

    /* void clearSmtState( SmtState &smtState ) */
    /* { */
    /*     for ( const auto &stackEntry : smtState._stack ) */
    /*         delete stackEntry; */
    /*     smtState._stack = List<SmtStackEntry *>(); */
    /*     smtState._impliedValidSplitsAtRoot = List<PiecewiseLinearCaseSplit>(); */
    /* } */

    void test_todo()
    {
        // Reason: the inefficiency in resizing the tableau mutliple times
        TS_TRACE( "add support for adding multiple equations at once, not one-by-one" );
    }
};

//
// Local Variables:
// compile-command: "make -C ../../.. "
// tags-file-name: "../../../TAGS"
// c-basic-offset: 4
// End:
//
