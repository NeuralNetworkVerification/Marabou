/*********************                                                        */
/*! \file Test_LeakyReluConstraint.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz, Parth Shah, Duligur Ibeling
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2024 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

**/

#include "LeakyReluConstraint.h"
#include "LinearExpression.h"
#include "MarabouError.h"
#include "MockErrno.h"
#include "MockTableau.h"
#include "PiecewiseLinearCaseSplit.h"
#include "Query.h"
#include "context/context.h"

#include <cxxtest/TestSuite.h>
#include <string.h>

class MockForLeakyReluConstraint : public MockErrno
{
public:
};

/*
   Exposes protected members of LeakyReluConstraint for testing.
 */
class TestLeakyReluConstraint : public LeakyReluConstraint
{
public:
    TestLeakyReluConstraint( unsigned b, unsigned f, double slope )
        : LeakyReluConstraint( b, f, slope )
    {
    }

    using LeakyReluConstraint::getPhaseStatus;
};

using namespace CVC4::context;

class LeakyReluConstraintTestSuite : public CxxTest::TestSuite
{
public:
    MockForLeakyReluConstraint *mock;
    const double slope = 0.2;

    void setUp()
    {
        TS_ASSERT( mock = new MockForLeakyReluConstraint );
    }

    void tearDown()
    {
        TS_ASSERT_THROWS_NOTHING( delete mock );
    }

    void test_leaky_relu_constraint()
    {
        unsigned b = 1;
        unsigned f = 4;

        LeakyReluConstraint lrelu( b, f, slope );
        MockTableau tableau;
        lrelu.registerTableau( &tableau );

        List<unsigned> participatingVariables;
        TS_ASSERT_THROWS_NOTHING( participatingVariables = lrelu.getParticipatingVariables() );
        TS_ASSERT_EQUALS( participatingVariables.size(), 2U );
        auto it = participatingVariables.begin();
        TS_ASSERT_EQUALS( *it, b );
        ++it;
        TS_ASSERT_EQUALS( *it, f );

        TS_ASSERT( lrelu.participatingVariable( b ) );
        TS_ASSERT( lrelu.participatingVariable( f ) );
        TS_ASSERT( !lrelu.participatingVariable( 2 ) );
        TS_ASSERT( !lrelu.participatingVariable( 0 ) );
        TS_ASSERT( !lrelu.participatingVariable( 5 ) );

        TS_ASSERT_THROWS_EQUALS( lrelu.satisfied(),
                                 const MarabouError &e,
                                 e.getCode(),
                                 MarabouError::PARTICIPATING_VARIABLE_MISSING_ASSIGNMENT );

        tableau.setValue( b, 1 );
        tableau.setValue( f, 1 );

        TS_ASSERT( lrelu.satisfied() );

        tableau.setValue( f, 2 );

        TS_ASSERT( !lrelu.satisfied() );

        tableau.setValue( b, 2 );

        TS_ASSERT( lrelu.satisfied() );

        tableau.setValue( b, -2 );

        TS_ASSERT( !lrelu.satisfied() );

        tableau.setValue( f, -2 * slope );

        TS_ASSERT( lrelu.satisfied() );

        tableau.setValue( b, -3 );

        TS_ASSERT( !lrelu.satisfied() );

        tableau.setValue( b, 0 );

        TS_ASSERT( !lrelu.satisfied() );

        tableau.setValue( f, 0 );

        TS_ASSERT( lrelu.satisfied() );

        tableau.setValue( f, 0 );
        tableau.setValue( b, 11 );

        TS_ASSERT( !lrelu.satisfied() );

        // Changing variable indices
        tableau.setValue( b, 1 );
        tableau.setValue( f, 1 );
        TS_ASSERT( lrelu.satisfied() );

        unsigned newB = 12;
        unsigned newF = 14;

        TS_ASSERT_THROWS_NOTHING( lrelu.updateVariableIndex( b, newB ) );
        TS_ASSERT_THROWS_NOTHING( lrelu.updateVariableIndex( f, newF ) );
        tableau.setValue( newB, 1 );
        tableau.setValue( newF, 1 );

        TS_ASSERT( lrelu.satisfied() );

        tableau.setValue( newF, 2 );

        TS_ASSERT( !lrelu.satisfied() );

        tableau.setValue( newB, 2 );

        TS_ASSERT( lrelu.satisfied() );
    }

    void test_leaky_relu_fixes()
    {
        unsigned b = 1;
        unsigned f = 4;

        LeakyReluConstraint relu( b, f, slope );
        MockTableau tableau;
        relu.registerTableau( &tableau );

        List<PiecewiseLinearConstraint::Fix> fixes;
        List<PiecewiseLinearConstraint::Fix>::iterator it;

        tableau.setValue( b, -1 );
        tableau.setValue( f, 1 );

        fixes = relu.getPossibleFixes();
        it = fixes.begin();
        TS_ASSERT_EQUALS( it->_variable, b );
        TS_ASSERT_EQUALS( it->_value, 1 );
        ++it;
        TS_ASSERT_EQUALS( it->_variable, f );
        TS_ASSERT_EQUALS( it->_value, slope * -1 );

        tableau.setValue( b, 2 );
        tableau.setValue( f, 1 );

        fixes = relu.getPossibleFixes();
        it = fixes.begin();
        TS_ASSERT_EQUALS( it->_variable, b );
        TS_ASSERT_EQUALS( it->_value, 1 );
        ++it;
        TS_ASSERT_EQUALS( it->_variable, f );
        TS_ASSERT_EQUALS( it->_value, 2 );

        tableau.setValue( b, -2 );
        tableau.setValue( f, -1 );

        fixes = relu.getPossibleFixes();
        it = fixes.begin();
        TS_ASSERT_EQUALS( it->_variable, f );
        TS_ASSERT_EQUALS( it->_value, slope * -2 );
        ++it;
        TS_ASSERT_EQUALS( it->_variable, b );
        TS_ASSERT_EQUALS( it->_value, -1 / slope );

        tableau.setValue( b, 11 );
        tableau.setValue( f, -1 );

        fixes = relu.getPossibleFixes();
        it = fixes.begin();
        TS_ASSERT_EQUALS( it->_variable, b );
        TS_ASSERT_EQUALS( it->_value, -1 / slope );
        ++it;
        TS_ASSERT_EQUALS( it->_variable, f );
        TS_ASSERT_EQUALS( it->_value, 11 );
    }

    void test_leaky_relu_case_splits()
    {
        unsigned b = 1;
        unsigned f = 4;

        LeakyReluConstraint lrelu( b, f, slope );

        List<PiecewiseLinearCaseSplit> splits = lrelu.getCaseSplits();

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
        TS_ASSERT_EQUALS( addend->_coefficient, slope );
        TS_ASSERT_EQUALS( addend->_variable, b );

        ++addend;
        TS_ASSERT_EQUALS( addend->_coefficient, -1.0 );
        TS_ASSERT_EQUALS( addend->_variable, f );

        TS_ASSERT_EQUALS( inactiveEquation._type, Equation::EQ );

        return true;
    }

    void test_leaky_relu_case_splits_with_aux_var()
    {
        unsigned b = 1;
        unsigned f = 4;

        LeakyReluConstraint lrelu( b, f, slope );

        lrelu.notifyLowerBound( b, -10 );
        lrelu.notifyUpperBound( b, 5 );
        lrelu.notifyUpperBound( f, 5 );

        unsigned auxVarActive = 10;
        unsigned auxVarInactive = 11;
        Query inputQuery;
        inputQuery.setNumberOfVariables( auxVarActive );

        lrelu.transformToUseAuxVariables( inputQuery );

        TS_ASSERT( lrelu.auxVariablesInUse() );
        TS_ASSERT_EQUALS( lrelu.getActiveAux(), auxVarActive );
        TS_ASSERT_EQUALS( lrelu.getInactiveAux(), auxVarInactive );

        List<PiecewiseLinearCaseSplit> splits = lrelu.getCaseSplits();

        Equation activeEquation, inactiveEquation;

        TS_ASSERT_EQUALS( splits.size(), 2U );

        List<PiecewiseLinearCaseSplit>::iterator split1 = splits.begin();
        List<PiecewiseLinearCaseSplit>::iterator split2 = split1;
        ++split2;

        TS_ASSERT( isActiveSplitWithAux( b, f, auxVarActive, split1 ) ||
                   isActiveSplitWithAux( b, f, auxVarActive, split2 ) );
        TS_ASSERT( isInactiveSplitWithAux( b, f, auxVarInactive, split1 ) ||
                   isInactiveSplitWithAux( b, f, auxVarInactive, split2 ) );
    }

    bool isActiveSplitWithAux( unsigned b,
                               unsigned f,
                               unsigned activeAux,
                               List<PiecewiseLinearCaseSplit>::iterator &split )
    {
        List<Tightening> bounds = split->getBoundTightenings();

        TS_ASSERT_EQUALS( bounds.size(), 3U );

        auto bound = bounds.begin();
        Tightening bound1 = *bound;

        TS_ASSERT_EQUALS( bound1._variable, b );
        TS_ASSERT_EQUALS( bound1._value, 0.0 );

        if ( bound1._type != Tightening::LB )
            return false;

        ++bound;

        Tightening bound2 = *bound;

        TS_ASSERT_EQUALS( bound2._variable, f );
        TS_ASSERT_EQUALS( bound2._value, 0.0 );

        if ( bound2._type != Tightening::LB )
            return false;

        ++bound;

        Tightening bound3 = *bound;

        TS_ASSERT_EQUALS( bound3._variable, activeAux );
        TS_ASSERT_EQUALS( bound3._value, 0.0 );

        if ( bound3._type != Tightening::UB )
            return false;

        TS_ASSERT( split->getEquations().empty() );

        return true;
    }

    bool isInactiveSplitWithAux( unsigned b,
                                 unsigned f,
                                 unsigned inactiveAux,
                                 List<PiecewiseLinearCaseSplit>::iterator &split )
    {
        List<Tightening> bounds = split->getBoundTightenings();

        TS_ASSERT_EQUALS( bounds.size(), 3U );

        auto bound = bounds.begin();
        Tightening bound1 = *bound;

        TS_ASSERT_EQUALS( bound1._variable, b );
        TS_ASSERT_EQUALS( bound1._value, 0.0 );

        if ( bound1._type != Tightening::UB )
            return false;

        ++bound;

        Tightening bound2 = *bound;

        TS_ASSERT_EQUALS( bound2._variable, f );
        TS_ASSERT_EQUALS( bound2._value, 0.0 );

        if ( bound2._type != Tightening::UB )
            return false;

        ++bound;

        Tightening bound3 = *bound;

        TS_ASSERT_EQUALS( bound3._variable, inactiveAux );
        TS_ASSERT_EQUALS( bound3._value, 0.0 );

        if ( bound3._type != Tightening::UB )
            return false;

        TS_ASSERT( split->getEquations().empty() );

        return true;
    }

    void test_register_as_watcher()
    {
        unsigned b = 1;
        unsigned f = 4;

        MockTableau tableau;

        LeakyReluConstraint lrelu( b, f, slope );

        TS_ASSERT_THROWS_NOTHING( lrelu.registerAsWatcher( &tableau ) );

        TS_ASSERT_EQUALS( tableau.lastRegisteredVariableToWatcher.size(), 2U );
        TS_ASSERT( tableau.lastUnregisteredVariableToWatcher.empty() );
        TS_ASSERT_EQUALS( tableau.lastRegisteredVariableToWatcher[b].size(), 1U );
        TS_ASSERT( tableau.lastRegisteredVariableToWatcher[b].exists( &lrelu ) );
        TS_ASSERT_EQUALS( tableau.lastRegisteredVariableToWatcher[f].size(), 1U );
        TS_ASSERT( tableau.lastRegisteredVariableToWatcher[f].exists( &lrelu ) );

        tableau.lastRegisteredVariableToWatcher.clear();

        TS_ASSERT_THROWS_NOTHING( lrelu.unregisterAsWatcher( &tableau ) );

        TS_ASSERT( tableau.lastRegisteredVariableToWatcher.empty() );
        TS_ASSERT_EQUALS( tableau.lastUnregisteredVariableToWatcher.size(), 2U );
        TS_ASSERT_EQUALS( tableau.lastUnregisteredVariableToWatcher[b].size(), 1U );
        TS_ASSERT( tableau.lastUnregisteredVariableToWatcher[b].exists( &lrelu ) );
        TS_ASSERT_EQUALS( tableau.lastUnregisteredVariableToWatcher[f].size(), 1U );
        TS_ASSERT( tableau.lastUnregisteredVariableToWatcher[f].exists( &lrelu ) );
    }

    void test_valid_split_relu_phase_fixed_to_active()
    {
        unsigned b = 1;
        unsigned f = 4;

        LeakyReluConstraint lrelu( b, f, slope );

        TS_ASSERT( !lrelu.phaseFixed() );
        TS_ASSERT_THROWS_NOTHING( lrelu.notifyLowerBound( b, 5 ) );
        TS_ASSERT( lrelu.phaseFixed() );

        PiecewiseLinearCaseSplit split;
        TS_ASSERT_THROWS_NOTHING( split = lrelu.getValidCaseSplit() );

        List<PiecewiseLinearCaseSplit> dummy;
        dummy.append( split );
        List<PiecewiseLinearCaseSplit>::iterator split1 = dummy.begin();

        TS_ASSERT( isActiveSplit( b, f, split1 ) );
    }

    void test_valid_split_relu_phase_fixed_to_inactive()
    {
        unsigned b = 1;
        unsigned f = 4;

        LeakyReluConstraint lrelu( b, f, slope );

        TS_ASSERT( !lrelu.phaseFixed() );
        TS_ASSERT_THROWS_NOTHING( lrelu.notifyUpperBound( b, -2 ) );
        TS_ASSERT( lrelu.phaseFixed() );

        PiecewiseLinearCaseSplit split;
        TS_ASSERT_THROWS_NOTHING( split = lrelu.getValidCaseSplit() );

        List<PiecewiseLinearCaseSplit> dummy;
        dummy.append( split );
        List<PiecewiseLinearCaseSplit>::iterator split1 = dummy.begin();

        TS_ASSERT( isInactiveSplit( b, f, split1 ) );
    }

    void test_leaky_relu_entailed_tightenings()
    {
        unsigned b = 1;
        unsigned f = 4;

        LeakyReluConstraint lrelu( b, f, slope );

        lrelu.notifyUpperBound( b, 7 );
        lrelu.notifyUpperBound( f, 7 );

        lrelu.notifyLowerBound( b, -1 );
        lrelu.notifyLowerBound( f, slope * -1 );

        List<Tightening> entailedTightenings;
        lrelu.getEntailedTightenings( entailedTightenings );

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
        lrelu.notifyLowerBound( b, 1 );
        lrelu.notifyLowerBound( f, 2 );

        lrelu.getEntailedTightenings( entailedTightenings );

        TS_ASSERT_EQUALS( entailedTightenings.size(), 6U );
        TS_ASSERT( entailedTightenings.exists( Tightening( f, 1, Tightening::LB ) ) );
        TS_ASSERT( entailedTightenings.exists( Tightening( b, 2, Tightening::LB ) ) );
        TS_ASSERT( entailedTightenings.exists( Tightening( f, 7, Tightening::UB ) ) );
        TS_ASSERT( entailedTightenings.exists( Tightening( b, 7, Tightening::UB ) ) );

        TS_ASSERT( entailedTightenings.exists( Tightening( b, 0, Tightening::LB ) ) );
        TS_ASSERT( entailedTightenings.exists( Tightening( f, 0, Tightening::LB ) ) );

        entailedTightenings.clear();

        // Negative upper bound for b: inactive case. f is 0. B is non-positive.

        LeakyReluConstraint lrelu2( b, f, slope );

        lrelu2.notifyLowerBound( b, -3 );
        lrelu2.notifyLowerBound( f, -2 );

        lrelu2.notifyUpperBound( b, -1 );
        lrelu2.notifyUpperBound( f, 7 );

        lrelu2.getEntailedTightenings( entailedTightenings );

        TS_ASSERT_EQUALS( entailedTightenings.size(), 5U );

        TS_ASSERT( entailedTightenings.exists( Tightening( b, 0, Tightening::UB ) ) );
        TS_ASSERT( entailedTightenings.exists( Tightening( f, 0, Tightening::UB ) ) );

        TS_ASSERT( entailedTightenings.exists( Tightening( b, -2 / slope, Tightening::LB ) ) );
        TS_ASSERT( entailedTightenings.exists( Tightening( f, -3 * slope, Tightening::LB ) ) );

        TS_ASSERT( entailedTightenings.exists( Tightening( f, -1 * slope, Tightening::UB ) ) );
    }

    void test_leaky_relu_duplicate_and_restore()
    {
        LeakyReluConstraint *lrelu1 = new LeakyReluConstraint( 4, 6, slope );
        MockTableau tableau;
        lrelu1->registerTableau( &tableau );
        lrelu1->setActiveConstraint( false );
        tableau.setValue( 4, 1.0 );
        tableau.setValue( 6, 1.0 );

        lrelu1->notifyLowerBound( 4, -8.0 );
        lrelu1->notifyUpperBound( 4, 8.0 );

        lrelu1->notifyLowerBound( 6, 0.0 );
        lrelu1->notifyUpperBound( 6, 8.0 );

        PiecewiseLinearConstraint *lrelu2 = lrelu1->duplicateConstraint();
        TS_ASSERT( lrelu2->satisfied() );
        tableau.setValue( 4, -2 );
        TS_ASSERT( !lrelu1->satisfied() );

        TS_ASSERT( !lrelu2->isActive() );
        TS_ASSERT( !lrelu2->satisfied() );

        lrelu2->restoreState( lrelu1 );
        TS_ASSERT( !lrelu2->satisfied() );

        TS_ASSERT_THROWS_NOTHING( delete lrelu1 );
        TS_ASSERT_THROWS_NOTHING( delete lrelu2 );
    }

    void test_eliminate_variable_active()
    {
        unsigned b = 1;
        unsigned f = 4;

        MockTableau tableau;

        LeakyReluConstraint lrelu( b, f, slope );

        lrelu.registerAsWatcher( &tableau );

        TS_ASSERT( !lrelu.constraintObsolete() );
        TS_ASSERT_THROWS_NOTHING( lrelu.eliminateVariable( b, 5 ) );
        TS_ASSERT( lrelu.constraintObsolete() );
    }

    void test_serialize_and_unserialize()
    {
        unsigned b = 42;
        unsigned f = 7;

        LeakyReluConstraint originalLRelu( b, f, slope );
        originalLRelu.notifyLowerBound( b, -10 );
        originalLRelu.notifyUpperBound( f, 5 );
        originalLRelu.notifyUpperBound( f, -1 );
        originalLRelu.notifyUpperBound( f, 5 );

        String originalSerialized = originalLRelu.serializeToString();
        LeakyReluConstraint recoveredLRelu( originalSerialized );

        TS_ASSERT_EQUALS( originalLRelu.serializeToString(), recoveredLRelu.serializeToString() );

        TS_ASSERT( !originalLRelu.auxVariablesInUse() );
        TS_ASSERT( !recoveredLRelu.auxVariablesInUse() );

        Query dontCare;
        dontCare.setNumberOfVariables( 500 );

        originalLRelu.transformToUseAuxVariables( dontCare );

        TS_ASSERT( originalLRelu.auxVariablesInUse() );

        originalSerialized = originalLRelu.serializeToString();
        LeakyReluConstraint recoveredLRelu2( originalSerialized );

        TS_ASSERT_EQUALS( originalLRelu.serializeToString(), recoveredLRelu2.serializeToString() );

        TS_ASSERT( recoveredLRelu2.auxVariablesInUse() );
        TS_ASSERT_EQUALS( originalLRelu.getActiveAux(), recoveredLRelu2.getActiveAux() );
        TS_ASSERT_EQUALS( originalLRelu.getInactiveAux(), recoveredLRelu2.getInactiveAux() );
    }

    LeakyReluConstraint
    prepareLeakyRelu( unsigned b, unsigned f, unsigned activeAux, BoundManager *boundManager )
    {
        LeakyReluConstraint lrelu( b, f, slope );
        boundManager->initialize( activeAux + 2 );

        Query dontCare;
        dontCare.setNumberOfVariables( activeAux );
        lrelu.registerBoundManager( boundManager );

        lrelu.notifyLowerBound( b, -10 );
        lrelu.notifyLowerBound( f, slope * -10 );

        lrelu.notifyUpperBound( b, 15 );
        lrelu.notifyUpperBound( f, 15 );

        TS_ASSERT_THROWS_NOTHING( lrelu.transformToUseAuxVariables( dontCare ) );

        lrelu.notifyLowerBound( activeAux, 0 );
        lrelu.notifyUpperBound( activeAux, 10 );

        unsigned inactiveAux = activeAux + 1;
        lrelu.notifyLowerBound( inactiveAux, 0 );
        lrelu.notifyUpperBound( inactiveAux, 15 );


        boundManager->clearTightenings();
        return lrelu;
    }

    void test_notify_bounds()
    {
        unsigned b = 1;
        unsigned f = 4;
        unsigned activeAux = 10;
        Context context;

        // Initial state: b in [-10, 15], f in [slope * -10, 15]

        {
            BoundManager boundManager( context );
            List<Tightening> tightenings;
            LeakyReluConstraint lrelu = prepareLeakyRelu( b, f, activeAux, &boundManager );
            lrelu.notifyLowerBound( b, -20 );
            boundManager.getTightenings( tightenings );
            TS_ASSERT( tightenings.empty() );

            lrelu.notifyLowerBound( f, -20 * slope );
            boundManager.getTightenings( tightenings );
            TS_ASSERT( tightenings.empty() );
            for ( const auto &t : tightenings )
                t.dump();

            lrelu.notifyUpperBound( b, 20 );
            boundManager.getTightenings( tightenings );
            TS_ASSERT( tightenings.empty() );

            lrelu.notifyUpperBound( f, 23 );
            boundManager.getTightenings( tightenings );
            TS_ASSERT( tightenings.empty() );
        }

        {
            // Tighter lower bound for b that is negative
            BoundManager boundManager( context );
            List<Tightening> tightenings;
            LeakyReluConstraint lrelu = prepareLeakyRelu( b, f, activeAux, &boundManager );
            lrelu.notifyLowerBound( b, -8 );
            boundManager.getTightenings( tightenings );
            TS_ASSERT( tightenings.exists( Tightening( f, -8 * slope, Tightening::LB ) ) );
        }

        {
            // Tighter upper bound for b/f that is positive
            BoundManager boundManager( context );
            List<Tightening> tightenings;
            LeakyReluConstraint lrelu = prepareLeakyRelu( b, f, activeAux, &boundManager );
            lrelu.notifyUpperBound( b, 13 );
            boundManager.getTightenings( tightenings );
            TS_ASSERT( tightenings.exists( Tightening( f, 13, Tightening::UB ) ) );

            lrelu.notifyUpperBound( f, 12 );
            boundManager.getTightenings( tightenings );
            TS_ASSERT( tightenings.exists( Tightening( b, 12, Tightening::UB ) ) );
        }

        {
            // Tighter upper bound 0 for f
            BoundManager boundManager( context );
            List<Tightening> tightenings;
            LeakyReluConstraint lrelu = prepareLeakyRelu( b, f, activeAux, &boundManager );
            lrelu.notifyUpperBound( f, 0 );
            boundManager.getTightenings( tightenings );

            TS_ASSERT( tightenings.exists( Tightening( b, 0, Tightening::UB ) ) );
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
        inactiveEquation.addAddend( slope, b );
        inactiveEquation.addAddend( -1, f );
        inactiveEquation.setScalar( 0 );
        inactivePhase.addEquation( inactiveEquation );

        // b in [1, 2], polarity should be 1, and direction should be RELU_PHASE_ACTIVE
        {
            LeakyReluConstraint lrelu( b, f, slope );
            lrelu.notifyLowerBound( b, 1 );
            lrelu.notifyUpperBound( b, 2 );
            TS_ASSERT( lrelu.computePolarity() == 1 );

            lrelu.updateDirection();
            TS_ASSERT( lrelu.getDirection() == RELU_PHASE_ACTIVE );
        }
        // b in [-2, 0], polarity should be -1, and direction should be RELU_PHASE_INACTIVE
        {
            LeakyReluConstraint lrelu( b, f, slope );
            lrelu.notifyLowerBound( b, -2 );
            lrelu.notifyUpperBound( b, 0 );
            TS_ASSERT( lrelu.computePolarity() == -1 );

            lrelu.updateDirection();
            TS_ASSERT( lrelu.getDirection() == RELU_PHASE_INACTIVE );
        }
        // b in [-2, 2], polarity should be 0, the direction should be RELU_PHASE_INACTIVE,
        // the inactive case should be the first element of the returned list by
        // the getCaseSplits(), and getPossibleFix should return the inactive fix first
        {
            LeakyReluConstraint lrelu( b, f, slope );
            lrelu.notifyLowerBound( b, -2 );
            lrelu.notifyUpperBound( b, 2 );
            TS_ASSERT( lrelu.computePolarity() == 0 );

            lrelu.updateDirection();
            TS_ASSERT( lrelu.getDirection() == RELU_PHASE_INACTIVE );

            auto splits = lrelu.getCaseSplits();
            auto it = splits.begin();
            TS_ASSERT( *it == inactivePhase );
        }
        // b in [-2, 3], polarity should be 0.2, the direction should be RELU_PHASE_ACTIVE,
        // the active case should be the first element of the returned list by
        // the getCaseSplits(), and getPossibleFix should return the active fix first
        {
            LeakyReluConstraint lrelu( b, f, slope );
            lrelu.notifyLowerBound( b, -2 );
            lrelu.notifyUpperBound( b, 3 );
            TS_ASSERT( lrelu.computePolarity() == 0.2 );

            lrelu.updateDirection();
            TS_ASSERT( lrelu.getDirection() == RELU_PHASE_ACTIVE );

            auto splits = lrelu.getCaseSplits();
            auto it = splits.begin();
            TS_ASSERT( *it == activePhase );
        }
    }

    /*
      Test Case functionality of LeakyReluConstraint
      1. Check that all cases are returned by LeakyReluConstraint::getAllCases()
      2. Check that LeakyReluConstraint::getCaseSplit( case ) returns the correct case
     */
    void test_leaky_relu_get_cases()
    {
        unsigned b = 1;
        unsigned f = 4;

        LeakyReluConstraint lrelu( b, f, slope );

        List<PhaseStatus> cases = lrelu.getAllCases();

        TS_ASSERT_EQUALS( cases.size(), 2u );
        TS_ASSERT_EQUALS( cases.front(), RELU_PHASE_INACTIVE );
        TS_ASSERT_EQUALS( cases.back(), RELU_PHASE_ACTIVE );

        List<PiecewiseLinearCaseSplit> splits = lrelu.getCaseSplits();
        TS_ASSERT_EQUALS( splits.size(), 2u );
        TS_ASSERT_EQUALS( splits.front(), lrelu.getCaseSplit( RELU_PHASE_INACTIVE ) );
        TS_ASSERT_EQUALS( splits.back(), lrelu.getCaseSplit( RELU_PHASE_ACTIVE ) );
    }

    /*
      Test context-dependent lrelu state behavior.
     */
    void test_leaky_relu_context_dependent_state()
    {
        Context context;
        unsigned b = 1;
        unsigned f = 4;

        TestLeakyReluConstraint lrelu( b, f, slope );

        lrelu.initializeCDOs( &context );


        TS_ASSERT_EQUALS( lrelu.getPhaseStatus(), PHASE_NOT_FIXED );

        context.push();

        lrelu.notifyLowerBound( f, 1 );
        TS_ASSERT_EQUALS( lrelu.getPhaseStatus(), RELU_PHASE_ACTIVE );

        context.pop();
        TS_ASSERT_EQUALS( lrelu.getPhaseStatus(), PHASE_NOT_FIXED );

        context.push();
        lrelu.notifyUpperBound( b, -1 );
        TS_ASSERT_EQUALS( lrelu.getPhaseStatus(), RELU_PHASE_INACTIVE );

        context.pop();
        TS_ASSERT_EQUALS( lrelu.getPhaseStatus(), PHASE_NOT_FIXED );
    }

    /*
      Test correct initialization of context-dependent data structures.
     */
    void test_initialization_of_CDOs()
    {
        Context context;
        LeakyReluConstraint *lrelu1 = new LeakyReluConstraint( 4, 6, slope );

        TS_ASSERT_EQUALS( lrelu1->getContext(),
                          static_cast<Context *>( static_cast<Context *>( nullptr ) ) );

        TS_ASSERT_EQUALS( lrelu1->getActiveStatusCDO(), static_cast<CDO<bool> *>( nullptr ) );
        TS_ASSERT_EQUALS( lrelu1->getPhaseStatusCDO(), static_cast<CDO<PhaseStatus> *>( nullptr ) );
        TS_ASSERT_EQUALS( lrelu1->getInfeasibleCasesCDList(),
                          static_cast<CDList<PhaseStatus> *>( nullptr ) );
        TS_ASSERT_THROWS_NOTHING( lrelu1->initializeCDOs( &context ) );
        TS_ASSERT_EQUALS( lrelu1->getContext(), &context );
        TS_ASSERT_DIFFERS( lrelu1->getActiveStatusCDO(), static_cast<CDO<bool> *>( nullptr ) );
        TS_ASSERT_DIFFERS( lrelu1->getPhaseStatusCDO(),
                           static_cast<CDO<PhaseStatus> *>( nullptr ) );
        TS_ASSERT_DIFFERS( lrelu1->getInfeasibleCasesCDList(),
                           static_cast<CDList<PhaseStatus> *>( nullptr ) );

        bool active = false;
        TS_ASSERT_THROWS_NOTHING( active = lrelu1->isActive() );
        TS_ASSERT_EQUALS( active, true );

        bool phaseFixed = true;
        TS_ASSERT_THROWS_NOTHING( phaseFixed = lrelu1->phaseFixed() );
        TS_ASSERT_EQUALS( phaseFixed, PHASE_NOT_FIXED );
        TS_ASSERT_EQUALS( lrelu1->numFeasibleCases(), 2u );
        TS_ASSERT( lrelu1->isFeasible() );
        TS_ASSERT( !lrelu1->isImplication() );

        TS_ASSERT_THROWS_NOTHING( delete lrelu1 );
    }

    /*
      Test context-dependent storage of search state information via
      isFeasible/markInfeasible methods.
     */
    void test_lazy_backtracking_of_CDOs()
    {
        Context context;
        LeakyReluConstraint *lrelu1 = new LeakyReluConstraint( 4, 6, slope );
        TS_ASSERT_THROWS_NOTHING( lrelu1->initializeCDOs( &context ) );

        // L0 - Feasible, not an implication
        PhaseStatus phase1 = PHASE_NOT_FIXED;
        TS_ASSERT_THROWS_NOTHING( phase1 = lrelu1->nextFeasibleCase() );
        TS_ASSERT_EQUALS( phase1, lrelu1->nextFeasibleCase() );
        TS_ASSERT( lrelu1->isFeasible() );
        TS_ASSERT( !lrelu1->isImplication() );


        // L1 - Feasible, an implication, nextFeasibleCase returns a new case
        TS_ASSERT_THROWS_NOTHING( context.push() );
        TS_ASSERT_THROWS_NOTHING( lrelu1->markInfeasible( phase1 ) );
        TS_ASSERT( lrelu1->isFeasible() );
        TS_ASSERT( lrelu1->isImplication() );

        PhaseStatus phase2 = PHASE_NOT_FIXED;
        TS_ASSERT_THROWS_NOTHING( phase2 = lrelu1->nextFeasibleCase() );
        TS_ASSERT_EQUALS( phase2, lrelu1->nextFeasibleCase() );
        TS_ASSERT_DIFFERS( phase2, phase1 );

        // L2 - Infeasible, not an implication, nextCase returns CONSTRAINT_INFEASIBLE
        TS_ASSERT_THROWS_NOTHING( context.push() );
        TS_ASSERT_THROWS_NOTHING( lrelu1->markInfeasible( phase2 ) );
        TS_ASSERT( !lrelu1->isFeasible() );
        TS_ASSERT( !lrelu1->isImplication() );
        TS_ASSERT_EQUALS( lrelu1->nextFeasibleCase(), CONSTRAINT_INFEASIBLE );

        // L1 again - Feasible, an implication, nextFeasibleCase returns same values as phase2
        TS_ASSERT_THROWS_NOTHING( context.pop() );
        TS_ASSERT( lrelu1->isFeasible() );
        TS_ASSERT( lrelu1->isImplication() );
        TS_ASSERT_EQUALS( phase2, lrelu1->nextFeasibleCase() );

        // L0 again - Feasible, not an implication, nextFeasibleCase returns same value as phase1
        TS_ASSERT_THROWS_NOTHING( context.pop() );
        TS_ASSERT( lrelu1->isFeasible() );
        TS_ASSERT( !lrelu1->isImplication() );
        TS_ASSERT_EQUALS( phase1, lrelu1->nextFeasibleCase() );

        TS_ASSERT_THROWS_NOTHING( delete lrelu1 );
    }
};
