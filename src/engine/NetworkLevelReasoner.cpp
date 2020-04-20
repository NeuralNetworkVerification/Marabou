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
#include <cstring>

NetworkLevelReasoner::NetworkLevelReasoner()
    : _weights( NULL )
    , _maxLayerSize( 0 )
    , _work1( NULL )
    , _work2( NULL )
    , _tableau( NULL )
    , _lowerBoundsWeightedSums( NULL )
    , _upperBoundsWeightedSums( NULL )
    , _lowerBoundsActivations( NULL )
    , _upperBoundsActivations( NULL )
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

void NetworkLevelReasoner::allocateMemoryByTopology()
{
    freeMemoryIfNeeded();

    _weights = new double*[_numberOfLayers - 1];
    if ( !_weights )
        throw MarabouError( MarabouError::ALLOCATION_FAILED, "NetworkLevelReasoner::weights" );

    for ( unsigned i = 0; i < _numberOfLayers - 1; ++i )
    {
        _weights[i] = new double[_layerSizes[i] * _layerSizes[i+1]];
        if ( !_weights[i] )
            throw MarabouError( MarabouError::ALLOCATION_FAILED, "NetworkLevelReasoner::weights[i]" );
        std::fill_n( _weights[i], _layerSizes[i] * _layerSizes[i+1], 0 );
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
        memcpy( other._weights[i], _weights[i], sizeof(double) * _layerSizes[i] * _layerSizes[i+1] );

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

void NetworkLevelReasoner::obtainInputBounds()
{
    ASSERT( _tableau );

    for ( unsigned i = 0; i < _layerSizes[0]; ++i )
    {
        unsigned varIndex = _indexToActivationResultVariable[Index( 0, i )];
        _lowerBoundsActivations[0][i] = _tableau->getLowerBound( varIndex );
        _upperBoundsActivations[0][i] = _tableau->getUpperBound( varIndex );
    }
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

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
