/*********************                                                        */
/*! \file Test_SparseGaussianEliminator.h
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
#include "CSRMatrix.h"
#include "EtaMatrix.h"
#include "FloatUtils.h"
#include "SparseGaussianEliminator.h"
#include <cstring>

class MockForSparseGaussianEliminator
{
public:
};

class SparseGaussianEliminatorTestSuite : public CxxTest::TestSuite
{
public:
    MockForSparseGaussianEliminator *mock;

    void setUp()
    {
        TS_ASSERT( mock = new MockForSparseGaussianEliminator );
    }

    void tearDown()
    {
        TS_ASSERT_THROWS_NOTHING( delete mock );
    }

    void computeMatrixFromFactorization( SparseLUFactors *lu, double *result )
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
                    double fValue = ( i == k ) ? 1.0 : lu->_F->get( i, k );
                    double vValue = lu->_V->get( k, j );
                    result[i*m + j] += fValue * vValue;
                }
            }
        }
    }

    void computeTransposedMatrixFromFactorization( SparseLUFactors *lu, double *result )
    {
        // At = Vt * Ft

        unsigned m = lu->_m;

        std::fill_n( result, m * m, 0.0 );
        for ( unsigned i = 0; i < m; ++i )
        {
            for ( unsigned j = 0; j < m; ++j )
            {
                result[i*m + j] = 0;
                for ( unsigned k = 0; k < m; ++k )
                {
                    double vtValue = lu->_Vt->get( i, k );
                    double ftValue = ( k == j ) ? 1.0 : lu->_Ft->get( k, j );

                    result[i*m + j] += vtValue * ftValue;
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

    void transposeMatrix( const double *orig, double *result, unsigned m )
    {
        for ( unsigned i = 0; i < m; ++i )
        {
            for ( unsigned j = 0; j < m; ++j )
            {
                result[i*m + j] = orig[j*m + i];
            }
        }
    }

    void test_sanity()
    {
        SparseLUFactors lu3( 3 );
        SparseLUFactors lu4( 4 );

        {
            double A[] =
            {
                2, 3, 0,
                0, 1, 0,
                0, 0, 1
            };

            CSRMatrix sparseA( A, 3, 3 );

            SparseGaussianEliminator *ge;

            TS_ASSERT( ge = new SparseGaussianEliminator( 3 ) );

            TS_ASSERT_THROWS_NOTHING( ge->run( &sparseA, &lu3 ) );

            double result[9];
            computeMatrixFromFactorization( &lu3, result );

            for ( unsigned i = 0; i < 9; ++i )
            {
                TS_ASSERT( FloatUtils::areEqual( A[i], result[i] ) );
            }

            double At[9];
            transposeMatrix( A, At, 3 );
            computeTransposedMatrixFromFactorization( &lu3, result );

            for ( unsigned i = 0; i < 9; ++i )
            {
                TS_ASSERT( FloatUtils::areEqual( At[i], result[i] ) );
            }

            TS_ASSERT_THROWS_NOTHING( delete ge );
        }

        {
            double A[] =
            {
                2, 3, 0,
                0, 1, 2,
                0, 4, 1
            };

            CSRMatrix sparseA( A, 3, 3 );

            SparseGaussianEliminator *ge;

            TS_ASSERT( ge = new SparseGaussianEliminator( 3 ) );
            TS_ASSERT_THROWS_NOTHING( ge->run( &sparseA, &lu3 ) );

            double result[9];
            computeMatrixFromFactorization( &lu3, result );

            for ( unsigned i = 0; i < 9; ++i )
            {
                TS_ASSERT( FloatUtils::areEqual( A[i], result[i] ) );
            }

            double At[9];
            transposeMatrix( A, At, 3 );
            computeTransposedMatrixFromFactorization( &lu3, result );

            for ( unsigned i = 0; i < 9; ++i )
            {
                TS_ASSERT( FloatUtils::areEqual( At[i], result[i] ) );
            }

            TS_ASSERT_THROWS_NOTHING( delete ge );
        }

        {
            double A[] =
            {
                2, 3, -4,
                -5, 1, 2,
                0, 4, 1
            };

            CSRMatrix sparseA( A, 3, 3 );

            SparseGaussianEliminator *ge;

            TS_ASSERT( ge = new SparseGaussianEliminator( 3 ) );
            TS_ASSERT_THROWS_NOTHING( ge->run( &sparseA, &lu3 ) );

            double result[9];
            computeMatrixFromFactorization( &lu3, result );

            for ( unsigned i = 0; i < 9; ++i )
            {
                TS_ASSERT( FloatUtils::areEqual( A[i], result[i] ) );
            }

            double At[9];
            transposeMatrix( A, At, 3 );
            computeTransposedMatrixFromFactorization( &lu3, result );

            for ( unsigned i = 0; i < 9; ++i )
            {
                TS_ASSERT( FloatUtils::areEqual( At[i], result[i] ) );
            }

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

            CSRMatrix sparseA( A, 4, 4 );

            SparseGaussianEliminator *ge;

            TS_ASSERT( ge = new SparseGaussianEliminator( 4 ) );
            TS_ASSERT_THROWS_NOTHING( ge->run( &sparseA, &lu4 ) );

            double result[16];
            computeMatrixFromFactorization( &lu4, result );

            for ( unsigned i = 0; i < 16; ++i )
            {
                TS_ASSERT( FloatUtils::areEqual( A[i], result[i] ) );
            }

            double At[16];
            transposeMatrix( A, At, 4 );
            computeTransposedMatrixFromFactorization( &lu4, result );

            for ( unsigned i = 0; i < 16; ++i )
            {
                TS_ASSERT( FloatUtils::areEqual( At[i], result[i] ) );
            }

            TS_ASSERT_THROWS_NOTHING( delete ge );
        }

        {
            double A[] =
            {
                2, 3, 0,
                0, 1, 0,
                5, 4, 0
            };

            CSRMatrix sparseA( A, 3, 3 );

            SparseGaussianEliminator *ge;

            TS_ASSERT( ge = new SparseGaussianEliminator( 3 ) );
            TS_ASSERT_THROWS_EQUALS( ge->run( &sparseA, &lu3 ),
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

            CSRMatrix sparseA( A, 3, 3 );

            SparseGaussianEliminator *ge;

            TS_ASSERT( ge = new SparseGaussianEliminator( 3 ) );
            TS_ASSERT_THROWS_EQUALS( ge->run( &sparseA, &lu3 ),
                                     const BasisFactorizationError &e,
                                     e.getCode(),
                                     BasisFactorizationError::GAUSSIAN_ELIMINATION_FAILED );

            TS_ASSERT_THROWS_NOTHING( delete ge );
        }
    }

    void test_todo()
    {
        TS_TRACE( "Currently we don't update Vt, Ft during the factorization, and sometimes we call getColumn() which is inefficient. Consider changing." );
    }
};

//
// Local Variables:
// compile-command: "make -C ../../.. "
// tags-file-name: "../../../TAGS"
// c-basic-offset: 4
// End:
//
