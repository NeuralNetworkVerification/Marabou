#include <cxxtest/TestSuite.h>

#include "FloatUtils.h"

class FloatUtilsTestSuite : public CxxTest::TestSuite
{
public:
    void test_floatUtils()
    {
        double epsilon = 0.0000005;

        TS_ASSERT( FloatUtils::areEqual( 0.001, 0.001, epsilon ) );
        TS_ASSERT( !FloatUtils::areDisequal( 0.001, 0.001, epsilon ) );

        TS_ASSERT( !FloatUtils::areEqual( 0.001, 0.002, epsilon ) );
        TS_ASSERT( FloatUtils::areDisequal( 0.001, 0.002, epsilon ) );

        TS_ASSERT( !FloatUtils::isZero( 0.001, epsilon ) );
        TS_ASSERT( FloatUtils::isZero( epsilon / 2, epsilon ) );

        TS_ASSERT_EQUALS( FloatUtils::abs( 0.001 ), 0.001 );
        TS_ASSERT_EQUALS( FloatUtils::abs( -0.001 ), 0.001 );

        TS_ASSERT( FloatUtils::isPositive( 0.001, epsilon ) );
        TS_ASSERT( !FloatUtils::isNegative( 0.001, epsilon ) );
        TS_ASSERT( !FloatUtils::isPositive( -0.001, epsilon ) );
        TS_ASSERT( FloatUtils::isNegative( -0.001, epsilon ) );

        TS_ASSERT( FloatUtils::gt( 0.002, 0.001, epsilon ) );
        TS_ASSERT( FloatUtils::gte( 0.002, 0.001, epsilon ) );
        TS_ASSERT( !FloatUtils::gt( 0.001, 0.001, epsilon ) );
        TS_ASSERT( FloatUtils::gte( 0.001, 0.001, epsilon ) );

        TS_ASSERT( FloatUtils::lt( 0.001, 0.002, epsilon ) );
        TS_ASSERT( FloatUtils::lte( 0.001, 0.002, epsilon ) );
        TS_ASSERT( !FloatUtils::lt( 0.002, 0.002, epsilon ) );
        TS_ASSERT( FloatUtils::lte( 0.002, 0.002, epsilon ) );

        TS_ASSERT_EQUALS( FloatUtils::max( 0.001, 0.002, epsilon ), 0.002 );
        TS_ASSERT_EQUALS( FloatUtils::max( 0.002, 0.001, epsilon ), 0.002 );
        TS_ASSERT_EQUALS( FloatUtils::min( 0.001, 0.002, epsilon ), 0.001 );
        TS_ASSERT_EQUALS( FloatUtils::min( 0.002, 0.001, epsilon ), 0.001 );

        TS_ASSERT_EQUALS( FloatUtils::doubleToString( 0.1 ), String( "0.1" ) );
        TS_ASSERT_EQUALS( FloatUtils::doubleToString( 0.1321211 ), String( "0.1321211" ) );
        TS_ASSERT_EQUALS( FloatUtils::doubleToString( 0.1321211, 3 ), String( "0.132" ) );
        TS_ASSERT_EQUALS( FloatUtils::doubleToString( -0.52 ), String( "-0.52" ) );
        TS_ASSERT_EQUALS( FloatUtils::doubleToString( 3.14 ), String( "3.14" ) );
    }
};

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
