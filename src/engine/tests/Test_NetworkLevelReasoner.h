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
#include "NetworkLevelReasoner.h"

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

        nlr.allocateWeightMatrices();

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
    }

    void test_evaluate()
    {
        NetworkLevelReasoner nlr;

        populateNetwork( nlr );

        double input[2];
        double output[2];

        // Inputs are zeros, only biases count
        input[0] = 0;
        input[1] = 0;

        TS_ASSERT_THROWS_NOTHING( nlr.evaluate( input, output ) );

        TS_ASSERT( FloatUtils::areEqual( output[0], 1 ) );
        TS_ASSERT( FloatUtils::areEqual( output[1], 4 ) );

        // No ReLUs, case 1
        input[0] = 1;
        input[1] = 1;

        TS_ASSERT_THROWS_NOTHING( nlr.evaluate( input, output ) );

        TS_ASSERT( FloatUtils::areEqual( output[0], 0 ) );
        TS_ASSERT( FloatUtils::areEqual( output[1], -6 ) );

        // No ReLUs, case 2
        input[0] = 1;
        input[1] = 2;

        TS_ASSERT_THROWS_NOTHING( nlr.evaluate( input, output ) );

        TS_ASSERT( FloatUtils::areEqual( output[0], -4 ) );
        TS_ASSERT( FloatUtils::areEqual( output[1], -22 ) );

        // Set all neurons to ReLU, except for input and output neurons
        nlr.setNeuronActivationFunction( 1, 0, NetworkLevelReasoner::ReLU );
        nlr.setNeuronActivationFunction( 1, 1, NetworkLevelReasoner::ReLU );
        nlr.setNeuronActivationFunction( 1, 2, NetworkLevelReasoner::ReLU );

        nlr.setNeuronActivationFunction( 2, 0, NetworkLevelReasoner::ReLU );
        nlr.setNeuronActivationFunction( 2, 1, NetworkLevelReasoner::ReLU );

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
};

//
// Local Variables:
// compile-command: "make -C ../../.. "
// tags-file-name: "../../../TAGS"
// c-basic-offset: 4
// End:
//
