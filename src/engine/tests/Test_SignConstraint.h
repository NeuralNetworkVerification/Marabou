//
// Created by guyam on 6/4/20.
//

#ifndef MARABOU_TEST_SIGNCONSTRAINT_H
#define MARABOU_TEST_SIGNCONSTRAINT_H


#include <cxxtest/TestSuite.h>

#include "InputQuery.h"
#include "MockConstraintBoundTightener.h"
#include "MockTableau.h"
#include "PiecewiseLinearCaseSplit.h"
#include "ReluConstraint.h"
#include "SignConstraint.h"
#include "MarabouError.h"
#include "/cs/usr/guyam/CLionProjects/Marabou/src/common/tests/MockErrno.h"
//#include "MockErrno.h" // todo - yuval says red line ok


#include <string.h>





class MockForSignConstraint
        : public MockErrno
{
public:
};




class SignConstraintTestSuite : public CxxTest::TestSuite
{
public:
    MockForSignConstraint *mock;

    void setUp()
    {
        TS_ASSERT( mock = new MockForSignConstraint );
    }

    void tearDown()
    {
        TS_ASSERT_THROWS_NOTHING( delete mock );
    }

    void test_sign_constraint() // todo - PASSED
    { // TODO - PASSES
        unsigned b = 1;
        unsigned f = 4;

        SignConstraint sign( b, f ); // define constraint

        List<unsigned> participatingVariables; // needs to return 1 and 4 - the tableu vars
        TS_ASSERT_THROWS_NOTHING( participatingVariables = sign.getParticipatingVariables() ); // see doesnt throw
        TS_ASSERT_EQUALS( participatingVariables.size(), 2U ); // check that equals 2 - becausse there are 2vars
        auto it = participatingVariables.begin();
        TS_ASSERT_EQUALS( *it, b );
        ++it;
        TS_ASSERT_EQUALS( *it, f );

        TS_ASSERT( sign.participatingVariable( b ) );
        TS_ASSERT( sign.participatingVariable( f ) );
        TS_ASSERT( !sign.participatingVariable( 2 ) );
        TS_ASSERT( !sign.participatingVariable( 0 ) );
        TS_ASSERT( !sign.participatingVariable( 5 ) );

        TS_ASSERT_THROWS_EQUALS( sign.satisfied(),
                                 const MarabouError &e,
                                 e.getCode(),
                                 MarabouError::PARTICIPATING_VARIABLES_ABSENT );

        sign.notifyVariableValue( b, -1 ); // change val of b to '-1'
        sign.notifyVariableValue( f, -1 ); // change val of f to '1

        TS_ASSERT( sign.satisfied() ); // f= Relu (b) or f=sign(b)

        sign.notifyVariableValue( f, 1 );

        TS_ASSERT( !sign.satisfied() );

        sign.notifyVariableValue( b, 2 );

        TS_ASSERT( sign.satisfied() );

        sign.notifyVariableValue( b, -2 );

        TS_ASSERT( !sign.satisfied() ); // because f is '1'

        sign.notifyVariableValue( f, 1 );

        TS_ASSERT( !sign.satisfied() );

        sign.notifyVariableValue( b, 0 );

        TS_ASSERT( sign.satisfied() );

        sign.notifyVariableValue( b, 9 );

        TS_ASSERT( sign.satisfied() );

        sign.notifyVariableValue( 4, -8 ); // is first var is 'f'

        // A relu cannot be satisfied if f is not '1' or '-1'
        TS_ASSERT( !sign.satisfied() );

        sign.notifyVariableValue( 4, 1.5 ); // is first var is 'f'

        // A relu cannot be satisfied if f is not '1' or '-1'
        TS_ASSERT( !sign.satisfied() );

        sign.notifyVariableValue( f, -1 );
        sign.notifyVariableValue( b, 11 );

        TS_ASSERT( !sign.satisfied() );

        // Changing variable indices
        sign.notifyVariableValue( b, 1 );
        sign.notifyVariableValue( f, 1 );
        TS_ASSERT( sign.satisfied() );


        unsigned newB = 12;
        unsigned newF = 14;

        TS_ASSERT_THROWS_NOTHING( sign.updateVariableIndex( b, newB ) );
        TS_ASSERT_THROWS_NOTHING( sign.updateVariableIndex( f, newF ) );

        TS_ASSERT( sign.satisfied() );

        sign.notifyVariableValue( newF, -1 );

        TS_ASSERT( !sign.satisfied() );

        sign.notifyVariableValue( newB, -0.1 );

        TS_ASSERT( sign.satisfied() );
    }


    void test_sign_fixes() // todo - PASSED
    {
        unsigned b = 1;
        unsigned f = 4;

        SignConstraint sign( b, f );

        List<PiecewiseLinearConstraint::Fix> fixes;
        List<PiecewiseLinearConstraint::Fix>::iterator it;

        sign.notifyVariableValue( b, -1 );
        sign.notifyVariableValue( f, 1 );
        fixes = sign.getPossibleFixes();
        it = fixes.begin();
        TS_ASSERT_EQUALS( it->_variable, f );
        TS_ASSERT_EQUALS( it->_value, -1 ); // check if fix we changed to make f '-1'

        sign.notifyVariableValue( b, 0 );
        sign.notifyVariableValue( f, -1 );
        fixes = sign.getPossibleFixes();
        it = fixes.begin();
        TS_ASSERT_EQUALS( it->_variable, f );
        TS_ASSERT_EQUALS( it->_value, 1 ); // check if fix we changed to make f '1'


        sign.notifyVariableValue( b, 3 );
        sign.notifyVariableValue( f, -1 );
        fixes = sign.getPossibleFixes();
        it = fixes.begin();
        TS_ASSERT_EQUALS( it->_variable, f );
        TS_ASSERT_EQUALS( it->_value, 1 ); // check if fix we changed to make f '1'



        sign.notifyVariableValue( b, -2 );
        sign.notifyVariableValue( f, 1 );

        fixes = sign.getPossibleFixes();
        it = fixes.begin();
        TS_ASSERT_EQUALS( it->_variable, f );
        TS_ASSERT_EQUALS( it->_value, -1);


        sign.notifyVariableValue( b, 11 );
        sign.notifyVariableValue( f, 0 );

        fixes = sign.getPossibleFixes();
        it = fixes.begin();
        TS_ASSERT_EQUALS( it->_variable, f );
        TS_ASSERT_EQUALS( it->_value, 1 );
    }



    void test_sign_case_splits() // todo - PASSED
    {

        unsigned b = 1;
        unsigned f = 4;

        SignConstraint sign( b, f );

        List<PiecewiseLinearConstraint::Fix> fixes;
        List<PiecewiseLinearConstraint::Fix>::iterator it;

        List<PiecewiseLinearCaseSplit> splits = sign.getCaseSplits(); // returns list with 2 elemns for relu

        Equation activeEquation, inactiveEquation;

        TS_ASSERT_EQUALS( splits.size(), 2U );

        List<PiecewiseLinearCaseSplit>::iterator split1 = splits.begin();
        List<PiecewiseLinearCaseSplit>::iterator split2 = split1;
        ++split2;

        TS_ASSERT( isPositiveSplit( b, f, split1 ) || isPositiveSplit( b, f, split2 ) );
        TS_ASSERT( isNegativeSplit( b, f, split1 ) || isNegativeSplit( b, f, split2 ) );
    }


    bool isPositiveSplit( unsigned b, unsigned f, List<PiecewiseLinearCaseSplit>::iterator &split ) // todo - PASSED
    { // return true only if 2 matching bound and no equations
        List<Tightening> bounds = split->getBoundTightenings();

        auto bound = bounds.begin();
        Tightening bound1 = *bound;

        TS_ASSERT_EQUALS( bound1._variable, b );
        TS_ASSERT_EQUALS( bound1._value, 0.0 );

        if ( bound1._type != Tightening::LB )
            return false;

        TS_ASSERT_EQUALS( bounds.size(), 2U );

        ++bound;
        Tightening bound2 = *bound;

        TS_ASSERT_EQUALS( bound2._variable, f );
        TS_ASSERT_EQUALS( bound2._value, 1 );
        TS_ASSERT_EQUALS( bound2._type, Tightening::LB );

        auto equations = split->getEquations();
        TS_ASSERT( equations.empty() );

        return true;
    }


    bool isNegativeSplit( unsigned b, unsigned f, List<PiecewiseLinearCaseSplit>::iterator &split ) // todo - PASSED
    { // return true only if 2 matching bound and no equations
        List<Tightening> bounds = split->getBoundTightenings();

        auto bound = bounds.begin();
        Tightening bound1 = *bound;

        TS_ASSERT_EQUALS( bound1._variable, b );
        TS_ASSERT_EQUALS( bound1._value, 0.0 );

        if ( bound1._type != Tightening::UB )
            return false;

        TS_ASSERT_EQUALS( bounds.size(), 2U );

        ++bound;
        Tightening bound2 = *bound;

        TS_ASSERT_EQUALS( bound2._variable, f );
        TS_ASSERT_EQUALS( bound2._value, -1 );
        TS_ASSERT_EQUALS( bound2._type, Tightening::UB );

        auto equations = split->getEquations();
        TS_ASSERT( equations.empty() );

        return true;
    }


    void test_register_as_watcher() // todo - PASSED
    {
        unsigned b = 1;
        unsigned f = 4;

        MockTableau tableau;

        ReluConstraint relu( b, f );

        TS_ASSERT_THROWS_NOTHING( relu.registerAsWatcher( &tableau ) );

        TS_ASSERT_EQUALS( tableau.lastRegisteredVariableToWatcher.size(), 2U );
        TS_ASSERT( tableau.lastUnregisteredVariableToWatcher.empty() );
        TS_ASSERT_EQUALS( tableau.lastRegisteredVariableToWatcher[b].size(), 1U );
        TS_ASSERT( tableau.lastRegisteredVariableToWatcher[b].exists( &relu ) );
        TS_ASSERT_EQUALS( tableau.lastRegisteredVariableToWatcher[f].size(), 1U );
        TS_ASSERT( tableau.lastRegisteredVariableToWatcher[f].exists( &relu ) );

        tableau.lastRegisteredVariableToWatcher.clear();

        TS_ASSERT_THROWS_NOTHING( relu.unregisterAsWatcher( &tableau ) );

        TS_ASSERT( tableau.lastRegisteredVariableToWatcher.empty() );
        TS_ASSERT_EQUALS( tableau.lastUnregisteredVariableToWatcher.size(), 2U );
        TS_ASSERT_EQUALS( tableau.lastUnregisteredVariableToWatcher[b].size(), 1U );
        TS_ASSERT( tableau.lastUnregisteredVariableToWatcher[b].exists( &relu ) );
        TS_ASSERT_EQUALS( tableau.lastUnregisteredVariableToWatcher[f].size(), 1U );
        TS_ASSERT( tableau.lastUnregisteredVariableToWatcher[f].exists( &relu ) );
    }


    void test_fix_positive() // todo - PASSED
    {
        unsigned b = 1;
        unsigned f = 4;

        MockTableau tableau;

        SignConstraint sign( b, f );

        sign.registerAsWatcher( &tableau );

        List<PiecewiseLinearCaseSplit> splits = sign.getCaseSplits();
        TS_ASSERT_EQUALS( splits.size(), 2U );

        sign.notifyLowerBound( 1, 0 );
        TS_ASSERT_THROWS_EQUALS( splits = sign.getCaseSplits(),
                                 const MarabouError &e,
                                 e.getCode(),
                                 MarabouError::REQUESTED_CASE_SPLITS_FROM_FIXED_CONSTRAINT );

        sign.unregisterAsWatcher( &tableau );

        sign = SignConstraint( b, f );

        sign.registerAsWatcher( &tableau );

        splits = sign.getCaseSplits();
        TS_ASSERT_EQUALS( splits.size(), 2U );

        sign.notifyLowerBound( 4, -0.5 );
        TS_ASSERT_THROWS_EQUALS( splits = sign.getCaseSplits(),
                                 const MarabouError &e,
                                 e.getCode(),
                                 MarabouError::REQUESTED_CASE_SPLITS_FROM_FIXED_CONSTRAINT );

        sign.unregisterAsWatcher( &tableau );
    }


    void test_fix_negative() // todo - PASSED
    {
        unsigned b = 1;
        unsigned f = 4;

        MockTableau tableau;

        SignConstraint sign( b, f );

        sign.registerAsWatcher( &tableau );

        List<PiecewiseLinearCaseSplit> splits = sign.getCaseSplits();
        TS_ASSERT_EQUALS( splits.size(), 2U );

        sign.notifyUpperBound( 4, 0.5 );
        TS_ASSERT_THROWS_EQUALS( splits = sign.getCaseSplits(),
                                 const MarabouError &e,
                                 e.getCode(),
                                 MarabouError::REQUESTED_CASE_SPLITS_FROM_FIXED_CONSTRAINT );

        sign.unregisterAsWatcher( &tableau );


        sign = SignConstraint( b, f );

        sign.registerAsWatcher( &tableau );

        splits = sign.getCaseSplits();
        TS_ASSERT_EQUALS( splits.size(), 2U );

        sign.notifyUpperBound( 1, -0.5 );
        TS_ASSERT_THROWS_EQUALS( splits = sign.getCaseSplits(),
                                 const MarabouError &e,
                                 e.getCode(),
                                 MarabouError::REQUESTED_CASE_SPLITS_FROM_FIXED_CONSTRAINT );

        sign.unregisterAsWatcher( &tableau );

    }


    void test_constraint_phase_gets_fixed() // todo - PASSED
    {
        unsigned b = 1;
        unsigned f = 4;

        MockTableau tableau;

        // Upper bounds
        {
            SignConstraint sign( b, f );
            TS_ASSERT( !sign.phaseFixed() );
            sign.notifyUpperBound( b, -0.1 );
            TS_ASSERT( sign.phaseFixed() );
        }

        {
            SignConstraint sign( b, f );
            TS_ASSERT( !sign.phaseFixed() );
            sign.notifyUpperBound( b, -0.001 );
            TS_ASSERT( sign.phaseFixed() );
        }

        {
            SignConstraint sign( b, f );
            TS_ASSERT( !sign.phaseFixed() );
            sign.notifyUpperBound( f, 1 );
            TS_ASSERT( !sign.phaseFixed() );
        }

        {
            SignConstraint sign( b, f );
            TS_ASSERT( !sign.phaseFixed() );
            sign.notifyUpperBound( f, 0.5 );
            TS_ASSERT( sign.phaseFixed() );
        }

        {
            SignConstraint sign( b, f );
            TS_ASSERT( !sign.phaseFixed() );
            sign.notifyUpperBound( b, 3.0 );
            TS_ASSERT( !sign.phaseFixed() );
        }

        {
            SignConstraint sign( b, f );
            TS_ASSERT( !sign.phaseFixed() );
            sign.notifyUpperBound( b, 0 );
            TS_ASSERT( !sign.phaseFixed() );
        }

        // Lower bounds
        {
            SignConstraint sign( b, f );
            TS_ASSERT( !sign.phaseFixed() );
            sign.notifyLowerBound( b, -0.1 );
            TS_ASSERT( !sign.phaseFixed() );
        }

        {
            SignConstraint sign( b, f );
            TS_ASSERT( !sign.phaseFixed() );
            sign.notifyLowerBound( b, 0 );
            TS_ASSERT( sign.phaseFixed() );
        }

        {
            SignConstraint sign( b, f );
            TS_ASSERT( !sign.phaseFixed() );
            sign.notifyLowerBound( f, -1 );
            TS_ASSERT( !sign.phaseFixed() );
        }

        {
            SignConstraint sign( b, f );
            TS_ASSERT( !sign.phaseFixed() );
            sign.notifyLowerBound( f, -0.6 );
            TS_ASSERT( sign.phaseFixed() );
        }

        {
            SignConstraint sign( b, f );
            TS_ASSERT( !sign.phaseFixed() );
            sign.notifyLowerBound( b, 0.0 );
            TS_ASSERT( sign.phaseFixed() );
        }

        {
            SignConstraint sign( b, f );
            TS_ASSERT( !sign.phaseFixed() );
            sign.notifyLowerBound( f, 6.0 );
            TS_ASSERT( sign.phaseFixed() );
        }

        {
            SignConstraint sign( b, f );
            TS_ASSERT( !sign.phaseFixed() );
            sign.notifyLowerBound( f, 0.0 );
            TS_ASSERT( sign.phaseFixed() );
        }

        {
            SignConstraint sign( b, f );
            TS_ASSERT( !sign.phaseFixed() );
            sign.notifyLowerBound( b, -2.0 );
            TS_ASSERT( !sign.phaseFixed() );
        }

    }


};









#endif //MARABOU_TEST_SIGNCONSTRAINT_H
