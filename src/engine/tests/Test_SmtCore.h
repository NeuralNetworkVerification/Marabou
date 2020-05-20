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

#include "GlobalConfiguration.h"
#include "MockEngine.h"
#include "MockErrno.h"
#include "PiecewiseLinearConstraint.h"
#include "ReluConstraint.h"
#include "SmtCore.h"

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
        List<PiecewiseLinearCaseSplit> getCaseSplits() const
        {
            return nextSplits;
        }

        bool phaseFixed() const
        {
            return true;
        }

        PiecewiseLinearCaseSplit getValidCaseSplit() const
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

        SmtCore smtCore( engine );

        for ( unsigned i = 0; i < GlobalConfiguration::CONSTRAINT_VIOLATION_THRESHOLD - 1; ++i )
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
        SmtCore smtCore( engine );

        MockConstraint constraint;

        // Split 1
        PiecewiseLinearCaseSplit split1;
        Tightening bound1( 1, 3.0, Tightening::LB );
        Tightening bound2( 1, 5.0, Tightening::UB );

        Equation equation1( Equation::EQ );
        equation1.addAddend( 1, 0 );
        equation1.addAddend( 2, 1 );
        equation1.addAddend( -1, 2 );
        equation1.setScalar( 11 );

        split1.storeBoundTightening( bound1 );
        split1.storeBoundTightening( bound2 );
        split1.addEquation( equation1 );

        // Split 2
        PiecewiseLinearCaseSplit split2;
        Tightening bound3( 2, 13.0, Tightening::UB );
        Tightening bound4( 3, 25.0, Tightening::UB );

        Equation equation2( Equation::EQ );
        equation2.addAddend( -3, 0 );
        equation2.addAddend( 3, 1 );
        equation2.setScalar( -5 );

        split2.storeBoundTightening( bound3 );
        split2.storeBoundTightening( bound4 );
        split2.addEquation( equation2 );

        // Split 3
        PiecewiseLinearCaseSplit split3;
        Tightening bound5( 14, 2.3, Tightening::LB );

        split3.storeBoundTightening( bound5 );
        split3.addEquation( equation1 );
        split3.addEquation( equation2 );

        // Store the splits
        constraint.nextSplits.append( split1 );
        constraint.nextSplits.append( split2 );
        constraint.nextSplits.append( split3 );

        for ( unsigned i = 0; i < GlobalConfiguration::CONSTRAINT_VIOLATION_THRESHOLD; ++i )
            smtCore.reportViolatedConstraint( &constraint );

        engine->lastStoredState = NULL;
        engine->lastRestoredState = NULL;

        TS_ASSERT( smtCore.needToSplit() );
        TS_ASSERT_EQUALS( smtCore.getStackDepth(), 0U );
        TS_ASSERT( !constraint.setActiveWasCalled );
        TS_ASSERT_THROWS_NOTHING( smtCore.performSplit() );
        TS_ASSERT( constraint.setActiveWasCalled );
        TS_ASSERT( !smtCore.needToSplit() );
        TS_ASSERT_EQUALS( smtCore.getStackDepth(), 1U );

        // Check that Split1 was performed and tableau state was stored
        TS_ASSERT_EQUALS( engine->lastLowerBounds.size(), 1U );
        TS_ASSERT_EQUALS( engine->lastLowerBounds.begin()->_variable, 1U );
        TS_ASSERT_EQUALS( engine->lastLowerBounds.begin()->_bound, 3.0 );

        TS_ASSERT_EQUALS( engine->lastUpperBounds.size(), 1U );
        TS_ASSERT_EQUALS( engine->lastUpperBounds.begin()->_variable, 1U );
        TS_ASSERT_EQUALS( engine->lastUpperBounds.begin()->_bound, 5.0 );

        TS_ASSERT_EQUALS( engine->lastEquations.size(), 1U );
        Equation equation4 = equation1;
        TS_ASSERT_EQUALS( *engine->lastEquations.begin(), equation4 );

        TS_ASSERT( engine->lastStoredState );
        TS_ASSERT( !engine->lastRestoredState );

        EngineState *originalState = engine->lastStoredState;
        engine->lastStoredState = NULL;
        engine->lastLowerBounds.clear();
        engine->lastUpperBounds.clear();
        engine->lastEquations.clear();

        // Pop Split1, check that the tableau was restored and that
        // a Split2 was performed
        TS_ASSERT( smtCore.popSplit() );
        TS_ASSERT_EQUALS( smtCore.getStackDepth(), 1U );

        TS_ASSERT_EQUALS( engine->lastRestoredState, originalState );
        TS_ASSERT( !engine->lastStoredState );
        engine->lastRestoredState = NULL;

        TS_ASSERT( engine->lastLowerBounds.empty() );

        TS_ASSERT_EQUALS( engine->lastUpperBounds.size(), 2U );
        auto it = engine->lastUpperBounds.begin();
        TS_ASSERT_EQUALS( it->_variable, 2U );
        TS_ASSERT_EQUALS( it->_bound, 13.0 );
        ++it;
        TS_ASSERT_EQUALS( it->_variable, 3U );
        TS_ASSERT_EQUALS( it->_bound, 25.0 );

        TS_ASSERT_EQUALS( engine->lastEquations.size(), 1U );
        Equation equation5 = equation2;
        TS_ASSERT_EQUALS( *engine->lastEquations.begin(), equation5 );

        engine->lastRestoredState = NULL;
        engine->lastLowerBounds.clear();
        engine->lastUpperBounds.clear();
        engine->lastEquations.clear();

        // Pop Split2, check that the tableau was restored and that
        // a Split3 was performed
        TS_ASSERT( smtCore.popSplit() );
        TS_ASSERT_EQUALS( smtCore.getStackDepth(), 1U );

        TS_ASSERT_EQUALS( engine->lastRestoredState, originalState );
        TS_ASSERT( !engine->lastStoredState );
        engine->lastRestoredState = NULL;

        TS_ASSERT_EQUALS( engine->lastLowerBounds.size(), 1U );
        it = engine->lastLowerBounds.begin();
        TS_ASSERT_EQUALS( it->_variable, 14U );
        TS_ASSERT_EQUALS( it->_bound, 2.3 );

        TS_ASSERT( engine->lastUpperBounds.empty() );

        TS_ASSERT_EQUALS( engine->lastEquations.size(), 2U );
        auto equation = engine->lastEquations.begin();
        Equation equation6 = equation1;
        TS_ASSERT_EQUALS( *equation, equation6 );
        ++equation;
        Equation equation7 = equation2;
        TS_ASSERT_EQUALS( *equation, equation7 );

        engine->lastRestoredState = NULL;
        engine->lastLowerBounds.clear();
        engine->lastUpperBounds.clear();
        engine->lastEquations.clear();

        // Final pop
        TS_ASSERT( !smtCore.popSplit() );
        TS_ASSERT( !engine->lastRestoredState );
        TS_ASSERT_EQUALS( smtCore.getStackDepth(), 0U );
    }

    void test_perform_split__inactive_constraint()
    {
        SmtCore smtCore( engine );

        MockConstraint constraint;

        // Split 1
        PiecewiseLinearCaseSplit split1;
        Tightening bound1( 1, 3.0, Tightening::LB );
        Tightening bound2( 1, 5.0, Tightening::UB );

        Equation equation1( Equation::EQ );
        equation1.addAddend( 1, 0 );
        equation1.addAddend( 2, 1 );
        equation1.addAddend( -1, 2 );
        equation1.setScalar( 11 );

        split1.storeBoundTightening( bound1 );
        split1.storeBoundTightening( bound2 );
        split1.addEquation( equation1 );

        // Split 2
        PiecewiseLinearCaseSplit split2;
        Tightening bound3( 2, 13.0, Tightening::UB );
        Tightening bound4( 3, 25.0, Tightening::UB );

        Equation equation2( Equation::EQ );
        equation2.addAddend( -3, 0 );
        equation2.addAddend( 3, 1 );
        equation2.setScalar( -5 );

        split2.storeBoundTightening( bound3 );
        split2.storeBoundTightening( bound4 );
        split2.addEquation( equation2 );

        // Split 3
        PiecewiseLinearCaseSplit split3;
        Tightening bound5( 14, 2.3, Tightening::LB );

        split3.storeBoundTightening( bound5 );
        split3.addEquation( equation1 );
        split3.addEquation( equation2 );

        // Store the splits
        constraint.nextSplits.append( split1 );
        constraint.nextSplits.append( split2 );
        constraint.nextSplits.append( split3 );

        for ( unsigned i = 0; i < GlobalConfiguration::CONSTRAINT_VIOLATION_THRESHOLD; ++i )
            smtCore.reportViolatedConstraint( &constraint );

        constraint.nextIsActive = false;

        TS_ASSERT( smtCore.needToSplit() );
        TS_ASSERT_THROWS_NOTHING( smtCore.performSplit() );
        TS_ASSERT( !smtCore.needToSplit() );

        // Check that no split was performed

        TS_ASSERT( engine->lastLowerBounds.empty() );
        TS_ASSERT( engine->lastUpperBounds.empty() );
        TS_ASSERT( engine->lastEquations.empty() );
        TS_ASSERT( !engine->lastStoredState );
    }

    void test_all_splits_so_far()
    {
        SmtCore smtCore( engine );

        MockConstraint constraint;

        // Split 1
        PiecewiseLinearCaseSplit split1;
        Tightening bound1( 1, 3.0, Tightening::LB );
        Tightening bound2( 1, 5.0, Tightening::UB );

        Equation equation1( Equation::EQ );
        equation1.addAddend( 1, 0 );
        equation1.addAddend( 2, 1 );
        equation1.addAddend( -1, 2 );
        equation1.setScalar( 11 );

        split1.storeBoundTightening( bound1 );
        split1.storeBoundTightening( bound2 );
        split1.addEquation( equation1 );

        // Split 2
        PiecewiseLinearCaseSplit split2;
        Tightening bound3( 2, 13.0, Tightening::UB );
        Tightening bound4( 3, 25.0, Tightening::UB );

        Equation equation2( Equation::EQ );
        equation2.addAddend( -3, 0 );
        equation2.addAddend( 3, 1 );
        equation2.setScalar( -5 );

        split2.storeBoundTightening( bound3 );
        split2.storeBoundTightening( bound4 );
        split2.addEquation( equation2 );

        // Store the splits
        constraint.nextSplits.append( split1 );
        constraint.nextSplits.append( split2 );

        for ( unsigned i = 0; i < GlobalConfiguration::CONSTRAINT_VIOLATION_THRESHOLD; ++i )
            smtCore.reportViolatedConstraint( &constraint );

        constraint.nextIsActive = true;

        TS_ASSERT( smtCore.needToSplit() );
        TS_ASSERT_THROWS_NOTHING( smtCore.performSplit() );
        TS_ASSERT( !smtCore.needToSplit() );

        // Register a valid split

        // Split 3
        PiecewiseLinearCaseSplit split3;
        Tightening bound5( 14, 2.3, Tightening::LB );

        TS_ASSERT_THROWS_NOTHING( smtCore.recordImpliedValidSplit( split3 ) );

        // Do another real split

        MockConstraint constraint2;

        // Split 4
        PiecewiseLinearCaseSplit split4;
        Tightening bound6( 7, 3.0, Tightening::LB );
        split4.storeBoundTightening( bound6 );

        PiecewiseLinearCaseSplit split5;
        Tightening bound7( 8, 13.0, Tightening::UB );
        split5.storeBoundTightening( bound7 );

        constraint2.nextSplits.append( split4 );
        constraint2.nextSplits.append( split5 );

        for ( unsigned i = 0; i < GlobalConfiguration::CONSTRAINT_VIOLATION_THRESHOLD; ++i )
            smtCore.reportViolatedConstraint( &constraint2 );

        constraint2.nextIsActive = true;

        TS_ASSERT( smtCore.needToSplit() );
        TS_ASSERT_THROWS_NOTHING( smtCore.performSplit() );
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
