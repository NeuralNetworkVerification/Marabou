//
// Created by shirana on 27/12/2019.
//

#ifndef MARABOU_ACAS_ABS_NO_CONSTRAINTS_H
#define MARABOU_ACAS_ABS_NO_CONSTRAINTS_H

#define b 1

#include "AcasParser.h"
#include "Engine.h"
#include "FloatUtils.h"
#include "GlobalConfiguration.h"
#include "InputQuery.h"
#include "Preprocessor.h"
#include "ReluplexError.h"

class Acas_Abs_Constraints
{
public:
    void run()
    {
        int outputStream = redirectOutputToFile( "logs/acas_abs_constraints.txt" );

        InputQuery inputQuery;

        AcasParser acasParser( "./acas_nnet/ACASXU_run2a_1_1_batch_2000.nnet" );
        acasParser.generateQuery( inputQuery );

        unsigned num_variables = inputQuery.getNumberOfVariables();
        inputQuery.setNumberOfVariables( num_variables + 10 );

        for ( unsigned i = 0; i < 5; ++i )
        {
            inputQuery.setLowerBound( num_variables + i, -0.1 );
            inputQuery.setUpperBound( num_variables + i, 0.1);
            inputQuery.markInputVariable(num_variables + i, i );
        }

        for ( unsigned i = 0; i < 5; ++i )
        {
            Equation equation;
            equation.addAddend(1, num_variables + i);
            equation.addAddend(-1, num_variables + i + 5);
            equation.setScalar(b);
            inputQuery.addEquation(equation);
        }

        for ( unsigned i = 0; i < 5; ++i )
        {
            unsigned variable = acasParser.getInputVariable( i );
            AbsConstraint *abs = new AbsConstraint( num_variables + i + 5, variable );
            inputQuery.addPiecewiseLinearConstraint( abs );
        }

        struct timespec start = TimeUtils::sampleMicro();

        Engine engine;
        if ( !engine.processInputQuery( inputQuery ) )
        {
            struct timespec end = TimeUtils::sampleMicro();
            restoreOutputStream( outputStream );
            printFailed( "acas_abs_constraints", start, end );
            return;
        }

        bool result = engine.solve();

        struct timespec end = TimeUtils::sampleMicro();

        restoreOutputStream( outputStream );

        if ( !result )
        {
            printFailed( "acas_abs_constraints", start, end );
            return;
        }

        engine.extractSolution( inputQuery );

        //todo: how to check on new network

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
            printFailed( "acas_abs_constraints", start, end );
        else
            printPassed( "acas_abs_constraints", start, end );
    }
};


#endif //MARABOU_ACAS_ABS_NO_CONSTRAINTS_H
