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
    { // TODO - PASSES
        unsigned b = 1;
        unsigned f = 4;

        SignConstraint sign( b, f ); // define constraint

        List<unsigned> participatingVariables; // needs to return 1 and 4 - the tableu vars
        TS_ASSERT_THROWS_NOTHING( participatingVariables = sign.getParticipatingVariables() ); // see doesnt throw
        TS_ASSERT_EQUALS( participatingVariables.size(), 2U ); // check that equals 2 - becausse there are 2vars
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

        sign.notifyVariableValue( b, -1 ); // change val of b to '-1'
        sign.notifyVariableValue( f, -1 ); // change val of f to '1

        TS_ASSERT( sign.satisfied() ); // f= Relu (b) or f=sign(b)

        sign.notifyVariableValue( f, 1 );

        TS_ASSERT( !sign.satisfied() );

        sign.notifyVariableValue( b, 2 );

        TS_ASSERT( sign.satisfied() );

        sign.notifyVariableValue( b, -2 );

        TS_ASSERT( !sign.satisfied() ); // because f is '1'

        sign.notifyVariableValue( f, 1 );

        TS_ASSERT( !sign.satisfied() );

        sign.notifyVariableValue( b, 0 );

        TS_ASSERT( sign.satisfied() );

        sign.notifyVariableValue( b, 9 );

        TS_ASSERT( sign.satisfied() );

        sign.notifyVariableValue( 4, -8 ); // is first var is 'f'

        // A relu cannot be satisfied if f is not '1' or '-1'
        TS_ASSERT( !sign.satisfied() );

        sign.notifyVariableValue( 4, 1.5 ); // is first var is 'f'

        // A relu cannot be satisfied if f is not '1' or '-1'
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

        TS_ASSERT( sign.satisfied() );
    }


//
};









#endif //MARABOU_TEST_SIGNCONSTRAINT_H
