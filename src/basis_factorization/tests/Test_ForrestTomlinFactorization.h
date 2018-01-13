/*********************                                                        */
/*! \file Test_ForrestTomlinFactorization.h
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
#include "GlobalConfiguration.h"
#include "ForrestTomlinFactorization.h"
#include "List.h"
#include "MockErrno.h"

class MockForForrestTomlinFactorization
{
public:
};

class ForrestTomlinFactorizationTestSuite : public CxxTest::TestSuite
{
public:
    MockForForrestTomlinFactorization *mock;

    bool isIdentityPermutation( const PermutationMatrix *matrix )
    {
        for ( unsigned i = 0; i < matrix->getM(); ++i )
            if ( matrix->_ordering[i] != i )
                return false;

        return true;
    }

    void setUp()
    {
        TS_ASSERT( mock = new MockForForrestTomlinFactorization );
    }

    void tearDown()
    {
        TS_ASSERT_THROWS_NOTHING( delete mock );
    }

    void test_factorization_enabled_disabled()
    {
        ForrestTomlinFactorization *ft;

        TS_ASSERT( ft = new ForrestTomlinFactorization( 3 ) );

        TS_ASSERT( ft->factorizationEnabled() );

        TS_ASSERT_THROWS_NOTHING( ft->toggleFactorization( false ) );

        TS_ASSERT( !ft->factorizationEnabled() );

        TS_ASSERT_THROWS_NOTHING( ft->toggleFactorization( true ) );

        TS_ASSERT( ft->factorizationEnabled() );

        TS_ASSERT_THROWS_NOTHING( delete ft );
    }

    void test_set_basis()
    {
        ForrestTomlinFactorization *ft;

        TS_ASSERT( ft = new ForrestTomlinFactorization( 4 ) );

        double basisMatrix[16] = {
            1,   3, -2,  4,
            1,   5, -1,  5,
            1,   3, -3,  6,
            -1, -3,  3, -8,
        };

        TS_ASSERT_THROWS_NOTHING( ft->setBasis( basisMatrix ) );

        // LU factorization should have occurred. All A matrices should
        // be the identity, and Q and R should be the identity permutation.

        for ( unsigned i = 0; i < 4; ++i )
            TS_ASSERT( ft->getA()[i]._identity );
        TS_ASSERT( isIdentityPermutation( ft->getQ() ) );
        TS_ASSERT( isIdentityPermutation( ft->getR() ) );

        /*
          First step of the diagonlization:
             R1 := 1 * R1
             R2 := R2 - R1
             R3 := R3 - R1
             R4 := R4 + R1

          Matrix becomes:
            1,   3, -2,  4,
            0,   2,  1,  1,
            0,   0, -1,  2,
            0,   0,  1, -4,

          So L1 has the column (1, -1, -1, 1)
        */
        double expectedL1Col[] = { 1, -1, -1, 1 };
        EtaMatrix expectedL1( 4, 0, expectedL1Col );

        /*
          Second step of the diagonlization:
             R2 := 1/2 * R2

          Matrix becomes:
            1,   3,   -2,   4,
            0,   1,  1/2, 1/2,
            0,   0,   -1,   2,
            0,   0,    1,  -4,

            So L2 has the column (0, 1/2, 0, 0)
        */
        double expectedL2Col[] = { 0, 0.5, 0, 0 };
        EtaMatrix expectedL2( 4, 1, expectedL2Col );

        /*
          Third step of the diagonlization:
             R3 := -1 * R2
             R4 := R4 + R3

          Matrix becomes:
            1,   3,   -2,   4,
            0,   1,  1/2, 1/2,
            0,   0,    1,  -2,
            0,   0,    0,  -2,

            So L3 has the column (0, 0, -1, 1)
        */
        double expectedL3Col[] = { 0, 0, -1, 1 };
        EtaMatrix expectedL3( 4, 2, expectedL3Col );

        /*
          Fourth step of the diagonlization:
             R4 := -1/2 * R4

          Matrix becomes:
            1,   3,   -2,   4,
            0,   1,  1/2, 1/2,
            0,   0,    1,  -2,
            0,   0,    0,  1,

            So L4 has the column (0, 0, 0, -0.5)
        */
        double expectedL4Col[] = { 0, 0, 0, -0.5 };
        EtaMatrix expectedL4( 4, 3, expectedL4Col );

        // The upper traingular matrix discovered gives us the Us
        double expectedU4Col[] = { 4, 0.5, -2, 1 };
        EtaMatrix expectedU4( 4, 3, expectedU4Col );

        double expectedU3Col[] = { -2, 0.5, 1, 0 };
        EtaMatrix expectedU3( 4, 2, expectedU3Col );

        double expectedU2Col[] = { 3, 1, 0, 0 };
        EtaMatrix expectedU2( 4, 1, expectedU2Col );

        double expectedU1Col[] = { 1, 0, 0, 0 };
        EtaMatrix expectedU1( 4, 0, expectedU1Col );

        // Check that the FT object got it right
        const EtaMatrix **U = ft->getU();

        TS_ASSERT_EQUALS( *U[3], expectedU4 );
        TS_ASSERT_EQUALS( *U[2], expectedU3 );
        TS_ASSERT_EQUALS( *U[1], expectedU2 );
        TS_ASSERT_EQUALS( *U[0], expectedU1 );

        const List<LPElement *> *LP = ft->getLP();
        TS_ASSERT_EQUALS( LP->size(), 4U );
        auto lIt = LP->begin();
        TS_ASSERT( (*lIt)->_eta );
        TS_ASSERT_EQUALS( *((*lIt)->_eta), expectedL4 );
        ++lIt;
        TS_ASSERT_EQUALS( *((*lIt)->_eta), expectedL3 );
        ++lIt;
        TS_ASSERT_EQUALS( *((*lIt)->_eta), expectedL2 );
        ++lIt;
        TS_ASSERT_EQUALS( *((*lIt)->_eta), expectedL1 );

        TS_ASSERT_THROWS_NOTHING( delete ft );
    }

    void test_set_basis_2()
    {
        ForrestTomlinFactorization *ft;

        TS_ASSERT( ft = new ForrestTomlinFactorization( 4 ) );

        double basisMatrix[16] = {
            1,   4,  -2,  4,
            -2, -6,  -1,  5,
            1,   3,  -3,  6,
            -1, -3,   3, -8,
        };

        TS_ASSERT_THROWS_NOTHING( ft->setBasis( basisMatrix ) );

        // LU factorization should have occurred. All A matrices should
        // be the identity, and Q and R should be the identity permutation.

        for ( unsigned i = 0; i < 4; ++i )
            TS_ASSERT( ft->getA()[i]._identity );
        TS_ASSERT( isIdentityPermutation( ft->getQ() ) );
        TS_ASSERT( isIdentityPermutation( ft->getR() ) );

        /*
          First step of the diagonlization:
             R1 <-> R2

          Matrix becomes:
            -2, -6,  -1,  5,
            1,   4,  -2,  4,
            1,   3,  -3,  6,
            -1, -3,   3, -8,

        */
        std::pair<unsigned, unsigned> P1pair( 0, 1 );

        /*
          Second step of the diagonlization:
             R1 := -1/2 * R1
             R2 := R2 + 1/2 R1
             R3 := R3 + 1/2 R1
             R4 := R4 - 1/2 R1

          Matrix becomes:
            1,   3,  1/2,  -2.5,
            0,   1, -5/2,  13/2,
            0,   0, -7/2,  17/2,
            0,   0,  7/2, -21/2,

            So L2 has the column (-1/2, 1/2, 1/2, -1/2)
        */
        double expectedL2Col[] = { -0.5, 0.5, 0.5, -0.5 };
        EtaMatrix expectedL2( 4, 0, expectedL2Col );

        /*
          Third step of the diagonlization:
             R2 := 1 * R2

          Matrix becomes:
            1,   3,  1/2,  -2.5,
            0,   1, -5/2,  13/2,
            0,   0, -7/2,  17/2,
            0,   0,  7/2, -21/2,

            So L3 has the column (0, 1, 0, 0)
        */
        double expectedL3Col[] = { 0, 1, 0, 0 };
        EtaMatrix expectedL3( 4, 1, expectedL3Col );

        /*
          Fourth step of the diagonlization:
             R3 := -2/7 * R3
             R4 := R4 + R3

          Matrix becomes:
            1,   3,  1/2,  -2.5,
            0,   1, -5/2,  13/2,
            0,   0,    1, -17/7,
            0,   0,    0,    -2,

            So L3 has the column (0, 0, -2/7, 1)
        */
        double expectedL4Col[] = { 0, 0, -2.0/7, 1 };
        EtaMatrix expectedL4( 4, 2, expectedL4Col );

        /*
          Fifth step of the diagonlization:
             R4 := -1/2 * R4

          Matrix becomes:
            1,   3,  1/2,  -2.5,
            0,   1, -5/2,  13/2,
            0,   0,    1, -17/7,
            0,   0,    0,     1,

            So L3 has the column (0, 0, 0, 1/3)
        */
        double expectedL5Col[] = { 0, 0, 0, -0.5 };
        EtaMatrix expectedL5( 4, 3, expectedL5Col );

        // The upper traingular matrix discovered gives us the Us
        double expectedU4Col[] = { -2.5, 6.5, -17.0/7, 1 };
        EtaMatrix expectedU4( 4, 3, expectedU4Col );

        double expectedU3Col[] = { 0.5, -2.5, 1, 0 };
        EtaMatrix expectedU3( 4, 2, expectedU3Col );

        double expectedU2Col[] = { 3, 1, 0, 0 };
        EtaMatrix expectedU2( 4, 1, expectedU2Col );

        double expectedU1Col[] = { 1, 0, 0, 0 };
        EtaMatrix expectedU1( 4, 0, expectedU1Col );

        // Check that the FT object got it right
        const EtaMatrix **U = ft->getU();
        TS_ASSERT_EQUALS( *U[3], expectedU4 );
        TS_ASSERT_EQUALS( *U[2], expectedU3 );
        TS_ASSERT_EQUALS( *U[1], expectedU2 );
        TS_ASSERT_EQUALS( *U[0], expectedU1 );

        const List<LPElement *> *LP = ft->getLP();
        TS_ASSERT_EQUALS( LP->size(), 5U );
        auto lIt = LP->begin();
        TS_ASSERT( (*lIt)->_eta );
        TS_ASSERT_EQUALS( *((*lIt)->_eta), expectedL5 );
        ++lIt;
        TS_ASSERT_EQUALS( *((*lIt)->_eta), expectedL4 );
        ++lIt;
        TS_ASSERT_EQUALS( *((*lIt)->_eta), expectedL3 );
        ++lIt;
        TS_ASSERT_EQUALS( *((*lIt)->_eta), expectedL2 );
        ++lIt;
        TS_ASSERT_EQUALS( *((*lIt)->_pair), P1pair );

        TS_ASSERT_THROWS_NOTHING( delete ft );
    }

    void test_forward_transformation()
    {
        ForrestTomlinFactorization *ft;

        TS_ASSERT( ft = new ForrestTomlinFactorization( 4 ) );

        double basisMatrix[16] = {
            1,   3, -2,  4,
            1,   5, -1,  5,
            1,   3, -3,  6,
            -1, -3,  3, -8,
        };

        TS_ASSERT_THROWS_NOTHING( ft->setBasis( basisMatrix ) );

        /*
          The factorization of this matrix gives:

          L1 = |  1 0 0 0 |     L2 = | 1 0   0 0 |
               | -1 1 0 0 |          | 0 1/2 0 0 |
               | -1 0 1 0 |          | 0 0   1 0 |
               |  1 0 0 1 |          | 0 0   0 1 |

          L3 = | 1 0 0  0 |     L4 = | 1 0 0    0 |
               | 0 1 0  0 |          | 0 1 0    0 |
               | 0 0 -1 0 |          | 0 0 1    0 |
               | 0 0 1  1 |          | 0 0 0 -1/2 |


          U4 = | 1 0 0   4 |    U3 = | 1 0 -2  0 |
               | 0 1 0 1/2 |         | 0 1 1/2 0 |
               | 0 0 1  -2 |         | 0 0 1   0 |
               | 0 0 0   1 |         | 0 0 0   1 |

          U2 = | 1 3 0 0 |
               | 0 1 0 0 |
               | 0 0 1 0 |
               | 0 0 0 1 |
        */

        {
            double expectedX[4] = { 1, 2, 1, 1 };
            double y[4] = { 9, 15, 10, -12 };
            double x[4];

            TS_ASSERT_THROWS_NOTHING( ft->forwardTransformation( y, x ) );
            for ( unsigned i = 0; i < 4; ++i )
                TS_ASSERT( FloatUtils::areEqual( x[i], expectedX[i] ) );
        }

        /*
          Now manually add A, Q and R.

          Q = | 0 1 0 0 |   R = | 1 0 0 0 |
              | 1 0 0 0 |       | 0 1 0 0 |
              | 0 0 0 1 |       | 0 0 0 1 |
              | 0 0 1 0 |       | 0 0 1 0 |

          A1 = | 1 0 0 0 |  A2 = | 1  0 0 0 |
               | 0 1 0 3 |       | 0  1 0 0 |
               | 0 0 1 0 |       | -2 0 1 0 |
               | 0 0 0 1 |       | 0  0 0 1 |
        */

        PermutationMatrix Q( 4 );
        Q._ordering[0] = 1;
        Q._ordering[1] = 0;
        Q._ordering[2] = 3;
        Q._ordering[3] = 2;

        PermutationMatrix R( 4 );
        R._ordering[0] = 0;
        R._ordering[1] = 1;
        R._ordering[2] = 3;
        R._ordering[3] = 2;

        ft->setQ( Q );
        ft->setR( R );

        AlmostIdentityMatrix A1;
        A1._identity = false;
        A1._row = 1;
        A1._column = 3;
        A1._value = 3;

        AlmostIdentityMatrix A2;
        A2._identity = false;
        A2._row = 2;
        A2._column = 0;
        A2._value = -2;

        ft->setA( 0, A1 );
        ft->setA( 2, A2 );

        {
            /*
              Should hold:

              Q * U4U3U2 * R * x = A2A1 * L4L3L2L1 * y

              Manually checking gives:

              | 0 1 1/2 1/2 | * x = | 1       0    0    0 | * y
              | 1 3   4  -2 |       | -1/2  1/2 -3/2 -3/2 |
              | 0 0   1   0 |       | -1      0   -1    0 |
              | 0 0  -2   1 |       | 0       0 -1/2 -1/2 |

              Or:

              x = inv( | 0 1 1/2 1/2 | ) * | 1       0    0    0 | * y
                       | 1 3   4  -2 |     | -1/2  1/2 -3/2 -3/2 |
                       | 0 0   1   0 |     | -1      0   -1    0 |
                       | 0 0  -2   1 |     | 0       0 -1/2 -1/2 |

                = | -8  1/2 -31/4 -13/4 | * y
                  | 5/2   0   7/4   1/4 |
                  | -1    0    -1     0 |
                  | -2    0  -5/2  -1/2 |
            */

            double expectedX[4] = { -3.75, 2.25, -1, -1.5 };

            double y[4] = { 1, 2, 0, -1 };
            double x[4];

            TS_ASSERT_THROWS_NOTHING( ft->forwardTransformation( y, x ) );

            for ( unsigned i = 0; i < 4; ++i )
                TS_ASSERT( FloatUtils::areEqual( x[i], expectedX[i] ) );
        }
    }

    void test_backward_transformation()
    {
        ForrestTomlinFactorization *ft;

        TS_ASSERT( ft = new ForrestTomlinFactorization( 4 ) );

        double basisMatrix[16] = {
            1,   3, -2,  4,
            1,   5, -1,  5,
            1,   3, -3,  6,
            -1, -3,  3, -8,
        };

        TS_ASSERT_THROWS_NOTHING( ft->setBasis( basisMatrix ) );

        /*
          The factorization of this matrix gives:

          L1 = |  1 0 0 0 |     L2 = | 1 0   0 0 |
               | -1 1 0 0 |          | 0 1/2 0 0 |
               | -1 0 1 0 |          | 0 0   1 0 |
               |  1 0 0 1 |          | 0 0   0 1 |

          L3 = | 1 0 0  0 |     L4 = | 1 0 0    0 |
               | 0 1 0  0 |          | 0 1 0    0 |
               | 0 0 -1 0 |          | 0 0 1    0 |
               | 0 0 1  1 |          | 0 0 0 -1/2 |


          U4 = | 1 0 0   4 |    U3 = | 1 0 -2  0 |
               | 0 1 0 1/2 |         | 0 1 1/2 0 |
               | 0 0 1  -2 |         | 0 0 1   0 |
               | 0 0 0   1 |         | 0 0 0   1 |

          U2 = | 1 3 0 0 |
               | 0 1 0 0 |
               | 0 0 1 0 |
               | 0 0 0 1 |
        */

        {
            double expectedX[4] = { 1, 2, 1, 1 };
            double y[4] = { 3, 13, -4, 12 };
            double x[4];

            TS_ASSERT_THROWS_NOTHING( ft->backwardTransformation( y, x ) );
            for ( unsigned i = 0; i < 4; ++i )
                TS_ASSERT( FloatUtils::areEqual( x[i], expectedX[i] ) );
        }

        /*
          Now manually add A, Q and R.

          Q = | 0 1 0 0 |   R = | 1 0 0 0 |
              | 1 0 0 0 |       | 0 1 0 0 |
              | 0 0 0 1 |       | 0 0 0 1 |
              | 0 0 1 0 |       | 0 0 1 0 |

          A1 = | 1 0 0 0 |  A2 = | 1  0 0 0 |
               | 0 1 0 3 |       | 0  1 0 0 |
               | 0 0 1 0 |       | -2 0 1 0 |
               | 0 0 0 1 |       | 0  0 0 1 |
        */

        PermutationMatrix Q( 4 );
        Q._ordering[0] = 1;
        Q._ordering[1] = 0;
        Q._ordering[2] = 3;
        Q._ordering[3] = 2;

        PermutationMatrix R( 4 );
        R._ordering[0] = 0;
        R._ordering[1] = 1;
        R._ordering[2] = 3;
        R._ordering[3] = 2;

        ft->setQ( Q );
        ft->setR( R );

        AlmostIdentityMatrix A1;
        A1._identity = false;
        A1._row = 1;
        A1._column = 3;
        A1._value = 3;

        AlmostIdentityMatrix A2;
        A2._identity = false;
        A2._row = 2;
        A2._column = 0;
        A2._value = -2;

        ft->setA( 0, A1 );
        ft->setA( 2, A2 );

        {
            /*
              Should hold:

              x = y * inv(R) * inv(Um...U1) * inv(Q) * Am...A1 * LsPs...L1p1

              Manually checking gives:

              Am...A1 * LsPs...L1p1 = | 1      0    0    0 |
                                      | -1/2 1/2 -3/2 -3/2 |
                                      | -1     0   -1    0 |
                                      | 0      0 -1/2 -1/2 |

              inv(Um...U1) = | 1 -3  7/2  9/2 |
                             | 0  1 -1/2 -3/2 |
                             | 0  0    1    2 |
                             | 0  0    0    1 |


              inv(Q) = | 0 1 0 0 |   inv(R) = | 1 0 0 0 |
                       | 1 0 0 0 |            | 0 1 0 0 |
                       | 0 0 0 1 |            | 0 0 0 1 |
                       | 0 0 1 0 |            | 0 0 1 0 |

              inv(R) * inv(Um...U1) * inv(Q) = | -3 1  9/2  7/2 |
                                               |  1 0 -3/2 -1/2 |
                                               |  0 0    1    0 |
                                               |  0 0    2    1 |

              Or:

              x = y * |  -8 1/2 -31/4 -13/4 |
                      | 5/2   0   7/4   1/4 |
                      | -1    0    -1     0 |
                      | -2    0  -5/2  -1/2 |
            */

            double expectedX[4] = { -1, 1.0/2, -7.0/4, -9.0/4 };

            double y[4] = { 1, 2, 0, -1 };
            double x[4];

            TS_ASSERT_THROWS_NOTHING( ft->backwardTransformation( y, x ) );

            for ( unsigned i = 0; i < 4; ++i )
                TS_ASSERT( FloatUtils::areEqual( x[i], expectedX[i] ) );
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
