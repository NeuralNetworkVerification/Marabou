/*********************                                                        */
/*! \file SearchTreeStackEntry.h
** \verbatim
** Top contributors (to current version):
**   Guy Katz, Haoze Wu
** This file is part of the Marabou project.
** Copyright (c) 2017-2024 by the authors listed in the file AUTHORS
** in the top-level source directory) and their institutional affiliations.
** All rights reserved. See the file COPYING in the top-level source
** directory for licensing information.\endverbatim
**
** [[ Add lengthier description here ]]

**/

#ifndef __SearchTreeStackEntry_h__
#define __SearchTreeStackEntry_h__

#include "EngineState.h"
#include "PiecewiseLinearCaseSplit.h"

/*
  A stack entry consists of the engine state before the split,
  the active split, the alternative splits (in case of backtrack),
  and also any implied splits that were discovered subsequently.
*/
struct SearchTreeStackEntry
{
public:
    PiecewiseLinearCaseSplit _activeSplit;
    List<PiecewiseLinearCaseSplit> _impliedValidSplits;
    List<PiecewiseLinearCaseSplit> _alternativeSplits;
    EngineState *_engineState;

    /*
      Create a copy of the SearchTreeStackEntry on the stack and returns a pointer to
      the copy.
      We do not copy the engineState for now, since where this method is called,
      we recreate the engineState by replaying the caseSplits.
    */
    SearchTreeStackEntry *duplicateSearchTreeStackEntry()
    {
        SearchTreeStackEntry *copy = new SearchTreeStackEntry();

        copy->_activeSplit = _activeSplit;
        copy->_impliedValidSplits = _impliedValidSplits;
        copy->_alternativeSplits = _alternativeSplits;
        copy->_engineState = NULL;

        return copy;
    }
};

#endif // __SearchTreeStackEntry_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
