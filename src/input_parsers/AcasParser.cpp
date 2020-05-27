/*********************                                                        */
/*! \file AcasParser.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz, Andrew Wu
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

**/

#include "AcasParser.h"
#include "DivideStrategy.h"
#include "FloatUtils.h"
#include "InputParserError.h"
#include "InputQuery.h"
#include "MString.h"
#include "NetworkLevelReasoner.h"
#include "ReluConstraint.h"

AcasParser::NodeIndex::NodeIndex( unsigned layer, unsigned node )
    : _layer( layer )
    , _node( node )
{
}

bool AcasParser::NodeIndex::operator<( const NodeIndex &other ) const
{
    if ( _layer != other._layer )
        return _layer < other._layer;

    return _node < other._node;
}

AcasParser::AcasParser( const String &path )
    : _acasNeuralNetwork( path )
{
}

void AcasParser::generateQuery( InputQuery &inputQuery )
{
    // First encode the actual network
    // _acasNeuralNetwork doesn't count the input layer, so add 1
    unsigned numberOfLayers = _acasNeuralNetwork.getNumLayers() + 1;
    unsigned inputLayerSize = _acasNeuralNetwork.getLayerSize( 0 );
    unsigned outputLayerSize = _acasNeuralNetwork.getLayerSize( numberOfLayers - 1 );

    unsigned numberOfInternalNodes = 0;
    for ( unsigned i = 1; i < numberOfLayers - 1; ++i )
        numberOfInternalNodes += _acasNeuralNetwork.getLayerSize( i );

    printf( "Number of layers: %u. Input layer size: %u. Output layer size: %u. Number of ReLUs: %u\n",
            numberOfLayers, inputLayerSize, outputLayerSize, numberOfInternalNodes );

    // The total number of variables required for the encoding is computed as follows:
    //   1. Each input node appears once
    //   2. Each internal node has a B variable and an F variable
    //   3. Each output node appears once
    unsigned numberOfVariables = inputLayerSize + ( 2 * numberOfInternalNodes ) + outputLayerSize;
    printf( "Total number of variables: %u\n", numberOfVariables );

    inputQuery.setNumberOfVariables( numberOfVariables );

    // Next, we want to map each node to its corresponding
    // variables. We group variables according to this order: f's from
    // layer i, b's from layer i+1, and repeat.
    unsigned currentIndex = 0;
    for ( unsigned i = 1; i < numberOfLayers; ++i )
    {
        unsigned previousLayerSize = _acasNeuralNetwork.getLayerSize( i - 1 );
        unsigned currentLayerSize = _acasNeuralNetwork.getLayerSize( i );

        // First add the F variables from layer i-1
        for ( unsigned j = 0; j < previousLayerSize; ++j )
        {
            _nodeToF[NodeIndex( i - 1, j )] = currentIndex;
            ++currentIndex;
        }

        // Now add the B variables from layer i
        for ( unsigned j = 0; j < currentLayerSize; ++j )
        {
            _nodeToB[NodeIndex( i, j )] = currentIndex;
            ++currentIndex;
        }
    }

    // Now we set the variable bounds. Input bounds are
    // given as part of the network. B variables are
    // unbounded, and F variables are non-negative.
    for ( unsigned i = 0; i < inputLayerSize; ++i )
    {
        double min, max;
        _acasNeuralNetwork.getInputRange( i, min, max );

        inputQuery.setLowerBound( _nodeToF[NodeIndex(0, i)], min );
        inputQuery.setUpperBound( _nodeToF[NodeIndex(0, i)], max );
    }

    for ( const auto &fNode : _nodeToF )
    {
        // Be careful not to override the bounds for the input layer
        if ( fNode.first._layer != 0 )
        {
            inputQuery.setLowerBound( fNode.second, 0.0 );
            inputQuery.setUpperBound( fNode.second, FloatUtils::infinity() );
        }
    }

    for ( const auto &fNode : _nodeToB )
    {
        inputQuery.setLowerBound( fNode.second, FloatUtils::negativeInfinity() );
        inputQuery.setUpperBound( fNode.second, FloatUtils::infinity() );
    }

    // Next come the actual equations
    for ( unsigned layer = 0; layer < numberOfLayers - 1; ++layer )
    {
        unsigned targetLayerSize = _acasNeuralNetwork.getLayerSize( layer + 1 );
        for ( unsigned target = 0; target < targetLayerSize; ++target )
        {
            // This will represent the equation:
            //   sum - b + fs = -bias
            Equation equation;

            // The b variable
            unsigned bVar = _nodeToB[NodeIndex(layer + 1, target)];
            equation.addAddend( -1.0, bVar );

            // The f variables from the previous layer
            for ( unsigned source = 0; source < _acasNeuralNetwork.getLayerSize( layer ); ++source )
            {
                unsigned fVar = _nodeToF[NodeIndex(layer, source)];
                equation.addAddend( _acasNeuralNetwork.getWeight( layer, source, target ), fVar );
            }

            // The bias
            equation.setScalar( -_acasNeuralNetwork.getBias( layer + 1, target ) );

            // Add the equation to the input query
            inputQuery.addEquation( equation );
        }
    }

    // Add the ReLU constraints
    for ( unsigned i = 1; i < numberOfLayers - 1; ++i )
    {
        unsigned currentLayerSize = _acasNeuralNetwork.getLayerSize( i );

        for ( unsigned j = 0; j < currentLayerSize; ++j )
        {
            unsigned b = _nodeToB[NodeIndex(i, j)];
            unsigned f = _nodeToF[NodeIndex(i, j)];
            PiecewiseLinearConstraint *relu = new ReluConstraint( b, f );
            if ( GlobalConfiguration::SPLITTING_HEURISTICS ==
                 DivideStrategy::EarliestReLU )
                relu->setScore( i );
            inputQuery.addPiecewiseLinearConstraint( relu );
        }
    }

    // Mark the input and output variables
    for ( unsigned i = 0; i < inputLayerSize; ++i )
        inputQuery.markInputVariable( _nodeToF[NodeIndex( 0, i )], i );

    for ( unsigned i = 0; i < outputLayerSize; ++i )
        inputQuery.markOutputVariable( _nodeToB[NodeIndex( numberOfLayers - 1, i )], i );

    // Populate the Network-Level Reasoner
    NetworkLevelReasoner *nlr = new NetworkLevelReasoner;

    nlr->setNumberOfLayers( numberOfLayers );

    for ( unsigned i = 0; i < numberOfLayers; ++i )
        nlr->setLayerSize( i, _acasNeuralNetwork.getLayerSize( i ) );

    nlr->allocateMemoryByTopology();

    // Biases
    for ( unsigned i = 1; i < numberOfLayers; ++i )
        for ( unsigned j = 0; j < _acasNeuralNetwork.getLayerSize( i ); ++j )
            nlr->setBias( i, j, _acasNeuralNetwork.getBias( i, j ) );

    // Weights
    for ( unsigned layer = 0; layer < numberOfLayers - 1; ++layer )
    {
        unsigned targetLayerSize = _acasNeuralNetwork.getLayerSize( layer + 1 );
        for ( unsigned target = 0; target < targetLayerSize; ++target )
        {
            for ( unsigned source = 0; source < _acasNeuralNetwork.getLayerSize( layer ); ++source )
                nlr->setWeight( layer, source, target, _acasNeuralNetwork.getWeight( layer, source, target ) );
        }
    }

    // Mark all hidden nodes as ReLU nodes, per the ACAS implicit convention
    for ( unsigned layer = 1; layer < numberOfLayers - 1; ++layer )
    {
        unsigned layerSize = _acasNeuralNetwork.getLayerSize( layer  );
        for ( unsigned neuron = 0; neuron < layerSize; ++neuron )
        {
            nlr->setNeuronActivationFunction( layer, neuron, PiecewiseLinearFunctionType::RELU );
        }
    }

    // Variable indexing
    for ( unsigned i = 1; i < numberOfLayers - 1; ++i )
    {
        unsigned layerSize = _acasNeuralNetwork.getLayerSize( i );

        for ( unsigned j = 0; j < layerSize; ++j )
        {
            unsigned b = _nodeToB[NodeIndex( i, j )];
            unsigned f = _nodeToF[NodeIndex( i, j )];
            nlr->setWeightedSumVariable( i, j, b );
            nlr->setActivationResultVariable( i, j, f );
        }
    }

    // Input variables are treated as "activation results"; output
    // variables are treated as "weighted sums"
    for ( unsigned i = 0; i < _acasNeuralNetwork.getLayerSize( 0 ); ++i )
    {
        unsigned var = _nodeToF[NodeIndex( 0, i )];
        nlr->setActivationResultVariable( 0, i, var );
    }

    for ( unsigned i = 0; i < _acasNeuralNetwork.getLayerSize( numberOfLayers - 1 ); ++i )
    {
        unsigned var = _nodeToB[NodeIndex( numberOfLayers - 1, i )];
        nlr->setWeightedSumVariable( numberOfLayers - 1, i, var );
    }

    // Store the reasoner in the input query
    inputQuery.setNetworkLevelReasoner( nlr );
}

unsigned AcasParser::getNumInputVaribales() const
{
    return _acasNeuralNetwork.getLayerSize( 0 );
}

unsigned AcasParser::getNumOutputVariables() const
{
    return _acasNeuralNetwork.getLayerSize( _acasNeuralNetwork.getNumLayers() );
}

unsigned AcasParser::getInputVariable( unsigned index ) const
{
    return getFVariable( 0, index );
}

unsigned AcasParser::getOutputVariable( unsigned index ) const
{
    return getBVariable( _acasNeuralNetwork.getNumLayers(), index );
}

unsigned AcasParser::getBVariable( unsigned layer, unsigned index ) const
{
    NodeIndex nodeIndex( layer, index );
    if ( !_nodeToB.exists( nodeIndex ) )
        throw InputParserError( InputParserError::VARIABLE_INDEX_OUT_OF_RANGE );

    return _nodeToB.get( nodeIndex );
}

unsigned AcasParser::getFVariable( unsigned layer, unsigned index ) const
{
    NodeIndex nodeIndex( layer, index );
    if ( !_nodeToF.exists( nodeIndex ) )
        throw InputParserError( InputParserError::VARIABLE_INDEX_OUT_OF_RANGE );

    return _nodeToF.get( nodeIndex );
}

void AcasParser::evaluate( const Vector<double> &inputs, Vector<double> &outputs ) const
{
    _acasNeuralNetwork.evaluate( inputs, outputs,
                                 _acasNeuralNetwork.getLayerSize( _acasNeuralNetwork.getNumLayers() ) );
}

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
