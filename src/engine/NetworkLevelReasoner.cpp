/*********************                                                        */
/*! \file NetworkLevelReasoner.cpp
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

#include <cstring>
#include "Debug.h"
#include "NetworkLevelReasoner.h"
#include "ReluplexError.h"

NetworkLevelReasoner::NetworkLevelReasoner()
    : _weights( NULL )
    , _maxLayerSize( 0 )
    , _work1( NULL )
    , _work2( NULL )
{
}

NetworkLevelReasoner::~NetworkLevelReasoner()
{
    freeMemoryIfNeeded();
}

void NetworkLevelReasoner::freeMemoryIfNeeded()
{
    if ( _weights )
    {
        for ( unsigned i = 0; i < _numberOfLayers - 1; ++i )
        {
            if ( _weights[i] )
            {
                delete[] _weights[i];
                _weights[i] = NULL;
            }
        }

        delete[] _weights;
        _weights = NULL;
    }

    if ( _work1 )
    {
        delete[] _work1;
        _work1 = NULL;
    }

    if ( _work2 )
    {
        delete[] _work2;
        _work2 = NULL;
    }
}

void NetworkLevelReasoner::setNumberOfLayers( unsigned numberOfLayers )
{
    _numberOfLayers = numberOfLayers;
}

void NetworkLevelReasoner::setLayerSize( unsigned layer, unsigned size )
{
    ASSERT( layer < _numberOfLayers );
    _layerSizes[layer] = size;

    if ( size > _maxLayerSize )
        _maxLayerSize = size;
}

void NetworkLevelReasoner::allocateWeightMatrices()
{
    freeMemoryIfNeeded();

    _weights = new double*[_numberOfLayers - 1];
    if ( !_weights )
        throw ReluplexError( ReluplexError::ALLOCATION_FAILED, "NetworkLevelReasoner::weights" );

    for ( unsigned i = 0; i < _numberOfLayers - 1; ++i )
    {
        _weights[i] = new double[_layerSizes[i] * _layerSizes[i+1]];
        if ( !_weights[i] )
            throw ReluplexError( ReluplexError::ALLOCATION_FAILED, "NetworkLevelReasoner::weights[i]" );
        std::fill_n( _weights[i], _layerSizes[i] * _layerSizes[i+1], 0 );
    }

    _work1 = new double[_maxLayerSize];
    if ( !_work1 )
        throw ReluplexError( ReluplexError::ALLOCATION_FAILED, "NetworkLevelReasoner::work1" );

    _work2 = new double[_maxLayerSize];
    if ( !_work2 )
        throw ReluplexError( ReluplexError::ALLOCATION_FAILED, "NetworkLevelReasoner::work2" );
}

void NetworkLevelReasoner::setNeuronActivationFunction( unsigned layer, unsigned neuron, ActivationFunction activationFuction )
{
    _neuronToActivationFunction[Index( layer, neuron )] = activationFuction;
}

void NetworkLevelReasoner::setWeight( unsigned sourceLayer, unsigned sourceNeuron, unsigned targetNeuron, double weight )
{
    ASSERT( _weights );

    unsigned targetLayerSize = _layerSizes[sourceLayer + 1];
    _weights[sourceLayer][sourceNeuron * targetLayerSize + targetNeuron] = weight;
}

void NetworkLevelReasoner::setBias( unsigned layer, unsigned neuron, double bias )
{
    _bias[Index( layer, neuron )] = bias;
}

void NetworkLevelReasoner::evaluate( double *input, double *output ) const
{
    memcpy( _work1, input, sizeof(double) * _layerSizes[0] );

    for ( unsigned targetLayer = 1; targetLayer < _numberOfLayers; ++targetLayer )
    {
        unsigned sourceLayer = targetLayer - 1;
        unsigned sourceLayerSize = _layerSizes[sourceLayer];
        unsigned targetLayerSize = _layerSizes[targetLayer];

        for ( unsigned targetNeuron = 0; targetNeuron < targetLayerSize; ++targetNeuron )
        {
            Index index( targetLayer, targetNeuron );
            _work2[targetNeuron] = _bias.exists( index ) ? _bias[index] : 0;

            for ( unsigned sourceNeuron = 0; sourceNeuron < sourceLayerSize; ++sourceNeuron )
            {
                double weight = _weights[sourceLayer][sourceNeuron * targetLayerSize + targetNeuron];
                _work2[targetNeuron] += _work1[sourceNeuron] * weight;
            }

            // Apply activation function
            if ( _neuronToActivationFunction.exists( index ) )
            {
                switch ( _neuronToActivationFunction[index] )
                {
                case ReLU:
                    if ( _work2[targetNeuron] < 0 )
                        _work2[targetNeuron] = 0;
                    break;

                default:
                    ASSERT( false );
                    break;
                }
            }
        }

        memcpy( _work1, _work2, sizeof(double) * targetLayerSize );
    }

    memcpy( output, _work1, sizeof(double) * _layerSizes[_numberOfLayers - 1] );
}

void NetworkLevelReasoner::storeIntoOther( NetworkLevelReasoner &other ) const
{
    other.freeMemoryIfNeeded();

    other.setNumberOfLayers( _numberOfLayers );
    for ( const auto &pair : _layerSizes )
        other.setLayerSize( pair.first, pair.second );
    other.allocateWeightMatrices();

    for ( const auto &pair : _neuronToActivationFunction )
        other.setNeuronActivationFunction( pair.first._layer, pair.first._neuron, pair.second );

    for ( unsigned i = 0; i < _numberOfLayers - 1; ++i )
        memcpy( other._weights[i], _weights[i], sizeof(double) * _layerSizes[i] * _layerSizes[i+1] );

    for ( const auto &pair : _bias )
        other.setBias( pair.first._layer, pair.first._neuron, pair.second );
}

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
