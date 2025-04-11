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

#include "../nlr/NetworkLevelReasoner.h"
#include "MockErrno.h"
#include "MockTableau.h"
#include "Query.h"
#include "ReluConstraint.h"
#include "context/context.h"

#include <cxxtest/TestSuite.h>
#include <iostream>
#include <string.h>

using namespace CVC4::context;

class MockForNetworkLevelReasoner : public MockErrno
{
public:
};

class Test_BaBsrSplitting : public CxxTest::TestSuite
{
public:
    MockForNetworkLevelReasoner *mock;
    MockTableau *tableau;

    void setUp()
    {
        TS_ASSERT( mock = new MockForNetworkLevelReasoner );
        TS_ASSERT( tableau = new MockTableau() );
    }

    void tearDown()
    {
        delete tableau;
        delete mock;
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

    void test_relu_0()
    {
        NLR::NetworkLevelReasoner nlr;
        populateNetwork( nlr );

        // Generate Constraints
        Query query;
        nlr.generateQuery( query );
        List<PiecewiseLinearConstraint *> constraints = query.getPiecewiseLinearConstraints();

        for ( const auto &constraint : constraints )
        {
            ReluConstraint *relu = dynamic_cast<ReluConstraint *>( constraint );
            nlr.addConstraintInTopologicalOrder( relu );
        }

        // Get Constraints in topological order
        List<PiecewiseLinearConstraint *> orderedConstraints =
            nlr.getConstraintsInTopologicalOrder();
        MockTableau tableau;

        // Test ReLU 0
        ReluConstraint *relu = dynamic_cast<ReluConstraint *>( orderedConstraints.front() );

        // Register Bounds
        relu->registerTableau( &tableau );
        relu->initializeNLRForBaBSR( &nlr );

        unsigned b = relu->getB();
        unsigned f = relu->getF();

        relu->notifyLowerBound( b, -1 );
        relu->notifyUpperBound( b, 1 );
        relu->notifyLowerBound( f, 0 );
        relu->notifyUpperBound( f, 1 );

        tableau.setValue( b, 0.5 );
        tableau.setValue( f, 0.5 );

        // Compute BaBSR and compare
        double score = relu->computeBaBsr();

        TS_ASSERT_EQUALS( score, 0 );
    }

    void test_relu_1()
    {
        NLR::NetworkLevelReasoner nlr;
        populateNetwork( nlr );

        // Generate Constraints
        Query query;
        nlr.generateQuery( query );
        List<PiecewiseLinearConstraint *> constraints = query.getPiecewiseLinearConstraints();

        for ( const auto &constraint : constraints )
        {
            ReluConstraint *relu = dynamic_cast<ReluConstraint *>( constraint );
            nlr.addConstraintInTopologicalOrder( relu );
        }

        // Get Constraints in topological order
        List<PiecewiseLinearConstraint *> orderedConstraints =
            nlr.getConstraintsInTopologicalOrder();
        MockTableau tableau;

        // Test ReLU 1
        auto it = orderedConstraints.begin();
        std::advance( it, 1 );
        ReluConstraint *relu = dynamic_cast<ReluConstraint *>( *it );

        // Register Bounds
        relu->registerTableau( &tableau );
        relu->initializeNLRForBaBSR( &nlr );

        unsigned b = relu->getB();
        unsigned f = relu->getF();

        relu->notifyLowerBound( b, -1 );
        relu->notifyUpperBound( b, 1 );
        relu->notifyLowerBound( f, 0 );
        relu->notifyUpperBound( f, 1 );

        tableau.setValue( b, 0.5 );
        tableau.setValue( f, 0.5 );

        // Compute BaBSR and compare
        double score = relu->computeBaBsr();

        TS_ASSERT_EQUALS( score, 0.25 );
    }

    void test_relu_2()
    {
        NLR::NetworkLevelReasoner nlr;
        populateNetwork( nlr );

        // Generate Constraints
        Query query;
        nlr.generateQuery( query );
        List<PiecewiseLinearConstraint *> constraints = query.getPiecewiseLinearConstraints();

        for ( const auto &constraint : constraints )
        {
            ReluConstraint *relu = dynamic_cast<ReluConstraint *>( constraint );
            nlr.addConstraintInTopologicalOrder( relu );
        }

        // Get Constraints in topological order
        List<PiecewiseLinearConstraint *> orderedConstraints =
            nlr.getConstraintsInTopologicalOrder();
        MockTableau tableau;

        // Test ReLU 2
        auto it = orderedConstraints.begin();
        std::advance( it, 2 );
        ReluConstraint *relu = dynamic_cast<ReluConstraint *>( *it );

        // Register Bounds
        relu->registerTableau( &tableau );
        relu->initializeNLRForBaBSR( &nlr );

        unsigned b = relu->getB();
        unsigned f = relu->getF();

        relu->notifyLowerBound( b, -1 );
        relu->notifyUpperBound( b, 1 );
        relu->notifyLowerBound( f, 0 );
        relu->notifyUpperBound( f, 1 );

        tableau.setValue( b, 0.5 );
        tableau.setValue( f, 0.5 );

        // Compute BaBSR and compare
        double score = relu->computeBaBsr();

        TS_ASSERT_EQUALS( score, 0.25 );
    }

    void test_relu_3()
    {
        NLR::NetworkLevelReasoner nlr;
        populateNetwork( nlr );

        // Generate Constraints
        Query query;
        nlr.generateQuery( query );
        List<PiecewiseLinearConstraint *> constraints = query.getPiecewiseLinearConstraints();

        for ( const auto &constraint : constraints )
        {
            ReluConstraint *relu = dynamic_cast<ReluConstraint *>( constraint );
            nlr.addConstraintInTopologicalOrder( relu );
        }

        // Get Constraints in topological order
        List<PiecewiseLinearConstraint *> orderedConstraints =
            nlr.getConstraintsInTopologicalOrder();
        MockTableau tableau;

        // Test ReLU 3
        auto it = orderedConstraints.begin();
        std::advance( it, 3 );
        ReluConstraint *relu = dynamic_cast<ReluConstraint *>( *it );

        // Register Bounds
        relu->registerTableau( &tableau );
        relu->initializeNLRForBaBSR( &nlr );

        unsigned b = relu->getB();
        unsigned f = relu->getF();
        relu->notifyLowerBound( b, -1 );
        relu->notifyUpperBound( b, 1 );
        relu->notifyLowerBound( f, 0 );
        relu->notifyUpperBound( f, 1 );

        tableau.setValue( b, 0.5 );
        tableau.setValue( f, 0.5 );

        // Compute BaBSR and compare
        double score = relu->computeBaBsr();

        TS_ASSERT_EQUALS( score, 0.25 );
    }

    void test_relu_4()
    {
        NLR::NetworkLevelReasoner nlr;
        populateNetwork( nlr );

        // Generate Constraints
        Query query;
        nlr.generateQuery( query );
        List<PiecewiseLinearConstraint *> constraints = query.getPiecewiseLinearConstraints();

        for ( const auto &constraint : constraints )
        {
            ReluConstraint *relu = dynamic_cast<ReluConstraint *>( constraint );
            nlr.addConstraintInTopologicalOrder( relu );
        }

        // Get Constraints in topological order
        List<PiecewiseLinearConstraint *> orderedConstraints =
            nlr.getConstraintsInTopologicalOrder();
        MockTableau tableau;

        // Test ReLU 4
        auto it = orderedConstraints.begin();
        std::advance( it, 4 );
        ReluConstraint *relu = dynamic_cast<ReluConstraint *>( *it );

        // Register Bounds
        relu->registerTableau( &tableau );
        relu->initializeNLRForBaBSR( &nlr );

        unsigned b = relu->getB();
        unsigned f = relu->getF();
        relu->notifyLowerBound( b, -1 );
        relu->notifyUpperBound( b, 1 );
        relu->notifyLowerBound( f, 0 );
        relu->notifyUpperBound( f, 1 );

        tableau.setValue( b, 0.5 );
        tableau.setValue( f, 0.5 );

        // Compute BaBSR and compare
        double score = relu->computeBaBsr();

        TS_ASSERT_EQUALS( score, -0.25 );
    }
};