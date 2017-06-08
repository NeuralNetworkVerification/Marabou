#include <cxxtest/TestSuite.h>

#include "MockErrno.h"
#include "BasisFactorization.h"
#include <string.h>

class MockForBasisFactorization
{
public:
};

class BasisFactorizationTestSuite : public CxxTest::TestSuite
{
public:
    MockForBasisFactorization *mock;

    void basisFactorizationUp()
    {
        TS_ASSERT( mock = new MockForBasisFactorization );
    }

    void tearDown()
    {
        TS_ASSERT_THROWS_NOTHING( delete mock );
    }

    void test_forward_transformation()
    {
        BasisFactorization basis( 3 );

        // If no eta matrices are provided, d = a
        double a1[] = { 1, 1, 3 };
        double d1[] = { 0, 0, 0 };
        double expected1[] = { 1, 1, 3 };

        TS_ASSERT_THROWS_NOTHING( basis.forwardTransformation( a1, d1 ) );
        TS_ASSERT_SAME_DATA( d1, expected1, sizeof(double) * 3 );

        // E1 = | 1 1   |
        //      |   1   |
        //      |   3 1 |
        basis.pushEtaMatrix( 1, a1 );

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
        basis.pushEtaMatrix( 0, d2 );

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

    void test_backward_transformation()
    {
        BasisFactorization basis( 3 );

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
        basis.pushEtaMatrix( 1, e1 );

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
        basis.pushEtaMatrix( 0, e2 );

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
        basis.pushEtaMatrix( 2, e3 );

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
};

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
