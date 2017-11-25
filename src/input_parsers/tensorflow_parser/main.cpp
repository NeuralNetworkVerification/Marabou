/*********************                                                        */
/*! \file main.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Kyle Julian
 ** This file is part of the Marabou project.
 ** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **/

#include <cstdio>

#include "TfParser.h"
#include "Engine.h"
#include "FloatUtils.h"
#include "GlobalConfiguration.h"
#include "InputQuery.h"
#include "Preprocessor.h"
#include "ReluplexError.h"

int main()
{
    try
    {
        // Extract an input query from the network
        InputQuery inputQuery;

        TfParser tfParser("./models/graph_test.pb");

        tfParser.generateQuery( inputQuery );

        // A simple query: all inputs are fixed to 0
        inputQuery.setLowerBound( tfParser.getInputVariable(0), 0.0 );
        inputQuery.setUpperBound( tfParser.getInputVariable(0), 0.0 );
        inputQuery.setLowerBound( tfParser.getInputVariable(1), 1.0 );
        inputQuery.setUpperBound( tfParser.getInputVariable(1), 1.0 );


        //inputQuery.setLowerBound( tfParser.getOutputVariable(0), 0.0 );
        inputQuery.setUpperBound( tfParser.getOutputVariable(0), 8.0 );
        //inputQuery.setLowerBound( tfParser.getOutputVariable(1), 1.0 );
        //inputQuery.setUpperBound( tfParser.getOutputVariable(1), 1.0 );


        // Feed the query to the engine
        Engine engine;

        bool sat = engine.processInputQuery( inputQuery ) && engine.solve();
        if ( !sat )
        {
            printf( "\n\nQuery is unsat\n" );
            return 0;
        }

        printf( "\n\nQuery is sat! Extracting solution...\n" );
        engine.extractSolution( inputQuery );

        Vector<double> inputs;
        for ( unsigned i = 0; i < (unsigned) tfParser.num_inputs(); ++i )
        {
            unsigned variable = tfParser.getInputVariable( i );
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
        tfParser.evaluate( inputs, outputs );
        double error = 0.0;

        for ( unsigned i = 0; i < (unsigned) tfParser.num_outputs(); ++i )
        {
            unsigned variable = tfParser.getOutputVariable( i );
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
// compile-command: "make -C ../../.. "
// tags-file-name: "../../../TAGS"
// c-basic-offset: 4
// End:
//
