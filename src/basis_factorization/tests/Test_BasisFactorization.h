/*********************                                                        */
/*! \file Test_BasisFactorization.h
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

#include "BasisFactorization.h"
#include "EtaMatrix.h"
#include "FloatUtils.h"
#include "GlobalConfiguration.h"
#include "List.h"
#include "MockErrno.h"
#include "BasisFactorizationError.h"

void matrixMultiply( unsigned dimension, const double *left, const double *right, double *result )
{
    for ( unsigned leftRow = 0; leftRow < dimension; ++leftRow )
    {
        for ( unsigned rightCol = 0; rightCol < dimension; ++rightCol )
        {
            double sum = 0;
			for ( unsigned i = 0; i < dimension; ++i )
				sum += left[leftRow * dimension + i] * right[i * dimension + rightCol];

			result[leftRow * dimension + rightCol] = sum;
		}
	}
}

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

	void test_forward_transformation_with_B0()
	{
        // Same etas as test_backward_transformation()
		BasisFactorization basis( 3 );
		double e1[] = {1., 1., 3.};
		basis.pushEtaMatrix( 1, e1 );
		double e2[] = {2., 1., 1.};
		basis.pushEtaMatrix ( 0, e2 );
		double e3[] = { 0.5, 0.5, 0.5 };
		basis.pushEtaMatrix( 2, e3 );

		double nB0[] = { 1, 2, 4,
                         4, 5, 7,
                         7, 8, 9 };
		basis.setBasis( nB0 );

		double a[] = { 2., -1., 4. };
		double d[] = { 0., 0., 0. };
        double expected[] = { 42, 116, -131 };

		basis.forwardTransformation( a, d );

        for ( unsigned i = 0; i < 3; ++i )
            TS_ASSERT( FloatUtils::areEqual( d[i], expected[i] ) );
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

	void test_backward_transformation_with_B0()
	{
        // Same etas as test_backward_transformation()
		BasisFactorization basis( 3 );
		double e1[] = {1., 1., 3.};
		basis.pushEtaMatrix( 1, e1 );
		double e2[] = {2., 1., 1.};
		basis.pushEtaMatrix( 0, e2 );
		double e3[] = { 0.5, 0.5, 0.5 };
        basis.pushEtaMatrix( 2, e3 );

		double nB0[] = {1.,2.,4.,4.,5.,7.,7.,8.,9};
		basis.setBasis( nB0 );

		double y[] = {19., 12., 17.};
		double x[] = {0., 0., 0.};
		double expected[] = {-6, 9, -4};
		//     	| 1 2 4	|  	| 1 1   |   | 2     |   | 1   0.5 |
        //  x *	| 4	5 7 | * |   1   | * | 1	1	| *	|	1 0.5 | = | 19 12 17 |
        //     	| 7 8 9	|	|   3 1 |   | 1   1 |   |     0.5 |
        //
        // --> x = [ -6 9 -4 ]
		basis.backwardTransformation( y, x );

        for ( unsigned i = 0; i < 3; ++i )
            TS_ASSERT( FloatUtils::areEqual( x[i], expected[i] ) );
	}

    void test_store_and_restore()
    {
        BasisFactorization basis( 3 );
        BasisFactorization otherBasis( 3 );

        double a1[] = { 1, 1, 3 };
        double d1[] = { 0, 0, 0 };

        TS_ASSERT_THROWS_NOTHING( basis.forwardTransformation( a1, d1 ) );
        basis.pushEtaMatrix( 1, a1 );

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
        otherBasis.pushEtaMatrix( 0, d2 );

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

    void test_factorization_pivot()
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
                double temp[9];
                double eta[9];
                std::fill_n( eta, 9, 0. );
                (*element)->_eta->toMatrix(eta);
                matrixMultiply( 3, eta, U, temp );
                memcpy( U, temp, sizeof(double) * 9 );
            }
		}

		for ( unsigned i = 0; i < 9; ++i )
			TS_ASSERT( FloatUtils::areEqual( U[i], basis.getU()[i] ) );
	}

	void test_factorization_textbook()
	{
        // Textbook example
		BasisFactorization basis( 4 );
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
        for ( unsigned i = 0; i < 9; ++i )
			TS_ASSERT( FloatUtils::areEqual( basis.getU()[i], U[i] ) );

        const List<LPElement *> lps = basis.getLP();

        TS_ASSERT_EQUALS( lps.size(), 4U );

        EtaMatrix *eta;

        auto it = lps.rbegin();
        TS_ASSERT( (*it)->_eta );
        TS_ASSERT( !(*it)->_pair );
        eta = (*it)->_eta;
        TS_ASSERT_EQUALS( eta->_columnIndex, 0U );
        double expectedCol1[] = { 1, -1, -1, 1 };
        for ( unsigned i = 0; i < 4; ++i )
            TS_ASSERT( FloatUtils::areEqual( eta->_column[i], expectedCol1[i] ) );

        // Matrix is now:
        //     1  3  -2  4
        //     0  2  1   1
        //     0  0  -1  2
        //     0  0  1   -4
        ++it;
        TS_ASSERT( (*it)->_eta );
        TS_ASSERT( !(*it)->_pair );
        eta = (*it)->_eta;
        TS_ASSERT_EQUALS( eta->_columnIndex, 1U );
        double expectedCol2[] = { 0, 0.5, 0, 0 };
        for ( unsigned i = 0; i < 4; ++i )
            TS_ASSERT( FloatUtils::areEqual( eta->_column[i], expectedCol2[i] ) );

        // Matrix is now:
        //     1  3  -2  4
        //     0  1  0.5 0.5
        //     0  0  -1  2
        //     0  0  1   -4
        ++it;
        TS_ASSERT( (*it)->_eta );
        TS_ASSERT( !(*it)->_pair );
        eta = (*it)->_eta;
        TS_ASSERT_EQUALS( eta->_columnIndex, 2U );
        double expectedCol3[] = { 0, 0, -1, 1 };
        for ( unsigned i = 0; i < 4; ++i )
            TS_ASSERT( FloatUtils::areEqual( eta->_column[i], expectedCol3[i] ) );

        // Matrix is now:
        //     1  3  -2  4
        //     0  1  0.5 0.5
        //     0  0  1   -2
        //     0  0  0   -2
        ++it;
        TS_ASSERT( (*it)->_eta );
        TS_ASSERT( !(*it)->_pair );
        eta = (*it)->_eta;
        TS_ASSERT_EQUALS( eta->_columnIndex, 3U );
        double expectedCol4[] = { 0, 0, 0, -0.5 };
        for ( unsigned i = 0; i < 4; ++i )
            TS_ASSERT( FloatUtils::areEqual( eta->_column[i], expectedCol4[i] ) );
    }

	void test_factorization_as_black_box()
	{
		BasisFactorization basis( 3 );
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
                matrixMultiply( 3, eta, U, temp );
                memcpy( U, temp, sizeof(double) * 9);
            }
		}

        for ( unsigned i = 0; i < 9; ++i )
			TS_ASSERT( FloatUtils::areEqual( U[i], basis.getU()[i] ) );
	}

    void test_factorization_numerical_stability()
    {
        BasisFactorization basis( 3 );

        double A[] =
            {
                2, 4, 5,
                3, -1, 0,
                0, -10, -2,
            };

        basis.factorizeMatrix( A );
        const List<LPElement *> lps = basis.getLP();

        TS_ASSERT_EQUALS( lps.size(), 5U );

        EtaMatrix *eta;

        auto it = lps.rbegin();
        TS_ASSERT( !(*it)->_eta );
        TS_ASSERT( (*it)->_pair );
        TS_ASSERT_EQUALS( (*it)->_pair->first, 0U );
        TS_ASSERT_EQUALS( (*it)->_pair->second, 1U );

        // Matrix is now:
        //     3  -1   0
        //     2  4    5
        //     0  -10 -2

        ++it;
        TS_ASSERT( (*it)->_eta );
        TS_ASSERT( !(*it)->_pair );
        eta = (*it)->_eta;
        TS_ASSERT_EQUALS( eta->_columnIndex, 0U );
        double expectedCol1[] = { 1.0/3, -2.0/3, 0 };
        for ( unsigned i = 0; i < 3; ++i )
        {
            TS_ASSERT( FloatUtils::areEqual( eta->_column[i], expectedCol1[i] ) );
        }

        // Matrix is now:
        //     1 -1/3   0
        //     0  14/3  5
        //     0  -10  -2

        ++it;
        TS_ASSERT( !(*it)->_eta );
        TS_ASSERT( (*it)->_pair );
        TS_ASSERT_EQUALS( (*it)->_pair->first, 1U );
        TS_ASSERT_EQUALS( (*it)->_pair->second, 2U );

        // Matrix is now:
        //     1 -1/3   0
        //     0  -10  -2
        //     0  14/3  5

        ++it;
        TS_ASSERT( (*it)->_eta );
        TS_ASSERT( !(*it)->_pair );
        eta = (*it)->_eta;
        TS_ASSERT_EQUALS( eta->_columnIndex, 1U );
        double expectedCol2[] = { 0, -1.0/10, 14.0/30 };
        for ( unsigned i = 0; i < 3; ++i )
            TS_ASSERT( FloatUtils::areEqual( eta->_column[i], expectedCol2[i] ) );

        // Matrix is now:
        //     1 -1/3  0
        //     0  1   -2
        //     0  0   122/30

        ++it;
        TS_ASSERT( (*it)->_eta );
        TS_ASSERT( !(*it)->_pair );
        eta = (*it)->_eta;
        TS_ASSERT_EQUALS( eta->_columnIndex, 2U );
        double expectedCol3[] = { 0, 0, 30.0/122 };
        for ( unsigned i = 0; i < 3; ++i )
            TS_ASSERT( FloatUtils::areEqual( eta->_column[i], expectedCol3[i] ) );
    }

	void xtest_refactor()
	{
        // TODO: this test fails when the REFACTORIZATION_THRESHOLD is too great (> 10 or so).
        // Disabling for now.

		BasisFactorization basis( 3 );
		BasisFactorization basis2( 3 );
		basis.toggleFactorization( false );
		int d = 3;
		unsigned etaCount = GlobalConfiguration::REFACTORIZATION_THRESHOLD + 2;

        srand( time( 0 ) );

        const unsigned ETA_POOL_SIZE = 1000;

        double etaPool[ETA_POOL_SIZE];
        std::fill( etaPool, etaPool + ETA_POOL_SIZE, 0.0 );

        for ( unsigned i = 0; i < ETA_POOL_SIZE; ++i )
        {
            while ( fabs( etaPool[i] ) < 0.001 )
                etaPool[i] = (float)(rand()) / (float)(RAND_MAX);
        }

		// Generate random etas
		for ( unsigned i = 0; i < etaCount; ++i )
        {
            unsigned startingIndex = rand() % ( ETA_POOL_SIZE - 2 );
            double *etaCol = etaPool + startingIndex;
			int col = rand() % d;
			basis.pushEtaMatrix( col, etaCol );
			basis2.pushEtaMatrix( col, etaCol );
		}

		// Check if etas have disappeared
		TS_ASSERT_EQUALS( basis2.getEtas().size(), etaCount - GlobalConfiguration::REFACTORIZATION_THRESHOLD - 1 );
		double a[] = {2., -1., 4.};
		double x1[] = {0., 0., 0.};
		double y1[] = {0., 0., 0.};
		double x2[] = {0., 0., 0.};
		double y2[] = {0., 0., 0.};
		basis2.forwardTransformation( a, x1 );
		basis.forwardTransformation( a, y1 );
		basis2.backwardTransformation( a, x2 );
		basis.backwardTransformation( a, y2 );
		for ( unsigned i = 0; i < 3; ++i )
        {
			TS_ASSERT( FloatUtils::areEqual( x1[i], y1[i], 0.001 ) );
			TS_ASSERT( FloatUtils::areEqual( x2[i], y2[i], 0.001 ) );
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

        matrixMultiply( 3, left, right, result );
        TS_ASSERT_SAME_DATA( result, expectedResult, sizeof(double) * 9 );
    }

    void test_invert_B0_fail()
    {
        BasisFactorization basis( 3 );

        double B0[] = { 1, 0, 0,
                        0, 1, 0,
                        0, 0, 1 };

        basis.setBasis( B0 );

        double a1[] = { 1, 1, 3 };
        basis.pushEtaMatrix( 1, a1 );

        double result[9];

        TS_ASSERT_THROWS_EQUALS( basis.invertBasis( result ),
                                 const BasisFactorizationError &e,
                                 e.getCode(),
                                 BasisFactorizationError::CANT_INVERT_BASIS_BECAUSE_OF_ETAS );
    }

    void test_invert_B0()
    {
        BasisFactorization basis( 3 );

        {
            double B0[] = { 1, 0, 0,
                            0, 1, 0,
                            0, 0, 1 };

            double expectedInverse[] = { 1, 0, 0,
                                         0, 1, 0,
                                         0, 0, 1 };

            double result[9];

            basis.setBasis( B0 );
            TS_ASSERT_THROWS_NOTHING( basis.invertBasis( result ) );

            for ( unsigned i = 0; i < sizeof(result) / sizeof(double); ++i )
            {
                TS_ASSERT( FloatUtils::areEqual( result[i], expectedInverse[i] ) );
            }
        }

        {
            // No B0 set implicity means B0 is the identity matrix
            double expectedInverse[] = { 1, 0, 0,
                                         0, 1, 0,
                                         0, 0, 1 };

            double result[9];

            TS_ASSERT_THROWS_NOTHING( basis.invertBasis( result ) );

            for ( unsigned i = 0; i < sizeof(result) / sizeof(double); ++i )
            {
                TS_ASSERT( FloatUtils::areEqual( result[i], expectedInverse[i] ) );
            }
        }

        {
            double B0[] = { 3, 0, 0,
                            0, -4, 0,
                            0, 0, 2 };

            double expectedInverse[] = { 1.0 / 3, 0,     0,
                                         0,       -0.25, 0,
                                         0,       0,     0.5 };

            double result[9];

            basis.setBasis( B0 );
            TS_ASSERT_THROWS_NOTHING( basis.invertBasis( result ) );

            for ( unsigned i = 0; i < sizeof(result) / sizeof(double); ++i )
            {
                TSM_ASSERT( i, FloatUtils::areEqual( result[i], expectedInverse[i] ) );
            }
        }

        {
            double B0[] = { 2, 0, 3,
                            -1, 2, 1,
                            0, 3, 4 };

            double expectedInverse[] = {  5,  9, -6,
                                          4,  8, -5,
                                         -3, -6,  4 };

            double result[9];

            basis.setBasis( B0 );
            TS_ASSERT_THROWS_NOTHING( basis.invertBasis( result ) );

            for ( unsigned i = 0; i < sizeof(result) / sizeof(double); ++i )
            {
                TSM_ASSERT( i, FloatUtils::areEqual( result[i], expectedInverse[i] ) );
            }
        }

        {
            double B0[] = { 7, -3, -3,
                            -1, 1, 0,
                            -1, 0, 1 };

            double expectedInverse[] = { 1, 3, 3,
                                         1, 4, 3,
                                         1, 3, 4 };

            double result[9];

            basis.setBasis( B0 );
            TS_ASSERT_THROWS_NOTHING( basis.invertBasis( result ) );

            for ( unsigned i = 0; i < sizeof(result) / sizeof(double); ++i )
            {
                TSM_ASSERT( i, FloatUtils::areEqual( result[i], expectedInverse[i] ) );
            }

            basis.setBasis( expectedInverse );
            TS_ASSERT_THROWS_NOTHING( basis.invertBasis( result ) );

            for ( unsigned i = 0; i < sizeof(result) / sizeof(double); ++i )
            {
                TSM_ASSERT( i, FloatUtils::areEqual( result[i], B0[i] ) );
            }
        }

        {
            BasisFactorization basis( 4 );

            double B0[] = { 1, 1, 1, 0,
                            0, 3, 1, 2,
                            2, 3, 1, 0,
                            1, 0, 2, 1 };

            double expectedInverse[] = { -3, -0.5, 1.5, 1,
                                         1, 0.25, -0.25, -0.5,
                                         3, 0.25, -1.25, -0.5,
                                         -3, 0, 1, 1, };

            double result[16];

            basis.setBasis( B0 );
            TS_ASSERT_THROWS_NOTHING( basis.invertBasis( result ) );

            for ( unsigned i = 0; i < sizeof(result) / sizeof(double); ++i )
            {
                TSM_ASSERT( i, FloatUtils::areEqual( result[i], expectedInverse[i] ) );
            }

            basis.setBasis( expectedInverse );
            TS_ASSERT_THROWS_NOTHING( basis.invertBasis( result ) );

            for ( unsigned i = 0; i < sizeof(result) / sizeof(double); ++i )
            {
                TSM_ASSERT( i, FloatUtils::areEqual( result[i], B0[i] ) );
            }
        }
    }
};

//
// Local Variables:
// compile-command: "make -C ../../.. "
// tags-file-name: "../../../TAGS"
// c-basic-offset: 4
// End:
//
