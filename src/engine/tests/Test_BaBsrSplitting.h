/*********************                                                        */
/*! \file test_Test_BaBsrSplitting.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Liam Davis
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2024 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]
 **/

#include "FloatUtils.h"
#include "LinearExpression.h"
#include "MarabouError.h"
#include "MockErrno.h"
#include "MockTableau.h"
#include "PiecewiseLinearCaseSplit.h"
#include "Query.h"
#include "ReluConstraint.h"
#include "context/context.h"

#include <cxxtest/TestSuite.h>
#include <iostream>
#include <string.h>

using namespace CVC4::context;

class MockNetworkLevelReasoner : public MockErrno
{
public:
};

class Test_BaBsrSplitting : public CxxTest::TestSuite
{
public:
    NLR::NetworkLevelReasoner *nlr;

    // Moved the population method outside test functions for reuse
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

    void setUp()
    {
        TS_ASSERT( nlr = new NLR::NetworkLevelReasoner );
        populateNetwork( *nlr ); // Populate network globally in setUp
    }

    void tearDown()
    {
        TS_ASSERT_THROWS_NOTHING( delete nlr );
    }

    // Test case for ReLU without network
    void test_babsr_without_relu_in_network()
    {
        // Initialize the error context
        MockErrno mockErrno;

        // Test error throwing when ReLU is not in network
        unsigned b = 2;
        unsigned f = 3;

        ReluConstraint relu( b, f );
        MockTableau tableau;
        relu.registerTableau( &tableau );

        // Set up network level reasoner with relu
        relu.setNetworkLevelReasoner( nlr );

        // Set bounds for b
        relu.notifyLowerBound( b, 1.0 );
        relu.notifyUpperBound( b, 3.0 );

        // Mock the values in the tableau
        tableau.setValue( b, 2.0 );
        tableau.setValue( f, 2.0 );

        // Assert that an NLRError is thrown when calling computeBaBsr
        TS_ASSERT_THROWS_ANYTHING( relu.computeBaBsr() );
    }

    // Test case for evaluating neurons
    void test_relu_neurons()
    {
        struct TestParams
        {
            NLR::NeuronIndex neuronIndex;
            double lowerBound;
            double upperBound;
        };

        std::vector<TestParams> testParams = {
            // ... (test parameters)
        };

        // Initialize MockErrno (error context)
        MockErrno mockErrno;

        // Iterate through each test case
        for ( const auto &params : testParams )
        {
            double input[2] = { 0, 0 };
            double output[2];

            TS_ASSERT_THROWS_NOTHING( nlr->evaluate( input, output ) );

            const NLR::Layer *layer = nlr->getLayer( params.neuronIndex._layer );
            double b = layer->getAssignment( params.neuronIndex._neuron );
            double f = FloatUtils::max( b, 0.0 );

            // Create the ReluConstraint with neuron variables
            ReluConstraint relu( layer->neuronToVariable( params.neuronIndex._neuron ), f );

            // retrieve bias using the new method
            double bias = nlr->getPrevBiasForReluConstraint( &relu );

            double ub = params.upperBound;
            double lb = params.lowerBound;

            double scaler = ( ub ) / ( ub - lb );
            double term1 = std::min( scaler * b * bias, ( scaler - 1.0 ) * b * bias );
            double term2 = ( scaler * lb ) * f;

            double expectedBaBsr = term1 - term2;

            // Setup mock tableau
            MockTableau tableau;
            relu.registerTableau( &tableau );

            relu.setNetworkLevelReasoner( nlr );

            relu.notifyLowerBound( b, lb );
            relu.notifyUpperBound( f, ub );

            tableau.setValue( b, b );
            tableau.setValue( f, f );

            // Assert BaBsr calculation
            double actualBaBsr = relu.computeBaBsr();

            TS_ASSERT_DELTA( actualBaBsr, expectedBaBsr, 1e-6 );
        }
    }
};