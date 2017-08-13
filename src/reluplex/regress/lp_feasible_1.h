/*********************                                                        */
/*! \file Lp_Feasible_1.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **/

#ifndef __Lp_Feasible_1_h__
#define __Lp_Feasible_1_h__

#include "Engine.h"
#include "FloatUtils.h"
#include "InputQuery.h"

class Lp_Feasible_1
{
public:
    void run()
    {
        printf( "Running lp_feasible_1... " );
        // Simple satisfiable query:
        //   0  <= x0 <= 2
        //   -3 <= x1 <= 3
        //   4  <= x2 <= 6
        //
        //  x0 + 2x1 -x2 <= 11 --> x0 + 2x1 - x2 + x3 = 11, x3 >= 0

        InputQuery inputQuery;
        inputQuery.setNumberOfVariables( 4 );

        inputQuery.setLowerBound( 0, 0 );
        inputQuery.setUpperBound( 0, 2 );

        inputQuery.setLowerBound( 1, -3 );
        inputQuery.setUpperBound( 1, 3 );

        inputQuery.setLowerBound( 2, 4 );
        inputQuery.setUpperBound( 2, 6 );

        inputQuery.setLowerBound( 3, 0 );

        Equation equation;
        equation.addAddend( 1, 0 );
        equation.addAddend( 2, 1 );
        equation.addAddend( -1, 2 );
        equation.addAuxAddend( 1 );
        equation.setScalar( 11 );
        equation.markAuxVariable( 3 );
        inputQuery.addEquation( equation );

        Engine engine;

        engine.processInputQuery( inputQuery );

        if ( !engine.solve() )
        {
            printf( "\nError! Query is feasible but no solution found\n" );
            exit( 1 );
        }

        engine.extractSolution( inputQuery );

        // Sanity test
        double value = 0;

        double value0 = inputQuery.getSolutionValue( 0 );
        double value1 = inputQuery.getSolutionValue( 1 );
        double value2 = inputQuery.getSolutionValue( 2 );
        double value3 = inputQuery.getSolutionValue( 3 );

        value += 1  * value0;
        value += 2  * value1;
        value += -1 * value2;
        value += 1  * value3;

        if ( !FloatUtils::areEqual( value, 11 ) )
        {
            printf( "\nError! The solution does not satisfy the equation\n" );
            exit( 1 );
        }

        if ( ( value0 < 0 ) || ( value0 > 2 ) ||
             ( value1 < -3 ) || ( value1 > 3 ) ||
             ( value2 < 4 ) || ( value2 > 6 ) ||
             ( value3 < 0 ) )
        {
            printf( "\nError! Values violate the variable bounds\n" );
            exit( 1 );
        }

        printf( "\nQuery is satisfiable\n" );
        printf( "\nRegression test passed!\n" );
    }
};

#endif // __Lp_Feasible_1_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
