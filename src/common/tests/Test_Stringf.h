#include <cxxtest/TestSuite.h>

#include "Stringf.h"

class StringfTestSuite : public CxxTest::TestSuite
{
public:
	void test_constructor()
	{
        Stringf stringf( "Hello %s %u world", "bla", 5 );
        String expected( "Hello bla 5 world" );

        TS_ASSERT_EQUALS( stringf, expected );
    }
};

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
