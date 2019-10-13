/*********************                                                        */
/*! \file TimeUtils.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

**/

#include "MStringf.h"
#include "TimeUtils.h"

struct timespec TimeUtils::sampleMicro()
{
    struct timespec now;
#ifdef _WIN32
	LARGE_INTEGER count;

	if (g_first_time)
	{
		g_first_time = 0;

		if (0 == QueryPerformanceFrequency(&g_counts_per_sec))
		{
			g_counts_per_sec.QuadPart = 0;
		}
	}
	QueryPerformanceCounter(&count);
	now.tv_sec = count.QuadPart / g_counts_per_sec.QuadPart;
	now.tv_nsec = ((count.QuadPart % g_counts_per_sec.QuadPart) * static_cast<long>(1000000000)) / g_counts_per_sec.QuadPart;
#else
    clock_gettime( CLOCK_MONOTONIC, &now );
#endif
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
    time_t secondsSinceEpoch = time( NULL );
    struct tm *formattedTime = localtime( &secondsSinceEpoch);

    return Stringf( "%02u:%02u:%02u", formattedTime->tm_hour, formattedTime->tm_min, formattedTime->tm_sec );
}

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
