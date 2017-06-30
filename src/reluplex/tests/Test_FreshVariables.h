/*********************                                                        */
/*! \file Test_FreshVariables.h
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

#include "FreshVariables.h"

class MockForFreshVariables
{
public:
};

class FreshVariablesTestSuite : public CxxTest::TestSuite
{
public:
    MockForFreshVariables *mock;

    void setUp()
    {
        TS_ASSERT( mock = new MockForFreshVariables );
    }

    void tearDown()
    {
        TS_ASSERT_THROWS_NOTHING( delete mock );
    }

    void test_set_get()
    {
        TS_ASSERT_THROWS_NOTHING( FreshVariables::setNextVariable( 0U ) );

        TS_ASSERT_EQUALS( FreshVariables::getNextVariable(), 0U );
        TS_ASSERT_EQUALS( FreshVariables::getNextVariable(), 1U );
        TS_ASSERT_EQUALS( FreshVariables::getNextVariable(), 2U );

        TS_ASSERT_THROWS_NOTHING( FreshVariables::setNextVariable( 1U ) );

        TS_ASSERT_EQUALS( FreshVariables::getNextVariable(), 1U );
        TS_ASSERT_EQUALS( FreshVariables::getNextVariable(), 2U );
        TS_ASSERT_EQUALS( FreshVariables::getNextVariable(), 3U );

        TS_ASSERT_THROWS_NOTHING( FreshVariables::setNextVariable( 100U ) );

        TS_ASSERT_EQUALS( FreshVariables::getNextVariable(), 100U );
        TS_ASSERT_EQUALS( FreshVariables::getNextVariable(), 101U );
        TS_ASSERT_EQUALS( FreshVariables::getNextVariable(), 102U );
    }
};

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
