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
#include <iostream>
#include "BasisFactorization.h"
#include <string.h>
#include <vector>
#include <EtaMatrix.h>
#include "List.h"
#include "FloatUtils.h"
#include "GlobalConfiguration.h"

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

	void test_forward_transformation_with_B0() //Same etas as test_backward_transformation()
	{
		BasisFactorization basis( 3 );
		double e1[] = {1., 1., 3.};
		basis.pushEtaMatrix( 1, e1 );
		double e2[] = {2., 1., 1.};
		basis.pushEtaMatrix ( 0, e2 );
		double e3[] = { 0.5, 0.5, 0.5 };
		basis.pushEtaMatrix ( 2, e3 );

		double nB0[] = {1.,2.,4.,4.,5.,7.,7.,8.,9.};
		basis.setB0( nB0 );

		double a[] = {2., -1., 4.};
		double d[] = {0., 0., 0.};
		double expected[] = {42, 116, -131};
		basis.forwardTransformation (a, d);
		TS_ASSERT_SAME_DATA ( d, expected, sizeof(double) * 3 );
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

	void test_backward_transformation_with_B0() //Same etas as test_backward_transformation()
	{
		BasisFactorization basis ( 3 );
		double e1[] = {1., 1., 3.};
		basis.pushEtaMatrix ( 1, e1 );
		double e2[] = {2., 1., 1.};
		basis.pushEtaMatrix ( 0, e2 );
		double e3[] = { 0.5, 0.5, 0.5 };
        basis.pushEtaMatrix( 2, e3 );

		double nB0[] = {1.,2.,4.,4.,5.,7.,7.,8.,9};
		basis.setB0 ( nB0 );

		double y[] = {19., 12., 17.};
		double x[] = {0., 0., 0.};
		double expected[] = {-6, 9, -4};
		//     	| 1 2 4	|  	| 1 1   |   | 2     |   | 1   0.5 |
        //  x *	| 4	5 7 | * |   1   | * | 1	1	| *	|	1 0.5 | = | 19 12 0 |
        //     	| 7 8 9	|	|   3 1 |   | 1   1 |   |     0.5 |
        //
        // --> x = [ -6 9 -4 ]
		basis.backwardTransformation (y, x);
		TS_ASSERT_SAME_DATA ( x, expected, sizeof(double) * 3 );
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
		BasisFactorization basis( 3 );
		const int nsq = 9;

	    double A[nsq]= {0., 1. , 0.,
						1., 8. , 0.,
						4., -3., 3. };

		double U[nsq]={ 0., 1. , 0.,
						1., 8. , 0.,
						4., -3., 3. };

		basis.factorizeMatrix( A );
        const List<LPElement *> lp = basis.getLP();
        for ( auto element = lp.rbegin(); element != lp.rend(); ++element )
        {
            if ( (*element)->_pair )
                basis.rowSwap( (*element)->_pair->first, (*element)->_pair->second, U );
            else
            {
                (*element)->_eta->dump();
                double temp[9];
                double eta[9];
                std::fill_n( eta, 9, 0. );
                (*element)->_eta->toMatrix(eta);
                basis.matrixMultiply( 3, eta, U, temp );
                memcpy( U, temp, sizeof(double) * 9 );
            }
		}

		for ( unsigned i = 0; i < 9; ++i )
        {
			TS_ASSERT( FloatUtils::areEqual( U[i], basis.getU()[i] ) );
        }
	}

	void test_factorization_textbook()//textbook
	{
		BasisFactorization basis ( 4 );
        const int nsq = 16;
        double A[nsq]= {1., 3., -2., 4.,
						1., 5., -1., 5.,
						1., 3., -3., 6.,
						-1., -3., 3., -8.};

        double U[nsq]= {1., 3., -2., 4.,
						0.,1.,.5,.5,
						0.,0.,1.,-2.,
						0.,0.,0.,1.};

        basis.factorizeMatrix( A );
    		for (int i = 0; i < 9; ++i) {
			TS_ASSERT (FloatUtils::areEqual( basis.getU()[i], U[i]) );
		}
	}

	void test_factorization_box()
	{
		BasisFactorization basis ( 3 );
        const int nsq = 9;
        double A[nsq]= { 1., 2., 4.,
                         4., 5., 7.,
                         7., 8., 9.};

		double U[nsq]={ 1., 2., 4.,
                        4., 5., 7.,
                        7., 8., 9. };

		basis.factorizeMatrix( A );

        const List<LPElement *> lp = basis.getLP();
        for ( auto element = lp.rbegin(); element != lp.rend(); ++element )
        {
            if ( (*element)->_pair )
                basis.rowSwap( (*element)->_pair->first, (*element)->_pair->second, U );
            else
            {
                double temp[9];
                double eta[9];
                std::fill_n( eta, 9, 0. );
                (*element)->_eta->toMatrix(eta);
                basis.matrixMultiply( 3, eta, U, temp );
                memcpy( U, temp, sizeof(double) * 9);
            }
		}

        for ( unsigned i = 0; i < 9; ++i )
			TS_ASSERT( FloatUtils::areEqual( U[i], basis.getU()[i] ) );
	}

	void test_refactor()
	{
		BasisFactorization basis ( 3 );
		BasisFactorization bas2 ( 3 );
		basis.toggleFactorization( false );
		int d = 3;
		unsigned etaCount = 12;
		//generate random etas

		for (unsigned i = 0; i < etaCount; ++i) {
			double eta_col[d];
			std::fill_n(eta_col, d, 0.);
			int col = rand() % d;
			for (int j = 0; j < d; ++j) {
					eta_col[j] = rand() % (13) - 5;
			}
			basis.pushEtaMatrix( col, eta_col );
			bas2.pushEtaMatrix( col, eta_col );
		}

		//check if etas have disappeared
		TS_ASSERT_EQUALS( bas2.getEtas().size(), etaCount - GlobalConfiguration::REFACTORIZATION_THRESHOLD - 1 );
		double a[] = {2., -1., 4.};
		double x1[] = {0., 0., 0.};
		double y1[] = {0., 0., 0.};
		double x2[] = {0., 0., 0.};
		double y2[] = {0., 0., 0.};
		bas2.forwardTransformation( a, x1 );
		basis.forwardTransformation( a, y1 );
		bas2.backwardTransformation( a, x2 );
		basis.backwardTransformation( a, y2 );
		for (int i = 0; i < 3; ++i) {
			TS_ASSERT (FloatUtils::areEqual( x1[i], y1[i]) );
			TS_ASSERT (FloatUtils::areEqual( x2[i], y2[i] ) );
		}
	}

    void test_matrix_multiply()
    {
        double left[] =
            {
                1 , 2, 0,
                -1, 3, 4,
                0 , 0, 2
            };

        double right[] =
            {
                0, -1, 2,
                3, 0 , 0,
                1, 1 , -2
            };

        double expectedResult[] =
            {
                6, -1, 2,
                13, 5, -10,
                2, 2, -4
            };

        double result[9];

        TS_ASSERT_THROWS_NOTHING( BasisFactorization::matrixMultiply( 3, left, right, result ) );
        TS_ASSERT_SAME_DATA( result, expectedResult, sizeof(double) * 9 );
    }
};

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
