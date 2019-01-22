/*********************                                                        */
/*! \file lp_infeasible_1.h
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

#ifndef __Lp_Infeasible_1_h__
#define __Lp_Infeasible_1_h__

#include "Engine.h"
#include "InputQuery.h"

class Lp_Infeasible_1
{
public:
    void run()
    {
        // Simple infeasible query:
        //   0   <= x0  <= 1
        //   0   <= x1  <= 1
        //   -1  <= x2  <= 0
        //   0.5 <= x3  <= 1

        //  x0 = x1         --> x0 - x1 + x4 = 0
        //  x0 = -x2        --> x0 + x2 + x5 = 0
        //  x3 = x1 + x2    --> - x1 - x2 + x3 + x6 = 0

        InputQuery inputQuery;
        inputQuery.setNumberOfVariables( 7 );

        inputQuery.setLowerBound( 0, 0 );
        inputQuery.setUpperBound( 0, 1 );

        inputQuery.setLowerBound( 1, 0 );
        inputQuery.setUpperBound( 1, 1 );

        inputQuery.setLowerBound( 2, -1 );
        inputQuery.setUpperBound( 2, 0 );

        inputQuery.setLowerBound( 3, 0.5 );
        inputQuery.setUpperBound( 3, 1 );

        inputQuery.setLowerBound( 4, 0 );
        inputQuery.setUpperBound( 4, 0 );

        inputQuery.setLowerBound( 5, 0 );
        inputQuery.setUpperBound( 5, 0 );

        inputQuery.setLowerBound( 6, 0 );
        inputQuery.setUpperBound( 6, 0 );

        Equation equation1;
        equation1.addAddend( 1, 0 );
        equation1.addAddend( -1, 1 );
        equation1.addAddend( 1, 4 );
        equation1.setScalar( 0 );
        inputQuery.addEquation( equation1 );

        Equation equation2;
        equation2.addAddend( 1, 0 );
        equation2.addAddend( 1, 2 );
        equation2.addAddend( 1, 5 );
        equation2.setScalar( 0 );
        inputQuery.addEquation( equation2 );

        Equation equation3;
        equation3.addAddend( -1, 1 );
        equation3.addAddend( -1, 2 );
        equation3.addAddend( 1, 3 );
        equation3.addAddend( 1, 6 );
        equation3.setScalar( 0 );
        inputQuery.addEquation( equation3 );

        int outputStream = redirectOutputToFile( "logs/lp_infeasible_1.txt" );

        struct timespec start = TimeUtils::sampleMicro();

        Engine engine;
        if ( !engine.processInputQuery( inputQuery ) )
        {
            restoreOutputStream( outputStream );
            struct timespec end = TimeUtils::sampleMicro();
            printPassed( "lp_infeasible_1", start, end );
            return;
        }

        bool result = engine.solve();

        struct timespec end = TimeUtils::sampleMicro();

        restoreOutputStream( outputStream );

        if ( result )
            printFailed( "lp_infeasible_1", start, end );
        else
            printPassed( "lp_infeasible_1", start, end );
    }
};

#endif // __Lp_Infeasible_1_h__

//
// Local Variables:
// compile-command: "make -C .. "
// tags-file-name: "../TAGS"
// c-basic-offset: 4
// End:
//
