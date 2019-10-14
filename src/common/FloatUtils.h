/*********************                                                        */
/*! \file FloatUtils.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz, Duligur Ibeling
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

 **/

#ifndef __FloatUtils_h__
#define __FloatUtils_h__

#include "GlobalConfiguration.h"
#include "MString.h"

#include <cfloat>

class FloatUtils
{
public:
    static bool areEqual( double x, double y, double epsilon = GlobalConfiguration::DEFAULT_EPSILON_FOR_COMPARISONS );
    static double abs( double x );
    static bool areDisequal( double x,
                             double y,
                             double epsilon = GlobalConfiguration::DEFAULT_EPSILON_FOR_COMPARISONS );
static bool isZero( double x, double epsilon = GlobalConfiguration::DEFAULT_EPSILON_FOR_COMPARISONS );
    static double roundToZero( double x, double epsilon = GlobalConfiguration::DEFAULT_EPSILON_FOR_COMPARISONS );
inline static bool isPositive( double x, double epsilon = GlobalConfiguration::DEFAULT_EPSILON_FOR_COMPARISONS ){
    return x > epsilon;
}
inline static bool isNegative( double x, double epsilon = GlobalConfiguration::DEFAULT_EPSILON_FOR_COMPARISONS ){
    return x < -epsilon;
}
    static bool gt( double x, double y, double epsilon = GlobalConfiguration::DEFAULT_EPSILON_FOR_COMPARISONS );
    static bool gte( double x, double y, double epsilon = GlobalConfiguration::DEFAULT_EPSILON_FOR_COMPARISONS );
    static bool lt( double x, double y, double epsilon = GlobalConfiguration::DEFAULT_EPSILON_FOR_COMPARISONS );
    static bool lte( double x, double y, double epsilon = GlobalConfiguration::DEFAULT_EPSILON_FOR_COMPARISONS );
    static double min( double x, double y, double epsilon = GlobalConfiguration::DEFAULT_EPSILON_FOR_COMPARISONS );
    static double max( double x, double y, double epsilon = GlobalConfiguration::DEFAULT_EPSILON_FOR_COMPARISONS );
    static double infinity();
    static double negativeInfinity();
    static bool isFinite( double x );
    static String doubleToString( double x,
                                  unsigned precision = GlobalConfiguration::DEFAULT_DOUBLE_TO_STRING_PRECISION );
    static bool wellFormed( double x );
    static bool isNan( double x );
    static bool isInf( double x );
};

#endif // __FloatUtils_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
