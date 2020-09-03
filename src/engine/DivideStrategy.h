/*********************                                                        */
/*! \file DivideStrategy.h
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

#ifndef __DivideStrategy_h__
#define __DivideStrategy_h__

enum class DivideStrategy
{
    // Relu splitting
    Polarity = 0,      // Pick the ReLU with the polarity closest to 0 among the first K nodes
    EarliestReLU,  // Pick a ReLU that appears in the earliest layer
    ReLUViolation, // Pick the ReLU that has been violated for the most times
    LargestInterval, // Pick the largest interval
    Auto,
};

#endif // __DivideStrategy_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
