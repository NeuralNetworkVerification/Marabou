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

        TS_ASSERT( FloatUtils::areEqual( output[0], -1, 0.0001) );
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
        TS_ASSERT( FloatUtils::areEqual( output1[1], output2[1] ) );

        // case 2
        input[0] = 1;
        input[1] = 1;

        TS_ASSERT_THROWS_NOTHING( nlr.evaluate( input, output1 ) );
        TS_ASSERT_THROWS_NOTHING( nlr2.evaluate( input, output2 ) );

        TS_ASSERT( FloatUtils::areEqual( output1[0], output2[0] ) );
        TS_ASSERT( FloatUtils::areEqual( output1[1], output2[1] ) );
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
        TS_ASSERT( FloatUtils::areEqual( output1[1], output2[1] ) );

        // case 2
        input[0] = 1;
        input[1] = 1;

        TS_ASSERT_THROWS_NOTHING( nlr.evaluate( input, output1 ) );
        TS_ASSERT_THROWS_NOTHING( nlr2.evaluate( input, output2 ) );

        TS_ASSERT( FloatUtils::areEqual( output1[0], output2[0] ) );
        TS_ASSERT( FloatUtils::areEqual( output1[1], output2[1] ) );
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

        TS_ASSERT_EQUALS( expectedBounds.size(), bounds.size() );
        for ( const auto &bound : expectedBounds )
            TS_ASSERT( bounds.exists( bound ) );

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

        TS_ASSERT_EQUALS( expectedBounds2.size(), bounds.size() );
        for ( const auto &bound : expectedBounds2 )
            TS_ASSERT( bounds.exists( bound ) );
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
        TS_ASSERT_EQUALS( expectedBounds.size(), bounds.size() );
        for ( const auto &bound : expectedBounds )
            TS_ASSERT( bounds.exists( bound ) );

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
        TS_ASSERT_EQUALS( expectedBounds2.size(), bounds.size() );
        for ( const auto &bound : expectedBounds2 )
            TS_ASSERT( bounds.exists( bound ) );
    }

    void populateNetworkSBT( NLR::NetworkLevelReasoner &nlr, MockTableau &tableau )
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

    void test_sbt_relus_all_active()
    {
        Options::get()->setString( Options::SYMBOLIC_BOUND_TIGHTENING_TYPE, "sbt" );

        NLR::NetworkLevelReasoner nlr;
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

        TS_ASSERT_EQUALS( expectedBounds.size(), bounds.size() );
        for ( const auto &bound : expectedBounds )
            TS_ASSERT( bounds.exists( bound ) );
    }

    void test_sbt_relus_active_and_inactive()
    {
        Options::get()->setString( Options::SYMBOLIC_BOUND_TIGHTENING_TYPE, "sbt" );

        NLR::NetworkLevelReasoner nlr;
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

        TS_ASSERT_EQUALS( expectedBounds.size(), bounds.size() );
        for ( const auto &bound : bounds )
            TS_ASSERT( expectedBounds.exists( bound ) );
    }

    void test_sbt_relus_active_and_not_fixed()
    {
        Options::get()->setString( Options::SYMBOLIC_BOUND_TIGHTENING_TYPE, "sbt" );

        NLR::NetworkLevelReasoner nlr;
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

        TS_ASSERT_EQUALS( expectedBounds.size(), bounds.size() );
        for ( const auto &bound : bounds )
            TS_ASSERT( expectedBounds.exists( bound ) );
    }

    void test_sbt_relus_active_and_externally_fixed()
    {
        Options::get()->setString( Options::SYMBOLIC_BOUND_TIGHTENING_TYPE, "sbt" );

        NLR::NetworkLevelReasoner nlr;
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

        TS_ASSERT_EQUALS( expectedBounds.size(), bounds.size() );
        for ( const auto &bound : bounds )
            TS_ASSERT( expectedBounds.exists( bound ) );
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

        TS_ASSERT_EQUALS( expectedBounds.size(), bounds.size() );
        for ( const auto &bound : bounds )
            TS_ASSERT( expectedBounds.exists( bound ) );
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
        TS_ASSERT_EQUALS( expectedBounds.size(), bounds.size() );

        for ( const auto &bound : bounds )
            TS_ASSERT( expectedBounds.exists( bound ) );
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

        TS_ASSERT_EQUALS( expectedBounds.size(), bounds.size() );
        for ( const auto &bound : bounds )
            TS_ASSERT( expectedBounds.exists( bound ) );
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

          x3.lb =  x0 +  x1   : [5, 11]
          x3.ub =  x0 +  x1   : [5, 11]

          Second absolute value is positive, bounds surive the activation

          x4: all set to 3

          x5.lb =  x0 +  x1   : [5, 11]
          x5.ub =  x0 +  x1   : [5, 11]

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

        printf( "Dumpign discovered bounds:\n" );
        for ( const auto &bound : bounds )
            bound.dump();

        TS_ASSERT_EQUALS( expectedBounds.size(), bounds.size() );
        for ( const auto &bound : bounds )
            TS_ASSERT( expectedBounds.exists( bound ) );
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
};
