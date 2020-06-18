/*********************                                                        */
/*! \file Test_sign.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz, Guy Amir
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2020 by the authors listed in the file AUTHORS
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
#include "SignConstraint.h"

#include "ReluConstraint.h"

#ifndef MARABOU_TEST_SIGN_H
#define MARABOU_TEST_SIGN_H

class SignTestSuite : public CxxTest::TestSuite
{
public:
    void setUp()
    {
    }

    void tearDown()
    {
    }


    void test_sign_2() // similar to test
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



#endif //MARABOU_TEST_SIGN_H
