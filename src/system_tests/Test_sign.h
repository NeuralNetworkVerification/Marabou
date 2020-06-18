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


    void test_sign_2() // similar to test_relu_2, with unSAT result required
    {
        InputQuery inputQuery;
        inputQuery.setNumberOfVariables( 6 );

        // 0 <= x0 <= 10
        inputQuery.setLowerBound( 0, 1 );
        inputQuery.setUpperBound( 0, 10 );

        // todo change!!
        inputQuery.setLowerBound( 5, 0.01 ); // unSAT! x5 = 0 always


        Equation equation1;
        equation1.addAddend( 1, 0 );
        equation1.addAddend( -1, 1 );
        equation1.setScalar( 0 );
        inputQuery.addEquation( equation1 ); // x1 = x0

        Equation equation2;
        equation2.addAddend( 1, 0 );
        equation2.addAddend( 1, 3 );
        equation2.setScalar( 0 );
        inputQuery.addEquation( equation2 ); // x1 = - x3

        Equation equation3;
        equation3.addAddend( 1, 2 );
        equation3.addAddend( 1, 4 );
        equation3.addAddend( -1, 5 );
        equation3.setScalar( 0 );
        inputQuery.addEquation( equation3 ); // x5 = x2 + x4

        SignConstraint *sign1 = new SignConstraint( 1, 2 ); //  x2 = sign (x1)
        SignConstraint *sign2 = new SignConstraint( 3, 4 ); //  x4 = sign (x3)

        inputQuery.addPiecewiseLinearConstraint( sign1 );
        inputQuery.addPiecewiseLinearConstraint( sign2 );

        Engine engine;

        TS_ASSERT( !engine.processInputQuery( inputQuery ) ); // was originally "TS_ASSET_THROWS_NOTHING"
        // should return 'false' = unSAT


//        TS_ASSERT( !engine.solve() ); // was originally "TS_ASSET_THROWS_NOTHING"


    }


    void test_sign_3() // similar to test_relu_2, only now its SAT
    {
        InputQuery inputQuery;
        inputQuery.setNumberOfVariables( 6 );

        // 0 <= x0 <= 10
        inputQuery.setLowerBound( 0, 1 );
        inputQuery.setUpperBound( 0, 10 );

        // means unSAT !
        inputQuery.setUpperBound( 5, 17 ); // unSAT! x5 = 0 always


        Equation equation1;
        equation1.addAddend( 1, 0 );
        equation1.addAddend( -1, 1 );
        equation1.setScalar( 0 );
        inputQuery.addEquation( equation1 ); // x1 = x0

        Equation equation2;
        equation2.addAddend( 1, 0 );
        equation2.addAddend( 1, 3 );
        equation2.setScalar( 0 );
        inputQuery.addEquation( equation2 ); // x1 = - x3

        Equation equation3;
        equation3.addAddend( 1, 2 );
        equation3.addAddend( 1, 4 );
        equation3.addAddend( -1, 5 );
        equation3.setScalar( 0 );
        inputQuery.addEquation( equation3 ); // x5 = x2 + x4

        SignConstraint *sign1 = new SignConstraint( 1, 2 ); //  x2 = sign (x1)
        SignConstraint *sign2 = new SignConstraint( 3, 4 ); //  x4 = sign (x3)

        inputQuery.addPiecewiseLinearConstraint( sign1 );
        inputQuery.addPiecewiseLinearConstraint( sign2 );

        Engine engine;

        TS_ASSERT(engine.processInputQuery( inputQuery ) ); // was originally "TS_ASSET_THROWS_NOTHING"

        TS_ASSERT(engine.solve() ); // was originally "TS_ASSET_THROWS_NOTHING"



        engine.extractSolution( inputQuery );

        bool correctSolution = true;
        // Sanity test


        // TODO - CHECK - phase not fixed after solve
        double value_x0 = inputQuery.getSolutionValue( 0 );
        double value_x1 = inputQuery.getSolutionValue( 1 );
        double value_x2 = inputQuery.getSolutionValue( 2 );
        double value_x3 = inputQuery.getSolutionValue( 3 );
        double value_x4 = inputQuery.getSolutionValue( 4 );
        double value_x5 = inputQuery.getSolutionValue( 5 );


//        (void) correctSolution;
//        (void) value_x0;
//        (void) value_x1;
//        (void) value_x2;
//        (void) value_x3;
//        (void) value_x4;
//        (void) value_x5;

        if ( !FloatUtils::areEqual( value_x0, value_x1 ) ) // we want x1 = x0
            correctSolution = false;

        if ( !FloatUtils::areEqual( value_x0, -value_x3 ) ) // we want x3 = - x0
            correctSolution = false;


        if (!FloatUtils::lt(value_x1, 0)) // if x1>= 0 -> we want x2 = 1
        {
            if (!FloatUtils::areEqual( value_x2, 1 )) {
                correctSolution = false;
            }
        }

        if (FloatUtils::lt(value_x1, 0))// if x1< 0 -> we want x2 = - 1
        {
            if (!FloatUtils::areEqual( value_x2, -1 )) {
                correctSolution = false;
            }
        }

        if (!FloatUtils::lt(value_x3, 0)) // if x3>= 0 -> we want x4 = 1
        {
            if (!FloatUtils::areEqual( value_x4, 1 )) {
                correctSolution = false;
            }
        }

        if (FloatUtils::lt(value_x3, 0))// if x3< 0 -> we want x4 = - 1
        {
            if (!FloatUtils::areEqual( value_x4, -1 )) {
                correctSolution = false;
            }
        }


        if ( !FloatUtils::areEqual( value_x5, value_x2 + value_x4 ) ) // we want x5 = x2 + x4
        {
            correctSolution = false;
        }

        if ( FloatUtils::gt( value_x5, 17 ) ) // we want x5 <= 17 // todo remove '!'
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
