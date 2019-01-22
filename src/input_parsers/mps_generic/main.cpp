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
        printf("Invalid invocation. Please supply an MPS file to read as follows:\n\
                \t$ mps_gensol.elf <FILENAME >\nNow exiting program.\n");
        return 0;
    }

    char *filename = argv[1];

    // Extract an input query from the network
    InputQuery inputQuery;

    MpsParser mpsParser( filename );
    mpsParser.generateQuery( inputQuery );

    Engine engine;
    engine.processInputQuery( inputQuery );
    if ( !engine.solve() )
    {
        printf( "\nStatus:\tUNSAT\n" );
        return 0;
    }

    printf( "Status:\tSAT\n\n" );
    engine.extractSolution( inputQuery );

    // print rows
    unsigned numEqns = mpsParser.getNumEqns();
    printf("ROWS:\n");
    printf("\tNAME\tVAL\t\tLB\t\tUB\n");
    for ( unsigned i = 0; i < numEqns; ++i )
    {
        // Print row i
        printf("\t%s\t%s\t%s\t%s\n", mpsParser.getEqnName( i ).ascii(),
               mpsParser.getEqnValue( i, inputQuery ).ascii(),
               mpsParser.getEqnLB( i ).ascii(),
               mpsParser.getEqnUB( i ).ascii());
    }

    unsigned numVars = mpsParser.getNumVars();
    // print columns
    printf("COLUMNS:\n");
    printf("\tNAME\tVAL\t\tLB\t\tUB\n");
    for ( unsigned i = 0; i < numVars; ++i )
    {
        printf("\t%s\t%f\t%s\t%s\n", mpsParser.getVarName( i ).ascii(),
               inputQuery.getSolutionValue( i ),
               mpsParser.getLowerBound( i ).ascii(),
               mpsParser.getUpperBound( i ).ascii());
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
