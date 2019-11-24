//
// Created by shirana on 6/13/19.
//

#ifndef MARABOU_TEST_ABSCONSTRAINT_H
#define MARABOU_TEST_ABSCONSTRAINT_H



#include <cxxtest/TestSuite.h>

#include "MockErrno.h"
#include "MockTableau.h"
#include "PiecewiseLinearCaseSplit.h"
#include "AbsConstraint.h"
#include "AbsError.h"

#include <string.h>


#include <iostream>



class MockForAbsConstraint
        : public MockErrno
{
public:
};

class AbsConstraintTestSuite : public CxxTest::TestSuite
{
public:
    MockForAbsConstraint *mock;

    void setUp()
    {
        TS_ASSERT( mock = new MockForAbsConstraint );
    }

    void tearDown()
    {
        TS_ASSERT_THROWS_NOTHING( delete mock );
    }


    void test_abs_duplicate_and_restore()
    {
        AbsConstraint *abs1 = new AbsConstraint( 4, 6 );
        abs1->setActiveConstraint( false );
        abs1->notifyVariableValue( 4, 1.0 );
        abs1->notifyVariableValue( 6, 1.0 );

        abs1->notifyLowerBound( 4, -8.0 );
        abs1->notifyUpperBound( 4, 8.0 );

        abs1->notifyLowerBound( 6, 0.0 );
        abs1->notifyUpperBound( 6, 8.0 );

        PiecewiseLinearConstraint *abs2 = abs1->duplicateConstraint();

        abs1->notifyVariableValue( 4, -2 );

        TS_ASSERT( !abs1->satisfied() );

        TS_ASSERT( !abs2->isActive() );
        TS_ASSERT( abs2->satisfied() );

        abs2->restoreState( abs1 );
        TS_ASSERT( !abs2->satisfied() );

        TS_ASSERT_THROWS_NOTHING( delete abs2 );
    }

    void test_register_and_unregister_as_watcher()
    {
        unsigned b = 1;
        unsigned f = 4;

        MockTableau tableau;

        AbsConstraint abs( b, f );

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

    void test_participatingVariable_and_getParticipatingVariables()
    {
        unsigned b = 1;
        unsigned f = 4;

        AbsConstraint abs( b, f );

        List<unsigned> participatingVariables;

        TS_ASSERT_THROWS_NOTHING( participatingVariables = abs.getParticipatingVariables() );
        TS_ASSERT_EQUALS( participatingVariables.size(), 2U );
        auto it = participatingVariables.begin();
        TS_ASSERT_EQUALS( *it, b );
        ++it;
        TS_ASSERT_EQUALS( *it, f );

        TS_ASSERT_EQUALS(abs.getParticipatingVariables(), List<unsigned>({1,4}))

        TS_ASSERT( abs.participatingVariable( b ) );
        TS_ASSERT( abs.participatingVariable( f ) );
        TS_ASSERT( !abs.participatingVariable( 2 ) );
        TS_ASSERT( !abs.participatingVariable( 0 ) );
        TS_ASSERT( !abs.participatingVariable( 5 ) );

        //todo:
//        TS_ASSERT_THROWS_EQUALS( abs.satisfied(),const ReluplexError &e,
//                e.getCode(), ReluplexError::PARTICIPATING_VARIABLES_ABSENT );
    }

    void test_abs_notifyVariableValue_and_satisfied()
    {
        unsigned b = 1;
        unsigned f = 4;

        AbsConstraint abs( b, f );

        //x_b = x_f --> SAT
        abs.notifyVariableValue( b, 5 );
        abs.notifyVariableValue( f, 5 );
        TS_ASSERT( abs.satisfied() );

        //-x_b = x_f --> SAT
        abs.notifyVariableValue( b, -5 );
        abs.notifyVariableValue( f, 5 );
        TS_ASSERT( abs.satisfied() );

        //x_b = -x_f --> UNSAT
        abs.notifyVariableValue( b, 5 );
        abs.notifyVariableValue( f, -5 );
        TS_ASSERT( !abs.satisfied() );

        //x_f < 0 --> UNSAT
        abs.notifyVariableValue( f, -1 );
        abs.notifyVariableValue( b, -5 );
        TS_ASSERT( !abs.satisfied() );

        abs.notifyVariableValue( b, 1 );
        TS_ASSERT( !abs.satisfied() );

        //x_b > x_f -->UNSAT
        abs.notifyVariableValue( b, 5 );
        abs.notifyVariableValue( f, 4 );
        TS_ASSERT( !abs.satisfied() );

        abs.notifyVariableValue( b, -4 );
        abs.notifyVariableValue( f, -5 );
        TS_ASSERT( !abs.satisfied() );

        //x_b < x_f -->UNSAT
        abs.notifyVariableValue( f, 1 );
        abs.notifyVariableValue( b, -2 );
        TS_ASSERT( !abs.satisfied() );

        abs.notifyVariableValue( f, -1 );
        abs.notifyVariableValue( b, -2 );
        TS_ASSERT( !abs.satisfied() );
    }


    void test_abs_updateVariableIndex()
    {
        unsigned b = 1;
        unsigned f = 4;

        AbsConstraint abs( b, f );

        // Changing variable indices
        abs.notifyVariableValue( b, 1 );
        abs.notifyVariableValue( f, 1 );
        TS_ASSERT( abs.satisfied() );

        unsigned newB = 12;
        unsigned newF = 14;

        TS_ASSERT_THROWS_NOTHING( abs.updateVariableIndex( b, newB ) );
        TS_ASSERT_THROWS_NOTHING( abs.updateVariableIndex( f, newF ) );

        TS_ASSERT( abs.satisfied() );

        abs.notifyVariableValue( newF, 2 );

        TS_ASSERT( !abs.satisfied() );

        abs.notifyVariableValue( newB, 2 );

        TS_ASSERT( abs.satisfied() );
    }


    void test_abs_getPossibleFixes()
    {
        // Possible violations:
        //   1. f is positive, b is positive, b and f are unequal
        //   2. f is positive, b is negative, -b and f are unequal

        unsigned b = 1;
        unsigned f = 4;

        AbsConstraint abs( b, f );

        List<PiecewiseLinearConstraint::Fix> fixes;
        List<PiecewiseLinearConstraint::Fix>::iterator it;

        //   1. f is positive, b is positive, b and f are unequal

        abs.notifyVariableValue( b, 2 );
        abs.notifyVariableValue( f, 1 );

        fixes = abs.getPossibleFixes();
        TS_ASSERT_EQUALS( fixes.size(),3U );
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
        abs.notifyVariableValue( b, -2 );
        abs.notifyVariableValue( f, 1 );

        fixes = abs.getPossibleFixes();
        TS_ASSERT_EQUALS( fixes.size(),3U );
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

        AbsConstraint abs( b, f );

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

        AbsConstraint abs( b, f );

        abs.registerAsWatcher( &tableau );

        TS_ASSERT( !abs.constraintObsolete() );
        TS_ASSERT_THROWS_NOTHING( abs.eliminateVariable( f, 5 ) );
        TS_ASSERT( abs.constraintObsolete() );
    }


    void test_abs_entailed_tighteningst0()
    {
        unsigned b = 1;
        unsigned f = 4;

        AbsConstraint abs(b, f);
        List<Tightening> entailedTightenings;
        List<Tightening>::iterator it;

        // B = D , A < C
        // A < C < BD
        abs.notifyUpperBound( f,10);
        abs.notifyUpperBound( b,10);
        abs.notifyLowerBound( f,-1);
        TS_TRACE("f lower bound");
        TS_TRACE(abs.get_lower_bound(f));
        TS_ASSERT_EQUALS( abs.get_lower_bound(f), -1);
        abs.notifyLowerBound( b, 5);
        TS_ASSERT( abs.phaseFixed() );
        TS_TRACE("f lower bound");
        TS_TRACE(abs.get_lower_bound(f));
        abs.getEntailedTightenings( entailedTightenings );
//        print_entailed_Tightenings(entailedTightenings);
//        TS_ASSERT_EQUALS( entailedTightenings.size(),1U );
//        it = entailedTightenings.begin();
//        TS_ASSERT_EQUALS( it->_variable, f );
//        TS_ASSERT_EQUALS( it->_value, 5 );
//        TS_ASSERT_EQUALS( it->_type, Tightening::LB );

    }

    void test_abs_entailed_tighteningst1()
    {
        /**
         * suppose A < x_b < B, C < x_f < D, remainder C >= 0 ,D > 0
         * A > 0 & B > 0 & C > 0
         */
        unsigned b = 1;
        unsigned f = 4;

        AbsConstraint abs(b, f);
        List<Tightening> entailedTightenings;
        List<Tightening>::iterator it;

        // B = D , A < C
        // A < C < BD
        abs.notifyUpperBound( b, 7 );
        abs.notifyUpperBound( f, 7 );
        abs.notifyLowerBound( b, 1);
        abs.notifyLowerBound( f, 2);
        // 1 < x_b < 7 , 2 < x_f < 7 -> 2 < x_b
        abs.getEntailedTightenings( entailedTightenings );
        TS_ASSERT_EQUALS( entailedTightenings.size(),1U );
        it = entailedTightenings.begin();
        TS_ASSERT_EQUALS( it->_variable, b );
        TS_ASSERT_EQUALS( it->_value, 2 );
        TS_ASSERT_EQUALS( it->_type, Tightening::LB );

        abs.notifyLowerBound( b, 3);
        // B = D , A > C
        //C < A < BD
        //3 < x_b < 7 , 2 < x_f < 7
        entailedTightenings.clear();
        abs.getEntailedTightenings( entailedTightenings );
        TS_ASSERT_EQUALS( entailedTightenings.size(),1U );
        it = entailedTightenings.begin();
        TS_ASSERT_EQUALS( it->_variable, f );
        TS_ASSERT_EQUALS( it->_value, 3 );
        TS_ASSERT_EQUALS( it->_type, Tightening::LB );

        abs.notifyLowerBound( f, 3);
        abs.notifyUpperBound(b, 6);
        // B < D , A = C
        //CA < B < D
        //3 < x_b < 6 , 3 < x_f < 7
        entailedTightenings.clear();
        abs.getEntailedTightenings( entailedTightenings );
        TS_ASSERT_EQUALS( entailedTightenings.size(),1U );
        it = entailedTightenings.begin();
        TS_ASSERT_EQUALS( it->_variable, f );
        TS_ASSERT_EQUALS( it->_value, 6 );
        TS_ASSERT_EQUALS( it->_type, Tightening::UB );

        abs.notifyUpperBound( f, 6);
        abs.notifyUpperBound(b, 7);
        // B > D , A = C
        //CA < B < D
        //3 < x_b < 6 , 3 < x_f < 6
        // --> x_b < 6
        entailedTightenings.clear();
        abs.getEntailedTightenings( entailedTightenings );
        TS_ASSERT_EQUALS( entailedTightenings.size(),0U );

        abs.notifyLowerBound(f, -3);
        //A > 0 & B > 0 & C < 0
        // B = D , A > C
        //3 < x_b < 6 , 3 < x_f < 6
        // --> 3 < x_f
        //CA < B < D
        entailedTightenings.clear();
        abs.getEntailedTightenings( entailedTightenings );
        TS_ASSERT_EQUALS( entailedTightenings.size(),0U );

        abs.notifyLowerBound(b, 5);
        abs.notifyUpperBound(f,5);
        //C <DA<B
        //5 < x_b < 6 , 4 < x_f < 5
        entailedTightenings.clear();
        abs.getEntailedTightenings( entailedTightenings );
        TS_ASSERT_EQUALS( entailedTightenings.size(),2U );
        it = entailedTightenings.begin();
        TS_ASSERT_EQUALS( it->_variable, f );
        TS_ASSERT_EQUALS( it->_value, 5 );
        TS_ASSERT_EQUALS( it->_type, Tightening::LB );
        it++;
        TS_ASSERT_EQUALS( it->_variable, b );
        TS_ASSERT_EQUALS( it->_value, 5 );
        TS_ASSERT_EQUALS( it->_type, Tightening::UB );
    }

    void test_abs_entailed_tighteningst1_nonOverlap1()
    {
        unsigned b = 1;
        unsigned f = 4;

        AbsConstraint abs(b, f);
        List<Tightening> entailedTightenings;
        List<Tightening>::iterator it;

        // A < B < C < D
        //8 < b < 18, 48<f<64
        abs.notifyUpperBound( b, 18 );
        abs.notifyUpperBound( f, 64 );
        abs.notifyLowerBound( b, 8);
        abs.notifyLowerBound( f, 48);
        abs.getEntailedTightenings( entailedTightenings );
        TS_ASSERT_EQUALS( entailedTightenings.size(),2U );
        it = entailedTightenings.begin();
        TS_ASSERT_EQUALS( it->_variable, b );
        TS_ASSERT_EQUALS( it->_value, 48 );
        TS_ASSERT_EQUALS( it->_type, Tightening::LB );
        it++;
        TS_ASSERT_EQUALS( it->_variable, f );
        TS_ASSERT_EQUALS( it->_value, 18 );
        TS_ASSERT_EQUALS( it->_type, Tightening::UB );
    }

    void test_abs_entailed_tighteningst1_nonOverlap2()
    {
        unsigned b = 1;
        unsigned f = 4;

        AbsConstraint abs(b, f);
        List<Tightening> entailedTightenings;
        List<Tightening>::iterator it;

        // C < D < A < B
        //8 < b < 18, 48<f<64
        abs.notifyUpperBound( b, 4 );
        abs.notifyUpperBound( f, 2 );
        abs.notifyLowerBound( b, 3);
        abs.notifyLowerBound( f, 1);
        abs.getEntailedTightenings( entailedTightenings );
        TS_ASSERT_EQUALS( entailedTightenings.size(),2U );
        it = entailedTightenings.begin();
        TS_ASSERT_EQUALS( it->_variable, f );
        TS_ASSERT_EQUALS( it->_value, 3 );
        TS_ASSERT_EQUALS( it->_type, Tightening::LB );
        it++;
        TS_ASSERT_EQUALS( it->_variable, b );
        TS_ASSERT_EQUALS( it->_value, 2 );
        TS_ASSERT_EQUALS( it->_type, Tightening::UB );
    }

    void test_abs_entailed_tighteningst2()
    {
        /**
         * suppose A < x_b < B, C < x_f < D, remainder C >= 0 ,D > 0
         * A > 0 & B > 0 & C = 0
         */
        unsigned b = 1;
        unsigned f = 4;

        AbsConstraint abs(b, f);
        List <Tightening> entailedTightenings;
        List<Tightening>::iterator it;

        abs.notifyUpperBound( b, 7 );
        abs.notifyUpperBound( f, 6);
        abs.notifyLowerBound( b, 0);
        abs.notifyLowerBound( f, 0);

        // B > D , A < C
        //AC<D<B
        // 0 < x_b < 7 ,0 < x_f < 6
        abs.getEntailedTightenings( entailedTightenings );
        TS_ASSERT_EQUALS( entailedTightenings.size(),1U );
        it = entailedTightenings.begin();
        TS_ASSERT_EQUALS( it->_variable, b );
        TS_ASSERT_EQUALS( it->_value, 6 );
        TS_ASSERT_EQUALS( it->_type, Tightening::UB );

        abs.notifyUpperBound(b, 5);
        //AC<B<D
        // 0 < x_b < 5 ,0 < x_f < 6
        entailedTightenings.clear();
        abs.getEntailedTightenings( entailedTightenings );
        TS_ASSERT_EQUALS( entailedTightenings.size(),1U );
        it = entailedTightenings.begin();
        TS_ASSERT_EQUALS( it->_variable, f);
        TS_ASSERT_EQUALS( it->_value, 5 );
        TS_ASSERT_EQUALS( it->_type, Tightening::UB );

        abs.notifyLowerBound(b, 1);
        //C<A<B<D
        // 1 < x_b < 5 ,0 < x_f < 6
        entailedTightenings.clear();
        abs.getEntailedTightenings( entailedTightenings );
        TS_ASSERT_EQUALS( entailedTightenings.size(),2U );
        it = entailedTightenings.begin();
        TS_ASSERT_EQUALS( it->_variable, f );
        TS_ASSERT_EQUALS( it->_value, 1 );
        TS_ASSERT_EQUALS( it->_type, Tightening::LB );
        it++;
        TS_ASSERT_EQUALS( it->_variable, f );
        TS_ASSERT_EQUALS( it->_value, 5 );
        TS_ASSERT_EQUALS( it->_type, Tightening::UB );

        abs.notifyUpperBound(f, 4);
        //C<A<D<B
        // 1 < x_b < 5 ,0 < x_f < 4
        entailedTightenings.clear();
        abs.getEntailedTightenings( entailedTightenings );
        TS_ASSERT_EQUALS( entailedTightenings.size(),2U );
        it = entailedTightenings.begin();
        TS_ASSERT_EQUALS( it->_variable, f );
        TS_ASSERT_EQUALS( it->_value, 1 );
        TS_ASSERT_EQUALS( it->_type, Tightening::LB );
        it++;
        TS_ASSERT_EQUALS( it->_variable, b );
        TS_ASSERT_EQUALS( it->_value, 4 );
        TS_ASSERT_EQUALS( it->_type, Tightening::UB );

        //non overlap
        abs.notifyUpperBound(f,2);
        abs.notifyLowerBound(b,3);

        //C<D<A<B
        // 3 < x_b < 5 ,0 < x_f < 2
        entailedTightenings.clear();
        abs.getEntailedTightenings( entailedTightenings );
        TS_ASSERT_EQUALS( entailedTightenings.size(),2U );
        it = entailedTightenings.begin();
        TS_ASSERT_EQUALS( it->_variable, f );
        TS_ASSERT_EQUALS( it->_value, 3 );
        TS_ASSERT_EQUALS( it->_type, Tightening::LB );
        it++;
        TS_ASSERT_EQUALS( it->_variable, b );
        TS_ASSERT_EQUALS( it->_value, 2 );
        TS_ASSERT_EQUALS( it->_type, Tightening::UB );
    }

    void test_abs_entailed_tighteningst2_more() {
        /**
         * suppose A < x_b < B, C < x_f < D, remainder C >= 0 ,D > 0
         * A > 0 & B > 0 & C = 0
         */
        unsigned b = 1;
        unsigned f = 4;

        AbsConstraint abs(b, f);
        List <Tightening> entailedTightenings;
        List<Tightening>::iterator it;

        abs.notifyUpperBound(b, 6);
        abs.notifyUpperBound(f, 5);
        abs.notifyLowerBound(b, 4);
        abs.notifyLowerBound(f, 3);

        //C<A<D<B
        // 3 < x_b < 5 ,4 < x_f < 6
        entailedTightenings.clear();
        abs.getEntailedTightenings( entailedTightenings );
        TS_ASSERT_EQUALS( entailedTightenings.size(),2U );
        it = entailedTightenings.begin();
        TS_ASSERT_EQUALS( it->_variable, f );
        TS_ASSERT_EQUALS( it->_value, 4 );
        TS_ASSERT_EQUALS( it->_type, Tightening::LB );
        it++;
        TS_ASSERT_EQUALS( it->_variable, b );
        TS_ASSERT_EQUALS( it->_value, 5 );
        TS_ASSERT_EQUALS( it->_type, Tightening::UB );
    }


    void test_abs_entailed_tighteningst3()
    {
        /**
         * suppose A < x_b < B, C < x_f < D, remainder C >= 0 ,D > 0
         * A < 0 & B > 0 & C > 0
         */
        unsigned b = 1;
        unsigned f = 4;

        AbsConstraint abs(b, f);
        List <Tightening> entailedTightenings;
        List<Tightening>::iterator it;


        abs.notifyUpperBound( b, 3 );
        abs.notifyUpperBound( f, 4);
        abs.notifyLowerBound( b, -6);
        abs.notifyLowerBound( f, 2);

        // B < D , A < C
        //A<0<C<B<D
        // -6 < x_b < 3 ,2 < x_f < 4
        abs.getEntailedTightenings( entailedTightenings );
        TS_ASSERT_EQUALS( entailedTightenings.size(),1U );
        it = entailedTightenings.begin();
        TS_ASSERT_EQUALS( it->_variable, b );
        TS_ASSERT_EQUALS( it->_value, -4 );
        TS_ASSERT_EQUALS( it->_type, Tightening::LB );


        abs.notifyUpperBound(b, 2);
        //A<0<B<C<D
        // -6 < x_b < 2 ,2 < x_f < 4
        entailedTightenings.clear();
        abs.getEntailedTightenings( entailedTightenings );
        TS_ASSERT_EQUALS( entailedTightenings.size(),1U );
        it = entailedTightenings.begin();
        TS_ASSERT_EQUALS( it->_variable, b );
        TS_ASSERT_EQUALS( it->_value, -4 );
        TS_ASSERT_EQUALS( it->_type, Tightening::LB );

        //non overlap
        abs.notifyUpperBound(b, 1);
        //A<0<B<C<D
        // -6 < x_b < 1 ,2 < x_f < 4
        entailedTightenings.clear();
        abs.getEntailedTightenings( entailedTightenings );
        TS_ASSERT_EQUALS( entailedTightenings.size(),2U );
        it = entailedTightenings.begin();
        TS_ASSERT_EQUALS( it->_variable, b );
        TS_ASSERT_EQUALS( it->_value, -4 );
        TS_ASSERT_EQUALS( it->_type, Tightening::LB );
        it++;
        TS_ASSERT_EQUALS( it->_variable, b );
        TS_ASSERT_EQUALS( it->_value, -2 );
        TS_ASSERT_EQUALS( it->_type, Tightening::UB );
    }

    void test_abs_entailed_tighteningst3_more()
    {
        /**
         * suppose A < x_b < B, C < x_f < D, remainder C >= 0 ,D > 0
         * A < 0 & B > 0 & C > 0
         */
        unsigned b = 1;
        unsigned f = 4;

        AbsConstraint abs(b, f);
        List <Tightening> entailedTightenings;
        List<Tightening>::iterator it;


        abs.notifyUpperBound( b, 10 );
        abs.notifyUpperBound( f, 7);
        abs.notifyLowerBound( b, -5);
        abs.notifyLowerBound( f, 3);

        //A<0<C<D<B
        // -5 < x_b < 10 ,3 < x_f < 7
        abs.getEntailedTightenings( entailedTightenings );
        TS_ASSERT_EQUALS( entailedTightenings.size(),1U );
        it = entailedTightenings.begin();
        TS_ASSERT_EQUALS( it->_variable, b );
        TS_ASSERT_EQUALS( it->_value, 7 );
        TS_ASSERT_EQUALS( it->_type, Tightening::UB );

        abs.notifyLowerBound(f, 6);
        //A<0<C<D<B
        // -5 < x_b < 10 ,6 < x_f < 7
        entailedTightenings.clear();
        abs.getEntailedTightenings( entailedTightenings );
        TS_ASSERT_EQUALS( entailedTightenings.size(),2U );
        it = entailedTightenings.begin();
        TS_ASSERT_EQUALS( it->_variable, b );
        TS_ASSERT_EQUALS( it->_value, 6 );
        TS_ASSERT_EQUALS( it->_type, Tightening::LB );
        it++;
        TS_ASSERT_EQUALS( it->_variable, b );
        TS_ASSERT_EQUALS( it->_value, 7 );
        TS_ASSERT_EQUALS( it->_type, Tightening::UB );

        abs.notifyUpperBound(b, 3);
        //A<0<C<D<B
        // -5 < x_b < 3 ,6 < x_f < 7
        entailedTightenings.clear();
        abs.getEntailedTightenings( entailedTightenings );
        TS_ASSERT_EQUALS( entailedTightenings.size(),3U );
        it = entailedTightenings.begin();
        TS_ASSERT_EQUALS( it->_variable, b );
        TS_ASSERT_EQUALS( it->_value, 6 );
        TS_ASSERT_EQUALS( it->_type, Tightening::LB );
        it++;
        TS_ASSERT_EQUALS( it->_variable, b );
        TS_ASSERT_EQUALS( it->_value, -6 );
        TS_ASSERT_EQUALS( it->_type, Tightening::UB );
        it++;
        TS_ASSERT_EQUALS( it->_variable, f );
        TS_ASSERT_EQUALS( it->_value, 5 );
        TS_ASSERT_EQUALS( it->_type, Tightening::UB );
    }


    void test_abs_entailed_tighteningst4()
    {
        /**
         * suppose A < x_b < B, C < x_f < D, remainder C >= 0 ,D > 0
         * A < 0 & B > 0 & C = 0
         */
        unsigned b = 1;
        unsigned f = 4;

        AbsConstraint abs(b, f);
        List <Tightening> entailedTightenings;
        List<Tightening>::iterator it;



        abs.notifyUpperBound( b, 7 );
        abs.notifyUpperBound( f, 6);
        abs.notifyLowerBound( b, -7);
        abs.notifyLowerBound( f, 0);

        // B > D , A < C
        // -7 < x_b < 7 ,0 < x_f < 6
        abs.getEntailedTightenings( entailedTightenings );
        TS_ASSERT_EQUALS( entailedTightenings.size(),2U );
        it = entailedTightenings.begin();
        TS_ASSERT_EQUALS( it->_variable, b );
        TS_ASSERT_EQUALS( it->_value, -6 );
        TS_ASSERT_EQUALS( it->_type, Tightening::LB );
        it++;
        TS_ASSERT_EQUALS( it->_variable, b );
        TS_ASSERT_EQUALS( it->_value, 6 );
        TS_ASSERT_EQUALS( it->_type, Tightening::UB );

        abs.notifyUpperBound(b, 5);
        // -7 < x_b < 5 ,0 < x_f < 6
        entailedTightenings.clear();
        abs.getEntailedTightenings( entailedTightenings );
        TS_ASSERT_EQUALS( entailedTightenings.size(),1U );
        it = entailedTightenings.begin();
        TS_ASSERT_EQUALS( it->_variable, b );
        TS_ASSERT_EQUALS( it->_value, -6 );
        TS_ASSERT_EQUALS( it->_type, Tightening::LB );

        abs.notifyLowerBound(b, 0);
        // 0 < x_b < 5 ,0 < x_f < 6
        entailedTightenings.clear();
        abs.getEntailedTightenings( entailedTightenings );
        TS_ASSERT_EQUALS( entailedTightenings.size(),1U );
        it = entailedTightenings.begin();
        TS_ASSERT_EQUALS( it->_variable, f );
        TS_ASSERT_EQUALS( it->_value, 5 );
        TS_ASSERT_EQUALS( it->_type, Tightening::UB );

        abs.notifyLowerBound(b, 3);
        // 3 < x_b < 5 ,0 < x_f < 6
        entailedTightenings.clear();
        abs.getEntailedTightenings( entailedTightenings );
        TS_ASSERT_EQUALS( entailedTightenings.size(),2U );
        it = entailedTightenings.begin();
        TS_ASSERT_EQUALS( it->_variable, f );
        TS_ASSERT_EQUALS( it->_value, 3 );
        TS_ASSERT_EQUALS( it->_type, Tightening::LB );
        it++;
        TS_ASSERT_EQUALS( it->_variable, f );
        TS_ASSERT_EQUALS( it->_value, 5 );
        TS_ASSERT_EQUALS( it->_type, Tightening::UB );
    }

    void test_abs_entailed_tighteningst5()
    {
        /**
         * suppose A < x_b < B, C < x_f < D, remainder C >= 0 ,D > 0
         * A < 0 & B < 0 & C > 0
         */
        unsigned b = 1;
        unsigned f = 4;

        AbsConstraint abs(b, f);
        List <Tightening> entailedTightenings;
        List<Tightening>::iterator it;

        abs.notifyUpperBound( b, -2);
        abs.notifyUpperBound( f, 15);
        abs.notifyLowerBound( b, -20);
        abs.notifyLowerBound( f, 0);

        abs.getEntailedTightenings( entailedTightenings );
        TS_ASSERT_EQUALS( entailedTightenings.size(),2U );
        it = entailedTightenings.begin();
        TS_ASSERT_EQUALS( it->_variable, b );
        TS_ASSERT_EQUALS( it->_value, -15 );
        TS_ASSERT_EQUALS( it->_type, Tightening::LB );
        it++;
        TS_ASSERT_EQUALS( it->_variable, f );
        TS_ASSERT_EQUALS( it->_value, 2 );
        TS_ASSERT_EQUALS( it->_type, Tightening::LB );

        abs.notifyLowerBound( f, 7);
        // -20 < x_b < -2 ,7 < x_f < 15
        entailedTightenings.clear();
        abs.getEntailedTightenings( entailedTightenings );
        TS_ASSERT_EQUALS( entailedTightenings.size(),2U );
        it = entailedTightenings.begin();
        TS_ASSERT_EQUALS( it->_variable, b );
        TS_ASSERT_EQUALS( it->_value, -15 );
        TS_ASSERT_EQUALS( it->_type, Tightening::LB );
        it++;
        TS_ASSERT_EQUALS( it->_variable, b );
        TS_ASSERT_EQUALS( it->_value, -7 );
        TS_ASSERT_EQUALS( it->_type, Tightening::UB );

        abs.notifyLowerBound(b, -12);
        // -12 < x_b < -2 ,7 < x_f < 15
        entailedTightenings.clear();
        abs.getEntailedTightenings( entailedTightenings );
        TS_ASSERT_EQUALS( entailedTightenings.size(),2U );
        it = entailedTightenings.begin();
        TS_ASSERT_EQUALS( it->_variable, b );
        TS_ASSERT_EQUALS( it->_value, -7 );
        TS_ASSERT_EQUALS( it->_type, Tightening::UB );
        it++;
        TS_ASSERT_EQUALS( it->_variable, f );
        TS_ASSERT_EQUALS( it->_value, 12 );
        TS_ASSERT_EQUALS( it->_type, Tightening::UB );

        abs.notifyUpperBound(b, -8);
        // -12 < x_b < -2 ,7 < x_f < 15

        entailedTightenings.clear();
        abs.getEntailedTightenings( entailedTightenings );

        TS_ASSERT_EQUALS( entailedTightenings.size(),2U );
        it = entailedTightenings.begin();
        TS_ASSERT_EQUALS( it->_variable, f );
        TS_ASSERT_EQUALS( it->_value, 8 );
        TS_ASSERT_EQUALS( it->_type, Tightening::LB );
        it++;
        TS_ASSERT_EQUALS( it->_variable, f );
        TS_ASSERT_EQUALS( it->_value, 12 );
        TS_ASSERT_EQUALS( it->_type, Tightening::UB );
    }

    void test_abs_entailed_tighteningst5_more() {
        /**
         * suppose A < x_b < B, C < x_f < D, remainder C >= 0 ,D > 0
         * A < 0 & B < 0 & C > 0
         */
        unsigned b = 1;
        unsigned f = 4;

        AbsConstraint abs(b, f);
        List <Tightening> entailedTightenings;
        List<Tightening>::iterator it;

        abs.notifyUpperBound(b, -2);
        abs.notifyUpperBound(f, 30);
        abs.notifyLowerBound(b, -20);
        abs.notifyLowerBound(f, 25);

        // -20 < x_b < -2 ,25 < x_f < 30
        abs.getEntailedTightenings(entailedTightenings);
        TS_ASSERT_EQUALS( entailedTightenings.size(),2U );
        it = entailedTightenings.begin();
        TS_ASSERT_EQUALS( it->_variable, b );
        TS_ASSERT_EQUALS( it->_value, -25 );
        TS_ASSERT_EQUALS( it->_type, Tightening::UB );
        it++;
        TS_ASSERT_EQUALS( it->_variable, f );
        TS_ASSERT_EQUALS( it->_value, 20 );
        TS_ASSERT_EQUALS( it->_type, Tightening::UB );
    }

    void print_bounds(AbsConstraint abs, unsigned b, unsigned f)
    {
        TS_TRACE("b upper bound");
        TS_TRACE(abs.get_upper_bound(b));
        TS_TRACE("f upper bound");
        TS_TRACE(abs.get_upper_bound(f));

        TS_TRACE("b lower bound");
        TS_TRACE(abs.get_lower_bound(b));
        TS_TRACE("f lower bound");
        TS_TRACE(abs.get_lower_bound(f));
    }


    void print_entailed_Tightenings(List<Tightening> entailedTightenings) {
        List<Tightening>::iterator it;
        it = entailedTightenings.begin();

        for (unsigned int i = 0; i < entailedTightenings.size(); i++) {
            TS_TRACE("entailedTightenings var, value, type");
            TS_TRACE(it->_variable);
            TS_TRACE(it->_value);
            TS_TRACE(it->_type);
            it++;
        }
    }

    void test_abs_case_splits()
    {
        unsigned b = 1;
        unsigned f = 4;

        AbsConstraint abs( b, f );

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

    void test_fix_positive_and_negative() {
        unsigned b = 1;
        unsigned f = 4;

        MockTableau tableau;

        AbsConstraint abs(b, f);

        abs.registerAsWatcher(&tableau);

        List <PiecewiseLinearCaseSplit> splits = abs.getCaseSplits();
        TS_ASSERT_EQUALS(splits.size(), 2U);

        abs.notifyLowerBound(1, 1.0);
        TS_ASSERT_THROWS_EQUALS(splits = abs.getCaseSplits(),
        const AbsError &e,
        e.getCode(),
                AbsError::REQUESTED_CASE_SPLITS_FROM_FIXED_CONSTRAINT );

        abs.unregisterAsWatcher(&tableau);

        abs = AbsConstraint( b, f );


        abs.registerAsWatcher( &tableau );

        splits = abs.getCaseSplits();
        TS_ASSERT_EQUALS( splits.size(), 2U );

        abs.notifyUpperBound( 1, -2.0 );
        TS_ASSERT_THROWS_EQUALS( splits = abs.getCaseSplits(),
        const AbsError &e,
        e.getCode(),
                AbsError::REQUESTED_CASE_SPLITS_FROM_FIXED_CONSTRAINT );

        abs.unregisterAsWatcher( &tableau );
    }

    void test_constraint_phase_gets_fixed()
    {
        unsigned b = 1;
        unsigned f = 4;

        MockTableau tableau;

        // Upper bounds
        {
            AbsConstraint abs( b, f );
            TS_ASSERT( !abs.phaseFixed() );
            abs.notifyUpperBound( b, -1.0 );
            TS_ASSERT( abs.phaseFixed() );
        }

        {
            AbsConstraint abs( b, f );
            TS_ASSERT( !abs.phaseFixed() );
            abs.notifyUpperBound( b, 0.0 );
            TS_ASSERT( abs.phaseFixed() );
        }

        {
            AbsConstraint abs( b, f );
            TS_ASSERT( !abs.phaseFixed() );
            abs.notifyUpperBound( f, 5 );
            TS_ASSERT( !abs.phaseFixed() );
        }

        {
            AbsConstraint abs( b, f );
            TS_ASSERT( !abs.phaseFixed() );
            abs.notifyUpperBound( b, 3.0 );
            TS_ASSERT( !abs.phaseFixed() );
        }


        // Lower bounds
        {
            AbsConstraint abs( b, f );
            TS_ASSERT( !abs.phaseFixed() );
            abs.notifyLowerBound( b, 3.0 );
            TS_ASSERT( abs.phaseFixed() );
        }

        {
            AbsConstraint abs( b, f );
            TS_ASSERT( !abs.phaseFixed() );
            abs.notifyLowerBound( b, 0.0 );
            TS_ASSERT( abs.phaseFixed() );
        }

        {
            AbsConstraint abs( b, f );
            TS_ASSERT( !abs.phaseFixed() );
            abs.notifyLowerBound( f, 6.0 );
            TS_ASSERT( !abs.phaseFixed() );
        }

        {
            AbsConstraint abs( b, f );
            TS_ASSERT( !abs.phaseFixed() );
            abs.notifyLowerBound( b, -2.5 );
            TS_ASSERT( !abs.phaseFixed() );
        }
    }

    void test_valid_split_abs_phase_fixed_to_active()
    {
        unsigned b = 1;
        unsigned f = 4;

        AbsConstraint abs( b, f );

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

        AbsConstraint abs( b, f );

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

};

#endif //MARABOU_TEST_ABSCONSTRAINT_H
