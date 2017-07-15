/*********************                                                        */
/*! \file Test_SteepestEdge.h
** \verbatim
** Top contributors (to current version):
**   Rachel Lim
** This file is part of the Marabou project.
** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
** in the top-level source directory) and their institutional affiliations.
** All rights reserved. See the file COPYING in the top-level source
** directory for licensing information.\endverbatim
**/

#include <cxxtest/TestSuite.h>

#include "SteepestEdge.h"
#include "MockTableau.h"
#include "ReluplexError.h"
#include "Tableau.h"

#include <string.h>

class MockForSteepestEdge
{
public:
};

class SteepestEdgeTestSuite : public CxxTest::TestSuite
{
public:
    MockForSteepestEdge *mock;
    MockTableau *tableau;

    void setUp()
    {
        TS_ASSERT( mock = new MockForSteepestEdge );
        TS_ASSERT( tableau = new MockTableau );

    }

    void tearDown()
    {
        TS_ASSERT_THROWS_NOTHING( delete tableau );
        TS_ASSERT_THROWS_NOTHING( delete mock );
    }

    void initializeTableauValues( Tableau &tableau )
    {
	// Copied from Test_Tableau.h
        /*
               | 3 2 1 2 1 0 0 | | x1 |   | 225 |
          Ax = | 1 1 1 1 0 1 0 | | x2 | = | 117 | = b
               | 4 3 3 4 0 0 1 | | x3 |   | 420 |
                                 | x4 |
                                 | x5 |
                                 | x6 |
                                 | x7 |

           x5 = 225 - 3x1 - 2x2 - x3  - 2x4
           x6 = 117 -  x1 -  x2 - x3  -  x4
           x7 = 420 - 4x1 - 3x2 - 3x3 - 4x4

        */

        tableau.setEntryValue( 0, 0, 3 );
        tableau.setEntryValue( 0, 1, 2 );
        tableau.setEntryValue( 0, 2, 1 );
        tableau.setEntryValue( 0, 3, 2 );
        tableau.setEntryValue( 0, 4, 1 );
        tableau.setEntryValue( 0, 5, 0 );
        tableau.setEntryValue( 0, 6, 0 );

        tableau.setEntryValue( 1, 0, 1 );
        tableau.setEntryValue( 1, 1, 1 );
        tableau.setEntryValue( 1, 2, 1 );
        tableau.setEntryValue( 1, 3, 1 );
        tableau.setEntryValue( 1, 4, 0 );
        tableau.setEntryValue( 1, 5, 1 );
        tableau.setEntryValue( 1, 6, 0 );

        tableau.setEntryValue( 2, 0, 4 );
        tableau.setEntryValue( 2, 1, 3 );
        tableau.setEntryValue( 2, 2, 3 );
        tableau.setEntryValue( 2, 3, 4 );
        tableau.setEntryValue( 2, 4, 0 );
        tableau.setEntryValue( 2, 5, 0 );
        tableau.setEntryValue( 2, 6, 1 );

        double b[3] = { 225, 117, 420 };
        tableau.setRightHandSide( b );
    }
	
    void testTableauInitGamma()
    {
	Tableau *tableau;

	TS_ASSERT( tableau = new Tableau );
	TS_ASSERT_THROWS_NOTHING( tableau->setDimensions( 3, 7 ) );
	TS_ASSERT_THROWS_NOTHING( tableau->useSteepestEdge( true ) );

	initializeTableauValues( *tableau );
	for ( unsigned i = 0; i < 7; ++i )
	{
	    // Doesn't matter
	    TS_ASSERT_THROWS_NOTHING( tableau->setLowerBound(i, 0) );
	    TS_ASSERT_THROWS_NOTHING( tableau->setUpperBound(i, 5) );
	}

	for ( unsigned i = 4; i < 7; ++i )
	{
	    TS_ASSERT_THROWS_NOTHING( tableau->markAsBasic( i ) );
	}

	TS_ASSERT_THROWS_NOTHING( tableau->initializeTableau() );

	const double *gamma = tableau->getSteepestEdgeGamma();

	// Expect: gamma = [27, 15, 12, 22]
	TS_ASSERT_EQUALS( gamma[0], 27.0 );
	TS_ASSERT_EQUALS( gamma[1], 15.0 );
	TS_ASSERT_EQUALS( gamma[2], 12.0 );
	TS_ASSERT_EQUALS( gamma[3], 22.0 );
    }
    
    void test_select()
    {/*
        SteepestEdge dantzigsRule;

        List<unsigned> candidates;
        tableau->mockCandidates = candidates;

	//  TS_ASSERT_THROWS_EQUALS( dantzigsRule.select( candidates, *tableau ),
        //                         const ReluplexError &e,
        //                         e.getCode(),
        //                         ReluplexError::NO_AVAILABLE_CANDIDATES );

        TS_ASSERT( !dantzigsRule.select( *tableau ) );

        tableau->setDimensions( 10, 100 );

        candidates.append( 2 );
        candidates.append( 3 );
        candidates.append( 10 );
        candidates.append( 51 );

        tableau->mockCandidates = candidates;

        tableau->nextCostFunction[2] = -5;
        tableau->nextCostFunction[3] = 7;
        tableau->nextCostFunction[10] = -15;
        tableau->nextCostFunction[51] = 12;

        // These guys are not candidates, so their coefficients don't matter
        tableau->nextCostFunction[14] = 102;
        tableau->nextCostFunction[25] = -1202;
        tableau->nextCostFunction[33] = 10;

        TS_ASSERT( dantzigsRule.select( *tableau ) );
        TS_ASSERT_EQUALS( tableau->getEnteringVariable(), 10U );

        candidates.append( 25 );

        tableau->mockCandidates = candidates;

        TS_ASSERT( dantzigsRule.select( *tableau ) );
        TS_ASSERT_EQUALS( tableau->getEnteringVariable(), 25U );
     */
    }

    void test_todo()
    {
	TS_TRACE( "Selects variables correctly." );
	TS_TRACE( "Updates gamma values correctly" );
	TS_TRACE( "Computes gradients correctly" );
    }
};

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
