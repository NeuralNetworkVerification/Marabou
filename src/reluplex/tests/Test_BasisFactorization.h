/*********************                                                        */
/*! \file Test_BasisFactorization.h
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

    void setUp()
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

    void test_backward_transformation_2()
    {
        BasisFactorization basis( 3 );

        // E1 = | -1     |
        //      |  0 1   |
        //      | -1   1 |
        double e1[] = { -1, 0, -1 };
        basis.pushEtaMatrix( 0, e1 );

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

    void test_store_and_restore()
    {
        BasisFactorization basis( 3 );
        BasisFactorization otherBasis( 3 );

        double a1[] = { 1, 1, 3 };
        double d1[] = { 0, 0, 0 };

        TS_ASSERT_THROWS_NOTHING( basis.forwardTransformation( a1, d1 ) );
        basis.pushEtaMatrix( 1, a1 );

        basis.storeFactorization( &otherBasis );

        // Do a computation using both basis, see that we get the same result.

        double a2[] = { 3, 1, 4 };
        double d2[] = { 0, 0, 0 };
        double d2other[] = { 0, 0, 0 };
        double expected2[] = { 2, 1, 1 };

        TS_ASSERT_THROWS_NOTHING( basis.forwardTransformation( a2, d2 ) );
        TS_ASSERT_THROWS_NOTHING( otherBasis.forwardTransformation( a2, d2other ) );

        TS_ASSERT_SAME_DATA( d2, expected2, sizeof(double) * 3 );
        TS_ASSERT_SAME_DATA( d2other, expected2, sizeof(double) * 3 );

        // Transform the new basis but not the original

        otherBasis.pushEtaMatrix( 0, d2 );

        double a3[] = { 2, 1, 4 };
        double d3[] = { 0, 0, 0 };
        double d3other[] = { 0, 0, 0 };
        double expected3[] = { 0.5, 0.5, 0.5 };

        TS_ASSERT_THROWS_NOTHING( otherBasis.forwardTransformation( a3, d3other ) );
        TS_ASSERT_SAME_DATA( d3other, expected3, sizeof(double) * 3 );

        // The original basis wasn't modified, so the result should be different

        TS_ASSERT_THROWS_NOTHING( basis.forwardTransformation( a3, d3 ) );
        TS_ASSERT( memcmp( d3other, d3, sizeof(double) * 3 ) );
    }
		void test_factorization_pivot()//pivot
	{
		BasisFactorization basis ( 3 );
		int nsq = 9;
	    int n = 3;
	    queue <double*> LP;
	    double A[nsq]={0.,1.,0.,
						1.,8.,0.,
						4.,-3.,3.};

		double U[nsq]={0.,1.,0.,
						1.,8.,0.,
						4.,-3.,3.};

		basis.factorization ( n, A, LP );
		double temp[nsq] = {0.};
		while (!LP.empty()){
			basis.matrixMultiply(n, LP.front(), U, temp);
			LP.pop();
			memcpy(U, temp, sizeof(double) * 9);
		}
		TS_ASSERT_SAME_DATA ( A, U, sizeof(double) * 9 );
	}
	void test_factorization_textbook()//textbook
	{
		BasisFactorization basis ( 4 );
        int nsq = 16;
        int n = 4;
        queue <double*> LP;
        double A[nsq]= {1., 3., -2., 4.,
						1., 5., -1., 5., 
						1., 3., -3., 6.,
						-1., -3., 3., -8.};

        double U[nsq]= {1., 3., -2., 4.,
						1., 5., -1., 5., 
						1., 3., -3., 6.,
						-1., -3., 3., -8.};

        basis.factorization ( n, A, LP );
        double temp[nsq] = {0.};
        while (!LP.empty()){
            basis.matrixMultiply(n, LP.front(), U, temp);
            LP.pop();                   
            memcpy(U, temp, sizeof(double) * 16);                
        }                                                                               
        TS_ASSERT_SAME_DATA ( A, U, sizeof(double) * 16 );
	}
};

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
