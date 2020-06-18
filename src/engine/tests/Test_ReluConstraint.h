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
#include "MockConstraintBoundTightener.h"
#include "MockErrno.h"
#include "MockTableau.h"
#include "PiecewiseLinearCaseSplit.h"
#include "ReluConstraint.h"
#include "MarabouError.h"

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

    void test_relu_case_splits_with_aux_var()
    {
        unsigned b = 1;
        unsigned f = 4;

        ReluConstraint relu( b, f );

        relu.notifyLowerBound( b, -10 );
        relu.notifyUpperBound( b, 5 );
        relu.notifyUpperBound( f, 5 );

        unsigned auxVar = 10;
        InputQuery inputQuery;
        inputQuery.setNumberOfVariables( auxVar );

        relu.addAuxiliaryEquations( inputQuery );

        TS_ASSERT( relu.auxVariableInUse() );
        TS_ASSERT_EQUALS( relu.getAux(), auxVar );

        List<PiecewiseLinearConstraint::Fix> fixes;
        List<PiecewiseLinearConstraint::Fix>::iterator it;

        List<PiecewiseLinearCaseSplit> splits = relu.getCaseSplits();

        Equation activeEquation, inactiveEquation;

        TS_ASSERT_EQUALS( splits.size(), 2U );

        List<PiecewiseLinearCaseSplit>::iterator split1 = splits.begin();
        List<PiecewiseLinearCaseSplit>::iterator split2 = split1;
        ++split2;

        TS_ASSERT( isActiveSplitWithAux( b, auxVar, split1 ) || isActiveSplitWithAux( b, auxVar, split2 ) );
        TS_ASSERT( isInactiveSplit( b, f, split1 ) || isInactiveSplit( b, f, split2 ) );
    }

    bool isActiveSplitWithAux( unsigned b, unsigned aux, List<PiecewiseLinearCaseSplit>::iterator &split )
    {
        List<Tightening> bounds = split->getBoundTightenings();

        TS_ASSERT_EQUALS( bounds.size(), 2U );

        auto bound = bounds.begin();
        Tightening bound1 = *bound;

        TS_ASSERT_EQUALS( bound1._variable, b );
        TS_ASSERT_EQUALS( bound1._value, 0.0 );

        if ( bound1._type != Tightening::LB )
            return false;

        ++bound;

        Tightening bound2 = *bound;

        TS_ASSERT_EQUALS( bound2._variable, aux );
        TS_ASSERT_EQUALS( bound2._value, 0.0 );

        if ( bound2._type != Tightening::UB )
            return false;

        TS_ASSERT( split->getEquations().empty() );

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
                                 const MarabouError &e,
                                 e.getCode(),
                                 MarabouError::REQUESTED_CASE_SPLITS_FROM_FIXED_CONSTRAINT );

        relu.unregisterAsWatcher( &tableau );

        relu = ReluConstraint( b, f );

        relu.registerAsWatcher( &tableau );

        splits = relu.getCaseSplits();
        TS_ASSERT_EQUALS( splits.size(), 2U );

        relu.notifyLowerBound( 4, 1.0 );
        TS_ASSERT_THROWS_EQUALS( splits = relu.getCaseSplits(),
                                 const MarabouError &e,
                                 e.getCode(),
                                 MarabouError::REQUESTED_CASE_SPLITS_FROM_FIXED_CONSTRAINT );

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
                                  const MarabouError &e,
                                  e.getCode(),
                                  MarabouError::REQUESTED_CASE_SPLITS_FROM_FIXED_CONSTRAINT );

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

        // Aux variables: upper bound
        {
            ReluConstraint relu( b, f );

            relu.notifyLowerBound( b, -5 );
            InputQuery dontCare;
            unsigned aux = 300;
            dontCare.setNumberOfVariables( aux );
            relu.addAuxiliaryEquations( dontCare );

            TS_ASSERT( !relu.phaseFixed() );
            relu.notifyUpperBound( aux, 3.0 );
            TS_ASSERT( !relu.phaseFixed() );
            relu.notifyUpperBound( aux, 0.0 );
            TS_ASSERT( relu.phaseFixed() );
        }

        // Aux variables: lower bound
        {
            ReluConstraint relu( b, f );

            relu.notifyLowerBound( b, -5 );
            InputQuery dontCare;
            unsigned aux = 300;
            dontCare.setNumberOfVariables( aux );
            relu.addAuxiliaryEquations( dontCare );

            TS_ASSERT( !relu.phaseFixed() );
            relu.notifyLowerBound( aux, 0.0 );
            TS_ASSERT( !relu.phaseFixed() );
            relu.notifyLowerBound( aux, 1.0 );
            TS_ASSERT( relu.phaseFixed() );
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

    void test_relu_entailed_tightenings() // TODO FINISH
    {
        unsigned b = 1;
        unsigned f = 4;

        InputQuery dontCare;
        dontCare.setNumberOfVariables( 500 );
        unsigned aux = 500;

        ReluConstraint relu( b, f );

        relu.notifyUpperBound( b, 7 );
        relu.notifyUpperBound( f, 7 );

        relu.notifyLowerBound( b, -1 );
        relu.notifyLowerBound( f, 0 );

        relu.addAuxiliaryEquations( dontCare );

        relu.notifyLowerBound( aux, 0 );
        relu.notifyUpperBound( aux, 1 );

        List<Tightening> entailedTightenings;
        relu.getEntailedTightenings( entailedTightenings );

        // The phase is not fixed: upper bounds propagated between b
        // and f, b lower bound propagated to aux and vice versa, f
        // and aux both non-negative
        TS_ASSERT_EQUALS( entailedTightenings.size(), 6U );
        TS_ASSERT( entailedTightenings.exists( Tightening( f, 7, Tightening::UB ) ) );
        TS_ASSERT( entailedTightenings.exists( Tightening( b, 7, Tightening::UB ) ) );
        TS_ASSERT( entailedTightenings.exists( Tightening( f, 0, Tightening::LB ) ) );
        TS_ASSERT( entailedTightenings.exists( Tightening( aux, 0, Tightening::LB ) ) );
        TS_ASSERT( entailedTightenings.exists( Tightening( b, -1, Tightening::LB ) ) );
        TS_ASSERT( entailedTightenings.exists( Tightening( aux, 1, Tightening::UB ) ) );

        entailedTightenings.clear();

        // Positive lower bounds for b and f: active case. All bounds
        // propagated between f and b, and aux is set to 0. F and b are
        // non-negative.
        relu.notifyLowerBound( b, 1 );
        relu.notifyLowerBound( f, 2 );

        relu.getEntailedTightenings( entailedTightenings );

        TS_ASSERT_EQUALS( entailedTightenings.size(), 8U );
        TS_ASSERT( entailedTightenings.exists( Tightening( f, 1, Tightening::LB ) ) );
        TS_ASSERT( entailedTightenings.exists( Tightening( b, 2, Tightening::LB ) ) );
        TS_ASSERT( entailedTightenings.exists( Tightening( f, 7, Tightening::UB ) ) );
        TS_ASSERT( entailedTightenings.exists( Tightening( b, 7, Tightening::UB ) ) );

        TS_ASSERT( entailedTightenings.exists( Tightening( aux, 0, Tightening::LB ) ) );
        TS_ASSERT( entailedTightenings.exists( Tightening( aux, 0, Tightening::UB ) ) );

        TS_ASSERT( entailedTightenings.exists( Tightening( b, 0, Tightening::LB ) ) );
        TS_ASSERT( entailedTightenings.exists( Tightening( f, 0, Tightening::LB ) ) );

        entailedTightenings.clear();

        // Negative upper bound for b: inactive case. F is 0, b =
        // -aux. B is non-positive, aux is non-negative.

        dontCare.setNumberOfVariables( 500 );
        ReluConstraint relu2( b, f );

        relu2.notifyUpperBound( b, -1 );
        relu2.notifyUpperBound( f, 7 );

        relu2.notifyLowerBound( b, -2 );
        relu2.notifyLowerBound( f, 0 );

        relu2.addAuxiliaryEquations( dontCare );

        relu2.notifyLowerBound( aux, 0 );
        relu2.notifyUpperBound( aux, 2 );

        relu2.getEntailedTightenings( entailedTightenings );

        TS_ASSERT_EQUALS( entailedTightenings.size(), 8U );

        TS_ASSERT( entailedTightenings.exists( Tightening( b, 0, Tightening::UB ) ) );
        TS_ASSERT( entailedTightenings.exists( Tightening( aux, 0, Tightening::LB ) ) );

        TS_ASSERT( entailedTightenings.exists( Tightening( b, -2, Tightening::LB ) ) );
        TS_ASSERT( entailedTightenings.exists( Tightening( b, 0, Tightening::UB ) ) );
        TS_ASSERT( entailedTightenings.exists( Tightening( aux, 1, Tightening::LB ) ) );
        TS_ASSERT( entailedTightenings.exists( Tightening( aux, 2, Tightening::UB ) ) );

        TS_ASSERT( entailedTightenings.exists( Tightening( f, 0, Tightening::LB ) ) );
        TS_ASSERT( entailedTightenings.exists( Tightening( f, 0, Tightening::UB ) ) );
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

        TS_ASSERT_THROWS_NOTHING( delete relu1 );
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
        originalRelu.notifyLowerBound( b, -10 );
        originalRelu.notifyUpperBound( f, 5 );
        originalRelu.notifyUpperBound( f, 5 );

        String originalSerialized = originalRelu.serializeToString();
        ReluConstraint recoveredRelu( originalSerialized );

        TS_ASSERT_EQUALS( originalRelu.serializeToString(),
                          recoveredRelu.serializeToString() );

        TS_ASSERT( !originalRelu.auxVariableInUse() );
        TS_ASSERT( !recoveredRelu.auxVariableInUse() );

        InputQuery dontCare;
        dontCare.setNumberOfVariables( 500 );

        originalRelu.addAuxiliaryEquations( dontCare );

        TS_ASSERT( originalRelu.auxVariableInUse() );

        originalSerialized = originalRelu.serializeToString();
        ReluConstraint recoveredRelu2( originalSerialized );

        TS_ASSERT_EQUALS( originalRelu.serializeToString(),
                          recoveredRelu2.serializeToString() );

        TS_ASSERT( recoveredRelu2.auxVariableInUse() );
        TS_ASSERT_EQUALS( originalRelu.getAux(), recoveredRelu2.getAux() );
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
        TS_ASSERT_EQUALS( query.getUpperBound( aux ), 10 );

        Equation eq = *equations.begin();

        TS_ASSERT_EQUALS( eq._addends.size(), 3U );

        auto it = eq._addends.begin();
        TS_ASSERT_EQUALS( *it, Equation::Addend( 1, 6 ) );
        ++it;
        TS_ASSERT_EQUALS( *it, Equation::Addend( -1, 4 ) );
        ++it;
        TS_ASSERT_EQUALS( *it, Equation::Addend( -1, aux ) );

        TS_ASSERT_EQUALS( eq._scalar, 0 );

        // Special case: add aux equations in active phase
        ReluConstraint relu2( 4, 6 );
        InputQuery query2;

        query2.setNumberOfVariables( 9 );

        relu2.notifyLowerBound( 4, 3 );
        relu2.notifyLowerBound( 6, 0 );

        relu2.notifyUpperBound( 4, 15 );
        relu2.notifyUpperBound( 6, 15 );

        TS_ASSERT_THROWS_NOTHING( relu2.addAuxiliaryEquations( query2 ) );

        TS_ASSERT_EQUALS( query2.getLowerBound( aux ), 0 );
        TS_ASSERT_EQUALS( query2.getUpperBound( aux ), 0 );
    }

    ReluConstraint prepareRelu( unsigned b, unsigned f, unsigned aux, IConstraintBoundTightener *tightener )
    {
        ReluConstraint relu( b, f );

        InputQuery dontCare;
        dontCare.setNumberOfVariables( aux );

        relu.notifyLowerBound( b, -10 );
        relu.notifyUpperBound( b, 15 );

        relu.notifyLowerBound( f, 0 );
        relu.notifyUpperBound( f, 15 );

        TS_ASSERT_THROWS_NOTHING( relu.addAuxiliaryEquations( dontCare ) );

        relu.notifyLowerBound( aux, 0 );
        relu.notifyUpperBound( aux, 10 );

        relu.registerConstraintBoundTightener( tightener ); // here

        return relu;
    }

    void test_notify_bounds() // TODO FINISH
    {
        unsigned b = 1;
        unsigned f = 4;
        unsigned aux = 10;
        MockConstraintBoundTightener tightener;
        List<Tightening> tightenings;

        tightener.getConstraintTightenings( tightenings );

        // Initial state: b in [-10, 15], f in [0, 15], aux in [0, 10]

        {
            ReluConstraint relu = prepareRelu( b, f, aux, &tightener );

            relu.notifyLowerBound( b, -20 );
            tightener.getConstraintTightenings( tightenings );
            TS_ASSERT( tightenings.empty() );

            relu.notifyLowerBound( f, -3 );
            tightener.getConstraintTightenings( tightenings );
            TS_ASSERT( tightenings.empty() );

            relu.notifyLowerBound( aux, -5 );
            tightener.getConstraintTightenings( tightenings );
            TS_ASSERT( tightenings.empty() );

            relu.notifyUpperBound( b, 20 );
            tightener.getConstraintTightenings( tightenings );
            TS_ASSERT( tightenings.empty() );

            relu.notifyUpperBound( f, 23 );
            tightener.getConstraintTightenings( tightenings );
            TS_ASSERT( tightenings.empty() );

            relu.notifyUpperBound( aux, 35 );
            tightener.getConstraintTightenings( tightenings );
            TS_ASSERT( tightenings.empty() );
        }

        {
            // Tighter lower bound for b that is negative
            ReluConstraint relu = prepareRelu( b, f, aux, &tightener );
            relu.notifyLowerBound( b, -8 );
            tightener.getConstraintTightenings( tightenings );
            TS_ASSERT( tightenings.exists( Tightening( aux, 8, Tightening::UB ) ) );
        }

        {
            // Tighter upper bound for aux that is positive
            ReluConstraint relu = prepareRelu( b, f, aux, &tightener );
            relu.notifyUpperBound( aux, 7 );
            tightener.getConstraintTightenings( tightenings );
            TS_ASSERT( tightenings.exists( Tightening( b, -7, Tightening::LB ) ) );
        }

        {
            // Tighter upper bound for b/f that is positive
            ReluConstraint relu = prepareRelu( b, f, aux, &tightener );
            relu.notifyUpperBound( b, 13 );
            tightener.getConstraintTightenings( tightenings );
            TS_ASSERT( tightenings.exists( Tightening( f, 13, Tightening::UB ) ) );

            relu.notifyUpperBound( f, 12 );
            tightener.getConstraintTightenings( tightenings );
            TS_ASSERT( tightenings.exists( Tightening( b, 12, Tightening::UB ) ) );
        }

        {
            // Tighter upper bound 0 for f
            ReluConstraint relu = prepareRelu( b, f, aux, &tightener );
            relu.notifyUpperBound( f, 0 );
            tightener.getConstraintTightenings( tightenings );

            TS_ASSERT( tightenings.exists( Tightening( b, 0, Tightening::UB ) ) );
        }

        {
            // Tighter negative upper bound for b
            ReluConstraint relu = prepareRelu( b, f, aux, &tightener );
            relu.notifyUpperBound( b, -1 );
            tightener.getConstraintTightenings( tightenings );

            TS_ASSERT( tightenings.exists( Tightening( f, 0, Tightening::UB ) ) );
            TS_ASSERT( tightenings.exists( Tightening( aux, 1, Tightening::LB ) ) );
        }

        {
            // Tighter positive lower bound for aux
            ReluConstraint relu = prepareRelu( b, f, aux, &tightener );
            relu.notifyLowerBound( aux, 1 );
            tightener.getConstraintTightenings( tightenings );

            TS_ASSERT( tightenings.exists( Tightening( f, 0, Tightening::UB ) ) );
            TS_ASSERT( tightenings.exists( Tightening( b, -1, Tightening::UB ) ) );
        }
    }

    void test_polarity()
    {
        unsigned b = 1;
        unsigned f = 4;

        PiecewiseLinearCaseSplit activePhase;
        activePhase.storeBoundTightening( Tightening( b, 0.0, Tightening::LB ) );
        Equation activeEquation( Equation::EQ );
        activeEquation.addAddend( 1, b );
        activeEquation.addAddend( -1, f );
        activeEquation.setScalar( 0 );
        activePhase.addEquation( activeEquation );

        PiecewiseLinearCaseSplit inactivePhase;
        inactivePhase.storeBoundTightening( Tightening( b, 0.0, Tightening::UB ) );
        inactivePhase.storeBoundTightening( Tightening( f, 0.0, Tightening::UB ) );

        // b in [1, 2], polarity should be 1, and direction should be PHASE_ACTIVE
        {
            ReluConstraint relu( b, f );
            relu.notifyLowerBound( b, 1 );
            relu.notifyUpperBound( b, 2 );
            TS_ASSERT( relu.computePolarity() == 1 );

            relu.updateDirection();
            TS_ASSERT( relu.getDirection() == ReluConstraint::PHASE_ACTIVE );
        }
        // b in [-2, 0], polarity should be -1, and direction should be PHASE_INACTIVE
        {
            ReluConstraint relu( b, f );
            relu.notifyLowerBound( b, -2 );
            relu.notifyUpperBound( b, 0 );
            TS_ASSERT( relu.computePolarity() == -1 );

            relu.updateDirection();
            TS_ASSERT( relu.getDirection() == ReluConstraint::PHASE_INACTIVE );
        }
        // b in [-2, 2], polarity should be 0, the direction should be PHASE_INACTIVE,
        // the inactive case should be the first element of the returned list by
        // the getCaseSplits(), and getPossibleFix should return the inactive fix first
        {
            ReluConstraint relu( b, f );
            relu.notifyLowerBound( b, -2 );
            relu.notifyUpperBound( b, 2 );
            TS_ASSERT( relu.computePolarity() == 0 );

            relu.updateDirection();
            TS_ASSERT( relu.getDirection() == ReluConstraint::PHASE_INACTIVE );

            auto splits = relu.getCaseSplits();
            auto it = splits.begin();
            TS_ASSERT( *it == inactivePhase );

            List<PiecewiseLinearConstraint::Fix> fixes;
            List<PiecewiseLinearConstraint::Fix>::iterator itFix;

            relu.notifyVariableValue( b, -1 );
            relu.notifyVariableValue( f, 1 );

            fixes = relu.getPossibleFixes();
            itFix = fixes.begin();
            TS_ASSERT_EQUALS( itFix->_variable, f );
            TS_ASSERT_EQUALS( itFix->_value, 0 );
            ++itFix;
            TS_ASSERT_EQUALS( itFix->_variable, b );
            TS_ASSERT_EQUALS( itFix->_value, 1 );

        }
        // b in [-2, 3], polarity should be 0.2, the direction should be PHASE_ACTIVE,
        // the active case should be the first element of the returned list by
        // the getCaseSplits(), and getPossibleFix should return the active fix first
        {
            ReluConstraint relu( b, f );
            relu.notifyLowerBound( b, -2 );
            relu.notifyUpperBound( b, 3 );
            TS_ASSERT( relu.computePolarity() == 0.2 );

            relu.updateDirection();
            TS_ASSERT( relu.getDirection() == ReluConstraint::PHASE_ACTIVE );

            auto splits = relu.getCaseSplits();
            auto it = splits.begin();
            TS_ASSERT( *it == activePhase );

            List<PiecewiseLinearConstraint::Fix> fixes;
            List<PiecewiseLinearConstraint::Fix>::iterator itFix;

            relu.notifyVariableValue( b, -1 );
            relu.notifyVariableValue( f, 1 );

            fixes = relu.getPossibleFixes();
            itFix = fixes.begin();
            TS_ASSERT_EQUALS( itFix->_variable, b );
            TS_ASSERT_EQUALS( itFix->_value, 1 );
            ++itFix;
            TS_ASSERT_EQUALS( itFix->_variable, f );
            TS_ASSERT_EQUALS( itFix->_value, 0 );
        }
    }
};

//
// Local Variables:
// compile-command: "make -C ../../.. "
// tags-file-name: "../../../TAGS"
// c-basic-offset: 4
// End:
//
