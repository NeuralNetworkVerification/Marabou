/*********************                                                        */
/*! \file Engine.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Reluplex project.
 ** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **/

#include "Engine.h"

Engine::Engine()
{
}

Engine::~Engine()
{
}

bool Engine::solve()
{
    // Todo: If l >= u for some var, fail immediately

    while ( true )
    {
        _tableau->computeBasicStatus();

        if ( !_tableau->existsBasicOutOfBounds() )
            return true;

        _tableau->computeCostFunction();
        if ( !_tableau->pickEnteringVariable() )
            return false;

        _tableau->computeD();
        _tableau->pickLeavingVariable();
        _tableau->performPivot();
    }
}

//
// Local Variables:
// compile-command: "make -C .. "
// tags-file-name: "../TAGS"
// c-basic-offset: 4
// End:
//
