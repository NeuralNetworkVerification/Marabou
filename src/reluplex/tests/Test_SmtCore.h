 /*********************                                                        */
/*! \file Test_SmtCore.h
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
        bool participatingVariable( unsigned ) const
        {
            return true;
        }

        List<unsigned> getParticiatingVariables() const
        {
            return List<unsigned>();
        }

        bool satisfied( const Map<unsigned, double> & ) const
        {
            return true;
        }


        List<PiecewiseLinearConstraint::Fix> getPossibleFixes( const Map<unsigned, double> & ) const
        {
            return List<PiecewiseLinearConstraint::Fix>();
        }

        List<PiecewiseLinearCaseSplit> nextSplits;
        List<PiecewiseLinearCaseSplit> getCaseSplits() const
        {
            return nextSplits;
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

        for ( unsigned i = 0; i < SmtCore::SPLIT_THRESHOLD - 1; ++i )
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
        PiecewiseLinearCaseSplit::Bound bound1( 1, PiecewiseLinearCaseSplit::Bound::LOWER, 3.0 );
        PiecewiseLinearCaseSplit::Bound bound2( 1, PiecewiseLinearCaseSplit::Bound::UPPER, 5.0 );

        Equation equation1;
        equation1.addAddend( 1, 0 );
        equation1.addAddend( 2, 1 );
        equation1.addAddend( -1, 2 );
        equation1.addAddend( 1, 3 );
        equation1.setScalar( 11 );
        equation1.markAuxiliaryVariable( 3 );

        split1.storeBoundTightening( bound1 );
        split1.storeBoundTightening( bound2 );
        split1.setEquation( equation1 );

        // Split 2
        PiecewiseLinearCaseSplit split2;
        PiecewiseLinearCaseSplit::Bound bound3( 2, PiecewiseLinearCaseSplit::Bound::UPPER, 13.0 );
        PiecewiseLinearCaseSplit::Bound bound4( 3, PiecewiseLinearCaseSplit::Bound::UPPER, 25.0 );

        Equation equation2;
        equation2.addAddend( -3, 0 );
        equation2.addAddend( 3, 1 );
        equation2.addAddend( 1, 4 );
        equation2.setScalar( -5 );
        equation2.markAuxiliaryVariable( 4 );

        split2.storeBoundTightening( bound3 );
        split2.storeBoundTightening( bound4 );
        split2.setEquation( equation2 );

        // Split 3
        PiecewiseLinearCaseSplit split3;
        PiecewiseLinearCaseSplit::Bound bound5( 14, PiecewiseLinearCaseSplit::Bound::LOWER, 2.3 );

        split3.storeBoundTightening( bound5 );
        split3.setEquation( equation1 );

        // Store the splits
        constraint.nextSplits.append( split1 );
        constraint.nextSplits.append( split2 );
        constraint.nextSplits.append( split3 );

        for ( unsigned i = 0; i < SmtCore::SPLIT_THRESHOLD; ++i )
            smtCore.reportViolatedConstraint( &constraint );

        engine->lastStoredState = NULL;
        engine->lastRestoredState = NULL;

        TS_ASSERT( smtCore.needToSplit() );
        TS_ASSERT_EQUALS( smtCore.getStackDepth(), 0U );
        TS_ASSERT_THROWS_NOTHING( smtCore.performSplit() );
        TS_ASSERT_EQUALS( smtCore.getStackDepth(), 1U );

        // Check that Split1 was performed and tableau state was stored
        TS_ASSERT_EQUALS( engine->lastLowerBounds.size(), 1U );
        TS_ASSERT_EQUALS( engine->lastLowerBounds.begin()->_variable, 1U );
        TS_ASSERT_EQUALS( engine->lastLowerBounds.begin()->_bound, 3.0 );

        TS_ASSERT_EQUALS( engine->lastUpperBounds.size(), 1U );
        TS_ASSERT_EQUALS( engine->lastUpperBounds.begin()->_variable, 1U );
        TS_ASSERT_EQUALS( engine->lastUpperBounds.begin()->_bound, 5.0 );

        TS_ASSERT_EQUALS( engine->lastEquations.size(), 1U );
        TS_ASSERT_EQUALS( *engine->lastEquations.begin(), equation1 );

        TS_ASSERT( engine->lastStoredState );
        TS_ASSERT( !engine->lastRestoredState );

        TableauState *originalState = engine->lastStoredState;
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

        TS_ASSERT_EQUALS( engine->lastLowerBounds.size(), 0U );

        TS_ASSERT_EQUALS( engine->lastUpperBounds.size(), 2U );
        auto it = engine->lastUpperBounds.begin();
        TS_ASSERT_EQUALS( it->_variable, 2U );
        TS_ASSERT_EQUALS( it->_bound, 13.0 );
        ++it;
        TS_ASSERT_EQUALS( it->_variable, 3U );
        TS_ASSERT_EQUALS( it->_bound, 25.0 );

        TS_ASSERT_EQUALS( engine->lastEquations.size(), 1U );
        TS_ASSERT_EQUALS( *engine->lastEquations.begin(), equation2 );

        engine->lastRestoredState = NULL;
        engine->lastLowerBounds.clear();
        engine->lastUpperBounds.clear();
        engine->lastEquations.clear();

        // Pop Split2, check that the tableau was restored and that
        // a Split3 was performed
        TS_ASSERT( smtCore.popSplit() );
        TS_ASSERT_EQUALS( smtCore.getStackDepth(), 0U );

        TS_ASSERT_EQUALS( engine->lastRestoredState, originalState );
        TS_ASSERT( !engine->lastStoredState );
        engine->lastRestoredState = NULL;

        TS_ASSERT_EQUALS( engine->lastLowerBounds.size(), 1U );
        it = engine->lastLowerBounds.begin();
        TS_ASSERT_EQUALS( it->_variable, 14U );
        TS_ASSERT_EQUALS( it->_bound, 2.3 );

        TS_ASSERT_EQUALS( engine->lastUpperBounds.size(), 0U );

        TS_ASSERT_EQUALS( engine->lastEquations.size(), 1U );
        TS_ASSERT_EQUALS( *engine->lastEquations.begin(), equation1 );

        engine->lastRestoredState = NULL;
        engine->lastLowerBounds.clear();
        engine->lastUpperBounds.clear();
        engine->lastEquations.clear();

        // Final pop
        TS_ASSERT( !smtCore.popSplit() );
        TS_ASSERT( !engine->lastRestoredState );
        TS_ASSERT_EQUALS( smtCore.getStackDepth(), 0U );
    }
};

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
