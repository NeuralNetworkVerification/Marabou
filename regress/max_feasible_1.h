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

#ifndef __Max_Feasible_1_h__
#define __Max_Feasible_1_h__

#include "Engine.h"
#include "FloatUtils.h"
#include "InputQuery.h"
#include "MaxConstraint.h"

class Max_Feasible_1
{
public:
	void run()
	{
        //   0   <= x0  <= 1
        //   0   <= x1f
		//	 0	 <= x2f
        //   1/2 <= x3  <= 1
        //
        //  x0 - x1b = 0        -->  x0 - x1b + x6 = 0
        //  x0 + x2b = 0        -->  x0 + x2b + x7 = 0
        //  x1f + x2f - x3 = 0  -->  x1f + x2f - x3 + x8 = 0
        //
        //  x5 = max(x0, x1f, x2f)
		// 	x2b = max(x0, x2f)
        //
        //   x0: x0
        //   x1: x1b
        //   x2: x1f
        //   x3: x2b
        //   x4: x2f
        //   x5: x3
		//	 x6: x6
		//	 x7: x7
		//	 x8: x8

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
        equation1.markAuxiliaryVariable( 6 );
        inputQuery.addEquation( equation1 );

        Equation equation2;
        equation2.addAddend( 1, 0 );
        equation2.addAddend( 1, 3 );
        equation2.addAddend( 1, 7 );
        equation2.setScalar( 0 );
        equation2.markAuxiliaryVariable( 7 );
        inputQuery.addEquation( equation2 );

        Equation equation3;
        equation3.addAddend( 1, 2 );
        equation3.addAddend( 1, 4 );
        equation3.addAddend( -1, 5 );
        equation3.addAddend( 1, 8 );
        equation3.setScalar( 0 );
        equation3.markAuxiliaryVariable( 8 );
        inputQuery.addEquation( equation3 );

		MaxConstraint *max1 = new MaxConstraint( 5, Set<unsigned>( { 0, 2, 3 } ) );
		MaxConstraint *max2 = new MaxConstraint( 3, Set<unsigned>( { 0, 4 } ) );

		inputQuery.addPiecewiseLinearConstraint( max1 );
		inputQuery.addPiecewiseLinearConstraint( max2 );

        int outputStream = redirectOutputToFile( "logs/max_feasible_1.txt" );

        struct timespec start = TimeUtils::sampleMicro();

        Engine engine;
        if ( !engine.processInputQuery( inputQuery, false ) )
        {
            struct timespec end = TimeUtils::sampleMicro();
            restoreOutputStream( outputStream );
            printFailed( "max_feasible_1", start, end );
            return;
        }

        bool result = engine.solve();

        struct timespec end = TimeUtils::sampleMicro();

        restoreOutputStream( outputStream );

        if ( !result )
        {
            printFailed( "max_feasible_1", start, end );
            return;
        }

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
             FloatUtils::lt( value_x1f, 0 ) ||
             FloatUtils::lt( value_x3, 0.5 ) || FloatUtils::gt( value_x3, 1 ) ||
			 FloatUtils::lt( value_x2f, 0.0 ) )
        {
            correctSolution = false;
        }

       	if ( !FloatUtils::areEqual
             ( FloatUtils::max( value_x2b, FloatUtils::max( value_x0, value_x2f ) ), value_x2b ) )
        {
            correctSolution = false;
        }

		if ( !FloatUtils::areEqual
             ( FloatUtils::max( value_x1b, FloatUtils::max( value_x0, FloatUtils::max( value_x2b, value_x1f ) ) ), value_x3 ) )
		{
            correctSolution = false;
		}

        if ( !correctSolution )
            printFailed( "max_feasible_1", start, end );
        else
            printPassed( "max_feasible_1", start, end );
    }
};

#endif // __Max_Feasible_1_h__

//
// Local Variables:
// compile-command: "make -C .. "
// tags-file-name: "../TAGS"
// c-basic-offset: 4
// End:
//
