/*********************                                                        */
/*! \file TimeUtils.h
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

#ifndef __TimeUtils_h__
#define __TimeUtils_h__

#include "MString.h"

#include <time.h>

#ifdef _WIN32
#include <winsock2.h>
static BOOL g_first_time = 1;
static LARGE_INTEGER g_counts_per_sec;
#else
#include <sys/time.h>
#endif

class TimeUtils
{
public:
    static struct timespec sampleMicro();
    static unsigned long long timePassed( const struct timespec &then,
                                          const struct timespec &now );
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
