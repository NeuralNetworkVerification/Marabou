/*********************                                                        */
/*! \file acas_1_1_no_constraints.h
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

#ifndef __Acas_1_1_No_Constraints_h__
#define __Acas_1_1_No_Constraints_h__

#include "AcasParser.h"
#include "Engine.h"
#include "FloatUtils.h"
#include "GlobalConfiguration.h"
#include "InputQuery.h"
#include "Preprocessor.h"
#include "MarabouError.h"

class Acas_1_1_No_Constraints
{
public:
    void run()
    {
        int outputStream = redirectOutputToFile( "logs/acas_1_1_no_constraints.txt" );

        InputQuery inputQuery;
        AcasParser acasParser( "./acas_nnet/ACASXU_run2a_1_1_batch_2000.nnet" );
        acasParser.generateQuery( inputQuery );

        struct timespec start = TimeUtils::sampleMicro();

        Engine engine;
        if ( !engine.processInputQuery( inputQuery ) )
        {
            struct timespec end = TimeUtils::sampleMicro();
            restoreOutputStream( outputStream );
            printFailed( "acas_1_1_no_constraints", start, end );
            return;
        }

        bool result = engine.solve();

        struct timespec end = TimeUtils::sampleMicro();

        restoreOutputStream( outputStream );

        if ( !result )
        {
            printFailed( "acas_1_1_no_constraints", start, end );
            return;
        }

        engine.extractSolution( inputQuery );

        // Run through the original network to check correctness
        Vector<double> inputs;
        for ( unsigned i = 0; i < 5; ++i )
        {
            unsigned variable = acasParser.getInputVariable( i );
            inputs.append( inputQuery.getSolutionValue( variable ) );
        }

        Vector<double> outputs;
        acasParser.evaluate( inputs, outputs );
        double maxError = 0.0;

        for ( unsigned i = 0; i < 5; ++i )
        {
            unsigned variable = acasParser.getOutputVariable( i );
            double newError = FloatUtils::abs( outputs[i] - inputQuery.getSolutionValue( variable ) );
            if ( FloatUtils::gt( newError, maxError ) )
              maxError = newError;
        }

        if ( FloatUtils::gt( maxError, 0.00001 ) )
            printFailed( "acas_1_1_no_constraints", start, end );
        else
            printPassed( "acas_1_1_no_constraints", start, end );
    }
};

#endif // __Acas_1_1_No_Constraints_h__

//
// Local Variables:
// compile-command: "make -C .. "
// tags-file-name: "../TAGS"
// c-basic-offset: 4
// End:
//
