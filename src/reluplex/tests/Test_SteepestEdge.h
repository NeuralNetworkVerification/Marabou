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

#include "FloatUtils.h"
#include "MockTableau.h"
#include "ReluplexError.h"
#include "SteepestEdge.h"
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

    Tableau *newSteepestEdgeTableau()
    {
        Tableau *tableau;

        TS_ASSERT( tableau = new Tableau );
        TS_ASSERT_THROWS_NOTHING( tableau->setDimensions( 3, 7 ) );
        TS_ASSERT_THROWS_NOTHING( tableau->useSteepestEdge( true ) );

        initializeTableauValues( *tableau );
        for ( unsigned i = 0; i < 7; ++i )
        {
            // Doesn't matter
            TS_ASSERT_THROWS_NOTHING( tableau->setLowerBound( i, 0 ) );
            TS_ASSERT_THROWS_NOTHING( tableau->setUpperBound( i, 5 ) );
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

        return tableau;
    }

    void testTableauUpdateGamma()
    {
        /*
          Original:
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
        // Try diff combinations of entering and leaving variable and make sure gamma checks out

        // (1) 0, 0
        Tableau *tableau = newSteepestEdgeTableau();
        tableau->setEnteringVariable( 0 ); // x1
        tableau->setLeavingVariable( 0 ); // x5

        /*
          New equations:
          x1 = 75  - 1/3 x5 - 2/3 x2 - 1/3 x3 - 2/3 x4
          x6 = 42  + 1/3 x5 - 1/3 x2 - 2/3 x3 - 1/3 x4
          x7 = 120 + 4/3 x5 - 1/3 x2 - 5/3 x3 - 4/3 x4

               | 1/3  2/3  1/3  2/3  1 0 0 | | x5 |   | 75  |
          Ax = | -1/3 1/3  2/3  1/3  0 1 0 | | x2 | = | 42  | = b
               | -4/3 1/3  5/3  4/3  0 0 1 | | x3 |   | 420 |
                                             | x4 |
                                             | x1 |
                                             | x6 |
                                             | x7 |
        */

        tableau->updateGamma();
        const double *gamma = tableau->getSteepestEdgeGamma();

        TS_ASSERT( FloatUtils::areEqual( gamma[0], 3.0 ) );
        TS_ASSERT( FloatUtils::areEqual( gamma[1], 5.0/3 ) );
        TS_ASSERT( FloatUtils::areEqual( gamma[2], 13.0/3 ) );
        TS_ASSERT( FloatUtils::areEqual( gamma[3], 10.0/3 ) );

        // (2) 0, 1
        tableau = newSteepestEdgeTableau();
        tableau->setEnteringVariable(0); // x1
        tableau->setLeavingVariable(1); // x6

        /*
          New equations:
          x5 = -126 + 3x6 + x2 + 2x3 + x4
          x1 = 117  -  x6 - x2 -  x3 - x4
          x7 = -48  + 4x6 + x2 + x3

               | -3 -1 -2 -1 1 0 0 | | x6 |   | -126 |
          Ax = |  1  1  1  1 0 1 0 | | x2 | = | 117  | = b
               | -4 -1 -1  0 0 0 1 | | x3 |   | -48  |
                                     | x4 |
                                     | x5 |
                                     | x1 |
                                     | x7 |
        */

        tableau->updateGamma();
        gamma = tableau->getSteepestEdgeGamma();
        // Tests are failing bc floating point comparison is imprecise
        TS_ASSERT_EQUALS( gamma[0], 27.0 );
        TS_ASSERT_EQUALS( gamma[1], 4.0 );
        TS_ASSERT_EQUALS( gamma[2], 7.0 );
        TS_ASSERT_EQUALS( gamma[3], 3.0 );
        /*
          TS_ASSERT( FloatUtils::areEqual( gamma[0], 3.0 ) );
          TS_ASSERT( FloatUtils::areEqual( gamma[1], 5.0/3 ) );
          TS_ASSERT( FloatUtils::areEqual( gamma[2], 13.0/3 ) );
          TS_ASSERT( FloatUtils::areEqual( gamma[3], 10.0/3 ) );
        */
    }

    void test_select()
    {
        SteepestEdgeRule steepestEdgeRule;

        List<unsigned> candidates;
        tableau->mockCandidates = candidates;

        //  TS_ASSERT_THROWS_EQUALS( dantzigsRule.select( candidates, *tableau ),
        //                         const ReluplexError &e,
        //                         e.getCode(),
        //                         ReluplexError::NO_AVAILABLE_CANDIDATES );

        TS_ASSERT( !steepestEdgeRule.select( *tableau ) );
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

        tableau->steepestEdgeGamma[2] = 5.0;
        tableau->steepestEdgeGamma[3] = 2.0;
        tableau->steepestEdgeGamma[51] = 5.0;

        // These guys are not candidates, so their coefficients don't matter
        tableau->nextCostFunction[14] = 102;
        tableau->nextCostFunction[25] = -1202;
        tableau->nextCostFunction[33] = 10;

        TS_ASSERT( steepestEdgeRule.select( *tableau ) );
        TS_ASSERT_EQUALS( tableau->getEnteringVariable(), 10U );

        tableau->steepestEdgeGamma[51] = 0.5;

        TS_ASSERT( steepestEdgeRule.select( *tableau ) );
        TS_ASSERT_EQUALS( tableau->getEnteringVariable(), 51U );

        candidates.append( 25 );

        tableau->mockCandidates = candidates;

        TS_ASSERT( steepestEdgeRule.select( *tableau ) );
        TS_ASSERT_EQUALS( tableau->getEnteringVariable(), 25U );
    }
};

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
