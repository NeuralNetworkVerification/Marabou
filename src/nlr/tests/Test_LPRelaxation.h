/*********************                                                        */
/*! \file Test_LPRelaxation.h
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

    void populateNetworkBackwardReLU( NLR::NetworkLevelReasoner &nlr, MockTableau &tableau )
    {
        /*

              1      R      -1      R      -1  2
          x0 --- x2 ---> x4 --- x6 ---> x8 --- x10
            \    /        \    /          \    /
           1 \  /        2 \  /          1 \  /
              \/            \/              \/
              /\            /\              /\
          -1 /  \        1 /  \         -1 /  \
            /    \   R    /    \    R     /    \
          x1 --- x3 ---> x5 --- x7 ---> x9 --- x11
              1             -1              2

          The example described in Fig. 2 of
          https://dl.acm.org/doi/10.1145/3563325
          using ReLU activation instead of LeakyReLU
        */

        // Create the layers
        nlr.addLayer( 0, NLR::Layer::INPUT, 2 );
        nlr.addLayer( 1, NLR::Layer::WEIGHTED_SUM, 2 );
        nlr.addLayer( 2, NLR::Layer::RELU, 2 );
        nlr.addLayer( 3, NLR::Layer::WEIGHTED_SUM, 2 );
        nlr.addLayer( 4, NLR::Layer::RELU, 2 );
        nlr.addLayer( 5, NLR::Layer::WEIGHTED_SUM, 2 );

        nlr.getLayer( 2 )->setAlpha( 0.1 );
        nlr.getLayer( 4 )->setAlpha( 0.1 );

        // Mark layer dependencies
        for ( unsigned i = 1; i <= 5; ++i )
            nlr.addLayerDependency( i - 1, i );

        // Set the weights and biases for the weighted sum layers
        nlr.setWeight( 0, 0, 1, 0, 1 );
        nlr.setWeight( 0, 0, 1, 1, 1 );
        nlr.setWeight( 0, 1, 1, 0, -1 );
        nlr.setWeight( 0, 1, 1, 1, 1 );

        nlr.setWeight( 2, 0, 3, 0, -1 );
        nlr.setWeight( 2, 0, 3, 1, 2 );
        nlr.setWeight( 2, 1, 3, 0, 1 );
        nlr.setWeight( 2, 1, 3, 1, -1 );

        nlr.setWeight( 4, 0, 5, 0, -1 );
        nlr.setWeight( 4, 0, 5, 1, 1 );
        nlr.setWeight( 4, 1, 5, 0, -1 );
        nlr.setWeight( 4, 1, 5, 1, 2 );

        nlr.setBias( 5, 1, 2 );

        // Mark the ReLU sources
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

    void populateNetworkBackwardReLU2( NLR::NetworkLevelReasoner &nlr, MockTableau &tableau )
    {
        /*
                x3    x7
          x0                 x11    x14
                x4    x8                   x17    x19
          x1                 x12    x15                  x21
                x5    x9                   x18    x20
          x2                 x13    x16
                x6    x10

          x3 = -x0 + x1
          x4 = x0 + 2*x1
          x5 = x0 + x1 + x2
          x6 = 3*x0 - 2*x1 - x2

          x7 = ReLU( x3 )
          x8 = ReLU( x4 )
          x9 = ReLU( x5 )
          x10 = ReLU( x6 )

          x11 = 2x7 + x9 - x10 + 2
          x12 = -x7 + 2x8 + x10
          x13 = x7 - x8 + 2x9 - 2

          x14 = ReLU( x11 )
          x15 = ReLU( x12 )
          x16 = ReLU( x13 )

          x17 = -x14 + x15 - x16
          x18 = x14 + x15 + x16

          x19 = ReLU( x17 )
          x20 = ReLU( x18 )

          x21 = x19 - x20 - 1
        */

        // Create the layers
        nlr.addLayer( 0, NLR::Layer::INPUT, 3 );
        nlr.addLayer( 1, NLR::Layer::WEIGHTED_SUM, 4 );
        nlr.addLayer( 2, NLR::Layer::RELU, 4 );
        nlr.addLayer( 3, NLR::Layer::WEIGHTED_SUM, 3 );
        nlr.addLayer( 4, NLR::Layer::RELU, 3 );
        nlr.addLayer( 5, NLR::Layer::WEIGHTED_SUM, 2 );
        nlr.addLayer( 6, NLR::Layer::RELU, 2 );
        nlr.addLayer( 7, NLR::Layer::WEIGHTED_SUM, 1 );

        // Mark layer dependencies
        for ( unsigned i = 1; i <= 7; ++i )
            nlr.addLayerDependency( i - 1, i );

        // Set the weights and biases for the weighted sum layers
        nlr.setWeight( 0, 0, 1, 0, -1 );
        nlr.setWeight( 0, 0, 1, 1, 1 );
        nlr.setWeight( 0, 0, 1, 2, 1 );
        nlr.setWeight( 0, 0, 1, 3, 3 );
        nlr.setWeight( 0, 1, 1, 0, 1 );
        nlr.setWeight( 0, 1, 1, 1, 2 );
        nlr.setWeight( 0, 1, 1, 2, 1 );
        nlr.setWeight( 0, 1, 1, 3, -2 );
        nlr.setWeight( 0, 2, 1, 2, 1 );
        nlr.setWeight( 0, 2, 1, 3, -1 );

        nlr.setWeight( 2, 0, 3, 0, 2 );
        nlr.setWeight( 2, 0, 3, 1, -1 );
        nlr.setWeight( 2, 0, 3, 2, 1 );
        nlr.setWeight( 2, 1, 3, 1, 2 );
        nlr.setWeight( 2, 1, 3, 2, -1 );
        nlr.setWeight( 2, 2, 3, 0, 1 );
        nlr.setWeight( 2, 2, 3, 2, 2 );
        nlr.setWeight( 2, 3, 3, 0, -1 );
        nlr.setWeight( 2, 3, 3, 1, 1 );

        nlr.setWeight( 4, 0, 5, 0, -1 );
        nlr.setWeight( 4, 0, 5, 1, 1 );
        nlr.setWeight( 4, 1, 5, 0, 1 );
        nlr.setWeight( 4, 1, 5, 1, 1 );
        nlr.setWeight( 4, 2, 5, 0, -1 );
        nlr.setWeight( 4, 2, 5, 1, 1 );

        nlr.setWeight( 6, 0, 7, 0, 1 );
        nlr.setWeight( 6, 1, 7, 0, -1 );

        nlr.setBias( 3, 0, 2 );
        nlr.setBias( 3, 2, -2 );
        nlr.setBias( 7, 0, -1 );

        // Mark the ReLU sources
        nlr.addActivationSource( 1, 0, 2, 0 );
        nlr.addActivationSource( 1, 1, 2, 1 );
        nlr.addActivationSource( 1, 2, 2, 2 );
        nlr.addActivationSource( 1, 3, 2, 3 );

        nlr.addActivationSource( 3, 0, 4, 0 );
        nlr.addActivationSource( 3, 1, 4, 1 );
        nlr.addActivationSource( 3, 2, 4, 2 );

        nlr.addActivationSource( 5, 0, 6, 0 );
        nlr.addActivationSource( 5, 1, 6, 1 );

        // Variable indexing
        nlr.setNeuronVariable( NLR::NeuronIndex( 0, 0 ), 0 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 0, 1 ), 1 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 0, 2 ), 2 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 1, 0 ), 3 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 1, 1 ), 4 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 1, 2 ), 5 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 1, 3 ), 6 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 2, 0 ), 7 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 2, 1 ), 8 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 2, 2 ), 9 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 2, 3 ), 10 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 3, 0 ), 11 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 3, 1 ), 12 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 3, 2 ), 13 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 4, 0 ), 14 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 4, 1 ), 15 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 4, 2 ), 16 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 5, 0 ), 17 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 5, 1 ), 18 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 6, 0 ), 19 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 6, 1 ), 20 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 7, 0 ), 21 );

        // Very loose bounds for neurons except inputs
        double large = 1000000;

        tableau.getBoundManager().initialize( 22 );
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
        tableau.setLowerBound( 17, -large );
        tableau.setUpperBound( 17, large );
        tableau.setLowerBound( 18, -large );
        tableau.setUpperBound( 18, large );
        tableau.setLowerBound( 19, -large );
        tableau.setUpperBound( 19, large );
        tableau.setLowerBound( 20, -large );
        tableau.setUpperBound( 20, large );
        tableau.setLowerBound( 21, -large );
        tableau.setUpperBound( 21, large );
    }

    void populateNetworkBackwardSigmoid( NLR::NetworkLevelReasoner &nlr, MockTableau &tableau )
    {
        /*

              1      S      -1      S      -1  2
          x0 --- x2 ---> x4 --- x6 ---> x8 --- x10
            \    /        \    /          \    /
           1 \  /        2 \  /          1 \  /
              \/            \/              \/
              /\            /\              /\
          -1 /  \        1 /  \         -1 /  \
            /    \   S    /    \    S     /    \
          x1 --- x3 ---> x5 --- x7 ---> x9 --- x11
              1             -1              2

          The example described in Fig. 2 of
          https://dl.acm.org/doi/10.1145/3563325
          using Sigmoid activation instead of LeakyReLU
        */

        // Create the layers
        nlr.addLayer( 0, NLR::Layer::INPUT, 2 );
        nlr.addLayer( 1, NLR::Layer::WEIGHTED_SUM, 2 );
        nlr.addLayer( 2, NLR::Layer::SIGMOID, 2 );
        nlr.addLayer( 3, NLR::Layer::WEIGHTED_SUM, 2 );
        nlr.addLayer( 4, NLR::Layer::SIGMOID, 2 );
        nlr.addLayer( 5, NLR::Layer::WEIGHTED_SUM, 2 );

        // Mark layer dependencies
        for ( unsigned i = 1; i <= 5; ++i )
            nlr.addLayerDependency( i - 1, i );

        // Set the weights and biases for the weighted sum layers
        nlr.setWeight( 0, 0, 1, 0, 1 );
        nlr.setWeight( 0, 0, 1, 1, 1 );
        nlr.setWeight( 0, 1, 1, 0, -1 );
        nlr.setWeight( 0, 1, 1, 1, 1 );

        nlr.setWeight( 2, 0, 3, 0, -1 );
        nlr.setWeight( 2, 0, 3, 1, 2 );
        nlr.setWeight( 2, 1, 3, 0, 1 );
        nlr.setWeight( 2, 1, 3, 1, -1 );

        nlr.setWeight( 4, 0, 5, 0, -1 );
        nlr.setWeight( 4, 0, 5, 1, 1 );
        nlr.setWeight( 4, 1, 5, 0, -1 );
        nlr.setWeight( 4, 1, 5, 1, 2 );

        nlr.setBias( 5, 1, 2 );

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

    void populateNetworkBackwardSigmoid2( NLR::NetworkLevelReasoner &nlr, MockTableau &tableau )
    {
        /*
                x3    x7
          x0                 x11    x14
                x4    x8                   x17    x19
          x1                 x12    x15                  x21
                x5    x9                   x18    x20
          x2                 x13    x16
                x6    x10

          x3 = -x0 + x1
          x4 = x0 + 2*x1
          x5 = x0 + x1 + x2
          x6 = 3*x0 - 2*x1 - x2

          x7 = Sigmoid( x3 )
          x8 = Sigmoid( x4 )
          x9 = Sigmoid( x5 )
          x10 = Sigmoid( x6 )

          x11 = 2x7 + x9 - x10 + 2
          x12 = -x7 + 2x8 + x10
          x13 = x7 - x8 + 2x9 - 2

          x14 = Sigmoid( x11 )
          x15 = Sigmoid( x12 )
          x16 = Sigmoid( x13 )

          x17 = -x14 + x15 - x16
          x18 = x14 + x15 + x16

          x19 = Sigmoid( x17 )
          x20 = Sigmoid( x18 )

          x21 = x19 - x20 - 1
        */

        // Create the layers
        nlr.addLayer( 0, NLR::Layer::INPUT, 3 );
        nlr.addLayer( 1, NLR::Layer::WEIGHTED_SUM, 4 );
        nlr.addLayer( 2, NLR::Layer::SIGMOID, 4 );
        nlr.addLayer( 3, NLR::Layer::WEIGHTED_SUM, 3 );
        nlr.addLayer( 4, NLR::Layer::SIGMOID, 3 );
        nlr.addLayer( 5, NLR::Layer::WEIGHTED_SUM, 2 );
        nlr.addLayer( 6, NLR::Layer::SIGMOID, 2 );
        nlr.addLayer( 7, NLR::Layer::WEIGHTED_SUM, 1 );

        // Mark layer dependencies
        for ( unsigned i = 1; i <= 7; ++i )
            nlr.addLayerDependency( i - 1, i );

        // Set the weights and biases for the weighted sum layers
        nlr.setWeight( 0, 0, 1, 0, -1 );
        nlr.setWeight( 0, 0, 1, 1, 1 );
        nlr.setWeight( 0, 0, 1, 2, 1 );
        nlr.setWeight( 0, 0, 1, 3, 3 );
        nlr.setWeight( 0, 1, 1, 0, 1 );
        nlr.setWeight( 0, 1, 1, 1, 2 );
        nlr.setWeight( 0, 1, 1, 2, 1 );
        nlr.setWeight( 0, 1, 1, 3, -2 );
        nlr.setWeight( 0, 2, 1, 2, 1 );
        nlr.setWeight( 0, 2, 1, 3, -1 );

        nlr.setWeight( 2, 0, 3, 0, 2 );
        nlr.setWeight( 2, 0, 3, 1, -1 );
        nlr.setWeight( 2, 0, 3, 2, 1 );
        nlr.setWeight( 2, 1, 3, 1, 2 );
        nlr.setWeight( 2, 1, 3, 2, -1 );
        nlr.setWeight( 2, 2, 3, 0, 1 );
        nlr.setWeight( 2, 2, 3, 2, 2 );
        nlr.setWeight( 2, 3, 3, 0, -1 );
        nlr.setWeight( 2, 3, 3, 1, 1 );

        nlr.setWeight( 4, 0, 5, 0, -1 );
        nlr.setWeight( 4, 0, 5, 1, 1 );
        nlr.setWeight( 4, 1, 5, 0, 1 );
        nlr.setWeight( 4, 1, 5, 1, 1 );
        nlr.setWeight( 4, 2, 5, 0, -1 );
        nlr.setWeight( 4, 2, 5, 1, 1 );

        nlr.setWeight( 6, 0, 7, 0, 1 );
        nlr.setWeight( 6, 1, 7, 0, -1 );

        nlr.setBias( 3, 0, 2 );
        nlr.setBias( 3, 2, -2 );
        nlr.setBias( 7, 0, -1 );

        // Mark the Sigmoid sources
        nlr.addActivationSource( 1, 0, 2, 0 );
        nlr.addActivationSource( 1, 1, 2, 1 );
        nlr.addActivationSource( 1, 2, 2, 2 );
        nlr.addActivationSource( 1, 3, 2, 3 );

        nlr.addActivationSource( 3, 0, 4, 0 );
        nlr.addActivationSource( 3, 1, 4, 1 );
        nlr.addActivationSource( 3, 2, 4, 2 );

        nlr.addActivationSource( 5, 0, 6, 0 );
        nlr.addActivationSource( 5, 1, 6, 1 );

        // Variable indexing
        nlr.setNeuronVariable( NLR::NeuronIndex( 0, 0 ), 0 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 0, 1 ), 1 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 0, 2 ), 2 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 1, 0 ), 3 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 1, 1 ), 4 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 1, 2 ), 5 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 1, 3 ), 6 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 2, 0 ), 7 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 2, 1 ), 8 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 2, 2 ), 9 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 2, 3 ), 10 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 3, 0 ), 11 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 3, 1 ), 12 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 3, 2 ), 13 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 4, 0 ), 14 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 4, 1 ), 15 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 4, 2 ), 16 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 5, 0 ), 17 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 5, 1 ), 18 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 6, 0 ), 19 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 6, 1 ), 20 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 7, 0 ), 21 );

        // Very loose bounds for neurons except inputs
        double large = 1000000;

        tableau.getBoundManager().initialize( 22 );
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
        tableau.setLowerBound( 17, -large );
        tableau.setUpperBound( 17, large );
        tableau.setLowerBound( 18, -large );
        tableau.setUpperBound( 18, large );
        tableau.setLowerBound( 19, -large );
        tableau.setUpperBound( 19, large );
        tableau.setLowerBound( 20, -large );
        tableau.setUpperBound( 20, large );
        tableau.setLowerBound( 21, -large );
        tableau.setUpperBound( 21, large );
    }

    void populateNetworkBackwardSign( NLR::NetworkLevelReasoner &nlr, MockTableau &tableau )
    {
        /*

              1     Sign    -1     Sign    -1  2
          x0 --- x2 ---> x4 --- x6 ---> x8 --- x10
            \    /        \    /          \    /
           1 \  /        2 \  /          1 \  /
              \/            \/              \/
              /\            /\              /\
          -1 /  \        1 /  \         -1 /  \
            /    \  Sign  /    \   Sign   /    \
          x1 --- x3 ---> x5 --- x7 ---> x9 --- x11
              1             -1              2

          The example described in Fig. 2 of
          https://dl.acm.org/doi/10.1145/3563325
          using Sign activation instead of LeakyReLU
        */

        // Create the layers
        nlr.addLayer( 0, NLR::Layer::INPUT, 2 );
        nlr.addLayer( 1, NLR::Layer::WEIGHTED_SUM, 2 );
        nlr.addLayer( 2, NLR::Layer::SIGN, 2 );
        nlr.addLayer( 3, NLR::Layer::WEIGHTED_SUM, 2 );
        nlr.addLayer( 4, NLR::Layer::SIGN, 2 );
        nlr.addLayer( 5, NLR::Layer::WEIGHTED_SUM, 2 );

        // Mark layer dependencies
        for ( unsigned i = 1; i <= 5; ++i )
            nlr.addLayerDependency( i - 1, i );

        // Set the weights and biases for the weighted sum layers
        nlr.setWeight( 0, 0, 1, 0, 1 );
        nlr.setWeight( 0, 0, 1, 1, 1 );
        nlr.setWeight( 0, 1, 1, 0, -1 );
        nlr.setWeight( 0, 1, 1, 1, 1 );

        nlr.setWeight( 2, 0, 3, 0, -1 );
        nlr.setWeight( 2, 0, 3, 1, 2 );
        nlr.setWeight( 2, 1, 3, 0, 1 );
        nlr.setWeight( 2, 1, 3, 1, -1 );

        nlr.setWeight( 4, 0, 5, 0, -1 );
        nlr.setWeight( 4, 0, 5, 1, 1 );
        nlr.setWeight( 4, 1, 5, 0, -1 );
        nlr.setWeight( 4, 1, 5, 1, 2 );

        nlr.setBias( 5, 1, 2 );

        // Mark the Sign sources
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

    void populateNetworkBackwardSign2( NLR::NetworkLevelReasoner &nlr, MockTableau &tableau )
    {
        /*
                x3    x7
          x0                 x11    x14
                x4    x8                   x17    x19
          x1                 x12    x15                  x21
                x5    x9                   x18    x20
          x2                 x13    x16
                x6    x10

          x3 = -x0 + x1
          x4 = x0 + 2*x1
          x5 = x0 + x1 + x2
          x6 = 3*x0 - 2*x1 - x2

          x7 = Sign( x3 )
          x8 = Sign( x4 )
          x9 = SIgn( x5 )
          x10 = Sign( x6 )

          x11 = 2x7 + x9 - x10 + 2
          x12 = -x7 + 2x8 + x10
          x13 = x7 - x8 + 2x9 - 2

          x14 = Sign( x11 )
          x15 = Sign( x12 )
          x16 = Sign( x13 )

          x17 = -x14 + x15 - x16
          x18 = x14 + x15 + x16

          x19 = Sign( x17 )
          x20 = Sign( x18 )

          x21 = x19 - x20 - 1
        */

        // Create the layers
        nlr.addLayer( 0, NLR::Layer::INPUT, 3 );
        nlr.addLayer( 1, NLR::Layer::WEIGHTED_SUM, 4 );
        nlr.addLayer( 2, NLR::Layer::SIGN, 4 );
        nlr.addLayer( 3, NLR::Layer::WEIGHTED_SUM, 3 );
        nlr.addLayer( 4, NLR::Layer::SIGN, 3 );
        nlr.addLayer( 5, NLR::Layer::WEIGHTED_SUM, 2 );
        nlr.addLayer( 6, NLR::Layer::SIGN, 2 );
        nlr.addLayer( 7, NLR::Layer::WEIGHTED_SUM, 1 );

        // Mark layer dependencies
        for ( unsigned i = 1; i <= 7; ++i )
            nlr.addLayerDependency( i - 1, i );

        // Set the weights and biases for the weighted sum layers
        nlr.setWeight( 0, 0, 1, 0, -1 );
        nlr.setWeight( 0, 0, 1, 1, 1 );
        nlr.setWeight( 0, 0, 1, 2, 1 );
        nlr.setWeight( 0, 0, 1, 3, 3 );
        nlr.setWeight( 0, 1, 1, 0, 1 );
        nlr.setWeight( 0, 1, 1, 1, 2 );
        nlr.setWeight( 0, 1, 1, 2, 1 );
        nlr.setWeight( 0, 1, 1, 3, -2 );
        nlr.setWeight( 0, 2, 1, 2, 1 );
        nlr.setWeight( 0, 2, 1, 3, -1 );

        nlr.setWeight( 2, 0, 3, 0, 2 );
        nlr.setWeight( 2, 0, 3, 1, -1 );
        nlr.setWeight( 2, 0, 3, 2, 1 );
        nlr.setWeight( 2, 1, 3, 1, 2 );
        nlr.setWeight( 2, 1, 3, 2, -1 );
        nlr.setWeight( 2, 2, 3, 0, 1 );
        nlr.setWeight( 2, 2, 3, 2, 2 );
        nlr.setWeight( 2, 3, 3, 0, -1 );
        nlr.setWeight( 2, 3, 3, 1, 1 );

        nlr.setWeight( 4, 0, 5, 0, -1 );
        nlr.setWeight( 4, 0, 5, 1, 1 );
        nlr.setWeight( 4, 1, 5, 0, 1 );
        nlr.setWeight( 4, 1, 5, 1, 1 );
        nlr.setWeight( 4, 2, 5, 0, -1 );
        nlr.setWeight( 4, 2, 5, 1, 1 );

        nlr.setWeight( 6, 0, 7, 0, 1 );
        nlr.setWeight( 6, 1, 7, 0, -1 );

        nlr.setBias( 3, 0, 2 );
        nlr.setBias( 3, 2, -2 );
        nlr.setBias( 7, 0, -1 );

        // Mark the Sign sources
        nlr.addActivationSource( 1, 0, 2, 0 );
        nlr.addActivationSource( 1, 1, 2, 1 );
        nlr.addActivationSource( 1, 2, 2, 2 );
        nlr.addActivationSource( 1, 3, 2, 3 );

        nlr.addActivationSource( 3, 0, 4, 0 );
        nlr.addActivationSource( 3, 1, 4, 1 );
        nlr.addActivationSource( 3, 2, 4, 2 );

        nlr.addActivationSource( 5, 0, 6, 0 );
        nlr.addActivationSource( 5, 1, 6, 1 );

        // Variable indexing
        nlr.setNeuronVariable( NLR::NeuronIndex( 0, 0 ), 0 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 0, 1 ), 1 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 0, 2 ), 2 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 1, 0 ), 3 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 1, 1 ), 4 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 1, 2 ), 5 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 1, 3 ), 6 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 2, 0 ), 7 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 2, 1 ), 8 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 2, 2 ), 9 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 2, 3 ), 10 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 3, 0 ), 11 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 3, 1 ), 12 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 3, 2 ), 13 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 4, 0 ), 14 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 4, 1 ), 15 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 4, 2 ), 16 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 5, 0 ), 17 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 5, 1 ), 18 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 6, 0 ), 19 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 6, 1 ), 20 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 7, 0 ), 21 );

        // Very loose bounds for neurons except inputs
        double large = 1000000;

        tableau.getBoundManager().initialize( 22 );
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
        tableau.setLowerBound( 17, -large );
        tableau.setUpperBound( 17, large );
        tableau.setLowerBound( 18, -large );
        tableau.setUpperBound( 18, large );
        tableau.setLowerBound( 19, -large );
        tableau.setUpperBound( 19, large );
        tableau.setLowerBound( 20, -large );
        tableau.setUpperBound( 20, large );
        tableau.setLowerBound( 21, -large );
        tableau.setUpperBound( 21, large );
    }

    void populateNetworkBackwardRound( NLR::NetworkLevelReasoner &nlr, MockTableau &tableau )
    {
        /*

              1     Rnd     -1     Rnd     -1  2
          x0 --- x2 ---> x4 --- x6 ---> x8 --- x10
            \    /        \    /          \    /
           1 \  /        2 \  /          1 \  /
              \/            \/              \/
              /\            /\              /\
          -1 /  \        1 /  \         -1 /  \
            /    \  Rnd   /    \   Rnd    /    \
          x1 --- x3 ---> x5 --- x7 ---> x9 --- x11
              1             -1              2

          The example described in Fig. 2 of
          https://dl.acm.org/doi/10.1145/3563325
          using Round activation instead of LeakyReLU
        */

        // Create the layers
        nlr.addLayer( 0, NLR::Layer::INPUT, 2 );
        nlr.addLayer( 1, NLR::Layer::WEIGHTED_SUM, 2 );
        nlr.addLayer( 2, NLR::Layer::ROUND, 2 );
        nlr.addLayer( 3, NLR::Layer::WEIGHTED_SUM, 2 );
        nlr.addLayer( 4, NLR::Layer::ROUND, 2 );
        nlr.addLayer( 5, NLR::Layer::WEIGHTED_SUM, 2 );

        // Mark layer dependencies
        for ( unsigned i = 1; i <= 5; ++i )
            nlr.addLayerDependency( i - 1, i );

        // Set the weights and biases for the weighted sum layers
        nlr.setWeight( 0, 0, 1, 0, 1 );
        nlr.setWeight( 0, 0, 1, 1, 1 );
        nlr.setWeight( 0, 1, 1, 0, -1 );
        nlr.setWeight( 0, 1, 1, 1, 1 );

        nlr.setWeight( 2, 0, 3, 0, -1 );
        nlr.setWeight( 2, 0, 3, 1, 2 );
        nlr.setWeight( 2, 1, 3, 0, 1 );
        nlr.setWeight( 2, 1, 3, 1, -1 );

        nlr.setWeight( 4, 0, 5, 0, -1 );
        nlr.setWeight( 4, 0, 5, 1, 1 );
        nlr.setWeight( 4, 1, 5, 0, -1 );
        nlr.setWeight( 4, 1, 5, 1, 2 );

        nlr.setBias( 5, 1, 2 );

        // Mark the Round sources
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

    void populateNetworkBackwardRound2( NLR::NetworkLevelReasoner &nlr, MockTableau &tableau )
    {
        /*
                x3    x7
          x0                 x11    x14
                x4    x8                   x17    x19
          x1                 x12    x15                  x21
                x5    x9                   x18    x20
          x2                 x13    x16
                x6    x10

          x3 = -x0 + x1
          x4 = x0 + 2*x1
          x5 = x0 + x1 + x2
          x6 = 3*x0 - 2*x1 - x2

          x7 = Round( x3 )
          x8 = Round( x4 )
          x9 = Round( x5 )
          x10 = Round( x6 )

          x11 = 2x7 + x9 - x10 + 2
          x12 = -x7 + 2x8 + x10
          x13 = x7 - x8 + 2x9 - 2

          x14 = Round( x11 )
          x15 = Round( x12 )
          x16 = Round( x13 )

          x17 = -x14 + x15 - x16
          x18 = x14 + x15 + x16

          x19 = Round( x17 )
          x20 = Round( x18 )

          x21 = x19 - x20 - 1
        */

        // Create the layers
        nlr.addLayer( 0, NLR::Layer::INPUT, 3 );
        nlr.addLayer( 1, NLR::Layer::WEIGHTED_SUM, 4 );
        nlr.addLayer( 2, NLR::Layer::ROUND, 4 );
        nlr.addLayer( 3, NLR::Layer::WEIGHTED_SUM, 3 );
        nlr.addLayer( 4, NLR::Layer::ROUND, 3 );
        nlr.addLayer( 5, NLR::Layer::WEIGHTED_SUM, 2 );
        nlr.addLayer( 6, NLR::Layer::ROUND, 2 );
        nlr.addLayer( 7, NLR::Layer::WEIGHTED_SUM, 1 );

        // Mark layer dependencies
        for ( unsigned i = 1; i <= 7; ++i )
            nlr.addLayerDependency( i - 1, i );

        // Set the weights and biases for the weighted sum layers
        nlr.setWeight( 0, 0, 1, 0, -1 );
        nlr.setWeight( 0, 0, 1, 1, 1 );
        nlr.setWeight( 0, 0, 1, 2, 1 );
        nlr.setWeight( 0, 0, 1, 3, 3 );
        nlr.setWeight( 0, 1, 1, 0, 1 );
        nlr.setWeight( 0, 1, 1, 1, 2 );
        nlr.setWeight( 0, 1, 1, 2, 1 );
        nlr.setWeight( 0, 1, 1, 3, -2 );
        nlr.setWeight( 0, 2, 1, 2, 1 );
        nlr.setWeight( 0, 2, 1, 3, -1 );

        nlr.setWeight( 2, 0, 3, 0, 2 );
        nlr.setWeight( 2, 0, 3, 1, -1 );
        nlr.setWeight( 2, 0, 3, 2, 1 );
        nlr.setWeight( 2, 1, 3, 1, 2 );
        nlr.setWeight( 2, 1, 3, 2, -1 );
        nlr.setWeight( 2, 2, 3, 0, 1 );
        nlr.setWeight( 2, 2, 3, 2, 2 );
        nlr.setWeight( 2, 3, 3, 0, -1 );
        nlr.setWeight( 2, 3, 3, 1, 1 );

        nlr.setWeight( 4, 0, 5, 0, -1 );
        nlr.setWeight( 4, 0, 5, 1, 1 );
        nlr.setWeight( 4, 1, 5, 0, 1 );
        nlr.setWeight( 4, 1, 5, 1, 1 );
        nlr.setWeight( 4, 2, 5, 0, -1 );
        nlr.setWeight( 4, 2, 5, 1, 1 );

        nlr.setWeight( 6, 0, 7, 0, 1 );
        nlr.setWeight( 6, 1, 7, 0, -1 );

        nlr.setBias( 3, 0, 2 );
        nlr.setBias( 3, 2, -2 );
        nlr.setBias( 7, 0, -1 );

        // Mark the Round sources
        nlr.addActivationSource( 1, 0, 2, 0 );
        nlr.addActivationSource( 1, 1, 2, 1 );
        nlr.addActivationSource( 1, 2, 2, 2 );
        nlr.addActivationSource( 1, 3, 2, 3 );

        nlr.addActivationSource( 3, 0, 4, 0 );
        nlr.addActivationSource( 3, 1, 4, 1 );
        nlr.addActivationSource( 3, 2, 4, 2 );

        nlr.addActivationSource( 5, 0, 6, 0 );
        nlr.addActivationSource( 5, 1, 6, 1 );

        // Variable indexing
        nlr.setNeuronVariable( NLR::NeuronIndex( 0, 0 ), 0 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 0, 1 ), 1 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 0, 2 ), 2 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 1, 0 ), 3 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 1, 1 ), 4 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 1, 2 ), 5 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 1, 3 ), 6 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 2, 0 ), 7 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 2, 1 ), 8 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 2, 2 ), 9 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 2, 3 ), 10 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 3, 0 ), 11 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 3, 1 ), 12 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 3, 2 ), 13 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 4, 0 ), 14 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 4, 1 ), 15 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 4, 2 ), 16 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 5, 0 ), 17 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 5, 1 ), 18 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 6, 0 ), 19 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 6, 1 ), 20 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 7, 0 ), 21 );

        // Very loose bounds for neurons except inputs
        double large = 1000000;

        tableau.getBoundManager().initialize( 22 );
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
        tableau.setLowerBound( 17, -large );
        tableau.setUpperBound( 17, large );
        tableau.setLowerBound( 18, -large );
        tableau.setUpperBound( 18, large );
        tableau.setLowerBound( 19, -large );
        tableau.setUpperBound( 19, large );
        tableau.setLowerBound( 20, -large );
        tableau.setUpperBound( 20, large );
        tableau.setLowerBound( 21, -large );
        tableau.setUpperBound( 21, large );
    }

    void populateNetworkBackwardAbs( NLR::NetworkLevelReasoner &nlr, MockTableau &tableau )
    {
        /*

              1      A      -1      A      -1  2
          x0 --- x2 ---> x4 --- x6 ---> x8 --- x10
            \    /        \    /          \    /
           1 \  /        2 \  /          1 \  /
              \/            \/              \/
              /\            /\              /\
          -1 /  \        1 /  \         -1 /  \
            /    \   A    /    \    A     /    \
          x1 --- x3 ---> x5 --- x7 ---> x9 --- x11
              1             -1              2

          The example described in Fig. 2 of
          https://dl.acm.org/doi/10.1145/3563325
          using Absolute value activation instead of LeakyReLU
        */

        // Create the layers
        nlr.addLayer( 0, NLR::Layer::INPUT, 2 );
        nlr.addLayer( 1, NLR::Layer::WEIGHTED_SUM, 2 );
        nlr.addLayer( 2, NLR::Layer::ABSOLUTE_VALUE, 2 );
        nlr.addLayer( 3, NLR::Layer::WEIGHTED_SUM, 2 );
        nlr.addLayer( 4, NLR::Layer::ABSOLUTE_VALUE, 2 );
        nlr.addLayer( 5, NLR::Layer::WEIGHTED_SUM, 2 );

        // Mark layer dependencies
        for ( unsigned i = 1; i <= 5; ++i )
            nlr.addLayerDependency( i - 1, i );

        // Set the weights and biases for the weighted sum layers
        nlr.setWeight( 0, 0, 1, 0, 1 );
        nlr.setWeight( 0, 0, 1, 1, 1 );
        nlr.setWeight( 0, 1, 1, 0, -1 );
        nlr.setWeight( 0, 1, 1, 1, 1 );

        nlr.setWeight( 2, 0, 3, 0, -1 );
        nlr.setWeight( 2, 0, 3, 1, 2 );
        nlr.setWeight( 2, 1, 3, 0, 1 );
        nlr.setWeight( 2, 1, 3, 1, -1 );

        nlr.setWeight( 4, 0, 5, 0, -1 );
        nlr.setWeight( 4, 0, 5, 1, 1 );
        nlr.setWeight( 4, 1, 5, 0, -1 );
        nlr.setWeight( 4, 1, 5, 1, 2 );

        nlr.setBias( 5, 1, 2 );

        // Mark the Abs sources
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

    void populateNetworkBackwardAbs2( NLR::NetworkLevelReasoner &nlr, MockTableau &tableau )
    {
        /*
                x3    x7
          x0                 x11    x14
                x4    x8                   x17    x19
          x1                 x12    x15                  x21
                x5    x9                   x18    x20
          x2                 x13    x16
                x6    x10

          x3 = -x0 + x1
          x4 = x0 + 2*x1
          x5 = x0 + x1 + x2
          x6 = 3*x0 - 2*x1 - x2

          x7 = Abs( x3 )
          x8 = Abs( x4 )
          x9 = Abs( x5 )
          x10 = Abs( x6 )

          x11 = 2x7 + x9 - x10 + 2
          x12 = -x7 + 2x8 + x10
          x13 = x7 - x8 + 2x9 - 2

          x14 = Abs( x11 )
          x15 = Abs( x12 )
          x16 = Abs( x13 )

          x17 = -x14 + x15 - x16
          x18 = x14 + x15 + x16

          x19 = Abs( x17 )
          x20 = Abs( x18 )

          x21 = x19 - x20 - 1
        */

        // Create the layers
        nlr.addLayer( 0, NLR::Layer::INPUT, 3 );
        nlr.addLayer( 1, NLR::Layer::WEIGHTED_SUM, 4 );
        nlr.addLayer( 2, NLR::Layer::ABSOLUTE_VALUE, 4 );
        nlr.addLayer( 3, NLR::Layer::WEIGHTED_SUM, 3 );
        nlr.addLayer( 4, NLR::Layer::ABSOLUTE_VALUE, 3 );
        nlr.addLayer( 5, NLR::Layer::WEIGHTED_SUM, 2 );
        nlr.addLayer( 6, NLR::Layer::ABSOLUTE_VALUE, 2 );
        nlr.addLayer( 7, NLR::Layer::WEIGHTED_SUM, 1 );

        // Mark layer dependencies
        for ( unsigned i = 1; i <= 7; ++i )
            nlr.addLayerDependency( i - 1, i );

        // Set the weights and biases for the weighted sum layers
        nlr.setWeight( 0, 0, 1, 0, -1 );
        nlr.setWeight( 0, 0, 1, 1, 1 );
        nlr.setWeight( 0, 0, 1, 2, 1 );
        nlr.setWeight( 0, 0, 1, 3, 3 );
        nlr.setWeight( 0, 1, 1, 0, 1 );
        nlr.setWeight( 0, 1, 1, 1, 2 );
        nlr.setWeight( 0, 1, 1, 2, 1 );
        nlr.setWeight( 0, 1, 1, 3, -2 );
        nlr.setWeight( 0, 2, 1, 2, 1 );
        nlr.setWeight( 0, 2, 1, 3, -1 );

        nlr.setWeight( 2, 0, 3, 0, 2 );
        nlr.setWeight( 2, 0, 3, 1, -1 );
        nlr.setWeight( 2, 0, 3, 2, 1 );
        nlr.setWeight( 2, 1, 3, 1, 2 );
        nlr.setWeight( 2, 1, 3, 2, -1 );
        nlr.setWeight( 2, 2, 3, 0, 1 );
        nlr.setWeight( 2, 2, 3, 2, 2 );
        nlr.setWeight( 2, 3, 3, 0, -1 );
        nlr.setWeight( 2, 3, 3, 1, 1 );

        nlr.setWeight( 4, 0, 5, 0, -1 );
        nlr.setWeight( 4, 0, 5, 1, 1 );
        nlr.setWeight( 4, 1, 5, 0, 1 );
        nlr.setWeight( 4, 1, 5, 1, 1 );
        nlr.setWeight( 4, 2, 5, 0, -1 );
        nlr.setWeight( 4, 2, 5, 1, 1 );

        nlr.setWeight( 6, 0, 7, 0, 1 );
        nlr.setWeight( 6, 1, 7, 0, -1 );

        nlr.setBias( 3, 0, 2 );
        nlr.setBias( 3, 2, -2 );
        nlr.setBias( 7, 0, -1 );

        // Mark the Abs sources
        nlr.addActivationSource( 1, 0, 2, 0 );
        nlr.addActivationSource( 1, 1, 2, 1 );
        nlr.addActivationSource( 1, 2, 2, 2 );
        nlr.addActivationSource( 1, 3, 2, 3 );

        nlr.addActivationSource( 3, 0, 4, 0 );
        nlr.addActivationSource( 3, 1, 4, 1 );
        nlr.addActivationSource( 3, 2, 4, 2 );

        nlr.addActivationSource( 5, 0, 6, 0 );
        nlr.addActivationSource( 5, 1, 6, 1 );

        // Variable indexing
        nlr.setNeuronVariable( NLR::NeuronIndex( 0, 0 ), 0 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 0, 1 ), 1 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 0, 2 ), 2 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 1, 0 ), 3 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 1, 1 ), 4 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 1, 2 ), 5 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 1, 3 ), 6 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 2, 0 ), 7 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 2, 1 ), 8 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 2, 2 ), 9 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 2, 3 ), 10 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 3, 0 ), 11 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 3, 1 ), 12 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 3, 2 ), 13 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 4, 0 ), 14 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 4, 1 ), 15 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 4, 2 ), 16 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 5, 0 ), 17 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 5, 1 ), 18 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 6, 0 ), 19 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 6, 1 ), 20 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 7, 0 ), 21 );

        // Very loose bounds for neurons except inputs
        double large = 1000000;

        tableau.getBoundManager().initialize( 22 );
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
        tableau.setLowerBound( 17, -large );
        tableau.setUpperBound( 17, large );
        tableau.setLowerBound( 18, -large );
        tableau.setUpperBound( 18, large );
        tableau.setLowerBound( 19, -large );
        tableau.setUpperBound( 19, large );
        tableau.setLowerBound( 20, -large );
        tableau.setUpperBound( 20, large );
        tableau.setLowerBound( 21, -large );
        tableau.setUpperBound( 21, large );
    }

    void populateNetworkBackwardLeakyReLU( NLR::NetworkLevelReasoner &nlr, MockTableau &tableau )
    {
        /*

              1      LR     -1      LR     -1  2
          x0 --- x2 ---> x4 --- x6 ---> x8 --- x10
            \    /        \    /          \    /
           1 \  /        2 \  /          1 \  /
              \/            \/              \/
              /\            /\              /\
          -1 /  \        1 /  \         -1 /  \
            /    \   LR   /    \    LR    /    \
          x1 --- x3 ---> x5 --- x7 ---> x9 --- x11
              1             -1              2

          The example described in Fig. 2 of
          https://dl.acm.org/doi/10.1145/3563325
        */

        // Create the layers
        nlr.addLayer( 0, NLR::Layer::INPUT, 2 );
        nlr.addLayer( 1, NLR::Layer::WEIGHTED_SUM, 2 );
        nlr.addLayer( 2, NLR::Layer::LEAKY_RELU, 2 );
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
        nlr.setWeight( 0, 0, 1, 1, 1 );
        nlr.setWeight( 0, 1, 1, 0, -1 );
        nlr.setWeight( 0, 1, 1, 1, 1 );

        nlr.setWeight( 2, 0, 3, 0, -1 );
        nlr.setWeight( 2, 0, 3, 1, 2 );
        nlr.setWeight( 2, 1, 3, 0, 1 );
        nlr.setWeight( 2, 1, 3, 1, -1 );

        nlr.setWeight( 4, 0, 5, 0, -1 );
        nlr.setWeight( 4, 0, 5, 1, 1 );
        nlr.setWeight( 4, 1, 5, 0, -1 );
        nlr.setWeight( 4, 1, 5, 1, 2 );

        nlr.setBias( 5, 1, 2 );

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

    void populateNetworkBackwardLeakyRelu2( NLR::NetworkLevelReasoner &nlr, MockTableau &tableau )
    {
        /*
                x3    x7
          x0                 x11    x14
                x4    x8                   x17    x19
          x1                 x12    x15                  x21
                x5    x9                   x18    x20
          x2                 x13    x16
                x6    x10

          x3 = -x0 + x1
          x4 = x0 + 2*x1
          x5 = x0 + x1 + x2
          x6 = 3*x0 - 2*x1 - x2

          x7 = LeakyReLU( x3 )
          x8 = LeakyReLU( x4 )
          x9 = LeakyReLU( x5 )
          x10 = LeakyReLU( x6 )

          x11 = 2x7 + x9 - x10 + 2
          x12 = -x7 + 2x8 + x10
          x13 = x7 - x8 + 2x9 - 2

          x14 = LeakyReLU( x11 )
          x15 = LeakyReLU( x12 )
          x16 = LeakyReLU( x13 )

          x17 = -x14 + x15 - x16
          x18 = x14 + x15 + x16

          x19 = LeakyReLU( x17 )
          x20 = LeakyReLU( x18 )

          x21 = x19 - x20 - 1
        */

        // Create the layers
        nlr.addLayer( 0, NLR::Layer::INPUT, 3 );
        nlr.addLayer( 1, NLR::Layer::WEIGHTED_SUM, 4 );
        nlr.addLayer( 2, NLR::Layer::LEAKY_RELU, 4 );
        nlr.addLayer( 3, NLR::Layer::WEIGHTED_SUM, 3 );
        nlr.addLayer( 4, NLR::Layer::LEAKY_RELU, 3 );
        nlr.addLayer( 5, NLR::Layer::WEIGHTED_SUM, 2 );
        nlr.addLayer( 6, NLR::Layer::LEAKY_RELU, 2 );
        nlr.addLayer( 7, NLR::Layer::WEIGHTED_SUM, 1 );

        nlr.getLayer( 2 )->setAlpha( 0.1 );
        nlr.getLayer( 4 )->setAlpha( 0.1 );
        nlr.getLayer( 6 )->setAlpha( 0.1 );

        // Mark layer dependencies
        for ( unsigned i = 1; i <= 7; ++i )
            nlr.addLayerDependency( i - 1, i );

        // Set the weights and biases for the weighted sum layers
        nlr.setWeight( 0, 0, 1, 0, -1 );
        nlr.setWeight( 0, 0, 1, 1, 1 );
        nlr.setWeight( 0, 0, 1, 2, 1 );
        nlr.setWeight( 0, 0, 1, 3, 3 );
        nlr.setWeight( 0, 1, 1, 0, 1 );
        nlr.setWeight( 0, 1, 1, 1, 2 );
        nlr.setWeight( 0, 1, 1, 2, 1 );
        nlr.setWeight( 0, 1, 1, 3, -2 );
        nlr.setWeight( 0, 2, 1, 2, 1 );
        nlr.setWeight( 0, 2, 1, 3, -1 );

        nlr.setWeight( 2, 0, 3, 0, 2 );
        nlr.setWeight( 2, 0, 3, 1, -1 );
        nlr.setWeight( 2, 0, 3, 2, 1 );
        nlr.setWeight( 2, 1, 3, 1, 2 );
        nlr.setWeight( 2, 1, 3, 2, -1 );
        nlr.setWeight( 2, 2, 3, 0, 1 );
        nlr.setWeight( 2, 2, 3, 2, 2 );
        nlr.setWeight( 2, 3, 3, 0, -1 );
        nlr.setWeight( 2, 3, 3, 1, 1 );

        nlr.setWeight( 4, 0, 5, 0, -1 );
        nlr.setWeight( 4, 0, 5, 1, 1 );
        nlr.setWeight( 4, 1, 5, 0, 1 );
        nlr.setWeight( 4, 1, 5, 1, 1 );
        nlr.setWeight( 4, 2, 5, 0, -1 );
        nlr.setWeight( 4, 2, 5, 1, 1 );

        nlr.setWeight( 6, 0, 7, 0, 1 );
        nlr.setWeight( 6, 1, 7, 0, -1 );

        nlr.setBias( 3, 0, 2 );
        nlr.setBias( 3, 2, -2 );
        nlr.setBias( 7, 0, -1 );

        // Mark the LeakyReLU sources
        nlr.addActivationSource( 1, 0, 2, 0 );
        nlr.addActivationSource( 1, 1, 2, 1 );
        nlr.addActivationSource( 1, 2, 2, 2 );
        nlr.addActivationSource( 1, 3, 2, 3 );

        nlr.addActivationSource( 3, 0, 4, 0 );
        nlr.addActivationSource( 3, 1, 4, 1 );
        nlr.addActivationSource( 3, 2, 4, 2 );

        nlr.addActivationSource( 5, 0, 6, 0 );
        nlr.addActivationSource( 5, 1, 6, 1 );

        // Variable indexing
        nlr.setNeuronVariable( NLR::NeuronIndex( 0, 0 ), 0 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 0, 1 ), 1 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 0, 2 ), 2 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 1, 0 ), 3 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 1, 1 ), 4 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 1, 2 ), 5 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 1, 3 ), 6 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 2, 0 ), 7 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 2, 1 ), 8 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 2, 2 ), 9 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 2, 3 ), 10 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 3, 0 ), 11 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 3, 1 ), 12 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 3, 2 ), 13 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 4, 0 ), 14 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 4, 1 ), 15 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 4, 2 ), 16 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 5, 0 ), 17 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 5, 1 ), 18 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 6, 0 ), 19 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 6, 1 ), 20 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 7, 0 ), 21 );

        // Very loose bounds for neurons except inputs
        double large = 1000000;

        tableau.getBoundManager().initialize( 22 );
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
        tableau.setLowerBound( 17, -large );
        tableau.setUpperBound( 17, large );
        tableau.setLowerBound( 18, -large );
        tableau.setUpperBound( 18, large );
        tableau.setLowerBound( 19, -large );
        tableau.setUpperBound( 19, large );
        tableau.setLowerBound( 20, -large );
        tableau.setUpperBound( 20, large );
        tableau.setLowerBound( 21, -large );
        tableau.setUpperBound( 21, large );
    }

    void populateNetworkBackwardSoftmaxAndMax( NLR::NetworkLevelReasoner &nlr,
                                               MockTableau &tableau )
    {
        /*
                a    a'
          x                e
                b    b'         f    h
          y                d
                c    c'
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


    void populateNetworkBackwardSoftmaxAndMax2( NLR::NetworkLevelReasoner &nlr,
                                                MockTableau &tableau )
    {
        /*
                x3    x7
          x0                 x11    x14
                x4    x8
          x1                 x12    x15    x17
                x5    x9
          x2                 x13    x16
                x6    x10

          x3 = -x0 + x1
          x4 = x0 + 2*x1
          x5 = x0 + x1 + x2
          x6 = 3*x0 - 2*x1 - x2

          x7, x8, x9, x10 = Softmax( x2, x3, x4, x5 )

          x11 = 2x7 + x9 - x10 + 2
          x12 = -x7 + 2x8 + x10
          x13 = x7 - x8 + 2x9 - 2

          x14, x15, x16 = Softmax( x11, x12, x13 )

          x17 = Max( x14, x15, x16 )
        */

        // Create the layers
        nlr.addLayer( 0, NLR::Layer::INPUT, 3 );
        nlr.addLayer( 1, NLR::Layer::WEIGHTED_SUM, 4 );
        nlr.addLayer( 2, NLR::Layer::SOFTMAX, 4 );
        nlr.addLayer( 3, NLR::Layer::WEIGHTED_SUM, 3 );
        nlr.addLayer( 4, NLR::Layer::SOFTMAX, 3 );
        nlr.addLayer( 5, NLR::Layer::MAX, 1 );

        // Mark layer dependencies
        for ( unsigned i = 1; i <= 5; ++i )
            nlr.addLayerDependency( i - 1, i );

        // Set the weights and biases for the weighted sum layers
        nlr.setWeight( 0, 0, 1, 0, -1 );
        nlr.setWeight( 0, 0, 1, 1, 1 );
        nlr.setWeight( 0, 0, 1, 2, 1 );
        nlr.setWeight( 0, 0, 1, 3, 3 );
        nlr.setWeight( 0, 1, 1, 0, 1 );
        nlr.setWeight( 0, 1, 1, 1, 2 );
        nlr.setWeight( 0, 1, 1, 2, 1 );
        nlr.setWeight( 0, 1, 1, 3, -2 );
        nlr.setWeight( 0, 2, 1, 2, 1 );
        nlr.setWeight( 0, 2, 1, 3, -1 );

        nlr.setWeight( 2, 0, 3, 0, 2 );
        nlr.setWeight( 2, 0, 3, 1, -1 );
        nlr.setWeight( 2, 0, 3, 2, 1 );
        nlr.setWeight( 2, 1, 3, 1, 2 );
        nlr.setWeight( 2, 1, 3, 2, -1 );
        nlr.setWeight( 2, 2, 3, 0, 1 );
        nlr.setWeight( 2, 2, 3, 2, 2 );
        nlr.setWeight( 2, 3, 3, 0, -1 );
        nlr.setWeight( 2, 3, 3, 1, 1 );

        nlr.setBias( 3, 0, 2 );
        nlr.setBias( 3, 2, -2 );

        // Mark the Softmax/Max sources
        nlr.addActivationSource( 1, 0, 2, 0 );
        nlr.addActivationSource( 1, 1, 2, 0 );
        nlr.addActivationSource( 1, 2, 2, 0 );
        nlr.addActivationSource( 1, 3, 2, 0 );
        nlr.addActivationSource( 1, 0, 2, 1 );
        nlr.addActivationSource( 1, 1, 2, 1 );
        nlr.addActivationSource( 1, 2, 2, 1 );
        nlr.addActivationSource( 1, 3, 2, 1 );
        nlr.addActivationSource( 1, 0, 2, 2 );
        nlr.addActivationSource( 1, 1, 2, 2 );
        nlr.addActivationSource( 1, 2, 2, 2 );
        nlr.addActivationSource( 1, 3, 2, 2 );
        nlr.addActivationSource( 1, 0, 2, 3 );
        nlr.addActivationSource( 1, 1, 2, 3 );
        nlr.addActivationSource( 1, 2, 2, 3 );
        nlr.addActivationSource( 1, 3, 2, 3 );

        nlr.addActivationSource( 3, 0, 4, 0 );
        nlr.addActivationSource( 3, 1, 4, 0 );
        nlr.addActivationSource( 3, 2, 4, 0 );
        nlr.addActivationSource( 3, 0, 4, 1 );
        nlr.addActivationSource( 3, 1, 4, 1 );
        nlr.addActivationSource( 3, 2, 4, 1 );
        nlr.addActivationSource( 3, 0, 4, 2 );
        nlr.addActivationSource( 3, 1, 4, 2 );
        nlr.addActivationSource( 3, 2, 4, 2 );

        nlr.addActivationSource( 4, 0, 5, 0 );
        nlr.addActivationSource( 4, 1, 5, 0 );
        nlr.addActivationSource( 4, 2, 5, 0 );

        // Variable indexing
        nlr.setNeuronVariable( NLR::NeuronIndex( 0, 0 ), 0 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 0, 1 ), 1 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 0, 2 ), 2 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 1, 0 ), 3 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 1, 1 ), 4 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 1, 2 ), 5 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 1, 3 ), 6 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 2, 0 ), 7 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 2, 1 ), 8 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 2, 2 ), 9 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 2, 3 ), 10 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 3, 0 ), 11 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 3, 1 ), 12 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 3, 2 ), 13 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 4, 0 ), 14 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 4, 1 ), 15 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 4, 2 ), 16 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 5, 0 ), 17 );

        // Very loose bounds for neurons except inputs
        double large = 1000000;

        tableau.getBoundManager().initialize( 18 );
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
        tableau.setLowerBound( 17, -large );
        tableau.setUpperBound( 17, large );
    }

    void populateNetworkBackwardReluAndBilinear( NLR::NetworkLevelReasoner &nlr,
                                                 MockTableau &tableau )
    {
        /*
                a    a'
          x                e
                b    b'         f    h
          y                d
                c    c'
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

    void populateNetworkBackwardReluAndBilinear2( NLR::NetworkLevelReasoner &nlr,
                                                  MockTableau &tableau )
    {
        /*
                x3    x7
          x0
                x4    x8     x11    x13
          x1                               x15
                x5    x9     x12    x14
          x2
                x6    x10

          x3 = -x0 + x1
          x4 = x0 + 2*x1
          x5 = x0 + x1 + x2
          x6 = 3*x0 - 2*x1 - x2

          x7 = ReLU( x3 )
          x8 = ReLU( x4 )
          x9 = ReLU( x5 )
          x10 = ReLU( x6 )

          x11 = 2x7 + x9 - x10 + 2
          x12 = -x7 + 2x8 + x10

          x13 = ReLU( x11 )
          x14 = ReLU( x12 )

          x15 = x13 * x14
        */

        // Create the layers
        nlr.addLayer( 0, NLR::Layer::INPUT, 3 );
        nlr.addLayer( 1, NLR::Layer::WEIGHTED_SUM, 4 );
        nlr.addLayer( 2, NLR::Layer::RELU, 4 );
        nlr.addLayer( 3, NLR::Layer::WEIGHTED_SUM, 2 );
        nlr.addLayer( 4, NLR::Layer::RELU, 2 );
        nlr.addLayer( 5, NLR::Layer::BILINEAR, 1 );

        // Mark layer dependencies
        for ( unsigned i = 1; i <= 5; ++i )
            nlr.addLayerDependency( i - 1, i );

        // Set the weights and biases for the weighted sum layers
        nlr.setWeight( 0, 0, 1, 0, -1 );
        nlr.setWeight( 0, 0, 1, 1, 1 );
        nlr.setWeight( 0, 0, 1, 2, 1 );
        nlr.setWeight( 0, 0, 1, 3, 3 );
        nlr.setWeight( 0, 1, 1, 0, 1 );
        nlr.setWeight( 0, 1, 1, 1, 2 );
        nlr.setWeight( 0, 1, 1, 2, 1 );
        nlr.setWeight( 0, 1, 1, 3, -2 );
        nlr.setWeight( 0, 2, 1, 2, 1 );
        nlr.setWeight( 0, 2, 1, 3, -1 );

        nlr.setWeight( 2, 0, 3, 0, 2 );
        nlr.setWeight( 2, 0, 3, 1, -1 );
        nlr.setWeight( 2, 1, 3, 1, 2 );
        nlr.setWeight( 2, 2, 3, 0, 1 );
        nlr.setWeight( 2, 3, 3, 0, -1 );
        nlr.setWeight( 2, 3, 3, 1, 1 );

        nlr.setBias( 3, 0, 2 );

        // Mark the ReLU/Bilinear sources
        nlr.addActivationSource( 1, 0, 2, 0 );
        nlr.addActivationSource( 1, 1, 2, 1 );
        nlr.addActivationSource( 1, 2, 2, 2 );
        nlr.addActivationSource( 1, 3, 2, 3 );

        nlr.addActivationSource( 3, 0, 4, 0 );
        nlr.addActivationSource( 3, 1, 4, 1 );

        nlr.addActivationSource( 4, 0, 5, 0 );
        nlr.addActivationSource( 4, 1, 5, 0 );

        // Variable indexing
        nlr.setNeuronVariable( NLR::NeuronIndex( 0, 0 ), 0 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 0, 1 ), 1 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 0, 2 ), 2 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 1, 0 ), 3 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 1, 1 ), 4 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 1, 2 ), 5 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 1, 3 ), 6 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 2, 0 ), 7 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 2, 1 ), 8 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 2, 2 ), 9 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 2, 3 ), 10 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 3, 0 ), 11 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 3, 1 ), 12 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 4, 0 ), 13 );
        nlr.setNeuronVariable( NLR::NeuronIndex( 4, 1 ), 14 );

        nlr.setNeuronVariable( NLR::NeuronIndex( 5, 0 ), 15 );

        // Very loose bounds for neurons except inputs
        double large = 1000000;

        tableau.getBoundManager().initialize( 16 );
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
    }

    void test_backwards_relu()
    {
        Options::get()->setString( Options::SYMBOLIC_BOUND_TIGHTENING_TYPE, "sbt" );
        Options::get()->setString( Options::MILP_SOLVER_BOUND_TIGHTENING_TYPE,
                                   "backward-converge" );

        NLR::NetworkLevelReasoner nlr;
        MockTableau tableau;
        nlr.setTableau( &tableau );
        populateNetworkBackwardReLU( nlr, tableau );

        tableau.setLowerBound( 0, 0 );
        tableau.setUpperBound( 0, 1 );
        tableau.setLowerBound( 1, 0 );
        tableau.setUpperBound( 1, 1 );

        // Invoke DeepPoly
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );
        TS_ASSERT_THROWS_NOTHING( nlr.deepPolyPropagation() );

        List<Tightening> expectedBounds( {
            Tightening( 2, -1, Tightening::LB ),   Tightening( 2, 1, Tightening::UB ),
            Tightening( 3, 0, Tightening::LB ),    Tightening( 3, 2, Tightening::UB ),

            Tightening( 4, 0, Tightening::LB ),    Tightening( 4, 1, Tightening::UB ),
            Tightening( 5, 0, Tightening::LB ),    Tightening( 5, 2, Tightening::UB ),

            Tightening( 6, -0.5, Tightening::LB ), Tightening( 6, 2, Tightening::UB ),
            Tightening( 7, -2, Tightening::LB ),   Tightening( 7, 1, Tightening::UB ),

            Tightening( 8, -0.5, Tightening::LB ), Tightening( 8, 2, Tightening::UB ),
            Tightening( 9, 0, Tightening::LB ),    Tightening( 9, 1, Tightening::UB ),

            Tightening( 10, -2, Tightening::LB ),  Tightening( 10, 0.5, Tightening::UB ),
            Tightening( 11, 1.5, Tightening::LB ), Tightening( 11, 4.4, Tightening::UB ),
        } );

        List<Tightening> bounds;
        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );
        TS_ASSERT( boundsEqual( bounds, expectedBounds ) );


        // Invoke backward LP propagation
        TS_ASSERT_THROWS_NOTHING( updateTableau( tableau, bounds ) );
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );
        TS_ASSERT_THROWS_NOTHING( nlr.lpRelaxationPropagation() );

        List<Tightening> expectedBounds2( {
            Tightening( 8, 0, Tightening::LB ),
        } );

        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );
        TS_ASSERT( boundsEqual( bounds, expectedBounds2 ) );


        // Change the current bounds
        tableau.setLowerBound( 0, -3 );
        tableau.setUpperBound( 0, 1 );
        tableau.setLowerBound( 1, -1 );
        tableau.setUpperBound( 1, 2 );

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


        // Invoke DeepPoly
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );
        TS_ASSERT_THROWS_NOTHING( nlr.deepPolyPropagation() );

        List<Tightening> expectedBounds3( {
            Tightening( 2, -5, Tightening::LB ),       Tightening( 2, 2, Tightening::UB ),
            Tightening( 3, -4, Tightening::LB ),       Tightening( 3, 3, Tightening::UB ),

            Tightening( 4, 0, Tightening::LB ),        Tightening( 4, 2, Tightening::UB ),
            Tightening( 5, 0, Tightening::LB ),        Tightening( 5, 3, Tightening::UB ),

            Tightening( 6, -2, Tightening::LB ),       Tightening( 6, 3, Tightening::UB ),
            Tightening( 7, -3, Tightening::LB ),       Tightening( 7, 4, Tightening::UB ),

            Tightening( 8, -2, Tightening::LB ),       Tightening( 8, 3, Tightening::UB ),
            Tightening( 9, -3, Tightening::LB ),       Tightening( 9, 4, Tightening::UB ),

            Tightening( 10, -4.0489, Tightening::LB ), Tightening( 10, 0, Tightening::UB ),
            Tightening( 11, -1, Tightening::LB ),      Tightening( 11, 10, Tightening::UB ),
        } );

        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );
        TS_ASSERT( boundsEqual( bounds, expectedBounds3 ) );


        // Invoke backward LP propagation
        TS_ASSERT_THROWS_NOTHING( updateTableau( tableau, bounds ) );
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );
        TS_ASSERT_THROWS_NOTHING( nlr.lpRelaxationPropagation() );

        List<Tightening> expectedBounds4( {
            Tightening( 8, 0, Tightening::LB ),
            Tightening( 9, 0, Tightening::LB ),
        } );

        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );
        TS_ASSERT( boundsEqual( bounds, expectedBounds4 ) );
    }

    void test_backwards_relu2()
    {
        Options::get()->setString( Options::SYMBOLIC_BOUND_TIGHTENING_TYPE, "sbt" );
        Options::get()->setString( Options::MILP_SOLVER_BOUND_TIGHTENING_TYPE,
                                   "backward-converge" );

        NLR::NetworkLevelReasoner nlr;
        MockTableau tableau;
        nlr.setTableau( &tableau );
        populateNetworkBackwardReLU2( nlr, tableau );

        tableau.setLowerBound( 0, -1 );
        tableau.setUpperBound( 0, 1 );
        tableau.setLowerBound( 1, -1 );
        tableau.setUpperBound( 1, 1 );
        tableau.setLowerBound( 2, -1 );
        tableau.setUpperBound( 2, 1 );


        // Invoke DeepPoly
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );
        TS_ASSERT_THROWS_NOTHING( nlr.deepPolyPropagation() );

        List<Tightening> expectedBounds( {
            Tightening( 3, -2, Tightening::LB ),     Tightening( 3, 2, Tightening::UB ),
            Tightening( 4, -3, Tightening::LB ),     Tightening( 4, 3, Tightening::UB ),
            Tightening( 5, -3, Tightening::LB ),     Tightening( 5, 3, Tightening::UB ),
            Tightening( 6, -6, Tightening::LB ),     Tightening( 6, 6, Tightening::UB ),

            Tightening( 7, 0, Tightening::LB ),      Tightening( 7, 2, Tightening::UB ),
            Tightening( 8, 0, Tightening::LB ),      Tightening( 8, 3, Tightening::UB ),
            Tightening( 9, 0, Tightening::LB ),      Tightening( 9, 3, Tightening::UB ),
            Tightening( 10, 0, Tightening::LB ),     Tightening( 10, 6, Tightening::UB ),

            Tightening( 11, -4, Tightening::LB ),    Tightening( 11, 8, Tightening::UB ),
            Tightening( 12, -2, Tightening::LB ),    Tightening( 12, 10, Tightening::UB ),
            Tightening( 13, -5, Tightening::LB ),    Tightening( 13, 5, Tightening::UB ),

            Tightening( 14, -4, Tightening::LB ),    Tightening( 14, 8, Tightening::UB ),
            Tightening( 15, -2, Tightening::LB ),    Tightening( 15, 10, Tightening::UB ),
            Tightening( 16, -5, Tightening::LB ),    Tightening( 16, 5, Tightening::UB ),

            Tightening( 17, -14.5, Tightening::LB ), Tightening( 17, 17, Tightening::UB ),
            Tightening( 18, 0, Tightening::LB ),     Tightening( 18, 17.1667, Tightening::UB ),

            Tightening( 19, -14.5, Tightening::LB ), Tightening( 19, 17, Tightening::UB ),
            Tightening( 20, 0, Tightening::LB ),     Tightening( 20, 17.1667, Tightening::UB ),

            Tightening( 21, -26, Tightening::LB ),   Tightening( 21, 13.9206, Tightening::UB ),
        } );

        List<Tightening> bounds;
        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );
        TS_ASSERT( boundsEqual( bounds, expectedBounds ) );


        // Invoke backward LP propagation
        TS_ASSERT_THROWS_NOTHING( updateTableau( tableau, bounds ) );
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );
        TS_ASSERT_THROWS_NOTHING( nlr.lpRelaxationPropagation() );

        List<Tightening> expectedBounds2( {
            Tightening( 14, 0, Tightening::LB ),
            Tightening( 15, 0, Tightening::LB ),
            Tightening( 16, 0, Tightening::LB ),

            Tightening( 19, 0, Tightening::LB ),
        } );

        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );
        TS_ASSERT( boundsEqual( bounds, expectedBounds2 ) );


        // Change the current bounds
        tableau.setLowerBound( 0, -3 );
        tableau.setUpperBound( 0, 1 );
        tableau.setLowerBound( 1, -1 );
        tableau.setUpperBound( 1, 2 );
        tableau.setLowerBound( 2, -2 );
        tableau.setUpperBound( 2, 2 );

        double large = 1000000;
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
        tableau.setLowerBound( 17, -large );
        tableau.setUpperBound( 17, large );
        tableau.setLowerBound( 18, -large );
        tableau.setUpperBound( 18, large );
        tableau.setLowerBound( 19, -large );
        tableau.setUpperBound( 19, large );
        tableau.setLowerBound( 20, -large );
        tableau.setUpperBound( 20, large );
        tableau.setLowerBound( 21, -large );
        tableau.setUpperBound( 21, large );


        // Invoke DeepPoly
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );
        TS_ASSERT_THROWS_NOTHING( nlr.deepPolyPropagation() );

        List<Tightening> expectedBounds3( {
            Tightening( 3, -2, Tightening::LB ),        Tightening( 3, 5, Tightening::UB ),
            Tightening( 4, -5, Tightening::LB ),        Tightening( 4, 5, Tightening::UB ),
            Tightening( 5, -6, Tightening::LB ),        Tightening( 5, 5, Tightening::UB ),
            Tightening( 6, -15, Tightening::LB ),       Tightening( 6, 7, Tightening::UB ),

            Tightening( 7, -2, Tightening::LB ),        Tightening( 7, 5, Tightening::UB ),
            Tightening( 8, 0, Tightening::LB ),         Tightening( 8, 5, Tightening::UB ),
            Tightening( 9, 0, Tightening::LB ),         Tightening( 9, 5, Tightening::UB ),
            Tightening( 10, 0, Tightening::LB ),        Tightening( 10, 7, Tightening::UB ),

            Tightening( 11, -9, Tightening::LB ),       Tightening( 11, 15.1818, Tightening::UB ),
            Tightening( 12, -5, Tightening::LB ),       Tightening( 12, 14.0909, Tightening::UB ),
            Tightening( 13, -6, Tightening::LB ),       Tightening( 13, 10.1429, Tightening::UB ),

            Tightening( 14, -9, Tightening::LB ),       Tightening( 14, 15.1818, Tightening::UB ),
            Tightening( 15, -5, Tightening::LB ),       Tightening( 15, 14.0909, Tightening::UB ),
            Tightening( 16, -6, Tightening::LB ),       Tightening( 16, 10.1429, Tightening::UB ),

            Tightening( 17, -29.8351, Tightening::LB ), Tightening( 17, 28.2857, Tightening::UB ),
            Tightening( 18, -4, Tightening::LB ),       Tightening( 18, 29.6479, Tightening::UB ),

            Tightening( 19, 0, Tightening::LB ),        Tightening( 19, 28.2857, Tightening::UB ),
            Tightening( 20, -4, Tightening::LB ),       Tightening( 20, 29.6479, Tightening::UB ),

            Tightening( 21, -30.6479, Tightening::LB ), Tightening( 21, 29.1467, Tightening::UB ),
        } );

        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );
        TS_ASSERT( boundsEqual( bounds, expectedBounds3 ) );


        // Invoke backward LP propagation
        TS_ASSERT_THROWS_NOTHING( updateTableau( tableau, bounds ) );
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );
        TS_ASSERT_THROWS_NOTHING( nlr.lpRelaxationPropagation() );

        List<Tightening> expectedBounds4( {
            Tightening( 7, 0, Tightening::LB ),

            Tightening( 14, 0, Tightening::LB ),
            Tightening( 15, 0, Tightening::LB ),
            Tightening( 16, 0, Tightening::LB ),

            Tightening( 20, 0, Tightening::LB ),
        } );

        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );
        TS_ASSERT( boundsEqual( bounds, expectedBounds4 ) );
    }

    void test_backwards_sigmoid()
    {
        Options::get()->setString( Options::SYMBOLIC_BOUND_TIGHTENING_TYPE, "sbt" );
        Options::get()->setString( Options::MILP_SOLVER_BOUND_TIGHTENING_TYPE,
                                   "backward-converge" );

        NLR::NetworkLevelReasoner nlr;
        MockTableau tableau;
        nlr.setTableau( &tableau );
        populateNetworkBackwardSigmoid( nlr, tableau );

        tableau.setLowerBound( 0, 0 );
        tableau.setUpperBound( 0, 1 );
        tableau.setLowerBound( 1, 0 );
        tableau.setUpperBound( 1, 1 );


        // Invoke DeepPoly
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );
        TS_ASSERT_THROWS_NOTHING( nlr.deepPolyPropagation() );

        List<Tightening> expectedBounds( {
            Tightening( 2, -1, Tightening::LB ),       Tightening( 2, 1, Tightening::UB ),
            Tightening( 3, 0, Tightening::LB ),        Tightening( 3, 2, Tightening::UB ),

            Tightening( 4, 0.2689, Tightening::LB ),   Tightening( 4, 0.7311, Tightening::UB ),
            Tightening( 5, 0.5, Tightening::LB ),      Tightening( 5, 0.8808, Tightening::UB ),

            Tightening( 6, -0.1261, Tightening::LB ),  Tightening( 6, 0.5069, Tightening::UB ),
            Tightening( 7, -0.2379, Tightening::LB ),  Tightening( 7, 0.8571, Tightening::UB ),

            Tightening( 8, 0.4685, Tightening::LB ),   Tightening( 8, 0.6241, Tightening::UB ),
            Tightening( 9, 0.4408, Tightening::LB ),   Tightening( 9, 0.7021, Tightening::UB ),

            Tightening( 10, -1.1819, Tightening::LB ), Tightening( 10, -1.0535, Tightening::UB ),
            Tightening( 11, 3.4986, Tightening::LB ),  Tightening( 11, 3.8797, Tightening::UB ),
        } );

        List<Tightening> bounds;
        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );
        TS_ASSERT( boundsEqual( bounds, expectedBounds ) );


        // Invoke backward LP propagation
        TS_ASSERT_THROWS_NOTHING( updateTableau( tableau, bounds ) );
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );
        TS_ASSERT_THROWS_NOTHING( nlr.lpRelaxationPropagation() );

        List<Tightening> expectedBounds2( {} );

        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );
        TS_ASSERT( boundsEqual( bounds, expectedBounds2 ) );


        // Change the current bounds
        tableau.setLowerBound( 0, -3 );
        tableau.setUpperBound( 0, 1 );
        tableau.setLowerBound( 1, -1 );
        tableau.setUpperBound( 1, 2 );

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


        // Invoke DeepPoly
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );
        TS_ASSERT_THROWS_NOTHING( nlr.deepPolyPropagation() );

        List<Tightening> expectedBounds3( {
            Tightening( 2, -5, Tightening::LB ),       Tightening( 2, 2, Tightening::UB ),
            Tightening( 3, -4, Tightening::LB ),       Tightening( 3, 3, Tightening::UB ),

            Tightening( 4, 0.0066, Tightening::LB ),   Tightening( 4, 0.8807, Tightening::UB ),
            Tightening( 5, 0.0179, Tightening::LB ),   Tightening( 5, 0.9526, Tightening::UB ),

            Tightening( 6, -0.8362, Tightening::LB ),  Tightening( 6, 0.9193, Tightening::UB ),
            Tightening( 7, -0.8860, Tightening::LB ),  Tightening( 7, 1.6904, Tightening::UB ),

            Tightening( 8, 0.3023, Tightening::LB ),   Tightening( 8, 0.7148, Tightening::UB ),
            Tightening( 9, 0.2919, Tightening::LB ),   Tightening( 9, 0.8443, Tightening::UB ),

            Tightening( 10, -1.2694, Tightening::LB ), Tightening( 10, -0.8841, Tightening::UB ),
            Tightening( 11, 3.2396, Tightening::LB ),  Tightening( 11, 4.05, Tightening::UB ),
        } );

        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );
        TS_ASSERT( boundsEqual( bounds, expectedBounds3 ) );


        // Invoke backward LP propagation
        TS_ASSERT_THROWS_NOTHING( updateTableau( tableau, bounds ) );
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );
        TS_ASSERT_THROWS_NOTHING( nlr.lpRelaxationPropagation() );

        List<Tightening> expectedBounds4( {} );

        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );
        TS_ASSERT( boundsEqual( bounds, expectedBounds4 ) );
    }

    void test_backwards_sigmoid2()
    {
        Options::get()->setString( Options::SYMBOLIC_BOUND_TIGHTENING_TYPE, "sbt" );
        Options::get()->setString( Options::MILP_SOLVER_BOUND_TIGHTENING_TYPE,
                                   "backward-converge" );

        NLR::NetworkLevelReasoner nlr;
        MockTableau tableau;
        nlr.setTableau( &tableau );
        populateNetworkBackwardSigmoid2( nlr, tableau );

        tableau.setLowerBound( 0, -1 );
        tableau.setUpperBound( 0, 1 );
        tableau.setLowerBound( 1, -1 );
        tableau.setUpperBound( 1, 1 );
        tableau.setLowerBound( 2, -1 );
        tableau.setUpperBound( 2, 1 );


        // Invoke DeepPoly
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );
        TS_ASSERT_THROWS_NOTHING( nlr.deepPolyPropagation() );

        List<Tightening> expectedBounds( {
            Tightening( 3, -2, Tightening::LB ),       Tightening( 3, 2, Tightening::UB ),
            Tightening( 4, -3, Tightening::LB ),       Tightening( 4, 3, Tightening::UB ),
            Tightening( 5, -3, Tightening::LB ),       Tightening( 5, 3, Tightening::UB ),
            Tightening( 6, -6, Tightening::LB ),       Tightening( 6, 6, Tightening::UB ),

            Tightening( 7, 0.1192, Tightening::LB ),   Tightening( 7, 0.8808, Tightening::UB ),
            Tightening( 8, 0.0474, Tightening::LB ),   Tightening( 8, 0.9526, Tightening::UB ),
            Tightening( 9, 0.0474, Tightening::LB ),   Tightening( 9, 0.9526, Tightening::UB ),
            Tightening( 10, 0.0025, Tightening::LB ),  Tightening( 10, 0.9975, Tightening::UB ),

            Tightening( 11, 1.3787, Tightening::LB ),  Tightening( 11, 4.6213, Tightening::UB ),
            Tightening( 12, -0.5636, Tightening::LB ), Tightening( 12, 2.5636, Tightening::UB ),
            Tightening( 13, -2.3771, Tightening::LB ), Tightening( 13, 0.3771, Tightening::UB ),

            Tightening( 14, 0.7988, Tightening::LB ),  Tightening( 14, 0.9903, Tightening::UB ),
            Tightening( 15, 0.3627, Tightening::LB ),  Tightening( 15, 0.9285, Tightening::UB ),
            Tightening( 16, 0.0849, Tightening::LB ),  Tightening( 16, 0.5932, Tightening::UB ),

            Tightening( 17, -1.2113, Tightening::LB ), Tightening( 17, 0.0354, Tightening::UB ),
            Tightening( 18, 1.4027, Tightening::LB ),  Tightening( 18, 2.4177, Tightening::UB ),

            Tightening( 19, 0.2295, Tightening::LB ),  Tightening( 19, 0.5088, Tightening::UB ),
            Tightening( 20, 0.8026, Tightening::LB ),  Tightening( 20, 0.9182, Tightening::UB ),

            Tightening( 21, -1.6539, Tightening::LB ), Tightening( 21, -1.3393, Tightening::UB ),
        } );

        List<Tightening> bounds;
        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );
        TS_ASSERT( boundsEqual( bounds, expectedBounds ) );


        // Invoke backward LP propagation
        TS_ASSERT_THROWS_NOTHING( updateTableau( tableau, bounds ) );
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );
        TS_ASSERT_THROWS_NOTHING( nlr.lpRelaxationPropagation() );

        List<Tightening> expectedBounds2( {} );

        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );
        TS_ASSERT( boundsEqual( bounds, expectedBounds2 ) );


        // Change the current bounds
        tableau.setLowerBound( 0, -3 );
        tableau.setUpperBound( 0, 1 );
        tableau.setLowerBound( 1, -1 );
        tableau.setUpperBound( 1, 2 );
        tableau.setLowerBound( 2, -2 );
        tableau.setUpperBound( 2, 2 );

        double large = 1000000;
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
        tableau.setLowerBound( 17, -large );
        tableau.setUpperBound( 17, large );
        tableau.setLowerBound( 18, -large );
        tableau.setUpperBound( 18, large );
        tableau.setLowerBound( 19, -large );
        tableau.setUpperBound( 19, large );
        tableau.setLowerBound( 20, -large );
        tableau.setUpperBound( 20, large );
        tableau.setLowerBound( 21, -large );
        tableau.setUpperBound( 21, large );


        // Invoke DeepPoly
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );
        TS_ASSERT_THROWS_NOTHING( nlr.deepPolyPropagation() );

        List<Tightening> expectedBounds3( {
            Tightening( 3, -2, Tightening::LB ),       Tightening( 3, 5, Tightening::UB ),
            Tightening( 4, -5, Tightening::LB ),       Tightening( 4, 5, Tightening::UB ),
            Tightening( 5, -6, Tightening::LB ),       Tightening( 5, 5, Tightening::UB ),
            Tightening( 6, -15, Tightening::LB ),      Tightening( 6, 7, Tightening::UB ),

            Tightening( 7, 0.1192, Tightening::LB ),   Tightening( 7, 0.9933, Tightening::UB ),
            Tightening( 8, 0.0067, Tightening::LB ),   Tightening( 8, 0.9933, Tightening::UB ),
            Tightening( 9, 0.0025, Tightening::LB ),   Tightening( 9, 0.9933, Tightening::UB ),
            Tightening( 10, 0, Tightening::LB ),       Tightening( 10, 0.9991, Tightening::UB ),

            Tightening( 11, 1.2517, Tightening::LB ),  Tightening( 11, 4.9701, Tightening::UB ),
            Tightening( 12, -0.9599, Tightening::LB ), Tightening( 12, 2.8466, Tightening::UB ),
            Tightening( 13, -2.8147, Tightening::LB ), Tightening( 13, 0.9188, Tightening::UB ),

            Tightening( 14, 0.7776, Tightening::LB ),  Tightening( 14, 0.9931, Tightening::UB ),
            Tightening( 15, 0.2769, Tightening::LB ),  Tightening( 15, 0.9451, Tightening::UB ),
            Tightening( 16, 0.0565, Tightening::LB ),  Tightening( 16, 0.7148, Tightening::UB ),

            Tightening( 17, -1.4307, Tightening::LB ), Tightening( 17, 0.1083, Tightening::UB ),
            Tightening( 18, 1.2592, Tightening::LB ),  Tightening( 18, 2.5519, Tightening::UB ),

            Tightening( 19, 0.1929, Tightening::LB ),  Tightening( 19, 0.5270, Tightening::UB ),
            Tightening( 20, 0.7789, Tightening::LB ),  Tightening( 20, 0.9277, Tightening::UB ),

            Tightening( 21, -1.6967, Tightening::LB ), Tightening( 21, -1.3115, Tightening::UB ),
        } );

        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );
        TS_ASSERT( boundsEqual( bounds, expectedBounds3 ) );


        // Invoke backward LP propagation
        TS_ASSERT_THROWS_NOTHING( updateTableau( tableau, bounds ) );
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );
        TS_ASSERT_THROWS_NOTHING( nlr.lpRelaxationPropagation() );

        List<Tightening> expectedBounds4( {} );

        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );
        TS_ASSERT( boundsEqual( bounds, expectedBounds4 ) );
    }

    void test_backwards_abs()
    {
        Options::get()->setString( Options::SYMBOLIC_BOUND_TIGHTENING_TYPE, "sbt" );
        Options::get()->setString( Options::MILP_SOLVER_BOUND_TIGHTENING_TYPE,
                                   "backward-converge" );

        NLR::NetworkLevelReasoner nlr;
        MockTableau tableau;
        nlr.setTableau( &tableau );
        populateNetworkBackwardAbs( nlr, tableau );

        tableau.setLowerBound( 0, 0 );
        tableau.setUpperBound( 0, 1 );
        tableau.setLowerBound( 1, 0 );
        tableau.setUpperBound( 1, 1 );


        // Invoke DeepPoly
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );
        TS_ASSERT_THROWS_NOTHING( nlr.deepPolyPropagation() );

        List<Tightening> expectedBounds( {
            Tightening( 2, -1, Tightening::LB ),  Tightening( 2, 1, Tightening::UB ),
            Tightening( 3, 0, Tightening::LB ),   Tightening( 3, 2, Tightening::UB ),

            Tightening( 4, 0, Tightening::LB ),   Tightening( 4, 1, Tightening::UB ),
            Tightening( 5, 0, Tightening::LB ),   Tightening( 5, 2, Tightening::UB ),

            Tightening( 6, -1, Tightening::LB ),  Tightening( 6, 2, Tightening::UB ),
            Tightening( 7, -2, Tightening::LB ),  Tightening( 7, 2, Tightening::UB ),

            Tightening( 8, 0, Tightening::LB ),   Tightening( 8, 2, Tightening::UB ),
            Tightening( 9, 0, Tightening::LB ),   Tightening( 9, 2, Tightening::UB ),

            Tightening( 10, -4, Tightening::LB ), Tightening( 10, 0, Tightening::UB ),
            Tightening( 11, 2, Tightening::LB ),  Tightening( 11, 8, Tightening::UB ),
        } );

        List<Tightening> bounds;
        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );
        TS_ASSERT( boundsEqual( bounds, expectedBounds ) );


        // Invoke backward LP propagation
        TS_ASSERT_THROWS_NOTHING( updateTableau( tableau, bounds ) );
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );
        TS_ASSERT_THROWS_NOTHING( nlr.lpRelaxationPropagation() );

        List<Tightening> expectedBounds2( {} );

        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );
        TS_ASSERT( boundsEqual( bounds, expectedBounds2 ) );


        // Change the current bounds
        tableau.setLowerBound( 0, -3 );
        tableau.setUpperBound( 0, 1 );
        tableau.setLowerBound( 1, -1 );
        tableau.setUpperBound( 1, 2 );

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


        // Invoke DeepPoly
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );
        TS_ASSERT_THROWS_NOTHING( nlr.deepPolyPropagation() );

        List<Tightening> expectedBounds3( {
            Tightening( 2, -5, Tightening::LB ),   Tightening( 2, 2, Tightening::UB ),
            Tightening( 3, -4, Tightening::LB ),   Tightening( 3, 3, Tightening::UB ),

            Tightening( 4, 0, Tightening::LB ),    Tightening( 4, 5, Tightening::UB ),
            Tightening( 5, 0, Tightening::LB ),    Tightening( 5, 4, Tightening::UB ),

            Tightening( 6, -5, Tightening::LB ),   Tightening( 6, 4, Tightening::UB ),
            Tightening( 7, -4, Tightening::LB ),   Tightening( 7, 10, Tightening::UB ),

            Tightening( 8, 0, Tightening::LB ),    Tightening( 8, 5, Tightening::UB ),
            Tightening( 9, 0, Tightening::LB ),    Tightening( 9, 10, Tightening::UB ),

            Tightening( 10, -15, Tightening::LB ), Tightening( 10, 0, Tightening::UB ),
            Tightening( 11, 2, Tightening::LB ),   Tightening( 11, 27, Tightening::UB ),
        } );

        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );
        TS_ASSERT( boundsEqual( bounds, expectedBounds3 ) );


        // Invoke backward LP propagation
        TS_ASSERT_THROWS_NOTHING( updateTableau( tableau, bounds ) );
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );
        TS_ASSERT_THROWS_NOTHING( nlr.lpRelaxationPropagation() );

        List<Tightening> expectedBounds4( {} );

        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );
        TS_ASSERT( boundsEqual( bounds, expectedBounds4 ) );
    }

    void test_backwards_abs2()
    {
        Options::get()->setString( Options::SYMBOLIC_BOUND_TIGHTENING_TYPE, "sbt" );
        Options::get()->setString( Options::MILP_SOLVER_BOUND_TIGHTENING_TYPE,
                                   "backward-converge" );

        NLR::NetworkLevelReasoner nlr;
        MockTableau tableau;
        nlr.setTableau( &tableau );
        populateNetworkBackwardAbs2( nlr, tableau );

        tableau.setLowerBound( 0, -1 );
        tableau.setUpperBound( 0, 1 );
        tableau.setLowerBound( 1, -1 );
        tableau.setUpperBound( 1, 1 );
        tableau.setLowerBound( 2, -1 );
        tableau.setUpperBound( 2, 1 );


        // Invoke DeepPoly
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );
        TS_ASSERT_THROWS_NOTHING( nlr.deepPolyPropagation() );

        List<Tightening> expectedBounds( {
            Tightening( 3, -2, Tightening::LB ),   Tightening( 3, 2, Tightening::UB ),
            Tightening( 4, -3, Tightening::LB ),   Tightening( 4, 3, Tightening::UB ),
            Tightening( 5, -3, Tightening::LB ),   Tightening( 5, 3, Tightening::UB ),
            Tightening( 6, -6, Tightening::LB ),   Tightening( 6, 6, Tightening::UB ),

            Tightening( 7, 0, Tightening::LB ),    Tightening( 7, 2, Tightening::UB ),
            Tightening( 8, 0, Tightening::LB ),    Tightening( 8, 3, Tightening::UB ),
            Tightening( 9, 0, Tightening::LB ),    Tightening( 9, 3, Tightening::UB ),
            Tightening( 10, 0, Tightening::LB ),   Tightening( 10, 6, Tightening::UB ),

            Tightening( 11, -4, Tightening::LB ),  Tightening( 11, 9, Tightening::UB ),
            Tightening( 12, -2, Tightening::LB ),  Tightening( 12, 12, Tightening::UB ),
            Tightening( 13, -5, Tightening::LB ),  Tightening( 13, 6, Tightening::UB ),

            Tightening( 14, 0, Tightening::LB ),   Tightening( 14, 9, Tightening::UB ),
            Tightening( 15, 0, Tightening::LB ),   Tightening( 15, 12, Tightening::UB ),
            Tightening( 16, 0, Tightening::LB ),   Tightening( 16, 6, Tightening::UB ),

            Tightening( 17, -15, Tightening::LB ), Tightening( 17, 12, Tightening::UB ),
            Tightening( 18, 0, Tightening::LB ),   Tightening( 18, 27, Tightening::UB ),

            Tightening( 19, 0, Tightening::LB ),   Tightening( 19, 15, Tightening::UB ),
            Tightening( 20, 0, Tightening::LB ),   Tightening( 20, 27, Tightening::UB ),

            Tightening( 21, -28, Tightening::LB ), Tightening( 21, 14, Tightening::UB ),
        } );

        List<Tightening> bounds;
        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );
        TS_ASSERT( boundsEqual( bounds, expectedBounds ) );


        // Invoke backward LP propagation
        TS_ASSERT_THROWS_NOTHING( updateTableau( tableau, bounds ) );
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );
        TS_ASSERT_THROWS_NOTHING( nlr.lpRelaxationPropagation() );

        List<Tightening> expectedBounds2( {} );

        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );
        TS_ASSERT( boundsEqual( bounds, expectedBounds2 ) );


        // Change the current bounds
        tableau.setLowerBound( 0, -3 );
        tableau.setUpperBound( 0, 1 );
        tableau.setLowerBound( 1, -1 );
        tableau.setUpperBound( 1, 2 );
        tableau.setLowerBound( 2, -2 );
        tableau.setUpperBound( 2, 2 );

        double large = 1000000;
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
        tableau.setLowerBound( 17, -large );
        tableau.setUpperBound( 17, large );
        tableau.setLowerBound( 18, -large );
        tableau.setUpperBound( 18, large );
        tableau.setLowerBound( 19, -large );
        tableau.setUpperBound( 19, large );
        tableau.setLowerBound( 20, -large );
        tableau.setUpperBound( 20, large );
        tableau.setLowerBound( 21, -large );
        tableau.setUpperBound( 21, large );


        // Invoke DeepPoly
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );
        TS_ASSERT_THROWS_NOTHING( nlr.deepPolyPropagation() );

        List<Tightening> expectedBounds3( {
            Tightening( 3, -2, Tightening::LB ),   Tightening( 3, 5, Tightening::UB ),
            Tightening( 4, -5, Tightening::LB ),   Tightening( 4, 5, Tightening::UB ),
            Tightening( 5, -6, Tightening::LB ),   Tightening( 5, 5, Tightening::UB ),
            Tightening( 6, -15, Tightening::LB ),  Tightening( 6, 7, Tightening::UB ),

            Tightening( 7, 0, Tightening::LB ),    Tightening( 7, 5, Tightening::UB ),
            Tightening( 8, 0, Tightening::LB ),    Tightening( 8, 5, Tightening::UB ),
            Tightening( 9, 0, Tightening::LB ),    Tightening( 9, 6, Tightening::UB ),
            Tightening( 10, 0, Tightening::LB ),   Tightening( 10, 15, Tightening::UB ),

            Tightening( 11, -13, Tightening::LB ), Tightening( 11, 18, Tightening::UB ),
            Tightening( 12, -5, Tightening::LB ),  Tightening( 12, 25, Tightening::UB ),
            Tightening( 13, -7, Tightening::LB ),  Tightening( 13, 15, Tightening::UB ),

            Tightening( 14, 0, Tightening::LB ),   Tightening( 14, 18, Tightening::UB ),
            Tightening( 15, 0, Tightening::LB ),   Tightening( 15, 25, Tightening::UB ),
            Tightening( 16, 0, Tightening::LB ),   Tightening( 16, 15, Tightening::UB ),

            Tightening( 17, -33, Tightening::LB ), Tightening( 17, 25, Tightening::UB ),
            Tightening( 18, 0, Tightening::LB ),   Tightening( 18, 58, Tightening::UB ),

            Tightening( 19, 0, Tightening::LB ),   Tightening( 19, 33, Tightening::UB ),
            Tightening( 20, 0, Tightening::LB ),   Tightening( 20, 58, Tightening::UB ),

            Tightening( 21, -59, Tightening::LB ), Tightening( 21, 32, Tightening::UB ),
        } );

        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );
        TS_ASSERT( boundsEqual( bounds, expectedBounds3 ) );


        // Invoke backward LP propagation
        TS_ASSERT_THROWS_NOTHING( updateTableau( tableau, bounds ) );
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );
        TS_ASSERT_THROWS_NOTHING( nlr.lpRelaxationPropagation() );

        List<Tightening> expectedBounds4( {} );

        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );
        TS_ASSERT( boundsEqual( bounds, expectedBounds4 ) );
    }

    void test_backwards_round()
    {
        Options::get()->setString( Options::SYMBOLIC_BOUND_TIGHTENING_TYPE, "sbt" );
        Options::get()->setString( Options::MILP_SOLVER_BOUND_TIGHTENING_TYPE,
                                   "backward-converge" );

        NLR::NetworkLevelReasoner nlr;
        MockTableau tableau;
        nlr.setTableau( &tableau );
        populateNetworkBackwardRound( nlr, tableau );

        tableau.setLowerBound( 0, 0 );
        tableau.setUpperBound( 0, 1 );
        tableau.setLowerBound( 1, 0 );
        tableau.setUpperBound( 1, 1 );


        // Invoke DeepPoly
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );
        TS_ASSERT_THROWS_NOTHING( nlr.deepPolyPropagation() );

        List<Tightening> expectedBounds( {
            Tightening( 2, -1, Tightening::LB ),    Tightening( 2, 1, Tightening::UB ),
            Tightening( 3, 0, Tightening::LB ),     Tightening( 3, 2, Tightening::UB ),

            Tightening( 4, -1, Tightening::LB ),    Tightening( 4, 1, Tightening::UB ),
            Tightening( 5, 0, Tightening::LB ),     Tightening( 5, 2, Tightening::UB ),

            Tightening( 6, -1, Tightening::LB ),    Tightening( 6, 3, Tightening::UB ),
            Tightening( 7, -4, Tightening::LB ),    Tightening( 7, 2, Tightening::UB ),

            Tightening( 8, -1, Tightening::LB ),    Tightening( 8, 3, Tightening::UB ),
            Tightening( 9, -4, Tightening::LB ),    Tightening( 9, 2, Tightening::UB ),

            Tightening( 10, -2, Tightening::LB ),   Tightening( 10, 2, Tightening::UB ),
            Tightening( 11, -4.5, Tightening::LB ), Tightening( 11, 6.5, Tightening::UB ),
        } );

        List<Tightening> bounds;
        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );
        TS_ASSERT( boundsEqual( bounds, expectedBounds ) );


        // Invoke backward LP propagation
        TS_ASSERT_THROWS_NOTHING( updateTableau( tableau, bounds ) );
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );
        TS_ASSERT_THROWS_NOTHING( nlr.lpRelaxationPropagation() );

        List<Tightening> expectedBounds2( {
            Tightening( 11, -4, Tightening::LB ),
            Tightening( 11, 6, Tightening::UB ),
        } );

        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );
        TS_ASSERT( boundsEqual( bounds, expectedBounds2 ) );


        // Change the current bounds
        tableau.setLowerBound( 0, -3 );
        tableau.setUpperBound( 0, 1 );
        tableau.setLowerBound( 1, -1 );
        tableau.setUpperBound( 1, 2 );

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


        // Invoke DeepPoly
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );
        TS_ASSERT_THROWS_NOTHING( nlr.deepPolyPropagation() );

        List<Tightening> expectedBounds3( {
            Tightening( 2, -5, Tightening::LB ),     Tightening( 2, 2, Tightening::UB ),
            Tightening( 3, -4, Tightening::LB ),     Tightening( 3, 3, Tightening::UB ),

            Tightening( 4, -5, Tightening::LB ),     Tightening( 4, 2, Tightening::UB ),
            Tightening( 5, -4, Tightening::LB ),     Tightening( 5, 3, Tightening::UB ),

            Tightening( 6, -3, Tightening::LB ),     Tightening( 6, 5, Tightening::UB ),
            Tightening( 7, -10.5, Tightening::LB ),  Tightening( 7, 5.5, Tightening::UB ),

            Tightening( 8, -3, Tightening::LB ),     Tightening( 8, 5, Tightening::UB ),
            Tightening( 9, -10, Tightening::LB ),    Tightening( 9, 6, Tightening::UB ),

            Tightening( 10, -3, Tightening::LB ),    Tightening( 10, 6, Tightening::UB ),
            Tightening( 11, -15.5, Tightening::LB ), Tightening( 11, 11.5, Tightening::UB ),
        } );

        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );
        TS_ASSERT( boundsEqual( bounds, expectedBounds3 ) );


        // Invoke backward LP propagation
        TS_ASSERT_THROWS_NOTHING( updateTableau( tableau, bounds ) );
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );
        TS_ASSERT_THROWS_NOTHING( nlr.lpRelaxationPropagation() );

        List<Tightening> expectedBounds4( {
            Tightening( 7, -10, Tightening::LB ),
            Tightening( 7, 5, Tightening::UB ),

            Tightening( 11, -14, Tightening::LB ),
            Tightening( 11, 11, Tightening::UB ),
        } );

        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );
        TS_ASSERT( boundsEqual( bounds, expectedBounds4 ) );
    }

    void test_backwards_round2()
    {
        Options::get()->setString( Options::SYMBOLIC_BOUND_TIGHTENING_TYPE, "sbt" );
        Options::get()->setString( Options::MILP_SOLVER_BOUND_TIGHTENING_TYPE,
                                   "backward-converge" );

        NLR::NetworkLevelReasoner nlr;
        MockTableau tableau;
        nlr.setTableau( &tableau );
        populateNetworkBackwardRound2( nlr, tableau );

        tableau.setLowerBound( 0, -1 );
        tableau.setUpperBound( 0, 1 );
        tableau.setLowerBound( 1, -1 );
        tableau.setUpperBound( 1, 1 );
        tableau.setLowerBound( 2, -1 );
        tableau.setUpperBound( 2, 1 );


        // Invoke DeepPoly
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );
        TS_ASSERT_THROWS_NOTHING( nlr.deepPolyPropagation() );

        List<Tightening> expectedBounds( {
            Tightening( 3, -2, Tightening::LB ),     Tightening( 3, 2, Tightening::UB ),
            Tightening( 4, -3, Tightening::LB ),     Tightening( 4, 3, Tightening::UB ),
            Tightening( 5, -3, Tightening::LB ),     Tightening( 5, 3, Tightening::UB ),
            Tightening( 6, -6, Tightening::LB ),     Tightening( 6, 6, Tightening::UB ),

            Tightening( 7, -2, Tightening::LB ),     Tightening( 7, 2, Tightening::UB ),
            Tightening( 8, -3, Tightening::LB ),     Tightening( 8, 3, Tightening::UB ),
            Tightening( 9, -3, Tightening::LB ),     Tightening( 9, 3, Tightening::UB ),
            Tightening( 10, -6, Tightening::LB ),    Tightening( 10, 6, Tightening::UB ),

            Tightening( 11, -11, Tightening::LB ),   Tightening( 11, 15, Tightening::UB ),
            Tightening( 12, -10, Tightening::LB ),   Tightening( 12, 10, Tightening::UB ),
            Tightening( 13, -7, Tightening::LB ),    Tightening( 13, 3, Tightening::UB ),

            Tightening( 14, -11, Tightening::LB ),   Tightening( 14, 15, Tightening::UB ),
            Tightening( 15, -10, Tightening::LB ),   Tightening( 15, 10, Tightening::UB ),
            Tightening( 16, -7, Tightening::LB ),    Tightening( 16, 3, Tightening::UB ),

            Tightening( 17, -27.5, Tightening::LB ), Tightening( 17, 27.5, Tightening::UB ),
            Tightening( 18, -16.5, Tightening::LB ), Tightening( 18, 16.5, Tightening::UB ),

            Tightening( 19, -28, Tightening::LB ),   Tightening( 19, 28, Tightening::UB ),
            Tightening( 20, -17, Tightening::LB ),   Tightening( 20, 17, Tightening::UB ),

            Tightening( 21, -38, Tightening::LB ),   Tightening( 21, 36, Tightening::UB ),
        } );

        List<Tightening> bounds;
        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );
        TS_ASSERT( boundsEqual( bounds, expectedBounds ) );


        // Invoke backward LP propagation
        TS_ASSERT_THROWS_NOTHING( updateTableau( tableau, bounds ) );
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );
        TS_ASSERT_THROWS_NOTHING( nlr.lpRelaxationPropagation() );

        List<Tightening> expectedBounds2( {} );

        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );
        TS_ASSERT( boundsEqual( bounds, expectedBounds2 ) );


        // Change the current bounds
        tableau.setLowerBound( 0, -3 );
        tableau.setUpperBound( 0, 1 );
        tableau.setLowerBound( 1, -1 );
        tableau.setUpperBound( 1, 2 );
        tableau.setLowerBound( 2, -2 );
        tableau.setUpperBound( 2, 2 );

        double large = 1000000;
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
        tableau.setLowerBound( 17, -large );
        tableau.setUpperBound( 17, large );
        tableau.setLowerBound( 18, -large );
        tableau.setUpperBound( 18, large );
        tableau.setLowerBound( 19, -large );
        tableau.setUpperBound( 19, large );
        tableau.setLowerBound( 20, -large );
        tableau.setUpperBound( 20, large );
        tableau.setLowerBound( 21, -large );
        tableau.setUpperBound( 21, large );


        // Invoke DeepPoly
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );
        TS_ASSERT_THROWS_NOTHING( nlr.deepPolyPropagation() );

        List<Tightening> expectedBounds3( {
            Tightening( 3, -2, Tightening::LB ),     Tightening( 3, 5, Tightening::UB ),
            Tightening( 4, -5, Tightening::LB ),     Tightening( 4, 5, Tightening::UB ),
            Tightening( 5, -6, Tightening::LB ),     Tightening( 5, 5, Tightening::UB ),
            Tightening( 6, -15, Tightening::LB ),    Tightening( 6, 7, Tightening::UB ),

            Tightening( 7, -2, Tightening::LB ),     Tightening( 7, 5, Tightening::UB ),
            Tightening( 8, -5, Tightening::LB ),     Tightening( 8, 5, Tightening::UB ),
            Tightening( 9, -6, Tightening::LB ),     Tightening( 9, 5, Tightening::UB ),
            Tightening( 10, -15, Tightening::LB ),   Tightening( 10, 7, Tightening::UB ),

            Tightening( 11, -13, Tightening::LB ),   Tightening( 11, 30, Tightening::UB ),
            Tightening( 12, -23, Tightening::LB ),   Tightening( 12, 12, Tightening::UB ),
            Tightening( 13, -9, Tightening::LB ),    Tightening( 13, 6, Tightening::UB ),

            Tightening( 14, -13, Tightening::LB ),   Tightening( 14, 30, Tightening::UB ),
            Tightening( 15, -23, Tightening::LB ),   Tightening( 15, 12, Tightening::UB ),
            Tightening( 16, -9, Tightening::LB ),    Tightening( 16, 6, Tightening::UB ),

            Tightening( 17, -57.5, Tightening::LB ), Tightening( 17, 32.5, Tightening::UB ),
            Tightening( 18, -23.5, Tightening::LB ), Tightening( 18, 26.5, Tightening::UB ),

            Tightening( 19, -58, Tightening::LB ),   Tightening( 19, 33, Tightening::UB ),
            Tightening( 20, -24, Tightening::LB ),   Tightening( 20, 27, Tightening::UB ),

            Tightening( 21, -74, Tightening::LB ),   Tightening( 21, 44, Tightening::UB ),
        } );

        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );
        TS_ASSERT( boundsEqual( bounds, expectedBounds3 ) );


        // Invoke backward LP propagation
        TS_ASSERT_THROWS_NOTHING( updateTableau( tableau, bounds ) );
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );
        TS_ASSERT_THROWS_NOTHING( nlr.lpRelaxationPropagation() );

        List<Tightening> expectedBounds4( {} );

        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );
        TS_ASSERT( boundsEqual( bounds, expectedBounds4 ) );
    }

    void test_backwards_sign()
    {
        Options::get()->setString( Options::SYMBOLIC_BOUND_TIGHTENING_TYPE, "sbt" );
        Options::get()->setString( Options::MILP_SOLVER_BOUND_TIGHTENING_TYPE,
                                   "backward-converge" );

        NLR::NetworkLevelReasoner nlr;
        MockTableau tableau;
        nlr.setTableau( &tableau );
        populateNetworkBackwardSign( nlr, tableau );

        tableau.setLowerBound( 0, 0 );
        tableau.setUpperBound( 0, 1 );
        tableau.setLowerBound( 1, 0 );
        tableau.setUpperBound( 1, 1 );


        // Invoke DeepPoly
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );
        TS_ASSERT_THROWS_NOTHING( nlr.deepPolyPropagation() );

        List<Tightening> expectedBounds( {
            Tightening( 2, -1, Tightening::LB ),  Tightening( 2, 1, Tightening::UB ),
            Tightening( 3, 0, Tightening::LB ),   Tightening( 3, 2, Tightening::UB ),

            Tightening( 4, -1, Tightening::LB ),  Tightening( 4, 1, Tightening::UB ),
            Tightening( 5, 1, Tightening::LB ),   Tightening( 5, 1, Tightening::UB ),

            Tightening( 6, 0, Tightening::LB ),   Tightening( 6, 2, Tightening::UB ),
            Tightening( 7, -3, Tightening::LB ),  Tightening( 7, 1, Tightening::UB ),

            Tightening( 8, 1, Tightening::LB ),   Tightening( 8, 1, Tightening::UB ),
            Tightening( 9, -1, Tightening::LB ),  Tightening( 9, 1, Tightening::UB ),

            Tightening( 10, -2, Tightening::LB ), Tightening( 10, 0, Tightening::UB ),
            Tightening( 11, 1, Tightening::LB ),  Tightening( 11, 5, Tightening::UB ),
        } );

        List<Tightening> bounds;
        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );
        TS_ASSERT( boundsEqual( bounds, expectedBounds ) );


        // Invoke backward LP propagation
        TS_ASSERT_THROWS_NOTHING( updateTableau( tableau, bounds ) );
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );
        TS_ASSERT_THROWS_NOTHING( nlr.lpRelaxationPropagation() );

        List<Tightening> expectedBounds2( {} );

        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );
        TS_ASSERT( boundsEqual( bounds, expectedBounds2 ) );


        // Change the current bounds
        tableau.setLowerBound( 0, -3 );
        tableau.setUpperBound( 0, 1 );
        tableau.setLowerBound( 1, -1 );
        tableau.setUpperBound( 1, 2 );

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


        // Invoke DeepPoly
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );
        TS_ASSERT_THROWS_NOTHING( nlr.deepPolyPropagation() );

        List<Tightening> expectedBounds3( {
            Tightening( 2, -5, Tightening::LB ),  Tightening( 2, 2, Tightening::UB ),
            Tightening( 3, -4, Tightening::LB ),  Tightening( 3, 3, Tightening::UB ),

            Tightening( 4, -1, Tightening::LB ),  Tightening( 4, 1, Tightening::UB ),
            Tightening( 5, -1, Tightening::LB ),  Tightening( 5, 1, Tightening::UB ),

            Tightening( 6, -2, Tightening::LB ),  Tightening( 6, 2, Tightening::UB ),
            Tightening( 7, -3, Tightening::LB ),  Tightening( 7, 3, Tightening::UB ),

            Tightening( 8, -1, Tightening::LB ),  Tightening( 8, 1, Tightening::UB ),
            Tightening( 9, -1, Tightening::LB ),  Tightening( 9, 1, Tightening::UB ),

            Tightening( 10, -2, Tightening::LB ), Tightening( 10, 2, Tightening::UB ),
            Tightening( 11, -1, Tightening::LB ), Tightening( 11, 5, Tightening::UB ),
        } );

        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );
        TS_ASSERT( boundsEqual( bounds, expectedBounds3 ) );


        // Invoke backward LP propagation
        TS_ASSERT_THROWS_NOTHING( updateTableau( tableau, bounds ) );
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );
        TS_ASSERT_THROWS_NOTHING( nlr.lpRelaxationPropagation() );

        List<Tightening> expectedBounds4( {} );

        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );
        TS_ASSERT( boundsEqual( bounds, expectedBounds4 ) );
    }

    void test_backwards_sign2()
    {
        Options::get()->setString( Options::SYMBOLIC_BOUND_TIGHTENING_TYPE, "sbt" );
        Options::get()->setString( Options::MILP_SOLVER_BOUND_TIGHTENING_TYPE,
                                   "backward-converge" );

        NLR::NetworkLevelReasoner nlr;
        MockTableau tableau;
        nlr.setTableau( &tableau );
        populateNetworkBackwardSign2( nlr, tableau );

        tableau.setLowerBound( 0, -1 );
        tableau.setUpperBound( 0, 1 );
        tableau.setLowerBound( 1, -1 );
        tableau.setUpperBound( 1, 1 );
        tableau.setLowerBound( 2, -1 );
        tableau.setUpperBound( 2, 1 );


        // Invoke DeepPoly
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );
        TS_ASSERT_THROWS_NOTHING( nlr.deepPolyPropagation() );

        List<Tightening> expectedBounds( {
            Tightening( 3, -2, Tightening::LB ),  Tightening( 3, 2, Tightening::UB ),
            Tightening( 4, -3, Tightening::LB ),  Tightening( 4, 3, Tightening::UB ),
            Tightening( 5, -3, Tightening::LB ),  Tightening( 5, 3, Tightening::UB ),
            Tightening( 6, -6, Tightening::LB ),  Tightening( 6, 6, Tightening::UB ),

            Tightening( 7, -1, Tightening::LB ),  Tightening( 7, 1, Tightening::UB ),
            Tightening( 8, -1, Tightening::LB ),  Tightening( 8, 1, Tightening::UB ),
            Tightening( 9, -1, Tightening::LB ),  Tightening( 9, 1, Tightening::UB ),
            Tightening( 10, -1, Tightening::LB ), Tightening( 10, 1, Tightening::UB ),

            Tightening( 11, -2, Tightening::LB ), Tightening( 11, 6, Tightening::UB ),
            Tightening( 12, -4, Tightening::LB ), Tightening( 12, 4, Tightening::UB ),
            Tightening( 13, -6, Tightening::LB ), Tightening( 13, 2, Tightening::UB ),

            Tightening( 14, -1, Tightening::LB ), Tightening( 14, 1, Tightening::UB ),
            Tightening( 15, -1, Tightening::LB ), Tightening( 15, 1, Tightening::UB ),
            Tightening( 16, -1, Tightening::LB ), Tightening( 16, 1, Tightening::UB ),

            Tightening( 17, -3, Tightening::LB ), Tightening( 17, 3, Tightening::UB ),
            Tightening( 18, -3, Tightening::LB ), Tightening( 18, 3, Tightening::UB ),

            Tightening( 19, -1, Tightening::LB ), Tightening( 19, 1, Tightening::UB ),
            Tightening( 20, -1, Tightening::LB ), Tightening( 20, 1, Tightening::UB ),

            Tightening( 21, -3, Tightening::LB ), Tightening( 21, 1, Tightening::UB ),
        } );

        List<Tightening> bounds;
        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );
        TS_ASSERT( boundsEqual( bounds, expectedBounds ) );


        // Invoke backward LP propagation
        TS_ASSERT_THROWS_NOTHING( updateTableau( tableau, bounds ) );
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );
        TS_ASSERT_THROWS_NOTHING( nlr.lpRelaxationPropagation() );

        List<Tightening> expectedBounds2( {} );

        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );
        TS_ASSERT( boundsEqual( bounds, expectedBounds2 ) );


        // Change the current bounds
        tableau.setLowerBound( 0, -3 );
        tableau.setUpperBound( 0, 1 );
        tableau.setLowerBound( 1, -1 );
        tableau.setUpperBound( 1, 2 );
        tableau.setLowerBound( 2, -2 );
        tableau.setUpperBound( 2, 2 );

        double large = 1000000;
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
        tableau.setLowerBound( 17, -large );
        tableau.setUpperBound( 17, large );
        tableau.setLowerBound( 18, -large );
        tableau.setUpperBound( 18, large );
        tableau.setLowerBound( 19, -large );
        tableau.setUpperBound( 19, large );
        tableau.setLowerBound( 20, -large );
        tableau.setUpperBound( 20, large );
        tableau.setLowerBound( 21, -large );
        tableau.setUpperBound( 21, large );


        // Invoke DeepPoly
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );
        TS_ASSERT_THROWS_NOTHING( nlr.deepPolyPropagation() );

        List<Tightening> expectedBounds3( {
            Tightening( 3, -2, Tightening::LB ),  Tightening( 3, 5, Tightening::UB ),
            Tightening( 4, -5, Tightening::LB ),  Tightening( 4, 5, Tightening::UB ),
            Tightening( 5, -6, Tightening::LB ),  Tightening( 5, 5, Tightening::UB ),
            Tightening( 6, -15, Tightening::LB ), Tightening( 6, 7, Tightening::UB ),

            Tightening( 7, -1, Tightening::LB ),  Tightening( 7, 1, Tightening::UB ),
            Tightening( 8, -1, Tightening::LB ),  Tightening( 8, 1, Tightening::UB ),
            Tightening( 9, -1, Tightening::LB ),  Tightening( 9, 1, Tightening::UB ),
            Tightening( 10, -1, Tightening::LB ), Tightening( 10, 1, Tightening::UB ),

            Tightening( 11, -2, Tightening::LB ), Tightening( 11, 6, Tightening::UB ),
            Tightening( 12, -4, Tightening::LB ), Tightening( 12, 4, Tightening::UB ),
            Tightening( 13, -6, Tightening::LB ), Tightening( 13, 2, Tightening::UB ),

            Tightening( 14, -1, Tightening::LB ), Tightening( 14, 1, Tightening::UB ),
            Tightening( 15, -1, Tightening::LB ), Tightening( 15, 1, Tightening::UB ),
            Tightening( 16, -1, Tightening::LB ), Tightening( 16, 1, Tightening::UB ),

            Tightening( 17, -3, Tightening::LB ), Tightening( 17, 3, Tightening::UB ),
            Tightening( 18, -3, Tightening::LB ), Tightening( 18, 3, Tightening::UB ),

            Tightening( 19, -1, Tightening::LB ), Tightening( 19, 1, Tightening::UB ),
            Tightening( 20, -1, Tightening::LB ), Tightening( 20, 1, Tightening::UB ),

            Tightening( 21, -3, Tightening::LB ), Tightening( 21, 1, Tightening::UB ),
        } );

        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );
        TS_ASSERT( boundsEqual( bounds, expectedBounds3 ) );


        // Invoke backward LP propagation
        TS_ASSERT_THROWS_NOTHING( updateTableau( tableau, bounds ) );
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );
        TS_ASSERT_THROWS_NOTHING( nlr.lpRelaxationPropagation() );

        List<Tightening> expectedBounds4( {} );

        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );
        TS_ASSERT( boundsEqual( bounds, expectedBounds4 ) );
    }

    void test_backwards_leaky_relu()
    {
        Options::get()->setString( Options::SYMBOLIC_BOUND_TIGHTENING_TYPE, "sbt" );
        Options::get()->setString( Options::MILP_SOLVER_BOUND_TIGHTENING_TYPE,
                                   "backward-converge" );

        NLR::NetworkLevelReasoner nlr;
        MockTableau tableau;
        nlr.setTableau( &tableau );
        populateNetworkBackwardLeakyReLU( nlr, tableau );

        tableau.setLowerBound( 0, 0 );
        tableau.setUpperBound( 0, 1 );
        tableau.setLowerBound( 1, 0 );
        tableau.setUpperBound( 1, 1 );


        // Invoke DeepPoly
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );
        TS_ASSERT_THROWS_NOTHING( nlr.deepPolyPropagation() );

        List<Tightening> expectedBounds( {
            Tightening( 2, -1, Tightening::LB ),      Tightening( 2, 1, Tightening::UB ),
            Tightening( 3, 0, Tightening::LB ),       Tightening( 3, 2, Tightening::UB ),

            Tightening( 4, -1, Tightening::LB ),      Tightening( 4, 1, Tightening::UB ),
            Tightening( 5, 0, Tightening::LB ),       Tightening( 5, 2, Tightening::UB ),

            Tightening( 6, -0.45, Tightening::LB ),   Tightening( 6, 2, Tightening::UB ),
            Tightening( 7, -3, Tightening::LB ),      Tightening( 7, 1, Tightening::UB ),

            Tightening( 8, -0.45, Tightening::LB ),   Tightening( 8, 2, Tightening::UB ),
            Tightening( 9, -3, Tightening::LB ),      Tightening( 9, 1, Tightening::UB ),

            Tightening( 10, -2.025, Tightening::LB ), Tightening( 10, 1, Tightening::UB ),
            Tightening( 11, -2, Tightening::LB ),     Tightening( 11, 4.3306, Tightening::UB ),
        } );

        List<Tightening> bounds;
        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );
        TS_ASSERT( boundsEqual( bounds, expectedBounds ) );


        // Invoke backward LP propagation
        TS_ASSERT_THROWS_NOTHING( updateTableau( tableau, bounds ) );
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );
        TS_ASSERT_THROWS_NOTHING( nlr.lpRelaxationPropagation() );

        List<Tightening> expectedBounds2( {
            Tightening( 4, -0.1, Tightening::LB ),

            Tightening( 8, -0.045, Tightening::LB ),
            Tightening( 9, -0.3, Tightening::LB ),
        } );

        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );
        TS_ASSERT( boundsEqual( bounds, expectedBounds2 ) );


        // Change the current bounds
        tableau.setLowerBound( 0, -3 );
        tableau.setUpperBound( 0, 1 );
        tableau.setLowerBound( 1, -1 );
        tableau.setUpperBound( 1, 2 );

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


        // Invoke DeepPoly
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );
        TS_ASSERT_THROWS_NOTHING( nlr.deepPolyPropagation() );

        List<Tightening> expectedBounds3( {
            Tightening( 2, -5, Tightening::LB ),        Tightening( 2, 2, Tightening::UB ),
            Tightening( 3, -4, Tightening::LB ),        Tightening( 3, 3, Tightening::UB ),

            Tightening( 4, -5, Tightening::LB ),        Tightening( 4, 2, Tightening::UB ),
            Tightening( 5, -4, Tightening::LB ),        Tightening( 5, 3, Tightening::UB ),

            Tightening( 6, -4.5714, Tightening::LB ),   Tightening( 6, 6.0571, Tightening::UB ),
            Tightening( 7, -11.0571, Tightening::LB ),  Tightening( 7, 5.1429, Tightening::UB ),

            Tightening( 8, -4.5714, Tightening::LB ),   Tightening( 8, 6.0571, Tightening::UB ),
            Tightening( 9, -11.0571, Tightening::LB ),  Tightening( 9, 5.1429, Tightening::UB ),

            Tightening( 10, -6.3327, Tightening::LB ),  Tightening( 10, 5, Tightening::UB ),
            Tightening( 11, -14.0571, Tightening::LB ), Tightening( 11, 12.523, Tightening::UB ),
        } );

        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );
        TS_ASSERT( boundsEqual( bounds, expectedBounds3 ) );


        // Invoke backward LP propagation
        TS_ASSERT_THROWS_NOTHING( updateTableau( tableau, bounds ) );
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );
        TS_ASSERT_THROWS_NOTHING( nlr.lpRelaxationPropagation() );

        List<Tightening> expectedBounds4( {
            Tightening( 4, -0.5, Tightening::LB ),
            Tightening( 5, -0.4, Tightening::LB ),

            Tightening( 8, -0.4571, Tightening::LB ),
            Tightening( 9, -1.1057, Tightening::LB ),
        } );

        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );
        TS_ASSERT( boundsEqual( bounds, expectedBounds4 ) );
    }

    void test_backwards_leaky_relu2()
    {
        Options::get()->setString( Options::SYMBOLIC_BOUND_TIGHTENING_TYPE, "sbt" );
        Options::get()->setString( Options::MILP_SOLVER_BOUND_TIGHTENING_TYPE,
                                   "backward-converge" );

        NLR::NetworkLevelReasoner nlr;
        MockTableau tableau;
        nlr.setTableau( &tableau );
        populateNetworkBackwardLeakyRelu2( nlr, tableau );

        tableau.setLowerBound( 0, -1 );
        tableau.setUpperBound( 0, 1 );
        tableau.setLowerBound( 1, -1 );
        tableau.setUpperBound( 1, 1 );
        tableau.setLowerBound( 2, -1 );
        tableau.setUpperBound( 2, 1 );


        // Invoke DeepPoly
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );
        TS_ASSERT_THROWS_NOTHING( nlr.deepPolyPropagation() );

        List<Tightening> expectedBounds( {
            Tightening( 3, -2, Tightening::LB ),        Tightening( 3, 2, Tightening::UB ),
            Tightening( 4, -3, Tightening::LB ),        Tightening( 4, 3, Tightening::UB ),
            Tightening( 5, -3, Tightening::LB ),        Tightening( 5, 3, Tightening::UB ),
            Tightening( 6, -6, Tightening::LB ),        Tightening( 6, 6, Tightening::UB ),

            Tightening( 7, -2, Tightening::LB ),        Tightening( 7, 2, Tightening::UB ),
            Tightening( 8, -3, Tightening::LB ),        Tightening( 8, 3, Tightening::UB ),
            Tightening( 9, -3, Tightening::LB ),        Tightening( 9, 3, Tightening::UB ),
            Tightening( 10, -6, Tightening::LB ),       Tightening( 10, 6, Tightening::UB ),

            Tightening( 11, -9, Tightening::LB ),       Tightening( 11, 13.9, Tightening::UB ),
            Tightening( 12, -8.9, Tightening::LB ),     Tightening( 12, 9.8, Tightening::UB ),
            Tightening( 13, -7.7, Tightening::LB ),     Tightening( 13, 3.5, Tightening::UB ),

            Tightening( 14, -9, Tightening::LB ),       Tightening( 14, 13.9, Tightening::UB ),
            Tightening( 15, -8.9, Tightening::LB ),     Tightening( 15, 9.8, Tightening::UB ),
            Tightening( 16, -7.7, Tightening::LB ),     Tightening( 16, 3.5, Tightening::UB ),

            Tightening( 17, -23.1331, Tightening::LB ), Tightening( 17, 25.4857, Tightening::UB ),
            Tightening( 18, -12, Tightening::LB ),      Tightening( 18, 19.3146, Tightening::UB ),

            Tightening( 19, -23.1331, Tightening::LB ), Tightening( 19, 25.4857, Tightening::UB ),
            Tightening( 20, -12, Tightening::LB ),      Tightening( 20, 19.3146, Tightening::UB ),

            Tightening( 21, -38.0879, Tightening::LB ), Tightening( 21, 30.6367, Tightening::UB ),
        } );

        List<Tightening> bounds;
        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );
        TS_ASSERT( boundsEqual( bounds, expectedBounds ) );


        // Invoke backward LP propagation
        TS_ASSERT_THROWS_NOTHING( updateTableau( tableau, bounds ) );
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );
        TS_ASSERT_THROWS_NOTHING( nlr.lpRelaxationPropagation() );

        List<Tightening> expectedBounds2( {
            Tightening( 7, -0.2, Tightening::LB ),
            Tightening( 8, -0.3, Tightening::LB ),
            Tightening( 9, -0.3, Tightening::LB ),
            Tightening( 10, -0.6, Tightening::LB ),

            Tightening( 14, -0.9, Tightening::LB ),
            Tightening( 15, -0.89, Tightening::LB ),
            Tightening( 16, -0.77, Tightening::LB ),

            Tightening( 19, -2.3133, Tightening::LB ),
            Tightening( 20, -1.2, Tightening::LB ),
        } );

        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );
        TS_ASSERT( boundsEqual( bounds, expectedBounds2 ) );


        // Change the current bounds
        tableau.setLowerBound( 0, -3 );
        tableau.setUpperBound( 0, 1 );
        tableau.setLowerBound( 1, -1 );
        tableau.setUpperBound( 1, 2 );
        tableau.setLowerBound( 2, -2 );
        tableau.setUpperBound( 2, 2 );

        double large = 1000000;
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
        tableau.setLowerBound( 17, -large );
        tableau.setUpperBound( 17, large );
        tableau.setLowerBound( 18, -large );
        tableau.setUpperBound( 18, large );
        tableau.setLowerBound( 19, -large );
        tableau.setUpperBound( 19, large );
        tableau.setLowerBound( 20, -large );
        tableau.setUpperBound( 20, large );
        tableau.setLowerBound( 21, -large );
        tableau.setUpperBound( 21, large );


        // Invoke DeepPoly
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );
        TS_ASSERT_THROWS_NOTHING( nlr.deepPolyPropagation() );

        List<Tightening> expectedBounds3( {
            Tightening( 3, -2, Tightening::LB ),        Tightening( 3, 5, Tightening::UB ),
            Tightening( 4, -5, Tightening::LB ),        Tightening( 4, 5, Tightening::UB ),
            Tightening( 5, -6, Tightening::LB ),        Tightening( 5, 5, Tightening::UB ),
            Tightening( 6, -15, Tightening::LB ),       Tightening( 6, 7, Tightening::UB ),

            Tightening( 7, -2, Tightening::LB ),        Tightening( 7, 5, Tightening::UB ),
            Tightening( 8, -5, Tightening::LB ),        Tightening( 8, 5, Tightening::UB ),
            Tightening( 9, -6, Tightening::LB ),        Tightening( 9, 5, Tightening::UB ),
            Tightening( 10, -15, Tightening::LB ),      Tightening( 10, 7, Tightening::UB ),

            Tightening( 11, -11, Tightening::LB ),      Tightening( 11, 29.9636, Tightening::UB ),
            Tightening( 12, -21.7714, Tightening::LB ), Tightening( 12, 13.6818, Tightening::UB ),
            Tightening( 13, -11.5, Tightening::LB ),    Tightening( 13, 8.6442, Tightening::UB ),

            Tightening( 14, -11, Tightening::LB ),      Tightening( 14, 29.9636, Tightening::UB ),
            Tightening( 15, -21.7714, Tightening::LB ), Tightening( 15, 13.6818, Tightening::UB ),
            Tightening( 16, -11.5, Tightening::LB ),    Tightening( 16, 8.6442, Tightening::UB ),

            Tightening( 17, -56.2592, Tightening::LB ), Tightening( 17, 33.8084, Tightening::UB ),
            Tightening( 18, -19, Tightening::LB ),      Tightening( 18, 38.5043, Tightening::UB ),

            Tightening( 19, -56.2592, Tightening::LB ), Tightening( 19, 33.8084, Tightening::UB ),
            Tightening( 20, -19, Tightening::LB ),      Tightening( 20, 38.5043, Tightening::UB ),

            Tightening( 21, -82.9440, Tightening::LB ), Tightening( 21, 40.7983, Tightening::UB ),
        } );

        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );
        TS_ASSERT( boundsEqual( bounds, expectedBounds3 ) );


        // Invoke backward LP propagation
        TS_ASSERT_THROWS_NOTHING( updateTableau( tableau, bounds ) );
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );
        TS_ASSERT_THROWS_NOTHING( nlr.lpRelaxationPropagation() );

        List<Tightening> expectedBounds4( {
            Tightening( 7, -0.2, Tightening::LB ),
            Tightening( 8, -0.5, Tightening::LB ),
            Tightening( 9, -0.6, Tightening::LB ),
            Tightening( 10, -1.5, Tightening::LB ),

            Tightening( 14, -1.1, Tightening::LB ),
            Tightening( 15, -2.1771, Tightening::LB ),
            Tightening( 16, -1.15, Tightening::LB ),

            Tightening( 19, -5.6259, Tightening::LB ),
            Tightening( 20, -1.9, Tightening::LB ),
        } );

        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );
        TS_ASSERT( boundsEqual( bounds, expectedBounds4 ) );
    }

    void test_backwards_softmax_and_max()
    {
        Options::get()->setString( Options::SYMBOLIC_BOUND_TIGHTENING_TYPE, "sbt" );
        Options::get()->setString( Options::SOFTMAX_BOUND_TYPE, "lse" );
        Options::get()->setString( Options::MILP_SOLVER_BOUND_TIGHTENING_TYPE,
                                   "backward-converge" );

        NLR::NetworkLevelReasoner nlr;
        MockTableau tableau;
        nlr.setTableau( &tableau );
        populateNetworkBackwardSoftmaxAndMax( nlr, tableau );

        tableau.setLowerBound( 0, 0 );
        tableau.setUpperBound( 0, 1 );
        tableau.setLowerBound( 1, 0 );
        tableau.setUpperBound( 1, 1 );


        // Invoke SBT
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );
        TS_ASSERT_THROWS_NOTHING( nlr.deepPolyPropagation() );

        List<Tightening> expectedBounds( {
            Tightening( 2, 1, Tightening::LB ),        Tightening( 2, 2, Tightening::UB ),
            Tightening( 4, -3, Tightening::LB ),       Tightening( 4, 2, Tightening::UB ),
            Tightening( 6, 0, Tightening::LB ),        Tightening( 6, 1, Tightening::UB ),

            Tightening( 3, 0.2119, Tightening::LB ),   Tightening( 3, 0.8756, Tightening::UB ),
            Tightening( 5, 0.0049, Tightening::LB ),   Tightening( 5, 0.6652, Tightening::UB ),
            Tightening( 7, 0.0634, Tightening::LB ),   Tightening( 7, 0.4955, Tightening::UB ),

            Tightening( 8, -0.1870, Tightening::LB ),  Tightening( 8, 1.4488, Tightening::UB ),
            Tightening( 9, 0.6814, Tightening::LB ),   Tightening( 9, 2.2432, Tightening::UB ),

            Tightening( 10, 0.6814, Tightening::LB ),  Tightening( 10, 2.2432, Tightening::UB ),

            Tightening( 11, -2.2432, Tightening::LB ), Tightening( 11, -0.6814, Tightening::UB ),
        } );

        List<Tightening> bounds;
        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );
        TS_ASSERT( boundsEqual( bounds, expectedBounds ) );


        // Invoke backward LP propagation
        TS_ASSERT_THROWS_NOTHING( updateTableau( tableau, bounds ) );
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );
        TS_ASSERT_THROWS_NOTHING( nlr.lpRelaxationPropagation() );

        List<Tightening> expectedBounds2( {} );

        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );
        TS_ASSERT( boundsEqual( bounds, expectedBounds2 ) );


        // Change the current bounds
        tableau.setLowerBound( 0, -3 );
        tableau.setUpperBound( 0, 1 );
        tableau.setLowerBound( 1, -1 );
        tableau.setUpperBound( 1, 2 );

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


        // Invoke DeepPoly
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );
        TS_ASSERT_THROWS_NOTHING( nlr.deepPolyPropagation() );

        List<Tightening> expectedBounds3( {
            Tightening( 2, -2, Tightening::LB ),       Tightening( 2, 2, Tightening::UB ),
            Tightening( 4, -12, Tightening::LB ),      Tightening( 4, 5, Tightening::UB ),
            Tightening( 6, -1, Tightening::LB ),       Tightening( 6, 2, Tightening::UB ),

            Tightening( 3, 0.0009, Tightening::LB ),   Tightening( 3, 0.9526, Tightening::UB ),
            Tightening( 5, 0, Tightening::LB ),        Tightening( 5, 0.9966, Tightening::UB ),
            Tightening( 7, 0.0024, Tightening::LB ),   Tightening( 7, 0.9820, Tightening::UB ),

            Tightening( 8, -0.9811, Tightening::LB ),  Tightening( 8, 1.9468, Tightening::UB ),
            Tightening( 9, 0.2194, Tightening::LB ),   Tightening( 9, 2.9934, Tightening::UB ),

            Tightening( 10, 0.2194, Tightening::LB ),  Tightening( 10, 2.9934, Tightening::UB ),

            Tightening( 11, -2.9934, Tightening::LB ), Tightening( 11, -0.2194, Tightening::UB ),
        } );

        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );
        TS_ASSERT( boundsEqual( bounds, expectedBounds3 ) );


        // Invoke backward LP propagation
        TS_ASSERT_THROWS_NOTHING( updateTableau( tableau, bounds ) );
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );
        TS_ASSERT_THROWS_NOTHING( nlr.lpRelaxationPropagation() );

        List<Tightening> expectedBounds4( {} );

        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );
        TS_ASSERT( boundsEqual( bounds, expectedBounds4 ) );
    }

    void test_backwards_softmax_and_max2()
    {
        Options::get()->setString( Options::SYMBOLIC_BOUND_TIGHTENING_TYPE, "sbt" );
        Options::get()->setString( Options::SOFTMAX_BOUND_TYPE, "lse" );
        Options::get()->setString( Options::MILP_SOLVER_BOUND_TIGHTENING_TYPE,
                                   "backward-converge" );

        NLR::NetworkLevelReasoner nlr;
        MockTableau tableau;
        nlr.setTableau( &tableau );
        populateNetworkBackwardSoftmaxAndMax2( nlr, tableau );

        tableau.setLowerBound( 0, -1 );
        tableau.setUpperBound( 0, 1 );
        tableau.setLowerBound( 1, -1 );
        tableau.setUpperBound( 1, 1 );
        tableau.setLowerBound( 2, -1 );
        tableau.setUpperBound( 2, 1 );


        // Invoke DeepPoly
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );
        TS_ASSERT_THROWS_NOTHING( nlr.deepPolyPropagation() );

        List<Tightening> expectedBounds( {
            Tightening( 3, -2, Tightening::LB ),       Tightening( 3, 2, Tightening::UB ),
            Tightening( 4, -3, Tightening::LB ),       Tightening( 4, 3, Tightening::UB ),
            Tightening( 5, -3, Tightening::LB ),       Tightening( 5, 3, Tightening::UB ),
            Tightening( 6, -6, Tightening::LB ),       Tightening( 6, 6, Tightening::UB ),

            Tightening( 7, 0.0003, Tightening::LB ),   Tightening( 7, 0.9864, Tightening::UB ),
            Tightening( 8, 0.0001, Tightening::LB ),   Tightening( 8, 0.9907, Tightening::UB ),
            Tightening( 9, 0.0001, Tightening::LB ),   Tightening( 9, 0.9907, Tightening::UB ),
            Tightening( 10, 0.0001, Tightening::LB ),  Tightening( 10, 0.9994, Tightening::UB ),

            Tightening( 11, 1.0013, Tightening::LB ),  Tightening( 11, 4.9634, Tightening::UB ),
            Tightening( 12, -0.9861, Tightening::LB ), Tightening( 12, 2.9152, Tightening::UB ),
            Tightening( 13, -2.9868, Tightening::LB ), Tightening( 13, 0.9678, Tightening::UB ),

            Tightening( 14, 0.1143, Tightening::LB ),  Tightening( 14, 0.9970, Tightening::UB ),
            Tightening( 15, 0.0026, Tightening::LB ),  Tightening( 15, 0.8694, Tightening::UB ),
            Tightening( 16, 0.0003, Tightening::LB ),  Tightening( 16, 0.4596, Tightening::UB ),

            Tightening( 17, 0.1143, Tightening::LB ),  Tightening( 17, 0.9970, Tightening::UB ),
        } );

        List<Tightening> bounds;
        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );
        TS_ASSERT( boundsEqual( bounds, expectedBounds ) );


        // Invoke backward LP propagation
        TS_ASSERT_THROWS_NOTHING( updateTableau( tableau, bounds ) );
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );
        TS_ASSERT_THROWS_NOTHING( nlr.lpRelaxationPropagation() );

        List<Tightening> expectedBounds2( {} );

        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );
        TS_ASSERT( boundsEqual( bounds, expectedBounds2 ) );


        // Change the current bounds
        tableau.setLowerBound( 0, -3 );
        tableau.setUpperBound( 0, 1 );
        tableau.setLowerBound( 1, -1 );
        tableau.setUpperBound( 1, 2 );
        tableau.setLowerBound( 2, -2 );
        tableau.setUpperBound( 2, 2 );

        double large = 1000000;
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
        tableau.setLowerBound( 17, -large );
        tableau.setUpperBound( 17, large );


        // Invoke DeepPoly
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );
        TS_ASSERT_THROWS_NOTHING( nlr.deepPolyPropagation() );

        List<Tightening> expectedBounds3( {
            Tightening( 3, -2, Tightening::LB ),       Tightening( 3, 5, Tightening::UB ),
            Tightening( 4, -5, Tightening::LB ),       Tightening( 4, 5, Tightening::UB ),
            Tightening( 5, -6, Tightening::LB ),       Tightening( 5, 5, Tightening::UB ),
            Tightening( 6, -15, Tightening::LB ),      Tightening( 6, 7, Tightening::UB ),

            Tightening( 7, 0.0001, Tightening::LB ),   Tightening( 7, 0.9999, Tightening::UB ),
            Tightening( 8, 0, Tightening::LB ),        Tightening( 8, 0.9991, Tightening::UB ),
            Tightening( 9, 0, Tightening::LB ),        Tightening( 9, 0.9990, Tightening::UB ),
            Tightening( 10, 0, Tightening::LB ),       Tightening( 10, 0.9999, Tightening::UB ),

            Tightening( 11, 1.0003, Tightening::LB ),  Tightening( 11, 4.9326, Tightening::UB ),
            Tightening( 12, -0.9999, Tightening::LB ), Tightening( 12, 2.9979, Tightening::UB ),
            Tightening( 13, -2.9990, Tightening::LB ), Tightening( 13, 0.9980, Tightening::UB ),

            Tightening( 14, 0.1067, Tightening::LB ),  Tightening( 14, 0.9970, Tightening::UB ),
            Tightening( 15, 0.0026, Tightening::LB ),  Tightening( 15, 0.8786, Tightening::UB ),
            Tightening( 16, 0.0003, Tightening::LB ),  Tightening( 16, 0.4677, Tightening::UB ),

            Tightening( 17, 0.1067, Tightening::LB ),  Tightening( 17, 0.9970, Tightening::UB ),
        } );

        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );
        TS_ASSERT( boundsEqual( bounds, expectedBounds3 ) );


        // Invoke backward LP propagation
        TS_ASSERT_THROWS_NOTHING( updateTableau( tableau, bounds ) );
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );
        TS_ASSERT_THROWS_NOTHING( nlr.lpRelaxationPropagation() );

        List<Tightening> expectedBounds4( {} );

        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );
        TS_ASSERT( boundsEqual( bounds, expectedBounds4 ) );
    }

    void test_backwards_relu_and_bilinear()
    {
        Options::get()->setString( Options::SYMBOLIC_BOUND_TIGHTENING_TYPE, "sbt" );
        Options::get()->setString( Options::MILP_SOLVER_BOUND_TIGHTENING_TYPE,
                                   "backward-converge" );

        NLR::NetworkLevelReasoner nlr;
        MockTableau tableau;
        nlr.setTableau( &tableau );
        populateNetworkBackwardReluAndBilinear( nlr, tableau );

        tableau.setLowerBound( 0, 0 );
        tableau.setUpperBound( 0, 1 );
        tableau.setLowerBound( 1, 0 );
        tableau.setUpperBound( 1, 1 );


        // Invoke DeepPoly
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );
        TS_ASSERT_THROWS_NOTHING( nlr.deepPolyPropagation() );

        List<Tightening> expectedBounds( {
            Tightening( 2, 1, Tightening::LB ),     Tightening( 2, 2, Tightening::UB ),
            Tightening( 4, -3, Tightening::LB ),    Tightening( 4, 2, Tightening::UB ),
            Tightening( 6, 0, Tightening::LB ),     Tightening( 6, 1, Tightening::UB ),

            Tightening( 3, 1, Tightening::LB ),     Tightening( 3, 2, Tightening::UB ),
            Tightening( 5, 0, Tightening::LB ),     Tightening( 5, 2, Tightening::UB ),
            Tightening( 7, 0, Tightening::LB ),     Tightening( 7, 1, Tightening::UB ),

            Tightening( 8, 0, Tightening::LB ),     Tightening( 8, 4, Tightening::UB ),
            Tightening( 9, -1, Tightening::LB ),    Tightening( 9, 2.2, Tightening::UB ),

            Tightening( 10, -4, Tightening::LB ),   Tightening( 10, 8.8, Tightening::UB ),

            Tightening( 11, -8.8, Tightening::LB ), Tightening( 11, 4, Tightening::UB ),
        } );

        List<Tightening> bounds;
        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );
        TS_ASSERT( boundsEqual( bounds, expectedBounds ) );


        // Invoke backward LP propagation
        TS_ASSERT_THROWS_NOTHING( updateTableau( tableau, bounds ) );
        TS_ASSERT_THROWS_NOTHING( nlr.lpRelaxationPropagation() );

        List<Tightening> expectedBounds2( {} );

        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );
        TS_ASSERT( boundsEqual( bounds, expectedBounds2 ) );


        // Change the current bounds
        tableau.setLowerBound( 0, -3 );
        tableau.setUpperBound( 0, 1 );
        tableau.setLowerBound( 1, -1 );
        tableau.setUpperBound( 1, 2 );

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


        // Invoke DeepPoly
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );
        TS_ASSERT_THROWS_NOTHING( nlr.deepPolyPropagation() );

        List<Tightening> expectedBounds3( {
            Tightening( 2, -2, Tightening::LB ),   Tightening( 2, 2, Tightening::UB ),
            Tightening( 4, -12, Tightening::LB ),  Tightening( 4, 5, Tightening::UB ),
            Tightening( 6, -1, Tightening::LB ),   Tightening( 6, 2, Tightening::UB ),

            Tightening( 3, 0, Tightening::LB ),    Tightening( 3, 2, Tightening::UB ),
            Tightening( 5, 0, Tightening::LB ),    Tightening( 5, 5, Tightening::UB ),
            Tightening( 7, -1, Tightening::LB ),   Tightening( 7, 2, Tightening::UB ),

            Tightening( 8, -2, Tightening::LB ),   Tightening( 8, 8, Tightening::UB ),
            Tightening( 9, -2, Tightening::LB ),   Tightening( 9, 8, Tightening::UB ),

            Tightening( 10, -16, Tightening::LB ), Tightening( 10, 64, Tightening::UB ),

            Tightening( 11, -64, Tightening::LB ), Tightening( 11, 16, Tightening::UB ),
        } );

        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );
        TS_ASSERT( boundsEqual( bounds, expectedBounds3 ) );


        // Invoke backward LP propagation
        TS_ASSERT_THROWS_NOTHING( updateTableau( tableau, bounds ) );
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );
        TS_ASSERT_THROWS_NOTHING( nlr.lpRelaxationPropagation() );

        List<Tightening> expectedBounds4( {
            Tightening( 7, 0, Tightening::LB ),
        } );

        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );
        TS_ASSERT( boundsEqual( bounds, expectedBounds4 ) );
    }

    void test_backwards_relu_and_bilinear2()
    {
        Options::get()->setString( Options::SYMBOLIC_BOUND_TIGHTENING_TYPE, "sbt" );
        Options::get()->setString( Options::MILP_SOLVER_BOUND_TIGHTENING_TYPE,
                                   "backward-converge" );

        NLR::NetworkLevelReasoner nlr;
        MockTableau tableau;
        nlr.setTableau( &tableau );
        populateNetworkBackwardReluAndBilinear2( nlr, tableau );

        tableau.setLowerBound( 0, -1 );
        tableau.setUpperBound( 0, 1 );
        tableau.setLowerBound( 1, -1 );
        tableau.setUpperBound( 1, 1 );
        tableau.setLowerBound( 2, -1 );
        tableau.setUpperBound( 2, 1 );


        // Invoke DeepPoly
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );
        TS_ASSERT_THROWS_NOTHING( nlr.deepPolyPropagation() );

        List<Tightening> expectedBounds( {
            Tightening( 3, -2, Tightening::LB ),   Tightening( 3, 2, Tightening::UB ),
            Tightening( 4, -3, Tightening::LB ),   Tightening( 4, 3, Tightening::UB ),
            Tightening( 5, -3, Tightening::LB ),   Tightening( 5, 3, Tightening::UB ),
            Tightening( 6, -6, Tightening::LB ),   Tightening( 6, 6, Tightening::UB ),

            Tightening( 7, 0, Tightening::LB ),    Tightening( 7, 2, Tightening::UB ),
            Tightening( 8, 0, Tightening::LB ),    Tightening( 8, 3, Tightening::UB ),
            Tightening( 9, 0, Tightening::LB ),    Tightening( 9, 3, Tightening::UB ),
            Tightening( 10, 0, Tightening::LB ),   Tightening( 10, 6, Tightening::UB ),

            Tightening( 11, -4, Tightening::LB ),  Tightening( 11, 8, Tightening::UB ),
            Tightening( 12, -2, Tightening::LB ),  Tightening( 12, 10, Tightening::UB ),

            Tightening( 13, -4, Tightening::LB ),  Tightening( 13, 8, Tightening::UB ),
            Tightening( 14, -2, Tightening::LB ),  Tightening( 14, 10, Tightening::UB ),

            Tightening( 15, -40, Tightening::LB ), Tightening( 15, 80, Tightening::UB ),
        } );

        List<Tightening> bounds;
        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );
        TS_ASSERT( boundsEqual( bounds, expectedBounds ) );


        // Invoke backward LP propagation
        TS_ASSERT_THROWS_NOTHING( updateTableau( tableau, bounds ) );
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );
        TS_ASSERT_THROWS_NOTHING( nlr.lpRelaxationPropagation() );

        List<Tightening> expectedBounds2( {
            Tightening( 13, 0, Tightening::LB ),
            Tightening( 14, 0, Tightening::LB ),
        } );

        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );
        TS_ASSERT( boundsEqual( bounds, expectedBounds2 ) );


        // Change the current bounds
        tableau.setLowerBound( 0, -3 );
        tableau.setUpperBound( 0, 1 );
        tableau.setLowerBound( 1, -1 );
        tableau.setUpperBound( 1, 2 );
        tableau.setLowerBound( 2, -2 );
        tableau.setUpperBound( 2, 2 );

        double large = 1000000;
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


        // Invoke DeepPoly
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );
        TS_ASSERT_THROWS_NOTHING( nlr.deepPolyPropagation() );

        List<Tightening> expectedBounds3( {
            Tightening( 3, -2, Tightening::LB ),         Tightening( 3, 5, Tightening::UB ),
            Tightening( 4, -5, Tightening::LB ),         Tightening( 4, 5, Tightening::UB ),
            Tightening( 5, -6, Tightening::LB ),         Tightening( 5, 5, Tightening::UB ),
            Tightening( 6, -15, Tightening::LB ),        Tightening( 6, 7, Tightening::UB ),

            Tightening( 7, -2, Tightening::LB ),         Tightening( 7, 5, Tightening::UB ),
            Tightening( 8, 0, Tightening::LB ),          Tightening( 8, 5, Tightening::UB ),
            Tightening( 9, 0, Tightening::LB ),          Tightening( 9, 5, Tightening::UB ),
            Tightening( 10, 0, Tightening::LB ),         Tightening( 10, 7, Tightening::UB ),

            Tightening( 11, -9, Tightening::LB ),        Tightening( 11, 15.1818, Tightening::UB ),
            Tightening( 12, -5, Tightening::LB ),        Tightening( 12, 14.0909, Tightening::UB ),

            Tightening( 13, -9, Tightening::LB ),        Tightening( 13, 15.1818, Tightening::UB ),
            Tightening( 14, -5, Tightening::LB ),        Tightening( 14, 14.0909, Tightening::UB ),

            Tightening( 15, -126.8182, Tightening::LB ), Tightening( 15, 213.9256, Tightening::UB ),
        } );

        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );
        TS_ASSERT( boundsEqual( bounds, expectedBounds3 ) );


        // Invoke backward LP propagation
        TS_ASSERT_THROWS_NOTHING( updateTableau( tableau, bounds ) );
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );
        TS_ASSERT_THROWS_NOTHING( nlr.lpRelaxationPropagation() );

        List<Tightening> expectedBounds4( {
            Tightening( 7, 0, Tightening::LB ),

            Tightening( 13, 0, Tightening::LB ),
            Tightening( 14, 0, Tightening::LB ),
        } );

        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );
        TS_ASSERT( boundsEqual( bounds, expectedBounds4 ) );
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
