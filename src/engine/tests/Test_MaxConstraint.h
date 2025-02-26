/*********************                                                        */
/*! \file Test_MaxConstraint.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Derek Huang, Shantanu Thakoor, Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2024 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

**/

#include "FloatUtils.h"
#include "MarabouError.h"
#include "MaxConstraint.h"
#include "MockTableau.h"
#include "PiecewiseLinearCaseSplit.h"
#include "Query.h"

#include <cxxtest/TestSuite.h>
#include <string.h>

class MockForMaxConstraint
{
public:
};

/*
   Exposes protected members of MaxConstraint for testing.
 */
class TestMaxConstraint : public MaxConstraint
{
public:
    TestMaxConstraint( unsigned f, const Set<unsigned> elements )
        : MaxConstraint( f, elements )
    {
    }

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

        Query ipq;
        ipq.setNumberOfVariables( 10 );

        MaxConstraint max( f, elements );
        MockTableau tableau;
        max.registerTableau( &tableau );

        List<unsigned> participatingVariables;
        TS_ASSERT_THROWS_NOTHING( participatingVariables = max.getParticipatingVariables() );
        TS_ASSERT_EQUALS( participatingVariables.size(), 9U );

        TS_ASSERT_THROWS_NOTHING( max.transformToUseAuxVariables( ipq ) );

        TS_ASSERT_THROWS_NOTHING( participatingVariables = max.getParticipatingVariables() );
        TS_ASSERT_EQUALS( participatingVariables.size(), 17U );

        auto it = participatingVariables.begin();
        for ( unsigned i = 1; i < 10; ++i, ++it )
            TS_ASSERT( max.participatingVariable( i ) );

        TS_ASSERT( !max.participatingVariable( 20 ) );
        TS_ASSERT( max.participatingVariable( 15 ) );
        TS_ASSERT( max.participatingVariable( 3 ) );
        TS_ASSERT( !max.participatingVariable( 0 ) );
        TS_ASSERT( max.participatingVariable( 1 ) );

        // f = max(x_2 ... x_9)
        // f = 10
        // x_8 = 5, the rest is 0

        tableau.setValue( f, 10 );
        for ( unsigned i = 2; i < 10; ++i )
            tableau.setValue( i, 0 );

        tableau.setValue( 8, 5 );

        TS_ASSERT( !max.satisfied() );

        // now f = 4
        tableau.setValue( f, 4 );

        TS_ASSERT( !max.satisfied() );

        // now x_8 = 4
        tableau.setValue( 8, 4 );

        TS_ASSERT( max.satisfied() );

        // now x_8 = 6
        tableau.setValue( 8, 6 );

        TS_ASSERT( !max.satisfied() );

        tableau.setValue( f, 6 );

        TS_ASSERT( max.satisfied() );

        tableau.setValue( 7, 12 );

        TS_ASSERT( !max.satisfied() );

        tableau.setValue( f, 12 );

        TS_ASSERT( max.satisfied() );

        //
        // From here, the test is going to run tests for the case that f is included in elements.
        //
        elements.insert( f );

        MaxConstraint max2( f, elements );
        max2.registerTableau( &tableau );

        TS_ASSERT_THROWS_NOTHING( participatingVariables = max2.getParticipatingVariables() );
        TS_ASSERT_EQUALS( participatingVariables.size(), 9U );

        it = participatingVariables.begin();
        for ( unsigned i = 1; i < 10; ++i, ++it )
            TS_ASSERT( max2.participatingVariable( i ) );

        TS_ASSERT( !max2.participatingVariable( 20 ) );
        TS_ASSERT( !max2.participatingVariable( 15 ) );

        // f = max(x_1 ... x_9)
        // f = 5
        // x_2..x7,x9 = 0
        // x_8 = 10

        tableau.setValue( f, 5 );
        for ( unsigned i = 1; i < 10; ++i )
            tableau.setValue( i, 0 );

        tableau.setValue( 8, 10 );
        TS_ASSERT( !max2.satisfied() );

        // now f = 4
        tableau.setValue( f, 4 );

        TS_ASSERT( !max2.satisfied() );

        // now x_8 = 4
        tableau.setValue( 8, 4 );

        TS_ASSERT( max2.satisfied() );

        // now x_8 = 6
        tableau.setValue( 8, 6 );

        TS_ASSERT( !max2.satisfied() );

        // now f = 6
        tableau.setValue( f, 6 );

        TS_ASSERT( max2.satisfied() );

        // now x_7 = 12
        tableau.setValue( 7, 12 );

        TS_ASSERT( !max2.satisfied() );

        // now f = 12
        tableau.setValue( f, 12 );

        TS_ASSERT( max2.satisfied() );

        // now x_6 = 13
        tableau.setValue( 6, 13 );

        TS_ASSERT( !max2.satisfied() );

        // now f = 14
        tableau.setValue( f, 14 );

        TS_ASSERT( max2.satisfied() );
    }

    void test_f_in_input_var()
    {
        unsigned f = 1;
        Set<unsigned> elements;

        for ( unsigned i = 1; i < 10; ++i )
            elements.insert( i );

        MaxConstraint max( f, elements );

        Query ipq;
        ipq.setNumberOfVariables( 10 );
        TS_ASSERT_EQUALS( ipq.getNumberOfVariables(), 10u );
        TS_ASSERT_EQUALS( ipq.getEquations().size(), 0u );
        TS_ASSERT_THROWS_NOTHING( max.transformToUseAuxVariables( ipq ) );
        TS_ASSERT_EQUALS( ipq.getNumberOfVariables(), 18u );
        TS_ASSERT_EQUALS( ipq.getEquations().size(), 8u ); // f >= x2...x9

        // Check that correct equations are added to input query
        // And aux variables have correct lower bounds (0)
        auto equation = ipq.getEquations().begin();
        for ( unsigned i = 0; i < 8; ++i )
        {
            Equation eq;
            eq.addAddend( 1, 1 );
            eq.addAddend( -1, i + 2 );  // max input
            eq.addAddend( -1, 10 + i ); // aux
            eq._scalar = 0;
            TS_ASSERT_EQUALS( eq, *equation );
            TS_ASSERT_EQUALS( ipq.getLowerBound( 10 + i ), 0 );

            ++equation;
        }

        TS_ASSERT( max.constraintObsolete() );
    }

    void test_max_case_splits()
    {
        unsigned f = 1;
        Set<unsigned> elements;

        for ( unsigned i = 2; i < 10; ++i )
            elements.insert( i );

        MaxConstraint max( f, elements );

        Query ipq;
        ipq.setNumberOfVariables( 10 );
        TS_ASSERT_THROWS_NOTHING( max.transformToUseAuxVariables( ipq ) );

        TS_ASSERT_EQUALS( ipq.getNumberOfVariables(), 18u );

        // f = max(x_2 ... x_9)
        // f = 1

        List<PiecewiseLinearCaseSplit> splits = max.getCaseSplits();

        // there are 8 possible phases
        TS_ASSERT_EQUALS( splits.size(), 8U );

        auto split = splits.begin();
        for ( unsigned i = 0; i < splits.size(); ++i, ++split )
        {
            unsigned aux = 10 + i;
            List<Tightening> bounds = split->getBoundTightenings();

            // Since no upper bounds known for any of the variables, no bounds
            TS_ASSERT_EQUALS( bounds.size(), 1U );
            TS_ASSERT_EQUALS( *bounds.begin(), Tightening( aux, 0, Tightening::UB ) );

            auto equations = split->getEquations();
            TS_ASSERT_EQUALS( equations.size(), 0U );
        }
    }

    void test_max_phase_fixed()
    {
        unsigned f = 1;
        Set<unsigned> elements;

        for ( unsigned i = 2; i < 10; ++i )
            elements.insert( i );

        MaxConstraint max( f, elements );

        Query ipq;
        ipq.setNumberOfVariables( 10 );
        TS_ASSERT_THROWS_NOTHING( max.transformToUseAuxVariables( ipq ) );

        TS_ASSERT_EQUALS( ipq.getNumberOfVariables(), 18u );

        // all variables initially between 1 and 10
        for ( unsigned i = 2; i < 10; i++ )
        {
            max.notifyUpperBound( i, 10 );
            max.notifyLowerBound( i, 1 );
        }

        TS_ASSERT( !max.phaseFixed() );

        // set x_2 to be at least 6, others to be at most 5
        max.notifyLowerBound( 2, 6 );
        for ( unsigned i = 3; i < 10; i++ )
            max.notifyUpperBound( i, 5 );

        // now, phase should be fixed to x_2; all other variables should be removed
        TS_ASSERT( max.phaseFixed() );
        TS_ASSERT_EQUALS( max.getParticipatingVariables().size(), 3U );

        PiecewiseLinearCaseSplit validSplit;
        TS_ASSERT_THROWS_NOTHING( validSplit = max.getValidCaseSplit() );
        auto equations = validSplit.getEquations();
        TS_ASSERT_EQUALS( equations.size(), 0U );

        auto bounds = validSplit.getBoundTightenings();
        TS_ASSERT_EQUALS( bounds.size(), 1U );
        // The aux var to the first max case is 10
        TS_ASSERT_EQUALS( *bounds.begin(), Tightening( 10, 0, Tightening::UB ) );
    }

    void test_max_var_elims()
    {
        unsigned f = 1;
        Set<unsigned> elements;

        elements.insert( 2 );
        elements.insert( 3 );

        MaxConstraint max( f, elements );
        Query ipq;
        ipq.setNumberOfVariables( 4 );
        TS_ASSERT_THROWS_NOTHING( max.transformToUseAuxVariables( ipq ) );

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
        Query ipq;
        ipq.setNumberOfVariables( 4 );
        TS_ASSERT_THROWS_NOTHING( max.transformToUseAuxVariables( ipq ) );

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
        for ( unsigned i = 3; i < 10; i++ )
            max2.eliminateVariable( i, 0 );

        TS_ASSERT( !max2.constraintObsolete() );
        max2.eliminateVariable( 1, 0 );
        TS_ASSERT( max2.constraintObsolete() );

        elements.insert( 1 );
        MaxConstraint max3( f, elements );
        Query ipq;
        ipq.setNumberOfVariables( 10 );
        TS_ASSERT_THROWS_NOTHING( max3.transformToUseAuxVariables( ipq ) );

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
        MaxConstraint recoveredMax( originalSerialized );

        TS_ASSERT_EQUALS( originalMax.serializeToString(), recoveredMax.serializeToString() );
    }

    void test_serialize_and_unserialize_2()
    {
        unsigned f = 17;
        Set<unsigned> elements;

        for ( unsigned i = 0; i < 4; ++i )
            elements.insert( i );

        MaxConstraint originalMax( f, elements );

        TS_ASSERT_THROWS_NOTHING( originalMax.eliminateVariable( 2, 12 ) );
        TS_ASSERT_THROWS_NOTHING( originalMax.eliminateVariable( 1, 11 ) );
        TS_ASSERT_THROWS_NOTHING( originalMax.eliminateVariable( 3, 13 ) );

        String originalSerialized = originalMax.serializeToString();

        MaxConstraint recoveredMax( originalSerialized );

        TS_ASSERT_EQUALS( originalMax.serializeToString(), recoveredMax.serializeToString() );
    }

    void test_phase_fixed_with_variable_elimination()
    {
        // Check phase fixed after eliminating - and upper bound updated
        unsigned f = 4;
        Set<unsigned> elements;

        for ( unsigned i = 0; i < 4; ++i )
            elements.insert( i );

        MaxConstraint max( f, elements );
        Query ipq;
        ipq.setNumberOfVariables( 5 );
        TS_ASSERT_THROWS_NOTHING( max.transformToUseAuxVariables( ipq ) );
        TS_ASSERT_EQUALS( ipq.getNumberOfVariables(), 9u );

        TS_ASSERT( !max.getAllCases().exists( MAX_PHASE_ELIMINATED ) );
        max.eliminateVariable( 1, -11 );
        TS_ASSERT( max.getAllCases().exists( MAX_PHASE_ELIMINATED ) );
        max.eliminateVariable( 2, 12 );
        max.eliminateVariable( 3, 13 );
        // 13 is the maximum value eliminated

        max.notifyLowerBound( 0, 1 );
        max.notifyUpperBound( 0, 14 );
        TS_ASSERT( !max.phaseFixed() )

        max.notifyUpperBound( 0, 12 );
        TS_ASSERT( max.phaseFixed() );

        // Check phase fixed after eliminating - and lower bound updated
        unsigned f2 = 44;
        Set<unsigned> elements2;

        for ( unsigned i = 0; i < 4; ++i )
            elements2.insert( i );

        MaxConstraint max2( f2, elements2 );
        Query ipq2;
        ipq2.setNumberOfVariables( 45 );
        TS_ASSERT_THROWS_NOTHING( max2.transformToUseAuxVariables( ipq2 ) );
        TS_ASSERT_EQUALS( ipq2.getNumberOfVariables(), 49u );

        TS_ASSERT( !max2.getAllCases().exists( MAX_PHASE_ELIMINATED ) );
        max2.eliminateVariable( 1, -11 );
        TS_ASSERT( max2.getAllCases().exists( MAX_PHASE_ELIMINATED ) );

        max2.eliminateVariable( 2, 12 );
        max2.eliminateVariable( 3, 13 );
        TS_ASSERT( max2.getAllCases().exists( MAX_PHASE_ELIMINATED ) );

        max2.notifyLowerBound( 0, 12.5 );
        max2.notifyUpperBound( 0, 13.5 );
        TS_ASSERT( !max2.phaseFixed() )

        max2.notifyLowerBound( 0, 13.1 );
        TS_ASSERT( max2.phaseFixed() );
    }

    void test_phase_fixed_with_variable_elimination_and_bound_manager()
    {
        // Check phase fixed after eliminating - and upper bound updated
        unsigned f = 4;
        Set<unsigned> elements;

        for ( unsigned i = 0; i < 4; ++i )
            elements.insert( i );

        MaxConstraint max( f, elements );
        Query ipq;
        ipq.setNumberOfVariables( 5 );
        TS_ASSERT_THROWS_NOTHING( max.transformToUseAuxVariables( ipq ) );
        TS_ASSERT_EQUALS( ipq.getNumberOfVariables(), 9u );

        Context context;
        BoundManager boundManager( context );
        boundManager.initialize( 9 );
        max.registerBoundManager( &boundManager );

        TS_ASSERT( !max.getAllCases().exists( MAX_PHASE_ELIMINATED ) );
        max.eliminateVariable( 1, -11 );
        TS_ASSERT( max.getAllCases().exists( MAX_PHASE_ELIMINATED ) );
        max.eliminateVariable( 2, 12 );
        max.eliminateVariable( 3, 13 );
        // 13 is the maximum value eliminated

        max.notifyLowerBound( 0, 1 );
        max.notifyUpperBound( 0, 14 );
        TS_ASSERT( !max.phaseFixed() )

        max.notifyUpperBound( 0, 12 );
        TS_ASSERT( max.phaseFixed() );

        // Check phase fixed after eliminating - and lower bound updated
        unsigned f2 = 44;
        Set<unsigned> elements2;

        for ( unsigned i = 0; i < 4; ++i )
            elements2.insert( i );

        MaxConstraint max2( f2, elements2 );
        Query ipq2;
        ipq2.setNumberOfVariables( 45 );
        TS_ASSERT_THROWS_NOTHING( max2.transformToUseAuxVariables( ipq2 ) );
        TS_ASSERT_EQUALS( ipq2.getNumberOfVariables(), 49u );

        Context context2;
        BoundManager boundManager2( context2 );
        boundManager2.initialize( 49 );
        max2.registerBoundManager( &boundManager2 );

        TS_ASSERT( !max2.getAllCases().exists( MAX_PHASE_ELIMINATED ) );
        max2.eliminateVariable( 1, -11 );
        TS_ASSERT( max2.getAllCases().exists( MAX_PHASE_ELIMINATED ) );

        max2.eliminateVariable( 2, 12 );
        max2.eliminateVariable( 3, 13 );
        TS_ASSERT( max2.getAllCases().exists( MAX_PHASE_ELIMINATED ) );

        max2.notifyLowerBound( 0, 12.5 );
        max2.notifyUpperBound( 0, 13.5 );
        TS_ASSERT( !max2.phaseFixed() )

        max2.notifyLowerBound( 0, 13.1 );
        TS_ASSERT( max2.phaseFixed() );
    }

    void test_max_case_splits_with_variable_elimination()
    {
        unsigned f = 1;
        Set<unsigned> elements;

        for ( unsigned i = 2; i < 6; ++i )
            elements.insert( i );

        MaxConstraint max( f, elements );
        Query ipq;
        ipq.setNumberOfVariables( 6 );
        TS_ASSERT_THROWS_NOTHING( max.transformToUseAuxVariables( ipq ) );
        TS_ASSERT_EQUALS( ipq.getNumberOfVariables(), 10u );

        // f = max(x_2 ... x_5)
        // f = 1

        TS_ASSERT( !max.getAllCases().exists( MAX_PHASE_ELIMINATED ) );
        // Eliminate single variable x_5 = 5
        unsigned eliminatedVariableValue = 5;
        max.eliminateVariable( 5, eliminatedVariableValue );
        TS_ASSERT( max.getAllCases().exists( MAX_PHASE_ELIMINATED ) );

        List<PiecewiseLinearCaseSplit> splits = max.getCaseSplits();

        // There are 4 possible phases: f1=b2, f1=b3, f1=b4, f1=maxValueOfEliminated
        TS_ASSERT_EQUALS( splits.size(), 4U );

        // The cases are x6 <= 0, x7 <=0, x8 <=0, f1 = 5
        auto split = splits.begin();
        {
            List<Tightening> bounds = split->getBoundTightenings();
            TS_ASSERT_EQUALS( bounds.size(), 1U );
            TS_ASSERT_EQUALS( *bounds.begin(), Tightening( 6, 0, Tightening::UB ) );
            TS_ASSERT_EQUALS( split->getEquations().size(), 0U );
        }
        ++split;
        {
            List<Tightening> bounds = split->getBoundTightenings();
            TS_ASSERT_EQUALS( bounds.size(), 1U );
            TS_ASSERT_EQUALS( *bounds.begin(), Tightening( 7, 0, Tightening::UB ) );
            TS_ASSERT_EQUALS( split->getEquations().size(), 0U );
        }
        ++split;
        {
            List<Tightening> bounds = split->getBoundTightenings();
            TS_ASSERT_EQUALS( bounds.size(), 1U );
            TS_ASSERT_EQUALS( *bounds.begin(), Tightening( 8, 0, Tightening::UB ) );
            TS_ASSERT_EQUALS( split->getEquations().size(), 0U );
        }
        ++split;
        {
            List<Tightening> bounds = split->getBoundTightenings();
            TS_ASSERT_EQUALS( bounds.size(), 2U );
            TS_ASSERT_EQUALS( *bounds.begin(), Tightening( f, 5, Tightening::LB ) );
            TS_ASSERT_EQUALS( *( ++bounds.begin() ), Tightening( f, 5, Tightening::UB ) );
            TS_ASSERT_EQUALS( split->getEquations().size(), 0U );
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
        Query ipq;
        ipq.setNumberOfVariables( 6 );
        TS_ASSERT_THROWS_NOTHING( max.transformToUseAuxVariables( ipq ) );
        TS_ASSERT_EQUALS( ipq.getNumberOfVariables(), 10u );

        // All input variables initially between 1 and 10
        for ( unsigned i = 2; i < 6; i++ )
        {
            max.notifyUpperBound( i, 10 );
            max.notifyLowerBound( i, 1 );
        }

        TS_ASSERT( !max.getAllCases().exists( MAX_PHASE_ELIMINATED ) );
        // Eliminate single variable x_5 = 5.5
        max.eliminateVariable( 5, 5.5 );
        TS_ASSERT( max.getAllCases().exists( MAX_PHASE_ELIMINATED ) );

        TS_ASSERT( !max.phaseFixed() );

        // Set x_2 to be at least 6, and x_3 and x_4 to be at most 5
        max.notifyLowerBound( 2, 6 );

        TS_ASSERT( !max.getAllCases().exists( MAX_PHASE_ELIMINATED ) );

        for ( unsigned i = 3; i < 5; i++ )
            max.notifyUpperBound( i, 5 );

        // Now, phase should be fixed to x_2; all other variables should be removed
        TS_ASSERT( max.phaseFixed() );
        TS_ASSERT_EQUALS( max.getParticipatingVariables().size(), 3U );

        // In the case f is no in the input elements, and variables were eliminated
        PiecewiseLinearCaseSplit validSplit = max.getValidCaseSplit();

        List<Tightening> bounds = validSplit.getBoundTightenings();

        // Check single bound f >= maxValueEliminated
        TS_ASSERT_EQUALS( bounds.size(), 1U );

        auto it = bounds.begin();

        TS_ASSERT_EQUALS( it->_variable, 6U );
        TS_ASSERT_EQUALS( it->_value, 0 );
        TS_ASSERT_EQUALS( it->_type, Tightening::UB );

        // Case 2
        // Check the case in which the eliminated variable is the maximum

        unsigned f2 = 1;
        Set<unsigned> elements2;

        for ( unsigned i = 2; i < 6; ++i )
            elements2.insert( i );

        MaxConstraint max2( f2, elements2 );
        Query ipq2;
        ipq2.setNumberOfVariables( 6 );
        TS_ASSERT_THROWS_NOTHING( max2.transformToUseAuxVariables( ipq2 ) );
        TS_ASSERT_EQUALS( ipq2.getNumberOfVariables(), 10u );

        // All input variables initially between 1 and 10
        for ( unsigned i = 2; i < 6; i++ )
        {
            max2.notifyUpperBound( i, 10 );
            max2.notifyLowerBound( i, 1 );
        }

        TS_ASSERT( !max2.getAllCases().exists( MAX_PHASE_ELIMINATED ) );
        // Eliminate single variable x_5 = 9.5
        max2.eliminateVariable( 5, 9.5 );
        TS_ASSERT( max2.getAllCases().exists( MAX_PHASE_ELIMINATED ) );

        TS_ASSERT( !max2.phaseFixed() );

        // Set x_2 to be at least 6, and x_3 and x_4 to be at most 5
        for ( unsigned i = 3; i < 5; i++ )
        {
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
        // Check the case in which the eliminated variable is the range of the single variable which
        // is larger than the rest (but may or may not be larger than the eliminated value)

        unsigned f3 = 1;
        Set<unsigned> elements3;

        for ( unsigned i = 2; i < 6; ++i )
            elements3.insert( i );

        MaxConstraint max3( f3, elements3 );
        Query ipq3;
        ipq3.setNumberOfVariables( 6 );
        TS_ASSERT_THROWS_NOTHING( max3.transformToUseAuxVariables( ipq3 ) );
        TS_ASSERT_EQUALS( ipq3.getNumberOfVariables(), 10u );

        // All input variables initially between 1 and 10
        for ( unsigned i = 2; i < 6; i++ )
        {
            max3.notifyUpperBound( i, 10 );
            max3.notifyLowerBound( i, 1 );
        }

        TS_ASSERT( !max3.getAllCases().exists( MAX_PHASE_ELIMINATED ) );
        // Eliminate single variable x_5 = 7.5
        max3.eliminateVariable( 5, 7.5 );
        TS_ASSERT( max3.getAllCases().exists( MAX_PHASE_ELIMINATED ) );

        TS_ASSERT( !max3.phaseFixed() );

        // Set x_2 to be at least 6, and x_3 and x_4 to be at most 5
        for ( unsigned i = 3; i < 5; i++ )
        {
            max3.notifyUpperBound( i, 5 );
        }

        max3.notifyLowerBound( 2, 6 );
        max3.notifyUpperBound( 2, 9 );

        // Now, phase should not be fixed! because maxEliminated = 7.5 in range of X_2 = [6, 9]
        TS_ASSERT( !max3.phaseFixed() );
        TS_ASSERT( max3.getAllCases().exists( MAX_PHASE_ELIMINATED ) );
    }

    void test_get_entailed_tightenings_with_elimination()
    {
        unsigned f = 1;
        Set<unsigned> elements;

        elements.insert( 2 );
        elements.insert( 3 );
        elements.insert( 4 );

        MaxConstraint max( f, elements );
        Query ipq;
        ipq.setNumberOfVariables( 5 );
        TS_ASSERT_THROWS_NOTHING( max.transformToUseAuxVariables( ipq ) );
        TS_ASSERT_EQUALS( ipq.getNumberOfVariables(), 8u );

        // Eliminate variable x_4 = 2.5
        TS_ASSERT( !max.getAllCases().exists( MAX_PHASE_ELIMINATED ) );
        max.eliminateVariable( 4, 2.5 );
        TS_ASSERT( max.getAllCases().exists( MAX_PHASE_ELIMINATED ) );

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
    }

    void test_constraint_satisfied_after_elimination()
    {
        unsigned f = 1;
        Set<unsigned> elements;

        for ( unsigned i = 2; i < 10; ++i )
            elements.insert( i );

        MaxConstraint max( f, elements );
        MockTableau tableau;
        max.registerTableau( &tableau );
        Query ipq;
        ipq.setNumberOfVariables( 10 );
        TS_ASSERT_THROWS_NOTHING( max.transformToUseAuxVariables( ipq ) );
        TS_ASSERT_EQUALS( ipq.getNumberOfVariables(), 18u );

        List<unsigned> participatingVariables;
        TS_ASSERT_THROWS_NOTHING( participatingVariables = max.getParticipatingVariables() );
        TS_ASSERT_EQUALS( participatingVariables.size(), 17U );

        auto it = participatingVariables.begin();
        for ( unsigned i = 1; i < 10; ++i, ++it )
        {
            TS_ASSERT( max.participatingVariable( i ) );
            tableau.setValue( i, i );
        }

        // Constraint not satisfied
        TS_ASSERT( !max.satisfied() );
        // Eliminate variable x_4 = 9.5
        TS_ASSERT( !max.getAllCases().exists( MAX_PHASE_ELIMINATED ) );
        max.eliminateVariable( 4, 9.5 );
        TS_ASSERT( max.getAllCases().exists( MAX_PHASE_ELIMINATED ) );

        // Constraint not satisfied
        TS_ASSERT( !max.satisfied() );
        tableau.setValue( f, 9 );
        TS_ASSERT( !max.satisfied() );

        // Constraint satisfied
        tableau.setValue( f, 9.5 );
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
        Query ipq;
        ipq.setNumberOfVariables( 10 );
        TS_ASSERT_THROWS_NOTHING( max.transformToUseAuxVariables( ipq ) );

        TS_ASSERT_EQUALS( max.getContext(), static_cast<Context *>( nullptr ) );
        TS_ASSERT_EQUALS( max.getActiveStatusCDO(), static_cast<CDO<bool> *>( nullptr ) );
        TS_ASSERT_EQUALS( max.getPhaseStatusCDO(), static_cast<CDO<PhaseStatus> *>( nullptr ) );
        TS_ASSERT_EQUALS( max.getInfeasibleCasesCDList(),
                          static_cast<CDList<PhaseStatus> *>( nullptr ) );
        TS_ASSERT_THROWS_NOTHING( max.initializeCDOs( &context ) );
        TS_ASSERT_EQUALS( max.getContext(), &context );
        TS_ASSERT_DIFFERS( max.getActiveStatusCDO(), static_cast<CDO<bool> *>( nullptr ) );
        TS_ASSERT_DIFFERS( max.getPhaseStatusCDO(), static_cast<CDO<PhaseStatus> *>( nullptr ) );
        TS_ASSERT_DIFFERS( max.getInfeasibleCasesCDList(),
                           static_cast<CDList<PhaseStatus> *>( nullptr ) );

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
        Query ipq;
        ipq.setNumberOfVariables( 10 );
        TS_ASSERT_THROWS_NOTHING( max.transformToUseAuxVariables( ipq ) );

        List<PhaseStatus> cases = max.getAllCases();

        TS_ASSERT_EQUALS( cases.size(), elements.size() );

        List<PiecewiseLinearCaseSplit> splits = max.getCaseSplits();
        TS_ASSERT_EQUALS( splits.size(), elements.size() );

        PiecewiseLinearCaseSplit split;
        for ( auto c : cases )
        {
            split = max.getCaseSplit( c );
            TS_ASSERT( splits.exists( split ) );
        }

        // TODO: Test cases when variables are eliminated
    }

    /*
       Test context-dependent Max state behavior.
     */
    void test_max_context_dependent_state()
    {
        Context context;

        unsigned f = 1;
        Set<unsigned> elements;

        for ( unsigned i = 2; i < 5; ++i )
            elements.insert( i );

        TestMaxConstraint max( f, elements );

        Query ipq;
        ipq.setNumberOfVariables( 5 );
        TS_ASSERT_THROWS_NOTHING( max.transformToUseAuxVariables( ipq ) );


        // In search phase, we initialize context-dependent structures
        max.initializeCDOs( &context );
        context.push();

        TS_ASSERT_EQUALS( max.getPhaseStatus(), PHASE_NOT_FIXED );

        max.notifyLowerBound( 4, 7 );
        max.notifyLowerBound( 3, 5 );
        max.notifyUpperBound( 2, 4 ); // This should eliminate variable 2;

        TS_ASSERT_EQUALS( max.numFeasibleCases(), 2u );

        context.push();               // L1
        max.notifyUpperBound( 3, 6 ); // This should eliminate variable 3;
        TS_ASSERT( max.isImplication() );

        context.pop(); // L0
        TS_ASSERT( !max.isImplication() );

        context.push(); // L1
        PhaseStatus phase = max.nextFeasibleCase();
        max.markInfeasible( phase );
        TS_ASSERT( max.isImplication() );

        context.pop(); // L0
        TS_ASSERT( !max.isImplication() );
        phase = max.nextFeasibleCase();
        max.markInfeasible( phase );

        TS_ASSERT( max.isImplication() );

        phase = max.nextFeasibleCase();
        max.markInfeasible( phase );
        TS_ASSERT( !max.isFeasible() );

        context.pop(); // L1

        TS_ASSERT_EQUALS( max.getPhaseStatus(), PHASE_NOT_FIXED );

        context.pop(); // L0
    }

    void test_max_context_dependent_state_with_elimination()
    {
        // Case 2 - with eliminated variables during pre-processing
        Context context;

        unsigned f = 1;
        Set<unsigned> elements;

        for ( unsigned i = 2; i < 6; ++i )
            elements.insert( i );

        TestMaxConstraint max( f, elements );
        Query ipq;
        ipq.setNumberOfVariables( 6 );
        TS_ASSERT_THROWS_NOTHING( max.transformToUseAuxVariables( ipq ) );
        TS_ASSERT_EQUALS( ipq.getNumberOfVariables(), 10u );

        BoundManager bm( context );
        bm.initialize( 10 );
        max.registerBoundManager( &bm );

        // During pre-processing we eliminate some variables
        max.eliminateVariable( 5, 7 );

        // In search phase, we initialize context-dependent structures
        max.initializeCDOs( &context );
        context.push();

        TS_ASSERT_EQUALS( max.getPhaseStatus(), PHASE_NOT_FIXED );

        max.notifyLowerBound( 4, 6 );
        max.notifyLowerBound( 3, 5 );
        max.notifyUpperBound( 2, 4 ); // This should eliminate variable 2;

        TS_ASSERT_EQUALS( max.numFeasibleCases(), 3u );

        bm.storeLocalBounds();
        context.push();               // L1
        max.notifyUpperBound( 3, 5 ); // This should eliminate variable 3;
        TS_ASSERT_EQUALS( max.numFeasibleCases(), 2u );

        bm.storeLocalBounds();
        context.push(); // L2
        PhaseStatus phase = max.nextFeasibleCase();
        TS_ASSERT_DIFFERS( phase, MAX_PHASE_ELIMINATED );

        TS_ASSERT( !max.isImplication() );
        max.notifyUpperBound( 4, 6 ); // This should eliminate variable 4;
        TS_ASSERT( max.isImplication() );
        TS_ASSERT_EQUALS( max.nextFeasibleCase(), MAX_PHASE_ELIMINATED );

        context.pop(); // L1
        bm.restoreLocalBounds();
        TS_ASSERT( !max.isImplication() );
        max.notifyUpperBound( 4, 6 ); // This should eliminate variable 4;
        TS_ASSERT( max.isImplication() );

        phase = max.nextFeasibleCase();
        TS_ASSERT_EQUALS( phase, MAX_PHASE_ELIMINATED );

        max.markInfeasible( phase );
        TS_ASSERT( !max.isFeasible() );

        context.pop(); // L1
        bm.restoreLocalBounds();

        TS_ASSERT_EQUALS( max.getPhaseStatus(), PHASE_NOT_FIXED );
    }

    void test_get_cost_function_component()
    {
        unsigned f = 1;
        Set<unsigned> elements;

        for ( unsigned i = 2; i < 5; ++i )
            elements.insert( i );

        MaxConstraint max1( f, elements );
        MockTableau tableau;
        max1.registerTableau( &tableau );

        Query ipq;
        ipq.setNumberOfVariables( 5 );
        TS_ASSERT_THROWS_NOTHING( max1.transformToUseAuxVariables( ipq ) );
        TS_ASSERT_EQUALS( ipq.getNumberOfVariables(), 8u );

        // element to aux: 2: 5, 3: 6, 4: 7.

        TS_ASSERT_THROWS_NOTHING( max1.notifyLowerBound( 1, 1 ) );
        TS_ASSERT_THROWS_NOTHING( max1.notifyUpperBound( 1, 2 ) );
        TS_ASSERT_THROWS_NOTHING( max1.notifyLowerBound( 2, 0 ) );
        TS_ASSERT_THROWS_NOTHING( max1.notifyUpperBound( 2, 0.9 ) );
        TS_ASSERT_THROWS_NOTHING( max1.notifyLowerBound( 3, 1 ) );
        TS_ASSERT_THROWS_NOTHING( max1.notifyUpperBound( 3, 2 ) );
        TS_ASSERT_THROWS_NOTHING( max1.notifyLowerBound( 4, 0 ) );
        TS_ASSERT_THROWS_NOTHING( max1.notifyUpperBound( 4, 0.9 ) );
        TS_ASSERT_THROWS_NOTHING( max1.notifyLowerBound( 5, 0 ) );
        TS_ASSERT_THROWS_NOTHING( max1.notifyUpperBound( 5, 2 ) );
        TS_ASSERT_THROWS_NOTHING( max1.notifyLowerBound( 6, 0 ) );
        TS_ASSERT_THROWS_NOTHING( max1.notifyUpperBound( 6, 1 ) );
        TS_ASSERT_THROWS_NOTHING( max1.notifyLowerBound( 7, 0 ) );
        TS_ASSERT_THROWS_NOTHING( max1.notifyUpperBound( 7, 2 ) );


        tableau.setValue( 1, 2 );
        tableau.setValue( 2, 0 );
        tableau.setValue( 3, 1.5 );
        tableau.setValue( 4, 0.5 );
        tableau.setValue( 5, 2 );
        tableau.setValue( 6, 0.5 );
        tableau.setValue( 7, 1.5 );

        // Phase fixed, should not add any cost terms.
        TS_ASSERT( max1.phaseFixed() );
        LinearExpression cost1;
        TS_ASSERT_THROWS_NOTHING( max1.getCostFunctionComponent( cost1, MAX_PHASE_ELIMINATED ) );
        TS_ASSERT_EQUALS( cost1._addends.size(), 0u );
        TS_ASSERT_EQUALS( cost1._constant, 0 );

        MaxConstraint max2( f, elements );
        max2.registerTableau( &tableau );

        TS_ASSERT_THROWS_NOTHING( max2.transformToUseAuxVariables( ipq ) );
        TS_ASSERT_EQUALS( ipq.getNumberOfVariables(), 11u );
        // element to aux: 2: 8, 3: 9, 4: 10.

        max2.notifyLowerBound( 1, 1 );
        max2.notifyUpperBound( 1, 2 );
        max2.notifyLowerBound( 2, 1 );
        max2.notifyUpperBound( 2, 1 );
        max2.notifyLowerBound( 3, 1 );
        max2.notifyUpperBound( 3, 2 );
        max2.notifyLowerBound( 4, 1 );
        max2.notifyUpperBound( 4, 2 );
        max2.notifyLowerBound( 8, 0 );
        max2.notifyUpperBound( 8, 1 );
        max2.notifyLowerBound( 9, 0 );
        max2.notifyUpperBound( 9, 1 );
        max2.notifyLowerBound( 10, 0 );
        max2.notifyUpperBound( 10, 1 );

        max2.eliminateVariable( 2, 1 );

        tableau.setValue( 1, 2 );
        tableau.setValue( 3, 1.5 );
        tableau.setValue( 4, 1.8 );
        tableau.setValue( 9, 0.5 );
        tableau.setValue( 10, 0.2 );

        // Phase not fixed, can add cost terms to eligible phases.
        TS_ASSERT( !max2.phaseFixed() );
        LinearExpression cost2;
        TS_ASSERT_THROWS_NOTHING( max2.getCostFunctionComponent( cost2, MAX_PHASE_ELIMINATED ) );
        TS_ASSERT_EQUALS( cost2._addends.size(), 1u );
        TS_ASSERT_EQUALS( cost2._addends[1], 1 );
        TS_ASSERT_EQUALS( cost2._constant, -1 );

        TS_ASSERT_THROWS_NOTHING( max2.getCostFunctionComponent( cost2, MAX_PHASE_ELIMINATED ) );
        TS_ASSERT_EQUALS( cost2._addends.size(), 1u );
        TS_ASSERT_EQUALS( cost2._addends[1], 2 );
        TS_ASSERT_EQUALS( cost2._constant, -2 );

        LinearExpression cost3;
        List<PhaseStatus> phases;
        TS_ASSERT_THROWS_NOTHING( phases = max2.getAllCases() );
        TS_ASSERT_THROWS_NOTHING( max2.getCostFunctionComponent( cost3, *( phases.begin() ) ) );
        TS_ASSERT_EQUALS( cost3._addends.size(), 1u );
        // The first case is eliminate, the aux var to the second case is 9
        TS_ASSERT_EQUALS( cost3._addends[9], 1 );
        TS_ASSERT_EQUALS( cost3._constant, 0 );

        // Each case returned by getAllCases() should be eligible as a term in
        // the SoI.
        for ( const auto &phase : phases )
            TS_ASSERT_THROWS_NOTHING( max2.getCostFunctionComponent( cost3, phase ) );
    }

    void test_get_phase_in_assignment()
    {
        unsigned f = 1;
        Set<unsigned> elements;

        for ( unsigned i = 2; i < 5; ++i )
            elements.insert( i );

        MaxConstraint max( f, elements );
        MockTableau tableau;
        max.registerTableau( &tableau );

        max.eliminateVariable( 2, 1 );

        tableau.setValue( 1, 1 );
        tableau.setValue( 3, 1.5 );
        tableau.setValue( 4, 1.8 );

        Map<unsigned, double> assignment;
        assignment[1] = 2;
        assignment[3] = 2;
        assignment[4] = 1;
        List<PhaseStatus> phases;
        TS_ASSERT_THROWS_NOTHING( phases = max.getAllCases() );
        TS_ASSERT_EQUALS( max.getPhaseStatusInAssignment( assignment ), *phases.begin() );

        assignment[1] = 1;
        assignment[3] = 0.9;
        assignment[4] = 0.8;
        TS_ASSERT_EQUALS( max.getPhaseStatusInAssignment( assignment ), MAX_PHASE_ELIMINATED );
    }

    void test_get_all_cases()
    {
        unsigned f = 1;
        Set<unsigned> elements;

        for ( unsigned i = 2; i < 5; ++i )
            elements.insert( i );

        MaxConstraint max( f, elements );
        MockTableau tableau;
        max.registerTableau( &tableau );

        for ( unsigned i = 1; i < 5; ++i )
        {
            max.notifyLowerBound( i, 1 );
            max.notifyUpperBound( i, 2 );
        }

        List<PhaseStatus> phases;
        TS_ASSERT_THROWS_NOTHING( phases = max.getAllCases() );

        tableau.setValue( 4, 1.8 );

        TS_ASSERT_THROWS_NOTHING( phases = max.getAllCases() );
    }

    void test_tightening_from_eliminated_variable1()
    {
        // Elimination of input variable should update the lower bound of
        // the output variable.
        unsigned f = 1;
        Set<unsigned> elements;

        for ( unsigned i = 2; i < 5; ++i )
            elements.insert( i );

        MaxConstraint max( f, elements );
        Query ipq;
        ipq.setNumberOfVariables( 5 );
        TS_ASSERT_THROWS_NOTHING( max.transformToUseAuxVariables( ipq ) );
        TS_ASSERT_EQUALS( ipq.getNumberOfVariables(), 8u );

        for ( unsigned i = 2; i < 5; ++i )
        {
            max.notifyLowerBound( i, 1 );
            max.notifyUpperBound( i, 2 );
        }

        List<Tightening> tightenings;
        max.getEntailedTightenings( tightenings );
        double lowerBound = FloatUtils::negativeInfinity();
        for ( const auto &t : tightenings )
            if ( t._variable == f && t._type == Tightening::LB && t._value > lowerBound )
                lowerBound = t._value;
        TS_ASSERT_EQUALS( lowerBound, 1 );

        tightenings.clear();
        // Eliminate some input variable.
        TS_ASSERT_THROWS_NOTHING( max.eliminateVariable( 2, 1.5 ) );
        max.getEntailedTightenings( tightenings );

        lowerBound = FloatUtils::negativeInfinity();
        for ( const auto &t : tightenings )
            if ( t._variable == f && t._type == Tightening::LB && t._value > lowerBound )
                lowerBound = t._value;
        TS_ASSERT_EQUALS( lowerBound, 1.5 );
    }

    void test_tightening_from_eliminated_variable2()
    {
        // Elimination of input variable should *not* update the upper bound of
        // the output variable.
        unsigned f = 1;
        Set<unsigned> elements;

        for ( unsigned i = 2; i < 5; ++i )
            elements.insert( i );

        MaxConstraint max( f, elements );
        Query ipq;
        ipq.setNumberOfVariables( 5 );
        TS_ASSERT_THROWS_NOTHING( max.transformToUseAuxVariables( ipq ) );
        TS_ASSERT_EQUALS( ipq.getNumberOfVariables(), 8u );

        for ( unsigned i = 2; i < 5; ++i )
        {
            max.notifyLowerBound( i, 1 );
            max.notifyUpperBound( i, 2 );
        }

        List<Tightening> tightenings;
        max.getEntailedTightenings( tightenings );
        double upperBound = FloatUtils::infinity();
        for ( const auto &t : tightenings )
            if ( t._variable == f && t._type == Tightening::UB && t._value < upperBound )
                upperBound = t._value;
        TS_ASSERT_EQUALS( upperBound, 2 );

        tightenings.clear();
        // Eliminate some input variable.
        TS_ASSERT_THROWS_NOTHING( max.eliminateVariable( 2, 1.5 ) );
        max.getEntailedTightenings( tightenings );

        for ( const auto &t : tightenings )
            if ( t._variable == f && t._type == Tightening::UB && t._value < upperBound )
                upperBound = t._value;
        TS_ASSERT_EQUALS( upperBound, 2 );
    }
};
