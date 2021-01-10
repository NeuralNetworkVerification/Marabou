/*********************                                                        */
/*! \file Test_DeepPolyAnalysis.h
** \verbatim
** Top contributors (to current version):
**   Haoze Andrew Wu
** This file is part of the Marabou project.
** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
** in the top-level source directory) and their institutional affiliations.
** All rights reserved. See the file COPYING in the top-level source
** directory for licensing information.\endverbatim
**
** [[ Add lengthier description here ]]

**/

#include <cxxtest/TestSuite.h>

#include "../../engine/tests/MockTableau.h"
#include "FloatUtils.h"
#include "InputQuery.h"
#include "Layer.h"
#include "NetworkLevelReasoner.h"
#include "Tightening.h"

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

        List<Tightening> expectedBounds({
                Tightening( 2, -2, Tightening::LB ),
                Tightening( 2, 2, Tightening::UB ),
                Tightening( 3, -2, Tightening::LB ),
                Tightening( 3, 2, Tightening::UB ),

                Tightening( 4, 0, Tightening::LB ),
                Tightening( 4, 2, Tightening::UB ),
                Tightening( 5, 0, Tightening::LB ),
                Tightening( 5, 2, Tightening::UB ),

                Tightening( 6, 0, Tightening::LB ),
                Tightening( 6, 3, Tightening::UB ),
                Tightening( 7, -2, Tightening::LB ),
                Tightening( 7, 2, Tightening::UB ),

                Tightening( 8, 0, Tightening::LB ),
                Tightening( 8, 3, Tightening::UB ),
                Tightening( 9, 0, Tightening::LB ),
                Tightening( 9, 2, Tightening::UB ),

                Tightening( 10, 1, Tightening::LB ),
                Tightening( 10, 5.5, Tightening::UB ),
                Tightening( 11, 0, Tightening::LB ),
                Tightening( 11, 2, Tightening::UB )

                    });

        List<Tightening> bounds;
        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );

        TS_ASSERT_EQUALS( expectedBounds.size(), bounds.size() );
        for ( const auto &bound : expectedBounds )
            TS_ASSERT( bounds.exists( bound ) );
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

        tableau.setLowerBound( 1, -large ); tableau.setUpperBound( 1, large );
        tableau.setLowerBound( 2, -large ); tableau.setUpperBound( 2, large );
        tableau.setLowerBound( 3, -large ); tableau.setUpperBound( 3, large );
        tableau.setLowerBound( 4, -large ); tableau.setUpperBound( 4, large );
        tableau.setLowerBound( 5, -large ); tableau.setUpperBound( 5, large );
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

        List<Tightening> expectedBounds({
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

                    });

        List<Tightening> bounds;
        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );

        TS_ASSERT_EQUALS( expectedBounds.size(), bounds.size() );
        for ( const auto &bound : expectedBounds )
            TS_ASSERT( bounds.exists( bound ) );
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

        tableau.setLowerBound( 1, -large ); tableau.setUpperBound( 1, large );
        tableau.setLowerBound( 2, -large ); tableau.setUpperBound( 2, large );
        tableau.setLowerBound( 3, -large ); tableau.setUpperBound( 3, large );
        tableau.setLowerBound( 4, -large ); tableau.setUpperBound( 4, large );
        tableau.setLowerBound( 5, -large ); tableau.setUpperBound( 5, large );
        tableau.setLowerBound( 6, -large ); tableau.setUpperBound( 6, large );
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

        List<Tightening> expectedBounds({
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
                    });

        List<Tightening> bounds;
        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );

        for ( const auto &bound : bounds )
            std::cout << "x" << bound._variable << (bound._type ? " <= " : " >= " ) << bound._value << std::endl;

        TS_ASSERT_EQUALS( expectedBounds.size(), bounds.size() );
        for ( const auto &bound : expectedBounds )
            TS_ASSERT( bounds.exists( bound ) );
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

        tableau.setLowerBound( 2, -large ); tableau.setUpperBound( 2, large );
        tableau.setLowerBound( 3, -large ); tableau.setUpperBound( 3, large );
        tableau.setLowerBound( 4, -large ); tableau.setUpperBound( 4, large );
        tableau.setLowerBound( 5, -large ); tableau.setUpperBound( 5, large );
        tableau.setLowerBound( 6, -large ); tableau.setUpperBound( 6, large );
        tableau.setLowerBound( 7, -large ); tableau.setUpperBound( 7, large );
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

        List<Tightening> expectedBounds({
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

                    });

        List<Tightening> bounds;
        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );

        for ( const auto &bound : bounds )
            std::cout << "x" << bound._variable << (bound._type ? " <= " : " >= " ) << bound._value << std::endl;

        TS_ASSERT_EQUALS( expectedBounds.size(), bounds.size() );
        for ( const auto &bound : expectedBounds )
            TS_ASSERT( bounds.exists( bound ) );
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

        List<Tightening> expectedBounds({
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

                    });

        List<Tightening> bounds;
        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );

        for ( const auto &bound : bounds )
            std::cout << "x" << bound._variable << (bound._type ? " <= " : " >= " ) << bound._value << std::endl;

        TS_ASSERT_EQUALS( expectedBounds.size(), bounds.size() );
        for ( const auto &bound : expectedBounds )
            TS_ASSERT( bounds.exists( bound ) );
    }
};
