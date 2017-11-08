/*********************                                                        */
/*! \file TimeUtils.cpp
** \verbatim
** Top contributors (to current version):
**   Guy Katz
** This file is part of the Marabou project.
** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
** in the top-level source directory) and their institutional affiliations.
** All rights reserved. See the file COPYING in the top-level source
** directory for licensing information.\endverbatim
**/

#include "MStringf.h"
#include "TimeUtils.h"

struct timespec TimeUtils::sampleMicro()
{
    struct timespec now;
    clock_gettime( CLOCK_MONOTONIC, &now );
    return now;
}

unsigned long long TimeUtils::timePassed( const struct timespec &then,
                                          const struct timespec &now )
{
    enum {
        MICROSECONDS_IN_SECOND = 1000000,
        NANOSECONDS_IN_MICROSECOND = 1000,
    };

    unsigned long long secondsAsMicro = ( now.tv_sec - then.tv_sec ) * MICROSECONDS_IN_SECOND;
    unsigned long long nanoAsMicro = ( now.tv_nsec - then.tv_nsec ) / NANOSECONDS_IN_MICROSECOND;

    return secondsAsMicro + nanoAsMicro;
}

String TimeUtils::now()
{
    time_t secodnsSinceEpoch = time( NULL );
    struct tm *formattedTime = localtime( &secodnsSinceEpoch );

    return Stringf( "%02u:%02u:%02u", formattedTime->tm_hour, formattedTime->tm_min, formattedTime->tm_sec );
}

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
