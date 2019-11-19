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
    PiecewiseLinearCaseSplit *cs1;
    PiecewiseLinearCaseSplit *cs2;
    PiecewiseLinearCaseSplit *cs3;

    void setUp()
    {
        TS_ASSERT( mock = new MockForDisjunctionConstraint );

        TS_ASSERT( cs1 = new PiecewiseLinearCaseSplit );
        TS_ASSERT( cs2 = new PiecewiseLinearCaseSplit );
        TS_ASSERT( cs3 = new PiecewiseLinearCaseSplit );

        // x0 <= 1, x1 = 2
        cs1->storeBoundTightening( Tightening( 0, 1, Tightening::UB ) );
        Equation eq1;
        eq1.addAddend( 1, 1 );
        eq1.setScalar( 2 );
        cs1->addEquation( eq1 );

        // 1 <= x0 <= 5, x1 = x0
        cs2->storeBoundTightening( Tightening( 0, 1, Tightening::LB ) );
        cs2->storeBoundTightening( Tightening( 0, 5, Tightening::UB ) );
        Equation eq2;
        eq2.addAddend( 1, 0 );
        eq2.addAddend( -1, 1 );
        eq2.setScalar( 0 );
        cs2->addEquation( eq2 );

        // 5 <= x0 , x1 = 2x2 + 5
        cs3->storeBoundTightening( Tightening( 0, 5, Tightening::LB ) );
        Equation eq3;
        eq3.addAddend( 1, 1 );
        eq3.addAddend( -2, 2 );
        eq3.setScalar( 5 );
        cs3->addEquation( eq3 );
    }

    void tearDown()
    {
        TS_ASSERT_THROWS_NOTHING( delete cs3 );
        TS_ASSERT_THROWS_NOTHING( delete cs2 );
        TS_ASSERT_THROWS_NOTHING( delete cs1 );

        TS_ASSERT_THROWS_NOTHING( delete mock );
    }

    void test_get_case_splits()
    {
        List<PiecewiseLinearCaseSplit> caseSplits = { *cs1, *cs2, *cs3 };
        DisjunctionConstraint dc( caseSplits );

        List<PiecewiseLinearCaseSplit> returnedSplits;
        TS_ASSERT_THROWS_NOTHING( returnedSplits = dc.getCaseSplits() );

        TS_ASSERT_EQUALS( returnedSplits.size(), caseSplits.size() );
        TS_ASSERT_EQUALS( returnedSplits, caseSplits );
    }

    void test_getParticipatingVariables()
    {
        List<PiecewiseLinearCaseSplit> caseSplits = { *cs1, *cs2, *cs3 };
        DisjunctionConstraint dc( caseSplits );

        List<unsigned> varibales = dc.getParticipatingVariables();

        TS_ASSERT_EQUALS( varibales.size(), 3U );
        TS_ASSERT( varibales.exists( 0 ) );
        TS_ASSERT( varibales.exists( 1 ) );
        TS_ASSERT( varibales.exists( 2 ) );

        TS_ASSERT( dc.participatingVariable( 0 ) );
        TS_ASSERT( dc.participatingVariable( 1 ) );
        TS_ASSERT( dc.participatingVariable( 2 ) );
        TS_ASSERT( !dc.participatingVariable( 3 ) );

        PiecewiseLinearCaseSplit cs4;
        cs4.storeBoundTightening( Tightening( 5, 17, Tightening::LB ) );
        caseSplits = { cs4 };

        DisjunctionConstraint dc2( caseSplits );
        varibales = dc2.getParticipatingVariables();

        TS_ASSERT_EQUALS( varibales.size(), 1U );
        TS_ASSERT( varibales.exists( 5 ) );

        TS_ASSERT( !dc2.participatingVariable( 0 ) );
        TS_ASSERT( !dc2.participatingVariable( 1 ) );
        TS_ASSERT( dc2.participatingVariable( 5 ) );
        TS_ASSERT( !dc2.participatingVariable( 17 ) );
    }

    void test_satisfied()
    {
        List<PiecewiseLinearCaseSplit> caseSplits = { *cs1, *cs2, *cs3 };
        DisjunctionConstraint dc( caseSplits );

        /*
          x0 <= 1       -->   x1 = 2
          1 <= x0 <= 5  -->   x1 = x0
          5 <= x0       -->   x1 = 2x2 + 5
        */

        dc.notifyVariableValue( 0, -5 );
        dc.notifyVariableValue( 1, 2 );
        TS_ASSERT( dc.satisfied() );

        dc.notifyVariableValue( 0, -3 );
        TS_ASSERT( dc.satisfied() );

        dc.notifyVariableValue( 12, 4 );
        TS_ASSERT( dc.satisfied() );

        dc.notifyVariableValue( 0, 3 );
        TS_ASSERT( !dc.satisfied() );

        dc.notifyVariableValue( 1, 3 );
        TS_ASSERT( dc.satisfied() );

        dc.notifyVariableValue( 2, 4 );
        TS_ASSERT( dc.satisfied() );

        dc.notifyVariableValue( 1, 2 );
        TS_ASSERT( !dc.satisfied() );

        dc.notifyVariableValue( 0, 7 );
        dc.notifyVariableValue( 1, 7 );
        TS_ASSERT( !dc.satisfied() );

        dc.notifyVariableValue( 2, 1 );
        TS_ASSERT( dc.satisfied() );

        dc.notifyVariableValue( 0, 15 );
        TS_ASSERT( dc.satisfied() );

        dc.notifyVariableValue( 1, 8 );
        TS_ASSERT( !dc.satisfied() );
    }
};

//
// Local Variables:
// compile-command: "make -C ../../.. "
// tags-file-name: "../../../TAGS"
// c-basic-offset: 4
// End:
//
