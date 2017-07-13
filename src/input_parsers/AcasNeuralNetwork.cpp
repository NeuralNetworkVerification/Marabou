/*********************                                                        */
/*! \file AcasNeuralNetwork.cpp
** \verbatim
** Top contributors (to current version):
**   Guy Katz
** This file is part of the Marabou project.
** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
** in the top-level source directory) and their institutional affiliations.
** All rights reserved. See the file COPYING in the top-level source
** directory for licensing information.\endverbatim
**/

#include "AcasNeuralNetwork.h"
#include "AcasNnet.h"
#include "FloatUtils.h"

#include <iostream>

AcasNeuralNetwork::AcasNeuralNetwork( const String &path )
    : _network( NULL )
{
    _network = load_network( path.ascii() );
}

AcasNeuralNetwork::~AcasNeuralNetwork()
{
    if ( _network )
    {
        destroy_network( _network );
        _network = NULL;
    }
}

double AcasNeuralNetwork::getWeight( int sourceLayer, int sourceNeuron, int targetNeuron )
{
    return _network->matrix[sourceLayer][0][targetNeuron][sourceNeuron];
}

String AcasNeuralNetwork::getWeightAsString( int sourceLayer, int sourceNeuron, int targetNeuron )
{
    double weight = getWeight( sourceLayer, sourceNeuron, targetNeuron );
    return FloatUtils::doubleToString( weight );
}

String AcasNeuralNetwork::getBiasAsString( int layer, int neuron )
{
    double bias = getBias( layer, neuron );
    return FloatUtils::doubleToString( bias );
}

double AcasNeuralNetwork::getBias( int layer, int neuron )
{
    // The bias for layer i is in index i-1 in the array.
    assert( layer > 0 );
    return _network->matrix[layer - 1][1][neuron][0];
}

int AcasNeuralNetwork::getNumLayers() const
{
    return _network->numLayers;
}

unsigned AcasNeuralNetwork::getLayerSize( unsigned layer )
{
    return (unsigned)_network->layerSizes[layer];
}

void AcasNeuralNetwork::evaluate( const Vector<double> &inputs, Vector<double> &outputs, unsigned outputSize ) const
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

void AcasNeuralNetwork::getInputRange( unsigned index, double &min, double &max )
{
    max =
        ( _network->maxes[index] - _network->means[index] )
        / ( _network->ranges[index] );

    min =
        ( _network->mins[index] - _network->means[index] )
        / ( _network->ranges[index] );
}

//
// Local Variables:
// compile-command: "make -C .. "
// tags-file-name: "../TAGS"
// c-basic-offset: 4
// End:
//
