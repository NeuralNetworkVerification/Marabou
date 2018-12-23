/*********************                                                        */
/*! \file SymbolicBoundTightener.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Duligur Ibeling
 ** This file is part of the Marabou project.
 ** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **/

#include "Debug.h"
#include "SymbolicBoundTightener.h"

SymbolicBoundTightener::SymbolicBoundTightener()
    : _layerSizes( NULL )
    , _biases( NULL )
{
}

SymbolicBoundTightener::~SymbolicBoundTightener()
{
    freeMemoryIfNeeded();
}

void SymbolicBoundTightener::freeMemoryIfNeeded()
{
    if ( _biases )
    {
        for ( unsigned i = 0; i < _numberOfLayers; ++i )
        {
            if ( _biases[i] )
            {
                delete[] _biases[i];
                _biases[i] = NULL;
            }
        }

        delete[] _biases;
        _biases = NULL;
    }

    if ( _weights )
    {
        for ( unsigned i = 0; i < _numberOfLayers - 1; ++i )
        {
            if ( _weights[i]._values )
            {
                delete[] _weights[i]._values;
                _weights[i]._values = NULL;
            }
        }

        delete[] _weights;
        _weights = NULL;
    }

    if ( _currentLayerBias )
    {
        delete[] _currentLayerBias;
        _currentLayerBias = NULL;
    }

    if ( _currentLayerLowerBounds )
    {
        delete[] _currentLayerLowerBounds;
        _currentLayerLowerBounds = NULL;
    }

    if ( _currentLayerUpperBounds )
    {
        delete[] _currentLayerUpperBounds;
        _currentLayerUpperBounds = NULL;
    }

    if ( _nextLayerBias )
    {
        delete[] _nextLayerBias;
        _nextLayerBias = NULL;
    }

    if ( _nextLayerLowerBounds )
    {
        delete[] _nextLayerLowerBounds;
        _nextLayerLowerBounds = NULL;
    }

    if ( _nextLayerUpperBounds )
    {
        delete[] _nextLayerUpperBounds;
        _nextLayerUpperBounds = NULL;
    }

    if ( _layerSizes )
    {
        delete[] _layerSizes;
        _layerSizes = NULL;
    }

    _inputLowerBounds.clear();
    _inputUpperBounds.clear();
}

void SymbolicBoundTightener::setNumberOfLayers( unsigned layers )
{
    freeMemoryIfNeeded();

    _numberOfLayers = layers;
    _layerSizes = new unsigned[layers];

    std::fill_n( _layerSizes, layers, 0 );
}

void SymbolicBoundTightener::setLayerSize( unsigned layer, unsigned layerSize )
{
    _layerSizes[layer] = layerSize;
}

void SymbolicBoundTightener::allocateWeightAndBiasSpace()
{
    // Allocate biases
    _biases = new double *[_numberOfLayers];
    for ( unsigned i = 0; i < _numberOfLayers; ++i )
    {
        ASSERT( _layerSizes[i] > 0 );
        _biases[i] = new double[_layerSizes[i]];

        std::fill_n( _biases[i], _layerSizes[i], 0 );
    }

    // Allocate weights
    _weights = new WeightMatrix[_numberOfLayers - 1];
    for ( unsigned i = 0; i < _numberOfLayers - 1; ++i )
    {
        // The rows represent the sources, the columns the targets
        _weights[i]._rows = _layerSizes[i];
        _weights[i]._columns = _layerSizes[i+1];
        _weights[i]._values = new double[_weights[i]._rows * _weights[i]._columns];
    }

    // Allocate work space for the bound computation
    unsigned maxLayerSize = 0;
    for ( unsigned i = 0; i < _numberOfLayers; ++i )
        if ( maxLayerSize < _layerSizes[i] )
            maxLayerSize = _layerSizes[i];

    unsigned inputLayerSize = _layerSizes[0];

    _currentLayerBias = new double[maxLayerSize];
    _currentLayerLowerBounds = new double[maxLayerSize * inputLayerSize];
    _currentLayerUpperBounds = new double[maxLayerSize * inputLayerSize];

    _nextLayerBias = new double[maxLayerSize];
    _nextLayerLowerBounds = new double[maxLayerSize * inputLayerSize];
    _nextLayerUpperBounds = new double[maxLayerSize * inputLayerSize];
}

void SymbolicBoundTightener::setBias( unsigned layer, unsigned neuron, double bias )
{
    _biases[layer][neuron] = bias;
}

void SymbolicBoundTightener::setWeight( unsigned sourceLayer, unsigned sourceNeuron, unsigned targetNeuron, double weight )
{
    _weights[sourceLayer]._values[sourceNeuron * _weights[sourceLayer]._columns + targetNeuron] = weight;
}

void SymbolicBoundTightener::setInputLowerBound( unsigned neuron, double bound )
{
    _inputLowerBounds[neuron] = bound;
}

void SymbolicBoundTightener::setInputUpperBound( unsigned neuron, double bound )
{
    _inputUpperBounds[neuron] = bound;
}

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
