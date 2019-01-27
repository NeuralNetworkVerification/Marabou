/*********************                                                        */
/*! \file main.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

 **/

#include <cstdio>

#include "BerkeleyParser.h"
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
        BerkeleyParser berkeleyParser( "./madry_network/model" );
        berkeleyParser.generateQuery( inputQuery );

        // Run one of the examples through the network
        File fixedInputFile( "./madry_network/real_0_adv_0.in" );
        fixedInputFile.open( File::MODE_READ );
        String line = fixedInputFile.readLine();
        List<String> tokens = line.tokenize( " " );
        if ( tokens.size() != 784 )
        {
            printf( "Error! Input size != 784 (= %u)\n", tokens.size() );
            exit( 1 );
        }

        unsigned count = 0;
        for ( const auto &token : tokens )
        {
            double bound = atof( token.ascii() );
            inputQuery.setLowerBound( count, bound );
            inputQuery.setUpperBound( count, bound );
            ++count;
        }

        // Feed the query to the engine
        Engine engine;
        if ( !engine.processInputQuery( inputQuery ) )
        {
            printf( "Error! Preprocessing failed\n" );
            exit( 1 );
        }

        if ( !engine.solve() )
        {
            printf( "\n\nQuery is unsat\n" );
            return 0;
        }

        printf( "\n\nQuery is sat! Extracting solution...\n\n" );
        engine.extractSolution( inputQuery );

        Vector<double> inputs;
        for ( unsigned i = 0; i < 784; ++i )
        {
            inputs.append( inputQuery.getSolutionValue( i ) );
            printf( "Input[%u] = %.15lf\n", i, inputQuery.getSolutionValue( i ) );
        }

        Set<unsigned> outputVariables = berkeleyParser.getOutputVariables();
        count = 0;
        for ( unsigned outputVariable : outputVariables )
        {
            printf( "Output[%u] = %.15lf\n", count, inputQuery.getSolutionValue( outputVariable ) );
            ++count;
        }
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
