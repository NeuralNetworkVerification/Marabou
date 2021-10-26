/*********************                                                        */
/*! \file Test_SigmoidConstraint.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Teruhiro Tagomori
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

**/

#include <cxxtest/TestSuite.h>

#include "InputQuery.h"
#include "MarabouError.h"
#include "SigmoidConstraint.h"
#include "MockTableau.h"
#include <string.h>

class MockForSigmoidConstraint
{
public:
};

/*
   Exposes protected members of SigmoidConstraint for testing.
 */
class TestSigmoidConstraint : public SigmoidConstraint
{
public:
    TestSigmoidConstraint( unsigned b, unsigned f  )
        : SigmoidConstraint( b, f )
    {}
};

class MaxConstraintTestSuite : public CxxTest::TestSuite
{
public:
    MockForSigmoidConstraint *mock;

    void setUp()
    {
        TS_ASSERT( mock = new MockForSigmoidConstraint );
    }

    void tearDown()
    {
        TS_ASSERT_THROWS_NOTHING( delete mock );
    }

    void test_sigmoid_constraint()
    {
        unsigned b = 1;
        unsigned f = 4;

        SigmoidConstraint sigmoid( b, f );

        List<unsigned> participatingVariables;
        TS_ASSERT_THROWS_NOTHING( participatingVariables = sigmoid.getParticipatingVariables() );
        TS_ASSERT_EQUALS( participatingVariables.size(), 2U );
        auto it = participatingVariables.begin();
        TS_ASSERT_EQUALS( *it, b );
        ++it;
        TS_ASSERT_EQUALS( *it, f );

        TS_ASSERT( sigmoid.participatingVariable( b ) );
        TS_ASSERT( sigmoid.participatingVariable( f ) );
        TS_ASSERT( !sigmoid.participatingVariable( 2 ) );
        TS_ASSERT( !sigmoid.participatingVariable( 0 ) );
        TS_ASSERT( !sigmoid.participatingVariable( 5 ) );
    }
};
