/*********************                                                        */
/*! \file SnCDivideStrategy.h
** \verbatim
** Top contributors (to current version):
**   Haoze Wu
** This file is part of the Marabou project.
** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
** in the top-level source directory) and their institutional affiliations.
** All rights reserved. See the file COPYING in the top-level source
** directory for licensing information.\endverbatim
**
** [[ Add lengthier description here ]]

**/

#ifndef __SnCDivideStrategy_h__
#define __SnCDivideStrategy_h__

enum class SnCDivideStrategy
{
    // Input splitting
    LargestInterval = 0,

    // Relu splitting
    Polarity,      // Pick the ReLU with the polarity closest to 0 among the first K nodes

    Auto
};

#endif // __SnCDivideStrategy_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
