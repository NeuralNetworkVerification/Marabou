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
#include "ReluplexError.h"

int main()
{
    try
    {
        // Extract an input query from the network
        InputQuery inputQuery;

        // AcasParser acasParser( "./ACASXU_run2a_1_1_batch_2000.nnet" );
        AcasParser acasParser( "ACASXU_run2a_1_1_tiny.nnet" );
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

        for ( unsigned i = 0; i < 5; ++i )
        {
            unsigned b = acasParser.getBVariable( 1, i );
            unsigned f = acasParser.getFVariable( 1, i );
            unsigned aux = acasParser.getAuxVariable( 1, i );

            printf( "Node (%u, %u): b = %.15lf, f = %.15lf, aux = %.15lf\n",
                    1, i,
                    inputQuery.getSolutionValue( b ),
                    inputQuery.getSolutionValue( f ),
                    inputQuery.getSolutionValue( aux ) );
        }

        for ( unsigned i = 0; i < 5; ++i )
        {
            unsigned variable = acasParser.getOutputVariable( i );
            printf( "Output[%u] = %.15lf\n", i, inputQuery.getSolutionValue( variable ) );
        }
    }
    catch ( const ReluplexError &e )
    {
        printf( "Caught a ReluplexError. Code: %u\n", e.getCode() );
        return 0;
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
