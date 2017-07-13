#include <cxxtest/TestSuite.h>

#include "MString.h"
#include "Pair.h"

class PairTestSuite : public CxxTest::TestSuite
{
public:

    void test_pair()
    {
        Pair<int, String> p1;
        Pair<int, String> p2( 5, "dog" );

        TS_ASSERT( p1 != p2 );
        TS_ASSERT( !( p1 == p2 ) );

        p1.first() = 5;

        TS_ASSERT( p1 != p2 );
        TS_ASSERT( !( p1 == p2 ) );

        p1.second() = "dog";

        TS_ASSERT( p1 == p2 );
        TS_ASSERT( !( p1 != p2 ) );

        p2.second() = "bird";

        TS_ASSERT( p1 != p2 );
        TS_ASSERT( !( p1 == p2 ) );

        p1 = p2;

        TS_ASSERT( p1 == p2 );
        TS_ASSERT( !( p1 != p2 ) );
    }
};

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
