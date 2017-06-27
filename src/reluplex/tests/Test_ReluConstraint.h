/*********************                                                        */
/*! \file Test_ReluConstraint.h
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

#include "ReluConstraint.h"
#include "ReluplexError.h"

#include <cfloat>
#include <string.h>

class MockForReluConstraint
{
public:
};

class ReluConstraintTestSuite : public CxxTest::TestSuite
{
public:
    MockForReluConstraint *mock;

    void setUp()
    {
        TS_ASSERT( mock = new MockForReluConstraint );
    }

    void tearDown()
    {
        TS_ASSERT_THROWS_NOTHING( delete mock );
    }

    void test_relu_constraint()
    {
        unsigned b = 1;
        unsigned f = 4;

        ReluConstraint relu( b, f );

        TS_ASSERT( relu.participatingVariable( b ) );
        TS_ASSERT( relu.participatingVariable( f ) );
        TS_ASSERT( !relu.participatingVariable( 2 ) );
        TS_ASSERT( !relu.participatingVariable( 0 ) );
        TS_ASSERT( !relu.participatingVariable( 5 ) );

        Map<unsigned, double> assignment;
        TS_ASSERT_THROWS_EQUALS( relu.satisfied( assignment ),
                                 const ReluplexError &e,
                                 e.getCode(),
                                 ReluplexError::PARTICIPATING_VARIABLES_ABSENT );

        assignment[b] = 1;
        assignment[f] = 1;
        assignment[0] = -17;
        assignment[2] = 5.67;
        assignment[5] = 0;

        TS_ASSERT( relu.satisfied( assignment ) );

        assignment[f] = 2;

        TS_ASSERT( !relu.satisfied( assignment ) );

        assignment[b] = 2;

        TS_ASSERT( relu.satisfied( assignment ) );

        assignment[b] = -2;

        TS_ASSERT( !relu.satisfied( assignment ) );

        assignment[f] = 0;

        TS_ASSERT( relu.satisfied( assignment ) );

        assignment[b] = -3;

        TS_ASSERT( relu.satisfied( assignment ) );

        assignment[b] = 0;

        TS_ASSERT( relu.satisfied( assignment ) );
    }
};

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
