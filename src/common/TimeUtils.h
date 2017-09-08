/*********************                                                        */
/*! \file TimeUtils.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **/

#ifndef __TimeUtils_h__
#define __TimeUtils_h__

#include "MString.h"

#include <time.h>
#include <sys/time.h>

class TimeUtils
{
public:
    static time_t sample();
    static struct timeval sampleMicro();
    static String timePassed( time_t then, time_t now );

    // Return time in milliseconds
    static unsigned long long timePassed( struct timeval then, struct timeval now );
    static String now();
};

#endif // __TimeUtils_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
