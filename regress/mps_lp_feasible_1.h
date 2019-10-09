/*********************                                                        */
/*! \file main.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

 **/

#include "Engine.h"
#include "InputQuery.h"
#include "MpsParser.h"
#include <cstdio>

class Mps_Lp_Feasible_1
{
public:
    void run()
    {
        const char *filename = "./lp_feasible_1.mps";

        // Extract an input query from the network
        InputQuery inputQuery;

        MpsParser mpsParser( filename );
        mpsParser.generateQuery( inputQuery );

        int outputStream = redirectOutputToFile( "logs/mps_lp_feasible_1.txt" );

        struct timespec start = TimeUtils::sampleMicro();

        Engine engine;
        if ( !engine.processInputQuery( inputQuery ) )
        {
            struct timespec end = TimeUtils::sampleMicro();
            restoreOutputStream( outputStream );
            printFailed( "mps_lp_feasible_1", start, end );
            return;
        }

        bool result = engine.solve();

        struct timespec end = TimeUtils::sampleMicro();

        restoreOutputStream( outputStream );

        if ( !result )
        {
            printFailed( "mps_lp_feasible_1", start, end );
            return;
        }

        engine.extractSolution( inputQuery );

        bool correctSolution = true;

        // Sanity test
        double value = 0;

        double value0 = inputQuery.getSolutionValue( 0 );
        double value1 = inputQuery.getSolutionValue( 1 );
        double value2 = inputQuery.getSolutionValue( 2 );

        value += 1  * value0;
        value += 2  * value1;
        value += -1 * value2;

        if ( !FloatUtils::lte( value, 11 ) )
            correctSolution = false;

        if ( ( value0 < 0 ) || ( value0 > 2 ) ||
             ( value1 < -3 ) || ( value1 > 3 ) ||
             ( value2 < 4 ) || ( value2 > 6 ) )
        {
            correctSolution = false;
        }

        if ( !correctSolution )
            printFailed( "mps_lp_feasible_1", start, end );
        else
            printPassed( "mps_lp_feasible_1", start, end );
    }
};

//
// Local Variables:
// compile-command: "make -C ../../.. "
// tags-file-name: "../../../TAGS"
// c-basic-offset: 4
// End:
//
