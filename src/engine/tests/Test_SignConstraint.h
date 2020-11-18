/*********************                                                        */
/*! \file Test_SignConstraint.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Amir
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
#include "MarabouError.h"
#include "MockConstraintBoundTightener.h"
#include "MockErrno.h"
#include "MockTableau.h"
#include "PiecewiseLinearCaseSplit.h"
#include "SignConstraint.h"

#include <string.h>

class MockForSignConstraint
    : public MockErrno
{
public:
};

class SignConstraintTestSuite : public CxxTest::TestSuite
{
public:
    MockForSignConstraint *mock;

    void setUp()
    {
        TS_ASSERT( mock = new MockForSignConstraint );
    }

    void tearDown()
    {
        TS_ASSERT_THROWS_NOTHING( delete mock );
    }

    void test_sign_constraint()
    {
        unsigned b = 1;
        unsigned f = 4;

        SignConstraint sign( b, f );

        List<unsigned> participatingVariables;

        TS_ASSERT_THROWS_NOTHING( participatingVariables = sign.getParticipatingVariables() );

        TS_ASSERT_EQUALS( participatingVariables.size(), 2U );

        auto it = participatingVariables.begin();
        TS_ASSERT_EQUALS( *it, b );
        ++it;
        TS_ASSERT_EQUALS( *it, f );

        TS_ASSERT( sign.participatingVariable( b ) );
        TS_ASSERT( sign.participatingVariable( f ) );
        TS_ASSERT( !sign.participatingVariable( 2 ) );
        TS_ASSERT( !sign.participatingVariable( 0 ) );
        TS_ASSERT( !sign.participatingVariable( 5 ) );

        TS_ASSERT_THROWS_EQUALS( sign.satisfied(),
                                 const MarabouError &e,
                                 e.getCode(),
                                 MarabouError::PARTICIPATING_VARIABLES_ABSENT );

        sign.notifyVariableValue( b, -1 );
        sign.notifyVariableValue( f, -1 );

        TS_ASSERT( sign.satisfied() );

        sign.notifyVariableValue( f, 1 );

        TS_ASSERT( !sign.satisfied() );

        sign.notifyVariableValue( b, 2 );

        TS_ASSERT( sign.satisfied() );

        sign.notifyVariableValue( b, -2 );

        TS_ASSERT( !sign.satisfied() );

        sign.notifyVariableValue( f, 1 );

        TS_ASSERT( !sign.satisfied() );

        sign.notifyVariableValue( b, 0 );

        TS_ASSERT( sign.satisfied() );

        sign.notifyVariableValue( b, 9 );

        TS_ASSERT( sign.satisfied() );

        sign.notifyVariableValue( 4, -8 );

        // A sign constraint cannot be satisfied if f is not '1' or '-1'
        TS_ASSERT( !sign.satisfied() );

        sign.notifyVariableValue( 4, 1.5 );

        // A sign cannot be satisfied if f is not '1' or '-1'
        TS_ASSERT( !sign.satisfied() );

        sign.notifyVariableValue( f, -1 );
        sign.notifyVariableValue( b, 11 );

        TS_ASSERT( !sign.satisfied() );

        // Changing variable indices
        sign.notifyVariableValue( b, 1 );
        sign.notifyVariableValue( f, 1 );
        TS_ASSERT( sign.satisfied() );

        unsigned newB = 12;
        unsigned newF = 14;

        TS_ASSERT_THROWS_NOTHING( sign.updateVariableIndex( b, newB ) );
        TS_ASSERT_THROWS_NOTHING( sign.updateVariableIndex( f, newF ) );

        TS_ASSERT( sign.satisfied() );

        sign.notifyVariableValue( newF, -1 );

        TS_ASSERT( !sign.satisfied() );

        sign.notifyVariableValue( newB, -0.1 );

        TS_ASSERT( sign.satisfied());
    }

    void test_sign_fixes()
    {
        unsigned b = 1;
        unsigned f = 4;

        SignConstraint sign( b, f );

        List<PiecewiseLinearConstraint::Fix> fixes;
        List<PiecewiseLinearConstraint::Fix>::iterator it;

        sign.notifyVariableValue( b, -1 );
        sign.notifyVariableValue( f, 1 );

        fixes = sign.getPossibleFixes();
        it = fixes.begin();

        TS_ASSERT_EQUALS( it->_variable, f );
        TS_ASSERT_EQUALS( it->_value, -1 );

        sign.notifyVariableValue( b, 0 );
        sign.notifyVariableValue( f, -1 );

        fixes = sign.getPossibleFixes();
        it = fixes.begin();

        TS_ASSERT_EQUALS( it->_variable, f );
        TS_ASSERT_EQUALS( it->_value, 1 );

        sign.notifyVariableValue( b, 3 );
        sign.notifyVariableValue( f, -1 );

        fixes = sign.getPossibleFixes();
        it = fixes.begin();

        TS_ASSERT_EQUALS( it->_variable, f );
        TS_ASSERT_EQUALS( it->_value, 1 );

        sign.notifyVariableValue( b, -2);
        sign.notifyVariableValue( f, 1 );

        fixes = sign.getPossibleFixes();
        it = fixes.begin();

        TS_ASSERT_EQUALS( it->_variable, f );
        TS_ASSERT_EQUALS( it->_value, -1 );

        sign.notifyVariableValue( b, 11 );
        sign.notifyVariableValue( f, 0 );

        fixes = sign.getPossibleFixes();
        it = fixes.begin();

        TS_ASSERT_EQUALS( it->_variable, f );
        TS_ASSERT_EQUALS( it->_value, 1 );
    }

    void test_sign_case_splits()
    {
        unsigned b = 1;
        unsigned f = 4;

        SignConstraint sign( b, f );

        List<PiecewiseLinearConstraint::Fix> fixes;
        List<PiecewiseLinearConstraint::Fix>::iterator it;

        List<PiecewiseLinearCaseSplit> splits = sign.getCaseSplits();

        TS_ASSERT_EQUALS( splits.size(), 2U );

        List<PiecewiseLinearCaseSplit>::iterator split1 = splits.begin();
        List<PiecewiseLinearCaseSplit>::iterator split2 = split1;
        ++split2;

        TS_ASSERT( isPositiveSplit( b, f, split1 ) || isPositiveSplit( b, f, split2 ) );
        TS_ASSERT( isNegativeSplit( b, f, split1 ) || isNegativeSplit( b, f, split2 ) );
    }

    // Return true only if the 2 bounds match and there are no equations
    bool isPositiveSplit( unsigned b, unsigned f,
                          List<PiecewiseLinearCaseSplit>::iterator &split )
    {
        List<Tightening> bounds = split->getBoundTightenings();

        auto bound = bounds.begin();
        Tightening bound1 = *bound;

        if ( ( bound1._variable != b ) ||
             ( bound1._value != 0.0 ) ||
             ( bound1._type != Tightening::LB ) )
            return false;

        if ( bounds.size() != 2U )
            return false;

        ++bound;
        Tightening bound2 = *bound;

        if ( ( bound2._variable != f ) ||
             ( bound2._value != 1 ) ||
             ( bound2._type != Tightening::LB ) )
            return false;

        if ( !split->getEquations().empty() )
            return false;

        return true;
    }

    // Return true only if the 2 bounds match and there are no equations
    bool isNegativeSplit( unsigned b, unsigned f,
                          List<PiecewiseLinearCaseSplit>::iterator &split )
    {
        List<Tightening> bounds = split->getBoundTightenings();

        auto bound = bounds.begin();
        Tightening bound1 = *bound;

        if ( ( bound1._variable != b ) ||
             ( bound1._value != 0.0 ) ||
             ( bound1._type != Tightening::UB ) )
            return false;

        if ( bounds.size() != 2U )
            return false;

        ++bound;
        Tightening bound2 = *bound;

        if ( ( bound2._variable != f ) ||
             ( bound2._value != -1 ) ||
             ( bound2._type != Tightening::UB ) )
            return false;

        if ( !split->getEquations().empty() )
            return false;

        return true;
    }

    void test_register_as_watcher()
    {
        unsigned b = 1;
        unsigned f = 4;

        MockTableau tableau;

        SignConstraint sign( b, f );

        TS_ASSERT_THROWS_NOTHING( sign.registerAsWatcher( &tableau ) );

        TS_ASSERT_EQUALS( tableau.lastRegisteredVariableToWatcher.size(), 2U );
        TS_ASSERT( tableau.lastUnregisteredVariableToWatcher.empty() );
        TS_ASSERT_EQUALS( tableau.lastRegisteredVariableToWatcher[b].size(), 1U );
        TS_ASSERT( tableau.lastRegisteredVariableToWatcher[b].exists( &sign ) );
        TS_ASSERT_EQUALS( tableau.lastRegisteredVariableToWatcher[f].size(), 1U );
        TS_ASSERT( tableau.lastRegisteredVariableToWatcher[f].exists( &sign ) );

        tableau.lastRegisteredVariableToWatcher.clear();

        TS_ASSERT_THROWS_NOTHING( sign.unregisterAsWatcher( &tableau ) );

        TS_ASSERT( tableau.lastRegisteredVariableToWatcher.empty() );
        TS_ASSERT_EQUALS( tableau.lastUnregisteredVariableToWatcher.size(), 2U );
        TS_ASSERT_EQUALS( tableau.lastUnregisteredVariableToWatcher[b].size(), 1U );
        TS_ASSERT( tableau.lastUnregisteredVariableToWatcher[b].exists( &sign ) );
        TS_ASSERT_EQUALS( tableau.lastUnregisteredVariableToWatcher[f].size(), 1U );
        TS_ASSERT( tableau.lastUnregisteredVariableToWatcher[f].exists( &sign ) );
    }

    void test_case_splits_after_fixed_to_positive()
    {
        unsigned b = 1;
        unsigned f = 4;

        MockTableau tableau;

        SignConstraint sign( b, f );

        sign.registerAsWatcher( &tableau );

        List<PiecewiseLinearCaseSplit> splits = sign.getCaseSplits();
        TS_ASSERT_EQUALS( splits.size(), 2U );

        MockConstraintBoundTightener tightener;
        sign.registerConstraintBoundTightener( &tightener );
        sign.notifyLowerBound( 1, 0 );
        TS_ASSERT_THROWS_EQUALS( splits = sign.getCaseSplits(),
                                 const MarabouError &e,
                                 e.getCode(),
                                 MarabouError::REQUESTED_CASE_SPLITS_FROM_FIXED_CONSTRAINT );

        sign.unregisterAsWatcher( &tableau );

        sign = SignConstraint( b, f );
        sign.registerConstraintBoundTightener( &tightener );
        sign.registerAsWatcher( &tableau );

        splits = sign.getCaseSplits();
        TS_ASSERT_EQUALS( splits.size(), 2U );

        sign.notifyLowerBound( 4, -0.5 );
        TS_ASSERT_THROWS_EQUALS( splits = sign.getCaseSplits(),
                                 const MarabouError &e,
                                 e.getCode(),
                                 MarabouError::REQUESTED_CASE_SPLITS_FROM_FIXED_CONSTRAINT );

        sign.unregisterAsWatcher( &tableau );
    }

    void test_case_splits_after_fixed_to_negative()
    {
        unsigned b = 1;
        unsigned f = 4;

        MockTableau tableau;

        SignConstraint sign( b, f );

        sign.registerAsWatcher( &tableau );
        MockConstraintBoundTightener tightener;
        sign.registerConstraintBoundTightener( &tightener );

        List<PiecewiseLinearCaseSplit> splits = sign.getCaseSplits();
        TS_ASSERT_EQUALS( splits.size(), 2U );

        sign.notifyUpperBound( 4, 0.5 );
        TS_ASSERT_THROWS_EQUALS( splits = sign.getCaseSplits(),
                                 const MarabouError &e,
                                 e.getCode(),
                                 MarabouError::REQUESTED_CASE_SPLITS_FROM_FIXED_CONSTRAINT );

        sign.unregisterAsWatcher( &tableau );

        sign = SignConstraint( b, f );

        sign.registerAsWatcher( &tableau );
        sign.registerConstraintBoundTightener( &tightener );
        splits = sign.getCaseSplits();
        TS_ASSERT_EQUALS( splits.size(), 2U );

        sign.notifyUpperBound( 1, -0.5 );
        TS_ASSERT_THROWS_EQUALS( splits = sign.getCaseSplits(),
                                 const MarabouError &e,
                                 e.getCode(),
                                 MarabouError::REQUESTED_CASE_SPLITS_FROM_FIXED_CONSTRAINT );

        sign.unregisterAsWatcher( &tableau );
    }

    void test_constraint_phase_gets_fixed()
    {
        unsigned b = 1;
        unsigned f = 4;

        MockTableau tableau;
        MockConstraintBoundTightener tightener;

        // Upper bounds
        {
            SignConstraint sign( b, f );
            sign.registerConstraintBoundTightener( &tightener );
            TS_ASSERT( !sign.phaseFixed() );
            sign.notifyUpperBound( b, -0.1 );
            TS_ASSERT( sign.phaseFixed() );
        }

        {
            SignConstraint sign( b, f );
            sign.registerConstraintBoundTightener( &tightener );
            TS_ASSERT( !sign.phaseFixed() );
            sign.notifyUpperBound( b, -0.001 );
            TS_ASSERT( sign.phaseFixed() );
        }

        {
            SignConstraint sign( b, f );
            sign.registerConstraintBoundTightener( &tightener );
            TS_ASSERT( !sign.phaseFixed() );
            sign.notifyUpperBound( f, 1 );
            TS_ASSERT( !sign.phaseFixed() );
        }

        {
            SignConstraint sign( b, f );
            sign.registerConstraintBoundTightener( &tightener );
            TS_ASSERT( !sign.phaseFixed() );
            sign.notifyUpperBound( f, 0.5 );
            TS_ASSERT( sign.phaseFixed() );
        }

        {
            SignConstraint sign( b, f );
            sign.registerConstraintBoundTightener( &tightener );
            TS_ASSERT( !sign.phaseFixed() );
            sign.notifyUpperBound( b, 3.0 );
            TS_ASSERT( !sign.phaseFixed() );
        }

        {
            SignConstraint sign( b, f );
            sign.registerConstraintBoundTightener( &tightener );
            TS_ASSERT( !sign.phaseFixed() );
            sign.notifyUpperBound( b, 0 );
            TS_ASSERT( !sign.phaseFixed() );
        }

        // Lower bounds
        {
            SignConstraint sign( b, f );
            sign.registerConstraintBoundTightener( &tightener );
            TS_ASSERT( !sign.phaseFixed() );
            sign.notifyLowerBound( b, -0.1 );
            TS_ASSERT( !sign.phaseFixed() );
        }

        {
            SignConstraint sign( b, f );
            sign.registerConstraintBoundTightener( &tightener );
            TS_ASSERT( !sign.phaseFixed() );
            sign.notifyLowerBound( b, 0 );
            TS_ASSERT( sign.phaseFixed() );
        }

        {
            SignConstraint sign( b, f );
            sign.registerConstraintBoundTightener( &tightener );
            TS_ASSERT( !sign.phaseFixed() );
            sign.notifyLowerBound( f, -1 );
            TS_ASSERT( !sign.phaseFixed() );
        }

        {
            SignConstraint sign( b, f );
            sign.registerConstraintBoundTightener( &tightener );
            TS_ASSERT( !sign.phaseFixed() );
            sign.notifyLowerBound( f, -0.6 );
            TS_ASSERT( sign.phaseFixed() );
        }

        {
            SignConstraint sign( b, f );
            sign.registerConstraintBoundTightener( &tightener );
            TS_ASSERT( !sign.phaseFixed() );
            sign.notifyLowerBound( b, 0.0 );
            TS_ASSERT( sign.phaseFixed() );
        }

        {
            SignConstraint sign( b, f );
            sign.registerConstraintBoundTightener( &tightener );
            TS_ASSERT( !sign.phaseFixed() );
            sign.notifyLowerBound( f, 6.0 );
            TS_ASSERT( sign.phaseFixed() );
        }

        {
            SignConstraint sign( b, f );
            sign.registerConstraintBoundTightener( &tightener );
            TS_ASSERT( !sign.phaseFixed() );
            sign.notifyLowerBound( f, 0.0 );
            TS_ASSERT( sign.phaseFixed() );
        }

        {
            SignConstraint sign( b, f );
            sign.registerConstraintBoundTightener( &tightener );
            TS_ASSERT( !sign.phaseFixed() );
            sign.notifyLowerBound( b, -2.0 );
            TS_ASSERT( !sign.phaseFixed() );
        }
    }

    void test_valid_split_sign_phase_fixed_to_positive()
    {
        unsigned b = 1;
        unsigned f = 4;

        SignConstraint sign( b, f );
        MockConstraintBoundTightener tightener;
        sign.registerConstraintBoundTightener( &tightener );

        List<PiecewiseLinearConstraint::Fix> fixes;
        List<PiecewiseLinearConstraint::Fix>::iterator it;

        TS_ASSERT( !sign.phaseFixed() );
        TS_ASSERT_THROWS_NOTHING( sign.notifyLowerBound( b, -0.5 ) );
        TS_ASSERT( !sign.phaseFixed() );

        TS_ASSERT_THROWS_NOTHING( sign.notifyLowerBound( b, 0 ) );
        TS_ASSERT( sign.phaseFixed() );

        PiecewiseLinearCaseSplit split;
        TS_ASSERT_THROWS_NOTHING( split = sign.getValidCaseSplit() );

        Equation activeEquation;

        List<Tightening> bounds = split.getBoundTightenings();

        TS_ASSERT_EQUALS( bounds.size(), 2U );
        auto bound = bounds.begin();
        Tightening bound1 = *bound;

        TS_ASSERT_EQUALS( bound1._variable, b );
        TS_ASSERT_EQUALS( bound1._type, Tightening::LB );
        TS_ASSERT_EQUALS( bound1._value, 0.0 );

        ++bound;
        Tightening bound2 = *bound;

        TS_ASSERT_EQUALS( bound2._variable, f );
        TS_ASSERT_EQUALS( bound2._value, 1 );
        TS_ASSERT_EQUALS( bound2._type, Tightening::LB );

        auto equations = split.getEquations();
        TS_ASSERT( equations.empty() );
    }

    void test_valid_split_sign_phase_fixed_to_negative()
    {
        unsigned b = 1;
        unsigned f = 4;

        SignConstraint sign( b, f );

        MockConstraintBoundTightener tightener;
        sign.registerConstraintBoundTightener( &tightener );

        List<PiecewiseLinearConstraint::Fix> fixes;
        List<PiecewiseLinearConstraint::Fix>::iterator it;

        TS_ASSERT( !sign.phaseFixed() );

        TS_ASSERT_THROWS_NOTHING( sign.notifyUpperBound( b, 0.5 ) );
        TS_ASSERT( !sign.phaseFixed() );

        TS_ASSERT_THROWS_NOTHING( sign.notifyUpperBound( b, -2 ) );
        TS_ASSERT( sign.phaseFixed() );

        PiecewiseLinearCaseSplit split;
        TS_ASSERT_THROWS_NOTHING( split = sign.getValidCaseSplit() );

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
        TS_ASSERT_EQUALS( bound2._value, -1 );
        TS_ASSERT_EQUALS( bound2._type, Tightening::UB );

        auto equations = split.getEquations();
        TS_ASSERT( equations.empty() );
    }

    void test_sign_duplicate_and_restore()
    {
        SignConstraint *sign1 = new SignConstraint( 4, 6 );
        MockConstraintBoundTightener tightener;
        sign1->registerConstraintBoundTightener( &tightener );
        sign1->setActiveConstraint( false );
        sign1->notifyVariableValue( 4, 1.0 );
        sign1->notifyVariableValue( 6, 1.0 );

        sign1->notifyLowerBound( 4, -8.0 );
        sign1->notifyUpperBound( 4, 8.0 );

        sign1->notifyLowerBound( 6, -1 );
        sign1->notifyUpperBound( 6, 1 );

        PiecewiseLinearConstraint *sign2 = sign1->duplicateConstraint();

        sign1->notifyVariableValue( 4, -2 );
        // f != sign(b)
        TS_ASSERT( !sign1->satisfied() );

        sign1->notifyVariableValue( 6, -1 );
        // f = sign(b)
        TS_ASSERT( sign1->satisfied() );

        sign1->notifyVariableValue( 6, 0.5 );
        // f != sign(b)
        TS_ASSERT( !sign1->satisfied() );

        TS_ASSERT( !sign2->isActive() );
        TS_ASSERT( sign2->satisfied() );

        sign2->restoreState( sign1 );
        TS_ASSERT( !sign2->satisfied() );

        TS_ASSERT_THROWS_NOTHING( delete sign1 );
        TS_ASSERT_THROWS_NOTHING( delete sign2 );
    }

    void test_eliminate_variable_active()
    {
        unsigned b = 1;
        unsigned f = 4;

        MockTableau tableau;

        SignConstraint sign( b, f );

        sign.registerAsWatcher( &tableau );

        TS_ASSERT( !sign.constraintObsolete() );
        TS_ASSERT_THROWS_NOTHING( sign.eliminateVariable( b, 5 ) );
        TS_ASSERT( sign.constraintObsolete() );
    }

    void test_sign_entailed_tightenings()
    {
        unsigned b = 1;
        unsigned f = 4;

        InputQuery dontCare;
        dontCare.setNumberOfVariables( 500 );

        SignConstraint sign( b, f );

        MockConstraintBoundTightener tightener;
        sign.registerConstraintBoundTightener( &tightener );

        sign.notifyLowerBound( b, -1 );
        sign.notifyUpperBound( b, 7 );

        sign.notifyLowerBound( f, -1 );
        sign.notifyUpperBound( f, 1 );

        List<Tightening> entailedTightenings;
        sign.getEntailedTightenings( entailedTightenings );

        // no phase fixed - only 2 trivial tightening -1<=f, f<=1
        TS_ASSERT_EQUALS( entailedTightenings.size(), 2U );
        TS_ASSERT( entailedTightenings.exists( Tightening( f, 1, Tightening::UB ) ) );
        TS_ASSERT( entailedTightenings.exists( Tightening( f, -1, Tightening::LB ) ) );

        entailedTightenings.clear();

        sign.notifyLowerBound( b, -1 );
        // the most important test
        sign.notifyUpperBound( b, -0.5 );

        sign.notifyLowerBound( f, -1 );
        sign.notifyUpperBound( f, 1 );

        sign.getEntailedTightenings( entailedTightenings );

        // negative phase - because of b
        TS_ASSERT_EQUALS( entailedTightenings.size(), 4U );
        TS_ASSERT( entailedTightenings.exists( Tightening( f, 1, Tightening::UB ) ) );
        TS_ASSERT( entailedTightenings.exists( Tightening( f, -1, Tightening::LB ) ) );
        TS_ASSERT( entailedTightenings.exists( Tightening( f, -1, Tightening::UB ) ) );
        TS_ASSERT( entailedTightenings.exists( Tightening( b, 0, Tightening::UB ) ) );

        entailedTightenings.clear();

        sign.notifyLowerBound( b, -1 );
        sign.notifyUpperBound( b, -0.5 );
        sign.notifyLowerBound( f, -1 );
        // the most important test
        sign.notifyUpperBound( f, 0.5 );

        sign.getEntailedTightenings( entailedTightenings );

        // negative phase - because of f
        TS_ASSERT_EQUALS( entailedTightenings.size(), 4U );
        TS_ASSERT( entailedTightenings.exists( Tightening( f, 1, Tightening::UB) )  );
        TS_ASSERT( entailedTightenings.exists( Tightening( f, -1, Tightening::LB ) ) );
        TS_ASSERT( entailedTightenings.exists( Tightening( f, -1, Tightening::UB ) ) );
        TS_ASSERT( entailedTightenings.exists( Tightening( b, 0, Tightening::UB ) ) );

        entailedTightenings.clear();

        sign.notifyLowerBound( b, 0 );
        sign.notifyUpperBound( b, 7 );
        sign.notifyLowerBound( f, -1 );
        sign.notifyUpperBound( f, 1 );

        sign.getEntailedTightenings( entailedTightenings );

        // positive phase - because of b
        TS_ASSERT_EQUALS( entailedTightenings.size(), 4U );
        TS_ASSERT( entailedTightenings.exists( Tightening( f, 1, Tightening::UB ) ) );
        TS_ASSERT( entailedTightenings.exists( Tightening( f, -1, Tightening::LB ) ) );
        TS_ASSERT( entailedTightenings.exists( Tightening( f, 1, Tightening::LB ) ) );
        TS_ASSERT( entailedTightenings.exists( Tightening( b, 0, Tightening::LB ) ) );

        entailedTightenings.clear();

        sign.notifyLowerBound( b, -5 );
        sign.notifyUpperBound( b, 5 );
        sign.notifyLowerBound( f, -0.5 );
        sign.notifyUpperBound( f, 1 );

        sign.getEntailedTightenings( entailedTightenings );

        // positive phase - because of f
        TS_ASSERT_EQUALS( entailedTightenings.size(), 4U );
        TS_ASSERT( entailedTightenings.exists( Tightening( f, 1, Tightening::UB ) ) );
        TS_ASSERT( entailedTightenings.exists( Tightening( f, -1, Tightening::LB ) ) );
        TS_ASSERT( entailedTightenings.exists( Tightening( f, 1, Tightening::LB ) ) );
        TS_ASSERT( entailedTightenings.exists( Tightening( b, 0, Tightening::LB ) ) );

        entailedTightenings.clear();

        unsigned b2 = 1;
        unsigned f2 = 4;

        InputQuery dontCare2;
        dontCare.setNumberOfVariables( 500 );

        SignConstraint sign2( b2, f2 );

        sign2.registerConstraintBoundTightener( &tightener );

        sign2.notifyLowerBound( b2, -1 );
        sign2.notifyUpperBound( b2, 1 );

        sign2.notifyLowerBound( f2, -1 );
        sign2.notifyUpperBound( f2, 1 );

        List<Tightening> entailedTightenings2;
        sign2.getEntailedTightenings( entailedTightenings2 );

        // no phase fixed - only 2 trivial tightening -1<=f, f<=1
        TS_ASSERT_EQUALS( entailedTightenings2.size(), 2U );
        TS_ASSERT( entailedTightenings2.exists( Tightening( f2, 1, Tightening::UB ) ) );
        TS_ASSERT( entailedTightenings2.exists( Tightening( f2, -1, Tightening::LB ) ) );

        entailedTightenings2.clear();

        // new case
        sign2.notifyUpperBound( b, 0 );

        sign2.getEntailedTightenings( entailedTightenings2 );

        // no phase fixed - only 2 trivial tightening -1<=f, f<=1
        TS_ASSERT_EQUALS( entailedTightenings2.size(), 2U );
        TS_ASSERT( entailedTightenings2.exists( Tightening( f2, 1, Tightening::UB ) ) );
        TS_ASSERT( entailedTightenings2.exists( Tightening( f2, -1, Tightening::LB ) ) );

        entailedTightenings2.clear();
    }

    SignConstraint prepareSign( unsigned b, unsigned f, IConstraintBoundTightener *tightener )
    {
        SignConstraint sign( b, f );

        sign.registerConstraintBoundTightener( tightener );

        InputQuery dontCare;

        sign.notifyLowerBound( b, -10 );
        sign.notifyUpperBound( b, 15 );

        sign.notifyLowerBound( f, -1 );
        sign.notifyUpperBound( f, 1 );

        sign.registerConstraintBoundTightener( tightener );

        return sign;
    }

    void test_notify_bounds()
    {
        unsigned b = 1;
        unsigned f = 4;

        MockConstraintBoundTightener tightener;
        List<Tightening> tightenings;

        tightener.getConstraintTightenings( tightenings );

        SignConstraint sign = prepareSign( b, f, &tightener );

        sign.notifyLowerBound( b, -5 );
        sign.notifyUpperBound( b, 5 );

        {
            sign.notifyLowerBound( b, -5 );
            tightener.getConstraintTightenings( tightenings );
            TS_ASSERT( tightenings.empty() );

            sign.notifyLowerBound( b, -7 );
            tightener.getConstraintTightenings( tightenings );
            TS_ASSERT( tightenings.empty() );

            sign.notifyLowerBound( f, -3 );
            tightener.getConstraintTightenings( tightenings );
            TS_ASSERT( tightenings.empty() );

            sign.notifyUpperBound( b, 20 );
            tightener.getConstraintTightenings( tightenings );
            TS_ASSERT( tightenings.empty() );

            sign.notifyUpperBound( f, 23 );
            tightener.getConstraintTightenings( tightenings );
            TS_ASSERT( tightenings.empty() );

            sign.notifyLowerBound( f, -1 );
            tightener.getConstraintTightenings( tightenings );
            TS_ASSERT( tightenings.empty() );

            sign.notifyUpperBound( f, 1 );
            tightener.getConstraintTightenings( tightenings );
            TS_ASSERT( tightenings.empty() );

            // although higher lower bound - then bounds are reported only if phase is fixed!
            sign.notifyLowerBound( b, -2 );
            tightener.getConstraintTightenings( tightenings );
            TS_ASSERT( tightenings.empty() );
        }

        {
            // Tighter lower bound for b/f that is positive
            SignConstraint sign = prepareSign( b, f, &tightener );
            sign.notifyLowerBound( b, 1 );
            tightener.getConstraintTightenings( tightenings );
            TS_ASSERT_EQUALS( tightenings.size(), 1U);
            TS_ASSERT( tightenings.exists( Tightening( f, 1, Tightening::LB ) ) );

            sign.notifyUpperBound( f, -0.5 );
            tightener.getConstraintTightenings( tightenings );
            TS_ASSERT( tightenings.exists( Tightening( f, -1, Tightening::UB ) ) );
        }

        {
            // Tighter upper bound 0 for f
            SignConstraint sign = prepareSign( b, f, &tightener );
            sign.notifyUpperBound( f, 0 );
            tightener.getConstraintTightenings( tightenings );
            TS_ASSERT_EQUALS(tightenings.size(), 2U);
            TS_ASSERT( tightenings.exists( Tightening( b, 0, Tightening::UB ) ) );
            TS_ASSERT( tightenings.exists( Tightening( f, -1, Tightening::UB ) ) );
        }

        {
            // upper bound 0 for b is inconclusive - because for 0 its +1, for <0 its '-1'
            SignConstraint sign = prepareSign( b, f, &tightener );
            sign.notifyUpperBound( b, 0 );
            tightener.getConstraintTightenings( tightenings );
            TS_ASSERT( tightenings.empty() );
        }

        {
            // lower bound 0 for b is '+1'
            SignConstraint sign = prepareSign( b, f, &tightener );
            sign.notifyLowerBound( b, 0 );
            tightener.getConstraintTightenings( tightenings );
            TS_ASSERT_EQUALS( tightenings.size(), 1U );
            TS_ASSERT( tightenings.exists( Tightening( f, 1, Tightening::LB ) ) );
        }

        {
            // Tighter negative upper bound for b
            SignConstraint sign = prepareSign( b, f, &tightener );
            sign.notifyUpperBound( f, 0.5 );
            tightener.getConstraintTightenings( tightenings );
            TS_ASSERT_EQUALS(tightenings.size(), 2U);
            TS_ASSERT( tightenings.exists( Tightening( f, -1, Tightening::UB ) ) );
            TS_ASSERT( tightenings.exists( Tightening( b, 0, Tightening::UB ) ) );
        }
    }

    void test_serialize_and_unserialize()
    {
        unsigned b = 42;
        unsigned f = 7;

        SignConstraint originalSign( b, f );
        originalSign.notifyLowerBound( b, -10 );
        originalSign.notifyUpperBound( f, 3 );

        String originalSerialized = originalSign.serializeToString();
        SignConstraint recoveredSign( originalSerialized );

        TS_ASSERT_EQUALS( originalSign.serializeToString(),
                          recoveredSign.serializeToString() );
    }

      void test_polarity()
    {
        unsigned b = 1;
        unsigned f = 4;

        // b in [1, 2], polarity should be 1, and direction should be SIGN_PHASE_POSITIVE
        {
            SignConstraint sign( b, f );
            sign.notifyLowerBound( b, 1 );
            sign.notifyUpperBound( b, 2 );
            TS_ASSERT( sign.computePolarity() == 1 );

            sign.updateDirection();
            TS_ASSERT( sign.getDirection() == SIGN_PHASE_POSITIVE );
        }
        // b in [-2, 0], polarity should be -1, and direction should be SIGN_PHASE_NEGATIVE
        {
            SignConstraint sign( b, f );
            sign.notifyLowerBound( b, -2 );
            sign.notifyUpperBound( b, 0 );
            TS_ASSERT( sign.computePolarity() == -1 );

            sign.updateDirection();
            TS_ASSERT( sign.getDirection() == SIGN_PHASE_NEGATIVE );
        }
        // b in [-2, 2], polarity should be 0, the direction should be SIGN_PHASE_NEGATIVE,
        // the inactive case should be the first element of the returned list by
        // the getCaseSplits()
        {
            SignConstraint sign( b, f );
            sign.notifyLowerBound( b, -2 );
            sign.notifyUpperBound( b, 2 );
            TS_ASSERT( sign.computePolarity() == 0 );

            sign.updateDirection();
            TS_ASSERT( sign.getDirection() == SIGN_PHASE_POSITIVE );

            auto splits = sign.getCaseSplits();
            auto it = splits.begin();
            TS_ASSERT( isPositiveSplit( b, f, it ) );
        }
        // b in [-2, 3], polarity should be 0.2, the direction should be SIGN_PHASE_POSITIVE,
        // the active case should be the first element of the returned list by
        // the getCaseSplits()
        {
            SignConstraint sign( b, f );
            sign.notifyLowerBound( b, -2 );
            sign.notifyUpperBound( b, 3 );
            TS_ASSERT( sign.computePolarity() == 0.2 );

            sign.updateDirection();
            TS_ASSERT( sign.getDirection() == SIGN_PHASE_POSITIVE );

            auto splits = sign.getCaseSplits();
            auto it = splits.begin();
            TS_ASSERT( isPositiveSplit( b, f, it ) );
        }
    }
};
