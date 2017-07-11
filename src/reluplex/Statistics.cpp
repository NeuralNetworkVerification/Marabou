/*********************                                                        */
/*! \file Statistics.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **/

#include "Statistics.h"
#include "Time.h"

void Statistics::print()
{
    printf( "\n%s Statistics update:\n", Time::now().ascii() );

    printf( "\tNumber of main loop iterators: %llu (%llu simplex steps, %llu constraint-fixing steps)\n"
            , _numMainLoopIterations
            , _numSimplexSteps
            , _numConstraintFixingSteps );
}

void Statistics::incNumMainLoopIterations()
{
    ++_numMainLoopIterations;
}

void Statistics::incNumSimplexSteps()
{
    ++_numSimplexSteps;
}

void Statistics::incNumConstraintFixingSteps()
{
    ++_numConstraintFixingSteps;
}

//
// Local Variables:
// compile-command: "make -C .. "
// tags-file-name: "../TAGS"
// c-basic-offset: 4
// End:
//
