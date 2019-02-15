/*********************                                                        */
/*! \file Test_ConstraintBoundTightener.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

**/

#include <cxxtest/TestSuite.h>

#include "MockTableau.h"
#include "ConstraintBoundTightener.h"
#include "FactTracker.h"
#include "ReluConstraint.h"

class MockForConstraintBoundTightener
{
public:
};

class ConstraintBoundTightenerTestSuite : public CxxTest::TestSuite
{
public:
    MockForConstraintBoundTightener *mock;
    MockTableau *tableau;

    void setUp()
    {
        TS_ASSERT( mock = new MockForConstraintBoundTightener );
        TS_ASSERT( tableau = new MockTableau );
    }

    void tearDown()
    {
        TS_ASSERT_THROWS_NOTHING( delete tableau );
        TS_ASSERT_THROWS_NOTHING( delete mock );
    }

    void test_bounds_tightened()
    {
        ConstraintBoundTightener tightener( *tableau );

        tableau->setDimensions( 2, 5 );

        // Current bounds:
        //  0 <= x0 <= 0
        //    5  <= x1 <= 10
        //    -2 <= x2 <= 3
        //  -100 <= x4 <= 100
        tableau->setLowerBound( 0, -200 );
        tableau->setUpperBound( 0, 200 );
        tableau->setLowerBound( 1, 5 );
        tableau->setUpperBound( 1, 10 );
        tableau->setLowerBound( 2, -2 );
        tableau->setUpperBound( 2, 3 );
        tableau->setLowerBound( 3, -5 );
        tableau->setUpperBound( 3, 5 );
        tableau->setLowerBound( 4, -100 );
        tableau->setUpperBound( 4, 100 );

        tightener.setDimensions();

        TS_ASSERT_THROWS_NOTHING( tightener.registerTighterLowerBound( 1, 7, NULL ) );
        TS_ASSERT_THROWS_NOTHING( tightener.registerTighterUpperBound( 3, 2, NULL ) );

        List<Tightening> tightenings;
        TS_ASSERT_THROWS_NOTHING( tightener.getConstraintTightenings( tightenings ) );

        // Lower and upper bounds should have been tightened
        TS_ASSERT_EQUALS( tightenings.size(), 2U );

        auto lower = tightenings.begin();
        while ( ( lower != tightenings.end() ) && !( ( lower->_variable == 1 ) && ( lower->_type == Tightening::LB ) ) )
            ++lower;
        TS_ASSERT( lower != tightenings.end() );

        auto upper = tightenings.begin();
        while ( ( upper != tightenings.end() ) && !( ( upper->_variable == 3 ) && ( upper->_type == Tightening::UB ) ) )
            ++upper;
        TS_ASSERT( upper != tightenings.end() );

        TS_ASSERT_EQUALS( lower->_value, 7 );
        TS_ASSERT_EQUALS( upper->_value, 2 );

        TS_ASSERT_THROWS_NOTHING( tightener.notifyLowerBound( 1, 8 ) );

        tightenings.clear();

        TS_ASSERT_THROWS_NOTHING( tightener.getConstraintTightenings( tightenings ) );
        TS_ASSERT_EQUALS( tightenings.size(), 1U );

        upper = tightenings.begin();

        TS_ASSERT_EQUALS( upper->_type, Tightening::UB );
        TS_ASSERT_EQUALS( upper->_variable, 3U );
        TS_ASSERT_EQUALS( upper->_value, 2 );
    }

    void test_explain_tightening_from_relu()
    {
        ConstraintBoundTightener cbt( *tableau );

        FactTracker* factTracker = new FactTracker();
        cbt.setFactTracker( factTracker );

        tableau->setDimensions( 2, 2 );

        tableau->setLowerBound( 0, -200 );
        tableau->setUpperBound( 0, 200 );
        tableau->setLowerBound( 1, -5 );
        tableau->setUpperBound( 1, 10 );

        Tightening x0_lb( 0, -200, Tightening::LB );
        Tightening x0_ub( 0, 200, Tightening::UB );
        Tightening x1_lb( 1, -5, Tightening::LB );
        Tightening x1_ub( 1, 10, Tightening::UB );

        x0_lb.setCausingSplitInfo( 0, 0, 0 );
        x0_ub.setCausingSplitInfo( 0, 0, 0 );
        x1_lb.setCausingSplitInfo( 0, 0, 0 );
        x1_ub.setCausingSplitInfo( 0, 0, 0 );

        factTracker->addBoundFact( 0, x0_lb );
        factTracker->addBoundFact( 0, x0_ub );
        factTracker->addBoundFact( 1, x1_lb );
        factTracker->addBoundFact( 1, x1_ub );

        const Fact* fact_x1_lb = factTracker->getFactAffectingBound( 1, FactTracker::LB );
        const Fact* fact_x1_ub = factTracker->getFactAffectingBound( 1, FactTracker::UB );
        cbt.setDimensions();

        unsigned fn = 0, bn = 1;
        ReluConstraint relu( fn, bn, 1 );
        relu.setFactTracker( factTracker );
        relu.registerConstraintBoundTightener( &cbt );
        relu.notifyLowerBound( fn, -200 );
        relu.notifyUpperBound( fn, 200 );
        relu.notifyLowerBound( bn, -5 );
        relu.notifyUpperBound( bn, 10 );

        // expect UB tightening for fn and LB tightening for bn
        List<Tightening> tightenings;
        cbt.getConstraintTightenings( tightenings );
        TS_ASSERT_EQUALS( tightenings.size(), 2U );
        Tightening bound = tightenings.back();
        TS_ASSERT_EQUALS( bound._variable, bn );
        TS_ASSERT_EQUALS( bound._value, 0 );
        TS_ASSERT_EQUALS( bound._type, Tightening::LB );
        List<const Fact*> explanations = bound.getExplanations();
        TS_ASSERT_EQUALS( explanations.size(), 1U );
        TS_ASSERT_EQUALS( explanations.front(), fact_x1_lb );
        bound = tightenings.front();
        TS_ASSERT_EQUALS( bound._variable, fn );
        TS_ASSERT_EQUALS( bound._value, 10 );
        TS_ASSERT_EQUALS( bound._type, Tightening::UB );
        explanations = bound.getExplanations();
        TS_ASSERT_EQUALS( explanations.size(), 1U );
        TS_ASSERT_EQUALS( explanations.front(), fact_x1_ub );

        // tighten LB of bn, and relu is required to be active
        Tightening x1_new_lb( 1, 5, Tightening::LB );
        x1_new_lb.setCausingSplitInfo(0, 0, 0);
        factTracker->addBoundFact( 1, x1_new_lb );
        const Fact* fact_x1_new_lb = factTracker->getFactAffectingBound( 1, FactTracker::LB );
        relu.notifyLowerBound( bn, 5 );

        // expect LB tightening for fn
        tightenings.clear();
        cbt.getConstraintTightenings( tightenings );
        // including previous 2 tightenings
        TS_ASSERT_EQUALS( tightenings.size(), 3U );
        bound = tightenings.front();
        TS_ASSERT_EQUALS( bound._variable, fn );
        TS_ASSERT_EQUALS( bound._value, 5 );
        TS_ASSERT_EQUALS( bound._type, Tightening::LB );
        explanations = bound.getExplanations();
        TS_ASSERT_EQUALS( explanations.size(), 1U );
        TS_ASSERT_EQUALS( explanations.front(), fact_x1_new_lb );

        delete factTracker;
    }
};

//
// Local Variables:
// compile-command: "make -C ../../.. "
// tags-file-name: "../../../TAGS"
// c-basic-offset: 4
// End:
//
