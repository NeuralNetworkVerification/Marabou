/*********************                                                        */
/*! \file Test_SparseLUFactorization.h
** \verbatim
** Top contributors (to current version):
**   Guy Katz
** This file is part of the Marabou project.
** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
** in the top-level source directory) and their institutional affiliations.
** All rights reserved. See the file COPYING in the top-level source
** directory for licensing information.\endverbatim
**/

#include <cxxtest/TestSuite.h>

#include "BasisFactorizationError.h"
#include "EtaMatrix.h"
#include "FloatUtils.h"
#include "GlobalConfiguration.h"
#include "SparseLUFactorization.h"
#include "List.h"
#include "MockColumnOracle.h"
#include "MockErrno.h"

class MockForSparseLUFactorization
{
public:
};

class SparseLUFactorizationTestSuite : public CxxTest::TestSuite
{
public:
    MockForSparseLUFactorization *mock;
    MockColumnOracle *oracle;

    void setUp()
    {
        TS_ASSERT( mock = new MockForSparseLUFactorization );
        TS_ASSERT( oracle = new MockColumnOracle );
    }

    void tearDown()
    {
        TS_ASSERT_THROWS_NOTHING( delete oracle );
        TS_ASSERT_THROWS_NOTHING( delete mock );
    }

    void test_forward_transformation()
    {
        SparseLUFactorization basis( 3, *oracle );

        double B[] = {
            1, 0, 0,
            0, 1, 0,
            0, 0, 1,
        };
        oracle->storeBasis( 3, B );
        basis.obtainFreshBasis();

        // If no eta matrices are provided, d = a
        double a1[] = { 1, 1, 3 };
        double d1[] = { 0, 0, 0 };
        double expected1[] = { 1, 1, 3 };

        TS_ASSERT_THROWS_NOTHING( basis.forwardTransformation( a1, d1 ) );
        TS_ASSERT_SAME_DATA( d1, expected1, sizeof(double) * 3 );

        // E1 = | 1 1   |
        //      |   1   |
        //      |   3 1 |
        basis.updateToAdjacentBasis( 1, a1, NULL );

        double a2[] = { 3, 1, 4 };
        double d2[] = { 0, 0, 0 };
        double expected2[] = { 2, 1, 1 };

        //  | 1 1   |        | 3 |
        //  |   1   | * d =  | 1 |
        //  |   3 1 |        | 4 |
        //
        // --> d = [ 2 1 1 ]^T

        TS_ASSERT_THROWS_NOTHING( basis.forwardTransformation( a2, d2 ) );
        TS_ASSERT_SAME_DATA( d2, expected2, sizeof(double) * 3 );

        // E2 = | 2     |
        //      | 1 1   |
        //      | 1   1 |
        basis.updateToAdjacentBasis( 0, d2, NULL );

        double a3[] = { 2, 1, 4 };
        double d3[] = { 0, 0, 0 };
        double expected3[] = { 0.5, 0.5, 0.5 };

        //  | 1 1   |   | 2     |       | 2 |
        //  |   1   | * | 1 1   | * d = | 1 |
        //  |   3 1 |   | 1   1 |       | 4 |
        //
        // --> d = [ 0.5 0.5 0.5 ]^T

        TS_ASSERT_THROWS_NOTHING( basis.forwardTransformation( a3, d3 ) );
        TS_ASSERT_SAME_DATA( d3, expected3, sizeof(double) * 3 );
    }

    void test_forward_transformation_with_B0()
    {
        // Same etas as test_backward_transformation()
        SparseLUFactorization basis( 3, *oracle );

        double e1[] = { 1, 1, 3 };
        basis.updateToAdjacentBasis( 1, e1, NULL );

        double e2[] = { 2, 1, 1 };
        basis.updateToAdjacentBasis ( 0, e2, NULL );

        double e3[] = { 0.5, 0.5, 0.5 };
        basis.updateToAdjacentBasis( 2, e3, NULL );

        double B[] = {
            1, 2, 4,
            4, 5, 7,
            7, 8, 9
        };
        oracle->storeBasis( 3, B );
        basis.obtainFreshBasis();

        double a[] = { 2., -1., 4. };
        double d[] = { 0., 0., 0. };
        double expected[] = { -20, 27, -8 };

        basis.forwardTransformation( a, d );

        for ( unsigned i = 0; i < 3; ++i )
            TS_ASSERT( FloatUtils::areEqual( d[i], expected[i] ) );
    }

    void test_backward_transformation()
    {
        SparseLUFactorization basis( 3, *oracle );

        double B[] = {
            1, 0, 0,
            0, 1, 0,
            0, 0, 1,
        };
        oracle->storeBasis( 3, B );
        basis.obtainFreshBasis();

        // If no eta matrices are provided, x = y
        double y1[] = { 1, 2, 3 };
        double x1[] = { 0, 0, 0 };
        double expected1[] = { 1, 2, 3 };

        TS_ASSERT_THROWS_NOTHING( basis.backwardTransformation( y1, x1 ) );
        TS_ASSERT_SAME_DATA( x1, expected1, sizeof(double) * 3 );

        // E1 = | 1 1   |
        //      |   1   |
        //      |   3 1 |
        double e1[] = { 1, 1, 3 };
        basis.updateToAdjacentBasis( 1, e1, NULL );

        double y2[] = { 0, 12, 0 };
        double x2[] = { 0, 0, 0 };
        double expected2[] = { 0, 12, 0 };

        //     | 1 1   |
        // x * |   1   | = | 0 12 0 |
        //     |   3 1 |
        //
        // --> x = [ 0 12 0 ]

        TS_ASSERT_THROWS_NOTHING( basis.backwardTransformation( y2, x2 ) );
        TS_ASSERT_SAME_DATA( x2, expected2, sizeof(double) * 3 );

        // E2 = | 2     |
        //      | 1 1   |
        //      | 1   1 |
        double e2[] = { 2, 1, 1 };
        basis.updateToAdjacentBasis( 0, e2, NULL );

        double y3[] = { 19, 12, 0 };
        double x3[] = { 0, 0, 0 };
        double expected3[] = { 3.5, 8.5, 0 };

        //      | 1 1   |   | 2     |
        //  x * |   1   | * | 1 1   | = | 19 12 0 |
        //      |   3 1 |   | 1   1 |
        //
        // --> x = [ 3.5 8.5 0 ]

        TS_ASSERT_THROWS_NOTHING( basis.backwardTransformation( y3, x3 ) );
        TS_ASSERT_SAME_DATA( x3, expected3, sizeof(double) * 3 );

        // E3 = | 1   0.5 |
        //      |   1 0.5 |
        //      |     0.5 |
        double e3[] = { 0.5, 0.5, 0.5 };
        basis.updateToAdjacentBasis( 2, e3, NULL );

        double y4[] = { 19, 12, 17 };
        double x4[] = { 0, 0, 0 };
        double expected4[] = { 2, 1, 3 };

        //      | 1 1   |   | 2     |   | 1   0.5 |
        //  x * |   1   | * | 1 1   | * |   1 0.5 | = | 19 12 0 |
        //      |   3 1 |   | 1   1 |   |     0.5 |
        //
        // --> x = [ 2 1 3 ]
        TS_ASSERT_THROWS_NOTHING( basis.backwardTransformation( y4, x4 ) );
        TS_ASSERT_SAME_DATA( x4, expected4, sizeof(double) * 3 );
    }

    void test_backward_transformation_2()
    {
        SparseLUFactorization basis( 3, *oracle );

        double B[] = {
            1, 0, 0,
            0, 1, 0,
            0, 0, 1,
        };
        oracle->storeBasis( 3, B );
        basis.obtainFreshBasis();

        // E1 = | -1     |
        //      |  0 1   |
        //      | -1   1 |
        double e1[] = { -1, 0, -1 };
        basis.updateToAdjacentBasis( 0, e1, NULL );

        double y[] = { 1, 0, -1 };
        double x[] = { 0, 0, 0 };
        double expected[] = { 0, 0, -1 };

        //     | -1     |
        // x * |  0 1   | = | 1 0 -1 |
        //     | -1   1 |
        //
        // --> x = [ 0 0 -1 ]

        TS_ASSERT_THROWS_NOTHING( basis.backwardTransformation( y, x ) );
        TS_ASSERT_SAME_DATA( x, expected, sizeof(double) * 3 );
    }

    void test_backward_transformation_with_B0()
    {
        // Same etas as test_backward_transformation()
        SparseLUFactorization basis( 3, *oracle );

        double e1[] = { 1, 1, 3 };
        basis.updateToAdjacentBasis( 1, e1, NULL );

        double e2[] = { 2, 1, 1 };
        basis.updateToAdjacentBasis( 0, e2, NULL );

        double e3[] = { 0.5, 0.5, 0.5 };
        basis.updateToAdjacentBasis( 2, e3, NULL );

        double B[] = {
            1, 2, 4,
            4, 5, 7,
            7, 8, 9,
        };
        oracle->storeBasis( 3, B );
        basis.obtainFreshBasis();

        double y[] = { 19, 12, 17 };
        double x[] = { 0, 0, 0 };
        double expected[] = { -104.0/3, 140.0/3, -19 };

        //     	| 1 2 4	|
        //  x *	| 4	5 7 | = | 19 12 17 |
        //     	| 7 8 9	|
        //
        // --> x = [ -104/3, 140/3, -19 ]
        basis.backwardTransformation( y, x );

        for ( unsigned i = 0; i < 3; ++i )
            TS_ASSERT( FloatUtils::areEqual( x[i], expected[i] ) );
    }

    void test_store_and_restore()
    {
        SparseLUFactorization basis( 3, *oracle );
        SparseLUFactorization otherBasis( 3, *oracle );

        double B[] = {
            1, 0, 0,
            0, 1, 0,
            0, 0, 1,
        };
        oracle->storeBasis( 3, B );
        basis.obtainFreshBasis();

        double a1[] = { 1, 1, 3 };
        double d1[] = { 0, 0, 0 };

        TS_ASSERT_THROWS_NOTHING( basis.forwardTransformation( a1, d1 ) );
        basis.updateToAdjacentBasis( 1, a1, NULL );

        // Save the expected basis after this push
        double currentBasis[] = {
            1, 1, 0,
            0, 1, 0,
            0, 3, 1
        };
        oracle->storeBasis( 3, currentBasis );

        // Do a computation using both basis, see that we get the same result.

        double a2[] = { 3, 1, 4 };
        double d2[] = { 0, 0, 0 };
        double d2other[] = { 0, 0, 0 };
        double expected2[] = { 2, 1, 1 };

        // First see that storing the basis doesn't destroy the original
        TS_ASSERT_THROWS_NOTHING( basis.forwardTransformation( a2, d2 ) );
        for ( unsigned i = 0; i < 3; ++i )
            TS_ASSERT( FloatUtils::areEqual( expected2[i], d2[i] ) );

        basis.storeFactorization( &otherBasis );
        TS_ASSERT_THROWS_NOTHING( basis.forwardTransformation( a2, d2 ) );
        for ( unsigned i = 0; i < 3; ++i )
            TS_ASSERT( FloatUtils::areEqual( expected2[i], d2[i] ) );

        // Then see that the other basis produces the same result
        TS_ASSERT_THROWS_NOTHING( otherBasis.forwardTransformation( a2, d2other ) );

        for ( unsigned i = 0; i < 3; ++i )
            TS_ASSERT( FloatUtils::areEqual( expected2[i], d2other[i] ) );

        // Transform the new basis but not the original
        otherBasis.updateToAdjacentBasis( 0, d2, NULL );

        double a3[] = { 2, 1, 4 };
        double d3[] = { 0, 0, 0 };
        double d3other[] = { 0, 0, 0 };
        double expected3[] = { 0.5, 0.5, 0.5 };

        TS_ASSERT_THROWS_NOTHING( otherBasis.forwardTransformation( a3, d3other ) );

        for ( unsigned i = 0; i < 3; ++i )
            TS_ASSERT( FloatUtils::areEqual( expected3[i], d3other[i] ) );

        // The original basis wasn't modified, so the result should be different

        TS_ASSERT_THROWS_NOTHING( basis.forwardTransformation( a3, d3 ) );
        TS_ASSERT( memcmp( d3other, d3, sizeof(double) * 3 ) );
    }
};

//
// Local Variables:
// compile-command: "make -C ../../.. "
// tags-file-name: "../../../TAGS"
// c-basic-offset: 4
// End:
//
