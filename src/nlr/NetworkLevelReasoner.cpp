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

#include "AbsoluteValueConstraint.h"
#include "Debug.h"
#include "FloatUtils.h"
#include "MStringf.h"
#include "MarabouError.h"
#include "NetworkLevelReasoner.h"
#include "ReluConstraint.h"
#include <cstring>

namespace NLR {

NetworkLevelReasoner::NetworkLevelReasoner()
    : _tableau( NULL )
{
}

NetworkLevelReasoner::~NetworkLevelReasoner()
{
    freeMemoryIfNeeded();
}

bool NetworkLevelReasoner::functionTypeSupported( PiecewiseLinearFunctionType type )
{
    if ( type == PiecewiseLinearFunctionType::RELU )
        return true;

    if ( type == PiecewiseLinearFunctionType::ABSOLUTE_VALUE )
        return true;

    return false;
}

void NetworkLevelReasoner::addLayer( unsigned layerIndex, Layer::Type type, unsigned layerSize )
{
    Layer *layer = new Layer( layerIndex, type, layerSize, this );
    _layerIndexToLayer[layerIndex] = layer;
}

void NetworkLevelReasoner::addLayerDependency( unsigned sourceLayer, unsigned targetLayer )
{
    _layerIndexToLayer[targetLayer]->addSourceLayer( sourceLayer, _layerIndexToLayer[sourceLayer]->getSize() );
}

void NetworkLevelReasoner::setWeight( unsigned sourceLayer,
                                      unsigned sourceNeuron,
                                      unsigned targetLayer,
                                      unsigned targetNeuron,
                                      double weight )
{
    _layerIndexToLayer[targetLayer]->setWeight
        ( sourceLayer, sourceNeuron, targetNeuron, weight );
}

void NetworkLevelReasoner::setBias( unsigned layer, unsigned neuron, double bias )
{
    _layerIndexToLayer[layer]->setBias( neuron, bias );
}

void NetworkLevelReasoner::addActivationSource( unsigned sourceLayer,
                                                unsigned sourceNeuron,
                                                unsigned targetLeyer,
                                                unsigned targetNeuron )
{
    _layerIndexToLayer[targetLeyer]->addActivationSource( sourceLayer, sourceNeuron, targetNeuron );
}

const Layer *NetworkLevelReasoner::getLayer( unsigned index ) const
{
    return _layerIndexToLayer[index];
}

void NetworkLevelReasoner::evaluate( double *input, double *output )
{
    _layerIndexToLayer[0]->setAssignment( input );
    for ( unsigned i = 1; i < _layerIndexToLayer.size(); ++i )
        _layerIndexToLayer[i]->computeAssignment();

    const Layer *outputLayer = _layerIndexToLayer[_layerIndexToLayer.size() - 1];
    memcpy( output,
            outputLayer->getAssignment(),
            sizeof(double) * outputLayer->getSize() );
}

void NetworkLevelReasoner::setNeuronVariable( NeuronIndex index, unsigned variable )
{
    _layerIndexToLayer[index._layer]->setNeuronVariable( index._neuron, variable );
}

void NetworkLevelReasoner::receiveTighterBound( Tightening tightening )
{
    _boundTightenings.append( tightening );
}

void NetworkLevelReasoner::getConstraintTightenings( List<Tightening> &tightenings )
{
    tightenings = _boundTightenings;
    _boundTightenings.clear();
}

void NetworkLevelReasoner::symbolicBoundPropagation()
{
    for ( unsigned i = 0; i < _layerIndexToLayer.size(); ++i )
        _layerIndexToLayer[i]->computeSymbolicBounds();
}


// NetworkLevelReasoner::NetworkLevelReasoner()
//     : _numberOfLayers( 0 )
//     , _weights( NULL )
//     , _positiveWeights( NULL )
//     , _negativeWeights( NULL )
//     , _maxLayerSize( 0 )
//     , _inputLayerSize( 0 )
//     , _work1( NULL )
//     , _work2( NULL )
//     , _tableau( NULL )
//     , _lowerBoundsWeightedSums( NULL )
//     , _upperBoundsWeightedSums( NULL )
//     , _lowerBoundsActivations( NULL )
//     , _upperBoundsActivations( NULL )
//     , _currentLayerLowerBounds( NULL )
//     , _currentLayerUpperBounds( NULL )
//     , _currentLayerLowerBias( NULL )
//     , _currentLayerUpperBias( NULL )
//     , _previousLayerLowerBounds( NULL )
//     , _previousLayerUpperBounds( NULL )
//     , _previousLayerLowerBias( NULL )
//     , _previousLayerUpperBias( NULL )
// {
// }

// NetworkLevelReasoner::~NetworkLevelReasoner()
// {
//     freeMemoryIfNeeded();
// }

void NetworkLevelReasoner::freeMemoryIfNeeded()
{
    for ( const auto &layer : _layerIndexToLayer )
        delete layer.second;
    _layerIndexToLayer.clear();
}

// void NetworkLevelReasoner::setNumberOfLayers( unsigned numberOfLayers )
// {
//     _numberOfLayers = numberOfLayers;
// }

// void NetworkLevelReasoner::setLayerSize( unsigned layer, unsigned size )
// {
//     ASSERT( layer < _numberOfLayers );
//     _layerSizes[layer] = size;

//     if ( size > _maxLayerSize )
//         _maxLayerSize = size;

//     if ( layer == 0 )
//         _inputLayerSize = size;
// }

// void NetworkLevelReasoner::allocateMemoryByTopology()
// {
//     freeMemoryIfNeeded();

//     if ( _numberOfLayers == 0 )
//         return;

//     _weights = new double*[_numberOfLayers - 1];
//     if ( !_weights )
//         throw MarabouError( MarabouError::ALLOCATION_FAILED, "NetworkLevelReasoner::weights" );

//     for ( unsigned i = 0; i < _numberOfLayers - 1; ++i )
//     {
//         _weights[i] = new double[_layerSizes[i] * _layerSizes[i+1]];
//         if ( !_weights[i] )
//             throw MarabouError( MarabouError::ALLOCATION_FAILED, "NetworkLevelReasoner::weights[i]" );
//         std::fill_n( _weights[i], _layerSizes[i] * _layerSizes[i+1], 0 );
//     }

//     _positiveWeights = new double *[_numberOfLayers - 1];
//     if ( !_positiveWeights )
//         throw MarabouError( MarabouError::ALLOCATION_FAILED, "NetworkLevelReasoner::positiveWeights" );

//     for ( unsigned i = 0; i < _numberOfLayers - 1; ++i )
//     {
//         _positiveWeights[i] = new double[_layerSizes[i] * _layerSizes[i+1]];
//         if ( !_positiveWeights[i] )
//             throw MarabouError( MarabouError::ALLOCATION_FAILED, "NetworkLevelReasoner::positiveWeights[i]" );
//         std::fill_n( _positiveWeights[i], _layerSizes[i] * _layerSizes[i+1], 0 );
//     }

//     _negativeWeights = new double *[_numberOfLayers - 1];
//     if ( !_negativeWeights )
//         throw MarabouError( MarabouError::ALLOCATION_FAILED, "NetworkLevelReasoner::negativeWeights" );

//     for ( unsigned i = 0; i < _numberOfLayers - 1; ++i )
//     {
//         _negativeWeights[i] = new double[_layerSizes[i] * _layerSizes[i+1]];
//         if ( !_negativeWeights[i] )
//             throw MarabouError( MarabouError::ALLOCATION_FAILED, "NetworkLevelReasoner::negativeWeights[i]" );
//         std::fill_n( _negativeWeights[i], _layerSizes[i] * _layerSizes[i+1], 0 );
//     }

//     _work1 = new double[_maxLayerSize];
//     if ( !_work1 )
//         throw MarabouError( MarabouError::ALLOCATION_FAILED, "NetworkLevelReasoner::work1" );

//     _work2 = new double[_maxLayerSize];
//     if ( !_work2 )
//         throw MarabouError( MarabouError::ALLOCATION_FAILED, "NetworkLevelReasoner::work2" );

//     _lowerBoundsWeightedSums = new double *[_numberOfLayers];
//     _upperBoundsWeightedSums = new double *[_numberOfLayers];
//     _lowerBoundsActivations = new double *[_numberOfLayers];
//     _upperBoundsActivations = new double *[_numberOfLayers];

//     if ( !_lowerBoundsWeightedSums || !_upperBoundsWeightedSums ||
//          !_lowerBoundsActivations || !_upperBoundsActivations )
//         throw MarabouError( MarabouError::ALLOCATION_FAILED, "NetworkLevelReasoner::bounds" );

//     for ( unsigned i = 0; i < _numberOfLayers; ++i )
//     {
//         _lowerBoundsWeightedSums[i] = new double[_layerSizes[i]];
//         _upperBoundsWeightedSums[i] = new double[_layerSizes[i]];
//         _lowerBoundsActivations[i] = new double[_layerSizes[i]];
//         _upperBoundsActivations[i] = new double[_layerSizes[i]];

//         if ( !_lowerBoundsWeightedSums[i] || !_lowerBoundsWeightedSums[i] ||
//              !_lowerBoundsActivations[i] || !_lowerBoundsActivations[i] )
//             throw MarabouError( MarabouError::ALLOCATION_FAILED, "NetworkLevelReasoner::bounds[i]" );
//     }

//     _currentLayerLowerBounds = new double[_maxLayerSize * _layerSizes[0]];
//     _currentLayerUpperBounds = new double[_maxLayerSize * _layerSizes[0]];
//     _currentLayerLowerBias = new double[_maxLayerSize];
//     _currentLayerUpperBias = new double[_maxLayerSize];

//     _previousLayerLowerBounds = new double[_maxLayerSize * _layerSizes[0]];
//     _previousLayerUpperBounds = new double[_maxLayerSize * _layerSizes[0]];
//     _previousLayerLowerBias = new double[_maxLayerSize];
//     _previousLayerUpperBias = new double[_maxLayerSize];
// }

// void NetworkLevelReasoner::setNeuronActivationFunction( unsigned layer, unsigned neuron, PiecewiseLinearFunctionType activationFuction )
// {
//     _neuronToActivationFunction[NeuronIndex( layer, neuron )] = activationFuction;
// }

// void NetworkLevelReasoner::setWeight( unsigned sourceLayer, unsigned sourceNeuron, unsigned targetNeuron, double weight )
// {
//     ASSERT( _weights );

//     unsigned targetLayerSize = _layerSizes[sourceLayer + 1];
//     _weights[sourceLayer][sourceNeuron * targetLayerSize + targetNeuron] = weight;

//     if ( weight > 0 )
//         _positiveWeights[sourceLayer][sourceNeuron * targetLayerSize + targetNeuron] = weight;
//     else
//         _negativeWeights[sourceLayer][sourceNeuron * targetLayerSize + targetNeuron] = weight;
// }

// void NetworkLevelReasoner::setBias( unsigned layer, unsigned neuron, double bias )
// {
//     _bias[NeuronIndex( layer, neuron )] = bias;
// }


// void NetworkLevelReasoner::setWeightedSumVariable( unsigned layer, unsigned neuron, unsigned variable )
// {
//     _indexToWeightedSumVariable[NeuronIndex( layer, neuron )] = variable;
//     _weightedSumVariableToIndex[variable] = NeuronIndex( layer, neuron );
// }

// unsigned NetworkLevelReasoner::getWeightedSumVariable( unsigned layer, unsigned neuron ) const
// {
//     NeuronIndex index( layer, neuron );
//     if ( !_indexToWeightedSumVariable.exists( index ) )
//         throw MarabouError( MarabouError::INVALID_WEIGHTED_SUM_INDEX, Stringf( "weighted sum: <%u,%u>", layer, neuron ).ascii() );

//     return _indexToWeightedSumVariable[index];
// }

// void NetworkLevelReasoner::setActivationResultVariable( unsigned layer, unsigned neuron, unsigned variable )
// {
//     _indexToActivationResultVariable[NeuronIndex( layer, neuron )] = variable;
//     _activationResultVariableToIndex[variable] = NeuronIndex( layer, neuron );
// }

// unsigned NetworkLevelReasoner::getActivationResultVariable( unsigned layer, unsigned neuron ) const
// {
//     NeuronIndex index( layer, neuron );
//     if ( !_indexToActivationResultVariable.exists( index ) )
//         throw MarabouError( MarabouError::INVALID_WEIGHTED_SUM_INDEX, Stringf( "activation result: <%u,%u>", layer, neuron ).ascii() );

//     return _indexToActivationResultVariable[index];
// }

void NetworkLevelReasoner::storeIntoOther( NetworkLevelReasoner &other ) const
{
    other.freeMemoryIfNeeded();

    for ( const auto &layer : _layerIndexToLayer )
    {
        const Layer *thisLayer = layer.second;
        Layer *newLayer = new Layer( thisLayer );
        newLayer->setLayerOwner( &other );
        other._layerIndexToLayer[newLayer->getLayerIndex()] = newLayer;
    }
}

// const Map<NeuronIndex, unsigned> &NetworkLevelReasoner::getIndexToWeightedSumVariable()
// {
//     return _indexToWeightedSumVariable;
// }

// const Map<NeuronIndex, unsigned> &NetworkLevelReasoner::getIndexToActivationResultVariable()
// {
//     return _indexToActivationResultVariable;
// }

// const Map<NeuronIndex, double> &NetworkLevelReasoner::getIndexToWeightedSumAssignment()
// {
//     return _indexToWeightedSumAssignment;
// }

// const Map<NeuronIndex, double> &NetworkLevelReasoner::getIndexToActivationResultAssignment()
// {
//     return _indexToActivationResultAssignment;
// }

void NetworkLevelReasoner::updateVariableIndices( const Map<unsigned, unsigned> &oldIndexToNewIndex,
                                                  const Map<unsigned, unsigned> &mergedVariables )
{
    for ( auto &layer : _layerIndexToLayer )
        layer.second->updateVariableIndices( oldIndexToNewIndex, mergedVariables );
}

void NetworkLevelReasoner::obtainCurrentBounds()
{
    ASSERT( _tableau );
    for ( const auto &layer : _layerIndexToLayer )
        layer.second->obtainCurrentBounds();
}

void NetworkLevelReasoner::setTableau( const ITableau *tableau )
{
    _tableau = tableau;
}

const ITableau *NetworkLevelReasoner::getTableau() const
{
    return _tableau;
}

// void NetworkLevelReasoner::intervalArithmeticBoundPropagation()
// {
//     for ( unsigned i = 1; i < _numberOfLayers; ++i )
//     {
//         for ( unsigned j = 0; j < _layerSizes[i]; ++j )
//         {
//             NeuronIndex index( i, j );
//             double lb = _bias.exists( index ) ? _bias[index] : 0;
//             double ub = _bias.exists( index ) ? _bias[index] : 0;

//             for ( unsigned k = 0; k < _layerSizes[i - 1]; ++k )
//             {
//                 double previousLb = _lowerBoundsActivations[i - 1][k];
//                 double previousUb = _upperBoundsActivations[i - 1][k];
//                 double weight = _weights[i - 1][k * _layerSizes[i] + j];

//                 if ( weight > 0 )
//                 {
//                     lb += weight * previousLb;
//                     ub += weight * previousUb;
//                 }
//                 else
//                 {
//                     lb += weight * previousUb;
//                     ub += weight * previousLb;
//                 }
//             }

//             if ( lb > _lowerBoundsWeightedSums[i][j] )
//                 _lowerBoundsWeightedSums[i][j] = lb;
//             if ( ub < _upperBoundsWeightedSums[i][j] )
//                 _upperBoundsWeightedSums[i][j] = ub;

//             if ( i != _numberOfLayers - 1 )
//             {
//                 // Apply activation function
//                 ASSERT( _neuronToActivationFunction.exists( index ) );

//                 switch ( _neuronToActivationFunction[index] )
//                 {
//                 case PiecewiseLinearFunctionType::RELU:
//                     if ( lb < 0 )
//                         lb = 0;

//                     if ( lb > _lowerBoundsActivations[i][j] )
//                         _lowerBoundsActivations[i][j] = lb;
//                     if ( ub < _upperBoundsActivations[i][j] )
//                         _upperBoundsActivations[i][j] = ub;

//                     break;

//                 case PiecewiseLinearFunctionType::ABSOLUTE_VALUE:
//                     if ( lb > 0 )
//                     {
//                         if ( lb > _lowerBoundsActivations[i][j] )
//                             _lowerBoundsActivations[i][j] = lb;
//                         if ( ub < _upperBoundsActivations[i][j] )
//                             _upperBoundsActivations[i][j] = ub;
//                     }
//                     else if ( ub < 0 )
//                     {
//                         if ( -ub > _lowerBoundsActivations[i][j] )
//                             _lowerBoundsActivations[i][j] = -ub;
//                         if ( -lb < _upperBoundsActivations[i][j] )
//                             _upperBoundsActivations[i][j] = -lb;
//                     }
//                     else
//                     {
//                         // lb < 0 < ub
//                         if ( _lowerBoundsActivations[i][j] < 0 )
//                             _lowerBoundsActivations[i][j] = 0;

//                         if ( FloatUtils::max( ub, -lb ) < _upperBoundsActivations[i][j] )
//                             _upperBoundsActivations[i][j] = FloatUtils::max( ub, -lb );
//                     }

//                     break;

//                 default:
//                     throw MarabouError( MarabouError::NETWORK_LEVEL_REASONER_ACTIVATION_NOT_SUPPORTED );
//                     break;
//                 }
//             }
//         }
//     }
// }

// void NetworkLevelReasoner::symbolicBoundPropagation()
// {
//     /*
//       Initialize the symbolic bounds for the first layer. Each
//       variable has symbolic upper and lower bound 1 for itself, 0 for
//       all other varibales. The input layer has no biases.
//     */

//     std::fill_n( _previousLayerLowerBounds, _maxLayerSize * _inputLayerSize, 0 );
//     std::fill_n( _previousLayerUpperBounds, _maxLayerSize * _inputLayerSize, 0 );
//     for ( unsigned i = 0; i < _inputLayerSize; ++i )
//     {
//         _previousLayerLowerBounds[i * _inputLayerSize + i] = 1;
//         _previousLayerUpperBounds[i * _inputLayerSize + i] = 1;
//     }
//     std::fill_n( _previousLayerLowerBias, _maxLayerSize, 0 );
//     std::fill_n( _previousLayerUpperBias, _maxLayerSize, 0 );

//     for ( unsigned currentLayer = 1; currentLayer < _numberOfLayers; ++currentLayer )
//     {
//         unsigned currentLayerSize = _layerSizes[currentLayer];
//         unsigned previousLayerSize = _layerSizes[currentLayer - 1];

//         /*
//           Computing symbolic bounds for layer i, based on layer i-1.
//           We assume that the bounds for the previous layer have been
//           computed.
//         */
//         std::fill_n( _currentLayerLowerBounds, _maxLayerSize * _inputLayerSize, 0 );
//         std::fill_n( _currentLayerUpperBounds, _maxLayerSize * _inputLayerSize, 0 );

//         /*
//           Perform the multiplication.

//             newUB = oldUB * posWeights + oldLB * negWeights
//             newLB = oldUB * negWeights + oldLB * posWeights

//             dimensions for oldUB and oldLB: inputLayerSize x previousLayerSize
//             dimensions for posWeights and negWeights: previousLayerSize x layerSize

//             newUB, newLB dimensions: inputLayerSize x layerSize
//         */

//         for ( unsigned i = 0; i < _inputLayerSize; ++i )
//         {
//             for ( unsigned j = 0; j < currentLayerSize; ++j )
//             {
//                 for ( unsigned k = 0; k < previousLayerSize; ++k )
//                 {
//                     _currentLayerLowerBounds[i * currentLayerSize + j] +=
//                         _previousLayerUpperBounds[i * previousLayerSize + k] *
//                         _negativeWeights[currentLayer - 1][k * currentLayerSize + j];

//                     _currentLayerLowerBounds[i * currentLayerSize + j] +=
//                         _previousLayerLowerBounds[i * previousLayerSize + k] *
//                         _positiveWeights[currentLayer - 1][k * currentLayerSize + j];

//                     _currentLayerUpperBounds[i * currentLayerSize + j] +=
//                         _previousLayerUpperBounds[i * previousLayerSize + k] *
//                         _positiveWeights[currentLayer - 1][k * currentLayerSize + j];

//                     _currentLayerUpperBounds[i * currentLayerSize + j] +=
//                         _previousLayerLowerBounds[i * previousLayerSize + k] *
//                         _negativeWeights[currentLayer - 1][k * currentLayerSize + j];
//                 }
//             }
//         }

//         /*
//           Compute the biases for the new layer
//         */
//         for ( unsigned j = 0; j < currentLayerSize; ++j )
//         {
//             _currentLayerLowerBias[j] = _bias[NeuronIndex( currentLayer, j )];
//             _currentLayerUpperBias[j] = _bias[NeuronIndex( currentLayer, j )];

//             // Add the weighted bias from the previous layer
//             for ( unsigned k = 0; k < previousLayerSize; ++k )
//             {
//                 double weight = _weights[currentLayer - 1][k * currentLayerSize + j];

//                 if ( weight > 0 )
//                 {
//                     _currentLayerLowerBias[j] += _previousLayerLowerBias[k] * weight;
//                     _currentLayerUpperBias[j] += _previousLayerUpperBias[k] * weight;
//                 }
//                 else
//                 {
//                     _currentLayerLowerBias[j] += _previousLayerUpperBias[k] * weight;
//                     _currentLayerUpperBias[j] += _previousLayerLowerBias[k] * weight;
//                 }
//             }
//         }

//         /*
//           We now have the symbolic representation for the new
//           layer. Next, we compute new lower and upper bounds for
//           it. For each of these bounds, we compute an upper bound and
//           a lower bound.

//           newUB, newLB dimensions: inputLayerSize x layerSize
//         */
//         for ( unsigned i = 0; i < currentLayerSize; ++i )
//         {
//             /*
//               lbLb: the lower bound for the expression of the lower bound
//               lbUb: the upper bound for the expression of the lower bound
//               etc.
//             */

//             double lbLb = 0;
//             double lbUb = 0;
//             double ubLb = 0;
//             double ubUb = 0;

//             for ( unsigned j = 0; j < _inputLayerSize; ++j )
//             {
//                 double entry = _currentLayerLowerBounds[j * currentLayerSize + i];

//                 if ( entry >= 0 )
//                 {
//                     lbLb += ( entry * _lowerBoundsActivations[0][j] );
//                     lbUb += ( entry * _upperBoundsActivations[0][j] );
//                 }
//                 else
//                 {
//                     lbLb += ( entry * _upperBoundsActivations[0][j] );
//                     lbUb += ( entry * _lowerBoundsActivations[0][j] );
//                 }

//                 entry = _currentLayerUpperBounds[j * currentLayerSize + i];

//                 if ( entry >= 0 )
//                 {
//                     ubLb += ( entry * _lowerBoundsActivations[0][j] );
//                     ubUb += ( entry * _upperBoundsActivations[0][j] );
//                 }
//                 else
//                 {
//                     ubLb += ( entry * _upperBoundsActivations[0][j] );
//                     ubUb += ( entry * _lowerBoundsActivations[0][j] );
//                 }
//             }

//             // Add the network bias to all bounds
//             lbLb += _currentLayerLowerBias[i];
//             lbUb += _currentLayerLowerBias[i];
//             ubLb += _currentLayerUpperBias[i];
//             ubUb += _currentLayerUpperBias[i];

//             /*
//               We now have the tightest bounds we can for the weighted
//               sum variable. If they are tigheter than what was
//               previously known, store them.
//             */
//             if ( _lowerBoundsWeightedSums[currentLayer][i] < lbLb )
//                 _lowerBoundsWeightedSums[currentLayer][i] = lbLb;
//             if ( _upperBoundsWeightedSums[currentLayer][i] > ubUb )
//                 _upperBoundsWeightedSums[currentLayer][i] = ubUb;

//             /*
//               Next, we see what can be deduced regarding the bounds of
//               the activation result
//             */
//             if ( currentLayer < _numberOfLayers - 1 )
//             {
//                 // Propagate according to the specific activation function
//                 NeuronIndex index( currentLayer, i );
//                 ASSERT( _neuronToActivationFunction.exists( index ) );

//                 switch ( _neuronToActivationFunction[index] )
//                 {
//                 case PiecewiseLinearFunctionType::RELU:
//                     reluSymbolicPropagation( index, lbLb, lbUb, ubLb, ubUb );
//                     break;

//                 case PiecewiseLinearFunctionType::ABSOLUTE_VALUE:
//                     absoluteValueSymbolicPropagation( index, lbLb, lbUb, ubLb, ubUb );
//                     break;

//                 default:
//                     throw MarabouError( MarabouError::NETWORK_LEVEL_REASONER_ACTIVATION_NOT_SUPPORTED );
//                     break;
//                 }

//                 // Store the bounds for this neuron
//                 if ( _lowerBoundsActivations[currentLayer][i] < lbLb )
//                     _lowerBoundsActivations[currentLayer][i] = lbLb;
//                 if ( _upperBoundsActivations[currentLayer][i] > ubUb )
//                     _upperBoundsActivations[currentLayer][i] = ubUb;
//             }
//         }

//         // Prepare for next iteration
//         memcpy( _previousLayerLowerBounds, _currentLayerLowerBounds, sizeof(double) * _maxLayerSize * _inputLayerSize );
//         memcpy( _previousLayerUpperBounds, _currentLayerUpperBounds, sizeof(double) * _maxLayerSize * _inputLayerSize );
//         memcpy( _previousLayerLowerBias, _currentLayerLowerBias, sizeof(double) * _maxLayerSize );
//         memcpy( _previousLayerUpperBias, _currentLayerUpperBias, sizeof(double) * _maxLayerSize );
//     }
// }

// void NetworkLevelReasoner::reluSymbolicPropagation( const NeuronIndex &index, double &lbLb, double &lbUb, double &ubLb, double &ubUb )
// {
//     /*
//       There are two ways we can determine that a ReLU has become fixed:

//       1. If the ReLU's variables have been externally fixed
//       2. lbLb >= 0 (ACTIVE) or ubUb <= 0 (INACTIVE)
//     */
//     unsigned currentLayer = index._layer;
//     unsigned i = index._neuron;
//     unsigned currentLayerSize = _layerSizes[currentLayer];

//     ReluConstraint::PhaseStatus reluPhase = ReluConstraint::PHASE_NOT_FIXED;
//     if ( _eliminatedWeightedSumVariables.exists( index ) )
//     {
//         if ( _eliminatedWeightedSumVariables[index] <= 0 )
//             reluPhase = ReluConstraint::PHASE_INACTIVE;
//         else
//             reluPhase = ReluConstraint::PHASE_ACTIVE;
//     }
//     else if ( _eliminatedActivationResultVariables.exists( index ) )
//     {
//         if ( FloatUtils::isZero( _eliminatedWeightedSumVariables[index] ) )
//             reluPhase = ReluConstraint::PHASE_INACTIVE;
//         else
//             reluPhase = ReluConstraint::PHASE_ACTIVE;
//     }
//     else if ( _lowerBoundsWeightedSums[currentLayer][i] >= 0 )
//         reluPhase = ReluConstraint::PHASE_ACTIVE;
//     else if ( _upperBoundsWeightedSums[currentLayer][i] <= 0 )
//         reluPhase = ReluConstraint::PHASE_INACTIVE;

//     /*
//       If the ReLU phase is not fixed yet, see whether the
//       newly computed bounds imply that it should be fixed.
//     */
//     if ( reluPhase == ReluConstraint::PHASE_NOT_FIXED )
//     {
//         // If we got here, we know that lbLb < 0 and ubUb
//         // > 0 There are four possible cases, depending on
//         // whether ubLb and lbUb are negative or positive
//         // (see Neurify paper, page 14).

//         // Upper bound
//         if ( ubLb <= 0 )
//         {
//             // ubLb < 0 < ubUb
//             // Concretize the upper bound using the Ehler's-like approximation
//             for ( unsigned j = 0; j < _inputLayerSize; ++j )
//                 _currentLayerUpperBounds[j * currentLayerSize + i] =
//                     _currentLayerUpperBounds[j * currentLayerSize + i] * ubUb / ( ubUb - ubLb );

//             // Do the same for the bias, and then adjust
//             _currentLayerUpperBias[i] = _currentLayerUpperBias[i] * ubUb / ( ubUb - ubLb );
//             _currentLayerUpperBias[i] -= ubLb * ubUb / ( ubUb - ubLb );
//         }

//         // Lower bound
//         if ( lbUb <= 0 )
//         {
//             for ( unsigned j = 0; j < _inputLayerSize; ++j )
//                 _currentLayerLowerBounds[j * currentLayerSize + i] = 0;

//             _currentLayerLowerBias[i] = 0;
//         }
//         else
//         {
//             for ( unsigned j = 0; j < _inputLayerSize; ++j )
//                 _currentLayerLowerBounds[j * currentLayerSize + i] =
//                     _currentLayerLowerBounds[j * currentLayerSize + i] * lbUb / ( lbUb - lbLb );

//             _currentLayerLowerBias[i] = _currentLayerLowerBias[i] * lbUb / ( lbUb - lbLb );
//         }

//         lbLb = 0;
//     }
//     else
//     {
//         // The phase of this ReLU is fixed!
//         if ( reluPhase == ReluConstraint::PHASE_ACTIVE )
//         {
//             // Active ReLU, bounds are propagated as is
//         }
//         else
//         {
//             // Inactive ReLU, returns zero
//             lbLb = 0;
//             lbUb = 0;
//             ubLb = 0;
//             ubUb = 0;

//             for ( unsigned j = 0; j < _inputLayerSize; ++j )
//             {
//                 _currentLayerLowerBounds[j * currentLayerSize + i] = 0;
//                 _currentLayerUpperBounds[j * currentLayerSize + i] = 0;
//             }
//             _currentLayerLowerBias[i] = 0;
//             _currentLayerUpperBias[i] = 0;
//         }
//     }

//     if ( lbLb < 0 )
//         lbLb = 0;
// }

// void NetworkLevelReasoner::absoluteValueSymbolicPropagation( const NeuronIndex &index, double &lbLb, double &lbUb, double &ubLb, double &ubUb )
// {
//     /*
//       There are two ways we can determine that an AbsoluteValue has become fixed:

//       1. If the weighted sum variable has been externally fixed
//       2. If the weighted sum bounds are lb >= 0 (POSITIVE) or ub <= 0 (NEGATIVE)
//     */
//     unsigned currentLayer = index._layer;
//     unsigned i = index._neuron;
//     unsigned currentLayerSize = _layerSizes[currentLayer];

//     AbsoluteValueConstraint::PhaseStatus absPhase = AbsoluteValueConstraint::PHASE_NOT_FIXED;
//     if ( _eliminatedWeightedSumVariables.exists( index ) )
//     {
//         if ( _eliminatedWeightedSumVariables[index] <= 0 )
//             absPhase = AbsoluteValueConstraint::PHASE_NEGATIVE;
//         else
//             absPhase = AbsoluteValueConstraint::PHASE_POSITIVE;
//     }
//     else if ( _lowerBoundsWeightedSums[currentLayer][i] >= 0 )
//         absPhase = AbsoluteValueConstraint::PHASE_POSITIVE;
//     else if ( _upperBoundsWeightedSums[currentLayer][i] <= 0 )
//         absPhase = AbsoluteValueConstraint::PHASE_NEGATIVE;

//     /*
//       If the phase is not fixed yet, see whether the
//       newly computed bounds imply that it should be fixed.
//     */
//     if ( absPhase == AbsoluteValueConstraint::PHASE_NOT_FIXED )
//     {
//         // If we got here, we know that lbLb < 0 < lbUb In this case,
//         // we do naive concretization: lb is 0, ub is the max between
//         // -lbLb and ubUb.
//         for ( unsigned j = 0; j < _inputLayerSize; ++j )
//             _currentLayerUpperBounds[j * currentLayerSize + i] = 0;

//         for ( unsigned j = 0; j < _inputLayerSize; ++j )
//             _currentLayerLowerBounds[j * currentLayerSize + i] = 0;

//         if ( -_currentLayerLowerBias[i] > _currentLayerUpperBias[i] )
//             _currentLayerUpperBias[i] = -_currentLayerLowerBias[i];

//         if ( -lbLb > ubUb )
//             ubUb = -lbLb;
//         lbLb = 0;

//         _currentLayerLowerBias[i] = lbLb;
//         _currentLayerUpperBias[i] = ubUb;
//     }
//     else
//     {
//         // The phase of this AbsoluteValueConstraint is fixed!
//         if ( absPhase == AbsoluteValueConstraint::PHASE_POSITIVE )
//         {
//             // Positive AbsoluteValue, bounds are propagated as is
//         }
//         else
//         {
//             // Negative AbsoluteValue, bounds are negated and flipped
//             double temp;
//             for ( unsigned j = 0; j < _inputLayerSize; ++j )
//             {
//                 temp = _currentLayerUpperBounds[j * currentLayerSize + i];
//                 _currentLayerUpperBounds[j * currentLayerSize + i] =
//                     -_currentLayerLowerBounds[j * currentLayerSize + i];
//                 _currentLayerLowerBounds[j * currentLayerSize + i] = -temp;
//             }

//             temp = _currentLayerLowerBias[i];
//             _currentLayerLowerBias[i] = -_currentLayerUpperBias[i];
//             _currentLayerUpperBias[i] = -temp;

//             // Old lb, negated, is the new ub
//             temp = lbLb;
//             lbLb = -ubUb;
//             ubUb = -temp;

//             temp = lbUb;
//             lbUb = -ubLb;
//             ubLb = -temp;

//             // In extreme cases (constraint set externally), lbLb
//             // could be negative - so adjust this
//         }
//     }

//     if ( lbLb < 0 )
//         lbLb = 0;
// }

// void NetworkLevelReasoner::getConstraintTightenings( List<Tightening> &tightenings ) const
// {
//     tightenings.clear();

//     for ( unsigned i = 1; i < _numberOfLayers; ++i )
//     {
//         for ( unsigned j = 0; j < _layerSizes[i]; ++j )
//         {
//             NeuronIndex index( i, j );

//             // Weighted sums
//             if ( _indexToWeightedSumVariable.exists( index ) )
//             {
//                 tightenings.append( Tightening( _indexToWeightedSumVariable[index],
//                                                 _lowerBoundsWeightedSums[i][j],
//                                                 Tightening::LB ) );

//                 tightenings.append( Tightening( _indexToWeightedSumVariable[index],
//                                                 _upperBoundsWeightedSums[i][j],
//                                                 Tightening::UB ) );
//             }

//             // Activation results
//             if ( i != _numberOfLayers - 1 )
//             {
//                 if ( _indexToActivationResultVariable.exists( index ) )
//                 {
//                     tightenings.append( Tightening( _indexToActivationResultVariable[index],
//                                                     _lowerBoundsActivations[i][j],
//                                                     Tightening::LB ) );
//                     tightenings.append( Tightening( _indexToActivationResultVariable[index],
//                                                     _upperBoundsActivations[i][j],
//                                                     Tightening::UB ) );
//                 }
//             }
//         }
//     }
// }

void NetworkLevelReasoner::eliminateVariable( unsigned variable, double value )
{
    for ( auto &layer : _layerIndexToLayer )
        layer.second->eliminateVariable( variable, value );
}

void NetworkLevelReasoner::log( const String &message )
{
    if ( GlobalConfiguration::NETWORK_LEVEL_REASONER_LOGGING )
        printf( "%s", message.ascii() );
}


void NetworkLevelReasoner::dumpTopology() const
{
    printf( "Number of layers: %u. Sizes:\n", _layerIndexToLayer.size() );
    for ( unsigned i = 0; i < _layerIndexToLayer.size(); ++i )
        printf( "\tLayer %u: %u\n", i, _layerIndexToLayer[i]->getSize() );

    for ( const auto &layer : _layerIndexToLayer )
        layer.second->dump();
}

} // namespace NLR
