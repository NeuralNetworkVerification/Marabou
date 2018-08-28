/*********************                                                        */
/*! \file Test_SparseUnsortedList.h
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
#include "Map.h"
#include "SparseUnsortedList.h"

class MockForSparseUnsortedList
{
public:
};

class SparseUnsortedListTestSuite : public CxxTest::TestSuite
{
public:
    MockForSparseUnsortedList *mock;

    void setUp()
    {
        TS_ASSERT( mock = new MockForSparseUnsortedList );
    }

    void tearDown()
    {
        TS_ASSERT_THROWS_NOTHING( delete mock );
    }

    void test_empty_unsorted_list()
    {
        SparseUnsortedList v1( 4 );

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

        SparseUnsortedList v1( dense, 8 );

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

        SparseUnsortedList v1( dense, 8 );

        SparseUnsortedList v2;

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

        SparseUnsortedList v1( dense, 8 );

        TS_ASSERT_EQUALS( v1.getNnz(), 6U );

        auto it = v1.begin();
        for ( unsigned i = 0; i < 6; ++i )
        {
            TS_ASSERT( answers.exists( it->_index ) );
            TS_ASSERT_EQUALS( answers[it->_index], it->_value );
            ++it;
        }

        TS_ASSERT_EQUALS( it, v1.end() );
    }

    void test_set()
    {
        SparseUnsortedList v1( 5 );
        TS_ASSERT( v1.empty() );

        v1.set( 0, 4 );
        v1.set( 2, 3 );
        v1.set( 4, -7 );

        TS_ASSERT( !v1.empty() );

        TS_ASSERT_EQUALS( v1.getNnz(), 3U );

        double dense[5];
        TS_ASSERT_THROWS_NOTHING( v1.toDense( dense ) );

        double expected[5] = {
            4, 0, 3, 0, -7
        };

        for ( unsigned i = 0; i < 5; ++i )
            TS_ASSERT( FloatUtils::areEqual( expected[i], dense[i] ) );

        v1.set( 2, 0 );
        v1.set( 1, 5 );

        TS_ASSERT_THROWS_NOTHING( v1.toDense( dense ) );

        double expected2[5] = {
            4, 5, 0, 0, -7
        };

        for ( unsigned i = 0; i < 5; ++i )
            TS_ASSERT( FloatUtils::areEqual( expected2[i], dense[i] ) );
    }

    void test_merge_entries()
    {
        SparseUnsortedList v1( 5 );
        TS_ASSERT( v1.empty() );

        v1.set( 0, 4 );
        v1.set( 2, 3 );
        v1.set( 4, -7 );

        TS_ASSERT_EQUALS( v1.get( 0 ), 4 );
        TS_ASSERT_EQUALS( v1.get( 4 ), -7 );

        TS_ASSERT_THROWS_NOTHING( v1.mergeEntries( 0, 4 ) );

        TS_ASSERT_EQUALS( v1.getNnz(), 2U );

        TS_ASSERT_EQUALS( v1.get( 0 ), 0 );
        TS_ASSERT_EQUALS( v1.get( 4 ), -3 );

        TS_ASSERT_THROWS_NOTHING( v1.mergeEntries( 2, 4 ) );

        TS_ASSERT_EQUALS( v1.getNnz(), 0U );
    }
};

//
// Local Variables:
// compile-command: "make -C ../../.. "
// tags-file-name: "../../../TAGS"
// c-basic-offset: 4
// End:
//
