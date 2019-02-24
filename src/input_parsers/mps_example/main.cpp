/*********************                                                        */
/*! \file main.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Rachel Lim, Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

 **/

// Evoke this file by calling ./mps_gensol.elf <MPS FILENAME>

#include "Engine.h"
#include "InputQuery.h"
#include "MpsParser.h"
#include <cstdio>

int main( int argc, char *argv[] )
{
    if ( argc != 2 )
    {
        printf( "Error: please provide an mps file\n");
        return 0;
    }

    char *filename = argv[1];

    // Extract an input query from the network
    InputQuery inputQuery;

    MpsParser mpsParser( filename );
    mpsParser.generateQuery( inputQuery );

    Engine engine;
    bool preprocess = engine.processInputQuery( inputQuery );

    if ( !preprocess || !engine.solve() )
    {
        printf( "\n\nQuery is unsat\n" );
        return 0;
    }

    printf( "Status:\tSAT\n\n" );
    engine.extractSolution( inputQuery );

    printf( "Printing feasible solution:\n" );
    for ( unsigned i = 0; i < mpsParser.getNumVars(); ++i )
    {
        printf( "\t%s: %.2lf\n",
                mpsParser.getVarName( i ).ascii(),
                inputQuery.getSolutionValue( i ) );
    }

    return 0;
}

//
// Local Variables:
// compile-command: "make -C ../../.. "
// tags-file-name: "../../../TAGS"
// c-basic-offset: 4
// End:
//
