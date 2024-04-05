/*********************                                                        */
/*! \file Test_DeepPolyAnalysis.h
** \verbatim
** Top contributors (to current version):
**   Haoze Andrew Wu
** This file is part of the Marabou project.
** Copyright (c) 2017-2024 by the authors listed in the file AUTHORS
** in the top-level source directory) and their institutional affiliations.
** All rights reserved. See the file COPYING in the top-level source
** directory for licensing information.\endverbatim
**
** [[ Add lengthier description here ]]

**/

#include "../../engine/tests/MockTableau.h"
#include "DeepPolySoftmaxElement.h"
#include "FloatUtils.h"
#include "InputQuery.h"
#include "Layer.h"
#include "NetworkLevelReasoner.h"
#include "Options.h"
#include "Tightening.h"

#include <cxxtest/TestSuite.h>

class DeepPolyAnalysisTestSuite : public CxxTest::TestSuite
{
public:
    void setUp()
    {
    }

    void tearDown()
    {
    }

    void populateNetwork( NLR::NetworkLevelReasoner &nlr, MockTableau &tableau )
    {
        /*

              1      R       1      R       1  1
          x0 --- x2 ---> x4 --- x6 ---> x8 --- x10
            \    /        \    /          \    /
           1 \  /        1 \  /          0 \  /
              \/            \/              \/
              /\            /\              /\
           1 /  \        1 /  \          1 /  \
            /    \   R    /    \    R     / 1  \
          x1 --- x3 ---> x5 --- x7 ---> x9 --- x11
              -1            -1

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
        nlr.setWeight( 2, 0, 3, 1, 1 );
        nlr.setWeight( 2, 1, 3, 0, 1 );
        nlr.setWeight( 2, 1, 3, 1, -1 );

        nlr.setWeight( 4, 0, 5, 0, 1 );
        nlr.setWeight( 4, 0, 5, 1, 0 );
        nlr.setWeight( 4, 1, 5, 0, 1 );
        nlr.setWeight( 4, 1, 5, 1, 1 );

        nlr.setBias( 5, 0, 1 );

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

    void test_deeppoly_relus()
    {
        NLR::NetworkLevelReasoner nlr;
        MockTableau tableau;
        nlr.setTableau( &tableau );
        populateNetwork( nlr, tableau );

        tableau.setLowerBound( 0, -1 );
        tableau.setUpperBound( 0, 1 );
        tableau.setLowerBound( 1, -1 );
        tableau.setUpperBound( 1, 1 );

        // Invoke Deeppoly
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );
        TS_ASSERT_THROWS_NOTHING( nlr.deepPolyPropagation() );

        /*
          Input ranges:

          x0: [-1, 1]
          x1: [-1, 1]

          Layer 1:

          x2: [-2, 2]
          x3: [-2, 2]

          Layer 2:

          x4: [0, 2]
          x5: [0, 2]

          Layer 3:

          x6: [0, 3]
          x7: [-2, 2]

          Layer 4:

          x8: [0, 3]
          x9: [0, 2]

          Layer 5:

          x10: [1, 5.5]
          x11: [0, 2]

        */

        List<Tightening> expectedBounds(
            { Tightening( 2, -2, Tightening::LB ), Tightening( 2, 2, Tightening::UB ),
              Tightening( 3, -2, Tightening::LB ), Tightening( 3, 2, Tightening::UB ),

              Tightening( 4, 0, Tightening::LB ),  Tightening( 4, 2, Tightening::UB ),
              Tightening( 5, 0, Tightening::LB ),  Tightening( 5, 2, Tightening::UB ),

              Tightening( 6, 0, Tightening::LB ),  Tightening( 6, 3, Tightening::UB ),
              Tightening( 7, -2, Tightening::LB ), Tightening( 7, 2, Tightening::UB ),

              Tightening( 8, 0, Tightening::LB ),  Tightening( 8, 3, Tightening::UB ),
              Tightening( 9, 0, Tightening::LB ),  Tightening( 9, 2, Tightening::UB ),

              Tightening( 10, 1, Tightening::LB ), Tightening( 10, 5.5, Tightening::UB ),
              Tightening( 11, 0, Tightening::LB ), Tightening( 11, 2, Tightening::UB )

            } );

        List<Tightening> bounds;
        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );

        TS_ASSERT_EQUALS( expectedBounds.size(), bounds.size() );
        for ( const auto &bound : expectedBounds )
            TS_ASSERT( existsBound( bounds, bound ) );
    }

    void populateResidualNetwork1( NLR::NetworkLevelReasoner &nlr, MockTableau &tableau )
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

    void test_deeppoly_residual1()
    {
        NLR::NetworkLevelReasoner nlr;
        MockTableau tableau;
        nlr.setTableau( &tableau );
        populateResidualNetwork1( nlr, tableau );

        tableau.setLowerBound( 0, -1 );
        tableau.setUpperBound( 0, 1 );

        // Invoke DeepPoly
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );
        TS_ASSERT_THROWS_NOTHING( nlr.deepPolyPropagation() );

        /*
          Input ranges:

          x0: [-1, 1]

          x1: [-1, 1]
          x2: [-2, 2]
          x3: [-2, 2]
          x4: [0, 2]
          x5: [0, 2]
        */

        List<Tightening> expectedBounds( {
            Tightening( 1, -1, Tightening::LB ),
            Tightening( 1, 1, Tightening::UB ),
            Tightening( 2, 0, Tightening::LB ),
            Tightening( 2, 1, Tightening::UB ),
            Tightening( 3, -1, Tightening::LB ),
            Tightening( 3, 2, Tightening::UB ),
            Tightening( 4, -1, Tightening::LB ),
            Tightening( 4, 2, Tightening::UB ),
            Tightening( 5, 1, Tightening::LB ),
            Tightening( 5, 6, Tightening::UB ),

        } );

        List<Tightening> bounds;
        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );

        TS_ASSERT_EQUALS( expectedBounds.size(), bounds.size() );
        for ( const auto &bound : expectedBounds )
            TS_ASSERT( existsBound( bounds, bound ) );
    }

    void populateResidualNetwork2( NLR::NetworkLevelReasoner &nlr, MockTableau &tableau )
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

    void test_deeppoly_residual2()
    {
        NLR::NetworkLevelReasoner nlr;
        MockTableau tableau;
        nlr.setTableau( &tableau );
        populateResidualNetwork2( nlr, tableau );

        tableau.setLowerBound( 0, -1 );
        tableau.setUpperBound( 0, 1 );

        // Invoke DeepPoly
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );
        TS_ASSERT_THROWS_NOTHING( nlr.deepPolyPropagation() );

        /*
          Input ranges:

          x0: [-1, 1]
        */

        List<Tightening> expectedBounds( {
            Tightening( 1, -1, Tightening::LB ),
            Tightening( 1, 1, Tightening::UB ),
            Tightening( 2, 0, Tightening::LB ),
            Tightening( 2, 1, Tightening::UB ),
            Tightening( 3, -1, Tightening::LB ),
            Tightening( 3, 2, Tightening::UB ),
            Tightening( 4, -1, Tightening::LB ),
            Tightening( 4, 2, Tightening::UB ),
            Tightening( 5, -1, Tightening::LB ),
            Tightening( 5, 6, Tightening::UB ),
            Tightening( 6, -1, Tightening::LB ),
            Tightening( 6, 6, Tightening::UB ),
        } );

        List<Tightening> bounds;
        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );

        TS_ASSERT_EQUALS( expectedBounds.size(), bounds.size() );
        for ( const auto &bound : expectedBounds )
            TS_ASSERT( existsBound( bounds, bound ) );
    }

    void populateMaxNetwork( NLR::NetworkLevelReasoner &nlr, MockTableau &tableau )
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

    void test_deeppoly_max_not_fixed()
    {
        NLR::NetworkLevelReasoner nlr;
        MockTableau tableau;
        nlr.setTableau( &tableau );
        populateMaxNetwork( nlr, tableau );

        tableau.setLowerBound( 0, -1 );
        tableau.setUpperBound( 0, 1 );
        tableau.setLowerBound( 1, -1 );
        tableau.setUpperBound( 1, 2 );

        // Invoke DeepPoly
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );
        TS_ASSERT_THROWS_NOTHING( nlr.deepPolyPropagation() );

        /*
          Input ranges:

          x0: [-1, 1]
          x1: [-1, 2]
        */

        List<Tightening> expectedBounds( {
            Tightening( 2, -2, Tightening::LB ),
            Tightening( 2, 3, Tightening::UB ),
            Tightening( 3, -3, Tightening::LB ),
            Tightening( 3, 2, Tightening::UB ),
            Tightening( 4, -2, Tightening::LB ),
            Tightening( 4, 3, Tightening::UB ),
            Tightening( 5, 0, Tightening::LB ),
            Tightening( 5, 2, Tightening::UB ),
            Tightening( 6, 0, Tightening::LB ),
            Tightening( 6, 3, Tightening::UB ),
            Tightening( 7, 0, Tightening::LB ),
            Tightening( 7, 6, Tightening::UB ),

        } );

        List<Tightening> bounds;
        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );

        TS_ASSERT_EQUALS( expectedBounds.size(), bounds.size() );
        for ( const auto &bound : expectedBounds )
            TS_ASSERT( existsBound( bounds, bound ) );
    }

    void test_deeppoly_max_fixed()
    {
        NLR::NetworkLevelReasoner nlr;
        MockTableau tableau;
        nlr.setTableau( &tableau );
        populateMaxNetwork( nlr, tableau );

        tableau.setLowerBound( 0, 1 );
        tableau.setUpperBound( 0, 2 );
        tableau.setLowerBound( 1, -3 );
        tableau.setUpperBound( 1, -2 );

        // Invoke DeepPoly
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );
        TS_ASSERT_THROWS_NOTHING( nlr.deepPolyPropagation() );

        /*
          Input ranges:

          x0: [1, 2]
          x1: [-3, -2]
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

        TS_ASSERT_EQUALS( expectedBounds.size(), bounds.size() );
        for ( const auto &bound : expectedBounds )
            TS_ASSERT( existsBound( bounds, bound ) );
    }

    void populateNetworkReindex( NLR::NetworkLevelReasoner &nlr, MockTableau &tableau )
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

    void test_deeppoly_reindex_relu()
    {
        NLR::NetworkLevelReasoner nlr;
        MockTableau tableau;
        nlr.setTableau( &tableau );
        populateNetworkReindex( nlr, tableau );

        tableau.setLowerBound( 0, -1 );
        tableau.setUpperBound( 0, 1 );
        tableau.setLowerBound( 1, -1 );
        tableau.setUpperBound( 1, 1 );

        // Invoke Deeppoly
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );
        TS_ASSERT_THROWS_NOTHING( nlr.deepPolyPropagation() );

        /*
          Input ranges:

          x0: [-1, 1]
          x1: [-1, 1]

          Layer 1:

          x2: [-2, 2]
          x3: [-2, 2]

          Layer 2:

          x4: [0, 2]
          x5: [0, 2]

          Layer 3:

          x6: [0, 3]
          x7: [-2, 2]

          Layer 4:

          x8: [0, 3]
          x9: [0, 2]

          Layer 5:

          x10: [1, 5.5]
          x11: [0, 2]

        */

        List<Tightening> expectedBounds(
            { Tightening( 2, -2, Tightening::LB ), Tightening( 2, 2, Tightening::UB ),
              Tightening( 3, -2, Tightening::LB ), Tightening( 3, 2, Tightening::UB ),

              Tightening( 4, 0, Tightening::LB ),  Tightening( 4, 2, Tightening::UB ),
              Tightening( 5, 0, Tightening::LB ),  Tightening( 5, 2, Tightening::UB ),

              Tightening( 6, 0, Tightening::LB ),  Tightening( 6, 3, Tightening::UB ),
              Tightening( 7, -2, Tightening::LB ), Tightening( 7, 2, Tightening::UB ),

              Tightening( 8, 0, Tightening::LB ),  Tightening( 8, 3, Tightening::UB ),
              Tightening( 9, 0, Tightening::LB ),  Tightening( 9, 2, Tightening::UB ),

              Tightening( 10, 1, Tightening::LB ), Tightening( 10, 5.5, Tightening::UB ),
              Tightening( 11, 0, Tightening::LB ), Tightening( 11, 2, Tightening::UB )

            } );

        List<Tightening> bounds;
        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );

        TS_ASSERT_EQUALS( expectedBounds.size(), bounds.size() );
        for ( const auto &bound : expectedBounds )
            TS_ASSERT( existsBound( bounds, bound ) );
    }

    void populateNetworkWithSigmoidsAndRound( NLR::NetworkLevelReasoner &nlr, MockTableau &tableau )
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

    void test_deeppoly_sigmoids_and_round()
    {
        NLR::NetworkLevelReasoner nlr;
        MockTableau tableau;
        nlr.setTableau( &tableau );
        populateNetworkWithSigmoidsAndRound( nlr, tableau );

        tableau.setLowerBound( 0, -1 );
        tableau.setUpperBound( 0, 1 );
        tableau.setLowerBound( 1, -1 );
        tableau.setUpperBound( 1, 1 );

        // Invoke Deeppoly
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );
        TS_ASSERT_THROWS_NOTHING( nlr.deepPolyPropagation() );

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

    void populateNetworkSoftmax( NLR::NetworkLevelReasoner &nlr, MockTableau &tableau )
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


    void test_deeppoly_softmax1()
    {
        NLR::NetworkLevelReasoner nlr;
        MockTableau tableau;
        nlr.setTableau( &tableau );
        populateNetworkSoftmax( nlr, tableau );

        tableau.setLowerBound( 0, -1 );
        tableau.setUpperBound( 0, 1 );
        tableau.setLowerBound( 1, -1 );
        tableau.setUpperBound( 1, 1 );
        tableau.setLowerBound( 2, -1 );
        tableau.setUpperBound( 2, 1 );

        // Invoke DeepPoly
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );
        TS_ASSERT_THROWS_NOTHING( nlr.deepPolyPropagation() );
    }

    void test_deeppoly_softmax2()
    {
        {
            Options::get()->setString( Options::SOFTMAX_BOUND_TYPE, "lse" );
            NLR::NetworkLevelReasoner nlr;
            MockTableau tableau;
            nlr.setTableau( &tableau );
            populateNetworkSoftmax( nlr, tableau );

            tableau.setLowerBound( 0, 1 );
            tableau.setUpperBound( 0, 1.000001 );
            tableau.setLowerBound( 1, 1 );
            tableau.setUpperBound( 1, 1.000001 );
            tableau.setLowerBound( 2, 1 );
            tableau.setUpperBound( 2, 1.000001 );

            // Invoke DeepPoly
            TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );
            TS_ASSERT_THROWS_NOTHING( nlr.deepPolyPropagation() );

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

            for ( const auto &b : bounds )
                b.dump();

            TS_ASSERT_EQUALS( expectedBounds.size(), bounds.size() );
            for ( const auto &bound : expectedBounds )
                TS_ASSERT( existsBound( bounds, bound ) );
        }
        {
            Options::get()->setString( Options::SOFTMAX_BOUND_TYPE, "er" );
            NLR::NetworkLevelReasoner nlr;
            MockTableau tableau;
            nlr.setTableau( &tableau );
            populateNetworkSoftmax( nlr, tableau );

            tableau.setLowerBound( 0, 1 );
            tableau.setUpperBound( 0, 1.000001 );
            tableau.setLowerBound( 1, 1 );
            tableau.setUpperBound( 1, 1.000001 );
            tableau.setLowerBound( 2, 1 );
            tableau.setUpperBound( 2, 1.000001 );

            // Invoke DeepPoly
            TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );
            TS_ASSERT_THROWS_NOTHING( nlr.deepPolyPropagation() );

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

            for ( const auto &b : bounds )
                b.dump();

            TS_ASSERT_EQUALS( expectedBounds.size(), bounds.size() );
            for ( const auto &bound : expectedBounds )
                TS_ASSERT( existsBound( bounds, bound ) );
        }
    }

    bool existsBound( const List<Tightening> &bounds, const Tightening &t )
    {
        for ( const auto &bound : bounds )
        {
            if ( bound._type == t._type && bound._variable == t._variable &&
                 FloatUtils::areEqual( bound._value, t._value, 0.0001 ) )
                return true;
        }
        return false;
    }


    void populateNetworkSoftmax2( NLR::NetworkLevelReasoner &nlr, MockTableau &tableau )
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

    void test_deeppoly_softmax3()
    {
        NLR::NetworkLevelReasoner nlr;
        MockTableau tableau;
        nlr.setTableau( &tableau );
        populateNetworkSoftmax2( nlr, tableau );

        tableau.setLowerBound( 0, 1 );
        tableau.setUpperBound( 0, 1.00001 );
        tableau.setLowerBound( 1, 1 );
        tableau.setUpperBound( 1, 1.00001 );
        tableau.setLowerBound( 2, 1 );
        tableau.setUpperBound( 2, 1.00001 );

        // Invoke DeepPoly
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );
        TS_ASSERT_THROWS_NOTHING( nlr.deepPolyPropagation() );

        /*
          Input ranges:

          x0: [1, 1.0001]
          x1: [1, 1.0001]
          x2: [1, 1.0001]
        */

        List<Tightening> expectedBounds( { Tightening( 13, 1, Tightening::LB ),
                                           Tightening( 13, 1, Tightening::UB ),
                                           Tightening( 14, -1, Tightening::LB ),
                                           Tightening( 14, -1, Tightening::UB ),
                                           Tightening( 15, 1, Tightening::LB ),
                                           Tightening( 15, 1, Tightening::UB ),
                                           Tightening( 16, -1, Tightening::LB ),
                                           Tightening( 16, -1, Tightening::UB )

        } );

        List<Tightening> bounds;
        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );

        for ( const auto &b : bounds )
            b.dump();

        for ( const auto &bound : expectedBounds )
            TS_ASSERT( existsBound( bounds, bound ) );
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

    void populateNetworkBilinear( NLR::NetworkLevelReasoner &nlr, MockTableau &tableau )
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

    void test_bilinear()
    {
        NLR::NetworkLevelReasoner nlr;
        MockTableau tableau;
        nlr.setTableau( &tableau );
        populateNetworkBilinear( nlr, tableau );

        tableau.setLowerBound( 0, 1 );
        tableau.setUpperBound( 0, 1.000001 );
        tableau.setLowerBound( 1, 1 );
        tableau.setUpperBound( 1, 1.000001 );

        // Invoke DeepPoly
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );
        TS_ASSERT_THROWS_NOTHING( nlr.deepPolyPropagation() );

        /*
          Input ranges:

          x0: [1, 1.0001]
          x1: [1, 1.0001]
        */
        List<Tightening> expectedBounds( { Tightening( 2, -1, Tightening::LB ),
                                           Tightening( 2, -1, Tightening::UB ),
                                           Tightening( 3, 2, Tightening::LB ),
                                           Tightening( 3, 2, Tightening::UB ),
                                           Tightening( 4, -2, Tightening::LB ),
                                           Tightening( 4, -2, Tightening::UB ),
                                           Tightening( 5, 2, Tightening::LB ),
                                           Tightening( 5, 2, Tightening::UB ) } );

        List<Tightening> bounds;
        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );

        for ( const auto &b : bounds )
            b.dump();

        TS_ASSERT_EQUALS( expectedBounds.size(), bounds.size() );
        for ( const auto &bound : expectedBounds )
            TS_ASSERT( existsBound( bounds, bound ) );
    }

    void populateLeakyReLUNetwork( NLR::NetworkLevelReasoner &nlr, MockTableau &tableau )
    {
        /*

              1      R       1      R       1  1
          x0 --- x2 ---> x4 --- x6 ---> x8 --- x10
            \    /        \    /          \    /
           1 \  /        1 \  /          0 \  /
              \/            \/              \/
              /\            /\              /\
           1 /  \        1 /  \          1 /  \
            /    \   R    /    \    R     / 1  \
          x1 --- x3 ---> x5 --- x7 ---> x9 --- x11
              -1            -1

          The example described in Fig. 3 of
          https://files.sri.inf.ethz.ch/website/papers/DeepPoly.pdf
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

    void test_deeppoly_leaky_relus()
    {
        NLR::NetworkLevelReasoner nlr;
        MockTableau tableau;
        nlr.setTableau( &tableau );
        populateLeakyReLUNetwork( nlr, tableau );

        tableau.setLowerBound( 0, -1 );
        tableau.setUpperBound( 0, 1 );
        tableau.setLowerBound( 1, -1 );
        tableau.setUpperBound( 1, 1 );

        // Invoke Deeppoly
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );
        TS_ASSERT_THROWS_NOTHING( nlr.deepPolyPropagation() );

        /*
          Input ranges:

          x0: [-1, 1]
          x1: [-1, 1]

          Layer 1:

          x2: [-2, 2]
          x3: [-2, 2]

          Layer 2:

          x4: [-2, 2]
          x5: [-2, 2]

          Layer 3:

          x6: [-2, 2.8]
          x7: [-2.8, 2.8]

          Layer 4:

          x8: [0, 3]
          x9: [0, 2]

          Layer 5:

          x10: [1, 5.5]
          x11: [0, 2]

        */

        List<Tightening> expectedBounds(
            { Tightening( 2, -2, Tightening::LB ),    Tightening( 2, 2, Tightening::UB ),
              Tightening( 3, -2, Tightening::LB ),    Tightening( 3, 2, Tightening::UB ),

              Tightening( 4, -2, Tightening::LB ),    Tightening( 4, 2, Tightening::UB ),
              Tightening( 5, -2, Tightening::LB ),    Tightening( 5, 2, Tightening::UB ),

              Tightening( 6, -2, Tightening::LB ),    Tightening( 6, 2.8, Tightening::UB ),
              Tightening( 7, -2.8, Tightening::LB ),  Tightening( 7, 2.8, Tightening::UB ),

              Tightening( 8, -2, Tightening::LB ),    Tightening( 8, 2.8, Tightening::UB ),
              Tightening( 9, -2.8, Tightening::LB ),  Tightening( 9, 2.8, Tightening::UB ),

              Tightening( 10, -3, Tightening::LB ),   Tightening( 10, 5.64, Tightening::UB ),
              Tightening( 11, -2.8, Tightening::LB ), Tightening( 11, 2.8, Tightening::UB )

            } );

        List<Tightening> bounds;
        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );

        TS_ASSERT_EQUALS( expectedBounds.size(), bounds.size() );
        for ( const auto &bound : expectedBounds )
            TS_ASSERT( existsBound( bounds, bound ) );
    }

    void test_deeppoly_leaky_relus_fixed_input1()
    {
        NLR::NetworkLevelReasoner nlr;
        MockTableau tableau;
        nlr.setTableau( &tableau );
        populateLeakyReLUNetwork( nlr, tableau );

        tableau.setLowerBound( 0, -1 );
        tableau.setUpperBound( 0, -1 );
        tableau.setLowerBound( 1, 1 );
        tableau.setUpperBound( 1, 1 );

        // Invoke Deeppoly
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );
        TS_ASSERT_THROWS_NOTHING( nlr.deepPolyPropagation() );

        /*
          Input ranges:

          x0: [-1, -1]
          x1: [1, 1]

          Layer 1:

          x2: [0, 0]
          x3: [-2, -2]

          Layer 2:

          x4: [0, 0]
          x5: [-0.4, -0.4]

          Layer 3:

          x6: [-0.4, -0.4]
          x7: [0.4, 0.4]

          Layer 4:

          x8: [-0.08, -0.08]
          x9: [0.4, 0.4]

          Layer 5:

          x10: [1.32, 1.32]
          x11: [0.4, 0.4]

        */

        List<Tightening> expectedBounds(
            { Tightening( 2, 0, Tightening::LB ),     Tightening( 2, 0, Tightening::UB ),
              Tightening( 3, -2, Tightening::LB ),    Tightening( 3, -2, Tightening::UB ),

              Tightening( 4, 0, Tightening::LB ),     Tightening( 4, 0, Tightening::UB ),
              Tightening( 5, -0.4, Tightening::LB ),  Tightening( 5, -0.4, Tightening::UB ),

              Tightening( 6, -0.4, Tightening::LB ),  Tightening( 6, -0.4, Tightening::UB ),
              Tightening( 7, 0.4, Tightening::LB ),   Tightening( 7, 0.4, Tightening::UB ),

              Tightening( 8, -0.08, Tightening::LB ), Tightening( 8, -0.08, Tightening::UB ),
              Tightening( 9, 0.4, Tightening::LB ),   Tightening( 9, 0.4, Tightening::UB ),

              Tightening( 10, 1.32, Tightening::LB ), Tightening( 10, 1.32, Tightening::UB ),
              Tightening( 11, 0.4, Tightening::LB ),  Tightening( 11, 0.4, Tightening::UB )

            } );

        List<Tightening> bounds;
        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );

        TS_ASSERT_EQUALS( expectedBounds.size(), bounds.size() );
        for ( const auto &bound : expectedBounds )
        {
            TS_ASSERT( existsBounds( bounds, bound ) );
        }
    }

    bool existsBounds( const List<Tightening> &bounds, Tightening bound )
    {
        for ( const auto &b : bounds )
        {
            if ( b._type == bound._type && b._variable == bound._variable )
            {
                if ( FloatUtils::areEqual( b._value, bound._value ) )
                    return true;
            }
        }
        return false;
    }
};
