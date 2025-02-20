/*********************                                                        */
/*! \file Test_NetworkLevelReasoner.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz, Andrew Wu
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

    void populateNetwork( NLR::NetworkLevelReasoner &nlr )
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
        nlr.setNeuronVariable( NLR::NeuronIndex( 0, 0 ), 0 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 0, 1 ), 1 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 1, 0 ), 2 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 1, 1 ), 4 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 1, 2 ), 6 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 2, 0 ), 3 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 2, 1 ), 5 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 2, 2 ), 7 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 3, 0 ), 8 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 3, 1 ), 10 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 4, 0 ), 9 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 4, 1 ), 11 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 5, 0 ), 12 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 5, 1 ), 13 );
    }

    void populateNetworkWithSigmoids( NLR::NetworkLevelReasoner &nlr )
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
        nlr.addLayer( 2, NLR::Layer::SIGMOID, 3 );
        nlr.addLayer( 3, NLR::Layer::WEIGHTED_SUM, 2 );
        nlr.addLayer( 4, NLR::Layer::SIGMOID, 2 );
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
        nlr.setNeuronVariable( NLR::NeuronIndex( 0, 0 ), 0 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 0, 1 ), 1 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 1, 0 ), 2 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 1, 1 ), 4 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 1, 2 ), 6 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 2, 0 ), 3 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 2, 1 ), 5 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 2, 2 ), 7 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 3, 0 ), 8 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 3, 1 ), 10 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 4, 0 ), 9 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 4, 1 ), 11 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 5, 0 ), 12 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 5, 1 ), 13 );
    }

    void populateNetworkWithAbs( NLR::NetworkLevelReasoner &nlr )
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
        nlr.addLayer( 4, NLR::Layer::ABSOLUTE_VALUE, 2 );
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

        // Mark the Abs sources
        nlr.addActivationSource( 1, 0, 2, 0 );
        nlr.addActivationSource( 1, 1, 2, 1 );
        nlr.addActivationSource( 1, 2, 2, 2 );

        nlr.addActivationSource( 3, 0, 4, 0 );
        nlr.addActivationSource( 3, 1, 4, 1 );

        // Variable indexing
        nlr.setNeuronVariable( NLR::NeuronIndex( 0, 0 ), 0 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 0, 1 ), 1 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 1, 0 ), 2 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 1, 1 ), 4 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 1, 2 ), 6 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 2, 0 ), 3 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 2, 1 ), 5 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 2, 2 ), 7 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 3, 0 ), 8 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 3, 1 ), 10 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 4, 0 ), 9 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 4, 1 ), 11 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 5, 0 ), 12 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 5, 1 ), 13 );
    }

    void populateNetworkWithSign( NLR::NetworkLevelReasoner &nlr )
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
        nlr.addLayer( 2, NLR::Layer::SIGN, 3 );
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

        // Mark the Sign sources
        nlr.addActivationSource( 1, 0, 2, 0 );
        nlr.addActivationSource( 1, 1, 2, 1 );
        nlr.addActivationSource( 1, 2, 2, 2 );

        nlr.addActivationSource( 3, 0, 4, 0 );
        nlr.addActivationSource( 3, 1, 4, 1 );

        // Variable indexing
        nlr.setNeuronVariable( NLR::NeuronIndex( 0, 0 ), 0 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 0, 1 ), 1 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 1, 0 ), 2 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 1, 1 ), 4 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 1, 2 ), 6 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 2, 0 ), 3 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 2, 1 ), 5 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 2, 2 ), 7 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 3, 0 ), 8 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 3, 1 ), 10 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 4, 0 ), 9 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 4, 1 ), 11 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 5, 0 ), 12 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 5, 1 ), 13 );
    }

    void populateNetworkWithRound( NLR::NetworkLevelReasoner &nlr )
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
        nlr.addLayer( 4, NLR::Layer::ROUND, 2 );
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

        nlr.addActivationSource( 3, 0, 4, 0 );
        nlr.addActivationSource( 3, 1, 4, 1 );

        // Variable indexing
        nlr.setNeuronVariable( NLR::NeuronIndex( 0, 0 ), 0 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 0, 1 ), 1 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 1, 0 ), 2 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 1, 1 ), 4 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 1, 2 ), 6 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 2, 0 ), 3 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 2, 1 ), 5 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 2, 2 ), 7 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 3, 0 ), 8 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 3, 1 ), 10 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 4, 0 ), 9 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 4, 1 ), 11 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 5, 0 ), 12 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 5, 1 ), 13 );
    }

    void populateNetworkWithLeakyRelu( NLR::NetworkLevelReasoner &nlr )
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
        nlr.addLayer( 4, NLR::Layer::LEAKY_RELU, 2 );
        nlr.addLayer( 5, NLR::Layer::WEIGHTED_SUM, 2 );

        nlr.getLayer( 2 )->setAlpha( 0.1 );
        nlr.getLayer( 4 )->setAlpha( 0.1 );

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

        nlr.addActivationSource( 3, 0, 4, 0 );
        nlr.addActivationSource( 3, 1, 4, 1 );

        // Variable indexing
        nlr.setNeuronVariable( NLR::NeuronIndex( 0, 0 ), 0 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 0, 1 ), 1 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 1, 0 ), 2 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 1, 1 ), 4 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 1, 2 ), 6 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 2, 0 ), 3 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 2, 1 ), 5 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 2, 2 ), 7 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 3, 0 ), 8 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 3, 1 ), 10 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 4, 0 ), 9 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 4, 1 ), 11 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 5, 0 ), 12 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 5, 1 ), 13 );
    }


    void populateNetworkWithMax( NLR::NetworkLevelReasoner &nlr )
    {
        /*
                a
          x     b      e
                              g
          y     c      f
                d
        */

        // Create the layers
        nlr.addLayer( 0, NLR::Layer::INPUT, 2 );
        nlr.addLayer( 1, NLR::Layer::WEIGHTED_SUM, 4 );
        nlr.addLayer( 2, NLR::Layer::MAX, 2 );
        nlr.addLayer( 3, NLR::Layer::WEIGHTED_SUM, 2 );
        nlr.addLayer( 4, NLR::Layer::MAX, 1 );
        nlr.addLayer( 5, NLR::Layer::WEIGHTED_SUM, 1 );

        // Mark layer dependencies
        for ( unsigned i = 1; i <= 5; ++i )
            nlr.addLayerDependency( i - 1, i );

        // Set the weights and biases for the weighted sum layers
        nlr.setWeight( 0, 0, 1, 0, 1 );
        nlr.setWeight( 0, 0, 1, 1, -2 );
        nlr.setWeight( 0, 0, 1, 2, 1 );
        nlr.setWeight( 0, 1, 1, 1, -3 );
        nlr.setWeight( 0, 1, 1, 2, 2 );
        nlr.setWeight( 0, 1, 1, 3, -3 );

        nlr.setWeight( 2, 0, 3, 0, 1 );
        nlr.setWeight( 2, 0, 3, 1, -1 );
        nlr.setWeight( 2, 1, 3, 0, 1 );
        nlr.setWeight( 2, 1, 3, 1, 1 );

        nlr.setWeight( 4, 0, 5, 0, -1 );

        nlr.setBias( 1, 0, 1 );
        nlr.setBias( 3, 0, 2 );

        // Mark the Max sources
        nlr.addActivationSource( 1, 0, 2, 0 );
        nlr.addActivationSource( 1, 1, 2, 0 );
        nlr.addActivationSource( 1, 2, 2, 1 );
        nlr.addActivationSource( 1, 3, 2, 1 );

        nlr.addActivationSource( 3, 0, 4, 0 );
        nlr.addActivationSource( 3, 1, 4, 0 );

        // Variable indexing
        nlr.setNeuronVariable( NLR::NeuronIndex( 0, 0 ), 0 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 0, 1 ), 1 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 1, 0 ), 2 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 1, 1 ), 3 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 1, 2 ), 4 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 1, 3 ), 5 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 2, 0 ), 6 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 2, 1 ), 7 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 3, 0 ), 8 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 3, 1 ), 9 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 4, 0 ), 10 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 5, 0 ), 11 );
    }

    void populateNetworkWithSoftmax( NLR::NetworkLevelReasoner &nlr )
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
        nlr.addLayer( 2, NLR::Layer::SOFTMAX, 3 );
        nlr.addLayer( 3, NLR::Layer::WEIGHTED_SUM, 2 );
        nlr.addLayer( 4, NLR::Layer::SOFTMAX, 2 );
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

        nlr.addActivationSource( 3, 0, 4, 0 );
        nlr.addActivationSource( 3, 0, 4, 1 );
        nlr.addActivationSource( 3, 1, 4, 0 );
        nlr.addActivationSource( 3, 1, 4, 1 );

        // Variable indexing
        nlr.setNeuronVariable( NLR::NeuronIndex( 0, 0 ), 0 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 0, 1 ), 1 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 1, 0 ), 2 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 1, 1 ), 4 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 1, 2 ), 6 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 2, 0 ), 3 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 2, 1 ), 5 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 2, 2 ), 7 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 3, 0 ), 8 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 3, 1 ), 10 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 4, 0 ), 9 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 4, 1 ), 11 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 5, 0 ), 12 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 5, 1 ), 13 );
    }

    void populateNetworkWithBilinear( NLR::NetworkLevelReasoner &nlr )
    {
        /*
                a
          x     b      e
                              g
          y     c      f
                d
        */

        // Create the layers
        nlr.addLayer( 0, NLR::Layer::INPUT, 2 );
        nlr.addLayer( 1, NLR::Layer::WEIGHTED_SUM, 4 );
        nlr.addLayer( 2, NLR::Layer::BILINEAR, 2 );
        nlr.addLayer( 3, NLR::Layer::WEIGHTED_SUM, 2 );
        nlr.addLayer( 4, NLR::Layer::BILINEAR, 1 );
        nlr.addLayer( 5, NLR::Layer::WEIGHTED_SUM, 1 );

        // Mark layer dependencies
        for ( unsigned i = 1; i <= 5; ++i )
            nlr.addLayerDependency( i - 1, i );

        // Set the weights and biases for the weighted sum layers
        nlr.setWeight( 0, 0, 1, 0, 1 );
        nlr.setWeight( 0, 0, 1, 1, -2 );
        nlr.setWeight( 0, 0, 1, 2, 1 );
        nlr.setWeight( 0, 1, 1, 1, -3 );
        nlr.setWeight( 0, 1, 1, 2, 2 );
        nlr.setWeight( 0, 1, 1, 3, -3 );

        nlr.setWeight( 2, 0, 3, 0, 1 );
        nlr.setWeight( 2, 0, 3, 1, -1 );
        nlr.setWeight( 2, 1, 3, 0, 1 );
        nlr.setWeight( 2, 1, 3, 1, 1 );

        nlr.setWeight( 4, 0, 5, 0, -1 );

        nlr.setBias( 1, 0, 1 );
        nlr.setBias( 3, 0, 2 );

        // Mark the Bilinear sources
        nlr.addActivationSource( 1, 0, 2, 0 );
        nlr.addActivationSource( 1, 1, 2, 0 );
        nlr.addActivationSource( 1, 2, 2, 1 );
        nlr.addActivationSource( 1, 3, 2, 1 );

        nlr.addActivationSource( 3, 0, 4, 0 );
        nlr.addActivationSource( 3, 1, 4, 0 );

        // Variable indexing
        nlr.setNeuronVariable( NLR::NeuronIndex( 0, 0 ), 0 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 0, 1 ), 1 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 1, 0 ), 2 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 1, 1 ), 3 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 1, 2 ), 4 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 1, 3 ), 5 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 2, 0 ), 6 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 2, 1 ), 7 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 3, 0 ), 8 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 3, 1 ), 9 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 4, 0 ), 10 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 5, 0 ), 11 );
    }

    void populateNetworkWithAbsAndRelu( NLR::NetworkLevelReasoner &nlr )
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

        // Mark the Round/Sign sources
        nlr.addActivationSource( 1, 0, 2, 0 );
        nlr.addActivationSource( 1, 1, 2, 1 );
        nlr.addActivationSource( 1, 2, 2, 2 );

        nlr.addActivationSource( 3, 0, 4, 0 );
        nlr.addActivationSource( 3, 1, 4, 1 );

        // Variable indexing
        nlr.setNeuronVariable( NLR::NeuronIndex( 0, 0 ), 0 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 0, 1 ), 1 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 1, 0 ), 2 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 1, 1 ), 4 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 1, 2 ), 6 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 2, 0 ), 3 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 2, 1 ), 5 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 2, 2 ), 7 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 3, 0 ), 8 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 3, 1 ), 10 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 4, 0 ), 9 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 4, 1 ), 11 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 5, 0 ), 12 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 5, 1 ), 13 );
    }

    void populateNetworkWithRoundAndSign( NLR::NetworkLevelReasoner &nlr )
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

        // Mark the Round/Sign sources
        nlr.addActivationSource( 1, 0, 2, 0 );
        nlr.addActivationSource( 1, 1, 2, 1 );
        nlr.addActivationSource( 1, 2, 2, 2 );

        nlr.addActivationSource( 3, 0, 4, 0 );
        nlr.addActivationSource( 3, 1, 4, 1 );

        // Variable indexing
        nlr.setNeuronVariable( NLR::NeuronIndex( 0, 0 ), 0 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 0, 1 ), 1 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 1, 0 ), 2 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 1, 1 ), 4 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 1, 2 ), 6 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 2, 0 ), 3 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 2, 1 ), 5 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 2, 2 ), 7 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 3, 0 ), 8 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 3, 1 ), 10 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 4, 0 ), 9 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 4, 1 ), 11 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 5, 0 ), 12 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 5, 1 ), 13 );
    }

    void populateNetworkWithLeakyReluAndSigmoid( NLR::NetworkLevelReasoner &nlr )
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

        // Mark the LeakyReLU/Sigmoid sources
        nlr.addActivationSource( 1, 0, 2, 0 );
        nlr.addActivationSource( 1, 1, 2, 1 );
        nlr.addActivationSource( 1, 2, 2, 2 );

        nlr.addActivationSource( 3, 0, 4, 0 );
        nlr.addActivationSource( 3, 1, 4, 1 );

        // Variable indexing
        nlr.setNeuronVariable( NLR::NeuronIndex( 0, 0 ), 0 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 0, 1 ), 1 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 1, 0 ), 2 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 1, 1 ), 4 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 1, 2 ), 6 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 2, 0 ), 3 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 2, 1 ), 5 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 2, 2 ), 7 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 3, 0 ), 8 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 3, 1 ), 10 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 4, 0 ), 9 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 4, 1 ), 11 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 5, 0 ), 12 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 5, 1 ), 13 );
    }

    void populateNetworkWithSoftmaxAndMax( NLR::NetworkLevelReasoner &nlr )
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

        // Mark the Softmax/Max sources
        nlr.addActivationSource( 1, 0, 2, 0 );
        nlr.addActivationSource( 1, 0, 2, 1 );
        nlr.addActivationSource( 1, 0, 2, 2 );
        nlr.addActivationSource( 1, 1, 2, 0 );
        nlr.addActivationSource( 1, 1, 2, 1 );
        nlr.addActivationSource( 1, 1, 2, 2 );
        nlr.addActivationSource( 1, 2, 2, 0 );
        nlr.addActivationSource( 1, 2, 2, 1 );
        nlr.addActivationSource( 1, 2, 2, 2 );

        nlr.addActivationSource( 3, 0, 4, 0 );
        nlr.addActivationSource( 3, 1, 4, 0 );

        // Variable indexing
        nlr.setNeuronVariable( NLR::NeuronIndex( 0, 0 ), 0 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 0, 1 ), 1 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 1, 0 ), 2 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 1, 1 ), 4 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 1, 2 ), 6 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 2, 0 ), 3 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 2, 1 ), 5 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 2, 2 ), 7 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 3, 0 ), 8 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 3, 1 ), 9 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 4, 0 ), 10 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 5, 0 ), 11 );
    }

    void populateNetworkWithReluAndBilinear( NLR::NetworkLevelReasoner &nlr )
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

        // Mark the ReLU/Bilinear sources
        nlr.addActivationSource( 1, 0, 2, 0 );
        nlr.addActivationSource( 1, 1, 2, 1 );
        nlr.addActivationSource( 1, 2, 2, 2 );

        nlr.addActivationSource( 3, 0, 4, 0 );
        nlr.addActivationSource( 3, 1, 4, 0 );

        // Variable indexing
        nlr.setNeuronVariable( NLR::NeuronIndex( 0, 0 ), 0 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 0, 1 ), 1 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 1, 0 ), 2 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 1, 1 ), 4 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 1, 2 ), 6 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 2, 0 ), 3 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 2, 1 ), 5 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 2, 2 ), 7 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 3, 0 ), 8 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 3, 1 ), 9 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 4, 0 ), 10 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 5, 0 ), 11 );
    }

    void populateNetworkSBTRelu( NLR::NetworkLevelReasoner &nlr, MockTableau &tableau )
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

        // Create the layers
        nlr.addLayer( 0, NLR::Layer::INPUT, 2 );
        nlr.addLayer( 1, NLR::Layer::WEIGHTED_SUM, 2 );
        nlr.addLayer( 2, NLR::Layer::RELU, 2 );
        nlr.addLayer( 3, NLR::Layer::WEIGHTED_SUM, 1 );

        // Mark layer dependencies
        for ( unsigned i = 1; i <= 3; ++i )
            nlr.addLayerDependency( i - 1, i );

        // Weights
        nlr.setWeight( 0, 0, 1, 0, 2 );
        nlr.setWeight( 0, 0, 1, 1, 1 );
        nlr.setWeight( 0, 1, 1, 0, 3 );
        nlr.setWeight( 0, 1, 1, 1, 1 );
        nlr.setWeight( 2, 0, 3, 0, 1 );
        nlr.setWeight( 2, 1, 3, 0, -1 );

        // Mark the ReLU sources
        nlr.addActivationSource( 1, 0, 2, 0 );
        nlr.addActivationSource( 1, 1, 2, 1 );

        // Variable indexing
        nlr.setNeuronVariable( NLR::NeuronIndex( 0, 0 ), 0 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 0, 1 ), 1 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 1, 0 ), 2 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 1, 1 ), 3 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 2, 0 ), 4 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 2, 1 ), 5 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 3, 0 ), 6 );

        // Very loose bounds for neurons except inputs
        double large = 1000000;

        tableau.getBoundManager().initialize( 7 );
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
    }

    void populateNetworkSBTReluResidual1( NLR::NetworkLevelReasoner &nlr, MockTableau &tableau )
    {
        /*
                     -1
             __________________
            /                  \
           /  1      R       -1  1    R    3  1
          x0 --- x1 ---> x2 --- x3 ---> x4 --- x5
                  \                            /
                   \            3             /
                    \________________________/

        */

        // Create the layers
        nlr.addLayer( 0, NLR::Layer::INPUT, 1 );
        nlr.addLayer( 1, NLR::Layer::WEIGHTED_SUM, 1 );
        nlr.addLayer( 2, NLR::Layer::RELU, 1 );
        nlr.addLayer( 3, NLR::Layer::WEIGHTED_SUM, 1 );
        nlr.addLayer( 4, NLR::Layer::RELU, 1 );
        nlr.addLayer( 5, NLR::Layer::WEIGHTED_SUM, 1 );

        // Mark layer dependencies
        for ( unsigned i = 1; i <= 5; ++i )
            nlr.addLayerDependency( i - 1, i );
        nlr.addLayerDependency( 0, 3 );
        nlr.addLayerDependency( 1, 5 );

        // Set the weights and biases for the weighted sum layers
        nlr.setWeight( 0, 0, 1, 0, 1 );
        nlr.setWeight( 2, 0, 3, 0, -1 );
        nlr.setWeight( 4, 0, 5, 0, 3 );
        nlr.setWeight( 0, 0, 3, 0, -1 );
        nlr.setWeight( 1, 0, 5, 0, 3 );


        nlr.setBias( 3, 0, 1 );
        nlr.setBias( 5, 0, 1 );

        // Mark the ReLU sources
        nlr.addActivationSource( 1, 0, 2, 0 );

        nlr.addActivationSource( 3, 0, 4, 0 );

        // Variable indexing
        nlr.setNeuronVariable( NLR::NeuronIndex( 0, 0 ), 0 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 1, 0 ), 1 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 2, 0 ), 2 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 3, 0 ), 3 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 4, 0 ), 4 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 5, 0 ), 5 );

        // Very loose bounds for neurons except inputs
        double large = 1000000;

        tableau.getBoundManager().initialize( 6 );
        tableau.setLowerBound( 1, -large );
        tableau.setUpperBound( 1, large );
        tableau.setLowerBound( 2, -large );
        tableau.setUpperBound( 2, large );
        tableau.setLowerBound( 3, -large );
        tableau.setUpperBound( 3, large );
        tableau.setLowerBound( 4, -large );
        tableau.setUpperBound( 4, large );
        tableau.setLowerBound( 5, -large );
        tableau.setUpperBound( 5, large );
    }

    void populateNetworkSBTReluResidual2( NLR::NetworkLevelReasoner &nlr, MockTableau &tableau )
    {
        /*
                     -1
             __________________
            /                  \
           /  1      R       -1  1    R     3  1   1
          x0 --- x1 ---> x2 --- x3 ---> x4 --- x5 --- x6
           \                                   /
            \                1                /
             \_______________________________/

        */

        // Create the layers
        nlr.addLayer( 0, NLR::Layer::INPUT, 1 );
        nlr.addLayer( 1, NLR::Layer::WEIGHTED_SUM, 1 );
        nlr.addLayer( 2, NLR::Layer::RELU, 1 );
        nlr.addLayer( 3, NLR::Layer::WEIGHTED_SUM, 1 );
        nlr.addLayer( 4, NLR::Layer::RELU, 1 );
        nlr.addLayer( 5, NLR::Layer::WEIGHTED_SUM, 1 );
        nlr.addLayer( 6, NLR::Layer::WEIGHTED_SUM, 1 );

        // Mark layer dependencies
        for ( unsigned i = 1; i <= 6; ++i )
            nlr.addLayerDependency( i - 1, i );
        nlr.addLayerDependency( 0, 3 );
        nlr.addLayerDependency( 0, 5 );

        // Set the weights and biases for the weighted sum layers
        nlr.setWeight( 0, 0, 1, 0, 1 );
        nlr.setWeight( 2, 0, 3, 0, -1 );
        nlr.setWeight( 4, 0, 5, 0, 3 );
        nlr.setWeight( 0, 0, 3, 0, -1 );
        nlr.setWeight( 0, 0, 5, 0, 1 );
        nlr.setWeight( 5, 0, 6, 0, 1 );

        nlr.setBias( 3, 0, 1 );
        nlr.setBias( 5, 0, 1 );

        // Mark the ReLU sources
        nlr.addActivationSource( 1, 0, 2, 0 );

        nlr.addActivationSource( 3, 0, 4, 0 );

        // Variable indexing
        nlr.setNeuronVariable( NLR::NeuronIndex( 0, 0 ), 0 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 1, 0 ), 1 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 2, 0 ), 2 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 3, 0 ), 3 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 4, 0 ), 4 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 5, 0 ), 5 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 6, 0 ), 6 );

        // Very loose bounds for neurons except inputs
        double large = 1000000;

        tableau.getBoundManager().initialize( 7 );
        tableau.setLowerBound( 1, -large );
        tableau.setUpperBound( 1, large );
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
    }

    void populateNetworkSBTReluReindex( NLR::NetworkLevelReasoner &nlr, MockTableau &tableau )
    {
        /*

              1             1            1   1
          x0 --- x2    x5 --- x6     x9 --- x10
            \    /\    /\    /  \    / \    /
           1 \  / R\  /-1\  /  R \  / 1 \  /
              \/    \/    \/      \/     \/
              /\    /\    /\      /\     /\
           1 /  \ R/  \ 1/  \  R /  \ 1 /  \
            /    \/    \/    \  /    \ / 0  \
          x1 --- x3    x4 --- x7     x8 --- x11
              -1           1

          The example described in Fig. 3 of
          https://files.sri.inf.ethz.ch/website/papers/DeepPoly.pdf
        */

        // Create the layers
        nlr.addLayer( 0, NLR::Layer::INPUT, 2 );
        nlr.addLayer( 1, NLR::Layer::WEIGHTED_SUM, 2 );
        nlr.addLayer( 2, NLR::Layer::RELU, 2 );
        nlr.addLayer( 3, NLR::Layer::WEIGHTED_SUM, 2 );
        nlr.addLayer( 4, NLR::Layer::RELU, 2 );
        nlr.addLayer( 5, NLR::Layer::WEIGHTED_SUM, 2 );

        // Mark layer dependencies
        for ( unsigned i = 1; i <= 5; ++i )
            nlr.addLayerDependency( i - 1, i );

        // Set the weights and biases for the weighted sum layers
        nlr.setWeight( 0, 0, 1, 0, 1 );
        nlr.setWeight( 0, 0, 1, 1, 1 );
        nlr.setWeight( 0, 1, 1, 0, 1 );
        nlr.setWeight( 0, 1, 1, 1, -1 );

        nlr.setWeight( 2, 0, 3, 0, 1 );
        nlr.setWeight( 2, 0, 3, 1, -1 );
        nlr.setWeight( 2, 1, 3, 0, 1 );
        nlr.setWeight( 2, 1, 3, 1, 1 );

        nlr.setWeight( 4, 0, 5, 0, 1 );
        nlr.setWeight( 4, 0, 5, 1, 1 );
        nlr.setWeight( 4, 1, 5, 0, 1 );
        nlr.setWeight( 4, 1, 5, 1, 0 );

        nlr.setBias( 5, 0, 1 );

        // Mark the ReLU sources
        nlr.addActivationSource( 1, 0, 2, 1 );
        nlr.addActivationSource( 1, 1, 2, 0 );

        nlr.addActivationSource( 3, 0, 4, 1 );
        nlr.addActivationSource( 3, 1, 4, 0 );

        // Variable indexing
        nlr.setNeuronVariable( NLR::NeuronIndex( 0, 0 ), 0 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 0, 1 ), 1 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 1, 0 ), 2 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 1, 1 ), 3 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 2, 0 ), 4 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 2, 1 ), 5 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 3, 0 ), 6 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 3, 1 ), 7 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 4, 0 ), 9 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 4, 1 ), 8 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 5, 0 ), 10 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 5, 1 ), 11 );

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

    void populateNetworkSBTLeakyReLU( NLR::NetworkLevelReasoner &nlr, MockTableau &tableau )
    {
        /*

              1      LR      1     LR      1   1
          x0 --- x2 ---> x4 --- x6 ---> x8 --- x10
            \    /        \    /          \    /
           1 \  /        1 \  /          0 \  /
              \/            \/              \/
              /\            /\              /\
           1 /  \        1 /  \          1 /  \
            /    \   LR   /    \    LR    / 1  \
          x1 --- x3 ---> x5 --- x7 ---> x9 --- x11
              -1            -1

          The example described in Fig. 3 of
          https://files.sri.inf.ethz.ch/website/papers/DeepPoly.pdf
          using LeakyReLU activation instead of ReLU
        */

        // Create the layers
        nlr.addLayer( 0, NLR::Layer::INPUT, 2 );
        nlr.addLayer( 1, NLR::Layer::WEIGHTED_SUM, 2 );
        nlr.addLayer( 2, NLR::Layer::LEAKY_RELU, 2 );
        nlr.addLayer( 3, NLR::Layer::WEIGHTED_SUM, 2 );
        nlr.addLayer( 4, NLR::Layer::LEAKY_RELU, 2 );
        nlr.addLayer( 5, NLR::Layer::WEIGHTED_SUM, 2 );

        nlr.getLayer( 2 )->setAlpha( 0.2 );
        nlr.getLayer( 4 )->setAlpha( 0.2 );

        // Mark layer dependencies
        for ( unsigned i = 1; i <= 5; ++i )
            nlr.addLayerDependency( i - 1, i );

        // Set the weights and biases for the weighted sum layers
        nlr.setWeight( 0, 0, 1, 0, 1 );
        nlr.setWeight( 0, 0, 1, 1, 1 );
        nlr.setWeight( 0, 1, 1, 0, 1 );
        nlr.setWeight( 0, 1, 1, 1, -1 );

        nlr.setWeight( 2, 0, 3, 0, 1 );
        nlr.setWeight( 2, 0, 3, 1, 1 );
        nlr.setWeight( 2, 1, 3, 0, 1 );
        nlr.setWeight( 2, 1, 3, 1, -1 );

        nlr.setWeight( 4, 0, 5, 0, 1 );
        nlr.setWeight( 4, 0, 5, 1, 0 );
        nlr.setWeight( 4, 1, 5, 0, 1 );
        nlr.setWeight( 4, 1, 5, 1, 1 );

        nlr.setBias( 5, 0, 1 );

        // Mark the LeakyReLU sources
        nlr.addActivationSource( 1, 0, 2, 0 );
        nlr.addActivationSource( 1, 1, 2, 1 );

        nlr.addActivationSource( 3, 0, 4, 0 );
        nlr.addActivationSource( 3, 1, 4, 1 );

        // Variable indexing
        nlr.setNeuronVariable( NLR::NeuronIndex( 0, 0 ), 0 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 0, 1 ), 1 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 1, 0 ), 2 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 1, 1 ), 3 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 2, 0 ), 4 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 2, 1 ), 5 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 3, 0 ), 6 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 3, 1 ), 7 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 4, 0 ), 8 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 4, 1 ), 9 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 5, 0 ), 10 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 5, 1 ), 11 );

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

    void populateNetworkSBTSigmoidsAndRound( NLR::NetworkLevelReasoner &nlr, MockTableau &tableau )
    {
        /*

              1      S       1     Rd
          x0 --- x2 ---> x4 --- x6 --- x8
            \    /        \    /
           1 \  /        1 \  /
              \/            \/
              /\            /\
           1 /  \        1 /  \
            /    \   S    /    \   Rd
          x1 --- x3 ---> x5 --- x7 --- x9
              -1            -1

        */

        // Create the layers
        nlr.addLayer( 0, NLR::Layer::INPUT, 2 );
        nlr.addLayer( 1, NLR::Layer::WEIGHTED_SUM, 2 );
        nlr.addLayer( 2, NLR::Layer::SIGMOID, 2 );
        nlr.addLayer( 3, NLR::Layer::WEIGHTED_SUM, 2 );
        nlr.addLayer( 4, NLR::Layer::ROUND, 2 );

        // Mark layer dependencies
        for ( unsigned i = 1; i <= 4; ++i )
            nlr.addLayerDependency( i - 1, i );

        // Set the weights and biases for the weighted sum layers
        nlr.setWeight( 0, 0, 1, 0, 1 );
        nlr.setWeight( 0, 0, 1, 1, 1 );
        nlr.setWeight( 0, 1, 1, 0, 1 );
        nlr.setWeight( 0, 1, 1, 1, -1 );

        nlr.setWeight( 2, 0, 3, 0, 1 );
        nlr.setWeight( 2, 0, 3, 1, 1 );
        nlr.setWeight( 2, 1, 3, 0, 1 );
        nlr.setWeight( 2, 1, 3, 1, -1 );

        // Mark the Sigmoid sources
        nlr.addActivationSource( 1, 0, 2, 0 );
        nlr.addActivationSource( 1, 1, 2, 1 );
        nlr.addActivationSource( 3, 0, 4, 0 );
        nlr.addActivationSource( 3, 1, 4, 1 );

        // Variable indexing
        nlr.setNeuronVariable( NLR::NeuronIndex( 0, 0 ), 0 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 0, 1 ), 1 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 1, 0 ), 2 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 1, 1 ), 3 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 2, 0 ), 4 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 2, 1 ), 5 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 3, 0 ), 6 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 3, 1 ), 7 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 4, 0 ), 8 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 4, 1 ), 9 );

        // Very loose bounds for neurons except inputs
        double large = 1000000;

        tableau.getBoundManager().initialize( 10 );
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
    }

    void populateNetworkSBTMax( NLR::NetworkLevelReasoner &nlr, MockTableau &tableau )
    {
        /*

              1      R          Max  2
          x0 --- x2 ---> x4 --- x6  ---> x7
           \    /               /
          1 \  /               /
             \/               /
             /\              /
          1 /  \            /
           /    \    R     /
          x1 --- x3 ---> x5
             -1

        */

        // Create the layers
        nlr.addLayer( 0, NLR::Layer::INPUT, 2 );
        nlr.addLayer( 1, NLR::Layer::WEIGHTED_SUM, 2 );
        nlr.addLayer( 2, NLR::Layer::RELU, 2 );
        nlr.addLayer( 3, NLR::Layer::MAX, 1 );
        nlr.addLayer( 4, NLR::Layer::WEIGHTED_SUM, 1 );

        // Mark layer dependencies
        for ( unsigned i = 1; i <= 4; ++i )
            nlr.addLayerDependency( i - 1, i );

        // Set the weights and biases for the weighted sum layers
        nlr.setWeight( 0, 0, 1, 0, 1 );
        nlr.setWeight( 0, 0, 1, 1, 1 );
        nlr.setWeight( 0, 1, 1, 0, 1 );
        nlr.setWeight( 0, 1, 1, 1, -1 );
        nlr.setWeight( 3, 0, 4, 0, 2 );

        // Mark the ReLU sources
        nlr.addActivationSource( 1, 0, 2, 0 );
        nlr.addActivationSource( 1, 1, 2, 1 );

        // Mark the Max sources
        nlr.addActivationSource( 2, 0, 3, 0 );
        nlr.addActivationSource( 2, 1, 3, 0 );

        // Variable indexing
        nlr.setNeuronVariable( NLR::NeuronIndex( 0, 0 ), 0 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 0, 1 ), 1 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 1, 0 ), 2 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 1, 1 ), 3 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 2, 0 ), 4 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 2, 1 ), 5 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 3, 0 ), 6 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 4, 0 ), 7 );
        // Very loose bounds for neurons except inputs
        double large = 1000000;

        tableau.getBoundManager().initialize( 8 );
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
    }

    void populateNetworkSBTSoftmax( NLR::NetworkLevelReasoner &nlr, MockTableau &tableau )
    {
        /*


          x0      x3  S  x6

          x1      x4  S  x7

          x2      x5  S  x8

          x3 = x0 - x1 + x2 + 1
          x4 = -x0 + x1 + x2 + 2
          x5 = -x0 - x1 - x2 + 3

          x6 x7 x8 = softmax(x3, x4, x5)

          x9 = x6 + x7 + x8
          x10 = x6 + x7 + x8

        */

        // Create the layers
        nlr.addLayer( 0, NLR::Layer::INPUT, 3 );
        nlr.addLayer( 1, NLR::Layer::WEIGHTED_SUM, 3 );
        nlr.addLayer( 2, NLR::Layer::SOFTMAX, 3 );
        nlr.addLayer( 3, NLR::Layer::WEIGHTED_SUM, 2 );

        // Mark layer dependencies
        for ( unsigned i = 1; i <= 3; ++i )
            nlr.addLayerDependency( i - 1, i );

        // Set the weights and biases for the weighted sum layers
        nlr.setWeight( 0, 0, 1, 0, 1 );
        nlr.setWeight( 0, 0, 1, 1, -1 );
        nlr.setWeight( 0, 0, 1, 2, -1 );
        nlr.setWeight( 0, 1, 1, 0, -1 );
        nlr.setWeight( 0, 1, 1, 1, 1 );
        nlr.setWeight( 0, 1, 1, 2, -1 );
        nlr.setWeight( 0, 2, 1, 0, 1 );
        nlr.setWeight( 0, 2, 1, 1, 1 );
        nlr.setWeight( 0, 2, 1, 2, -1 );
        nlr.setWeight( 2, 0, 3, 0, 1 );
        nlr.setWeight( 2, 1, 3, 0, 1 );
        nlr.setWeight( 2, 2, 3, 0, 1 );
        nlr.setWeight( 2, 0, 3, 1, -1 );
        nlr.setWeight( 2, 1, 3, 1, -1 );
        nlr.setWeight( 2, 2, 3, 1, -1 );

        nlr.setBias( 1, 0, 1 );
        nlr.setBias( 1, 1, 2 );
        nlr.setBias( 1, 2, 3 );

        nlr.addActivationSource( 1, 0, 2, 0 );
        nlr.addActivationSource( 1, 1, 2, 0 );
        nlr.addActivationSource( 1, 2, 2, 0 );
        nlr.addActivationSource( 1, 0, 2, 1 );
        nlr.addActivationSource( 1, 1, 2, 1 );
        nlr.addActivationSource( 1, 2, 2, 1 );
        nlr.addActivationSource( 1, 0, 2, 2 );
        nlr.addActivationSource( 1, 1, 2, 2 );
        nlr.addActivationSource( 1, 2, 2, 2 );


        // Variable indexing
        nlr.setNeuronVariable( NLR::NeuronIndex( 0, 0 ), 0 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 0, 1 ), 1 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 0, 2 ), 2 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 1, 0 ), 3 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 1, 1 ), 4 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 1, 2 ), 5 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 2, 0 ), 6 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 2, 1 ), 7 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 2, 2 ), 8 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 3, 0 ), 9 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 3, 1 ), 10 );

        // Very loose bounds for neurons except inputs
        double large = 1000000;

        tableau.getBoundManager().initialize( 11 );
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
    }

    void populateNetworkSBTSoftmax2( NLR::NetworkLevelReasoner &nlr, MockTableau &tableau )
    {
        /*


          x0      x3  S  x8

          x1      x4  S  x9

          x2      x5  S  x10

                  x6  S  x11

                  x7  S  x12

          x3 = x0 - x1 + x2 + 1
          x4 = -x0 + x1 + x2 + 2
          x5 = -x0 - x1 - x2 + 3
          x6 = -x0 - x1 - x2 + 2
          x7 = -x0 - x1 - x2 + 1

          x8 x10 x12 = softmax(x3, x5, x7)

          x9 x11 = softmax(x4, x6)

          x13 = x8 + x10 + x12
          x14 = -x8 - x10 - x12
          x15 = x9 + x11
          x16 = -x9 - x11

        */

        // Create the layers
        nlr.addLayer( 0, NLR::Layer::INPUT, 3 );
        nlr.addLayer( 1, NLR::Layer::WEIGHTED_SUM, 5 );
        nlr.addLayer( 2, NLR::Layer::SOFTMAX, 5 );
        nlr.addLayer( 3, NLR::Layer::WEIGHTED_SUM, 4 );

        // Mark layer dependencies
        for ( unsigned i = 1; i <= 3; ++i )
            nlr.addLayerDependency( i - 1, i );

        // Set the weights and biases for the weighted sum layers
        nlr.setWeight( 0, 0, 1, 0, 1 );
        nlr.setWeight( 0, 0, 1, 1, -1 );
        nlr.setWeight( 0, 0, 1, 2, -1 );
        nlr.setWeight( 0, 0, 1, 3, -1 );
        nlr.setWeight( 0, 0, 1, 4, -1 );
        nlr.setWeight( 0, 1, 1, 0, -1 );
        nlr.setWeight( 0, 1, 1, 1, 1 );
        nlr.setWeight( 0, 1, 1, 2, -1 );
        nlr.setWeight( 0, 1, 1, 3, -1 );
        nlr.setWeight( 0, 1, 1, 4, -1 );
        nlr.setWeight( 0, 2, 1, 0, 1 );
        nlr.setWeight( 0, 2, 1, 1, 1 );
        nlr.setWeight( 0, 2, 1, 2, -1 );
        nlr.setWeight( 0, 2, 1, 3, -1 );
        nlr.setWeight( 0, 2, 1, 4, -1 );
        nlr.setWeight( 2, 0, 3, 0, 1 );
        nlr.setWeight( 2, 2, 3, 0, 1 );
        nlr.setWeight( 2, 4, 3, 0, 1 );
        nlr.setWeight( 2, 0, 3, 1, -1 );
        nlr.setWeight( 2, 2, 3, 1, -1 );
        nlr.setWeight( 2, 4, 3, 1, -1 );
        nlr.setWeight( 2, 1, 3, 2, 1 );
        nlr.setWeight( 2, 3, 3, 2, 1 );
        nlr.setWeight( 2, 1, 3, 3, -1 );
        nlr.setWeight( 2, 3, 3, 3, -1 );

        nlr.setBias( 1, 0, 1 );
        nlr.setBias( 1, 1, 2 );
        nlr.setBias( 1, 2, 3 );
        nlr.setBias( 1, 3, 2 );
        nlr.setBias( 1, 4, 1 );

        nlr.addActivationSource( 1, 0, 2, 0 );
        nlr.addActivationSource( 1, 2, 2, 0 );
        nlr.addActivationSource( 1, 4, 2, 0 );
        nlr.addActivationSource( 1, 0, 2, 2 );
        nlr.addActivationSource( 1, 2, 2, 2 );
        nlr.addActivationSource( 1, 4, 2, 2 );
        nlr.addActivationSource( 1, 0, 2, 4 );
        nlr.addActivationSource( 1, 2, 2, 4 );
        nlr.addActivationSource( 1, 4, 2, 4 );
        nlr.addActivationSource( 1, 1, 2, 1 );
        nlr.addActivationSource( 1, 3, 2, 1 );
        nlr.addActivationSource( 1, 1, 2, 3 );
        nlr.addActivationSource( 1, 3, 2, 3 );

        // Variable indexing
        nlr.setNeuronVariable( NLR::NeuronIndex( 0, 0 ), 0 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 0, 1 ), 1 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 0, 2 ), 2 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 1, 0 ), 3 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 1, 1 ), 4 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 1, 2 ), 5 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 1, 3 ), 6 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 1, 4 ), 7 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 2, 0 ), 8 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 2, 1 ), 9 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 2, 2 ), 10 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 2, 3 ), 11 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 2, 4 ), 12 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 3, 0 ), 13 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 3, 1 ), 14 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 3, 2 ), 15 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 3, 3 ), 16 );

        // Very loose bounds for neurons except inputs
        double large = 1000000;

        tableau.getBoundManager().initialize( 17 );
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
        tableau.setLowerBound( 14, -large );
        tableau.setUpperBound( 14, large );
        tableau.setLowerBound( 15, -large );
        tableau.setUpperBound( 15, large );
        tableau.setLowerBound( 16, -large );
        tableau.setUpperBound( 16, large );
    }

    void populateNetworkSBTBilinear( NLR::NetworkLevelReasoner &nlr, MockTableau &tableau )
    {
        /*


          x0    x2
                    x  x4 -- x5
          x1    x3

          x2 = x0 - 2 * x1
          x3 = x0 + x1
          x4 = -x5

          x4 = x2 * x3
        */

        // Create the layers
        nlr.addLayer( 0, NLR::Layer::INPUT, 2 );
        nlr.addLayer( 1, NLR::Layer::WEIGHTED_SUM, 2 );
        nlr.addLayer( 2, NLR::Layer::BILINEAR, 1 );
        nlr.addLayer( 3, NLR::Layer::WEIGHTED_SUM, 1 );

        // Mark layer dependencies
        for ( unsigned i = 1; i <= 3; ++i )
            nlr.addLayerDependency( i - 1, i );

        // Set the weights and biases for the weighted sum layers
        nlr.setWeight( 0, 0, 1, 0, 1 );
        nlr.setWeight( 0, 0, 1, 1, 1 );
        nlr.setWeight( 0, 1, 1, 0, -2 );
        nlr.setWeight( 0, 1, 1, 1, 1 );
        nlr.setWeight( 2, 0, 3, 0, -1 );

        nlr.addActivationSource( 1, 0, 2, 0 );
        nlr.addActivationSource( 1, 1, 2, 0 );


        // Variable indexing
        nlr.setNeuronVariable( NLR::NeuronIndex( 0, 0 ), 0 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 0, 1 ), 1 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 1, 0 ), 2 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 1, 1 ), 3 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 2, 0 ), 4 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 3, 0 ), 5 );

        // Very loose bounds for neurons except inputs
        double large = 1000000;

        tableau.getBoundManager().initialize( 6 );
        tableau.setLowerBound( 2, -large );
        tableau.setUpperBound( 2, large );
        tableau.setLowerBound( 3, -large );
        tableau.setUpperBound( 3, large );
        tableau.setLowerBound( 4, -large );
        tableau.setUpperBound( 4, large );
        tableau.setLowerBound( 5, -large );
        tableau.setUpperBound( 5, large );
    }

    void test_evaluate_relu()
    {
        NLR::NetworkLevelReasoner nlr;

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

        TS_ASSERT_THROWS_NOTHING( nlr.evaluate( input, output ) );

        TS_ASSERT( FloatUtils::areEqual( output[0], 0 ) );
        TS_ASSERT( FloatUtils::areEqual( output[1], 0 ) );
    }

    void test_evaluate_sigmoids()
    {
        NLR::NetworkLevelReasoner nlr;

        populateNetworkWithSigmoids( nlr );

        double input[2];
        double output[2];

        // case 1
        input[0] = 0;
        input[1] = 0;

        TS_ASSERT_THROWS_NOTHING( nlr.evaluate( input, output ) );

        TS_ASSERT( FloatUtils::areEqual( output[0], 0.6750, 0.0001 ) );
        TS_ASSERT( FloatUtils::areEqual( output[1], 3.0167, 0.0001 ) );

        // case 2
        input[0] = 1;
        input[1] = 1;

        TS_ASSERT_THROWS_NOTHING( nlr.evaluate( input, output ) );

        TS_ASSERT( FloatUtils::areEqual( output[0], 0.6032, 0.0001 ) );
        TS_ASSERT( FloatUtils::areEqual( output[1], 2.5790, 0.0001 ) );

        // case 3
        input[0] = 1;
        input[1] = 2;

        TS_ASSERT_THROWS_NOTHING( nlr.evaluate( input, output ) );

        TS_ASSERT( FloatUtils::areEqual( output[0], 0.5045, 0.0001 ) );
        TS_ASSERT( FloatUtils::areEqual( output[1], 2.1957, 0.0001 ) );
    }

    void test_evaluate_abs()
    {
        NLR::NetworkLevelReasoner nlr;

        populateNetworkWithAbs( nlr );

        double input[2];
        double output[2];

        // With Abs, Inputs are zeros, only biases count
        input[0] = 0;
        input[1] = 0;

        TS_ASSERT_THROWS_NOTHING( nlr.evaluate( input, output ) );

        TS_ASSERT( FloatUtils::areEqual( output[0], 1 ) );
        TS_ASSERT( FloatUtils::areEqual( output[1], 4 ) );

        // With Abs, case 1
        input[0] = -2;
        input[1] = -2;

        TS_ASSERT_THROWS_NOTHING( nlr.evaluate( input, output ) );

        TS_ASSERT( FloatUtils::areEqual( output[0], 1 ) );
        TS_ASSERT( FloatUtils::areEqual( output[1], 4 ) );

        // With Abs, case 2
        input[0] = 1;
        input[1] = 2;

        TS_ASSERT_THROWS_NOTHING( nlr.evaluate( input, output ) );

        TS_ASSERT( FloatUtils::areEqual( output[0], 4 ) );
        TS_ASSERT( FloatUtils::areEqual( output[1], 10 ) );
    }

    void test_evaluate_sign()
    {
        NLR::NetworkLevelReasoner nlr;

        populateNetworkWithSign( nlr );

        double input[2];
        double output[2];

        // With Sign, Inputs are zeros, only biases count
        input[0] = 0;
        input[1] = 0;

        TS_ASSERT_THROWS_NOTHING( nlr.evaluate( input, output ) );

        TS_ASSERT( FloatUtils::areEqual( output[0], 1 ) );
        TS_ASSERT( FloatUtils::areEqual( output[1], 4 ) );

        // With Sign, case 1
        input[0] = -2;
        input[1] = -2;

        TS_ASSERT_THROWS_NOTHING( nlr.evaluate( input, output ) );

        TS_ASSERT( FloatUtils::areEqual( output[0], 1 ) );
        TS_ASSERT( FloatUtils::areEqual( output[1], 4 ) );

        // With Sign, case 2  (0 considered "non-negative", sign(0)=1)
        input[0] = -1;
        input[1] = 1;

        TS_ASSERT_THROWS_NOTHING( nlr.evaluate( input, output ) );

        TS_ASSERT( FloatUtils::areEqual( output[0], -1 ) );
        TS_ASSERT( FloatUtils::areEqual( output[1], -4 ) );
    }

    void test_evaluate_round()
    {
        NLR::NetworkLevelReasoner nlr;

        populateNetworkWithRound( nlr );

        double input[2];
        double output[2];

        // With Round, Inputs are zeros, only biases count
        input[0] = 0;
        input[1] = 0;

        TS_ASSERT_THROWS_NOTHING( nlr.evaluate( input, output ) );

        TS_ASSERT( FloatUtils::areEqual( output[0], 1 ) );
        TS_ASSERT( FloatUtils::areEqual( output[1], 4 ) );

        // With Round, case 1
        input[0] = 2.1;
        input[1] = 1.4;

        TS_ASSERT_THROWS_NOTHING( nlr.evaluate( input, output ) );

        TS_ASSERT( FloatUtils::areEqual( output[0], 2, 0.0001 ) );
        TS_ASSERT( FloatUtils::areEqual( output[1], -4, 0.0001 ) );

        // With Round, case 2
        input[0] = 2.1;
        input[1] = 1.6;

        TS_ASSERT_THROWS_NOTHING( nlr.evaluate( input, output ) );

        TS_ASSERT( FloatUtils::areEqual( output[0], 0, 0.0001 ) );
        TS_ASSERT( FloatUtils::areEqual( output[1], -12, 0.0001 ) );
    }

    void test_evaluate_leaky_relu()
    {
        NLR::NetworkLevelReasoner nlr;

        populateNetworkWithLeakyRelu( nlr );

        double input[2];
        double output[2];

        // With Leaky ReLU, Inputs are zeros, only biases count
        input[0] = 0;
        input[1] = 0;

        TS_ASSERT_THROWS_NOTHING( nlr.evaluate( input, output ) );

        TS_ASSERT( FloatUtils::areEqual( output[0], 1 ) );
        TS_ASSERT( FloatUtils::areEqual( output[1], 4 ) );

        // With Leaky ReLU, case 1  (alpha=0.1)
        input[0] = 1;
        input[1] = 1;

        TS_ASSERT_THROWS_NOTHING( nlr.evaluate( input, output ) );

        TS_ASSERT( FloatUtils::areEqual( output[0], 0.9, 0.0001 ) );
        TS_ASSERT( FloatUtils::areEqual( output[1], 0.57, 0.0001 ) );

        // With Leaky ReLU, case 2
        input[0] = 1;
        input[1] = 2;

        TS_ASSERT_THROWS_NOTHING( nlr.evaluate( input, output ) );

        TS_ASSERT( FloatUtils::areEqual( output[0], -0.04, 0.0001 ) );
        TS_ASSERT( FloatUtils::areEqual( output[1], -0.76, 0.0001 ) );
    }

    void test_evaluate_max()
    {
        NLR::NetworkLevelReasoner nlr;

        populateNetworkWithMax( nlr );

        double input[2];
        double output[1];

        // With Max, Inputs are zeros, only biases count
        input[0] = 0;
        input[1] = 0;

        TS_ASSERT_THROWS_NOTHING( nlr.evaluate( input, output ) );

        TS_ASSERT( FloatUtils::areEqual( output[0], -3 ) );

        // With Max, case 1
        input[0] = 1;
        input[1] = -3;

        TS_ASSERT_THROWS_NOTHING( nlr.evaluate( input, output ) );

        TS_ASSERT( FloatUtils::areEqual( output[0], -18 ) );

        // With Max, case 2
        input[0] = -3;
        input[1] = 3;

        TS_ASSERT_THROWS_NOTHING( nlr.evaluate( input, output ) );

        TS_ASSERT( FloatUtils::areEqual( output[0], -5 ) );
    }


    void test_evaluate_softmax()
    {
        NLR::NetworkLevelReasoner nlr;

        populateNetworkWithSoftmax( nlr );

        double input[2];
        double output[2];

        // With Softmax, Inputs are zeros, only biases count
        input[0] = 0;
        input[1] = 0;

        TS_ASSERT_THROWS_NOTHING( nlr.evaluate( input, output ) );
        TS_ASSERT( FloatUtils::areEqual( output[0], 0.2999, 0.0001 ) );
        TS_ASSERT( FloatUtils::areEqual( output[1], 2.4001, 0.0001 ) );

        // With Softmax, case 1
        input[0] = 1;
        input[1] = -3;

        TS_ASSERT_THROWS_NOTHING( nlr.evaluate( input, output ) );
        TS_ASSERT( FloatUtils::areEqual( output[0], 0.1192, 0.0001 ) );
        TS_ASSERT( FloatUtils::areEqual( output[1], 2.7615, 0.0001 ) );

        // With Softmax, case 2
        input[0] = -3;
        input[1] = 3;

        TS_ASSERT_THROWS_NOTHING( nlr.evaluate( input, output ) );
        TS_ASSERT( FloatUtils::areEqual( output[0], 0.1206, 0.0001 ) );
        TS_ASSERT( FloatUtils::areEqual( output[1], 2.7588, 0.0001 ) );
    }

    void test_evaluate_bilinear()
    {
        NLR::NetworkLevelReasoner nlr;

        populateNetworkWithBilinear( nlr );

        double input[2];
        double output[1];

        // With Bilinear, Inputs are zeros, only biases count
        input[0] = 0;
        input[1] = 0;

        TS_ASSERT_THROWS_NOTHING( nlr.evaluate( input, output ) );

        TS_ASSERT( FloatUtils::areEqual( output[0], 0 ) );

        // With Bilinear, case 1
        input[0] = 0.1;
        input[1] = -0.3;

        TS_ASSERT_THROWS_NOTHING( nlr.evaluate( input, output ) );
        TS_ASSERT( FloatUtils::areEqual( output[0], 2.8304, 0.0001 ) );

        // With Bilinear, case 2
        input[0] = -0.3;
        input[1] = 0.3;

        TS_ASSERT_THROWS_NOTHING( nlr.evaluate( input, output ) );
        TS_ASSERT( FloatUtils::areEqual( output[0], 0.0912, 0.0001 ) );
    }

    void test_evaluate_non_consecutive_layers()
    {
        NLR::NetworkLevelReasoner nlr;

        // Create the layers
        nlr.addLayer( 0, NLR::Layer::INPUT, 2 );
        nlr.addLayer( 1, NLR::Layer::WEIGHTED_SUM, 3 );
        nlr.addLayer( 2, NLR::Layer::RELU, 3 );
        nlr.addLayer( 3, NLR::Layer::WEIGHTED_SUM, 2 );
        nlr.addLayer( 4, NLR::Layer::RELU, 3 );
        nlr.addLayer( 5, NLR::Layer::WEIGHTED_SUM, 1 );

        // Mark layer dependencies
        nlr.addLayerDependency( 0, 1 );
        nlr.addLayerDependency( 1, 2 );
        nlr.addLayerDependency( 2, 3 );
        nlr.addLayerDependency( 0, 3 );
        nlr.addLayerDependency( 3, 4 );
        nlr.addLayerDependency( 0, 4 );
        nlr.addLayerDependency( 4, 5 );

        // Set the weights and relus
        nlr.setWeight( 0, 0, 1, 0, 1 );
        nlr.setWeight( 0, 0, 1, 1, 2 );
        nlr.setWeight( 0, 1, 1, 1, -3 );
        nlr.setWeight( 0, 1, 1, 2, 1 );

        nlr.addActivationSource( 1, 0, 2, 0 );
        nlr.addActivationSource( 1, 1, 2, 1 );
        nlr.addActivationSource( 1, 2, 2, 2 );

        nlr.setWeight( 2, 0, 3, 0, 1 );
        nlr.setWeight( 2, 1, 3, 0, 2 );
        nlr.setWeight( 2, 2, 3, 1, -2 );
        nlr.setWeight( 0, 1, 3, 1, 1 );

        nlr.addActivationSource( 3, 0, 4, 0 );
        nlr.addActivationSource( 3, 1, 4, 1 );
        nlr.addActivationSource( 0, 0, 4, 2 );

        nlr.setWeight( 4, 0, 5, 0, 1 );
        nlr.setWeight( 4, 1, 5, 0, 1 );
        nlr.setWeight( 4, 2, 5, 0, 1 );

        // Evaluate
        double input[2];
        double output;

        input[0] = 1;
        input[1] = 1;

        TS_ASSERT_THROWS_NOTHING( nlr.evaluate( input, &output ) );
        TS_ASSERT( FloatUtils::areEqual( output, 2 ) );

        input[0] = -1;
        input[1] = 2;

        TS_ASSERT_THROWS_NOTHING( nlr.evaluate( input, &output ) );
        TS_ASSERT( FloatUtils::areEqual( output, 0 ) );

        TS_ASSERT_THROWS_NOTHING( nlr.computeSuccessorLayers() );

        TS_ASSERT_EQUALS( nlr.getLayer( 0 )->getSuccessorLayers(), Set<unsigned>( { 1, 3, 4 } ) );
        TS_ASSERT_EQUALS( nlr.getLayer( 1 )->getSuccessorLayers(), Set<unsigned>( { 2 } ) );
        TS_ASSERT_EQUALS( nlr.getLayer( 2 )->getSuccessorLayers(), Set<unsigned>( { 3 } ) );
        TS_ASSERT_EQUALS( nlr.getLayer( 3 )->getSuccessorLayers(), Set<unsigned>( { 4 } ) );
        TS_ASSERT_EQUALS( nlr.getLayer( 4 )->getSuccessorLayers(), Set<unsigned>( { 5 } ) );
    }

    void test_evaluate_abs_and_relu()
    {
        NLR::NetworkLevelReasoner nlr;

        populateNetworkWithAbsAndRelu( nlr );

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

    void test_evaluate_round_and_sign()
    {
        NLR::NetworkLevelReasoner nlr;

        populateNetworkWithRoundAndSign( nlr );

        double input[2];
        double output[2];

        input[0] = 1.6;
        input[1] = 1.4;

        TS_ASSERT_THROWS_NOTHING( nlr.evaluate( input, output ) );

        TS_ASSERT( FloatUtils::areEqual( output[0], 1, 0.0001 ) );
        TS_ASSERT( FloatUtils::areEqual( output[1], -2, 0.0001 ) );

        input[0] = 1.6;
        input[1] = 1.6;

        TS_ASSERT_THROWS_NOTHING( nlr.evaluate( input, output ) );

        TS_ASSERT( FloatUtils::areEqual( output[0], -1, 0.0001 ) );
        TS_ASSERT( FloatUtils::areEqual( output[1], -4, 0.0001 ) );
    }

    void test_evaluate_leaky_relu_and_sigmoid()
    {
        NLR::NetworkLevelReasoner nlr;

        populateNetworkWithLeakyReluAndSigmoid( nlr );

        double input[2];
        double output[2];

        input[0] = 1;
        input[1] = 1;

        TS_ASSERT_THROWS_NOTHING( nlr.evaluate( input, output ) );

        TS_ASSERT( FloatUtils::areEqual( output[0], 0.7109, 0.0001 ) );
        TS_ASSERT( FloatUtils::areEqual( output[1], 1.4602, 0.0001 ) );

        input[0] = 1;
        input[1] = 2;

        TS_ASSERT_THROWS_NOTHING( nlr.evaluate( input, output ) );

        TS_ASSERT( FloatUtils::areEqual( output[0], 0.4013, 0.0001 ) );
        TS_ASSERT( FloatUtils::areEqual( output[1], 0.6508, 0.0001 ) );
    }

    void test_evaluate_softmax_and_max()
    {
        NLR::NetworkLevelReasoner nlr;

        populateNetworkWithSoftmaxAndMax( nlr );

        double input[2];
        double output[1];

        input[0] = 1;
        input[1] = -3;

        TS_ASSERT_THROWS_NOTHING( nlr.evaluate( input, output ) );

        TS_ASSERT( FloatUtils::areEqual( output[0], -2.9998, 0.0001 ) );

        input[0] = -3;
        input[1] = 3;

        TS_ASSERT_THROWS_NOTHING( nlr.evaluate( input, output ) );

        TS_ASSERT( FloatUtils::areEqual( output[0], -1.0000, 0.0001 ) );
    }

    void test_evaluate_relu_and_bilinear()
    {
        NLR::NetworkLevelReasoner nlr;

        populateNetworkWithReluAndBilinear( nlr );

        double input[2];
        double output[1];

        input[0] = 1;
        input[1] = 1;

        TS_ASSERT_THROWS_NOTHING( nlr.evaluate( input, output ) );

        TS_ASSERT( FloatUtils::areEqual( output[0], 1 ) );

        input[0] = 1;
        input[1] = 2;

        TS_ASSERT_THROWS_NOTHING( nlr.evaluate( input, output ) );

        TS_ASSERT( FloatUtils::areEqual( output[0], 0 ) );
    }

    void test_store_into_other()
    {
        NLR::NetworkLevelReasoner nlr;

        populateNetwork( nlr );

        NLR::NetworkLevelReasoner nlr2;

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

    void test_store_into_other_with_sigmoids()
    {
        NLR::NetworkLevelReasoner nlr;

        populateNetworkWithSigmoids( nlr );

        NLR::NetworkLevelReasoner nlr2;

        TS_ASSERT_THROWS_NOTHING( nlr.storeIntoOther( nlr2 ) );

        double input[2];
        double output1[2];
        double output2[2];

        // case 1
        input[0] = 0;
        input[1] = 0;

        TS_ASSERT_THROWS_NOTHING( nlr.evaluate( input, output1 ) );
        TS_ASSERT_THROWS_NOTHING( nlr2.evaluate( input, output2 ) );

        TS_ASSERT( FloatUtils::areEqual( output1[0], output2[0] ) );
        TS_ASSERT( FloatUtils::areEqual( output1[1], output2[1] ) );

        // case 2
        input[0] = 1;
        input[1] = 1;

        TS_ASSERT_THROWS_NOTHING( nlr.evaluate( input, output1 ) );
        TS_ASSERT_THROWS_NOTHING( nlr2.evaluate( input, output2 ) );

        TS_ASSERT( FloatUtils::areEqual( output1[0], output2[0] ) );
        TS_ASSERT( FloatUtils::areEqual( output1[1], output2[1] ) );
    }

    void test_store_into_other_with_round()
    {
        NLR::NetworkLevelReasoner nlr;

        populateNetworkWithRound( nlr );

        NLR::NetworkLevelReasoner nlr2;

        TS_ASSERT_THROWS_NOTHING( nlr.storeIntoOther( nlr2 ) );

        double input[2];
        double output1[2];
        double output2[2];

        // case 1
        input[0] = 0;
        input[1] = 0;

        TS_ASSERT_THROWS_NOTHING( nlr.evaluate( input, output1 ) );
        TS_ASSERT_THROWS_NOTHING( nlr2.evaluate( input, output2 ) );

        TS_ASSERT( FloatUtils::areEqual( output1[0], output2[0] ) );
        TS_ASSERT( FloatUtils::areEqual( output1[1], output2[1] ) );

        // case 2
        input[0] = 1;
        input[1] = 1;

        TS_ASSERT_THROWS_NOTHING( nlr.evaluate( input, output1 ) );
        TS_ASSERT_THROWS_NOTHING( nlr2.evaluate( input, output2 ) );

        TS_ASSERT( FloatUtils::areEqual( output1[0], output2[0] ) );
        TS_ASSERT( FloatUtils::areEqual( output1[1], output2[1] ) );
    }

    void test_store_into_other_with_sign()
    {
        NLR::NetworkLevelReasoner nlr;

        populateNetworkWithSign( nlr );

        NLR::NetworkLevelReasoner nlr2;

        TS_ASSERT_THROWS_NOTHING( nlr.storeIntoOther( nlr2 ) );

        double input[2];
        double output1[2];
        double output2[2];

        // case 1
        input[0] = 0;
        input[1] = 0;

        TS_ASSERT_THROWS_NOTHING( nlr.evaluate( input, output1 ) );
        TS_ASSERT_THROWS_NOTHING( nlr2.evaluate( input, output2 ) );

        TS_ASSERT( FloatUtils::areEqual( output1[0], output2[0] ) );
        TS_ASSERT( FloatUtils::areEqual( output1[1], output2[1] ) );

        // case 2
        input[0] = 1;
        input[1] = 1;

        TS_ASSERT_THROWS_NOTHING( nlr.evaluate( input, output1 ) );
        TS_ASSERT_THROWS_NOTHING( nlr2.evaluate( input, output2 ) );

        TS_ASSERT( FloatUtils::areEqual( output1[0], output2[0] ) );
        TS_ASSERT( FloatUtils::areEqual( output1[1], output2[1] ) );
    }

    void test_store_into_other_with_abs()
    {
        NLR::NetworkLevelReasoner nlr;

        populateNetworkWithAbs( nlr );

        NLR::NetworkLevelReasoner nlr2;

        TS_ASSERT_THROWS_NOTHING( nlr.storeIntoOther( nlr2 ) );

        double input[2];
        double output1[2];
        double output2[2];

        // case 1
        input[0] = 0;
        input[1] = 0;

        TS_ASSERT_THROWS_NOTHING( nlr.evaluate( input, output1 ) );
        TS_ASSERT_THROWS_NOTHING( nlr2.evaluate( input, output2 ) );

        TS_ASSERT( FloatUtils::areEqual( output1[0], output2[0] ) );
        TS_ASSERT( FloatUtils::areEqual( output1[1], output2[1] ) );

        // case 2
        input[0] = 1;
        input[1] = 1;

        TS_ASSERT_THROWS_NOTHING( nlr.evaluate( input, output1 ) );
        TS_ASSERT_THROWS_NOTHING( nlr2.evaluate( input, output2 ) );

        TS_ASSERT( FloatUtils::areEqual( output1[0], output2[0] ) );
        TS_ASSERT( FloatUtils::areEqual( output1[1], output2[1] ) );
    }

    void test_store_into_other_with_leaky_relu()
    {
        NLR::NetworkLevelReasoner nlr;

        populateNetworkWithLeakyRelu( nlr );

        NLR::NetworkLevelReasoner nlr2;

        TS_ASSERT_THROWS_NOTHING( nlr.storeIntoOther( nlr2 ) );

        double input[2];
        double output1[2];
        double output2[2];

        // case 1
        input[0] = 0;
        input[1] = 0;

        TS_ASSERT_THROWS_NOTHING( nlr.evaluate( input, output1 ) );
        TS_ASSERT_THROWS_NOTHING( nlr2.evaluate( input, output2 ) );

        TS_ASSERT( FloatUtils::areEqual( output1[0], output2[0] ) );
        TS_ASSERT( FloatUtils::areEqual( output1[1], output2[1] ) );

        // case 2
        input[0] = 1;
        input[1] = 1;

        TS_ASSERT_THROWS_NOTHING( nlr.evaluate( input, output1 ) );
        TS_ASSERT_THROWS_NOTHING( nlr2.evaluate( input, output2 ) );

        TS_ASSERT( FloatUtils::areEqual( output1[0], output2[0] ) );
        TS_ASSERT( FloatUtils::areEqual( output1[1], output2[1] ) );
    }

    void test_store_into_other_with_max()
    {
        NLR::NetworkLevelReasoner nlr;

        populateNetworkWithMax( nlr );

        NLR::NetworkLevelReasoner nlr2;

        TS_ASSERT_THROWS_NOTHING( nlr.storeIntoOther( nlr2 ) );

        double input[2];
        double output1[2];
        double output2[2];

        // case 1
        input[0] = 0;
        input[1] = 0;

        TS_ASSERT_THROWS_NOTHING( nlr.evaluate( input, output1 ) );
        TS_ASSERT_THROWS_NOTHING( nlr2.evaluate( input, output2 ) );

        TS_ASSERT( FloatUtils::areEqual( output1[0], output2[0] ) );

        // case 2
        input[0] = 1;
        input[1] = 1;

        TS_ASSERT_THROWS_NOTHING( nlr.evaluate( input, output1 ) );
        TS_ASSERT_THROWS_NOTHING( nlr2.evaluate( input, output2 ) );

        TS_ASSERT( FloatUtils::areEqual( output1[0], output2[0] ) );
    }

    void test_store_into_other_with_softmax()
    {
        NLR::NetworkLevelReasoner nlr;

        populateNetworkWithSoftmax( nlr );

        NLR::NetworkLevelReasoner nlr2;

        TS_ASSERT_THROWS_NOTHING( nlr.storeIntoOther( nlr2 ) );

        double input[2];
        double output1[2];
        double output2[2];

        // case 1
        input[0] = 0;
        input[1] = 0;

        TS_ASSERT_THROWS_NOTHING( nlr.evaluate( input, output1 ) );
        TS_ASSERT_THROWS_NOTHING( nlr2.evaluate( input, output2 ) );

        TS_ASSERT( FloatUtils::areEqual( output1[0], output2[0] ) );
        TS_ASSERT( FloatUtils::areEqual( output1[1], output2[1] ) );

        // case 2
        input[0] = 1;
        input[1] = 1;

        TS_ASSERT_THROWS_NOTHING( nlr.evaluate( input, output1 ) );
        TS_ASSERT_THROWS_NOTHING( nlr2.evaluate( input, output2 ) );

        TS_ASSERT( FloatUtils::areEqual( output1[0], output2[0] ) );
        TS_ASSERT( FloatUtils::areEqual( output1[1], output2[1] ) );
    }

    void test_store_into_other_with_bilinear()
    {
        NLR::NetworkLevelReasoner nlr;

        populateNetworkWithBilinear( nlr );

        NLR::NetworkLevelReasoner nlr2;

        TS_ASSERT_THROWS_NOTHING( nlr.storeIntoOther( nlr2 ) );

        double input[2];
        double output1[2];
        double output2[2];

        // case 1
        input[0] = 0;
        input[1] = 0;

        TS_ASSERT_THROWS_NOTHING( nlr.evaluate( input, output1 ) );
        TS_ASSERT_THROWS_NOTHING( nlr2.evaluate( input, output2 ) );

        TS_ASSERT( FloatUtils::areEqual( output1[0], output2[0] ) );

        // case 2
        input[0] = 1;
        input[1] = 1;

        TS_ASSERT_THROWS_NOTHING( nlr.evaluate( input, output1 ) );
        TS_ASSERT_THROWS_NOTHING( nlr2.evaluate( input, output2 ) );

        TS_ASSERT( FloatUtils::areEqual( output1[0], output2[0] ) );
    }

    void test_generate_input_query()
    {
        NLR::NetworkLevelReasoner nlr;

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
        nlr.setBias( 1, 1, 0 );
        nlr.setBias( 1, 2, 0 );

        nlr.setBias( 3, 0, 0 );
        nlr.setBias( 3, 1, 2 );

        nlr.setBias( 5, 0, 0 );
        nlr.setBias( 5, 1, 0 );

        // Mark the ReLU/Abs sources
        nlr.addActivationSource( 1, 0, 2, 0 );
        nlr.addActivationSource( 1, 1, 2, 1 );
        nlr.addActivationSource( 1, 2, 2, 2 );

        nlr.addActivationSource( 3, 0, 4, 0 );
        nlr.addActivationSource( 3, 1, 4, 1 );

        // Start the testing
        Query ipq;
        nlr.generateQuery( ipq );
        List<Equation> unhandledEquations;
        Set<unsigned> varsInUnhandledConstraints;
        TS_ASSERT(
            ipq.constructNetworkLevelReasoner( unhandledEquations, varsInUnhandledConstraints ) );
        NLR::NetworkLevelReasoner *reconstructedNlr = ipq.getNetworkLevelReasoner();

        double input[2];
        double output[2];

        input[0] = 1;
        input[1] = 1;

        TS_ASSERT_THROWS_NOTHING( reconstructedNlr->evaluate( input, output ) );

        TS_ASSERT( FloatUtils::areEqual( output[0], 2 ) );
        TS_ASSERT( FloatUtils::areEqual( output[1], 2 ) );

        input[0] = 1;
        input[1] = 2;

        TS_ASSERT_THROWS_NOTHING( reconstructedNlr->evaluate( input, output ) );

        TS_ASSERT( FloatUtils::areEqual( output[0], 4 ) );
        TS_ASSERT( FloatUtils::areEqual( output[1], 4 ) );
    }

    void test_simulate_relu()
    {
        NLR::NetworkLevelReasoner nlr;

        populateNetwork( nlr );

        unsigned simulationSize = Options::get()->getInt( Options::NUMBER_OF_SIMULATIONS );

        // With ReLUs, Inputs are zeros, only biases count
        Vector<Vector<double>> simulations1;
        simulations1.append( Vector<double>( simulationSize, 0 ) );
        simulations1.append( Vector<double>( simulationSize, 0 ) );

        TS_ASSERT_THROWS_NOTHING( nlr.simulate( &simulations1 ) );

        for ( unsigned i = 0; i < simulationSize; ++i )
        {
            TS_ASSERT( FloatUtils::areEqual(
                ( *( nlr.getLayer( nlr.getNumberOfLayers() - 1 )->getSimulations() ) )
                    .get( 0 )
                    .get( i ),
                1 ) );
            TS_ASSERT( FloatUtils::areEqual(
                ( *( nlr.getLayer( nlr.getNumberOfLayers() - 1 )->getSimulations() ) )
                    .get( 1 )
                    .get( i ),
                4 ) );
        }

        // With ReLUs, case 1
        Vector<Vector<double>> simulations2;
        simulations2.append( Vector<double>( simulationSize, 1 ) );
        simulations2.append( Vector<double>( simulationSize, 1 ) );

        TS_ASSERT_THROWS_NOTHING( nlr.simulate( &simulations2 ) );

        for ( unsigned i = 0; i < simulationSize; ++i )
        {
            TS_ASSERT( FloatUtils::areEqual(
                ( *( nlr.getLayer( nlr.getNumberOfLayers() - 1 )->getSimulations() ) )
                    .get( 0 )
                    .get( i ),
                1 ) );
            TS_ASSERT( FloatUtils::areEqual(
                ( *( nlr.getLayer( nlr.getNumberOfLayers() - 1 )->getSimulations() ) )
                    .get( 1 )
                    .get( i ),
                1 ) );
        }

        // With ReLUs, case 1 and 2
        Vector<Vector<double>> simulations3;
        simulations3.append( Vector<double>( simulationSize, 1 ) );
        simulations3.append( Vector<double>( simulationSize, 2 ) );

        TS_ASSERT_THROWS_NOTHING( nlr.simulate( &simulations3 ) );

        for ( unsigned i = 0; i < simulationSize; ++i )
        {
            TS_ASSERT( FloatUtils::areEqual(
                ( *( nlr.getLayer( nlr.getNumberOfLayers() - 1 )->getSimulations() ) )
                    .get( 0 )
                    .get( i ),
                0 ) );
            TS_ASSERT( FloatUtils::areEqual(
                ( *( nlr.getLayer( nlr.getNumberOfLayers() - 1 )->getSimulations() ) )
                    .get( 1 )
                    .get( i ),
                0 ) );
        }
    }

    void test_simulate_sigmoids()
    {
        NLR::NetworkLevelReasoner nlr;

        populateNetworkWithSigmoids( nlr );

        unsigned simulationSize = Options::get()->getInt( Options::NUMBER_OF_SIMULATIONS );

        // case 1
        Vector<Vector<double>> simulations1;
        simulations1.append( Vector<double>( simulationSize, 0 ) );
        simulations1.append( Vector<double>( simulationSize, 0 ) );

        TS_ASSERT_THROWS_NOTHING( nlr.simulate( &simulations1 ) );

        for ( unsigned i = 0; i < simulationSize; ++i )
        {
            TS_ASSERT( FloatUtils::areEqual(
                ( *( nlr.getLayer( nlr.getNumberOfLayers() - 1 )->getSimulations() ) )
                    .get( 0 )
                    .get( i ),
                0.6750,
                0.0001 ) );
            TS_ASSERT( FloatUtils::areEqual(
                ( *( nlr.getLayer( nlr.getNumberOfLayers() - 1 )->getSimulations() ) )
                    .get( 1 )
                    .get( i ),
                3.0167,
                0.0001 ) );
        }

        // case 2
        Vector<Vector<double>> simulations2;
        simulations2.append( Vector<double>( simulationSize, 1 ) );
        simulations2.append( Vector<double>( simulationSize, 1 ) );

        TS_ASSERT_THROWS_NOTHING( nlr.simulate( &simulations2 ) );

        for ( unsigned i = 0; i < simulationSize; ++i )
        {
            TS_ASSERT( FloatUtils::areEqual(
                ( *( nlr.getLayer( nlr.getNumberOfLayers() - 1 )->getSimulations() ) )
                    .get( 0 )
                    .get( i ),
                0.6032,
                0.0001 ) );
            TS_ASSERT( FloatUtils::areEqual(
                ( *( nlr.getLayer( nlr.getNumberOfLayers() - 1 )->getSimulations() ) )
                    .get( 1 )
                    .get( i ),
                2.5790,
                0.0001 ) );
        }

        // case 3
        Vector<Vector<double>> simulations3;
        simulations3.append( Vector<double>( simulationSize, 1 ) );
        simulations3.append( Vector<double>( simulationSize, 2 ) );

        TS_ASSERT_THROWS_NOTHING( nlr.simulate( &simulations3 ) );

        for ( unsigned i = 0; i < simulationSize; ++i )
        {
            TS_ASSERT( FloatUtils::areEqual(
                ( *( nlr.getLayer( nlr.getNumberOfLayers() - 1 )->getSimulations() ) )
                    .get( 0 )
                    .get( i ),
                0.5045,
                0.0001 ) );
            TS_ASSERT( FloatUtils::areEqual(
                ( *( nlr.getLayer( nlr.getNumberOfLayers() - 1 )->getSimulations() ) )
                    .get( 1 )
                    .get( i ),
                2.1957,
                0.0001 ) );
        }
    }

    void test_simulate_round()
    {
        NLR::NetworkLevelReasoner nlr;

        populateNetworkWithRound( nlr );

        unsigned simulationSize = Options::get()->getInt( Options::NUMBER_OF_SIMULATIONS );

        // With Round, Inputs are zeros, only biases count
        Vector<Vector<double>> simulations1;
        simulations1.append( Vector<double>( simulationSize, 0 ) );
        simulations1.append( Vector<double>( simulationSize, 0 ) );

        TS_ASSERT_THROWS_NOTHING( nlr.simulate( &simulations1 ) );

        for ( unsigned i = 0; i < simulationSize; ++i )
        {
            TS_ASSERT( FloatUtils::areEqual(
                ( *( nlr.getLayer( nlr.getNumberOfLayers() - 1 )->getSimulations() ) )
                    .get( 0 )
                    .get( i ),
                1 ) );
            TS_ASSERT( FloatUtils::areEqual(
                ( *( nlr.getLayer( nlr.getNumberOfLayers() - 1 )->getSimulations() ) )
                    .get( 1 )
                    .get( i ),
                4 ) );
        }

        // With Round, case 1
        Vector<Vector<double>> simulations2;
        simulations2.append( Vector<double>( simulationSize, 2.1 ) );
        simulations2.append( Vector<double>( simulationSize, 1.4 ) );

        TS_ASSERT_THROWS_NOTHING( nlr.simulate( &simulations2 ) );

        for ( unsigned i = 0; i < simulationSize; ++i )
        {
            TS_ASSERT( FloatUtils::areEqual(
                ( *( nlr.getLayer( nlr.getNumberOfLayers() - 1 )->getSimulations() ) )
                    .get( 0 )
                    .get( i ),
                2,
                0.0001 ) );
            TS_ASSERT( FloatUtils::areEqual(
                ( *( nlr.getLayer( nlr.getNumberOfLayers() - 1 )->getSimulations() ) )
                    .get( 1 )
                    .get( i ),
                -4,
                0.0001 ) );
        }

        // With Round, case 2
        Vector<Vector<double>> simulations3;
        simulations3.append( Vector<double>( simulationSize, 2.1 ) );
        simulations3.append( Vector<double>( simulationSize, 1.6 ) );

        TS_ASSERT_THROWS_NOTHING( nlr.simulate( &simulations3 ) );

        for ( unsigned i = 0; i < simulationSize; ++i )
        {
            TS_ASSERT( FloatUtils::areEqual(
                ( *( nlr.getLayer( nlr.getNumberOfLayers() - 1 )->getSimulations() ) )
                    .get( 0 )
                    .get( i ),
                0,
                0.0001 ) );
            TS_ASSERT( FloatUtils::areEqual(
                ( *( nlr.getLayer( nlr.getNumberOfLayers() - 1 )->getSimulations() ) )
                    .get( 1 )
                    .get( i ),
                -12,
                0.0001 ) );
        }
    }

    void test_simulate_sign()
    {
        NLR::NetworkLevelReasoner nlr;

        populateNetworkWithSign( nlr );

        unsigned simulationSize = Options::get()->getInt( Options::NUMBER_OF_SIMULATIONS );

        // With Sign, Inputs are zeros, only biases count
        Vector<Vector<double>> simulations1;
        simulations1.append( Vector<double>( simulationSize, 0 ) );
        simulations1.append( Vector<double>( simulationSize, 0 ) );

        TS_ASSERT_THROWS_NOTHING( nlr.simulate( &simulations1 ) );

        for ( unsigned i = 0; i < simulationSize; ++i )
        {
            TS_ASSERT( FloatUtils::areEqual(
                ( *( nlr.getLayer( nlr.getNumberOfLayers() - 1 )->getSimulations() ) )
                    .get( 0 )
                    .get( i ),
                1 ) );
            TS_ASSERT( FloatUtils::areEqual(
                ( *( nlr.getLayer( nlr.getNumberOfLayers() - 1 )->getSimulations() ) )
                    .get( 1 )
                    .get( i ),
                4 ) );
        }

        // With Sign, case 1
        Vector<Vector<double>> simulations2;
        simulations2.append( Vector<double>( simulationSize, -2 ) );
        simulations2.append( Vector<double>( simulationSize, -2 ) );

        TS_ASSERT_THROWS_NOTHING( nlr.simulate( &simulations2 ) );

        for ( unsigned i = 0; i < simulationSize; ++i )
        {
            TS_ASSERT( FloatUtils::areEqual(
                ( *( nlr.getLayer( nlr.getNumberOfLayers() - 1 )->getSimulations() ) )
                    .get( 0 )
                    .get( i ),
                1 ) );
            TS_ASSERT( FloatUtils::areEqual(
                ( *( nlr.getLayer( nlr.getNumberOfLayers() - 1 )->getSimulations() ) )
                    .get( 1 )
                    .get( i ),
                4 ) );
        }

        // With Sign, case 2
        Vector<Vector<double>> simulations3;
        simulations3.append( Vector<double>( simulationSize, -1 ) );
        simulations3.append( Vector<double>( simulationSize, 1 ) );

        TS_ASSERT_THROWS_NOTHING( nlr.simulate( &simulations3 ) );

        for ( unsigned i = 0; i < simulationSize; ++i )
        {
            TS_ASSERT( FloatUtils::areEqual(
                ( *( nlr.getLayer( nlr.getNumberOfLayers() - 1 )->getSimulations() ) )
                    .get( 0 )
                    .get( i ),
                -1 ) );
            TS_ASSERT( FloatUtils::areEqual(
                ( *( nlr.getLayer( nlr.getNumberOfLayers() - 1 )->getSimulations() ) )
                    .get( 1 )
                    .get( i ),
                -4 ) );
        }
    }

    void test_simulate_abs()
    {
        NLR::NetworkLevelReasoner nlr;

        populateNetworkWithAbs( nlr );

        unsigned simulationSize = Options::get()->getInt( Options::NUMBER_OF_SIMULATIONS );

        // With Abs, Inputs are zeros, only biases count
        Vector<Vector<double>> simulations1;
        simulations1.append( Vector<double>( simulationSize, 0 ) );
        simulations1.append( Vector<double>( simulationSize, 0 ) );

        TS_ASSERT_THROWS_NOTHING( nlr.simulate( &simulations1 ) );

        for ( unsigned i = 0; i < simulationSize; ++i )
        {
            TS_ASSERT( FloatUtils::areEqual(
                ( *( nlr.getLayer( nlr.getNumberOfLayers() - 1 )->getSimulations() ) )
                    .get( 0 )
                    .get( i ),
                1 ) );
            TS_ASSERT( FloatUtils::areEqual(
                ( *( nlr.getLayer( nlr.getNumberOfLayers() - 1 )->getSimulations() ) )
                    .get( 1 )
                    .get( i ),
                4 ) );
        }

        // With Abs, case 1
        Vector<Vector<double>> simulations2;
        simulations2.append( Vector<double>( simulationSize, -2 ) );
        simulations2.append( Vector<double>( simulationSize, -2 ) );

        TS_ASSERT_THROWS_NOTHING( nlr.simulate( &simulations2 ) );

        for ( unsigned i = 0; i < simulationSize; ++i )
        {
            TS_ASSERT( FloatUtils::areEqual(
                ( *( nlr.getLayer( nlr.getNumberOfLayers() - 1 )->getSimulations() ) )
                    .get( 0 )
                    .get( i ),
                1 ) );
            TS_ASSERT( FloatUtils::areEqual(
                ( *( nlr.getLayer( nlr.getNumberOfLayers() - 1 )->getSimulations() ) )
                    .get( 1 )
                    .get( i ),
                4 ) );
        }

        // With Abs, case 2
        Vector<Vector<double>> simulations3;
        simulations3.append( Vector<double>( simulationSize, 1 ) );
        simulations3.append( Vector<double>( simulationSize, 2 ) );

        TS_ASSERT_THROWS_NOTHING( nlr.simulate( &simulations3 ) );

        for ( unsigned i = 0; i < simulationSize; ++i )
        {
            TS_ASSERT( FloatUtils::areEqual(
                ( *( nlr.getLayer( nlr.getNumberOfLayers() - 1 )->getSimulations() ) )
                    .get( 0 )
                    .get( i ),
                4 ) );
            TS_ASSERT( FloatUtils::areEqual(
                ( *( nlr.getLayer( nlr.getNumberOfLayers() - 1 )->getSimulations() ) )
                    .get( 1 )
                    .get( i ),
                10 ) );
        }
    }

    void test_simulate_leaky_relu()
    {
        NLR::NetworkLevelReasoner nlr;

        populateNetworkWithLeakyRelu( nlr );

        unsigned simulationSize = Options::get()->getInt( Options::NUMBER_OF_SIMULATIONS );

        // With Leaky ReLU, Inputs are zeros, only biases count
        Vector<Vector<double>> simulations1;
        simulations1.append( Vector<double>( simulationSize, 0 ) );
        simulations1.append( Vector<double>( simulationSize, 0 ) );

        TS_ASSERT_THROWS_NOTHING( nlr.simulate( &simulations1 ) );

        for ( unsigned i = 0; i < simulationSize; ++i )
        {
            TS_ASSERT( FloatUtils::areEqual(
                ( *( nlr.getLayer( nlr.getNumberOfLayers() - 1 )->getSimulations() ) )
                    .get( 0 )
                    .get( i ),
                1 ) );
            TS_ASSERT( FloatUtils::areEqual(
                ( *( nlr.getLayer( nlr.getNumberOfLayers() - 1 )->getSimulations() ) )
                    .get( 1 )
                    .get( i ),
                4 ) );
        }

        // With Leaky ReLU, case 1  (alpha=0.1)
        Vector<Vector<double>> simulations2;
        simulations2.append( Vector<double>( simulationSize, 1 ) );
        simulations2.append( Vector<double>( simulationSize, 1 ) );

        TS_ASSERT_THROWS_NOTHING( nlr.simulate( &simulations2 ) );

        for ( unsigned i = 0; i < simulationSize; ++i )
        {
            TS_ASSERT( FloatUtils::areEqual(
                ( *( nlr.getLayer( nlr.getNumberOfLayers() - 1 )->getSimulations() ) )
                    .get( 0 )
                    .get( i ),
                0.9,
                0.0001 ) );
            TS_ASSERT( FloatUtils::areEqual(
                ( *( nlr.getLayer( nlr.getNumberOfLayers() - 1 )->getSimulations() ) )
                    .get( 1 )
                    .get( i ),
                0.57,
                0.0001 ) );
        }

        // With Leaky ReLU, case 2
        Vector<Vector<double>> simulations3;
        simulations3.append( Vector<double>( simulationSize, 1 ) );
        simulations3.append( Vector<double>( simulationSize, 2 ) );

        TS_ASSERT_THROWS_NOTHING( nlr.simulate( &simulations3 ) );

        for ( unsigned i = 0; i < simulationSize; ++i )
        {
            TS_ASSERT( FloatUtils::areEqual(
                ( *( nlr.getLayer( nlr.getNumberOfLayers() - 1 )->getSimulations() ) )
                    .get( 0 )
                    .get( i ),
                -0.04,
                0.0001 ) );
            TS_ASSERT( FloatUtils::areEqual(
                ( *( nlr.getLayer( nlr.getNumberOfLayers() - 1 )->getSimulations() ) )
                    .get( 1 )
                    .get( i ),
                -0.76,
                0.0001 ) );
        }
    }

    void test_simulate_max()
    {
        NLR::NetworkLevelReasoner nlr;

        populateNetworkWithMax( nlr );

        unsigned simulationSize = Options::get()->getInt( Options::NUMBER_OF_SIMULATIONS );

        // With Max, Inputs are zeros, only biases count
        Vector<Vector<double>> simulations1;
        simulations1.append( Vector<double>( simulationSize, 0 ) );
        simulations1.append( Vector<double>( simulationSize, 0 ) );

        TS_ASSERT_THROWS_NOTHING( nlr.simulate( &simulations1 ) );

        for ( unsigned i = 0; i < simulationSize; ++i )
        {
            TS_ASSERT( FloatUtils::areEqual(
                ( *( nlr.getLayer( nlr.getNumberOfLayers() - 1 )->getSimulations() ) )
                    .get( 0 )
                    .get( i ),
                -3 ) );
        }

        // With Max, case 1
        Vector<Vector<double>> simulations2;
        simulations2.append( Vector<double>( simulationSize, 1 ) );
        simulations2.append( Vector<double>( simulationSize, -3 ) );

        TS_ASSERT_THROWS_NOTHING( nlr.simulate( &simulations2 ) );

        for ( unsigned i = 0; i < simulationSize; ++i )
        {
            TS_ASSERT( FloatUtils::areEqual(
                ( *( nlr.getLayer( nlr.getNumberOfLayers() - 1 )->getSimulations() ) )
                    .get( 0 )
                    .get( i ),
                -18 ) );
        }

        // With Max, case 2
        Vector<Vector<double>> simulations3;
        simulations3.append( Vector<double>( simulationSize, -3 ) );
        simulations3.append( Vector<double>( simulationSize, 3 ) );

        TS_ASSERT_THROWS_NOTHING( nlr.simulate( &simulations3 ) );

        for ( unsigned i = 0; i < simulationSize; ++i )
        {
            TS_ASSERT( FloatUtils::areEqual(
                ( *( nlr.getLayer( nlr.getNumberOfLayers() - 1 )->getSimulations() ) )
                    .get( 0 )
                    .get( i ),
                -5 ) );
        }
    }

    void test_simulate_softmax()
    {
        NLR::NetworkLevelReasoner nlr;

        populateNetworkWithSoftmax( nlr );

        unsigned simulationSize = Options::get()->getInt( Options::NUMBER_OF_SIMULATIONS );

        // With Softmax, Inputs are zeros, only biases count
        Vector<Vector<double>> simulations1;
        simulations1.append( Vector<double>( simulationSize, 0 ) );
        simulations1.append( Vector<double>( simulationSize, 0 ) );

        TS_ASSERT_THROWS_NOTHING( nlr.simulate( &simulations1 ) );

        for ( unsigned i = 0; i < simulationSize; ++i )
        {
            TS_ASSERT( FloatUtils::areEqual(
                ( *( nlr.getLayer( nlr.getNumberOfLayers() - 1 )->getSimulations() ) )
                    .get( 0 )
                    .get( i ),
                0.2999,
                0.0001 ) );
            TS_ASSERT( FloatUtils::areEqual(
                ( *( nlr.getLayer( nlr.getNumberOfLayers() - 1 )->getSimulations() ) )
                    .get( 1 )
                    .get( i ),
                2.4001,
                0.0001 ) );
        }

        // With Softmax, case 1
        Vector<Vector<double>> simulations2;
        simulations2.append( Vector<double>( simulationSize, 1 ) );
        simulations2.append( Vector<double>( simulationSize, -3 ) );

        TS_ASSERT_THROWS_NOTHING( nlr.simulate( &simulations2 ) );

        for ( unsigned i = 0; i < simulationSize; ++i )
        {
            TS_ASSERT( FloatUtils::areEqual(
                ( *( nlr.getLayer( nlr.getNumberOfLayers() - 1 )->getSimulations() ) )
                    .get( 0 )
                    .get( i ),
                0.1192,
                0.0001 ) );
            TS_ASSERT( FloatUtils::areEqual(
                ( *( nlr.getLayer( nlr.getNumberOfLayers() - 1 )->getSimulations() ) )
                    .get( 1 )
                    .get( i ),
                2.7615,
                0.0001 ) );
        }

        // With Softmax, case 2
        Vector<Vector<double>> simulations3;
        simulations3.append( Vector<double>( simulationSize, -3 ) );
        simulations3.append( Vector<double>( simulationSize, 3 ) );

        TS_ASSERT_THROWS_NOTHING( nlr.simulate( &simulations3 ) );

        for ( unsigned i = 0; i < simulationSize; ++i )
        {
            TS_ASSERT( FloatUtils::areEqual(
                ( *( nlr.getLayer( nlr.getNumberOfLayers() - 1 )->getSimulations() ) )
                    .get( 0 )
                    .get( i ),
                0.1206,
                0.0001 ) );
            TS_ASSERT( FloatUtils::areEqual(
                ( *( nlr.getLayer( nlr.getNumberOfLayers() - 1 )->getSimulations() ) )
                    .get( 1 )
                    .get( i ),
                2.7588,
                0.0001 ) );
        }
    }

    void test_simulate_bilinear()
    {
        NLR::NetworkLevelReasoner nlr;

        populateNetworkWithBilinear( nlr );

        unsigned simulationSize = Options::get()->getInt( Options::NUMBER_OF_SIMULATIONS );

        // With Bilinear, Inputs are zeros, only biases count
        Vector<Vector<double>> simulations1;
        simulations1.append( Vector<double>( simulationSize, 0 ) );
        simulations1.append( Vector<double>( simulationSize, 0 ) );

        TS_ASSERT_THROWS_NOTHING( nlr.simulate( &simulations1 ) );

        for ( unsigned i = 0; i < simulationSize; ++i )
        {
            TS_ASSERT( FloatUtils::areEqual(
                ( *( nlr.getLayer( nlr.getNumberOfLayers() - 1 )->getSimulations() ) )
                    .get( 0 )
                    .get( i ),
                0 ) );
        }

        // With Bilinear, case 1
        Vector<Vector<double>> simulations2;
        simulations2.append( Vector<double>( simulationSize, 0.1 ) );
        simulations2.append( Vector<double>( simulationSize, -0.3 ) );

        TS_ASSERT_THROWS_NOTHING( nlr.simulate( &simulations2 ) );

        for ( unsigned i = 0; i < simulationSize; ++i )
        {
            TS_ASSERT( FloatUtils::areEqual(
                ( *( nlr.getLayer( nlr.getNumberOfLayers() - 1 )->getSimulations() ) )
                    .get( 0 )
                    .get( i ),
                2.8304,
                0.0001 ) );
        }

        // With Bilinear, case 2
        Vector<Vector<double>> simulations3;
        simulations3.append( Vector<double>( simulationSize, -0.3 ) );
        simulations3.append( Vector<double>( simulationSize, 0.3 ) );

        TS_ASSERT_THROWS_NOTHING( nlr.simulate( &simulations3 ) );

        for ( unsigned i = 0; i < simulationSize; ++i )
        {
            TS_ASSERT( FloatUtils::areEqual(
                ( *( nlr.getLayer( nlr.getNumberOfLayers() - 1 )->getSimulations() ) )
                    .get( 0 )
                    .get( i ),
                0.0912,
                0.0001 ) );
        }
    }

    void test_simulate_non_consecutive_layers()
    {
        NLR::NetworkLevelReasoner nlr;

        // Create the layers
        nlr.addLayer( 0, NLR::Layer::INPUT, 2 );
        nlr.addLayer( 1, NLR::Layer::WEIGHTED_SUM, 3 );
        nlr.addLayer( 2, NLR::Layer::RELU, 3 );
        nlr.addLayer( 3, NLR::Layer::WEIGHTED_SUM, 2 );
        nlr.addLayer( 4, NLR::Layer::RELU, 3 );
        nlr.addLayer( 5, NLR::Layer::WEIGHTED_SUM, 1 );

        // Mark layer dependencies
        nlr.addLayerDependency( 0, 1 );
        nlr.addLayerDependency( 1, 2 );
        nlr.addLayerDependency( 2, 3 );
        nlr.addLayerDependency( 0, 3 );
        nlr.addLayerDependency( 3, 4 );
        nlr.addLayerDependency( 0, 4 );
        nlr.addLayerDependency( 4, 5 );

        // Set the weights and relus
        nlr.setWeight( 0, 0, 1, 0, 1 );
        nlr.setWeight( 0, 0, 1, 1, 2 );
        nlr.setWeight( 0, 1, 1, 1, -3 );
        nlr.setWeight( 0, 1, 1, 2, 1 );

        nlr.addActivationSource( 1, 0, 2, 0 );
        nlr.addActivationSource( 1, 1, 2, 1 );
        nlr.addActivationSource( 1, 2, 2, 2 );

        nlr.setWeight( 2, 0, 3, 0, 1 );
        nlr.setWeight( 2, 1, 3, 0, 2 );
        nlr.setWeight( 2, 2, 3, 1, -2 );
        nlr.setWeight( 0, 1, 3, 1, 1 );

        nlr.addActivationSource( 3, 0, 4, 0 );
        nlr.addActivationSource( 3, 1, 4, 1 );
        nlr.addActivationSource( 0, 0, 4, 2 );

        nlr.setWeight( 4, 0, 5, 0, 1 );
        nlr.setWeight( 4, 1, 5, 0, 1 );
        nlr.setWeight( 4, 2, 5, 0, 1 );

        unsigned simulationSize = Options::get()->getInt( Options::NUMBER_OF_SIMULATIONS );

        // Simulate1
        Vector<Vector<double>> simulations1;
        simulations1.append( Vector<double>( simulationSize, 1 ) );
        simulations1.append( Vector<double>( simulationSize, 1 ) );

        TS_ASSERT_THROWS_NOTHING( nlr.simulate( &simulations1 ) );

        for ( unsigned i = 0; i < simulationSize; ++i )
            TS_ASSERT( FloatUtils::areEqual(
                ( *( nlr.getLayer( nlr.getNumberOfLayers() - 1 )->getSimulations() ) )
                    .get( 0 )
                    .get( i ),
                2 ) );

        // Simulate2
        Vector<Vector<double>> simulations2;
        simulations2.append( Vector<double>( simulationSize, -1 ) );
        simulations2.append( Vector<double>( simulationSize, 2 ) );

        TS_ASSERT_THROWS_NOTHING( nlr.simulate( &simulations2 ) );

        for ( unsigned i = 0; i < simulationSize; ++i )
            TS_ASSERT( FloatUtils::areEqual(
                ( *( nlr.getLayer( nlr.getNumberOfLayers() - 1 )->getSimulations() ) )
                    .get( 0 )
                    .get( i ),
                0 ) );
    }

    void test_simulate_abs_and_relu()
    {
        NLR::NetworkLevelReasoner nlr;

        populateNetworkWithAbsAndRelu( nlr );

        unsigned simulationSize = Options::get()->getInt( Options::NUMBER_OF_SIMULATIONS );

        // Simulate1
        Vector<Vector<double>> simulations1;
        simulations1.append( Vector<double>( simulationSize, 1 ) );
        simulations1.append( Vector<double>( simulationSize, 1 ) );

        TS_ASSERT_THROWS_NOTHING( nlr.simulate( &simulations1 ) );

        for ( unsigned i = 0; i < simulationSize; ++i )
        {
            TS_ASSERT( FloatUtils::areEqual(
                ( *( nlr.getLayer( nlr.getNumberOfLayers() - 1 )->getSimulations() ) )
                    .get( 0 )
                    .get( i ),
                2 ) );
            TS_ASSERT( FloatUtils::areEqual(
                ( *( nlr.getLayer( nlr.getNumberOfLayers() - 1 )->getSimulations() ) )
                    .get( 1 )
                    .get( i ),
                2 ) );
        }

        // Simulate2
        Vector<Vector<double>> simulations2;
        simulations2.append( Vector<double>( simulationSize, 1 ) );
        simulations2.append( Vector<double>( simulationSize, 2 ) );

        TS_ASSERT_THROWS_NOTHING( nlr.simulate( &simulations2 ) );

        for ( unsigned i = 0; i < simulationSize; ++i )
        {
            TS_ASSERT( FloatUtils::areEqual(
                ( *( nlr.getLayer( nlr.getNumberOfLayers() - 1 )->getSimulations() ) )
                    .get( 0 )
                    .get( i ),
                4 ) );
            TS_ASSERT( FloatUtils::areEqual(
                ( *( nlr.getLayer( nlr.getNumberOfLayers() - 1 )->getSimulations() ) )
                    .get( 1 )
                    .get( i ),
                4 ) );
        }
    }

    void test_simulate_round_and_sign()
    {
        NLR::NetworkLevelReasoner nlr;

        populateNetworkWithRoundAndSign( nlr );

        unsigned simulationSize = Options::get()->getInt( Options::NUMBER_OF_SIMULATIONS );

        // With Round/Sign, case 1
        Vector<Vector<double>> simulations1;
        simulations1.append( Vector<double>( simulationSize, 1.6 ) );
        simulations1.append( Vector<double>( simulationSize, 1.4 ) );

        TS_ASSERT_THROWS_NOTHING( nlr.simulate( &simulations1 ) );

        for ( unsigned i = 0; i < simulationSize; ++i )
        {
            TS_ASSERT( FloatUtils::areEqual(
                ( *( nlr.getLayer( nlr.getNumberOfLayers() - 1 )->getSimulations() ) )
                    .get( 0 )
                    .get( i ),
                1 ) );
            TS_ASSERT( FloatUtils::areEqual(
                ( *( nlr.getLayer( nlr.getNumberOfLayers() - 1 )->getSimulations() ) )
                    .get( 1 )
                    .get( i ),
                -2 ) );
        }

        // With Round/Sign, case 2
        Vector<Vector<double>> simulations2;
        simulations2.append( Vector<double>( simulationSize, 1.6 ) );
        simulations2.append( Vector<double>( simulationSize, 1.6 ) );

        TS_ASSERT_THROWS_NOTHING( nlr.simulate( &simulations2 ) );

        for ( unsigned i = 0; i < simulationSize; ++i )
        {
            TS_ASSERT( FloatUtils::areEqual(
                ( *( nlr.getLayer( nlr.getNumberOfLayers() - 1 )->getSimulations() ) )
                    .get( 0 )
                    .get( i ),
                -1 ) );
            TS_ASSERT( FloatUtils::areEqual(
                ( *( nlr.getLayer( nlr.getNumberOfLayers() - 1 )->getSimulations() ) )
                    .get( 1 )
                    .get( i ),
                -4 ) );
        }
    }

    void test_simulate_leaky_relu_and_sigmoid()
    {
        NLR::NetworkLevelReasoner nlr;

        populateNetworkWithLeakyReluAndSigmoid( nlr );

        unsigned simulationSize = Options::get()->getInt( Options::NUMBER_OF_SIMULATIONS );

        // With LeakyReLU/Sigmoid, case 1
        Vector<Vector<double>> simulations1;
        simulations1.append( Vector<double>( simulationSize, 1 ) );
        simulations1.append( Vector<double>( simulationSize, 1 ) );

        TS_ASSERT_THROWS_NOTHING( nlr.simulate( &simulations1 ) );

        for ( unsigned i = 0; i < simulationSize; ++i )
        {
            TS_ASSERT( FloatUtils::areEqual(
                ( *( nlr.getLayer( nlr.getNumberOfLayers() - 1 )->getSimulations() ) )
                    .get( 0 )
                    .get( i ),
                0.7109,
                0.0001 ) );
            TS_ASSERT( FloatUtils::areEqual(
                ( *( nlr.getLayer( nlr.getNumberOfLayers() - 1 )->getSimulations() ) )
                    .get( 1 )
                    .get( i ),
                1.4602,
                0.0001 ) );
        }

        // With LeakyReLU/Sigmoid, case 2
        Vector<Vector<double>> simulations2;
        simulations2.append( Vector<double>( simulationSize, 1 ) );
        simulations2.append( Vector<double>( simulationSize, 2 ) );

        TS_ASSERT_THROWS_NOTHING( nlr.simulate( &simulations2 ) );

        for ( unsigned i = 0; i < simulationSize; ++i )
        {
            TS_ASSERT( FloatUtils::areEqual(
                ( *( nlr.getLayer( nlr.getNumberOfLayers() - 1 )->getSimulations() ) )
                    .get( 0 )
                    .get( i ),
                0.4013,
                0.0001 ) );
            TS_ASSERT( FloatUtils::areEqual(
                ( *( nlr.getLayer( nlr.getNumberOfLayers() - 1 )->getSimulations() ) )
                    .get( 1 )
                    .get( i ),
                0.6508,
                0.0001 ) );
        }
    }

    void test_simulate_softmax_and_max()
    {
        NLR::NetworkLevelReasoner nlr;

        populateNetworkWithSoftmaxAndMax( nlr );

        unsigned simulationSize = Options::get()->getInt( Options::NUMBER_OF_SIMULATIONS );

        // With LeakyReLU/Sigmoid, case 1
        Vector<Vector<double>> simulations1;
        simulations1.append( Vector<double>( simulationSize, 1 ) );
        simulations1.append( Vector<double>( simulationSize, -3 ) );

        TS_ASSERT_THROWS_NOTHING( nlr.simulate( &simulations1 ) );

        for ( unsigned i = 0; i < simulationSize; ++i )
        {
            TS_ASSERT( FloatUtils::areEqual(
                ( *( nlr.getLayer( nlr.getNumberOfLayers() - 1 )->getSimulations() ) )
                    .get( 0 )
                    .get( i ),
                -2.9998,
                0.0001 ) );
        }

        // With LeakyReLU/Sigmoid, case 2
        Vector<Vector<double>> simulations2;
        simulations2.append( Vector<double>( simulationSize, -3 ) );
        simulations2.append( Vector<double>( simulationSize, 3 ) );

        TS_ASSERT_THROWS_NOTHING( nlr.simulate( &simulations2 ) );

        for ( unsigned i = 0; i < simulationSize; ++i )
        {
            TS_ASSERT( FloatUtils::areEqual(
                ( *( nlr.getLayer( nlr.getNumberOfLayers() - 1 )->getSimulations() ) )
                    .get( 0 )
                    .get( i ),
                -1,
                0.0001 ) );
        }
    }

    void test_simulate_relu_and_bilinear()
    {
        NLR::NetworkLevelReasoner nlr;

        populateNetworkWithReluAndBilinear( nlr );

        unsigned simulationSize = Options::get()->getInt( Options::NUMBER_OF_SIMULATIONS );

        // With Relu/Bilinear, case 1
        Vector<Vector<double>> simulations1;
        simulations1.append( Vector<double>( simulationSize, 1 ) );
        simulations1.append( Vector<double>( simulationSize, 1 ) );

        TS_ASSERT_THROWS_NOTHING( nlr.simulate( &simulations1 ) );

        for ( unsigned i = 0; i < simulationSize; ++i )
        {
            TS_ASSERT( FloatUtils::areEqual(
                ( *( nlr.getLayer( nlr.getNumberOfLayers() - 1 )->getSimulations() ) )
                    .get( 0 )
                    .get( i ),
                1 ) );
        }

        // With ReLU/Bilinear, case 2
        Vector<Vector<double>> simulations2;
        simulations2.append( Vector<double>( simulationSize, 1 ) );
        simulations2.append( Vector<double>( simulationSize, 2 ) );

        TS_ASSERT_THROWS_NOTHING( nlr.simulate( &simulations2 ) );

        for ( unsigned i = 0; i < simulationSize; ++i )
        {
            TS_ASSERT( FloatUtils::areEqual(
                ( *( nlr.getLayer( nlr.getNumberOfLayers() - 1 )->getSimulations() ) )
                    .get( 0 )
                    .get( i ),
                0 ) );
        }
    }

    void test_interval_arithmetic_bound_propagation_relu_constraints()
    {
        NLR::NetworkLevelReasoner nlr;
        populateNetwork( nlr );

        MockTableau tableau;
        tableau.getBoundManager().initialize( 14 );

        // Initialize the bounds
        tableau.setLowerBound( 0, -1 );
        tableau.setUpperBound( 0, 1 );
        tableau.setLowerBound( 1, -1 );
        tableau.setUpperBound( 1, 1 );

        double large = 1000;
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

        nlr.setTableau( &tableau );

        // Initialize
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );

        // Perform the tightening pass
        TS_ASSERT_THROWS_NOTHING( nlr.intervalArithmeticBoundPropagation() );

        List<Tightening> expectedBounds( {
            Tightening( 2, 0, Tightening::LB ),   Tightening( 2, 2, Tightening::UB ),
            Tightening( 3, 0, Tightening::LB ),   Tightening( 3, 2, Tightening::UB ),

            Tightening( 4, -5, Tightening::LB ),  Tightening( 4, 5, Tightening::UB ),
            Tightening( 5, 0, Tightening::LB ),   Tightening( 5, 5, Tightening::UB ),

            Tightening( 6, -1, Tightening::LB ),  Tightening( 6, 1, Tightening::UB ),
            Tightening( 7, 0, Tightening::LB ),   Tightening( 7, 1, Tightening::UB ),

            Tightening( 8, -1, Tightening::LB ),  Tightening( 8, 7, Tightening::UB ),
            Tightening( 9, 0, Tightening::LB ),   Tightening( 9, 7, Tightening::UB ),

            Tightening( 10, -1, Tightening::LB ), Tightening( 10, 7, Tightening::UB ),
            Tightening( 11, 0, Tightening::LB ),  Tightening( 11, 7, Tightening::UB ),

            Tightening( 12, 0, Tightening::LB ),  Tightening( 12, 7, Tightening::UB ),
            Tightening( 13, 0, Tightening::LB ),  Tightening( 13, 28, Tightening::UB ),
        } );

        List<Tightening> bounds;
        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );
        TS_ASSERT( boundsEqual( bounds, expectedBounds ) );

        // Change the current bounds
        tableau.setLowerBound( 0, -3 );
        tableau.setUpperBound( 0, 1 );
        tableau.setLowerBound( 1, -1 );
        tableau.setUpperBound( 1, 2 );

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

        // Initialize
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );

        // Perform the tightening pass
        TS_ASSERT_THROWS_NOTHING( nlr.intervalArithmeticBoundPropagation() );

        List<Tightening> expectedBounds2( {
            Tightening( 2, -2, Tightening::LB ),  Tightening( 2, 2, Tightening::UB ),
            Tightening( 3, 0, Tightening::LB ),   Tightening( 3, 2, Tightening::UB ),

            Tightening( 4, -12, Tightening::LB ), Tightening( 4, 5, Tightening::UB ),
            Tightening( 5, 0, Tightening::LB ),   Tightening( 5, 5, Tightening::UB ),

            Tightening( 6, -1, Tightening::LB ),  Tightening( 6, 2, Tightening::UB ),
            Tightening( 7, 0, Tightening::LB ),   Tightening( 7, 2, Tightening::UB ),

            Tightening( 8, -2, Tightening::LB ),  Tightening( 8, 7, Tightening::UB ),
            Tightening( 9, 0, Tightening::LB ),   Tightening( 9, 7, Tightening::UB ),

            Tightening( 10, -2, Tightening::LB ), Tightening( 10, 7, Tightening::UB ),
            Tightening( 11, 0, Tightening::LB ),  Tightening( 11, 7, Tightening::UB ),

            Tightening( 12, 0, Tightening::LB ),  Tightening( 12, 7, Tightening::UB ),
            Tightening( 13, 0, Tightening::LB ),  Tightening( 13, 28, Tightening::UB ),
        } );

        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );
        TS_ASSERT( boundsEqual( bounds, expectedBounds2 ) );
    }

    void test_interval_arithmetic_bound_propagation_abs_constraints()
    {
        NLR::NetworkLevelReasoner nlr;

        // Create the layers
        nlr.addLayer( 0, NLR::Layer::INPUT, 2 );
        nlr.addLayer( 1, NLR::Layer::WEIGHTED_SUM, 3 );
        nlr.addLayer( 2, NLR::Layer::ABSOLUTE_VALUE, 3 );
        nlr.addLayer( 3, NLR::Layer::WEIGHTED_SUM, 2 );
        nlr.addLayer( 4, NLR::Layer::ABSOLUTE_VALUE, 2 );
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

        // Layer dependenices
        nlr.addLayerDependency( 0, 1 );
        nlr.addLayerDependency( 1, 2 );
        nlr.addLayerDependency( 2, 3 );
        nlr.addLayerDependency( 3, 4 );
        nlr.addLayerDependency( 4, 5 );

        // Variable indexing
        nlr.setNeuronVariable( NLR::NeuronIndex( 0, 0 ), 0 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 0, 1 ), 1 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 1, 0 ), 2 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 1, 1 ), 4 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 1, 2 ), 6 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 2, 0 ), 3 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 2, 1 ), 5 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 2, 2 ), 7 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 3, 0 ), 8 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 3, 1 ), 10 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 4, 0 ), 9 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 4, 1 ), 11 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 5, 0 ), 12 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 5, 1 ), 13 );

        MockTableau tableau;
        tableau.getBoundManager().initialize( 14 );

        // Initialize the bounds
        tableau.setLowerBound( 0, -1 );
        tableau.setUpperBound( 0, 2 );
        tableau.setLowerBound( 1, -1 );
        tableau.setUpperBound( 1, 2 );

        double large = 1000;
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

        nlr.setTableau( &tableau );

        // Initialize
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );

        // Perform the tightening pass
        TS_ASSERT_THROWS_NOTHING( nlr.intervalArithmeticBoundPropagation() );

        List<Tightening> expectedBounds( {
            Tightening( 2, 0, Tightening::LB ),   Tightening( 2, 3, Tightening::UB ),
            Tightening( 3, 0, Tightening::LB ),   Tightening( 3, 3, Tightening::UB ),

            Tightening( 4, -8, Tightening::LB ),  Tightening( 4, 7, Tightening::UB ),
            Tightening( 5, 0, Tightening::LB ),   Tightening( 5, 8, Tightening::UB ),

            Tightening( 6, -1, Tightening::LB ),  Tightening( 6, 2, Tightening::UB ),
            Tightening( 7, 0, Tightening::LB ),   Tightening( 7, 2, Tightening::UB ),

            Tightening( 8, -2, Tightening::LB ),  Tightening( 8, 11, Tightening::UB ),
            Tightening( 9, 0, Tightening::LB ),   Tightening( 9, 11, Tightening::UB ),

            Tightening( 10, -3, Tightening::LB ), Tightening( 10, 10, Tightening::UB ),
            Tightening( 11, 0, Tightening::LB ),  Tightening( 11, 10, Tightening::UB ),

            Tightening( 12, 0, Tightening::LB ),  Tightening( 12, 11, Tightening::UB ),
            Tightening( 13, 0, Tightening::LB ),  Tightening( 13, 41, Tightening::UB ),
        } );

        List<Tightening> bounds;
        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );
        TS_ASSERT( boundsEqual( bounds, expectedBounds ) );

        // Change the current bounds
        tableau.setLowerBound( 0, -3 );
        tableau.setUpperBound( 0, 1 );
        tableau.setLowerBound( 1, -1 );
        tableau.setUpperBound( 1, 2 );

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

        // Initialize
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );

        // Perform the tightening pass
        TS_ASSERT_THROWS_NOTHING( nlr.intervalArithmeticBoundPropagation() );

        List<Tightening> expectedBounds2( {
            Tightening( 2, -2, Tightening::LB ),  Tightening( 2, 2, Tightening::UB ),
            Tightening( 3, 0, Tightening::LB ),   Tightening( 3, 2, Tightening::UB ),

            Tightening( 4, -12, Tightening::LB ), Tightening( 4, 5, Tightening::UB ),
            Tightening( 5, 0, Tightening::LB ),   Tightening( 5, 12, Tightening::UB ),

            Tightening( 6, -1, Tightening::LB ),  Tightening( 6, 2, Tightening::UB ),
            Tightening( 7, 0, Tightening::LB ),   Tightening( 7, 2, Tightening::UB ),

            Tightening( 8, -2, Tightening::LB ),  Tightening( 8, 14, Tightening::UB ),
            Tightening( 9, 0, Tightening::LB ),   Tightening( 9, 14, Tightening::UB ),

            Tightening( 10, -2, Tightening::LB ), Tightening( 10, 14, Tightening::UB ),
            Tightening( 11, 0, Tightening::LB ),  Tightening( 11, 14, Tightening::UB ),

            Tightening( 12, 0, Tightening::LB ),  Tightening( 12, 14, Tightening::UB ),
            Tightening( 13, 0, Tightening::LB ),  Tightening( 13, 56, Tightening::UB ),
        } );

        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );
        TS_ASSERT( boundsEqual( bounds, expectedBounds2 ) );
    }

    void test_interval_arithmetic_bound_propagation_sign_constraints()
    {
        NLR::NetworkLevelReasoner nlr;
        populateNetworkWithSign( nlr );

        MockTableau tableau;
        tableau.getBoundManager().initialize( 14 );

        // Initialize the bounds
        tableau.setLowerBound( 0, -1 );
        tableau.setUpperBound( 0, 1 );
        tableau.setLowerBound( 1, -1 );
        tableau.setUpperBound( 1, 1 );

        double large = 1000;
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

        nlr.setTableau( &tableau );

        // Initialize
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );

        // Perform the tightening pass
        TS_ASSERT_THROWS_NOTHING( nlr.intervalArithmeticBoundPropagation() );

        List<Tightening> expectedBounds( {
            Tightening( 2, 0, Tightening::LB ),   Tightening( 2, 2, Tightening::UB ),
            Tightening( 3, 1, Tightening::LB ),   Tightening( 3, 1, Tightening::UB ),

            Tightening( 4, -5, Tightening::LB ),  Tightening( 4, 5, Tightening::UB ),
            Tightening( 5, -1, Tightening::LB ),  Tightening( 5, 1, Tightening::UB ),

            Tightening( 6, -1, Tightening::LB ),  Tightening( 6, 1, Tightening::UB ),
            Tightening( 7, -1, Tightening::LB ),  Tightening( 7, 1, Tightening::UB ),

            Tightening( 8, -1, Tightening::LB ),  Tightening( 8, 3, Tightening::UB ),
            Tightening( 9, -1, Tightening::LB ),  Tightening( 9, 1, Tightening::UB ),

            Tightening( 10, -1, Tightening::LB ), Tightening( 10, 3, Tightening::UB ),
            Tightening( 11, -1, Tightening::LB ), Tightening( 11, 1, Tightening::UB ),

            Tightening( 12, -1, Tightening::LB ), Tightening( 12, 1, Tightening::UB ),
            Tightening( 13, -4, Tightening::LB ), Tightening( 13, 4, Tightening::UB ),
        } );

        List<Tightening> bounds;
        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );
        TS_ASSERT( boundsEqual( bounds, expectedBounds ) );

        // Change the current bounds
        tableau.setLowerBound( 0, 3 );
        tableau.setUpperBound( 0, 4 );
        tableau.setLowerBound( 1, -1 );
        tableau.setUpperBound( 1, 2 );

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

        // Initialize
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );

        // Perform the tightening pass
        TS_ASSERT_THROWS_NOTHING( nlr.intervalArithmeticBoundPropagation() );

        List<Tightening> expectedBounds2( {
            Tightening( 2, 4, Tightening::LB ),  Tightening( 2, 5, Tightening::UB ),
            Tightening( 3, 1, Tightening::LB ),  Tightening( 3, 1, Tightening::UB ),

            Tightening( 4, 0, Tightening::LB ),  Tightening( 4, 11, Tightening::UB ),
            Tightening( 5, 1, Tightening::LB ),  Tightening( 5, 1, Tightening::UB ),

            Tightening( 6, -1, Tightening::LB ), Tightening( 6, 2, Tightening::UB ),
            Tightening( 7, -1, Tightening::LB ), Tightening( 7, 1, Tightening::UB ),

            Tightening( 8, 1, Tightening::LB ),  Tightening( 8, 3, Tightening::UB ),
            Tightening( 9, 1, Tightening::LB ),  Tightening( 9, 1, Tightening::UB ),

            Tightening( 10, 1, Tightening::LB ), Tightening( 10, 3, Tightening::UB ),
            Tightening( 11, 1, Tightening::LB ), Tightening( 11, 1, Tightening::UB ),

            Tightening( 12, 1, Tightening::LB ), Tightening( 12, 1, Tightening::UB ),
            Tightening( 13, 4, Tightening::LB ), Tightening( 13, 4, Tightening::UB ),
        } );

        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );
        TS_ASSERT( boundsEqual( bounds, expectedBounds2 ) );
    }

    void test_interval_arithmetic_bound_propagation_leaky_relu_constraints()
    {
        NLR::NetworkLevelReasoner nlr;
        populateNetworkWithLeakyRelu( nlr );

        MockTableau tableau;
        tableau.getBoundManager().initialize( 14 );

        // Initialize the bounds
        tableau.setLowerBound( 0, -1 );
        tableau.setUpperBound( 0, 2 );
        tableau.setLowerBound( 1, -1 );
        tableau.setUpperBound( 1, 2 );

        double large = 1000;
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

        nlr.setTableau( &tableau );

        // Initialize
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );

        // Perform the tightening pass
        TS_ASSERT_THROWS_NOTHING( nlr.intervalArithmeticBoundPropagation() );

        List<Tightening> expectedBounds( {
            Tightening( 2, 0, Tightening::LB ),      Tightening( 2, 3, Tightening::UB ),
            Tightening( 4, -8, Tightening::LB ),     Tightening( 4, 7, Tightening::UB ),
            Tightening( 6, -1, Tightening::LB ),     Tightening( 6, 2, Tightening::UB ),

            Tightening( 3, 0, Tightening::LB ),      Tightening( 3, 3, Tightening::UB ),
            Tightening( 5, -0.8, Tightening::LB ),   Tightening( 5, 7, Tightening::UB ),
            Tightening( 7, -0.1, Tightening::LB ),   Tightening( 7, 2, Tightening::UB ),

            Tightening( 8, -2.8, Tightening::LB ),   Tightening( 8, 10.1, Tightening::UB ),
            Tightening( 10, -3.8, Tightening::LB ),  Tightening( 10, 9.1, Tightening::UB ),

            Tightening( 9, -0.28, Tightening::LB ),  Tightening( 9, 10.1, Tightening::UB ),
            Tightening( 11, -0.38, Tightening::LB ), Tightening( 11, 9.1, Tightening::UB ),

            Tightening( 12, -0.28, Tightening::LB ), Tightening( 12, 10.1, Tightening::UB ),
            Tightening( 13, -1.42, Tightening::LB ), Tightening( 13, 37.4, Tightening::UB ),
        } );

        List<Tightening> bounds;
        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );
        TS_ASSERT( boundsEqual( bounds, expectedBounds ) );

        // Change the current bounds
        tableau.setLowerBound( 0, -3 );
        tableau.setUpperBound( 0, 1 );
        tableau.setLowerBound( 1, -1 );
        tableau.setUpperBound( 1, 2 );

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

        // Initialize
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );

        // Perform the tightening pass
        TS_ASSERT_THROWS_NOTHING( nlr.intervalArithmeticBoundPropagation() );

        List<Tightening> expectedBounds2( {
            Tightening( 2, -2, Tightening::LB ),     Tightening( 2, 2, Tightening::UB ),
            Tightening( 4, -12, Tightening::LB ),    Tightening( 4, 5, Tightening::UB ),
            Tightening( 6, -1, Tightening::LB ),     Tightening( 6, 2, Tightening::UB ),

            Tightening( 3, -0.2, Tightening::LB ),   Tightening( 3, 2, Tightening::UB ),
            Tightening( 5, -1.2, Tightening::LB ),   Tightening( 5, 5, Tightening::UB ),
            Tightening( 7, -0.1, Tightening::LB ),   Tightening( 7, 2, Tightening::UB ),

            Tightening( 8, -3.4, Tightening::LB ),   Tightening( 8, 7.1, Tightening::UB ),
            Tightening( 10, -3.2, Tightening::LB ),  Tightening( 10, 7.3, Tightening::UB ),

            Tightening( 9, -0.34, Tightening::LB ),  Tightening( 9, 7.1, Tightening::UB ),
            Tightening( 11, -0.32, Tightening::LB ), Tightening( 11, 7.3, Tightening::UB ),

            Tightening( 12, -0.34, Tightening::LB ), Tightening( 12, 7.1, Tightening::UB ),
            Tightening( 13, -1.3, Tightening::LB ),  Tightening( 13, 29, Tightening::UB ),
        } );

        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );
        TS_ASSERT( boundsEqual( bounds, expectedBounds2 ) );
    }

    void test_interval_arithmetic_bound_propagation_round_constraints()
    {
        NLR::NetworkLevelReasoner nlr;
        populateNetworkWithRound( nlr );


        MockTableau tableau;
        tableau.getBoundManager().initialize( 14 );

        // Initialize the bounds
        tableau.setLowerBound( 0, 1.4 );
        tableau.setUpperBound( 0, 1.6 );
        tableau.setLowerBound( 1, -1.4 );
        tableau.setUpperBound( 1, 2.1 );

        double large = 1000;
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

        nlr.setTableau( &tableau );

        // Initialize
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );

        // Perform the tightening pass
        TS_ASSERT_THROWS_NOTHING( nlr.intervalArithmeticBoundPropagation() );

        List<Tightening> expectedBounds( {
            Tightening( 2, 2.4, Tightening::LB ),  Tightening( 2, 2.6, Tightening::UB ),
            Tightening( 4, -3.5, Tightening::LB ), Tightening( 4, 7.4, Tightening::UB ),
            Tightening( 6, -1.4, Tightening::LB ), Tightening( 6, 2.1, Tightening::UB ),

            Tightening( 3, 2, Tightening::LB ),    Tightening( 3, 3, Tightening::UB ),
            Tightening( 5, -4, Tightening::LB ),   Tightening( 5, 7, Tightening::UB ),
            Tightening( 7, -1, Tightening::LB ),   Tightening( 7, 2, Tightening::UB ),

            Tightening( 8, -4, Tightening::LB ),   Tightening( 8, 11, Tightening::UB ),
            Tightening( 10, -7, Tightening::LB ),  Tightening( 10, 8, Tightening::UB ),

            Tightening( 9, -4, Tightening::LB ),   Tightening( 9, 11, Tightening::UB ),
            Tightening( 11, -7, Tightening::LB ),  Tightening( 11, 8, Tightening::UB ),

            Tightening( 12, -4, Tightening::LB ),  Tightening( 12, 11, Tightening::UB ),
            Tightening( 13, -25, Tightening::LB ), Tightening( 13, 35, Tightening::UB ),
        } );

        List<Tightening> bounds;
        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );
        TS_ASSERT( boundsEqual( bounds, expectedBounds ) );

        // Change the current bounds
        tableau.setLowerBound( 0, -3.1 );
        tableau.setUpperBound( 0, 1.6 );
        tableau.setLowerBound( 1, 1.4 );
        tableau.setUpperBound( 1, 2.1 );

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

        // Initialize
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );

        // Perform the tightening pass
        TS_ASSERT_THROWS_NOTHING( nlr.intervalArithmeticBoundPropagation() );

        List<Tightening> expectedBounds2( {
            Tightening( 2, -2.1, Tightening::LB ),  Tightening( 2, 2.6, Tightening::UB ),
            Tightening( 4, -12.5, Tightening::LB ), Tightening( 4, -1, Tightening::UB ),
            Tightening( 6, 1.4, Tightening::LB ),   Tightening( 6, 2.1, Tightening::UB ),

            Tightening( 3, -2, Tightening::LB ),    Tightening( 3, 3, Tightening::UB ),
            Tightening( 5, -12, Tightening::LB ),   Tightening( 5, -1, Tightening::UB ),
            Tightening( 7, 1, Tightening::LB ),     Tightening( 7, 2, Tightening::UB ),

            Tightening( 8, -16, Tightening::LB ),   Tightening( 8, 1, Tightening::UB ),
            Tightening( 10, -15, Tightening::LB ),  Tightening( 10, 2, Tightening::UB ),

            Tightening( 9, -16, Tightening::LB ),   Tightening( 9, 1, Tightening::UB ),
            Tightening( 11, -15, Tightening::LB ),  Tightening( 11, 2, Tightening::UB ),

            Tightening( 12, -16, Tightening::LB ),  Tightening( 12, 1, Tightening::UB ),
            Tightening( 13, -61, Tightening::LB ),  Tightening( 13, 7, Tightening::UB ),
        } );

        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );
        TS_ASSERT( boundsEqual( bounds, expectedBounds2 ) );
    }

    void test_interval_arithmetic_bound_propagation_sigmoid_constraints()
    {
        NLR::NetworkLevelReasoner nlr;
        populateNetworkWithSigmoids( nlr );


        MockTableau tableau;
        tableau.getBoundManager().initialize( 14 );

        // Initialize the bounds
        tableau.setLowerBound( 0, -1 );
        tableau.setUpperBound( 0, 1 );
        tableau.setLowerBound( 1, -1 );
        tableau.setUpperBound( 1, 1 );

        double large = 1000;
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

        nlr.setTableau( &tableau );

        // Initialize
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );

        // Perform the tightening pass
        TS_ASSERT_THROWS_NOTHING( nlr.intervalArithmeticBoundPropagation() );

        List<Tightening> expectedBounds( {
            Tightening( 2, 0, Tightening::LB ),       Tightening( 2, 2, Tightening::UB ),
            Tightening( 4, -5, Tightening::LB ),      Tightening( 4, 5, Tightening::UB ),
            Tightening( 6, -1, Tightening::LB ),      Tightening( 6, 1, Tightening::UB ),

            Tightening( 3, 0.5000, Tightening::LB ),  Tightening( 3, 0.8808, Tightening::UB ),
            Tightening( 5, 0.0067, Tightening::LB ),  Tightening( 5, 0.9933, Tightening::UB ),
            Tightening( 7, 0.2689, Tightening::LB ),  Tightening( 7, 0.7311, Tightening::UB ),

            Tightening( 8, -0.2244, Tightening::LB ), Tightening( 8, 1.6052, Tightening::UB ),
            Tightening( 10, 0.3948, Tightening::LB ), Tightening( 10, 2.2244, Tightening::UB ),

            Tightening( 9, 0.4441, Tightening::LB ),  Tightening( 9, 0.8327, Tightening::UB ),
            Tightening( 11, 0.5974, Tightening::LB ), Tightening( 11, 0.9024, Tightening::UB ),

            Tightening( 12, 0.4441, Tightening::LB ), Tightening( 12, 0.8327, Tightening::UB ),
            Tightening( 13, 2.2364, Tightening::LB ), Tightening( 13, 3.5399, Tightening::UB ),
        } );

        List<Tightening> bounds;
        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );
        TS_ASSERT( boundsEqual( bounds, expectedBounds ) );

        // Change the current bounds
        tableau.setLowerBound( 0, -3 );
        tableau.setUpperBound( 0, 1 );
        tableau.setLowerBound( 1, -1 );
        tableau.setUpperBound( 1, 2 );

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

        // Initialize
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );

        // Perform the tightening pass
        TS_ASSERT_THROWS_NOTHING( nlr.intervalArithmeticBoundPropagation() );

        List<Tightening> expectedBounds2( {
            Tightening( 2, -2, Tightening::LB ),      Tightening( 2, 2, Tightening::UB ),
            Tightening( 4, -12, Tightening::LB ),     Tightening( 4, 5, Tightening::UB ),
            Tightening( 6, -1, Tightening::LB ),      Tightening( 6, 2, Tightening::UB ),

            Tightening( 3, 0.1192, Tightening::LB ),  Tightening( 3, 0.8808, Tightening::UB ),
            Tightening( 5, 0, Tightening::LB ),       Tightening( 5, 0.9933, Tightening::UB ),
            Tightening( 7, 0.2689, Tightening::LB ),  Tightening( 7, 0.8808, Tightening::UB ),

            Tightening( 8, -0.7616, Tightening::LB ), Tightening( 8, 1.6052, Tightening::UB ),
            Tightening( 10, 0.2384, Tightening::LB ), Tightening( 10, 2.6052, Tightening::UB ),

            Tightening( 9, 0.3183, Tightening::LB ),  Tightening( 9, 0.8327, Tightening::UB ),
            Tightening( 11, 0.5593, Tightening::LB ), Tightening( 11, 0.9312, Tightening::UB ),

            Tightening( 12, 0.3183, Tightening::LB ), Tightening( 12, 0.8327, Tightening::UB ),
            Tightening( 13, 1.9963, Tightening::LB ), Tightening( 13, 3.6263, Tightening::UB ),
        } );

        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );
        TS_ASSERT( boundsEqual( bounds, expectedBounds2 ) );
    }

    void test_interval_arithmetic_bound_propagation_max_constraints()
    {
        NLR::NetworkLevelReasoner nlr;
        populateNetworkWithMax( nlr );


        MockTableau tableau;
        tableau.getBoundManager().initialize( 12 );

        // Initialize the bounds
        tableau.setLowerBound( 0, -1 );
        tableau.setUpperBound( 0, 1 );
        tableau.setLowerBound( 1, -1 );
        tableau.setUpperBound( 1, 1 );

        double large = 1000;
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

        nlr.setTableau( &tableau );

        // Initialize
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );

        // Perform the tightening pass
        TS_ASSERT_THROWS_NOTHING( nlr.intervalArithmeticBoundPropagation() );

        List<Tightening> expectedBounds( {
            Tightening( 2, 0, Tightening::LB ),    Tightening( 2, 2, Tightening::UB ),
            Tightening( 3, -5, Tightening::LB ),   Tightening( 3, 5, Tightening::UB ),
            Tightening( 4, -3, Tightening::LB ),   Tightening( 4, 3, Tightening::UB ),
            Tightening( 5, -3, Tightening::LB ),   Tightening( 5, 3, Tightening::UB ),

            Tightening( 6, 0, Tightening::LB ),    Tightening( 6, 5, Tightening::UB ),
            Tightening( 7, -3, Tightening::LB ),   Tightening( 7, 3, Tightening::UB ),

            Tightening( 8, -1, Tightening::LB ),   Tightening( 8, 10, Tightening::UB ),
            Tightening( 9, -8, Tightening::LB ),   Tightening( 9, 3, Tightening::UB ),

            Tightening( 10, -1, Tightening::LB ),  Tightening( 10, 10, Tightening::UB ),

            Tightening( 11, -10, Tightening::LB ), Tightening( 11, 1, Tightening::UB ),
        } );

        List<Tightening> bounds;
        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );

        TS_ASSERT( boundsEqual( bounds, expectedBounds ) );

        // Change the current bounds
        tableau.setLowerBound( 0, -3 );
        tableau.setUpperBound( 0, 1 );
        tableau.setLowerBound( 1, -1 );
        tableau.setUpperBound( 1, 2 );

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

        // Initialize
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );

        // Perform the tightening pass
        TS_ASSERT_THROWS_NOTHING( nlr.intervalArithmeticBoundPropagation() );

        List<Tightening> expectedBounds2( {
            Tightening( 2, -2, Tightening::LB ),   Tightening( 2, 2, Tightening::UB ),
            Tightening( 3, -8, Tightening::LB ),   Tightening( 3, 9, Tightening::UB ),
            Tightening( 4, -5, Tightening::LB ),   Tightening( 4, 5, Tightening::UB ),
            Tightening( 5, -6, Tightening::LB ),   Tightening( 5, 3, Tightening::UB ),

            Tightening( 6, -2, Tightening::LB ),   Tightening( 6, 9, Tightening::UB ),
            Tightening( 7, -5, Tightening::LB ),   Tightening( 7, 5, Tightening::UB ),

            Tightening( 8, -5, Tightening::LB ),   Tightening( 8, 16, Tightening::UB ),
            Tightening( 9, -14, Tightening::LB ),  Tightening( 9, 7, Tightening::UB ),

            Tightening( 10, -5, Tightening::LB ),  Tightening( 10, 16, Tightening::UB ),

            Tightening( 11, -16, Tightening::LB ), Tightening( 11, 5, Tightening::UB ),
        } );

        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );
        TS_ASSERT( boundsEqual( bounds, expectedBounds2 ) );
    }

    void test_interval_arithmetic_bound_propagation_softmax_constraints()
    {
        NLR::NetworkLevelReasoner nlr;
        populateNetworkWithSoftmax( nlr );


        MockTableau tableau;
        tableau.getBoundManager().initialize( 14 );

        // Initialize the bounds
        tableau.setLowerBound( 0, -1 );
        tableau.setUpperBound( 0, 1 );
        tableau.setLowerBound( 1, -1 );
        tableau.setUpperBound( 1, 1 );

        double large = 1000;
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

        nlr.setTableau( &tableau );

        // Initialize
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );

        // Perform the tightening pass
        TS_ASSERT_THROWS_NOTHING( nlr.intervalArithmeticBoundPropagation() );

        List<Tightening> expectedBounds( {
            Tightening( 2, 0, Tightening::LB ),       Tightening( 2, 2, Tightening::UB ),
            Tightening( 4, -5, Tightening::LB ),      Tightening( 4, 5, Tightening::UB ),
            Tightening( 6, -1, Tightening::LB ),      Tightening( 6, 1, Tightening::UB ),

            Tightening( 3, 0.0066, Tightening::LB ),  Tightening( 3, 0.9517, Tightening::UB ),
            Tightening( 5, 0.0007, Tightening::LB ),  Tightening( 5, 0.9909, Tightening::UB ),
            Tightening( 7, 0.0024, Tightening::LB ),  Tightening( 7, 0.7297, Tightening::UB ),

            Tightening( 8, -0.7225, Tightening::LB ), Tightening( 8, 1.9403, Tightening::UB ),
            Tightening( 10, 0.3192, Tightening::LB ), Tightening( 10, 2.9819, Tightening::UB ),

            Tightening( 9, 0.0240, Tightening::LB ),  Tightening( 9, 0.8349, Tightening::UB ),
            Tightening( 11, 0.1651, Tightening::LB ), Tightening( 11, 0.9759, Tightening::UB ),

            Tightening( 12, 0.0240, Tightening::LB ), Tightening( 12, 0.8349, Tightening::UB ),
            Tightening( 13, 0.5192, Tightening::LB ), Tightening( 13, 3.7629, Tightening::UB ),
        } );

        List<Tightening> bounds;
        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );
        TS_ASSERT( boundsEqual( bounds, expectedBounds ) );

        // Change the current bounds
        tableau.setLowerBound( 0, -3 );
        tableau.setUpperBound( 0, 1 );
        tableau.setLowerBound( 1, -1 );
        tableau.setUpperBound( 1, 2 );

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

        // Initialize
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );

        // Perform the tightening pass
        TS_ASSERT_THROWS_NOTHING( nlr.intervalArithmeticBoundPropagation() );

        List<Tightening> expectedBounds2( {
            Tightening( 2, -2, Tightening::LB ),      Tightening( 2, 2, Tightening::UB ),
            Tightening( 4, -12, Tightening::LB ),     Tightening( 4, 5, Tightening::UB ),
            Tightening( 6, -1, Tightening::LB ),      Tightening( 6, 2, Tightening::UB ),

            Tightening( 3, 0.0009, Tightening::LB ),  Tightening( 3, 0.9526, Tightening::UB ),
            Tightening( 5, 0, Tightening::LB ),       Tightening( 5, 0.9966, Tightening::UB ),
            Tightening( 7, 0.0024, Tightening::LB ),  Tightening( 7, 0.9820, Tightening::UB ),

            Tightening( 8, -0.9811, Tightening::LB ), Tightening( 8, 1.9468, Tightening::UB ),
            Tightening( 10, 0.0654, Tightening::LB ), Tightening( 10, 2.9933, Tightening::UB ),

            Tightening( 9, 0.0184, Tightening::LB ),  Tightening( 9, 0.8678, Tightening::UB ),
            Tightening( 11, 0.1322, Tightening::LB ), Tightening( 11, 0.9816, Tightening::UB ),

            Tightening( 12, 0.0184, Tightening::LB ), Tightening( 12, 0.8678, Tightening::UB ),
            Tightening( 13, 0.4151, Tightening::LB ), Tightening( 13, 3.8125, Tightening::UB ),
        } );

        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );
        TS_ASSERT( boundsEqual( bounds, expectedBounds2 ) );
    }

    void test_interval_arithmetic_bound_propagation_bilinear_constraints()
    {
        NLR::NetworkLevelReasoner nlr;
        populateNetworkWithBilinear( nlr );


        MockTableau tableau;
        tableau.getBoundManager().initialize( 12 );

        // Initialize the bounds
        tableau.setLowerBound( 0, -0.1 );
        tableau.setUpperBound( 0, 0.1 );
        tableau.setLowerBound( 1, -0.1 );
        tableau.setUpperBound( 1, 0.1 );

        double large = 1000;
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

        nlr.setTableau( &tableau );

        // Initialize
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );

        // Perform the tightening pass
        TS_ASSERT_THROWS_NOTHING( nlr.intervalArithmeticBoundPropagation() );

        List<Tightening> expectedBounds( {
            Tightening( 2, 0.9, Tightening::LB ),      Tightening( 2, 1.1, Tightening::UB ),
            Tightening( 3, -0.5, Tightening::LB ),     Tightening( 3, 0.5, Tightening::UB ),
            Tightening( 4, -0.3, Tightening::LB ),     Tightening( 4, 0.3, Tightening::UB ),
            Tightening( 5, -0.3, Tightening::LB ),     Tightening( 5, 0.3, Tightening::UB ),

            Tightening( 6, -0.55, Tightening::LB ),    Tightening( 6, 0.55, Tightening::UB ),
            Tightening( 7, -0.09, Tightening::LB ),    Tightening( 7, 0.09, Tightening::UB ),

            Tightening( 8, 1.36, Tightening::LB ),     Tightening( 8, 2.64, Tightening::UB ),
            Tightening( 9, -0.64, Tightening::LB ),    Tightening( 9, 0.64, Tightening::UB ),

            Tightening( 10, -1.6896, Tightening::LB ), Tightening( 10, 1.6896, Tightening::UB ),

            Tightening( 11, -1.6896, Tightening::LB ), Tightening( 11, 1.6896, Tightening::UB ),
        } );

        List<Tightening> bounds;
        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );

        TS_ASSERT( boundsEqual( bounds, expectedBounds ) );

        // Change the current bounds
        tableau.setLowerBound( 0, -0.3 );
        tableau.setUpperBound( 0, 0.1 );
        tableau.setLowerBound( 1, -0.1 );
        tableau.setUpperBound( 1, 0.2 );

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

        // Initialize
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );

        // Perform the tightening pass
        TS_ASSERT_THROWS_NOTHING( nlr.intervalArithmeticBoundPropagation() );

        List<Tightening> expectedBounds2( {
            Tightening( 2, 0.7, Tightening::LB ),      Tightening( 2, 1.1, Tightening::UB ),
            Tightening( 3, -0.8, Tightening::LB ),     Tightening( 3, 0.9, Tightening::UB ),
            Tightening( 4, -0.5, Tightening::LB ),     Tightening( 4, 0.5, Tightening::UB ),
            Tightening( 5, -0.6, Tightening::LB ),     Tightening( 5, 0.3, Tightening::UB ),

            Tightening( 6, -0.88, Tightening::LB ),    Tightening( 6, 0.99, Tightening::UB ),
            Tightening( 7, -0.3, Tightening::LB ),     Tightening( 7, 0.3, Tightening::UB ),

            Tightening( 8, 0.82, Tightening::LB ),     Tightening( 8, 3.29, Tightening::UB ),
            Tightening( 9, -1.29, Tightening::LB ),    Tightening( 9, 1.18, Tightening::UB ),

            Tightening( 10, -4.2441, Tightening::LB ), Tightening( 10, 3.8822, Tightening::UB ),

            Tightening( 11, -3.8822, Tightening::LB ), Tightening( 11, 4.2441, Tightening::UB ),
        } );

        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );
        TS_ASSERT( boundsEqual( bounds, expectedBounds2 ) );
    }

    void test_interval_arithmetic_bound_propagation_abs_and_relu_constraints()
    {
        NLR::NetworkLevelReasoner nlr;
        populateNetworkWithAbsAndRelu( nlr );

        MockTableau tableau;
        tableau.getBoundManager().initialize( 14 );

        // Initialize the bounds
        tableau.setLowerBound( 0, -1 );
        tableau.setUpperBound( 0, 1 );
        tableau.setLowerBound( 1, -1 );
        tableau.setUpperBound( 1, 1 );

        double large = 1000;
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

        nlr.setTableau( &tableau );

        // Initialize
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );

        // Perform the tightening pass
        TS_ASSERT_THROWS_NOTHING( nlr.intervalArithmeticBoundPropagation() );

        List<Tightening> expectedBounds( {
            Tightening( 2, 0, Tightening::LB ),   Tightening( 2, 2, Tightening::UB ),
            Tightening( 3, 0, Tightening::LB ),   Tightening( 3, 2, Tightening::UB ),

            Tightening( 4, -5, Tightening::LB ),  Tightening( 4, 5, Tightening::UB ),
            Tightening( 5, 0, Tightening::LB ),   Tightening( 5, 5, Tightening::UB ),

            Tightening( 6, -1, Tightening::LB ),  Tightening( 6, 1, Tightening::UB ),
            Tightening( 7, 0, Tightening::LB ),   Tightening( 7, 1, Tightening::UB ),

            Tightening( 8, -1, Tightening::LB ),  Tightening( 8, 7, Tightening::UB ),
            Tightening( 9, 0, Tightening::LB ),   Tightening( 9, 7, Tightening::UB ),

            Tightening( 10, -5, Tightening::LB ), Tightening( 10, 7, Tightening::UB ),
            Tightening( 11, 0, Tightening::LB ),  Tightening( 11, 7, Tightening::UB ),

            Tightening( 12, 0, Tightening::LB ),  Tightening( 12, 7, Tightening::UB ),
            Tightening( 13, 0, Tightening::LB ),  Tightening( 13, 28, Tightening::UB ),
        } );

        List<Tightening> bounds;
        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );
        TS_ASSERT( boundsEqual( bounds, expectedBounds ) );

        // Change the current bounds
        tableau.setLowerBound( 0, -3 );
        tableau.setUpperBound( 0, 1 );
        tableau.setLowerBound( 1, -1 );
        tableau.setUpperBound( 1, 2 );

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

        // Initialize
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );

        // Perform the tightening pass
        TS_ASSERT_THROWS_NOTHING( nlr.intervalArithmeticBoundPropagation() );

        List<Tightening> expectedBounds2( {
            Tightening( 2, -2, Tightening::LB ),   Tightening( 2, 2, Tightening::UB ),
            Tightening( 3, 0, Tightening::LB ),    Tightening( 3, 2, Tightening::UB ),

            Tightening( 4, -12, Tightening::LB ),  Tightening( 4, 5, Tightening::UB ),
            Tightening( 5, 0, Tightening::LB ),    Tightening( 5, 12, Tightening::UB ),

            Tightening( 6, -1, Tightening::LB ),   Tightening( 6, 2, Tightening::UB ),
            Tightening( 7, 0, Tightening::LB ),    Tightening( 7, 2, Tightening::UB ),

            Tightening( 8, -2, Tightening::LB ),   Tightening( 8, 14, Tightening::UB ),
            Tightening( 9, 0, Tightening::LB ),    Tightening( 9, 14, Tightening::UB ),

            Tightening( 10, -10, Tightening::LB ), Tightening( 10, 14, Tightening::UB ),
            Tightening( 11, 0, Tightening::LB ),   Tightening( 11, 14, Tightening::UB ),

            Tightening( 12, 0, Tightening::LB ),   Tightening( 12, 14, Tightening::UB ),
            Tightening( 13, 0, Tightening::LB ),   Tightening( 13, 56, Tightening::UB ),
        } );

        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );
        TS_ASSERT( boundsEqual( bounds, expectedBounds2 ) );
    }

    void test_interval_arithmetic_bound_propagation_round_and_sign_constraints()
    {
        NLR::NetworkLevelReasoner nlr;
        populateNetworkWithRoundAndSign( nlr );

        MockTableau tableau;
        tableau.getBoundManager().initialize( 14 );

        // Initialize the bounds
        tableau.setLowerBound( 0, 1.4 );
        tableau.setUpperBound( 0, 1.6 );
        tableau.setLowerBound( 1, -1.4 );
        tableau.setUpperBound( 1, 2.1 );

        double large = 1000;
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

        nlr.setTableau( &tableau );

        // Initialize
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );

        // Perform the tightening pass
        TS_ASSERT_THROWS_NOTHING( nlr.intervalArithmeticBoundPropagation() );

        List<Tightening> expectedBounds( {
            Tightening( 2, 2.4, Tightening::LB ),  Tightening( 2, 2.6, Tightening::UB ),
            Tightening( 4, -3.5, Tightening::LB ), Tightening( 4, 7.4, Tightening::UB ),
            Tightening( 6, -1.4, Tightening::LB ), Tightening( 6, 2.1, Tightening::UB ),

            Tightening( 3, 2, Tightening::LB ),    Tightening( 3, 3, Tightening::UB ),
            Tightening( 5, -4, Tightening::LB ),   Tightening( 5, 7, Tightening::UB ),
            Tightening( 7, -1, Tightening::LB ),   Tightening( 7, 2, Tightening::UB ),

            Tightening( 8, -4, Tightening::LB ),   Tightening( 8, 11, Tightening::UB ),
            Tightening( 10, -7, Tightening::LB ),  Tightening( 10, 8, Tightening::UB ),

            Tightening( 9, -1, Tightening::LB ),   Tightening( 9, 1, Tightening::UB ),
            Tightening( 11, -1, Tightening::LB ),  Tightening( 11, 1, Tightening::UB ),

            Tightening( 12, -1, Tightening::LB ),  Tightening( 12, 1, Tightening::UB ),
            Tightening( 13, -4, Tightening::LB ),  Tightening( 13, 4, Tightening::UB ),
        } );

        List<Tightening> bounds;
        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );

        TS_ASSERT( boundsEqual( bounds, expectedBounds ) );

        // Change the current bounds
        tableau.setLowerBound( 0, -3.1 );
        tableau.setUpperBound( 0, 1.6 );
        tableau.setLowerBound( 1, 1.4 );
        tableau.setUpperBound( 1, 2.1 );

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

        // Initialize
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );

        // Perform the tightening pass
        TS_ASSERT_THROWS_NOTHING( nlr.intervalArithmeticBoundPropagation() );

        List<Tightening> expectedBounds2( {
            Tightening( 2, -2.1, Tightening::LB ),  Tightening( 2, 2.6, Tightening::UB ),
            Tightening( 4, -12.5, Tightening::LB ), Tightening( 4, -1, Tightening::UB ),
            Tightening( 6, 1.4, Tightening::LB ),   Tightening( 6, 2.1, Tightening::UB ),

            Tightening( 3, -2, Tightening::LB ),    Tightening( 3, 3, Tightening::UB ),
            Tightening( 5, -12, Tightening::LB ),   Tightening( 5, -1, Tightening::UB ),
            Tightening( 7, 1, Tightening::LB ),     Tightening( 7, 2, Tightening::UB ),

            Tightening( 8, -16, Tightening::LB ),   Tightening( 8, 1, Tightening::UB ),
            Tightening( 10, -15, Tightening::LB ),  Tightening( 10, 2, Tightening::UB ),

            Tightening( 9, -1, Tightening::LB ),    Tightening( 9, 1, Tightening::UB ),
            Tightening( 11, -1, Tightening::LB ),   Tightening( 11, 1, Tightening::UB ),

            Tightening( 12, -1, Tightening::LB ),   Tightening( 12, 1, Tightening::UB ),
            Tightening( 13, -4, Tightening::LB ),   Tightening( 13, 4, Tightening::UB ),
        } );

        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );
        TS_ASSERT( boundsEqual( bounds, expectedBounds2 ) );
    }

    void test_interval_arithmetic_bound_propagation_leaky_relu_and_sigmoid_constraints()
    {
        NLR::NetworkLevelReasoner nlr;
        populateNetworkWithLeakyReluAndSigmoid( nlr );

        MockTableau tableau;
        tableau.getBoundManager().initialize( 14 );

        // Initialize the bounds
        tableau.setLowerBound( 0, -1 );
        tableau.setUpperBound( 0, 2 );
        tableau.setLowerBound( 1, -1 );
        tableau.setUpperBound( 1, 2 );

        double large = 1000;
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

        nlr.setTableau( &tableau );

        // Initialize
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );

        // Perform the tightening pass
        TS_ASSERT_THROWS_NOTHING( nlr.intervalArithmeticBoundPropagation() );

        List<Tightening> expectedBounds( {
            Tightening( 2, 0, Tightening::LB ),       Tightening( 2, 3, Tightening::UB ),
            Tightening( 4, -8, Tightening::LB ),      Tightening( 4, 7, Tightening::UB ),
            Tightening( 6, -1, Tightening::LB ),      Tightening( 6, 2, Tightening::UB ),

            Tightening( 3, 0, Tightening::LB ),       Tightening( 3, 3, Tightening::UB ),
            Tightening( 5, -0.8, Tightening::LB ),    Tightening( 5, 7, Tightening::UB ),
            Tightening( 7, -0.1, Tightening::LB ),    Tightening( 7, 2, Tightening::UB ),

            Tightening( 8, -2.8, Tightening::LB ),    Tightening( 8, 10.1, Tightening::UB ),
            Tightening( 10, -3.8, Tightening::LB ),   Tightening( 10, 9.1, Tightening::UB ),

            Tightening( 9, 0.0573, Tightening::LB ),  Tightening( 9, 0.9999, Tightening::UB ),
            Tightening( 11, 0.0219, Tightening::LB ), Tightening( 11, 0.9999, Tightening::UB ),

            Tightening( 12, 0.0573, Tightening::LB ), Tightening( 12, 0.9999, Tightening::UB ),
            Tightening( 13, 0.1229, Tightening::LB ), Tightening( 13, 3.9996, Tightening::UB ),
        } );

        List<Tightening> bounds;
        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );

        TS_ASSERT( boundsEqual( bounds, expectedBounds ) );

        // Change the current bounds
        tableau.setLowerBound( 0, -3 );
        tableau.setUpperBound( 0, 1 );
        tableau.setLowerBound( 1, -1 );
        tableau.setUpperBound( 1, 2 );

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

        // Initialize
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );

        // Perform the tightening pass
        TS_ASSERT_THROWS_NOTHING( nlr.intervalArithmeticBoundPropagation() );

        List<Tightening> expectedBounds2( {
            Tightening( 2, -2, Tightening::LB ),      Tightening( 2, 2, Tightening::UB ),
            Tightening( 4, -12, Tightening::LB ),     Tightening( 4, 5, Tightening::UB ),
            Tightening( 6, -1, Tightening::LB ),      Tightening( 6, 2, Tightening::UB ),

            Tightening( 3, -0.2, Tightening::LB ),    Tightening( 3, 2, Tightening::UB ),
            Tightening( 5, -1.2, Tightening::LB ),    Tightening( 5, 5, Tightening::UB ),
            Tightening( 7, -0.1, Tightening::LB ),    Tightening( 7, 2, Tightening::UB ),

            Tightening( 8, -3.4, Tightening::LB ),    Tightening( 8, 7.1, Tightening::UB ),
            Tightening( 10, -3.2, Tightening::LB ),   Tightening( 10, 7.3, Tightening::UB ),

            Tightening( 9, 0.0323, Tightening::LB ),  Tightening( 9, 0.9992, Tightening::UB ),
            Tightening( 11, 0.0392, Tightening::LB ), Tightening( 11, 0.9993, Tightening::UB ),

            Tightening( 12, 0.0323, Tightening::LB ), Tightening( 12, 0.9992, Tightening::UB ),
            Tightening( 13, 0.1498, Tightening::LB ), Tightening( 13, 3.9972, Tightening::UB ),
        } );

        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );
        TS_ASSERT( boundsEqual( bounds, expectedBounds2 ) );
    }

    void test_interval_arithmetic_bound_propagation_softmax_and_max_constraints()
    {
        NLR::NetworkLevelReasoner nlr;
        populateNetworkWithSoftmaxAndMax( nlr );


        MockTableau tableau;
        tableau.getBoundManager().initialize( 12 );

        // Initialize the bounds
        tableau.setLowerBound( 0, -1 );
        tableau.setUpperBound( 0, 1 );
        tableau.setLowerBound( 1, -1 );
        tableau.setUpperBound( 1, 1 );

        double large = 1000;
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

        nlr.setTableau( &tableau );

        // Initialize
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );

        // Perform the tightening pass
        TS_ASSERT_THROWS_NOTHING( nlr.intervalArithmeticBoundPropagation() );

        List<Tightening> expectedBounds( { Tightening( 2, 0, Tightening::LB ),
                                           Tightening( 2, 2, Tightening::UB ),
                                           Tightening( 4, -5, Tightening::LB ),
                                           Tightening( 4, 5, Tightening::UB ),
                                           Tightening( 6, -1, Tightening::LB ),
                                           Tightening( 6, 1, Tightening::UB ),

                                           Tightening( 3, 0.0066, Tightening::LB ),
                                           Tightening( 3, 0.9517, Tightening::UB ),
                                           Tightening( 5, 0.0007, Tightening::LB ),
                                           Tightening( 5, 0.9909, Tightening::UB ),
                                           Tightening( 7, 0.0024, Tightening::LB ),
                                           Tightening( 7, 0.7297, Tightening::UB ),

                                           Tightening( 8, -0.7225, Tightening::LB ),
                                           Tightening( 8, 1.9403, Tightening::UB ),
                                           Tightening( 9, 0.3192, Tightening::LB ),
                                           Tightening( 9, 2.9819, Tightening::UB ),

                                           Tightening( 10, 0.3192, Tightening::LB ),
                                           Tightening( 10, 2.9819, Tightening::UB ),

                                           Tightening( 11, -2.9819, Tightening::LB ),
                                           Tightening( 11, -0.3192, Tightening::UB ) } );

        List<Tightening> bounds;
        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );

        TS_ASSERT( boundsEqual( bounds, expectedBounds ) );

        // Change the current bounds
        tableau.setLowerBound( 0, -3 );
        tableau.setUpperBound( 0, 1 );
        tableau.setLowerBound( 1, -1 );
        tableau.setUpperBound( 1, 2 );

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

        // Initialize
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );

        // Perform the tightening pass
        TS_ASSERT_THROWS_NOTHING( nlr.intervalArithmeticBoundPropagation() );

        List<Tightening> expectedBounds2( { Tightening( 2, -2, Tightening::LB ),
                                            Tightening( 2, 2, Tightening::UB ),
                                            Tightening( 4, -12, Tightening::LB ),
                                            Tightening( 4, 5, Tightening::UB ),
                                            Tightening( 6, -1, Tightening::LB ),
                                            Tightening( 6, 2, Tightening::UB ),

                                            Tightening( 3, 0.0009, Tightening::LB ),
                                            Tightening( 3, 0.9526, Tightening::UB ),
                                            Tightening( 5, 0, Tightening::LB ),
                                            Tightening( 5, 0.9966, Tightening::UB ),
                                            Tightening( 7, 0.0024, Tightening::LB ),
                                            Tightening( 7, 0.9820, Tightening::UB ),

                                            Tightening( 8, -0.9811, Tightening::LB ),
                                            Tightening( 8, 1.9468, Tightening::UB ),
                                            Tightening( 9, 0.0654, Tightening::LB ),
                                            Tightening( 9, 2.9933, Tightening::UB ),

                                            Tightening( 10, 0.0654, Tightening::LB ),
                                            Tightening( 10, 2.9933, Tightening::UB ),

                                            Tightening( 11, -2.9933, Tightening::LB ),
                                            Tightening( 11, -0.0654, Tightening::UB ) } );

        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );

        TS_ASSERT( boundsEqual( bounds, expectedBounds2 ) );
    }

    void test_interval_arithmetic_bound_propagation_relu_and_bilinear_constraints()
    {
        NLR::NetworkLevelReasoner nlr;
        populateNetworkWithReluAndBilinear( nlr );

        MockTableau tableau;
        tableau.getBoundManager().initialize( 12 );

        // Initialize the bounds
        tableau.setLowerBound( 0, -1 );
        tableau.setUpperBound( 0, 1 );
        tableau.setLowerBound( 1, -1 );
        tableau.setUpperBound( 1, 1 );

        double large = 1000;
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

        nlr.setTableau( &tableau );

        // Initialize
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );

        // Perform the tightening pass
        TS_ASSERT_THROWS_NOTHING( nlr.intervalArithmeticBoundPropagation() );

        List<Tightening> expectedBounds( {
            Tightening( 2, 0, Tightening::LB ),    Tightening( 2, 2, Tightening::UB ),
            Tightening( 4, -5, Tightening::LB ),   Tightening( 4, 5, Tightening::UB ),
            Tightening( 6, -1, Tightening::LB ),   Tightening( 6, 1, Tightening::UB ),

            Tightening( 3, 0, Tightening::LB ),    Tightening( 3, 2, Tightening::UB ),
            Tightening( 5, 0, Tightening::LB ),    Tightening( 5, 5, Tightening::UB ),
            Tightening( 7, 0, Tightening::LB ),    Tightening( 7, 1, Tightening::UB ),

            Tightening( 8, -1, Tightening::LB ),   Tightening( 8, 7, Tightening::UB ),
            Tightening( 9, -1, Tightening::LB ),   Tightening( 9, 7, Tightening::UB ),

            Tightening( 10, -7, Tightening::LB ),  Tightening( 10, 49, Tightening::UB ),

            Tightening( 11, -49, Tightening::LB ), Tightening( 11, 7, Tightening::UB ),
        } );

        List<Tightening> bounds;
        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );

        TS_ASSERT( boundsEqual( bounds, expectedBounds ) );

        // Change the current bounds
        tableau.setLowerBound( 0, -3 );
        tableau.setUpperBound( 0, 1 );
        tableau.setLowerBound( 1, -1 );
        tableau.setUpperBound( 1, 2 );

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

        // Initialize
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );

        // Perform the tightening pass
        TS_ASSERT_THROWS_NOTHING( nlr.intervalArithmeticBoundPropagation() );

        List<Tightening> expectedBounds2( {
            Tightening( 2, -2, Tightening::LB ),   Tightening( 2, 2, Tightening::UB ),
            Tightening( 4, -12, Tightening::LB ),  Tightening( 4, 5, Tightening::UB ),
            Tightening( 6, -1, Tightening::LB ),   Tightening( 6, 2, Tightening::UB ),

            Tightening( 3, 0, Tightening::LB ),    Tightening( 3, 2, Tightening::UB ),
            Tightening( 5, 0, Tightening::LB ),    Tightening( 5, 5, Tightening::UB ),
            Tightening( 7, 0, Tightening::LB ),    Tightening( 7, 2, Tightening::UB ),

            Tightening( 8, -2, Tightening::LB ),   Tightening( 8, 7, Tightening::UB ),
            Tightening( 9, -2, Tightening::LB ),   Tightening( 9, 7, Tightening::UB ),

            Tightening( 10, -14, Tightening::LB ), Tightening( 10, 49, Tightening::UB ),

            Tightening( 11, -49, Tightening::LB ), Tightening( 11, 14, Tightening::UB ),
        } );

        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );

        TS_ASSERT( boundsEqual( bounds, expectedBounds2 ) );
    }

    void test_sbt_relus_all_active()
    {
        Options::get()->setString( Options::SYMBOLIC_BOUND_TIGHTENING_TYPE, "sbt" );

        NLR::NetworkLevelReasoner nlr;
        MockTableau tableau;
        nlr.setTableau( &tableau );
        populateNetworkSBTRelu( nlr, tableau );

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

          x3.lb =  x0 + x1   : [5, 11]
          x3.ub =  x0 + x1   : [5, 11]

          Both ReLUs active, bound survive through activations:

          x4.lb = 2x0 + 3x1   : [11, 27]
          x4.ub = 2x0 + 3x1   : [11, 27]

          x5.lb =  x0 + x1   : [5, 11]
          x5.ub =  x0 + x1   : [5, 11]

          Layer 2:

          x6.lb =  x0 + 2x1   : [6, 16]
          x6.ub =  x0 + 2x1   : [6, 16]
        */

        List<Tightening> expectedBounds( {
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
        } );

        List<Tightening> bounds;
        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );
        TS_ASSERT( boundsEqual( bounds, expectedBounds ) );
    }

    void test_sbt_relus_active_and_inactive()
    {
        Options::get()->setString( Options::SYMBOLIC_BOUND_TIGHTENING_TYPE, "sbt" );

        NLR::NetworkLevelReasoner nlr;
        MockTableau tableau;
        nlr.setTableau( &tableau );
        populateNetworkSBTRelu( nlr, tableau );

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

          x3.lb =  x0 + x1   : [5, 11]
          x3.ub =  x0 + x1   : [5, 11]

          First ReLU is inactive, bounds get zeroed
          Second ReLU is active, bounds surive the activation

          x4.lb = 0
          x4.ub = 0

          x5.lb =  x0 + x1   : [5, 11]
          x5.ub =  x0 + x1   : [5, 11]

          Layer 2:

          x6.lb =  - x0 - x1  : [-11, -5]
          x6.ub =  - x0 - x1  : [-11, -5]
        */

        List<Tightening> expectedBounds( {
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
        } );

        List<Tightening> bounds;
        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );
        TS_ASSERT( boundsEqual( bounds, expectedBounds ) );
    }

    void test_sbt_relus_active_and_not_fixed()
    {
        Options::get()->setString( Options::SYMBOLIC_BOUND_TIGHTENING_TYPE, "sbt" );

        NLR::NetworkLevelReasoner nlr;
        MockTableau tableau;
        nlr.setTableau( &tableau );
        populateNetworkSBTRelu( nlr, tableau );

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

          x3.lb =  x0 + x1   : [5, 11]
          x3.ub =  x0 + x1   : [5, 11]

          First ReLU is undecided, bound is concretized.
            Coefficient: 12/(12--4) = 12/16 = 0.75
          Second ReLU is active, bounds surive the activation

          x4 range: [0, 12]
          x4.lb = 0.75( 2x0 + 3x1 ) - 0.75 * 15      = 1.5x0 + 2.25x1 - 11.25
          x4.ub = 0.75( 2x0 + 3x1 ) - 0.75 * 15 + 3  = 1.5x0 + 2.25x1 -  8.25

          x5.lb =  x0 + x1   : [5, 11]
          x5.ub =  x0 + x1   : [5, 11]

          Layer 2:

          x6.lb =  0.5x0 + 1.25x1 - 11.25
          x6.ub =  0.5x0 + 1.25x1 -  8.25

          x6 range: [2 + 1.25 - 11.25 = -8, 3 + 6.25 - 8.25 = 1] = [-8, 1]
        */

        List<Tightening> expectedBounds( {
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
        } );

        List<Tightening> bounds;
        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );
        TS_ASSERT( boundsEqual( bounds, expectedBounds ) );
    }

    void test_sbt_relus_active_and_externally_fixed()
    {
        Options::get()->setString( Options::SYMBOLIC_BOUND_TIGHTENING_TYPE, "sbt" );

        NLR::NetworkLevelReasoner nlr;
        MockTableau tableau;
        nlr.setTableau( &tableau );
        populateNetworkSBTRelu( nlr, tableau );

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

          x3.lb =  x0 + x1   : [5, 11]
          x3.ub =  x0 + x1   : [5, 11]

          First ReLU is inactive (set externally), bounds get zeroed
          Second ReLU is active, bounds surive the activation

          x4.lb = 0
          x4.ub = 0

          x5.lb =  x0 + x1   : [5, 11]
          x5.ub =  x0 + x1   : [5, 11]

          Layer 2:

          x6.lb =  - x0 - x1  : [-11, -5]
          x6.ub =  - x0 - x1  : [-11, -5]
        */

        List<Tightening> expectedBounds( {
            // x2 does not appear, because it has been eliminated

            Tightening( 3, 5, Tightening::LB ),
            Tightening( 3, 11, Tightening::UB ),

            Tightening( 4, 0, Tightening::LB ),
            Tightening( 4, 0, Tightening::UB ),
            Tightening( 5, 5, Tightening::LB ),
            Tightening( 5, 11, Tightening::UB ),

            Tightening( 6, -11, Tightening::LB ),
            Tightening( 6, -5, Tightening::UB ),
        } );

        List<Tightening> bounds;
        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );
        TS_ASSERT( boundsEqual( bounds, expectedBounds ) );
    }

    void test_sbt_relu_residual1()
    {
        Options::get()->setString( Options::SYMBOLIC_BOUND_TIGHTENING_TYPE, "sbt" );

        NLR::NetworkLevelReasoner nlr;
        MockTableau tableau;
        nlr.setTableau( &tableau );
        populateNetworkSBTReluResidual1( nlr, tableau );

        tableau.setLowerBound( 0, -1 );
        tableau.setUpperBound( 0, 1 );

        // Invoke SBT
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );
        TS_ASSERT_THROWS_NOTHING( nlr.symbolicBoundPropagation() );

        /*
          Input ranges:

          x0: [-1, 1]

          Layer 1:

          x1.lb = x0   : [-1, 1]
          x1.ub = x0   : [-1, 1]

          ReLU is undecided, bound is concretized.
            Coefficient: 1/( 1--1 ) = 1/2 = 0.5

          x2.lb = 0.5x0
          x2.ub = 0.5x0 + 0.5
          x2 range: [0, 1]

          Layer 2 (with residual from x0):

          x3.lb = -1( 0.5x0 + 0.5 ) -x0 + 1 = -1.5x0 + 0.5 : [-1, 1]
          x3.ub = -1( 0.5x0 ) -1x0 + 1 = -1.5x0 + 1 : [-0.5, 2.5]
          x3 range: [-1, 2.5]

          ReLU is undecided, bound is concretized.
            Coefficient: 2.5/( 2.5--1 ) = 2.5/3.5 = 5/7.

          x4.lb = 0
          x4.ub = 5/7 ( -1.5x0 + 1 ) + 5/7 = -15/14 x0 + 20/14 : [5/14, 35/14 = 2.5]
          x4 range: [0, 2.5]

          Layer 3 (with residual from x1):

          x5.lb =  3 ( 0 ) + 3 ( x0 ) + 1 = 3x0 + 1 : [-2, 4]
          x5.ub =  3 ( -15/14 x0 + 20/14 ) + 3 ( x0 ) + 1 = -3/14 x0 + 74/14 : [71/14, 77/14 = 5.5]

          x5 range: [-2, 4]
        */

        List<Tightening> expectedBounds( {
            Tightening( 1, -1, Tightening::LB ),
            Tightening( 1, 1, Tightening::UB ),
            Tightening( 2, 0, Tightening::LB ),
            Tightening( 2, 1, Tightening::UB ),
            Tightening( 3, -1, Tightening::LB ),
            Tightening( 3, 2.5, Tightening::UB ),
            Tightening( 4, 0, Tightening::LB ),
            Tightening( 4, 2.5, Tightening::UB ),
            Tightening( 5, 2, Tightening::LB ),
            Tightening( 5, 5.5, Tightening::UB ),
        } );

        List<Tightening> bounds;
        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );
        TS_ASSERT( boundsEqual( bounds, expectedBounds ) );
    }

    void test_sbt_relu_residual2()
    {
        Options::get()->setString( Options::SYMBOLIC_BOUND_TIGHTENING_TYPE, "sbt" );

        NLR::NetworkLevelReasoner nlr;
        MockTableau tableau;
        nlr.setTableau( &tableau );
        populateNetworkSBTReluResidual2( nlr, tableau );

        tableau.setLowerBound( 0, -1 );
        tableau.setUpperBound( 0, 1 );

        // Invoke SBT
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );
        TS_ASSERT_THROWS_NOTHING( nlr.symbolicBoundPropagation() );

        /*
          Input ranges:

          x0: [-1, 1]

          Layer 1:

          x1.lb = x0   : [-1, 1]
          x1.ub = x0   : [-1, 1]

          ReLU is undecided, bound is concretized.
            Coefficient: 1/( 1--1 ) = 1/2 = 0.5

          x2.lb = 0.5x0
          x2.ub = 0.5x0 + 0.5
          x2 range: [0, 1]

          Layer 2 (with residual from x0):

          x3.lb = -1( 0.5x0 + 0.5 ) -x0 + 1 = -1.5x0 + 0.5 : [-1, 1]
          x3.ub = -1( 0.5x0 ) -1x0 + 1 = -1.5x0 + 1 : [-0.5, 2.5]
          x3 range: [-1, 2.5]

          ReLU is undecided, bound is concretized.
            Coefficient: 2.5/( 2.5--1 ) = 2.5/3.5 = 5/7.

          x4.lb = 0
          x4.ub = 5/7 ( -1.5x0 + 1 ) + 5/7 = -15/14 x0 + 20/14 : [5/14, 35/14 = 2.5]
          x4 range: [0, 2.5]

          Layer 3 (with residual from x0):

          x5.lb =  3 ( 0 ) + 1 ( x0 ) + 1 = 1x0 + 1 : [0, 2]
          x5.ub =  3 ( -15/14 x0 + 20/14 ) + 1 ( x0 ) + 1 = -31/14 x0 + 74/14 : [43/14, 105/14
          = 7.5] x5 range: [0, 7.5]

          Layer 4:
          x6.lb = 1x0 + 1 : [0, 2]
          x6.ub = -31/14 x0 + 74/14 : [43/14, 105/14 = 7.5]
          x6 range: [0, 7.5]
        */

        List<Tightening> expectedBounds( {
            Tightening( 1, -1, Tightening::LB ),
            Tightening( 1, 1, Tightening::UB ),
            Tightening( 2, 0, Tightening::LB ),
            Tightening( 2, 1, Tightening::UB ),
            Tightening( 3, -1, Tightening::LB ),
            Tightening( 3, 2.5, Tightening::UB ),
            Tightening( 4, 0, Tightening::LB ),
            Tightening( 4, 2.5, Tightening::UB ),
            Tightening( 5, 0, Tightening::LB ),
            Tightening( 5, 7.5, Tightening::UB ),
            Tightening( 6, 0, Tightening::LB ),
            Tightening( 6, 7.5, Tightening::UB ),
        } );

        List<Tightening> bounds;
        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );
        TS_ASSERT( boundsEqual( bounds, expectedBounds ) );
    }

    void test_sbt_relu_reindex()
    {
        Options::get()->setString( Options::SYMBOLIC_BOUND_TIGHTENING_TYPE, "sbt" );

        NLR::NetworkLevelReasoner nlr;
        MockTableau tableau;
        nlr.setTableau( &tableau );
        populateNetworkSBTReluReindex( nlr, tableau );

        tableau.setLowerBound( 0, -1 );
        tableau.setUpperBound( 0, 1 );
        tableau.setLowerBound( 1, -1 );
        tableau.setUpperBound( 1, 1 );

        // Invoke SBT
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );
        TS_ASSERT_THROWS_NOTHING( nlr.symbolicBoundPropagation() );

        /*
          Input ranges:

          x0: [-1, 1]
          x1: [-1, 1]

          Layer 1:

          x2.lb = x0 + x1   : [-2, 2]
          x2.ub = x0 + x1   : [-2, 2]

          x3.lb = x0 - x1   : [-2, 2]
          x3.ub = x0 - x1   : [-2, 2]

          Both ReLUs are undecided, bounds are concretized.
            Coefficient: 2/( 2--2 ) = 2/4 = 0.5

          x4.lb = 0.5 ( x0 + x1 ) = 0.5x0 + 0.5x1
          x4.ub = 0.5 ( x0 + x1 ) + 1 = 0.5x0 + 0.5x1 + 1
          x4 range: [0, 2]

          x5.lb = 0.5 ( x0 - x1 ) = 0.5x0 - 0.5x1
          x5.ub = 0.5 ( x0 - x1 ) + 1 = 0.5x0 - 0.5x1 + 1
          x5 range: [0, 2]

          Layer 2:

          x6.lb = 1 ( 0.5x0 + 0.5x1 ) + 1 ( 0.5x0 - 0.5x1 ) = x0   : [-1, 1]
          x6.ub = 1 ( 0.5x0 + 0.5x1 + 1 ) + 1 ( 0.5x0 - 0.5x1 + 1 ) = x0 + 2   : [1, 3]
          x6 range: [-1, 3]

          x7.lb = 1 ( 0.5x0 + 0.5x1 ) - 1 ( 0.5x0 - 0.5x1 + 1 ) = x1 - 1   : [-2, 0]
          x7.ub = 1 ( 0.5x0 + 0.5x1 + 1 ) - 1 ( 0.5x0 - 0.5x1 ) = x1 + 1  : [0, 2]
          x7 range: [-2, 2]

          Both ReLUs are undecided, bounds are concretized.
            Coefficient (first ReLU, lower): 1/( 1--1 ) = 1/2 = 0.5
            Coefficient (first ReLU, upper): 1 (propagated as is)
            Coefficient (second ReLU, lower): 0 (bound is zero)
            Coefficient (second ReLU, upper): 2/( 2--2 ) = 2/4 = 0.5

          x8.lb = 0.5 ( x0 ) = 0.5x0
          x8.ub = x0 + 2
          x8 range: [0, 3]

          x9.lb = 0
          x9.ub = 0.5 ( x1 + 1 ) + 1 = 0.5x1 + 1.5
          x9 range: [0, 2]

          Layer 3:

          x10.lb =  1 ( 0.5x0 ) + 1 ( 0 ) + 1 = 0.5x0 + 1 : [0.5, 1.5]
          x10.ub =  1 ( x0 + 2 ) + 1 ( 0.5x1 + 1.5 ) + 1 = x0 + 0.5x1 + 4.5   : [3, 6]
          x10 range: [0.5, 6]

          x11.lb = 0.5x1 - 0.5
          x11.ub = 0.5x1 + 1.5
          x11 range: [0, 2]

        */

        List<Tightening> expectedBounds(
            { Tightening( 2, -2, Tightening::LB ),   Tightening( 2, 2, Tightening::UB ),
              Tightening( 3, -2, Tightening::LB ),   Tightening( 3, 2, Tightening::UB ),

              Tightening( 4, 0, Tightening::LB ),    Tightening( 4, 2, Tightening::UB ),
              Tightening( 5, 0, Tightening::LB ),    Tightening( 5, 2, Tightening::UB ),

              Tightening( 6, -1, Tightening::LB ),   Tightening( 6, 3, Tightening::UB ),
              Tightening( 7, -2, Tightening::LB ),   Tightening( 7, 2, Tightening::UB ),

              Tightening( 8, 0, Tightening::LB ),    Tightening( 8, 3, Tightening::UB ),
              Tightening( 9, 0, Tightening::LB ),    Tightening( 9, 2, Tightening::UB ),

              Tightening( 10, 0.5, Tightening::LB ), Tightening( 10, 6, Tightening::UB ),
              Tightening( 11, 0, Tightening::LB ),   Tightening( 11, 2, Tightening::UB )

            } );

        List<Tightening> bounds;
        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );
        TS_ASSERT( boundsEqual( bounds, expectedBounds ) );
    }

    void test_sbt_abs_all_positive()
    {
        Options::get()->setString( Options::SYMBOLIC_BOUND_TIGHTENING_TYPE, "sbt" );

        NLR::NetworkLevelReasoner nlr;
        MockTableau tableau;
        tableau.getBoundManager().initialize( 7 );
        nlr.setTableau( &tableau );

        // Create the layers
        nlr.addLayer( 0, NLR::Layer::INPUT, 2 );
        nlr.addLayer( 1, NLR::Layer::WEIGHTED_SUM, 2 );
        nlr.addLayer( 2, NLR::Layer::ABSOLUTE_VALUE, 2 );
        nlr.addLayer( 3, NLR::Layer::WEIGHTED_SUM, 1 );

        // Mark layer dependencies
        for ( unsigned i = 1; i <= 3; ++i )
            nlr.addLayerDependency( i - 1, i );

        // Weights
        nlr.setWeight( 0, 0, 1, 0, 2 );
        nlr.setWeight( 0, 0, 1, 1, 1 );
        nlr.setWeight( 0, 1, 1, 0, 3 );
        nlr.setWeight( 0, 1, 1, 1, 1 );
        nlr.setWeight( 2, 0, 3, 0, 1 );
        nlr.setWeight( 2, 1, 3, 0, -1 );

        // Mark the Abs sources
        nlr.addActivationSource( 1, 0, 2, 0 );
        nlr.addActivationSource( 1, 1, 2, 1 );

        // Variable indexing
        nlr.setNeuronVariable( NLR::NeuronIndex( 0, 0 ), 0 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 0, 1 ), 1 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 1, 0 ), 2 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 1, 1 ), 3 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 2, 0 ), 4 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 2, 1 ), 5 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 3, 0 ), 6 );

        // Very loose bounds for neurons except inputs
        double large = 1000000;

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

          x3.lb =  x0 + x1   : [5, 11]
          x3.ub =  x0 + x1   : [5, 11]

          Both absolute values positive, bound survive through activations:

          x4.lb = 2x0 + 3x1   : [11, 27]
          x4.ub = 2x0 + 3x1   : [11, 27]

          x5.lb =  x0 + x1   : [5, 11]
          x5.ub =  x0 + x1   : [5, 11]

          Layer 2:

          x6.lb =  x0 + 2x1   : [6, 16]
          x6.ub =  x0 + 2x1   : [6, 16]
        */

        List<Tightening> expectedBounds( {
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
        } );

        List<Tightening> bounds;
        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );
        TS_ASSERT( boundsEqual( bounds, expectedBounds ) );
    }

    void test_sbt_abs_positive_and_negative()
    {
        Options::get()->setString( Options::SYMBOLIC_BOUND_TIGHTENING_TYPE, "sbt" );

        NLR::NetworkLevelReasoner nlr;
        MockTableau tableau;
        tableau.getBoundManager().initialize( 7 );
        nlr.setTableau( &tableau );

        // Create the layers
        nlr.addLayer( 0, NLR::Layer::INPUT, 2 );
        nlr.addLayer( 1, NLR::Layer::WEIGHTED_SUM, 2 );
        nlr.addLayer( 2, NLR::Layer::ABSOLUTE_VALUE, 2 );
        nlr.addLayer( 3, NLR::Layer::WEIGHTED_SUM, 1 );

        // Mark layer dependencies
        for ( unsigned i = 1; i <= 3; ++i )
            nlr.addLayerDependency( i - 1, i );

        // Weights
        nlr.setWeight( 0, 0, 1, 0, 2 );
        nlr.setWeight( 0, 0, 1, 1, 1 );
        nlr.setWeight( 0, 1, 1, 0, 3 );
        nlr.setWeight( 0, 1, 1, 1, 1 );
        nlr.setWeight( 2, 0, 3, 0, 1 );
        nlr.setWeight( 2, 1, 3, 0, -1 );

        // Mark the Abs sources
        nlr.addActivationSource( 1, 0, 2, 0 );
        nlr.addActivationSource( 1, 1, 2, 1 );

        // Variable indexing
        nlr.setNeuronVariable( NLR::NeuronIndex( 0, 0 ), 0 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 0, 1 ), 1 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 1, 0 ), 2 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 1, 1 ), 3 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 2, 0 ), 4 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 2, 1 ), 5 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 3, 0 ), 6 );

        // Very loose bounds for neurons except inputs
        double large = 1000000;

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

          x3.lb =  x0 + x1   : [5, 11]
          x3.ub =  x0 + x1   : [5, 11]

          First absolute value is negative, bounds get flipped
          Second absolute value is positive, bounds surive the activation

          x4.lb = -2x0 -3x1 + 30   : [3, 19]
          x4.ub = -2x0 -3x1 + 30   : [3, 19]

          x5.lb =  x0 + x1   : [5, 11]
          x5.ub =  x0 + x1   : [5, 11]

          Layer 2:

          x6.lb =  - 3x0 - 4x1 + 30  : [-8, 14]
          x6.ub =  - 3x0 - 4x1 + 30  : [-8, 14]
        */

        List<Tightening> expectedBounds( {
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
        } );

        List<Tightening> bounds;
        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );
        TS_ASSERT( boundsEqual( bounds, expectedBounds ) );
    }

    void test_sbt_absolute_values_positive_and_not_fixed()
    {
        Options::get()->setString( Options::SYMBOLIC_BOUND_TIGHTENING_TYPE, "sbt" );

        NLR::NetworkLevelReasoner nlr;
        MockTableau tableau;
        tableau.getBoundManager().initialize( 7 );
        nlr.setTableau( &tableau );

        // Create the layers
        nlr.addLayer( 0, NLR::Layer::INPUT, 2 );
        nlr.addLayer( 1, NLR::Layer::WEIGHTED_SUM, 2 );
        nlr.addLayer( 2, NLR::Layer::ABSOLUTE_VALUE, 2 );
        nlr.addLayer( 3, NLR::Layer::WEIGHTED_SUM, 1 );

        // Mark layer dependencies
        for ( unsigned i = 1; i <= 3; ++i )
            nlr.addLayerDependency( i - 1, i );

        // Weights
        nlr.setWeight( 0, 0, 1, 0, 2 );
        nlr.setWeight( 0, 0, 1, 1, 1 );
        nlr.setWeight( 0, 1, 1, 0, 3 );
        nlr.setWeight( 0, 1, 1, 1, 1 );
        nlr.setWeight( 2, 0, 3, 0, 1 );
        nlr.setWeight( 2, 1, 3, 0, -1 );

        // Mark the Abs sources
        nlr.addActivationSource( 1, 0, 2, 0 );
        nlr.addActivationSource( 1, 1, 2, 1 );

        // Variable indexing
        nlr.setNeuronVariable( NLR::NeuronIndex( 0, 0 ), 0 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 0, 1 ), 1 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 1, 0 ), 2 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 1, 1 ), 3 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 2, 0 ), 4 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 2, 1 ), 5 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 3, 0 ), 6 );

        // Very loose bounds for neurons except inputs
        double large = 1000000;

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

          x3.lb =  x0 + x1   : [5, 11]
          x3.ub =  x0 + x1   : [5, 11]

          First absolute value is undecided, bounds are concretized.
          Second absolute value is active, bounds surive the activation

          x4 range: [0, 12]
          x4.lb = 0
          x4.ub = 12

          x5.lb =  x0 + x1   : [5, 11]
          x5.ub =  x0 + x1   : [5, 11]

          Layer 2:

          x6.lb =  - x0 - x1       : [-11, -5]
          x6.ub =  - x0 - x1 + 12  : [  1,  7]

          x6 range: [-11, 7]
        */

        List<Tightening> expectedBounds( {
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
        } );

        List<Tightening> bounds;
        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );
        TS_ASSERT( boundsEqual( bounds, expectedBounds ) );
    }

    void test_sbt_absolute_values_active_and_externally_fixed()
    {
        Options::get()->setString( Options::SYMBOLIC_BOUND_TIGHTENING_TYPE, "sbt" );

        NLR::NetworkLevelReasoner nlr;
        MockTableau tableau;
        tableau.getBoundManager().initialize( 7 );
        nlr.setTableau( &tableau );

        // Create the layers
        nlr.addLayer( 0, NLR::Layer::INPUT, 2 );
        nlr.addLayer( 1, NLR::Layer::WEIGHTED_SUM, 2 );
        nlr.addLayer( 2, NLR::Layer::ABSOLUTE_VALUE, 2 );
        nlr.addLayer( 3, NLR::Layer::WEIGHTED_SUM, 1 );

        // Mark layer dependencies
        for ( unsigned i = 1; i <= 3; ++i )
            nlr.addLayerDependency( i - 1, i );

        // Weights
        nlr.setWeight( 0, 0, 1, 0, 2 );
        nlr.setWeight( 0, 0, 1, 1, 1 );
        nlr.setWeight( 0, 1, 1, 0, 3 );
        nlr.setWeight( 0, 1, 1, 1, 1 );
        nlr.setWeight( 2, 0, 3, 0, 1 );
        nlr.setWeight( 2, 1, 3, 0, -1 );

        // Mark the Abs sources
        nlr.addActivationSource( 1, 0, 2, 0 );
        nlr.addActivationSource( 1, 1, 2, 1 );

        // Variable indexing
        nlr.setNeuronVariable( NLR::NeuronIndex( 0, 0 ), 0 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 0, 1 ), 1 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 1, 0 ), 2 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 1, 1 ), 3 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 2, 0 ), 4 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 2, 1 ), 5 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 3, 0 ), 6 );

        // Very loose bounds for neurons except inputs
        double large = 1000000;

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

          x2 is eliminated, everything set to -3

          x3.lb =  x0 + x1   : [5, 11]
          x3.ub =  x0 + x1   : [5, 11]

          Second absolute value is positive, bounds surive the activation

          x4: all set to 3

          x5.lb =  x0 + x1   : [5, 11]
          x5.ub =  x0 + x1   : [5, 11]

          Layer 2:

          x6.lb =  - x0 - x1 + 3  : [-8, -2]
          x6.ub =  - x0 - x1 + 3  : [-8, -2]
        */

        List<Tightening> expectedBounds( {
            // x2 does not appear, because it has been eliminated

            Tightening( 3, 5, Tightening::LB ),
            Tightening( 3, 11, Tightening::UB ),

            Tightening( 4, 3, Tightening::LB ),
            Tightening( 4, 3, Tightening::UB ),
            Tightening( 5, 5, Tightening::LB ),
            Tightening( 5, 11, Tightening::UB ),

            Tightening( 6, -8, Tightening::LB ),
            Tightening( 6, -2, Tightening::UB ),
        } );

        List<Tightening> bounds;
        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );
        TS_ASSERT( boundsEqual( bounds, expectedBounds ) );
    }

    void test_sbt_signs_positive_and_not_fixed()
    {
        Options::get()->setString( Options::SYMBOLIC_BOUND_TIGHTENING_TYPE, "sbt" );

        NLR::NetworkLevelReasoner nlr;
        MockTableau tableau;
        tableau.getBoundManager().initialize( 7 );
        nlr.setTableau( &tableau );

        // Create the layers
        nlr.addLayer( 0, NLR::Layer::INPUT, 2 );
        nlr.addLayer( 1, NLR::Layer::WEIGHTED_SUM, 2 );
        nlr.addLayer( 2, NLR::Layer::SIGN, 2 );
        nlr.addLayer( 3, NLR::Layer::WEIGHTED_SUM, 1 );

        // Mark layer dependencies
        for ( unsigned i = 1; i <= 3; ++i )
            nlr.addLayerDependency( i - 1, i );

        // Weights
        nlr.setWeight( 0, 0, 1, 0, 2 );
        nlr.setWeight( 0, 0, 1, 1, 1 );
        nlr.setWeight( 0, 1, 1, 0, 3 );
        nlr.setWeight( 0, 1, 1, 1, 1 );
        nlr.setWeight( 2, 0, 3, 0, 1 );
        nlr.setWeight( 2, 1, 3, 0, -1 );

        // Mark the Sign sources
        nlr.addActivationSource( 1, 0, 2, 0 );
        nlr.addActivationSource( 1, 1, 2, 1 );

        // Variable indexing
        nlr.setNeuronVariable( NLR::NeuronIndex( 0, 0 ), 0 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 0, 1 ), 1 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 1, 0 ), 2 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 1, 1 ), 3 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 2, 0 ), 4 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 2, 1 ), 5 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 3, 0 ), 6 );

        // Very loose bounds for neurons except inputs
        double large = 1000000;

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

          x3.lb =  x0 + x1   : [5, 11]
          x3.ub =  x0 + x1   : [5, 11]

          First sign is undecided, bounds are concretized.
          Second sign is active, bounds become constant 1
            Coefficient (first Sign, lower): 2/12 = 1/6.
            Coefficient (first Sign, upper): -2/-4 = 1/2.

          x4 range: [-1, 1]
          x4.lb = 1/6 ( 2x0 + 3x1 - 15 ) - 1 = 2/6 x0 + 3/6 x1 - 21/6
          x4.ub = 1/2 ( 2x0 + 3x1 - 15 ) + 1 = x0 + 1.5x1 - 6.5

          x5 range: [1, 1]
          x5.lb = 1
          x5.ub = 1

          Layer 2:

          x6.lb =  1 ( 2/6 x0 + 3/6 x1 - 21/6 ) - 1 ( 1 ) = 2/6 x0 + 3/6 x1 - 27/6 : [-16/6, 0]
          x6.ub =  1 ( x0 + 1.5x1 - 6.5 ) - 1 ( 1 ) = x0 + 1.5x1 - 7.5 : [-2, 6]

          x6 range: [-8/3, 6]
        */

        List<Tightening> expectedBounds( {
            Tightening( 2, -4, Tightening::LB ),
            Tightening( 2, 12, Tightening::UB ),
            Tightening( 3, 5, Tightening::LB ),
            Tightening( 3, 11, Tightening::UB ),

            Tightening( 4, -1, Tightening::LB ),
            Tightening( 4, 1, Tightening::UB ),
            Tightening( 5, 1, Tightening::LB ),
            Tightening( 5, 1, Tightening::UB ),

            Tightening( 6, -2.6667, Tightening::LB ),
            Tightening( 6, 6, Tightening::UB ),
        } );

        List<Tightening> bounds;
        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );
        TS_ASSERT( boundsEqual( bounds, expectedBounds ) );
    }

    void test_sbt_signs_active_and_externally_fixed()
    {
        Options::get()->setString( Options::SYMBOLIC_BOUND_TIGHTENING_TYPE, "sbt" );

        NLR::NetworkLevelReasoner nlr;
        MockTableau tableau;
        tableau.getBoundManager().initialize( 7 );
        nlr.setTableau( &tableau );

        // Create the layers
        nlr.addLayer( 0, NLR::Layer::INPUT, 2 );
        nlr.addLayer( 1, NLR::Layer::WEIGHTED_SUM, 2 );
        nlr.addLayer( 2, NLR::Layer::SIGN, 2 );
        nlr.addLayer( 3, NLR::Layer::WEIGHTED_SUM, 1 );

        // Mark layer dependencies
        for ( unsigned i = 1; i <= 3; ++i )
            nlr.addLayerDependency( i - 1, i );

        // Weights
        nlr.setWeight( 0, 0, 1, 0, 2 );
        nlr.setWeight( 0, 0, 1, 1, 1 );
        nlr.setWeight( 0, 1, 1, 0, 3 );
        nlr.setWeight( 0, 1, 1, 1, 1 );
        nlr.setWeight( 2, 0, 3, 0, 1 );
        nlr.setWeight( 2, 1, 3, 0, -1 );

        // Mark the Sign sources
        nlr.addActivationSource( 1, 0, 2, 0 );
        nlr.addActivationSource( 1, 1, 2, 1 );

        // Variable indexing
        nlr.setNeuronVariable( NLR::NeuronIndex( 0, 0 ), 0 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 0, 1 ), 1 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 1, 0 ), 2 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 1, 1 ), 3 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 2, 0 ), 4 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 2, 1 ), 5 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 3, 0 ), 6 );

        // Very loose bounds for neurons except inputs
        double large = 1000000;

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

          x2 is eliminated, everything set to -3

          x3.lb =  x0 + x1   : [5, 11]
          x3.ub =  x0 + x1   : [5, 11]

          First sign is negative, bounds become constant -1
          Second sign is positive, bounds become constant 1

          x4: all set to -1

          x5: all set to 1

          Layer 2:

          x6.lb = 1 ( -1 ) - 1 ( 1 ) = -2
          x6.ub = 1 ( -1 ) - 1 ( 1 ) = -2
        */

        List<Tightening> expectedBounds( {
            // x2 does not appear, because it has been eliminated

            Tightening( 3, 5, Tightening::LB ),
            Tightening( 3, 11, Tightening::UB ),

            Tightening( 4, -1, Tightening::LB ),
            Tightening( 4, -1, Tightening::UB ),
            Tightening( 5, 1, Tightening::LB ),
            Tightening( 5, 1, Tightening::UB ),

            Tightening( 6, -2, Tightening::LB ),
            Tightening( 6, -2, Tightening::UB ),
        } );

        List<Tightening> bounds;
        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );
        TS_ASSERT( boundsEqual( bounds, expectedBounds ) );
    }

    void test_sbt_leaky_relu()
    {
        NLR::NetworkLevelReasoner nlr;
        MockTableau tableau;
        nlr.setTableau( &tableau );
        populateNetworkSBTLeakyReLU( nlr, tableau ); // alpha = 0.2

        tableau.setLowerBound( 0, -1 );
        tableau.setUpperBound( 0, 1 );
        tableau.setLowerBound( 1, -1 );
        tableau.setUpperBound( 1, 1 );

        // Invoke SBT
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );
        TS_ASSERT_THROWS_NOTHING( nlr.symbolicBoundPropagation() );

        /*
          Input ranges:

          x0: [-1, 1]
          x1: [-1, 1]

          Layer 1:

          x2.lb = x0 + x1   : [-2, 2]
          x2.ub = x0 + x1   : [-2, 2]

          x3.lb = x0 - x1   : [-2, 2]
          x3.ub = x0 - x1   : [-2, 2]

          Both LeakyReLUs are undecided, bounds are concretized.
            Coefficient: ( 2 - 0.2*-2 )/( 2--2 ) = 2.4/4 = 0.6
            Bias: ( 0.2 - 1 ) * 2 * -2 / ( 2--2 ) = 0.8

          x4.lb = x0 + x1
          x4.ub = 0.6 ( x0 + x1 ) + 0.8 = 0.6x0 + 0.6x1 + 0.8
          x4 range: [-0.4, 2]

          x5.lb = x0 - x1
          x5.ub = 0.6 ( x0 - x1 ) + 0.8 = 0.6x0 - 0.6x1 + 0.8
          x5 range: [-0.4, 2]

          Layer 2:

          x6.lb = 1 ( x0 + x1 ) + 1 ( x0 - x1 ) = 2x0   : [-2, 2]
          x6.ub = 1 ( 0.6x0 + 0.6x1 + 0.8 ) + 1 ( 0.6x0 - 0.6x1 + 0.8 ) = 1.2x0 + 1.6   : [0.4, 2.8]
          x6 range: [-2, 2.8]

          x7.lb = 1 ( x0 + x1 ) - 1 ( 0.6x0 - 0.6x1 + 0.8 ) = 0.4x0 + 1.6x1 - 0.8   : [-2.8, 1.2]
          x7.ub = 1 ( 0.6x0 + 0.6x1 + 0.8 ) - 1 ( x0 - x1 ) = -0.4x0 + 1.6x1 + 0.8  : [-1.2, 2.8]
          x7 range: [-2.8, 2.8]

          Both LeakyReLUs are undecided, bounds are concretized.
            Coefficient (first LeakyReLU): ( 2.8 - 0.2*-2 )/( 2.8--2 ) = 3.2/4.8 = 10/15
            Bias (first LeakyReLU): ( 0.2 - 1 ) * 2.8 * -2 / ( 2.8--2 ) = 14/15

            Coefficient (second LeakyReLU): ( 2.8 - 0.2*-2.8 )/( 2.8--2.8 ) = 3.36/5.6 = 0.6
            Bias (second LeakyReLU): ( 0.2 - 1 ) * 2.8 * -2.8 / ( 2.8--2.8 ) = 1.12

          x8.lb = 2x0
          x8.ub = 10/15 ( 1.2x0 + 1.6 ) + 14/15 = 0.8x0 + 2
          x8 range: [-0.4, 2.8]

          x9.lb = 0.4x0 + 1.6x1 - 0.8
          x9.ub = 0.6 ( -0.4x0 + 1.6x1 + 0.8 ) + 1.12 = -0.24 x0 + 0.96 x1 + 1.6
          x9 range: [-0.56, 2.8]

          Layer 3:

          x10.lb =  1 ( 0.4x0 + 1.6x1 - 0.8 ) + 1 ( 2x0 ) + 1 = 2.4x0 + 1.6x1 + 0.2    : [-3.8, 5.2]
          x10.ub =  1 ( -0.24 x0 + 0.96 x1 + 1.6 ) + 1 ( 0.8x0 + 2 ) + 1 = 0.56x0 + 0.96x1 + 4.6   :
          [3.08, 6.12] x10 range: [-3.8, 6.12]

          x11.lb = 0.4x0 + 1.6x1 - 0.8   : [-2.8, 1.2]
          x11.ub = 0.6 ( -0.4x0 + 1.6x1 + 0.8 ) + 1.12 = -0.24 x0 + 0.96 x1 + 1.6   : [0.4, 2.8]
          x11 range: [-2.8, 2.8]

        */

        List<Tightening> expectedBounds(
            { Tightening( 2, -2, Tightening::LB ),    Tightening( 2, 2, Tightening::UB ),
              Tightening( 3, -2, Tightening::LB ),    Tightening( 3, 2, Tightening::UB ),

              Tightening( 4, -0.4, Tightening::LB ),  Tightening( 4, 2, Tightening::UB ),
              Tightening( 5, -0.4, Tightening::LB ),  Tightening( 5, 2, Tightening::UB ),

              Tightening( 6, -2, Tightening::LB ),    Tightening( 6, 2.8, Tightening::UB ),
              Tightening( 7, -2.8, Tightening::LB ),  Tightening( 7, 2.8, Tightening::UB ),

              Tightening( 8, -0.4, Tightening::LB ),  Tightening( 8, 2.8, Tightening::UB ),
              Tightening( 9, -0.56, Tightening::LB ), Tightening( 9, 2.8, Tightening::UB ),

              Tightening( 10, -3.8, Tightening::LB ), Tightening( 10, 6.12, Tightening::UB ),
              Tightening( 11, -2.8, Tightening::LB ), Tightening( 11, 2.8, Tightening::UB )

            } );

        List<Tightening> bounds;
        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );
        TS_ASSERT( boundsEqual( bounds, expectedBounds ) );
    }

    void test_sbt_sigmoids_and_round()
    {
        Options::get()->setString( Options::SYMBOLIC_BOUND_TIGHTENING_TYPE, "sbt" );

        NLR::NetworkLevelReasoner nlr;
        MockTableau tableau;
        nlr.setTableau( &tableau );
        populateNetworkSBTSigmoidsAndRound( nlr, tableau );

        tableau.setLowerBound( 0, -1 );
        tableau.setUpperBound( 0, 1 );
        tableau.setLowerBound( 1, -1 );
        tableau.setUpperBound( 1, 1 );

        // Invoke SBT
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );
        TS_ASSERT_THROWS_NOTHING( nlr.symbolicBoundPropagation() );

        List<Tightening> bounds;
        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );

        // Layer 1
        TS_ASSERT( FloatUtils::areEqual( nlr.getLayer( 1 )->getLb( 0 ), -2, 0.00001 ) );
        TS_ASSERT( FloatUtils::areEqual( nlr.getLayer( 1 )->getUb( 0 ), 2, 0.00001 ) );
        TS_ASSERT( FloatUtils::areEqual( nlr.getLayer( 1 )->getLb( 1 ), -2, 0.00001 ) );
        TS_ASSERT( FloatUtils::areEqual( nlr.getLayer( 1 )->getUb( 1 ), 2, 0.00001 ) );

        // Layer 2
        TS_ASSERT( FloatUtils::areEqual( nlr.getLayer( 2 )->getLb( 0 ), 0.1192, 0.0001 ) );
        TS_ASSERT( FloatUtils::areEqual( nlr.getLayer( 2 )->getUb( 0 ), 0.8807, 0.0001 ) );
        TS_ASSERT( FloatUtils::areEqual( nlr.getLayer( 2 )->getLb( 1 ), 0.1192, 0.0001 ) );
        TS_ASSERT( FloatUtils::areEqual( nlr.getLayer( 2 )->getUb( 1 ), 0.8807, 0.0001 ) );

        // Layer 3
        /*
         Double-check with Python
            ---
            from math import exp as e
            def g(x):
                return 1 / (1 + e(-x))

            def g_prime(x):
                return g(x) * (1 - g(x))

            def lam(l, u):
                return (g(u) - g(l)) / (u - l)

            def lam_prime(l, u):
                return min(g_prime(l), g_prime(u))

            l3 = l4 = -2
            u3 = u4 = 2
            l5 = l6 = g(-2)
            u5 = u6 = g(2)
            lambda7 = lam(l3, u3)
            lambda7_prime = lam_prime(l3, u3)
            lambda8 = lam(l4, u4)
            lambda8_prime = lam_prime(l4, u4)
            x7_l = lambda7_prime * (-2) + g(-2) + g(-2) - lambda7_prime * (-2 + -2)
            x7_u = lambda7_prime * (2) + g(2) + g(2) -lambda7_prime * (2 + 2)
            x8_l = lambda8_prime * (-2) + g(-2) - g(2) - lambda8_prime * (-2 - 2)
            x8_u = lambda8_prime * (2) + g(2) - g(-2) -lambda8_prime * (2 - -2)
            print(x7_l)
            print(x7_u)
            print(x8_l)
            print(x8_u)
            ---
            [output]:
            0.4483930148512481
            1.5516069851487517
            -0.5516069851487517
            0.5516069851487517
        */
        TS_ASSERT( FloatUtils::areEqual( nlr.getLayer( 3 )->getLb( 0 ), 0.4483, 0.0001 ) );
        TS_ASSERT( FloatUtils::areEqual( nlr.getLayer( 3 )->getUb( 0 ), 1.5516, 0.0001 ) );
        TS_ASSERT( FloatUtils::areEqual( nlr.getLayer( 3 )->getLb( 1 ), -0.5516, 0.0001 ) );
        TS_ASSERT( FloatUtils::areEqual( nlr.getLayer( 3 )->getUb( 1 ), 0.5516, 0.0001 ) );

        // Layer 4
        TS_ASSERT_EQUALS( nlr.getLayer( 4 )->getLb( 0 ), 0 );
        TS_ASSERT_EQUALS( nlr.getLayer( 4 )->getUb( 0 ), 2 );
        TS_ASSERT_EQUALS( nlr.getLayer( 4 )->getLb( 1 ), -1 );
        TS_ASSERT_EQUALS( nlr.getLayer( 4 )->getUb( 1 ), 1 );
    }

    void test_sbt_max_not_fixed()
    {
        Options::get()->setString( Options::SYMBOLIC_BOUND_TIGHTENING_TYPE, "sbt" );

        NLR::NetworkLevelReasoner nlr;
        MockTableau tableau;
        nlr.setTableau( &tableau );
        populateNetworkSBTMax( nlr, tableau );

        tableau.setLowerBound( 0, -1 );
        tableau.setUpperBound( 0, 1 );
        tableau.setLowerBound( 1, -1 );
        tableau.setUpperBound( 1, 2 );

        // Invoke SBT
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );
        TS_ASSERT_THROWS_NOTHING( nlr.symbolicBoundPropagation() );

        /*
          Input ranges:

          x0: [-1, 1]
          x1: [-1, 2]

          Layer 1:

          x2.lb =  x0 + x1   : [-2, 3]
          x2.ub =  x0 + x1   : [-2, 3]

          x3.lb =  x0 - x1   : [-3, 2]
          x3.ub =  x0 - x1   : [-3, 2]

          Both ReLUs are undecided, bounds are concretized.
            Coefficient (first ReLU): 3/( 3--2 ) = 3/5 = 0.6
            Coefficient (second ReLU): 2/( 2--3 ) = 2/5 = 0.4

          x4.lb =  0.6 ( x0 + x1 ) = 0.6x0 + 0.6x1
          x4.ub =  0.6 ( x0 + x1 ) + 1.2 = 0.6x0 + 0.6x1 + 1.2
          x4 range: [0, 3]

          x5.lb =  0.4 ( x0 - x1 ) = 0.4x0 + 0.4x1
          x5.ub =  0.4 ( x0 - x1 ) + 1.2 = 0.4x0 + 0.4x1 + 1.2
          x5 range: [0, 2]

          Max is not fixed because x5.lb <= x4.ub and x4.lb <= x5.ub
          Max inherits lower bound from x4, and its upper bound is constant 3.

          x6.lb =  0.6x0 + 0.6x1  : [-1.2, 1.8]
          x6.ub =  3   : [3, 3]
          x6 range: [-1.2, 3]

          Layer 3:

          x7.lb = 2 ( 0.6x0 + 0.6x1 ) = 1.2x0 + 1.8x1   : [-2.4, 3.6]
          x7.ub = 2 ( 3 ) = 6   : [6, 6]
          x7 range: [-2.4, 6]
        */

        List<Tightening> expectedBounds( {
            Tightening( 2, -2, Tightening::LB ),
            Tightening( 2, 3, Tightening::UB ),
            Tightening( 3, -3, Tightening::LB ),
            Tightening( 3, 2, Tightening::UB ),
            Tightening( 4, 0, Tightening::LB ),
            Tightening( 4, 3, Tightening::UB ),
            Tightening( 5, 0, Tightening::LB ),
            Tightening( 5, 2, Tightening::UB ),
            Tightening( 6, 0, Tightening::LB ),
            Tightening( 6, 3, Tightening::UB ),
            Tightening( 7, -2.4, Tightening::LB ),
            Tightening( 7, 6, Tightening::UB ),

        } );

        List<Tightening> bounds;
        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );
        TS_ASSERT( boundsEqual( bounds, expectedBounds ) );
    }

    void test_sbt_max_fixed()
    {
        Options::get()->setString( Options::SYMBOLIC_BOUND_TIGHTENING_TYPE, "sbt" );

        NLR::NetworkLevelReasoner nlr;
        MockTableau tableau;
        nlr.setTableau( &tableau );
        populateNetworkSBTMax( nlr, tableau );

        tableau.setLowerBound( 0, 1 );
        tableau.setUpperBound( 0, 2 );
        tableau.setLowerBound( 1, -3 );
        tableau.setUpperBound( 1, -2 );

        // Invoke SBT
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );
        TS_ASSERT_THROWS_NOTHING( nlr.symbolicBoundPropagation() );

        /*
          Input ranges:

          x0: [1, 2]
          x1: [-3, -2]

          Layer 1:

          x2.lb =  x0 + x1   : [-2, 0]
          x2.ub =  x0 + x1   : [-2, 0]

          x3.lb =  x0 - x1   : [3, 5]
          x3.ub =  x0 - x1   : [3, 5]

          First ReLU is negative, bounds become constant 0
          Second ReLU is positive, bounds survive the activation

          x4: all set to 0

          x5.lb =  x0 - x1   : [3, 5]
          x5.ub =  x0 - x1   : [3, 5]

          Max is fixed because x5.lb > x4.ub, it inherits x5's bounds

          x6.lb =  x0 - x1   : [3, 5]
          x6.ub =  x0 - x1   : [3, 5]

          Layer 3:

          x7.lb = 2 ( x0 - x1 ) = 2x0 - 2x1   : [6, 10]
          x7.ub = 2 ( x0 - x1 ) = 2x0 - 2x1   : [6, 10]
        */

        List<Tightening> expectedBounds( {
            Tightening( 2, -2, Tightening::LB ),
            Tightening( 2, 0, Tightening::UB ),
            Tightening( 3, 3, Tightening::LB ),
            Tightening( 3, 5, Tightening::UB ),
            Tightening( 4, 0, Tightening::LB ),
            Tightening( 4, 0, Tightening::UB ),
            Tightening( 5, 3, Tightening::LB ),
            Tightening( 5, 5, Tightening::UB ),
            Tightening( 6, 3, Tightening::LB ),
            Tightening( 6, 5, Tightening::UB ),
            Tightening( 7, 6, Tightening::LB ),
            Tightening( 7, 10, Tightening::UB ),

        } );

        List<Tightening> bounds;
        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );
        TS_ASSERT( boundsEqual( bounds, expectedBounds ) );
    }

    void test_sbt_softmax1()
    {
        Options::get()->setString( Options::SYMBOLIC_BOUND_TIGHTENING_TYPE, "sbt" );

        NLR::NetworkLevelReasoner nlr;
        MockTableau tableau;
        nlr.setTableau( &tableau );
        populateNetworkSBTSoftmax( nlr, tableau );

        tableau.setLowerBound( 0, -1 );
        tableau.setUpperBound( 0, 1 );
        tableau.setLowerBound( 1, -1 );
        tableau.setUpperBound( 1, 1 );
        tableau.setLowerBound( 2, -1 );
        tableau.setUpperBound( 2, 1 );

        // Invoke SBT
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );
        TS_ASSERT_THROWS_NOTHING( nlr.symbolicBoundPropagation() );
    }

    void test_sbt_softmax2()
    {
        Options::get()->setString( Options::SYMBOLIC_BOUND_TIGHTENING_TYPE, "sbt" );

        {
            Options::get()->setString( Options::SOFTMAX_BOUND_TYPE, "lse" );
            NLR::NetworkLevelReasoner nlr;
            MockTableau tableau;
            nlr.setTableau( &tableau );
            populateNetworkSBTSoftmax( nlr, tableau );

            tableau.setLowerBound( 0, 1 );
            tableau.setUpperBound( 0, 1.000001 );
            tableau.setLowerBound( 1, 1 );
            tableau.setUpperBound( 1, 1.000001 );
            tableau.setLowerBound( 2, 1 );
            tableau.setUpperBound( 2, 1.000001 );

            // Invoke SBT
            TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );
            TS_ASSERT_THROWS_NOTHING( nlr.symbolicBoundPropagation() );

            /*
              Input ranges:

              x0: [1, 1.0001]
              x1: [1, 1.0001]
              x2: [1, 1.0001]
            */
            List<Tightening> expectedBounds( { Tightening( 3, 2, Tightening::LB ),
                                               Tightening( 3, 2, Tightening::UB ),
                                               Tightening( 4, 3, Tightening::LB ),
                                               Tightening( 4, 3, Tightening::UB ),
                                               Tightening( 5, 0, Tightening::LB ),
                                               Tightening( 5, 0, Tightening::UB ),
                                               Tightening( 6, 0.2595, Tightening::LB ),
                                               Tightening( 6, 0.2595, Tightening::UB ),
                                               Tightening( 7, 0.7054, Tightening::LB ),
                                               Tightening( 7, 0.7054, Tightening::UB ),
                                               Tightening( 8, 0.0351, Tightening::LB ),
                                               Tightening( 8, 0.0351, Tightening::UB ),
                                               Tightening( 9, 1, Tightening::LB ),
                                               Tightening( 9, 1, Tightening::UB ),
                                               Tightening( 10, -1, Tightening::LB ),
                                               Tightening( 10, -1, Tightening::UB )

            } );

            List<Tightening> bounds;
            TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );
            TS_ASSERT( boundsEqual( bounds, expectedBounds ) );
        }
        {
            Options::get()->setString( Options::SOFTMAX_BOUND_TYPE, "er" );
            NLR::NetworkLevelReasoner nlr;
            MockTableau tableau;
            nlr.setTableau( &tableau );
            populateNetworkSBTSoftmax( nlr, tableau );

            tableau.setLowerBound( 0, 1 );
            tableau.setUpperBound( 0, 1.000001 );
            tableau.setLowerBound( 1, 1 );
            tableau.setUpperBound( 1, 1.000001 );
            tableau.setLowerBound( 2, 1 );
            tableau.setUpperBound( 2, 1.000001 );

            // Invoke SBT
            TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );
            TS_ASSERT_THROWS_NOTHING( nlr.symbolicBoundPropagation() );

            /*
              Input ranges:

              x0: [1, 1.0001]
              x1: [1, 1.0001]
              x2: [1, 1.0001]
            */
            List<Tightening> expectedBounds( { Tightening( 3, 2, Tightening::LB ),
                                               Tightening( 3, 2, Tightening::UB ),
                                               Tightening( 4, 3, Tightening::LB ),
                                               Tightening( 4, 3, Tightening::UB ),
                                               Tightening( 5, 0, Tightening::LB ),
                                               Tightening( 5, 0, Tightening::UB ),
                                               Tightening( 6, 0.2595, Tightening::LB ),
                                               Tightening( 6, 0.2595, Tightening::UB ),
                                               Tightening( 7, 0.7054, Tightening::LB ),
                                               Tightening( 7, 0.7054, Tightening::UB ),
                                               Tightening( 8, 0.0351, Tightening::LB ),
                                               Tightening( 8, 0.0351, Tightening::UB ),
                                               Tightening( 9, 1, Tightening::LB ),
                                               Tightening( 9, 1, Tightening::UB ),
                                               Tightening( 10, -1, Tightening::LB ),
                                               Tightening( 10, -1, Tightening::UB )

            } );

            List<Tightening> bounds;
            TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );
            TS_ASSERT( boundsEqual( bounds, expectedBounds ) );
        }
    }

    void test_sbt_softmax3()
    {
        Options::get()->setString( Options::SYMBOLIC_BOUND_TIGHTENING_TYPE, "sbt" );

        NLR::NetworkLevelReasoner nlr;
        MockTableau tableau;
        nlr.setTableau( &tableau );
        populateNetworkSBTSoftmax2( nlr, tableau );

        tableau.setLowerBound( 0, 1 );
        tableau.setUpperBound( 0, 1.00001 );
        tableau.setLowerBound( 1, 1 );
        tableau.setUpperBound( 1, 1.00001 );
        tableau.setLowerBound( 2, 1 );
        tableau.setUpperBound( 2, 1.00001 );

        // Invoke SBT
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );
        TS_ASSERT_THROWS_NOTHING( nlr.symbolicBoundPropagation() );

        /*
          Input ranges:

          x0: [1, 1.0001]
          x1: [1, 1.0001]
          x2: [1, 1.0001]
        */

        List<Tightening> expectedBounds(
            { Tightening( 3, 2, Tightening::LB ),        Tightening( 3, 2, Tightening::UB ),
              Tightening( 4, 3, Tightening::LB ),        Tightening( 4, 3, Tightening::UB ),
              Tightening( 5, 0, Tightening::LB ),        Tightening( 5, 0, Tightening::UB ),
              Tightening( 6, -1, Tightening::LB ),       Tightening( 6, -1, Tightening::UB ),
              Tightening( 7, -2, Tightening::LB ),       Tightening( 7, -2, Tightening::UB ),
              Tightening( 8, 0.8668, Tightening::LB ),   Tightening( 8, 0.8668, Tightening::UB ),
              Tightening( 9, 0.9820, Tightening::LB ),   Tightening( 9, 0.9820, Tightening::UB ),
              Tightening( 10, 0.1173, Tightening::LB ),  Tightening( 10, 0.1173, Tightening::UB ),
              Tightening( 11, 0.0179, Tightening::LB ),  Tightening( 11, 0.0179, Tightening::UB ),
              Tightening( 12, 0.0159, Tightening::LB ),  Tightening( 12, 0.0159, Tightening::UB ),
              Tightening( 13, 0.9470, Tightening::LB ),  Tightening( 13, 0.9470, Tightening::UB ),
              Tightening( 14, -0.9470, Tightening::LB ), Tightening( 14, -0.9470, Tightening::UB ),
              Tightening( 15, 1.0253, Tightening::LB ),  Tightening( 15, 1.0253, Tightening::UB ),
              Tightening( 16, -1.0253, Tightening::LB ), Tightening( 16, -1.0253, Tightening::UB )

            } );

        List<Tightening> bounds;
        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );
        TS_ASSERT( boundsEqual( bounds, expectedBounds ) );
    }

    void test_softmax_bounds_er()
    {
        Vector<double> inputLb = { -1, 0, 1 };
        Vector<double> inputUb = { 0, 2, 4 };
        Vector<double> input = { -0.5, 1, 2.5 };

        double value = NLR::DeepPolySoftmaxElement::ERLowerBound( input, inputLb, inputUb, 0 );
        TS_ASSERT( FloatUtils::areEqual( value, 0.0114799, 0.00001 ) );
        value = NLR::DeepPolySoftmaxElement::dERLowerBound( input, inputLb, inputUb, 0, 0 );
        TS_ASSERT( FloatUtils::areEqual( value, 0.00563867, 0.00001 ) );
        value = NLR::DeepPolySoftmaxElement::dERLowerBound( input, inputLb, inputUb, 0, 1 );
        TS_ASSERT( FloatUtils::areEqual( value, -0.000838421, 0.00001 ) );


        Vector<double> outputLb = { 0.2, 0, 0 };
        Vector<double> outputUb = { 0.4, 0.1, 0.1 };

        value = NLR::DeepPolySoftmaxElement::ERUpperBound( input, outputLb, outputUb, 0 );
        TS_ASSERT( FloatUtils::areEqual( value, -1.44538, 0.00001 ) );
        value = NLR::DeepPolySoftmaxElement::dERUpperBound( input, outputLb, outputUb, 0, 0 );
        TS_ASSERT( FloatUtils::areEqual( value, 1.96538, 0.00001 ) );
        value = NLR::DeepPolySoftmaxElement::dERUpperBound( input, outputLb, outputUb, 0, 1 );
        TS_ASSERT( FloatUtils::areEqual( value, -0.358535, 0.00001 ) );
    }

    void test_softmax_bounds_lse1()
    {
        Vector<double> inputLb = { -1, 0, 1 };
        Vector<double> inputUb = { 0, 2, 3 };
        Vector<double> input = { -0.5, 1, 2 };
        double value = NLR::DeepPolySoftmaxElement::LSELowerBound( input, inputLb, inputUb, 0 );
        TS_ASSERT( FloatUtils::areEqual( value, 0.0365, 0.001 ) );
        value = NLR::DeepPolySoftmaxElement::dLSELowerBound( input, inputLb, inputUb, 0, 0 );
        TS_ASSERT( FloatUtils::areEqual( value, 0.0365, 0.001 ) );
        value = NLR::DeepPolySoftmaxElement::dLSELowerBound( input, inputLb, inputUb, 0, 1 );
        TS_ASSERT( FloatUtils::areEqual( value, -0.00703444, 0.001 ) );

        Vector<double> outputLb = { 0.2, 0, 0 };
        Vector<double> outputUb = { 0.4, 0.1, 0.1 };
        value = NLR::DeepPolySoftmaxElement::LSEUpperBound( input, outputLb, outputUb, 0 );
        TS_ASSERT( FloatUtils::areEqual( value, -0.164165, 0.00001 ) );
        value = NLR::DeepPolySoftmaxElement::dLSEUpperbound( input, outputLb, outputUb, 0, 0 );
        TS_ASSERT( FloatUtils::areEqual( value, 0.272204, 0.00001 ) );
        value = NLR::DeepPolySoftmaxElement::dLSEUpperbound( input, outputLb, outputUb, 0, 1 );
        TS_ASSERT( FloatUtils::areEqual( value, -0.073207, 0.00001 ) );
    }

    void test_sbt_bilinear()
    {
        NLR::NetworkLevelReasoner nlr;
        MockTableau tableau;
        nlr.setTableau( &tableau );
        populateNetworkSBTBilinear( nlr, tableau );

        tableau.setLowerBound( 0, 1 );
        tableau.setUpperBound( 0, 2 );
        tableau.setLowerBound( 1, -2 );
        tableau.setUpperBound( 1, 1 );

        // Invoke SBT
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );
        TS_ASSERT_THROWS_NOTHING( nlr.symbolicBoundPropagation() );

        /*
          Input ranges:

          x0: [1, 2]
          x1: [-2, 1]

          Layer 1:

          x2.lb = x0 - 2x1   : [-1, 6]
          x2.ub = x0 - 2x1   : [-1, 6]

          x3.lb = x0 + x1   : [-1, 3]
          x3.ub = x0 + x1   : [-1, 3]

          Coefficients for bilinear layer:
          Lower bound:
              alpha_l = x3.lb = -1
              beta = x2.lb = -1
              gamma_l = -x2.lb * x3.lb = --1 * -1 = -1

          Upper bound:
              alpha_u = x3.ub = 3
              beta = x2.lb = -1
              gamma_u = -x2.lb * x3.ub = --1 * 3 = 3

          x4.lb = -1 ( x0 - 2x1 ) + -1 ( x0 + x1 ) + -1 = -2x0 + x1 - 1     : [-7, -2]
          x4.ub = 3 ( x0 - 2x1 ) + -1 ( x0 + x1 ) + 3 = 2x0 - 7x1 + 3    : [0, 21]
          x4 range: [-7, 21]

          Layer 3:

          x7.lb = -1 ( 2x0 - 5x1 + 3 ) = -2x0 + 7x1 - 3   : [-21, 0]
          x7.ub = -1 ( -2x0 + 3x1 - 1 ) = 2x0 + x1 + 1   : [2, 7]
          x4 range: [-21, 5]
        */

        List<Tightening> expectedBounds( { Tightening( 2, -1, Tightening::LB ),
                                           Tightening( 2, 6, Tightening::UB ),
                                           Tightening( 3, -1, Tightening::LB ),
                                           Tightening( 3, 3, Tightening::UB ),
                                           Tightening( 4, -7, Tightening::LB ),
                                           Tightening( 4, 21, Tightening::UB ),
                                           Tightening( 5, -21, Tightening::LB ),
                                           Tightening( 5, 7, Tightening::UB ) } );

        List<Tightening> bounds;
        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );
        TS_ASSERT( boundsEqual( bounds, expectedBounds ) );
    }

    void test_concretize_input_assignment()
    {
        NLR::NetworkLevelReasoner nlr;
        MockTableau tableau;
        nlr.setTableau( &tableau );

        populateNetwork( nlr );

        // With ReLUs, Inputs are zeros, only biases count
        tableau.nextValues[0] = 0;
        tableau.nextValues[1] = 0;

        Map<unsigned, double> assignment;

        TS_ASSERT_THROWS_NOTHING( nlr.concretizeInputAssignment( assignment ) );

        TS_ASSERT( FloatUtils::areEqual( nlr.getLayer( 5 )->getAssignment( 0 ), 1 ) );
        TS_ASSERT( FloatUtils::areEqual( nlr.getLayer( 5 )->getAssignment( 1 ), 4 ) );

        TS_ASSERT( assignment.size() == 14 );
        TS_ASSERT( FloatUtils::areEqual( assignment[12], 1 ) );
        TS_ASSERT( FloatUtils::areEqual( assignment[13], 4 ) );

        // With ReLUs, case 1
        tableau.nextValues[0] = 1;
        tableau.nextValues[1] = 1;

        TS_ASSERT_THROWS_NOTHING( nlr.concretizeInputAssignment( assignment ) );

        TS_ASSERT( FloatUtils::areEqual( nlr.getLayer( 5 )->getAssignment( 0 ), 1 ) );
        TS_ASSERT( FloatUtils::areEqual( nlr.getLayer( 5 )->getAssignment( 1 ), 1 ) );

        TS_ASSERT( FloatUtils::areEqual( assignment[12], 1 ) );
        TS_ASSERT( FloatUtils::areEqual( assignment[13], 1 ) );

        // With ReLUs, case 2
        tableau.nextValues[0] = 1;
        tableau.nextValues[1] = 2;

        TS_ASSERT_THROWS_NOTHING( nlr.concretizeInputAssignment( assignment ) );

        TS_ASSERT( FloatUtils::areEqual( nlr.getLayer( 5 )->getAssignment( 0 ), 0 ) );
        TS_ASSERT( FloatUtils::areEqual( nlr.getLayer( 5 )->getAssignment( 1 ), 0 ) );

        TS_ASSERT( FloatUtils::areEqual( assignment[12], 0 ) );
        TS_ASSERT( FloatUtils::areEqual( assignment[13], 0 ) );
    }


    void test_obtain_bound_from_ipq()
    {
        NLR::NetworkLevelReasoner nlr;
        populateNetwork( nlr );

        Query query;
        query.setNumberOfVariables( 14 );


        // Initialize the bounds
        query.setLowerBound( 0, -1 );
        query.setUpperBound( 0, 1 );
        query.setLowerBound( 1, -1 );
        query.setUpperBound( 1, 1 );

        double large = 1000;
        query.setLowerBound( 2, -large );
        query.setUpperBound( 2, large );
        query.setLowerBound( 3, -large );
        query.setUpperBound( 3, large );
        query.setLowerBound( 4, -large );
        query.setUpperBound( 4, large );
        query.setLowerBound( 5, -large );
        query.setUpperBound( 5, large );
        query.setLowerBound( 6, -large );
        query.setUpperBound( 6, large );
        query.setLowerBound( 7, -large );
        query.setUpperBound( 7, large );
        query.setLowerBound( 8, -large );
        query.setUpperBound( 8, large );
        query.setLowerBound( 9, -large );
        query.setUpperBound( 9, large );
        query.setLowerBound( 10, -large );
        query.setUpperBound( 10, large );
        query.setLowerBound( 11, -large );
        query.setUpperBound( 11, large );
        query.setLowerBound( 12, -large );
        query.setUpperBound( 12, large );
        query.setLowerBound( 13, -large );
        query.setUpperBound( 13, large );

        // Initialize
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds( query ) );

        // Perform the tightening pass
        TS_ASSERT_THROWS_NOTHING( nlr.intervalArithmeticBoundPropagation() );

        List<Tightening> expectedBounds( {
            Tightening( 2, 0, Tightening::LB ),   Tightening( 2, 2, Tightening::UB ),
            Tightening( 3, 0, Tightening::LB ),   Tightening( 3, 2, Tightening::UB ),

            Tightening( 4, -5, Tightening::LB ),  Tightening( 4, 5, Tightening::UB ),
            Tightening( 5, 0, Tightening::LB ),   Tightening( 5, 5, Tightening::UB ),

            Tightening( 6, -1, Tightening::LB ),  Tightening( 6, 1, Tightening::UB ),
            Tightening( 7, 0, Tightening::LB ),   Tightening( 7, 1, Tightening::UB ),

            Tightening( 8, -1, Tightening::LB ),  Tightening( 8, 7, Tightening::UB ),
            Tightening( 9, 0, Tightening::LB ),   Tightening( 9, 7, Tightening::UB ),

            Tightening( 10, -1, Tightening::LB ), Tightening( 10, 7, Tightening::UB ),
            Tightening( 11, 0, Tightening::LB ),  Tightening( 11, 7, Tightening::UB ),

            Tightening( 12, 0, Tightening::LB ),  Tightening( 12, 7, Tightening::UB ),
            Tightening( 13, 0, Tightening::LB ),  Tightening( 13, 28, Tightening::UB ),
        } );

        List<Tightening> bounds;
        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );

        TS_ASSERT_EQUALS( expectedBounds.size(), bounds.size() );
        for ( const auto &bound : expectedBounds )
            TS_ASSERT( bounds.exists( bound ) );
    }

    void test_get_previous_bias()
    {
        NLR::NetworkLevelReasoner nlr;
        populateNetwork( nlr );

        // Generate query to create ReLU constraints
        Query query;
        nlr.generateQuery( query );

        // Find ReLU constraints from the query
        List<PiecewiseLinearConstraint *> constraints = query.getPiecewiseLinearConstraints();

        for ( const auto &constraint : constraints )
        {
            ReluConstraint *relu = dynamic_cast<ReluConstraint *>( constraint );
            TS_ASSERT( relu );

            nlr.addConstraintInTopologicalOrder( relu );

            // First ReLU layer (nodes 2,0 through 2,2) has previous bias 1
            if ( relu->getB() == 3 || relu->getB() == 5 || relu->getB() == 7 )
            {
                TS_ASSERT_EQUALS( nlr.getPreviousBias( relu ), 1 );
            }
            // Second ReLU layer (nodes 4,0 and 4,1) has previous bias 2
            else if ( relu->getB() == 9 || relu->getB() == 11 )
            {
                TS_ASSERT_EQUALS( nlr.getPreviousBias( relu ), 2 );
            }
        }
    }

    void test_get_previous_bias_error_handling()
    {
        NLR::NetworkLevelReasoner nlr;
        populateNetwork( nlr );

        // Generate invalid ReLU constraint
        ReluConstraint invalidRelu( 15, 16 ); // Variables not in network

        // Should throw since variables don't exist in network
        TS_ASSERT_THROWS_EQUALS( nlr.getPreviousBias( &invalidRelu ),
                                 const NLRError &e,
                                 e.getCode(),
                                 NLRError::RELU_NOT_FOUND );

        // Test missing activation source using fresh network
        NLR::NetworkLevelReasoner nlrNoActivations;
        // Create minimal network without activation sources
        nlrNoActivations.addLayer( 0, NLR::Layer::INPUT, 2 );
        nlrNoActivations.addLayer( 1, NLR::Layer::WEIGHTED_SUM, 2 );
        nlrNoActivations.addLayer( 2, NLR::Layer::RELU, 2 );

        ReluConstraint missingActivation( 2, 3 );

        TS_ASSERT_THROWS_EQUALS( nlrNoActivations.getPreviousBias( &missingActivation ),
                                 const NLRError &e,
                                 e.getCode(),
                                 NLRError::RELU_NOT_FOUND );
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
