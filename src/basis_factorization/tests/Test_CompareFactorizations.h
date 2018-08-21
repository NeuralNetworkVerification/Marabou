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
#include "MockColumnOracle.h"
#include "MockErrno.h"
#include "SparseFTFactorization.h"
#include "SparseLUFactorization.h"

class MockForCompareFactorizations
{
public:
};

class CompareFactorizationsTestSuite : public CxxTest::TestSuite
{
public:
    MockForCompareFactorizations *mock;
    MockColumnOracle *oracle;

    void setUp()
    {
        TS_ASSERT( mock = new MockForCompareFactorizations );
        TS_ASSERT( oracle = new MockColumnOracle );
    }

    void tearDown()
    {
        TS_ASSERT_THROWS_NOTHING( delete oracle );
        TS_ASSERT_THROWS_NOTHING( delete mock );
    }

    void test_compare_ft_and_lu()
    {
        ForrestTomlinFactorization *ft;
        LUFactorization *lu;
        SparseFTFactorization *sft;
        SparseLUFactorization *slu;

        TS_ASSERT( ft = new ForrestTomlinFactorization( 4, *oracle ) );
        TS_ASSERT( lu = new LUFactorization( 4, *oracle ) );
        TS_ASSERT( sft = new SparseFTFactorization( 4, *oracle ) );
        TS_ASSERT( slu = new SparseLUFactorization( 4, *oracle ) );

        double B[] =
            {
                2, 0, 3, -4,
                0, 1, 10, 0,
                -3, 4.5, 1, 1,
                0, 0, 2, 2
            };

        TS_ASSERT_THROWS_NOTHING( ft->setBasis( B ) );
        TS_ASSERT_THROWS_NOTHING( lu->setBasis( B ) );
        TS_ASSERT_THROWS_NOTHING( sft->setBasis( B ) );
        TS_ASSERT_THROWS_NOTHING( slu->setBasis( B ) );

        double y[4] = { 9, 15, 10, -12 };
        double x1[4];
        double x2[4];
        double x3[4];
        double x4[4];

        TS_ASSERT_THROWS_NOTHING( ft->forwardTransformation( y, x1 ) );
        TS_ASSERT_THROWS_NOTHING( lu->forwardTransformation( y, x2 ) );
        TS_ASSERT_THROWS_NOTHING( sft->forwardTransformation( y, x3 ) );
        TS_ASSERT_THROWS_NOTHING( slu->forwardTransformation( y, x4 ) );

        for ( unsigned i = 0; i < 4; ++i )
        {
            TS_ASSERT( FloatUtils::areEqual( x1[i], x2[i] ) );
            TS_ASSERT( FloatUtils::areEqual( x1[i], x3[i] ) );
            TS_ASSERT( FloatUtils::areEqual( x1[i], x4[i] ) );
        }

        double d1[] = { -4, 2, 0, 3 };
        double a1[] = { 0, 1, 4.5, 0 };

        ft->updateToAdjacentBasis( 1, d1, a1 );
        lu->updateToAdjacentBasis( 1, d1, a1 );
        // sft->updateToAdjacentBasis( 1, d1, a1 );
        slu->updateToAdjacentBasis( 1, d1, a1 );

        TS_ASSERT_THROWS_NOTHING( ft->forwardTransformation( y, x1 ) );
        TS_ASSERT_THROWS_NOTHING( lu->forwardTransformation( y, x2 ) );
        // TS_ASSERT_THROWS_NOTHING( sft->forwardTransformation( y, x3 ) );
        TS_ASSERT_THROWS_NOTHING( slu->forwardTransformation( y, x4 ) );

        for ( unsigned i = 0; i < 4; ++i )
        {
            TS_ASSERT( FloatUtils::areEqual( x1[i], x2[i] ) );
            // TS_ASSERT( FloatUtils::areEqual( x1[i], x3[i] ) );
            TS_ASSERT( FloatUtils::areEqual( x1[i], x4[i] ) );
        }

        double d2[] = { 3.2, -2, 10, 3 };
        double a2[] = { -26.33, 10, -44, 0 };

        ft->updateToAdjacentBasis( 2, d2, a2 );
        lu->updateToAdjacentBasis( 2, d2, a2 );
        // sft->updateToAdjacentBasis( 2, d2, a2 );
        slu->updateToAdjacentBasis( 2, d2, a2 );

        TS_ASSERT_THROWS_NOTHING( ft->forwardTransformation( y, x1 ) );
        TS_ASSERT_THROWS_NOTHING( lu->forwardTransformation( y, x2 ) );
        // TS_ASSERT_THROWS_NOTHING( sft->forwardTransformation( y, x3 ) );
        TS_ASSERT_THROWS_NOTHING( slu->forwardTransformation( y, x4 ) );

        for ( unsigned i = 0; i < 4; ++i )
        {
            TS_ASSERT( FloatUtils::areEqual( x1[i], x2[i] ) );
            // TS_ASSERT( FloatUtils::areEqual( x1[i], x3[i] ) );
            TS_ASSERT( FloatUtils::areEqual( x1[i], x4[i] ) );
        }

        double basisAtThisPoint[] = {
            2, -20, 64, -4,
            0, 2, 96, 0,
            -3, 24, -45, 1,
            0, 6, 14, 2,
        };

        oracle->storeBasis( 4, basisAtThisPoint );

        ft->makeExplicitBasisAvailable();
        lu->makeExplicitBasisAvailable();
        sft->makeExplicitBasisAvailable();
        slu->makeExplicitBasisAvailable();

        TS_ASSERT_THROWS_NOTHING( ft->forwardTransformation( y, x1 ) );
        TS_ASSERT_THROWS_NOTHING( lu->forwardTransformation( y, x2 ) );
        TS_ASSERT_THROWS_NOTHING( sft->forwardTransformation( y, x3 ) );
        TS_ASSERT_THROWS_NOTHING( slu->forwardTransformation( y, x4 ) );

        for ( unsigned i = 0; i < 4; ++i )

        for ( unsigned i = 0; i < 4; ++i )
        {
            TS_ASSERT( FloatUtils::areEqual( x1[i], x2[i] ) );
            TS_ASSERT( FloatUtils::areEqual( x1[i], x3[i] ) );
            TS_ASSERT( FloatUtils::areEqual( x1[i], x4[i] ) );
        }

        lu->makeExplicitBasisAvailable();

        TS_ASSERT_THROWS_NOTHING( ft->forwardTransformation( y, x1 ) );
        TS_ASSERT_THROWS_NOTHING( lu->forwardTransformation( y, x2 ) );
        TS_ASSERT_THROWS_NOTHING( sft->forwardTransformation( y, x3 ) );
        TS_ASSERT_THROWS_NOTHING( slu->forwardTransformation( y, x4 ) );

        for ( unsigned i = 0; i < 4; ++i )
        {
            TS_ASSERT( FloatUtils::areEqual( x1[i], x2[i] ) );
            TS_ASSERT( FloatUtils::areEqual( x1[i], x3[i] ) );
            TS_ASSERT( FloatUtils::areEqual( x1[i], x4[i] ) );
        }

        TS_ASSERT_THROWS_NOTHING( delete lu );
        TS_ASSERT_THROWS_NOTHING( delete ft );
        TS_ASSERT_THROWS_NOTHING( delete slu );
        TS_ASSERT_THROWS_NOTHING( delete sft );
    }
};

//
// Local Variables:
// compile-command: "make -C ../../.. "
// tags-file-name: "../../../TAGS"
// c-basic-offset: 4
// End:
//
