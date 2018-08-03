/*********************                                                        */
/*! \file Test_DantzigsRule.h
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

#include "DantzigsRule.h"
#include "MockTableau.h"
#include "ReluplexError.h"

#include <string.h>

class MockForDantzigsRule
{
public:
};

class DantzigsRuleTestSuite : public CxxTest::TestSuite
{
public:
    MockForDantzigsRule *mock;
    MockTableau *tableau;

    void setUp()
    {
        TS_ASSERT( mock = new MockForDantzigsRule );
        TS_ASSERT( tableau = new MockTableau );

    }

    void tearDown()
    {
        TS_ASSERT_THROWS_NOTHING( delete tableau );
        TS_ASSERT_THROWS_NOTHING( delete mock );
    }

    void test_select()
    {
        DantzigsRule dantzigsRule;

        Set<unsigned> excluded;
        List<unsigned> candidates;

        TS_ASSERT( !dantzigsRule.select( *tableau, candidates, excluded ) );

        tableau->setDimensions( 10, 100 );

        candidates.append( 2 );
        candidates.append( 3 );
        candidates.append( 10 );
        candidates.append( 51 );

        tableau->nextCostFunction[2] = -5;
        tableau->nextCostFunction[3] = 7;
        tableau->nextCostFunction[10] = -15;
        tableau->nextCostFunction[51] = 12;

        // These guys are not candidates, so their coefficients don't matter
        tableau->nextCostFunction[14] = 102;
        tableau->nextCostFunction[25] = -1202;
        tableau->nextCostFunction[33] = 10;

        TS_ASSERT( dantzigsRule.select( *tableau, candidates, excluded ) );
        TS_ASSERT_EQUALS( tableau->mockEnteringVariable, 10U );

        excluded.insert( 10 );
        TS_ASSERT( dantzigsRule.select( *tableau, candidates, excluded ) );
        TS_ASSERT_EQUALS( tableau->mockEnteringVariable, 51U );
        excluded.clear();

        candidates.append( 25 );

        TS_ASSERT( dantzigsRule.select( *tableau, candidates, excluded ) );
        TS_ASSERT_EQUALS( tableau->mockEnteringVariable, 25U );
    }
};

//
// Local Variables:
// compile-command: "make -C ../../.. "
// tags-file-name: "../../../TAGS"
// c-basic-offset: 4
// End:
//
