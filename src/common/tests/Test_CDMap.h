/*********************                                                        */
/*! \file Test_CDMap.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Andrew Wu
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2024 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** \brief [[ Add one-line brief description here ]]
 **
 ** [[ Add lengthier description here ]]
 **/

#include "CDMap.h"
#include "MString.h"
#include "Map.h"
#include "MockErrno.h"
#include "context/context.h"

using CVC4::context::Context;

#include <cxxtest/TestSuite.h>
class CDMapTestSuite : public CxxTest::TestSuite
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

    void test_brackets()
    {
        Context ctx;
        CDMap<unsigned, String> hashMap( &ctx );

        hashMap[1] = "Apple";
        hashMap[2] = "Red";
        hashMap[3] = "Tasty";

        TS_ASSERT_EQUALS( hashMap.get( 1 ), "Apple" );
        TS_ASSERT_EQUALS( hashMap.get( 2 ), "Red" );
        TS_ASSERT_EQUALS( hashMap.get( 3 ), "Tasty" );

        hashMap[3] = "otherBla";
        TS_ASSERT_EQUALS( hashMap.get( 3 ), "otherBla" );
    }

    void test_size_and_exists()
    {
        Context ctx;
        CDMap<unsigned, String> hashMap( &ctx );

        TS_ASSERT_EQUALS( hashMap.size(), 0u );
        TS_ASSERT( hashMap.empty() );
        TS_ASSERT( !hashMap.exists( 1 ) );

        hashMap[1] = "Apple";
        TS_ASSERT_EQUALS( hashMap.size(), 1u );
        TS_ASSERT( !hashMap.empty() );
        TS_ASSERT( hashMap.exists( 1 ) );

        ctx.push();

        hashMap[2] = "Red";
        TS_ASSERT_EQUALS( hashMap.size(), 2u );
        TS_ASSERT( hashMap.exists( 1 ) );
        TS_ASSERT( hashMap.exists( 2 ) );
        TS_ASSERT( !hashMap.exists( 3 ) );

        ctx.pop();

        TS_ASSERT_EQUALS( hashMap.size(), 1u );
        TS_ASSERT( !hashMap.empty() );
        TS_ASSERT( hashMap.exists( 1 ) );
        TS_ASSERT( !hashMap.exists( 2 ) );
        TS_ASSERT( !hashMap.exists( 3 ) );
    }
};
