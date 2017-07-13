/*********************                                                        */
/*! \file AcasParser.cpp
** \verbatim
** Top contributors (to current version):
**   Guy Katz
** This file is part of the Marabou project.
** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
** in the top-level source directory) and their institutional affiliations.
** All rights reserved. See the file COPYING in the top-level source
** directory for licensing information.\endverbatim
**/

#include "AcasParser.h"
#include "InputParserError.h"
#include "InputQuery.h"
#include "MString.h"
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
    //   2. Each internal node has a B variable, an F variable, and an auxiliary varibale
    //   3. Each output node appears once and also has an auxiliary variable
    unsigned numberOfVariables = inputLayerSize + ( 3 * numberOfInternalNodes ) + ( 2 * outputLayerSize );
    inputQuery.setNumberOfVariables( numberOfVariables );

    // Next, we want to map each node to its corresponding
    // variables. We group variables according to this order: f's from
    // layer i, b's from layer i+1, auxiliary variables from layer
    // i+1, and repeat. The
    unsigned currentIndex = 0;
    for ( unsigned i = 1; i < numberOfLayers; ++i )
    {
        unsigned previousLayerSize = _acasNeuralNetwork.getLayerSize( i - 1 );
        unsigned currentLayerSize = _acasNeuralNetwork.getLayerSize( i );

        // First add the F variables from layer i-1
        for ( unsigned j = 0; j < previousLayerSize; ++j )
        {
            printf( "adding nodeToF (%u,%u)\n", i-1, j );

            _nodeToF[NodeIndex( i - 1, j )] = currentIndex;
            ++currentIndex;
        }

        // Now add the B variables from layer i
        for ( unsigned j = 0; j < currentLayerSize; ++j )
        {
            _nodeToB[NodeIndex( i, j )] = currentIndex;
            ++currentIndex;
        }

        // And now the aux variables from layer i
        for ( unsigned j = 0; j < currentLayerSize; ++j )
        {
            _nodeToAux[NodeIndex( i, j )] = currentIndex;
            ++currentIndex;
        }
    }

    // Now we set the variable bounds. Input bounds are
    // given as part of the network. Auxiliary variables are set to
    // 0. B variables are unbounded, and F variables are non-negative.
    for ( unsigned i = 0; i < inputLayerSize; ++i )
    {
        double min, max;
        _acasNeuralNetwork.getInputRange( i, min, max );

        inputQuery.setLowerBound( _nodeToF[NodeIndex(0, i)], min );
        inputQuery.setUpperBound( _nodeToF[NodeIndex(0, i)], max );
    }

    for ( const auto &auxNode : _nodeToAux )
    {
        inputQuery.setLowerBound( auxNode.second, 0.0 );
        inputQuery.setUpperBound( auxNode.second, 0.0 );
    }

    for ( const auto &fNode : _nodeToF )
    {
        inputQuery.setLowerBound( fNode.second, 0.0 );
    }

    // Next come the actual equations
    for ( unsigned layer = 0; layer < numberOfLayers - 1; ++layer )
    {
        unsigned targetLayerSize = _acasNeuralNetwork.getLayerSize( layer + 1 );;
        for ( unsigned target = 0; target < targetLayerSize; ++target )
        {
            // This will represent the equation:
            //   sum fs - b + aux = -bias
            Equation equation;

            // The auxiliary variable
            unsigned auxVar = _nodeToAux[NodeIndex(layer + 1, target)];
            equation.addAddend( 1.0, auxVar );
            equation.markAuxiliaryVariable( auxVar );

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
            equation.setScalar( _acasNeuralNetwork.getBias( layer + 1, target ) );

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
            inputQuery.addPiecewiseLinearConstraint( relu );
        }
    }
}

unsigned AcasParser::getInputVariable( unsigned index ) const
{
    NodeIndex nodeIndex( 0, index );
    if ( !_nodeToF.exists( nodeIndex ) )
        throw InputParserError( InputParserError::VARIABLE_INDEX_OUT_OF_RANGE );

    return _nodeToF.get( nodeIndex );
}

unsigned AcasParser::getOutputVariable( unsigned index ) const
{
    NodeIndex nodeIndex( _acasNeuralNetwork.getNumLayers(), index );
    if ( !_nodeToB.exists( nodeIndex ) )
        throw InputParserError( InputParserError::VARIABLE_INDEX_OUT_OF_RANGE );

    return _nodeToB.get( nodeIndex );
}

//
// Local Variables:
// compile-command: "make -C .. "
// tags-file-name: "../TAGS"
// c-basic-offset: 4
// End:
//
