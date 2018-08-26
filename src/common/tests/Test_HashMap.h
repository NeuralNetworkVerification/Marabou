#include <cxxtest/TestSuite.h>


#include "HashMap.h"
#include "MString.h"
#include "Map.h"
#include "MockErrno.h"
class HashMapTestSuite : public CxxTest::TestSuite
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
        HashMap<unsigned, String> hashMap;

        hashMap[1] = "Apple";
        hashMap[2] = "Red";
        hashMap[3] = "Tasty";

        TS_ASSERT_EQUALS( hashMap[1], "Apple" );
        TS_ASSERT_EQUALS( hashMap[2], "Red" );
        TS_ASSERT_EQUALS( hashMap[3], "Tasty" );

        hashMap.set( 4, "bla" );

        TS_ASSERT_EQUALS( hashMap[1], "Apple" );
        TS_ASSERT_EQUALS( hashMap[2], "Red" );
        TS_ASSERT_EQUALS( hashMap[3], "Tasty" );
        TS_ASSERT_EQUALS( hashMap[4], "bla" );

        hashMap.set( 4, "otherBla" );

        TS_ASSERT_EQUALS( hashMap[1], "Apple" );
        TS_ASSERT_EQUALS( hashMap[2], "Red" );
        TS_ASSERT_EQUALS( hashMap[3], "Tasty" );
        TS_ASSERT_EQUALS( hashMap[4], "otherBla" );
    }

    void test_size_and_exists()
    {
        HashMap<unsigned, String> hashMap;

        TS_ASSERT_EQUALS( hashMap.size(), 0u );
        TS_ASSERT( hashMap.empty() );
        TS_ASSERT( !hashMap.exists( 1 ) );

        hashMap[1] = "Apple";
        TS_ASSERT_EQUALS( hashMap.size(), 1u );
        TS_ASSERT( !hashMap.empty() );
        TS_ASSERT( hashMap.exists( 1 ) );

        hashMap[2] = "Red";
        TS_ASSERT_EQUALS( hashMap.size(), 2u );
        TS_ASSERT( hashMap.exists( 1 ) );
        TS_ASSERT( hashMap.exists( 2 ) );
        TS_ASSERT( !hashMap.exists( 3 ) );

        hashMap[3] = "Tasty";
        TS_ASSERT_EQUALS( hashMap.size(), 3u );
        TS_ASSERT( hashMap.exists( 1 ) );
        TS_ASSERT( hashMap.exists( 2 ) );
        TS_ASSERT( hashMap.exists( 3 ) );
        TS_ASSERT( !hashMap.exists( 4 ) );

        hashMap[2] = "OtherRed";
        TS_ASSERT_EQUALS( hashMap.size(), 3u );
        TS_ASSERT( hashMap.exists( 1 ) );
        TS_ASSERT( hashMap.exists( 2 ) );
        TS_ASSERT( hashMap.exists( 3 ) );
        TS_ASSERT( !hashMap.exists( 4 ) );
    }

    void test_erase()
    {
        HashMap<unsigned, String> hashMap;

        hashMap[1] = "Apple";
        TS_ASSERT( hashMap.exists( 1 ) );
        TS_ASSERT_THROWS_NOTHING( hashMap.erase( 1 ) );
        TS_ASSERT( !hashMap.exists( 1 ) );
        TS_ASSERT_EQUALS( hashMap.size(), 0U );

        hashMap[2] = "Red";
        hashMap[3] = "Tasty";
        TS_ASSERT( !hashMap.exists( 1 ) );
        TS_ASSERT( hashMap.exists( 2 ) );
        TS_ASSERT( hashMap.exists( 3 ) );

        TS_ASSERT_THROWS_NOTHING( hashMap.erase( 3 ) );
        TS_ASSERT( !hashMap.exists( 1 ) );
        TS_ASSERT( hashMap.exists( 2 ) );
        TS_ASSERT( !hashMap.exists( 3 ) );

        TS_ASSERT_EQUALS( hashMap[2], "Red" );

        TS_ASSERT_THROWS_EQUALS( hashMap.erase( 1 ),
                                 const CommonError &e,
                                 e.getCode(),
                                 CommonError::KEY_DOESNT_EXIST_IN_HASHMAP );
    }

    void test_iterating()
    {
        HashMap<unsigned, String> hashMap;

        hashMap[1] = "Apple";
        hashMap[2] = "Red";
        hashMap[3] = "Tasty";

        HashMap<unsigned, String>::iterator it;
        it = hashMap.begin();

        Map<unsigned, String> answers;
        answers[1] = "Apple";
        answers[2] = "Red";
        answers[3] = "Tasty";

        for ( unsigned i = 0; i < 3; ++i )
        {
            TS_ASSERT( answers.exists( it->first ) );
            TS_ASSERT_EQUALS( it->second, answers[it->first] );
            ++it;
        }

        TS_ASSERT_EQUALS( it, hashMap.end() );
    }

    void test_clear()
    {
        HashMap<unsigned, String> hashMap;

        hashMap[1] = "Apple";
        hashMap[2] = "Red";
        hashMap[3] = "Tasty";

        TS_ASSERT_EQUALS( hashMap.size(), 3U );

        TS_ASSERT_THROWS_NOTHING( hashMap.clear() );
        TS_ASSERT_EQUALS( hashMap.size(), 0U );

        hashMap[1] = "Apple";

        TS_ASSERT_THROWS_NOTHING( hashMap.clear() );
        TS_ASSERT_EQUALS( hashMap.size(), 0U );

        TS_ASSERT_THROWS_NOTHING( hashMap.clear() );
        TS_ASSERT_EQUALS( hashMap.size(), 0U );
    }

    void test_get()
    {
        HashMap<unsigned, String> hashMap;

        hashMap[1] = "Apple";
        hashMap[2] = "Red";
        hashMap[3] = "Tasty";

        TS_ASSERT_EQUALS( hashMap.get( 1 ), "Apple" );
        TS_ASSERT_EQUALS( hashMap.get( 2 ), "Red" );
        TS_ASSERT_EQUALS( hashMap.get( 3 ), "Tasty" );

        TS_ASSERT_THROWS_EQUALS( hashMap.at( 4 ),
                                 const CommonError &e,
                                 e.getCode(),
                                 CommonError::KEY_DOESNT_EXIST_IN_HASHMAP );
    }

    void test_equality()
    {
        HashMap<unsigned, String> hashMap1, hashMap2;

        TS_ASSERT_EQUALS( hashMap1, hashMap2 );

        hashMap1[0] = "dog";
        hashMap1[1] = "cat";
        hashMap1[2] = "bird";

        TS_ASSERT_DIFFERS( hashMap1, hashMap2 );

        hashMap2[0] = "dog";
        hashMap2[1] = "cat";
        hashMap2[2] = "not_a_bird";

        TS_ASSERT_DIFFERS( hashMap1, hashMap2 );

        hashMap2[2] = "bird";

        TS_ASSERT_EQUALS( hashMap1, hashMap2 );

        hashMap1.erase( 1 );

        TS_ASSERT_DIFFERS( hashMap1, hashMap2 );
    }

    // void test_keys()
    // {
    //     HashMap<int, String> hashMap;
    //     hashMap[1] = "cat";
    //     hashMap[2] = "dog";
    //     hashMap[3] = "bear";

    //     Set<int> keys = hashMap.keys();

    //     TS_ASSERT_EQUALS( keys.size(), 3U );
    //     TS_ASSERT( keys.exists( 1 ) );
    //     TS_ASSERT( keys.exists( 2 ) );
    //     TS_ASSERT( keys.exists( 3 ) );
    // }

    void test_at()
    {
        HashMap<int, String> hashMap;
        hashMap[1] = "cat";
        hashMap[2] = "dog";
        hashMap[3] = "bear";

        TS_ASSERT_EQUALS( hashMap.at( 2 ), "dog" );

        TS_ASSERT_THROWS_NOTHING( hashMap.at( 3 ) = String( "fox" ) );

        TS_ASSERT_EQUALS( hashMap.at( 3 ), "fox" );

        TS_ASSERT_THROWS_EQUALS( hashMap.at( 4 ),
                                 const CommonError &e,
                                 e.getCode(),
                                 CommonError::KEY_DOESNT_EXIST_IN_HASHMAP );
    }
};

//
// Local Variables:
// compile-command: "make -C ../../.. "
// tags-file-name: "../../../TAGS"
// c-basic-offset: 4
// End:
//
