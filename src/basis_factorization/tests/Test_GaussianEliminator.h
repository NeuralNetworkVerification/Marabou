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

    void test_dummy()
    {
        TS_ASSERT( true );
    }

    // void test_identity()
    // {
    //     double A[] =
    //         {
    //             1, 0, 0,
    //             0, 1, 0,
    //             0, 0, 1
    //         };

    //     GaussianEliminator eliminator( A, 3 );

    //     List<LPElement *> lp;
    //     double U[9];
    //     unsigned rowHeaders[3];

    //     TS_ASSERT_THROWS_NOTHING( eliminator.factorize( &lp, U, rowHeaders ) );

    //     double expectedU[] =
    //         {
    //             1, 0, 0,
    //             0, 1, 0,
    //             0, 0, 1
    //         };

    //     TS_ASSERT_SAME_DATA( U, expectedU, sizeof(U) );
    //     TS_ASSERT( lp.empty() );
    // }

    // void applyLpOnA( List<LPElement *> &lp, double *A, unsigned m )
    // {
    //     for ( auto op = lp.rbegin(); op != lp.rend(); ++op )
    //     {
    //         LPElement &element( *(*op) );
    //         if ( element._pair )
    //         {
    //             double *temp = new double[m];
    //             memcpy( temp, A + ( m * element._pair->first ), sizeof(double) * m );
    //             memcpy( A + ( m * element._pair->first ), A + ( m * element._pair->second ), sizeof(double) * m );
    //             memcpy( A + ( m * element._pair->second ), temp, sizeof(double) * m );
    //             delete[] temp;
    //         }
    //         else
    //         {
    //             EtaMatrix &eta( *( element._eta ) );
    //             for ( unsigned i = 0; i < m; ++i )
    //             {
    //                 if ( i == eta._columnIndex )
    //                     continue;

    //                 for ( unsigned j = 0; j < m; ++j )
    //                 {
    //                     A[i*m + j] += eta._column[i] * A[eta._columnIndex*m + j];
    //                 }
    //             }

    //             for ( unsigned j = 0; j < m; ++j )
    //                 A[eta._columnIndex*m + j] *= eta._column[eta._columnIndex];
    //         }
    //     }
    // }

    // void dumpMatrix( double *A, unsigned m )
    // {
    //     for ( unsigned i = 0; i < m; ++i )
    //     {
    //         for ( unsigned j = 0; j < m; ++j )
    //         {
    //             printf( "%.2lf ", A[i*m + j] );
    //         }
    //         printf( "\n" );
    //     }

    //     printf( "\n\n" );
    // }

    // void dumpMatrix( double *A, unsigned m, unsigned *rowHeaders )
    // {
    //     for ( unsigned i = 0; i < m; ++i )
    //     {
    //         for ( unsigned j = 0; j < m; ++j )
    //         {
    //             printf( "%.2lf ", A[rowHeaders[i]*m + j] );
    //         }
    //         printf( "\n" );
    //     }

    //     printf( "\n\n" );
    // }

    // void test_sanity()
    // {
    //     {
    //         double A[] =
    //             {
    //                 1, 1, 1,
    //                 0, 1, 0,
    //                 0, 1, 1
    //             };

    //         unsigned m = 3;
    //         GaussianEliminator eliminator( A, m );
    //         List<LPElement *> lp;
    //         double U[m * m];
    //         unsigned rowHeaders[m];

    //         TS_ASSERT_THROWS_NOTHING( eliminator.factorize( &lp, U, rowHeaders ) );

    //         applyLpOnA( lp, A, m );

    //         for ( unsigned i = 0; i < m; ++i )
    //         {
    //             for ( unsigned j = 0; j < m; ++j )
    //             {
    //                 TS_ASSERT( FloatUtils::areEqual( A[i*m + j], U[rowHeaders[i]*m + j] ) );
    //             }
    //         }
    //     }

    //     {
    //         double A[] =
    //             {
    //                 1, 2, 3,
    //                 0, 4, 5,
    //                 2, 1, 3
    //             };

    //         unsigned m = 3;
    //         GaussianEliminator eliminator( A, m );
    //         List<LPElement *> lp;
    //         double U[m * m];
    //         unsigned rowHeaders[m];

    //         TS_ASSERT_THROWS_NOTHING( eliminator.factorize( &lp, U, rowHeaders ) );

    //         applyLpOnA( lp, A, m );

    //         for ( unsigned i = 0; i < m; ++i )
    //         {
    //             for ( unsigned j = 0; j < m; ++j )
    //             {
    //                 TS_ASSERT( FloatUtils::areEqual( A[i*m + j], U[rowHeaders[i]*m + j] ) );
    //             }
    //         }
    //     }

    //     {
    //         double A[] =
    //             {
    //                 1, 2, 3, -3,
    //                 -2, 4, 5, 1,
    //                 2, 1, 3, 0,
    //                 0, 0, 1, 0,
    //             };

    //         unsigned m = 4;
    //         GaussianEliminator eliminator( A, m );
    //         List<LPElement *> lp;
    //         double U[m * m];
    //         unsigned rowHeaders[m];

    //         TS_ASSERT_THROWS_NOTHING( eliminator.factorize( &lp, U, rowHeaders ) );

    //         applyLpOnA( lp, A, m );

    //         for ( unsigned i = 0; i < m; ++i )
    //         {
    //             for ( unsigned j = 0; j < m; ++j )
    //             {
    //                 TS_ASSERT( FloatUtils::areEqual( A[i*m + j], U[rowHeaders[i]*m + j] ) );
    //             }
    //         }
    //     }
    // }
};

//
// Local Variables:
// compile-command: "make -C ../../.. "
// tags-file-name: "../../../TAGS"
// c-basic-offset: 4
// End:
//
