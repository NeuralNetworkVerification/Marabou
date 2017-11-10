#include <cxxtest/TestSuite.h>

#include "List.h"
#include "MString.h"
#include "MockErrno.h"

class ListTestSuite : public CxxTest::TestSuite
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

    void test_append()
    {
        List<int> list;

        TS_ASSERT_EQUALS( list.begin(), list.end() );

        list.append( 5 );
        list.append( 8 );

        List<int>::iterator it = list.begin();

        TS_ASSERT_EQUALS( *it, 5 );
        ++it;
        TS_ASSERT_EQUALS( *it, 8 );
        ++it;
        TS_ASSERT_EQUALS( it, list.end() );

        --it;
        TS_ASSERT_EQUALS( *it, 8 );
        --it;
        TS_ASSERT_EQUALS( *it, 5 );
        TS_ASSERT_EQUALS( it, list.begin() );

        list.appendHead( 3 );
        TS_ASSERT_DIFFERS( it, list.begin() );
        --it;
        TS_ASSERT_EQUALS( it, list.begin() );
        TS_ASSERT_EQUALS( *it, 3 );
    }

    void test_erase()
    {
        List<int> list;

        list.append( 1 );
        list.append( 2 );
        list.append( 3 );

        TS_ASSERT( !list.empty() );
        TS_ASSERT_EQUALS( list.size(), 3U );

        List<int>::iterator it = list.begin();

        TS_ASSERT_EQUALS( *it, 1 );

        ++it;

        TS_ASSERT_EQUALS( *it, 2 );

        it = list.erase( it );

        TS_ASSERT_EQUALS( list.size(), 2U );
        TS_ASSERT_EQUALS( *it, 3 );
        --it;
        TS_ASSERT_EQUALS( *it, 1 );

        it = list.erase( it );

        TS_ASSERT_EQUALS( list.size(), 1U );
        TS_ASSERT_EQUALS( *it, 3 );

        list.clear();
        TS_ASSERT_EQUALS( list.size(), 0U );

        TS_ASSERT( list.empty() );
    }

    void test_erase_by_value()
    {
        List<int> list;

        list.append( 1 );
        list.append( 2 );
        list.append( 3 );

        TS_ASSERT_EQUALS( list.size(), 3U );

        list.erase( 2 );

        TS_ASSERT_EQUALS( list.size(), 2U );

        list.erase( 3 );

        TS_ASSERT_EQUALS( list.size(), 1U );

        TS_ASSERT_EQUALS( *( list.begin() ), 1 );
    }

    void test_exists()
    {
        List<int> list;

        TS_ASSERT( !list.exists( 1 ) );
        list.append( 1 );
        TS_ASSERT( list.exists( 1 ) );

        TS_ASSERT( !list.exists( 2 ) );
        list.append( 2 );
        TS_ASSERT( list.exists( 2 ) );

        list.erase( 1 );
        TS_ASSERT( !list.exists( 1 ) );
        TS_ASSERT( list.exists( 2 ) );
    }

    void test_front_and_back()
    {
        List<int> list;

        list.append( 1 );
        TS_ASSERT_EQUALS( list.front(), 1 );
        TS_ASSERT_EQUALS( list.back(), 1 );

        list.append( 2 );
        TS_ASSERT_EQUALS( list.front(), 1 );
        TS_ASSERT_EQUALS( list.back(), 2 );

        list.append( 3 );
        TS_ASSERT_EQUALS( list.front(), 1 );
        TS_ASSERT_EQUALS( list.back(), 3 );

        list.erase( 1 );
        TS_ASSERT_EQUALS( list.front(), 2 );
        TS_ASSERT_EQUALS( list.back(), 3 );
    }

    void test_operator_equality()
    {
        List<int> a, b;

        TS_ASSERT( a == b );

        a.append( 1 );

        TS_ASSERT( a != b );

        b.append( 1 );

        TS_ASSERT( a == b );

        a.append( 2 );

        TS_ASSERT( a != b );
    }

    void test_initializer_list()
    {
        List<int> a { 1, 2, 3 };

        TS_ASSERT_EQUALS( a.size(), 3U );

        auto it = a.begin();

        TS_ASSERT_EQUALS( *it, 1 );
        ++it;
        TS_ASSERT_EQUALS( *it, 2 );
        ++it;
        TS_ASSERT_EQUALS( *it, 3 );
    }

    void test_append_another_list()
    {
        List<int> a;
        List<int> b;

        a.append( 1 );
        a.append( 2 );
        a.append( 3 );

        b.append( 4 );
        b.append( 5 );
        b.append( 6 );

        TS_ASSERT_EQUALS( a.size(), 3U );

        a.append( b );

        TS_ASSERT_EQUALS( a.size(), 6U );

        TS_ASSERT_EQUALS( a, List<int> ( { 1, 2, 3, 4, 5, 6 } ) );
    }

    void test_reverse_iterator()
    {
        List<int> a;

        a.append( 1 );
        a.append( 2 );
        a.append( 3 );

        auto it = a.rbegin();
        TS_ASSERT_EQUALS( *it, 3 );

        ++it;
        TS_ASSERT_EQUALS( *it, 2 );

        ++it;
        TS_ASSERT_EQUALS( *it, 1 );

        ++it;
        TS_ASSERT_EQUALS( it, a.rend() );
    }

    void test_pop_back()
    {
        List<int> a;

        a.append( 1 );
        a.append( 2 );
        a.append( 3 );

        a.popBack();

        auto it = a.begin();
        TS_ASSERT_EQUALS( *it, 1 );

        ++it;
        TS_ASSERT_EQUALS( *it, 2 );

        ++it;
        TS_ASSERT_EQUALS( it, a.end() );

        a.popBack();

        it = a.begin();
        TS_ASSERT_EQUALS( *it, 1 );

        ++it;
        TS_ASSERT_EQUALS( it, a.end() );

        a.popBack();
        TS_ASSERT( a.empty() );

        TS_ASSERT_THROWS_EQUALS( a.popBack(),
                                 const CommonError &e,
                                 e.getCode(),
                                 CommonError::LIST_IS_EMPTY );
    }
};

//
// Local Variables:
// compile-command: "make -C ../../.. "
// tags-file-name: "../../../TAGS"
// c-basic-offset: 4
// End:
//
