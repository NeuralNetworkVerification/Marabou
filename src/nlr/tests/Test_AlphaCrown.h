//
// Created by maya-swisa on 8/6/25.
//

#ifndef TEST_ALPHACROWN_H
#define TEST_ALPHACROWN_H

#include "../../engine/tests/MockTableau.h"
#include "AcasParser.h"
#include "CWAttack.h"
#include "Engine.h"
#include "InputQuery.h"
#include "Layer.h"
#include "NetworkLevelReasoner.h"
#include "PropertyParser.h"
#include "Tightening.h"

#include <cxxtest/TestSuite.h>
#include <memory>

class AlphaCrownAnalysisTestSuite : public CxxTest::TestSuite
{
public:
    void setUp()
    {
    }

    void tearDown()
    {
    }

    //     void testWithAttack()
    //     {
    // #ifdef BUILD_TORCH
    //
    //         auto networkFilePath = "../../../resources/nnet/acasxu/"
    //                                "ACASXU_experimental_v2a_1_1.nnet";
    //         auto propertyFilePath = "../../../resources/properties/"
    //                                 "acas_property_4.txt";
    //
    //         auto *_acasParser = new AcasParser( networkFilePath );
    //         InputQuery _inputQuery;
    //         _acasParser->generateQuery( _inputQuery );
    //         PropertyParser().parse( propertyFilePath, _inputQuery );
    //         std::unique_ptr<Engine> _engine = std::make_unique<Engine>();
    //         Options *options = Options::get();
    //         options->setString( Options::SYMBOLIC_BOUND_TIGHTENING_TYPE, "alphacrown" );
    //         // obtain the alpha crown proceeder
    //         _engine->processInputQuery( _inputQuery );
    //         NLR::NetworkLevelReasoner *_networkLevelReasoner =
    //         _engine->getNetworkLevelReasoner(); TS_ASSERT_THROWS_NOTHING(
    //         _networkLevelReasoner->obtainCurrentBounds() ); std::unique_ptr<CWAttack> cwAttack =
    //         std::make_unique<CWAttack>( _networkLevelReasoner ); auto
    //         attackResultAfterBoundTightening = cwAttack->runAttack(); TS_ASSERT(
    //         !attackResultAfterBoundTightening ); delete _acasParser;

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

    void test_alphacrown_relus()
    {
        NLR::NetworkLevelReasoner nlr;
        MockTableau tableau;
        nlr.setTableau( &tableau );
        populateNetwork( nlr, tableau );

        tableau.setLowerBound( 0, -1 );
        tableau.setUpperBound( 0, 1 );
        tableau.setLowerBound( 1, -1 );
        tableau.setUpperBound( 1, 1 );

        // Invoke alpha crow
        TS_ASSERT_THROWS_NOTHING( nlr.obtainCurrentBounds() );
        TS_ASSERT_THROWS_NOTHING( nlr.alphaCrown() );

        List<Tightening> bounds;
        TS_ASSERT_THROWS_NOTHING( nlr.getConstraintTightenings( bounds ) );

        for ( const auto &bound : bounds )
        {
            if ( bound._type == Tightening::LB )
                printf( "lower:\n" );
            else
                printf( "upper:\n" );
            std::cout << "var : " << bound._variable << " bound : " << bound._value << std::endl;
        }

        double large = 1000000;
        nlr.setBounds( nlr.getNumberOfLayers() -1 , 1, 0.1 , large );
        std::unique_ptr<CWAttack> cwAttack = std::make_unique<CWAttack>( &nlr );
        auto attackResultAfterBoundTightening = cwAttack->runAttack();
        TS_ASSERT( !attackResultAfterBoundTightening );

        nlr.setBounds( nlr.getNumberOfLayers() -1 , 1, -large , -0.1 );
        cwAttack = std::make_unique<CWAttack>( &nlr );
        attackResultAfterBoundTightening = cwAttack->runAttack();
        TS_ASSERT( !attackResultAfterBoundTightening );

        nlr.setBounds( nlr.getNumberOfLayers() -1 , 0 , -large , 0.99 );
        cwAttack = std::make_unique<CWAttack>( &nlr );
        attackResultAfterBoundTightening = cwAttack->runAttack();
        TS_ASSERT( !attackResultAfterBoundTightening );

        nlr.setBounds( nlr.getNumberOfLayers() -1 , 0 , 1.1 , large );
        cwAttack = std::make_unique<CWAttack>( &nlr );
        attackResultAfterBoundTightening = cwAttack->runAttack();
        TS_ASSERT( !attackResultAfterBoundTightening );


    }
};

#endif // TEST_ALPHACROWN_H
