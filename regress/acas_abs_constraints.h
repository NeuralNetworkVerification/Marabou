//
// Created by shirana on 27/12/2019.
//

#ifndef MARABOU_ACAS_ABS_CONSTRAINTS_H
#define MARABOU_ACAS_ABS_CONSTRAINTS_H

#define b 0
#define LARGE 100
#define BOUND 5
#define NOT_MIN_VAR 1
#define MIN_VAR 0

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

//            // Generate query adds bounds, we want to over ride them and use the bounds from the equations
//            inputQuery.setLowerBound( variable, -LARGE );
//            inputQuery.setUpperBound( variable, LARGE );

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
//        inputQuery.setLowerBound( num_variables + 10, 0.001 );
        inputQuery.setUpperBound( num_variables + 10, BOUND);

        unsigned min_var = acasParser.getOutputVariable( MIN_VAR );
        Equation equation_out;
        equation_out.setType( Equation::LE );
        unsigned variable = acasParser.getOutputVariable(NOT_MIN_VAR );
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
            printf("it's unsat\n");
            printFailed( "acas_abs_constraints", start, end );
            return;
        }

        engine.extractSolution( inputQuery );

        // Run through the original network to check correctness
        Vector<double> inputs;
        for ( unsigned i = 0; i < 5; ++i )
        {
            inputs.append( inputQuery.getSolutionValue( i ) );
        }

        Vector<double> outputs;
        acasParser.evaluate( inputs, outputs );

        double min_var_out = outputs[MIN_VAR];
        double variable_out = outputs[NOT_MIN_VAR];
        if ( !FloatUtils::gt(  min_var_out , variable_out ) )
        {
            printf("min var %f", min_var_out);
            printf("var %f", variable_out);
            printf("not min val\n");
            printFailed( "acas_abs_constraints", start, end );
        }


        double sum = 0.0;
        for ( unsigned i = 0; i < 5; ++i )
        {
            unsigned var = acasParser.getInputVariable( i );
            sum += FloatUtils::abs( inputs[var] - b );
            if ( !FloatUtils::gt( BOUND, sum ) )
            {
                printf("not sum\n");
                printFailed( "acas_1_1_no_constraints", start, end );
            }

        }

        printPassed( "acas_1_1_no_constraints", start, end );
    }
};


#endif //MARABOU_ACAS_ABS_NO_CONSTRAINTS_H
