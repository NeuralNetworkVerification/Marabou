#include <cxxtest/TestSuite.h>

#include "Map.h"
#include "MockErrno.h"
#include "String.h"

class MapTestSuite : public CxxTest::TestSuite
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
        Map<unsigned, String> map;

        map[1] = "Apple";
        map[2] = "Red";
        map[3] = "Tasty";

        TS_ASSERT_EQUALS( map[1], "Apple" );
        TS_ASSERT_EQUALS( map[2], "Red" );
        TS_ASSERT_EQUALS( map[3], "Tasty" );
    }

    void test_size_and_exists()
    {
        Map<unsigned, String> map;

        TS_ASSERT_EQUALS( map.size(), 0u );
        TS_ASSERT( map.empty() );
        TS_ASSERT( !map.exists( 1 ) );

        map[1] = "Apple";
        TS_ASSERT_EQUALS( map.size(), 1u );
        TS_ASSERT( !map.empty() );
        TS_ASSERT( map.exists( 1 ) );

        map[2] = "Red";
        TS_ASSERT_EQUALS( map.size(), 2u );
        TS_ASSERT( map.exists( 1 ) );
        TS_ASSERT( map.exists( 2 ) );
        TS_ASSERT( !map.exists( 3 ) );

        map[3] = "Tasty";
        TS_ASSERT_EQUALS( map.size(), 3u );
        TS_ASSERT( map.exists( 1 ) );
        TS_ASSERT( map.exists( 2 ) );
        TS_ASSERT( map.exists( 3 ) );
        TS_ASSERT( !map.exists( 4 ) );

        map[2] = "OtherRed";
        TS_ASSERT_EQUALS( map.size(), 3u );
        TS_ASSERT( map.exists( 1 ) );
        TS_ASSERT( map.exists( 2 ) );
        TS_ASSERT( map.exists( 3 ) );
        TS_ASSERT( !map.exists( 4 ) );
    }

    void test_erase()
    {
        Map<unsigned, String> map;

        map[1] = "Apple";
        TS_ASSERT( map.exists( 1 ) );
        TS_ASSERT_THROWS_NOTHING( map.erase( 1 ) );
        TS_ASSERT( !map.exists( 1 ) );
        TS_ASSERT_EQUALS( map.size(), 0U );

        map[2] = "Red";
        map[3] = "Tasty";
        TS_ASSERT( !map.exists( 1 ) );
        TS_ASSERT( map.exists( 2 ) );
        TS_ASSERT( map.exists( 3 ) );

        TS_ASSERT_THROWS_NOTHING( map.erase( 3 ) );
        TS_ASSERT( !map.exists( 1 ) );
        TS_ASSERT( map.exists( 2 ) );
        TS_ASSERT( !map.exists( 3 ) );

        TS_ASSERT_EQUALS( map[2], "Red" );

        TS_ASSERT_THROWS_EQUALS( map.erase( 1 ),
                                 const CommonError &e,
                                 e.getCode(),
                                 CommonError::KEY_DOESNT_EXIST_IN_MAP );
    }

    void test_iterating()
    {
        Map<unsigned, String> map;

        map[1] = "Apple";
        map[2] = "Red";
        map[3] = "Tasty";

        Map<unsigned, String>::iterator it;
        it = map.begin();

        TS_ASSERT_EQUALS( it->first, 1U );
        TS_ASSERT_EQUALS( it->second, "Apple" );

        ++it;

        TS_ASSERT_EQUALS( it->first, 2U );
        TS_ASSERT_EQUALS( it->second, "Red" );

        ++it;

        TS_ASSERT_EQUALS( it->first, 3U );
        TS_ASSERT_EQUALS( it->second, "Tasty" );

        ++it;

        TS_ASSERT_EQUALS( it, map.end() );
    }

    void test_clear()
    {
        Map<unsigned, String> map;

        map[1] = "Apple";
        map[2] = "Red";
        map[3] = "Tasty";

        TS_ASSERT_EQUALS( map.size(), 3U );

        TS_ASSERT_THROWS_NOTHING( map.clear() );
        TS_ASSERT_EQUALS( map.size(), 0U );

        map[1] = "Apple";

        TS_ASSERT_THROWS_NOTHING( map.clear() );
        TS_ASSERT_EQUALS( map.size(), 0U );

        TS_ASSERT_THROWS_NOTHING( map.clear() );
        TS_ASSERT_EQUALS( map.size(), 0U );
    }

    void test_get()
    {
        Map<unsigned, String> map;

        map[1] = "Apple";
        map[2] = "Red";
        map[3] = "Tasty";

        TS_ASSERT_EQUALS( map.get( 1 ), "Apple" );
        TS_ASSERT_EQUALS( map.get( 2 ), "Red" );
        TS_ASSERT_EQUALS( map.get( 3 ), "Tasty" );

        TS_ASSERT_THROWS_EQUALS( map.at( 4 ),
                                 const CommonError &e,
                                 e.getCode(),
                                 CommonError::KEY_DOESNT_EXIST_IN_MAP );
    }

    void test_equality()
    {
        Map<unsigned, String> map1, map2;

        TS_ASSERT_EQUALS( map1, map2 );

        map1[0] = "dog";
        map1[1] = "cat";
        map1[2] = "bird";

        TS_ASSERT_DIFFERS( map1, map2 );

        map2[0] = "dog";
        map2[1] = "cat";
        map2[2] = "not_a_bird";

        TS_ASSERT_DIFFERS( map1, map2 );

        map2[2] = "bird";

        TS_ASSERT_EQUALS( map1, map2 );

        map1.erase( 1 );

        TS_ASSERT_DIFFERS( map1, map2 );
    }

    void test_keys()
    {
        Map<int, String> map;
        map[1] = "cat";
        map[2] = "dog";
        map[3] = "bear";

        Set<int> keys = map.keys();

        TS_ASSERT_EQUALS( keys.size(), 3U );
        TS_ASSERT( keys.exists( 1 ) );
        TS_ASSERT( keys.exists( 2 ) );
        TS_ASSERT( keys.exists( 3 ) );
    }

    void test_set_if_does_not_exists()
    {
        Map<int, String> map;
        map[1] = "cat";
        map[2] = "dog";
        map[3] = "bear";

        TS_ASSERT_THROWS_NOTHING( map.setIfDoesNotExist( 2, String( "otter" ) ) );

        TS_ASSERT_EQUALS( map[2], "dog" );

        TS_ASSERT_THROWS_NOTHING( map.setIfDoesNotExist( 4, String( "otter" ) ) );

        TS_ASSERT_EQUALS( map[4], "otter" );
    }

    void test_at()
    {
        Map<int, String> map;
        map[1] = "cat";
        map[2] = "dog";
        map[3] = "bear";

        TS_ASSERT_EQUALS( map.at( 2 ), "dog" );

        TS_ASSERT_THROWS_NOTHING( map.at( 3 ) = String( "fox" ) );

        TS_ASSERT_EQUALS( map.at( 3 ), "fox" );

        TS_ASSERT_THROWS_EQUALS( map.at( 4 ),
                                 const CommonError &e,
                                 e.getCode(),
                                 CommonError::KEY_DOESNT_EXIST_IN_MAP );
    }

    void test_rbegin_and_rend()
    {
        Map<int, String> map;

        map[1] = "cat";
        map[2] = "dog";
        map[3] = "bear";

        Map<int, String>::reverse_iterator it = map.rbegin();

        TS_ASSERT_EQUALS( it->first, 3 );
        ++it;

        TS_ASSERT_EQUALS( it->first, 2 );
        ++it;

        TS_ASSERT_EQUALS( it->first, 1 );
        ++it;

        TS_ASSERT_EQUALS( it, map.rend() );
    }

    void test_flip()
    {
        Map<int, String> map;

        auto flipped = map.flip();

        TS_ASSERT( flipped.empty() );

        map[1] = "cat";
        map[2] = "dog";
        map[3] = "bear";

        TS_ASSERT_THROWS_NOTHING( flipped = map.flip() );

        TS_ASSERT_EQUALS( flipped.size(), 3U );

        TS_ASSERT_EQUALS( flipped["cat"].size(), 1U );
        TS_ASSERT_EQUALS( *(flipped["cat"].begin()), 1 );

        TS_ASSERT_EQUALS( flipped["dog"].size(), 1U );
        TS_ASSERT_EQUALS( *(flipped["dog"].begin()), 2 );

        TS_ASSERT_EQUALS( flipped["bear"].size(), 1U );
        TS_ASSERT_EQUALS( *(flipped["bear"].begin()), 3 );

        map[4] = "dog";

        TS_ASSERT_THROWS_NOTHING( flipped = map.flip() );

        TS_ASSERT_EQUALS( flipped.size(), 3U );

        TS_ASSERT_EQUALS( flipped["cat"].size(), 1U );
        TS_ASSERT_EQUALS( *(flipped["cat"].begin()), 1 );

        TS_ASSERT_EQUALS( flipped["dog"].size(), 2U );
        TS_ASSERT_EQUALS( *(flipped["dog"].begin()), 2 );
        TS_ASSERT_EQUALS( flipped["dog"].back(), 4 );

        TS_ASSERT_EQUALS( flipped["bear"].size(), 1U );
        TS_ASSERT_EQUALS( *(flipped["bear"].begin()), 3 );
    }

    void test_key_with_largest_value()
    {
        Map<String, unsigned> map;

        TS_ASSERT_THROWS_EQUALS( map.keyWithLargestValue(),
                                 const CommonError &e,
                                 e.getCode(),
                                 CommonError::MAP_IS_EMPTY );

        map["dog"] = 5;
        map["cat"] = 16;
        map["fox"] = 7;

        TS_ASSERT_EQUALS( map.keyWithLargestValue(), String( "cat" ) );

        map["fox"] = 16;

        TS_ASSERT_EQUALS( map.keyWithLargestValue(), String( "cat" ) );

        map["dog"] = 25;

        TS_ASSERT_EQUALS( map.keyWithLargestValue(), String( "dog" ) );
    }

    void test_key_by_value()
    {
        Map<unsigned, String> map;

        map[5] = "dog";
        map[16] = "cat";
        map[7] = "fox";

        TS_ASSERT_EQUALS( map.keyByValue( "dog" ), 5U );
        TS_ASSERT_EQUALS( map.keyByValue( "cat" ), 16U );
        TS_ASSERT_EQUALS( map.keyByValue( "fox" ), 7U );

        TS_ASSERT_THROWS_EQUALS( map.keyByValue( "wrong value" ),
                                 const CommonError &e,
                                 e.getCode(),
                                 CommonError::KEY_DOESNT_EXIST_IN_MAP );
    }
};

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
