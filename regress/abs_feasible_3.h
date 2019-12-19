//
// Created by shirana on 19/12/2019.
//

#ifndef MARABOU_ABS_FEASIBLE_3_H
#define MARABOU_ABS_FEASIBLE_3_H


#include "Engine.h"
#include "FloatUtils.h"
#include "InputQuery.h"
#include "AbsConstraint.h"

class Abs_Feasible_3
{
public:
    void run()
    {
        // The example from the CAV'17 paper, without the aux variables:
        //   0   <= x0  <= 1
        //   0   <= x1f
        //   0   <= x2f
        //   1/2 <= x3  <= 1
        //
        //  x0 - x1b = 0
        //  x0 + x2b = 0
        //  x1f + x2f - x3 = 0
        //
        //  x2f = Abs(x2b)
        //  x3f = Abs(x3b)
        //
        //   x0: x0
        //   x1: x1b
        //   x2: x1f
        //   x3: x2b
        //   x4: x2f
        //   x5: x3

        InputQuery inputQuery;
        inputQuery.setNumberOfVariables( 6 );

        inputQuery.setLowerBound( 0, -5 );
        inputQuery.setUpperBound( 0, -3 );

        inputQuery.setLowerBound( 5, 9 );
        inputQuery.setUpperBound( 5, 10 );

        Equation equation1;
        equation1.addAddend( 1, 0 );
        equation1.addAddend( -1, 1 );
        equation1.setScalar( 0 );
        inputQuery.addEquation( equation1 );

        Equation equation2;
        equation2.addAddend( 1, 0 );
        equation2.addAddend( 1, 3 );
        equation2.setScalar( 0 );
        inputQuery.addEquation( equation2 );

        Equation equation3;
        equation3.addAddend( 1, 2 );
        equation3.addAddend( 1, 4 );
        equation3.addAddend( -1, 5 );
        equation3.setScalar( 0 );
        inputQuery.addEquation( equation3 );

        AbsConstraint *abs1 = new AbsConstraint( 1, 2 );
        AbsConstraint *abs2 = new AbsConstraint( 3, 4 );

        inputQuery.addPiecewiseLinearConstraint( abs1 );
        inputQuery.addPiecewiseLinearConstraint( abs2 );

//        int outputStream = redirectOutputToFile( "logs/abs_feasible_3.txt" );

        struct timespec start = TimeUtils::sampleMicro();

        Engine engine;
        if ( !engine.processInputQuery( inputQuery ) )
        {
            struct timespec end = TimeUtils::sampleMicro();
//            restoreOutputStream( outputStream );
            printFailed( "abs_feasible_3", start, end );
            return;
        }

        bool result = engine.solve();

        struct timespec end = TimeUtils::sampleMicro();

//        restoreOutputStream( outputStream );

        if ( !result )
        {
            printFailed( "abs_feasible_3", start, end );
            return;
        }

        engine.extractSolution( inputQuery );

        bool correctSolution = true;
        // Sanity test

        double value_x0 = inputQuery.getSolutionValue( 0 );
        double value_x1b = inputQuery.getSolutionValue( 1 );
        double value_x1f = inputQuery.getSolutionValue( 2 );
        double value_x2b = inputQuery.getSolutionValue( 3 );
        double value_x2f = inputQuery.getSolutionValue( 4 );
        double value_x3 = inputQuery.getSolutionValue( 5 );

        printf(" value_x0:%f\n", value_x0);
        printf(" value_x3:%f\n", value_x3);
        printf("value_x1b: %f, value_x1f: %f \n" , value_x1b, value_x1f );
        printf("value_x2b: %f, value_x2f: %f \n" , value_x2b, value_x2f );

        if ( !FloatUtils::areEqual( value_x0, value_x1b ) )
            correctSolution = false;



        if ( !FloatUtils::areEqual( value_x0, -value_x2b ) )
        {
            correctSolution = false;
        }


        if ( !FloatUtils::areEqual( value_x3, value_x1f + value_x2f ) )
        {
            correctSolution = false;
        }

        if ( FloatUtils::lt( value_x0, -5 ) || FloatUtils::gt( value_x0, -3 ) ||
             FloatUtils::lt( value_x1f, 0 ) || FloatUtils::lt( value_x2f, 0 ) ||
             FloatUtils::lt( value_x3, 9 ) || FloatUtils::gt( value_x3, 10 ) )
        {
            correctSolution = false;
        }

        if ( !FloatUtils::areEqual(FloatUtils::abs(value_x1b), value_x1f ) )
        {
            correctSolution = false;
        }

        if ( !FloatUtils::areEqual(FloatUtils::abs(value_x2b), value_x2f ) )
        {
            correctSolution = false;
        }

        if ( !correctSolution )
            printFailed( "abs_feasible_3", start, end );
        else
            printPassed( "abs_feasible_3", start, end );
    }
};

#endif //MARABOU_ABS_FEASIBLE_3_H
