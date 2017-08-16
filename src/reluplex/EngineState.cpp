/*********************                                                        */
/*! \file EngineState.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **/

#include "EngineState.h"

EngineState::EngineState()
{
}

EngineState::~EngineState()
{
    for ( auto &kv : _plConstraintToState )
    {
        PiecewiseLinearConstraint *state = kv.second;
        if ( state )
        {
            delete state;
            kv.second = NULL;
        }
    }
}

//
// Local Variables:
// compile-command: "make -C .. "
// tags-file-name: "../TAGS"
// c-basic-offset: 4
// End:
//
