/*********************                                                        */
/*! \file Test_FloatUtils.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** \brief [[ Add one-line brief description here ]]
 **
 ** [[ Add lengthier description here ]]
 **/

#include <cxxtest/TestSuite.h>

#include "FloatUtils.h"
#include <math.h>

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

    void test_well_formed()
    {
        TS_ASSERT( FloatUtils::wellFormed( -1 ) );
        TS_ASSERT( !FloatUtils::wellFormed( sqrt( -1 ) ) );
        float zero = 0.0;
        TS_ASSERT( !FloatUtils::wellFormed( 5.0 / zero ) );
    }

    void test_isnan()
    {
        TS_ASSERT( !FloatUtils::isNan( -1 ) );
        TS_ASSERT( FloatUtils::isNan( sqrt( -1 ) ) );
        float zero = 0.0;
        TS_ASSERT( !FloatUtils::isNan( 5.0 / zero ) );
    }

    void test_isinf()
    {
        TS_ASSERT( !FloatUtils::isInf( -1 ) );
        TS_ASSERT( !FloatUtils::isInf( sqrt( -1 ) ) );
        float zero = 0.0;
        TS_ASSERT( FloatUtils::isInf( 5.0 / zero ) );
    }
};

//
// Local Variables:
// compile-command: "make -C ../../.. "
// tags-file-name: "../../../TAGS"
// c-basic-offset: 4
// End:
//
