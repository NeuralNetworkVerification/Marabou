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

//todo: delete before submit
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

    void test_register_and_unregister__as_watcher()
    {
        unsigned b = 1;
        unsigned f = 4;

        MockTableau tableau;

        AbsConstraint abs( b, f );

        std::cout<<"test response"<< std::endl;

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


    void test_abs_entailed_tighteningst() {
        unsigned b = 1;
        unsigned f = 4;

        AbsConstraint abs(b, f);
        List<Tightening> entailedTightenings;
        abs.notifyUpperBound( b, 7 );
        abs.notifyUpperBound( f, 7 );

        abs.notifyLowerBound( b, -1 );
        abs.notifyLowerBound( f, 0 );
        abs.getEntailedTightenings( entailedTightenings );
//        TS_ASSERT( entailedTightenings.empty() );

        TS_ASSERT(abs.duplicateConstraint())
    }
};
#endif //MARABOU_TEST_ABSCONSTRAINT_H
