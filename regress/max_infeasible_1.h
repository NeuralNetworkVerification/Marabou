/*********************                                                        */
/*! \file max_infeasible_1.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Derek Huang, Guy Katz, Duligur Ibeling
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

 **/

#ifndef __Max_Infeasible_1_h__
#define __Max_Infeasible_1_h__

#include "Engine.h"
#include "FloatUtils.h"
#include "InputQuery.h"
#include "MaxConstraint.h"

class Max_Infeasible_1
{
public:
	void run()
	{
        //   x0  <= 0
        //   .5  <= x1f
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


       	inputQuery.setLowerBound( 0, -large );
        inputQuery.setUpperBound( 0, 0 );

        inputQuery.setLowerBound( 1, 0.5 );
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

		MaxConstraint *max1 = new MaxConstraint( 5, Set<unsigned>( { 0, 2, 3 } ) );
		MaxConstraint *max2 = new MaxConstraint( 3, Set<unsigned>( { 0, 4 } ) );

		inputQuery.addPiecewiseLinearConstraint( max1 );
		inputQuery.addPiecewiseLinearConstraint( max2 );

        int outputStream = redirectOutputToFile( "logs/max_infeasible_1.txt" );

        struct timespec start = TimeUtils::sampleMicro();

        Engine engine;
        if ( !engine.processInputQuery( inputQuery, false ) )
        {
            restoreOutputStream( outputStream );
            struct timespec end = TimeUtils::sampleMicro();
            printPassed( "max_infeasible_1", start, end );
            return;
        }

        bool result = engine.solve();

        struct timespec end = TimeUtils::sampleMicro();

        restoreOutputStream( outputStream );

        if ( result )
            printFailed( "max_infeasible_1", start, end );
        else
            printPassed( "max_infeasible_1", start, end );
    }
};

#endif // __Max_Infeasible_1_h__

//
// Local Variables:
// compile-command: "make -C .. "
// tags-file-name: "../TAGS"
// c-basic-offset: 4
// End:
//
