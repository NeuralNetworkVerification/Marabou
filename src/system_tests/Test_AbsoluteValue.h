/*********************                                                        */
/*! \file Test_Bsolutevalue.h
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

#include "AbsoluteValueConstraint.h"
#include "AcasParser.h"
#include "Engine.h"
#include "FloatUtils.h"
#include "InputQuery.h"

class AbsoluteValueTestSuite : public CxxTest::TestSuite
{
public:
    void setUp()
    {
    }

    void tearDown()
    {
    }

    void test_absoluate_value()
    {
        InputQuery inputQuery;
        AcasParser acasParser( RESOURCES_DIR "/nnet/acasxu/ACASXU_experimental_v2a_1_1.nnet" );
        acasParser.generateQuery( inputQuery );

        // We run an adversarial-robustness-like query around 0, by
        // bounding the expression
        //
        //   \sum_{i=1}^5 | x_i |
        //
        // where x_i are the input nodes. This requires adding 6 new
        // variables to the query:
        //
        //    5 variables for x'_i = abs( x_i )
        //    1 variable for sum( x'i )

        unsigned numVariables = inputQuery.getNumberOfVariables();
        inputQuery.setNumberOfVariables( numVariables + 6 );

        // Individual absolute values
        for ( unsigned i = 0; i < 5; ++i )
        {
            unsigned inputVariable = acasParser.getInputVariable( i );
            AbsoluteValueConstraint *abs = new AbsoluteValueConstraint
                ( inputVariable, numVariables + i );
            inputQuery.addPiecewiseLinearConstraint( abs );
        }

        // Sum of absolute values
        Equation equation;
        equation.addAddend( -1, numVariables + 5 );
        for ( unsigned i = 0; i < 5; ++i )
        {
            equation.addAddend( 1, numVariables + i );
        }
        inputQuery.addEquation( equation );

        // Bound the maximal L1 change (delta)
        const double delta = 0.01;
        inputQuery.setUpperBound( numVariables + 5, delta );

        // Output constraint: we seek a point in which output 0 gets
        // a lower score than output 1
        unsigned minimalOutputVar = acasParser.getOutputVariable( 0 );
        unsigned largerOutputVar = acasParser.getOutputVariable( 1 );

        Equation equationOut;
        equationOut.setType( Equation::LE );

        // minimal - larger <= 0    -->     larger >= minimal
        equationOut.addAddend( 1 , minimalOutputVar );
        equationOut.addAddend( -1, largerOutputVar );
        equationOut.setScalar( 0 );
        inputQuery.addEquation( equationOut );

        // Run the query
        Engine engine;
        if( !engine.processInputQuery( inputQuery ) )
        {
            // No counter example found, this is acceptable
            return;
        }

        bool result = engine.solve();
        if ( !result )
        {
            // No counter example found, this is acceptable
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

        double minimalOutputValue = outputs[0];
        double largerOutputValue = outputs[1];

        // Check whether minVar >= runnerUp, as we asked
        TS_ASSERT( FloatUtils::gte( largerOutputValue, minimalOutputValue ) );

        // Check whether the sum of inptus <= delta
        double sum = 0.0;
        for ( unsigned i = 0; i < 5; ++i )
            sum += FloatUtils::abs( inputs[i] );

        TS_ASSERT( FloatUtils::gte( delta, sum, 0.001 ) );
    }
};

//
// Local Variables:
// compile-command: "make -C ../../.. "
// tags-file-name: "../../../TAGS"
// c-basic-offset: 4
// End:
//
