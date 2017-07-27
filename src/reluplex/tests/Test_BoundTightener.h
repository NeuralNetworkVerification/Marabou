/*********************                                                        */
/*! \file Test_BoundTightener.h
** \verbatim
** Top contributors (to current version):
**   Duligur Ibeling
** This file is part of the Marabou project.
** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
** in the top-level source directory) and their institutional affiliations.
** All rights reserved. See the file COPYING in the top-level source
** directory for licensing information.\endverbatim
**/

#include <cxxtest/TestSuite.h>

#include "BoundTightener.h"

class MockForBoundTightener
{
public:
};

class BoundTightenerTestSuite : public CxxTest::TestSuite
{
public:
    MockForBoundTightener *mock;

    void setUp()
    {
        TS_ASSERT( mock = new MockForBoundTightener );

    }

    void tearDown()
    {
        TS_ASSERT_THROWS_NOTHING( delete mock );
    }

    void test_todo()
    {
        TS_TRACE( "Add tests for bound tightener" );
    }
};

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
