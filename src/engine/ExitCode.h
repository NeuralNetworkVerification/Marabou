/*********************                                                        */
/*! \file ExitCode.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz, Duligur Ibeling
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2024 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

**/

#ifndef __ExitCode_h__
#define __ExitCode_h__

enum ExitCode {
    UNSAT = 0,
    SAT = 1,
    ERROR = 2,
    UNKNOWN = 3,
    TIMEOUT = 4,
    QUIT_REQUESTED = 5,

    NOT_DONE = 999,
};

#endif //__ExitCode_h__
