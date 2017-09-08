#include <cxxtest/TestSuite.h>

#include "Error.h"
#include "MString.h"
#include "MockErrno.h"

class ErrorTestSuite : public CxxTest::TestSuite
{
public:
	void test_code_and_errno()
	{
        MockErrno mockErrno;

        mockErrno.nextErrno = 21;

        const char *msg = "common error";
		Error error1( msg, 5 );
		TS_ASSERT_EQUALS( error1.getCode(), 5 );
        TS_ASSERT_EQUALS( error1.getErrno(), 21 );
        TS_ASSERT_EQUALS( String( error1.getErrorClass() ), String( "common error" ) );

        mockErrno.nextErrno = 23;

		Error error2( "common error", 166, "user message" );
		TS_ASSERT_EQUALS( error2.getCode(), 166 );
        TS_ASSERT_EQUALS( error2.getErrno(), 23 );
        TS_ASSERT_EQUALS( String( error2.getErrorClass() ), String( "common error" ) );
        TS_ASSERT_EQUALS( String( error2.getUserMessage() ), String( "user message" ) );
	}
};

//
// Local Variables:
// compile-command: "make -C ../../.. "
// tags-file-name: "../../../TAGS"
// c-basic-offset: 4
// End:
//
