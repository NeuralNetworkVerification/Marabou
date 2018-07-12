/*********************                                                        */
/*! \file Test_SparseVector.h
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

#include "FloatUtils.h"
#include "SparseVector.h"

class MockForSparseVector
{
public:
};

class SparseVectorTestSuite : public CxxTest::TestSuite
{
public:
    MockForSparseVector *mock;

    void setUp()
    {
        TS_ASSERT( mock = new MockForSparseVector );
    }

    void tearDown()
    {
        TS_ASSERT_THROWS_NOTHING( delete mock );
    }

    void test_empty_vector()
    {
        SparseVector v1( 4 );

        for ( unsigned i = 0; i < 4; ++i )
            TS_ASSERT( FloatUtils::isZero( v1.get( i ) ) );
        TS_ASSERT( v1.empty() );
        TS_ASSERT_EQUALS( v1.getNnz(), 0U );

        SparseVector v2;

        TS_ASSERT_THROWS_NOTHING( v2.initializeToEmpty( 4 ) );
        for ( unsigned i = 0; i < 4; ++i )
            TS_ASSERT( FloatUtils::isZero( v2.get( i ) ) );

        TS_ASSERT( v2.empty() );
        TS_ASSERT_EQUALS( v2.getNnz(), 0U );
    }

    void test_initialize_from_dense()
    {
        double dense[8] = {
            1, 2, 3, 0, 0, 4, 5, 6
        };

        SparseVector v1( dense, 8 );

        TS_ASSERT_EQUALS( v1.getNnz(), 6U );
        TS_ASSERT( !v1.empty() );

        for ( unsigned i = 0; i < 8; ++i )
            TS_ASSERT( FloatUtils::areEqual( dense[i], v1.get( i ) ) );
    }

    void test_cloning()
    {
        double dense[8] = {
            1, 2, 3, 0, 0, 4, 5, 6
        };

        SparseVector v1( dense, 8 );

        SparseVector v2;

        TS_ASSERT_THROWS_NOTHING( v2 = v1 );

        v1.clear();
        TS_ASSERT( v1.empty() );

        TS_ASSERT_EQUALS( v2.getNnz(), 6U );
        TS_ASSERT( !v2.empty() );

        for ( unsigned i = 0; i < 8; ++i )
            TS_ASSERT( FloatUtils::areEqual( dense[i], v2.get( i ) ) );
    }

    void test_get_index_and_value()
    {
        double dense[8] = {
            1, 2, 3, 0, 0, 4, 5, 6
        };

        SparseVector v1( dense, 8 );

        TS_ASSERT_EQUALS( v1.getNnz(), 6U );

        TS_ASSERT_EQUALS( v1.getIndexOfEntry( 0 ), 0U );
        TS_ASSERT_EQUALS( v1.getIndexOfEntry( 1 ), 1U );
        TS_ASSERT_EQUALS( v1.getIndexOfEntry( 2 ), 2U );
        TS_ASSERT( FloatUtils::areEqual( v1.getValueOfEntry( 0 ), 1 ) );
        TS_ASSERT( FloatUtils::areEqual( v1.getValueOfEntry( 1 ), 2 ) );
        TS_ASSERT( FloatUtils::areEqual( v1.getValueOfEntry( 2 ), 3 ) );

        TS_ASSERT_EQUALS( v1.getIndexOfEntry( 3 ), 5U );
        TS_ASSERT_EQUALS( v1.getIndexOfEntry( 4 ), 6U );
        TS_ASSERT_EQUALS( v1.getIndexOfEntry( 5 ), 7U );
        TS_ASSERT( FloatUtils::areEqual( v1.getValueOfEntry( 3 ), 4 ) );
        TS_ASSERT( FloatUtils::areEqual( v1.getValueOfEntry( 4 ), 5 ) );
        TS_ASSERT( FloatUtils::areEqual( v1.getValueOfEntry( 5 ), 6 ) );

        for ( unsigned i = 0; i < 8; ++i )
            TS_ASSERT( FloatUtils::areEqual( v1.get( i ), dense[i] ) );
    }

    void test_commit_changes()
    {
        SparseVector v1( 5 );
        TS_ASSERT( v1.empty() );

        v1.commitChange( 0, 4 );
        v1.commitChange( 2, 3 );
        v1.commitChange( 4, -7 );

        TS_ASSERT( v1.empty() );

        TS_ASSERT_THROWS_NOTHING( v1.executeChanges() );

        TS_ASSERT( !v1.empty() );
        TS_ASSERT_EQUALS( v1.getNnz(), 3U );

        double dense[5];
        TS_ASSERT_THROWS_NOTHING( v1.toDense( dense ) );

        double expected[5] = {
            4, 0, 3, 0, -7
        };

        for ( unsigned i = 0; i < 5; ++i )
            TS_ASSERT( FloatUtils::areEqual( expected[i], dense[i] ) );
    }

};

//
// Local Variables:
// compile-command: "make -C ../../.. "
// tags-file-name: "../../../TAGS"
// c-basic-offset: 4
// End:
//
