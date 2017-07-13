#include <cxxtest/TestSuite.h>

#include "MString.h"
#include "MockErrno.h"
#include "Vector.h"

class VectorTestSuite : public CxxTest::TestSuite
{
public:
    MockErrno *mockErrno;

    void setUp()
    {
        TS_ASSERT( mockErrno = new MockErrno );
    }

    void tearDown()
    {
        TS_ASSERT_THROWS_NOTHING( delete mockErrno );
    }

    void test_brackets()
    {
        Vector<String> vector;

        vector.append( "Apple" );
        vector.append( "Red" );
        vector.append( "Tasty" );

        TS_ASSERT_EQUALS( vector[0], "Apple" );
        TS_ASSERT_EQUALS( vector[1], "Red" );
        TS_ASSERT_EQUALS( vector[2], "Tasty" );
    }

    void test_size_and_exists()
    {
        Vector<String> vector;

        TS_ASSERT_EQUALS( vector.size(), 0u );
        TS_ASSERT( vector.empty() );

        vector.append( "Apple" );
        TS_ASSERT_EQUALS( vector.size(), 1u );
        TS_ASSERT( vector.exists( "Apple" ) );
        TS_ASSERT( !vector.exists( "Red" ) );

        TS_ASSERT( !vector.empty() );

        vector.append( "Red" );
        TS_ASSERT_EQUALS( vector.size(), 2u );
        TS_ASSERT( vector.exists( "Apple" ) );
        TS_ASSERT( vector.exists( "Red" ) );

        vector.append( "Tasty" );
        TS_ASSERT_EQUALS( vector.size(), 3u );
        TS_ASSERT( vector.exists( "Apple" ) );
        TS_ASSERT( vector.exists( "Red" ) );
        TS_ASSERT( vector.exists( "Tasty" ) );
    }

    void test_erase()
    {
        Vector<String> vector;

        vector.append( "Apple" );
        TS_ASSERT( vector.exists( "Apple" ) );
        TS_ASSERT_THROWS_NOTHING( vector.erase( "Apple" ) );
        TS_ASSERT( !vector.exists( "Apple" ) );
        TS_ASSERT_EQUALS( vector.size(), 0U );

        vector.append( "Red" );
        vector.append( "Tasty" );
        TS_ASSERT( !vector.exists( "Apple" ) );
        TS_ASSERT( vector.exists( "Red" ) );
        TS_ASSERT( vector.exists( "Tasty" ) );

        TS_ASSERT_THROWS_NOTHING( vector.erase( "Tasty" ) );
        TS_ASSERT( !vector.exists( "Apple" ) );
        TS_ASSERT( vector.exists( "Red" ) );
        TS_ASSERT( !vector.exists( "Tasty" ) );

        TS_ASSERT_THROWS_EQUALS( vector.erase( "Bla" ),
                                 const CommonError &e,
                                 e.getCode(),
                                 CommonError::VALUE_DOESNT_EXIST_IN_VECTOR );

        Vector<String> anotherVector;
        anotherVector.append( "1" );

        String toDelete = anotherVector[0];

        TS_ASSERT_THROWS_NOTHING( anotherVector.erase( toDelete ) );
        TS_ASSERT_EQUALS( anotherVector.size(), 0u );
    }

    void test_eraseByValue()
    {
        Vector<String> vector;

        vector.append( "Apple" );
        TS_ASSERT( vector.exists( "Apple" ) );
        TS_ASSERT_THROWS_NOTHING( vector.eraseByValue( "Apple" ) );
        TS_ASSERT( !vector.exists( "Apple" ) );
        TS_ASSERT_EQUALS( vector.size(), 0U );

        vector.append( "Red" );
        vector.append( "Tasty" );
        TS_ASSERT( !vector.exists( "Apple" ) );
        TS_ASSERT( vector.exists( "Red" ) );
        TS_ASSERT( vector.exists( "Tasty" ) );

        TS_ASSERT_THROWS_NOTHING( vector.eraseByValue( "Tasty" ) );
        TS_ASSERT( !vector.exists( "Apple" ) );
        TS_ASSERT( vector.exists( "Red" ) );
        TS_ASSERT( !vector.exists( "Tasty" ) );

        TS_ASSERT_THROWS_EQUALS( vector.eraseByValue( "Bla" ),
                                 const CommonError &e,
                                 e.getCode(),
                                 CommonError::VALUE_DOESNT_EXIST_IN_VECTOR );

        Vector<String> anotherVector;
        anotherVector.append( "1" );

        String toDelete = anotherVector[0];

        TS_ASSERT_THROWS_NOTHING( anotherVector.eraseByValue( toDelete ) );
        TS_ASSERT_EQUALS( anotherVector.size(), 0u );
    }

    void test_erase_at()
    {
        Vector<int> vector;

        vector.append( 2 );
        vector.append( 4 );
        vector.append( 6 );
        vector.append( 8 );

        vector.eraseAt( 0 );

        TS_ASSERT_EQUALS( vector[0], 4 );
        TS_ASSERT_EQUALS( vector[1], 6 );
        TS_ASSERT_EQUALS( vector[2], 8 );

        vector.eraseAt( 1 );

        TS_ASSERT_EQUALS( vector[0], 4 );
        TS_ASSERT_EQUALS( vector[1], 8 );

        TS_ASSERT_THROWS_EQUALS( vector.eraseAt( 17 ),
                                 const CommonError &e,
                                 e.getCode(),
                                 CommonError::VECTOR_OUT_OF_BOUNDS );
    }

    void test_clear()
    {
        Vector<unsigned> vector;

        vector.append( 5 );
        vector.append( 10 );

        TS_ASSERT_EQUALS( vector.size(), 2U );

        TS_ASSERT_THROWS_NOTHING( vector.clear() );

        TS_ASSERT_EQUALS( vector.size(), 0U );

        TS_ASSERT( !vector.exists( 5 ) );
        TS_ASSERT( !vector.exists( 10 ) );

        vector.append( 10 );

        TS_ASSERT_EQUALS( vector.size(), 1U );
        TS_ASSERT( vector.exists( 10 ) );
    }

    void test_concatenation()
    {
        Vector<unsigned> one;
        Vector<unsigned> two;
        Vector<unsigned> output;

        one.append( 1 );
        one.append( 15 );

        two.append( 27 );
        two.append( 13 );

        output = one + two;

        TS_ASSERT_EQUALS( output.size(), 4U );

        TS_ASSERT_EQUALS( output[0], 1U );
        TS_ASSERT_EQUALS( output[1], 15U );
        TS_ASSERT_EQUALS( output[2], 27U );
        TS_ASSERT_EQUALS( output[3], 13U );

        TS_ASSERT_EQUALS( one.size(), 2U );

        one += two;

        TS_ASSERT_EQUALS( one.size(), 4U );

        TS_ASSERT_EQUALS( one[0], 1U );
        TS_ASSERT_EQUALS( one[1], 15U );
        TS_ASSERT_EQUALS( one[2], 27U );
        TS_ASSERT_EQUALS( one[3], 13U );
    }

    void test_pop_first()
    {
        Vector<int> vector;

        vector.append( 1 );
        vector.append( 2 );
        vector.append( 3 );

        TS_ASSERT_EQUALS( vector.popFirst(), 1 );
        TS_ASSERT_EQUALS( vector.size(), 2U );

        TS_ASSERT_EQUALS( vector.popFirst(), 2 );
        TS_ASSERT_EQUALS( vector.size(), 1U );

        TS_ASSERT_EQUALS( vector.popFirst(), 3 );
        TS_ASSERT_EQUALS( vector.size(), 0U );

        TS_ASSERT_THROWS_EQUALS( vector.popFirst(),
                                 const CommonError &e,
                                 e.getCode(),
                                 CommonError::POPPING_FROM_EMPTY_VECTOR );
    }

    void test_pop()
    {
        Vector<int> vector;

        vector.append( 1 );
        vector.append( 2 );
        vector.append( 3 );

        TS_ASSERT_EQUALS( vector.pop(), 3 );
        TS_ASSERT_EQUALS( vector.size(), 2U );

        TS_ASSERT_EQUALS( vector.pop(), 2 );
        TS_ASSERT_EQUALS( vector.size(), 1U );

        TS_ASSERT_EQUALS( vector.pop(), 1 );
        TS_ASSERT_EQUALS( vector.size(), 0U );

        TS_ASSERT_THROWS_EQUALS( vector.pop(),
                                 const CommonError &e,
                                 e.getCode(),
                                 CommonError::POPPING_FROM_EMPTY_VECTOR );
    }

    void test_first()
    {
        Vector<int> vector;

        TS_ASSERT_THROWS_EQUALS( vector.first(),
                                 const CommonError &e,
                                 e.getCode(),
                                 CommonError::VECTOR_OUT_OF_BOUNDS );

        vector.append( 1 );
        TS_ASSERT_EQUALS( vector.first(), 1 );

        vector.append( 2 );
        TS_ASSERT_EQUALS( vector.first(), 1 );

        vector.append( 3 );
        TS_ASSERT_EQUALS( vector.first(), 1 );

        vector.erase( 1 );
        TS_ASSERT_EQUALS( vector.first(), 2 );
    }

    void test_last()
    {
        Vector<int> vector;

        TS_ASSERT_THROWS_EQUALS( vector.last(),
                                 const CommonError &e,
                                 e.getCode(),
                                 CommonError::VECTOR_OUT_OF_BOUNDS );

        vector.append( 1 );
        TS_ASSERT_EQUALS( vector.last(), 1 );

        vector.append( 2 );
        TS_ASSERT_EQUALS( vector.last(), 2 );

        vector.append( 3 );
        TS_ASSERT_EQUALS( vector.last(), 3 );
    }

    void test_insert_head()
    {
        Vector<int> vector;

        vector.append( 1 );
        vector.append( 2 );

        vector.insertHead( 3 );

        TS_ASSERT_EQUALS( vector.size(), 3U );
        TS_ASSERT_EQUALS( vector[0], 3 );
        TS_ASSERT_EQUALS( vector[1], 1 );
        TS_ASSERT_EQUALS( vector[2], 2 );

        vector.insertHead( 4 );

        TS_ASSERT_EQUALS( vector.size(), 4U );
        TS_ASSERT_EQUALS( vector[0], 4 );
        TS_ASSERT_EQUALS( vector[1], 3 );
        TS_ASSERT_EQUALS( vector[2], 1 );
        TS_ASSERT_EQUALS( vector[3], 2 );
    }

    void test_equality()
    {
        Vector<int> a;
        Vector<int> b;

        TS_ASSERT( a == b );
        TS_ASSERT( a == a );
        TS_ASSERT( b == b );

        b.append( 1 );

        TS_ASSERT( a != b );

        a.append( 1 );

        TS_ASSERT( a == b );

        a.append( 3 );
        a.append( 2 );

        b.append( 2 );

        TS_ASSERT( a != b );

        b.append( 3 );

        TS_ASSERT( a == b );
    }

    void test_equality_complex()
    {
        Vector<int> a, b;

        a.append( 1 );
        a.append( 2 );
        a.append( 1 );

        b.append( 1 );
        b.append( 2 );
        b.append( 2 );

        TS_ASSERT( !( a == b ) );
    }

    void test_sort()
    {
        Vector<int> a;

        a.append( 2 );
        a.append( 3 );
        a.append( 1 );

        TS_ASSERT_EQUALS( a[0], 2 );

        TS_ASSERT_THROWS_NOTHING( a.sort() );

        TS_ASSERT_EQUALS( a[0], 1 );
        TS_ASSERT_EQUALS( a[1], 2 );
        TS_ASSERT_EQUALS( a[2], 3 );

        Vector<String> b;

        b.append( "egg" );
        b.append( "dog" );
        b.append( "cat" );

        TS_ASSERT_EQUALS( b[0], "egg" );

        TS_ASSERT_THROWS_NOTHING( b.sort() );

        TS_ASSERT_EQUALS( b[0], "cat" );
        TS_ASSERT_EQUALS( b[1], "dog" );
        TS_ASSERT_EQUALS( b[2], "egg" );
    }

    void test_erase_by_iterator()
    {
        Vector<int> a;

        a.append( 1 );
        a.append( 2 );
        a.append( 3 );
        a.append( 4 );

        Vector<int>::iterator it = a.begin();

        TS_ASSERT_EQUALS( *it, 1 );

        TS_ASSERT_THROWS_NOTHING( a.erase( it ) );

        TS_ASSERT_EQUALS( *it, 2 );

        TS_ASSERT_EQUALS( a.size(), 3U );

        ++it;

        TS_ASSERT_EQUALS( *it, 3 );
        TS_ASSERT_THROWS_NOTHING( a.erase( it ) );
        TS_ASSERT_EQUALS( *it, 4 );

        ++it;

        TS_ASSERT_EQUALS( it, a.end() );

        TS_ASSERT_EQUALS( a.size(), 2U );
    }

    void test_assignemnt()
    {
        Vector<int> a,b;

        a.append( 1 );
        a.append( 2 );

        TS_ASSERT_EQUALS( a.size(), 2U );
        TS_ASSERT_EQUALS( b.size(), 0U );

        b = a;

        TS_ASSERT_EQUALS( a.size(), 2U );
        TS_ASSERT_EQUALS( b.size(), 2U );

        TS_ASSERT_EQUALS( b[0], 1 );
        TS_ASSERT_EQUALS( b[1], 2 );

        a.erase( 1 );
        TS_ASSERT_EQUALS( b.size(), 2U );
    }

    void test_get()
    {
        Vector<int> a;

        a.append( 1 );
        a.append( 2 );

        TS_ASSERT_EQUALS( a.get( 0 ), a[0] );
        TS_ASSERT_EQUALS( a.get( 1 ), a[1] );

        a[0] = 13;

        TS_ASSERT_EQUALS( a.get( 0 ), 13 );
        TS_ASSERT_EQUALS( a.get( 0 ), a[0] );
    }
};

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
