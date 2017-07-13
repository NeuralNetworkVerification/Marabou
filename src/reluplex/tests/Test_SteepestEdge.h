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

    void test_select()
    {
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

    }
};

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
