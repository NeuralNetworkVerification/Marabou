/*********************                                                        */
/*! \file Test_relu.h
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

#include "Engine.h"
#include "InputQuery.h"
#include "FloatUtils.h"
#include "ReluConstraint.h"


class ReluTestSuite : public CxxTest::TestSuite
{
public:
    void setUp()
    {
    }

    void tearDown()
    {
    }

    void test_relu_1()
    {
        double large = 1000;

        InputQuery inputQuery;
        inputQuery.setNumberOfVariables( 9 );

        inputQuery.setLowerBound( 0, 0 );
        inputQuery.setUpperBound( 0, 1 );

        inputQuery.setLowerBound( 1, -large );
        inputQuery.setUpperBound( 1, large );

        inputQuery.setLowerBound( 2, 0 );
        inputQuery.setUpperBound( 2, large );

        inputQuery.setLowerBound( 3, -large );
        inputQuery.setUpperBound( 3, large );

        inputQuery.setLowerBound( 4, 0 );
        inputQuery.setUpperBound( 4, large );

        inputQuery.setLowerBound( 5, 0.5 );
        inputQuery.setUpperBound( 5, 1 );

        inputQuery.setLowerBound( 6, 0 );
        inputQuery.setUpperBound( 6, 0 );
        inputQuery.setLowerBound( 7, 0 );
        inputQuery.setUpperBound( 7, 0 );
        inputQuery.setLowerBound( 8, 0 );
        inputQuery.setUpperBound( 8, 0 );

        Equation equation1;
        equation1.addAddend( 1, 0 );
        equation1.addAddend( -1, 1 );
        equation1.addAddend( 1, 6 );
        equation1.setScalar( 0 );
        inputQuery.addEquation( equation1 );

        Equation equation2;
        equation2.addAddend( 1, 0 );
        equation2.addAddend( 1, 3 );
        equation2.addAddend( 1, 7 );
        equation2.setScalar( 0 );
        inputQuery.addEquation( equation2 );

        Equation equation3;
        equation3.addAddend( 1, 2 );
        equation3.addAddend( 1, 4 );
        equation3.addAddend( -1, 5 );
        equation3.addAddend( 1, 8 );
        equation3.setScalar( 0 );
        inputQuery.addEquation( equation3 );

        ReluConstraint *relu1 = new ReluConstraint( 1, 2 );
        ReluConstraint *relu2 = new ReluConstraint( 3, 4 );

        inputQuery.addPiecewiseLinearConstraint( relu1 );
        inputQuery.addPiecewiseLinearConstraint( relu2 );

        Engine engine;
        TS_ASSERT_THROWS_NOTHING( engine.processInputQuery( inputQuery ) );

        TS_ASSERT_THROWS_NOTHING( engine.solve() );

        engine.extractSolution( inputQuery );

        bool correctSolution = true;
        // Sanity test

        double value_x0 = inputQuery.getSolutionValue( 0 );
        double value_x1b = inputQuery.getSolutionValue( 1 );
        double value_x1f = inputQuery.getSolutionValue( 2 );
        double value_x2b = inputQuery.getSolutionValue( 3 );
        double value_x2f = inputQuery.getSolutionValue( 4 );
        double value_x3 = inputQuery.getSolutionValue( 5 );

        if ( !FloatUtils::areEqual( value_x0, value_x1b ) )
            correctSolution = false;

        if ( !FloatUtils::areEqual( value_x0, -value_x2b ) )
            correctSolution = false;

        if ( !FloatUtils::areEqual( value_x3, value_x1f + value_x2f ) )
            correctSolution = false;

        if ( FloatUtils::lt( value_x0, 0 ) || FloatUtils::gt( value_x0, 1 ) ||
             FloatUtils::lt( value_x1f, 0 ) || FloatUtils::lt( value_x2f, 0 ) ||
             FloatUtils::lt( value_x3, 0.5 ) || FloatUtils::gt( value_x3, 1 ) )
        {
            correctSolution = false;
        }

        if ( FloatUtils::isPositive( value_x1f ) && !FloatUtils::areEqual( value_x1b, value_x1f ) )
        {
            correctSolution = false;
        }

        if ( FloatUtils::isPositive( value_x2f ) && !FloatUtils::areEqual( value_x2b, value_x2f ) )
        {
            correctSolution = false;
        }

        TS_ASSERT( correctSolution );
    }

    void test_relu_2()
    {
        InputQuery inputQuery;
        inputQuery.setNumberOfVariables( 6 );

        inputQuery.setLowerBound( 0, 0 );
        inputQuery.setUpperBound( 0, 1 );

        inputQuery.setLowerBound( 5, 0.5 );
        inputQuery.setUpperBound( 5, 1 );

        Equation equation1;
        equation1.addAddend( 1, 0 );
        equation1.addAddend( -1, 1 );
        equation1.setScalar( 0 );
        inputQuery.addEquation( equation1 );

        Equation equation2;
        equation2.addAddend( 1, 0 );
        equation2.addAddend( 1, 3 );
        equation2.setScalar( 0 );
        inputQuery.addEquation( equation2 );

        Equation equation3;
        equation3.addAddend( 1, 2 );
        equation3.addAddend( 1, 4 );
        equation3.addAddend( -1, 5 );
        equation3.setScalar( 0 );
        inputQuery.addEquation( equation3 );

        ReluConstraint *relu1 = new ReluConstraint( 1, 2 );
        ReluConstraint *relu2 = new ReluConstraint( 3, 4 );

        inputQuery.addPiecewiseLinearConstraint( relu1 );
        inputQuery.addPiecewiseLinearConstraint( relu2 );

        Engine engine;
        TS_ASSERT_THROWS_NOTHING( engine.processInputQuery( inputQuery ) );

        TS_ASSERT_THROWS_NOTHING ( engine.solve() );


        engine.extractSolution( inputQuery );

        bool correctSolution = true;
        // Sanity test

        double value_x0 = inputQuery.getSolutionValue( 0 );
        double value_x1b = inputQuery.getSolutionValue( 1 );
        double value_x1f = inputQuery.getSolutionValue( 2 );
        double value_x2b = inputQuery.getSolutionValue( 3 );
        double value_x2f = inputQuery.getSolutionValue( 4 );
        double value_x3 = inputQuery.getSolutionValue( 5 );

        if ( !FloatUtils::areEqual( value_x0, value_x1b ) )
            correctSolution = false;

        if ( !FloatUtils::areEqual( value_x0, -value_x2b ) )
            correctSolution = false;

        if ( !FloatUtils::areEqual( value_x3, value_x1f + value_x2f ) )
            correctSolution = false;

        if ( FloatUtils::lt( value_x0, 0 ) || FloatUtils::gt( value_x0, 1 ) ||
             FloatUtils::lt( value_x1f, 0 ) || FloatUtils::lt( value_x2f, 0 ) ||
             FloatUtils::lt( value_x3, 0.5 ) || FloatUtils::gt( value_x3, 1 ) )
        {
            correctSolution = false;
        }

        if ( FloatUtils::isPositive( value_x1f ) && !FloatUtils::areEqual( value_x1b, value_x1f ) )
        {
            correctSolution = false;
        }

        if ( FloatUtils::isPositive( value_x2f ) && !FloatUtils::areEqual( value_x2b, value_x2f ) )
        {
            correctSolution = false;
        }

        TS_ASSERT( correctSolution );
    }
};

//
// Local Variables:
// compile-command: "make -C ../../.. "
// tags-file-name: "../../../TAGS"
// c-basic-offset: 4
// End:
//
