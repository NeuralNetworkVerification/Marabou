/*********************                                                        */
/*! \file EngineState.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **/

#ifndef __EngineState_h__
#define __EngineState_h__

#include "List.h"
#include "PiecewiseLinearConstraint.h"
#include "TableauState.h"

class EngineState
{
public:
    EngineState();
    ~EngineState();

    TableauState _tableauState;
    Map<PiecewiseLinearConstraint *, PiecewiseLinearConstraintState *> _plConstraintToState;

    unsigned _numPlConstraintsDisbaledByValidSplits;

    unsigned _nextAuxVariable;
};

#endif // __EngineState_h__

//
// Local Variables:
// compile-command: "make -C .. "
// tags-file-name: "../TAGS"
// c-basic-offset: 4
// End:
//
