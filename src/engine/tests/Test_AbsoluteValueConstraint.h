/*********************                                                        */
/*! \file Test_AbsoluteValueConstraint.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Shiran Aziz, Aleksandar Zeljic, Andrew Wu
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

**/

#include <cxxtest/TestSuite.h>

#include "AbsoluteValueConstraint.h"
#include "InputQuery.h"
#include "MarabouError.h"
#include "MockErrno.h"
#include "MockTableau.h"
#include "PiecewiseLinearCaseSplit.h"
#include <iostream>
#include <string.h>

class MockForAbsoluteValueConstraint : public MockErrno
{
public:
};

/*
   Exposes protected members of AbsConstraint for testing.
 */
class TestAbsConstraint : public AbsoluteValueConstraint
{
public:
    TestAbsConstraint( unsigned b, unsigned f )
        : AbsoluteValueConstraint( b, f )
    {}

    using AbsoluteValueConstraint::getPhaseStatus;
};

using namespace CVC4::context;

class AbsoluteValueConstraintTestSuite : public CxxTest::TestSuite
{
public:
    MockForAbsoluteValueConstraint *mock;

    void setUp()
    {
        TS_ASSERT( mock = new MockForAbsoluteValueConstraint );
    }

    void tearDown()
    {
        TS_ASSERT_THROWS_NOTHING( delete mock );
    }

    void test_abs_duplicate_and_restore()
    {
        AbsoluteValueConstraint abs1( 4, 6 );
        MockTableau tableau;
        abs1.registerTableau( &tableau );

        abs1.setActiveConstraint( false );
        tableau.setValue( 4, 1.0 );
        tableau.setValue( 6, 1.0 );

        abs1.notifyLowerBound( 4, -8.0 );
        abs1.notifyUpperBound( 4, 8.0 );

        abs1.notifyLowerBound( 6, 0.0 );
        abs1.notifyUpperBound( 6, 8.0 );

        PiecewiseLinearConstraint *abs2 = abs1.duplicateConstraint();

        tableau.setValue( 4, -2 );
        TS_ASSERT( !abs1.satisfied() );
        TS_ASSERT( !abs1.isActive() );
        TS_ASSERT( !abs2->isActive() );
        TS_ASSERT( !abs2->satisfied() );

        abs2->restoreState( &abs1 );
        TS_ASSERT( !abs2->isActive() );
        TS_ASSERT( !abs2->satisfied() );

        abs1.setActiveConstraint( true );
        TS_ASSERT( abs1.isActive() );
        TS_ASSERT( !abs2->isActive() );

        TS_ASSERT_THROWS_NOTHING( delete abs2 );
    }

    void test_register_and_unregister_as_watcher()
    {
        unsigned b = 1;
        unsigned f = 4;

        MockTableau tableau;

        AbsoluteValueConstraint abs( b, f );

        TS_ASSERT_THROWS_NOTHING( abs.registerAsWatcher( &tableau ) );

        TS_ASSERT_EQUALS( tableau.lastRegisteredVariableToWatcher.size(), 2U );
        TS_ASSERT( tableau.lastUnregisteredVariableToWatcher.empty() );
        TS_ASSERT_EQUALS( tableau.lastRegisteredVariableToWatcher[b].size(), 1U );
        TS_ASSERT( tableau.lastRegisteredVariableToWatcher[b].exists( &abs ) );
        TS_ASSERT_EQUALS( tableau.lastRegisteredVariableToWatcher[f].size(), 1U );
        TS_ASSERT( tableau.lastRegisteredVariableToWatcher[f].exists( &abs ) );

        tableau.lastRegisteredVariableToWatcher.clear();

        TS_ASSERT_THROWS_NOTHING( abs.unregisterAsWatcher( &tableau ) );

        TS_ASSERT( tableau.lastRegisteredVariableToWatcher.empty() );
        TS_ASSERT_EQUALS( tableau.lastUnregisteredVariableToWatcher.size(), 2U );
        TS_ASSERT_EQUALS( tableau.lastUnregisteredVariableToWatcher[b].size(), 1U );
        TS_ASSERT( tableau.lastUnregisteredVariableToWatcher[b].exists( &abs ) );
        TS_ASSERT_EQUALS( tableau.lastUnregisteredVariableToWatcher[f].size(), 1U );
        TS_ASSERT( tableau.lastUnregisteredVariableToWatcher[f].exists( &abs ) );
    }

    void test_participating_variables()
    {
        unsigned b = 1;
        unsigned f = 4;

        AbsoluteValueConstraint abs( b, f );

        List<unsigned> participatingVariables;

        TS_ASSERT_THROWS_NOTHING( participatingVariables = abs.getParticipatingVariables() );
        TS_ASSERT_EQUALS( participatingVariables.size(), 2U );
        auto it = participatingVariables.begin();
        TS_ASSERT_EQUALS( *it, b );
        ++it;
        TS_ASSERT_EQUALS( *it, f );

        TS_ASSERT_EQUALS( abs.getParticipatingVariables(), List<unsigned>( { 1, 4 } ) );

        TS_ASSERT( abs.participatingVariable( b ) );
        TS_ASSERT( abs.participatingVariable( f ) );
        TS_ASSERT( !abs.participatingVariable( 2 ) );
        TS_ASSERT( !abs.participatingVariable( 0 ) );
        TS_ASSERT( !abs.participatingVariable( 5 ) );
    }

    void test_abs_notify_variable_value_and_satisfied()
    {
        unsigned b = 1;
        unsigned f = 4;

        AbsoluteValueConstraint abs( b, f );
        MockTableau tableau;
        abs.registerTableau( &tableau );

        tableau.setValue( b, 5 );
        tableau.setValue( f, 5 );
        TS_ASSERT( abs.satisfied() );

        tableau.setValue( b, -5 );
        tableau.setValue( f, 5 );
        TS_ASSERT( abs.satisfied() );

        tableau.setValue( b, 5 );
        tableau.setValue( f, -5 );
        TS_ASSERT( !abs.satisfied() );

        tableau.setValue( f, -1 );
        tableau.setValue( b, -5 );
        TS_ASSERT( !abs.satisfied() );

        tableau.setValue( b, 1 );
        TS_ASSERT( !abs.satisfied() );

        tableau.setValue( b, 5 );
        tableau.setValue( f, 4 );
        TS_ASSERT( !abs.satisfied() );

        tableau.setValue( b, -4 );
        tableau.setValue( f, -5 );
        TS_ASSERT( !abs.satisfied() );

        tableau.setValue( f, 1 );
        tableau.setValue( b, -2 );
        TS_ASSERT( !abs.satisfied() );

        tableau.setValue( f, -1 );
        tableau.setValue( b, -2 );
        TS_ASSERT( !abs.satisfied() );
    }

    void test_abs_update_variable_index()
    {
        unsigned b = 1;
        unsigned f = 4;

        AbsoluteValueConstraint abs( b, f );
        MockTableau tableau;
        abs.registerTableau( &tableau );

        // Changing variable indices
        tableau.setValue( b, 1 );
        tableau.setValue( f, 1 );
        TS_ASSERT( abs.satisfied() );

        unsigned newB = 12;
        unsigned newF = 14;

        TS_ASSERT_THROWS_NOTHING( abs.updateVariableIndex( b, newB ) );
        TS_ASSERT_THROWS_NOTHING( abs.updateVariableIndex( f, newF ) );

        tableau.setValue( newB, 1 );
        tableau.setValue( newF, 1 );

        TS_ASSERT( abs.satisfied() );

        tableau.setValue( newF, 2 );

        TS_ASSERT( !abs.satisfied() );

        tableau.setValue( newB, 2 );

        TS_ASSERT( abs.satisfied() );
    }

    void test_abs_getPossibleFixes()
    {
        // Possible violations:
        //   1. f is positive, b is positive, b and f are unequal
        //   2. f is positive, b is negative, -b and f are unequal

        unsigned b = 1;
        unsigned f = 4;

        AbsoluteValueConstraint abs( b, f );
        MockTableau tableau;
        abs.registerTableau( &tableau );

        List<PiecewiseLinearConstraint::Fix> fixes;
        List<PiecewiseLinearConstraint::Fix>::iterator it;

        //   1. f is positive, b is positive, b and f are unequal

        tableau.setValue( b, 2 );
        tableau.setValue( f, 1 );

        fixes = abs.getPossibleFixes();
        TS_ASSERT_EQUALS( fixes.size(), 3U );
        it = fixes.begin();
        TS_ASSERT_EQUALS( it->_variable, b );
        TS_ASSERT_EQUALS( it->_value, 1 );
        ++it;
        TS_ASSERT_EQUALS( it->_variable, b );
        TS_ASSERT_EQUALS( it->_value, -1 );
        ++it;
        TS_ASSERT_EQUALS( it->_variable, f );
        TS_ASSERT_EQUALS( it->_value, 2 );

        //   2. f is positive, b is negative, -b and f are unequal
        tableau.setValue( b, -2 );
        tableau.setValue( f, 1 );

        fixes = abs.getPossibleFixes();
        TS_ASSERT_EQUALS( fixes.size(), 3U );
        it = fixes.begin();
        TS_ASSERT_EQUALS( it->_variable, b );
        TS_ASSERT_EQUALS( it->_value, 1 );
        ++it;
        TS_ASSERT_EQUALS( it->_variable, b );
        TS_ASSERT_EQUALS( it->_value, -1 );
        ++it;
        TS_ASSERT_EQUALS( it->_variable, f );
        TS_ASSERT_EQUALS( it->_value, 2 );
    }

    void test_eliminate_variable_b()
    {
        unsigned b = 1;
        unsigned f = 4;

        MockTableau tableau;

        AbsoluteValueConstraint abs( b, f );

        abs.registerAsWatcher( &tableau );

        TS_ASSERT( !abs.constraintObsolete() );
        TS_ASSERT_THROWS_NOTHING( abs.eliminateVariable( b, 5 ) );
        TS_ASSERT( abs.constraintObsolete() );
    }

    void test_eliminate_variable_f()
    {
        unsigned b = 1;
        unsigned f = 4;

        MockTableau tableau;

        AbsoluteValueConstraint abs( b, f );

        abs.registerAsWatcher( &tableau );

        TS_ASSERT( !abs.constraintObsolete() );
        TS_ASSERT_THROWS_NOTHING( abs.eliminateVariable( f, 5 ) );
        TS_ASSERT( abs.constraintObsolete() );
    }

    void test_abs_entailed_tightenings_positive_phase_1()
    {
        {
            unsigned b = 1;
            unsigned f = 4;

            AbsoluteValueConstraint abs( b, f );
            List<Tightening> entailedTightenings;

            abs.notifyLowerBound( b, 1 );
            abs.notifyLowerBound( f, 2 );
            abs.notifyUpperBound( b, 7 );
            abs.notifyUpperBound( f, 7 );

            // 1 < x_b < 7 , 2 < x_f < 7 -> 2 < x_b
            abs.getEntailedTightenings( entailedTightenings );
            assert_lower_upper_bound( f, b, 1, 2, 7, 7, entailedTightenings );

            abs.notifyLowerBound( b, 3 );
            // 3 < x_b < 7 , 2 < x_f < 7
            entailedTightenings.clear();
            abs.getEntailedTightenings( entailedTightenings );
            assert_lower_upper_bound( f, b, 3, 2, 7, 7, entailedTightenings );

            abs.notifyLowerBound( f, 3 );
            abs.notifyUpperBound( b, 6 );
            // 3 < x_b < 6 , 3 < x_f < 7
            entailedTightenings.clear();
            abs.getEntailedTightenings( entailedTightenings );
            assert_lower_upper_bound( f, b, 3, 3, 6, 7, entailedTightenings );

            abs.notifyUpperBound( f, 6 );
            abs.notifyUpperBound( b, 7 );
            // 3 < x_b < 6 , 3 < x_f < 6
            //  --> x_b < 6
            entailedTightenings.clear();
            abs.getEntailedTightenings( entailedTightenings );
            assert_lower_upper_bound( f, b, 3, 3, 6, 6, entailedTightenings );

            abs.notifyLowerBound( f, -3 );
            // 3 < x_b < 6 , 3 < x_f < 6
            // --> 3 < x_f
            entailedTightenings.clear();
            abs.getEntailedTightenings( entailedTightenings );
            assert_lower_upper_bound( f, b, 3, 3, 6, 6, entailedTightenings );

            abs.notifyLowerBound( b, 5 );
            abs.notifyUpperBound( f, 5 );
            // 5 < x_b < 6 , 3 < x_f < 5
            entailedTightenings.clear();
            abs.getEntailedTightenings( entailedTightenings );
            assert_lower_upper_bound( f, b, 5, 3, 6, 5, entailedTightenings );
        }

        {   // With BoundManager
            unsigned b = 1;
            unsigned f = 4;

            AbsoluteValueConstraint abs( b, f );
            Context context;
            BoundManager boundManager( context );
            boundManager.initialize( 5 );
            abs.registerBoundManager( &boundManager );

            List<Tightening> entailedTightenings;

            abs.notifyLowerBound( b, 1 );
            abs.notifyLowerBound( f, 2 );
            abs.notifyUpperBound( b, 7 );
            abs.notifyUpperBound( f, 7 );

            // 1 < x_b < 7 , 2 < x_f < 7 -> 2 < x_b
            abs.getEntailedTightenings( entailedTightenings );
            assert_lower_upper_bound( f, b, 1, 2, 7, 7, entailedTightenings );

            abs.notifyLowerBound( b, 3 );
            // 3 < x_b < 7 , 2 < x_f < 7
            entailedTightenings.clear();
            abs.getEntailedTightenings( entailedTightenings );
            assert_lower_upper_bound( f, b, 3, 2, 7, 7, entailedTightenings );

            abs.notifyLowerBound( f, 3 );
            abs.notifyUpperBound( b, 6 );
            // 3 < x_b < 6 , 3 < x_f < 7
            entailedTightenings.clear();
            abs.getEntailedTightenings( entailedTightenings );
            assert_lower_upper_bound( f, b, 3, 3, 6, 7, entailedTightenings );

            abs.notifyUpperBound( f, 6 );
            abs.notifyUpperBound( b, 7 );
            // 3 < x_b < 6 , 3 < x_f < 6
            //  --> x_b < 6
            entailedTightenings.clear();
            abs.getEntailedTightenings( entailedTightenings );
            assert_lower_upper_bound( f, b, 3, 3, 6, 6, entailedTightenings );

            abs.notifyLowerBound( f, -3 );
            // 3 < x_b < 6 , 3 < x_f < 6
            // --> 3 < x_f
            entailedTightenings.clear();
            abs.getEntailedTightenings( entailedTightenings );
            assert_lower_upper_bound( f, b, 3, 3, 6, 6, entailedTightenings );

            abs.notifyLowerBound( b, 5 );
            abs.notifyUpperBound( f, 5 );
            // 5 < x_b < 6 , 3 < x_f < 5
            entailedTightenings.clear();
            abs.getEntailedTightenings( entailedTightenings );
            assert_lower_upper_bound( f, b, 5, 3, 6, 5, entailedTightenings );
        }
    }

    void test_abs_entailed_tightenings_positive_phase_2()
    {
        {
            unsigned b = 1;
            unsigned f = 4;

            AbsoluteValueConstraint abs( b, f );
            List<Tightening> entailedTightenings;

            // 8 < b < 18, 48 < f < 64
            abs.notifyUpperBound( b, 18 );
            abs.notifyUpperBound( f, 64 );
            abs.notifyLowerBound( b, 8 );
            abs.notifyLowerBound( f, 48 );
            abs.getEntailedTightenings( entailedTightenings );
            assert_lower_upper_bound( f, b, 8, 48, 18, 64, entailedTightenings );
        }

        {   //With BoundManage
            unsigned b = 1;
            unsigned f = 4;

            AbsoluteValueConstraint abs( b, f );
            Context context;
            BoundManager boundManager( context );
            boundManager.initialize( 5 );

            abs.registerBoundManager( &boundManager );

            List<Tightening> entailedTightenings;

            // 8 < b < 18, 48 < f < 64
            abs.notifyUpperBound( b, 18 );
            abs.notifyUpperBound( f, 64 );
            abs.notifyLowerBound( b, 8 );
            abs.notifyLowerBound( f, 48 );
            abs.getEntailedTightenings( entailedTightenings );
            assert_lower_upper_bound( f, b, 8, 48, 18, 64, entailedTightenings );
        }
    }

    void test_abs_entailed_tightenings_positive_phase_3()
    {
        {
            unsigned b = 1;
            unsigned f = 4;

            AbsoluteValueConstraint abs( b, f );
            List<Tightening> entailedTightenings;

            // 3 < b < 4, 1 < f < 2
            abs.notifyUpperBound( b, 4 );
            abs.notifyUpperBound( f, 2 );
            abs.notifyLowerBound( b, 3 );
            abs.notifyLowerBound( f, 1 );
            abs.getEntailedTightenings( entailedTightenings );
            assert_lower_upper_bound( f, b, 3, 1, 4, 2, entailedTightenings );
        }

        {   // With Bound Manager
            unsigned b = 1;
            unsigned f = 4;

            AbsoluteValueConstraint abs( b, f );
            Context context;
            BoundManager boundManager( context );
            boundManager.initialize( 5 );

            abs.registerBoundManager( &boundManager );

            List<Tightening> entailedTightenings;

            // 3 < b < 4, 1 < f < 2
            abs.notifyUpperBound( b, 4 );
            abs.notifyUpperBound( f, 2 );
            abs.notifyLowerBound( b, 3 );
            abs.notifyLowerBound( f, 1 );
            abs.getEntailedTightenings( entailedTightenings );
            assert_lower_upper_bound( f, b, 3, 1, 4, 2, entailedTightenings );
        }
    }

    void test_abs_entailed_tightenings_positive_phase_4()
    {
        {
            unsigned b = 1;
            unsigned f = 4;

            AbsoluteValueConstraint abs( b, f );
            List<Tightening> entailedTightenings;
            List<Tightening>::iterator it;

            abs.notifyUpperBound( b, 7 );
            abs.notifyUpperBound( f, 6 );
            abs.notifyLowerBound( b, 0 );
            abs.notifyLowerBound( f, 0 );

            // 0 < x_b < 7 ,0 < x_f < 6
            abs.getEntailedTightenings( entailedTightenings );
            assert_lower_upper_bound( f, b, 0, 0, 7, 6, entailedTightenings );

            abs.notifyUpperBound( b, 5 );
            // 0 < x_b < 5 ,0 < x_f < 6
            entailedTightenings.clear();
            abs.getEntailedTightenings( entailedTightenings );
            assert_lower_upper_bound( f, b, 0, 0, 5, 6, entailedTightenings );

            abs.notifyLowerBound( b, 1 );
            // 1 < x_b < 5 ,0 < x_f < 6
            entailedTightenings.clear();
            abs.getEntailedTightenings( entailedTightenings );
            assert_lower_upper_bound( f, b, 1, 0, 5, 6, entailedTightenings );

            abs.notifyUpperBound( f, 4 );
            // 1 < x_b < 5 ,0 < x_f < 4
            entailedTightenings.clear();
            abs.getEntailedTightenings( entailedTightenings );
            assert_lower_upper_bound( f, b, 1, 0, 5, 4, entailedTightenings );

            // Non overlap
            abs.notifyUpperBound( f, 2 );
            abs.notifyLowerBound( b, 3 );

            // 3 < x_b < 5 ,0 < x_f < 2
            entailedTightenings.clear();
            abs.getEntailedTightenings( entailedTightenings );
            TS_ASSERT_EQUALS( entailedTightenings.size(), 4U );
            assert_lower_upper_bound( f, b, 3, 0, 5, 2, entailedTightenings );
        }

        {   // With Bound Manager
            unsigned b = 1;
            unsigned f = 4;

            AbsoluteValueConstraint abs( b, f );
            Context context;
            BoundManager boundManager( context );
            boundManager.initialize( 5 );
            abs.registerBoundManager( &boundManager );

            List<Tightening> entailedTightenings;
            List<Tightening>::iterator it;

            abs.notifyUpperBound( b, 7 );
            abs.notifyUpperBound( f, 6 );
            abs.notifyLowerBound( b, 0 );
            abs.notifyLowerBound( f, 0 );

            // 0 < x_b < 7 ,0 < x_f < 6
            abs.getEntailedTightenings( entailedTightenings );
            assert_lower_upper_bound( f, b, 0, 0, 7, 6, entailedTightenings );

            abs.notifyUpperBound( b, 5 );
            // 0 < x_b < 5 ,0 < x_f < 6
            entailedTightenings.clear();
            abs.getEntailedTightenings( entailedTightenings );
            assert_lower_upper_bound( f, b, 0, 0, 5, 6, entailedTightenings );

            abs.notifyLowerBound( b, 1 );
            // 1 < x_b < 5 ,0 < x_f < 6
            entailedTightenings.clear();
            abs.getEntailedTightenings( entailedTightenings );
            assert_lower_upper_bound( f, b, 1, 0, 5, 6, entailedTightenings );

            abs.notifyUpperBound( f, 4 );
            // 1 < x_b < 5 ,0 < x_f < 4
            entailedTightenings.clear();
            abs.getEntailedTightenings( entailedTightenings );
            assert_lower_upper_bound( f, b, 1, 0, 5, 4, entailedTightenings );

            // Non overlap
            abs.notifyUpperBound( f, 2 );
            abs.notifyLowerBound( b, 3 );

            // 3 < x_b < 5 ,0 < x_f < 2
            entailedTightenings.clear();
            abs.getEntailedTightenings( entailedTightenings );
            TS_ASSERT_EQUALS( entailedTightenings.size(), 4U );
            assert_lower_upper_bound( f, b, 3, 0, 5, 2, entailedTightenings );
        }
    }

    void test_abs_entailed_tightenings_positive_phase_5()
    {
        {
            unsigned b = 1;
            unsigned f = 4;

            AbsoluteValueConstraint abs( b, f );
            List<Tightening> entailedTightenings;

            abs.notifyUpperBound( b, 6 );
            abs.notifyUpperBound( f, 5 );
            abs.notifyLowerBound( b, 4 );
            abs.notifyLowerBound( f, 3 );

            // 4 < x_b < 6 ,3 < x_f < 5
            entailedTightenings.clear();
            abs.getEntailedTightenings( entailedTightenings );
            assert_lower_upper_bound( f, b, 4, 3, 6, 5, entailedTightenings );
        }

        {   // With Bound Manager
            unsigned b = 1;
            unsigned f = 4;

            AbsoluteValueConstraint abs( b, f );
            Context context;
            BoundManager boundManager( context );
            boundManager.initialize( 5 );
            abs.registerBoundManager( &boundManager );

            List<Tightening> entailedTightenings;

            abs.notifyUpperBound( b, 6 );
            abs.notifyUpperBound( f, 5 );
            abs.notifyLowerBound( b, 4 );
            abs.notifyLowerBound( f, 3 );

            // 4 < x_b < 6 ,3 < x_f < 5
            entailedTightenings.clear();
            abs.getEntailedTightenings( entailedTightenings );
            assert_lower_upper_bound( f, b, 4, 3, 6, 5, entailedTightenings );
        }
    }

    void test_abs_entailed_tightenings_positive_phase_6()
    {
        {
            unsigned b = 1;
            unsigned f = 4;

            AbsoluteValueConstraint abs( b, f );
            List<Tightening> entailedTightenings;
            List<Tightening>::iterator it;

            abs.notifyLowerBound( b, 5 );
            abs.notifyUpperBound( b, 10 );
            abs.notifyLowerBound( f, -1 );
            abs.notifyUpperBound( f, 10 );

            TS_ASSERT( abs.phaseFixed() );
            abs.getEntailedTightenings( entailedTightenings );

            assert_tightenings_match( entailedTightenings,
                                      List<Tightening>( {
                                          Tightening( f, 0, Tightening::LB ),

                                          Tightening( b, 0, Tightening::LB ),
                                          Tightening( b, 10, Tightening::UB ),
                                          Tightening( f, 5, Tightening::LB ),
                                          Tightening( f, 10, Tightening::UB ),
                                      } ) );
        }

        {   // With Bound Manager
            unsigned b = 1;
            unsigned f = 4;

            AbsoluteValueConstraint abs( b, f );
            Context context;
            BoundManager boundManager( context );
            boundManager.initialize( 5 );
            abs.registerBoundManager( &boundManager );

            List<Tightening> entailedTightenings;
            List<Tightening>::iterator it;

            abs.notifyLowerBound( b, 5 );
            abs.notifyUpperBound( b, 10 );
            abs.notifyLowerBound( f, -1 );
            abs.notifyUpperBound( f, 10 );

            TS_ASSERT( abs.phaseFixed() );
            abs.getEntailedTightenings( entailedTightenings );

            assert_tightenings_match( entailedTightenings,
                                      List<Tightening>( {
                                          Tightening( f, 0, Tightening::LB ),

                                          Tightening( b, 0, Tightening::LB ),
                                          Tightening( b, 10, Tightening::UB ),
                                          Tightening( f, 5, Tightening::LB ),
                                          Tightening( f, 10, Tightening::UB ),
                                      } ) );
        }
    }

    void test_abs_entailed_tightenings_phase_not_fixed_f_strictly_positive()
    {
        {
            unsigned b = 1;
            unsigned f = 4;

            AbsoluteValueConstraint abs( b, f );
            List<Tightening> entailedTightenings;
            List<Tightening>::iterator it;

            abs.notifyLowerBound( b, -6 );
            abs.notifyUpperBound( b, 3 );
            abs.notifyLowerBound( f, 2 );
            abs.notifyUpperBound( f, 4 );

            // -6 < x_b < 3 ,2 < x_f < 4
            abs.getEntailedTightenings( entailedTightenings );

            assert_tightenings_match( entailedTightenings,
                                      List<Tightening>( {
                                          Tightening( b, -4, Tightening::LB ),
                                          Tightening( b, 4, Tightening::UB ),
                                          Tightening( f, 6, Tightening::UB ),
                                      } ) );

            entailedTightenings.clear();

            // -6 < x_b < 2 ,2 < x_f < 4
            abs.notifyUpperBound( b, 2 );
            abs.getEntailedTightenings( entailedTightenings );

            assert_tightenings_match( entailedTightenings,
                                      List<Tightening>( {
                                          Tightening( b, -4, Tightening::LB ),
                                          Tightening( b, 4, Tightening::UB ),
                                          Tightening( f, 6, Tightening::UB ),
                                      } ) );

            entailedTightenings.clear();

            // -6 < x_b < 1 ,2 < x_f < 4, now stuck in negative phase
            abs.notifyUpperBound( b, 1 );
            abs.getEntailedTightenings( entailedTightenings );

            assert_tightenings_match( entailedTightenings,
                                      List<Tightening>( {
                                          Tightening( b, -4, Tightening::LB ),
                                          Tightening( b, 4, Tightening::UB ),
                                          Tightening( f, 6, Tightening::UB ),
                                          Tightening( b, -2, Tightening::UB ),
                                      } ) );
        }

        { // With Bound Manager
            unsigned b = 1;
            unsigned f = 4;

            AbsoluteValueConstraint abs( b, f );
            Context context;
            BoundManager boundManager( context );
            boundManager.initialize( 5 );
            abs.registerBoundManager( &boundManager );

            List<Tightening> entailedTightenings;
            List<Tightening>::iterator it;

            abs.notifyLowerBound( b, -6 );
            abs.notifyUpperBound( b, 3 );
            abs.notifyLowerBound( f, 2 );
            abs.notifyUpperBound( f, 4 );

            // -6 < x_b < 3 ,2 < x_f < 4
            abs.getEntailedTightenings( entailedTightenings );

            assert_tightenings_match( entailedTightenings,
                                      List<Tightening>( {
                                          Tightening( b, -4, Tightening::LB ),
                                          Tightening( b, 4, Tightening::UB ),
                                          Tightening( f, 6, Tightening::UB ),
                                      } ) );

            entailedTightenings.clear();

            // -6 < x_b < 2 ,2 < x_f < 4
            abs.notifyUpperBound( b, 2 );
            abs.getEntailedTightenings( entailedTightenings );

            assert_tightenings_match( entailedTightenings,
                                      List<Tightening>( {
                                          Tightening( b, -4, Tightening::LB ),
                                          Tightening( b, 4, Tightening::UB ),
                                          Tightening( f, 6, Tightening::UB ),
                                      } ) );

            entailedTightenings.clear();

            // -6 < x_b < 1 ,2 < x_f < 4, now stuck in negative phase
            abs.notifyUpperBound( b, 1 );
            abs.getEntailedTightenings( entailedTightenings );

            assert_tightenings_match( entailedTightenings,
                                      List<Tightening>( {
                                          Tightening( b, -4, Tightening::LB ),
                                          Tightening( b, 4, Tightening::UB ),
                                          Tightening( f, 6, Tightening::UB ),
                                          Tightening( b, -2, Tightening::UB ),
                                      } ) );
        }
    }

    void test_abs_entailed_tightenings_phase_not_fixed_f_strictly_positive_2()
    {
        {
            unsigned b = 1;
            unsigned f = 4;

            AbsoluteValueConstraint abs( b, f );
            List<Tightening> entailedTightenings;
            List<Tightening>::iterator it;

            abs.notifyLowerBound( b, -5 );
            abs.notifyUpperBound( b, 10 );
            abs.notifyLowerBound( f, 3 );
            abs.notifyUpperBound( f, 7 );

            // -5 < x_b < 10 ,3 < x_f < 7
            abs.getEntailedTightenings( entailedTightenings );
            assert_tightenings_match( entailedTightenings,
                                      List<Tightening>( {
                                          Tightening( b, -7, Tightening::LB ),
                                          Tightening( b, 7, Tightening::UB ),
                                          Tightening( f, 10, Tightening::UB ),
                                      } ) );

            entailedTightenings.clear();

            // -5 < x_b < 10 ,6 < x_f < 7, positive phase
            abs.notifyLowerBound( f, 6 );

            abs.getEntailedTightenings( entailedTightenings );
            assert_tightenings_match( entailedTightenings,
                                      List<Tightening>( {
                                          Tightening( b, -7, Tightening::LB ),
                                          Tightening( b, 7, Tightening::UB ),
                                          Tightening( f, 10, Tightening::UB ),
                                          Tightening( b, 6, Tightening::LB ),
                                      } ) );

            entailedTightenings.clear();

            // -5 < x_b < 3 ,6 < x_f < 7

            // Extreme case, disjoint ranges

            abs.notifyUpperBound( b, 3 );

            abs.getEntailedTightenings( entailedTightenings );
            assert_tightenings_match( entailedTightenings,
                                      List<Tightening>( {
                                          Tightening( b, -7, Tightening::LB ),
                                          Tightening( b, 7, Tightening::UB ),
                                          Tightening( f, 5, Tightening::UB ),
                                          Tightening( b, 6, Tightening::LB ),
                                          Tightening( b, -6, Tightening::UB ),
                                      } ) );
        }

        { // With Bound Manager
            unsigned b = 1;
            unsigned f = 4;

            AbsoluteValueConstraint abs( b, f );
            Context context;
            BoundManager boundManager( context );
            boundManager.initialize( 5 );
            abs.registerBoundManager( &boundManager );

            List<Tightening> entailedTightenings;
            List<Tightening>::iterator it;

            abs.notifyLowerBound( b, -5 );
            abs.notifyUpperBound( b, 10 );
            abs.notifyLowerBound( f, 3 );
            abs.notifyUpperBound( f, 7 );

            // -5 < x_b < 10 ,3 < x_f < 7
            abs.getEntailedTightenings( entailedTightenings );
            assert_tightenings_match( entailedTightenings,
                                      List<Tightening>( {
                                          Tightening( b, -7, Tightening::LB ),
                                          Tightening( b, 7, Tightening::UB ),
                                          Tightening( f, 10, Tightening::UB ),
                                      } ) );

            entailedTightenings.clear();

            // -5 < x_b < 10 ,6 < x_f < 7, positive phase
            abs.notifyLowerBound( f, 6 );

            abs.getEntailedTightenings( entailedTightenings );
            assert_tightenings_match( entailedTightenings,
                                      List<Tightening>( {
                                          Tightening( b, -7, Tightening::LB ),
                                          Tightening( b, 7, Tightening::UB ),
                                          Tightening( f, 10, Tightening::UB ),
                                          Tightening( b, 6, Tightening::LB ),
                                      } ) );

            entailedTightenings.clear();

            // -5 < x_b < 3 ,6 < x_f < 7

            // Extreme case, disjoint ranges

            abs.notifyUpperBound( b, 3 );

            abs.getEntailedTightenings( entailedTightenings );
            assert_tightenings_match( entailedTightenings,
                                      List<Tightening>( {
                                          Tightening( b, -7, Tightening::LB ),
                                          Tightening( b, 7, Tightening::UB ),
                                          Tightening( f, 5, Tightening::UB ),
                                          Tightening( b, 6, Tightening::LB ),
                                          Tightening( b, -6, Tightening::UB ),
                                      } ) );
        }
    }

    void test_abs_entailed_tightenings_phase_not_fixed_f_strictly_positive_3()
    {
        {
            unsigned b = 1;
            unsigned f = 4;

            AbsoluteValueConstraint abs( b, f );
            List<Tightening> entailedTightenings;
            List<Tightening>::iterator it;

            abs.notifyLowerBound( b, -1 );
            abs.notifyUpperBound( b, 1 );
            abs.notifyLowerBound( f, 2 );
            abs.notifyUpperBound( f, 4 );

            // -1 < x_b < 1 ,2 < x_f < 4
            abs.getEntailedTightenings( entailedTightenings );
            assert_tightenings_match( entailedTightenings,
                                      List<Tightening>( {
                                          Tightening( b, -4, Tightening::LB ),
                                          Tightening( b, 4, Tightening::UB ),
                                          Tightening( f, 1, Tightening::UB ),
                                          Tightening( b, 2, Tightening::LB ),
                                          Tightening( b, -2, Tightening::UB ),
                                      } ) );
        }

        { // With Bound Manager
            unsigned b = 1;
            unsigned f = 4;

            AbsoluteValueConstraint abs( b, f );
            Context context;
            BoundManager boundManager( context );
            boundManager.initialize( 5 );
            abs.registerBoundManager( &boundManager );

            List<Tightening> entailedTightenings;
            List<Tightening>::iterator it;

            abs.notifyLowerBound( b, -1 );
            abs.notifyUpperBound( b, 1 );
            abs.notifyLowerBound( f, 2 );
            abs.notifyUpperBound( f, 4 );

            // -1 < x_b < 1 ,2 < x_f < 4
            abs.getEntailedTightenings( entailedTightenings );
            assert_tightenings_match( entailedTightenings,
                                      List<Tightening>( {
                                          Tightening( b, -4, Tightening::LB ),
                                          Tightening( b, 4, Tightening::UB ),
                                          Tightening( f, 1, Tightening::UB ),
                                          Tightening( b, 2, Tightening::LB ),
                                          Tightening( b, -2, Tightening::UB ),
                                      } ) );
        }
    }

    void test_abs_entailed_tightenings_phase_not_fixed_f_non_negative()
    {
        {
            unsigned b = 1;
            unsigned f = 4;

            AbsoluteValueConstraint abs( b, f );
            List<Tightening> entailedTightenings;
            List<Tightening>::iterator it;

            abs.notifyLowerBound( b, -7 );
            abs.notifyUpperBound( b, 7 );
            abs.notifyLowerBound( f, 0 );
            abs.notifyUpperBound( f, 6 );

            // -7 < x_b < 7 ,0 < x_f < 6
            abs.getEntailedTightenings( entailedTightenings );
            assert_tightenings_match( entailedTightenings,
                                      List<Tightening>( {
                                          Tightening( b, -6, Tightening::LB ),
                                          Tightening( b, 6, Tightening::UB ),
                                          Tightening( f, 7, Tightening::UB ),
                                      } ) );


            entailedTightenings.clear();

            // -7 < x_b < 5 ,0 < x_f < 6
            abs.notifyUpperBound( b, 5 );
            abs.getEntailedTightenings( entailedTightenings );
            assert_tightenings_match( entailedTightenings,
                                      List<Tightening>( {
                                          Tightening( b, -6, Tightening::LB ),
                                          Tightening( b, 6, Tightening::UB ),
                                          Tightening( f, 7, Tightening::UB ),
                                      } ) );

            entailedTightenings.clear();
            // 0 < x_b < 5 ,0 < x_f < 6
            abs.notifyLowerBound( b, 0 );
            abs.getEntailedTightenings( entailedTightenings );
            assert_tightenings_match( entailedTightenings,
                                      List<Tightening>( {
                                          Tightening( b, 0, Tightening::LB ),
                                          Tightening( b, 6, Tightening::UB ),
                                          Tightening( f, 0, Tightening::LB ),
                                          Tightening( f, 5, Tightening::UB ),
                                      } ) );


            entailedTightenings.clear();

            // 3 < x_b < 5 ,0 < x_f < 6
            abs.notifyLowerBound( b, 3 );
            abs.getEntailedTightenings( entailedTightenings );
            assert_tightenings_match( entailedTightenings,
                                      List<Tightening>( {
                                          Tightening( b, 0, Tightening::LB ),
                                          Tightening( b, 6, Tightening::UB ),
                                          Tightening( f, 3, Tightening::LB ),
                                          Tightening( f, 5, Tightening::UB ),
                                      } ) );
        }
        {
            unsigned b = 1;
            unsigned f = 4;

            AbsoluteValueConstraint abs( b, f );
            List<Tightening> entailedTightenings;
            List<Tightening>::iterator it;

            abs.notifyLowerBound( b, -7 );
            abs.notifyUpperBound( b, 7 );
            abs.notifyLowerBound( f, 0 );
            abs.notifyUpperBound( f, 6 );

            // -7 < x_b < 7 ,0 < x_f < 6
            abs.getEntailedTightenings( entailedTightenings );
            assert_tightenings_match( entailedTightenings,
                                      List<Tightening>( {
                                          Tightening( b, -6, Tightening::LB ),
                                          Tightening( b, 6, Tightening::UB ),
                                          Tightening( f, 7, Tightening::UB ),
                                      } ) );


            entailedTightenings.clear();

            // -7 < x_b < 5 ,0 < x_f < 6
            abs.notifyUpperBound( b, 5 );
            abs.getEntailedTightenings( entailedTightenings );
            assert_tightenings_match( entailedTightenings,
                                      List<Tightening>( {
                                          Tightening( b, -6, Tightening::LB ),
                                          Tightening( b, 6, Tightening::UB ),
                                          Tightening( f, 7, Tightening::UB ),
                                      } ) );

            entailedTightenings.clear();
            // 0 < x_b < 5 ,0 < x_f < 6
            abs.notifyLowerBound( b, 0 );
            abs.getEntailedTightenings( entailedTightenings );
            assert_tightenings_match( entailedTightenings,
                                      List<Tightening>( {
                                          Tightening( b, 0, Tightening::LB ),
                                          Tightening( b, 6, Tightening::UB ),
                                          Tightening( f, 0, Tightening::LB ),
                                          Tightening( f, 5, Tightening::UB ),
                                      } ) );


            entailedTightenings.clear();

            // 3 < x_b < 5 ,0 < x_f < 6
            abs.notifyLowerBound( b, 3 );
            abs.getEntailedTightenings( entailedTightenings );
            assert_tightenings_match( entailedTightenings,
                                      List<Tightening>( {
                                          Tightening( b, 0, Tightening::LB ),
                                          Tightening( b, 6, Tightening::UB ),
                                          Tightening( f, 3, Tightening::LB ),
                                          Tightening( f, 5, Tightening::UB ),
                                      } ) );
        }
        { // With Bound Manager
            unsigned b = 1;
            unsigned f = 4;

            AbsoluteValueConstraint abs( b, f );
            Context context;
            BoundManager boundManager( context );
            boundManager.initialize( 5 );
            abs.registerBoundManager( &boundManager );

            List<Tightening> entailedTightenings;
            List<Tightening>::iterator it;

            abs.notifyLowerBound( b, -7 );
            abs.notifyUpperBound( b, 7 );
            abs.notifyLowerBound( f, 0 );
            abs.notifyUpperBound( f, 6 );

            // -7 < x_b < 7 ,0 < x_f < 6
            abs.getEntailedTightenings( entailedTightenings );
            assert_tightenings_match( entailedTightenings,
                                      List<Tightening>( {
                                          Tightening( b, -6, Tightening::LB ),
                                          Tightening( b, 6, Tightening::UB ),
                                          Tightening( f, 7, Tightening::UB ),
                                      } ) );


            entailedTightenings.clear();

            // -7 < x_b < 5 ,0 < x_f < 6
            abs.notifyUpperBound( b, 5 );
            abs.getEntailedTightenings( entailedTightenings );
            assert_tightenings_match( entailedTightenings,
                                      List<Tightening>( {
                                          Tightening( b, -6, Tightening::LB ),
                                          Tightening( b, 6, Tightening::UB ),
                                          Tightening( f, 7, Tightening::UB ),
                                      } ) );

            entailedTightenings.clear();
            // 0 < x_b < 5 ,0 < x_f < 6
            abs.notifyLowerBound( b, 0 );
            abs.getEntailedTightenings( entailedTightenings );
            assert_tightenings_match( entailedTightenings,
                                      List<Tightening>( {
                                          Tightening( b, 0, Tightening::LB ),
                                          Tightening( b, 6, Tightening::UB ),
                                          Tightening( f, 0, Tightening::LB ),
                                          Tightening( f, 5, Tightening::UB ),
                                      } ) );


            entailedTightenings.clear();

            // 3 < x_b < 5 ,0 < x_f < 6
            abs.notifyLowerBound( b, 3 );
            abs.getEntailedTightenings( entailedTightenings );
            assert_tightenings_match( entailedTightenings,
                                      List<Tightening>( {
                                          Tightening( b, 0, Tightening::LB ),
                                          Tightening( b, 6, Tightening::UB ),
                                          Tightening( f, 3, Tightening::LB ),
                                          Tightening( f, 5, Tightening::UB ),
                                      } ) );
        }
    }

    void test_abs_entailed_tightenings_negative_phase()
    {
        {
            unsigned b = 1;
            unsigned f = 4;

            AbsoluteValueConstraint abs( b, f );
            List<Tightening> entailedTightenings;
            List<Tightening>::iterator it;

            abs.notifyLowerBound( b, -20 );
            abs.notifyUpperBound( b, -2 );
            abs.notifyLowerBound( f, 0 );
            abs.notifyUpperBound( f, 15 );

            // -20 < x_b < -2 ,0 < x_f < 15
            abs.getEntailedTightenings( entailedTightenings );
            assert_tightenings_match( entailedTightenings,
                                      List<Tightening>( {
                                          Tightening( b, -15, Tightening::LB ),
                                          Tightening( b, 0, Tightening::UB ),
                                          Tightening( f, 2, Tightening::LB ),
                                          Tightening( f, 20, Tightening::UB ),
                                      } ) );


            entailedTightenings.clear();

            // -20 < x_b < -2 ,7 < x_f < 15
            abs.notifyLowerBound( f, 7 );
            abs.getEntailedTightenings( entailedTightenings );
            assert_tightenings_match( entailedTightenings,
                                      List<Tightening>( {
                                          Tightening( b, -15, Tightening::LB ),
                                          Tightening( b, -7, Tightening::UB ),
                                          Tightening( f, 2, Tightening::LB ),
                                          Tightening( f, 20, Tightening::UB ),
                                      } ) );

            entailedTightenings.clear();

            // -12 < x_b < -2 ,7 < x_f < 15
            abs.notifyLowerBound( b, -12 );
            abs.getEntailedTightenings( entailedTightenings );
            assert_tightenings_match( entailedTightenings,
                                      List<Tightening>( {
                                          Tightening( b, -15, Tightening::LB ),
                                          Tightening( b, -7, Tightening::UB ),
                                          Tightening( f, 2, Tightening::LB ),
                                          Tightening( f, 12, Tightening::UB ),
                                      } ) );

            entailedTightenings.clear();

            // -12 < x_b < -8 ,7 < x_f < 15
            abs.notifyUpperBound( b, -8 );
            abs.getEntailedTightenings( entailedTightenings );
            assert_tightenings_match( entailedTightenings,
                                      List<Tightening>( {
                                          Tightening( b, -15, Tightening::LB ),
                                          Tightening( b, -7, Tightening::UB ),
                                          Tightening( f, 8, Tightening::LB ),
                                          Tightening( f, 12, Tightening::UB ),
                                      } ) );
        }
        {   // With Bound Manager
            unsigned b = 1;
            unsigned f = 4;

            AbsoluteValueConstraint abs( b, f );
            Context context;
            BoundManager boundManager( context );
            boundManager.initialize( 5 );
            abs.registerBoundManager( &boundManager );

            List<Tightening> entailedTightenings;
            List<Tightening>::iterator it;

            abs.notifyLowerBound( b, -20 );
            abs.notifyUpperBound( b, -2 );
            abs.notifyLowerBound( f, 0 );
            abs.notifyUpperBound( f, 15 );

            // -20 < x_b < -2 ,0 < x_f < 15
            abs.getEntailedTightenings( entailedTightenings );
            assert_tightenings_match( entailedTightenings,
                                      List<Tightening>( {
                                          Tightening( b, -15, Tightening::LB ),
                                          Tightening( b, 0, Tightening::UB ),
                                          Tightening( f, 2, Tightening::LB ),
                                          Tightening( f, 20, Tightening::UB ),
                                      } ) );


            entailedTightenings.clear();

            // -20 < x_b < -2 ,7 < x_f < 15
            abs.notifyLowerBound( f, 7 );
            abs.getEntailedTightenings( entailedTightenings );
            assert_tightenings_match( entailedTightenings,
                                      List<Tightening>( {
                                          Tightening( b, -15, Tightening::LB ),
                                          Tightening( b, -7, Tightening::UB ),
                                          Tightening( f, 2, Tightening::LB ),
                                          Tightening( f, 20, Tightening::UB ),
                                      } ) );

            entailedTightenings.clear();

            // -12 < x_b < -2 ,7 < x_f < 15
            abs.notifyLowerBound( b, -12 );
            abs.getEntailedTightenings( entailedTightenings );
            assert_tightenings_match( entailedTightenings,
                                      List<Tightening>( {
                                          Tightening( b, -15, Tightening::LB ),
                                          Tightening( b, -7, Tightening::UB ),
                                          Tightening( f, 2, Tightening::LB ),
                                          Tightening( f, 12, Tightening::UB ),
                                      } ) );

            entailedTightenings.clear();

            // -12 < x_b < -8 ,7 < x_f < 15
            abs.notifyUpperBound( b, -8 );
            abs.getEntailedTightenings( entailedTightenings );
            assert_tightenings_match( entailedTightenings,
                                      List<Tightening>( {
                                          Tightening( b, -15, Tightening::LB ),
                                          Tightening( b, -7, Tightening::UB ),
                                          Tightening( f, 8, Tightening::LB ),
                                          Tightening( f, 12, Tightening::UB ),
                                      } ) );
        }
    }

    void test_abs_entailed_tightenings_negative_phase_2()
    {
        {
            unsigned b = 1;
            unsigned f = 4;

            AbsoluteValueConstraint abs( b, f );
            List<Tightening> entailedTightenings;
            List<Tightening>::iterator it;

            abs.notifyLowerBound( b, -20 );
            abs.notifyUpperBound( b, -2 );
            abs.notifyLowerBound( f, 25 );
            abs.notifyUpperBound( f, 30 );

            // -20 < x_b < -2 ,25 < x_f < 30
            abs.getEntailedTightenings( entailedTightenings );
            assert_tightenings_match( entailedTightenings,
                                      List<Tightening>( {
                                          Tightening( b, -30, Tightening::LB ),
                                          Tightening( b, -25, Tightening::UB ),
                                          Tightening( f, 2, Tightening::LB ),
                                          Tightening( f, 20, Tightening::UB ),
                                      } ) );
        }

        {   // With Bound Manager
            unsigned b = 1;
            unsigned f = 4;

            AbsoluteValueConstraint abs( b, f );
            Context context;
            BoundManager boundManager( context );
            boundManager.initialize( 5 );
            abs.registerBoundManager( &boundManager );

            List<Tightening> entailedTightenings;
            List<Tightening>::iterator it;

            abs.notifyLowerBound( b, -20 );
            abs.notifyUpperBound( b, -2 );
            abs.notifyLowerBound( f, 25 );
            abs.notifyUpperBound( f, 30 );

            // -20 < x_b < -2 ,25 < x_f < 30
            abs.getEntailedTightenings( entailedTightenings );
            assert_tightenings_match( entailedTightenings,
                                      List<Tightening>( {
                                          Tightening( b, -30, Tightening::LB ),
                                          Tightening( b, -25, Tightening::UB ),
                                          Tightening( f, 2, Tightening::LB ),
                                          Tightening( f, 20, Tightening::UB ),
                                      } ) );
        }
    }

    void test_abs_case_splits()
    {
        unsigned b = 1;
        unsigned f = 4;

        AbsoluteValueConstraint abs( b, f );

        List<PiecewiseLinearConstraint::Fix> fixes;
        List<PiecewiseLinearConstraint::Fix>::iterator it;

        List<PiecewiseLinearCaseSplit> splits = abs.getCaseSplits();

        Equation positiveEquation, negativeEquation;

        TS_ASSERT_EQUALS( splits.size(), 2U );

        List<PiecewiseLinearCaseSplit>::iterator split1 = splits.begin();
        List<PiecewiseLinearCaseSplit>::iterator split2 = split1;
        ++split2;

        TS_ASSERT( isPositiveSplit( b, f, split1 ) || isPositiveSplit( b, f, split2 ) );
        TS_ASSERT( isNegativeSplit( b, f, split1 ) || isNegativeSplit( b, f, split2 ) );
    }

    bool isPositiveSplit( unsigned b, unsigned f, List<PiecewiseLinearCaseSplit>::iterator &split )
    {
        List<Tightening> bounds = split->getBoundTightenings();

        auto bound = bounds.begin();
        Tightening bound1 = *bound;

        TS_ASSERT_EQUALS( bound1._variable, b );
        TS_ASSERT_EQUALS( bound1._value, 0.0 );

        if ( bound1._type != Tightening::LB )
            return false;

        TS_ASSERT_EQUALS( bounds.size(), 1U );

        Equation positiveEquation;
        auto equations = split->getEquations();
        TS_ASSERT_EQUALS( equations.size(), 1U );
        positiveEquation = split->getEquations().front();
        TS_ASSERT_EQUALS( positiveEquation._addends.size(), 2U );
        TS_ASSERT_EQUALS( positiveEquation._scalar, 0.0 );

        auto addend = positiveEquation._addends.begin();
        TS_ASSERT_EQUALS( addend->_coefficient, 1.0 );
        TS_ASSERT_EQUALS( addend->_variable, b );

        ++addend;
        TS_ASSERT_EQUALS( addend->_coefficient, -1.0 );
        TS_ASSERT_EQUALS( addend->_variable, f );

        TS_ASSERT_EQUALS( positiveEquation._type, Equation::EQ );

        return true;
    }

    bool isNegativeSplit( unsigned b, unsigned f, List<PiecewiseLinearCaseSplit>::iterator &split )
    {
        List<Tightening> bounds = split->getBoundTightenings();

        auto bound = bounds.begin();
        Tightening bound1 = *bound;

        TS_ASSERT_EQUALS( bound1._variable, b );
        TS_ASSERT_EQUALS( bound1._value, 0.0 );

        if ( bound1._type != Tightening::UB )
            return false;

        Equation negativeEquation;
        auto equations = split->getEquations();
        TS_ASSERT_EQUALS( equations.size(), 1U );
        negativeEquation = split->getEquations().front();
        TS_ASSERT_EQUALS( negativeEquation._addends.size(), 2U );
        TS_ASSERT_EQUALS( negativeEquation._scalar, 0.0 );

        auto addend = negativeEquation._addends.begin();
        TS_ASSERT_EQUALS( addend->_coefficient, 1.0 );
        TS_ASSERT_EQUALS( addend->_variable, b );

        ++addend;
        TS_ASSERT_EQUALS( addend->_coefficient, 1.0 );
        TS_ASSERT_EQUALS( addend->_variable, f );

        TS_ASSERT_EQUALS( negativeEquation._type, Equation::EQ );
        return true;
    }

    void test_constraint_phase_gets_fixed()
    {
        unsigned b = 1;
        unsigned f = 4;

        MockTableau tableau;

        // Upper bounds
        {
            AbsoluteValueConstraint abs( b, f );
            TS_ASSERT( !abs.phaseFixed() );
            abs.notifyUpperBound( b, -1.0 );
            TS_ASSERT( abs.phaseFixed() );
        }

        {
            AbsoluteValueConstraint abs( b, f );
            TS_ASSERT( !abs.phaseFixed() );
            abs.notifyUpperBound( b, 0.0 );
            TS_ASSERT( abs.phaseFixed() );
        }

        {
            AbsoluteValueConstraint abs( b, f );
            TS_ASSERT( !abs.phaseFixed() );
            abs.notifyUpperBound( f, 5 );
            TS_ASSERT( !abs.phaseFixed() );
        }

        {
            AbsoluteValueConstraint abs( b, f );
            TS_ASSERT( !abs.phaseFixed() );
            abs.notifyUpperBound( b, 3.0 );
            TS_ASSERT( !abs.phaseFixed() );
        }

        // Lower bounds
        {
            AbsoluteValueConstraint abs( b, f );
            TS_ASSERT( !abs.phaseFixed() );
            abs.notifyLowerBound( b, 3.0 );
            TS_ASSERT( abs.phaseFixed() );
        }

        {
            AbsoluteValueConstraint abs( b, f );
            TS_ASSERT( !abs.phaseFixed() );
            abs.notifyLowerBound( b, 0.0 );
            TS_ASSERT( abs.phaseFixed() );
        }

        {
            AbsoluteValueConstraint abs( b, f );
            TS_ASSERT( !abs.phaseFixed() );
            abs.notifyLowerBound( f, 6.0 );
            TS_ASSERT( !abs.phaseFixed() );
        }

        {
            AbsoluteValueConstraint abs( b, f );
            TS_ASSERT( !abs.phaseFixed() );
            abs.notifyLowerBound( b, -2.5 );
            TS_ASSERT( !abs.phaseFixed() );
        }
    }

    void test_valid_split_abs_phase_fixed_to_active()
    {
        unsigned b = 1;
        unsigned f = 4;

        AbsoluteValueConstraint abs( b, f );

        List<PiecewiseLinearConstraint::Fix> fixes;
        List<PiecewiseLinearConstraint::Fix>::iterator it;

        TS_ASSERT( !abs.phaseFixed() );
        TS_ASSERT_THROWS_NOTHING( abs.notifyLowerBound( b, 5 ) );
        TS_ASSERT( abs.phaseFixed() );

        PiecewiseLinearCaseSplit split;
        TS_ASSERT_THROWS_NOTHING( split = abs.getValidCaseSplit() );

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

    void test_valid_split_abs_phase_fixed_to_inactive()
    {
        unsigned b = 1;
        unsigned f = 4;

        AbsoluteValueConstraint abs( b, f );

        List<PiecewiseLinearConstraint::Fix> fixes;
        List<PiecewiseLinearConstraint::Fix>::iterator it;

        TS_ASSERT( !abs.phaseFixed() );
        TS_ASSERT_THROWS_NOTHING( abs.notifyUpperBound( b, -2 ) );
        TS_ASSERT( abs.phaseFixed() );

        PiecewiseLinearCaseSplit split;
        TS_ASSERT_THROWS_NOTHING( split = abs.getValidCaseSplit() );

        Equation activeEquation;

        List<Tightening> bounds = split.getBoundTightenings();

        TS_ASSERT_EQUALS( bounds.size(), 1U );
        auto bound = bounds.begin();
        Tightening bound1 = *bound;

        TS_ASSERT_EQUALS( bound1._variable, b );
        TS_ASSERT_EQUALS( bound1._type, Tightening::UB );
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
        TS_ASSERT_EQUALS( addend->_coefficient, 1.0 );
        TS_ASSERT_EQUALS( addend->_variable, f );

        TS_ASSERT_EQUALS( activeEquation._type, Equation::EQ );
    }

    void assert_lower_upper_bound( unsigned f,
                                   unsigned b,
                                   double fLower,
                                   double bLower,
                                   double fUpper,
                                   double bUpper,
                                   List<Tightening> entailedTightenings )
    {
        List<Tightening>::iterator it;
        it = entailedTightenings.begin();

        TS_ASSERT_EQUALS( entailedTightenings.size(), 4U );
        TS_ASSERT_EQUALS( it->_variable, f );
        TS_ASSERT_EQUALS( it->_value, fLower );
        TS_ASSERT_EQUALS( it->_type, Tightening::LB );
        it++;
        TS_ASSERT_EQUALS( it->_variable, b );
        TS_ASSERT_EQUALS( it->_value, bLower );
        TS_ASSERT_EQUALS( it->_type, Tightening::LB );
        it++;
        TS_ASSERT_EQUALS( it->_variable, f );
        TS_ASSERT_EQUALS( it->_value, fUpper );
        TS_ASSERT_EQUALS( it->_type, Tightening::UB );
        it++;
        TS_ASSERT_EQUALS( it->_variable, b );
        TS_ASSERT_EQUALS( it->_value, bUpper );
        TS_ASSERT_EQUALS( it->_type, Tightening::UB );
    }

    void assert_tightenings_match( List<Tightening> a, List<Tightening> b )
    {
        TS_ASSERT_EQUALS( a.size(), b.size() );

        for ( const auto &it : a )
            TS_ASSERT( b.exists( it ) );
    }

    void test_serialize_and_unserialize()
    {
        unsigned b = 42;
        unsigned f = 7;

        AbsoluteValueConstraint originalAbs( b, f );
        originalAbs.notifyLowerBound( b, -10 );
        originalAbs.notifyUpperBound( f, 5 );
        originalAbs.notifyUpperBound( f, 5 );

        String originalSerialized = originalAbs.serializeToString();
        AbsoluteValueConstraint recoveredAbs( originalSerialized );

        TS_ASSERT_EQUALS( originalAbs.serializeToString(),
                          recoveredAbs.serializeToString() );

        TS_ASSERT_EQUALS( originalSerialized, "absoluteValue,7,42" );
    }

    void test_abs_with_aux_vars()
    {
        {
            unsigned b = 1;
            unsigned f = 4;

            AbsoluteValueConstraint abs( b, f );

            InputQuery ipq;
            ipq.setNumberOfVariables( 10 );
            unsigned posAux = 10;
            unsigned negAux = 11;

            abs.notifyLowerBound( b, -5 );
            abs.notifyUpperBound( b, 7 );
            abs.notifyLowerBound( f, 0 );
            abs.notifyUpperBound( f, 7 );

            TS_ASSERT( !abs.participatingVariable( posAux ) );
            TS_ASSERT( !abs.participatingVariable( negAux ) );

            TS_ASSERT_THROWS_NOTHING( abs.transformToUseAuxVariables
                                      ( ipq ) );

            TS_ASSERT_EQUALS( abs.getParticipatingVariables(),
                              List<unsigned>( { b, f, posAux, negAux } ) );

            TS_ASSERT( abs.participatingVariable( posAux ) );
            TS_ASSERT( abs.participatingVariable( negAux ) );

            List<Tightening> tightenings;
            TS_ASSERT_THROWS_NOTHING( abs.getEntailedTightenings( tightenings ) );

            List<Tightening> expectedTightenings = {
                // B var, from f bounds
                Tightening( b, -7, Tightening::LB ),
                Tightening( b, 7, Tightening::UB ),

                // F var, from b bounds
                Tightening( f, 7, Tightening::UB ),

                // Aux vars
                Tightening( posAux, 0, Tightening::LB ),
                Tightening( negAux, 0, Tightening::LB ),

                Tightening( posAux, 12, Tightening::UB ),
                Tightening( negAux, 14, Tightening::UB ),
            };

            TS_ASSERT_EQUALS( expectedTightenings.size(), tightenings.size() );

            for ( const auto &it : tightenings )
                TS_ASSERT( expectedTightenings.exists( it ) );

            String serialized = abs.serializeToString();

            TS_TRACE( serialized.ascii() );

            AbsoluteValueConstraint abs2( serialized );

            TS_ASSERT( !abs2.phaseFixed() );
            abs2.notifyLowerBound( b, 2 );
            TS_ASSERT( abs2.phaseFixed() );

            PiecewiseLinearCaseSplit validSplit = abs2.getValidCaseSplit();
            TS_ASSERT( validSplit.getEquations().empty() );
            TS_ASSERT_EQUALS( validSplit.getBoundTightenings().size(), 2U );
            TS_ASSERT_EQUALS( *validSplit.getBoundTightenings().begin(),
                              Tightening( b, 0, Tightening::LB ) );
            TS_ASSERT_EQUALS( *validSplit.getBoundTightenings().rbegin(),
                              Tightening( posAux, 0, Tightening::UB ) );
        }

        {   // With Bound Manager
            unsigned b = 1;
            unsigned f = 4;

            InputQuery ipq;
            ipq.setNumberOfVariables( 10 );
            unsigned posAux = 10;
            unsigned negAux = 11;

            AbsoluteValueConstraint abs( b, f );
            Context context;
            BoundManager boundManager( context );
            boundManager.initialize( 12 );
            abs.registerBoundManager( &boundManager );

            abs.notifyLowerBound( b, -5 );
            abs.notifyUpperBound( b, 7 );
            abs.notifyLowerBound( f, 0 );
            abs.notifyUpperBound( f, 7 );

            TS_ASSERT( !abs.participatingVariable( posAux ) );
            TS_ASSERT( !abs.participatingVariable( negAux ) );

            TS_ASSERT_THROWS_NOTHING( abs.transformToUseAuxVariables( ipq ) );

            TS_ASSERT_EQUALS( abs.getParticipatingVariables(),
                              List<unsigned>( { b, f, posAux, negAux } ) );

            TS_ASSERT( abs.participatingVariable( posAux ) );
            TS_ASSERT( abs.participatingVariable( negAux ) );

            List<Tightening> tightenings;
            TS_ASSERT_THROWS_NOTHING( abs.getEntailedTightenings( tightenings ) );

            List<Tightening> expectedTightenings = {
                // B var, from f bounds
                Tightening( b, -7, Tightening::LB ),
                Tightening( b, 7, Tightening::UB ),

                // F var, from b bounds
                Tightening( f, 7, Tightening::UB ),

                // Aux vars
                Tightening( posAux, 0, Tightening::LB ),
                Tightening( negAux, 0, Tightening::LB ),

                Tightening( posAux, 12, Tightening::UB ),
                Tightening( negAux, 14, Tightening::UB ),
            };

            TS_ASSERT_EQUALS( expectedTightenings.size(), tightenings.size() );

            for ( const auto &it : tightenings )
                TS_ASSERT( expectedTightenings.exists( it ) );

            String serialized = abs.serializeToString();

            TS_TRACE( serialized.ascii() );

            AbsoluteValueConstraint abs2( serialized );

            TS_ASSERT( !abs2.phaseFixed() );
            abs2.notifyLowerBound( b, 2 );
            TS_ASSERT( abs2.phaseFixed() );

            PiecewiseLinearCaseSplit validSplit = abs2.getValidCaseSplit();
            TS_ASSERT( validSplit.getEquations().empty() );
            TS_ASSERT_EQUALS( validSplit.getBoundTightenings().size(), 2U );
            TS_ASSERT_EQUALS( *validSplit.getBoundTightenings().begin(),
                              Tightening( b, 0, Tightening::LB ) );
            TS_ASSERT_EQUALS( *validSplit.getBoundTightenings().rbegin(),
                              Tightening( posAux, 0, Tightening::UB ) );
        }
    }

    void test_initialization_of_CDOs()
    {
        Context context;
        AbsoluteValueConstraint *abs1 = new AbsoluteValueConstraint( 4, 6 );

        TS_ASSERT_EQUALS( abs1->getContext(), static_cast<Context*>( nullptr ) );
        TS_ASSERT_EQUALS( abs1->getActiveStatusCDO(), static_cast<CDO<bool>*>( nullptr ) );
        TS_ASSERT_EQUALS( abs1->getPhaseStatusCDO(), static_cast<CDO<PhaseStatus>*>( nullptr ) );
        TS_ASSERT_EQUALS( abs1->getInfeasibleCasesCDList(), static_cast<CDList<PhaseStatus>*>( nullptr ) );
        TS_ASSERT_THROWS_NOTHING( abs1->initializeCDOs( &context ) );
        TS_ASSERT_EQUALS( abs1->getContext(), &context );
        TS_ASSERT_DIFFERS( abs1->getActiveStatusCDO(), static_cast<CDO<bool>*>( nullptr ) );
        TS_ASSERT_DIFFERS( abs1->getPhaseStatusCDO(), static_cast<CDO<PhaseStatus>*>( nullptr ) );
        TS_ASSERT_DIFFERS( abs1->getInfeasibleCasesCDList(), static_cast<CDList<PhaseStatus>*>( nullptr ) );

        bool active = false;
        TS_ASSERT_THROWS_NOTHING( active = abs1->isActive() );
        TS_ASSERT_EQUALS( active, true );

        bool phaseFixed = true;
        TS_ASSERT_THROWS_NOTHING( phaseFixed = abs1->phaseFixed() );
        TS_ASSERT_EQUALS( phaseFixed, PHASE_NOT_FIXED );
        TS_ASSERT_EQUALS( abs1->numFeasibleCases(), 2u );


        TS_ASSERT_THROWS_NOTHING( delete abs1 );
    }

    /*
     * Test Case functionality of AbsoluteValueConstraint
     * 1. Check that all cases are returned by AbsoluteValueConstraint::getAllCases
     * 2. Check that AbsoluteValueConstraint::getCaseSplit( case ) returns the correct case
     */
    void test_abs_get_cases()
    {
        AbsoluteValueConstraint abs( 4, 6 );

        List<PhaseStatus> cases = abs.getAllCases();

        TS_ASSERT_EQUALS( cases.size(), 2u );
        TS_ASSERT_EQUALS( cases.front(), ABS_PHASE_NEGATIVE );
        TS_ASSERT_EQUALS( cases.back(), ABS_PHASE_POSITIVE );

        List<PiecewiseLinearCaseSplit> splits = abs.getCaseSplits();
        TS_ASSERT_EQUALS( splits.size(), 2u );
        TS_ASSERT_EQUALS( splits.front(), abs.getCaseSplit( ABS_PHASE_NEGATIVE ) );
        TS_ASSERT_EQUALS( splits.back(), abs.getCaseSplit( ABS_PHASE_POSITIVE ) );
    }

    /*
      Test context-dependent Abs state behavior.
     */
    void test_abs_context_dependent_state()
    {
        Context context;
        unsigned b = 1;
        unsigned f = 4;

        TestAbsConstraint abs( b, f );

        abs.initializeCDOs( &context );

        TS_ASSERT_EQUALS( abs.getPhaseStatus(), PHASE_NOT_FIXED );

        context.push();
        abs.notifyUpperBound( b, -1 );
        TS_ASSERT_EQUALS( abs.getPhaseStatus(), ABS_PHASE_NEGATIVE );

        context.pop();
        TS_ASSERT_EQUALS( abs.getPhaseStatus(), PHASE_NOT_FIXED );
        context.push();

        abs.notifyLowerBound( b, 1 );
        TS_ASSERT_EQUALS( abs.getPhaseStatus(), ABS_PHASE_POSITIVE );

        context.pop();
        TS_ASSERT_EQUALS( abs.getPhaseStatus(), PHASE_NOT_FIXED );
    }

    void test_get_cost_function_component()
    {
        /* Test the add cost function component methods */

        unsigned b = 0;
        unsigned f = 1;

        // The abs is fixed, do not add cost term.
        AbsoluteValueConstraint abs1 = AbsoluteValueConstraint( b, f );
        MockTableau tableau;
        abs1.registerTableau( &tableau );

        abs1.notifyLowerBound( b, 1 );
        abs1.notifyLowerBound( f, 1 );
        abs1.notifyUpperBound( b, 2 );
        abs1.notifyUpperBound( f, 2 );
        tableau.setValue( b, 1.5 );
        tableau.setValue( f, 2 );

        TS_ASSERT( abs1.phaseFixed() );
        LinearExpression cost1;
        TS_ASSERT_THROWS_NOTHING( abs1.getCostFunctionComponent( cost1, ABS_PHASE_POSITIVE ) );
        TS_ASSERT_EQUALS( cost1._addends.size(), 0u );
        TS_ASSERT_EQUALS( cost1._constant, 0 );


        // The abs is not fixed and add active cost term
        AbsoluteValueConstraint abs2 = AbsoluteValueConstraint( b, f );
        abs2.registerTableau( &tableau );

        LinearExpression cost2;
        abs2.notifyLowerBound( b, -1 );
        abs2.notifyLowerBound( f, 0 );
        abs2.notifyUpperBound( b, 2 );
        abs2.notifyUpperBound( f, 2 );
        tableau.setValue( b, -1 );
        tableau.setValue( f, 1 );
        TS_ASSERT( !abs2.phaseFixed() );
        TS_ASSERT_THROWS_NOTHING( abs2.getCostFunctionComponent( cost2, ABS_PHASE_POSITIVE ) );
        TS_ASSERT_EQUALS( cost2._addends.size(), 2u );
        TS_ASSERT_EQUALS( cost2._addends[b], -1 );
        TS_ASSERT_EQUALS( cost2._addends[f], 1 );

        // The abs is not fixed and add inactive cost term
        AbsoluteValueConstraint abs3 = AbsoluteValueConstraint( b, f );
        abs3.registerTableau( &tableau );

        LinearExpression cost3;
        abs3.notifyLowerBound( b, -1 );
        abs3.notifyLowerBound( f, 0 );
        abs3.notifyUpperBound( b, 2 );
        abs3.notifyUpperBound( f, 2 );
        TS_ASSERT( !abs3.phaseFixed() );
        TS_ASSERT_THROWS_NOTHING( abs3.getCostFunctionComponent( cost3, ABS_PHASE_NEGATIVE ) );
        TS_ASSERT_EQUALS( cost3._addends.size(), 2u );
        TS_ASSERT_EQUALS( cost3._addends[f], 1 );
        TS_ASSERT_EQUALS( cost3._addends[f], 1 );

        // Add the cost term for another abs
        unsigned b2 = 2;
        unsigned f2 = 3;
        AbsoluteValueConstraint abs4 = AbsoluteValueConstraint( b2, f2 );
        abs4.registerTableau( &tableau );

        abs4.notifyLowerBound( b2, -1 );
        abs4.notifyLowerBound( f2, 0 );
        abs4.notifyUpperBound( b2, 2 );
        abs4.notifyUpperBound( f2, 2 );
        tableau.setValue( b2, -1 );
        tableau.setValue( f2, 1 );

        TS_ASSERT( !abs4.phaseFixed() );
        TS_ASSERT_THROWS_NOTHING( abs4.getCostFunctionComponent( cost3, ABS_PHASE_POSITIVE ) );
        TS_ASSERT_EQUALS( cost3._addends.size(), 4u );
        TS_ASSERT_EQUALS( cost3._addends[f], 1 );
        TS_ASSERT_EQUALS( cost3._addends[b], 1 );
        TS_ASSERT_EQUALS( cost3._addends[b2], -1 );
        TS_ASSERT_EQUALS( cost3._addends[f2], 1 );

        TS_ASSERT_THROWS_NOTHING( abs4.getCostFunctionComponent( cost3, ABS_PHASE_POSITIVE ) );
        TS_ASSERT_EQUALS( cost3._addends.size(), 4u );
        TS_ASSERT_EQUALS( cost3._addends[f], 1 );
        TS_ASSERT_EQUALS( cost3._addends[b], 1 );
        TS_ASSERT_EQUALS( cost3._addends[b2], -2 );
        TS_ASSERT_EQUALS( cost3._addends[f2], 2 );
    }

    void test_get_phase_in_assignment()
    {
        unsigned b = 0;
        unsigned f = 1;

        // The abs is fixed, do not add cost term.
        AbsoluteValueConstraint abs = AbsoluteValueConstraint( b, f );
        MockTableau tableau;
        abs.registerTableau( &tableau );
        tableau.setValue( b, 1.5 );
        tableau.setValue( f, 2 );

        Map<unsigned, double> assignment;
        assignment[0] = -1;
        TS_ASSERT_EQUALS( abs.getPhaseStatusInAssignment( assignment ),
                          ABS_PHASE_NEGATIVE );

        assignment[0] = 15;
        TS_ASSERT_EQUALS( abs.getPhaseStatusInAssignment( assignment ),
                          ABS_PHASE_POSITIVE );
    }

};
