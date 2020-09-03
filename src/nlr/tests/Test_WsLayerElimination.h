
#ifndef MARABOU_TEST_WSLAYERELIMINATION_H
#define MARABOU_TEST_WSLAYERELIMINATION_H


/**********************/
/*! \file Test_WsLayerElimination.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Amir
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

**/

#include <cxxtest/TestSuite.h>

#include "../../engine/tests/MockTableau.h" // TODO: fix this
#include "FloatUtils.h"
#include "InputQuery.h"
#include "Layer.h"
#include "NetworkLevelReasoner.h"
#include "Tightening.h"
#include "Map.h"


//namespace NLR

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

    void tearDown() {
        TS_ASSERT_THROWS_NOTHING( delete mock );
    }

    // Test number 1 - a NN with no subsequent WS layers to merge
    void populateNetwork_CaseA( NLR::NetworkLevelReasoner &nlr )
    {
        /*
                a
          x           d    f
                b
          y           e    g
                c
        */

        // Create the layers
        nlr.addLayer( 0, NLR::Layer::INPUT, 2 );
        nlr.addLayer( 1, NLR::Layer::WEIGHTED_SUM, 3 );
        nlr.addLayer( 2, NLR::Layer::RELU, 3 );
        nlr.addLayer( 3, NLR::Layer::WEIGHTED_SUM, 2 );
        nlr.addLayer( 4, NLR::Layer::SIGN, 2 );
        nlr.addLayer( 5, NLR::Layer::WEIGHTED_SUM, 2 );

        // Mark layer dependencies
        for ( unsigned i = 1; i <= 5; ++i )
            nlr.addLayerDependency( i - 1, i );

        // Set the weights and biases for the weighted sum layers
        nlr.setWeight( 0, 0, 1, 0, 1 );
        nlr.setWeight( 0, 0, 1, 1, 2 );
        nlr.setWeight( 0, 1, 1, 1, -3 );
        nlr.setWeight( 0, 1, 1, 2, 1 );

        nlr.setWeight( 2, 0, 3, 0, 1 );
        nlr.setWeight( 2, 0, 3, 1, -1 );
        nlr.setWeight( 2, 1, 3, 0, 1 );
        nlr.setWeight( 2, 1, 3, 1, 1 );
        nlr.setWeight( 2, 2, 3, 0, -1 );
        nlr.setWeight( 2, 2, 3, 1, -1 );

        nlr.setWeight( 4, 0, 5, 0, 1 );
        nlr.setWeight( 4, 0, 5, 1, 1 );
        nlr.setWeight( 4, 1, 5, 1, 3 );

        nlr.setBias( 1, 0, 1 );
        nlr.setBias( 3, 1, 2 );

        // Mark the ReLU sources
        nlr.addActivationSource( 1, 0, 2, 0 );
        nlr.addActivationSource( 1, 1, 2, 1 );
        nlr.addActivationSource( 1, 2, 2, 2 );

        nlr.addActivationSource( 3, 0, 4, 0 );
        nlr.addActivationSource( 3, 1, 4, 1 );

        // Variable indexing
        nlr.setNeuronVariable( NLR::NeuronIndex( 0, 0 ), 0);
        nlr.setNeuronVariable( NLR::NeuronIndex( 0, 1 ), 1);

        nlr.setNeuronVariable( NLR::NeuronIndex( 1, 0 ), 2 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 1, 1 ), 3 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 1, 2 ), 4 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 2, 0 ), 5 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 2, 1 ), 6 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 2, 2 ), 7 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 3, 0 ), 8 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 3, 1 ), 9 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 4, 0 ), 10 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 4, 1 ), 11 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 5, 0 ), 12 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 5, 1 ), 13 );
    }

    void test_no_elimination()
    {
        unsigned originalNumberOfLayer = 6;

        NLR::NetworkLevelReasoner nlr;
        populateNetwork_CaseA( nlr );
        TS_ASSERT_EQUALS( nlr.getNumberOfLayers(), originalNumberOfLayer )

        nlr.mergeConsecutiveWSLayers();
        TS_ASSERT_EQUALS( nlr.getNumberOfLayers(), originalNumberOfLayer )

        NLR::NetworkLevelReasoner expectedNlr;
        populateNetwork_CaseA( expectedNlr );

        // Check the layers are correct
        for ( unsigned layerNumber = 0; layerNumber < originalNumberOfLayer - 1; ++layerNumber )
        {
            TS_ASSERT( *nlr.getLayer( layerNumber ) == *expectedNlr.getLayer( layerNumber ) )
        }

        // Check the NN outputs are correct
        double *input = new double[2];
        double *output = new double[2];
        double *expectedOutput = new double[2];

        for (auto i = -250; i < 250; ++i)
        {
            input[0] = ( i+4 )/2;
            input[1] = ( 2*i )/3 -3;

            nlr.evaluate( input, output );
            expectedNlr.evaluate( input, expectedOutput );

            TS_ASSERT_EQUALS( std::memcmp( output, expectedOutput, 2 ), 0 )
        }

        delete[] input;
        delete[] output;
        delete[] expectedOutput;
    }

    // Test number 2 - a simple NN with one pair of subsequent WS layers
    void populateNetwork_CaseB( NLR::NetworkLevelReasoner &nlr )
    {
        // Create the layers
        nlr.addLayer( 0, NLR::Layer::INPUT, 2 );
        nlr.addLayer( 1, NLR::Layer::WEIGHTED_SUM, 3 );
        nlr.addLayer( 2, NLR::Layer::RELU, 3 );

        // Subsequent WS layers
        nlr.addLayer( 3, NLR::Layer::WEIGHTED_SUM, 2 );
        nlr.addLayer( 4, NLR::Layer::WEIGHTED_SUM, 2 );

        nlr.addLayer( 5, NLR::Layer::SIGN, 2 );
        nlr.addLayer( 6, NLR::Layer::WEIGHTED_SUM, 2 );

        // Mark layer dependencies
        for ( unsigned i = 1; i <= 6; ++i )
            nlr.addLayerDependency( i - 1, i );

        // Set the weights and biases for the WS layers
        // [WS] 0 -> [WS] 1
        nlr.setWeight( 0, 0, 1, 0, 1 );
        nlr.setWeight( 0, 0, 1, 1, 2 );
        nlr.setWeight( 0, 1, 1, 1, -3 );
        nlr.setWeight( 0, 1, 1, 2, 1 );

        // [WS] 2 -> [WS] 3
        nlr.setWeight( 2, 0, 3, 0, 1 );
        nlr.setWeight( 2, 0, 3, 1, -1 );
        nlr.setWeight( 2, 1, 3, 0, 1 );
        nlr.setWeight( 2, 1, 3, 1, 1 );
        nlr.setWeight( 2, 2, 3, 0, -1 );
        nlr.setWeight( 2, 2, 3, 1, -1 );

        // [WS] 3 -> [WS] 4  - 2nd WS layer
        nlr.setWeight( 3, 0, 4, 0, 1 );
        nlr.setWeight( 3, 0, 4, 1, 2 );
        nlr.setWeight( 3, 1, 4, 0, 3 );
        nlr.setWeight( 3, 1, 4, 1, 4 );

        // [WS] 5 -> [WS] 6
        nlr.setWeight( 5, 0, 6, 0, 1 );
        nlr.setWeight( 5, 0, 6, 1, 1 );
        nlr.setWeight( 5, 1, 6, 1, 3 );

        nlr.setBias( 1, 0, 1 );
        nlr.setBias( 3, 0, 2 );
        nlr.setBias( 3, 1, 6 );
        nlr.setBias( 4, 0, 5 );

        // Mark the ReLU sources
        nlr.addActivationSource( 1, 0, 2, 0 );
        nlr.addActivationSource( 1, 1, 2, 1 );
        nlr.addActivationSource( 1, 2, 2, 2 );

        // Mark the SIGN sources
        nlr.addActivationSource( 4, 0, 5, 0 );
        nlr.addActivationSource( 4, 1, 5, 1 );

        // Variable indexing
        nlr.setNeuronVariable( NLR::NeuronIndex( 0, 0 ), 0 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 0, 1 ), 1 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 1, 0 ), 2 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 1, 1 ), 3 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 1, 2 ), 4 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 2, 0 ), 5 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 2, 1 ), 6 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 2, 2 ), 7 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 3, 0 ), 8 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 3, 1 ), 9 );

        // Variables of subsequent WS layer
        nlr.setNeuronVariable( NLR::NeuronIndex( 4, 0 ), 10 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 4, 1 ), 11 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 5, 0 ), 12 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 5, 1 ), 13 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 6, 0 ), 14 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 6, 1 ), 15 );
    }

    void populateNetworkAfterMerge_CaseB( NLR::NetworkLevelReasoner &nlr )
    {
        // Create the layers
        nlr.addLayer( 0, NLR::Layer::INPUT, 2 );
        nlr.addLayer( 1, NLR::Layer::WEIGHTED_SUM, 3 );
        nlr.addLayer( 2, NLR::Layer::RELU, 3 );
        nlr.addLayer( 3, NLR::Layer::WEIGHTED_SUM, 2 );
        nlr.addLayer( 4, NLR::Layer::SIGN, 2 );
        nlr.addLayer( 5, NLR::Layer::WEIGHTED_SUM, 2 );

        // Mark layer dependencies
        for ( unsigned i = 1; i <= 5; ++i )
            nlr.addLayerDependency( i - 1, i );

        // Set the weights and biases for the WS layers
        // [WS] 0 -> [WS] 1
        nlr.setWeight( 0, 0, 1, 0, 1 );
        nlr.setWeight( 0, 0, 1, 1, 2 );
        nlr.setWeight( 0, 1, 1, 1, -3 );
        nlr.setWeight( 0, 1, 1, 2, 1 );

        // [WS] 2 -> [WS] : WS layer after merging
        nlr.setWeight( 2, 0, 3, 0, -2 );
        nlr.setWeight( 2, 0, 3, 1, -2 );
        nlr.setWeight( 2, 1, 3, 0, 4 );
        nlr.setWeight( 2, 1, 3, 1, 6 );
        nlr.setWeight( 2, 2, 3, 0, -4 );
        nlr.setWeight( 2, 2, 3, 1, -6 );

        // [WS] 4 -> [WS] 5
        nlr.setWeight( 4, 0, 5, 0, 1 );
        nlr.setWeight( 4, 0, 5, 1, 1 );
        nlr.setWeight( 4, 1, 5, 1, 3 );

        nlr.setBias( 1, 0, 1 );

        // Bias of new merged WS layer: 25 = 5 + (2*1) +(3*6)
        nlr.setBias( 3, 0, 25 );

        // Mark the ReLU sources
        nlr.addActivationSource( 1, 0, 2, 0 );
        nlr.addActivationSource( 1, 1, 2, 1 );
        nlr.addActivationSource( 1, 2, 2, 2 );

        // Mark the SIGN sources
        nlr.addActivationSource( 3, 0, 4, 0 );
        nlr.addActivationSource( 3, 1, 4, 1 );

        // Variable indexing
        nlr.setNeuronVariable( NLR::NeuronIndex( 0, 0 ), 0 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 0, 1 ), 1 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 1, 0 ), 2 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 1, 1 ), 3 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 1, 2 ), 4 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 2, 0 ), 5 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 2, 1 ), 6 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 2, 2 ), 7 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 3, 0 ), 8 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 3, 1 ), 9 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 4, 0 ), 10 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 4, 1 ), 11 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 5, 0 ), 12 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 5, 1 ), 13 );
    }

    void test_eliminate_one_pair()
    {
        unsigned originalNumberOfLayer = 7;

        NLR::NetworkLevelReasoner nlr;
        populateNetwork_CaseB( nlr );

        // Before merging the layers
        TS_ASSERT_EQUALS( nlr.getNumberOfLayers(), originalNumberOfLayer )

        // After merging layers [WS] 3 & [WS] 4
        nlr.mergeConsecutiveWSLayers();
        TS_ASSERT_EQUALS( nlr.getNumberOfLayers(), originalNumberOfLayer - 1 )

        NLR::NetworkLevelReasoner expectedNlr;
        populateNetworkAfterMerge_CaseB( expectedNlr );

        // Check the layers are correct
        for (unsigned layerNumber = 0; layerNumber < originalNumberOfLayer - 1; ++layerNumber)
        {
            TS_ASSERT( *nlr.getLayer( layerNumber ) == *expectedNlr.getLayer( layerNumber ) )
        }

        // Check the NN outputs are correct
        double *input = new double[2];
        double *output = new double[2];
        double *expectedOutput = new double[2];

        for ( auto i = -250; i < 250; ++i )
        {
            input[0] = ( i-99 )/2 + 1;
            input[1] = ( 12*i )/3 -7;

            nlr.evaluate(input, output);
            expectedNlr.evaluate(input, expectedOutput);

            TS_ASSERT_EQUALS( std::memcmp( output, expectedOutput, 2 ), 0)
        }

        delete[] input;
        delete[] output;
        delete[] expectedOutput;
    }

    // Test number 3 - a NN with a single pair of subsequent WS layers, but 3 different activation
    // layers are input for the 1st WS out of the pair
    void populateNetwork_CaseC( NLR::NetworkLevelReasoner &nlr)
    {
        // Create the layers
        nlr.addLayer( 0, NLR::Layer::INPUT, 3 );
        nlr.addLayer( 1, NLR::Layer::WEIGHTED_SUM, 1 );
        nlr.addLayer( 4, NLR::Layer::SIGN, 1 );
        nlr.addLayer( 2, NLR::Layer::WEIGHTED_SUM, 1 );
        nlr.addLayer( 5, NLR::Layer::RELU, 1 );
        nlr.addLayer( 3, NLR::Layer::WEIGHTED_SUM, 1 );
        nlr.addLayer( 6, NLR::Layer::MAX, 1 );

        // subsequent WS layers
        nlr.addLayer( 7, NLR::Layer::WEIGHTED_SUM, 2 );
        nlr.addLayer( 8, NLR::Layer::WEIGHTED_SUM, 2 );

        nlr.addLayer( 9, NLR::Layer::SIGN, 2 );
        nlr.addLayer(10, NLR::Layer::WEIGHTED_SUM, 2);

        // Mark layer dependencies
        nlr.addLayerDependency( 0, 1 );
        nlr.addLayerDependency( 0, 2 );
        nlr.addLayerDependency( 0, 3 );

        nlr.addLayerDependency( 1, 4 );
        nlr.addLayerDependency( 2, 5 );
        nlr.addLayerDependency( 3, 6 );

        nlr.addLayerDependency( 4, 7 );
        nlr.addLayerDependency( 5, 7 );
        nlr.addLayerDependency( 6, 7 );

        for ( unsigned i = 8; i <= 10; ++i )
            nlr.addLayerDependency( i - 1, i );

        // Set the weights and biases for the WS layers
        // [Input] 0 -> [WS] 1
        nlr.setWeight( 0, 0, 1, 0, 1 );
        nlr.setWeight( 0, 1, 1, 0, 1 );
        nlr.setWeight( 0, 2, 1, 0, 1 );

        // [Input] 0 -> [WS] 2
        nlr.setWeight( 0, 1, 2, 0, -3 );

        // [Input] 0 -> [WS] 3
        nlr.setWeight( 0, 0, 3, 0, 2 );
        nlr.setWeight( 0, 1, 3, 0, 2 );
        nlr.setWeight( 0, 2, 3, 0, 2 );

        // [WS] 4,5,6 -> [WS] 7
        nlr.setWeight( 4, 0, 7, 0, 1 );
        nlr.setWeight( 4, 0, 7, 1, -1 );

        nlr.setWeight( 5, 0, 7, 0, 1 );
        nlr.setWeight( 5, 0, 7, 1, 1 );

        nlr.setWeight( 6, 0, 7, 0, -1 );
        nlr.setWeight( 6, 0, 7, 1, -1 );

        // [WS] 7 -> [WS] 8 : the 2nd second WS LAYER
        nlr.setWeight( 7, 0, 8, 0, 1 );
        nlr.setWeight( 7, 0, 8, 1, 2 );
        nlr.setWeight( 7, 1, 8, 0, 3 );
        nlr.setWeight( 7, 1, 8, 1, 4 );

        // 9 -> [WS] 10
        nlr.setWeight( 9, 0, 10, 0, 1 );
        nlr.setWeight( 9, 0, 10, 1, 1 );
        nlr.setWeight( 9, 1, 10, 1, 3 );

        nlr.setBias( 1, 0, 1 );
        nlr.setBias( 7, 0, 2 );
        nlr.setBias( 7, 1, 6 );

        // The bias of the 2nd subsequent WS layer
        nlr.setBias( 8, 0, 5 );

        // Mark the SIGN, ReLU, MAX sources, after the INPUT layer
        nlr.addActivationSource( 1, 0, 4, 0 );
        nlr.addActivationSource( 2, 0, 5, 0 );
        nlr.addActivationSource( 3, 0, 6, 0 );

        // Mark the SIGN sources
        nlr.addActivationSource( 8, 0, 9, 0 );
        nlr.addActivationSource( 8, 1, 9, 1 );

        // Variable indexing
        nlr.setNeuronVariable( NLR::NeuronIndex( 0, 0 ), 0 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 0, 1 ), 1 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 0, 2 ), 2 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 1, 0 ), 3 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 2, 0 ), 4 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 3, 0 ), 5 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 4, 0 ), 6 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 5, 0 ), 7 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 6, 0 ), 8 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 7, 0 ), 9 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 7, 1 ), 10 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 8, 0 ), 11 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 8, 1 ), 12 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 9, 0 ), 13 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 9, 1 ), 14 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 10, 0 ), 15 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 10, 1 ), 16 );
    }


    void populateNetworkAfterMerge_CaseC( NLR::NetworkLevelReasoner &nlr )
    {
        // Create the layers
        nlr.addLayer( 0, NLR::Layer::INPUT, 3 );
        nlr.addLayer( 1, NLR::Layer::WEIGHTED_SUM, 1 );
        nlr.addLayer( 4, NLR::Layer::SIGN, 1 );
        nlr.addLayer( 2, NLR::Layer::WEIGHTED_SUM, 1 );
        nlr.addLayer( 5, NLR::Layer::RELU, 1 );
        nlr.addLayer( 3, NLR::Layer::WEIGHTED_SUM, 1 );
        nlr.addLayer( 6, NLR::Layer::MAX, 1 );
        nlr.addLayer( 7, NLR::Layer::WEIGHTED_SUM, 2 );
        nlr.addLayer( 8, NLR::Layer::SIGN, 2 );
        nlr.addLayer( 9, NLR::Layer::WEIGHTED_SUM, 2 );

        // Mark layer dependencies
        nlr.addLayerDependency( 0, 1 );
        nlr.addLayerDependency( 0, 2 );
        nlr.addLayerDependency( 0, 3 );

        nlr.addLayerDependency( 1, 4 );
        nlr.addLayerDependency( 2, 5 );
        nlr.addLayerDependency( 3, 6 );

        nlr.addLayerDependency( 4, 7 );
        nlr.addLayerDependency( 5, 7 );
        nlr.addLayerDependency( 6, 7 );

        for ( unsigned i = 8; i <= 9; ++i )
            nlr.addLayerDependency( i - 1, i );

        // Set the weights and biases for the WS layers
        // [Input] 0 -> [WS] 1
        nlr.setWeight( 0, 0, 1, 0, 1 );
        nlr.setWeight( 0, 1, 1, 0, 1 );
        nlr.setWeight( 0, 2, 1, 0, 1 );

        // [Input] 0 -> [WS] 2
        nlr.setWeight( 0, 1, 2, 0, -3 );

        // [Input] 0 -> [WS] 3
        nlr.setWeight( 0, 0, 3, 0, 2 );
        nlr.setWeight( 0, 1, 3, 0, 2 );
        nlr.setWeight( 0, 2, 3, 0, 2 );

        // [WS] 4,5,6 -> [WS] 7 : the merged layer
        nlr.setWeight( 4, 0, 7, 0, -2 );
        nlr.setWeight( 4, 0, 7, 1, -2 );

        nlr.setWeight( 5, 0, 7, 0, 4 );
        nlr.setWeight( 5, 0, 7, 1, 6 );

        nlr.setWeight( 6, 0, 7, 0, -4 );
        nlr.setWeight( 6, 0, 7, 1, -6 );

        // 8 -> [WS] 9
        nlr.setWeight( 8, 0, 9, 0, 1 );
        nlr.setWeight( 8, 0, 9, 1, 1 );
        nlr.setWeight( 8, 1, 9, 1, 3 );

        nlr.setBias( 1, 0, 1 );

        // The bias of the 2nd subsequent WS payer
        nlr.setBias( 7, 0, 25 );

        // Mark the SIGN, ReLU, MAX sources, after the INPUT layer
        nlr.addActivationSource( 1, 0, 4, 0 );
        nlr.addActivationSource( 2, 0, 5, 0 );
        nlr.addActivationSource( 3, 0, 6, 0 );

        // Mark the SIGN sources
        nlr.addActivationSource( 7, 0, 8, 0 );
        nlr.addActivationSource( 7, 1, 8, 1 );

        // Variable indexing
        nlr.setNeuronVariable( NLR::NeuronIndex( 0, 0 ), 0 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 0, 1 ), 1 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 0, 2 ), 2 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 1, 0 ), 3 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 2, 0 ), 4 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 3, 0 ), 5 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 4, 0 ), 6 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 5, 0 ), 7 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 6, 0 ), 8 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 7, 0 ), 9 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 7, 1 ), 10 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 8, 0 ), 11 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 8, 1 ), 12 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 9, 0 ), 13 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 9, 1 ), 14 );
    }

    void test_eliminate_one_pair_with_many_inputs()
    {
        unsigned originalNumberOfLayer = 11;

        NLR::NetworkLevelReasoner nlr;
        populateNetwork_CaseC( nlr );

        // Before merging the layers
        TS_ASSERT_EQUALS( nlr.getNumberOfLayers(), originalNumberOfLayer )

        // After merging layer [WS] 7 & [WS] 8
        nlr.mergeConsecutiveWSLayers();
        TS_ASSERT_EQUALS( nlr.getNumberOfLayers(), originalNumberOfLayer - 1 )

        NLR::NetworkLevelReasoner expectedNlr;
        populateNetworkAfterMerge_CaseC( expectedNlr );

        // Check the NN outputs are correct
        double *input = new double[3];
        double *output = new double[2];
        double *expectedOutput = new double[2];

        for ( auto i = -250; i < 250; ++i )
        {
            input[0] = ( i+4 )/2;
            input[1] = ( 2*i )/3 -3;
            input[2] = 15;

            nlr.evaluate( input, output );
            expectedNlr.evaluate( input, expectedOutput );

            TS_ASSERT_EQUALS( std::memcmp( output, expectedOutput, 2 ), 0)
        }

        delete[] input;
        delete[] output;
        delete[] expectedOutput;
    }

    // Test number 4
    void populateNetwork_CaseD( NLR::NetworkLevelReasoner &nlr )
    {
        // Create the layers
        nlr.addLayer( 0, NLR::Layer::INPUT, 2 );

        nlr.addLayer( 1, NLR::Layer::WEIGHTED_SUM, 3 );
        nlr.addLayer( 2, NLR::Layer::RELU, 3 );

        // The 1st pair of subsequent WS layers
        nlr.addLayer( 3, NLR::Layer::WEIGHTED_SUM, 2 );
        nlr.addLayer( 4, NLR::Layer::WEIGHTED_SUM, 2 );

        nlr.addLayer( 5, NLR::Layer::SIGN, 2 );

        // The 2nd pair of subsequent WS layers
        nlr.addLayer( 6, NLR::Layer::WEIGHTED_SUM, 2 );
        nlr.addLayer( 7, NLR::Layer::WEIGHTED_SUM, 3 );

        // Mark layer dependencies
        for ( unsigned i = 1; i <= 7; ++i )
            nlr.addLayerDependency( i - 1, i );

        // Set the weights and biases for the WS layers
        // [WS] 0 -> [WS] 1
        nlr.setWeight( 0, 0, 1, 0, 1 );
        nlr.setWeight( 0, 0, 1, 1, 2 );
        nlr.setWeight( 0, 1, 1, 1, -3 );
        nlr.setWeight( 0, 1, 1, 2, 1 );

        // [WS] 2 -> [WS] 3
        nlr.setWeight( 2, 0, 3, 0, 1 );
        nlr.setWeight( 2, 0, 3, 1, -1 );
        nlr.setWeight( 2, 1, 3, 0, 1 );
        nlr.setWeight( 2, 1, 3, 1, 1 );
        nlr.setWeight( 2, 2, 3, 0, -1 );
        nlr.setWeight( 2, 2, 3, 1, -1 );

        // [WS] 3 -> [WS] 4
        nlr.setWeight( 3, 0, 4, 0, 1 );
        nlr.setWeight( 3, 0, 4, 1, 2 );
        nlr.setWeight( 3, 1, 4, 0, 3 );
        nlr.setWeight( 3, 1, 4, 1, 4 );

        // [WS] 5 -> [WS] 6
        nlr.setWeight( 5, 0, 6, 0, 1 );
        nlr.setWeight( 5, 0, 6, 1, 1 );
        nlr.setWeight( 5, 1, 6, 1, 3 );

        // [WS] 6 -> [WS] 7
        nlr.setWeight( 6, 0, 7, 0,-1 );
        nlr.setWeight( 6, 0, 7, 1,-2 );
        nlr.setWeight( 6, 1, 7, 1, 5 );
        nlr.setWeight( 6, 1, 7, 2, 1 );

        nlr.setBias( 1, 0, 1 );
        nlr.setBias( 3, 0, 2 );
        nlr.setBias( 3, 1, 6 );

        // The bias of the merged WS layer from the first pair of subsequent WS layers
        nlr.setBias( 4, 0, 5 );

        // Mark the ReLU sources
        nlr.addActivationSource( 1, 0, 2, 0 );
        nlr.addActivationSource( 1, 1, 2, 1 );
        nlr.addActivationSource( 1, 2, 2, 2 );

        // Mark the SIGN sources
        nlr.addActivationSource( 4, 0, 5, 0 );
        nlr.addActivationSource( 4, 1, 5, 1 );

        // Variable indexing
        nlr.setNeuronVariable( NLR::NeuronIndex( 0, 0 ), 0 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 0, 1 ), 1 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 1, 0 ), 2 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 1, 1 ), 3 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 1, 2 ), 4 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 2, 0 ), 5 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 2, 1 ), 6 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 2, 2 ), 7 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 3, 0 ), 8 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 3, 1 ), 9 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 4, 0 ), 10 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 4, 1 ), 11 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 5, 0 ), 12 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 5, 1 ), 13 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 6, 0 ), 14 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 6, 1 ), 15 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 7, 0 ), 16 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 7, 1 ), 17 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 7, 2 ), 18 );
    }

    void populateNetworkAfterMerge_CaseD( NLR::NetworkLevelReasoner &nlr )
    {
        // Create the layers
        nlr.addLayer( 0, NLR::Layer::INPUT, 2 );
        nlr.addLayer( 1, NLR::Layer::WEIGHTED_SUM, 3 );
        nlr.addLayer( 2, NLR::Layer::RELU, 3 );
        nlr.addLayer( 3, NLR::Layer::WEIGHTED_SUM, 2 );
        nlr.addLayer( 4, NLR::Layer::SIGN, 2 );
        nlr.addLayer( 5, NLR::Layer::WEIGHTED_SUM, 3 );

        // Mark layer dependencies
        for ( unsigned i = 1; i <= 5; ++i )
            nlr.addLayerDependency( i - 1, i );

        // Set the weights and biases for the WS layers
        // [WS] 0 -> [WS] 1
        nlr.setWeight( 0, 0, 1, 0, 1 );
        nlr.setWeight( 0, 0, 1, 1, 2 );
        nlr.setWeight( 0, 1, 1, 1, -3 );
        nlr.setWeight( 0, 1, 1, 2, 1 );

        // [WS] 2 -> [WS] 3
        nlr.setWeight( 2, 0, 3, 0, -2 );
        nlr.setWeight( 2, 0, 3, 1, -2 );
        nlr.setWeight( 2, 1, 3, 0, 4 );
        nlr.setWeight( 2, 1, 3, 1, 6 );
        nlr.setWeight( 2, 2, 3, 0, -4 );
        nlr.setWeight( 2, 2, 3, 1, -6 );

        // [WS] 4 -> [WS] 5
        nlr.setWeight( 4, 0, 5, 0, -1 );
        nlr.setWeight( 4, 0, 5, 1, 3 );
        nlr.setWeight( 4, 0, 5, 2, 1 );
        nlr.setWeight( 4, 1, 5, 0, 0 );
        nlr.setWeight( 4, 1, 5, 1, 15 );
        nlr.setWeight( 4, 1, 5, 2, 3 );

        nlr.setBias( 1, 0, 1 );

        // The bias of the WS layer created by the merge of the first subsequent WS layers
        nlr.setBias( 3, 0, 25 );

        // Mark the ReLU sources
        nlr.addActivationSource( 1, 0, 2, 0 );
        nlr.addActivationSource( 1, 1, 2, 1 );
        nlr.addActivationSource( 1, 2, 2, 2 );

        // Mark the SIGN sources
        nlr.addActivationSource( 3, 0, 4, 0 );
        nlr.addActivationSource( 3, 1, 4, 1 );

        // Variable indexing
        nlr.setNeuronVariable( NLR::NeuronIndex( 0, 0 ), 0 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 0, 1 ), 1 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 1, 0 ), 2 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 1, 1 ), 3 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 1, 2 ), 4 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 2, 0 ), 5 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 2, 1 ), 6 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 2, 2 ), 7 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 3, 0 ), 8 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 3, 1 ), 9 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 4, 0 ), 10 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 4, 1 ), 11 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 5, 0 ), 12 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 5, 1 ), 13 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 5, 2 ), 14 );
    }

    void test_eliminate_two_pairs()
    {
        unsigned originalNumberOfLayer = 8;

        NLR::NetworkLevelReasoner nlr;
        populateNetwork_CaseD( nlr );

        // Before merging the layers
        TS_ASSERT_EQUALS( nlr.getNumberOfLayers(), originalNumberOfLayer )

        // After merging layers [WS] 3 & [WS] 4, and also layers [WS] 6 & [WS] 7
        nlr.mergeConsecutiveWSLayers();
        TS_ASSERT_EQUALS( nlr.getNumberOfLayers(), originalNumberOfLayer - 2 )

        NLR::NetworkLevelReasoner expectedNlr;
        populateNetworkAfterMerge_CaseD( expectedNlr );

        // Check the NN outputs are correct
        double *input = new double[2];
        double *output = new double[3];
        double *expectedOutput = new double[3];

        for ( auto i = -250; i < 250; ++i ) // was -25 -> 25
        {
            input[0] = ( i+19 )/2 -7;
            input[1] = ( 3*i )/4 -1;

            nlr.evaluate(input, output);
            expectedNlr.evaluate(input, expectedOutput);

            TS_ASSERT_EQUALS( std::memcmp( output, expectedOutput, 2 ), 0)
        }

        delete[] input;
        delete[] output;
        delete[] expectedOutput;
    }

};



#endif //MARABOU_TEST_WSLAYERELIMINATION_H
