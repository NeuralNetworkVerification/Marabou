//
// Created by shirana on 27/12/2019.
//

#ifndef MARABOU_ACAS_ABS_CONSTRAINTS_H
#define MARABOU_ACAS_ABS_CONSTRAINTS_H

#define b 0
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
    void run(double bound)
    {

//        int outputStream = redirectOutputToFile( "logs/acas_abs_constraints.txt" );

        InputQuery inputQuery;

        AcasParser acasParser( "./acas_nnet/ACASXU_run2a_1_1_batch_2000.nnet" );
        acasParser.generateQuery( inputQuery );

        unsigned num_variables = inputQuery.getNumberOfVariables();

        // 5 vars for x'_i = x_i - b
        // 5 vars for x''_i = abs( x'_i )
        // 1 var for sum( x''i ) , 0 <= i <= 5
        inputQuery.setNumberOfVariables( num_variables + 11 );

        // x_i - x'_i = b -> x_i - b = x'_i
        for ( unsigned i = 0; i < 5; ++i )
        {
            Equation equation;
            unsigned variable = acasParser.getInputVariable( i );
            //x_i - input to acas
            equation.addAddend( 1, variable );
            //x'_i - new var
            equation.addAddend( -1 , num_variables + i );
            //sclar we run throw the network at the h file
            //acas_abs_fixed_input
            equation.setScalar(b);
            inputQuery.addEquation(equation);
        }

        // abs( x_b, x_f ) = abs( x'_i, x''_i ) -> x''_i = abs( x'_i ) = abs( x_i - b )
        for ( unsigned i = 0; i < 5; ++i )
        {
            //abs( x'_i, x''_i )
            AbsConstraint *abs = new AbsConstraint( num_variables + i, num_variables + i + 5 );
            inputQuery.addPiecewiseLinearConstraint( abs );
        }

        // t = sum( x''_i ) , 0 <= i <= 5
        //  -> sum( abs( x'_i ) ) = sum( abs( x_i - b ) ) , 0 <= i <= 5
        Equation equation;
        equation.addAddend( -1, num_variables + 10  );
        for ( unsigned i = 0; i < 5; ++i )
        {
            equation.addAddend( 1, num_variables + i + 5  );
        }
        equation.setScalar(0);
        inputQuery.addEquation(equation);

        // t <= BOUND
        inputQuery.setUpperBound( num_variables + 10, bound);

        //add equation x_MIN_VAR >= x_NOT_MIN_VAR
        unsigned min_var = acasParser.getOutputVariable( MIN_VAR );
        Equation equation_out;
        equation_out.setType( Equation::LE );
        unsigned variable = acasParser.getOutputVariable( NOT_MIN_VAR );
        equation_out.addAddend( 1, variable );
        equation_out.addAddend( -1 , min_var);
        equation_out.setScalar(0);
        inputQuery.addEquation( equation_out );

        struct timespec start = TimeUtils::sampleMicro();

        Engine engine;

        if ( !engine.processInputQuery( inputQuery ) )
        {
            struct timespec end = TimeUtils::sampleMicro();
//            restoreOutputStream( outputStream );
            printFailed( "acas_abs_constraints - engine error ", start, end );
            return;
        }

        bool result = engine.solve();
        struct timespec end = TimeUtils::sampleMicro();
//        restoreOutputStream( outputStream );
        if ( !result )
        {
            printf("\n");
            printPassed( "acas_1_1_no_constraints - UNSAT, there is'n counter"
                         "example", start, end );
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
        //check if x_MIN_VAR >= x_NOT_MIN_VAR as we asked
        if ( !FloatUtils::gt(  min_var_out , variable_out ) )
        {

            printf("not min val\n");
            printFailed( "acas_abs_constraints - x_MIN_VAR >= x_NOT_MIN_VAR not hold", start, end );
        }

        //check if sum(output) <= BOUND
        double sum = 0.0;
        for ( unsigned i = 0; i < 5; ++i )
        {
            unsigned var = acasParser.getInputVariable( i );
            sum += FloatUtils::abs( inputs[var] - b );
            if ( !FloatUtils::gt( BOUND, sum ) )
            {
                printFailed( "acas_1_1_no_constraints - sum(output) <= BOUND not hold", start, end );
            }

        }

        printPassed( "acas_1_1_no_constraints - SAT", start, end );
    }
};


#endif //MARABOU_ACAS_ABS_NO_CONSTRAINTS_H
