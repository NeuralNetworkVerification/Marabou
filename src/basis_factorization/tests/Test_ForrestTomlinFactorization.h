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
#include "ForrestTomlinFactorization.h"
#include "GlobalConfiguration.h"
#include "List.h"
#include "MockColumnOracle.h"
#include "MockErrno.h"

class MockForForrestTomlinFactorization
{
public:
};

class ForrestTomlinFactorizationTestSuite : public CxxTest::TestSuite
{
public:
    MockForForrestTomlinFactorization *mock;
    MockColumnOracle *oracle;

    bool isIdentityPermutation( const PermutationMatrix *matrix )
    {
        for ( unsigned i = 0; i < matrix->getM(); ++i )
            if ( matrix->_rowOrdering[i] != i )
                return false;

        return true;
    }

    void setUp()
    {
        TS_ASSERT( mock = new MockForForrestTomlinFactorization );
        TS_ASSERT( oracle = new MockColumnOracle );
    }

    void tearDown()
    {
        TS_ASSERT_THROWS_NOTHING( delete oracle );
        TS_ASSERT_THROWS_NOTHING( delete mock );
    }

    void test_set_basis()
    {
        ForrestTomlinFactorization *ft;

        TS_ASSERT( ft = new ForrestTomlinFactorization( 4, *oracle ) );

        double basisMatrix[16] = {
            1,   3, -2,  4,
            1,   5, -1,  5,
            1,   3, -3,  6,
            -1, -3,  3, -8,
        };

        oracle->storeBasis( 4, basisMatrix );
        ft->obtainFreshBasis();

        // LU factorization should have occurred. There should be no A matrices,
        // and Q and R should be the identity permutation.

        TS_ASSERT( ft->getA()->empty() );
        TS_ASSERT( isIdentityPermutation( ft->getQ() ) );
        TS_ASSERT( isIdentityPermutation( ft->getInvQ() ) );

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

        TS_ASSERT( ft = new ForrestTomlinFactorization( 4, *oracle ) );

        double basisMatrix[16] = {
            1,   4,  -2,  4,
            -2, -6,  -1,  5,
            1,   3,  -3,  6,
            -1, -3,   3, -8,
        };

        oracle->storeBasis( 4, basisMatrix );
        ft->obtainFreshBasis();

        // LU factorization should have occurred. There should be no A matrices,
        // and Q and R should be the identity permutation.

        TS_ASSERT( ft->getA()->empty() );
        TS_ASSERT( isIdentityPermutation( ft->getQ() ) );
        TS_ASSERT( isIdentityPermutation( ft->getInvQ() ) );

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

        TS_ASSERT( ft = new ForrestTomlinFactorization( 4, *oracle ) );

        double basisMatrix[16] = {
            1,   3, -2,  4,
            1,   5, -1,  5,
            1,   3, -3,  6,
            -1, -3,  3, -8,
        };

        oracle->storeBasis( 4, basisMatrix );
        ft->obtainFreshBasis();

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
          Now manually add A and Q

          Q = | 0 1 0 0 |   R = invQ = Q
              | 1 0 0 0 |
              | 0 0 0 1 |
              | 0 0 1 0 |

          A1 = | 1 0 0 0 |  A2 = | 1  0 0 0 |
               | 0 1 0 3 |       | 0  1 0 0 |
               | 0 0 1 0 |       | -2 0 1 0 |
               | 0 0 0 1 |       | 0  0 0 1 |
        */

        PermutationMatrix Q( 4 );
        Q._rowOrdering[0] = 1;
        Q._rowOrdering[1] = 0;
        Q._rowOrdering[2] = 3;
        Q._rowOrdering[3] = 2;

        ft->setQ( Q );

        AlmostIdentityMatrix A1;
        A1._row = 1;
        A1._column = 3;
        A1._value = 3;

        AlmostIdentityMatrix A2;
        A2._row = 2;
        A2._column = 0;
        A2._value = -2;

        ft->pushA( A1 );
        ft->pushA( A2 );

        {
            /*
              Should hold:

              Q * U4U3U2 * R * x = A2A1 * L4L3L2L1 * y

              Manually checking gives:

              | 1 0 1/2 1/2 | * x = | 1       0    0    0 | * y
              | 3 1   4  -2 |       | -1/2  1/2 -3/2 -3/2 |
              | 0 0   1   0 |       | -1      0   -1    0 |
              | 0 0  -2   1 |       | 0       0 -1/2 -1/2 |

              Or:

              x = inv( | 1 0 1/2 1/2 | ) * | 1       0    0    0 | * y
                       | 3 1   4  -2 |     | -1/2  1/2 -3/2 -3/2 |
                       | 0 0   1   0 |     | -1      0   -1    0 |
                       | 0 0  -2   1 |     | 0       0 -1/2 -1/2 |

                = | 1 0  -3/2 -1/2 | * | 1       0    0    0 | * y
                  | -3 1  9/2  7/2 |   | -1/2  1/2 -3/2 -3/2 |
                  | 0 0     1    0 |   | -1      0   -1    0 |
                  | 0 0     2    1 |   | 0       0 -1/2 -1/2 |

                = | 5/2   0   7/4   1/4 | * y
                  |  -8 1/2 -31/4 -13/4 |
                  |  -1   0    -1     0 |
                  |  -2   0  -5/2  -1/2 |
            */

            double expectedX[4] = { 2.25, -3.75, -1, -1.5 };

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

        TS_ASSERT( ft = new ForrestTomlinFactorization( 4, *oracle ) );

        double basisMatrix[16] = {
            1,   3, -2,  4,
            1,   5, -1,  5,
            1,   3, -3,  6,
            -1, -3,  3, -8,
        };

        oracle->storeBasis( 4, basisMatrix );
        ft->obtainFreshBasis();

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
          Now manually add A and Q.

          Q = | 0 1 0 0 |   invQ = Q
              | 1 0 0 0 |
              | 0 0 0 1 |
              | 0 0 1 0 |

          A1 = | 1 0 0 0 |  A2 = | 1  0 0 0 |
               | 0 1 0 3 |       | 0  1 0 0 |
               | 0 0 1 0 |       | -2 0 1 0 |
               | 0 0 0 1 |       | 0  0 0 1 |
        */

        PermutationMatrix Q( 4 );
        Q._rowOrdering[0] = 1;
        Q._rowOrdering[1] = 0;
        Q._rowOrdering[2] = 3;
        Q._rowOrdering[3] = 2;

        ft->setQ( Q );

        AlmostIdentityMatrix A1;
        A1._row = 1;
        A1._column = 3;
        A1._value = 3;

        AlmostIdentityMatrix A2;
        A2._row = 2;
        A2._column = 0;
        A2._value = -2;

        ft->pushA( A1 );
        ft->pushA( A2 );

        {
            /*
              Should hold:

              x = y * Q * inv(Um...U1) * invQ * Ak...A1 * LsPs...L1p1

              Manually checking gives:

              Ak...A1 * LsPs...L1p1 = | 1      0    0    0 |
                                      | -1/2 1/2 -3/2 -3/2 |
                                      | -1     0   -1    0 |
                                      | 0      0 -1/2 -1/2 |

              inv(Um...U1) = | 1 -3  7/2  9/2 |
                             | 0  1 -1/2 -3/2 |
                             | 0  0    1    2 |
                             | 0  0    0    1 |


              Q = invQ = | 0 1 0 0 |
                         | 1 0 0 0 |
                         | 0 0 0 1 |
                         | 0 0 1 0 |

              Q * inv(Um...U1) * invQ = | 1  0 -3/2  -1/2 |
                                        | -3 1  9/2   7/2 |
                                        | 0  0    1     0 |
                                        | 0  0    2     1 |

              Or:

              x = y * | 1  0 -3/2  -1/2 | * |    1   0    0    0 |
                      | -3 1  9/2   7/2 |   | -1/2 1/2 -3/2 -3/2 |
                      | 0  0    1     0 |   |   -1   0   -1    0 |
                      | 0  0    2     1 |   |    0   0 -1/2 -1/2 |

                = y * | 5/2   0   7/4   1/4 |
                      |  -8 1/2 -31/4 -13/4 |
                      |  -1   0    -1     0 |
                      |  -2   0  -5/2  -1/2 |
            */

            double expectedX[4] = { -11.5, 1, -11.25, -5.75 };

            double y[4] = { 1, 2, 0, -1 };
            double x[4];

            TS_ASSERT_THROWS_NOTHING( ft->backwardTransformation( y, x ) );

            for ( unsigned i = 0; i < 4; ++i )
                TS_ASSERT( FloatUtils::areEqual( x[i], expectedX[i] ) );
        }
    }

    void test_push_eta_matrix_refactorization()
    {
        ForrestTomlinFactorization *ft;

        TS_ASSERT( ft = new ForrestTomlinFactorization( 4, *oracle ) );

        // B = | 1   3 -2  4 |
        //     | 1   5 -1  5 |
        //     | 1   3 -3  6 |
        //     | -1 -3  3 -8 |
        double basisMatrix[16] = {
            1,   3, -2,  4,
            1,   5, -1,  5,
            1,   3, -3,  6,
            -1, -3,  3, -8,
        };

        oracle->storeBasis( 4, basisMatrix );
        ft->obtainFreshBasis();

        // E1 = | 1 -4     |
        //      |    2     |
        //      |    0 1   |
        //      | 0  3 0 1 |
        double a1[] = { -4, 2, 0, 3 };

        ft->updateToAdjacentBasis( 1, a1, NULL );

        // B * E1 = | 1   14 -2  4 |
        //          | 1   21 -1  5 |
        //          | 1   20 -3  6 |
        //          | -1 -26  3 -8 |

        /*
          Previous factorization:

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

          With A, Q and R all the identity matrices.

          After E1 is pushed, the factorization is updated as follows:

          V = URE * inv(R) = U*E = | 1 3  -2   4 | * | 1 -4     |
                                   | 0 1 1/2 1/2 |   |    2     |
                                   | 0 0   1  -2 |   |    0 1   |
                                   | 0 0   0   1 |   |    3   1 |

                                 = | 1  14  -2   4 |
                                   | 0 7/2 1/2 1/2 |
                                   | 0  -6   1  -2 |
                                   | 0   3   0   1 |

          We now pick Q' and R' = inv(Q') that transfer the bump column
          to be a bump row:

          Q' = | 1 0 0 0 |    R' = | 1 0 0 0 |
               | 0 0 1 0 |         | 0 0 0 1 |
               | 0 0 0 1 |         | 0 1 0 0 |
               | 0 1 0 0 |         | 0 0 1 0 |

          Q'VR' = | 1  -2   4  14 |
                  | 0   1  -2  -6 |
                  | 0   0   1   3 |
                  | 0 1/2 1/2 7/2 |

          This dictates the values of the almost-diagonal matrices:

          A1' = I,

          A2' = | 1    0 0 0 |  A3' = | 1 0    0 0 |
                | 0    1 0 0 |        | 0 1    0 0 |
                | 0    0 1 0 |        | 0 0    1 0 |
                | 0 -1/2 0 1 |        | 0 0 -3/2 1 |

          A4' = | 1 0 0 0   |
                | 0 1 0 0   |
                | 0 0 1 0   |
                | 0 0 0 1/2 |

          And of U':

          U' = | 1  -2   4  14 |
               | 0  1   -2  -6 |
               | 0  0    1   3 |
               | 0  0    0   1 |


          And this gives us the new values of the factorization:

          U'' = U'
          Q'' = Q * inv(Q') = I * inv(Q') = | 1 0 0 0 |
                                            | 0 0 0 1 |
                                            | 0 1 0 0 |
                                            | 0 0 1 0 |

          R'' = inv(R') * R = inv(R') * I = | 1 0 0 0 |
                                            | 0 0 1 0 |
                                            | 0 0 0 1 |
                                            | 0 1 0 0 |

          A4'' = Q'' A4' inv(Q'') = | 1 0   0 0 |
                                    | 0 1/2 0 0 |
                                    | 0 0   1 0 |
                                    | 0 0   0 1 |

          A3'' = Q'' A3' inv(Q'') = | 1 0 0   0  |
                                    | 0 1 0 -3/2 |
                                    | 0 0 1   0  |
                                    | 0 0 0   1  |

          A2'' = Q'' A2' inv(Q'') = | 1 0    0 0 |
                                    | 0 1 -1/2 0 |
                                    | 0 0    1 0 |
                                    | 0 0    0 1 |

          A1'' = I

          The Ls and Ps are unchanged.
        */

        double expectedL1Col[] = { 1, -1, -1, 1 };
        EtaMatrix expectedL1( 4, 0, expectedL1Col );
        double expectedL2Col[] = { 0, 0.5, 0, 0 };
        EtaMatrix expectedL2( 4, 1, expectedL2Col );
        double expectedL3Col[] = { 0, 0, -1, 1 };
        EtaMatrix expectedL3( 4, 2, expectedL3Col );
        double expectedL4Col[] = { 0, 0, 0, -0.5 };
        EtaMatrix expectedL4( 4, 3, expectedL4Col );

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

        double expectedU4Col[] = { 14, -6, 3, 1 };
        EtaMatrix expectedU4( 4, 3, expectedU4Col );

        double expectedU3Col[] = { 4, -2, 1, 0 };
        EtaMatrix expectedU3( 4, 2, expectedU3Col );

        double expectedU2Col[] = { -2, 1, 0, 0 };
        EtaMatrix expectedU2( 4, 1, expectedU2Col );

        double expectedU1Col[] = { 1, 0, 0, 0 };
        EtaMatrix expectedU1( 4, 0, expectedU1Col );

        const EtaMatrix **U = ft->getU();

        TS_ASSERT_EQUALS( *U[3], expectedU4 );
        TS_ASSERT_EQUALS( *U[2], expectedU3 );
        TS_ASSERT_EQUALS( *U[1], expectedU2 );
        TS_ASSERT_EQUALS( *U[0], expectedU1 );

        const PermutationMatrix *Q = ft->getQ();
        const PermutationMatrix *invQ = ft->getInvQ();

        TS_ASSERT_EQUALS( Q->_rowOrdering[0], 0U );
        TS_ASSERT_EQUALS( Q->_rowOrdering[1], 3U );
        TS_ASSERT_EQUALS( Q->_rowOrdering[2], 1U );
        TS_ASSERT_EQUALS( Q->_rowOrdering[3], 2U );

        TS_ASSERT_EQUALS( invQ->_rowOrdering[0], 0U );
        TS_ASSERT_EQUALS( invQ->_rowOrdering[1], 2U );
        TS_ASSERT_EQUALS( invQ->_rowOrdering[2], 3U );
        TS_ASSERT_EQUALS( invQ->_rowOrdering[3], 1U );

        const List<AlmostIdentityMatrix *> *A = ft->getA();

        TS_ASSERT_EQUALS( A->size(), 3U );

        auto it = A->begin();

        TS_ASSERT_EQUALS( (*it)->_row, 1U );
        TS_ASSERT_EQUALS( (*it)->_column, 2U );
        TS_ASSERT( FloatUtils::areEqual( (*it)->_value, -0.5 ) );

        ++it;
        TS_ASSERT_EQUALS( (*it)->_row, 1U );
        TS_ASSERT_EQUALS( (*it)->_column, 3U );
        TS_ASSERT( FloatUtils::areEqual( (*it)->_value, -1.5 ) );

        ++it;
        TS_ASSERT_EQUALS( (*it)->_row, 1U );
        TS_ASSERT_EQUALS( (*it)->_column, 1U );
        TS_ASSERT( FloatUtils::areEqual( (*it)->_value, 0.5 ) );

        TS_ASSERT_THROWS_NOTHING( delete ft );
    }

    void test_store_and_restore()
    {
        ForrestTomlinFactorization *ft;

        TS_ASSERT( ft = new ForrestTomlinFactorization( 4, *oracle ) );

        double basisMatrix[16] = {
            1,   3, -2,  4,
            1,   5, -1,  5,
            1,   3, -3,  6,
            -1, -3,  3, -8,
        };

        oracle->storeBasis( 4, basisMatrix );
        ft->obtainFreshBasis();

        double a1[] = { -4, 2, 0, 3 };
        ft->updateToAdjacentBasis( 1, a1, NULL );

        ForrestTomlinFactorization *ft2 = new ForrestTomlinFactorization( 4, *oracle );
        ForrestTomlinFactorization *ft3 = new ForrestTomlinFactorization( 4, *oracle );

        ft->storeFactorization( ft2 );
        ft3->restoreFactorization( ft2 );

        double y[] = { 1.25, 3.85, -4.5, 17 };
        double x1[4];
        double x2[4];
        double x3[4];

        TS_ASSERT_THROWS_NOTHING( ft->forwardTransformation( y, x1 ) );
        TS_ASSERT_THROWS_NOTHING( ft2->forwardTransformation( y, x2 ) );
        TS_ASSERT_THROWS_NOTHING( ft3->forwardTransformation( y, x3 ) );

        for ( unsigned i = 0; i < 4; ++i )
        {
            TS_ASSERT_EQUALS( x1[i], x2[i] );
            TS_ASSERT_EQUALS( x1[i], x3[i] );
        }
    }

    void test_get_basis()
    {
        ForrestTomlinFactorization *ft;

        TS_ASSERT( ft = new ForrestTomlinFactorization( 4, *oracle ) );

        // B = | 1   3 -2  4 |
        //     | 1   5 -1  5 |
        //     | 1   3 -3  6 |
        //     | -1 -3  3 -8 |
        double basisMatrix[16] = {
            1,   3, -2,  4,
            1,   5, -1,  5,
            1,   3, -3,  6,
            -1, -3,  3, -8,
        };

        oracle->storeBasis( 4, basisMatrix );
        ft->obtainFreshBasis();

        TS_ASSERT( ft->explicitBasisAvailable() );
        const double *basis;
        TS_ASSERT_THROWS_NOTHING( basis = ft->getBasis() );
        for ( unsigned i = 0; i < 16; ++i )
            TS_ASSERT( FloatUtils::areEqual( basisMatrix[i], basis[i] ) );

        // E1 = | 1 -4     |
        //      |    2     |
        //      |    0 1   |
        //      | 0  3 0 1 |
        double a1[] = { -4, 2, 0, 3 };
        ft->updateToAdjacentBasis( 1, a1, NULL );

        // B * E1 = | 1   14 -2  4 |
        //          | 1   21 -1  5 |
        //          | 1   20 -3  6 |
        //          | -1 -26  3 -8 |

        double expectedB[16] = {
            1,   14, -2,  4,
            1,   21, -1,  5,
            1,   20, -3,  6,
            -1, -26,  3, -8,
        };

        oracle->storeBasis( 4, expectedB );

        TS_ASSERT( !ft->explicitBasisAvailable() );
        TS_ASSERT_THROWS_NOTHING( ft->makeExplicitBasisAvailable() );
        TS_ASSERT( ft->explicitBasisAvailable() );

        TS_ASSERT_THROWS_NOTHING( basis = ft->getBasis() );
        for ( unsigned i = 0; i < 16; ++i )
        {
            TS_ASSERT( FloatUtils::areEqual( expectedB[i], basis[i] ) );
        }

        TS_ASSERT_THROWS_NOTHING( delete ft );
    }

    void test_invert_basis()
    {
        ForrestTomlinFactorization *ft;

        TS_ASSERT( ft = new ForrestTomlinFactorization( 4, *oracle ) );

        // B = | 1   3 -2  4 |
        //     | 1   5 -1  5 |
        //     | 1   3 -3  6 |
        //     | -1 -3  3 -8 |
        double basisMatrix[16] = {
            1,   3, -2,  4,
            1,   5, -1,  5,
            1,   3, -3,  6,
            -1, -3,  3, -8,
        };

        oracle->storeBasis( 4, basisMatrix );
        ft->obtainFreshBasis();

        // invB = |  6 -3/2 -23/4 -9/4 |
        //        | -1  1/2   5/4  3/4 |
        //        |  1    0    -2   -1 |
        //        |  0    0  -1/2 -1/2 |
        double expectedInvB1[] = {
                6, -3.0/2, -23.0/4, -9.0/4,
                -1, 1.0/2, 5.0/4, 3.0/4,
                1, 0, -2, -1,
                0, 0, -1.0/2, -1.0/2,
        };

        double invB[16];

        TS_ASSERT_THROWS_NOTHING( ft->invertBasis( invB ) );
        for ( unsigned i = 0; i < 16; ++i )
            TS_ASSERT( FloatUtils::areEqual( invB[i], expectedInvB1[i] ) );

        // E1 = | 1 -4     |
        //      |    2     |
        //      |    0 1   |
        //      | 0  3 0 1 |
        double a1[] = { -4, 2, 0, 3 };
        ft->updateToAdjacentBasis( 1, a1, NULL );

        // B * E1 = | 1   14 -2  4 |
        //          | 1   21 -1  5 |
        //          | 1   20 -3  6 |
        //          | -1 -26  3 -8 |

        double expectedB[16] = {
            1,   14, -2,  4,
            1,   21, -1,  5,
            1,   20, -3,  6,
            -1, -26,  3, -8,
        };

        oracle->storeBasis( 4, expectedB );

        // invB = |  4   -1/2 -13/4  -3/4 |
        //        | -1/2  1/4   5/8   3/8 |
        //        |  1      0    -2    -1 |
        //        |  3/2 -3/4 -19/8 -13/8 |

        double expectedInvB2[] = {
            4, -1.0/2, -13.0/4, -3.0/4,
            -1.0/2, 1.0/4, 5.0/8, 3.0/8,
            1, 0, -2, -1,
            3.0/2, -3.0/4, -19.0/8, -13.0/8,
        };

        TS_ASSERT_THROWS_EQUALS( ft->invertBasis( invB ),
                                 const BasisFactorizationError &e,
                                 e.getCode(),
                                 BasisFactorizationError::CANT_INVERT_BASIS_BECAUSE_BASIS_ISNT_AVAILABLE );

        TS_ASSERT_THROWS_NOTHING( ft->makeExplicitBasisAvailable() );

        TS_ASSERT_THROWS_NOTHING( ft->invertBasis( invB ) );
        for ( unsigned i = 0; i < 16; ++i )
            TS_ASSERT( FloatUtils::areEqual( invB[i], expectedInvB2[i] ) );

        TS_ASSERT_THROWS_NOTHING( delete ft );
    }
};

//
// Local Variables:
// compile-command: "make -C ../../.. "
// tags-file-name: "../../../TAGS"
// c-basic-offset: 4
// End:
//
