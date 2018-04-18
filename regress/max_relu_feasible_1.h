/*********************                                                        */
/*! \file Max_Feasible_1.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Derek Huang
 ** This file is part of the Marabou project.
 ** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **/

#ifndef __Max_Relu_Feasible_1_h__
#define __Max_Relu_Feasible_1_h__

#include "Engine.h"
#include "FloatUtils.h"
#include "InputQuery.h"
#include "MaxConstraint.h"
#include "ReluConstraint.h"
#include <iostream>

class Max_Relu_Feasible_1
{
public:
	void run()
	{
        // a, b, c side lengths

        // d = |a-b| = max(a-b, b-a)
        // t = Relu(d-c) + Relu(c-a-b)

        InputQuery inputQuery;
        inputQuery.setNumberOfVariables( 20 );

        inputQuery.setLowerBound( 0, 0.001 ); // a
        inputQuery.setUpperBound( 0, 1 );
        inputQuery.setLowerBound( 1, 0.001 ); // b
        inputQuery.setUpperBound( 1, 1 );
        inputQuery.setLowerBound( 2, 0.001 ); // c
        inputQuery.setUpperBound( 2, 1 );

        // inputQuery.setLowerBound( 0, 0.5 ); // a
        // inputQuery.setUpperBound( 0, 0.5 );
        // inputQuery.setLowerBound( 1, 0.5 ); // b
        // inputQuery.setUpperBound( 1, 0.5 );
        // inputQuery.setLowerBound( 2, 0.5 ); // c
        // inputQuery.setUpperBound( 2, 0.5 );
        
 		// for ( unsigned i = 3; i < 10; ++i )
   //      {
   //          inputQuery.setLowerBound( i, -1000 );
   //          inputQuery.setUpperBound( i, 1000 );
   //      }

        for ( unsigned i = 11; i < 16; ++i )
        {
            inputQuery.setLowerBound( i, 0 );
            inputQuery.setUpperBound( i, 0 );
        }

        Equation equation1;
        equation1.addAddend( -1, 0 );
        equation1.addAddend( 1, 1 );
        equation1.addAddend( 1, 3 );
        equation1.addAddend( 1, 11 );
        equation1.setScalar( 0 );
        equation1.markAuxiliaryVariable( 11 );
        inputQuery.addEquation( equation1 ); // x3 is a-b

        Equation equation2;
        equation2.addAddend( 1, 0 );
        equation2.addAddend( -1, 1 );
        equation2.addAddend( 1, 4 );
        equation2.addAddend( 1, 12 );
        equation2.setScalar( 0 );
        equation2.markAuxiliaryVariable( 12 );
        inputQuery.addEquation( equation2 ); // x4 is b-a
        
        MaxConstraint* max1 = new MaxConstraint( 5, Set<unsigned>( { 3, 4 } ) ) ;
        inputQuery.addPiecewiseLinearConstraint( max1 ); // x5 is |a-b|
        Equation maxEquation1;
        maxEquation1.addAddend( -1, 5 );
        maxEquation1.addAddend( 1, 3 );
        maxEquation1.addAddend( 1, 16 );
        maxEquation1.markAuxiliaryVariable( 16 );
        maxEquation1.setScalar( 0 );
        inputQuery.addEquation( maxEquation1 );
        inputQuery.setLowerBound( 16, 0 );
        // inputQuery.setUpperBound( 16, 1000 );
        Equation maxEquation2;
        maxEquation2.addAddend( -1, 5 );
        maxEquation2.addAddend( 1, 4 );
        maxEquation2.addAddend( 1, 17 );
        maxEquation2.markAuxiliaryVariable( 17 );
        maxEquation2.setScalar( 0 );
        inputQuery.addEquation( maxEquation2 );
        inputQuery.setLowerBound( 17, 0 );
        // inputQuery.setUpperBound( 17, 1000 );

        Equation equation3;
        equation3.addAddend( -1, 5 );
        equation3.addAddend( 1, 2 );
        equation3.addAddend( 1, 6 );
        equation3.addAddend( 1, 13 );
        equation3.setScalar( 0 );
        equation3.markAuxiliaryVariable( 13 );
        inputQuery.addEquation( equation3 ); // x6 is |a-b|-c
        
        ReluConstraint* relu1 = new ReluConstraint( 6, 7 );
        inputQuery.addPiecewiseLinearConstraint( relu1 ); // x7 is Relu(|a-b|-c)
        Equation reluEquation1;
        reluEquation1.addAddend( 1, 6 );
        reluEquation1.addAddend( -1, 7 );
        reluEquation1.addAddend( 1, 18 );
        reluEquation1.markAuxiliaryVariable( 18 );
        reluEquation1.setScalar( 0 );
        inputQuery.addEquation( reluEquation1 );
        inputQuery.setLowerBound( 18, 0 );
        // inputQuery.setUpperBound( 18, 1000 );

        Equation equation4;
        equation4.addAddend( 1, 0 );
        equation4.addAddend( 1, 1 );
        equation4.addAddend( -1, 2 );
        equation4.addAddend( 1, 8 );
        equation4.addAddend( 1, 14 );
        equation4.setScalar( 0 );
        equation4.markAuxiliaryVariable( 14 );
        inputQuery.addEquation( equation4 ); // x8 is c-a-b
        
        ReluConstraint* relu2 = new ReluConstraint( 8, 9 );
        inputQuery.addPiecewiseLinearConstraint( relu2 ); // x9 is Relu(c-a-b)
        Equation reluEquation2;
        reluEquation2.addAddend( 1, 8 );
        reluEquation2.addAddend( -1, 9 );
        reluEquation2.addAddend( 1, 19 );
        reluEquation2.markAuxiliaryVariable( 19 );
        reluEquation2.setScalar( 0 );
        inputQuery.addEquation( reluEquation2 );
        inputQuery.setLowerBound( 19, 0 );
        // inputQuery.setUpperBound( 19, 1000 );

        Equation equation5;
        equation5.addAddend( -1, 0 );
        equation5.addAddend( -1, 1 );
        equation5.addAddend( 1, 10 );
        equation5.addAddend( 1, 15 );
        equation5.setScalar( 0 );
        equation5.markAuxiliaryVariable( 15 );
        inputQuery.addEquation( equation5 ); // x10 is == 0 iff trianlge inequality satisfied

        inputQuery.setLowerBound( 10, 0 ); // assert that x10 == 0
        inputQuery.setUpperBound( 10, 0 );

        int outputStream = redirectOutputToFile( "logs/max_relu_feasible_1.txt" );

        struct timespec start = TimeUtils::sampleMicro();
        
        std::cout<<"Before preprocessing"<<std::endl;
        Engine engine;
        if ( !engine.processInputQuery( inputQuery ) )
        {
            struct timespec end = TimeUtils::sampleMicro();
            restoreOutputStream( outputStream );
            printFailed( "max_relu_feasible_1 preprocessing", start, end );
            return;
        }
        std::cout<<"After preprocessing"<<std::endl;

        bool result = engine.solve();

        struct timespec end = TimeUtils::sampleMicro();

        restoreOutputStream( outputStream );

        if ( !result )
        {
            printFailed( "max_relu_feasible_1 solving", start, end );
            return;
        }

        engine.extractSolution( inputQuery );
        // sanity check
        bool correctSolution = true;
        
        double value_a = inputQuery.getSolutionValue( 0 );
        double value_b = inputQuery.getSolutionValue( 1 );
        double value_c = inputQuery.getSolutionValue( 2 );
        
        double largest = value_a;
        if ( value_b > largest )
            largest = value_b;
        if ( value_c > largest )
            largest = value_c;
        if ( 2*largest >= value_a + value_b + value_c )
            correctSolution = false;

        if ( !correctSolution )
            printFailed( "max_relu_feasible_1 soln", start, end );
        else
            printPassed( "max_relu_feasible_1 soln", start, end );
    }
};

#endif // __Max_Relu_Feasible_1_h__

//
// Local Variables:
// compile-command: "make -C .. "
// tags-file-name: "../TAGS"
// c-basic-offset: 4
// End:
//
