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

class Mps_Lp_Infeasible_1
{
public:
    void run()
    {
        const char *filename = "./lp_infeasible_1.mps";

        // Extract an input query from the network
        InputQuery inputQuery;

        MpsParser mpsParser( filename );
        mpsParser.generateQuery( inputQuery );

        int outputStream = redirectOutputToFile( "logs/mps_lp_infeasible_1.txt" );

        struct timespec start = TimeUtils::sampleMicro();

        Engine engine;
        if ( !engine.processInputQuery( inputQuery ) )
        {
            struct timespec end = TimeUtils::sampleMicro();
            restoreOutputStream( outputStream );
            printPassed( "mps_lp_infeasible_1", start, end );
            return;
        }

        bool result = engine.solve();

        printf( "Result is: %u\n", result );

        struct timespec end = TimeUtils::sampleMicro();

        restoreOutputStream( outputStream );

        if ( result )
            printFailed( "mps_lp_infeasible_1", start, end );
        else
            printPassed( "mps_lp_infeasible_1", start, end );
    }
};

//
// Local Variables:
// compile-command: "make -C ../../.. "
// tags-file-name: "../../../TAGS"
// c-basic-offset: 4
// End:
//
