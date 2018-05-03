/*********************                                                        */
/*! \file Test_CompareFactorizations.h
** \verbatim
** Top contributors (to current version):
**   Derek Huang
**   Guy Katz
** This file is part of the Marabou project.
** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
** in the top-level source directory) and their institutional affiliations.
** All rights reserved. See the file COPYING in the top-level source
** directory for licensing information.\endverbatim
**/

#include <cxxtest/TestSuite.h>

#include "FloatUtils.h"
#include "ForrestTomlinFactorization.h"
#include "LUFactorization.h"
#include "MockErrno.h"

class MockForCompareFactorizations
{
public:
};

class CompareFactorizationsTestSuite : public CxxTest::TestSuite
{
public:
    MockForCompareFactorizations *mock;

    void setUp()
    {
        TS_ASSERT( mock = new MockForCompareFactorizations );
    }

    void tearDown()
    {
        TS_ASSERT_THROWS_NOTHING( delete mock );
    }

    void test_compare_ft_and_lu()
    {
        ForrestTomlinFactorization *ft;
        LUFactorization *lu;

        TS_ASSERT( ft = new ForrestTomlinFactorization( 4 ) );
        TS_ASSERT( lu = new LUFactorization( 4 ) );

        double y[4] = { 9, 15, 10, -12 };
        double x1[4];
        double x2[4];

        TS_ASSERT_THROWS_NOTHING( ft->forwardTransformation( y, x1 ) );
        TS_ASSERT_THROWS_NOTHING( lu->forwardTransformation( y, x2 ) );

        for ( unsigned i = 0; i < 4; ++i )
            TS_ASSERT( FloatUtils::areEqual( x1[i], x2[i] ) );

        double d1[] = { -4, 2, 0, 3 };
        ft->pushEtaMatrix( 1, d1 );
        lu->pushEtaMatrix( 1, d1 );

        TS_ASSERT_THROWS_NOTHING( ft->forwardTransformation( y, x1 ) );
        TS_ASSERT_THROWS_NOTHING( lu->forwardTransformation( y, x2 ) );

        for ( unsigned i = 0; i < 4; ++i )
            TS_ASSERT( FloatUtils::areEqual( x1[i], x2[i] ) );

        double d2[] = { 3.2, -2, 10, 3 };
        ft->pushEtaMatrix( 2, d2 );
        lu->pushEtaMatrix( 2, d2 );

        TS_ASSERT_THROWS_NOTHING( ft->forwardTransformation( y, x1 ) );
        TS_ASSERT_THROWS_NOTHING( lu->forwardTransformation( y, x2 ) );

        for ( unsigned i = 0; i < 4; ++i )
            TS_ASSERT( FloatUtils::areEqual( x1[i], x2[i] ) );

        ft->makeExplicitBasisAvailable();

        TS_ASSERT_THROWS_NOTHING( ft->forwardTransformation( y, x1 ) );
        TS_ASSERT_THROWS_NOTHING( lu->forwardTransformation( y, x2 ) );

        for ( unsigned i = 0; i < 4; ++i )
            TS_ASSERT( FloatUtils::areEqual( x1[i], x2[i] ) );

        lu->makeExplicitBasisAvailable();

        TS_ASSERT_THROWS_NOTHING( ft->forwardTransformation( y, x1 ) );
        TS_ASSERT_THROWS_NOTHING( lu->forwardTransformation( y, x2 ) );

        for ( unsigned i = 0; i < 4; ++i )
            TS_ASSERT( FloatUtils::areEqual( x1[i], x2[i] ) );

        TS_ASSERT_THROWS_NOTHING( delete lu );
        TS_ASSERT_THROWS_NOTHING( delete ft );
    }
};

//
// Local Variables:
// compile-command: "make -C ../../.. "
// tags-file-name: "../../../TAGS"
// c-basic-offset: 4
// End:
//
