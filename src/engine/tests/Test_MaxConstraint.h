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
#include "MarabouError.h"
#include "InputQuery.h"

#include <string.h>

class MockForMaxConstraint
{
public:
};

/*
   Exposes protected members of AbsConstraint for testing.
 */
class TestMaxConstraint : public MaxConstraint
{
public:
    TestMaxConstraint( unsigned f, const Set<unsigned> elements  )
        : MaxConstraint( f, elements )
    {}

    using MaxConstraint::getPhaseStatus;
};

using namespace CVC4::context;

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

        //
        // From here, the test is going to run tests for the case that f is included in elements.
        //
        elements.insert( f );

        MaxConstraint max2( f, elements );

        TS_ASSERT_THROWS_NOTHING( participatingVariables = max2.getParticipatingVariables() );
        TS_ASSERT_EQUALS( participatingVariables.size(), 9U );

        it = participatingVariables.begin();
        for ( unsigned i = 1; i < 10; ++i, ++it )
        TS_ASSERT( max2.participatingVariable( i ) );

        TS_ASSERT( !max2.participatingVariable( 20 ) );
        TS_ASSERT( !max2.participatingVariable( 15 ) );

        // f = max(x_1 ... x_9)
        // f = 5
        // x_8 = 10

        max2.notifyVariableValue( f, 5 );
        max2.notifyVariableValue( 8, 10 );

        TS_ASSERT( !max2.satisfied() );

        // now f = 4
        max2.notifyVariableValue( f, 4 );

        TS_ASSERT( !max2.satisfied() );

        // now x_8 = 4
        max2.notifyVariableValue( 8, 4 );

        TS_ASSERT( max2.satisfied() );

        // now x_8 = 6
        max2.notifyVariableValue( 8, 6 );

        TS_ASSERT( !max2.satisfied() );

        // now f = 6
        max2.notifyVariableValue( f, 6 );

        TS_ASSERT( max2.satisfied() );

        // now x_7 = 12
        max2.notifyVariableValue( 7, 12 );

        TS_ASSERT( !max2.satisfied() );

        // now f = 12
        max2.notifyVariableValue( f, 12 );

        TS_ASSERT( max2.satisfied() );

        // now x_6 = 13
        max2.notifyVariableValue( 6, 13 );

        TS_ASSERT( !max2.satisfied() );

        // now f = 14
        max2.notifyVariableValue( f, 14 );

        TS_ASSERT( max2.satisfied() );
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

        //
        // From here, the test is going to run tests for the case that f is included in elements.
        //
        elements.insert( f );

        MaxConstraint max2( f, elements );


        // f = 5, x_2 = 6, x_3 = 4
        max2.notifyVariableValue( f, 5 );
        max2.notifyVariableValue( 2, 6 );
        max2.notifyVariableValue( 3, 4 );

        // possible fixes: set f to 6, x_2 to 5
        fixes = max2.getPossibleFixes();
        TS_ASSERT_EQUALS( fixes.size(), 2U );

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

        //
        // From here, the test is going to run tests for the case that f is included in elements.
        //
        elements.insert( f );
        MaxConstraint max2( f, elements );

        for ( unsigned i = 1; i < 10; ++i )
            max2.notifyVariableValue( i, i );

        // f = max(x_1 ... x_9)
        // f = 1
        max2.notifyVariableValue( f, 1 );

        splits = max2.getCaseSplits();

        // there are 1 possible phases
        TS_ASSERT_EQUALS( splits.size(), 1U );

        split = splits.begin();

        List<Tightening> bounds = split->getBoundTightenings();

        // Since no upper bounds known for any of the variables, no bounds
        TS_ASSERT_EQUALS( bounds.size(), 0U );

        auto equations = split->getEquations();

        TS_ASSERT_EQUALS( equations.size(), 8U );

        auto cur = equations.begin();

        for ( unsigned i = 2; i < 10;  ++i )
        {
            TS_ASSERT_EQUALS( cur->_addends.size(), 2U );
            TS_ASSERT_EQUALS( cur->_scalar, 0.0 );
            TS_ASSERT_EQUALS( cur->_type, Equation::GE );

            auto addend = cur->_addends.begin();

            TS_ASSERT_EQUALS( addend->_coefficient, -1.0 );
            TS_ASSERT_EQUALS( addend->_variable, i );

            ++addend;

            TS_ASSERT_EQUALS( addend->_coefficient, 1.0 );
            TS_ASSERT_EQUALS( addend->_variable, f );

            ++cur;
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

        max.notifyVariableValue( 2, 7);
        // now, phase should be fixed to x_2; all other variables should be removed
        TS_ASSERT( max.phaseFixed() );
        TS_ASSERT_EQUALS( max.getParticipatingVariables().size(), 2U );

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

        //
        // From here, the test is going to run tests for the case that f is included in elements.
        //
        Set<unsigned> elements2;

        for ( unsigned i = 1; i < 10; ++i )
            elements2.insert( i );

        MaxConstraint max2( f, elements2 );

        // all variables initially between 1 and 10
        for ( unsigned i = 1; i < 10; i++ )
        {
            max2.notifyUpperBound( i, 10 );
            max2.notifyLowerBound( i, 1 );
        }


        // set x_2 and x_3 to be at least 6, others to be at most 5
        max2.notifyLowerBound( 2, 6 );
        max2.notifyLowerBound( 3, 6 );
        for( unsigned i = 4; i < 10; i++ )
            max2.notifyUpperBound( i, 5 );
        max2.notifyUpperBound( f, 5 );

        max2.notifyVariableValue( f, 5);
        // now, phase should be fixed to x_2 and x_3; all other variables should be removed
        TS_ASSERT( max2.phaseFixed() );
        TS_ASSERT_EQUALS( max2.getParticipatingVariables().size(), 3U );

        validSplit = max2.getValidCaseSplit();
        equations = validSplit.getEquations();

        TS_ASSERT_EQUALS( equations.size(), 2U );

        cur = equations.begin();
        TS_ASSERT_EQUALS( cur->_addends.size(), 2U );
        TS_ASSERT_EQUALS( cur->_scalar, 0.0 );
        TS_ASSERT_EQUALS( cur->_type, Equation::GE );

        addend = cur->_addends.begin();

        TS_ASSERT_EQUALS( addend->_coefficient, -1.0 );
        TS_ASSERT_EQUALS( addend->_variable, 2U );

        ++addend;

        TS_ASSERT_EQUALS( addend->_coefficient, 1.0 );
        TS_ASSERT_EQUALS( addend->_variable, f );

        ++cur;
        TS_ASSERT_EQUALS( cur->_addends.size(), 2U );
        TS_ASSERT_EQUALS( cur->_scalar, 0.0 );
        TS_ASSERT_EQUALS( cur->_type, Equation::GE );

        addend = cur->_addends.begin();

        TS_ASSERT_EQUALS( addend->_coefficient, -1.0 );
        TS_ASSERT_EQUALS( addend->_variable, 3U );

        ++addend;

        TS_ASSERT_EQUALS( addend->_coefficient, 1.0 );
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

        //
        // From here, the test is going to run tests for the case that f is included in elements.
        //
        Set<unsigned> elements2;

        elements2.insert( 1 );
        elements2.insert( 2 );
        elements2.insert( 3 );

        MaxConstraint max2( f, elements2 );

        max2.notifyUpperBound( 2, 8 );
        max2.notifyLowerBound( 2, 1 );

        max2.notifyUpperBound( 3, 6 );
        max2.notifyLowerBound( 3, 1 );


        max2.notifyUpperBound( 1, 6 );
        max2.notifyLowerBound( 1, 0 );

        TS_ASSERT( max2.getParticipatingVariables().exists( 1 ) );
        TS_ASSERT( max2.getParticipatingVariables().exists( 2 ) );
        TS_ASSERT( max2.getParticipatingVariables().exists( 3 ) );

        max2.notifyLowerBound( 2, 7 );
        TS_ASSERT( max2.getParticipatingVariables().exists( 1 ) );
        TS_ASSERT( max2.getParticipatingVariables().exists( 2 ) );
        TS_ASSERT( !max2.getParticipatingVariables().exists( 3 ) );

        max2.notifyUpperBound( 1, 5 );

        TS_ASSERT( max2.serializeToString() == "max,1,1,2,e,0,0" );
        TS_ASSERT( max2.getParticipatingVariables().exists( 2 ) );
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


        //
        // From here, the test is going to run tests for the case that f is included in elements.
        //
        Set<unsigned> elements2;
        elements2.insert( 1 );
        elements2.insert( 2 );
        elements2.insert( 3 );

        MaxConstraint max2( f, elements2 );

        // No lower bound for 1
        max2.notifyUpperBound( 1, 7 );

        max2.notifyLowerBound( 2, 1 );
        max2.notifyUpperBound( 2, 8 );

        // No lower bound for 3
        max2.notifyUpperBound( 3, 6 );

        List<Tightening> tightenings2;
        TS_ASSERT_THROWS_NOTHING( max2.getEntailedTightenings( tightenings2 ) );

        // expect f to be in the range [1, 7], x2 to be in the range [1, 7]
        TS_ASSERT_EQUALS( tightenings2.size(), 2U );
        it = tightenings2.begin();

        TS_ASSERT_EQUALS( it->_variable, 2U );
        TS_ASSERT_EQUALS( it->_value, 7 );
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

        elements.insert( 1 );
        MaxConstraint max3( f, elements );
        for(unsigned i = 2; i < 10; i++ )
            max3.eliminateVariable( i, 0 );

        TS_ASSERT( max3.constraintObsolete() );
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

    void test_register_as_watcher2()
    {
        unsigned f = 1;
        Set<unsigned> elements;

        MockTableau tableau;

        for ( unsigned i = 1; i < 10; ++i )
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

    void test_max_duplicate()
    {
        TS_TRACE( "TODO: add a test for duplicate" );
    }

    void test_serialize_and_unserialize()
    {
        unsigned f = 42;
        Set<unsigned> elements;

        for ( unsigned i = 2; i < 7; ++i )
            elements.insert( i );

        MaxConstraint originalMax( f, elements );
        String originalSerialized = originalMax.serializeToString();
        MaxConstraint recoveredMax(  originalSerialized );

        TS_ASSERT_EQUALS( originalMax.serializeToString(),
                          recoveredMax.serializeToString() );
    }

    void test_serialize_and_unserialize_2()
    {
        unsigned f = 17;
        Set<unsigned> elements;

        for ( unsigned i = 0; i < 4; ++i )
            elements.insert( i );

        MaxConstraint originalMax( f, elements );

        originalMax.eliminateVariable(2, 12);
        originalMax.eliminateVariable(1, 11);
        originalMax.eliminateVariable(3, 13);


        String originalSerialized = originalMax.serializeToString();
        MaxConstraint recoveredMax(  originalSerialized );

        TS_ASSERT_EQUALS( originalMax.serializeToString(),
                          recoveredMax.serializeToString() );
    }

    void test_phase_fixed_with_variable_elimination()
    {
        // Check phase fixed after eliminating - and upper bound updated
        unsigned f = 4;
        Set<unsigned> elements;

        for ( unsigned i = 0; i < 4; ++i )
            elements.insert( i );

        MaxConstraint max( f, elements );

        TS_ASSERT( !max.wereVariablesEliminated() );
        max.eliminateVariable( 1, -11 );
        TS_ASSERT( max.wereVariablesEliminated() );
        max.eliminateVariable( 2, 12 );
        max.eliminateVariable( 3, 13 );
        // 13 is the maximum value eliminated

        max.notifyLowerBound( 0,1 );
        max.notifyUpperBound( 0,14 );
        TS_ASSERT( !max.phaseFixed() )

        max.notifyUpperBound( 0,13 );
        TS_ASSERT( max.phaseFixed() )

        // Check phase fixed after eliminating - and lower bound updated
        unsigned f2 = 44;
        Set<unsigned> elements2;

        for ( unsigned i = 0; i < 4; ++i )
            elements2.insert( i );

        MaxConstraint max2( f2, elements2 );

        TS_ASSERT( !max2.wereVariablesEliminated() );
        max2.eliminateVariable( 1, -11 );
        TS_ASSERT( max2.wereVariablesEliminated() );
        max2.eliminateVariable( 2, 12 );
        max2.eliminateVariable( 3, 13 );
        // 13 is the maximum value eliminated

        max2.notifyLowerBound( 0,12.5 );
        max2.notifyUpperBound( 0,13.5 );
        TS_ASSERT( !max2.phaseFixed() )

        max2.notifyLowerBound( 0,13 );
        TS_ASSERT( max2.phaseFixed() )
    }

    void test_max_case_splits_with_variable_elimination()
    {
        unsigned f = 1;
        Set<unsigned> elements;

        for ( unsigned i = 2; i < 6; ++i )
            elements.insert( i );

        MaxConstraint max( f, elements );

        for ( unsigned i = 2; i < 6; ++i )
            max.notifyVariableValue( i, i );

        max.notifyVariableValue( f, 1 );
        // f = max(x_2 ... x_5)
        // f = 1

        TS_ASSERT( !max.wereVariablesEliminated() );
        // Eliminate single variable x_5 = 5
        unsigned eliminatedVariableValue = 5;
        max.eliminateVariable( 5,eliminatedVariableValue );
        TS_ASSERT( max.wereVariablesEliminated() );

        List<PiecewiseLinearCaseSplit> splits = max.getCaseSplits();

        // There are 4 possible phases: f1=b2, f1=b3, f1=b4, f1=maxValueOfEliminated
        TS_ASSERT_EQUALS( splits.size(), 4U );

        // Check first two phases: f1=b2, f1=b3
        auto split = splits.begin();
        for ( unsigned i = 2; i < 5; ++i, ++split )
        {
            List<Tightening> bounds = split->getBoundTightenings();

            // For each split, there is a single LB element >= maxValueEliminated
            TS_ASSERT_EQUALS( bounds.size(), 1U );

            auto it = bounds.begin();

            TS_ASSERT_EQUALS( it->_variable, i );
            TS_ASSERT_EQUALS( it->_value, eliminatedVariableValue );
            TS_ASSERT_EQUALS( it->_type, Tightening::LB );

            auto equations = split->getEquations();

            TS_ASSERT_EQUALS( equations.size(), 3U );

            auto cur = equations.begin();

            // Check equation f = b_i
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

            for ( unsigned j = 2; j < 5;  ++j )
            {
                // Check that there is bi >= bj for each couple (i, j) of input elements
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

        //
        // From here, the test is going to run tests for the case that f is included in elements.
        //
        elements.insert( f );
        MaxConstraint max2( f, elements );

        for ( unsigned i = 1; i < 6; ++i )
            max2.notifyVariableValue( i, i );

        // f = max(x_1 ... x_5)
        // f = 1
        max2.notifyVariableValue( f, 1 );

        TS_ASSERT( !max2.wereVariablesEliminated() );
        // Eliminate single variable x_5 = 5
        double eliminatedVariableValue2 = 5;
        max2.eliminateVariable( 5,eliminatedVariableValue2 );
        TS_ASSERT( max2.wereVariablesEliminated() );

        splits = max2.getCaseSplits();

        // There is one possible phase (because 7 is an element now)
        TS_ASSERT_EQUALS( splits.size(), 1U );

        split = splits.begin();

        List<Tightening> bounds = split->getBoundTightenings();

        // Check single bound f >= maxValueEliminated
        TS_ASSERT_EQUALS( bounds.size(), 1U );

        auto it = bounds.begin();

        TS_ASSERT_EQUALS( it->_variable, f );
        TS_ASSERT_EQUALS( it->_value, eliminatedVariableValue2 );
        TS_ASSERT_EQUALS( it->_type, Tightening::LB );

        auto equations = split->getEquations();

        // f >= x2, f >= x3, f >= x4
        TS_ASSERT_EQUALS( equations.size(), 3U );

        // First phase is the one without max=_maxEliminated
        auto cur = equations.begin();

        for ( unsigned i = 2; i < 5;  ++i )
        {
            // Check that there is f >= bj for each input element j
            TS_ASSERT_EQUALS( cur->_addends.size(), 2U );
            TS_ASSERT_EQUALS( cur->_scalar, 0.0 );
            TS_ASSERT_EQUALS( cur->_type, Equation::GE );

            auto addend = cur->_addends.begin();

            TS_ASSERT_EQUALS( addend->_coefficient, -1.0 );
            TS_ASSERT_EQUALS( addend->_variable, i );

            ++addend;

            TS_ASSERT_EQUALS( addend->_coefficient, 1.0 );
            TS_ASSERT_EQUALS( addend->_variable, f );

            ++cur;
        }
    }

    void test_max_phase_fixed_with_variable_elimination()
    {
        // For all the following tests the f variable is not in the input (in the next test it is)

        // Case 1
        // Check the case in which an input variable is the max, and higher than the eliminated one
        unsigned f = 1;
        Set<unsigned> elements;

        for ( unsigned i = 2; i < 6; ++i )
            elements.insert( i );

        MaxConstraint max( f, elements );

        // All input variables initially between 1 and 10
        for ( unsigned i = 2; i < 6; i++ ) {
            max.notifyUpperBound( i, 10 );
            max.notifyLowerBound( i, 1 );
        }

        TS_ASSERT( !max.wereVariablesEliminated() );
        // Eliminate single variable x_5 = 5.5
        max.eliminateVariable( 5, 5.5 );
        TS_ASSERT( max.wereVariablesEliminated() );

        TS_ASSERT( !max.phaseFixed() );

        // Set x_2 to be at least 6, and x_3 and x_4 to be at most 5
        max.notifyLowerBound( 2, 6 );
        for ( unsigned i = 3; i < 5; i++)
            max.notifyUpperBound( i, 5 );

        max.notifyVariableValue( 2, 7 );
        // Now, phase should be fixed to x_2; all other variables should be removed
        TS_ASSERT( max.phaseFixed() );
        TS_ASSERT_EQUALS( max.getParticipatingVariables().size(), 2U );

        // In the case f is no in the input elements, and variables were eliminated
        PiecewiseLinearCaseSplit validSplit = max.getValidCaseSplit();

        List<Tightening> bounds = validSplit.getBoundTightenings();

        // Check single bound f >= maxValueEliminated
        TS_ASSERT_EQUALS( bounds.size(), 1U );

        auto it = bounds.begin();

        TS_ASSERT_EQUALS( it->_variable, 2U );
        TS_ASSERT_EQUALS( it->_value, 5.5 );
        TS_ASSERT_EQUALS( it->_type, Tightening::LB );

        // The equation is: f=x_2
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

        ++cur;

        // Case 2
        // Check the case in which the eliminated variable is the maximum

        unsigned f2 = 1;
        Set<unsigned> elements2;

        for ( unsigned i = 2; i < 6; ++i )
            elements2.insert( i );

        MaxConstraint max2( f2, elements2 );

        // All input variables initially between 1 and 10
        for ( unsigned i = 2; i < 6; i++ ) {
            max2.notifyUpperBound( i, 10 );
            max2.notifyLowerBound( i, 1 );
        }

        TS_ASSERT( !max2.wereVariablesEliminated() );
        // Eliminate single variable x_5 = 9.5
        max2.eliminateVariable(5, 9.5 );
        TS_ASSERT( max2.wereVariablesEliminated() );

        TS_ASSERT( !max2.phaseFixed() );

        // Set x_2 to be at least 6, and x_3 and x_4 to be at most 5
        for ( unsigned i = 3; i < 5; i++ ) {
            max2.notifyUpperBound( i, 5 );
        }

        max2.notifyLowerBound( 2, 6 );
        // notifyUpperBound should eliminate x2 because UB_x2 <= _maxLowerBound = 9.5
        max2.notifyUpperBound( 2, 8 );


        // Now, phase should be fixed - there are no variables but there is an eliminatedValue
        TS_ASSERT( max2.phaseFixed() );
        // f is the single participating variable
        TS_ASSERT_EQUALS( max2.getParticipatingVariables().size(), 1U );

        // In the case f is not in the input elements, and variables were eliminated
        PiecewiseLinearCaseSplit validSplit2 = max2.getValidCaseSplit();

        bounds = validSplit2.getBoundTightenings();

        // f = maxValueEliminated
        // Check 2 bounds bound f >= maxValueEliminated, f <= maxValueEliminated
        TS_ASSERT_EQUALS( bounds.size(), 2U );

        it = bounds.begin();

        TS_ASSERT_EQUALS( it->_variable, f2 );
        TS_ASSERT_EQUALS( it->_value, 9.5 );
        TS_ASSERT_EQUALS( it->_type, Tightening::LB );

        it++;

        TS_ASSERT_EQUALS( it->_variable, f2 );
        TS_ASSERT_EQUALS( it->_value, 9.5 );
        TS_ASSERT_EQUALS( it->_type, Tightening::UB );

        // No additional equations
        auto equations2 = validSplit2.getEquations();

        TS_ASSERT_EQUALS( equations2.size(), 0U );

        // Case 3
        // Check the case in which the eliminated variable is the range of the single variable which is larger than the rest
        // (but may or may not be larger than the eliminated value)

        unsigned f3 = 1;
        Set<unsigned> elements3;

        for ( unsigned i = 2; i < 6; ++i )
            elements3.insert( i );

        MaxConstraint max3( f3, elements3 );

        // All input variables initially between 1 and 10
        for ( unsigned i = 2; i < 6; i++ ) {
            max3.notifyUpperBound( i, 10 );
            max3.notifyLowerBound( i, 1 );
        }

        TS_ASSERT( !max3.wereVariablesEliminated() );
        // Eliminate single variable x_5 = 7.5
        max3.eliminateVariable( 5, 7.5 );
        TS_ASSERT( max3.wereVariablesEliminated() );

        TS_ASSERT( !max3.phaseFixed() );

        // Set x_2 to be at least 6, and x_3 and x_4 to be at most 5
        for ( unsigned i = 3; i < 5; i++ ) {
            max3.notifyUpperBound( i, 5 );
        }

        max3.notifyLowerBound( 2, 6 );
        max3.notifyUpperBound( 2, 9 );

        // Now, phase should not be fixed! because maxEliminated = 7.5 in range of X_2 = [6, 9]
        TS_ASSERT( !max3.phaseFixed() );
    }

    void test_max_phase_fixed_with_variable_elimination_2()
    {
        // For all the following tests the f variable is in the input (unlike the previous test)

        // Case 1
        // Check the case in which an input variable is the max, and higher than the eliminated one
        unsigned f = 1;
        Set<unsigned> elements;

        for ( unsigned i = 1; i < 10; ++i )
            elements.insert( i );

        MaxConstraint max( f, elements );

        // All variables initially between 1 and 10
        for ( unsigned i = 1; i < 10; i++ )
        {
            max.notifyUpperBound( i, 10 );
            max.notifyLowerBound( i, 1 );
        }

        // Set x_2 and x_3 to be at least 6, others to be at most 5
        max.notifyLowerBound( 2, 6 );
        max.notifyLowerBound( 3, 6 );
        for( unsigned i = 5; i < 10; i++ )
            max.notifyUpperBound( i, 5 );
        max.notifyUpperBound( f, 5 );

        max.notifyVariableValue( f, 5);
        // Now, phase should be fixed to x_2 and x_3; all other variables should be removed
        TS_ASSERT( max.phaseFixed() );

        TS_ASSERT( !max.wereVariablesEliminated() );
        // Eliminate single variable x_4 = 5.5
        max.eliminateVariable( 4, 5.5 );
        TS_ASSERT( max.wereVariablesEliminated() );

        TS_ASSERT_EQUALS( max.getParticipatingVariables().size(), 3U );

        PiecewiseLinearCaseSplit validSplit = max.getValidCaseSplit();

        List<Tightening> bounds = validSplit.getBoundTightenings();

        // Check 3 bounds
        TS_ASSERT_EQUALS( bounds.size(), 3U );

        auto it = bounds.begin();

        // x2 <= UB(f)=5
        TS_ASSERT_EQUALS( it->_variable, 2U );
        TS_ASSERT_EQUALS( it->_value, 5 );
        TS_ASSERT_EQUALS( it->_type, Tightening::UB );

        it++;

        // x3 <= UB(f)=5
        TS_ASSERT_EQUALS( it->_variable, 3U );
        TS_ASSERT_EQUALS( it->_value, 5 );
        TS_ASSERT_EQUALS( it->_type, Tightening::UB );

        it++;

        // f >= maxValueEliminated = 5.5
        TS_ASSERT_EQUALS( it->_variable, f );
        TS_ASSERT_EQUALS( it->_value, 5.5 );
        TS_ASSERT_EQUALS( it->_type, Tightening::LB );

        auto equations = validSplit.getEquations();

        TS_ASSERT_EQUALS( equations.size(), 2U );

        auto cur = equations.begin();
        // f >= x_2
        TS_ASSERT_EQUALS( cur->_addends.size(), 2U );
        TS_ASSERT_EQUALS( cur->_scalar, 0.0 );
        TS_ASSERT_EQUALS( cur->_type, Equation::GE );

        auto addend = cur->_addends.begin();

        TS_ASSERT_EQUALS( addend->_coefficient, -1.0 );
        TS_ASSERT_EQUALS( addend->_variable, 2U );

        ++addend;

        TS_ASSERT_EQUALS( addend->_coefficient, 1.0 );
        TS_ASSERT_EQUALS( addend->_variable, f );

        ++cur;
        // f >= x_3
        TS_ASSERT_EQUALS( cur->_addends.size(), 2U );
        TS_ASSERT_EQUALS( cur->_scalar, 0.0 );
        TS_ASSERT_EQUALS( cur->_type, Equation::GE );

        addend = cur->_addends.begin();

        TS_ASSERT_EQUALS( addend->_coefficient, -1.0 );
        TS_ASSERT_EQUALS( addend->_variable, 3U );

        ++addend;

        TS_ASSERT_EQUALS( addend->_coefficient, 1.0 );
        TS_ASSERT_EQUALS( addend->_variable, f );


        // Case 2
        // The eliminated variable is larger than f
        unsigned f2 = 1;
        Set<unsigned> elements2;

        for ( unsigned i = 1; i < 10; ++i )
            elements2.insert( i );

        MaxConstraint max2( f2, elements2 );

        // All variables initially between 1 and 10
        for ( unsigned i = 1; i < 10; i++ )
        {
            max2.notifyUpperBound( i, 10 );
            max2.notifyLowerBound( i, 1 );
        }

        // Set x_2 and x_3 to be at least 6, others to be at most 5
        max2.notifyUpperBound( 2, 8 );
        max2.notifyLowerBound( 2, 6 );

        max2.notifyUpperBound( 3, 8 );
        max2.notifyLowerBound( 3, 6 );

        for( unsigned i = 5; i < 10; i++ )
            max2.notifyUpperBound( i, 5 );
        max2.notifyUpperBound( f2, 5 );

        max2.notifyVariableValue( f2, 5 );
        // Now, phase should be fixed to x_2 and x_3; all other variables should be removed
        TS_ASSERT( max2.phaseFixed() );

        TS_ASSERT( !max2.wereVariablesEliminated() );
        // Eliminate single variable x_4 = 9.5
        max2.eliminateVariable( 4, 9.5);
        TS_ASSERT( max2.wereVariablesEliminated() );

        TS_ASSERT_EQUALS( max2.getParticipatingVariables().size(), 3U );

        PiecewiseLinearCaseSplit validSplit2 = max2.getValidCaseSplit();

        bounds = validSplit2.getBoundTightenings();

        // Check 3 bounds
        TS_ASSERT_EQUALS( bounds.size(), 3U );

        it = bounds.begin();

        // x2 <= UB(f)=5
        TS_ASSERT_EQUALS( it->_variable, 2U );
        TS_ASSERT_EQUALS( it->_value, 5 );
        TS_ASSERT_EQUALS( it->_type, Tightening::UB );

        it++;

        // x3 <= UB(f)=5
        TS_ASSERT_EQUALS( it->_variable, 3U );
        TS_ASSERT_EQUALS( it->_value, 5 );
        TS_ASSERT_EQUALS( it->_type, Tightening::UB );

        it++;

        // f >= maxValueEliminated = 9.5
        TS_ASSERT_EQUALS( it->_variable, f2 );
        TS_ASSERT_EQUALS( it->_value, 9.5 );
        TS_ASSERT_EQUALS( it->_type, Tightening::LB );

        auto equations2 = validSplit2.getEquations();

        TS_ASSERT_EQUALS( equations2.size(), 2U );

        cur = equations2.begin();
        // f >= x_2
        TS_ASSERT_EQUALS( cur->_addends.size(), 2U );
        TS_ASSERT_EQUALS( cur->_scalar, 0.0 );
        TS_ASSERT_EQUALS( cur->_type, Equation::GE );

        addend = cur->_addends.begin();

        TS_ASSERT_EQUALS( addend->_coefficient, -1.0 );
        TS_ASSERT_EQUALS( addend->_variable, 2U );

        ++addend;

        TS_ASSERT_EQUALS( addend->_coefficient, 1.0 );
        TS_ASSERT_EQUALS( addend->_variable, f2 );

        ++cur;
        // f >= x_3
        TS_ASSERT_EQUALS( cur->_addends.size(), 2U );
        TS_ASSERT_EQUALS( cur->_scalar, 0.0 );
        TS_ASSERT_EQUALS( cur->_type, Equation::GE );

        addend = cur->_addends.begin();

        TS_ASSERT_EQUALS( addend->_coefficient, -1.0 );
        TS_ASSERT_EQUALS( addend->_variable, 3U );

        ++addend;

        TS_ASSERT_EQUALS( addend->_coefficient, 1.0 );
        TS_ASSERT_EQUALS( addend->_variable, f2 );

        // Case 3
        // The eliminated variable is in f's range
        unsigned f3 = 1;
        Set<unsigned> elements3;

        for ( unsigned i = 1; i < 10; ++i )
            elements3.insert( i );

        MaxConstraint max3( f3, elements3 );

        // All variables initially between 1 and 10
        for ( unsigned i = 1; i < 10; i++ )
        {
            max3.notifyUpperBound( i, 10 );
            max3.notifyLowerBound( i, 1 );
        }

        // Set x_2 and x_3 to be at least 6, others to be at most 5
        max3.notifyUpperBound( 2, 8 );
        max3.notifyLowerBound( 2, 6 );

        max3.notifyUpperBound( 3, 8 );
        max3.notifyLowerBound( 3, 6 );

        for( unsigned i = 5; i < 10; i++ )
            max3.notifyUpperBound( i, 5 );
        max3.notifyUpperBound( f3, 5 );

        max3.notifyVariableValue( f3, 5);
        // Now, phase should be fixed to x_2 and x_3; all other variables should be removed
        TS_ASSERT( max3.phaseFixed() );

        TS_ASSERT( !max3.wereVariablesEliminated() );
        // Eliminate single variable x_4 = 7
        max3.eliminateVariable( 4, 7 );
        TS_ASSERT( max3.wereVariablesEliminated() );

        TS_ASSERT_EQUALS( max3.getParticipatingVariables().size(), 3U );

        PiecewiseLinearCaseSplit validSplit3 = max3.getValidCaseSplit();

        bounds = validSplit3.getBoundTightenings();

        // Check 3 bounds
        TS_ASSERT_EQUALS( bounds.size(), 3U );

        it = bounds.begin();

        // x2 <= UB(f)=5
        TS_ASSERT_EQUALS( it->_variable, 2U );
        TS_ASSERT_EQUALS( it->_value, 5 );
        TS_ASSERT_EQUALS( it->_type, Tightening::UB );

        it++;

        // x3 <= UB(f)=5
        TS_ASSERT_EQUALS( it->_variable, 3U );
        TS_ASSERT_EQUALS( it->_value, 5 );
        TS_ASSERT_EQUALS( it->_type, Tightening::UB );

        it++;

        // f >= maxValueEliminated = 7
        TS_ASSERT_EQUALS( it->_variable, f3 );
        TS_ASSERT_EQUALS( it->_value, 7 );
        TS_ASSERT_EQUALS( it->_type, Tightening::LB );

        auto equations3 = validSplit3.getEquations();

        TS_ASSERT_EQUALS( equations3.size(), 2U );

        cur = equations3.begin();
        // f >= x_2
        TS_ASSERT_EQUALS( cur->_addends.size(), 2U );
        TS_ASSERT_EQUALS( cur->_scalar, 0.0 );
        TS_ASSERT_EQUALS( cur->_type, Equation::GE );

        addend = cur->_addends.begin();

        TS_ASSERT_EQUALS( addend->_coefficient, -1.0 );
        TS_ASSERT_EQUALS( addend->_variable, 2U );

        ++addend;

        TS_ASSERT_EQUALS( addend->_coefficient, 1.0 );
        TS_ASSERT_EQUALS( addend->_variable, f3 );

        ++cur;
        // f >= x_3
        TS_ASSERT_EQUALS( cur->_addends.size(), 2U );
        TS_ASSERT_EQUALS( cur->_scalar, 0.0 );
        TS_ASSERT_EQUALS( cur->_type, Equation::GE );

        addend = cur->_addends.begin();

        TS_ASSERT_EQUALS( addend->_coefficient, -1.0 );
        TS_ASSERT_EQUALS( addend->_variable, 3U );

        ++addend;

        TS_ASSERT_EQUALS( addend->_coefficient, 1.0 );
        TS_ASSERT_EQUALS( addend->_variable, f3 );
    }

    void test_get_entailed_tightenings_with_elimination()
    {
        unsigned f = 1;
        Set<unsigned> elements;

        elements.insert( 2 );
        elements.insert( 3 );
        elements.insert( 4 );

        MaxConstraint max( f, elements );

        // Eliminate variable x_4 = 2.5
        TS_ASSERT( !max.wereVariablesEliminated() );
        max.eliminateVariable( 4, 2.5 );
        TS_ASSERT( max.wereVariablesEliminated() );

        max.notifyLowerBound( 2, 1 );
        max.notifyUpperBound( 2, 8 );
        // No lower bound for 3
        max.notifyUpperBound( 3, 6 );

        List<Tightening> tightenings;
        TS_ASSERT_THROWS_NOTHING( max.getEntailedTightenings( tightenings ) );

        // Expect f to be in the range [1, 8]
        TS_ASSERT_EQUALS( tightenings.size(), 2U );
        auto it = tightenings.begin();

        // Update f_UB to have the value of max_(Element_UB) = 8 (=x_2_UB)
        TS_ASSERT_EQUALS( it->_variable, 1U );
        TS_ASSERT_EQUALS( it->_value, 8 );
        TS_ASSERT_EQUALS( it->_type, Tightening::UB );

        ++it;

        // Update f_LB to have the value of max_(Element_LB) = 2.5 (= maxValueEliminated)
        TS_ASSERT_EQUALS( it->_variable, 1U );
        TS_ASSERT_EQUALS( it->_value, 2.5 );
        TS_ASSERT_EQUALS( it->_type, Tightening::LB );

        //From here, the test is going to run tests for the case that f is included in elements.
        // also here - with a value eliminated

        Set<unsigned> elements2;
        elements2.insert( 1 );
        elements2.insert( 2 );
        elements2.insert( 3 );
        // add variable to be eliminated
        elements2.insert( 4 );

        MaxConstraint max2( f, elements2 );

        // No lower bound for 1
        max2.notifyUpperBound( 1, 7 );

        max2.notifyLowerBound( 2, 1 );
        max2.notifyUpperBound( 2, 8 );

        // No lower bound for 3
        max2.notifyUpperBound( 3, 6 );

        // Eliminate variable x_4 = 9.5
        TS_ASSERT( !max2.wereVariablesEliminated() );
        max2.eliminateVariable( 4, 9.5 );
        TS_ASSERT( max2.wereVariablesEliminated() );

        List<Tightening> tightenings2;
        TS_ASSERT_THROWS_NOTHING( max2.getEntailedTightenings( tightenings2 ) );

        // Expect f to be in the range [1, 7], x2 to be in the range [1, 7]
        TS_ASSERT_EQUALS( tightenings2.size(), 2U );
        it = tightenings2.begin();

        // Update X_2_UB = 7
        TS_ASSERT_EQUALS( it->_variable, 2U );
        TS_ASSERT_EQUALS( it->_value, 7 );
        TS_ASSERT_EQUALS( it->_type, Tightening::UB );

        ++it;

        // Update f_LB = 9.5
        TS_ASSERT_EQUALS( it->_variable, 1U );
        TS_ASSERT_EQUALS( it->_value, 9.5 );
        TS_ASSERT_EQUALS( it->_type, Tightening::LB );
    }

    void test_constraint_satisfied_after_elimination()
    {
        unsigned f = 1;
        Set<unsigned> elements;

        for ( unsigned i = 2; i < 10; ++i )
            elements.insert( i );

        MaxConstraint max( f, elements );

        List<unsigned> participatingVariables;
        TS_ASSERT_THROWS_NOTHING( participatingVariables = max.getParticipatingVariables() );
        TS_ASSERT_EQUALS( participatingVariables.size(), 9U );

        auto it = participatingVariables.begin();
        for ( unsigned i = 1; i < 10; ++i, ++it ) {
            TS_ASSERT( max.participatingVariable( i ) );
            max.notifyVariableValue( i, i );
        }

        // Constraint not satisfied
        TS_ASSERT(!max.satisfied());

        // Eliminate variable x_4 = 9.5
        TS_ASSERT( !max.wereVariablesEliminated() );
        max.eliminateVariable( 4, 9.5 );
        TS_ASSERT( max.wereVariablesEliminated() );

        // Constraint not satisfied
        max.notifyVariableValue( f, 9 );
        TS_ASSERT( !max.satisfied() );

        // Constraint satisfied
        max.notifyVariableValue( f, 9.5);
        TS_ASSERT( max.satisfied() );
    }

    void test_initialization_of_CDOs()
    {
        Context context;
        unsigned f = 1;
        Set<unsigned> elements;

        for ( unsigned i = 2; i < 10; ++i )
            elements.insert( i );

        MaxConstraint max( f, elements );

        TS_ASSERT_EQUALS( max.getContext(), static_cast<Context*>( nullptr ) );
        TS_ASSERT_EQUALS( max.getActiveStatusCDO(), static_cast<CDO<bool>*>( nullptr ) );
        TS_ASSERT_EQUALS( max.getPhaseStatusCDO(), static_cast<CDO<PhaseStatus>*>( nullptr ) );
        TS_ASSERT_EQUALS( max.getInfeasibleCasesCDList(), static_cast<CDList<PhaseStatus>*>( nullptr ) );
        TS_ASSERT_THROWS_NOTHING( max.initializeCDOs( &context ) );
        TS_ASSERT_EQUALS( max.getContext(), &context );
        TS_ASSERT_DIFFERS( max.getActiveStatusCDO(), static_cast<CDO<bool>*>( nullptr ) );
        TS_ASSERT_DIFFERS( max.getPhaseStatusCDO(), static_cast<CDO<PhaseStatus>*>( nullptr ) );
        TS_ASSERT_DIFFERS( max.getInfeasibleCasesCDList(), static_cast<CDList<PhaseStatus>*>( nullptr ) );

        bool active = false;
        TS_ASSERT_THROWS_NOTHING( active = max.isActive() );
        TS_ASSERT_EQUALS( active, true );

        bool phaseFixed = true;
        TS_ASSERT_THROWS_NOTHING( phaseFixed = max.phaseFixed() );
        TS_ASSERT_EQUALS( phaseFixed, PHASE_NOT_FIXED );
        TS_ASSERT_EQUALS( max.numFeasibleCases(), 8u );
    }

    /*
     * Test Case functionality of MaxConstraint
     * 1. Check that all cases are returned by MaxConstraint::getAllCases
     * 2. Check that MaxConstraint::getCaseSplit( case ) returns the correct case
     */
    void test_max_get_cases()
    {
        unsigned f = 1;
        Set<unsigned> elements;

        for ( unsigned i = 2; i < 10; ++i )
            elements.insert( i );

        MaxConstraint max( f, elements );

        List<PhaseStatus> cases = max.getAllCases();

        TS_ASSERT_EQUALS( cases.size(), elements.size() );

        for ( auto e : elements )
        {
            TS_ASSERT_DIFFERS( std::find( elements.begin(), elements.end(), e ), elements.end() );
        }

        List<PiecewiseLinearCaseSplit> splits = max.getCaseSplits();
        TS_ASSERT_EQUALS( splits.size(), elements.size() );

        PiecewiseLinearCaseSplit split;
        for ( auto e : elements )
        {
            split = max.getCaseSplit( static_cast<PhaseStatus>( e ) );
            TS_ASSERT_DIFFERS( std::find( splits.begin(), splits.end(), split ), splits.end() );
        }


        // TODO: Test cases when variables are eliminated
    }

    /*
      Test context-dependent Max state behavior.
     */
    void test_sign_context_dependent_state()
    {
        Context context;

        unsigned f = 1;
        Set<unsigned> elements;

        for ( unsigned i = 2; i < 10; ++i )
            elements.insert( i );

        TestMaxConstraint max( f, elements );


        max.initializeCDOs( &context );

        /* TS_ASSERT_EQUALS( sign.getPhaseStatus(), PHASE_NOT_FIXED ); */

        /* context.push(); */
        /* sign.notifyUpperBound( b, -1 ); */
        /* TS_ASSERT_EQUALS( sign.getPhaseStatus(), SIGN_PHASE_NEGATIVE ); */

        /* context.pop(); */
        /* TS_ASSERT_EQUALS( sign.getPhaseStatus(), PHASE_NOT_FIXED ); */
        /* context.push(); */

        /* sign.notifyLowerBound( b, 3 ); */
        /* TS_ASSERT_EQUALS( sign.getPhaseStatus(), SIGN_PHASE_POSITIVE ); */

        /* context.pop(); */
        /* TS_ASSERT_EQUALS( sign.getPhaseStatus(), PHASE_NOT_FIXED ); */
    }
};


//
// Local Variables:
// compile-command: "make -C ../../.. "
// tags-file-name: "../../../TAGS"
// c-basic-offset: 4
// End:
//
