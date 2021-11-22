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
        TS_ASSERT( !sigmoid.participatingVariable( 0 ) );
        TS_ASSERT( !sigmoid.participatingVariable( 2 ) );
        TS_ASSERT( !sigmoid.participatingVariable( 3 ) );
        TS_ASSERT( !sigmoid.participatingVariable( 5 ) );

        // not obsolete yet
        TS_ASSERT( !sigmoid.constraintObsolete() );

        // eliminate variable b
        sigmoid.eliminateVariable( b, 0 );  // 0 is dummy for the argument of fixedValue

        // sigmoid is obsolete now
        TS_ASSERT( sigmoid.constraintObsolete() );
    }

    void test_sigmoid_duplicate()
    {
        unsigned b = 1;
        unsigned f = 4;

        SigmoidConstraint *sigmoid = new SigmoidConstraint( b, f );

        List<unsigned> participatingVariables;
        TS_ASSERT_THROWS_NOTHING( participatingVariables = sigmoid->getParticipatingVariables() );

        // not obsolete yet
        TS_ASSERT( !sigmoid->constraintObsolete() );

        // duplicate constraint
        SigmoidConstraint *sigmoid2 = dynamic_cast<SigmoidConstraint *>( sigmoid->duplicateConstraint() );
        TS_ASSERT_THROWS_NOTHING( participatingVariables = sigmoid2->getParticipatingVariables() );
        TS_ASSERT_EQUALS( participatingVariables.size(), 2U );
        auto it = participatingVariables.begin();
        TS_ASSERT_EQUALS( *it, b );
        ++it;
        TS_ASSERT_EQUALS( *it, f );
        TS_ASSERT( sigmoid2->participatingVariable( b ) );
        TS_ASSERT( sigmoid2->participatingVariable( f ) );
        TS_ASSERT( !sigmoid2->participatingVariable( 0 ) );
        TS_ASSERT( !sigmoid2->participatingVariable( 2 ) );
        TS_ASSERT( !sigmoid2->participatingVariable( 3 ) );
        TS_ASSERT( !sigmoid2->participatingVariable( 5 ) );

        // eliminate variable b
        sigmoid->eliminateVariable( b, 0 );  // 0 is dummy for the argument of fixedValue

        // sigmoid is obsolete now
        TS_ASSERT( sigmoid->constraintObsolete() );

        // sigmoid2 is not obsolete
        TS_ASSERT( !sigmoid2->constraintObsolete() );

        TS_ASSERT_THROWS_NOTHING( delete sigmoid );
        TS_ASSERT_THROWS_NOTHING( delete sigmoid2 );
    }

    void test_sigmoid_notify_bounds()
    {
        unsigned b = 1;
        unsigned f = 4;

        SigmoidConstraint sigmoid( b, f );

        sigmoid.notifyLowerBound( b, -1.0 );
        sigmoid.notifyLowerBound( f, 0.268 );
        sigmoid.notifyUpperBound( b, 1.0 );
        sigmoid.notifyUpperBound( f, 0.731 );

        List<Tightening> tightenings;
        sigmoid.getEntailedTightenings( tightenings );

        auto it = tightenings.begin();
        TS_ASSERT_EQUALS( it->_type, Tightening::LB );
        TS_ASSERT_EQUALS( it->_variable, b );
        TS_ASSERT_EQUALS( it->_value, -1.0 );

        ++it;
        TS_ASSERT_EQUALS( it->_type, Tightening::LB );
        TS_ASSERT_EQUALS( it->_variable, f );
        TS_ASSERT_EQUALS( it->_value, 0.268 );

        ++it;
        TS_ASSERT_EQUALS( it->_type, Tightening::UB );
        TS_ASSERT_EQUALS( it->_variable, b );
        TS_ASSERT_EQUALS( it->_value, 1.0 );

        ++it;
        TS_ASSERT_EQUALS( it->_type, Tightening::UB );
        TS_ASSERT_EQUALS( it->_variable, f );
        TS_ASSERT_EQUALS( it->_value, 0.731 );


        // call notifyLowerBound and notifyUppderBound, but bounds aren't updated.
        sigmoid.notifyLowerBound( b, -2.0 );
        sigmoid.notifyLowerBound( f, 0.119 );
        sigmoid.notifyUpperBound( b, 2.0 );
        sigmoid.notifyUpperBound( f, 0.880 );

        List<Tightening> tightenings2;
        sigmoid.getEntailedTightenings( tightenings2 );

        it = tightenings2.begin();
        TS_ASSERT_EQUALS( it->_type, Tightening::LB );
        TS_ASSERT_EQUALS( it->_variable, b );
        TS_ASSERT_EQUALS( it->_value, -1.0 );

        ++it;
        TS_ASSERT_EQUALS( it->_type, Tightening::LB );
        TS_ASSERT_EQUALS( it->_variable, f );
        TS_ASSERT_EQUALS( it->_value, 0.268 );

        ++it;
        TS_ASSERT_EQUALS( it->_type, Tightening::UB );
        TS_ASSERT_EQUALS( it->_variable, b );
        TS_ASSERT_EQUALS( it->_value, 1.0 );

        ++it;
        TS_ASSERT_EQUALS( it->_type, Tightening::UB );
        TS_ASSERT_EQUALS( it->_variable, f );
        TS_ASSERT_EQUALS( it->_value, 0.731 );


        // call notifyLowerBound and notifyUppderBound, and bounds are updated.
        sigmoid.notifyLowerBound( b, -0.5 );
        sigmoid.notifyLowerBound( f, 0.377 );
        sigmoid.notifyUpperBound( b, 0.5 );
        sigmoid.notifyUpperBound( f, 0.622 );

        List<Tightening> tightenings3;
        sigmoid.getEntailedTightenings( tightenings3 );

        it = tightenings3.begin();
        TS_ASSERT_EQUALS( it->_type, Tightening::LB );
        TS_ASSERT_EQUALS( it->_variable, b );
        TS_ASSERT_EQUALS( it->_value, -0.5 );

        ++it;
        TS_ASSERT_EQUALS( it->_type, Tightening::LB );
        TS_ASSERT_EQUALS( it->_variable, f );
        TS_ASSERT_EQUALS( it->_value, 0.377 );

        ++it;
        TS_ASSERT_EQUALS( it->_type, Tightening::UB );
        TS_ASSERT_EQUALS( it->_variable, b );
        TS_ASSERT_EQUALS( it->_value, 0.5 );

        ++it;
        TS_ASSERT_EQUALS( it->_type, Tightening::UB );
        TS_ASSERT_EQUALS( it->_variable, f );
        TS_ASSERT_EQUALS( it->_value, 0.622 );
    }

    void test_sigmoid_update_variable_index()
    {
        unsigned b = 0;
        unsigned f = 1;
        unsigned new_b = 2;
        unsigned new_f = 3;

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
        TS_ASSERT( !sigmoid.participatingVariable( new_b ) );
        TS_ASSERT( !sigmoid.participatingVariable( new_f ) );

        sigmoid.notifyLowerBound( b, -1.0 );
        sigmoid.notifyLowerBound( f, 0.268 );
        sigmoid.notifyUpperBound( b, 1.0 );
        sigmoid.notifyUpperBound( f, 0.731 );


        // update variable index
        sigmoid.updateVariableIndex( b, new_b );
        sigmoid.updateVariableIndex( f, new_f );

        TS_ASSERT( !sigmoid.participatingVariable( b ) );
        TS_ASSERT( !sigmoid.participatingVariable( f ) );
        TS_ASSERT_EQUALS( sigmoid.getB(), new_b );
        TS_ASSERT_EQUALS( sigmoid.getF(), new_f );
        TS_ASSERT( sigmoid.participatingVariable( new_b ) );
        TS_ASSERT( sigmoid.participatingVariable( new_f ) );

        List<Tightening> tightenings;
        sigmoid.getEntailedTightenings( tightenings );

        auto it2 = tightenings.begin();
        TS_ASSERT_EQUALS( it2->_type, Tightening::LB );
        TS_ASSERT_EQUALS( it2->_variable, new_b );
        TS_ASSERT_EQUALS( it2->_value, -1.0 );

        ++it2;
        TS_ASSERT_EQUALS( it2->_type, Tightening::LB );
        TS_ASSERT_EQUALS( it2->_variable, new_f );
        TS_ASSERT_EQUALS( it2->_value, 0.268 );

        ++it2;
        TS_ASSERT_EQUALS( it2->_type, Tightening::UB );
        TS_ASSERT_EQUALS( it2->_variable, new_b );
        TS_ASSERT_EQUALS( it2->_value, 1.0 );

        ++it2;
        TS_ASSERT_EQUALS( it2->_type, Tightening::UB );
        TS_ASSERT_EQUALS( it2->_variable, new_f );
        TS_ASSERT_EQUALS( it2->_value, 0.731 );
    }

    void test_register_as_watcher()
    {
        unsigned b = 1;
        unsigned f = 4;

        MockTableau tableau;

        SigmoidConstraint sigmoid( b, f );

        TS_ASSERT_THROWS_NOTHING( sigmoid.registerAsWatcher( &tableau ) );

        TS_ASSERT_EQUALS( tableau.lastRegisteredVariableToWatcher.size(), 2U );
        TS_ASSERT( tableau.lastUnregisteredVariableToWatcher.empty() );
        TS_ASSERT_EQUALS( tableau.lastRegisteredVariableToWatcher[b].size(), 1U );
        TS_ASSERT( tableau.lastRegisteredVariableToWatcher[b].exists( &sigmoid ) );
        TS_ASSERT_EQUALS( tableau.lastRegisteredVariableToWatcher[f].size(), 1U );
        TS_ASSERT( tableau.lastRegisteredVariableToWatcher[f].exists( &sigmoid ) );

        tableau.lastRegisteredVariableToWatcher.clear();

        TS_ASSERT_THROWS_NOTHING( sigmoid.unregisterAsWatcher( &tableau ) );

        TS_ASSERT( tableau.lastRegisteredVariableToWatcher.empty() );
        TS_ASSERT_EQUALS( tableau.lastUnregisteredVariableToWatcher.size(), 2U );
        TS_ASSERT_EQUALS( tableau.lastUnregisteredVariableToWatcher[b].size(), 1U );
        TS_ASSERT( tableau.lastUnregisteredVariableToWatcher[b].exists( &sigmoid ) );
        TS_ASSERT_EQUALS( tableau.lastUnregisteredVariableToWatcher[f].size(), 1U );
        TS_ASSERT( tableau.lastUnregisteredVariableToWatcher[f].exists( &sigmoid ) );
    }

    void test_sigmoid_entailed_tightenings()
    {
        unsigned b = 1;
        unsigned f = 4;

        SigmoidConstraint sigmoid( b, f );

        sigmoid.notifyLowerBound( b, -1.0 );
        sigmoid.notifyLowerBound( f, 0.268 );
        sigmoid.notifyUpperBound( b, 1.0 );
        sigmoid.notifyUpperBound( f, 0.731 );

        List<Tightening> entailedTightenings;
        sigmoid.getEntailedTightenings( entailedTightenings );

        TS_ASSERT_EQUALS( entailedTightenings.size(), 4U );
        TS_ASSERT( entailedTightenings.exists( Tightening( f, 0.731, Tightening::UB ) ) );
        TS_ASSERT( entailedTightenings.exists( Tightening( b, 1.0, Tightening::UB ) ) );
        TS_ASSERT( entailedTightenings.exists( Tightening( f, 0.268, Tightening::LB ) ) );
        TS_ASSERT( entailedTightenings.exists( Tightening( b, -1.0, Tightening::LB ) ) );
    }

    void test_sigmoid_serialize()
    {
        unsigned b = 0;
        unsigned f = 1;

        SigmoidConstraint sigmoid( b, f );
    
        String serializedString = sigmoid.serializeToString();

        SigmoidConstraint serializedSigmooid( serializedString.ascii() );

        TS_ASSERT_EQUALS( serializedSigmooid.getB(), b );
        TS_ASSERT_EQUALS( serializedSigmooid.getF(), f );
    }

    void test_sigmoid_restore()
    {
        unsigned b_1 = 0;
        unsigned f_1 = 1;
        unsigned b_2 = 2;
        unsigned f_2 = 3;

        SigmoidConstraint sigmoid1( b_1, f_1 );
        TS_ASSERT_EQUALS( sigmoid1.getB(), b_1 );
        TS_ASSERT_EQUALS( sigmoid1.getF(), f_1 );

        SigmoidConstraint sigmoid2( b_2, f_2 );
        TS_ASSERT_EQUALS( sigmoid2.getB(), b_2 );
        TS_ASSERT_EQUALS( sigmoid2.getF(), f_2 );
        
        // restore
        sigmoid1.restoreState( &sigmoid2 );
        TS_ASSERT_EQUALS( sigmoid1.getB(), b_2 );
        TS_ASSERT_EQUALS( sigmoid1.getF(), f_2 );        
    }
};
