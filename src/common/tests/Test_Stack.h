/*********************                                                        */
/*! \file Test_Stack.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** \brief [[ Add one-line brief description here ]]
 **
 ** [[ Add lengthier description here ]]
 **/

#include <cxxtest/TestSuite.h>

#include "CommonError.h"
#include "MockErrno.h"
#include "Stack.h"

class StackTestSuite : public CxxTest::TestSuite
{
public:
    MockErrno *mockErrno;

    void setUp()
    {
        TS_ASSERT( mockErrno = new MockErrno );
    }

    void tearDown()
    {
        TS_ASSERT_THROWS_NOTHING( delete mockErrno );
    }

    void test_basic_stack()
    {
        Stack<int> q;

        TS_ASSERT( q.empty() );

        TS_ASSERT_THROWS_NOTHING( q.push( 1 ) );
        TS_ASSERT( !q.empty() );
        TS_ASSERT_THROWS_NOTHING( q.push( 2 ) );
        TS_ASSERT_THROWS_NOTHING( q.push( 3 ) );

        TS_ASSERT_EQUALS( q.top(), 3 );
        TS_ASSERT_EQUALS( q.top(), 3 );
        TS_ASSERT_THROWS_NOTHING( q.pop() );
        TS_ASSERT_EQUALS( q.top(), 2 );
        TS_ASSERT_THROWS_NOTHING( q.pop() );
        TS_ASSERT_EQUALS( q.top(), 1 );

        TS_ASSERT( !q.empty() );

        TS_ASSERT_THROWS_NOTHING( q.pop() );

        TS_ASSERT( q.empty() );
    }

    void test_clear_and_throw()
    {
        Stack<int> q;

        TS_ASSERT( q.empty() );

        q.push( 1 );

        TS_ASSERT( !q.empty() );

        TS_ASSERT_THROWS_NOTHING( q.clear() );

        TS_ASSERT( q.empty() );

        TS_ASSERT_THROWS_EQUALS( q.top(),
                                 const CommonError &e,
                                 e.getCode(),
                                 CommonError::STACK_IS_EMPTY );

        TS_ASSERT_THROWS_EQUALS( q.pop(),
                                 const CommonError &e,
                                 e.getCode(),
                                 CommonError::STACK_IS_EMPTY );
    }
};

//
// Local Variables:
// compile-command: "make -C ../../.. "
// tags-file-name: "../../../TAGS"
// c-basic-offset: 4
// End:
//
