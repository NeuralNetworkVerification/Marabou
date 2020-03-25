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

        const double b = 0;

        // We run an adversarial robustness query, by bounding the expression
        //
        //   \sum_{i=1}^5 | x_i - b |
        //
        // where x_i are the input nodes. This requires adding 11 new variables to the query:
        //
        //    5 variables for x'_i = x_i - b
        //    5 variables for x''_i = abs( x'_i )
        //    1 variable for sum( x''i )

        unsigned numVariables = inputQuery.getNumberOfVariables();
        inputQuery.setNumberOfVariables( numVariables + 11 );

        // x_i - x'_i = b -> x_i - b = x'_i
        for ( unsigned i = 0; i < 5; ++i )
        {
            Equation equation;
            unsigned variable = acasParser.getInputVariable( i );

            // x_i - input to acas
            equation.addAddend( 1, variable );

            // x'_i - new variable
            equation.addAddend( -1 , numVariables + i );

            equation.setScalar( b );
            inputQuery.addEquation( equation );
        }

        // x''_i = abs( x'_i ) = abs( x_i - b )
        for ( unsigned i = 0; i < 5; ++i )
        {
            AbsoluteValueConstraint *abs = new AbsoluteValueConstraint
                ( numVariables + i, numVariables + i + 5 );
            inputQuery.addPiecewiseLinearConstraint( abs );
        }

        // t = sum( x''_i ) , 0 <= i <= 5
        //  -> sum( abs( x'_i ) ) = sum( abs( x_i - b ) ) , 0 <= i <= 5
        Equation equation;
        equation.addAddend( -1, numVariables + 10 );
        for ( unsigned i = 0; i < 5; ++i )
        {
            equation.addAddend( 1, numVariables + i + 5 );
        }
        equation.setScalar( 0 );
        inputQuery.addEquation(equation);

        // t <= bound
        const unsigned bound = 5;
        inputQuery.setUpperBound( numVariables + 10, bound );

        // Add the equation minVar >= runnerUp
        const unsigned minVarIndex = 0;
        const unsigned runnerUpIndex = 1;

        unsigned minVar = acasParser.getOutputVariable( minVarIndex );
        Equation equationOut;
        equationOut.setType( Equation::LE );

        unsigned runnerUp = acasParser.getOutputVariable( runnerUpIndex );
        equationOut.addAddend( 1, runnerUp );
        equationOut.addAddend( -1 , minVar );
        equationOut.setScalar( 0 );
        inputQuery.addEquation( equationOut );

        // Run the query
        Engine engine;
        TS_ASSERT( engine.processInputQuery( inputQuery ) );

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
            inputs.append( inputQuery.getSolutionValue( i ) );

        Vector<double> outputs;
        acasParser.evaluate( inputs, outputs );

        double minVarOut = outputs[minVarIndex];
        double runnerUpOut = outputs[runnerUpIndex];

        // Check whether minVar >= runnerUp, as we asked
        TS_ASSERT( FloatUtils::gt( minVarOut , runnerUpOut ) );

        // Check whether the sum of inptus <= BOUND
        double sum = 0.0;
        for ( unsigned i = 0; i < 5; ++i )
        {
            unsigned var = acasParser.getInputVariable( i );
            sum += FloatUtils::abs( inputs[var] - b );
        }

        TS_ASSERT( FloatUtils::gt( bound, sum ) );
    }
};

//
// Local Variables:
// compile-command: "make -C ../../.. "
// tags-file-name: "../../../TAGS"
// c-basic-offset: 4
// End:
//
