/*********************                                                        */
/*! \file TensorflowNeuralNetwork.cpp
** \verbatim
** Top contributors (to current version):
**   Kyle Julian
** This file is part of the Marabou project.
** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
** in the top-level source directory) and their institutional affiliations.
** All rights reserved. See the file COPYING in the top-level source
** directory for licensing information.\endverbatim
**/

#include "TensorflowNeuralNetwork.h"
#include "TfNnet.h"
#include "FloatUtils.h"

#include <iostream>

TensorflowNeuralNetwork::TensorflowNeuralNetwork( const String &path )
    : _network( NULL )
{
    _network = load_network( path.ascii() );
}

TensorflowNeuralNetwork::~TensorflowNeuralNetwork()
{
    if ( _network )
    {
        destroy_network( _network );
        _network = NULL;
    }
}

double TensorflowNeuralNetwork::getWeight( int sourceLayer, int sourceNeuron, int targetNeuron )
{
    return _network->weights[sourceLayer](sourceNeuron,targetNeuron);//,sourceNeuron);
}

String TensorflowNeuralNetwork::getWeightAsString( int sourceLayer, int sourceNeuron, int targetNeuron )
{
    double weight = getWeight( sourceLayer, sourceNeuron, targetNeuron );
    return FloatUtils::doubleToString( weight );
}

String TensorflowNeuralNetwork::getBiasAsString( int layer, int neuron )
{
    double bias = getBias( layer, neuron );
    return FloatUtils::doubleToString( bias );
}

double TensorflowNeuralNetwork::getBias( int layer, int neuron )
{
    // The bias for layer i is in index i-1 in the array.
    assert( layer > 0 );
    return _network->biases[layer - 1](neuron);
}

int TensorflowNeuralNetwork::getNumLayers() const
{
    return _network->num_layers;
}

unsigned TensorflowNeuralNetwork::getLayerSize( unsigned layer ) const
{
    return (unsigned)_network->layer_sizes[layer];
}


void TensorflowNeuralNetwork::evaluate( const Vector<double> &inputs, Vector<double> &outputs, unsigned outputSize ) const
{
    double input[inputs.size()];
    double output[outputSize];

    memset( input, 0, num_inputs( _network ) );
    memset( output, 0, num_outputs( _network ) );

    for ( unsigned i = 0; i < inputs.size();  ++i )
        input[i] = inputs.get( i );

    bool normalizeInput = false;
    bool normalizeOutput = false;

    if ( evaluate_network( _network, input, output, normalizeInput, normalizeOutput ) != 1 )
    {
        std::cout << "Error! Network evaluation failed" << std::endl;
        exit( 1 );
    }

    for ( unsigned i = 0; i < outputSize; ++i )
    {
        outputs.append( output[i] );
    }
}


//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//


