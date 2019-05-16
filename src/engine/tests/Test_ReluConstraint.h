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

#include "InputQuery.h"
#include "MockErrno.h"
#include "MockTableau.h"
#include "PiecewiseLinearCaseSplit.h"
#include "ReluConstraint.h"
#include "ReluplexError.h"

#include <string.h>

class MockForReluConstraint
    : public MockErrno
{
public:
};

class ReluConstraintTestSuite : public CxxTest::TestSuite
{
public:
    MockForReluConstraint *mock;

    void setUp()
    {
        TS_ASSERT( mock = new MockForReluConstraint );
    }

    void tearDown()
    {
        TS_ASSERT_THROWS_NOTHING( delete mock );
    }

    void test_relu_constraint()
    {
        unsigned b = 1;
        unsigned f = 4;

        ReluConstraint relu( b, f );

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
                                 const ReluplexError &e,
                                 e.getCode(),
                                 ReluplexError::PARTICIPATING_VARIABLES_ABSENT );

        relu.notifyVariableValue( b, 1 );
        relu.notifyVariableValue( f, 1 );

        TS_ASSERT( relu.satisfied() );

        relu.notifyVariableValue( f, 2 );

        TS_ASSERT( !relu.satisfied() );

        relu.notifyVariableValue( b, 2 );

        TS_ASSERT( relu.satisfied() );

        relu.notifyVariableValue( b, -2 );

        TS_ASSERT( !relu.satisfied() );

        relu.notifyVariableValue( f, 0 );

        TS_ASSERT( relu.satisfied() );

        relu.notifyVariableValue( b, -3 );

        TS_ASSERT( relu.satisfied() );

        relu.notifyVariableValue( b, 0 );

        TS_ASSERT( relu.satisfied() );

        relu.notifyVariableValue( 4, -1 );

        // A relu cannot be satisfied if f is negative
        TS_ASSERT( !relu.satisfied() );

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

    void test_relu_fixes()
    {
        unsigned b = 1;
        unsigned f = 4;

        ReluConstraint relu( b, f );

        List<PiecewiseLinearConstraint::Fix> fixes;
        List<PiecewiseLinearConstraint::Fix>::iterator it;

        relu.notifyVariableValue( b, -1 );
        relu.notifyVariableValue( f, 1 );

        fixes = relu.getPossibleFixes();
        it = fixes.begin();
        TS_ASSERT_EQUALS( it->_variable, b );
        TS_ASSERT_EQUALS( it->_value, 1 );
        ++it;
        TS_ASSERT_EQUALS( it->_variable, f );
        TS_ASSERT_EQUALS( it->_value, 0 );

        relu.notifyVariableValue( b, 2 );
        relu.notifyVariableValue( f, 1 );

        fixes = relu.getPossibleFixes();
        it = fixes.begin();
        TS_ASSERT_EQUALS( it->_variable, b );
        TS_ASSERT_EQUALS( it->_value, 1 );
        ++it;
        TS_ASSERT_EQUALS( it->_variable, f );
        TS_ASSERT_EQUALS( it->_value, 2 );

        relu.notifyVariableValue( b, 11 );
        relu.notifyVariableValue( f, 0 );

        fixes = relu.getPossibleFixes();
        it = fixes.begin();
        TS_ASSERT_EQUALS( it->_variable, b );
        TS_ASSERT_EQUALS( it->_value, 0 );
        ++it;
        TS_ASSERT_EQUALS( it->_variable, f );
        TS_ASSERT_EQUALS( it->_value, 11 );
    }

    void test_relu_case_splits()
    {
        unsigned b = 1;
        unsigned f = 4;

        ReluConstraint relu( b, f );

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

        TS_ASSERT_EQUALS( bounds.size(), 1U );

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

        auto equations = split->getEquations();
        TS_ASSERT( equations.empty() );

        return true;
    }

    void test_register_as_watcher()
    {
        unsigned b = 1;
        unsigned f = 4;

        MockTableau tableau;

        ReluConstraint relu( b, f );

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

    void test_fix_active()
    {
        unsigned b = 1;
        unsigned f = 4;

        MockTableau tableau;

        ReluConstraint relu( b, f );

        relu.registerAsWatcher( &tableau );

        List<PiecewiseLinearCaseSplit> splits = relu.getCaseSplits();
        TS_ASSERT_EQUALS( splits.size(), 2U );

        relu.notifyLowerBound( 1, 1.0 );
        TS_ASSERT_THROWS_EQUALS( splits = relu.getCaseSplits(),
                                 const ReluplexError &e,
                                 e.getCode(),
                                 ReluplexError::REQUESTED_CASE_SPLITS_FROM_FIXED_CONSTRAINT );

        relu.unregisterAsWatcher( &tableau );

        relu = ReluConstraint( b, f );

        relu.registerAsWatcher( &tableau );

        splits = relu.getCaseSplits();
        TS_ASSERT_EQUALS( splits.size(), 2U );

        relu.notifyLowerBound( 4, 1.0 );
        TS_ASSERT_THROWS_EQUALS( splits = relu.getCaseSplits(),
                                 const ReluplexError &e,
                                 e.getCode(),
                                 ReluplexError::REQUESTED_CASE_SPLITS_FROM_FIXED_CONSTRAINT );

        relu.unregisterAsWatcher( &tableau );
    }

    void test_fix_inactive()
    {
        unsigned b = 1;
        unsigned f = 4;

        MockTableau tableau;

        ReluConstraint relu( b, f );

        relu.registerAsWatcher( &tableau );

        List<PiecewiseLinearCaseSplit> splits = relu.getCaseSplits();
        TS_ASSERT_EQUALS( splits.size(), 2U );

        relu.notifyUpperBound( 4, -1.0 );
        TS_ASSERT_THROWS_EQUALS( splits = relu.getCaseSplits(),
                                  const ReluplexError &e,
                                  e.getCode(),
                                  ReluplexError::REQUESTED_CASE_SPLITS_FROM_FIXED_CONSTRAINT );

        relu.unregisterAsWatcher( &tableau );
    }

    void test_constraint_phase_gets_fixed()
    {
        unsigned b = 1;
        unsigned f = 4;

        MockTableau tableau;

        // Upper bounds
        {
            ReluConstraint relu( b, f );
            TS_ASSERT( !relu.phaseFixed() );
            relu.notifyUpperBound( b, -1.0 );
            TS_ASSERT( relu.phaseFixed() );
        }

        {
            ReluConstraint relu( b, f );
            TS_ASSERT( !relu.phaseFixed() );
            relu.notifyUpperBound( b, 0.0 );
            TS_ASSERT( relu.phaseFixed() );
        }

        {
            ReluConstraint relu( b, f );
            TS_ASSERT( !relu.phaseFixed() );
            relu.notifyUpperBound( f, 0.0 );
            TS_ASSERT( relu.phaseFixed() );
        }

        {
            ReluConstraint relu( b, f );
            TS_ASSERT( !relu.phaseFixed() );
            relu.notifyUpperBound( b, 3.0 );
            TS_ASSERT( !relu.phaseFixed() );
        }

        {
            ReluConstraint relu( b, f );
            TS_ASSERT( !relu.phaseFixed() );
            relu.notifyUpperBound( b, 5.0 );
            TS_ASSERT( !relu.phaseFixed() );
        }

        // Lower bounds
        {
            ReluConstraint relu( b, f );
            TS_ASSERT( !relu.phaseFixed() );
            relu.notifyLowerBound( b, 3.0 );
            TS_ASSERT( relu.phaseFixed() );
        }

        {
            ReluConstraint relu( b, f );
            TS_ASSERT( !relu.phaseFixed() );
            relu.notifyLowerBound( b, 0.0 );
            TS_ASSERT( relu.phaseFixed() );
        }

        {
            ReluConstraint relu( b, f );
            TS_ASSERT( !relu.phaseFixed() );
            relu.notifyLowerBound( f, 6.0 );
            TS_ASSERT( relu.phaseFixed() );
        }

        {
            ReluConstraint relu( b, f );
            TS_ASSERT( !relu.phaseFixed() );
            relu.notifyLowerBound( f, 0.0 );
            TS_ASSERT( !relu.phaseFixed() );
        }

        {
            ReluConstraint relu( b, f );
            TS_ASSERT( !relu.phaseFixed() );
            relu.notifyLowerBound( b, -2.0 );
            TS_ASSERT( !relu.phaseFixed() );
        }
    }

    void test_valid_split_relu_phase_fixed_to_active()
    {
        unsigned b = 1;
        unsigned f = 4;

        ReluConstraint relu( b, f );

        List<PiecewiseLinearConstraint::Fix> fixes;
        List<PiecewiseLinearConstraint::Fix>::iterator it;

        TS_ASSERT( !relu.phaseFixed() );
        TS_ASSERT_THROWS_NOTHING( relu.notifyLowerBound( b, 5 ) );
        TS_ASSERT( relu.phaseFixed() );

        PiecewiseLinearCaseSplit split;
        TS_ASSERT_THROWS_NOTHING( split = relu.getValidCaseSplit() );

        Equation activeEquation;

        List<Tightening> bounds = split.getBoundTightenings();

        TS_ASSERT_EQUALS( bounds.size(), 1U );
        auto bound = bounds.begin();
        Tightening bound1 = *bound;

        TS_ASSERT_EQUALS( bound1._variable, b );
        TS_ASSERT_EQUALS( bound1._type, Tightening::LB );
        TS_ASSERT_EQUALS( bound1._value, 0.0 );

        auto equations = split.getEquations();
        TS_ASSERT_EQUALS( equations.size(), 1U );
        activeEquation = split.getEquations().front();
        TS_ASSERT_EQUALS( activeEquation._addends.size(), 2U );
        TS_ASSERT_EQUALS( activeEquation._scalar, 0.0 );

        auto addend = activeEquation._addends.begin();
        TS_ASSERT_EQUALS( addend->_coefficient, 1.0 );
        TS_ASSERT_EQUALS( addend->_variable, b );

        ++addend;
        TS_ASSERT_EQUALS( addend->_coefficient, -1.0 );
        TS_ASSERT_EQUALS( addend->_variable, f );

        TS_ASSERT_EQUALS( activeEquation._type, Equation::EQ );
    }

    void test_valid_split_relu_phase_fixed_to_inactive()
    {
        unsigned b = 1;
        unsigned f = 4;

        ReluConstraint relu( b, f );

        List<PiecewiseLinearConstraint::Fix> fixes;
        List<PiecewiseLinearConstraint::Fix>::iterator it;

        TS_ASSERT( !relu.phaseFixed() );
        TS_ASSERT_THROWS_NOTHING( relu.notifyUpperBound( b, -2 ) );
        TS_ASSERT( relu.phaseFixed() );

        PiecewiseLinearCaseSplit split;
        TS_ASSERT_THROWS_NOTHING( split = relu.getValidCaseSplit() );

        Equation activeEquation;

        List<Tightening> bounds = split.getBoundTightenings();

        TS_ASSERT_EQUALS( bounds.size(), 2U );
        auto bound = bounds.begin();
        Tightening bound1 = *bound;

        TS_ASSERT_EQUALS( bound1._variable, b );
        TS_ASSERT_EQUALS( bound1._type, Tightening::UB );
        TS_ASSERT_EQUALS( bound1._value, 0.0 );

        ++bound;
        Tightening bound2 = *bound;

        TS_ASSERT_EQUALS( bound2._variable, f );
        TS_ASSERT_EQUALS( bound2._value, 0.0 );
        TS_ASSERT_EQUALS( bound2._type, Tightening::UB );

        auto equations = split.getEquations();
        TS_ASSERT( equations.empty() );
    }

    void xtest_relu_entailed_tightenings()
    {
        unsigned b = 1;
        unsigned f = 4;

        ReluConstraint relu( b, f );

        List<Tightening> entailedTightenings;

        relu.notifyUpperBound( b, 7 );
        relu.notifyUpperBound( f, 7 );

        // Negative b lower bound is not propagated
        relu.notifyLowerBound( b, -1 );
        relu.notifyLowerBound( f, 0 );

        relu.getEntailedTightenings( entailedTightenings );
        TS_ASSERT( entailedTightenings.empty() );

        // Positive lower bounds are propagated
        relu.notifyLowerBound( b, 1 );
        relu.notifyLowerBound( f, 2 );

        entailedTightenings.clear();
        relu.getEntailedTightenings( entailedTightenings );
        TS_ASSERT_EQUALS( entailedTightenings.size(), 1U );
        Tightening tightening = *entailedTightenings.begin();

        TS_ASSERT_EQUALS( tightening._variable, b );
        TS_ASSERT_EQUALS( tightening._value, 2 );
        TS_ASSERT_EQUALS( tightening._type, Tightening::LB );

        relu.notifyLowerBound( b, 2 );
        entailedTightenings.clear();
        relu.getEntailedTightenings( entailedTightenings );
        TS_ASSERT( entailedTightenings.empty() );

        relu.notifyLowerBound( b, 3 );

        entailedTightenings.clear();
        relu.getEntailedTightenings( entailedTightenings );
        TS_ASSERT_EQUALS( entailedTightenings.size(), 1U );
        tightening = *entailedTightenings.begin();

        TS_ASSERT_EQUALS( tightening._variable, f );
        TS_ASSERT_EQUALS( tightening._value, 3 );
        TS_ASSERT_EQUALS( tightening._type, Tightening::LB );

        relu.notifyLowerBound( f, 3 );

        // Positive upper bounds are propagated
        relu.notifyUpperBound( b, 5 );
        relu.notifyUpperBound( f, 6 );

        entailedTightenings.clear();
        relu.getEntailedTightenings( entailedTightenings );
        TS_ASSERT_EQUALS( entailedTightenings.size(), 1U );
        tightening = *entailedTightenings.begin();

        TS_ASSERT_EQUALS( tightening._variable, f );
        TS_ASSERT_EQUALS( tightening._value, 5 );
        TS_ASSERT_EQUALS( tightening._type, Tightening::UB );

        relu.notifyUpperBound( f, 4 );

        entailedTightenings.clear();
        relu.getEntailedTightenings( entailedTightenings );
        TS_ASSERT_EQUALS( entailedTightenings.size(), 1U );
        tightening = *entailedTightenings.begin();

        TS_ASSERT_EQUALS( tightening._variable, b );
        TS_ASSERT_EQUALS( tightening._value, 4 );
        TS_ASSERT_EQUALS( tightening._type, Tightening::UB );
    }

    void test_relu_duplicate_and_restore()
    {
        ReluConstraint *relu1 = new ReluConstraint( 4, 6 );
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

        TS_ASSERT_THROWS_NOTHING( delete relu2 );
    }

    void test_eliminate_variable_active()
    {
        unsigned b = 1;
        unsigned f = 4;

        MockTableau tableau;

        ReluConstraint relu( b, f );

        relu.registerAsWatcher( &tableau );

        TS_ASSERT( !relu.constraintObsolete() );
        TS_ASSERT_THROWS_NOTHING( relu.eliminateVariable( b, 5 ) );
        TS_ASSERT( relu.constraintObsolete() );
    }

    void test_serialize_and_unserialize()
    {
        unsigned b = 42;
        unsigned f = 7;

        ReluConstraint originalRelu( b, f );
        String originalSerialized = originalRelu.serializeToString();
        ReluConstraint recoveredRelu( originalSerialized );

        TS_ASSERT_EQUALS( originalRelu.serializeToString(),
                          recoveredRelu.serializeToString() );
    }

    bool haveFix( List<PiecewiseLinearConstraint::Fix> &fixes, unsigned var, double value )
    {
        PiecewiseLinearConstraint::Fix targetFix( var, value );
        for ( const auto &fix : fixes )
        {
            if ( fix == targetFix )
                return true;
        }

        return false;
    }

    void test_relu_smart_fixes()
    {
        unsigned b = 1;
        unsigned f = 4;

        ReluConstraint relu( b, f );

        MockTableau tableau;

        List<PiecewiseLinearConstraint::Fix> fixes;
        List<PiecewiseLinearConstraint::Fix>::iterator it;

        // b and f are not linearly dependent. Return non-smart fixes

        tableau.nextLinearlyDependentResult = false;

        relu.notifyVariableValue( b, -1 );
        relu.notifyVariableValue( f, 1 );

        TS_ASSERT_THROWS_NOTHING( fixes = relu.getSmartFixes( &tableau ) );

        TS_ASSERT_EQUALS( fixes.size(), 2U );
        TS_ASSERT( haveFix( fixes, b, 1 ) );
        TS_ASSERT( haveFix( fixes, f, 0 ) );

        TS_ASSERT_EQUALS( tableau.lastLinearlyDependentX1, b );
        TS_ASSERT_EQUALS( tableau.lastLinearlyDependentX2, f );

        // From now on, assume b and f are linearly dependent
        tableau.nextLinearlyDependentResult = true;

        // First, assume f is basic, b is non basic
        tableau.nextIsBasic.insert( f );

        double bDeltaToFDelta = -2;
        tableau.nextLinearlyDependentCoefficient = bDeltaToFDelta;

        relu.notifyVariableValue( b, 5 );
        relu.notifyVariableValue( f, 2 );

        // We expect b to decrease by 1, which will cause f to increase by 2
        fixes.clear();
        TS_ASSERT_THROWS_NOTHING( fixes = relu.getSmartFixes( &tableau ) );

        TS_ASSERT_EQUALS( fixes.size(), 1U );
        TS_ASSERT( haveFix( fixes, b, 4 ) );

        relu.notifyVariableValue( b, 5 );
        relu.notifyVariableValue( f, 1 );

        bDeltaToFDelta = 3;
        tableau.nextLinearlyDependentCoefficient = bDeltaToFDelta;

        // We expect b to increase to 7, so that f will catch up
        fixes.clear();
        TS_ASSERT_THROWS_NOTHING( fixes = relu.getSmartFixes( &tableau ) );

        TS_ASSERT_EQUALS( fixes.size(), 1U );
        TS_ASSERT( haveFix( fixes, b, 7 ) );

        // If the coefficient is 1, no fix is possible
        bDeltaToFDelta = 1;
        tableau.nextLinearlyDependentCoefficient = bDeltaToFDelta;

        fixes.clear();
        TS_ASSERT_THROWS_NOTHING( fixes = relu.getSmartFixes( &tableau ) );

        TS_ASSERT( fixes.empty() );

        // Now a case where both fixes are possible
        relu.notifyVariableValue( b, -1 );
        relu.notifyVariableValue( f, 1 );

        bDeltaToFDelta = 1.0 / 3;
        tableau.nextLinearlyDependentCoefficient = bDeltaToFDelta;

        // Option 1: decrease b by 3, so that f decreases by 1
        // Option 2: increase b by 3, so that f increases by 1
        fixes.clear();
        TS_ASSERT_THROWS_NOTHING( fixes = relu.getSmartFixes( &tableau ) );

        TS_ASSERT_EQUALS( fixes.size(), 2U );
        TS_ASSERT( haveFix( fixes, b, -4 ) );
        TS_ASSERT( haveFix( fixes, b, 2 ) );

        // Now, assume b is basic, f is non basic
        tableau.nextIsBasic.clear();
        tableau.nextIsBasic.insert( b );

        double fDeltaToBDelta = -1.0 / 2;
        tableau.nextLinearlyDependentCoefficient = 1.0 / fDeltaToBDelta;

        relu.notifyVariableValue( b, 5 );
        relu.notifyVariableValue( f, 2 );

        // We expect f to increase by 2, which will cause b to decrease by 1
        fixes.clear();
        TS_ASSERT_THROWS_NOTHING( fixes = relu.getSmartFixes( &tableau ) );

        TS_ASSERT_EQUALS( fixes.size(), 1U );
        TS_ASSERT( haveFix( fixes, f, 4 ) );

        relu.notifyVariableValue( b, 5 );
        relu.notifyVariableValue( f, 1 );

        fDeltaToBDelta = 1.0 / 3;
        tableau.nextLinearlyDependentCoefficient = 1.0 / fDeltaToBDelta;

        // We expect f to increase to 7, so that f will catch up
        fixes.clear();
        TS_ASSERT_THROWS_NOTHING( fixes = relu.getSmartFixes( &tableau ) );

        TS_ASSERT_EQUALS( fixes.size(), 1U );
        TS_ASSERT( haveFix( fixes, f, 7 ) );

        // If the coefficient is 1, no fix is possible
        fDeltaToBDelta = 1;
        tableau.nextLinearlyDependentCoefficient = 1.0 / fDeltaToBDelta;

        fixes.clear();
        TS_ASSERT_THROWS_NOTHING( fixes = relu.getSmartFixes( &tableau ) );

        TS_ASSERT( fixes.empty() );

        // Now a case where both fixes are possible
        relu.notifyVariableValue( b, -1 );
        relu.notifyVariableValue( f, 1 );

        fDeltaToBDelta = 3;
        tableau.nextLinearlyDependentCoefficient = 1 / fDeltaToBDelta;

        // Option 1: f decreases by 1, so that b decreases by 3
        // Option 2: f increases by 1, so that b increases by 3
        fixes.clear();
        TS_ASSERT_THROWS_NOTHING( fixes = relu.getSmartFixes( &tableau ) );

        TS_ASSERT_EQUALS( fixes.size(), 2U );
        TS_ASSERT( haveFix( fixes, f, 0 ) );
        TS_ASSERT( haveFix( fixes, f, 2 ) );
    }

    void test_add_auxiliary_equations()
    {
        ReluConstraint relu( 4, 6 );
        InputQuery query;

        query.setNumberOfVariables( 9 );

        relu.notifyLowerBound( 4, -10 );
        relu.notifyLowerBound( 6, 0 );

        relu.notifyUpperBound( 4, 15 );
        relu.notifyUpperBound( 6, 15 );

        TS_ASSERT_THROWS_NOTHING( relu.addAuxiliaryEquations( query ) );

        const List<Equation> &equations( query.getEquations() );

        TS_ASSERT_EQUALS( equations.size(), 1U );
        TS_ASSERT_EQUALS( query.getNumberOfVariables(), 10U );

        unsigned aux = 9;
        TS_ASSERT_EQUALS( query.getLowerBound( aux ), 0 );
        // TS_ASSERT_EQUALS( query.getUpperBound( aux ), 10 );

        Equation eq = *equations.begin();

        TS_ASSERT_EQUALS( eq._addends.size(), 3U );

        auto it = eq._addends.begin();
        TS_ASSERT_EQUALS( *it, Equation::Addend( 1, 6 ) );
        ++it;
        TS_ASSERT_EQUALS( *it, Equation::Addend( -1, 4 ) );
        ++it;
        TS_ASSERT_EQUALS( *it, Equation::Addend( -1, aux ) );

        TS_ASSERT_EQUALS( eq._scalar, 0 );
    }
};

// serialize and unserialize, duplicate. Register as watcher for aux?

//
// Local Variables:
// compile-command: "make -C ../../.. "
// tags-file-name: "../../../TAGS"
// c-basic-offset: 4
// End:
//
