/*********************                                                        */
/*! \file Test_MaxConstraint.h
** \verbatim
** Top contributors (to current version):
**   Derek Huang
** This file is part of the Marabou project.
** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
** in the top-level source directory) and their institutional affiliations.
** All rights reserved. See the file COPYING in the top-level source
** directory for licensing information.\endverbatim
**/

#include <cxxtest/TestSuite.h>

#include "FreshVariables.h"
#include "MaxConstraint.h"
#include "MockTableau.h"
#include "PiecewiseLinearCaseSplit.h"
#include "ReluplexError.h"

#include <cfloat>
#include <string.h>

class MockForMaxConstraint
{
public:
};

class MaxConstraintTestSuite : public CxxTest::TestSuite
{
public:
    MockForMaxConstraint *mock;

	void setUp()
    {
        TS_ASSERT( mock = new MockForMaxConstraint );
    }

    void tearDown()
    {
        TS_ASSERT_THROWS_NOTHING( delete mock );
    }

    void test_max_constraint()
    {
		unsigned f = 1;
		List<unsigned> elements;

		for ( unsigned i = 2; i < 10; ++i )
			elements.append( i );

		MaxConstraint max( f, elements );

		List<unsigned> participatingVariables;
        TS_ASSERT_THROWS_NOTHING( participatingVariables = max.getParticiatingVariables() );
        TS_ASSERT_EQUALS( participatingVariables.size(), 9U );

		auto it = participatingVariables.begin();
		TS_ASSERT_EQUALS( *it, 2U );

        // Guy: I didn't understand this. Shouldn't we check that the participatingVariables() include all indices from 1 to 9?
		std::advance( it, 10 );

        TS_ASSERT( max.participatingVariable( f ) );
        TS_ASSERT( max.participatingVariable( 8 ) );
        TS_ASSERT( !max.participatingVariable( 20 ) );
        TS_ASSERT( !max.participatingVariable( 15 ) );

		max.notifyVariableValue( f, 10 );
		max.notifyVariableValue( 8, 5 );

		TS_ASSERT( !max.satisfied() );

		max.notifyVariableValue( f, 4 );

		TS_ASSERT( !max.satisfied() );

		max.notifyVariableValue( 8, 4 );

		TS_ASSERT( max.satisfied() );

		max.notifyVariableValue( 8, 6 );

		TS_ASSERT( !max.satisfied() );

		max.notifyVariableValue( f, 6 );

		TS_ASSERT( max.satisfied() );

        // Guy: Please check also with more than one element
	}

	void test_max_fixes()
	{
        unsigned f = 1;
		List<unsigned> elements;

		for ( unsigned i = 2; i < 10; ++i )
			elements.append( i );

		MaxConstraint max( f, elements );

		List<PiecewiseLinearConstraint::Fix> fixes;
		List<PiecewiseLinearConstraint::Fix>::iterator it;

		max.notifyVariableValue( f, 7 );
		max.notifyVariableValue( 2, 6 );
		max.notifyVariableValue( 3, 5 );

		fixes = max.getPossibleFixes();
		it = fixes.begin();
		TS_ASSERT_EQUALS( it->_variable, f );
		TS_ASSERT_EQUALS( it->_value, 6 );
		++it;
		TS_ASSERT_EQUALS( it->_variable, 2U );
		TS_ASSERT_EQUALS( it->_value, 7 );

		max.notifyVariableValue( f, 4 );

		fixes = max.getPossibleFixes();
		it = fixes.begin();
		TS_ASSERT_EQUALS( it->_variable, f );
		TS_ASSERT_EQUALS( it->_value, 6 );
		++it;
		TS_ASSERT_EQUALS( it->_variable, 2U );
		TS_ASSERT_EQUALS( it->_value, 4 );

        // Guy: I think the test is wrong - the second fix doesn't satisfy the property because variable 3 is assigned 5
	}

	void test_max_case_splits()
	{
		unsigned f = 1;
		List<unsigned> elements;

		for ( unsigned i = 2; i < 10; ++i )
			elements.append( i );

		MaxConstraint max( f, elements );

		for ( unsigned i = 2; i < 10; ++i )
			max.notifyVariableValue( i, i );

        // Guy: lets notify max about f, as well, as this case (where f doesn't have an initial value) should not occur.
        // This also causes undesired behavior in the code - it attempts to access the value of f, and gets a 0 because it's not in the map.
        // I added an assertion in the code to ensure this.

		Map<unsigned, double> assignment;

		List<PiecewiseLinearConstraint::Fix> fixes;
		List<PiecewiseLinearConstraint::Fix>::iterator it;

		unsigned auxVar = 100;
		FreshVariables::setNextVariable( auxVar );

		List<PiecewiseLinearCaseSplit> splits = max.getCaseSplits();

		TS_ASSERT_EQUALS( splits.size(), 1U );

		auto split = splits.begin();
		List<PiecewiseLinearCaseSplit::Bound> bounds = split->getBoundTightenings();

        // For each case split, 2 + 7 = 9 bounds for each element, and we have 8 elements.
		TS_ASSERT_EQUALS( bounds.size(), 72U );

		auto bound = bounds.begin();

		PiecewiseLinearCaseSplit::Bound bound1 = *bound;
		std::advance( bound, 2 );
		PiecewiseLinearCaseSplit::Bound bound2 = *bound;
		++bound;
		//PiecewiseLinearCaseSplit::Bound bound3 = *bound;

        // Guy: please check that all 72 bounds are as expected. We can do this in a loop.
		TS_ASSERT_EQUALS( bound1._variable, auxVar );
		TS_ASSERT_EQUALS( bound1._boundType, PiecewiseLinearCaseSplit::Bound::UPPER );
		TS_ASSERT_EQUALS( bound1._newBound, 0.0 );

		TS_ASSERT_EQUALS( bound2._variable, auxVar + 1 );
		TS_ASSERT_EQUALS( bound2._boundType, PiecewiseLinearCaseSplit::Bound::LOWER );
		TS_ASSERT_EQUALS( bound2._newBound, 0.0 );

        // Likewise for the equations: please check all of them, in a loop.
		List<Equation> equations = split->getEquations();

		TS_ASSERT_EQUALS( equations.size(), 64U );

		auto cur = equations.begin();

		TS_ASSERT_EQUALS( cur->_addends.size(), 3U );
		TS_ASSERT_EQUALS( cur->_scalar, 0.0 );

		auto addend = cur->_addends.begin();
		TS_ASSERT_EQUALS( addend->_coefficient, 1.0 );
		TS_ASSERT_EQUALS( addend->_variable, 2U );

		cur++;
		auto addend2 = cur->_addends.begin();
		TS_ASSERT_EQUALS( addend2->_coefficient, 1.0 );
		TS_ASSERT_EQUALS( addend2->_variable, 3U );
		TS_ASSERT_EQUALS( cur->_auxVariable, 101U );
	}

	void test_register_as_watcher()
	{
		unsigned f = 1;
		List<unsigned> elements;

		MockTableau tableau;

		for ( unsigned i = 2; i < 10; ++i )
			elements.append( i );

		MaxConstraint max( f, elements );

        // Guy: why do we need the loop? A single call to registerAsWatcher should register all the variables
        // Do one call to registerAsWatcher(), then check that all variables are registered, then a single call to unregisterAsWatcher(),
        // and check that all were unregistered.
		for ( int i = 2; i < 10; ++i )
		{
			TS_ASSERT_THROWS_NOTHING( max.registerAsWatcher( &tableau ) );
			TS_ASSERT_EQUALS( tableau.lastRegisteredVariableToWatcher.size(), 9U );
			TS_ASSERT_EQUALS( tableau.lastRegisteredVariableToWatcher[i].size(), 1U );
			TS_ASSERT( tableau.lastRegisteredVariableToWatcher[i].exists( &max ) );
			TS_ASSERT_EQUALS( tableau.lastRegisteredVariableToWatcher[f].size(), 1U );
			TS_ASSERT( tableau.lastRegisteredVariableToWatcher[f].exists( &max ) );

			tableau.lastRegisteredVariableToWatcher.clear();

			TS_ASSERT_THROWS_NOTHING( max.unregisterAsWatcher( &tableau ) );

			TS_ASSERT( tableau.lastRegisteredVariableToWatcher.empty() );
			TS_ASSERT_EQUALS( tableau.lastUnregisteredVariableToWatcher.size(), 9U );
			TS_ASSERT_EQUALS( tableau.lastUnregisteredVariableToWatcher[i].size(), 1U );
			TS_ASSERT( tableau.lastUnregisteredVariableToWatcher[i].exists( &max ) );
		}
	}
};

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
