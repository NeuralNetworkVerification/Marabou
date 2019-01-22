/*********************                                                        */
/*! \file lp_feasible_1.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz, Duligur Ibeling
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

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
        equation.addAddend( 1, 3 );
        equation.setScalar( 11 );
        inputQuery.addEquation( equation );

        int outputStream = redirectOutputToFile( "logs/lp_feasible_1.txt" );

        struct timespec start = TimeUtils::sampleMicro();

        Engine engine;
        if ( !engine.processInputQuery( inputQuery ) )
        {
            struct timespec end = TimeUtils::sampleMicro();
            restoreOutputStream( outputStream );
            printFailed( "lp_feasible_1", start, end );
            return;
        }

        bool result = engine.solve();

        struct timespec end = TimeUtils::sampleMicro();

        restoreOutputStream( outputStream );

        if ( !result )
        {
            printFailed( "lp_feasible_1", start, end );
            return;
        }

        engine.extractSolution( inputQuery );

        bool correctSolution = true;

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
            correctSolution = false;

        if ( ( value0 < 0 ) || ( value0 > 2 ) ||
             ( value1 < -3 ) || ( value1 > 3 ) ||
             ( value2 < 4 ) || ( value2 > 6 ) ||
             ( value3 < 0 ) )
        {
            correctSolution = false;
        }

        if ( !correctSolution )
            printFailed( "lp_feasible_1", start, end );
        else
            printPassed( "lp_feasible_1", start, end );
    }
};

#endif // __Lp_Feasible_1_h__

//
// Local Variables:
// compile-command: "make -C .. "
// tags-file-name: "../TAGS"
// c-basic-offset: 4
// End:
//
