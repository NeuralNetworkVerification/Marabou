//
// Created by shirana on 27/12/2019.
//

#ifndef MARABOU_ACAS_ABS_CONSTRAINTS_H
#define MARABOU_ACAS_ABS_CONSTRAINTS_H

#define b 0
#define LARGE 100

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
//        int outputStream = redirectOutputToFile( "logs/acas_abs_constraints.txt" );

        InputQuery inputQuery;

        AcasParser acasParser( "./acas_nnet/ACASXU_run2a_1_1_batch_2000.nnet" );
        acasParser.generateQuery( inputQuery );


        unsigned num_variables = inputQuery.getNumberOfVariables();
        inputQuery.setNumberOfVariables( num_variables + 11 );

        for ( unsigned i = 0; i < 5; ++i )
        {
            Equation equation;
            unsigned variable = acasParser.getInputVariable( i );

            // Generate query adds bounds, we want to over ride them and use the bounds from the equations
            inputQuery.setLowerBound( variable, -LARGE );
            inputQuery.setUpperBound( variable, LARGE );

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
        inputQuery.setLowerBound( num_variables + 10, 0.001 );
        inputQuery.setUpperBound( num_variables + 10, 0.002);

        unsigned min_var = acasParser.getOutputVariable( 0 );
        Equation equation_out;
        equation_out.setType( Equation::GE );
        unsigned variable = acasParser.getOutputVariable( 1 );
        equation_out.addAddend( 1, variable );
        equation_out.addAddend( -1 , min_var);
        equation_out.setScalar(0);
        inputQuery.addEquation(equation_out);

        struct timespec start = TimeUtils::sampleMicro();

        Engine engine;
        if ( !engine.processInputQuery( inputQuery ) )
        {
            struct timespec end = TimeUtils::sampleMicro();
//            restoreOutputStream( outputStream );
            printFailed( "acas_abs_constraints", start, end );
            return;
        }

        bool result = engine.solve();

        struct timespec end = TimeUtils::sampleMicro();

//        restoreOutputStream( outputStream );

        if ( !result )
        {
            printFailed( "acas_abs_constraints", start, end );
            return;
        }

        engine.extractSolution( inputQuery );

//        // Run through the original network to check correctness
//        Vector<double> inputs;
//        for ( unsigned i = 0; i < 5; ++i )
//        {
//            inputs.append( inputQuery.getSolutionValue( num_variables + i ) );
//        }
//
//        Vector<double> outputs;
//        acasParser.evaluate( inputs, outputs );
//        double maxError = 0.0;

        //unsigned min_var = inputQuery.outputVariableByIndex(0);
//        for ( unsigned i = 0; i < 5; ++i )
//        {
//            unsigned variable = inputQuery.outputVariableByIndex(i);
//            double newError = FloatUtils::abs( outputs[i]  - outputs[var] );
//            if ( !FloatUtils::lt( newError, maxError ) )
//                maxError = newError;
//        }
//
//        if ( FloatUtils::gt( maxError, 0.00001 ) )
//            printFailed( "acas_abs_constraints", start, end );
//        else
//            printPassed( "acas_abs_constraints", start, end );
        printPassed( "acas_abs_constraints", start, end );
    }
};


#endif //MARABOU_ACAS_ABS_NO_CONSTRAINTS_H
