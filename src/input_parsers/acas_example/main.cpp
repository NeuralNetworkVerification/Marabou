/*********************                                                        */
/*! \file main.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **/

#include <cstdio>

#include "../AcasParser.h"
#include "Engine.h"

int main()
{
    // Extract an input query from the network
    InputQuery inputQuery;

    AcasParser acasParser( "./ACASXU_run2a_1_1_batch_2000.nnet" );
    acasParser.generateQuery( inputQuery );


    // Feed the query to the engine
    Engine engine;
    engine.processInputQuery( inputQuery );
    if ( !engine.solve() )
    {
        printf( "Query is unsat\n" );
        return 0;
    }

    printf( "Query is sat! Extracting solution...\n" );
    engine.extractSolution( inputQuery );

    return 0;
}

//
// Local Variables:
// compile-command: "make -C . "
// tags-file-name: "./TAGS"
// c-basic-offset: 4
// End:
//
