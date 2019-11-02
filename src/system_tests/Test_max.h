/*********************                                                        */
/*! \file Test_max.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Derek Huang, Guy Katz, Duligur Ibeling	
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **

 **/

#include <cxxtest/TestSuite.h>

#include "Engine.h"
#include "FloatUtils.h"
#include "InputQuery.h"
#include "MaxConstraint.h"


class MaxTestSuite : public CxxTest::TestSuite
{
public:

    void setUp()
    {
    }

    void tearDown()
    {
    }

    void test_max_infeasible() 
    {
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

        Engine engine;
        if ( !engine.processInputQuery( inputQuery, false ) )
        {
            // got infeasible in the preprocessing
            TS_ASSERT( 1 );
        }
        else
        {
            bool result = engine.solve();
            TS_ASSERT( !result );
        }
    }

    void test_max_fesible() 
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

		MaxConstraint *max1 = new MaxConstraint( 5, Set<unsigned>( { 0, 2, 3 } ) );
		MaxConstraint *max2 = new MaxConstraint( 3, Set<unsigned>( { 0, 4 } ) );

		inputQuery.addPiecewiseLinearConstraint( max1 );
		inputQuery.addPiecewiseLinearConstraint( max2 );

        Engine engine;
        TS_ASSERT_THROWS_NOTHING( engine.processInputQuery( inputQuery, false ) )

        TS_ASSERT_THROWS_NOTHING ( engine.solve() );

        engine.extractSolution( inputQuery );

        // Sanity test

        double value_x0 = inputQuery.getSolutionValue( 0 );
        double value_x1b = inputQuery.getSolutionValue( 1 );
        double value_x1f = inputQuery.getSolutionValue( 2 );
        double value_x2b = inputQuery.getSolutionValue( 3 );
        double value_x2f = inputQuery.getSolutionValue( 4 );
        double value_x3 = inputQuery.getSolutionValue( 5 );


        TS_ASSERT( FloatUtils::areEqual( value_x0, value_x1b ) )

        TS_ASSERT( FloatUtils::areEqual( value_x0, -value_x2b ) )

        TS_ASSERT( FloatUtils::areEqual( value_x3, value_x1f + value_x2f ) )

        TS_ASSERT( !FloatUtils::lt( value_x0, 0 ) ) 
        TS_ASSERT( !FloatUtils::gt( value_x0, 1 ) ) 
        TS_ASSERT( !FloatUtils::lt( value_x1f, 0 ) )
        TS_ASSERT( !FloatUtils::lt( value_x3, 0.5 ) )
        TS_ASSERT( !FloatUtils::gt( value_x3, 1 ) )
        TS_ASSERT( !FloatUtils::lt( value_x2f, 0.0 ) )

        double x2_relu = FloatUtils::max( value_x2b, FloatUtils::max( value_x0, value_x2f ) );
       	TS_ASSERT( FloatUtils::areEqual( x2_relu, value_x2b ) )

        double x1_relu = FloatUtils::max( value_x1b, FloatUtils::max( value_x0, FloatUtils::max( value_x2b, value_x1f ) ) );
		TS_ASSERT ( FloatUtils::areEqual( x1_relu, value_x3 ) )

    }
};

//
// Local Variables:
// compile-command: "make -C ../../.. "
// tags-file-name: "../../../TAGS"
// c-basic-offset: 4
// End:
//
