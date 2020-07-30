/*********************                                                        */
/*! \file Test_sign.h
 ** \verbatim
 ** Top contributors (to current version):
 ** Guy Amir
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

class SignTestSuite : public CxxTest::TestSuite
{
public:
    void setUp()
    {
    }

    void tearDown()
    {
    }

    // similar to test_relu_2, with unSAT result required
    void test_sign_1()
    {
        InputQuery inputQuery;
        inputQuery.setNumberOfVariables( 6 );

        // 0 <= x0 <= 10
        inputQuery.setLowerBound( 0, 1 );
        inputQuery.setUpperBound( 0, 10 );

        // unSAT! x5 = 0 always
        inputQuery.setLowerBound( 5, 0.01 );

        Equation equation1;
        equation1.addAddend( 1, 0 );
        equation1.addAddend( -1, 1 );
        equation1.setScalar( 0 );

        // x1 = x0
        inputQuery.addEquation( equation1 );

        Equation equation2;
        equation2.addAddend( 1, 0 );
        equation2.addAddend( 1, 3 );
        equation2.setScalar( 0 );

        // x1 = - x3
        inputQuery.addEquation( equation2 );

        Equation equation3;
        equation3.addAddend( 1, 2 );
        equation3.addAddend( 1, 4 );
        equation3.addAddend( -1, 5 );
        equation3.setScalar( 0 );

        // x5 = x2 + x4
        inputQuery.addEquation( equation3 );

        //  x2 = sign (x1)
        SignConstraint *sign1 = new SignConstraint( 1, 2 );

        //  x4 = sign (x3)
        SignConstraint *sign2 = new SignConstraint( 3, 4 );

        inputQuery.addPiecewiseLinearConstraint( sign1 );
        inputQuery.addPiecewiseLinearConstraint( sign2 );

        Engine engine;

        // should return 'false' = unSAT
        TS_ASSERT( !engine.processInputQuery( inputQuery ) );

//        TS_ASSERT( !engine.solve() );
    }

    // similar to test_relu_2, only now its SAT
    void test_sign_2()
    {
        InputQuery inputQuery;
        inputQuery.setNumberOfVariables( 6 );

        // 0 <= x0 <= 10
        inputQuery.setLowerBound( 0, 1 );
        inputQuery.setUpperBound( 0, 10 );

        // means that can be SAT !
        inputQuery.setUpperBound( 5, 17 );

        Equation equation1;
        equation1.addAddend( 1, 0 );
        equation1.addAddend( -1, 1 );
        equation1.setScalar( 0 );

        // x1 = x0
        inputQuery.addEquation( equation1 );

        Equation equation2;
        equation2.addAddend( 1, 0 );
        equation2.addAddend( 1, 3 );
        equation2.setScalar( 0 );

        // x1 = - x3
        inputQuery.addEquation( equation2 );

        Equation equation3;
        equation3.addAddend( 1, 2 );
        equation3.addAddend( 1, 4 );
        equation3.addAddend( -1, 5 );
        equation3.setScalar( 0 );

        // x5 = x2 + x4
        inputQuery.addEquation( equation3 );

        //  x2 = sign (x1)
        SignConstraint *sign1 = new SignConstraint( 1, 2 );

        //  x4 = sign (x3)
        SignConstraint *sign2 = new SignConstraint( 3, 4 );

        inputQuery.addPiecewiseLinearConstraint( sign1 );
        inputQuery.addPiecewiseLinearConstraint( sign2 );

        Engine engine;

        TS_ASSERT(engine.processInputQuery( inputQuery ) );

        TS_ASSERT(engine.solve() );

        engine.extractSolution( inputQuery );

        bool correctSolution = true;

        double value_x0 = inputQuery.getSolutionValue( 0 );
        double value_x1 = inputQuery.getSolutionValue( 1 );
        double value_x2 = inputQuery.getSolutionValue( 2 );
        double value_x3 = inputQuery.getSolutionValue( 3 );
        double value_x4 = inputQuery.getSolutionValue( 4 );
        double value_x5 = inputQuery.getSolutionValue( 5 );

        // want x1 = x0
        if ( !FloatUtils::areEqual( value_x0, value_x1 ) )
            correctSolution = false;

        // want x3 = - x0
        if ( !FloatUtils::areEqual( value_x0, -value_x3 ) )
            correctSolution = false;

        // if x1>= 0 -> we want x2 = 1
        if (!FloatUtils::lt(value_x1, 0))
        {
            if (!FloatUtils::areEqual( value_x2, 1 ))
            {
                correctSolution = false;
            }
        }

        // if x1< 0 -> we want x2 = - 1
        if (FloatUtils::lt(value_x1, 0))
        {
            if (!FloatUtils::areEqual( value_x2, -1 ))
            {
                correctSolution = false;
            }
        }

        // if x3>= 0 -> we want x4 = 1
        if (!FloatUtils::lt(value_x3, 0))
        {
            if (!FloatUtils::areEqual( value_x4, 1 ))
            {
                correctSolution = false;
            }
        }

        // if x3< 0 -> we want x4 = - 1
        if (FloatUtils::lt(value_x3, 0))
        {
            if (!FloatUtils::areEqual( value_x4, -1 ))
            {
                correctSolution = false;
            }
        }

        // we want x5 = x2 + x4
        if ( !FloatUtils::areEqual( value_x5, value_x2 + value_x4 ) )
        {
            correctSolution = false;
        }

        // we want x5 <= 17
        if ( FloatUtils::gt( value_x5, 17 ) )
        {
            correctSolution = false;
        }

        TS_ASSERT( correctSolution );
    }

    // more advanced BNN
    void test_sign_3()
    {
        InputQuery inputQuery;
        inputQuery.setNumberOfVariables( 11 );

        // -10 <= x0 <= -5   -> input to the BNN is negative
        inputQuery.setLowerBound( 0, -10 );
        inputQuery.setUpperBound( 0, -5 );

        // unSAT! because x10 = -1 -1 -1 = -3 (always)
        inputQuery.setLowerBound( 10, -2.5 );

        // equations of 1st layer (x0 is input)
        Equation equation1;
        equation1.addAddend( 1, 0 );
        equation1.addAddend( -1, 1 );
        equation1.setScalar( -2 );

        // x1 = x0 + 2
        inputQuery.addEquation( equation1 );

        Equation equation2;
        equation2.addAddend( 1, 0 );
        equation2.addAddend( -1, 2 );
        equation2.setScalar( 0 );

        // x2 = x0
        inputQuery.addEquation( equation2 );

        Equation equation3;
        equation3.addAddend( 0, 1 );
        equation3.addAddend( 1, 3 );
        equation3.setScalar( 0 );

        // x3 = 0*x1 = 0
        inputQuery.addEquation( equation3 );

        //equations of 2nd layer
        Equation equation4;
        equation4.addAddend( 1, 1 );
        equation4.addAddend( 2, 2 );
        equation4.addAddend( 3, 3 );
        equation4.addAddend( -1, 4 );
        equation4.setScalar( 0 );

        // x4 = 1*x1 + 2*x2 +3*x3
        inputQuery.addEquation( equation4 );

        Equation equation5;
        equation5.addAddend( 2, 2 );
        equation5.addAddend( -1, 5 );
        equation5.setScalar( 0 );

        // x5 = 2*x2
        inputQuery.addEquation( equation5 );

        Equation equation6;
        equation6.addAddend( 1, 1 );
        equation6.addAddend( 2, 2 );
        equation6.addAddend( 3, 3 );
        equation6.addAddend( -1, 6 );
        equation6.setScalar( 0 );

        // x6 = 1*x1 + 2*x2 +3*x3
        inputQuery.addEquation( equation6 );

        //equations of 3rd layer (sign activation layer)
        // x7 = sign (x4)
        SignConstraint *sign1 = new SignConstraint( 4, 7 );

        // x8 = sign (x5)
        SignConstraint *sign2 = new SignConstraint( 5, 8 );

        // x9 = sign (x6)
        SignConstraint *sign3 = new SignConstraint( 6, 9 );

        inputQuery.addPiecewiseLinearConstraint( sign1 );
        inputQuery.addPiecewiseLinearConstraint( sign2 );
        inputQuery.addPiecewiseLinearConstraint( sign3 );

        //equation of 4th layer (output)
        Equation equation7;
        equation7.addAddend( 1, 7 );
        equation7.addAddend( 1, 8 );
        equation7.addAddend( 1, 9 );
        equation7.addAddend( -1, 10 );
        equation7.setScalar( 0 );

        // x10 = 1*x7 + 1*x8 +1*x9
        inputQuery.addEquation( equation7 );

        Engine engine;
        // should return 'false' = unSAT

        TS_ASSERT( !engine.processInputQuery( inputQuery ) );
//        TS_ASSERT( !engine.solve() );
    }

    // more advanced BNN
    void test_sign_4()
    {
        InputQuery inputQuery;
        inputQuery.setNumberOfVariables( 11 );

        // x0 = 0   -> input to the BNN is negative
        inputQuery.setLowerBound( 0, -2 );
        inputQuery.setUpperBound( 0, -2 );

        // unSAT! because x10 = -1 -1 -1 = -3 (always)
        inputQuery.setLowerBound( 10, -2.8 );

        // equations of 1st layer (x0 is input)
        Equation equation1;
        equation1.addAddend( 1, 0 );
        equation1.addAddend( -1, 1 );
        equation1.setScalar( -2 );

        // x1 = x0 + 2
        inputQuery.addEquation( equation1 );

        Equation equation2;
        equation2.addAddend( 1, 0 );
        equation2.addAddend( -1, 2 );
        equation2.setScalar( 0 );

        // x2 = x0
        inputQuery.addEquation( equation2 );

        Equation equation3;
        equation3.addAddend( 0, 1 );
        equation3.addAddend( 1, 3 );
        equation3.setScalar( 0 );

        // x3 = 0*x1 = 0
        inputQuery.addEquation( equation3 );

        //equations of 2nd layer
        Equation equation4;
        equation4.addAddend( 1, 1 );
        equation4.addAddend( 2, 2 );
        equation4.addAddend( 3, 3 );
        equation4.addAddend( -1, 4 );
        equation4.setScalar( 0 );

        // x4 = 1*x1 + 2*x2 +3*x3
        inputQuery.addEquation( equation4 );

        Equation equation5;
        equation5.addAddend( 2, 2 );
        equation5.addAddend( -1, 5 );
        equation5.setScalar( 0 );

        // x5 = 2*x2
        inputQuery.addEquation( equation5 );

        Equation equation6;
        equation6.addAddend( 1, 1 );
        equation6.addAddend( 2, 2 );
        equation6.addAddend( 3, 3 );
        equation6.addAddend( -1, 6 );
        equation6.setScalar( 0 );

        // x6 = 1*x1 + 2*x2 +3*x3
        inputQuery.addEquation( equation6 );

        // equations of 3rd layer (sign activation layer)
        // x7 = sign (x4)
        SignConstraint *sign1 = new SignConstraint( 4, 7 );

        // x8 = sign (x5)
        SignConstraint *sign2 = new SignConstraint( 5, 8 );

        // x9 = sign (x6)
        SignConstraint *sign3 = new SignConstraint( 6, 9 );

        inputQuery.addPiecewiseLinearConstraint( sign1 );
        inputQuery.addPiecewiseLinearConstraint( sign2 );
        inputQuery.addPiecewiseLinearConstraint( sign3 );

        //equation of 4th layer (output)
        Equation equation7;
        equation7.addAddend( 1, 7 );
        equation7.addAddend( 1, 8 );
        equation7.addAddend( 1, 9 );
        equation7.addAddend( -1, 10 );
        equation7.setScalar( 0 );

        // x10 = 1*x7 + 1*x8 +1*x9
        inputQuery.addEquation( equation7 );

        Engine engine;

        // should return 'false' = unSAT
        TS_ASSERT( !engine.processInputQuery( inputQuery ) );

//        TS_ASSERT( !engine.solve() );
    }

    void test_sign_5()
    {
        InputQuery inputQuery;
        inputQuery.setNumberOfVariables( 11 );

        // -10 <= x0 <= -5   -> input to the BNN is negative
        inputQuery.setLowerBound( 0, -10 );
        inputQuery.setUpperBound( 0, -5 );

        // SAT! because x10 = -1 -1 -1 = -3 (always)
        inputQuery.setLowerBound( 10, -3 );

        // equations of 1st layer (x0 is input)
        Equation equation1;
        equation1.addAddend( 1, 0 );
        equation1.addAddend( -1, 1 );
        equation1.setScalar( -2 );

        // x1 = x0 + 2
        inputQuery.addEquation( equation1 );

        Equation equation2;
        equation2.addAddend( 1, 0 );
        equation2.addAddend( -1, 2 );
        equation2.setScalar( 0 );

        // x2 = x0
        inputQuery.addEquation( equation2 );

        Equation equation3;
        equation3.addAddend( 0, 1 );
        equation3.addAddend( 1, 3 );
        equation3.setScalar( 0 );

        // x3 = 0*x1 = 0
        inputQuery.addEquation( equation3 );

        //equations of 2nd layer
        Equation equation4;
        equation4.addAddend( 1, 1 );
        equation4.addAddend( 2, 2 );
        equation4.addAddend( 3, 3 );
        equation4.addAddend( -1, 4 );
        equation4.setScalar( 0 );

        // x4 = 1*x1 + 2*x2 +3*x3
        inputQuery.addEquation( equation4 );

        Equation equation5;
        equation5.addAddend( 2, 2 );
        equation5.addAddend( -1, 5 );
        equation5.setScalar( 0 );

        // x5 = 2*x2
        inputQuery.addEquation( equation5 );

        Equation equation6;
        equation6.addAddend( 1, 1 );
        equation6.addAddend( 2, 2 );
        equation6.addAddend( 3, 3 );
        equation6.addAddend( -1, 6 );
        equation6.setScalar( 0 );

        // x6 = 1*x1 + 2*x2 +3*x3
        inputQuery.addEquation( equation6 );

        //equations of 3rd layer (sign activation layer)
        // x7 = sign (x4)
        SignConstraint *sign1 = new SignConstraint( 4, 7 );

        // x8 = sign (x5)
        SignConstraint *sign2 = new SignConstraint( 5, 8 );

        // x9 = sign (x6)
        SignConstraint *sign3 = new SignConstraint( 6, 9 );

        inputQuery.addPiecewiseLinearConstraint( sign1 );
        inputQuery.addPiecewiseLinearConstraint( sign2 );
        inputQuery.addPiecewiseLinearConstraint( sign3 );

        //equation of 4th layer (output)
        Equation equation7;
        equation7.addAddend( 1, 7 );
        equation7.addAddend( 1, 8 );
        equation7.addAddend( 1, 9 );
        equation7.addAddend( -1, 10 );
        equation7.setScalar( 0 );

        // x10 = 1*x7 + 1*x8 +1*x9
        inputQuery.addEquation( equation7 );

        Engine engine;

        // should return SAT
        TS_ASSERT(engine.processInputQuery( inputQuery ) );
        TS_ASSERT( engine.solve() );

        engine.extractSolution( inputQuery );

        bool correctSolution = true;

        double value_x0 = inputQuery.getSolutionValue( 0 );
        double value_x1 = inputQuery.getSolutionValue( 1 );
        double value_x2 = inputQuery.getSolutionValue( 2 );
        double value_x3 = inputQuery.getSolutionValue( 3 );
        double value_x4 = inputQuery.getSolutionValue( 4 );
        double value_x5 = inputQuery.getSolutionValue( 5 );
        double value_x6 = inputQuery.getSolutionValue( 6 );
        double value_x7 = inputQuery.getSolutionValue( 7 );
        double value_x8 = inputQuery.getSolutionValue( 8 );
        double value_x9 = inputQuery.getSolutionValue( 9 );
        double value_x10 = inputQuery.getSolutionValue( 10 );

        // check first layer variables
        // want x1 = x0 + 2
        if ( !FloatUtils::areEqual( value_x1, value_x0 + 2 ) )
        {
            correctSolution = false;
        }

        // want x2 = x0
        if ( !FloatUtils::areEqual( value_x2, value_x0 ) )
        {
            correctSolution = false;
        }

        // we want x3 = 0
        if ( !FloatUtils::areEqual( value_x3, 0 ) )
        {
            correctSolution = false;
        }

        // check 2nd layer variables
        // we want x4 = 1*x1 + 2*x2 + 3*x3
        if ( !FloatUtils::areEqual( value_x4, value_x1 + 2*value_x2 + 3*value_x3 ) )
        {
            correctSolution = false;
        }

        // we want x5 = 2*x2
        if ( !FloatUtils::areEqual( value_x5, 2*value_x2 ) )
        {
            correctSolution = false;
        }

        // we want x6 = 1*x1 + 2*x2 + 3*x3
        if ( !FloatUtils::areEqual( value_x6, value_x1 + 2*value_x2 + 3*value_x3 ) )
        {
            correctSolution = false;
        }

        // check 3rd layer
        // we want x7 = sign(x4)
        // if x4>= 0 -> we want x7 = 1
        if (!FloatUtils::lt(value_x4, 0))
        {
            if (!FloatUtils::areEqual( value_x7, 1 ))
            {
                correctSolution = false;
            }
        }

        // if x4< 0 -> we want x7 = - 1
        if (FloatUtils::lt(value_x4, 0))
        {
            if (!FloatUtils::areEqual( value_x7, -1 ))
            {
                correctSolution = false;
            }
        }

        // we want x8 = sign(x5)
        // if x5>= 0 -> we want x8 = 1
        if (!FloatUtils::lt(value_x5, 0))
        {
            if (!FloatUtils::areEqual( value_x8, 1 ))
            {
                correctSolution = false;
            }
        }

        // if x5< 0 -> we want x8 = - 1
        if (FloatUtils::lt(value_x5, 0))
        {
            if (!FloatUtils::areEqual( value_x8, -1 ))
            {
                correctSolution = false;
            }
        }

        // we want x9 = sign(x6)
        // if x6>= 0 -> we want x9 = 1
        if (!FloatUtils::lt(value_x6, 0))
        {
            if (!FloatUtils::areEqual( value_x9, 1 ))
            {
                correctSolution = false;
            }
        }

        // if x6< 0 -> we want x9 = - 1
        if (FloatUtils::lt(value_x6, 0))
        {
            if (!FloatUtils::areEqual( value_x9, -1 ))
            {
                correctSolution = false;
            }
        }

        // check last layer variable (BNN output)
        // we want x10 = x7 + x8 +x9
        if ( !FloatUtils::areEqual( value_x10, value_x7 + value_x8 + value_x9 ) )
        {
            correctSolution = false;
        }

        TS_ASSERT( correctSolution );
    }

    void test_sign_6()
    {
        InputQuery inputQuery;
        inputQuery.setNumberOfVariables( 11 );

        // -10 <= x0 <= 10   -> input to the BNN is negative
        inputQuery.setLowerBound( 0, -10 );
        inputQuery.setUpperBound( 0, 10 );

        // SAT! because x10 = -1 -1 -1 = -3 (always)
        inputQuery.setLowerBound( 10, -3 );

        // equations of 1st layer (x0 is input)
        Equation equation1;
        equation1.addAddend( 1, 0 );
        equation1.addAddend( -1, 1 );
        equation1.setScalar( -2 );

        // x1 = x0 + 2
        inputQuery.addEquation( equation1 );

        Equation equation2;
        equation2.addAddend( 1, 0 );
        equation2.addAddend( -1, 2 );
        equation2.setScalar( 0 );

        // x2 = x0
        inputQuery.addEquation( equation2 );

        Equation equation3;
        equation3.addAddend( 0, 1 );
        equation3.addAddend( 1, 3 );
        equation3.setScalar( 0 );

        // x3 = 0*x1 = 0
        inputQuery.addEquation( equation3 );

        //equations of 2nd layer
        Equation equation4;
        equation4.addAddend( 1, 1 );
        equation4.addAddend( 2, 2 );
        equation4.addAddend( 3, 3 );
        equation4.addAddend( -1, 4 );
        equation4.setScalar( 0 );

        // x4 = 1*x1 + 2*x2 +3*x3
        inputQuery.addEquation( equation4 );

        Equation equation5;
        equation5.addAddend( 2, 2 );
        equation5.addAddend( -1, 5 );
        equation5.setScalar( 0 );

        // x5 = 2*x2
        inputQuery.addEquation( equation5 );

        Equation equation6;
        equation6.addAddend( 1, 1 );
        equation6.addAddend( 2, 2 );
        equation6.addAddend( 3, 3 );
        equation6.addAddend( -1, 6 );
        equation6.setScalar( 0 );

        // x6 = 1*x1 + 2*x2 +3*x3
        inputQuery.addEquation( equation6 );

        // equations of 3rd layer (sign activation layer)
        // x7 = sign (x4)
        SignConstraint *sign1 = new SignConstraint( 4, 7 );

        // x8 = sign (x5)
        SignConstraint *sign2 = new SignConstraint( 5, 8 );

        // x9 = sign (x6)
        SignConstraint *sign3 = new SignConstraint( 6, 9 );

        inputQuery.addPiecewiseLinearConstraint( sign1 );
        inputQuery.addPiecewiseLinearConstraint( sign2 );
        inputQuery.addPiecewiseLinearConstraint( sign3 );

        //equation of 4th layer (output)
        Equation equation7;
        equation7.addAddend( 1, 7 );
        equation7.addAddend( 1, 8 );
        equation7.addAddend( 1, 9 );
        equation7.addAddend( -1, 10 );
        equation7.setScalar( 0 );

        // x10 = 1*x7 + 1*x8 +1*x9
        inputQuery.addEquation( equation7 );

        // should return SAT
        Engine engine;

        TS_ASSERT(engine.processInputQuery( inputQuery ) );
        TS_ASSERT( engine.solve() );

        engine.extractSolution( inputQuery );

        bool correctSolution = true;

        double value_x0 = inputQuery.getSolutionValue( 0 );
        double value_x1 = inputQuery.getSolutionValue( 1 );
        double value_x2 = inputQuery.getSolutionValue( 2 );
        double value_x3 = inputQuery.getSolutionValue( 3 );
        double value_x4 = inputQuery.getSolutionValue( 4 );
        double value_x5 = inputQuery.getSolutionValue( 5 );
        double value_x6 = inputQuery.getSolutionValue( 6 );
        double value_x7 = inputQuery.getSolutionValue( 7 );
        double value_x8 = inputQuery.getSolutionValue( 8 );
        double value_x9 = inputQuery.getSolutionValue( 9 );
        double value_x10 = inputQuery.getSolutionValue( 10 );

        // check first layer variables
        // want x1 = x0 + 2
        if ( !FloatUtils::areEqual( value_x1, value_x0 + 2 ) )
        {
            correctSolution = false;
        }

        // we want x2 = x0
        if ( !FloatUtils::areEqual( value_x2, value_x0 ) )
        {
            correctSolution = false;
        }

        // we want x3 = 0
        if ( !FloatUtils::areEqual( value_x3, 0 ) )
        {
            correctSolution = false;
        }

        // check 2nd layer variables
        // want x4 = 1*x1 + 2*x2 + 3*x3
        if ( !FloatUtils::areEqual( value_x4, value_x1 + 2*value_x2 + 3*value_x3 ) )
        {
            correctSolution = false;
        }

        // want x5 = 2*x2
        if ( !FloatUtils::areEqual( value_x5, 2*value_x2 ) )
        {
            correctSolution = false;
        }

        // want x6 = 1*x1 + 2*x2 + 3*x3
        if ( !FloatUtils::areEqual( value_x6, value_x1 + 2*value_x2 + 3*value_x3 ) )
        {
            correctSolution = false;
        }

        // check 3rd layer
        // want x7 = sign(x4)
        // if x4>= 0 -> we want x7 = 1
        if (!FloatUtils::lt(value_x4, 0))
        {
            if (!FloatUtils::areEqual( value_x7, 1 ))
            {
                correctSolution = false;
            }
        }

        // if x4< 0 -> we want x7 = - 1
        if (FloatUtils::lt(value_x4, 0))
        {
            if (!FloatUtils::areEqual( value_x7, -1 ))
            {
                correctSolution = false;
            }
        }

        // want x8 = sign(x5)
        // if x5>= 0 -> we want x8 = 1
        if (!FloatUtils::lt(value_x5, 0))
        {
            if (!FloatUtils::areEqual( value_x8, 1 ))
            {
                correctSolution = false;
            }
        }

        // if x5< 0 -> we want x8 = - 1
        if (FloatUtils::lt(value_x5, 0))
        {
            if (!FloatUtils::areEqual( value_x8, -1 ))
            {
                correctSolution = false;
            }
        }

        // we want x9 = sign(x6)
        // if x6>= 0 -> we want x9 = 1
        if (!FloatUtils::lt(value_x6, 0))
        {
            if (!FloatUtils::areEqual( value_x9, 1 ))
            {
                correctSolution = false;
            }
        }

        // if x6< 0 -> we want x9 = - 1
        if (FloatUtils::lt(value_x6, 0))
        {
            if (!FloatUtils::areEqual( value_x9, -1 ))
            {
                correctSolution = false;
            }
        }

        // check last layer variable (BNN output)
        // we want x10 = x7 + x8 +x9
        if ( !FloatUtils::areEqual( value_x10, value_x7 + value_x8 + value_x9 ) )
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
