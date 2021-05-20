/*********************                                                        */
/*! \file EngineState.h
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

#ifndef __EngineState_h__
#define __EngineState_h__

#include "List.h"
#include "Map.h"
#include "PiecewiseLinearConstraint.h"
#include "TableauState.h"

class EngineState
{
public:
    EngineState();
    ~EngineState();

    /*
      The state of the tableau
    */
    bool _tableauStateIsStored;
    TableauState _tableauState;

    /*
      The state of each of the PL constraints
    */
    Map<PiecewiseLinearConstraint *, PiecewiseLinearConstraint *> _plConstraintToState;
    unsigned _numPlConstraintsDisabledByValidSplits;

    std::vector<double> _groundUpperBounds;
	std::vector<double> _groundLowerBounds;

    /*
      A unique ID allocated to every state that is stored, for
      debugging purposes. These are assigned by the SMT core.
    */
    unsigned _stateId;
};

#endif // __EngineState_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
