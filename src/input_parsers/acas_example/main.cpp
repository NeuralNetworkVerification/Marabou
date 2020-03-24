/*********************                                                        */
/*! \file main.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz, Derek Huang
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

 **/

#include <cstdio>

#include "AcasParser.h"
#include "Engine.h"
#include "FloatUtils.h"
#include "GlobalConfiguration.h"
#include "InputQuery.h"
#include "Preprocessor.h"
#include "MarabouError.h"

int main()
{
    try
    {
        // Extract an input query from the network
        InputQuery inputQuery;

        AcasParser acasParser( "./ACASXU_run2a_1_1_batch_2000.nnet" );

        // Trimmed network: 5-5-5, 5 Relus
        // AcasParser acasParser( "ACASXU_run2a_1_1_tiny.nnet" );

        // Trimmed network: 5-50-5, 50 Relus
        // AcasParser acasParser( "ACASXU_run2a_1_1_tiny_2.nnet" );

        // Trimmed network: 5-50-50-5, 100 Relus
        // AcasParser acasParser( "ACASXU_run2a_1_1_tiny_3.nnet" );

        // Trimmed network: 5-50-50-50-5, 150 Relus
        // AcasParser acasParser( "ACASXU_run2a_1_1_tiny_4.nnet" );

        // Trimmed network: 5-50-50-50-50-5, 200 Relus
        // AcasParser acasParser( "ACASXU_run2a_1_1_tiny_5.nnet" );

        acasParser.generateQuery( inputQuery );

        // A simple query: all inputs are fixed to 0
        // for ( unsigned i = 0; i < 5; ++i )
        // {
        //     unsigned variable = acasParser.getInputVariable( i );
        //     inputQuery.setLowerBound( variable, 0.0 );
        //     inputQuery.setUpperBound( variable, 0.0 );
        // }

        // Simple constraint on an output variable
        // unsigned variable = acasParser.getOutputVariable( 0 );
        // inputQuery.setLowerBound( variable, 0.5 );

		// Feed the query to the engine
        Engine engine;
        bool preprocess = engine.processInputQuery( inputQuery );

        if ( !preprocess || !engine.solve() )
        {
            printf( "\n\nQuery is unsat\n" );
            return 0;
        }

        printf( "\n\nQuery is sat! Extracting solution...\n" );
        engine.extractSolution( inputQuery );

        Vector<double> inputs;
        for ( unsigned i = 0; i < 5; ++i )
        {
            unsigned variable = acasParser.getInputVariable( i );
            printf( "Input[%u] = %.15lf\n", i, inputQuery.getSolutionValue( variable ) );
            inputs.append( inputQuery.getSolutionValue( variable ) );
        }

        // for ( unsigned i = 0; i < 5; ++i )
        // {
        //     unsigned b = acasParser.getBVariable( 1, i );
        //     unsigned f = acasParser.getFVariable( 1, i );
        //     unsigned aux = acasParser.getAuxVariable( 1, i );
        //     unsigned slack = acasParser.getSlackVariable( 1, i );

        //     printf( "Node (%u, %u): b = %.15lf, f = %.15lf, aux = %.15lf, slack = %.15lf\n",
        //             1, i,
        //             inputQuery.getSolutionValue( b ),
        //             inputQuery.getSolutionValue( f ),
        //             inputQuery.getSolutionValue( aux ),
        //             inputQuery.getSolutionValue( slack ) );
        // }

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
    catch ( const MarabouError &e )
    {
        printf( "Caught a MarabouError. Code: %u. Message: %s\n", e.getCode(), e.getUserMessage() );
        return 0;
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
