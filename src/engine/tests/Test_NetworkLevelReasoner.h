/*********************                                                        */
/*! \file Test_NetworkLevelReasoner.h
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
#include "MockTableau.h"
#include "NetworkLevelReasoner.h"
#include "Tightening.h"

class MockForNetworkLevelReasoner
{
public:
};

class NetworkLevelReasonerTestSuite : public CxxTest::TestSuite
{
public:
    MockForNetworkLevelReasoner *mock;

    void setUp()
    {
        TS_ASSERT( mock = new MockForNetworkLevelReasoner );
    }

    void tearDown()
    {
        TS_ASSERT_THROWS_NOTHING( delete mock );
    }

    void populateNetwork( NetworkLevelReasoner &nlr )
    {
        /*
                a
          x           d    f
                b
          y           e    g
                c
        */

        nlr.setNumberOfLayers( 4 );

        nlr.setLayerSize( 0, 2 );
        nlr.setLayerSize( 1, 3 );
        nlr.setLayerSize( 2, 2 );
        nlr.setLayerSize( 3, 2 );

        nlr.allocateMemoryByTopology();

        // Weights
        nlr.setWeight( 0, 0, 0, 1 );
        nlr.setWeight( 0, 0, 1, 2 );
        nlr.setWeight( 0, 1, 1, -3 );
        nlr.setWeight( 0, 1, 2, 1 );

        nlr.setWeight( 1, 0, 0, 1 );
        nlr.setWeight( 1, 0, 1, -1 );
        nlr.setWeight( 1, 1, 0, 1 );
        nlr.setWeight( 1, 1, 1, 1 );
        nlr.setWeight( 1, 2, 0, -1 );
        nlr.setWeight( 1, 2, 1, -1 );

        nlr.setWeight( 2, 0, 0, 1 );
        nlr.setWeight( 2, 0, 1, 1 );
        nlr.setWeight( 2, 1, 1, 3 );

        // Biases
        nlr.setBias( 1, 0, 1 );
        nlr.setBias( 2, 1, 2 );

        // Variable indexing
        nlr.setActivationResultVariable( 0, 0, 0 );
        nlr.setActivationResultVariable( 0, 1, 1 );

        nlr.setWeightedSumVariable( 1, 0, 2 );
        nlr.setActivationResultVariable( 1, 0, 3 );
        nlr.setWeightedSumVariable( 1, 1, 4 );
        nlr.setActivationResultVariable( 1, 1, 5 );
        nlr.setWeightedSumVariable( 1, 2, 6 );
        nlr.setActivationResultVariable( 1, 2, 7 );

        nlr.setWeightedSumVariable( 2, 0, 8 );
        nlr.setActivationResultVariable( 2, 0, 9 );
        nlr.setWeightedSumVariable( 2, 1, 10 );
        nlr.setActivationResultVariable( 2, 1, 11 );

        nlr.setWeightedSumVariable( 3, 0, 12 );
        nlr.setWeightedSumVariable( 3, 1, 13 );

        // Mark nodes as ReLUs
        nlr.setNeuronActivationFunction( 1, 0, PiecewiseLinearFunctionType::RELU );
        nlr.setNeuronActivationFunction( 1, 1, PiecewiseLinearFunctionType::RELU );
        nlr.setNeuronActivationFunction( 1, 2, PiecewiseLinearFunctionType::RELU );

        nlr.setNeuronActivationFunction( 2, 0, PiecewiseLinearFunctionType::RELU );
        nlr.setNeuronActivationFunction( 2, 1, PiecewiseLinearFunctionType::RELU );
    }

    void test_evaluate_relus()
    {
        NetworkLevelReasoner nlr;

        populateNetwork( nlr );

        double input[2];
        double output[2];

        // With ReLUs, Inputs are zeros, only biases count
        input[0] = 0;
        input[1] = 0;

        TS_ASSERT_THROWS_NOTHING( nlr.evaluate( input, output ) );

        TS_ASSERT( FloatUtils::areEqual( output[0], 1 ) );
        TS_ASSERT( FloatUtils::areEqual( output[1], 4 ) );

        // With ReLUs, case 1
        input[0] = 1;
        input[1] = 1;

        TS_ASSERT_THROWS_NOTHING( nlr.evaluate( input, output ) );

        TS_ASSERT( FloatUtils::areEqual( output[0], 1 ) );
        TS_ASSERT( FloatUtils::areEqual( output[1], 1 ) );

        // With ReLUs, case 2
        input[0] = 1;
        input[1] = 2;

        nlr.setWeightedSumVariable( 2, 0, 8 );
        nlr.setActivationResultVariable( 2, 0, 9 );
        nlr.setWeightedSumVariable( 2, 1, 10 );
        nlr.setActivationResultVariable( 2, 1, 11 );

        TS_ASSERT_THROWS_NOTHING( nlr.evaluate( input, output ) );

        TS_ASSERT( FloatUtils::areEqual( output[0], 0 ) );
        TS_ASSERT( FloatUtils::areEqual( output[1], 0 ) );
    }

    void test_evaluate_relus_and_abs()
    {
        NetworkLevelReasoner nlr;

        populateNetwork( nlr );

        // Override activation functions for the first hidden layer
        nlr.setNeuronActivationFunction( 1, 0, PiecewiseLinearFunctionType::ABSOLUTE_VALUE );
        nlr.setNeuronActivationFunction( 1, 1, PiecewiseLinearFunctionType::ABSOLUTE_VALUE );
        nlr.setNeuronActivationFunction( 1, 2, PiecewiseLinearFunctionType::ABSOLUTE_VALUE );

        nlr.setWeight( 1, 2, 1, -5 );

        double input[2];
        double output[2];

        input[0] = 1;
        input[1] = 1;

        TS_ASSERT_THROWS_NOTHING( nlr.evaluate( input, output ) );

        TS_ASSERT( FloatUtils::areEqual( output[0], 2 ) );
        TS_ASSERT( FloatUtils::areEqual( output[1], 2 ) );

        input[0] = 1;
        input[1] = 2;

        TS_ASSERT_THROWS_NOTHING( nlr.evaluate( input, output ) );

        TS_ASSERT( FloatUtils::areEqual( output[0], 4 ) );
        TS_ASSERT( FloatUtils::areEqual( output[1], 4 ) );
    }

    void test_store_into_other()
    {
        NetworkLevelReasoner nlr;

        populateNetwork( nlr );

        NetworkLevelReasoner nlr2;

        TS_ASSERT_THROWS_NOTHING( nlr.storeIntoOther( nlr2 ) );

        double input[2];
        double output1[2];
        double output2[2];

        // Inputs are zeros, only biases count
        input[0] = 0;
        input[1] = 0;

        TS_ASSERT_THROWS_NOTHING( nlr.evaluate( input, output1 ) );
        TS_ASSERT_THROWS_NOTHING( nlr2.evaluate( input, output2 ) );

        TS_ASSERT( FloatUtils::areEqual( output1[0], output2[0] ) );
        TS_ASSERT( FloatUtils::areEqual( output1[1], output2[1] ) );

        TS_ASSERT_THROWS_NOTHING( nlr.storeIntoOther( nlr2 ) );

        // With ReLUs, Inputs are zeros, only biases count
        input[0] = 0;
        input[1] = 0;

        TS_ASSERT_THROWS_NOTHING( nlr.evaluate( input, output1 ) );
        TS_ASSERT_THROWS_NOTHING( nlr2.evaluate( input, output2 ) );

        TS_ASSERT( FloatUtils::areEqual( output1[0], output2[0] ) );
        TS_ASSERT( FloatUtils::areEqual( output1[1], output2[1] ) );

        // With ReLUs, case 1
        input[0] = 1;
        input[1] = 1;

        TS_ASSERT_THROWS_NOTHING( nlr.evaluate( input, output1 ) );
        TS_ASSERT_THROWS_NOTHING( nlr2.evaluate( input, output2 ) );

        TS_ASSERT( FloatUtils::areEqual( output1[0], output2[0] ) );
        TS_ASSERT( FloatUtils::areEqual( output1[1], output2[1] ) );
    }

    void test_interval_arithmetic_bound_propagation_relu_constraints()
    {
        NetworkLevelReasoner nlr;
        populateNetwork( nlr );

        MockTableau tableau;

        // Initialize the bounds
        tableau.setLowerBound( 0, -1 );
        tableau.setUpperBound( 0, 1 );
        tableau.setLowerBound( 1, -1 );
        tableau.setUpperBound( 1, 1 );

        double large = 1000;
        tableau.setLowerBound( 2, -large ); tableau.setUpperBound( 2, large );
        tableau.setLowerBound( 3, -large ); tableau.setUpperBound( 3, large );
        tableau.setLowerBound( 4, -large ); tableau.setUpperBound( 4, large );
        tableau.setLowerBound( 5, -large ); tableau.setUpperBound( 5, large );
        tableau.setLowerBound( 6, -large ); tableau.setUpperBound( 6, large );
        tableau.setLowerBound( 7, -large ); tableau.setUpperBound( 7, large );
        tableau.setLowerBound( 8, -large ); tableau.setUpperBound( 8, large );
        tableau.setLowerBound( 9, -large ); tableau.setUpperBound( 9, large );
        tableau.setLowerBound( 10, -large ); tableau.setUpperBound( 10, large );
        tableau.setLowerBound( 11, -large ); tableau.setUpperBound( 11, large );
        tableau.setLowerBound( 12, -large ); tableau.setUpperBound( 12, large );
        tableau.setLowerBound( 13, -large ); tableau.setUpperBound( 13, large );

        nlr.setTableau( &tableau );

        // Initialize
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );

        // Perform the tightening pass
        TS_ASSERT_THROWS_NOTHING( nlr.intervalArithmeticBoundPropagation() );

        List<Tightening> expectedBounds({
                Tightening( 2, 0, Tightening::LB ),
                Tightening( 2, 2, Tightening::UB ),
                Tightening( 3, 0, Tightening::LB ),
                Tightening( 3, 2, Tightening::UB ),

                Tightening( 4, -5, Tightening::LB ),
                Tightening( 4, 5, Tightening::UB ),
                Tightening( 5, 0, Tightening::LB ),
                Tightening( 5, 5, Tightening::UB ),

                Tightening( 6, -1, Tightening::LB ),
                Tightening( 6, 1, Tightening::UB ),
                Tightening( 7, 0, Tightening::LB ),
                Tightening( 7, 1, Tightening::UB ),

                Tightening( 8, -1, Tightening::LB ),
                Tightening( 8, 7, Tightening::UB ),
                Tightening( 9, 0, Tightening::LB ),
                Tightening( 9, 7, Tightening::UB ),

                Tightening( 10, -1, Tightening::LB ),
                Tightening( 10, 7, Tightening::UB ),
                Tightening( 11, 0, Tightening::LB ),
                Tightening( 11, 7, Tightening::UB ),

                Tightening( 12, 0, Tightening::LB ),
                Tightening( 12, 7, Tightening::UB ),
                Tightening( 13, 0, Tightening::LB ),
                Tightening( 13, 28, Tightening::UB ),
                    });

        List<Tightening> bounds;
        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );
        TS_ASSERT_EQUALS( expectedBounds, bounds );

        // Change the current bounds
        tableau.setLowerBound( 0, -3 );
        tableau.setUpperBound( 0, 1 );
        tableau.setLowerBound( 1, -1 );
        tableau.setUpperBound( 1, 2 );

        tableau.setLowerBound( 2, -large ); tableau.setUpperBound( 2, large );
        tableau.setLowerBound( 3, -large ); tableau.setUpperBound( 3, large );
        tableau.setLowerBound( 4, -large ); tableau.setUpperBound( 4, large );
        tableau.setLowerBound( 5, -large ); tableau.setUpperBound( 5, large );
        tableau.setLowerBound( 6, -large ); tableau.setUpperBound( 6, large );
        tableau.setLowerBound( 7, -large ); tableau.setUpperBound( 7, large );
        tableau.setLowerBound( 8, -large ); tableau.setUpperBound( 8, large );
        tableau.setLowerBound( 9, -large ); tableau.setUpperBound( 9, large );
        tableau.setLowerBound( 10, -large ); tableau.setUpperBound( 10, large );
        tableau.setLowerBound( 11, -large ); tableau.setUpperBound( 11, large );
        tableau.setLowerBound( 12, -large ); tableau.setUpperBound( 12, large );
        tableau.setLowerBound( 13, -large ); tableau.setUpperBound( 13, large );

        // Initialize
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );

        // Perform the tightening pass
        TS_ASSERT_THROWS_NOTHING( nlr.intervalArithmeticBoundPropagation() );

        List<Tightening> expectedBounds2({
                Tightening( 2, -2, Tightening::LB ),
                Tightening( 2, 2, Tightening::UB ),
                Tightening( 3, 0, Tightening::LB ),
                Tightening( 3, 2, Tightening::UB ),

                Tightening( 4, -12, Tightening::LB ),
                Tightening( 4, 5, Tightening::UB ),
                Tightening( 5, 0, Tightening::LB ),
                Tightening( 5, 5, Tightening::UB ),

                Tightening( 6, -1, Tightening::LB ),
                Tightening( 6, 2, Tightening::UB ),
                Tightening( 7, 0, Tightening::LB ),
                Tightening( 7, 2, Tightening::UB ),

                Tightening( 8, -2, Tightening::LB ),
                Tightening( 8, 7, Tightening::UB ),
                Tightening( 9, 0, Tightening::LB ),
                Tightening( 9, 7, Tightening::UB ),

                Tightening( 10, -2, Tightening::LB ),
                Tightening( 10, 7, Tightening::UB ),
                Tightening( 11, 0, Tightening::LB ),
                Tightening( 11, 7, Tightening::UB ),

                Tightening( 12, 0, Tightening::LB ),
                Tightening( 12, 7, Tightening::UB ),
                Tightening( 13, 0, Tightening::LB ),
                Tightening( 13, 28, Tightening::UB ),
                    });

        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );
        TS_ASSERT_EQUALS( expectedBounds2, bounds );
    }

    void test_interval_arithmetic_bound_propagation_abs_constraints()
    {
        NetworkLevelReasoner nlr;
        populateNetwork( nlr );

        nlr.setNeuronActivationFunction( 1, 0, PiecewiseLinearFunctionType::ABSOLUTE_VALUE );
        nlr.setNeuronActivationFunction( 1, 1, PiecewiseLinearFunctionType::ABSOLUTE_VALUE );
        nlr.setNeuronActivationFunction( 1, 2, PiecewiseLinearFunctionType::ABSOLUTE_VALUE );

        nlr.setNeuronActivationFunction( 2, 0, PiecewiseLinearFunctionType::ABSOLUTE_VALUE );
        nlr.setNeuronActivationFunction( 2, 1, PiecewiseLinearFunctionType::ABSOLUTE_VALUE );

        MockTableau tableau;

        // Initialize the bounds
        tableau.setLowerBound( 0, -1 );
        tableau.setUpperBound( 0, 2 );
        tableau.setLowerBound( 1, -1 );
        tableau.setUpperBound( 1, 2 );

        double large = 1000;
        tableau.setLowerBound( 2, -large ); tableau.setUpperBound( 2, large );
        tableau.setLowerBound( 3, -large ); tableau.setUpperBound( 3, large );
        tableau.setLowerBound( 4, -large ); tableau.setUpperBound( 4, large );
        tableau.setLowerBound( 5, -large ); tableau.setUpperBound( 5, large );
        tableau.setLowerBound( 6, -large ); tableau.setUpperBound( 6, large );
        tableau.setLowerBound( 7, -large ); tableau.setUpperBound( 7, large );
        tableau.setLowerBound( 8, -large ); tableau.setUpperBound( 8, large );
        tableau.setLowerBound( 9, -large ); tableau.setUpperBound( 9, large );
        tableau.setLowerBound( 10, -large ); tableau.setUpperBound( 10, large );
        tableau.setLowerBound( 11, -large ); tableau.setUpperBound( 11, large );
        tableau.setLowerBound( 12, -large ); tableau.setUpperBound( 12, large );
        tableau.setLowerBound( 13, -large ); tableau.setUpperBound( 13, large );

        nlr.setTableau( &tableau );

        // Initialize
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );

        // Perform the tightening pass
        TS_ASSERT_THROWS_NOTHING( nlr.intervalArithmeticBoundPropagation() );

        List<Tightening> expectedBounds({
                Tightening( 2, 0, Tightening::LB ),
                Tightening( 2, 3, Tightening::UB ),
                Tightening( 3, 0, Tightening::LB ),
                Tightening( 3, 3, Tightening::UB ),

                Tightening( 4, -8, Tightening::LB ),
                Tightening( 4, 7, Tightening::UB ),
                Tightening( 5, 0, Tightening::LB ),
                Tightening( 5, 8, Tightening::UB ),

                Tightening( 6, -1, Tightening::LB ),
                Tightening( 6, 2, Tightening::UB ),
                Tightening( 7, 0, Tightening::LB ),
                Tightening( 7, 2, Tightening::UB ),

                Tightening( 8, -2, Tightening::LB ),
                Tightening( 8, 11, Tightening::UB ),
                Tightening( 9, 0, Tightening::LB ),
                Tightening( 9, 11, Tightening::UB ),

                Tightening( 10, -3, Tightening::LB ),
                Tightening( 10, 10, Tightening::UB ),
                Tightening( 11, 0, Tightening::LB ),
                Tightening( 11, 10, Tightening::UB ),

                Tightening( 12, 0, Tightening::LB ),
                Tightening( 12, 11, Tightening::UB ),
                Tightening( 13, 0, Tightening::LB ),
                Tightening( 13, 41, Tightening::UB ),
                    });

        List<Tightening> bounds;
        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );
        TS_ASSERT_EQUALS( expectedBounds, bounds );

        // Change the current bounds
        tableau.setLowerBound( 0, -3 );
        tableau.setUpperBound( 0, 1 );
        tableau.setLowerBound( 1, -1 );
        tableau.setUpperBound( 1, 2 );

        tableau.setLowerBound( 2, -large ); tableau.setUpperBound( 2, large );
        tableau.setLowerBound( 3, -large ); tableau.setUpperBound( 3, large );
        tableau.setLowerBound( 4, -large ); tableau.setUpperBound( 4, large );
        tableau.setLowerBound( 5, -large ); tableau.setUpperBound( 5, large );
        tableau.setLowerBound( 6, -large ); tableau.setUpperBound( 6, large );
        tableau.setLowerBound( 7, -large ); tableau.setUpperBound( 7, large );
        tableau.setLowerBound( 8, -large ); tableau.setUpperBound( 8, large );
        tableau.setLowerBound( 9, -large ); tableau.setUpperBound( 9, large );
        tableau.setLowerBound( 10, -large ); tableau.setUpperBound( 10, large );
        tableau.setLowerBound( 11, -large ); tableau.setUpperBound( 11, large );
        tableau.setLowerBound( 12, -large ); tableau.setUpperBound( 12, large );
        tableau.setLowerBound( 13, -large ); tableau.setUpperBound( 13, large );

        // Initialize
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );

        // Perform the tightening pass
        TS_ASSERT_THROWS_NOTHING( nlr.intervalArithmeticBoundPropagation() );

        List<Tightening> expectedBounds2({
                Tightening( 2, -2, Tightening::LB ),
                Tightening( 2, 2, Tightening::UB ),
                Tightening( 3, 0, Tightening::LB ),
                Tightening( 3, 2, Tightening::UB ),

                Tightening( 4, -12, Tightening::LB ),
                Tightening( 4, 5, Tightening::UB ),
                Tightening( 5, 0, Tightening::LB ),
                Tightening( 5, 12, Tightening::UB ),

                Tightening( 6, -1, Tightening::LB ),
                Tightening( 6, 2, Tightening::UB ),
                Tightening( 7, 0, Tightening::LB ),
                Tightening( 7, 2, Tightening::UB ),

                Tightening( 8, -2, Tightening::LB ),
                Tightening( 8, 14, Tightening::UB ),
                Tightening( 9, 0, Tightening::LB ),
                Tightening( 9, 14, Tightening::UB ),

                Tightening( 10, -2, Tightening::LB ),
                Tightening( 10, 14, Tightening::UB ),
                Tightening( 11, 0, Tightening::LB ),
                Tightening( 11, 14, Tightening::UB ),

                Tightening( 12, 0, Tightening::LB ),
                Tightening( 12, 14, Tightening::UB ),
                Tightening( 13, 0, Tightening::LB ),
                Tightening( 13, 56, Tightening::UB ),
                    });

        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );
        TS_ASSERT_EQUALS( expectedBounds2, bounds );
    }

    void populateNetworkSBT( NetworkLevelReasoner &nlr, MockTableau &tableau )
    {
        /*
              2      R       1
          x0 --- x2 ---> x4 --- x6
            \    /              /
           1 \  /              /
              \/           -1 /
              /\             /
           3 /  \           /
            /    \   R     /
          x1 --- x3 ---> x5
              1
        */

        nlr.setNumberOfLayers( 3 );

        nlr.setLayerSize( 0, 2 );
        nlr.setLayerSize( 1, 2 );
        nlr.setLayerSize( 2, 1 );

        nlr.allocateMemoryByTopology();

        // Weights
        nlr.setWeight( 0, 0, 0, 2 );
        nlr.setWeight( 0, 0, 1, 1 );
        nlr.setWeight( 0, 1, 0, 3 );
        nlr.setWeight( 0, 1, 1, 1 );
        nlr.setWeight( 1, 0, 0, 1 );
        nlr.setWeight( 1, 1, 0, -1 );

        // All biases are 0
        nlr.setBias( 0, 0, 0 );
        nlr.setBias( 0, 1, 0 );
        nlr.setBias( 1, 0, 0 );
        nlr.setBias( 1, 1, 0 );
        nlr.setBias( 2, 0, 0 );

        // Variable indexing
        nlr.setActivationResultVariable( 0, 0, 0 );
        nlr.setActivationResultVariable( 0, 1, 1 );

        nlr.setWeightedSumVariable( 1, 0, 2 );
        nlr.setWeightedSumVariable( 1, 1, 3 );
        nlr.setActivationResultVariable( 1, 0, 4 );
        nlr.setActivationResultVariable( 1, 1, 5 );

        nlr.setWeightedSumVariable( 2, 0, 6 );

        // Mark nodes as ReLUs
        nlr.setNeuronActivationFunction( 1, 0, PiecewiseLinearFunctionType::RELU );
        nlr.setNeuronActivationFunction( 1, 1, PiecewiseLinearFunctionType::RELU );

        // Very loose bounds for neurons except inputs
        double large = 1000000;

        tableau.setLowerBound( 2, -large ); tableau.setUpperBound( 2, large );
        tableau.setLowerBound( 3, -large ); tableau.setUpperBound( 3, large );
        tableau.setLowerBound( 4, -large ); tableau.setUpperBound( 4, large );
        tableau.setLowerBound( 5, -large ); tableau.setUpperBound( 5, large );
        tableau.setLowerBound( 6, -large ); tableau.setUpperBound( 6, large );
    }

    void test_sbt_relus_all_active()
    {
        NetworkLevelReasoner nlr;
        MockTableau tableau;
        nlr.setTableau( &tableau );
        populateNetworkSBT( nlr, tableau );

        tableau.setLowerBound( 0, 4 );
        tableau.setUpperBound( 0, 6 );
        tableau.setLowerBound( 1, 1 );
        tableau.setUpperBound( 1, 5 );

        // Invoke SBT
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );
        TS_ASSERT_THROWS_NOTHING( nlr.symbolicBoundPropagation() );

        /*
          Input ranges:

          x0: [4, 6]
          x1: [1, 5]

          Layer 1:

          x2.lb = 2x0 + 3x1   : [11, 27]
          x2.ub = 2x0 + 3x1   : [11, 27]

          x3.lb =  x0 +  x1   : [5, 11]
          x3.ub =  x0 +  x1   : [5, 11]

          Both ReLUs active, bound survive through activations:

          x4.lb = 2x0 + 3x1   : [11, 27]
          x4.ub = 2x0 + 3x1   : [11, 27]

          x5.lb =  x0 +  x1   : [5, 11]
          x5.ub =  x0 +  x1   : [5, 11]

          Layer 2:

          x6.lb =  x0 + 2x1   : [6, 16]
          x6.ub =  x0 + 2x1   : [6, 16]
        */

        List<Tightening> expectedBounds({
                Tightening( 2, 11, Tightening::LB ),
                Tightening( 2, 27, Tightening::UB ),
                Tightening( 3, 5, Tightening::LB ),
                Tightening( 3, 11, Tightening::UB ),

                Tightening( 4, 11, Tightening::LB ),
                Tightening( 4, 27, Tightening::UB ),
                Tightening( 5, 5, Tightening::LB ),
                Tightening( 5, 11, Tightening::UB ),

                Tightening( 6, 6, Tightening::LB ),
                Tightening( 6, 16, Tightening::UB ),
                    });

        List<Tightening> bounds;
        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );

        TS_ASSERT_EQUALS( expectedBounds.size(), bounds.size() );
        for ( const auto &bound : bounds )
            TS_ASSERT( expectedBounds.exists( bound ) );
    }

    void test_sbt_relus_active_and_inactive()
    {
        NetworkLevelReasoner nlr;
        MockTableau tableau;
        nlr.setTableau( &tableau );
        populateNetworkSBT( nlr, tableau );

        tableau.setLowerBound( 0, 4 );
        tableau.setUpperBound( 0, 6 );
        tableau.setLowerBound( 1, 1 );
        tableau.setUpperBound( 1, 5 );

        // Strong negative bias for x2, which is node (1,0)
        nlr.setBias( 1, 0, -30 );

        // Invoke SBT
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );
        TS_ASSERT_THROWS_NOTHING( nlr.symbolicBoundPropagation() );

        /*
          Input ranges:

          x0: [4, 6]
          x1: [1, 5]

          Layer 1:

          x2.lb = 2x0 + 3x1 - 30   : [-19, -3]
          x2.ub = 2x0 + 3x1 - 30   : [-19, -3]

          x3.lb =  x0 +  x1   : [5, 11]
          x3.ub =  x0 +  x1   : [5, 11]

          First ReLU is inactive, bounds get zeroed
          Second ReLU is active, bounds surive the activation

          x4.lb = 0
          x4.ub = 0

          x5.lb =  x0 +  x1   : [5, 11]
          x5.ub =  x0 +  x1   : [5, 11]

          Layer 2:

          x6.lb =  - x0 - x1  : [-11, -5]
          x6.ub =  - x0 - x1  : [-11, -5]
        */

        List<Tightening> expectedBounds({
                Tightening( 2, -19, Tightening::LB ),
                Tightening( 2, -3, Tightening::UB ),
                Tightening( 3, 5, Tightening::LB ),
                Tightening( 3, 11, Tightening::UB ),

                Tightening( 4, 0, Tightening::LB ),
                Tightening( 4, 0, Tightening::UB ),
                Tightening( 5, 5, Tightening::LB ),
                Tightening( 5, 11, Tightening::UB ),

                Tightening( 6, -11, Tightening::LB ),
                Tightening( 6, -5, Tightening::UB ),
                    });

        List<Tightening> bounds;
        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );

        TS_ASSERT_EQUALS( expectedBounds.size(), bounds.size() );
        for ( const auto &bound : bounds )
            TS_ASSERT( expectedBounds.exists( bound ) );
    }

    void test_sbt_relus_active_and_not_fixed()
    {
        NetworkLevelReasoner nlr;
        MockTableau tableau;
        nlr.setTableau( &tableau );
        populateNetworkSBT( nlr, tableau );

        tableau.setLowerBound( 0, 4 );
        tableau.setUpperBound( 0, 6 );
        tableau.setLowerBound( 1, 1 );
        tableau.setUpperBound( 1, 5 );

        // Strong negative bias for x2, which is node (1,0)
        nlr.setBias( 1, 0, -15 );

        // Invoke SBT
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );
        TS_ASSERT_THROWS_NOTHING( nlr.symbolicBoundPropagation() );

        /*
          Input ranges:

          x0: [4, 6]
          x1: [1, 5]

          Layer 1:

          x2.lb = 2x0 + 3x1 - 15   : [-4, 12]
          x2.ub = 2x0 + 3x1 - 15   : [-4, 12]

          x3.lb =  x0 +  x1   : [5, 11]
          x3.ub =  x0 +  x1   : [5, 11]

          First ReLU is undecided, bound is concretized.
            Coefficient: 12/(12--4) = 12/16 = 0.75
          Second ReLU is active, bounds surive the activation

          x4 range: [0, 12]
          x4.lb = 0.75( 2x0 + 3x1 ) - 0.75 * 15      = 1.5x0 + 2.25x1 - 11.25
          x4.ub = 0.75( 2x0 + 3x1 ) - 0.75 * 15 + 3  = 1.5x0 + 2.25x1 -  8.25

          x5.lb =  x0 +  x1   : [5, 11]
          x5.ub =  x0 +  x1   : [5, 11]

          Layer 2:

          x6.lb =  0.5x0 + 1.25x1 - 11.25
          x6.ub =  0.5x0 + 1.25x1 -  8.25

          x6 range: [2 + 1.25 - 11.25 = -8, 3 + 6.25 - 8.25 = 1] = [-8, 1]
        */

        List<Tightening> expectedBounds({
                Tightening( 2, -4, Tightening::LB ),
                Tightening( 2, 12, Tightening::UB ),
                Tightening( 3, 5, Tightening::LB ),
                Tightening( 3, 11, Tightening::UB ),

                Tightening( 4, 0, Tightening::LB ),
                Tightening( 4, 12, Tightening::UB ),
                Tightening( 5, 5, Tightening::LB ),
                Tightening( 5, 11, Tightening::UB ),

                Tightening( 6, -8, Tightening::LB ),
                Tightening( 6, 1, Tightening::UB ),
                    });

        List<Tightening> bounds;
        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );

        TS_ASSERT_EQUALS( expectedBounds.size(), bounds.size() );
        for ( const auto &bound : bounds )
            TS_ASSERT( expectedBounds.exists( bound ) );
    }

    void test_sbt_relus_active_and_externally_fixed()
    {
        NetworkLevelReasoner nlr;
        MockTableau tableau;
        nlr.setTableau( &tableau );
        populateNetworkSBT( nlr, tableau );

        tableau.setLowerBound( 0, 4 );
        tableau.setUpperBound( 0, 6 );
        tableau.setLowerBound( 1, 1 );
        tableau.setUpperBound( 1, 5 );

        // Strong negative bias for x2, which is node (1,0). Should make the node unfixed.
        nlr.setBias( 1, 0, -15 );

        // However, one of the ReLU's variables has been eliminated
        nlr.eliminateVariable( 2, -3 );

        // Invoke SBT
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );
        TS_ASSERT_THROWS_NOTHING( nlr.symbolicBoundPropagation() );

        /*
          Input ranges:

          x0: [4, 6]
          x1: [1, 5]

          Layer 1:

          x2.lb = 2x0 + 3x1 - 15   : [-4, 12]
          x2.ub = 2x0 + 3x1 - 15   : [-4, 12]

          x3.lb =  x0 +  x1   : [5, 11]
          x3.ub =  x0 +  x1   : [5, 11]

          First ReLU is inactive (set externally), bounds get zeroed
          Second ReLU is active, bounds surive the activation

          x4.lb = 0
          x4.ub = 0

          x5.lb =  x0 +  x1   : [5, 11]
          x5.ub =  x0 +  x1   : [5, 11]

          Layer 2:

          x6.lb =  - x0 - x1  : [-11, -5]
          x6.ub =  - x0 - x1  : [-11, -5]
        */

        List<Tightening> expectedBounds({
                // x2 does not appear, because it has been eliminated

                Tightening( 3, 5, Tightening::LB ),
                Tightening( 3, 11, Tightening::UB ),

                Tightening( 4, 0, Tightening::LB ),
                Tightening( 4, 0, Tightening::UB ),
                Tightening( 5, 5, Tightening::LB ),
                Tightening( 5, 11, Tightening::UB ),

                Tightening( 6, -11, Tightening::LB ),
                Tightening( 6, -5, Tightening::UB ),
                    });

        List<Tightening> bounds;
        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );

        TS_ASSERT_EQUALS( expectedBounds.size(), bounds.size() );
        for ( const auto &bound : bounds )
            TS_ASSERT( expectedBounds.exists( bound ) );
    }

    void test_sbt_abs_all_positive()
    {
        NetworkLevelReasoner nlr;
        MockTableau tableau;
        nlr.setTableau( &tableau );
        populateNetworkSBT( nlr, tableau );

        nlr.setNeuronActivationFunction( 1, 0, PiecewiseLinearFunctionType::RELU );
        nlr.setNeuronActivationFunction( 1, 1, PiecewiseLinearFunctionType::RELU );

        tableau.setLowerBound( 0, 4 );
        tableau.setUpperBound( 0, 6 );
        tableau.setLowerBound( 1, 1 );
        tableau.setUpperBound( 1, 5 );

        // Invoke SBT
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );
        TS_ASSERT_THROWS_NOTHING( nlr.symbolicBoundPropagation() );

        /*
          Input ranges:

          x0: [4, 6]
          x1: [1, 5]

          Layer 1:

          x2.lb = 2x0 + 3x1   : [11, 27]
          x2.ub = 2x0 + 3x1   : [11, 27]

          x3.lb =  x0 +  x1   : [5, 11]
          x3.ub =  x0 +  x1   : [5, 11]

          Both absolute values positive, bound survive through activations:

          x4.lb = 2x0 + 3x1   : [11, 27]
          x4.ub = 2x0 + 3x1   : [11, 27]

          x5.lb =  x0 +  x1   : [5, 11]
          x5.ub =  x0 +  x1   : [5, 11]

          Layer 2:

          x6.lb =  x0 + 2x1   : [6, 16]
          x6.ub =  x0 + 2x1   : [6, 16]
        */

        List<Tightening> expectedBounds({
                Tightening( 2, 11, Tightening::LB ),
                Tightening( 2, 27, Tightening::UB ),
                Tightening( 3, 5, Tightening::LB ),
                Tightening( 3, 11, Tightening::UB ),

                Tightening( 4, 11, Tightening::LB ),
                Tightening( 4, 27, Tightening::UB ),
                Tightening( 5, 5, Tightening::LB ),
                Tightening( 5, 11, Tightening::UB ),

                Tightening( 6, 6, Tightening::LB ),
                Tightening( 6, 16, Tightening::UB ),
                    });

        List<Tightening> bounds;
        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );

        TS_ASSERT_EQUALS( expectedBounds.size(), bounds.size() );
        for ( const auto &bound : bounds )
            TS_ASSERT( expectedBounds.exists( bound ) );
    }

    void test_sbt_abs_positive_and_negative()
    {
        NetworkLevelReasoner nlr;
        MockTableau tableau;
        nlr.setTableau( &tableau );
        populateNetworkSBT( nlr, tableau );

        nlr.setNeuronActivationFunction( 1, 0, PiecewiseLinearFunctionType::ABSOLUTE_VALUE );
        nlr.setNeuronActivationFunction( 1, 1, PiecewiseLinearFunctionType::ABSOLUTE_VALUE );

        tableau.setLowerBound( 0, 4 );
        tableau.setUpperBound( 0, 6 );
        tableau.setLowerBound( 1, 1 );
        tableau.setUpperBound( 1, 5 );

        // Strong negative bias for x2, which is node (1,0)
        nlr.setBias( 1, 0, -30 );

        // Invoke SBT
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );
        TS_ASSERT_THROWS_NOTHING( nlr.symbolicBoundPropagation() );

        /*
          Input ranges:

          x0: [4, 6]
          x1: [1, 5]

          Layer 1:

          x2.lb = 2x0 + 3x1 - 30   : [-19, -3]
          x2.ub = 2x0 + 3x1 - 30   : [-19, -3]

          x3.lb =  x0 +  x1   : [5, 11]
          x3.ub =  x0 +  x1   : [5, 11]

          First absolute value is negative, bounds get flipped
          Second absolute value is positive, bounds surive the activation

          x4.lb = -2x0 -3x1 + 30   : [3, 19]
          x4.ub = -2x0 -3x1 + 30   : [3, 19]

          x5.lb =  x0 +  x1   : [5, 11]
          x5.ub =  x0 +  x1   : [5, 11]

          Layer 2:

          x6.lb =  - 3x0 - 4x1 + 30  : [-8, 14]
          x6.ub =  - 3x0 - 4x1 + 30  : [-8, 14]
        */

        List<Tightening> expectedBounds({
                Tightening( 2, -19, Tightening::LB ),
                Tightening( 2, -3, Tightening::UB ),
                Tightening( 3, 5, Tightening::LB ),
                Tightening( 3, 11, Tightening::UB ),

                Tightening( 4, 3, Tightening::LB ),
                Tightening( 4, 19, Tightening::UB ),
                Tightening( 5, 5, Tightening::LB ),
                Tightening( 5, 11, Tightening::UB ),

                Tightening( 6, -8, Tightening::LB ),
                Tightening( 6, 14, Tightening::UB ),
                    });

        List<Tightening> bounds;
        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );
        TS_ASSERT_EQUALS( expectedBounds.size(), bounds.size() );

        for ( const auto &bound : bounds )
            TS_ASSERT( expectedBounds.exists( bound ) );
    }

    void test_sbt_absolute_values_positive_and_not_fixed()
    {
        NetworkLevelReasoner nlr;
        MockTableau tableau;
        nlr.setTableau( &tableau );
        populateNetworkSBT( nlr, tableau );

        nlr.setNeuronActivationFunction( 1, 0, PiecewiseLinearFunctionType::ABSOLUTE_VALUE );
        nlr.setNeuronActivationFunction( 1, 1, PiecewiseLinearFunctionType::ABSOLUTE_VALUE );

        tableau.setLowerBound( 0, 4 );
        tableau.setUpperBound( 0, 6 );
        tableau.setLowerBound( 1, 1 );
        tableau.setUpperBound( 1, 5 );

        // Strong negative bias for x2, which is node (1,0)
        nlr.setBias( 1, 0, -15 );

        // Invoke SBT
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );
        TS_ASSERT_THROWS_NOTHING( nlr.symbolicBoundPropagation() );

        /*
          Input ranges:

          x0: [4, 6]
          x1: [1, 5]

          Layer 1:

          x2.lb = 2x0 + 3x1 - 15   : [-4, 12]
          x2.ub = 2x0 + 3x1 - 15   : [-4, 12]

          x3.lb =  x0 +  x1   : [5, 11]
          x3.ub =  x0 +  x1   : [5, 11]

          First absolute value is undecided, bounds are concretized.
          Second ReLU is active, bounds surive the activation

          x4 range: [0, 12]
          x4.lb = 0
          x4.ub = 12

          x5.lb =  x0 +  x1   : [5, 11]
          x5.ub =  x0 +  x1   : [5, 11]

          Layer 2:

          x6.lb =  - x0 - x1       : [-11, -5]
          x6.ub =  - x0 - x1 + 12  : [  1,  7]

          x6 range: [-11, 7]
        */

        List<Tightening> expectedBounds({
                Tightening( 2, -4, Tightening::LB ),
                Tightening( 2, 12, Tightening::UB ),
                Tightening( 3, 5, Tightening::LB ),
                Tightening( 3, 11, Tightening::UB ),

                Tightening( 4, 0, Tightening::LB ),
                Tightening( 4, 12, Tightening::UB ),
                Tightening( 5, 5, Tightening::LB ),
                Tightening( 5, 11, Tightening::UB ),

                Tightening( 6, -11, Tightening::LB ),
                Tightening( 6, 7, Tightening::UB ),
                    });

        List<Tightening> bounds;
        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );

        TS_ASSERT_EQUALS( expectedBounds.size(), bounds.size() );
        for ( const auto &bound : bounds )
            TS_ASSERT( expectedBounds.exists( bound ) );
    }

    void test_sbt_absolute_values_active_and_externally_fixed()
    {
        NetworkLevelReasoner nlr;
        MockTableau tableau;
        nlr.setTableau( &tableau );
        populateNetworkSBT( nlr, tableau );

        nlr.setNeuronActivationFunction( 1, 0, PiecewiseLinearFunctionType::ABSOLUTE_VALUE );
        nlr.setNeuronActivationFunction( 1, 1, PiecewiseLinearFunctionType::ABSOLUTE_VALUE );

        tableau.setLowerBound( 0, 4 );
        tableau.setUpperBound( 0, 6 );
        tableau.setLowerBound( 1, 1 );
        tableau.setUpperBound( 1, 5 );

        // Strong negative bias for x2, which is node (1,0). Should make the node unfixed.
        nlr.setBias( 1, 0, -15 );

        // However, the weighted sum variable has been eliminated
        nlr.eliminateVariable( 2, -3 );

        // Invoke SBT
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );
        TS_ASSERT_THROWS_NOTHING( nlr.symbolicBoundPropagation() );

        /*
          Input ranges:

          x0: [4, 6]
          x1: [1, 5]

          Layer 1:

          x2.lb = 2x0 + 3x1 - 15   : [-4, 12]
          x2.ub = 2x0 + 3x1 - 15   : [-4, 12]

          x3.lb =  x0 +  x1   : [5, 11]
          x3.ub =  x0 +  x1   : [5, 11]

          First absolute value is negative (set externally), bounds get flipped
          Second absolute value is positive, bounds surive the activation

          x4.lb = -2x0 - 3x1 + 15   : [-12, 4]
          x4.ub = -2x0 - 3x1 + 15   : [-12, 4]

          Bounds for x4: [0, 4]

          x5.lb =  x0 +  x1   : [5, 11]
          x5.ub =  x0 +  x1   : [5, 11]

          Layer 2:

          x6.lb =  - 3x0 - 4x1 + 15  : [-23, -1]
          x6.ub =  - 3x0 - 4x1 + 15  : [-23, -1]
        */

        List<Tightening> expectedBounds({
                // x2 does not appear, because it has been eliminated

                Tightening( 3, 5, Tightening::LB ),
                Tightening( 3, 11, Tightening::UB ),

                Tightening( 4, 0, Tightening::LB ),
                Tightening( 4, 4, Tightening::UB ),
                Tightening( 5, 5, Tightening::LB ),
                Tightening( 5, 11, Tightening::UB ),

                Tightening( 6, -23, Tightening::LB ),
                Tightening( 6, -1, Tightening::UB ),
                    });

        List<Tightening> bounds;
        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );

        TS_ASSERT_EQUALS( expectedBounds.size(), bounds.size() );
        for ( const auto &bound : bounds )
            TS_ASSERT( expectedBounds.exists( bound ) );
    }
};

//
// Local Variables:
// compile-command: "make -C ../../.. "
// tags-file-name: "../../../TAGS"
// c-basic-offset: 4
// End:
//
