//
// Created by shirana on 27/12/2019.
//

#ifndef MARABOU_ACAS_ABS_FIXED_INPUT_H
#define MARABOU_ACAS_ABS_FIXED_INPUT_H

#include <iostream>

#include "AcasParser.h"
#include "Engine.h"
#include "FloatUtils.h"
#include "GlobalConfiguration.h"
#include "InputQuery.h"
#include "Preprocessor.h"
#include "ReluplexError.h"

class Acas_abs_Fixed_Input
{
public:
    void run()
    {
        int outputStream = redirectOutputToFile( "logs/acas_abs_fixed_input.txt" );

        InputQuery inputQuery;
        AcasParser acasParser(  "./acas_nnet/ACASXU_run2a_1_1_batch_2000.nnet" );
        acasParser.generateQuery( inputQuery );

        // A simple query: all inputs are fixed to 0
        for ( unsigned i = 0; i < 5; ++i )
        {
            unsigned variable = acasParser.getInputVariable( i );
            inputQuery.setLowerBound( variable, 0.0 );
            inputQuery.setUpperBound( variable, 0.0 );
        }

        struct timespec start = TimeUtils::sampleMicro();

        Engine engine;
        if ( !engine.processInputQuery( inputQuery ) )
        {
            struct timespec end = TimeUtils::sampleMicro();
            restoreOutputStream( outputStream );
            printFailed( "acas_abs_fixed_input", start, end );
            return;
        }

        bool result = engine.solve();

        struct timespec end = TimeUtils::sampleMicro();

        restoreOutputStream( outputStream );

        if ( !result )
        {
            printFailed( "acas_abs_fixed_input", start, end );
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
        double min;
        int index = 0;
        for(unsigned i = 0; i < 5; i++)
        {
            std::cout << "\n The " << i << "'s output is: " << outputs[i] <<std::endl;
            if (i == 0)
            {
                min = outputs[i];
            }
            if (outputs[i] < min)
            {
                min = outputs[i];
                index = 0;
            }
        }

        std::cout << "\n The " << index << " is the min value\n" <<std::endl;

        if ( FloatUtils::gt( maxError, 0.00001 ) )
            printFailed( "acas_abs_fixed_input", start, end );
        else
            printPassed( "acas_abs_fixed_input", start, end );
    }
};

#endif //MARABOU_ACAS_ABS_FIXED_INPUT_H
