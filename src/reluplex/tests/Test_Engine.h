/*********************                                                        */
/*! \file Test_Engine.h
** \verbatim
** Top contributors (to current version):
**   Guy Katz
** This file is part of the Reluplex project.
** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
** in the top-level source directory) and their institutional affiliations.
** All rights reserved. See the file COPYING in the top-level source
** directory for licensing information.\endverbatim
**/

#include <cxxtest/TestSuite.h>

#include "Engine.h"
#include "MockErrno.h"
#include "MockTableauFactory.h"

#include <string.h>

class MockForEngine :
    public MockTableauFactory
{
public:
};

class EngineTestSuite : public CxxTest::TestSuite
{
public:
    MockForEngine *mock;
    MockTableau *tableau;

    void setUp()
    {
        TS_ASSERT( mock = new MockForEngine );

        tableau = &( mock->mockTableau );
    }

    void tearDown()
    {
        TS_ASSERT_THROWS_NOTHING( delete mock );
    }

    void test_constructor_destructor()
    {
        Engine *engine;

        TS_ASSERT_THROWS_NOTHING( engine = new Engine() );

        TS_ASSERT( tableau->wasCreated );

        TS_ASSERT_THROWS_NOTHING( delete engine );

        TS_ASSERT( tableau->wasDiscarded );
    }
};

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
