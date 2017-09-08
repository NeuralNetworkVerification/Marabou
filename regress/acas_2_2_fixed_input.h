/*********************                                                        */
/*! \file acas_2_2_fixed_input.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **/

#ifndef __Acas_2_2_Fixed_Input_h__
#define __Acas_2_2_Fixed_Input_h__

#include "AcasParser.h"
#include "Engine.h"
#include "FloatUtils.h"
#include "GlobalConfiguration.h"
#include "InputQuery.h"
#include "Preprocessor.h"
#include "ReluplexError.h"

class Acas_2_2_Fixed_Input
{
public:
    void run()
    {
        int outputStream = redirectOutputToFile( "logs/lp_feasible_2.txt" );

        InputQuery inputQuery;
        AcasParser acasParser( "./acas_nnet/ACASXU_run2a_2_2_batch_2000.nnet" );
        acasParser.generateQuery( inputQuery );

        // A simple query: all inputs are fixed to 0
        for ( unsigned i = 0; i < 5; ++i )
        {
            unsigned variable = acasParser.getInputVariable( i );
            inputQuery.setLowerBound( variable, i * 0.25 );
            inputQuery.setUpperBound( variable, i * 0.25 );
        }

        timeval start = TimeUtils::sampleMicro();

        Engine engine;
        engine.processInputQuery( inputQuery );
        bool result = engine.solve();

        timeval end = TimeUtils::sampleMicro();

        restoreOutputStream( outputStream );

        if ( !result )
        {
            printFailed( "acas_2_2_fixed_input", start, end );
            return;
        }

        engine.extractSolution( inputQuery );

        // Run through the original network to check correctness
        Vector<double> inputs;
        for ( unsigned i = 0; i < 5; ++i )
        {
            unsigned variable = acasParser.getInputVariable( i );
            inputs.append( inputQuery.getSolutionValue( variable ) );
        }

        Vector<double> outputs;
        acasParser.evaluate( inputs, outputs );
        double maxError = 0.0;

        for ( unsigned i = 0; i < 5; ++i )
        {
            unsigned variable = acasParser.getOutputVariable( i );
            double newError = FloatUtils::abs( outputs[i] - inputQuery.getSolutionValue( variable ) );
            if ( FloatUtils::gt( newError, maxError ) )
              maxError = newError;
        }

        if ( FloatUtils::gt( maxError, 0.00001 ) )
            printFailed( "acas_2_2_fixed_input", start, end );
        else
            printPassed( "acas_2_2_fixed_input", start, end );
    }
};

#endif // __Acas_2_2_Fixed_Input_h__

//
// Local Variables:
// compile-command: "make -C .. "
// tags-file-name: "../TAGS"
// c-basic-offset: 4
// End:
//
