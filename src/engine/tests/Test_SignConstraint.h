//
// Created by guyam on 6/4/20.
//

#ifndef MARABOU_TEST_SIGNCONSTRAINT_H
#define MARABOU_TEST_SIGNCONSTRAINT_H


#include <cxxtest/TestSuite.h>

#include "InputQuery.h"
#include "MockConstraintBoundTightener.h"
#include "MockTableau.h"
#include "PiecewiseLinearCaseSplit.h"
#include "ReluConstraint.h"
#include "SignConstraint.h"
#include "MarabouError.h"
#include "/cs/usr/guyam/CLionProjects/Marabou/src/common/tests/MockErrno.h"
//#include "MockErrno.h" // todo - yuval says red line ok


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
    { // TODO - CONTINUE!!!
        //TS_ASSERT(false)
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

        sign.notifyVariableValue( b, 1 );
        sign.notifyVariableValue( f, 1 );

        TS_ASSERT( sign.satisfied() );

        sign.notifyVariableValue( f, 2 );

        TS_ASSERT( !sign.satisfied() );

        sign.notifyVariableValue( b, 2 );

        TS_ASSERT( sign.satisfied() );

        sign.notifyVariableValue( b, -2 );

        TS_ASSERT( !sign.satisfied() );

        sign.notifyVariableValue( f, 0 );

        TS_ASSERT( sign.satisfied() );

        sign.notifyVariableValue( b, -3 );

        TS_ASSERT( sign.satisfied() );

        sign.notifyVariableValue( b, 0 );

        TS_ASSERT( sign.satisfied() );

        sign.notifyVariableValue( 4, -1 );

        // A relu cannot be satisfied if f is negative
        TS_ASSERT( !sign.satisfied() );

        sign.notifyVariableValue( f, 0 );
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

        sign.notifyVariableValue( newF, 2 );

        TS_ASSERT( !sign.satisfied() );

        sign.notifyVariableValue( newB, 2 );

        TS_ASSERT( sign.satisfied() );
    }

//
};









#endif //MARABOU_TEST_SIGNCONSTRAINT_H
