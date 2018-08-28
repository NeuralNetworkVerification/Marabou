#include <cxxtest/TestSuite.h>

#include "MockErrno.h"
#include "HashSet.h"

class MockForHashSet :
    public MockErrno
{
public:
};

class HashSetTestSuite : public CxxTest::TestSuite
{
public:
    MockForHashSet *mock;

    void hashSetUp()
    {
        TS_ASSERT( mock = new MockForHashSet );
    }

    void tearDown()
    {
        TS_ASSERT_THROWS_NOTHING( delete mock );
    }

    void test_initializer_list()
    {
        HashSet<int> hashSet { 1, 2, 3 };

        TS_ASSERT_EQUALS( hashSet.size(), 3U );

        TS_ASSERT( hashSet.exists( 1 ) );
        TS_ASSERT( hashSet.exists( 2 ) );
        TS_ASSERT( hashSet.exists( 3 ) );
    }

    void test_insert()
    {
        HashSet<int> hashSet;

        TS_ASSERT_EQUALS( hashSet.begin(), hashSet.end() );

        hashSet.insert( 5 );
        hashSet.insert( 8 );
        hashSet.insert( 3 );

        TS_ASSERT( hashSet.exists( 3 ) );
        TS_ASSERT( hashSet.exists( 5 ) );
        TS_ASSERT( hashSet.exists( 8 ) );
        TS_ASSERT( !hashSet.exists( 4 ) );
        TS_ASSERT( !hashSet.exists( 6 ) );

        TS_ASSERT_EQUALS( hashSet.size(), 3u );
        hashSet.insert( 3 );
        TS_ASSERT_EQUALS( hashSet.size(), 3u );
        hashSet.insert( 4 );
        TS_ASSERT_EQUALS( hashSet.size(), 4u );
    }

    void test_insert_hashSet()
    {
        HashSet<int> hashSet;
        HashSet<int> other;

        hashSet.insert( 5 );
        hashSet.insert( 8 );
        hashSet.insert( 3 );

        other.insert( 1 );
        other.insert( 4 );
        other.insert( 5 );
        other.insert( 7 );

        TS_ASSERT( hashSet.exists( 3 ) );
        TS_ASSERT( hashSet.exists( 5 ) );
        TS_ASSERT( hashSet.exists( 8 ) );
        TS_ASSERT( !hashSet.exists( 1 ) );
        TS_ASSERT( !hashSet.exists( 4 ) );
        TS_ASSERT( !hashSet.exists( 7 ) );

        TS_ASSERT_THROWS_NOTHING( hashSet.insert( other ) );

        TS_ASSERT_EQUALS( hashSet.size(), 6u );

        TS_ASSERT( hashSet.exists( 3 ) );
        TS_ASSERT( hashSet.exists( 5 ) );
        TS_ASSERT( hashSet.exists( 8 ) );
        TS_ASSERT( hashSet.exists( 1 ) );
        TS_ASSERT( hashSet.exists( 4 ) );
        TS_ASSERT( hashSet.exists( 7 ) );
    }

    void test_clear()
    {
        HashSet<int> hashSet;

        hashSet.insert( 5 );
        hashSet.insert( 8 );
        hashSet.insert( 3 );

        TS_ASSERT( hashSet.exists( 3 ) );
        TS_ASSERT( hashSet.exists( 5 ) );
        TS_ASSERT( hashSet.exists( 8 ) );
        TS_ASSERT_EQUALS( hashSet.size(), 3u );

        TS_ASSERT_THROWS_NOTHING( hashSet.clear() );

        TS_ASSERT( !hashSet.exists( 3 ) );
        TS_ASSERT( !hashSet.exists( 5 ) );
        TS_ASSERT( !hashSet.exists( 8 ) );
        TS_ASSERT_EQUALS( hashSet.size(), 0u );
    }

    void test_erase()
    {
        HashSet<int> hashSet;

        hashSet.insert( 1 );
        hashSet.insert( 2 );
        hashSet.insert( 3 );
        hashSet.insert( 4 );

        TS_ASSERT_EQUALS( hashSet.size(), 4U );

        TS_ASSERT_THROWS_NOTHING( hashSet.erase( 2 ) );

        TS_ASSERT_EQUALS( hashSet.size(), 3U );
        TS_ASSERT( !hashSet.exists( 2 ) );

        TS_ASSERT_THROWS_NOTHING( hashSet.erase( 3 ) );

        TS_ASSERT_EQUALS( hashSet.size(), 2U );
        TS_ASSERT( !hashSet.exists( 3 ) );

        TS_ASSERT( hashSet.exists( 1 ) );
        TS_ASSERT( hashSet.exists( 4 ) );

        TS_ASSERT_THROWS_NOTHING( hashSet.erase( 5 ) );
    }

    void test_find()
    {
        HashSet<int> hashSet;

        hashSet.insert( 1 );
        hashSet.insert( 2 );
        hashSet.insert( 3 );
        hashSet.insert( 4 );

        HashSet<int>::const_iterator it = hashSet.find( 3 );

        TS_ASSERT_EQUALS( *it, 3 );
    }

    void test_operator_plus_equals()
    {
        HashSet<int> a, b;

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
        HashSet<int> a, b;

        a.insert( 1 );
        a.insert( 3 );

        b.insert( 2 );
        b.insert( 4 );

        HashSet<int> c;
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

    void test_equality()
    {
        HashSet<int> a, b;

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
