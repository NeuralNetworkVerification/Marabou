/*********************                                                        */
/*! \file Test_MaxConstraint.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Derek Huang, Shantanu Thakoor, Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

**/

#include <cxxtest/TestSuite.h>

#include "MaxConstraint.h"
#include "MockTableau.h"
#include "PiecewiseLinearCaseSplit.h"
#include "ReluplexError.h"

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
    	// tests:
    	// getParticipatingVariables, participatingVariable()
    	// notifyVariableValue
    	// satisfied

		unsigned f = 1;
		Set<unsigned> elements;

		for ( unsigned i = 2; i < 10; ++i )
			elements.insert( i );

		MaxConstraint max( f, elements );

		List<unsigned> participatingVariables;
        TS_ASSERT_THROWS_NOTHING( participatingVariables = max.getParticipatingVariables() );
        TS_ASSERT_EQUALS( participatingVariables.size(), 9U );

		auto it = participatingVariables.begin();
		for ( unsigned i = 1; i < 10; ++i, ++it )
			TS_ASSERT( max.participatingVariable( i ) );

        TS_ASSERT( !max.participatingVariable( 20 ) );
        TS_ASSERT( !max.participatingVariable( 15 ) );

		// f = max(x_2 ... x_9)
		// f = 10
		// x_8 = 5

		max.notifyVariableValue( f, 10 );
		max.notifyVariableValue( 8, 5 );

		TS_ASSERT( !max.satisfied() );

		// now f = 4
		max.notifyVariableValue( f, 4 );

		TS_ASSERT( !max.satisfied() );

		// now x_8 = 4
		max.notifyVariableValue( 8, 4 );

		TS_ASSERT( max.satisfied() );

		// now x_8 = 6
		max.notifyVariableValue( 8, 6 );

		TS_ASSERT( !max.satisfied() );

		max.notifyVariableValue( f, 6 );

		TS_ASSERT( max.satisfied() );

		max.notifyVariableValue( 7, 12 );

		TS_ASSERT( !max.satisfied() );

		max.notifyVariableValue( f, 12 );

		TS_ASSERT( max.satisfied() );
	}

	void test_max_fixes()
	{
        unsigned f = 1;
		Set<unsigned> elements;

		for ( unsigned i = 2; i < 10; ++i )
			elements.insert( i );

		MaxConstraint max( f, elements );

		// f = max(x_2 ... x_9)
		// f = 7, x_2 = 6, x_3 = 4
		List<PiecewiseLinearConstraint::Fix> fixes;
		List<PiecewiseLinearConstraint::Fix>::iterator it;

		max.notifyVariableValue( f, 7 );
		max.notifyVariableValue( 2, 6 );
		max.notifyVariableValue( 3, 4 );

		// possible fixes: set f to 6, x_2 to 7, x_3 to 7
		fixes = max.getPossibleFixes();
		it = fixes.begin();
		TS_ASSERT_EQUALS( it->_variable, f );
		TS_ASSERT_EQUALS( it->_value, 6 );
		++it;
		TS_ASSERT_EQUALS( it->_variable, 2U );
		TS_ASSERT_EQUALS( it->_value, 7 );
		++it;
		TS_ASSERT_EQUALS( it->_variable, 3U );
		TS_ASSERT_EQUALS( it->_value, 7 );

		max.notifyVariableValue( f, 5 );
		// now f = 5, x_2 = 6, x_3 = 4
		// set f to 6, or x_2 to 5
		fixes = max.getPossibleFixes();
		it = fixes.begin();
		TS_ASSERT_EQUALS( it->_variable, f );
		TS_ASSERT_EQUALS( it->_value, 6 );

		++it;
		TS_ASSERT_EQUALS ( it->_variable, 2U );
		TS_ASSERT_EQUALS ( it->_value, 5 );
	}

	void test_max_case_splits()
	{
		unsigned f = 1;
		Set<unsigned> elements;

		for ( unsigned i = 2; i < 10; ++i )
			elements.insert( i );

		MaxConstraint max( f, elements );

		for ( unsigned i = 2; i < 10; ++i )
			max.notifyVariableValue( i, i );

		max.notifyVariableValue( f, 1 );
		// f = max(x_2 ... x_9)
		// f = 1
		Map<unsigned, double> assignment;

		List<PiecewiseLinearConstraint::Fix> fixes;

		List<PiecewiseLinearCaseSplit> splits = max.getCaseSplits();

		// there are 8 possible phases
		TS_ASSERT_EQUALS( splits.size(), 8U );

		auto split = splits.begin();
		for ( unsigned i = 2; i < 10; ++i, ++split )
		{
            List<Tightening> bounds = split->getBoundTightenings();

            // Since no upper bounds known for any of the variables, no bounds
            TS_ASSERT_EQUALS( bounds.size(), 0U );

            auto equations = split->getEquations();

            TS_ASSERT_EQUALS( equations.size(), 8U );

            auto cur = equations.begin();

            TS_ASSERT_EQUALS( cur->_addends.size(), 2U );
            TS_ASSERT_EQUALS( cur->_scalar, 0.0 );
            TS_ASSERT_EQUALS( cur->_type, Equation::EQ );

            auto addend = cur->_addends.begin();

            TS_ASSERT_EQUALS( addend->_coefficient, 1.0 );
            TS_ASSERT_EQUALS( addend->_variable, i );

            ++addend;

            TS_ASSERT_EQUALS( addend->_coefficient, -1.0 );
            TS_ASSERT_EQUALS( addend->_variable, f );

            ++cur;

            for ( unsigned j = 2; j < 10;  ++j )
            {
                if ( i == j ) continue;

                TS_ASSERT_EQUALS( cur->_addends.size(), 2U );
                TS_ASSERT_EQUALS( cur->_scalar, 0.0 );
                TS_ASSERT_EQUALS( cur->_type, Equation::GE );

                auto addend = cur->_addends.begin();

                TS_ASSERT_EQUALS( addend->_coefficient, -1.0 );
                TS_ASSERT_EQUALS( addend->_variable, j );

                ++addend;

                TS_ASSERT_EQUALS( addend->_coefficient, 1.0 );
                TS_ASSERT_EQUALS( addend->_variable, i );

                ++cur;
            }
		}
	}

	void test_max_phase_fixed()
    {
		unsigned f = 1;
		Set<unsigned> elements;

		for ( unsigned i = 2; i < 10; ++i )
			elements.insert( i );

		MaxConstraint max( f, elements );

		// all variables initially between 1 and 10
		for ( unsigned i = 2; i < 10; i++ )
        {
			max.notifyUpperBound( i, 10 );
			max.notifyLowerBound( i, 1 );
		}

		TS_ASSERT( !max.phaseFixed() );

		// set x_2 to be at least 6, others to be at most 5
		max.notifyLowerBound( 2, 6 );
		for( unsigned i = 3; i < 10; i++ )
			max.notifyUpperBound( i, 5 );

		// now, phase should be fixed to x_2; all other variables should be removed
		TS_ASSERT( max.phaseFixed() );
		TS_ASSERT_EQUALS( max.getParticipatingVariables().size(), 2U );

        max.notifyVariableValue( 2, 7);

        PiecewiseLinearCaseSplit validSplit = max.getValidCaseSplit();
		auto equations = validSplit.getEquations();

        TS_ASSERT_EQUALS( equations.size(), 1U );

        auto cur = equations.begin();
        TS_ASSERT_EQUALS( cur->_addends.size(), 2U );
        TS_ASSERT_EQUALS( cur->_scalar, 0.0 );
        TS_ASSERT_EQUALS( cur->_type, Equation::EQ );

        auto addend = cur->_addends.begin();

        TS_ASSERT_EQUALS( addend->_coefficient, 1.0 );
        TS_ASSERT_EQUALS( addend->_variable, 2U );

        ++addend;

        TS_ASSERT_EQUALS( addend->_coefficient, -1.0 );
        TS_ASSERT_EQUALS( addend->_variable, f );
	}

	void test_max_var_elims()
	{
		unsigned f = 1;
		Set<unsigned> elements;

		elements.insert( 2 );
		elements.insert( 3 );

		MaxConstraint max( f, elements );

		max.notifyUpperBound( 2, 8 );
		max.notifyLowerBound( 2, 1 );
		max.notifyUpperBound( 3, 6 );
		max.notifyLowerBound( 3, 1 );

		max.notifyUpperBound( 1, 10 );
		max.notifyLowerBound( 1, 0 );
		TS_ASSERT( max.getParticipatingVariables().exists( 2 ) );
		TS_ASSERT( max.getParticipatingVariables().exists( 3 ) );

		max.notifyLowerBound( 1, 6 );
		TS_ASSERT( max.getParticipatingVariables().exists( 2 ) );
		TS_ASSERT( max.getParticipatingVariables().exists( 3 ) );

		max.notifyLowerBound( 2, 7 );
		TS_ASSERT( max.getParticipatingVariables().exists( 2 ) );
		TS_ASSERT( !max.getParticipatingVariables().exists( 3 ) );
	}

    void test_get_entailed_tightenings()
    {
		unsigned f = 1;
		Set<unsigned> elements;

		elements.insert( 2 );
		elements.insert( 3 );

		MaxConstraint max( f, elements );

		max.notifyLowerBound( 2, 1 );
    	max.notifyUpperBound( 2, 8 );
        // No lower bound for 3
    	max.notifyUpperBound( 3, 6 );

        List<Tightening> tightenings;
        TS_ASSERT_THROWS_NOTHING( max.getEntailedTightenings( tightenings ) );

        // expect f to be in the range [1, 8]
        TS_ASSERT_EQUALS( tightenings.size(), 2U );
        auto it = tightenings.begin();

        TS_ASSERT_EQUALS( it->_variable, 1U );
        TS_ASSERT_EQUALS( it->_value, 8 );
        TS_ASSERT_EQUALS( it->_type, Tightening::UB );

        ++it;

        TS_ASSERT_EQUALS( it->_variable, 1U );
        TS_ASSERT_EQUALS( it->_value, 1 );
        TS_ASSERT_EQUALS( it->_type, Tightening::LB );
	}

	void test_max_obsolete()
    {
		unsigned f = 1;
		Set<unsigned> elements;

		MockTableau tableau;

		for ( unsigned i = 2; i < 10; ++i )
			elements.insert( i );

		MaxConstraint max( f, elements );

		TS_ASSERT( !max.constraintObsolete() );
		for ( unsigned i = 2; i < 10; ++i )
			max.eliminateVariable( i, 0 );
		TS_ASSERT( max.constraintObsolete() );

		MaxConstraint max2( f, elements );
		for(unsigned i = 3; i < 10; i++ )
			max2.eliminateVariable( i, 0 );

		TS_ASSERT( !max2.constraintObsolete() );
		max2.eliminateVariable( 1, 0 );
		TS_ASSERT( max2.constraintObsolete() );
	}

	void test_register_as_watcher()
	{
		unsigned f = 1;
		Set<unsigned> elements;

		MockTableau tableau;

		for ( unsigned i = 2; i < 10; ++i )
			elements.insert( i );

		MaxConstraint max( f, elements );

		TS_ASSERT_THROWS_NOTHING( max.registerAsWatcher( &tableau ) );
		TS_ASSERT_EQUALS( tableau.lastRegisteredVariableToWatcher.size(), 9U );
		for ( int i = 1; i < 10; ++i )
		{
			TS_ASSERT_EQUALS( tableau.lastRegisteredVariableToWatcher[i].size(), 1U );
			TS_ASSERT( tableau.lastRegisteredVariableToWatcher[i].exists( &max ) );
		}
		tableau.lastRegisteredVariableToWatcher.clear();

		TS_ASSERT_THROWS_NOTHING( max.unregisterAsWatcher( &tableau ) );

		TS_ASSERT( tableau.lastRegisteredVariableToWatcher.empty() );
		TS_ASSERT_EQUALS( tableau.lastUnregisteredVariableToWatcher.size(), 9U );
		for ( int i = 1; i < 10; ++i )
		{
            TS_ASSERT_EQUALS( tableau.lastUnregisteredVariableToWatcher[i].size(), 1U );
            TS_ASSERT( tableau.lastUnregisteredVariableToWatcher[i].exists( &max ) );
		}
	}

    void test_serialize_and_unserialize()
    {
        unsigned f = 42;
        Set<unsigned> elements;

        for ( unsigned i = 2; i < 7; ++i )
			elements.insert( i );

        MaxConstraint originalMax( f, elements );
        String originalSerialized = originalMax.serializeToString();
        MaxConstraint recoveredMax( originalSerialized );

        TS_ASSERT_EQUALS( originalMax.serializeToString(),
                          recoveredMax.serializeToString() );
    }
};

//
// Local Variables:
// compile-command: "make -C ../../.. "
// tags-file-name: "../../../TAGS"
// c-basic-offset: 4
// End:
//
