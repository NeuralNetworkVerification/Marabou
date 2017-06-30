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

        assignment[f] = 0;
        assignment[b] = 11;

        TS_ASSERT( !relu.satisfied( assignment ) );
    }

    void test_relu_fixes()
    {
        unsigned b = 1;
        unsigned f = 4;

        ReluConstraint relu( b, f );

        Map<unsigned, double> assignment;

        List<PiecewiseLinearConstraint::Fix> fixes;
        List<PiecewiseLinearConstraint::Fix>::iterator it;

        assignment[b] = -1;
        assignment[f] = 1;

        fixes = relu.getPossibleFixes( assignment );
        it = fixes.begin();
        TS_ASSERT_EQUALS( it->_variable, b );
        TS_ASSERT_EQUALS( it->_value, 1 );
        ++it;
        TS_ASSERT_EQUALS( it->_variable, f );
        TS_ASSERT_EQUALS( it->_value, 0 );

        assignment[b] = 2;
        assignment[f] = 1;

        fixes = relu.getPossibleFixes( assignment );
        it = fixes.begin();
        TS_ASSERT_EQUALS( it->_variable, b );
        TS_ASSERT_EQUALS( it->_value, 1 );
        ++it;
        TS_ASSERT_EQUALS( it->_variable, f );
        TS_ASSERT_EQUALS( it->_value, 2 );

        assignment[b] = 11;
        assignment[f] = 0;

        fixes = relu.getPossibleFixes( assignment );
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
        List<PiecewiseLinearCaseSplit::Bound> bounds = split->getBoundTightenings();

        TS_ASSERT_EQUALS( bounds.size(), 3U );
        auto bound = bounds.begin();
        PiecewiseLinearCaseSplit::Bound bound1 = *bound;
        ++bound;
        PiecewiseLinearCaseSplit::Bound bound2 = *bound;
        ++bound;
        PiecewiseLinearCaseSplit::Bound bound3 = *bound;

        TS_ASSERT_EQUALS( bound1._variable, b );
        TS_ASSERT_EQUALS( bound1._upperBound, false );
        TS_ASSERT_EQUALS( bound1._newBound, 0.0 );

        TS_ASSERT_EQUALS( bound2._variable, auxVar );
        TS_ASSERT_EQUALS( bound2._upperBound, true );
        TS_ASSERT_EQUALS( bound2._newBound, 0.0 );

        TS_ASSERT_EQUALS( bound3._variable, auxVar );
        TS_ASSERT_EQUALS( bound3._upperBound, false );
        TS_ASSERT_EQUALS( bound3._newBound, 0.0 );

        activeEquation = split->getEquation();
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
        TS_ASSERT_EQUALS( bound1._upperBound, true );
        TS_ASSERT_EQUALS( bound1._newBound, 0.0 );

        TS_ASSERT_EQUALS( bound2._variable, auxVar );
        TS_ASSERT_EQUALS( bound2._upperBound, true );
        TS_ASSERT_EQUALS( bound2._newBound, 0.0 );

        TS_ASSERT_EQUALS( bound3._variable, auxVar );
        TS_ASSERT_EQUALS( bound3._upperBound, false );
        TS_ASSERT_EQUALS( bound3._newBound, 0.0 );

        inactiveEquation = split->getEquation();
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
};

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
