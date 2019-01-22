/*********************                                                        */
/*! \file Test_Set.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** \brief [[ Add one-line brief description here ]]
 **
 ** [[ Add lengthier description here ]]
 **/

#include <cxxtest/TestSuite.h>

#include "MockErrno.h"
#include "Set.h"

class MockForSet :
    public MockErrno,
    public T::Base_rand
{
public:
    MockForSet()
    {
        randWasCalled = false;
    }

    bool randWasCalled;
    int nextRandValue;

    int rand()
    {
        randWasCalled = true;
        return nextRandValue;
    }
};

class SetTestSuite : public CxxTest::TestSuite
{
public:
    MockForSet *mock;

    void setUp()
    {
        TS_ASSERT( mock = new MockForSet );
    }

    void tearDown()
    {
        TS_ASSERT_THROWS_NOTHING( delete mock );
    }

    void test_initializer_list()
    {
        Set<int> set { 1, 2, 3 };

        TS_ASSERT_EQUALS( set.size(), 3U );

        TS_ASSERT( set.exists( 1 ) );
        TS_ASSERT( set.exists( 2 ) );
        TS_ASSERT( set.exists( 3 ) );
    }

    void test_insert()
    {
        Set<int> set;

        TS_ASSERT_EQUALS( set.begin(), set.end() );

        set.insert( 5 );
        set.insert( 8 );
        set.insert( 3 );

        TS_ASSERT( set.exists( 3 ) );
        TS_ASSERT( set.exists( 5 ) );
        TS_ASSERT( set.exists( 8 ) );
        TS_ASSERT( !set.exists( 4 ) );
        TS_ASSERT( !set.exists( 6 ) );

        TS_ASSERT_EQUALS( set.size(), 3u );
        set.insert( 3 );
        TS_ASSERT_EQUALS( set.size(), 3u );
        set.insert( 4 );
        TS_ASSERT_EQUALS( set.size(), 4u );
    }

    void test_insert_set()
    {
        Set<int> set;
        Set<int> other;

        set.insert( 5 );
        set.insert( 8 );
        set.insert( 3 );

        other.insert( 1 );
        other.insert( 4 );
        other.insert( 5 );
        other.insert( 7 );

        TS_ASSERT( set.exists( 3 ) );
        TS_ASSERT( set.exists( 5 ) );
        TS_ASSERT( set.exists( 8 ) );
        TS_ASSERT( !set.exists( 1 ) );
        TS_ASSERT( !set.exists( 4 ) );
        TS_ASSERT( !set.exists( 7 ) );

        TS_ASSERT_THROWS_NOTHING( set.insert( other ) );

        TS_ASSERT_EQUALS( set.size(), 6u );

        TS_ASSERT( set.exists( 3 ) );
        TS_ASSERT( set.exists( 5 ) );
        TS_ASSERT( set.exists( 8 ) );
        TS_ASSERT( set.exists( 1 ) );
        TS_ASSERT( set.exists( 4 ) );
        TS_ASSERT( set.exists( 7 ) );
    }

    void test_clear()
    {
        Set<int> set;

        set.insert( 5 );
        set.insert( 8 );
        set.insert( 3 );

        TS_ASSERT( set.exists( 3 ) );
        TS_ASSERT( set.exists( 5 ) );
        TS_ASSERT( set.exists( 8 ) );
        TS_ASSERT_EQUALS( set.size(), 3u );

        TS_ASSERT_THROWS_NOTHING( set.clear() );

        TS_ASSERT( !set.exists( 3 ) );
        TS_ASSERT( !set.exists( 5 ) );
        TS_ASSERT( !set.exists( 8 ) );
        TS_ASSERT_EQUALS( set.size(), 0u );
    }

    void test_difference()
    {
        Set<int> one, two, difference;

        one.insert( 1 );
        one.insert( 4 );
        one.insert( 3 );
        one.insert( 7 );

        two.insert( 2 );
        two.insert( 4 );
        two.insert( 7 );
        two.insert( 5 );

        TS_ASSERT_THROWS_NOTHING( difference = Set<int>::difference( one, two ) );

        TS_ASSERT_EQUALS( one.size(), 4U );
        TS_ASSERT_EQUALS( two.size(), 4U );
        TS_ASSERT_EQUALS( difference.size(), 2U );

        TS_ASSERT( difference.exists( 1 ) );
        TS_ASSERT( difference.exists( 3 ) );
    }

    void test_erase()
    {
        Set<int> set;

        set.insert( 1 );
        set.insert( 2 );
        set.insert( 3 );
        set.insert( 4 );

        TS_ASSERT_EQUALS( set.size(), 4U );

        TS_ASSERT_THROWS_NOTHING( set.erase( 2 ) );

        TS_ASSERT_EQUALS( set.size(), 3U );
        TS_ASSERT( !set.exists( 2 ) );

        TS_ASSERT_THROWS_NOTHING( set.erase( 3 ) );

        TS_ASSERT_EQUALS( set.size(), 2U );
        TS_ASSERT( !set.exists( 3 ) );

        TS_ASSERT( set.exists( 1 ) );
        TS_ASSERT( set.exists( 4 ) );

        TS_ASSERT_THROWS_NOTHING( set.erase( 5 ) );
    }

    void test_find()
    {
        Set<int> set;

        set.insert( 1 );
        set.insert( 2 );
        set.insert( 3 );
        set.insert( 4 );

        Set<int>::const_iterator it = set.find( 3 );

        TS_ASSERT_EQUALS( *it, 3 );
        ++it;
        TS_ASSERT_EQUALS( *it, 4 );
    }

    void test_operator_plus_equals()
    {
        Set<int> a, b;

        a.insert( 1 );
        a.insert( 3 );

        b.insert( 2 );
        b.insert( 4 );

        TS_ASSERT_THROWS_NOTHING( a += b );

        TS_ASSERT_EQUALS( a.size(), 4U );
        TS_ASSERT_EQUALS( b.size(), 2U );

        TS_ASSERT( a.exists( 1 ) );
        TS_ASSERT( a.exists( 2 ) );
        TS_ASSERT( a.exists( 3 ) );
        TS_ASSERT( a.exists( 4 ) );

        TS_ASSERT( b.exists( 2 ) );
        TS_ASSERT( b.exists( 4 ) );
    }

    void test_operator_plus()
    {
        Set<int> a, b;

        a.insert( 1 );
        a.insert( 3 );

        b.insert( 2 );
        b.insert( 4 );

        Set<int> c;
        TS_ASSERT_THROWS_NOTHING( c = a + b );

        TS_ASSERT_EQUALS( a.size(), 2U );
        TS_ASSERT_EQUALS( b.size(), 2U );
        TS_ASSERT_EQUALS( c.size(), 4U );

        TS_ASSERT( a.exists( 1 ) );
        TS_ASSERT( a.exists( 3 ) );

        TS_ASSERT( b.exists( 2 ) );
        TS_ASSERT( b.exists( 4 ) );

        TS_ASSERT( c.exists( 1 ) );
        TS_ASSERT( c.exists( 2 ) );
        TS_ASSERT( c.exists( 3 ) );
        TS_ASSERT( c.exists( 4 ) );
    }

    void test_intersection()
    {
        Set<int> a, b;

        a.insert( 1 );
        a.insert( 3 );

        b.insert( 3 );
        b.insert( 4 );

        Set<int> c;
        TS_ASSERT_THROWS_NOTHING( c = Set<int>::intersection( a, b ) );

        TS_ASSERT_EQUALS( c.size(), 1U );

        TS_ASSERT( c.exists( 3 ) );
    }

    void test_contained_in()
    {
        Set<int> a, b;

        a.insert( 1 );
        a.insert( 3 );

        b.insert( 3 );
        b.insert( 4 );

        TS_ASSERT( !Set<int>::containedIn( a, b ) );

        b.insert( 1 ) ;

        TS_ASSERT( Set<int>::containedIn( a, b ) );

        a.insert( 5 );

        TS_ASSERT( !Set<int>::containedIn( a, b ) );

        // Empty set should be contained in everything
        Set<int> empty;

        TS_ASSERT( Set<int>::containedIn( empty, a ) );
        TS_ASSERT( Set<int>::containedIn( empty, b ) );
    }

    void test_get_random_element()
    {
        Set<int> a;
        a.insert( 1 );
        a.insert( 2 );
        a.insert( 3 );

        mock->nextRandValue = 1;
        TS_ASSERT_EQUALS( a.getRandomElement(), 2 );
        TS_ASSERT( mock->randWasCalled );

        mock->randWasCalled = false;
        mock->nextRandValue = 2;
        TS_ASSERT_EQUALS( a.getRandomElement(), 3 );
        TS_ASSERT( mock->randWasCalled );
    }

    void test_equality()
    {
        Set<int> a, b;

        TS_ASSERT( a == b );
        TS_ASSERT( !( a != b ) );

        a.insert( 1 );
        b.insert( 2 );

        TS_ASSERT( !( a == b ) );
        TS_ASSERT( a != b );

        b.insert( 1 );

        TS_ASSERT( !( a == b ) );
        TS_ASSERT( a != b );

        a.insert( 2 );

        TS_ASSERT( a == b );
        TS_ASSERT( !( a != b ) );
    }
};

//
// Local Variables:
// compile-command: "make -C ../../.. "
// tags-file-name: "../../../TAGS"
// c-basic-offset: 4
// End:
//
