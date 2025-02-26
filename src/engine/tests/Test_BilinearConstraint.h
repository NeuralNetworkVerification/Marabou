/*********************                                                        */
/*! \file Test_BilinearConstraint.h
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

#include "BilinearConstraint.h"
#include "FloatUtils.h"
#include "MarabouError.h"
#include "MockTableau.h"
#include "Query.h"
#include "Tightening.h"

#include <cxxtest/TestSuite.h>
#include <string.h>

class BilinearConstraintTestSuite : public CxxTest::TestSuite
{
public:
    void setUp()
    {
    }

    void tearDown()
    {
    }

    void test_construction()
    {
        BilinearConstraint bilinear( 0, 1, 2 );

        auto bs = bilinear.getBs();
        TS_ASSERT_EQUALS( bs[0], 0u );
        TS_ASSERT_EQUALS( bs[1], 1u );
        TS_ASSERT_EQUALS( bilinear.getF(), 2u );

        bilinear.updateVariableIndex( 0, 4 );
        bs = bilinear.getBs();
        TS_ASSERT_EQUALS( bs[0], 4u );
        TS_ASSERT_EQUALS( bs[1], 1u );
        bilinear.updateVariableIndex( 2, 3 );
        TS_ASSERT_EQUALS( bilinear.getF(), 3u );
    }

    void test_tighten_bounds1()
    {
        BilinearConstraint bilinear( 0, 1, 2 );

        bilinear.notifyLowerBound( 0, -2 );
        bilinear.notifyUpperBound( 0, 1 );
        bilinear.notifyLowerBound( 1, -1 );
        bilinear.notifyUpperBound( 1, 2 );

        List<Tightening> t;
        TS_ASSERT_THROWS_NOTHING( bilinear.getEntailedTightenings( t ) );
        TS_ASSERT_EQUALS( t.size(), 2u );

        TS_ASSERT( containsTightening( t, Tightening( 2, -4, Tightening::LB ) ) );
        TS_ASSERT( containsTightening( t, Tightening( 2, 2, Tightening::UB ) ) );
    }

    void test_tighten_bounds2()
    {
        BilinearConstraint bilinear( 0, 1, 2 );

        bilinear.notifyLowerBound( 0, 1 );
        bilinear.notifyUpperBound( 0, 2 );
        bilinear.notifyLowerBound( 1, -1 );
        bilinear.notifyUpperBound( 1, 2 );

        List<Tightening> t;
        TS_ASSERT_THROWS_NOTHING( bilinear.getEntailedTightenings( t ) );
        TS_ASSERT_EQUALS( t.size(), 2u );

        TS_ASSERT( containsTightening( t, Tightening( 2, -2, Tightening::LB ) ) );
        TS_ASSERT( containsTightening( t, Tightening( 2, 4, Tightening::UB ) ) );
    }

    void test_tighten_bounds3()
    {
        BilinearConstraint bilinear( 0, 1, 2 );

        bilinear.notifyLowerBound( 0, -2 );
        bilinear.notifyUpperBound( 0, -1 );
        bilinear.notifyLowerBound( 1, -3 );
        bilinear.notifyUpperBound( 1, -2 );

        List<Tightening> t;
        TS_ASSERT_THROWS_NOTHING( bilinear.getEntailedTightenings( t ) );
        TS_ASSERT_EQUALS( t.size(), 2u );

        TS_ASSERT( containsTightening( t, Tightening( 2, 2, Tightening::LB ) ) );
        TS_ASSERT( containsTightening( t, Tightening( 2, 6, Tightening::UB ) ) );
    }

    void test_tighten_bounds4()
    {
        BilinearConstraint bilinear( 0, 1, 2 );

        bilinear.notifyLowerBound( 0, -2 );
        bilinear.notifyUpperBound( 0, -1 );
        bilinear.notifyLowerBound( 1, 2 );
        bilinear.notifyUpperBound( 1, 4 );

        List<Tightening> t;
        TS_ASSERT_THROWS_NOTHING( bilinear.getEntailedTightenings( t ) );
        TS_ASSERT_EQUALS( t.size(), 2u );

        TS_ASSERT( containsTightening( t, Tightening( 2, -8, Tightening::LB ) ) );
        TS_ASSERT( containsTightening( t, Tightening( 2, -2, Tightening::UB ) ) );
    }

    void test_tighten_bounds5()
    {
        BilinearConstraint bilinear( 0, 1, 2 );

        bilinear.notifyLowerBound( 0, 1 );
        bilinear.notifyUpperBound( 0, 2 );
        bilinear.notifyLowerBound( 1, 2 );
        bilinear.notifyUpperBound( 1, 4 );

        List<Tightening> t;
        TS_ASSERT_THROWS_NOTHING( bilinear.getEntailedTightenings( t ) );
        TS_ASSERT_EQUALS( t.size(), 2u );

        TS_ASSERT( containsTightening( t, Tightening( 2, 2, Tightening::LB ) ) );
        TS_ASSERT( containsTightening( t, Tightening( 2, 8, Tightening::UB ) ) );
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

    void test_bilinear_satisfied()
    {
        unsigned b1 = 0;
        unsigned b2 = 1;
        unsigned f = 2;

        BilinearConstraint bilinear( b1, b2, f );
        MockTableau tableau;
        bilinear.registerTableau( &tableau );
        tableau.setValue( b1, 0.5 );
        tableau.setValue( b2, -1.0 );
        tableau.setValue( f, -0.5 );
        TS_ASSERT( bilinear.satisfied() );

        tableau.setValue( b2, -0.5 );
        tableau.setValue( f, 0 );
        TS_ASSERT( !bilinear.satisfied() );
    }
};
