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

#include "FreshVariables.h"
#include "MockTableau.h"
#include "PiecewiseLinearCaseSplit.h"
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

        List<unsigned> participatingVariables;
        TS_ASSERT_THROWS_NOTHING( participatingVariables = relu.getParticiatingVariables() );
        TS_ASSERT_EQUALS( participatingVariables.size(), 2U );
        auto it = participatingVariables.begin();
        TS_ASSERT_EQUALS( *it, b );
        ++it;
        TS_ASSERT_EQUALS( *it, f );

        TS_ASSERT( relu.participatingVariable( b ) );
        TS_ASSERT( relu.participatingVariable( f ) );
        TS_ASSERT( !relu.participatingVariable( 2 ) );
        TS_ASSERT( !relu.participatingVariable( 0 ) );
        TS_ASSERT( !relu.participatingVariable( 5 ) );

        TS_ASSERT_THROWS_EQUALS( relu.satisfied(),
                                 const ReluplexError &e,
                                 e.getCode(),
                                 ReluplexError::PARTICIPATING_VARIABLES_ABSENT );

        relu.notifyVariableValue( b, 1 );
        relu.notifyVariableValue( f, 1 );

        TS_ASSERT( relu.satisfied() );

        relu.notifyVariableValue( f, 2 );

        TS_ASSERT( !relu.satisfied() );

        relu.notifyVariableValue( b, 2 );

        TS_ASSERT( relu.satisfied() );

        relu.notifyVariableValue( b, -2 );

        TS_ASSERT( !relu.satisfied() );

        relu.notifyVariableValue( f, 0 );

        TS_ASSERT( relu.satisfied() );

        relu.notifyVariableValue( b, -3 );

        TS_ASSERT( relu.satisfied() );

        relu.notifyVariableValue( b, 0 );

        TS_ASSERT( relu.satisfied() );

        relu.notifyVariableValue( f, 0 );
        relu.notifyVariableValue( b, 11 );

        TS_ASSERT( !relu.satisfied() );
    }

    void test_relu_fixes()
    {
        unsigned b = 1;
        unsigned f = 4;

        ReluConstraint relu( b, f );

        List<PiecewiseLinearConstraint::Fix> fixes;
        List<PiecewiseLinearConstraint::Fix>::iterator it;

        relu.notifyVariableValue( b, -1 );
        relu.notifyVariableValue( f, 1 );

        fixes = relu.getPossibleFixes();
        it = fixes.begin();
        TS_ASSERT_EQUALS( it->_variable, b );
        TS_ASSERT_EQUALS( it->_value, 1 );
        ++it;
        TS_ASSERT_EQUALS( it->_variable, f );
        TS_ASSERT_EQUALS( it->_value, 0 );

        relu.notifyVariableValue( b, 2 );
        relu.notifyVariableValue( f, 1 );

        fixes = relu.getPossibleFixes();
        it = fixes.begin();
        TS_ASSERT_EQUALS( it->_variable, b );
        TS_ASSERT_EQUALS( it->_value, 1 );
        ++it;
        TS_ASSERT_EQUALS( it->_variable, f );
        TS_ASSERT_EQUALS( it->_value, 2 );

        relu.notifyVariableValue( b, 11 );
        relu.notifyVariableValue( f, 0 );

        fixes = relu.getPossibleFixes();
        it = fixes.begin();
        TS_ASSERT_EQUALS( it->_variable, b );
        TS_ASSERT_EQUALS( it->_value, 0 );
        ++it;
        TS_ASSERT_EQUALS( it->_variable, f );
        TS_ASSERT_EQUALS( it->_value, 11 );
    }

    void test_relu_case_splits()
    {
        unsigned b = 1;
        unsigned f = 4;

        ReluConstraint relu( b, f );

        Map<unsigned, double> assignment;

        List<PiecewiseLinearConstraint::Fix> fixes;
        List<PiecewiseLinearConstraint::Fix>::iterator it;

        unsigned auxVar = 100;
        FreshVariables::setNextVariable( auxVar );

        List<PiecewiseLinearCaseSplit> splits = relu.getCaseSplits();

        Equation activeEquation, inactiveEquation;

        TS_ASSERT_EQUALS( splits.size(), 2U );

        // First split
        auto split = splits.begin();
        List<Tightening> bounds = split->getBoundTightenings();

        TS_ASSERT_EQUALS( bounds.size(), 3U );
        auto bound = bounds.begin();
        Tightening bound1 = *bound;
        ++bound;
        Tightening bound2 = *bound;
        ++bound;
        Tightening bound3 = *bound;

        TS_ASSERT_EQUALS( bound1._variable, b );
        TS_ASSERT_EQUALS( bound1._type, Tightening::LB );
        TS_ASSERT_EQUALS( bound1._value, 0.0 );

        TS_ASSERT_EQUALS( bound2._variable, auxVar );
        TS_ASSERT_EQUALS( bound2._type, Tightening::UB );
        TS_ASSERT_EQUALS( bound2._value, 0.0 );

        TS_ASSERT_EQUALS( bound3._variable, auxVar );
        TS_ASSERT_EQUALS( bound3._type, Tightening::LB );
        TS_ASSERT_EQUALS( bound3._value, 0.0 );

        List<Equation> equations = split->getEquations();
        TS_ASSERT_EQUALS( equations.size(), 1U );
        activeEquation = split->getEquations().front();
        TS_ASSERT_EQUALS( activeEquation._addends.size(), 3U );
        TS_ASSERT_EQUALS( activeEquation._scalar, 0.0 );

        auto addend = activeEquation._addends.begin();
        TS_ASSERT_EQUALS( addend->_coefficient, 1.0 );
        TS_ASSERT_EQUALS( addend->_variable, b );

        ++addend;
        TS_ASSERT_EQUALS( addend->_coefficient, -1.0 );
        TS_ASSERT_EQUALS( addend->_variable, f );

        ++addend;
        TS_ASSERT_EQUALS( addend->_coefficient, 1.0 );
        TS_ASSERT_EQUALS( addend->_variable, 100U );
        TS_ASSERT_EQUALS( activeEquation._auxVariable, 100U );

        // Second split
        ++split;
        bounds = split->getBoundTightenings();

        TS_ASSERT_EQUALS( bounds.size(), 3U );
        bound = bounds.begin();
        bound1 = *bound;
        ++bound;
        bound2 = *bound;
        ++bound;
        bound3 = *bound;

        TS_ASSERT_EQUALS( bound1._variable, b );
        TS_ASSERT_EQUALS( bound1._type, Tightening::UB );
        TS_ASSERT_EQUALS( bound1._value, 0.0 );

        TS_ASSERT_EQUALS( bound2._variable, auxVar );
        TS_ASSERT_EQUALS( bound2._type, Tightening::UB );
        TS_ASSERT_EQUALS( bound2._value, 0.0 );

        TS_ASSERT_EQUALS( bound3._variable, auxVar );
        TS_ASSERT_EQUALS( bound3._type, Tightening::LB );
        TS_ASSERT_EQUALS( bound3._value, 0.0 );

        equations = split->getEquations();
        TS_ASSERT_EQUALS( equations.size(), 1U );
        inactiveEquation = split->getEquations().front();
        TS_ASSERT_EQUALS( inactiveEquation._addends.size(), 2U );
        TS_ASSERT_EQUALS( inactiveEquation._scalar, 0.0 );

        addend = inactiveEquation._addends.begin();
        TS_ASSERT_EQUALS( addend->_coefficient, 1.0 );
        TS_ASSERT_EQUALS( addend->_variable, f );

        ++addend;
        TS_ASSERT_EQUALS( addend->_coefficient, 1.0 );
        TS_ASSERT_EQUALS( addend->_variable, 100U );
        TS_ASSERT_EQUALS( inactiveEquation._auxVariable, 100U );
    }

    void test_register_as_watcher()
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
};

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
