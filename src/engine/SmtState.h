/*********************                                                        */
/*! \file SmtState.h
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

#ifndef __SmtState_h__
#define __SmtState_h__

#include "List.h"
#include "Map.h"
#include "PiecewiseLinearConstraint.h"
#include "StackEntry.h"

class SmtState
{
public:
    /*
      Valid splits that were implied by level 0 of the stack.
    */
    List<PiecewiseLinearCaseSplit> _impliedValidSplitsAtRoot;

    /*
      The stack.
    */
    List< StackEntry *> _stack;

};

#endif // __SmtState_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
