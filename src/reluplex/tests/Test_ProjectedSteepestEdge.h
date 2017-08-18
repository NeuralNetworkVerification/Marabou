/*********************                                                        */
/*! \file Test_ProjectedSteepestEdge.h
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

#include "ProjectedSteepestEdge.h"

class MockForProjectedSteepestEdge
{
public:
};

class ProjectedSteepestEdgeTestSuite : public CxxTest::TestSuite
{
public:
    MockForProjectedSteepestEdge *mock;

    void setUp()
    {
        TS_ASSERT( mock = new MockForProjectedSteepestEdge );
    }

    void tearDown()
    {
        TS_ASSERT_THROWS_NOTHING( delete mock );
    }

    void test_todo()
    {
        TS_TRACE( "TODO: Add unit tests" );
    }
};

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
