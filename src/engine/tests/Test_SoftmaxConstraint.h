/*********************                                                        */
/*! \file Test_SoftmaxConstraint.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Haoze Wu
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2024 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

**/

#include "FloatUtils.h"
#include "MarabouError.h"
#include "MockTableau.h"
#include "Query.h"
#include "SoftmaxConstraint.h"
#include "Tightening.h"

#include <cxxtest/TestSuite.h>
#include <string.h>

class MockForSigmoidConstraint
{
public:
};


class MaxConstraintTestSuite : public CxxTest::TestSuite
{
public:
    void setUp()
    {
    }

    void tearDown()
    {
    }

    void test_evaluation()
    {
        Vector<double> inputs = { 1, 1, 1, 1 };
        Vector<double> outputs;
        SoftmaxConstraint::softmax( inputs, outputs );
        for ( const auto &output : outputs )
            TS_ASSERT( FloatUtils::areEqual( output, 0.25 ) );


        TS_ASSERT(
            FloatUtils::areEqual( SoftmaxConstraint::sumOfExponential( inputs ), 10.8731273138 ) );
    }

    void test_tighten_bounds()
    {
        Vector<unsigned> inputs = { 0, 1, 2 };
        Vector<unsigned> outputs = { 3, 4, 5 };
        SoftmaxConstraint softmax( inputs, outputs );

        softmax.notifyLowerBound( 0, -1 );
        softmax.notifyLowerBound( 1, -1 );
        softmax.notifyLowerBound( 2, 1 );
        softmax.notifyUpperBound( 0, 0 );
        softmax.notifyUpperBound( 1, 0 );

        List<Tightening> t;
        TS_ASSERT_THROWS_NOTHING( softmax.getEntailedTightenings( t ) );
        TS_ASSERT_EQUALS( t.size(), ( 5u + 6u ) );

        t.clear();
        softmax.notifyUpperBound( 2, 2 );
        TS_ASSERT_THROWS_NOTHING( softmax.getEntailedTightenings( t ) );
        TS_ASSERT_EQUALS( t.size(), ( 6u + 6u + 6u ) );

        TS_ASSERT( containsTightening( t, Tightening( 3, 0.0420, Tightening::LB ) ) );
        TS_ASSERT( containsTightening( t, Tightening( 3, 0.2447, Tightening::UB ) ) );
        TS_ASSERT( containsTightening( t, Tightening( 4, 0.0420, Tightening::LB ) ) );
        TS_ASSERT( containsTightening( t, Tightening( 4, 0.2447, Tightening::UB ) ) );
        TS_ASSERT( containsTightening( t, Tightening( 5, 0.5761, Tightening::LB ) ) );
        TS_ASSERT( containsTightening( t, Tightening( 5, 0.9094, Tightening::UB ) ) );
    }

    bool containsTightening( const List<Tightening> &ts, Tightening t )
    {
        for ( const auto &ts_ : ts )
        {
            if ( ts_._variable == t._variable && ts_._type == t._type &&
                 FloatUtils::areEqual( ts_._value, t._value, 0.001 ) )
                return true;
        }
        return false;
    }

    void test_softmax_satisfied()
    {
        Vector<unsigned> inputs = { 0, 1, 2 };
        Vector<unsigned> outputs = { 3, 4, 5 };
        SoftmaxConstraint softmax( inputs, outputs );

        MockTableau tableau;
        softmax.registerTableau( &tableau );
        tableau.setValue( 0, 0.5 );
        tableau.setValue( 1, 0.5 );
        tableau.setValue( 2, 0.5 );
        tableau.setValue( 3, 1.0 / 3 );
        tableau.setValue( 4, 1.0 / 3 );
        tableau.setValue( 5, 1.0 / 3 );
        TS_ASSERT( softmax.satisfied() );

        tableau.setValue( 3, 0.4 );
        TS_ASSERT( !softmax.satisfied() );
    }
};
