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

#include "Debug.h"
#include "FloatUtils.h"
#include "MStringf.h"
#include "MarabouError.h"
#include "NetworkLevelReasoner.h"
#include "ReluConstraint.h"
#include <cstring>

NetworkLevelReasoner::NetworkLevelReasoner()
    : _weights( NULL )
    , _positiveWeights( NULL )
    , _negativeWeights( NULL )
    , _maxLayerSize( 0 )
    , _inputLayerSize( 0 )
    , _work1( NULL )
    , _work2( NULL )
    , _tableau( NULL )
    , _lowerBoundsWeightedSums( NULL )
    , _upperBoundsWeightedSums( NULL )
    , _lowerBoundsActivations( NULL )
    , _upperBoundsActivations( NULL )
    , _currentLayerLowerBounds( NULL )
    , _currentLayerUpperBounds( NULL )
    , _currentLayerLowerBias( NULL )
    , _currentLayerUpperBias( NULL )
    , _previousLayerLowerBounds( NULL )
    , _previousLayerUpperBounds( NULL )
    , _previousLayerLowerBias( NULL )
    , _previousLayerUpperBias( NULL )
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

    if ( _positiveWeights )
    {
        for ( unsigned i = 0; i < _numberOfLayers - 1; ++i )
        {
            if ( _positiveWeights[i] )
            {
                delete[] _positiveWeights[i];
                _positiveWeights[i] = NULL;
            }
        }

        delete[] _positiveWeights;
        _positiveWeights = NULL;
    }

    if ( _negativeWeights )
    {
        for ( unsigned i = 0; i < _numberOfLayers - 1; ++i )
        {
            if ( _negativeWeights[i] )
            {
                delete[] _negativeWeights[i];
                _negativeWeights[i] = NULL;
            }
        }

        delete[] _negativeWeights;
        _negativeWeights = NULL;
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

    if ( _lowerBoundsWeightedSums )
    {
        for ( unsigned i = 0; i < _numberOfLayers; ++i )
        {
            if ( _lowerBoundsWeightedSums[i] )
            {
                delete[] _lowerBoundsWeightedSums[i];
                _lowerBoundsWeightedSums[i] = NULL;
            }
        }

        delete[] _lowerBoundsWeightedSums;
        _lowerBoundsWeightedSums = NULL;
    }

    if ( _upperBoundsWeightedSums )
    {
        for ( unsigned i = 0; i < _numberOfLayers; ++i )
        {
            if ( _upperBoundsWeightedSums[i] )
            {
                delete[] _upperBoundsWeightedSums[i];
                _upperBoundsWeightedSums[i] = NULL;
            }
        }

        delete[] _upperBoundsWeightedSums;
        _upperBoundsWeightedSums = NULL;
    }

    if ( _lowerBoundsActivations )
    {
        for ( unsigned i = 0; i < _numberOfLayers; ++i )
        {
            if ( _lowerBoundsActivations[i] )
            {
                delete[] _lowerBoundsActivations[i];
                _lowerBoundsActivations[i] = NULL;
            }
        }

        delete[] _lowerBoundsActivations;
        _lowerBoundsActivations = NULL;
    }

    if ( _upperBoundsActivations )
    {
        for ( unsigned i = 0; i < _numberOfLayers; ++i )
        {
            if ( _upperBoundsActivations[i] )
            {
                delete[] _upperBoundsActivations[i];
                _upperBoundsActivations[i] = NULL;
            }
        }

        delete[] _upperBoundsActivations;
        _upperBoundsActivations = NULL;
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

    if ( _currentLayerLowerBias )
    {
        delete[] _currentLayerLowerBias;
        _currentLayerLowerBias = NULL;
    }

    if ( _currentLayerUpperBias )
    {
        delete[] _currentLayerUpperBias;
        _currentLayerUpperBias = NULL;
    }

    if ( _previousLayerLowerBounds )
    {
        delete[] _previousLayerLowerBounds;
        _previousLayerLowerBounds = NULL;
    }

    if ( _previousLayerUpperBounds )
    {
        delete[] _previousLayerUpperBounds;
        _previousLayerUpperBounds = NULL;
    }

    if ( _previousLayerLowerBias )
    {
        delete[] _previousLayerLowerBias;
        _previousLayerLowerBias = NULL;
    }

    if ( _previousLayerUpperBias )
    {
        delete[] _previousLayerUpperBias;
        _previousLayerUpperBias = NULL;
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

    if ( layer == 0 )
        _inputLayerSize = size;
}

void NetworkLevelReasoner::allocateMemoryByTopology()
{
    freeMemoryIfNeeded();

    _weights = new double *[_numberOfLayers - 1];
    if ( !_weights )
        throw MarabouError( MarabouError::ALLOCATION_FAILED, "NetworkLevelReasoner::weights" );

    for ( unsigned i = 0; i < _numberOfLayers - 1; ++i )
    {
        _weights[i] = new double[_layerSizes[i] * _layerSizes[i+1]];
        if ( !_weights[i] )
            throw MarabouError( MarabouError::ALLOCATION_FAILED, "NetworkLevelReasoner::weights[i]" );
        std::fill_n( _weights[i], _layerSizes[i] * _layerSizes[i+1], 0 );
    }

    _positiveWeights = new double *[_numberOfLayers - 1];
    if ( !_positiveWeights )
        throw MarabouError( MarabouError::ALLOCATION_FAILED, "NetworkLevelReasoner::positiveWeights" );

    for ( unsigned i = 0; i < _numberOfLayers - 1; ++i )
    {
        _positiveWeights[i] = new double[_layerSizes[i] * _layerSizes[i+1]];
        if ( !_positiveWeights[i] )
            throw MarabouError( MarabouError::ALLOCATION_FAILED, "NetworkLevelReasoner::positiveWeights[i]" );
        std::fill_n( _positiveWeights[i], _layerSizes[i] * _layerSizes[i+1], 0 );
    }

    _negativeWeights = new double *[_numberOfLayers - 1];
    if ( !_negativeWeights )
        throw MarabouError( MarabouError::ALLOCATION_FAILED, "NetworkLevelReasoner::negativeWeights" );

    for ( unsigned i = 0; i < _numberOfLayers - 1; ++i )
    {
        _negativeWeights[i] = new double[_layerSizes[i] * _layerSizes[i+1]];
        if ( !_negativeWeights[i] )
            throw MarabouError( MarabouError::ALLOCATION_FAILED, "NetworkLevelReasoner::negativeWeights[i]" );
        std::fill_n( _negativeWeights[i], _layerSizes[i] * _layerSizes[i+1], 0 );
    }

    _work1 = new double[_maxLayerSize];
    if ( !_work1 )
        throw MarabouError( MarabouError::ALLOCATION_FAILED, "NetworkLevelReasoner::work1" );

    _work2 = new double[_maxLayerSize];
    if ( !_work2 )
        throw MarabouError( MarabouError::ALLOCATION_FAILED, "NetworkLevelReasoner::work2" );

    _lowerBoundsWeightedSums = new double *[_numberOfLayers];
    _upperBoundsWeightedSums = new double *[_numberOfLayers];
    _lowerBoundsActivations = new double *[_numberOfLayers];
    _upperBoundsActivations = new double *[_numberOfLayers];

    if ( !_lowerBoundsWeightedSums || !_upperBoundsWeightedSums ||
         !_lowerBoundsActivations || !_upperBoundsActivations )
        throw MarabouError( MarabouError::ALLOCATION_FAILED, "NetworkLevelReasoner::bounds" );

    for ( unsigned i = 0; i < _numberOfLayers; ++i )
    {
        _lowerBoundsWeightedSums[i] = new double[_layerSizes[i]];
        _upperBoundsWeightedSums[i] = new double[_layerSizes[i]];
        _lowerBoundsActivations[i] = new double[_layerSizes[i]];
        _upperBoundsActivations[i] = new double[_layerSizes[i]];

        if ( !_lowerBoundsWeightedSums[i] || !_lowerBoundsWeightedSums[i] ||
             !_lowerBoundsActivations[i] || !_lowerBoundsActivations[i] )
            throw MarabouError( MarabouError::ALLOCATION_FAILED, "NetworkLevelReasoner::bounds[i]" );
    }

    _currentLayerLowerBounds = new double[_maxLayerSize * _layerSizes[0]];
    _currentLayerUpperBounds = new double[_maxLayerSize * _layerSizes[0]];
    _currentLayerLowerBias = new double[_maxLayerSize];
    _currentLayerUpperBias = new double[_maxLayerSize];

    _previousLayerLowerBounds = new double[_maxLayerSize * _layerSizes[0]];
    _previousLayerUpperBounds = new double[_maxLayerSize * _layerSizes[0]];
    _previousLayerLowerBias = new double[_maxLayerSize];
    _previousLayerUpperBias = new double[_maxLayerSize];
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

    if ( weight > 0 )
        _positiveWeights[sourceLayer][sourceNeuron * targetLayerSize + targetNeuron] = weight;
    else
        _negativeWeights[sourceLayer][sourceNeuron * targetLayerSize + targetNeuron] = weight;
}

void NetworkLevelReasoner::setBias( unsigned layer, unsigned neuron, double bias )
{
    _bias[Index( layer, neuron )] = bias;
}

void NetworkLevelReasoner::evaluate( double *input, double *output )
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

            // Store weighted sum if needed
            if ( _indexToWeightedSumVariable.exists( index ) )
                _indexToWeightedSumAssignment[index] = _work2[targetNeuron];

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

            // Store activation result if needed
            if ( _indexToActivationResultVariable.exists( index ) )
                _indexToActivationResultAssignment[index] = _work2[targetNeuron];
        }

        memcpy( _work1, _work2, sizeof(double) * targetLayerSize );
    }

    memcpy( output, _work1, sizeof(double) * _layerSizes[_numberOfLayers - 1] );
}

void NetworkLevelReasoner::setWeightedSumVariable( unsigned layer, unsigned neuron, unsigned variable )
{
    _indexToWeightedSumVariable[Index( layer, neuron )] = variable;
}

unsigned NetworkLevelReasoner::getWeightedSumVariable( unsigned layer, unsigned neuron ) const
{
    Index index( layer, neuron );
    if ( !_indexToWeightedSumVariable.exists( index ) )
        throw MarabouError( MarabouError::INVALID_WEIGHTED_SUM_INDEX, Stringf( "weighted sum: <%u,%u>", layer, neuron ).ascii() );

    return _indexToWeightedSumVariable[index];
}

void NetworkLevelReasoner::setActivationResultVariable( unsigned layer, unsigned neuron, unsigned variable )
{
    _indexToActivationResultVariable[Index( layer, neuron )] = variable;
}

unsigned NetworkLevelReasoner::getActivationResultVariable( unsigned layer, unsigned neuron ) const
{
    Index index( layer, neuron );
    if ( !_indexToActivationResultVariable.exists( index ) )
        throw MarabouError( MarabouError::INVALID_WEIGHTED_SUM_INDEX, Stringf( "activation result: <%u,%u>", layer, neuron ).ascii() );

    return _indexToActivationResultVariable[index];
}

void NetworkLevelReasoner::storeIntoOther( NetworkLevelReasoner &other ) const
{
    other.freeMemoryIfNeeded();

    other.setNumberOfLayers( _numberOfLayers );
    for ( const auto &pair : _layerSizes )
        other.setLayerSize( pair.first, pair.second );
    other.allocateMemoryByTopology();

    for ( const auto &pair : _neuronToActivationFunction )
        other.setNeuronActivationFunction( pair.first._layer, pair.first._neuron, pair.second );

    for ( unsigned i = 0; i < _numberOfLayers - 1; ++i )
    {
        memcpy( other._weights[i], _weights[i], sizeof(double) * _layerSizes[i] * _layerSizes[i+1] );
        memcpy( other._positiveWeights[i], _positiveWeights[i], sizeof(double) * _layerSizes[i] * _layerSizes[i+1] );
        memcpy( other._negativeWeights[i], _negativeWeights[i], sizeof(double) * _layerSizes[i] * _layerSizes[i+1] );
    }

    for ( const auto &pair : _bias )
        other.setBias( pair.first._layer, pair.first._neuron, pair.second );

    other._indexToWeightedSumVariable = _indexToWeightedSumVariable;
    other._indexToActivationResultVariable = _indexToActivationResultVariable;
    other._indexToWeightedSumAssignment = _indexToWeightedSumAssignment;
    other._indexToActivationResultAssignment = _indexToActivationResultAssignment;
}

const Map<NetworkLevelReasoner::Index, unsigned> &NetworkLevelReasoner::getIndexToWeightedSumVariable()
{
    return _indexToWeightedSumVariable;
}

const Map<NetworkLevelReasoner::Index, unsigned> &NetworkLevelReasoner::getIndexToActivationResultVariable()
{
    return _indexToActivationResultVariable;
}

const Map<NetworkLevelReasoner::Index, double> &NetworkLevelReasoner::getIndexToWeightedSumAssignment()
{
    return _indexToWeightedSumAssignment;
}

const Map<NetworkLevelReasoner::Index, double> &NetworkLevelReasoner::getIndexToActivationResultAssignment()
{
    return _indexToActivationResultAssignment;
}

void NetworkLevelReasoner::updateVariableIndices( const Map<unsigned, unsigned> &oldIndexToNewIndex,
                                                  const Map<unsigned, unsigned> &mergedVariables )
{
    // First, do a pass to handle any merged variables
    auto bIt = _indexToWeightedSumVariable.begin();
    while ( bIt != _indexToWeightedSumVariable.end() )
    {
        unsigned b = bIt->second;

        while ( mergedVariables.exists( b ) )
        {
            bIt->second = mergedVariables[b];
            b = bIt->second;
        }

        ++bIt;
    }

    auto fIt = _indexToActivationResultVariable.begin();
    while ( fIt != _indexToActivationResultVariable.end() )
    {
        unsigned f = fIt->second;

        while ( mergedVariables.exists( f ) )
        {
            fIt->second = mergedVariables[f];
            f = fIt->second;
        }

        ++fIt;
    }

    // Now handle re-indexing
    bIt = _indexToWeightedSumVariable.begin();
    while ( bIt != _indexToWeightedSumVariable.end() )
    {
        unsigned b = bIt->second;

        if ( !oldIndexToNewIndex.exists( b ) )
        {
            // This variable has been eliminated, remove from map
            bIt = _indexToWeightedSumVariable.erase( bIt );
        }
        else
        {
            if ( oldIndexToNewIndex[b] == b )
            {
                // Index hasn't changed, skip
            }
            else
            {
                // Index has changed
                bIt->second = oldIndexToNewIndex[b];
            }

            ++bIt;
            continue;
        }
    }

    fIt = _indexToActivationResultVariable.begin();
    while ( fIt != _indexToActivationResultVariable.end() )
    {
        unsigned f = fIt->second;
        if ( !oldIndexToNewIndex.exists( f ) )
        {
            // This variable has been eliminated, remove from map
            fIt = _indexToActivationResultVariable.erase( fIt );
        }
        else
        {
            if ( oldIndexToNewIndex[f] == f )
            {
                // Index hasn't changed, skip
            }
            else
            {
                // Index has changed
                fIt->second = oldIndexToNewIndex[f];
            }

            ++fIt;
            continue;
        }
    }
}

void NetworkLevelReasoner::obtainCurrentBounds()
{
    ASSERT( _tableau );

    for ( unsigned i = 0; i < _numberOfLayers - 1; ++i )
    {
        for ( unsigned j = 0; j < _layerSizes[i]; ++j )
        {
            unsigned varIndex = _indexToActivationResultVariable[Index( i, j )];
            _lowerBoundsActivations[i][j] = _tableau->getLowerBound( varIndex );
            _upperBoundsActivations[i][j] = _tableau->getUpperBound( varIndex );
        }
    }

    for ( unsigned i = 1; i < _numberOfLayers; ++i )
    {
        for ( unsigned j = 0; j < _layerSizes[i]; ++j )
        {
            unsigned varIndex = _indexToWeightedSumVariable[Index( i, j )];
            _lowerBoundsActivations[i][j] = _tableau->getLowerBound( varIndex );
            _upperBoundsActivations[i][j] = _tableau->getUpperBound( varIndex );
        }
    }

    printf( "NLR: current input bounds:\n" );
    for ( unsigned i = 0; i < _inputLayerSize; ++i )
        printf( "\tx%u: [%lf, %lf]\n", i, _lowerBoundsActivations[0][i], _upperBoundsActivations[0][i] );
}

void NetworkLevelReasoner::setTableau( const ITableau *tableau )
{
    _tableau = tableau;
}

void NetworkLevelReasoner::intervalArithmeticBoundPropagation()
{
    for ( unsigned i = 1; i < _numberOfLayers; ++i )
    {
        for ( unsigned j = 0; j < _layerSizes[i]; ++j )
        {
            Index index( i, j );
            double lb = _bias.exists( index ) ? _bias[index] : 0;
            double ub = _bias.exists( index ) ? _bias[index] : 0;

            for ( unsigned k = 0; k < _layerSizes[i - 1]; ++k )
            {
                double previousLb = _lowerBoundsActivations[i - 1][k];
                double previousUb = _upperBoundsActivations[i - 1][k];
                double weight = _weights[i - 1][k * _layerSizes[i] + j];

                if ( weight > 0 )
                {
                    lb += weight * previousLb;
                    ub += weight * previousUb;
                }
                else
                {
                    lb += weight * previousUb;
                    ub += weight * previousLb;
                }
            }

            _lowerBoundsWeightedSums[i][j] = lb;
            _upperBoundsWeightedSums[i][j] = ub;

            if ( i != _numberOfLayers - 1 )
            {
                // Apply activation function
                ASSERT( _neuronToActivationFunction.exists( index ) );

                switch ( _neuronToActivationFunction[index] )
                {
                case ReLU:
                    _lowerBoundsActivations[i][j] = lb > 0 ? lb : 0;
                    _upperBoundsActivations[i][j] = ub;
                    break;

                default:
                    ASSERT( false );
                    break;
                }
            }
        }
    }
}

// void NetworkLevelReasoner::initializeSymbolicBoundTightening()
// {
//     // Layers and their sizes
//     _symbolicBoundTightener.setNumberOfLayers( _numberOfLayers );

//     for ( unsigned i = 0; i < _numberOfLayers; ++i )
//         _symbolicBoundTightener.setLayerSize( i, _layerSizes[i] );

//     _symbolicBoundTightener.allocateWeightAndBiasSpace();

//     // Weights
//     for ( unsigned i = 1; i < _numberOfLayers; ++i )
//         for ( unsigned j = 0; j < _layerSizes[i]; ++j )
//             for ( unsigned k = 0; k < _layerSizes[i-1]; ++k )
//             {
//                 double weight = _weights[i - 1][k * _layerSizes[i] + j];
//                 _symbolicBoundTightener.setWeight( i - 1, k, j, weight );
//             }

//     // Biases
//     for ( unsigned i = 1; i < _numberOfLayers; ++i )
//         for ( unsigned j = 0; j < _layerSizes[i]; ++j )
//             _symbolicBoundTightener.setBias( i, j, _bias[Index( i, j )] );

//     // Weighted sum and activation result variables
//     for ( unsigned i = 1; i < _numberOfLayers; ++i )
//     {
//         for ( unsigned j = 0; j < _layerSizes[i]; ++j )
//         {
//             Index index( i, j );

//             ASSERT( _neuronToActivationFunction.exists( index ) &&
//                     _neuronToActivationFunction[index] ==
//                     NetworkLevelReasoner::ReLU );

//             _symbolicBoundTightener.setReluBVariable( i, j, _indexToWeightedSumVariable[index] );
//             _symbolicBoundTightener.setReluFVariable( i, j, _indexToActivationResultVariable[index] );
//         }
//     }
// }

void NetworkLevelReasoner::symbolicBoundPropagation()
{
    /*
      Initialize the symbolic bounds for the first layer. Each
      variable has symbolic upper and lower bound 1 for itself, 0 for
      all other varibales. The input layer has no biases.
    */

    std::fill_n( _previousLayerLowerBounds, _maxLayerSize * _inputLayerSize, 0 );
    std::fill_n( _previousLayerUpperBounds, _maxLayerSize * _inputLayerSize, 0 );
    for ( unsigned i = 0; i < _inputLayerSize; ++i )
    {
        _previousLayerLowerBounds[i * _inputLayerSize + i] = 1;
        _previousLayerUpperBounds[i * _inputLayerSize + i] = 1;
    }
    std::fill_n( _previousLayerLowerBias, _maxLayerSize, 0 );
    std::fill_n( _previousLayerUpperBias, _maxLayerSize, 0 );

    for ( unsigned currentLayer = 1; currentLayer < _numberOfLayers; ++currentLayer )
    {
        unsigned currentLayerSize = _layerSizes[currentLayer];
        unsigned previousLayerSize = _layerSizes[currentLayer - 1];

        /*
          Computing symbolic bounds for layer i, based on layer i-1.
          We assume that the bounds for the previous layer have been
          computed.
        */
        std::fill_n( _currentLayerLowerBounds, _maxLayerSize * _inputLayerSize, 0 );
        std::fill_n( _currentLayerUpperBounds, _maxLayerSize * _inputLayerSize, 0 );

        /*
          Perform the multiplication.

            newUB = oldUB * posWeights + oldLB * negWeights
            newLB = oldUB * negWeights + oldLB * posWeights

            dimensions for oldUB and oldLB: inputLayerSize x previousLayerSize
            dimensions for posWeights and negWeights: previousLayerSize x layerSize

            newUB, newLB dimensions: inputLayerSize x layerSize
        */

        for ( unsigned i = 0; i < _inputLayerSize; ++i )
        {
            for ( unsigned j = 0; j < currentLayerSize; ++j )
            {
                for ( unsigned k = 0; k < previousLayerSize; ++k )
                {
                    _currentLayerLowerBounds[i * currentLayerSize + j] +=
                        _previousLayerUpperBounds[i * previousLayerSize + k] *
                        _negativeWeights[currentLayer - 1][k * currentLayerSize + j];
                    //weights._negativeValues[k * currentLayerSize + j];

                    _currentLayerLowerBounds[i * currentLayerSize + j] +=
                        _previousLayerLowerBounds[i * previousLayerSize + k] *
                        _positiveWeights[currentLayer - 1][k * currentLayerSize + j];
                    // weights._positiveValues[k * currentLayerSize + j];

                    _currentLayerUpperBounds[i * currentLayerSize + j] +=
                        _previousLayerUpperBounds[i * previousLayerSize + k] *
                        _positiveWeights[currentLayer - 1][k * currentLayerSize + j];
                    // weights._positiveValues[k * currentLayerSize + j];

                    _currentLayerUpperBounds[i * currentLayerSize + j] +=
                        _previousLayerLowerBounds[i * previousLayerSize + k] *
                        _negativeWeights[currentLayer - 1][k * currentLayerSize + j];
                    // weights._negativeValues[k * currentLayerSize + j];
                }
            }
        }

        /*
          Compute the biases for the new layer
        */
        for ( unsigned j = 0; j < currentLayerSize; ++j )
        {
            _currentLayerLowerBias[j] = _bias[Index( currentLayer, j )];
            _currentLayerUpperBias[j] = _bias[Index( currentLayer, j )];

            // Add the weighted bias from the previous layer
            for ( unsigned k = 0; k < previousLayerSize; ++k )
            {
                double weight = _weights[currentLayer - 1][k * currentLayerSize + j];

                if ( weight > 0 )
                {
                    _currentLayerLowerBias[j] += _previousLayerLowerBias[k] * weight;
                    _currentLayerUpperBias[j] += _previousLayerUpperBias[k] * weight;
                }
                else
                {
                    _currentLayerLowerBias[j] += _previousLayerUpperBias[k] * weight;
                    _currentLayerUpperBias[j] += _previousLayerLowerBias[k] * weight;
                }
            }
        }

        /*
          We now have the symbolic representation for the new
          layer. Next, we compute new lower and upper bounds for
          it. For each of these bounds, we compute an upper bound and
          a lower bound.

          newUB, newLB dimensions: inputLayerSize x layerSize
        */
        for ( unsigned i = 0; i < currentLayerSize; ++i )
        {
            /*
              lbLb: the lower bound for the expression of the lower bound
              lbUb: the upper bound for the expression of the lower bound
              etc.
            */

            double lbLb = 0;
            double lbUb = 0;
            double ubLb = 0;
            double ubUb = 0;

            for ( unsigned j = 0; j < _inputLayerSize; ++j )
            {
                double entry = _currentLayerLowerBounds[j * currentLayerSize + i];

                if ( entry >= 0 )
                {
                    lbLb += ( entry * _lowerBoundsActivations[0][j] );
                    lbUb += ( entry * _upperBoundsActivations[0][j] );
                }
                else
                {
                    lbLb += ( entry * _upperBoundsActivations[0][j] );
                    lbUb += ( entry * _lowerBoundsActivations[0][j] );
                }

                lbLb -= GlobalConfiguration::SYMBOLIC_TIGHTENING_ROUNDING_CONSTANT;
                lbUb += GlobalConfiguration::SYMBOLIC_TIGHTENING_ROUNDING_CONSTANT;

                entry = _currentLayerUpperBounds[j * currentLayerSize + i];

                if ( entry >= 0 )
                {
                    ubLb += ( entry * _lowerBoundsActivations[0][j] );
                    ubUb += ( entry * _upperBoundsActivations[0][j] );
                }
                else
                {
                    ubLb += ( entry * _upperBoundsActivations[0][j] );
                    ubUb += ( entry * _lowerBoundsActivations[0][j] );
                }

                ubLb -= GlobalConfiguration::SYMBOLIC_TIGHTENING_ROUNDING_CONSTANT;
                ubUb += GlobalConfiguration::SYMBOLIC_TIGHTENING_ROUNDING_CONSTANT;
            }

            // Add the network bias to all bounds
            lbLb += _currentLayerLowerBias[i];
            lbUb += _currentLayerLowerBias[i];
            ubLb += _currentLayerUpperBias[i];
            ubUb += _currentLayerUpperBias[i];

            /*
              We now have the tightest bounds we can for the weighted
              sum variable. If they are tigheter than what was
              previously known, store them.
            */
            if ( _lowerBoundsWeightedSums[currentLayer][i] < lbLb )
                _lowerBoundsWeightedSums[currentLayer][i] = lbLb;
            if ( _upperBoundsWeightedSums[currentLayer][i] > ubUb )
                _upperBoundsWeightedSums[currentLayer][i] = ubUb;

            /*
              Next, we see what can be deduced regarding the bounds of
              the activation result
            */
            if ( currentLayer < _numberOfLayers - 1 )
            {
                ReluConstraint::PhaseStatus reluPhase = ReluConstraint::PHASE_NOT_FIXED;
                if ( _lowerBoundsWeightedSums[currentLayer][i] >= 0 )
                    reluPhase = ReluConstraint::PHASE_ACTIVE;
                else if ( _upperBoundsWeightedSums[currentLayer][i] <= 0 )
                    reluPhase = ReluConstraint::PHASE_INACTIVE;

                /*
                  If the ReLU phase is not fixed yet, see whether the
                  newly computed bounds imply that it should be fixed.
                */
                if ( reluPhase == ReluConstraint::PHASE_NOT_FIXED )
                {
                    // If we got here, we know that lbLb < 0 and ubUb
                    // > 0 There are four possible cases, depending on
                    // whether ubLb and lbUb are negative or positive
                    // (see Neurify paper, page 14).

                    // Upper bound
                    if ( ubLb <= 0 )
                    {
                        // ubLb < 0 < ubUb
                        // Concretize the upper bound using the Ehler's-like approximation
                        for ( unsigned j = 0; j < _inputLayerSize; ++j )
                            _currentLayerUpperBounds[j * currentLayerSize + i] =
                                _currentLayerUpperBounds[j * currentLayerSize + i] * ubUb / ( ubUb - ubLb );

                        // Do the same for the bias, and then adjust
                        _currentLayerUpperBias[i] = _currentLayerUpperBias[i] * ubUb / ( ubUb - ubLb );
                        _currentLayerUpperBias[i] -= ubLb * ubUb / ( ubUb - ubLb );
                    }

                    // Lower bound
                    if ( lbUb <= 0 )
                    {
                        for ( unsigned j = 0; j < _inputLayerSize; ++j )
                            _currentLayerLowerBounds[j * currentLayerSize + i] = 0;

                        _currentLayerLowerBias[i] = 0;
                    }
                    else
                    {
                        for ( unsigned j = 0; j < _inputLayerSize; ++j )
                            _currentLayerLowerBounds[j * currentLayerSize + i] =
                                _currentLayerLowerBounds[j * currentLayerSize + i] * lbUb / ( lbUb - lbLb );

                        _currentLayerLowerBias[i] = _currentLayerLowerBias[i] * lbUb / ( lbUb - lbLb );
                    }

                    lbLb = 0;
                }
                else
                {
                    // The phase of this ReLU is fixed!
                    if ( reluPhase == ReluConstraint::PHASE_ACTIVE )
                    {
                        // Active ReLU, bounds are propagated as is
                    }
                    else
                    {
                        // Inactive ReLU, returns zero
                        lbLb = 0;
                        lbUb = 0;
                        ubLb = 0;
                        ubUb = 0;

                        for ( unsigned j = 0; j < _inputLayerSize; ++j )
                        {
                            _currentLayerLowerBounds[j * currentLayerSize + i] = 0;
                            _currentLayerUpperBounds[j * currentLayerSize + i] = 0;
                        }
                        _currentLayerLowerBias[i] = 0;
                        _currentLayerUpperBias[i] = 0;
                    }
                }
            }

            // Store the bounds for this neuron
            if ( _lowerBoundsActivations[currentLayer][i] < lbLb )
                _lowerBoundsActivations[currentLayer][i] = lbLb;
            if ( _upperBoundsActivations[currentLayer][i] > ubUb )
                _upperBoundsActivations[currentLayer][i] = ubUb;
        }

        // Prepare for next iteration
        memcpy( _previousLayerLowerBounds, _currentLayerLowerBounds, sizeof(double) * _maxLayerSize * _inputLayerSize );
        memcpy( _previousLayerUpperBounds, _currentLayerUpperBounds, sizeof(double) * _maxLayerSize * _inputLayerSize );
        memcpy( _previousLayerLowerBias, _currentLayerLowerBias, sizeof(double) * _maxLayerSize );
        memcpy( _previousLayerUpperBias, _currentLayerUpperBias, sizeof(double) * _maxLayerSize );
    }
}

void NetworkLevelReasoner::getConstraintTightenings( List<Tightening> &tightenings ) const
{
    tightenings.clear();

    for ( unsigned i = 1; i < _numberOfLayers; ++i )
    {
        for ( unsigned j = 0; j < _layerSizes[i]; ++j )
        {
            Index index( i, j );

            // Weighted sums
            tightenings.append( Tightening( _indexToWeightedSumVariable[index],
                                            _lowerBoundsWeightedSums[i][j],
                                            Tightening::LB ) );

            tightenings.append( Tightening( _indexToWeightedSumVariable[index],
                                            _upperBoundsWeightedSums[i][j],
                                            Tightening::UB ) );

            // Activation results
            if ( i != _numberOfLayers - 1 )
            {
                tightenings.append( Tightening( _indexToActivationResultVariable[index],
                                                _lowerBoundsActivations[i][j],
                                                Tightening::LB ) );
                tightenings.append( Tightening( _indexToActivationResultVariable[index],
                                                _upperBoundsActivations[i][j],
                                                Tightening::UB ) );
            }
        }
    }
}

void NetworkLevelReasoner::log( const String &message )
{
    if ( GlobalConfiguration::NETWORK_LEVEL_REASONER_LOGGING )
        printf( "%s", message.ascii() );
}

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
