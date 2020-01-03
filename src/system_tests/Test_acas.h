/*********************                                                        */
/*! \file Test_RowBoundTightener.h
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

#include <cxxtest/TestSuite.h>

#include "AcasParser.h"
#include "Engine.h"
#include "FloatUtils.h"
#include "GlobalConfiguration.h"
#include "InputQuery.h"
#include "Preprocessor.h"
#include "MarabouError.h"

class AcasTestSuite : public CxxTest::TestSuite
{
public:

    void setUp()
    {
    }

    void tearDown()
    {
    }

    void test_acas_1_1()
    {
        InputQuery inputQuery;
        AcasParser acasParser( RESOURCES_DIR "/nnet/acasxu/ACASXU_experimental_v2a_1_1.nnet" );
        acasParser.generateQuery( inputQuery );

        Engine engine;
        TS_ASSERT_THROWS_NOTHING( engine.processInputQuery( inputQuery ) );

        TS_ASSERT_THROWS_NOTHING( engine.solve() );
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

        TS_ASSERT( FloatUtils::lt( maxError, 0.00001 ) );
    }

    void test_acas_2_2_fixed_input()
    {
        InputQuery inputQuery;
        AcasParser acasParser( RESOURCES_DIR "/nnet/acasxu/ACASXU_experimental_v2a_2_2.nnet" );
        acasParser.generateQuery( inputQuery );

        // A simple query: all inputs are fixed to 0
        for ( unsigned i = 0; i < 5; ++i )
        {
            unsigned variable = acasParser.getInputVariable( i );
            inputQuery.setLowerBound( variable, i * 0.25 );
            inputQuery.setUpperBound( variable, i * 0.25 );
        }

        Engine engine;
        bool result = engine.processInputQuery( inputQuery ) ;
        TS_ASSERT( result );

        TS_ASSERT_THROWS_NOTHING( engine.solve() );
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

        TS_ASSERT( FloatUtils::lt( maxError, 0.00001 ) );
    }

    void test_acas_1_1_fixed_input()
    {
        InputQuery inputQuery;
        AcasParser acasParser( RESOURCES_DIR "/nnet/acasxu/ACASXU_experimental_v2a_1_1.nnet" );
        acasParser.generateQuery( inputQuery );

        // A simple query: all inputs are fixed to 0
        for ( unsigned i = 0; i < 5; ++i )
        {
            unsigned variable = acasParser.getInputVariable( i );
            inputQuery.setLowerBound( variable, 0.00 );
            inputQuery.setUpperBound( variable, 0.00 );
        }

        Engine engine;
        TS_ASSERT_THROWS_NOTHING( engine.processInputQuery( inputQuery ) );

        TS_ASSERT_THROWS_NOTHING( engine.solve() );
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

        TS_ASSERT( FloatUtils::lt( maxError, 0.00001 ) );

    }
};

//
// Local Variables:
// compile-command: "make -C ../../.. "
// tags-file-name: "../../../TAGS"
// c-basic-offset: 4
// End:
//
