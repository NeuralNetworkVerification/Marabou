/*********************                                                        */
/*! \file Test_ReluConstraint.h
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

#include "GlobalConfiguration.h"
#include "InputQuery.h"
#include "MockConstraintBoundTightener.h"
#include "MockErrno.h"
#include "MockTableau.h"
#include "PiecewiseLinearCaseSplit.h"
#include "LeakyReluConstraint.h"
#include "MarabouError.h"
#include "context/context.h"

#include <string.h>

class MockForLeakyReluConstraint
    : public MockErrno
{
public:
};

/*
   Exposes protected members of ReluConstraint for testing.
 */
class TestLeakyReluConstraint : public LeakyReluConstraint
{
public:
 TestLeakyReluConstraint( unsigned b, unsigned f )
     : LeakyReluConstraint( b, f )
    {}

    using LeakyReluConstraint::getPhaseStatus;
};

using namespace CVC4::context;

class LeakyReluConstraintTestSuite : public CxxTest::TestSuite
{
public:
    MockForLeakyReluConstraint *mock;

    void setUp()
    {
        TS_ASSERT( mock = new MockForLeakyReluConstraint );;
    }

    void tearDown()
    {
        TS_ASSERT_THROWS_NOTHING( delete mock );
    }

    void test_relu_constraint()
    {
        unsigned b = 1;
        unsigned f = 4;

        LeakyReluConstraint relu( b, f );

        List<unsigned> participatingVariables;
        TS_ASSERT_THROWS_NOTHING( participatingVariables = relu.getParticipatingVariables() );
        TS_ASSERT_EQUALS( participatingVariables.size(), 2U );
        auto it = participatingVariables.begin();
        TS_ASSERT_EQUALS( *it, b );
        ++it;
        TS_ASSERT_EQUALS( *it, f );

        TS_ASSERT( relu.participatingVariable( b ) );
        TS_ASSERT( relu.participatingVariable( f ) );
        TS_ASSERT( !relu.participatingVariable( 2 ) );
        TS_ASSERT( !relu.participatingVariable( 0 ) );
        TS_ASSERT( !relu.participatingVariable( 5 ) );

        TS_ASSERT_THROWS_EQUALS( relu.satisfied(),
                                 const MarabouError &e,
                                 e.getCode(),
                                 MarabouError::PARTICIPATING_VARIABLES_ABSENT );

        relu.notifyVariableValue( b, 1 );
        relu.notifyVariableValue( f, 1 );

        TS_ASSERT( relu.satisfied() );

        relu.notifyVariableValue( f, 2 );

        TS_ASSERT( !relu.satisfied() );

        relu.notifyVariableValue( b, 2 );

        TS_ASSERT( relu.satisfied() );

        relu.notifyVariableValue( b, -2 );

        TS_ASSERT( !relu.satisfied() );

        relu.notifyVariableValue( f, -2 * GlobalConfiguration::LEAKY_RELU_SLOPE );

        TS_ASSERT( relu.satisfied() );

        relu.notifyVariableValue( b, -3 );

        TS_ASSERT( !relu.satisfied() );

        relu.notifyVariableValue( b, 0 );

        TS_ASSERT( !relu.satisfied() );

        relu.notifyVariableValue( f, 0 );

        TS_ASSERT( relu.satisfied() );

        relu.notifyVariableValue( f, 0 );
        relu.notifyVariableValue( b, 11 );

        TS_ASSERT( !relu.satisfied() );

        // Changing variable indices
        relu.notifyVariableValue( b, 1 );
        relu.notifyVariableValue( f, 1 );
        TS_ASSERT( relu.satisfied() );

        unsigned newB = 12;
        unsigned newF = 14;

        TS_ASSERT_THROWS_NOTHING( relu.updateVariableIndex( b, newB ) );
        TS_ASSERT_THROWS_NOTHING( relu.updateVariableIndex( f, newF ) );

        TS_ASSERT( relu.satisfied() );

        relu.notifyVariableValue( newF, 2 );

        TS_ASSERT( !relu.satisfied() );

        relu.notifyVariableValue( newB, 2 );

        TS_ASSERT( relu.satisfied() );
    }

    void test_relu_case_splits()
    {
        unsigned b = 1;
        unsigned f = 4;

        LeakyReluConstraint relu( b, f );

        List<PiecewiseLinearConstraint::Fix> fixes;
        List<PiecewiseLinearConstraint::Fix>::iterator it;

        List<PiecewiseLinearCaseSplit> splits = relu.getCaseSplits();

        Equation activeEquation, inactiveEquation;

        TS_ASSERT_EQUALS( splits.size(), 2U );

        List<PiecewiseLinearCaseSplit>::iterator split1 = splits.begin();
        List<PiecewiseLinearCaseSplit>::iterator split2 = split1;
        ++split2;

        TS_ASSERT( isActiveSplit( b, f, split1 ) || isActiveSplit( b, f, split2 ) );
        TS_ASSERT( isInactiveSplit( b, f, split1 ) || isInactiveSplit( b, f, split2 ) );
    }

    bool isActiveSplit( unsigned b, unsigned f, List<PiecewiseLinearCaseSplit>::iterator &split )
    {
        List<Tightening> bounds = split->getBoundTightenings();

        auto bound = bounds.begin();
        Tightening bound1 = *bound;

        TS_ASSERT_EQUALS( bound1._variable, b );
        TS_ASSERT_EQUALS( bound1._value, 0.0 );

        if ( bound1._type != Tightening::LB )
            return false;

        TS_ASSERT_EQUALS( bounds.size(), 2U );

        ++bound;
        Tightening bound2 = *bound;

        TS_ASSERT_EQUALS( bound2._variable, f );
        TS_ASSERT_EQUALS( bound2._value, 0.0 );
        TS_ASSERT_EQUALS( bound2._type, Tightening::LB );

        Equation activeEquation;
        auto equations = split->getEquations();
        TS_ASSERT_EQUALS( equations.size(), 1U );
        activeEquation = split->getEquations().front();
        TS_ASSERT_EQUALS( activeEquation._addends.size(), 2U );
        TS_ASSERT_EQUALS( activeEquation._scalar, 0.0 );

        auto addend = activeEquation._addends.begin();
        TS_ASSERT_EQUALS( addend->_coefficient, 1.0 );
        TS_ASSERT_EQUALS( addend->_variable, b );

        ++addend;
        TS_ASSERT_EQUALS( addend->_coefficient, -1.0 );
        TS_ASSERT_EQUALS( addend->_variable, f );

        TS_ASSERT_EQUALS( activeEquation._type, Equation::EQ );

        return true;
    }

    bool isInactiveSplit( unsigned b, unsigned f, List<PiecewiseLinearCaseSplit>::iterator &split )
    {
        List<Tightening> bounds = split->getBoundTightenings();

        auto bound = bounds.begin();
        Tightening bound1 = *bound;

        TS_ASSERT_EQUALS( bound1._variable, b );
        TS_ASSERT_EQUALS( bound1._value, 0.0 );

        if ( bound1._type != Tightening::UB )
            return false;

        TS_ASSERT_EQUALS( bounds.size(), 2U );

        ++bound;
        Tightening bound2 = *bound;

        TS_ASSERT_EQUALS( bound2._variable, f );
        TS_ASSERT_EQUALS( bound2._value, 0.0 );
        TS_ASSERT_EQUALS( bound2._type, Tightening::UB );

        Equation inactiveEquation;
        auto equations = split->getEquations();
        TS_ASSERT_EQUALS( equations.size(), 1U );
        inactiveEquation = split->getEquations().front();
        TS_ASSERT_EQUALS( inactiveEquation._addends.size(), 2U );
        TS_ASSERT_EQUALS( inactiveEquation._scalar, 0.0 );

        auto addend = inactiveEquation._addends.begin();
        TS_ASSERT_EQUALS( addend->_coefficient, GlobalConfiguration::LEAKY_RELU_SLOPE );
        TS_ASSERT_EQUALS( addend->_variable, b );

        ++addend;
        TS_ASSERT_EQUALS( addend->_coefficient, -1.0 );
        TS_ASSERT_EQUALS( addend->_variable, f );

        TS_ASSERT_EQUALS( inactiveEquation._type, Equation::EQ );

        return true;
    }

    void test_register_as_watcher()
    {
        unsigned b = 1;
        unsigned f = 4;

        MockTableau tableau;

        LeakyReluConstraint relu( b, f );

        TS_ASSERT_THROWS_NOTHING( relu.registerAsWatcher( &tableau ) );

        TS_ASSERT_EQUALS( tableau.lastRegisteredVariableToWatcher.size(), 2U );
        TS_ASSERT( tableau.lastUnregisteredVariableToWatcher.empty() );
        TS_ASSERT_EQUALS( tableau.lastRegisteredVariableToWatcher[b].size(), 1U );
        TS_ASSERT( tableau.lastRegisteredVariableToWatcher[b].exists( &relu ) );
        TS_ASSERT_EQUALS( tableau.lastRegisteredVariableToWatcher[f].size(), 1U );
        TS_ASSERT( tableau.lastRegisteredVariableToWatcher[f].exists( &relu ) );

        tableau.lastRegisteredVariableToWatcher.clear();

        TS_ASSERT_THROWS_NOTHING( relu.unregisterAsWatcher( &tableau ) );

        TS_ASSERT( tableau.lastRegisteredVariableToWatcher.empty() );
        TS_ASSERT_EQUALS( tableau.lastUnregisteredVariableToWatcher.size(), 2U );
        TS_ASSERT_EQUALS( tableau.lastUnregisteredVariableToWatcher[b].size(), 1U );
        TS_ASSERT( tableau.lastUnregisteredVariableToWatcher[b].exists( &relu ) );
        TS_ASSERT_EQUALS( tableau.lastUnregisteredVariableToWatcher[f].size(), 1U );
        TS_ASSERT( tableau.lastUnregisteredVariableToWatcher[f].exists( &relu ) );
    }

    void test_valid_split_relu_phase_fixed_to_active()
    {
        unsigned b = 1;
        unsigned f = 4;

        LeakyReluConstraint relu( b, f );

        TS_ASSERT( !relu.phaseFixed() );
        TS_ASSERT_THROWS_NOTHING( relu.notifyLowerBound( b, 5 ) );
        TS_ASSERT( relu.phaseFixed() );

        PiecewiseLinearCaseSplit split;
        TS_ASSERT_THROWS_NOTHING( split = relu.getValidCaseSplit() );

        List<PiecewiseLinearCaseSplit> dummy;
        dummy.append( split );
        List<PiecewiseLinearCaseSplit>::iterator split1 = dummy.begin();

        TS_ASSERT( isActiveSplit( b, f, split1 ) );
    }

    void test_valid_split_relu_phase_fixed_to_inactive()
    {
        unsigned b = 1;
        unsigned f = 4;

        LeakyReluConstraint relu( b, f );

        TS_ASSERT( !relu.phaseFixed() );
        TS_ASSERT_THROWS_NOTHING( relu.notifyUpperBound( b, -2 ) );
        TS_ASSERT( relu.phaseFixed() );

        PiecewiseLinearCaseSplit split;
        TS_ASSERT_THROWS_NOTHING( split = relu.getValidCaseSplit() );

        List<PiecewiseLinearCaseSplit> dummy;
        dummy.append( split );
        List<PiecewiseLinearCaseSplit>::iterator split1 = dummy.begin();

        TS_ASSERT( isInactiveSplit( b, f, split1 ) );
    }

    void test_relu_entailed_tightenings()
    {
        unsigned b = 1;
        unsigned f = 4;
        double slope = GlobalConfiguration::LEAKY_RELU_SLOPE;

        LeakyReluConstraint relu( b, f );

        relu.notifyUpperBound( b, 7 );
        relu.notifyUpperBound( f, 7 );

        relu.notifyLowerBound( b, -1 );
        relu.notifyLowerBound( f, slope * -1 );

        List<Tightening> entailedTightenings;
        relu.getEntailedTightenings( entailedTightenings );

        // The phase is not fixed: upper bounds propagated between b
        // and f, f  non-negative
        TS_ASSERT_EQUALS( entailedTightenings.size(), 4U );
        TS_ASSERT( entailedTightenings.exists( Tightening( f, 7, Tightening::UB ) ) );
        TS_ASSERT( entailedTightenings.exists( Tightening( b, 7, Tightening::UB ) ) );
        TS_ASSERT( entailedTightenings.exists( Tightening( f, slope * -1, Tightening::LB ) ) );
        TS_ASSERT( entailedTightenings.exists( Tightening( b, -1, Tightening::LB ) ) );

        entailedTightenings.clear();

        // Positive lower bounds for b and f: active case. All bounds
        // propagated between f and b. f and b are
        // non-negative.
        relu.notifyLowerBound( b, 1 );
        relu.notifyLowerBound( f, 2 );

        relu.getEntailedTightenings( entailedTightenings );

        TS_ASSERT_EQUALS( entailedTightenings.size(), 6U );
        TS_ASSERT( entailedTightenings.exists( Tightening( f, 1, Tightening::LB ) ) );
        TS_ASSERT( entailedTightenings.exists( Tightening( b, 2, Tightening::LB ) ) );
        TS_ASSERT( entailedTightenings.exists( Tightening( f, 7, Tightening::UB ) ) );
        TS_ASSERT( entailedTightenings.exists( Tightening( b, 7, Tightening::UB ) ) );

        TS_ASSERT( entailedTightenings.exists( Tightening( b, 0, Tightening::LB ) ) );
        TS_ASSERT( entailedTightenings.exists( Tightening( f, 0, Tightening::LB ) ) );

        entailedTightenings.clear();

        // Negative upper bound for b: inactive case. f is 0. B is non-positive.

        LeakyReluConstraint relu2( b, f );

        relu2.notifyLowerBound( b, -3 );
        relu2.notifyLowerBound( f, -2 );

        relu2.notifyUpperBound( b, -1 );
        relu2.notifyUpperBound( f, 7 );

        relu2.getEntailedTightenings( entailedTightenings );

        TS_ASSERT_EQUALS( entailedTightenings.size(), 5U );

        TS_ASSERT( entailedTightenings.exists( Tightening( b, 0, Tightening::UB ) ) );
        TS_ASSERT( entailedTightenings.exists( Tightening( f, 0, Tightening::UB ) ) );

        TS_ASSERT( entailedTightenings.exists( Tightening( b, -2 / slope, Tightening::LB ) ) );
        TS_ASSERT( entailedTightenings.exists( Tightening( f, -3 * slope, Tightening::LB ) ) );

        TS_ASSERT( entailedTightenings.exists( Tightening( f, -1 * slope, Tightening::UB ) ) );
    }

    void test_relu_duplicate_and_restore()
    {
        LeakyReluConstraint *relu1 = new LeakyReluConstraint( 4, 6 );
        relu1->setActiveConstraint( false );
        relu1->notifyVariableValue( 4, 1.0 );
        relu1->notifyVariableValue( 6, 1.0 );

        relu1->notifyLowerBound( 4, -8.0 );
        relu1->notifyUpperBound( 4, 8.0 );

        relu1->notifyLowerBound( 6, 0.0 );
        relu1->notifyUpperBound( 6, 8.0 );

        PiecewiseLinearConstraint *relu2 = relu1->duplicateConstraint();

        relu1->notifyVariableValue( 4, -2 );
        TS_ASSERT( !relu1->satisfied() );

        TS_ASSERT( !relu2->isActive() );
        TS_ASSERT( relu2->satisfied() );

        relu2->restoreState( relu1 );
        TS_ASSERT( !relu2->satisfied() );

        TS_ASSERT_THROWS_NOTHING( delete relu1 );
        TS_ASSERT_THROWS_NOTHING( delete relu2 );
    }

    void test_eliminate_variable_active()
    {
        unsigned b = 1;
        unsigned f = 4;

        MockTableau tableau;

        LeakyReluConstraint relu( b, f );

        relu.registerAsWatcher( &tableau );

        TS_ASSERT( !relu.constraintObsolete() );
        TS_ASSERT_THROWS_NOTHING( relu.eliminateVariable( b, 5 ) );
        TS_ASSERT( relu.constraintObsolete() );
    }

    void test_serialize_and_unserialize()
    {
        // TODO: test serialize Leaky ReLU
    }

    LeakyReluConstraint prepareLeakyRelu( unsigned b, unsigned f, IConstraintBoundTightener *tightener )
    {
        LeakyReluConstraint relu( b, f );

        relu.notifyLowerBound( b, -10 );
        relu.notifyLowerBound( f, GlobalConfiguration::LEAKY_RELU_SLOPE * -10 );

        relu.notifyUpperBound( b, 15 );
        relu.notifyUpperBound( f, 15 );

        relu.registerConstraintBoundTightener( tightener );

        return relu;
    }

    void test_notify_bounds()
    {
        double slope = GlobalConfiguration::LEAKY_RELU_SLOPE;
        unsigned b = 1;
        unsigned f = 4;
        MockConstraintBoundTightener tightener;
        List<Tightening> tightenings;

        tightener.getConstraintTightenings( tightenings );

        // Initial state: b in [-10, 15], f in [slope * -10, 15]

        {
            LeakyReluConstraint relu = prepareLeakyRelu( b, f, &tightener );

            relu.notifyLowerBound( b, -20 );
            tightener.getConstraintTightenings( tightenings );
            TS_ASSERT( tightenings.empty() );

            relu.notifyLowerBound( f, -20 * slope);
            tightener.getConstraintTightenings( tightenings );
            TS_ASSERT( tightenings.empty() );

            relu.notifyUpperBound( b, 20 );
            tightener.getConstraintTightenings( tightenings );
            TS_ASSERT( tightenings.empty() );

            relu.notifyUpperBound( f, 23 );
            tightener.getConstraintTightenings( tightenings );
            TS_ASSERT( tightenings.empty() );
        }

        {
            // Tighter lower bound for b that is negative
            LeakyReluConstraint relu = prepareLeakyRelu( b, f, &tightener );
            relu.notifyLowerBound( b, -8 );
            tightener.getConstraintTightenings( tightenings );
            TS_ASSERT( tightenings.exists( Tightening( f, -8 * slope, Tightening::LB ) ) );
        }

        {
            // Tighter upper bound for b/f that is positive
            LeakyReluConstraint relu = prepareLeakyRelu( b, f, &tightener );
            relu.notifyUpperBound( b, 13 );
            tightener.getConstraintTightenings( tightenings );
            TS_ASSERT( tightenings.exists( Tightening( f, 13, Tightening::UB ) ) );

            relu.notifyUpperBound( f, 12 );
            tightener.getConstraintTightenings( tightenings );
            TS_ASSERT( tightenings.exists( Tightening( b, 12, Tightening::UB ) ) );
        }

        {
            // Tighter upper bound 0 for f
            LeakyReluConstraint relu = prepareLeakyRelu( b, f, &tightener );
            relu.notifyUpperBound( f, 0 );
            tightener.getConstraintTightenings( tightenings );

            TS_ASSERT( tightenings.exists( Tightening( b, 0, Tightening::UB ) ) );
        }

        {
            // Tighter negative upper bound for b
            LeakyReluConstraint relu = prepareLeakyRelu( b, f, &tightener );
            relu.notifyUpperBound( b, -1 );
            tightener.getConstraintTightenings( tightenings );

            TS_ASSERT( tightenings.exists( Tightening( f, -1 * slope, Tightening::UB ) ) );
        }

        {
            // Tighter negative upper bound for f
            LeakyReluConstraint relu = prepareLeakyRelu( b, f, &tightener );
            relu.notifyUpperBound( f, slope * -1 );
            tightener.getConstraintTightenings( tightenings );

            TS_ASSERT( tightenings.exists( Tightening( b, -1, Tightening::UB ) ) );
        }

    }

    void test_polarity()
    {
        unsigned b = 1;
        unsigned f = 4;

        PiecewiseLinearCaseSplit activePhase;
        activePhase.storeBoundTightening( Tightening( b, 0.0, Tightening::LB ) );
        activePhase.storeBoundTightening( Tightening( f, 0.0, Tightening::LB ) );
        Equation activeEquation( Equation::EQ );
        activeEquation.addAddend( 1, b );
        activeEquation.addAddend( -1, f );
        activeEquation.setScalar( 0 );
        activePhase.addEquation( activeEquation );

        PiecewiseLinearCaseSplit inactivePhase;
        inactivePhase.storeBoundTightening( Tightening( b, 0.0, Tightening::UB ) );
        inactivePhase.storeBoundTightening( Tightening( f, 0.0, Tightening::UB ) );
        Equation inactiveEquation( Equation::EQ );
        inactiveEquation.addAddend( GlobalConfiguration::LEAKY_RELU_SLOPE, b );
        inactiveEquation.addAddend( -1, f );
        inactiveEquation.setScalar( 0 );
        inactivePhase.addEquation( inactiveEquation );

        // b in [1, 2], polarity should be 1, and direction should be RELU_PHASE_ACTIVE
        {
            LeakyReluConstraint relu( b, f );
            relu.notifyLowerBound( b, 1 );
            relu.notifyUpperBound( b, 2 );
            TS_ASSERT( relu.computePolarity() == 1 );

            relu.updateDirection();
            TS_ASSERT( relu.getDirection() == RELU_PHASE_ACTIVE );
        }
        // b in [-2, 0], polarity should be -1, and direction should be RELU_PHASE_INACTIVE
        {
            LeakyReluConstraint relu( b, f );
            relu.notifyLowerBound( b, -2 );
            relu.notifyUpperBound( b, 0 );
            TS_ASSERT( relu.computePolarity() == -1 );

            relu.updateDirection();
            TS_ASSERT( relu.getDirection() == RELU_PHASE_INACTIVE );
        }
        // b in [-2, 2], polarity should be 0, the direction should be RELU_PHASE_INACTIVE,
        // the inactive case should be the first element of the returned list by
        // the getCaseSplits(), and getPossibleFix should return the inactive fix first
        {
            LeakyReluConstraint relu( b, f );
            relu.notifyLowerBound( b, -2 );
            relu.notifyUpperBound( b, 2 );
            TS_ASSERT( relu.computePolarity() == 0 );

            relu.updateDirection();
            TS_ASSERT( relu.getDirection() == RELU_PHASE_INACTIVE );

            auto splits = relu.getCaseSplits();
            auto it = splits.begin();
            TS_ASSERT( *it == inactivePhase );
        }
        // b in [-2, 3], polarity should be 0.2, the direction should be RELU_PHASE_ACTIVE,
        // the active case should be the first element of the returned list by
        // the getCaseSplits(), and getPossibleFix should return the active fix first
        {
            LeakyReluConstraint relu( b, f );
            relu.notifyLowerBound( b, -2 );
            relu.notifyUpperBound( b, 3 );
            TS_ASSERT( relu.computePolarity() == 0.2 );

            relu.updateDirection();
            TS_ASSERT( relu.getDirection() == RELU_PHASE_ACTIVE );

            auto splits = relu.getCaseSplits();
            auto it = splits.begin();
            TS_ASSERT( *it == activePhase );
        }
    }

    /*
      Test Case functionality of LeakyReluConstraint
      1. Check that all cases are returned by LeakyReluConstraint::getAllCases()
      2. Check that LeakyReluConstraint::getCaseSplit( case ) returns the correct case
     */
    void test_relu_get_cases()
    {
        unsigned b = 1;
        unsigned f = 4;

        LeakyReluConstraint relu( b, f );

        List<PhaseStatus> cases = relu.getAllCases();

        TS_ASSERT_EQUALS( cases.size(), 2u );
        TS_ASSERT_EQUALS( cases.front(), RELU_PHASE_INACTIVE );
        TS_ASSERT_EQUALS( cases.back(), RELU_PHASE_ACTIVE );

        List<PiecewiseLinearCaseSplit> splits = relu.getCaseSplits();
        TS_ASSERT_EQUALS( splits.size(), 2u );
        TS_ASSERT_EQUALS( splits.front(), relu.getCaseSplit( RELU_PHASE_INACTIVE ) ) ;
        TS_ASSERT_EQUALS( splits.back(), relu.getCaseSplit( RELU_PHASE_ACTIVE ) ) ;
    }

    /*
      Test context-dependent ReLU state behavior.
     */
    void test_relu_context_dependent_state()
    {
        Context context;
        unsigned b = 1;
        unsigned f = 4;

        TestLeakyReluConstraint relu( b, f );

        relu.initializeCDOs( &context );



        TS_ASSERT_EQUALS( relu.getPhaseStatus(), PHASE_NOT_FIXED );

        context.push();

        relu.notifyLowerBound( f, 1 );
        TS_ASSERT_EQUALS( relu.getPhaseStatus(), RELU_PHASE_ACTIVE );

        context.pop();
        TS_ASSERT_EQUALS( relu.getPhaseStatus(), PHASE_NOT_FIXED );

        context.push();
        relu.notifyUpperBound( b, -1 );
        TS_ASSERT_EQUALS( relu.getPhaseStatus(), RELU_PHASE_INACTIVE );

        context.pop();
        TS_ASSERT_EQUALS( relu.getPhaseStatus(), PHASE_NOT_FIXED );
    }

    /*
      Test correct initialization of context-dependent data structures.
     */
    void test_initialization_of_CDOs()
    {
        Context context;
        LeakyReluConstraint *relu1 = new LeakyReluConstraint( 4, 6 );

        TS_ASSERT_EQUALS(relu1->getContext(), static_cast<Context*>( static_cast<Context*>( nullptr ) ) );

        TS_ASSERT_EQUALS( relu1->getActiveStatusCDO(), static_cast<CDO<bool>*>( nullptr ) );
        TS_ASSERT_EQUALS( relu1->getPhaseStatusCDO(), static_cast<CDO<PhaseStatus>*>( nullptr ) );
        TS_ASSERT_EQUALS( relu1->getInfeasibleCasesCDList(), static_cast<CDList<PhaseStatus>*>( nullptr ) );
        TS_ASSERT_THROWS_NOTHING( relu1->initializeCDOs( &context ) );
        TS_ASSERT_EQUALS( relu1->getContext(), &context );
        TS_ASSERT_DIFFERS( relu1->getActiveStatusCDO(), static_cast<CDO<bool>*>( nullptr ) );
        TS_ASSERT_DIFFERS( relu1->getPhaseStatusCDO(), static_cast<CDO<PhaseStatus>*>( nullptr ) );
        TS_ASSERT_DIFFERS( relu1->getInfeasibleCasesCDList(), static_cast<CDList<PhaseStatus>*>( nullptr ) );

        bool active = false;
        TS_ASSERT_THROWS_NOTHING( active = relu1->isActive() );
        TS_ASSERT_EQUALS( active, true );

        bool phaseFixed = true;
        TS_ASSERT_THROWS_NOTHING( phaseFixed = relu1->phaseFixed() );
        TS_ASSERT_EQUALS( phaseFixed, PHASE_NOT_FIXED );
        TS_ASSERT_EQUALS( relu1->numFeasibleCases(), 2u );
        TS_ASSERT( relu1->isFeasible() );
        TS_ASSERT( !relu1->isImplication() );

        TS_ASSERT_THROWS_NOTHING( delete relu1 );
    }

    /*
      Test context-dependent storage of search state information via
      isFeasible/markInfeasible methods.
     */
    void test_lazy_backtracking_of_CDOs()
    {
        Context context;
        LeakyReluConstraint *relu1 = new LeakyReluConstraint( 4, 6 );
        TS_ASSERT_THROWS_NOTHING( relu1->initializeCDOs( &context ) );

        // L0 - Feasible, not an implication
        PhaseStatus phase1 = PHASE_NOT_FIXED;
        TS_ASSERT_THROWS_NOTHING( phase1 = relu1->nextFeasibleCase() );
        TS_ASSERT_EQUALS( phase1, relu1->nextFeasibleCase() );
        TS_ASSERT( relu1->isFeasible() );
        TS_ASSERT( !relu1->isImplication() );


        // L1 - Feasible, an implication, nextFeasibleCase returns a new case
        TS_ASSERT_THROWS_NOTHING( context.push() );
        TS_ASSERT_THROWS_NOTHING( relu1->markInfeasible( phase1 ) );
        TS_ASSERT( relu1->isFeasible() );
        TS_ASSERT( relu1->isImplication() );

        PhaseStatus phase2 = PHASE_NOT_FIXED;
        TS_ASSERT_THROWS_NOTHING( phase2 = relu1->nextFeasibleCase() );
        TS_ASSERT_EQUALS( phase2, relu1->nextFeasibleCase() );
        TS_ASSERT_DIFFERS( phase2, phase1 );

        // L2 - Infeasible, not an implication, nextCase returns CONSTRAINT_INFEASIBLE
        TS_ASSERT_THROWS_NOTHING( context.push() );
        TS_ASSERT_THROWS_NOTHING( relu1->markInfeasible( phase2 ) );
        TS_ASSERT( !relu1->isFeasible() );
        TS_ASSERT( !relu1->isImplication() );
        TS_ASSERT_EQUALS( relu1->nextFeasibleCase(), CONSTRAINT_INFEASIBLE );

        // L1 again - Feasible, an implication, nextFeasibleCase returns same values as phase2
        TS_ASSERT_THROWS_NOTHING( context.pop() );
        TS_ASSERT( relu1->isFeasible() );
        TS_ASSERT( relu1->isImplication() );
        TS_ASSERT_EQUALS( phase2, relu1->nextFeasibleCase() );

        // L0 again - Feasible, not an implication, nextFeasibleCase returns same value as phase1
        TS_ASSERT_THROWS_NOTHING( context.pop() );
        TS_ASSERT( relu1->isFeasible() );
        TS_ASSERT( !relu1->isImplication() );
        TS_ASSERT_EQUALS( phase1, relu1->nextFeasibleCase() );

        TS_ASSERT_THROWS_NOTHING( delete relu1 );
    }

};

//
// Local Variables:
// compile-command: "make -C ../../.. "
// tags-file-name: "../../../TAGS"
// c-basic-offset: 4
// End:
//

