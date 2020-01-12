//
// Created by shirana on 27/12/2019.
//

#ifndef MARABOU_ACAS_ABS_CONSTRAINTS_H
#define MARABOU_ACAS_ABS_CONSTRAINTS_H

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
        inputQuery.setNumberOfVariables( num_variables + 16 );

        for ( unsigned i = 0; i < 5; ++i )
        {
            Equation equation;
            unsigned variable = acasParser.getInputVariable( i );
            equation.addAddend( 1, variable );
            equation.addAddend( -1 , num_variables + i );
            equation.setScalar(b);
            inputQuery.addEquation(equation);
        }

        for ( unsigned i = 0; i < 5; ++i )
        {
            AbsConstraint *abs = new AbsConstraint( num_variables + i, num_variables + i + 5 );
            inputQuery.addPiecewiseLinearConstraint( abs );
        }

        Equation equation;
        equation.addAddend( -1, num_variables + 10  );
        for ( unsigned i = 0; i < 5; ++i )
        {
            equation.addAddend( 1, num_variables + i + 5  );
        }
        equation.setScalar(0);
        inputQuery.addEquation(equation);
        inputQuery.setLowerBound( num_variables + 10, 0 );
        inputQuery.setUpperBound( num_variables + 10, 7);

        for ( unsigned i = 1; i < 5; ++i ){
            Equation equation;
            unsigned variable = acasParser.getOutputVariable( i );
            //unsigned min_var = acasParser.getOutputVariable( 0 );
            equation.addAddend( 1, variable );
            equation.addAddend( -1 , num_variables + 11 + i);
            inputQuery.setLowerBound( num_variables + 11 + i, 0 );
            equation.setScalar(-0.0228164);
            inputQuery.addEquation(equation);
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

        // Run through the original network to check correctness
        Vector<double> inputs;
        for ( unsigned i = 0; i < 5; ++i )
        {
            inputs.append( inputQuery.getSolutionValue( num_variables + i ) );
        }

        Vector<double> outputs;
        acasParser.evaluate( inputs, outputs );
        double maxError = 0.0;

        for ( unsigned i = 0; i < 5; ++i )
        {
//            unsigned variable = inputQuery.outputVariableByIndex(i);
            double newError = FloatUtils::abs( outputs[i]  - (-0.0228164) );
            if ( !FloatUtils::gt( newError, maxError ) )
                maxError = newError;
        }

        if ( FloatUtils::gt( maxError, 0.00001 ) )
            printFailed( "acas_abs_constraints", start, end );
        else
            printPassed( "acas_abs_constraints", start, end );
    }
};


#endif //MARABOU_ACAS_ABS_NO_CONSTRAINTS_H
