/*********************                                                        */
/*! \file Test_RowBoundTightener.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2024 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

**/

#include "Engine.h"
#include "FloatUtils.h"
#include "LinearExpression.h"
#include "Query.h"

#include <cxxtest/TestSuite.h>

class LpTestSuite : public CxxTest::TestSuite
{
public:
    void setUp()
    {
    }

    void tearDown()
    {
    }

    void test_optimize_for_heuristic_cost()
    {
        Query inputQuery;
        inputQuery.setNumberOfVariables( 4 );

        for ( unsigned i = 0; i < 4; ++i )
        {
            inputQuery.setLowerBound( i, 0 );
            inputQuery.setUpperBound( i, 1 );
        }

        Equation equation1( Equation::LE );
        equation1.addAddend( 1, 0 );
        equation1.addAddend( 1, 1 );
        equation1.setScalar( 0.5 );
        inputQuery.addEquation( equation1 );

        Equation equation2( Equation::GE );
        equation2.addAddend( 1, 1 );
        equation2.addAddend( 1, 2 );
        equation2.addAddend( 1, 3 );
        equation2.setScalar( 0.5 );
        inputQuery.addEquation( equation2 );

        Equation equation3( Equation::GE );
        equation3.addAddend( 1, 1 );
        equation3.addAddend( -1, 2 );
        equation3.setScalar( 0 );
        inputQuery.addEquation( equation3 );

        Equation equation4( Equation::GE );
        equation4.addAddend( 1, 1 );
        equation4.addAddend( -1, 3 );
        equation4.setScalar( 0 );
        inputQuery.addEquation( equation4 );

        Engine engine;
        TS_ASSERT_THROWS_NOTHING( engine.processInputQuery( inputQuery ) );
        TS_ASSERT_THROWS_NOTHING( engine.solve() );

        // Cost function: x0 - x1
        Map<unsigned, double> heuristicCost;
        heuristicCost[0] = 1;
        heuristicCost[1] = -1;
        LinearExpression cost = LinearExpression( heuristicCost );

        TS_ASSERT_THROWS_NOTHING( engine.minimizeHeuristicCost( cost ) );

        engine.extractSolution( inputQuery );

        TS_ASSERT( FloatUtils::areEqual( engine.computeHeuristicCost( heuristicCost ), -0.5 ) );

        heuristicCost.clear();
        heuristicCost[0] = -2;
        heuristicCost[1] = 1;
        heuristicCost[3] = 2;
        cost = LinearExpression( heuristicCost );

        TS_ASSERT_THROWS_NOTHING( engine.minimizeHeuristicCost( cost ) );

        engine.extractSolution( inputQuery );

        TS_ASSERT( FloatUtils::areEqual( engine.computeHeuristicCost( cost ), -0.25 ) );

        heuristicCost.clear();
        heuristicCost[1] = -2;
        heuristicCost[2] = 1;
        cost = LinearExpression( heuristicCost );
        cost._constant = -5;

        TS_ASSERT_THROWS_NOTHING( engine.minimizeHeuristicCost( cost ) );

        engine.extractSolution( inputQuery );

        TS_ASSERT( FloatUtils::areEqual( engine.computeHeuristicCost( cost ), -6 ) );
    }

    void test_fesiablbe()
    {
        Query inputQuery;
        inputQuery.setNumberOfVariables( 4 );

        inputQuery.setLowerBound( 0, 0 );
        inputQuery.setUpperBound( 0, 2 );

        inputQuery.setLowerBound( 1, -3 );
        inputQuery.setUpperBound( 1, 3 );

        inputQuery.setLowerBound( 2, 4 );
        inputQuery.setUpperBound( 2, 6 );

        inputQuery.setLowerBound( 3, 0 );

        Equation equation;
        equation.addAddend( 1, 0 );
        equation.addAddend( 2, 1 );
        equation.addAddend( -1, 2 );
        equation.addAddend( 1, 3 );
        equation.setScalar( 11 );
        inputQuery.addEquation( equation );


        Engine engine;
        TS_ASSERT_THROWS_NOTHING( engine.processInputQuery( inputQuery ) );
        TS_ASSERT_THROWS_NOTHING( engine.solve() );

        engine.extractSolution( inputQuery );


        // Sanity test
        double value = 0;

        double value0 = inputQuery.getSolutionValue( 0 );
        double value1 = inputQuery.getSolutionValue( 1 );
        double value2 = inputQuery.getSolutionValue( 2 );
        double value3 = inputQuery.getSolutionValue( 3 );

        value += 1 * value0;
        value += 2 * value1;
        value += -1 * value2;
        value += 1 * value3;

        TS_ASSERT_EQUALS( value, 11 );

        TS_ASSERT( value0 >= 0 );
        TS_ASSERT( value0 <= 2 );
        TS_ASSERT( value1 >= -3 );
        TS_ASSERT( value1 <= 3 );
        TS_ASSERT( value2 >= 4 );
        TS_ASSERT( value2 <= 6 );
        TS_ASSERT( value3 >= 0 );
    }

    void test_infesiable()
    {
        Query inputQuery;
        inputQuery.setNumberOfVariables( 7 );
        inputQuery.setLowerBound( 0, 0 );
        inputQuery.setUpperBound( 0, 1 );

        inputQuery.setLowerBound( 1, 0 );
        inputQuery.setUpperBound( 1, 1 );

        inputQuery.setLowerBound( 2, -1 );
        inputQuery.setUpperBound( 2, 0 );

        inputQuery.setLowerBound( 3, 0.5 );
        inputQuery.setUpperBound( 3, 1 );

        inputQuery.setLowerBound( 4, 0 );
        inputQuery.setUpperBound( 4, 0 );

        inputQuery.setLowerBound( 5, 0 );
        inputQuery.setUpperBound( 5, 0 );

        inputQuery.setLowerBound( 6, 0 );
        inputQuery.setUpperBound( 6, 0 );

        Equation equation1;
        equation1.addAddend( 1, 0 );
        equation1.addAddend( -1, 1 );
        equation1.addAddend( 1, 4 );
        equation1.setScalar( 0 );
        inputQuery.addEquation( equation1 );

        Equation equation2;
        equation2.addAddend( 1, 0 );
        equation2.addAddend( 1, 2 );
        equation2.addAddend( 1, 5 );
        equation2.setScalar( 0 );
        inputQuery.addEquation( equation2 );

        Equation equation3;
        equation3.addAddend( -1, 1 );
        equation3.addAddend( -1, 2 );
        equation3.addAddend( 1, 3 );
        equation3.addAddend( 1, 6 );
        equation3.setScalar( 0 );
        inputQuery.addEquation( equation3 );

        Engine engine;
        bool result = engine.processInputQuery( inputQuery );
        if ( !result )
        {
            // got UNSAT from the preprocessing
            TS_ASSERT( 1 );
        }
        else
        {
            result = engine.solve();
            TS_ASSERT( !result );
        }
    }
};

//
// Local Variables:
// compile-command: "make -C ../../.. "
// tags-file-name: "../../../TAGS"
// c-basic-offset: 4
// End:
//
