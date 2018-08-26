/*********************                                                        */
/*! \file Test_SparseUnsortedVector.h
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
#include "SparseUnsortedVector.h"

class MockForSparseUnsortedVector
{
public:
};

class SparseUnsortedVectorTestSuite : public CxxTest::TestSuite
{
public:
    MockForSparseUnsortedVector *mock;

    void setUp()
    {
        TS_ASSERT( mock = new MockForSparseUnsortedVector );
    }

    void tearDown()
    {
        TS_ASSERT_THROWS_NOTHING( delete mock );
    }

    void test_empty_unsorted_vector()
    {
        SparseUnsortedVector v1( 4 );

        for ( unsigned i = 0; i < 4; ++i )
            TS_ASSERT( FloatUtils::isZero( v1.get( i ) ) );
        TS_ASSERT( v1.empty() );
        TS_ASSERT_EQUALS( v1.getNnz(), 0U );
    }

    void test_initialize_from_dense()
    {
        double dense[8] = {
            1, 2, 3, 0, 0, 4, 5, 6
        };

        SparseUnsortedVector v1( dense, 8 );

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

        SparseUnsortedVector v1( dense, 8 );

        SparseUnsortedVector v2;

        TS_ASSERT_THROWS_NOTHING( v2 = v1 );

        v1.clear();
        TS_ASSERT( v1.empty() );

        TS_ASSERT_EQUALS( v2.getNnz(), 6U );
        TS_ASSERT( !v2.empty() );

        for ( unsigned i = 0; i < 8; ++i )
            TS_ASSERT( FloatUtils::areEqual( dense[i], v2.get( i ) ) );
    }

    void test_iterate()
    {
        double dense[8] = {
            1, 2, 3, 0, 0, 4, 5, 6
        };

        Map<unsigned, double> answers;
        answers[0] = 1;
        answers[1] = 2;
        answers[2] = 3;
        answers[5] = 4;
        answers[6] = 5;
        answers[7] = 6;

        SparseUnsortedVector v1( dense, 8 );

        TS_ASSERT_EQUALS( v1.getNnz(), 6U );

        auto it = v1.begin();
        for ( unsigned i = 0; i < 6; ++i )
        {
            TS_ASSERT( answers.exists( it->first ) );
            TS_ASSERT_EQUALS( answers[it->first], it->second );
            ++it;
        }

        TS_ASSERT_EQUALS( it, v1.end() );
    }

    void test_commit_changes()
    {
        SparseUnsortedVector v1( 5 );
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
