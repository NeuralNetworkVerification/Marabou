/*********************                                                        */
/*! \file Test_DisjunctionConstraint.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz, Parth Shah, Duligur Ibeling
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

**/

#include <cxxtest/TestSuite.h>

#include "DisjunctionConstraint.h"
#include "MarabouError.h"
#include "MockErrno.h"

class MockForDisjunctionConstraint
    : public MockErrno
{
public:
};

/*
   Exposes protected members of DisjunctionConstraint for testing.
 */
class TestDisjunctionConstraint : public DisjunctionConstraint
{
public:
    TestDisjunctionConstraint( const List<PiecewiseLinearCaseSplit> elements  )
        : DisjunctionConstraint( elements )
    {}

    using DisjunctionConstraint::getPhaseStatus;
};

using namespace CVC4::context;

class DisjunctionConstraintTestSuite : public CxxTest::TestSuite
{
public:
    MockForDisjunctionConstraint *mock;
    PiecewiseLinearCaseSplit *cs1;
    PiecewiseLinearCaseSplit *cs2;
    PiecewiseLinearCaseSplit *cs3;

    void setUp()
    {
        TS_ASSERT( mock = new MockForDisjunctionConstraint );

        TS_ASSERT( cs1 = new PiecewiseLinearCaseSplit );
        TS_ASSERT( cs2 = new PiecewiseLinearCaseSplit );
        TS_ASSERT( cs3 = new PiecewiseLinearCaseSplit );

        // x0 <= 1, x1 = 2
        cs1->storeBoundTightening( Tightening( 0, 1, Tightening::UB ) );
        Equation eq1;
        eq1.addAddend( 1, 1 );
        eq1.setScalar( 2 );
        cs1->addEquation( eq1 );

        // 1 <= x0 <= 5, x1 = x0
        cs2->storeBoundTightening( Tightening( 0, 1, Tightening::LB ) );
        cs2->storeBoundTightening( Tightening( 0, 5, Tightening::UB ) );
        Equation eq2;
        eq2.addAddend( 1, 0 );
        eq2.addAddend( -1, 1 );
        eq2.setScalar( 0 );
        cs2->addEquation( eq2 );

        // 5 <= x0 , x1 = 2x2 + 5
        cs3->storeBoundTightening( Tightening( 0, 5, Tightening::LB ) );
        Equation eq3;
        eq3.addAddend( 1, 1 );
        eq3.addAddend( -2, 2 );
        eq3.setScalar( 5 );
        cs3->addEquation( eq3 );
    }

    void tearDown()
    {
        TS_ASSERT_THROWS_NOTHING( delete cs3 );
        TS_ASSERT_THROWS_NOTHING( delete cs2 );
        TS_ASSERT_THROWS_NOTHING( delete cs1 );

        TS_ASSERT_THROWS_NOTHING( delete mock );
    }

    void test_get_case_splits()
    {
        List<PiecewiseLinearCaseSplit> caseSplits = { *cs1, *cs2, *cs3 };
        DisjunctionConstraint dc( caseSplits );

        List<PiecewiseLinearCaseSplit> returnedSplits;
        TS_ASSERT_THROWS_NOTHING( returnedSplits = dc.getCaseSplits() );

        TS_ASSERT_EQUALS( returnedSplits.size(), caseSplits.size() );
        TS_ASSERT_EQUALS( returnedSplits, caseSplits );
    }

    void test_getParticipatingVariables()
    {
        List<PiecewiseLinearCaseSplit> caseSplits = { *cs1, *cs2, *cs3 };
        DisjunctionConstraint dc( caseSplits );

        List<unsigned> varibales = dc.getParticipatingVariables();

        TS_ASSERT_EQUALS( varibales.size(), 3U );
        TS_ASSERT( varibales.exists( 0 ) );
        TS_ASSERT( varibales.exists( 1 ) );
        TS_ASSERT( varibales.exists( 2 ) );

        TS_ASSERT( dc.participatingVariable( 0 ) );
        TS_ASSERT( dc.participatingVariable( 1 ) );
        TS_ASSERT( dc.participatingVariable( 2 ) );
        TS_ASSERT( !dc.participatingVariable( 3 ) );

        PiecewiseLinearCaseSplit cs4;
        cs4.storeBoundTightening( Tightening( 5, 17, Tightening::LB ) );
        caseSplits = { cs4 };

        DisjunctionConstraint dc2( caseSplits );
        varibales = dc2.getParticipatingVariables();

        TS_ASSERT_EQUALS( varibales.size(), 1U );
        TS_ASSERT( varibales.exists( 5 ) );

        TS_ASSERT( !dc2.participatingVariable( 0 ) );
        TS_ASSERT( !dc2.participatingVariable( 1 ) );
        TS_ASSERT( dc2.participatingVariable( 5 ) );
        TS_ASSERT( !dc2.participatingVariable( 17 ) );
    }

    void test_satisfied()
    {
        List<PiecewiseLinearCaseSplit> caseSplits = { *cs1, *cs2, *cs3 };
        DisjunctionConstraint dc( caseSplits );

        /*
          x0 <= 1       -->   x1 = 2
          1 <= x0 <= 5  -->   x1 = x0
          5 <= x0       -->   x1 = 2x2 + 5
        */

        dc.notifyVariableValue( 0, -5 );
        dc.notifyVariableValue( 1, 2 );
        TS_ASSERT( dc.satisfied() );

        dc.notifyVariableValue( 0, -3 );
        TS_ASSERT( dc.satisfied() );

        dc.notifyVariableValue( 12, 4 );
        TS_ASSERT( dc.satisfied() );

        dc.notifyVariableValue( 0, 3 );
        TS_ASSERT( !dc.satisfied() );

        dc.notifyVariableValue( 1, 3 );
        TS_ASSERT( dc.satisfied() );

        dc.notifyVariableValue( 2, 4 );
        TS_ASSERT( dc.satisfied() );

        dc.notifyVariableValue( 1, 2 );
        TS_ASSERT( !dc.satisfied() );

        dc.notifyVariableValue( 0, 7 );
        dc.notifyVariableValue( 1, 7 );
        TS_ASSERT( !dc.satisfied() );

        dc.notifyVariableValue( 2, 1 );
        TS_ASSERT( dc.satisfied() );

        dc.notifyVariableValue( 0, 15 );
        TS_ASSERT( dc.satisfied() );

        dc.notifyVariableValue( 1, 8 );
        TS_ASSERT( !dc.satisfied() );
    }

    void test_phase_fixed()
    {
        List<PiecewiseLinearCaseSplit> caseSplits = { *cs1, *cs2, *cs3 };
        /*
          x0 <= 1       -->   x1 = 2
          1 <= x0 <= 5  -->   x1 = x0
          5 <= x0       -->   x1 = 2x2 + 5
        */

        {
            DisjunctionConstraint dc( caseSplits );

            dc.notifyLowerBound( 0, -10 );
            dc.notifyUpperBound( 0, 10 );
            dc.notifyLowerBound( 1, -10 );
            dc.notifyUpperBound( 1, 10 );
            dc.notifyLowerBound( 2, -10 );
            dc.notifyUpperBound( 2, 10 );

            TS_ASSERT( !dc.phaseFixed() );

            dc.notifyLowerBound( 0, -1 );
            TS_ASSERT( !dc.phaseFixed() );

            dc.notifyLowerBound( 0, 2 );
            TS_ASSERT( !dc.phaseFixed() );

            dc.notifyLowerBound( 0, 6 );
            TS_ASSERT( dc.phaseFixed() );

            PiecewiseLinearCaseSplit validSplit = dc.getValidCaseSplit();
            TS_ASSERT_EQUALS( validSplit, *cs3 );
        }

        {
            DisjunctionConstraint dc( caseSplits );

            dc.notifyLowerBound( 0, -10 );
            dc.notifyUpperBound( 0, 10 );
            dc.notifyLowerBound( 1, -10 );
            dc.notifyUpperBound( 1, 10 );
            dc.notifyLowerBound( 2, -10 );
            dc.notifyUpperBound( 2, 10 );

            TS_ASSERT( !dc.phaseFixed() );

            dc.notifyUpperBound( 0, 7 );
            TS_ASSERT( !dc.phaseFixed() );

            dc.notifyUpperBound( 0, 2 );
            TS_ASSERT( !dc.phaseFixed() );

            dc.notifyUpperBound( 0, -2 );
            TS_ASSERT( dc.phaseFixed() );

            PiecewiseLinearCaseSplit validSplit = dc.getValidCaseSplit();
            TS_ASSERT_EQUALS( validSplit, *cs1 );
        }
    }

    void test_initialization_of_CDOs()
    {
        Context context;

        List<PiecewiseLinearCaseSplit> caseSplits = { *cs1, *cs2, *cs3 };

        DisjunctionConstraint disjunction( caseSplits );

        TS_ASSERT_EQUALS( disjunction.getContext(), static_cast<Context*>( nullptr ) );
        TS_ASSERT_EQUALS( disjunction.getActiveStatusCDO(), static_cast<CDO<bool>*>( nullptr ) );
        TS_ASSERT_EQUALS( disjunction.getPhaseStatusCDO(), static_cast<CDO<PhaseStatus>*>( nullptr ) );
        TS_ASSERT_EQUALS( disjunction.getInfeasibleCasesCDList(), static_cast<CDList<PhaseStatus>*>( nullptr ) );
        TS_ASSERT_THROWS_NOTHING( disjunction.initializeCDOs( &context ) );
        TS_ASSERT_EQUALS( disjunction.getContext(), &context );
        TS_ASSERT_DIFFERS( disjunction.getActiveStatusCDO(), static_cast<CDO<bool>*>( nullptr ) );
        TS_ASSERT_DIFFERS( disjunction.getPhaseStatusCDO(), static_cast<CDO<PhaseStatus>*>( nullptr ) );
        TS_ASSERT_DIFFERS( disjunction.getInfeasibleCasesCDList(), static_cast<CDList<PhaseStatus>*>( nullptr ) );

        bool active = false;
        TS_ASSERT_THROWS_NOTHING( active = disjunction.isActive() );
        TS_ASSERT_EQUALS( active, true );

        bool phaseFixed = true;
        TS_ASSERT_THROWS_NOTHING( phaseFixed = disjunction.phaseFixed() );
        TS_ASSERT_EQUALS( phaseFixed, PHASE_NOT_FIXED );
        TS_ASSERT_EQUALS( disjunction.numFeasibleCases(), 3u );
    }

    /*
     * Test Case functionality of DisjunctionConstraint
     * 1. Check that all cases are returned by DisjunctionConstraint::getAllCases
     * 2. Check that DisjunctionConstraint::getCaseSplit( case ) returns the correct case
     */
    void test_disjunction_get_cases()
    {
        List<PiecewiseLinearCaseSplit> caseSplits = { *cs1, *cs2, *cs3 };
        DisjunctionConstraint disjunction( caseSplits );

        List<PhaseStatus> cases = disjunction.getAllCases();

        TS_ASSERT_EQUALS( cases.size(), caseSplits.size() );

        List<PiecewiseLinearCaseSplit> splits = disjunction.getCaseSplits();
        TS_ASSERT_EQUALS( splits.size(), caseSplits.size() );

        for ( auto split : caseSplits )
        {
            TS_ASSERT_DIFFERS( std::find( splits.begin(), splits.end(), split ), splits.end() );
        }
    }

    /*
      Test Disjunction's context-dependent state behavior.
     */
    void test_disjunction_context_dependent_state()
    {
        Context context;
        List<PiecewiseLinearCaseSplit> caseSplits = { *cs1, *cs2, *cs3 };
        TestDisjunctionConstraint dc( caseSplits );


        dc.initializeCDOs( &context );
        context.push();

        dc.notifyLowerBound( 0, -10 );
        dc.notifyUpperBound( 0, 10 );
        dc.notifyLowerBound( 1, -10 );
        dc.notifyUpperBound( 1, 10 );
        dc.notifyLowerBound( 2, -10 );
        dc.notifyUpperBound( 2, 10 );


        context.push();

        // L0 - Feasible, not an implication
        PhaseStatus phase1 = PHASE_NOT_FIXED;
        TS_ASSERT_THROWS_NOTHING( phase1 = dc.nextFeasibleCase() );
        TS_ASSERT_EQUALS( phase1, dc.nextFeasibleCase() );
        TS_ASSERT( dc.isFeasible() );
        TS_ASSERT( !dc.isImplication() );


        // L1 - Feasible, nextFeasibleCase returns a new case
        TS_ASSERT_THROWS_NOTHING( context.push() );
        TS_ASSERT_THROWS_NOTHING( dc.markInfeasible( phase1 ) );
        TS_ASSERT( dc.isFeasible() );
        TS_ASSERT( !dc.isImplication() );

        PhaseStatus phase2 = PHASE_NOT_FIXED;
        TS_ASSERT_THROWS_NOTHING( phase2 = dc.nextFeasibleCase() );
        TS_ASSERT_EQUALS( phase2, dc.nextFeasibleCase() );
        TS_ASSERT_DIFFERS( phase2, phase1 );

        TS_ASSERT_THROWS_NOTHING( dc.markInfeasible( phase2 ) );
        TS_ASSERT( dc.isFeasible() );
        TS_ASSERT( dc.isImplication() );

        PhaseStatus phase3 = PHASE_NOT_FIXED;
        TS_ASSERT_THROWS_NOTHING( phase3 = dc.nextFeasibleCase() );
        TS_ASSERT_EQUALS( phase3, dc.nextFeasibleCase() );
        TS_ASSERT_DIFFERS( phase3, phase1 );
        TS_ASSERT_DIFFERS( phase3, phase2 );

        // L2 - Infeasible, nextCase returns CONSTRAINT_INFEASIBLE
        TS_ASSERT_THROWS_NOTHING( context.push() );
        TS_ASSERT_THROWS_NOTHING( dc.markInfeasible( phase3 ) );
        TS_ASSERT( !dc.isFeasible() );
        TS_ASSERT( !dc.isImplication() );
        TS_ASSERT_EQUALS( dc.nextFeasibleCase(), CONSTRAINT_INFEASIBLE );

        // L1 again - Feasible, an implication, nextFeasibleCase returns same values as phase3
        TS_ASSERT_THROWS_NOTHING( context.pop() );
        TS_ASSERT( dc.isFeasible() );
        TS_ASSERT( dc.isImplication() );
        TS_ASSERT_EQUALS( phase3, dc.nextFeasibleCase() );

        // L0 again - Feasible, not an implication, nextFeasibleCase returns same value as phase1
        TS_ASSERT_THROWS_NOTHING( context.pop() );
        TS_ASSERT( dc.isFeasible() );
        TS_ASSERT( !dc.isImplication() );
        TS_ASSERT_EQUALS( phase1, dc.nextFeasibleCase() );

        // Test Bound to Cases propagation
        dc.notifyLowerBound( 0, -1 );
        TS_ASSERT( !dc.isImplication() );

        dc.notifyLowerBound( 0, 2 );
        TS_ASSERT( !dc.isImplication() );

        dc.notifyLowerBound( 0, 6 );
        TS_ASSERT( dc.isImplication() );

        PiecewiseLinearCaseSplit validSplit = dc.getImpliedCaseSplit();
        TS_ASSERT_EQUALS( validSplit, *cs3 );

        dc.markInfeasible( dc.getPhaseStatus() );
        TS_ASSERT( !dc.isFeasible() );
        TS_ASSERT_EQUALS( dc.nextFeasibleCase(), CONSTRAINT_INFEASIBLE );

    }

};

