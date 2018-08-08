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
#include "FloatUtils.h"
#include "GlobalConfiguration.h"
#include "InputQuery.h"
#include "Preprocessor.h"
#include "ReluplexError.h"

#include "AbstractionManager.h"

int main()
{
    try
    {
        // Extract an input query from the network
        InputQuery inputQuery;

        // Trimmed network: 5-50-50-50-5, 150 Relus
        AcasParser acasParser( "ACASXU_run2a_1_1_tiny_4.nnet" );
        acasParser.generateQuery( inputQuery );

        // Input restrictions
        inputQuery.setUpperBound( acasParser.getInputVariable( 0 ), 0.0 );
        inputQuery.setUpperBound( acasParser.getInputVariable( 1 ), 0.0 );
        inputQuery.setLowerBound( acasParser.getInputVariable( 2 ), 0.0 );
        inputQuery.setUpperBound( acasParser.getInputVariable( 3 ), 0.0 );
        inputQuery.setUpperBound( acasParser.getInputVariable( 4 ), 0.0 );

        // Simple constraint on an output variable
        unsigned variable = acasParser.getOutputVariable( 0 );
        inputQuery.setLowerBound( variable, 0.0 );

		// Feed the query to the engine

        for ( unsigned i = 0; i < 5; ++i )
            inputQuery.markInputVariable( i );

        bool useAbstraction = true;

        if ( useAbstraction )
        {
            AbstractionManager am;

            bool result = am.run( inputQuery );

            if ( !result )
            {
                printf( "UNSAT!\n" );
                return 0;
            }
        }
        else
        {
            Engine engine;
            bool preprocess = engine.processInputQuery( inputQuery );

            if ( !preprocess || !engine.solve() )
            {
                printf( "UNSAT!\n" );
                return 0;
            }

            engine.extractSolution( inputQuery );
        }


        printf( "\n\nQuery is sat! Extracting solution...\n" );

        Vector<double> inputs;
        for ( unsigned i = 0; i < 5; ++i )
        {
            unsigned variable = acasParser.getInputVariable( i );
            printf( "Input[%u] = %.15lf\n", i, inputQuery.getSolutionValue( variable ) );
            inputs.append( inputQuery.getSolutionValue( variable ) );
        }

        // Run the inputs through the real network, to evaluate the error
        Vector<double> outputs;
        acasParser.evaluate( inputs, outputs );
        double error = 0.0;

        for ( unsigned i = 0; i < 5; ++i )
        {
            unsigned variable = acasParser.getOutputVariable( i );
            printf( "Output[%u] = %.15lf\n", i, inputQuery.getSolutionValue( variable ) );
            error += FloatUtils::abs( outputs[i] - inputQuery.getSolutionValue( variable ) );
        }

        printf( "\nTotal error: %.15lf\n", error );
    }
    catch ( const ReluplexError &e )
    {
        printf( "Caught a ReluplexError. Code: %u. Message: %s\n", e.getCode(), e.getUserMessage() );
        return 0;
    }

    return 0;
}

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
