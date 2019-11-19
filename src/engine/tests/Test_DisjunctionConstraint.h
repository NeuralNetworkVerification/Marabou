/*********************                                                        */
/*! \file Test_DisjunctionConstraint.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz, Parth Shah, Duligur Ibeling
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

**/

#include <cxxtest/TestSuite.h>

#include "DisjunctionConstraint.h"
#include "MarabouError.h"
#include "MockErrno.h"

class MockForDisjunctionConstraint
    : public MockErrno
{
public:
};

class DisjunctionConstraintTestSuite : public CxxTest::TestSuite
{
public:
    MockForDisjunctionConstraint *mock;

    void setUp()
    {
        TS_ASSERT( mock = new MockForDisjunctionConstraint );
    }

    void tearDown()
    {
        TS_ASSERT_THROWS_NOTHING( delete mock );
    }

    void test_dummy()
    {
        TS_ASSERT( 1 );
    }
};

//
// Local Variables:
// compile-command: "make -C ../../.. "
// tags-file-name: "../../../TAGS"
// c-basic-offset: 4
// End:
//
