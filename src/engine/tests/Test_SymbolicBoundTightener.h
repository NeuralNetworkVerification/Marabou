/*********************                                                        */
/*! \file Test_SymbolicBoundTightener.h
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

#include "FloatUtils.h"
#include "SymbolicBoundTightener.h"

class MockForSymbolicBoundTightener
{
public:
};

class SymbolicBoundTightenerTestSuite : public CxxTest::TestSuite
{
public:
    MockForSymbolicBoundTightener *mock;

    void setUp()
    {
        TS_ASSERT( mock = new MockForSymbolicBoundTightener );
    }

    void tearDown()
    {
        TS_ASSERT_THROWS_NOTHING( delete mock );
    }

    void test_all_lower_bounds_positive()
    {
        // // This simple example from the ReluVal paper

        // SymbolicBoundTightener sbt;

        // sbt.setNumberOfLayers( 3 );
        // sbt.setLayerSize( 0, 2 );
        // sbt.setLayerSize( 1, 2 );
        // sbt.setLayerSize( 2, 1 );

        // sbt.allocateWeightAndBiasSpace();

        // // All biases are 0
        // sbt.setBias( 0, 0, 0 );
        // sbt.setBias( 0, 1, 0 );
        // sbt.setBias( 1, 0, 0 );
        // sbt.setBias( 1, 1, 0 );
        // sbt.setBias( 2, 1, 0 );

        // // Weights
        // sbt.setWeight( 0, 0, 0, 2 );
        // sbt.setWeight( 0, 0, 1, 1 );
        // sbt.setWeight( 0, 1, 0, 3 );
        // sbt.setWeight( 0, 1, 1, 1 );
        // sbt.setWeight( 1, 0, 0, 1 );
        // sbt.setWeight( 1, 1, 0, -1 );

        // // Initial bounds
        // sbt.setInputLowerBound( 0, 4 );
        // sbt.setInputUpperBound( 0, 6 );
        // sbt.setInputLowerBound( 1, 1 );
        // sbt.setInputUpperBound( 1, 5 );

        // // Run the tightener
        // TS_ASSERT_THROWS_NOTHING( sbt.run() );

        // // Expected range: [6, 16], +- epsilon
        // TS_ASSERT( sbt.getLowerBound( 2, 0 ) < 6 );
        // TS_ASSERT( sbt.getLowerBound( 2, 0 ) > 6 - 0.001 );
        // TS_ASSERT( sbt.getUpperBound( 2, 0 ) > 16 );
        // TS_ASSERT( sbt.getUpperBound( 2, 0 ) < 16 + 0.001 );
    }

    void test_negative_lower_bounds_get_zeroed()
    {
        // // This simple example from the ReluVal paper

        // SymbolicBoundTightener sbt;

        // sbt.setNumberOfLayers( 3 );
        // sbt.setLayerSize( 0, 2 );
        // sbt.setLayerSize( 1, 2 );
        // sbt.setLayerSize( 2, 1 );

        // sbt.allocateWeightAndBiasSpace();

        // // All biases are 0
        // sbt.setBias( 0, 0, 0 );
        // sbt.setBias( 0, 1, 0 );
        // sbt.setBias( 1, 0, -15 ); // Strong negative bias for node (1,0)
        // sbt.setBias( 1, 1, 0 );
        // sbt.setBias( 2, 1, 0 );

        // // Weights
        // sbt.setWeight( 0, 0, 0, 2 );
        // sbt.setWeight( 0, 0, 1, 1 );
        // sbt.setWeight( 0, 1, 0, 3 );
        // sbt.setWeight( 0, 1, 1, 1 );
        // sbt.setWeight( 1, 0, 0, 1 );
        // sbt.setWeight( 1, 1, 0, -1 );

        // // Initial bounds
        // sbt.setInputLowerBound( 0, 4 );
        // sbt.setInputUpperBound( 0, 6 );
        // sbt.setInputLowerBound( 1, 1 );
        // sbt.setInputUpperBound( 1, 5 );

        // // Run the tightener
        // TS_ASSERT_THROWS_NOTHING( sbt.run( false ) ); // Test with costant concretization

        // // Expected range: [-11, 7], +- epsilon
        // TS_ASSERT( sbt.getLowerBound( 2, 0 ) < -11 );
        // TS_ASSERT( sbt.getLowerBound( 2, 0 ) > -11 - 0.001 );
        // TS_ASSERT( sbt.getUpperBound( 2, 0 ) > 7 );
        // TS_ASSERT( sbt.getUpperBound( 2, 0 ) < 7 + 0.001 );
    }

    void test_negative_lower_bounds_and_upper_bounds_get_zeroed()
    {
        // // This simple example from the ReluVal paper

        // SymbolicBoundTightener sbt;

        // sbt.setNumberOfLayers( 3 );
        // sbt.setLayerSize( 0, 2 );
        // sbt.setLayerSize( 1, 2 );
        // sbt.setLayerSize( 2, 1 );

        // sbt.allocateWeightAndBiasSpace();

        // // All biases are 0
        // sbt.setBias( 0, 0, 0 );
        // sbt.setBias( 0, 1, 0 );
        // sbt.setBias( 1, 0, -30 ); // Strong negative bias for node (1,0)
        // sbt.setBias( 1, 1, 0 );
        // sbt.setBias( 2, 1, 0 );

        // // Weights
        // sbt.setWeight( 0, 0, 0, 2 );
        // sbt.setWeight( 0, 0, 1, 1 );
        // sbt.setWeight( 0, 1, 0, 3 );
        // sbt.setWeight( 0, 1, 1, 1 );
        // sbt.setWeight( 1, 0, 0, 1 );
        // sbt.setWeight( 1, 1, 0, -1 );

        // // Initial bounds
        // sbt.setInputLowerBound( 0, 4 );
        // sbt.setInputUpperBound( 0, 6 );
        // sbt.setInputLowerBound( 1, 1 );
        // sbt.setInputUpperBound( 1, 5 );

        // // Run the tightener
        // TS_ASSERT_THROWS_NOTHING( sbt.run( false ) ); // Test with costant concretization

        // // Expected range: [-11, -5], +- epsilon
        // TS_ASSERT( sbt.getLowerBound( 2, 0 ) < -11 );
        // TS_ASSERT( sbt.getLowerBound( 2, 0 ) > -11 - 0.001 );
        // TS_ASSERT( sbt.getUpperBound( 2, 0 ) > -5 );
        // TS_ASSERT( sbt.getUpperBound( 2, 0 ) < -5 + 0.001 );
    }

    void test_fixed_relus()
    {
        // // This simple example from the ReluVal paper

        // SymbolicBoundTightener sbt;

        // sbt.setNumberOfLayers( 3 );
        // sbt.setLayerSize( 0, 2 );
        // sbt.setLayerSize( 1, 2 );
        // sbt.setLayerSize( 2, 1 );

        // sbt.allocateWeightAndBiasSpace();

        // // All biases are 0
        // sbt.setBias( 0, 0, 0 );
        // sbt.setBias( 0, 1, 0 );
        // sbt.setBias( 1, 0, -15 ); // Strong negative bias for node (1,0)
        // sbt.setBias( 1, 1, 0 );
        // sbt.setBias( 2, 1, 0 );

        // // Weights
        // sbt.setWeight( 0, 0, 0, 2 );
        // sbt.setWeight( 0, 0, 1, 1 );
        // sbt.setWeight( 0, 1, 0, 3 );
        // sbt.setWeight( 0, 1, 1, 1 );
        // sbt.setWeight( 1, 0, 0, 1 );
        // sbt.setWeight( 1, 1, 0, -1 );

        // // Initial bounds
        // sbt.setInputLowerBound( 0, 4 );
        // sbt.setInputUpperBound( 0, 6 );
        // sbt.setInputLowerBound( 1, 1 );
        // sbt.setInputUpperBound( 1, 5 );

        // /// Case 1: ReLU not fixed
        // sbt.setReluStatus( 1, 0, ReluConstraint::PHASE_NOT_FIXED );

        // // Run the tightener
        // TS_ASSERT_THROWS_NOTHING( sbt.run( false ) ); // Test with costant concretization

        // // Expected range: [-11, -5], +- epsilon
        // TS_ASSERT( sbt.getLowerBound( 2, 0 ) < -11 );
        // TS_ASSERT( sbt.getLowerBound( 2, 0 ) > -11 - 0.001 );
        // TS_ASSERT( sbt.getUpperBound( 2, 0 ) > 7 );
        // TS_ASSERT( sbt.getUpperBound( 2, 0 ) < 7 + 0.001 );

        // /// Case 2: ReLU fixed to ACTIVE
        // sbt.setReluStatus( 1, 0, ReluConstraint::PHASE_ACTIVE );

        // // Run the tightener
        // TS_ASSERT_THROWS_NOTHING( sbt.run( false ) ); // Test with costant concretization

        // // Expected range: [-11, -5], +- epsilon
        // TS_ASSERT( sbt.getLowerBound( 2, 0 ) < -9 );
        // TS_ASSERT( sbt.getLowerBound( 2, 0 ) > -9 - 0.001 );
        // TS_ASSERT( sbt.getUpperBound( 2, 0 ) > 1 );
        // TS_ASSERT( sbt.getUpperBound( 2, 0 ) < 1 + 0.001 );

        // /// Case 2: ReLU fixed to INACTIVE
        // sbt.setReluStatus( 1, 0, ReluConstraint::PHASE_INACTIVE );

        // // Run the tightener
        // TS_ASSERT_THROWS_NOTHING( sbt.run( false ) ); // Test with costant concretization

        // // Expected range: [-11, -5], +- epsilon
        // TS_ASSERT( sbt.getLowerBound( 2, 0 ) < -11 );
        // TS_ASSERT( sbt.getLowerBound( 2, 0 ) > -11 - 0.001 );
        // TS_ASSERT( sbt.getUpperBound( 2, 0 ) > -5 );
        // TS_ASSERT( sbt.getUpperBound( 2, 0 ) < -5 + 0.001 );
    }

    void test_todo()
    {
        // TS_TRACE( "TODO: add a test for linear concretizations" );
    }
};

//
// Local Variables:
// compile-command: "make -C ../../.. "
// tags-file-name: "../../../TAGS"
// c-basic-offset: 4
// End:
//
