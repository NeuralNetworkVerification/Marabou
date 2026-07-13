/*********************                                                        */
/*! \file Test_PMNR.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz, Andrew Wu, Ido Shmuel
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2024 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

**/

#include "../../engine/tests/MockTableau.h" // TODO: fix this
#include "DeepPolySoftmaxElement.h"
#include "FloatUtils.h"
#include "Layer.h"
#include "NetworkLevelReasoner.h"
#include "Options.h"
#include "Query.h"
#include "Tightening.h"
#include "Vector.h"

#include <cxxtest/TestSuite.h>

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

    void populateNetworkWithAbsAndRelu( NLR::NetworkLevelReasoner &nlr, MockTableau &tableau )
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
        nlr.addLayer( 2, NLR::Layer::ABSOLUTE_VALUE, 3 );
        nlr.addLayer( 3, NLR::Layer::WEIGHTED_SUM, 2 );
        nlr.addLayer( 4, NLR::Layer::RELU, 2 );
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
        nlr.setWeight( 2, 2, 3, 1, -5 );

        nlr.setWeight( 4, 0, 5, 0, 1 );
        nlr.setWeight( 4, 0, 5, 1, 1 );
        nlr.setWeight( 4, 1, 5, 1, 3 );

        nlr.setBias( 1, 0, 1 );
        nlr.setBias( 3, 1, 2 );

        // Mark the Abs sources
        nlr.addActivationSource( 1, 0, 2, 0 );
        nlr.addActivationSource( 1, 1, 2, 1 );
        nlr.addActivationSource( 1, 2, 2, 2 );

        // Mark the ReLU sources
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

        // Very loose bounds for neurons except inputs
        double large = 1000000;

        tableau.getBoundManager().initialize( 14 );
        tableau.setLowerBound( 2, -large );
        tableau.setUpperBound( 2, large );
        tableau.setLowerBound( 3, -large );
        tableau.setUpperBound( 3, large );
        tableau.setLowerBound( 4, -large );
        tableau.setUpperBound( 4, large );
        tableau.setLowerBound( 5, -large );
        tableau.setUpperBound( 5, large );
        tableau.setLowerBound( 6, -large );
        tableau.setUpperBound( 6, large );
        tableau.setLowerBound( 7, -large );
        tableau.setUpperBound( 7, large );
        tableau.setLowerBound( 8, -large );
        tableau.setUpperBound( 8, large );
        tableau.setLowerBound( 9, -large );
        tableau.setUpperBound( 9, large );
        tableau.setLowerBound( 10, -large );
        tableau.setUpperBound( 10, large );
        tableau.setLowerBound( 11, -large );
        tableau.setUpperBound( 11, large );
        tableau.setLowerBound( 12, -large );
        tableau.setUpperBound( 12, large );
        tableau.setLowerBound( 13, -large );
        tableau.setUpperBound( 13, large );
    }

    void populateNetworkWithRoundAndSign( NLR::NetworkLevelReasoner &nlr, MockTableau &tableau )
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
        nlr.addLayer( 2, NLR::Layer::ROUND, 3 );
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

        // Mark the Round sources
        nlr.addActivationSource( 1, 0, 2, 0 );
        nlr.addActivationSource( 1, 1, 2, 1 );
        nlr.addActivationSource( 1, 2, 2, 2 );

        // Mark the Sign sources
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

        // Very loose bounds for neurons except inputs
        double large = 1000000;

        tableau.getBoundManager().initialize( 14 );
        tableau.setLowerBound( 2, -large );
        tableau.setUpperBound( 2, large );
        tableau.setLowerBound( 3, -large );
        tableau.setUpperBound( 3, large );
        tableau.setLowerBound( 4, -large );
        tableau.setUpperBound( 4, large );
        tableau.setLowerBound( 5, -large );
        tableau.setUpperBound( 5, large );
        tableau.setLowerBound( 6, -large );
        tableau.setUpperBound( 6, large );
        tableau.setLowerBound( 7, -large );
        tableau.setUpperBound( 7, large );
        tableau.setLowerBound( 8, -large );
        tableau.setUpperBound( 8, large );
        tableau.setLowerBound( 9, -large );
        tableau.setUpperBound( 9, large );
        tableau.setLowerBound( 10, -large );
        tableau.setUpperBound( 10, large );
        tableau.setLowerBound( 11, -large );
        tableau.setUpperBound( 11, large );
        tableau.setLowerBound( 12, -large );
        tableau.setUpperBound( 12, large );
        tableau.setLowerBound( 13, -large );
        tableau.setUpperBound( 13, large );
    }

    void populateNetworkWithLeakyReluAndSigmoid( NLR::NetworkLevelReasoner &nlr,
                                                 MockTableau &tableau )
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
        nlr.addLayer( 2, NLR::Layer::LEAKY_RELU, 3 );
        nlr.addLayer( 3, NLR::Layer::WEIGHTED_SUM, 2 );
        nlr.addLayer( 4, NLR::Layer::SIGMOID, 2 );
        nlr.addLayer( 5, NLR::Layer::WEIGHTED_SUM, 2 );

        nlr.getLayer( 2 )->setAlpha( 0.1 );

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

        // Mark the LeakyReLU sources
        nlr.addActivationSource( 1, 0, 2, 0 );
        nlr.addActivationSource( 1, 1, 2, 1 );
        nlr.addActivationSource( 1, 2, 2, 2 );

        // Mark the Sigmoid sources
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

        // Very loose bounds for neurons except inputs
        double large = 1000000;

        tableau.getBoundManager().initialize( 14 );
        tableau.setLowerBound( 2, -large );
        tableau.setUpperBound( 2, large );
        tableau.setLowerBound( 3, -large );
        tableau.setUpperBound( 3, large );
        tableau.setLowerBound( 4, -large );
        tableau.setUpperBound( 4, large );
        tableau.setLowerBound( 5, -large );
        tableau.setUpperBound( 5, large );
        tableau.setLowerBound( 6, -large );
        tableau.setUpperBound( 6, large );
        tableau.setLowerBound( 7, -large );
        tableau.setUpperBound( 7, large );
        tableau.setLowerBound( 8, -large );
        tableau.setUpperBound( 8, large );
        tableau.setLowerBound( 9, -large );
        tableau.setUpperBound( 9, large );
        tableau.setLowerBound( 10, -large );
        tableau.setUpperBound( 10, large );
        tableau.setLowerBound( 11, -large );
        tableau.setUpperBound( 11, large );
        tableau.setLowerBound( 12, -large );
        tableau.setUpperBound( 12, large );
        tableau.setLowerBound( 13, -large );
        tableau.setUpperBound( 13, large );
    }

    void populateNetworkWithSoftmaxAndMax( NLR::NetworkLevelReasoner &nlr, MockTableau &tableau )
    {
        /*
                a
          x           d
                b          f
          y           e
                c
        */

        // Create the layers
        nlr.addLayer( 0, NLR::Layer::INPUT, 2 );
        nlr.addLayer( 1, NLR::Layer::WEIGHTED_SUM, 3 );
        nlr.addLayer( 2, NLR::Layer::SOFTMAX, 3 );
        nlr.addLayer( 3, NLR::Layer::WEIGHTED_SUM, 2 );
        nlr.addLayer( 4, NLR::Layer::MAX, 1 );
        nlr.addLayer( 5, NLR::Layer::WEIGHTED_SUM, 1 );

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

        nlr.setWeight( 4, 0, 5, 0, -1 );

        nlr.setBias( 1, 0, 1 );
        nlr.setBias( 3, 1, 2 );

        // Mark the Softmax sources
        nlr.addActivationSource( 1, 0, 2, 0 );
        nlr.addActivationSource( 1, 0, 2, 1 );
        nlr.addActivationSource( 1, 0, 2, 2 );
        nlr.addActivationSource( 1, 1, 2, 0 );
        nlr.addActivationSource( 1, 1, 2, 1 );
        nlr.addActivationSource( 1, 1, 2, 2 );
        nlr.addActivationSource( 1, 2, 2, 0 );
        nlr.addActivationSource( 1, 2, 2, 1 );
        nlr.addActivationSource( 1, 2, 2, 2 );

        // Mark the Max sources
        nlr.addActivationSource( 3, 0, 4, 0 );
        nlr.addActivationSource( 3, 1, 4, 0 );

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

        nlr.setNeuronVariable( NLR::NeuronIndex( 5, 0 ), 11 );

        // Very loose bounds for neurons except inputs
        double large = 1000000;

        tableau.getBoundManager().initialize( 12 );
        tableau.setLowerBound( 2, -large );
        tableau.setUpperBound( 2, large );
        tableau.setLowerBound( 3, -large );
        tableau.setUpperBound( 3, large );
        tableau.setLowerBound( 4, -large );
        tableau.setUpperBound( 4, large );
        tableau.setLowerBound( 5, -large );
        tableau.setUpperBound( 5, large );
        tableau.setLowerBound( 6, -large );
        tableau.setUpperBound( 6, large );
        tableau.setLowerBound( 7, -large );
        tableau.setUpperBound( 7, large );
        tableau.setLowerBound( 8, -large );
        tableau.setUpperBound( 8, large );
        tableau.setLowerBound( 9, -large );
        tableau.setUpperBound( 9, large );
        tableau.setLowerBound( 10, -large );
        tableau.setUpperBound( 10, large );
        tableau.setLowerBound( 11, -large );
        tableau.setUpperBound( 11, large );
    }

    void populateNetworkWithReluAndBilinear( NLR::NetworkLevelReasoner &nlr, MockTableau &tableau )
    {
        /*
                a
          x           d
                b          f
          y           e
                c
        */

        // Create the layers
        nlr.addLayer( 0, NLR::Layer::INPUT, 2 );
        nlr.addLayer( 1, NLR::Layer::WEIGHTED_SUM, 3 );
        nlr.addLayer( 2, NLR::Layer::RELU, 3 );
        nlr.addLayer( 3, NLR::Layer::WEIGHTED_SUM, 2 );
        nlr.addLayer( 4, NLR::Layer::BILINEAR, 1 );
        nlr.addLayer( 5, NLR::Layer::WEIGHTED_SUM, 1 );

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

        nlr.setWeight( 4, 0, 5, 0, -1 );

        nlr.setBias( 1, 0, 1 );
        nlr.setBias( 3, 1, 2 );

        // Mark the ReLU sources
        nlr.addActivationSource( 1, 0, 2, 0 );
        nlr.addActivationSource( 1, 1, 2, 1 );
        nlr.addActivationSource( 1, 2, 2, 2 );

        // Mark the Bilinear sources
        nlr.addActivationSource( 3, 0, 4, 0 );
        nlr.addActivationSource( 3, 1, 4, 0 );

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

        nlr.setNeuronVariable( NLR::NeuronIndex( 5, 0 ), 11 );

        // Very loose bounds for neurons except inputs
        double large = 1000000;

        tableau.getBoundManager().initialize( 12 );
        tableau.setLowerBound( 2, -large );
        tableau.setUpperBound( 2, large );
        tableau.setLowerBound( 3, -large );
        tableau.setUpperBound( 3, large );
        tableau.setLowerBound( 4, -large );
        tableau.setUpperBound( 4, large );
        tableau.setLowerBound( 5, -large );
        tableau.setUpperBound( 5, large );
        tableau.setLowerBound( 6, -large );
        tableau.setUpperBound( 6, large );
        tableau.setLowerBound( 7, -large );
        tableau.setUpperBound( 7, large );
        tableau.setLowerBound( 8, -large );
        tableau.setUpperBound( 8, large );
        tableau.setLowerBound( 9, -large );
        tableau.setUpperBound( 9, large );
        tableau.setLowerBound( 10, -large );
        tableau.setUpperBound( 10, large );
        tableau.setLowerBound( 11, -large );
        tableau.setUpperBound( 11, large );
    }

    void test_backward_abs_and_relu()
    {
        Options::get()->setString( Options::SYMBOLIC_BOUND_TIGHTENING_TYPE, "sbt" );
        Options::get()->setString( Options::MILP_SOLVER_BOUND_TIGHTENING_TYPE,
                                   "backward-converge" );

        NLR::NetworkLevelReasoner nlr;
        MockTableau tableau;
        nlr.setTableau( &tableau );
        populateNetworkWithAbsAndRelu( nlr, tableau );

        tableau.setLowerBound( 0, -1 );
        tableau.setUpperBound( 0, 1 );
        tableau.setLowerBound( 1, -1 );
        tableau.setUpperBound( 1, 1 );

        // Invoke DeepPoly
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );
        TS_ASSERT_THROWS_NOTHING( nlr.deepPolyPropagation() );

        List<Tightening> expectedBounds( {
            Tightening( 2, 0, Tightening::LB ),    Tightening( 2, 2, Tightening::UB ),
            Tightening( 3, -5, Tightening::LB ),   Tightening( 3, 5, Tightening::UB ),
            Tightening( 4, -1, Tightening::LB ),   Tightening( 4, 1, Tightening::UB ),

            Tightening( 5, 0, Tightening::LB ),    Tightening( 5, 2, Tightening::UB ),
            Tightening( 6, 0, Tightening::LB ),    Tightening( 6, 5, Tightening::UB ),
            Tightening( 7, 0, Tightening::LB ),    Tightening( 7, 1, Tightening::UB ),

            Tightening( 8, -1, Tightening::LB ),   Tightening( 8, 7, Tightening::UB ),
            Tightening( 9, -5, Tightening::LB ),   Tightening( 9, 7, Tightening::UB ),

            Tightening( 10, -1, Tightening::LB ),  Tightening( 10, 7, Tightening::UB ),
            Tightening( 11, -5, Tightening::LB ),  Tightening( 11, 7, Tightening::UB ),

            Tightening( 12, -1, Tightening::LB ),  Tightening( 12, 7, Tightening::UB ),
            Tightening( 13, -14, Tightening::LB ), Tightening( 13, 26.25, Tightening::UB ),
        } );

        List<Tightening> bounds, newBounds;
        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );
        TS_ASSERT( boundsEqual( bounds, expectedBounds ) );

        // Invoke backward LP propagation
        TS_ASSERT_THROWS_NOTHING( updateTableau( tableau, bounds ) );
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );
        TS_ASSERT_THROWS_NOTHING( nlr.lpRelaxationPropagation() );

        List<Tightening> expectedBounds2( {
            Tightening( 10, 0, Tightening::LB ),
            Tightening( 11, 0, Tightening::LB ),
        } );

        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( newBounds ) );
        TS_ASSERT_THROWS_NOTHING( bounds = removeRedundancies( newBounds ) );
        TS_ASSERT( boundsEqual( bounds, expectedBounds2 ) );
    }

    void test_backward_round_and_sign()
    {
        Options::get()->setString( Options::SYMBOLIC_BOUND_TIGHTENING_TYPE, "sbt" );
        Options::get()->setString( Options::MILP_SOLVER_BOUND_TIGHTENING_TYPE,
                                   "backward-converge" );

        NLR::NetworkLevelReasoner nlr;
        MockTableau tableau;
        nlr.setTableau( &tableau );
        populateNetworkWithRoundAndSign( nlr, tableau );

        tableau.setLowerBound( 0, -1 );
        tableau.setUpperBound( 0, 1 );
        tableau.setLowerBound( 1, -1 );
        tableau.setUpperBound( 1, 1 );

        // Invoke DeepPoly
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );
        TS_ASSERT_THROWS_NOTHING( nlr.deepPolyPropagation() );

        List<Tightening> expectedBounds( {
            Tightening( 2, 0, Tightening::LB ),    Tightening( 2, 2, Tightening::UB ),
            Tightening( 3, -5, Tightening::LB ),   Tightening( 3, 5, Tightening::UB ),
            Tightening( 4, -1, Tightening::LB ),   Tightening( 4, 1, Tightening::UB ),

            Tightening( 5, 0, Tightening::LB ),    Tightening( 5, 2, Tightening::UB ),
            Tightening( 6, -5, Tightening::LB ),   Tightening( 6, 5, Tightening::UB ),
            Tightening( 7, -1, Tightening::LB ),   Tightening( 7, 1, Tightening::UB ),

            Tightening( 8, -6, Tightening::LB ),   Tightening( 8, 8, Tightening::UB ),
            Tightening( 9, -5.5, Tightening::LB ), Tightening( 9, 7.5, Tightening::UB ),

            Tightening( 10, -1, Tightening::LB ),  Tightening( 10, 1, Tightening::UB ),
            Tightening( 11, -1, Tightening::LB ),  Tightening( 11, 1, Tightening::UB ),

            Tightening( 12, -1, Tightening::LB ),  Tightening( 12, 1, Tightening::UB ),
            Tightening( 13, -4, Tightening::LB ),  Tightening( 13, 4, Tightening::UB ),
        } );

        List<Tightening> bounds, newBounds;
        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );
        TS_ASSERT( boundsEqual( bounds, expectedBounds ) );

        // Invoke backward LP propagation
        TS_ASSERT_THROWS_NOTHING( updateTableau( tableau, bounds ) );
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );
        TS_ASSERT_THROWS_NOTHING( nlr.lpRelaxationPropagation() );

        List<Tightening> expectedBounds2( {} );

        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( newBounds ) );
        TS_ASSERT_THROWS_NOTHING( bounds = removeRedundancies( newBounds ) );
        TS_ASSERT( boundsEqual( bounds, expectedBounds2 ) );
    }

    void test_backward_leaky_relu_and_sigmoid()
    {
        Options::get()->setString( Options::SYMBOLIC_BOUND_TIGHTENING_TYPE, "sbt" );
        Options::get()->setString( Options::MILP_SOLVER_BOUND_TIGHTENING_TYPE,
                                   "backward-converge" );

        NLR::NetworkLevelReasoner nlr;
        MockTableau tableau;
        nlr.setTableau( &tableau );
        populateNetworkWithLeakyReluAndSigmoid( nlr, tableau );

        tableau.setLowerBound( 0, -1 );
        tableau.setUpperBound( 0, 1 );
        tableau.setLowerBound( 1, -1 );
        tableau.setUpperBound( 1, 1 );

        // Invoke DeepPoly
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );
        TS_ASSERT_THROWS_NOTHING( nlr.deepPolyPropagation() );

        List<Tightening> expectedBounds( {
            Tightening( 2, 0, Tightening::LB ),       Tightening( 2, 2, Tightening::UB ),
            Tightening( 3, -5, Tightening::LB ),      Tightening( 3, 5, Tightening::UB ),
            Tightening( 4, -1, Tightening::LB ),      Tightening( 4, 1, Tightening::UB ),

            Tightening( 5, 0, Tightening::LB ),       Tightening( 5, 2, Tightening::UB ),
            Tightening( 6, -5, Tightening::LB ),      Tightening( 6, 5, Tightening::UB ),
            Tightening( 7, -1, Tightening::LB ),      Tightening( 7, 1, Tightening::UB ),

            Tightening( 8, -6, Tightening::LB ),      Tightening( 8, 8, Tightening::UB ),
            Tightening( 9, -4, Tightening::LB ),      Tightening( 9, 6, Tightening::UB ),

            Tightening( 10, 0.0025, Tightening::LB ), Tightening( 10, 0.9997, Tightening::UB ),
            Tightening( 11, 0.0180, Tightening::LB ), Tightening( 11, 0.9975, Tightening::UB ),

            Tightening( 12, 0.0025, Tightening::LB ), Tightening( 12, 0.9997, Tightening::UB ),
            Tightening( 13, 0.0564, Tightening::LB ), Tightening( 13, 3.9922, Tightening::UB ),
        } );

        List<Tightening> bounds, newBounds;
        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );
        TS_ASSERT( boundsEqual( bounds, expectedBounds ) );

        // Invoke backward LP propagation
        TS_ASSERT_THROWS_NOTHING( updateTableau( tableau, bounds ) );
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );
        TS_ASSERT_THROWS_NOTHING( nlr.lpRelaxationPropagation() );

        List<Tightening> expectedBounds2( {
            Tightening( 6, -0.5, Tightening::LB ),
            Tightening( 7, -0.1, Tightening::LB ),
        } );

        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( newBounds ) );
        TS_ASSERT_THROWS_NOTHING( bounds = removeRedundancies( newBounds ) );
        TS_ASSERT( boundsEqual( bounds, expectedBounds2 ) );
    }

    void test_backward_softmax_and_max()
    {
        Options::get()->setString( Options::SYMBOLIC_BOUND_TIGHTENING_TYPE, "sbt" );
        Options::get()->setString( Options::MILP_SOLVER_BOUND_TIGHTENING_TYPE,
                                   "backward-converge" );

        NLR::NetworkLevelReasoner nlr;
        MockTableau tableau;
        nlr.setTableau( &tableau );
        populateNetworkWithSoftmaxAndMax( nlr, tableau );

        tableau.setLowerBound( 0, -1 );
        tableau.setUpperBound( 0, 1 );
        tableau.setLowerBound( 1, -1 );
        tableau.setUpperBound( 1, 1 );

        // Invoke DeepPoly
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );
        TS_ASSERT_THROWS_NOTHING( nlr.deepPolyPropagation() );

        List<Tightening> expectedBounds( {
            Tightening( 2, 0, Tightening::LB ),        Tightening( 2, 2, Tightening::UB ),
            Tightening( 3, -5, Tightening::LB ),       Tightening( 3, 5, Tightening::UB ),
            Tightening( 4, -1, Tightening::LB ),       Tightening( 4, 1, Tightening::UB ),

            Tightening( 5, 0.0066, Tightening::LB ),   Tightening( 5, 0.9517, Tightening::UB ),
            Tightening( 6, 0.0007, Tightening::LB ),   Tightening( 6, 0.9909, Tightening::UB ),
            Tightening( 7, 0.0024, Tightening::LB ),   Tightening( 7, 0.7297, Tightening::UB ),

            Tightening( 8, -0.7225, Tightening::LB ),  Tightening( 8, 1.9403, Tightening::UB ),
            Tightening( 9, 0.3192, Tightening::LB ),   Tightening( 9, 2.9819, Tightening::UB ),

            Tightening( 10, 0.3192, Tightening::LB ),  Tightening( 10, 2.9819, Tightening::UB ),

            Tightening( 11, -2.9819, Tightening::LB ), Tightening( 11, -0.3192, Tightening::UB ),
        } );

        List<Tightening> bounds, newBounds;
        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );
        TS_ASSERT( boundsEqual( bounds, expectedBounds ) );

        // Invoke backward LP propagation
        TS_ASSERT_THROWS_NOTHING( updateTableau( tableau, bounds ) );
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );
        TS_ASSERT_THROWS_NOTHING( nlr.lpRelaxationPropagation() );

        List<Tightening> expectedBounds2( {} );

        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( newBounds ) );
        TS_ASSERT_THROWS_NOTHING( bounds = removeRedundancies( newBounds ) );
        TS_ASSERT( boundsEqual( bounds, expectedBounds2 ) );
    }

    void test_backward_relu_and_bilinear()
    {
        Options::get()->setString( Options::SYMBOLIC_BOUND_TIGHTENING_TYPE, "sbt" );
        Options::get()->setString( Options::MILP_SOLVER_BOUND_TIGHTENING_TYPE,
                                   "backward-converge" );

        NLR::NetworkLevelReasoner nlr;
        MockTableau tableau;
        nlr.setTableau( &tableau );
        populateNetworkWithReluAndBilinear( nlr, tableau );

        tableau.setLowerBound( 0, -1 );
        tableau.setUpperBound( 0, 1 );
        tableau.setLowerBound( 1, -1 );
        tableau.setUpperBound( 1, 1 );

        // Invoke DeepPoly
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );
        TS_ASSERT_THROWS_NOTHING( nlr.deepPolyPropagation() );

        List<Tightening> expectedBounds( {
            Tightening( 2, 0, Tightening::LB ),    Tightening( 2, 2, Tightening::UB ),
            Tightening( 3, -5, Tightening::LB ),   Tightening( 3, 5, Tightening::UB ),
            Tightening( 4, -1, Tightening::LB ),   Tightening( 4, 1, Tightening::UB ),

            Tightening( 5, 0, Tightening::LB ),    Tightening( 5, 2, Tightening::UB ),
            Tightening( 6, 0, Tightening::LB ),    Tightening( 6, 5, Tightening::UB ),
            Tightening( 7, 0, Tightening::LB ),    Tightening( 7, 1, Tightening::UB ),

            Tightening( 8, -1, Tightening::LB ),   Tightening( 8, 7, Tightening::UB ),
            Tightening( 9, -1, Tightening::LB ),   Tightening( 9, 5, Tightening::UB ),

            Tightening( 10, -7, Tightening::LB ),  Tightening( 10, 35, Tightening::UB ),

            Tightening( 11, -35, Tightening::LB ), Tightening( 11, 7, Tightening::UB ),
        } );

        List<Tightening> bounds, newBounds;
        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );
        TS_ASSERT( boundsEqual( bounds, expectedBounds ) );

        // Invoke backward LP propagation
        TS_ASSERT_THROWS_NOTHING( updateTableau( tableau, bounds ) );
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );
        TS_ASSERT_THROWS_NOTHING( nlr.lpRelaxationPropagation() );

        List<Tightening> expectedBounds2( {} );

        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( newBounds ) );
        TS_ASSERT_THROWS_NOTHING( bounds = removeRedundancies( newBounds ) );
        TS_ASSERT( boundsEqual( bounds, expectedBounds2 ) );
    }

    void test_pmnr_abs_and_relu()
    {
        Options::get()->setString( Options::SYMBOLIC_BOUND_TIGHTENING_TYPE, "sbt" );
        Options::get()->setString( Options::MILP_SOLVER_BOUND_TIGHTENING_TYPE, "backward-pmnr" );

        NLR::NetworkLevelReasoner nlr;
        MockTableau tableau;
        nlr.setTableau( &tableau );
        populateNetworkWithAbsAndRelu( nlr, tableau );

        tableau.setLowerBound( 0, -1 );
        tableau.setUpperBound( 0, 1 );
        tableau.setLowerBound( 1, -1 );
        tableau.setUpperBound( 1, 1 );

        // Invoke DeepPoly
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );
        TS_ASSERT_THROWS_NOTHING( nlr.deepPolyPropagation() );

        List<Tightening> expectedBounds( {
            Tightening( 2, 0, Tightening::LB ),    Tightening( 2, 2, Tightening::UB ),
            Tightening( 3, -5, Tightening::LB ),   Tightening( 3, 5, Tightening::UB ),
            Tightening( 4, -1, Tightening::LB ),   Tightening( 4, 1, Tightening::UB ),

            Tightening( 5, 0, Tightening::LB ),    Tightening( 5, 2, Tightening::UB ),
            Tightening( 6, 0, Tightening::LB ),    Tightening( 6, 5, Tightening::UB ),
            Tightening( 7, 0, Tightening::LB ),    Tightening( 7, 1, Tightening::UB ),

            Tightening( 8, -1, Tightening::LB ),   Tightening( 8, 7, Tightening::UB ),
            Tightening( 9, -5, Tightening::LB ),   Tightening( 9, 7, Tightening::UB ),

            Tightening( 10, -1, Tightening::LB ),  Tightening( 10, 7, Tightening::UB ),
            Tightening( 11, -5, Tightening::LB ),  Tightening( 11, 7, Tightening::UB ),

            Tightening( 12, -1, Tightening::LB ),  Tightening( 12, 7, Tightening::UB ),
            Tightening( 13, -14, Tightening::LB ), Tightening( 13, 26.25, Tightening::UB ),
        } );

        List<Tightening> bounds, newBounds;
        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );
        TS_ASSERT( boundsEqual( bounds, expectedBounds ) );

        // Invoke PMNR for neuron selection.
        TS_ASSERT_THROWS_NOTHING( updateTableau( tableau, bounds ) );
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );
        TS_ASSERT_THROWS_NOTHING( nlr.lpRelaxationPropagation() );

        List<Tightening> expectedBounds2( {
            Tightening( 10, 0, Tightening::LB ),
            Tightening( 11, 0, Tightening::LB ),

            Tightening( 12, 0, Tightening::LB ),
            Tightening( 13, 0, Tightening::LB ),
            Tightening( 13, 26, Tightening::UB ),
        } );

        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( newBounds ) );
        TS_ASSERT_THROWS_NOTHING( bounds = removeRedundancies( newBounds, bounds ) );
        TS_ASSERT( boundsEqual( bounds, expectedBounds2 ) );

        List<Map<NLR::NeuronIndex, unsigned>> infeasibleBranches( {} );
        List<Map<NLR::NeuronIndex, unsigned>> expectedInfeasibleBranches( {} );
        TS_ASSERT_THROWS_NOTHING( nlr.getInfeasibleBranches( infeasibleBranches ) );
        TS_ASSERT( infeasibleBranchesEqual( infeasibleBranches, expectedInfeasibleBranches ) );
    }

    void test_pmnr_round_and_sign()
    {
        Options::get()->setString( Options::SYMBOLIC_BOUND_TIGHTENING_TYPE, "sbt" );
        Options::get()->setString( Options::MILP_SOLVER_BOUND_TIGHTENING_TYPE, "backward-pmnr" );

        NLR::NetworkLevelReasoner nlr;
        MockTableau tableau;
        nlr.setTableau( &tableau );
        populateNetworkWithRoundAndSign( nlr, tableau );

        tableau.setLowerBound( 0, -1 );
        tableau.setUpperBound( 0, 1 );
        tableau.setLowerBound( 1, -1 );
        tableau.setUpperBound( 1, 1 );

        // Invoke DeepPoly
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );
        TS_ASSERT_THROWS_NOTHING( nlr.deepPolyPropagation() );

        List<Tightening> expectedBounds( {
            Tightening( 2, 0, Tightening::LB ),    Tightening( 2, 2, Tightening::UB ),
            Tightening( 3, -5, Tightening::LB ),   Tightening( 3, 5, Tightening::UB ),
            Tightening( 4, -1, Tightening::LB ),   Tightening( 4, 1, Tightening::UB ),

            Tightening( 5, 0, Tightening::LB ),    Tightening( 5, 2, Tightening::UB ),
            Tightening( 6, -5, Tightening::LB ),   Tightening( 6, 5, Tightening::UB ),
            Tightening( 7, -1, Tightening::LB ),   Tightening( 7, 1, Tightening::UB ),

            Tightening( 8, -6, Tightening::LB ),   Tightening( 8, 8, Tightening::UB ),
            Tightening( 9, -5.5, Tightening::LB ), Tightening( 9, 7.5, Tightening::UB ),

            Tightening( 10, -1, Tightening::LB ),  Tightening( 10, 1, Tightening::UB ),
            Tightening( 11, -1, Tightening::LB ),  Tightening( 11, 1, Tightening::UB ),

            Tightening( 12, -1, Tightening::LB ),  Tightening( 12, 1, Tightening::UB ),
            Tightening( 13, -4, Tightening::LB ),  Tightening( 13, 4, Tightening::UB ),
        } );

        List<Tightening> bounds, newBounds;
        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );
        TS_ASSERT( boundsEqual( bounds, expectedBounds ) );

        // Invoke PMNR for neuron selection.
        TS_ASSERT_THROWS_NOTHING( updateTableau( tableau, bounds ) );
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );
        TS_ASSERT_THROWS_NOTHING( nlr.lpRelaxationPropagation() );

        List<Tightening> expectedBounds2(
            { Tightening( 9, -4.75, Tightening::LB ), Tightening( 9, 6.75, Tightening::UB ) } );

        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( newBounds ) );
        TS_ASSERT_THROWS_NOTHING( bounds = removeRedundancies( newBounds, bounds ) );
        TS_ASSERT( boundsEqual( bounds, expectedBounds2 ) );

        List<Map<NLR::NeuronIndex, unsigned>> infeasibleBranches( {} );
        List<Map<NLR::NeuronIndex, unsigned>> expectedInfeasibleBranches( {} );
        TS_ASSERT_THROWS_NOTHING( nlr.getInfeasibleBranches( infeasibleBranches ) );
        TS_ASSERT( infeasibleBranchesEqual( infeasibleBranches, expectedInfeasibleBranches ) );
    }

    void test_pmnr_leaky_relu_and_sigmoid()
    {
        Options::get()->setString( Options::SYMBOLIC_BOUND_TIGHTENING_TYPE, "sbt" );
        Options::get()->setString( Options::MILP_SOLVER_BOUND_TIGHTENING_TYPE, "backward-pmnr" );

        NLR::NetworkLevelReasoner nlr;
        MockTableau tableau;
        nlr.setTableau( &tableau );
        populateNetworkWithLeakyReluAndSigmoid( nlr, tableau );

        tableau.setLowerBound( 0, -1 );
        tableau.setUpperBound( 0, 1 );
        tableau.setLowerBound( 1, -1 );
        tableau.setUpperBound( 1, 1 );

        // Invoke DeepPoly
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );
        TS_ASSERT_THROWS_NOTHING( nlr.deepPolyPropagation() );

        List<Tightening> expectedBounds( {
            Tightening( 2, 0, Tightening::LB ),       Tightening( 2, 2, Tightening::UB ),
            Tightening( 3, -5, Tightening::LB ),      Tightening( 3, 5, Tightening::UB ),
            Tightening( 4, -1, Tightening::LB ),      Tightening( 4, 1, Tightening::UB ),

            Tightening( 5, 0, Tightening::LB ),       Tightening( 5, 2, Tightening::UB ),
            Tightening( 6, -5, Tightening::LB ),      Tightening( 6, 5, Tightening::UB ),
            Tightening( 7, -1, Tightening::LB ),      Tightening( 7, 1, Tightening::UB ),

            Tightening( 8, -6, Tightening::LB ),      Tightening( 8, 8, Tightening::UB ),
            Tightening( 9, -4, Tightening::LB ),      Tightening( 9, 6, Tightening::UB ),

            Tightening( 10, 0.0025, Tightening::LB ), Tightening( 10, 0.9997, Tightening::UB ),
            Tightening( 11, 0.0180, Tightening::LB ), Tightening( 11, 0.9975, Tightening::UB ),

            Tightening( 12, 0.0025, Tightening::LB ), Tightening( 12, 0.9997, Tightening::UB ),
            Tightening( 13, 0.0564, Tightening::LB ), Tightening( 13, 3.9922, Tightening::UB ),
        } );

        List<Tightening> bounds, newBounds;
        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );
        TS_ASSERT( boundsEqual( bounds, expectedBounds ) );

        // Invoke PMNR for neuron selection.
        TS_ASSERT_THROWS_NOTHING( updateTableau( tableau, bounds ) );
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );
        TS_ASSERT_THROWS_NOTHING( nlr.lpRelaxationPropagation() );

        List<Tightening> expectedBounds2( {
            Tightening( 6, -0.5, Tightening::LB ),
            Tightening( 7, -0.1, Tightening::LB ),
            Tightening( 8, -1.5, Tightening::LB ),
            Tightening( 8, 7.1, Tightening::UB ),
            Tightening( 9, -1.1, Tightening::LB ),
            Tightening( 9, 5.1, Tightening::UB ),
            Tightening( 10, 0.1541, Tightening::LB ),
            Tightening( 10, 0.9992, Tightening::UB ),
            Tightening( 11, 0.2422, Tightening::LB ),
            Tightening( 11, 0.9942, Tightening::UB ),
            Tightening( 12, 0.1541, Tightening::LB ),
            Tightening( 12, 0.9992, Tightening::UB ),
            Tightening( 13, 0.8827, Tightening::LB ),
            Tightening( 13, 3.9817, Tightening::UB ),
        } );

        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( newBounds ) );
        TS_ASSERT_THROWS_NOTHING( bounds = removeRedundancies( newBounds, bounds ) );
        TS_ASSERT( boundsEqual( bounds, expectedBounds2 ) );

        List<Map<NLR::NeuronIndex, unsigned>> infeasibleBranches( {} );
        List<Map<NLR::NeuronIndex, unsigned>> expectedInfeasibleBranches( {} );
        TS_ASSERT_THROWS_NOTHING( nlr.getInfeasibleBranches( infeasibleBranches ) );
        TS_ASSERT( infeasibleBranchesEqual( infeasibleBranches, expectedInfeasibleBranches ) );
    }

    void test_pmnr_softmax_and_max()
    {
        Options::get()->setString( Options::SYMBOLIC_BOUND_TIGHTENING_TYPE, "sbt" );
        Options::get()->setString( Options::MILP_SOLVER_BOUND_TIGHTENING_TYPE, "backward-pmnr" );

        NLR::NetworkLevelReasoner nlr;
        MockTableau tableau;
        nlr.setTableau( &tableau );
        populateNetworkWithSoftmaxAndMax( nlr, tableau );

        tableau.setLowerBound( 0, -1 );
        tableau.setUpperBound( 0, 1 );
        tableau.setLowerBound( 1, -1 );
        tableau.setUpperBound( 1, 1 );

        // Invoke DeepPoly
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );
        TS_ASSERT_THROWS_NOTHING( nlr.deepPolyPropagation() );

        List<Tightening> expectedBounds( {
            Tightening( 2, 0, Tightening::LB ),        Tightening( 2, 2, Tightening::UB ),
            Tightening( 3, -5, Tightening::LB ),       Tightening( 3, 5, Tightening::UB ),
            Tightening( 4, -1, Tightening::LB ),       Tightening( 4, 1, Tightening::UB ),

            Tightening( 5, 0.0066, Tightening::LB ),   Tightening( 5, 0.9517, Tightening::UB ),
            Tightening( 6, 0.0007, Tightening::LB ),   Tightening( 6, 0.9909, Tightening::UB ),
            Tightening( 7, 0.0024, Tightening::LB ),   Tightening( 7, 0.7297, Tightening::UB ),

            Tightening( 8, -0.7225, Tightening::LB ),  Tightening( 8, 1.9403, Tightening::UB ),
            Tightening( 9, 0.3192, Tightening::LB ),   Tightening( 9, 2.9819, Tightening::UB ),

            Tightening( 10, 0.3192, Tightening::LB ),  Tightening( 10, 2.9819, Tightening::UB ),

            Tightening( 11, -2.9819, Tightening::LB ), Tightening( 11, -0.3192, Tightening::UB ),
        } );

        List<Tightening> bounds, newBounds;
        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );
        TS_ASSERT( boundsEqual( bounds, expectedBounds ) );

        // Invoke PMNR for neuron selection.
        TS_ASSERT_THROWS_NOTHING( updateTableau( tableau, bounds ) );
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );
        TS_ASSERT_THROWS_NOTHING( nlr.lpRelaxationPropagation() );

        List<Tightening> expectedBounds2( {
            Tightening( 8, -0.6812, Tightening::LB ),
            Tightening( 8, 1.6790, Tightening::UB ),
        } );

        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( newBounds ) );
        TS_ASSERT_THROWS_NOTHING( bounds = removeRedundancies( newBounds, bounds ) );
        TS_ASSERT( boundsEqual( bounds, expectedBounds2 ) );

        List<Map<NLR::NeuronIndex, unsigned>> infeasibleBranches( {} );
        List<Map<NLR::NeuronIndex, unsigned>> expectedInfeasibleBranches( {} );
        TS_ASSERT_THROWS_NOTHING( nlr.getInfeasibleBranches( infeasibleBranches ) );
        TS_ASSERT( infeasibleBranchesEqual( infeasibleBranches, expectedInfeasibleBranches ) );
    }

    void test_pmnr_relu_and_bilinear()
    {
        Options::get()->setString( Options::SYMBOLIC_BOUND_TIGHTENING_TYPE, "sbt" );
        Options::get()->setString( Options::MILP_SOLVER_BOUND_TIGHTENING_TYPE, "backward-pmnr" );

        NLR::NetworkLevelReasoner nlr;
        MockTableau tableau;
        nlr.setTableau( &tableau );
        populateNetworkWithReluAndBilinear( nlr, tableau );

        tableau.setLowerBound( 0, -1 );
        tableau.setUpperBound( 0, 1 );
        tableau.setLowerBound( 1, -1 );
        tableau.setUpperBound( 1, 1 );

        // Invoke DeepPoly
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );
        TS_ASSERT_THROWS_NOTHING( nlr.deepPolyPropagation() );

        List<Tightening> expectedBounds( {
            Tightening( 2, 0, Tightening::LB ),    Tightening( 2, 2, Tightening::UB ),
            Tightening( 3, -5, Tightening::LB ),   Tightening( 3, 5, Tightening::UB ),
            Tightening( 4, -1, Tightening::LB ),   Tightening( 4, 1, Tightening::UB ),

            Tightening( 5, 0, Tightening::LB ),    Tightening( 5, 2, Tightening::UB ),
            Tightening( 6, 0, Tightening::LB ),    Tightening( 6, 5, Tightening::UB ),
            Tightening( 7, 0, Tightening::LB ),    Tightening( 7, 1, Tightening::UB ),

            Tightening( 8, -1, Tightening::LB ),   Tightening( 8, 7, Tightening::UB ),
            Tightening( 9, -1, Tightening::LB ),   Tightening( 9, 5, Tightening::UB ),

            Tightening( 10, -7, Tightening::LB ),  Tightening( 10, 35, Tightening::UB ),

            Tightening( 11, -35, Tightening::LB ), Tightening( 11, 7, Tightening::UB ),
        } );

        List<Tightening> bounds, newBounds;
        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );
        TS_ASSERT( boundsEqual( bounds, expectedBounds ) );

        // Invoke PMNR for neuron selection.
        TS_ASSERT_THROWS_NOTHING( updateTableau( tableau, bounds ) );
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );
        TS_ASSERT_THROWS_NOTHING( nlr.lpRelaxationPropagation() );

        List<Tightening> expectedBounds2( {} );

        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( newBounds ) );
        bounds = removeRedundancies( newBounds, bounds );
        TS_ASSERT( boundsEqual( bounds, expectedBounds2 ) );

        List<Map<NLR::NeuronIndex, unsigned>> infeasibleBranches( {} );
        List<Map<NLR::NeuronIndex, unsigned>> expectedInfeasibleBranches( {} );
        TS_ASSERT_THROWS_NOTHING( nlr.getInfeasibleBranches( infeasibleBranches ) );
        TS_ASSERT( infeasibleBranchesEqual( infeasibleBranches, expectedInfeasibleBranches ) );
    }

    bool boundsEqual( const List<Tightening> &bounds, const List<Tightening> &expectedBounds )
    {
        if ( bounds.size() != expectedBounds.size() )
            return false;

        bool allFound = true;
        for ( const auto &bound : bounds )
        {
            bool currentFound = false;
            for ( const auto &expectedBound : expectedBounds )
            {
                currentFound |=
                    ( bound._type == expectedBound._type &&
                      bound._variable == expectedBound._variable &&
                      FloatUtils::areEqual( bound._value, expectedBound._value, 0.0001 ) );
            }
            allFound &= currentFound;
        }
        return allFound;
    }

    bool infeasibleBranchesEqual(
        const List<Map<NLR::NeuronIndex, unsigned>> &infeasibleBranches,
        const List<Map<NLR::NeuronIndex, unsigned>> &expectedInfeasibleBranches )
    {
        if ( infeasibleBranches.size() != expectedInfeasibleBranches.size() )
            return false;

        bool allFound = true;
        for ( const auto &neuronToBranchIndex : infeasibleBranches )
        {
            bool currentFound = false;
            for ( const auto &expectedNeuronToBranchIndex : expectedInfeasibleBranches )
            {
                currentFound |=
                    neuronToBranchIndexEqual( neuronToBranchIndex, expectedNeuronToBranchIndex );
            }
            allFound &= currentFound;
        }
        return allFound;
    }

    bool
    neuronToBranchIndexEqual( const Map<NLR::NeuronIndex, unsigned> &neuronToBranchIndex,
                              const Map<NLR::NeuronIndex, unsigned> &expectedneuronToBranchIndex )
    {
        if ( neuronToBranchIndex.size() != expectedneuronToBranchIndex.size() )
            return false;

        bool allFound = true;
        for ( const auto &pair : neuronToBranchIndex )
        {
            bool currentFound = false;
            for ( const auto &expectedPair : expectedneuronToBranchIndex )
            {
                currentFound |= ( pair.first._layer == expectedPair.first._layer &&
                                  pair.first._neuron == expectedPair.first._neuron &&
                                  pair.second == expectedPair.second );
            }
            allFound &= currentFound;
        }
        return allFound;
    }

    // Create list of all tightenings in bounds for which there is no bound which is
    // at least as tight.
    List<Tightening> removeRedundancies( const List<Tightening> &bounds )
    {
        List<Tightening> minimalBounds;
        unsigned i = 0;
        for ( const auto &bound : bounds )
        {
            bool foundTighter = false;
            unsigned j = 0;
            for ( const auto &otherBound : bounds )
            {
                if ( i < j )
                {
                    foundTighter |=
                        ( bound._type == otherBound._type &&
                          bound._variable == otherBound._variable &&
                          ( ( bound._type == Tightening::LB &&
                              FloatUtils::lte( bound._value, otherBound._value, 0.0001 ) ) ||
                            ( bound._type == Tightening::UB &&
                              FloatUtils::gte( bound._value, otherBound._value, 0.0001 ) ) ) );
                }
                ++j;
            }
            if ( !foundTighter )
            {
                minimalBounds.append( bound );
            }
            ++i;
        }
        return minimalBounds;
    }

    // Create list of all tightenings in bounds for which there is no bound in newBounds
    // or in previousBounds which is at least as tight.
    List<Tightening> removeRedundancies( const List<Tightening> &newBounds,
                                         const List<Tightening> &bounds )
    {
        List<Tightening> minimalBounds;
        unsigned i = 0;
        for ( const auto &bound : newBounds )
        {
            bool foundTighter = false;
            unsigned j = 0;
            for ( const auto &otherBound : newBounds )
            {
                if ( i < j )
                {
                    foundTighter |=
                        ( bound._type == otherBound._type &&
                          bound._variable == otherBound._variable &&
                          ( ( bound._type == Tightening::LB &&
                              FloatUtils::lte( bound._value, otherBound._value, 0.0001 ) ) ||
                            ( bound._type == Tightening::UB &&
                              FloatUtils::gte( bound._value, otherBound._value, 0.0001 ) ) ) );
                }
                ++j;
            }
            for ( const auto &otherBound : bounds )
            {
                foundTighter |=
                    ( bound._type == otherBound._type && bound._variable == otherBound._variable &&
                      ( ( bound._type == Tightening::LB &&
                          FloatUtils::lte( bound._value, otherBound._value, 0.0001 ) ) ||
                        ( bound._type == Tightening::UB &&
                          FloatUtils::gte( bound._value, otherBound._value, 0.0001 ) ) ) );
            }
            if ( !foundTighter )
            {
                minimalBounds.append( bound );
            }
            ++i;
        }
        return minimalBounds;
    }

    void updateTableau( MockTableau &tableau, List<Tightening> &tightenings )
    {
        for ( const auto &tightening : tightenings )
        {
            if ( tightening._type == Tightening::LB )
            {
                tableau.setLowerBound( tightening._variable, tightening._value );
            }

            if ( tightening._type == Tightening::UB )
            {
                tableau.setUpperBound( tightening._variable, tightening._value );
            }
        }
    }
};
