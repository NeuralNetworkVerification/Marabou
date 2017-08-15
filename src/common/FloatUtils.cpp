/*********************                                                        */
/*! \file FloatUtils.h
** \verbatim
** Top contributors (to current version):
**   Guy Katz
** This file is part of the Marabou project.
** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
** in the top-level source directory) and their institutional affiliations.
** All rights reserved. See the file COPYING in the top-level source
** directory for licensing information.\endverbatim
**/

#include "FloatUtils.h"
#include <iomanip>
#include <math.h>
#include <sstream>

// https://randomascii.wordpress.com/2012/02/25/comparing-floating-point-numbers-2012-edition/
// http://docs.oracle.com/cd/E19957-01/806-3568/ncg_goldberg.html

bool FloatUtils::areEqual( double x, double y, double epsilon )
{
    // Check if the numbers are really close -- needed
    // when comparing numbers near zero.
    double diff = fabs( x - y );
    if ( diff <= epsilon )
        return true;

    x = fabs( x );
    y = fabs( y );
    double largest = ( x > y ) ? x : y;

    if ( diff <= largest * DBL_EPSILON )
        return true;

    return false;
}

double FloatUtils::abs( double x )
{
    return fabs( x );
}

bool FloatUtils::areDisequal( double x, double y, double epsilon )
{
    return !areEqual( x, y, epsilon );
}

bool FloatUtils::isZero( double x, double epsilon )
{
    return areEqual( x, 0.0, epsilon );
}

bool FloatUtils::isPositive( double x, double epsilon )
{
    return ( !isZero( x, epsilon ) ) && ( x > 0.0 );
}

bool FloatUtils::isNegative( double x, double epsilon )
{
    return ( !isZero( x, epsilon ) ) && ( x < 0.0 );
}

bool FloatUtils::gt( double x, double y, double epsilon )
{
    return isPositive( x - y, epsilon );
}

bool FloatUtils::gte( double x, double y, double epsilon )
{
    return !isNegative( x - y, epsilon );
}

bool FloatUtils::lt( double x, double y, double epsilon )
{
    return gt( y, x, epsilon );
}

bool FloatUtils::lte( double x, double y, double epsilon )
{
    return gte( y, x, epsilon );
}

double FloatUtils::min( double x, double y, double epsilon )
{
    return lt( x, y, epsilon ) ? x : y;
}

double FloatUtils::max( double x, double y, double epsilon )
{
    return gt( x, y, epsilon ) ? x : y;
}

double FloatUtils::infinity()
{
    return DBL_MAX;
}

double FloatUtils::negativeInfinity()
{
    return -DBL_MAX;
}

String FloatUtils::doubleToString( double x, unsigned precision )
{
    std::ostringstream strout;
    strout << std::fixed << std::setprecision(precision) << x;
    std::string str = strout.str();
    size_t end = str.find_last_not_of( '0' ) + 1;
    str.erase( end );

    if ( str[str.size() - 1] == '.' )
        str = str.substr(0, str.size() - 1);

    return str;
}

//
// Local Variables:
// compile-command: "make -C .. "
// tags-file-name: "../TAGS"
// c-basic-offset: 4
// End:
//
