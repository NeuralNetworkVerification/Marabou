/*********************                                                        */
/*! \file TfParser.cpp
** \verbatim
** Top contributors (to current version):
**   Kyle Julian
** This file is part of the Marabou project.
** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
** in the top-level source directory) and their institutional affiliations.
** All rights reserved. See the file COPYING in the top-level source
** directory for licensing information.\endverbatim
**/

#include "TfParser.h"
#include "FloatUtils.h"
#include "InputParserError.h"
#include "InputQuery.h"
#include "MString.h"
#include "ReluConstraint.h"

static bool USE_SLACK_VARIABLES = false;

TfParser::NodeIndex::NodeIndex( unsigned layer, unsigned node )
    : _layer( layer )
    , _node( node )
{
}

bool TfParser::NodeIndex::operator<( const NodeIndex &other ) const
{
    if ( _layer != other._layer )
        return _layer < other._layer;

    return _node < other._node;
}

TfParser::TfParser( const String &path )
    : _tfNeuralNetwork( path )
{
}

int TfParser::num_inputs()
{
    return _tfNeuralNetwork.getLayerSize(0);
}

int TfParser::num_outputs()
{
    int numLayers = _tfNeuralNetwork.getNumLayers() + 1;
    return _tfNeuralNetwork.getLayerSize(numLayers - 1);
}

void TfParser::generateQuery( InputQuery &inputQuery )
{
    // First encode the actual network
    // _acasNeuralNetwork doesn't count the input layer, so add 1
    unsigned numberOfLayers = _tfNeuralNetwork.getNumLayers() + 1;
    unsigned inputLayerSize = _tfNeuralNetwork.getLayerSize( 0 );
    unsigned outputLayerSize = _tfNeuralNetwork.getLayerSize( numberOfLayers - 1 );

    unsigned numberOfInternalNodes = 0;
    for ( unsigned i = 1; i < numberOfLayers - 1; ++i )
        numberOfInternalNodes += _tfNeuralNetwork.getLayerSize( i );

    printf( "Number of layers: %u. Input layer size: %u. Output layer size: %u. Number of ReLUs: %u\n",
            numberOfLayers, inputLayerSize, outputLayerSize, numberOfInternalNodes );

    // The total number of variables required for the encoding is computed as follows:
    //   1. Each input node appears once
    //   2. Each internal node has a B variable, an F variable, and an auxiliary varibale
    //   3. Each output node appears once and also has an auxiliary variable
    unsigned numberOfVariables = inputLayerSize + ( 3 * numberOfInternalNodes ) + ( 2 * outputLayerSize );

    if ( USE_SLACK_VARIABLES )
    {
        // An extra slack variable for each relu node
        numberOfVariables += numberOfInternalNodes;
        printf( "number of internal nodes: %u\n", numberOfInternalNodes );

    }

    printf( "Total number of variables: %u\n", numberOfVariables );

    inputQuery.setNumberOfVariables( numberOfVariables );

    // Next, we want to map each node to its corresponding
    // variables. We group variables according to this order: f's from
    // layer i, b's from layer i+1, auxiliary variables from layer
    // i+1, and repeat. The
    unsigned currentIndex = 0;
    for ( unsigned i = 1; i < numberOfLayers; ++i )
    {
        unsigned previousLayerSize = _tfNeuralNetwork.getLayerSize( i - 1 );
        unsigned currentLayerSize = _tfNeuralNetwork.getLayerSize( i );

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

        // And now the aux variables from layer i
        for ( unsigned j = 0; j < currentLayerSize; ++j )
        {
            _nodeToAux[NodeIndex( i, j )] = currentIndex;
            ++currentIndex;
        }

        // And the slack variables for any relu nodes
        if ( USE_SLACK_VARIABLES && ( i < numberOfLayers - 1 ) )
        {
            for ( unsigned j = 0; j < currentLayerSize; ++j )
            {
                printf( "Adding slack equation for node (%u, %u)\n", i, j );

                _nodeToSlack[NodeIndex( i, j )] = currentIndex;
                ++currentIndex;
            }
        }
    }

    for ( const auto &auxNode : _nodeToAux )
    {
        inputQuery.setLowerBound( auxNode.second, 0.0 );
        inputQuery.setUpperBound( auxNode.second, 0.0 );
    }

    for ( const auto &slackNode : _nodeToSlack )
    {
        // A slack node represents f - b + aux = 0, so aux is non-positive
        inputQuery.setLowerBound( slackNode.second, FloatUtils::negativeInfinity() );
        inputQuery.setUpperBound( slackNode.second, 0.0 );
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
        unsigned targetLayerSize = _tfNeuralNetwork.getLayerSize( layer + 1 );
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
            for ( unsigned source = 0; source < _tfNeuralNetwork.getLayerSize( layer ); ++source )
            {
                unsigned fVar = _nodeToF[NodeIndex(layer, source)];
                equation.addAddend( _tfNeuralNetwork.getWeight( layer, source, target ), fVar );
            }

            // The bias
            equation.setScalar( -_tfNeuralNetwork.getBias( layer + 1, target ) );

            // Add the equation to the input query
            inputQuery.addEquation( equation );
        }
    }

    // And the slack variable equations
    for ( const auto &slackNode : _nodeToSlack )
    {
        // A slack node represents f - b + aux == 0

        Equation equation;

        unsigned fVar = _nodeToF[slackNode.first];
        unsigned bVar = _nodeToB[slackNode.first];
        unsigned auxVar = slackNode.second;

        printf( "b: %u, f: %u, slack: %u\n", bVar, fVar, auxVar );

        equation.addAddend( 1.0, auxVar );
        equation.markAuxiliaryVariable( auxVar );

        equation.addAddend( 1, fVar );
        equation.addAddend( -1, bVar );
        equation.setScalar( 0 );

        equation.dump();

        inputQuery.addEquation( equation );
    }

    // Add the ReLU constraints
    for ( unsigned i = 1; i < numberOfLayers - 1; ++i )
    {
        unsigned currentLayerSize = _tfNeuralNetwork.getLayerSize( i );

        for ( unsigned j = 0; j < currentLayerSize; ++j )
        {
            unsigned b = _nodeToB[NodeIndex(i, j)];
            unsigned f = _nodeToF[NodeIndex(i, j)];
            PiecewiseLinearConstraint *relu = new ReluConstraint( b, f );
            inputQuery.addPiecewiseLinearConstraint( relu );
        }
    }
}

unsigned TfParser::getInputVariable( unsigned index ) const
{
    return getFVariable( 0, index );
}

unsigned TfParser::getOutputVariable( unsigned index ) const
{
    return getBVariable( _tfNeuralNetwork.getNumLayers(), index );
}

unsigned TfParser::getBVariable( unsigned layer, unsigned index ) const
{
    NodeIndex nodeIndex( layer, index );
    if ( !_nodeToB.exists( nodeIndex ) )
        throw InputParserError( InputParserError::VARIABLE_INDEX_OUT_OF_RANGE );

    return _nodeToB.get( nodeIndex );
}

unsigned TfParser::getFVariable( unsigned layer, unsigned index ) const
{
    NodeIndex nodeIndex( layer, index );
    if ( !_nodeToF.exists( nodeIndex ) )
        throw InputParserError( InputParserError::VARIABLE_INDEX_OUT_OF_RANGE );

    return _nodeToF.get( nodeIndex );
}

unsigned TfParser::getAuxVariable( unsigned layer, unsigned index ) const
{
    NodeIndex nodeIndex( layer, index );
    if ( !_nodeToAux.exists( nodeIndex ) )
        throw InputParserError( InputParserError::VARIABLE_INDEX_OUT_OF_RANGE );

    return _nodeToAux.get( nodeIndex );
}

unsigned TfParser::getSlackVariable( unsigned layer, unsigned index ) const
{
    NodeIndex nodeIndex( layer, index );
    if ( !_nodeToSlack.exists( nodeIndex ) )
        throw InputParserError( InputParserError::VARIABLE_INDEX_OUT_OF_RANGE, "Slacks" );

    return _nodeToSlack.get( nodeIndex );
}

void TfParser::evaluate( const Vector<double> &inputs, Vector<double> &outputs ) const
{
    _tfNeuralNetwork.evaluate( inputs, outputs,
                                 _tfNeuralNetwork.getLayerSize( _tfNeuralNetwork.getNumLayers() ) );
}

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
