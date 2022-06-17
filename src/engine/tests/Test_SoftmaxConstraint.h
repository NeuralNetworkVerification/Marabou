/*********************                                                        */
/*! \file Test_SigmoidConstraint.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Haoze Wu
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
#include "SoftmaxConstraint.h"
#include "MockTableau.h"
#include <string.h>

class SoftMaxConstraintTestSuite : public CxxTest::TestSuite
{
public:

    void setUp()
    {
    }

    void tearDown()
    {
    }

    void test_softmax_constraint()
    {
        Vector<unsigned> inputs = { 1, 2, 3, 4 };
        Vector<unsigned> outputs = { 5, 6, 7, 8 };

        SoftmaxConstraint softmax( inputs, outputs );

        List<unsigned> participatingVariables;
        TS_ASSERT_THROWS_NOTHING( participatingVariables = softmax.getParticipatingVariables() );
        TS_ASSERT_EQUALS( participatingVariables.size(), 8U );
        for ( unsigned i = 1; i <= 8; ++i )
            TS_ASSERT( participatingVariables.exists( i ) );

        TS_ASSERT( softmax.participatingVariable( 1 ) );
        TS_ASSERT( softmax.participatingVariable( 2 ) );
        TS_ASSERT( !softmax.participatingVariable( 0 ) );
        TS_ASSERT( !softmax.participatingVariable( 12 ) );

        // not obsolete yet
        TS_ASSERT( !softmax.constraintObsolete() );
    }

    void test_softmax_duplicate()
    {
        Vector<unsigned> inputs = { 1, 2, 3, 4 };
        Vector<unsigned> outputs = { 5, 6, 7, 8 };

        SoftmaxConstraint *softmax = new SoftmaxConstraint( inputs, outputs );
        SoftmaxConstraint *softmax2 = dynamic_cast<SoftmaxConstraint *>( softmax->duplicateConstraint() );

        TS_ASSERT_THROWS_NOTHING( softmax->getParticipatingVariables() =
                                  softmax2->getParticipatingVariables() );
        TS_ASSERT( !softmax2->constraintObsolete() );

        TS_ASSERT_THROWS_NOTHING( delete softmax );
        TS_ASSERT_THROWS_NOTHING( delete softmax2 );
    }

    void test_softmax_notify_bounds_and_entailed_bounds()
    {
        Vector<unsigned> inputs = { 1, 2, 3, 4 };
        Vector<unsigned> outputs = { 5, 6, 7, 8 };

        SoftmaxConstraint softmax( inputs, outputs );

        softmax.notifyLowerBound( 1, -1 );
        softmax.notifyLowerBound( 2, -1 );
        softmax.notifyUpperBound( 1, 1 );
        softmax.notifyUpperBound( 2, 1 );

        List<Tightening> tightenings;
        softmax.getEntailedTightenings( tightenings );
        TS_ASSERT_EQUALS( tightenings.size(), 8U );

        softmax.notifyLowerBound( 3, -1 );
        softmax.notifyLowerBound( 4, -1 );
        softmax.notifyUpperBound( 3, 1 );
        softmax.notifyUpperBound( 4, 1 );

        tightenings.clear();
        softmax.getEntailedTightenings( tightenings );
        TS_ASSERT_EQUALS( tightenings.size(), 16U );

        for ( unsigned i = 5; i <= 8; ++i )
        {
            TS_ASSERT( existsBound( tightenings, Tightening( i, 0, Tightening::LB ) ) );
            TS_ASSERT( existsBound( tightenings, Tightening( i, 1, Tightening::UB ) ) );
            TS_ASSERT( existsBound( tightenings, Tightening( i, 0.043164532979996, Tightening::LB ) ) );
            TS_ASSERT( existsBound( tightenings, Tightening( i, 0.711234594227593, Tightening::UB ) ) );
        }
    }

    bool existsBound( const List<Tightening> &tightenings,
                       const Tightening &tightening )
    {
        for ( const auto &t : tightenings )
        {
            if ( t._type == tightening._type  &&
                 t._variable == tightening._variable &&
                 FloatUtils::areEqual( t._value, tightening._value ) )
                return true;
        }

        return false;
    }

    void test_sigmoid_update_variable_index()
    {
        Vector<unsigned> inputs = { 1, 2, 3, 4 };
        Vector<unsigned> outputs = { 5, 6, 7, 8 };

        SoftmaxConstraint softmax( inputs, outputs );

        TS_ASSERT( softmax.participatingVariable( 1 ) );
        TS_ASSERT( softmax.participatingVariable( 2 ) );
        TS_ASSERT( !softmax.participatingVariable( 9 ) );
        TS_ASSERT( !softmax.participatingVariable( 10 ) );

        softmax.notifyLowerBound( 1, -1.0 );
        softmax.notifyLowerBound( 2, 0.268 );
        softmax.notifyUpperBound( 1, 1.0 );
        softmax.notifyUpperBound( 2, 0.731 );


        // update variable index
        softmax.updateVariableIndex( 1, 9 );
        softmax.updateVariableIndex( 2, 10 );

        TS_ASSERT( !softmax.participatingVariable( 1 ) );
        TS_ASSERT( !softmax.participatingVariable( 2 ) );
        TS_ASSERT( softmax.getInputs()[0] == 9 );
        TS_ASSERT( softmax.getInputs()[1] == 10 );
        TS_ASSERT( softmax.participatingVariable( 9 ) );
        TS_ASSERT( softmax.participatingVariable( 10 ) );
    }

    void test_register_as_watcher()
    {
        Vector<unsigned> inputs = { 1, 2, 3, 4 };
        Vector<unsigned> outputs = { 5, 6, 7, 8 };

        SoftmaxConstraint softmax( inputs, outputs );

        MockTableau tableau;

        TS_ASSERT_THROWS_NOTHING( softmax.registerAsWatcher( &tableau ) );

        TS_ASSERT_EQUALS( tableau.lastRegisteredVariableToWatcher.size(), 8U );
        TS_ASSERT( tableau.lastUnregisteredVariableToWatcher.empty() );
        TS_ASSERT_EQUALS( tableau.lastRegisteredVariableToWatcher[1].size(), 1U );
        TS_ASSERT( tableau.lastRegisteredVariableToWatcher[1].exists( &softmax ) );
        TS_ASSERT_EQUALS( tableau.lastRegisteredVariableToWatcher[5].size(), 1U );
        TS_ASSERT( tableau.lastRegisteredVariableToWatcher[5].exists( &softmax ) );

        tableau.lastRegisteredVariableToWatcher.clear();

        TS_ASSERT_THROWS_NOTHING( softmax.unregisterAsWatcher( &tableau ) );

        TS_ASSERT( tableau.lastRegisteredVariableToWatcher.empty() );
        TS_ASSERT_EQUALS( tableau.lastUnregisteredVariableToWatcher.size(), 8U );
        TS_ASSERT_EQUALS( tableau.lastUnregisteredVariableToWatcher[1].size(), 1U );
        TS_ASSERT( tableau.lastUnregisteredVariableToWatcher[1].exists( &softmax ) );
        TS_ASSERT_EQUALS( tableau.lastUnregisteredVariableToWatcher[5].size(), 1U );
        TS_ASSERT( tableau.lastUnregisteredVariableToWatcher[5].exists( &softmax ) );
    }

};
