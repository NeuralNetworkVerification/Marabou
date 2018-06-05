/*********************                                                        */
/*! \file Test_GaussianEliminator.h
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

#include "BasisFactorizationError.h"
#include "EtaMatrix.h"
#include "FloatUtils.h"
#include "GaussianEliminator.h"
#include <cstring>

class MockForGaussianEliminator
{
public:
};

class GaussianEliminatorTestSuite : public CxxTest::TestSuite
{
public:
    MockForGaussianEliminator *mock;

    void setUp()
    {
        TS_ASSERT( mock = new MockForGaussianEliminator );
    }

    void tearDown()
    {
        TS_ASSERT_THROWS_NOTHING( delete mock );
    }

    void computeMatrixFromFactorization( LUFactors *lu, double *result )
    {
        unsigned m = lu->_m;

        std::fill_n( result, m * m, 0.0 );
        for ( unsigned i = 0; i < m; ++i )
        {
            for ( unsigned j = 0; j < m; ++j )
            {
                result[i*m + j] = 0;
                for ( unsigned k = 0; k < m; ++k )
                {
                    result[i*m + j] += lu->_F[i*m + k] * lu->_V[k*m + j];
                }
            }
        }
    }

    void dumpMatrix( double *matrix, unsigned m, String message )
    {
        TS_TRACE( message );
        printf( "\n" );

        for ( unsigned i = 0; i < m; ++i )
        {
            printf( "\t" );
            for ( unsigned j = 0; j < m; ++j )
            {
                printf( "%8.lf ", matrix[i*m + j] );
            }
            printf( "\n" );
        }

        printf( "\n" );
    }

    void test_sanity()
    {
        {
            double A[] =
            {
                2, 3, 0,
                0, 1, 0,
                0, 0, 1
            };

            GaussianEliminator *ge;
            LUFactors *lu;

            TS_ASSERT( ge = new GaussianEliminator( A, 3 ) );
            TS_ASSERT_THROWS_NOTHING( lu = ge->run() );

            double result[9];
            computeMatrixFromFactorization( lu, result );

            for ( unsigned i = 0; i < 9; ++i )
            {
                TS_ASSERT( FloatUtils::areEqual( A[i], result[i] ) );
            }

            TS_ASSERT_THROWS_NOTHING( delete lu );
            TS_ASSERT_THROWS_NOTHING( delete ge );
        }

        {
            double A[] =
            {
                2, 3, 0,
                0, 1, 2,
                0, 4, 1
            };

            GaussianEliminator *ge;
            LUFactors *lu;

            TS_ASSERT( ge = new GaussianEliminator( A, 3 ) );
            TS_ASSERT_THROWS_NOTHING( lu = ge->run() );

            double result[9];
            computeMatrixFromFactorization( lu, result );

            for ( unsigned i = 0; i < 9; ++i )
            {
                TS_ASSERT( FloatUtils::areEqual( A[i], result[i] ) );
            }

            TS_ASSERT_THROWS_NOTHING( delete lu );
            TS_ASSERT_THROWS_NOTHING( delete ge );
        }

        {
            double A[] =
            {
                2, 3, -4,
                -5, 1, 2,
                0, 4, 1
            };

            GaussianEliminator *ge;
            LUFactors *lu;

            TS_ASSERT( ge = new GaussianEliminator( A, 3 ) );
            TS_ASSERT_THROWS_NOTHING( lu = ge->run() );

            double result[9];
            computeMatrixFromFactorization( lu, result );

            for ( unsigned i = 0; i < 9; ++i )
            {
                TS_ASSERT( FloatUtils::areEqual( A[i], result[i] ) );
            }

            TS_ASSERT_THROWS_NOTHING( delete lu );
            TS_ASSERT_THROWS_NOTHING( delete ge );
        }

        {
            double A[] =
            {
                2, 3, -4, 0,
                -5, 1, 2, 2,
                0, 4, 1, -5,
                1, 2, 3, 4,
            };

            GaussianEliminator *ge;
            LUFactors *lu;

            TS_ASSERT( ge = new GaussianEliminator( A, 4 ) );
            TS_ASSERT_THROWS_NOTHING( lu = ge->run() );

            double result[16];
            computeMatrixFromFactorization( lu, result );

            for ( unsigned i = 0; i < 16; ++i )
            {
                TS_ASSERT( FloatUtils::areEqual( A[i], result[i] ) );
            }

            TS_ASSERT_THROWS_NOTHING( delete lu );
            TS_ASSERT_THROWS_NOTHING( delete ge );
        }

        {
            double A[] =
            {
                2, 3, 0,
                0, 1, 0,
                5, 4, 0
            };

            GaussianEliminator *ge;

            TS_ASSERT( ge = new GaussianEliminator( A, 3 ) );
            TS_ASSERT_THROWS_EQUALS( ge->run(),
                                     const BasisFactorizationError &e,
                                     e.getCode(),
                                     BasisFactorizationError::GAUSSIAN_ELIMINATION_FAILED );

            TS_ASSERT_THROWS_NOTHING( delete ge );
        }

        {
            double A[] =
            {
                2, 3, 7,
                0, 0, 0,
                5, 4, 0
            };

            GaussianEliminator *ge;

            TS_ASSERT( ge = new GaussianEliminator( A, 3 ) );
            TS_ASSERT_THROWS_EQUALS( ge->run(),
                                     const BasisFactorizationError &e,
                                     e.getCode(),
                                     BasisFactorizationError::GAUSSIAN_ELIMINATION_FAILED );

            TS_ASSERT_THROWS_NOTHING( delete ge );
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
