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

        // LU factorization should have occurred. There should be no A
        // matrices, and Q and R should be the identity permutation.

        TS_ASSERT( ft->getA()->empty() );
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

            So L3 has the column (0, 0, 0, -0.5)
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

        // Check that the FT object got it right
        const List<EtaMatrix *> *U = ft->getU();
        TS_ASSERT_EQUALS( U->size(), 3U );
        auto uIt = U->begin();
        TS_ASSERT_EQUALS( **uIt, expectedU4 );
        ++uIt;
        TS_ASSERT_EQUALS( **uIt, expectedU3 );
        ++uIt;
        TS_ASSERT_EQUALS( **uIt, expectedU2 );

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

        // LU factorization should have occurred. There should be no A
        // matrices, and Q and R should be the identity permutation.

        TS_ASSERT( ft->getA()->empty() );
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

        // Check that the FT object got it right
        const List<EtaMatrix *> *U = ft->getU();
        TS_ASSERT_EQUALS( U->size(), 3U );
        auto uIt = U->begin();
        TS_ASSERT_EQUALS( **uIt, expectedU4 );
        ++uIt;
        TS_ASSERT_EQUALS( **uIt, expectedU3 );
        ++uIt;
        TS_ASSERT_EQUALS( **uIt, expectedU2 );

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
};

//
// Local Variables:
// compile-command: "make -C ../../.. "
// tags-file-name: "../../../TAGS"
// c-basic-offset: 4
// End:
//
