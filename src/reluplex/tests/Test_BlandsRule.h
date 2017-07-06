/*********************                                                        */
/*! \file Test_BlandsRule.h
** \verbatim
** Top contributors (to current version):
**   Guy Katz
** This file is part of the Marabou project.
** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
** in the top-level source directory) and their institutional affiliations.
** All rights reserved. See the file COPYING in the top-level source
** directory for licensing information.\endverbatim
**/

#include <cxxtest/TestSuite.h>

#include "BlandsRule.h"
#include "MockTableau.h"
#include "ReluplexError.h"

#include <string.h>

class MockForBlandsRule
{
public:
};

class BlandsRuleTestSuite : public CxxTest::TestSuite
{
public:
    MockForBlandsRule *mock;
    MockTableau *tableau;

    void setUp()
    {
        TS_ASSERT( mock = new MockForBlandsRule );
        TS_ASSERT( tableau = new MockTableau );

    }

    void tearDown()
    {
        TS_ASSERT_THROWS_NOTHING( delete tableau );
        TS_ASSERT_THROWS_NOTHING( delete mock );
    }

    void test_select()
    {
        BlandsRule blandsRule;

        List<unsigned> candidates;
        tableau->mockCandidates = candidates;

        // TS_ASSERT_THROWS_EQUALS( blandsRule.select( candidates, *tableau ),
        //                          const ReluplexError &e,
        //                          e.getCode(),
        //                          ReluplexError::NO_AVAILABLE_CANDIDATES );

        TS_ASSERT( !blandsRule.select(*tableau) );

        candidates.append( 3 );
        tableau->nextNonBasicIndexToVaribale[3] = 20;

        candidates.append( 10 );
        tableau->nextNonBasicIndexToVaribale[10] = 4;

        candidates.append( 2 );
        tableau->nextNonBasicIndexToVaribale[2] = 10;

        candidates.append( 51 );
        tableau->nextNonBasicIndexToVaribale[51] = 6;

        tableau->mockCandidates = candidates;

        // TS_ASSERT_EQUALS( blandsRule.select( candidates, *tableau ), 10U );

        TS_ASSERT( blandsRule.select( *tableau ) );
        TS_ASSERT_EQUALS( tableau->getEnteringVariable(), 10U );

        candidates.append( 100 );
        tableau->nextNonBasicIndexToVaribale[100] = 1;

        tableau->mockCandidates = candidates;

        // TS_ASSERT_EQUALS( blandsRule.select( candidates, *tableau ), 100U );

        TS_ASSERT( blandsRule.select( *tableau ) );
        TS_ASSERT_EQUALS( tableau->getEnteringVariable(), 100U );

    }
};

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
