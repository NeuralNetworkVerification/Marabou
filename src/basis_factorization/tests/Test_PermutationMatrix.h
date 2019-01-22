/*********************                                                        */
/*! \file Test_PermutationMatrix.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

**/

#include <cxxtest/TestSuite.h>

#include "BasisFactorizationError.h"
#include "EtaMatrix.h"
#include "FloatUtils.h"
#include "GlobalConfiguration.h"
#include "PermutationMatrix.h"
#include "List.h"
#include "MockErrno.h"

class MockForPermutationMatrix
{
public:
};

class PermutationMatrixTestSuite : public CxxTest::TestSuite
{
public:
    MockForPermutationMatrix *mock;

    bool isIdentityPermutation( const PermutationMatrix *matrix )
    {
        for ( unsigned i = 0; i < matrix->getM(); ++i )
            if ( matrix->_rowOrdering[i] != i )
                return false;

        return true;
    }

    void setUp()
    {
        TS_ASSERT( mock = new MockForPermutationMatrix );
    }

    void tearDown()
    {
        TS_ASSERT_THROWS_NOTHING( delete mock );
    }

    void test_reset()
    {
        PermutationMatrix p( 4 );

        p._rowOrdering[0] = 0;
        p._rowOrdering[1] = 3;
        p._rowOrdering[2] = 1;
        p._rowOrdering[3] = 2;

        TS_ASSERT( !isIdentityPermutation( &p ) );

        TS_ASSERT_THROWS_NOTHING( p.resetToIdentity() );

        TS_ASSERT( isIdentityPermutation( &p ) );
    }

    void test_assignment()
    {
        PermutationMatrix p1( 4 );

        p1._rowOrdering[0] = 0;
        p1._rowOrdering[1] = 3;
        p1._rowOrdering[2] = 1;
        p1._rowOrdering[3] = 2;

        PermutationMatrix p2( 3 );

        TS_ASSERT_THROWS_NOTHING( p2 = p1 );

        p1.resetToIdentity();

        TS_ASSERT_EQUALS( p2._rowOrdering[0], 0U );
        TS_ASSERT_EQUALS( p2._rowOrdering[1], 3U );
        TS_ASSERT_EQUALS( p2._rowOrdering[2], 1U );
        TS_ASSERT_EQUALS( p2._rowOrdering[3], 2U );
    }

    void test_invert()
    {
        PermutationMatrix p1( 4 );

        // Ordering should be: [0, 3, 1, 2]

        p1.swapRows( 1, 3 );
        p1.swapRows( 2, 3 );

        TS_ASSERT_EQUALS( p1._rowOrdering[0], 0U );
        TS_ASSERT_EQUALS( p1._columnOrdering[0], 0U );
        TS_ASSERT_EQUALS( p1._rowOrdering[1], 3U );
        TS_ASSERT_EQUALS( p1._columnOrdering[3], 1U );
        TS_ASSERT_EQUALS( p1._rowOrdering[2], 1U );
        TS_ASSERT_EQUALS( p1._columnOrdering[1], 2U );
        TS_ASSERT_EQUALS( p1._rowOrdering[3], 2U );
        TS_ASSERT_EQUALS( p1._columnOrdering[2], 3U );

        PermutationMatrix *invP1 = p1.invert();

        TS_ASSERT_EQUALS( invP1->getM(), 4U );

        TS_ASSERT_EQUALS( invP1->_rowOrdering[0], 0U );
        TS_ASSERT_EQUALS( invP1->_columnOrdering[0], 0U );
        TS_ASSERT_EQUALS( invP1->_rowOrdering[1], 2U );
        TS_ASSERT_EQUALS( invP1->_columnOrdering[2], 1U );
        TS_ASSERT_EQUALS( invP1->_rowOrdering[2], 3U );
        TS_ASSERT_EQUALS( invP1->_columnOrdering[3], 2U );
        TS_ASSERT_EQUALS( invP1->_rowOrdering[3], 1U );
        TS_ASSERT_EQUALS( invP1->_columnOrdering[1], 3U );

        TS_ASSERT_THROWS_NOTHING( delete invP1 );

        PermutationMatrix p2( 5 );

        p2._rowOrdering[0] = 2;
        p2._columnOrdering[2] = 0;
        p2._rowOrdering[1] = 4;
        p2._columnOrdering[4] = 1;
        p2._rowOrdering[2] = 3;
        p2._columnOrdering[3] = 2;
        p2._rowOrdering[3] = 0;
        p2._columnOrdering[0] = 3;
        p2._rowOrdering[4] = 1;
        p2._columnOrdering[1] = 4;

        PermutationMatrix *invP2 = p2.invert();

        TS_ASSERT_EQUALS( invP2->getM(), 5U );

        TS_ASSERT_EQUALS( invP2->_rowOrdering[0], 3U );
        TS_ASSERT_EQUALS( invP2->_columnOrdering[3], 0U );
        TS_ASSERT_EQUALS( invP2->_rowOrdering[1], 4U );
        TS_ASSERT_EQUALS( invP2->_columnOrdering[4], 1U );
        TS_ASSERT_EQUALS( invP2->_rowOrdering[2], 0U );
        TS_ASSERT_EQUALS( invP2->_columnOrdering[0], 2U );
        TS_ASSERT_EQUALS( invP2->_rowOrdering[3], 2U );
        TS_ASSERT_EQUALS( invP2->_columnOrdering[2], 3U );
        TS_ASSERT_EQUALS( invP2->_rowOrdering[4], 1U );
        TS_ASSERT_EQUALS( invP2->_columnOrdering[1], 4U );

        TS_ASSERT_THROWS_NOTHING( delete invP2 );

        PermutationMatrix otherInvP2( 5 );

        p2.invert( otherInvP2 );

        TS_ASSERT_EQUALS( otherInvP2._rowOrdering[0], 3U );
        TS_ASSERT_EQUALS( otherInvP2._columnOrdering[3], 0U );
        TS_ASSERT_EQUALS( otherInvP2._rowOrdering[1], 4U );
        TS_ASSERT_EQUALS( otherInvP2._columnOrdering[4], 1U );
        TS_ASSERT_EQUALS( otherInvP2._rowOrdering[2], 0U );
        TS_ASSERT_EQUALS( otherInvP2._columnOrdering[0], 2U );
        TS_ASSERT_EQUALS( otherInvP2._rowOrdering[3], 2U );
        TS_ASSERT_EQUALS( otherInvP2._columnOrdering[2], 3U );
        TS_ASSERT_EQUALS( otherInvP2._rowOrdering[4], 1U );
        TS_ASSERT_EQUALS( otherInvP2._columnOrdering[1], 4U );
    }

    void test_find_index_of_row()
    {
        PermutationMatrix p( 4 );

        p._rowOrdering[0] = 0;
        p._rowOrdering[1] = 3;
        p._rowOrdering[2] = 1;
        p._rowOrdering[3] = 2;

        TS_ASSERT_EQUALS( p.findIndexOfRow( 0 ), 0U );
        TS_ASSERT_EQUALS( p.findIndexOfRow( 1 ), 2U );
        TS_ASSERT_EQUALS( p.findIndexOfRow( 2 ), 3U );
        TS_ASSERT_EQUALS( p.findIndexOfRow( 3 ), 1U );

        p._rowOrdering[3] = 1;
        TS_ASSERT_THROWS_EQUALS( p.findIndexOfRow( 2 ),
                                 const BasisFactorizationError &e,
                                 e.getCode(),
                                 BasisFactorizationError::CORRUPT_PERMUATION_MATRIX );

        p.resetToIdentity();

        TS_ASSERT_EQUALS( p.findIndexOfRow( 0 ), 0U );
        TS_ASSERT_EQUALS( p.findIndexOfRow( 1 ), 1U );
        TS_ASSERT_EQUALS( p.findIndexOfRow( 2 ), 2U );
        TS_ASSERT_EQUALS( p.findIndexOfRow( 3 ), 3U );
    }
};

//
// Local Variables:
// compile-command: "make -C ../../.. "
// tags-file-name: "../../../TAGS"
// c-basic-offset: 4
// End:
//
