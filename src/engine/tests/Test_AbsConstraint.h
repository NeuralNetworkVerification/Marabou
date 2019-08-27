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

        TS_TRACE("abs test response");

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
        TS_ASSERT_EQUALS( fixes.size(),3 );
        it = fixes.begin();
        TS_ASSERT_EQUALS( it->_variable, b );
        TS_ASSERT_EQUALS( it->_value, 1 );
        ++it;
        TS_ASSERT_EQUALS( it->_variable, b );
        TS_ASSERT_EQUALS( it->_value, -2 );
        ++it;
        TS_ASSERT_EQUALS( it->_variable, f );
        TS_ASSERT_EQUALS( it->_value, 2 );

        //   2. f is positive, b is negative, -b and f are unequal
        abs.notifyVariableValue( b, -2 );
        abs.notifyVariableValue( f, 1 );

        fixes = abs.getPossibleFixes();
        TS_ASSERT_EQUALS( fixes.size(),3 );
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


//    void test_abs_entailed_tighteningst() {
//        unsigned b = 1;
//        unsigned f = 4;
//
//        AbsConstraint abs(b, f);
//        List<Tightening> entailedTightenings;
//        abs.notifyUpperBound( b, 7 );
//        abs.notifyUpperBound( f, 7 );
//        abs.getEntailedTightenings( entailedTightenings );
//        TS_ASSERT( !entailedTightenings.empty() );
//
//        abs.notifyLowerBound( b, 6);
//        abs.notifyLowerBound( f, -6);
//        abs.getEntailedTightenings( entailedTightenings );
//        TS_ASSERT( entailedTightenings.empty() );
//
//
//    }
};
#endif //MARABOU_TEST_ABSCONSTRAINT_H
