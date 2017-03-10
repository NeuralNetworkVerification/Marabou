#include <cxxtest/TestSuite.h>

#include "MockErrno.h"
#include "Queue.h"
#include "String.h"

class QueueTestSuite : public CxxTest::TestSuite
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

    void test_basic_queue()
    {
        Queue<int> q;

        TS_ASSERT( q.empty() );

        TS_ASSERT_THROWS_NOTHING( q.push( 1 ) );
        TS_ASSERT( !q.empty() );
        TS_ASSERT_THROWS_NOTHING( q.push( 2 ) );
        TS_ASSERT_THROWS_NOTHING( q.push( 3 ) );

        TS_ASSERT_EQUALS( q.peak(), 1 );
        TS_ASSERT_EQUALS( q.peak(), 1 );
        TS_ASSERT_THROWS_NOTHING( q.pop() );
        TS_ASSERT_EQUALS( q.peak(), 2 );
        TS_ASSERT_THROWS_NOTHING( q.pop() );
        TS_ASSERT_EQUALS( q.peak(), 3 );

        TS_ASSERT( !q.empty() );

        TS_ASSERT_THROWS_NOTHING( q.pop() );

        TS_ASSERT( q.empty() );
    }

    void test_clear_and_throw()
    {
        Queue<int> q;

        TS_ASSERT( q.empty() );

        q.push( 1 );

        TS_ASSERT( !q.empty() );

        TS_ASSERT_THROWS_NOTHING( q.clear() );

        TS_ASSERT( q.empty() );

        TS_ASSERT_THROWS_EQUALS( q.peak(),
                                 const CommonError &e,
                                 e.getCode(),
                                 CommonError::QUEUE_IS_EMPTY );

        TS_ASSERT_THROWS_EQUALS( q.pop(),
                                 const CommonError &e,
                                 e.getCode(),
                                 CommonError::QUEUE_IS_EMPTY );
    }
};

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
