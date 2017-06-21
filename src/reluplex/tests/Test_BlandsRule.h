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

    void setUp()
    {
        TS_ASSERT( mock = new MockForBlandsRule );
    }

    void tearDown()
    {
        TS_ASSERT_THROWS_NOTHING( delete mock );
    }

    void test_select()
    {
        BlandsRule blandsRule;

        List<unsigned> candidates;

        TS_ASSERT_THROWS_EQUALS( blandsRule.select( candidates ),
                                 const ReluplexError &e,
                                 e.getCode(),
                                 ReluplexError::NO_AVAILABLE_CANDIDATES );

        candidates.append( 3 );
        candidates.append( 10 );
        candidates.append( 2 );
        candidates.append( 51 );

        TS_ASSERT_EQUALS( blandsRule.select( candidates ), 2U );

        candidates.append( 1 );

        TS_ASSERT_EQUALS( blandsRule.select( candidates ), 1U );
    }
};

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
