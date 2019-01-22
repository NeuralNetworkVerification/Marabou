/*********************                                                        */
/*! \file stat.h
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

#ifndef __T__sys__Stat_h__
#define __T__sys__Stat_h__

#include <cxxtest/Mock.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

CXXTEST_MOCK_GLOBAL( int,
					 open,
					 ( const char *pathname, int flags, mode_t mode ),
					 ( pathname, flags, mode  ) );

#endif // __T__sys__Stat_h__

//
// Local Variables:
// compile-command: "make -C ../../../.. "
// tags-file-name: "../../../../TAGS"
// c-basic-offset: 4
// End:
//
