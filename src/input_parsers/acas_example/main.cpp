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

#include "AcasParser.h"
#include "Engine.h"
#include "InputQuery.h"

int main()
{
    // Extract an input query from the network
    InputQuery inputQuery;

    AcasParser acasParser( "./ACASXU_run2a_1_1_batch_2000.nnet" );
    acasParser.generateQuery( inputQuery );

    // A simple query: all inputs are fixed to 0
    for ( unsigned i = 0; i < 5; ++i )
    {
        unsigned variable = acasParser.getInputVariable( i );
        inputQuery.setLowerBound( variable, 0.0 );
        inputQuery.setUpperBound( variable, 0.0 );
    }

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

    for ( unsigned i = 0; i < 5; ++i )
    {
        unsigned variable = acasParser.getInputVariable( i );
        printf( "Input[%u] = %.15lf\n", i, inputQuery.getSolutionValue( variable ) );
    }

    for ( unsigned i = 1; i < 7; ++i )
    {
        for ( unsigned j = 0; j < 50; ++j )
        {
            unsigned b = acasParser.getBVariable( i, j );
            unsigned f = acasParser.getFVariable( i, j );
            unsigned aux = acasParser.getAuxVariable( i, j );

            printf( "Node (%u, %u): b = %.15lf, f = %.15lf, aux = %.15lf\n",
                    i, j,
                    inputQuery.getSolutionValue( b ),
                    inputQuery.getSolutionValue( f ),
                    inputQuery.getSolutionValue( aux ) );
        }
    }

    for ( unsigned i = 0; i < 5; ++i )
    {
        unsigned variable = acasParser.getOutputVariable( i );
        printf( "Output[%u] = %.15lf\n", i, inputQuery.getSolutionValue( variable ) );
    }

    return 0;
}

//
// Local Variables:
// compile-command: "make -C . "
// tags-file-name: "./TAGS"
// c-basic-offset: 4
// End:
//
