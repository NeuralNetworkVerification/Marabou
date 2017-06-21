/*********************                                                        */
/*! \file Lp_Feasible_2.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **/

#ifndef __Lp_Feasible_2_h__
#define __Lp_Feasible_2_h__

#include "Engine.h"
#include "InputQuery.h"

class Lp_Feasible_2
{
public:
    void run()
    {
        //   0  <= x0 <= 2
        //   -3 <= x1 <= 3
        //   4  <= x2 <= 6
        //
        //  x0 + 2x1 -x2 <= 11 --> x0 + 2x1 - x2 + x3 = 11, x3 >= 0
        //  -3x0 + 3x1  >= -5 --> -3x0 + 3x1 + x4 = -5, x4 <= 0

        printf( "Running lp_feasible_2... " );
        // Simple satisfiable query:
        //   0  <= x0 <= 2
        //   -3 <= x1 <= 3
        //   4  <= x2 <= 6
        //
        //  x0   + 2x1 -x2 <= 11 --> x0   + 2x1 - x2 + x3      = 11, x3 >= 0
        //  -3x0 + 3x1     >= -5 --> -3x0 + 3x1           + x4 = -5, x4 <= 0

        InputQuery inputQuery;
        inputQuery.setNumberOfVariables( 5 );

        inputQuery.setLowerBound( 0, 0 );
        inputQuery.setUpperBound( 0, 2 );

        inputQuery.setLowerBound( 1, -3 );
        inputQuery.setUpperBound( 1, 3 );

        inputQuery.setLowerBound( 2, 4 );
        inputQuery.setUpperBound( 2, 6 );

        inputQuery.setLowerBound( 3, 0 );
        inputQuery.setUpperBound( 4, 0 );

        InputQuery::Equation equation1;
        equation1.addAddend( 1, 0 );
        equation1.addAddend( 2, 1 );
        equation1.addAddend( -1, 2 );
        equation1.addAddend( 1, 3 );
        equation1.setScalar( 11 );
        equation1.markAuxiliaryVariable( 3 );
        inputQuery.addEquation( equation1 );

        InputQuery::Equation equation2;
        equation2.addAddend( -3, 0 );
        equation2.addAddend( 3, 1 );
        equation2.addAddend( 1, 4 );
        equation2.setScalar( -5 );
        equation2.markAuxiliaryVariable( 4 );
        inputQuery.addEquation( equation2 );

        Engine engine;

        engine.processInputQuery( inputQuery );

        if ( !engine.solve() )
        {
            printf( "\nError! Query is feasible but no solution found\n" );
            exit( 1 );
        }

        engine.extractSolution( inputQuery );

        // Sanity test

        double value0 = inputQuery.getSolutionValue( 0 );
        double value1 = inputQuery.getSolutionValue( 1 );
        double value2 = inputQuery.getSolutionValue( 2 );
        double value3 = inputQuery.getSolutionValue( 3 );
        double value4 = inputQuery.getSolutionValue( 4 );

        double value = 0;
        value += 1  * value0;
        value += 2  * value1;
        value += -1 * value2;
        value += 1  * value3;
        if ( value != 11 )
        {
            printf( "\nError! The solution does not satisfy the first equation\n" );
            exit( 1 );
        }

        value = 0;
        value += -3  * value0;
        value += 3  * value1;
        value += 1  * value4;
        if ( value != -5 )
        {
            printf( "\nError! The solution does not satisfy the second equation\n" );
            exit( 1 );
        }

        if ( ( value0 < 0 ) || ( value0 > 2 ) ||
             ( value1 < -3 ) || ( value1 > 3 ) ||
             ( value2 < 4 ) || ( value2 > 6 ) ||
             ( value3 < 0 ) ||
             ( value4 > 0 ) )
        {
            printf( "\nError! Values violate the variable bounds\n" );
            exit( 1 );
        }

        printf( "Success!\n" );
    }
};

#endif // __Lp_Feasible_2_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
