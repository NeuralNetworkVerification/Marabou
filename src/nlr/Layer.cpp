/*********************                                                        */
/*! \file Layer.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz, Ido Shmuel
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2024 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

 **/

#include "Layer.h"

#include "Options.h"
#include "Query.h"
#include "SoftmaxConstraint.h"
#include "SymbolicBoundTighteningType.h"

namespace NLR {

Layer::~Layer()
{
    freeMemoryIfNeeded();
}

void Layer::setLayerOwner( LayerOwner *layerOwner )
{
    _layerOwner = layerOwner;
}

Layer::Layer( unsigned index, Type type, unsigned size, LayerOwner *layerOwner )
    : _layerIndex( index )
    , _type( type )
    , _size( size )
    , _layerOwner( layerOwner )
    , _bias( NULL )
    , _assignment( NULL )
    , _lb( NULL )
    , _ub( NULL )
    , _inputLayerSize( 0 )
    , _symbolicLb( NULL )
    , _symbolicUb( NULL )
    , _symbolicLowerBias( NULL )
    , _symbolicUpperBias( NULL )
    , _symbolicLbOfLb( NULL )
    , _symbolicUbOfLb( NULL )
    , _symbolicLbOfUb( NULL )
    , _symbolicUbOfUb( NULL )
{
    allocateMemory();
}

void Layer::allocateMemory()
{
    if ( _type == WEIGHTED_SUM )
    {
        _bias = new double[_size];
        std::fill_n( _bias, _size, 0 );
    }

    _lb = new double[_size];
    _ub = new double[_size];

    std::fill_n( _lb, _size, 0 );
    std::fill_n( _ub, _size, 0 );

    _assignment = new double[_size];

    _simulations.assign(
        _size, Vector<double>( Options::get()->getInt( Options::NUMBER_OF_SIMULATIONS ) ) );

    _inputLayerSize = ( _type == INPUT ) ? _size : _layerOwner->getLayer( 0 )->getSize();
    if ( Options::get()->getSymbolicBoundTighteningType() ==
         SymbolicBoundTighteningType::SYMBOLIC_BOUND_TIGHTENING )
    {
        _symbolicLb = new double[_size * _inputLayerSize];
        _symbolicUb = new double[_size * _inputLayerSize];

        std::fill_n( _symbolicLb, _size * _inputLayerSize, 0 );
        std::fill_n( _symbolicUb, _size * _inputLayerSize, 0 );

        _symbolicLowerBias = new double[_size];
        _symbolicUpperBias = new double[_size];

        std::fill_n( _symbolicLowerBias, _size, 0 );
        std::fill_n( _symbolicUpperBias, _size, 0 );

        _symbolicLbOfLb = new double[_size];
        _symbolicUbOfLb = new double[_size];
        _symbolicLbOfUb = new double[_size];
        _symbolicUbOfUb = new double[_size];

        std::fill_n( _symbolicLbOfLb, _size, 0 );
        std::fill_n( _symbolicUbOfLb, _size, 0 );
        std::fill_n( _symbolicLbOfUb, _size, 0 );
        std::fill_n( _symbolicUbOfUb, _size, 0 );
    }
}

void Layer::setAssignment( const double *values )
{
    ASSERT( _eliminatedNeurons.empty() );
    memcpy( _assignment, values, _size * sizeof( double ) );
}

const double *Layer::getAssignment() const
{
    return _assignment;
}

double Layer::getAssignment( unsigned neuron ) const
{
    return _assignment[neuron];
}

void Layer::setSimulations( const Vector<Vector<double>> *values )
{
    _simulations = *values;
}

const Vector<Vector<double>> *Layer::getSimulations() const
{
    return &_simulations;
}

void Layer::computeAssignment()
{
    ASSERT( _type != INPUT );

    if ( _type == WEIGHTED_SUM )
    {
        // Initialize to bias
        memcpy( _assignment, _bias, sizeof( double ) * _size );

        // Process each of the source layers
        for ( auto &sourceLayerEntry : _sourceLayers )
        {
            const Layer *sourceLayer = _layerOwner->getLayer( sourceLayerEntry.first );
            const double *sourceAssignment = sourceLayer->getAssignment();
            unsigned sourceSize = sourceLayerEntry.second;
            const double *weights = _layerToWeights[sourceLayerEntry.first];

            for ( unsigned i = 0; i < sourceSize; ++i )
                for ( unsigned j = 0; j < _size; ++j )
                    _assignment[j] += ( sourceAssignment[i] * weights[i * _size + j] );
        }
    }

    else if ( _type == RELU )
    {
        for ( unsigned i = 0; i < _size; ++i )
        {
            NeuronIndex sourceIndex = *_neuronToActivationSources[i].begin();
            double inputValue =
                _layerOwner->getLayer( sourceIndex._layer )->getAssignment( sourceIndex._neuron );

            _assignment[i] = FloatUtils::max( inputValue, 0 );
        }
    }

    else if ( _type == ROUND )
    {
        for ( unsigned i = 0; i < _size; ++i )
        {
            NeuronIndex sourceIndex = *_neuronToActivationSources[i].begin();
            double inputValue =
                _layerOwner->getLayer( sourceIndex._layer )->getAssignment( sourceIndex._neuron );

            _assignment[i] = FloatUtils::round( inputValue );
        }
    }

    else if ( _type == LEAKY_RELU )
    {
        for ( unsigned i = 0; i < _size; ++i )
        {
            NeuronIndex sourceIndex = *_neuronToActivationSources[i].begin();
            double inputValue =
                _layerOwner->getLayer( sourceIndex._layer )->getAssignment( sourceIndex._neuron );
            ASSERT( _alpha > 0 && _alpha < 1 );
            _assignment[i] = FloatUtils::max( inputValue, _alpha * inputValue );
        }
    }

    else if ( _type == ABSOLUTE_VALUE )
    {
        for ( unsigned i = 0; i < _size; ++i )
        {
            NeuronIndex sourceIndex = *_neuronToActivationSources[i].begin();
            double inputValue =
                _layerOwner->getLayer( sourceIndex._layer )->getAssignment( sourceIndex._neuron );

            _assignment[i] = FloatUtils::abs( inputValue );
        }
    }

    else if ( _type == MAX )
    {
        for ( unsigned i = 0; i < _size; ++i )
        {
            _assignment[i] = FloatUtils::negativeInfinity();

            for ( const auto &input : _neuronToActivationSources[i] )
            {
                double value =
                    _layerOwner->getLayer( input._layer )->getAssignment( input._neuron );
                if ( value > _assignment[i] )
                    _assignment[i] = value;
            }
        }
    }

    else if ( _type == SIGN )
    {
        for ( unsigned i = 0; i < _size; ++i )
        {
            NeuronIndex sourceIndex = *_neuronToActivationSources[i].begin();
            double inputValue =
                _layerOwner->getLayer( sourceIndex._layer )->getAssignment( sourceIndex._neuron );

            _assignment[i] = FloatUtils::isNegative( inputValue ) ? -1 : 1;
        }
    }

    else if ( _type == SIGMOID )
    {
        for ( unsigned i = 0; i < _size; ++i )
        {
            NeuronIndex sourceIndex = *_neuronToActivationSources[i].begin();
            double inputValue =
                _layerOwner->getLayer( sourceIndex._layer )->getAssignment( sourceIndex._neuron );

            _assignment[i] = 1 / ( 1 + std::exp( -inputValue ) );
        }
    }

    else if ( _type == SOFTMAX )
    {
        for ( unsigned i = 0; i < _size; ++i )
        {
            _assignment[i] = FloatUtils::negativeInfinity();

            Vector<double> inputs;
            Vector<double> outputs;
            unsigned outputIndex = 0;
            unsigned index = 0;
            for ( const auto &input : _neuronToActivationSources[i] )
            {
                if ( input._neuron == i )
                    outputIndex = index;
                double value =
                    _layerOwner->getLayer( input._layer )->getAssignment( input._neuron );
                inputs.append( value );
                ++index;
            }

            SoftmaxConstraint::softmax( inputs, outputs );
            _assignment[i] = outputs[outputIndex];
        }
    }

    else if ( _type == BILINEAR )
    {
        for ( unsigned i = 0; i < _size; ++i )
        {
            _assignment[i] = 1;
            for ( const auto &input : _neuronToActivationSources[i] )
            {
                double value =
                    _layerOwner->getLayer( input._layer )->getAssignment( input._neuron );
                _assignment[i] *= value;
            }
        }
    }
    else
    {
        printf( "Error! Neuron type %u unsupported\n", _type );
        throw MarabouError( MarabouError::NETWORK_LEVEL_REASONER_ACTIVATION_NOT_SUPPORTED );
    }

    // Eliminated variables supersede anything else - no matter what
    // was computed due to left-over weights, etc, their set values
    // prevail.
    for ( const auto &eliminated : _eliminatedNeurons )
        _assignment[eliminated.first] = eliminated.second;
}

void Layer::computeSimulations()
{
    ASSERT( _type != INPUT );

    unsigned simulationSize = Options::get()->getInt( Options::NUMBER_OF_SIMULATIONS );
    if ( _type == WEIGHTED_SUM )
    {
        // Process each of the source layers
        for ( auto &sourceLayerEntry : _sourceLayers )
        {
            const Layer *sourceLayer = _layerOwner->getLayer( sourceLayerEntry.first );
            const Vector<Vector<double>> *sourceSimulations = sourceLayer->getSimulations();

            unsigned sourceSize = sourceLayerEntry.second;
            const double *weights = _layerToWeights[sourceLayerEntry.first];

            for ( unsigned i = 0; i < _size; ++i )
            {
                for ( unsigned j = 0; j < simulationSize; ++j )
                    _simulations[i][j] = _bias[i];
            }

            for ( unsigned i = 0; i < sourceSize; ++i )
                for ( unsigned j = 0; j < simulationSize; ++j )
                    for ( unsigned k = 0; k < _size; ++k )
                        _simulations[k][j] +=
                            ( ( *sourceSimulations ).get( i ).get( j ) * weights[i * _size + k] );
        }
    }
    else if ( _type == RELU )
    {
        for ( unsigned i = 0; i < _size; ++i )
        {
            NeuronIndex sourceIndex = *_neuronToActivationSources[i].begin();
            const Vector<double> &simulations =
                ( *( _layerOwner->getLayer( sourceIndex._layer )->getSimulations() ) )
                    .get( sourceIndex._neuron );
            for ( unsigned j = 0; j < simulationSize; ++j )
                _simulations[i][j] = FloatUtils::max( simulations.get( j ), 0 );
        }
    }
    else if ( _type == LEAKY_RELU )
    {
        for ( unsigned i = 0; i < _size; ++i )
        {
            NeuronIndex sourceIndex = *_neuronToActivationSources[i].begin();
            const Vector<double> &simulations =
                ( *( _layerOwner->getLayer( sourceIndex._layer )->getSimulations() ) )
                    .get( sourceIndex._neuron );
            ASSERT( _alpha > 0 && _alpha < 1 );
            for ( unsigned j = 0; j < simulationSize; ++j )
                _simulations[i][j] =
                    FloatUtils::max( simulations.get( j ), _alpha * simulations.get( j ) );
        }
    }
    else if ( _type == ABSOLUTE_VALUE )
    {
        for ( unsigned i = 0; i < _size; ++i )
        {
            NeuronIndex sourceIndex = *_neuronToActivationSources[i].begin();
            const Vector<double> &simulations =
                ( *( _layerOwner->getLayer( sourceIndex._layer )->getSimulations() ) )
                    .get( sourceIndex._neuron );
            for ( unsigned j = 0; j < simulationSize; ++j )
                _simulations[i][j] = FloatUtils::abs( simulations.get( j ) );
        }
    }
    else if ( _type == MAX )
    {
        for ( unsigned i = 0; i < _size; ++i )
        {
            for ( unsigned j = 0; j < simulationSize; ++j )
            {
                _simulations[i][j] = FloatUtils::negativeInfinity();

                for ( const auto &input : _neuronToActivationSources[i] )
                {
                    // double value = ( *( _layerOwner->getLayer( input._layer )->getSimulations() )
                    // )[input._neuron][j];
                    double value = ( *( _layerOwner->getLayer( input._layer )->getSimulations() ) )
                                       .get( input._neuron )
                                       .get( j );
                    if ( value > _simulations[i][j] )
                        _simulations[i][j] = value;
                }
            }
        }
    }
    else if ( _type == SIGN )
    {
        for ( unsigned i = 0; i < _size; ++i )
        {
            NeuronIndex sourceIndex = *_neuronToActivationSources[i].begin();
            const Vector<double> &simulations =
                ( *( _layerOwner->getLayer( sourceIndex._layer )->getSimulations() ) ).get( i );
            for ( unsigned j = 0; j < simulationSize; ++j )
                _simulations[i][j] = FloatUtils::isNegative( simulations.get( j ) ) ? -1 : 1;
        }
    }
    else if ( _type == SIGMOID )
    {
        for ( unsigned i = 0; i < _size; ++i )
        {
            NeuronIndex sourceIndex = *_neuronToActivationSources[i].begin();
            const Vector<double> &simulations =
                ( *( _layerOwner->getLayer( sourceIndex._layer )->getSimulations() ) ).get( i );
            for ( unsigned j = 0; j < simulationSize; ++j )
                _simulations[i][j] = 1 / ( 1 + std::exp( -simulations.get( j ) ) );
        }
    }
    else if ( _type == ROUND )
    {
        for ( unsigned i = 0; i < _size; ++i )
        {
            NeuronIndex sourceIndex = *_neuronToActivationSources[i].begin();
            const Vector<double> &simulations =
                ( *( _layerOwner->getLayer( sourceIndex._layer )->getSimulations() ) ).get( i );
            for ( unsigned j = 0; j < simulationSize; ++j )
                _simulations[i][j] = FloatUtils::round( simulations.get( j ) );
        }
    }
    else if ( _type == SOFTMAX )
    {
        for ( unsigned i = 0; i < _size; ++i )
        {
            for ( unsigned j = 0; j < simulationSize; ++j )
            {
                _simulations[i][j] = FloatUtils::negativeInfinity();

                Vector<double> inputs;
                Vector<double> outputs;
                unsigned outputIndex = 0;
                unsigned index = 0;

                for ( const auto &input : _neuronToActivationSources[i] )
                {
                    if ( input._neuron == i )
                        outputIndex = index;
                    double value = ( *( _layerOwner->getLayer( input._layer )->getSimulations() ) )
                                       .get( input._neuron )
                                       .get( j );
                    inputs.append( value );
                    ++index;
                }

                SoftmaxConstraint::softmax( inputs, outputs );
                _simulations[i][j] = outputs[outputIndex];
            }
        }
    }
    else if ( _type == BILINEAR )
    {
        for ( unsigned i = 0; i < _size; ++i )
        {
            for ( unsigned j = 0; j < simulationSize; ++j )
            {
                _simulations[i][j] = 1;
                for ( const auto &input : _neuronToActivationSources[i] )
                {
                    double value = ( *( _layerOwner->getLayer( input._layer )->getSimulations() ) )
                                       .get( input._neuron )
                                       .get( j );
                    _simulations[i][j] *= value;
                }
            }
        }
    }
    else
    {
        printf( "Error! Neuron type %u unsupported\n", _type );
        throw MarabouError( MarabouError::NETWORK_LEVEL_REASONER_ACTIVATION_NOT_SUPPORTED );
    }

    // Eliminated variables supersede anything else - no matter what
    // was computed due to left-over weights, etc, their set values
    // prevail.
    for ( const auto &eliminated : _eliminatedNeurons )
    {
        for ( unsigned j = 0; j < simulationSize; ++j )
            _simulations[eliminated.first][j] = eliminated.second;
    }
}

void Layer::addSourceLayer( unsigned layerNumber, unsigned layerSize )
{
    ASSERT( _type != INPUT );

    if ( _sourceLayers.exists( layerNumber ) )
        return;

    _sourceLayers[layerNumber] = layerSize;

    if ( _type == WEIGHTED_SUM )
    {
        _layerToWeights[layerNumber] = new double[layerSize * _size];
        _layerToPositiveWeights[layerNumber] = new double[layerSize * _size];
        _layerToNegativeWeights[layerNumber] = new double[layerSize * _size];

        std::fill_n( _layerToWeights[layerNumber], layerSize * _size, 0 );
        std::fill_n( _layerToPositiveWeights[layerNumber], layerSize * _size, 0 );
        std::fill_n( _layerToNegativeWeights[layerNumber], layerSize * _size, 0 );
    }
}

const Map<unsigned, unsigned> &Layer::getSourceLayers() const
{
    return _sourceLayers;
}

void Layer::addSuccessorLayer( unsigned layerNumber )
{
    _successorLayers.insert( layerNumber );
}

const Set<unsigned> &Layer::getSuccessorLayers() const
{
    return _successorLayers;
}

const double *Layer::getWeightMatrix( unsigned sourceLayer ) const
{
    ASSERT( _layerToWeights.exists( sourceLayer ) );
    return _layerToWeights[sourceLayer];
}

void Layer::removeSourceLayer( unsigned sourceLayer )
{
    ASSERT( _sourceLayers.exists( sourceLayer ) );

    delete[] _layerToWeights[sourceLayer];
    delete[] _layerToPositiveWeights[sourceLayer];
    delete[] _layerToNegativeWeights[sourceLayer];

    _sourceLayers.erase( sourceLayer );
    _layerToWeights.erase( sourceLayer );
    _layerToPositiveWeights.erase( sourceLayer );
    _layerToNegativeWeights.erase( sourceLayer );
}

void Layer::setWeight( unsigned sourceLayer,
                       unsigned sourceNeuron,
                       unsigned targetNeuron,
                       double weight )
{
    unsigned index = sourceNeuron * _size + targetNeuron;
    _layerToWeights[sourceLayer][index] = weight;

    if ( weight > 0 )
    {
        _layerToPositiveWeights[sourceLayer][index] = weight;
        _layerToNegativeWeights[sourceLayer][index] = 0;
    }
    else
    {
        _layerToPositiveWeights[sourceLayer][index] = 0;
        _layerToNegativeWeights[sourceLayer][index] = weight;
    }
}

double Layer::getWeight( unsigned sourceLayer, unsigned sourceNeuron, unsigned targetNeuron ) const
{
    unsigned index = sourceNeuron * _size + targetNeuron;
    return _layerToWeights[sourceLayer][index];
}

double *Layer::getWeights( unsigned sourceLayerIndex ) const
{
    return _layerToWeights[sourceLayerIndex];
}

double *Layer::getPositiveWeights( unsigned sourceLayerIndex ) const
{
    return _layerToPositiveWeights[sourceLayerIndex];
}

double *Layer::getNegativeWeights( unsigned sourceLayerIndex ) const
{
    return _layerToNegativeWeights[sourceLayerIndex];
}

void Layer::setBias( unsigned neuron, double bias )
{
    _bias[neuron] = bias;
}

double Layer::getBias( unsigned neuron ) const
{
    return _bias[neuron];
}

double *Layer::getBiases() const
{
    return _bias;
}

void Layer::addActivationSource( unsigned sourceLayer,
                                 unsigned sourceNeuron,
                                 unsigned targetNeuron )
{
    ASSERT( _type == RELU || _type == LEAKY_RELU || _type == ABSOLUTE_VALUE || _type == MAX ||
            _type == ROUND || _type == SIGN || _type == SIGMOID || _type == SOFTMAX ||
            _type == BILINEAR );

    if ( !_neuronToActivationSources.exists( targetNeuron ) )
        _neuronToActivationSources[targetNeuron] = List<NeuronIndex>();

    _neuronToActivationSources[targetNeuron].append( NeuronIndex( sourceLayer, sourceNeuron ) );

    DEBUG( {
        if ( _type == RELU || _type == LEAKY_RELU || _type == ABSOLUTE_VALUE || _type == SIGN ||
             _type == ROUND )
            ASSERT( _neuronToActivationSources[targetNeuron].size() == 1 );
    } );
}

List<NeuronIndex> Layer::getActivationSources( unsigned neuron ) const
{
    return _neuronToActivationSources[neuron];
}

unsigned Layer::getSize() const
{
    return _size;
}

Layer::Type Layer::getLayerType() const
{
    return _type;
}

void Layer::setNeuronVariable( unsigned neuron, unsigned variable )
{
    ASSERT( !_eliminatedNeurons.exists( neuron ) );

    _neuronToVariable[neuron] = variable;
    _variableToNeuron[variable] = neuron;
}

void Layer::obtainCurrentBounds( const Query &inputQuery )
{
    for ( unsigned i = 0; i < _size; ++i )
    {
        if ( _neuronToVariable.exists( i ) )
        {
            unsigned variable = _neuronToVariable[i];
            _lb[i] = inputQuery.getLowerBound( variable );
            _ub[i] = inputQuery.getUpperBound( variable );
        }
        else
        {
            ASSERT( _eliminatedNeurons.exists( i ) );
            _lb[i] = _eliminatedNeurons[i];
            _ub[i] = _eliminatedNeurons[i];
        }
    }
}

void Layer::obtainCurrentBounds()
{
    for ( unsigned i = 0; i < _size; ++i )
    {
        if ( _neuronToVariable.exists( i ) )
        {
            unsigned variable = _neuronToVariable[i];
            _lb[i] = _layerOwner->getTableau()->getLowerBound( variable );
            _ub[i] = _layerOwner->getTableau()->getUpperBound( variable );
        }
        else
        {
            ASSERT( _eliminatedNeurons.exists( i ) );
            _lb[i] = _eliminatedNeurons[i];
            _ub[i] = _eliminatedNeurons[i];
        }
    }
}

double Layer::getLb( unsigned neuron ) const
{
    if ( _eliminatedNeurons.exists( neuron ) )
        return _eliminatedNeurons[neuron];

    return _lb[neuron];
}

double Layer::getUb( unsigned neuron ) const
{
    if ( _eliminatedNeurons.exists( neuron ) )
        return _eliminatedNeurons[neuron];

    return _ub[neuron];
}

double *Layer::getLbs() const
{
    return _lb;
}

double *Layer::getUbs() const
{
    return _ub;
}

void Layer::setLb( unsigned neuron, double bound )
{
    ASSERT( !_eliminatedNeurons.exists( neuron ) );
    _lb[neuron] = bound;
}

void Layer::setUb( unsigned neuron, double bound )
{
    ASSERT( !_eliminatedNeurons.exists( neuron ) );
    _ub[neuron] = bound;
}

void Layer::computeIntervalArithmeticBounds()
{
    ASSERT( _type != INPUT );

    switch ( _type )
    {
    case WEIGHTED_SUM:
        computeIntervalArithmeticBoundsForWeightedSum();
        break;

    case RELU:
        computeIntervalArithmeticBoundsForRelu();
        break;

    case ABSOLUTE_VALUE:
        computeIntervalArithmeticBoundsForAbs();
        break;

    case SIGN:
        computeIntervalArithmeticBoundsForSign();
        break;

    case ROUND:
        computeIntervalArithmeticBoundsForRound();
        break;

    case LEAKY_RELU:
        computeIntervalArithmeticBoundsForLeakyRelu();
        break;

    case SIGMOID:
        computeIntervalArithmeticBoundsForSigmoid();
        break;

    case MAX:
        computeIntervalArithmeticBoundsForMax();
        break;

    case SOFTMAX:
        computeIntervalArithmeticBoundsForSoftmax();
        break;

    case BILINEAR:
        computeIntervalArithmeticBoundsForBilinear();
        break;

    default:
        printf( "Error! Activation type %u unsupported\n", _type );
        throw MarabouError( MarabouError::NETWORK_LEVEL_REASONER_ACTIVATION_NOT_SUPPORTED );
        break;
    }
}

void Layer::computeIntervalArithmeticBoundsForWeightedSum()
{
    double *newLb = new double[_size];
    double *newUb = new double[_size];

    for ( unsigned i = 0; i < _size; ++i )
    {
        newLb[i] = _bias[i];
        newUb[i] = _bias[i];
    }

    for ( const auto &sourceLayerEntry : _sourceLayers )
    {
        unsigned sourceLayerIndex = sourceLayerEntry.first;
        unsigned sourceLayerSize = sourceLayerEntry.second;
        const Layer *sourceLayer = _layerOwner->getLayer( sourceLayerIndex );
        const double *weights = _layerToWeights[sourceLayerIndex];

        for ( unsigned i = 0; i < _size; ++i )
        {
            for ( unsigned j = 0; j < sourceLayerSize; ++j )
            {
                double previousLb = sourceLayer->getLb( j );
                double previousUb = sourceLayer->getUb( j );
                double weight = weights[j * _size + i];

                if ( weight > 0 )
                {
                    newLb[i] += weight * previousLb;
                    newUb[i] += weight * previousUb;
                }
                else
                {
                    newLb[i] += weight * previousUb;
                    newUb[i] += weight * previousLb;
                }
            }
        }
    }

    for ( unsigned i = 0; i < _size; ++i )
    {
        if ( _eliminatedNeurons.exists( i ) )
            continue;

        if ( _lb[i] < newLb[i] )
        {
            _lb[i] = newLb[i];
            _layerOwner->receiveTighterBound(
                Tightening( _neuronToVariable[i], _lb[i], Tightening::LB ) );
        }
        if ( _ub[i] > newUb[i] )
        {
            _ub[i] = newUb[i];
            _layerOwner->receiveTighterBound(
                Tightening( _neuronToVariable[i], _ub[i], Tightening::UB ) );
        }
    }

    delete[] newLb;
    delete[] newUb;
}

void Layer::computeIntervalArithmeticBoundsForRelu()
{
    for ( unsigned i = 0; i < _size; ++i )
    {
        if ( _eliminatedNeurons.exists( i ) )
            continue;

        NeuronIndex sourceIndex = *_neuronToActivationSources[i].begin();
        const Layer *sourceLayer = _layerOwner->getLayer( sourceIndex._layer );

        double lb = sourceLayer->getLb( sourceIndex._neuron );
        double ub = sourceLayer->getUb( sourceIndex._neuron );

        if ( lb < 0 )
            lb = 0;

        if ( _lb[i] < lb )
        {
            _lb[i] = lb;
            _layerOwner->receiveTighterBound(
                Tightening( _neuronToVariable[i], _lb[i], Tightening::LB ) );
        }
        if ( _ub[i] > ub )
        {
            _ub[i] = ub;
            _layerOwner->receiveTighterBound(
                Tightening( _neuronToVariable[i], _ub[i], Tightening::UB ) );
        }
    }
}

void Layer::computeIntervalArithmeticBoundsForAbs()
{
    for ( unsigned i = 0; i < _size; ++i )
    {
        if ( _eliminatedNeurons.exists( i ) )
            continue;

        NeuronIndex sourceIndex = *_neuronToActivationSources[i].begin();
        const Layer *sourceLayer = _layerOwner->getLayer( sourceIndex._layer );

        double lb = sourceLayer->getLb( sourceIndex._neuron );
        double ub = sourceLayer->getUb( sourceIndex._neuron );

        if ( lb > 0 )
        {
            if ( _lb[i] < lb )
            {
                _lb[i] = lb;
                _layerOwner->receiveTighterBound(
                    Tightening( _neuronToVariable[i], _lb[i], Tightening::LB ) );
            }
            if ( _ub[i] > ub )
            {
                _ub[i] = ub;
                _layerOwner->receiveTighterBound(
                    Tightening( _neuronToVariable[i], _ub[i], Tightening::UB ) );
            }
        }
        else if ( ub < 0 )
        {
            if ( _lb[i] < -ub )
            {
                _lb[i] = -ub;
                _layerOwner->receiveTighterBound(
                    Tightening( _neuronToVariable[i], _lb[i], Tightening::LB ) );
            }
            if ( _ub[i] > -lb )
            {
                _ub[i] = -lb;
                _layerOwner->receiveTighterBound(
                    Tightening( _neuronToVariable[i], _ub[i], Tightening::UB ) );
            }
        }
        else
        {
            // lb < 0 < ub
            if ( _lb[i] < 0 )
            {
                _lb[i] = 0;
                _layerOwner->receiveTighterBound(
                    Tightening( _neuronToVariable[i], _lb[i], Tightening::LB ) );
            }

            if ( _ub[i] > FloatUtils::max( ub, -lb ) )
            {
                _ub[i] = FloatUtils::max( ub, -lb );
                _layerOwner->receiveTighterBound(
                    Tightening( _neuronToVariable[i], _ub[i], Tightening::UB ) );
            }
        }
    }
}

void Layer::computeIntervalArithmeticBoundsForSign()
{
    for ( unsigned i = 0; i < _size; ++i )
    {
        if ( _eliminatedNeurons.exists( i ) )
            continue;

        NeuronIndex sourceIndex = *_neuronToActivationSources[i].begin();
        const Layer *sourceLayer = _layerOwner->getLayer( sourceIndex._layer );

        double lb = sourceLayer->getLb( sourceIndex._neuron );
        double ub = sourceLayer->getUb( sourceIndex._neuron );

        double new_lb;
        double new_ub;

        if ( !FloatUtils::isNegative( lb ) )
        {
            new_lb = 1;
            new_ub = 1;
        }
        else if ( FloatUtils::isNegative( ub ) )
        {
            new_lb = -1;
            new_ub = -1;
        }
        else
        {
            new_lb = -1;
            new_ub = 1;
        }

        /*
          We now have the tightest bounds we can for the relu
          variable. If they are tigheter than what was previously
          known, store them.
        */
        if ( _lb[i] < new_lb )
        {
            _lb[i] = new_lb;
            _layerOwner->receiveTighterBound(
                Tightening( _neuronToVariable[i], _lb[i], Tightening::LB ) );
        }

        if ( _ub[i] > new_ub )
        {
            _ub[i] = new_ub;
            _layerOwner->receiveTighterBound(
                Tightening( _neuronToVariable[i], _ub[i], Tightening::UB ) );
        }
    }
}

void Layer::computeIntervalArithmeticBoundsForLeakyRelu()
{
    for ( unsigned i = 0; i < _size; ++i )
    {
        if ( _eliminatedNeurons.exists( i ) )
            continue;

        NeuronIndex sourceIndex = *_neuronToActivationSources[i].begin();
        const Layer *sourceLayer = _layerOwner->getLayer( sourceIndex._layer );

        double lb = sourceLayer->getLb( sourceIndex._neuron );
        double ub = sourceLayer->getUb( sourceIndex._neuron );

        if ( lb > 0 )
        {
            if ( _lb[i] < lb )
            {
                _lb[i] = lb;
                _layerOwner->receiveTighterBound(
                    Tightening( _neuronToVariable[i], _lb[i], Tightening::LB ) );
            }
            if ( _ub[i] > ub )
            {
                _ub[i] = ub;
                _layerOwner->receiveTighterBound(
                    Tightening( _neuronToVariable[i], _ub[i], Tightening::UB ) );
            }
        }
        else if ( ub < 0 )
        {
            if ( _lb[i] < _alpha * lb )
            {
                _lb[i] = _alpha * lb;
                _layerOwner->receiveTighterBound(
                    Tightening( _neuronToVariable[i], _lb[i], Tightening::LB ) );
            }
            if ( _ub[i] > _alpha * ub )
            {
                _ub[i] = _alpha * ub;
                _layerOwner->receiveTighterBound(
                    Tightening( _neuronToVariable[i], _ub[i], Tightening::UB ) );
            }
        }
        else
        {
            // lb < 0 < ub
            if ( _lb[i] < _alpha * lb )
            {
                _lb[i] = _alpha * lb;
                _layerOwner->receiveTighterBound(
                    Tightening( _neuronToVariable[i], _lb[i], Tightening::LB ) );
            }

            if ( _ub[i] > ub )
            {
                _ub[i] = ub;
                _layerOwner->receiveTighterBound(
                    Tightening( _neuronToVariable[i], _ub[i], Tightening::UB ) );
            }
        }
    }
}

void Layer::computeIntervalArithmeticBoundsForSigmoid()
{
    for ( unsigned i = 0; i < _size; ++i )
    {
        if ( _eliminatedNeurons.exists( i ) )
            continue;

        NeuronIndex sourceIndex = *_neuronToActivationSources[i].begin();
        const Layer *sourceLayer = _layerOwner->getLayer( sourceIndex._layer );

        double lb = sourceLayer->getLb( sourceIndex._neuron );
        double ub = sourceLayer->getUb( sourceIndex._neuron );

        double lbSigmoid = SigmoidConstraint::sigmoid( lb );
        double ubSigmoid = SigmoidConstraint::sigmoid( ub );


        if ( _lb[i] < lbSigmoid )
        {
            _lb[i] = lbSigmoid;
            _layerOwner->receiveTighterBound(
                Tightening( _neuronToVariable[i], _lb[i], Tightening::LB ) );
        }
        if ( _ub[i] > ubSigmoid )
        {
            _ub[i] = ubSigmoid;
            _layerOwner->receiveTighterBound(
                Tightening( _neuronToVariable[i], _ub[i], Tightening::UB ) );
        }
    }
}


void Layer::computeIntervalArithmeticBoundsForRound()
{
    for ( unsigned i = 0; i < _size; ++i )
    {
        if ( _eliminatedNeurons.exists( i ) )
            continue;

        NeuronIndex sourceIndex = *_neuronToActivationSources[i].begin();
        const Layer *sourceLayer = _layerOwner->getLayer( sourceIndex._layer );

        double lb = sourceLayer->getLb( sourceIndex._neuron );
        double ub = sourceLayer->getUb( sourceIndex._neuron );

        double lbRound = FloatUtils::round( lb );
        double ubRound = FloatUtils::round( ub );


        if ( _lb[i] < lbRound )
        {
            _lb[i] = lbRound;
            _layerOwner->receiveTighterBound(
                Tightening( _neuronToVariable[i], _lb[i], Tightening::LB ) );
        }
        if ( _ub[i] > ubRound )
        {
            _ub[i] = ubRound;
            _layerOwner->receiveTighterBound(
                Tightening( _neuronToVariable[i], _ub[i], Tightening::UB ) );
        }
    }
}


void Layer::computeIntervalArithmeticBoundsForMax()
{
    for ( unsigned i = 0; i < _size; ++i )
    {
        if ( _eliminatedNeurons.exists( i ) )
            continue;

        ASSERT( _neuronToActivationSources.exists( i ) );
        List<NeuronIndex> sources = getActivationSources( i );
        const Layer *sourceLayer = _layerOwner->getLayer( sources.begin()->_layer );

        NeuronIndex indexOfMaxLowerBound = *( sources.begin() );
        double maxLowerBound = FloatUtils::negativeInfinity();
        double maxUpperBound = FloatUtils::negativeInfinity();

        Map<NeuronIndex, double> sourceLbs;
        Map<NeuronIndex, double> sourceUbs;
        unsigned counter = 0;
        for ( const auto &sourceIndex : sources )
        {
            unsigned sourceNeuron = sourceIndex._neuron;
            double sourceLb = sourceLayer->getLb( sourceNeuron );
            double sourceUb = sourceLayer->getUb( sourceNeuron );

            sourceLbs[sourceIndex] = sourceLb;
            sourceUbs[sourceIndex] = sourceUb;

            if ( maxLowerBound < sourceLb )
            {
                indexOfMaxLowerBound = sourceIndex;
                maxLowerBound = sourceLb;
            }
            if ( maxUpperBound < sourceUb )
            {
                maxUpperBound = sourceUb;
            }
            ++counter;
        }

        // The phase is fixed if the lower-bound of a source variable x_b is
        // larger than the upper-bounds of the other source variables.
        bool phaseFixed = true;
        for ( const auto &sourceIndex : sources )
        {
            if ( sourceIndex != indexOfMaxLowerBound &&
                 FloatUtils::gt( sourceUbs[sourceIndex], maxLowerBound ) )
            {
                phaseFixed = false;
                break;
            }
        }

        if ( phaseFixed )
        {
            // Phase fixed
            // Concrete bound: lb_b <= x_f <= ub_b
            if ( _lb[i] < maxLowerBound )
            {
                _lb[i] = maxLowerBound;
                _layerOwner->receiveTighterBound(
                    Tightening( _neuronToVariable[i], _lb[i], Tightening::LB ) );
            }

            if ( _ub[i] > sourceUbs[indexOfMaxLowerBound] )
            {
                _ub[i] = sourceUbs[indexOfMaxLowerBound];
                _layerOwner->receiveTighterBound(
                    Tightening( _neuronToVariable[i], _ub[i], Tightening::UB ) );
            }
        }
        else
        {
            // MaxPool not fixed
            // Concrete bounds: lb_b <= x_f <= maxUpperBound
            if ( _lb[i] < maxLowerBound )
            {
                _lb[i] = maxLowerBound;
                _layerOwner->receiveTighterBound(
                    Tightening( _neuronToVariable[i], _lb[i], Tightening::LB ) );
            }

            if ( _ub[i] > maxUpperBound )
            {
                _ub[i] = maxUpperBound;
                _layerOwner->receiveTighterBound(
                    Tightening( _neuronToVariable[i], _ub[i], Tightening::UB ) );
            }
        }
    }
}

void Layer::computeIntervalArithmeticBoundsForSoftmax()
{
    Set<unsigned> handledInputNeurons;
    for ( unsigned i = 0; i < _size; ++i )
    {
        if ( _eliminatedNeurons.exists( i ) )
            continue;

        ASSERT( _neuronToActivationSources.exists( i ) );
        List<NeuronIndex> sources = getActivationSources( i );
        const Layer *sourceLayer = _layerOwner->getLayer( sources.begin()->_layer );

        ASSERT( sourceLayer->getSize() == _size );

        Vector<double> sourceLbs;
        Vector<double> sourceUbs;
        for ( const auto &sourceIndex : sources )
        {
            unsigned sourceNeuron = sourceIndex._neuron;
            double sourceLb = sourceLayer->getLb( sourceNeuron );
            double sourceUb = sourceLayer->getUb( sourceNeuron );

            sourceLbs.append( sourceLb - GlobalConfiguration::DEFAULT_EPSILON_FOR_COMPARISONS );
            sourceUbs.append( sourceUb + GlobalConfiguration::DEFAULT_EPSILON_FOR_COMPARISONS );
        }

        // Find the index of i in the softmax
        unsigned index = 0;
        for ( const auto &sourceIndex : sources )
        {
            if ( handledInputNeurons.exists( sourceIndex._neuron ) )
                ++index;
            else
            {
                handledInputNeurons.insert( sourceIndex._neuron );
                break;
            }
        }

        double lb = softmaxLinearLowerBound( sourceLbs, sourceUbs, index );
        double ub = softmaxLinearUpperBound( sourceLbs, sourceUbs, index );
        if ( _lb[i] < lb )
        {
            _lb[i] = lb;
            _layerOwner->receiveTighterBound(
                Tightening( _neuronToVariable[i], _lb[i], Tightening::LB ) );
        }
        if ( _ub[i] > ub )
        {
            _ub[i] = ub;
            _layerOwner->receiveTighterBound(
                Tightening( _neuronToVariable[i], _ub[i], Tightening::UB ) );
        }
    }
}

void Layer::computeIntervalArithmeticBoundsForBilinear()
{
    for ( unsigned i = 0; i < _size; ++i )
    {
        if ( _eliminatedNeurons.exists( i ) )
            continue;

        ASSERT( _neuronToActivationSources.exists( i ) );
        List<NeuronIndex> sources = getActivationSources( i );
        ASSERT( sources.size() == 2 );

        const Layer *sourceLayer = _layerOwner->getLayer( sources.begin()->_layer );

        Vector<double> sourceLbs;
        Vector<double> sourceUbs;
        Vector<double> sourceValues;
        bool allConstant = true;
        for ( const auto &sourceIndex : sources )
        {
            unsigned sourceNeuron = sourceIndex._neuron;
            double sourceLb = sourceLayer->getLb( sourceNeuron );
            double sourceUb = sourceLayer->getUb( sourceNeuron );

            sourceLbs.append( sourceLb );
            sourceUbs.append( sourceUb );

            if ( !sourceLayer->neuronEliminated( sourceNeuron ) )
            {
                allConstant = false;
            }
            else
            {
                double sourceValue = sourceLayer->getEliminatedNeuronValue( sourceNeuron );
                sourceValues.append( sourceValue );
            }
        }

        if ( allConstant )
        {
            // If the both source neurons have been eliminated, this neuron is constant
            double value = sourceValues[0] * sourceValues[1];

            if ( _lb[i] < value )
            {
                _lb[i] = value;
                _layerOwner->receiveTighterBound(
                    Tightening( _neuronToVariable[i], _lb[i], Tightening::LB ) );
            }

            if ( _ub[i] > value )
            {
                _ub[i] = value;
                _layerOwner->receiveTighterBound(
                    Tightening( _neuronToVariable[i], _ub[i], Tightening::UB ) );
            }
            continue;
        }

        double lb = FloatUtils::infinity();
        double ub = FloatUtils::negativeInfinity();
        List<double> values = { sourceLbs[0] * sourceLbs[1],
                                sourceLbs[0] * sourceUbs[1],
                                sourceUbs[0] * sourceLbs[1],
                                sourceUbs[0] * sourceUbs[1] };
        for ( const auto &v : values )
        {
            if ( v < lb )
                lb = v;
            if ( v > ub )
                ub = v;
        }


        if ( _lb[i] < lb )
        {
            _lb[i] = lb;
            _layerOwner->receiveTighterBound(
                Tightening( _neuronToVariable[i], _lb[i], Tightening::LB ) );
        }

        if ( _ub[i] > ub )
        {
            _ub[i] = ub;
            _layerOwner->receiveTighterBound(
                Tightening( _neuronToVariable[i], _ub[i], Tightening::UB ) );
        }
    }
}

void Layer::computeSymbolicBounds()
{
    switch ( _type )
    {
    case INPUT:
        computeSymbolicBoundsForInput();
        break;

    case WEIGHTED_SUM:
        computeSymbolicBoundsForWeightedSum();
        break;

    case RELU:
        computeSymbolicBoundsForRelu();
        break;

    case SIGN:
        computeSymbolicBoundsForSign();
        break;

    case ABSOLUTE_VALUE:
        computeSymbolicBoundsForAbsoluteValue();
        break;

    case LEAKY_RELU:
        computeSymbolicBoundsForLeakyRelu();
        break;

    case ROUND:
        computeSymbolicBoundsForRound();
        break;

    case SIGMOID:
        computeSymbolicBoundsForSigmoid();
        break;

    case MAX:
        computeSymbolicBoundsForMax();
        break;

    case SOFTMAX:
        computeSymbolicBoundsForSoftmax();
        break;

    case BILINEAR:
        computeSymbolicBoundsForBilinear();
        break;

    default:
        computeSymbolicBoundsDefault();
        break;
    }
}

void Layer::computeSymbolicBoundsDefault()
{
    // This is the default operation, for layers that are not
    // supported yet. The "symbolic" bounds computed are just the
    // concrete bounds.

    std::fill_n( _symbolicLb, _size * _inputLayerSize, 0 );
    std::fill_n( _symbolicUb, _size * _inputLayerSize, 0 );

    for ( unsigned i = 0; i < _size; ++i )
    {
        double lb;
        double ub;

        if ( _eliminatedNeurons.exists( i ) )
        {
            lb = _eliminatedNeurons[i];
            ub = _eliminatedNeurons[i];
        }
        else
        {
            lb = _lb[i];
            ub = _ub[i];
        }

        _symbolicLowerBias[i] = lb;
        _symbolicUpperBias[i] = ub;

        _symbolicLbOfLb[i] = lb;
        _symbolicUbOfLb[i] = ub;
        _symbolicLbOfUb[i] = lb;
        _symbolicUbOfUb[i] = ub;
    }
}

void Layer::computeSymbolicBoundsForInput()
{
    std::fill_n( _symbolicLb, _size * _inputLayerSize, 0 );
    std::fill_n( _symbolicUb, _size * _inputLayerSize, 0 );

    // For the input layer, the bounds are just the identity polynomials
    for ( unsigned i = 0; i < _size; ++i )
    {
        _symbolicLb[_size * i + i] = 1;
        _symbolicUb[_size * i + i] = 1;

        _symbolicLowerBias[i] = 0;
        _symbolicUpperBias[i] = 0;

        double lb = _lb[i];
        double ub = _ub[i];

        if ( _eliminatedNeurons.exists( i ) )
        {
            lb = _eliminatedNeurons[i];
            ub = _eliminatedNeurons[i];
        }

        _symbolicLbOfLb[i] = lb;
        _symbolicUbOfLb[i] = ub;
        _symbolicLbOfUb[i] = lb;
        _symbolicUbOfUb[i] = ub;
    }
}

void Layer::computeSymbolicBoundsForRelu()
{
    std::fill_n( _symbolicLb, _size * _inputLayerSize, 0 );
    std::fill_n( _symbolicUb, _size * _inputLayerSize, 0 );

    for ( unsigned i = 0; i < _size; ++i )
    {
        if ( _eliminatedNeurons.exists( i ) )
        {
            _symbolicLowerBias[i] = _eliminatedNeurons[i];
            _symbolicUpperBias[i] = _eliminatedNeurons[i];

            _symbolicLbOfLb[i] = _eliminatedNeurons[i];
            _symbolicUbOfLb[i] = _eliminatedNeurons[i];
            _symbolicLbOfUb[i] = _eliminatedNeurons[i];
            _symbolicUbOfUb[i] = _eliminatedNeurons[i];
        }
    }

    for ( unsigned i = 0; i < _size; ++i )
    {
        if ( _eliminatedNeurons.exists( i ) )
            continue;

        /*
          There are two ways we can determine that a ReLU has become fixed:

          1. If the ReLU's variable has been externally fixed
          2. lbLb >= 0 (ACTIVE) or ubUb <= 0 (INACTIVE)
        */
        PhaseStatus reluPhase = PHASE_NOT_FIXED;

        // Has the f variable been eliminated or fixed?
        if ( FloatUtils::isPositive( _lb[i] ) )
            reluPhase = RELU_PHASE_ACTIVE;
        else if ( FloatUtils::isZero( _ub[i] ) )
            reluPhase = RELU_PHASE_INACTIVE;

        ASSERT( _neuronToActivationSources.exists( i ) );
        NeuronIndex sourceIndex = *_neuronToActivationSources[i].begin();
        const Layer *sourceLayer = _layerOwner->getLayer( sourceIndex._layer );

        /*
          A ReLU initially "inherits" the symbolic bounds computed
          for its input variable
        */
        unsigned sourceLayerSize = sourceLayer->getSize();
        const double *sourceSymbolicLb = sourceLayer->getSymbolicLb();
        const double *sourceSymbolicUb = sourceLayer->getSymbolicUb();

        for ( unsigned j = 0; j < _inputLayerSize; ++j )
        {
            _symbolicLb[j * _size + i] =
                sourceSymbolicLb[j * sourceLayerSize + sourceIndex._neuron];
            _symbolicUb[j * _size + i] =
                sourceSymbolicUb[j * sourceLayerSize + sourceIndex._neuron];
        }
        _symbolicLowerBias[i] = sourceLayer->getSymbolicLowerBias()[sourceIndex._neuron];
        _symbolicUpperBias[i] = sourceLayer->getSymbolicUpperBias()[sourceIndex._neuron];

        double sourceLb = sourceLayer->getLb( sourceIndex._neuron );
        double sourceUb = sourceLayer->getUb( sourceIndex._neuron );

        _symbolicLbOfLb[i] = sourceLayer->getSymbolicLbOfLb( sourceIndex._neuron );
        _symbolicUbOfLb[i] = sourceLayer->getSymbolicUbOfLb( sourceIndex._neuron );
        _symbolicLbOfUb[i] = sourceLayer->getSymbolicLbOfUb( sourceIndex._neuron );
        _symbolicUbOfUb[i] = sourceLayer->getSymbolicUbOfUb( sourceIndex._neuron );

        // Has the b variable been fixed?
        if ( !FloatUtils::isNegative( sourceLb ) )
        {
            reluPhase = RELU_PHASE_ACTIVE;
        }
        else if ( !FloatUtils::isPositive( sourceUb ) )
        {
            reluPhase = RELU_PHASE_INACTIVE;
        }

        if ( reluPhase == PHASE_NOT_FIXED )
        {
            // If we got here, we know that lbLb < 0 and ubUb
            // > 0 There are four possible cases, depending on
            // whether ubLb and lbUb are negative or positive
            // (see Neurify paper, page 14).

            // Upper bound
            if ( _symbolicLbOfUb[i] <= 0 )
            {
                // lbOfUb[i] < 0 < ubOfUb[i]
                // Concretize the upper bound using the Ehler's-like approximation
                for ( unsigned j = 0; j < _inputLayerSize; ++j )
                    _symbolicUb[j * _size + i] = _symbolicUb[j * _size + i] * _symbolicUbOfUb[i] /
                                                 ( _symbolicUbOfUb[i] - _symbolicLbOfUb[i] );

                // Do the same for the bias, and then adjust
                _symbolicUpperBias[i] = _symbolicUpperBias[i] * _symbolicUbOfUb[i] /
                                        ( _symbolicUbOfUb[i] - _symbolicLbOfUb[i] );
                _symbolicUpperBias[i] -= _symbolicLbOfUb[i] * _symbolicUbOfUb[i] /
                                         ( _symbolicUbOfUb[i] - _symbolicLbOfUb[i] );
            }

            // Lower bound
            if ( _symbolicUbOfLb[i] <= 0 )
            {
                for ( unsigned j = 0; j < _inputLayerSize; ++j )
                    _symbolicLb[j * _size + i] = 0;

                _symbolicLowerBias[i] = 0;
            }
            else
            {
                for ( unsigned j = 0; j < _inputLayerSize; ++j )
                    _symbolicLb[j * _size + i] = _symbolicLb[j * _size + i] * _symbolicUbOfLb[i] /
                                                 ( _symbolicUbOfLb[i] - _symbolicLbOfLb[i] );

                _symbolicLowerBias[i] = _symbolicLowerBias[i] * _symbolicUbOfLb[i] /
                                        ( _symbolicUbOfLb[i] - _symbolicLbOfLb[i] );
            }

            _symbolicLbOfLb[i] = 0;
        }
        else
        {
            // The phase of this ReLU is fixed!
            if ( reluPhase == RELU_PHASE_ACTIVE )
            {
                // Active ReLU, bounds are propagated as is
            }
            else
            {
                // Inactive ReLU, returns zero
                _symbolicLbOfLb[i] = 0;
                _symbolicUbOfLb[i] = 0;
                _symbolicLbOfUb[i] = 0;
                _symbolicUbOfUb[i] = 0;

                for ( unsigned j = 0; j < _inputLayerSize; ++j )
                {
                    _symbolicUb[j * _size + i] = 0;
                    _symbolicLb[j * _size + i] = 0;
                }

                _symbolicLowerBias[i] = 0;
                _symbolicUpperBias[i] = 0;
            }
        }

        if ( _symbolicLbOfUb[i] < 0 )
            _symbolicLbOfUb[i] = 0;

        /*
          We now have the tightest bounds we can for the relu
          variable. If they are tigheter than what was previously
          known, store them.
        */
        if ( _lb[i] < _symbolicLbOfLb[i] )
        {
            _lb[i] = _symbolicLbOfLb[i];
            _layerOwner->receiveTighterBound(
                Tightening( _neuronToVariable[i], _lb[i], Tightening::LB ) );
        }

        if ( _ub[i] > _symbolicUbOfUb[i] )
        {
            _ub[i] = _symbolicUbOfUb[i];
            _layerOwner->receiveTighterBound(
                Tightening( _neuronToVariable[i], _ub[i], Tightening::UB ) );
        }
    }
}

void Layer::computeSymbolicBoundsForSign()
{
    std::fill_n( _symbolicLb, _size * _inputLayerSize, 0 );
    std::fill_n( _symbolicUb, _size * _inputLayerSize, 0 );

    for ( unsigned i = 0; i < _size; ++i )
    {
        // Eliminate neurons are skipped
        if ( _eliminatedNeurons.exists( i ) )
        {
            _symbolicLowerBias[i] = _eliminatedNeurons[i];
            _symbolicUpperBias[i] = _eliminatedNeurons[i];

            _symbolicLbOfLb[i] = _eliminatedNeurons[i];
            _symbolicUbOfLb[i] = _eliminatedNeurons[i];
            _symbolicLbOfUb[i] = _eliminatedNeurons[i];
            _symbolicUbOfUb[i] = _eliminatedNeurons[i];

            continue;
        }

        /*
          There are two ways we can determine that a Sign has become fixed:

          1. If the Sign's variable has been externally fixed
          2. lbLb >= 0 (Positive) or ubUb < 0 (Negative)
        */
        PhaseStatus signPhase = PHASE_NOT_FIXED;

        // Has the f variable been eliminated or fixed?
        if ( !FloatUtils::isNegative( _lb[i] ) )
            signPhase = SIGN_PHASE_POSITIVE;
        else if ( FloatUtils::isNegative( _ub[i] ) )
            signPhase = SIGN_PHASE_NEGATIVE;

        ASSERT( _neuronToActivationSources.exists( i ) );
        NeuronIndex sourceIndex = *_neuronToActivationSources[i].begin();
        const Layer *sourceLayer = _layerOwner->getLayer( sourceIndex._layer );

        /*
          A Sign initially "inherits" the symbolic bounds computed
          for its input variable
        */
        unsigned sourceLayerSize = sourceLayer->getSize();
        const double *sourceSymbolicLb = sourceLayer->getSymbolicLb();
        const double *sourceSymbolicUb = sourceLayer->getSymbolicUb();

        for ( unsigned j = 0; j < _inputLayerSize; ++j )
        {
            _symbolicLb[j * _size + i] =
                sourceSymbolicLb[j * sourceLayerSize + sourceIndex._neuron];
            _symbolicUb[j * _size + i] =
                sourceSymbolicUb[j * sourceLayerSize + sourceIndex._neuron];
        }
        _symbolicLowerBias[i] = sourceLayer->getSymbolicLowerBias()[sourceIndex._neuron];
        _symbolicUpperBias[i] = sourceLayer->getSymbolicUpperBias()[sourceIndex._neuron];

        double sourceLb = sourceLayer->getLb( sourceIndex._neuron );
        double sourceUb = sourceLayer->getUb( sourceIndex._neuron );

        _symbolicLbOfLb[i] = sourceLayer->getSymbolicLbOfLb( sourceIndex._neuron );
        _symbolicUbOfLb[i] = sourceLayer->getSymbolicUbOfLb( sourceIndex._neuron );
        _symbolicLbOfUb[i] = sourceLayer->getSymbolicLbOfUb( sourceIndex._neuron );
        _symbolicUbOfUb[i] = sourceLayer->getSymbolicUbOfUb( sourceIndex._neuron );

        // Has the b variable been fixed?
        if ( !FloatUtils::isNegative( sourceLb ) )
        {
            signPhase = SIGN_PHASE_POSITIVE;
        }
        else if ( FloatUtils::isNegative( sourceUb ) )
        {
            signPhase = SIGN_PHASE_NEGATIVE;
        }

        if ( signPhase == PHASE_NOT_FIXED )
        {
            PhaseStatus upperSignPhase = PHASE_NOT_FIXED;
            PhaseStatus lowerSignPhase = PHASE_NOT_FIXED;

            // If we got here, we know that lbLb < 0 and ubUb
            // > 0

            // Upper bound
            if ( !FloatUtils::isNegative( _symbolicLbOfUb[i] ) )
            {
                // The upper bound is strictly positive - turns into
                // the constant 1

                for ( unsigned j = 0; j < _inputLayerSize; ++j )
                    _symbolicUb[j * _size + i] = 0;

                _symbolicUpperBias[i] = 1;

                upperSignPhase = SIGN_PHASE_POSITIVE;
            }
            else
            {
                // The upper bound's phase is not fixed, use the
                // parallelogram approximation
                double factor = -2.0 / _symbolicLbOfLb[i];

                for ( unsigned j = 0; j < _inputLayerSize; ++j )
                    _symbolicUb[j * _size + i] *= factor;


                // Do the same for the bias, and then adjust
                _symbolicUpperBias[i] *= factor;
                _symbolicUpperBias[i] += 1;
            }

            // Lower bound
            if ( FloatUtils::isNegative( _symbolicUbOfLb[i] ) )
            {
                // The lower bound is strictly negative - turns into
                // the constant -1

                for ( unsigned j = 0; j < _inputLayerSize; ++j )
                    _symbolicLb[j * _size + i] = 0;

                _symbolicLowerBias[i] = -1;

                lowerSignPhase = SIGN_PHASE_NEGATIVE;
            }
            else
            {
                // The lower bound's phase is not fixed, use the
                // parallelogram approximation
                double factor = 2.0 / _symbolicUbOfUb[i];

                for ( unsigned j = 0; j < _inputLayerSize; ++j )
                {
                    _symbolicLb[j * _size + i] *= factor;
                }

                // Do the same for the bias, and then adjust
                _symbolicLowerBias[i] *= factor;
                _symbolicLowerBias[i] -= 1;
            }

            if ( upperSignPhase == PHASE_NOT_FIXED )
            {
                _symbolicUbOfUb[i] = 1;
                _symbolicLbOfUb[i] = -1;
            }
            else
            {
                _symbolicUbOfUb[i] = 1;
                _symbolicLbOfUb[i] = 1;
            }

            if ( lowerSignPhase == PHASE_NOT_FIXED )
            {
                _symbolicUbOfLb[i] = 1;
                _symbolicLbOfLb[i] = -1;
            }
            else
            {
                _symbolicUbOfLb[i] = -1;
                _symbolicLbOfLb[i] = -1;
            }
        }
        else
        {
            // The phase of this Sign is fixed!
            double constant = ( signPhase == SIGN_PHASE_POSITIVE ) ? 1 : -1;

            _symbolicLbOfLb[i] = constant;
            _symbolicUbOfLb[i] = constant;
            _symbolicLbOfUb[i] = constant;
            _symbolicUbOfUb[i] = constant;

            for ( unsigned j = 0; j < _inputLayerSize; ++j )
            {
                _symbolicUb[j * _size + i] = 0;
                _symbolicLb[j * _size + i] = 0;
            }

            _symbolicLowerBias[i] = constant;
            _symbolicUpperBias[i] = constant;
        }

        if ( _symbolicLbOfLb[i] < -1 )
            _symbolicLbOfLb[i] = -1;
        if ( _symbolicUbOfUb[i] > 1 )
            _symbolicUbOfUb[i] = 1;

        /*
          We now have the tightest bounds we can for the sign
          variable. If they are tigheter than what was previously
          known, store them.
        */
        if ( _lb[i] < _symbolicLbOfLb[i] )
        {
            _lb[i] = _symbolicLbOfLb[i];
            _layerOwner->receiveTighterBound(
                Tightening( _neuronToVariable[i], _lb[i], Tightening::LB ) );
        }

        if ( _ub[i] > _symbolicUbOfUb[i] )
        {
            _ub[i] = _symbolicUbOfUb[i];
            _layerOwner->receiveTighterBound(
                Tightening( _neuronToVariable[i], _ub[i], Tightening::UB ) );
        }
    }
}

void Layer::computeSymbolicBoundsForAbsoluteValue()
{
    std::fill_n( _symbolicLb, _size * _inputLayerSize, 0 );
    std::fill_n( _symbolicUb, _size * _inputLayerSize, 0 );

    for ( unsigned i = 0; i < _size; ++i )
    {
        if ( _eliminatedNeurons.exists( i ) )
        {
            _symbolicLowerBias[i] = _eliminatedNeurons[i];
            _symbolicUpperBias[i] = _eliminatedNeurons[i];

            _symbolicLbOfLb[i] = _eliminatedNeurons[i];
            _symbolicUbOfLb[i] = _eliminatedNeurons[i];
            _symbolicLbOfUb[i] = _eliminatedNeurons[i];
            _symbolicUbOfUb[i] = _eliminatedNeurons[i];

            continue;
        }

        PhaseStatus absPhase = PHASE_NOT_FIXED;

        ASSERT( _neuronToActivationSources.exists( i ) );
        NeuronIndex sourceIndex = *_neuronToActivationSources[i].begin();
        const Layer *sourceLayer = _layerOwner->getLayer( sourceIndex._layer );

        unsigned sourceLayerSize = sourceLayer->getSize();
        const double *sourceSymbolicLb = sourceLayer->getSymbolicLb();
        const double *sourceSymbolicUb = sourceLayer->getSymbolicUb();

        for ( unsigned j = 0; j < _inputLayerSize; ++j )
        {
            _symbolicLb[j * _size + i] =
                sourceSymbolicLb[j * sourceLayerSize + sourceIndex._neuron];
            _symbolicUb[j * _size + i] =
                sourceSymbolicUb[j * sourceLayerSize + sourceIndex._neuron];
        }

        _symbolicLowerBias[i] = sourceLayer->getSymbolicLowerBias()[sourceIndex._neuron];
        _symbolicUpperBias[i] = sourceLayer->getSymbolicUpperBias()[sourceIndex._neuron];

        double sourceLb = sourceLayer->getLb( sourceIndex._neuron );
        double sourceUb = sourceLayer->getUb( sourceIndex._neuron );

        _symbolicLbOfLb[i] = sourceLayer->getSymbolicLbOfLb( sourceIndex._neuron );
        _symbolicUbOfLb[i] = sourceLayer->getSymbolicUbOfLb( sourceIndex._neuron );
        _symbolicLbOfUb[i] = sourceLayer->getSymbolicLbOfUb( sourceIndex._neuron );
        _symbolicUbOfUb[i] = sourceLayer->getSymbolicUbOfUb( sourceIndex._neuron );

        if ( sourceLb >= 0 )
            absPhase = ABS_PHASE_POSITIVE;
        else if ( sourceUb <= 0 )
            absPhase = ABS_PHASE_NEGATIVE;

        if ( absPhase == PHASE_NOT_FIXED )
        {
            // If we got here, we know that lbOfLb < 0 < ubOfUb. In this case,
            // we do naive concretization: lb is 0, ub is the max between
            // -lb and ub of the input neuron
            for ( unsigned j = 0; j < _inputLayerSize; ++j )
            {
                _symbolicLb[j * _size + i] = 0;
                _symbolicUb[j * _size + i] = 0;
            }

            _symbolicLowerBias[i] = 0;
            _symbolicUpperBias[i] = FloatUtils::max( -sourceLb, sourceUb );

            _symbolicLbOfLb[i] = 0;
            _symbolicUbOfLb[i] = _symbolicUpperBias[i];
            _symbolicLbOfUb[i] = 0;
            _symbolicUbOfUb[i] = _symbolicUpperBias[i];
        }
        else
        {
            // The phase of this AbsoluteValueConstraint is fixed!
            if ( absPhase == ABS_PHASE_POSITIVE )
            {
                // Positive AbsoluteValue, bounds are propagated as is
            }
            else
            {
                // Negative AbsoluteValue, bounds are negated and flipped
                double temp;
                for ( unsigned j = 0; j < _inputLayerSize; ++j )
                {
                    temp = _symbolicUb[j * _size + i];
                    _symbolicUb[j * _size + i] = -_symbolicLb[j * _size + i];
                    _symbolicLb[j * _size + i] = -temp;
                }

                temp = _symbolicLowerBias[i];
                _symbolicLowerBias[i] = -_symbolicUpperBias[i];
                _symbolicUpperBias[i] = -temp;

                // Old lb, negated, is the new ub
                temp = _symbolicLbOfLb[i];
                _symbolicLbOfLb[i] = -_symbolicUbOfUb[i];
                _symbolicUbOfUb[i] = -temp;

                temp = _symbolicUbOfLb[i];
                _symbolicUbOfLb[i] = -_symbolicLbOfUb[i];
                _symbolicLbOfUb[i] = -temp;
            }
        }

        // In extreme cases (constraint set externally), _symbolicLbOfLb
        // could be negative - so adjust this
        if ( _symbolicLbOfLb[i] < 0 )
            _symbolicLbOfLb[i] = 0;

        /*
          We now have the tightest bounds we can for the abs
          variable. If they are tigheter than what was previously
          known, store them.
        */
        if ( _lb[i] < _symbolicLbOfLb[i] )
        {
            _lb[i] = _symbolicLbOfLb[i];
            _layerOwner->receiveTighterBound(
                Tightening( _neuronToVariable[i], _lb[i], Tightening::LB ) );
        }

        if ( _ub[i] > _symbolicUbOfUb[i] )
        {
            _ub[i] = _symbolicUbOfUb[i];
            _layerOwner->receiveTighterBound(
                Tightening( _neuronToVariable[i], _ub[i], Tightening::UB ) );
        }
    }
}

void Layer::computeSymbolicBoundsForLeakyRelu()
{
    ASSERT( _alpha > 0 && _alpha < 1 );

    std::fill_n( _symbolicLb, _size * _inputLayerSize, 0 );
    std::fill_n( _symbolicUb, _size * _inputLayerSize, 0 );

    for ( unsigned i = 0; i < _size; ++i )
    {
        if ( _eliminatedNeurons.exists( i ) )
        {
            _symbolicLowerBias[i] = _eliminatedNeurons[i];
            _symbolicUpperBias[i] = _eliminatedNeurons[i];

            _symbolicLbOfLb[i] = _eliminatedNeurons[i];
            _symbolicUbOfLb[i] = _eliminatedNeurons[i];
            _symbolicLbOfUb[i] = _eliminatedNeurons[i];
            _symbolicUbOfUb[i] = _eliminatedNeurons[i];
        }
    }

    for ( unsigned i = 0; i < _size; ++i )
    {
        if ( _eliminatedNeurons.exists( i ) )
            continue;

        /*
          There are two ways we can determine that a LeakyReLU has become fixed:

          1. If the LeakyReLU's variable has been externally fixed
          2. lbLb >= 0 (ACTIVE) or ubUb <= 0 (INACTIVE)
        */
        PhaseStatus leakyReluPhase = PHASE_NOT_FIXED;

        // Has the f variable been eliminated or fixed?
        if ( FloatUtils::isPositive( _lb[i] ) )
            leakyReluPhase = RELU_PHASE_ACTIVE;
        else if ( FloatUtils::isZero( _ub[i] ) )
            leakyReluPhase = RELU_PHASE_INACTIVE;

        ASSERT( _neuronToActivationSources.exists( i ) );
        NeuronIndex sourceIndex = *_neuronToActivationSources[i].begin();
        const Layer *sourceLayer = _layerOwner->getLayer( sourceIndex._layer );

        /*
          A LeakyReLU initially "inherits" the symbolic bounds computed
          for its input variable
        */
        unsigned sourceLayerSize = sourceLayer->getSize();
        const double *sourceSymbolicLb = sourceLayer->getSymbolicLb();
        const double *sourceSymbolicUb = sourceLayer->getSymbolicUb();

        for ( unsigned j = 0; j < _inputLayerSize; ++j )
        {
            _symbolicLb[j * _size + i] =
                sourceSymbolicLb[j * sourceLayerSize + sourceIndex._neuron];
            _symbolicUb[j * _size + i] =
                sourceSymbolicUb[j * sourceLayerSize + sourceIndex._neuron];
        }
        _symbolicLowerBias[i] = sourceLayer->getSymbolicLowerBias()[sourceIndex._neuron];
        _symbolicUpperBias[i] = sourceLayer->getSymbolicUpperBias()[sourceIndex._neuron];

        double sourceLb = sourceLayer->getLb( sourceIndex._neuron );
        double sourceUb = sourceLayer->getUb( sourceIndex._neuron );

        _symbolicLbOfLb[i] = sourceLayer->getSymbolicLbOfLb( sourceIndex._neuron );
        _symbolicUbOfLb[i] = sourceLayer->getSymbolicUbOfLb( sourceIndex._neuron );
        _symbolicLbOfUb[i] = sourceLayer->getSymbolicLbOfUb( sourceIndex._neuron );
        _symbolicUbOfUb[i] = sourceLayer->getSymbolicUbOfUb( sourceIndex._neuron );

        // Has the b variable been fixed?
        if ( !FloatUtils::isNegative( sourceLb ) )
        {
            leakyReluPhase = RELU_PHASE_ACTIVE;
        }
        else if ( !FloatUtils::isPositive( sourceUb ) )
        {
            leakyReluPhase = RELU_PHASE_INACTIVE;
        }

        if ( leakyReluPhase == PHASE_NOT_FIXED )
        {
            // LeakyReLU not fixed
            // Symbolic upper bound: x_f <= (x_b - l) * u / ( u - l)
            // Concrete upper bound: x_f <= ub_b
            double width = sourceUb - sourceLb;
            double coeff = ( sourceUb - _alpha * sourceLb ) / width;

            if ( _alpha <= 1 )
            {
                for ( unsigned j = 0; j < _inputLayerSize; ++j )
                {
                    _symbolicUb[j * _size + i] *= coeff;
                }

                // Do the same for the bias, and then adjust
                _symbolicUpperBias[i] *= coeff;
                _symbolicUpperBias[i] += ( ( _alpha - 1 ) * sourceUb * sourceLb ) / width;


                // For the lower bound, in general, x_f >= lambda * x_b, where
                // 0 <= lambda <= 1, would be a sound lower bound. We
                // use the heuristic described in section 4.1 of
                // https://files.sri.inf.ethz.ch/website/papers/DeepPoly.pdf
                // to set the value of lambda (either 0 or 1 is considered).
                if ( sourceUb > sourceLb )
                {
                    // lambda = 1
                    // Symbolic lower bound: x_f >= x_b
                    // Concrete lower bound: x_f >= sourceLb

                    // Lower bounds are passed as is
                }
                else
                {
                    // lambda = 1
                    // Symbolic lower bound: x_f >= _alpha x_b
                    // Concrete lower bound: x_f >= 0

                    for ( unsigned j = 0; j < _inputLayerSize; ++j )
                    {
                        _symbolicLb[j * _size + i] *= _alpha;
                    }

                    _symbolicLowerBias[i] *= _alpha;
                }
            }
            else
            {
                for ( unsigned j = 0; j < _inputLayerSize; ++j )
                {
                    _symbolicLb[j * _size + i] *= coeff;
                }

                // Do the same for the bias, and then adjust
                _symbolicLowerBias[i] *= coeff;
                _symbolicLowerBias[i] += ( ( _alpha - 1 ) * sourceUb * sourceLb ) / width;

                if ( sourceUb > sourceLb )
                {
                    // Upper bounds are passed as is
                }
                else
                {
                    for ( unsigned j = 0; j < _inputLayerSize; ++j )
                    {
                        _symbolicUb[j * _size + i] *= _alpha;
                    }

                    _symbolicUpperBias[i] *= _alpha;
                }
            }

            /*
              We now have the symbolic representation for the current
              layer. Next, we compute new lower and upper bounds for
              it. For each of these bounds, we compute an upper bound and
              a lower bound.
            */
            _symbolicLbOfLb[i] = _symbolicLowerBias[i];
            _symbolicUbOfLb[i] = _symbolicLowerBias[i];
            _symbolicLbOfUb[i] = _symbolicUpperBias[i];
            _symbolicUbOfUb[i] = _symbolicUpperBias[i];

            for ( unsigned j = 0; j < _inputLayerSize; ++j )
            {
                double inputLb = _layerOwner->getLayer( 0 )->getLb( j );
                double inputUb = _layerOwner->getLayer( 0 )->getUb( j );

                double entry = _symbolicLb[j * _size + i];

                if ( entry >= 0 )
                {
                    _symbolicLbOfLb[i] += ( entry * inputLb );
                    _symbolicUbOfLb[i] += ( entry * inputUb );
                }
                else
                {
                    _symbolicLbOfLb[i] += ( entry * inputUb );
                    _symbolicUbOfLb[i] += ( entry * inputLb );
                }

                entry = _symbolicUb[j * _size + i];

                if ( entry >= 0 )
                {
                    _symbolicLbOfUb[i] += ( entry * inputLb );
                    _symbolicUbOfUb[i] += ( entry * inputUb );
                }
                else
                {
                    _symbolicLbOfUb[i] += ( entry * inputUb );
                    _symbolicUbOfUb[i] += ( entry * inputLb );
                }
            }
        }
        else
        {
            // The phase of this LeakyReLU is fixed!
            if ( leakyReluPhase == RELU_PHASE_ACTIVE )
            {
                // Positive LeakyReLU, bounds are propagated as is
            }
            else
            {
                // Negative LeakyReLU, bounds are multiplied by _alpha
                _symbolicLbOfLb[i] *= _alpha;
                _symbolicUbOfLb[i] *= _alpha;
                _symbolicLbOfUb[i] *= _alpha;
                _symbolicUbOfUb[i] *= _alpha;

                for ( unsigned j = 0; j < _inputLayerSize; ++j )
                {
                    _symbolicUb[j * _size + i] *= _alpha;
                    _symbolicLb[j * _size + i] *= _alpha;
                }

                _symbolicLowerBias[i] *= _alpha;
                _symbolicUpperBias[i] *= _alpha;
            }
        }

        if ( _symbolicUbOfUb[i] > sourceUb )
            _symbolicUbOfUb[i] = sourceUb;
        if ( _symbolicLbOfLb[i] < _alpha * sourceLb )
            _symbolicLbOfLb[i] = _alpha * sourceLb;

        /*
          We now have the tightest bounds we can for the leakyRelu
          variable. If they are tigheter than what was previously
          known, store them.
        */
        if ( _lb[i] < _symbolicLbOfLb[i] )
        {
            _lb[i] = _symbolicLbOfLb[i];
            _layerOwner->receiveTighterBound(
                Tightening( _neuronToVariable[i], _lb[i], Tightening::LB ) );
        }

        if ( _ub[i] > _symbolicUbOfUb[i] )
        {
            _ub[i] = _symbolicUbOfUb[i];
            _layerOwner->receiveTighterBound(
                Tightening( _neuronToVariable[i], _ub[i], Tightening::UB ) );
        }
    }
}


void Layer::computeSymbolicBoundsForSigmoid()
{
    std::fill_n( _symbolicLb, _size * _inputLayerSize, 0 );
    std::fill_n( _symbolicUb, _size * _inputLayerSize, 0 );

    for ( unsigned i = 0; i < _size; ++i )
    {
        if ( _eliminatedNeurons.exists( i ) )
        {
            _symbolicLowerBias[i] = _eliminatedNeurons[i];
            _symbolicUpperBias[i] = _eliminatedNeurons[i];

            _symbolicLbOfLb[i] = _eliminatedNeurons[i];
            _symbolicUbOfLb[i] = _eliminatedNeurons[i];
            _symbolicLbOfUb[i] = _eliminatedNeurons[i];
            _symbolicUbOfUb[i] = _eliminatedNeurons[i];
        }
    }

    for ( unsigned i = 0; i < _size; ++i )
    {
        if ( _eliminatedNeurons.exists( i ) )
            continue;

        ASSERT( _neuronToActivationSources.exists( i ) );
        NeuronIndex sourceIndex = *_neuronToActivationSources[i].begin();
        const Layer *sourceLayer = _layerOwner->getLayer( sourceIndex._layer );

        /*
          A Sigmoid initially "inherits" the symbolic bounds computed
          for its input variable
        */
        unsigned sourceLayerSize = sourceLayer->getSize();
        const double *sourceSymbolicLb = sourceLayer->getSymbolicLb();
        const double *sourceSymbolicUb = sourceLayer->getSymbolicUb();

        for ( unsigned j = 0; j < _inputLayerSize; ++j )
        {
            _symbolicLb[j * _size + i] =
                sourceSymbolicLb[j * sourceLayerSize + sourceIndex._neuron];
            _symbolicUb[j * _size + i] =
                sourceSymbolicUb[j * sourceLayerSize + sourceIndex._neuron];
        }
        _symbolicLowerBias[i] = sourceLayer->getSymbolicLowerBias()[sourceIndex._neuron];
        _symbolicUpperBias[i] = sourceLayer->getSymbolicUpperBias()[sourceIndex._neuron];

        double sourceLb = sourceLayer->getLb( sourceIndex._neuron );
        double sourceUb = sourceLayer->getUb( sourceIndex._neuron );

        _symbolicLbOfLb[i] = sourceLayer->getSymbolicLbOfLb( sourceIndex._neuron );
        _symbolicUbOfLb[i] = sourceLayer->getSymbolicUbOfLb( sourceIndex._neuron );
        _symbolicLbOfUb[i] = sourceLayer->getSymbolicLbOfUb( sourceIndex._neuron );
        _symbolicUbOfUb[i] = sourceLayer->getSymbolicUbOfUb( sourceIndex._neuron );

        // Bounds of lb, ub are the Sigmoids of source lb, ub
        double sourceUbSigmoid = SigmoidConstraint::sigmoid( sourceUb );
        double sourceLbSigmoid = SigmoidConstraint::sigmoid( sourceLb );

        // Case when the Sigmoid constraint is fixed
        if ( FloatUtils::areEqual( FloatUtils::round( sourceUb ), FloatUtils::round( sourceLb ) ) )
        {
            for ( unsigned j = 0; j < _inputLayerSize; ++j )
            {
                _symbolicLb[j * _size + i] = 0;
                _symbolicUb[j * _size + i] = 0;
            }

            _symbolicLbOfUb[i] = sourceUbSigmoid;
            _symbolicUbOfUb[i] = sourceUbSigmoid;
            _symbolicLbOfLb[i] = sourceLbSigmoid;
            _symbolicUbOfLb[i] = sourceLbSigmoid;

            _symbolicUpperBias[i] = sourceUbSigmoid;
            _symbolicLowerBias[i] = sourceLbSigmoid;
        }

        // Sigmoid not fixed
        else
        {
            double lambda = ( _ub[i] - _lb[i] ) / ( sourceUb - sourceLb );
            double lambdaPrime = std::min( SigmoidConstraint::sigmoidDerivative( sourceLb ),
                                           SigmoidConstraint::sigmoidDerivative( sourceUb ) );

            // update lower bound
            if ( FloatUtils::isPositive( sourceLb ) )
            {
                for ( unsigned j = 0; j < _inputLayerSize; ++j )
                {
                    _symbolicLb[j * _size + i] *= lambda;
                }

                // Do the same for the bias, and then adjust
                _symbolicLowerBias[i] *= lambda;
                _symbolicLowerBias[i] += sourceLbSigmoid - lambda * sourceLb;
            }
            else
            {
                for ( unsigned j = 0; j < _inputLayerSize; ++j )
                {
                    _symbolicLb[j * _size + i] *= lambdaPrime;
                }

                // Do the same for the bias, and then adjust
                _symbolicLowerBias[i] *= lambdaPrime;
                _symbolicLowerBias[i] += sourceLbSigmoid - lambdaPrime * sourceLb;
            }

            // update upper bound
            if ( !FloatUtils::isPositive( sourceUb ) )
            {
                for ( unsigned j = 0; j < _inputLayerSize; ++j )
                {
                    _symbolicUb[j * _size + i] *= lambda;
                }

                // Do the same for the bias, and then adjust
                _symbolicUpperBias[i] *= lambda;
                _symbolicUpperBias[i] += sourceUbSigmoid - lambda * sourceUb;
            }
            else
            {
                for ( unsigned j = 0; j < _inputLayerSize; ++j )
                {
                    _symbolicUb[j * _size + i] *= lambdaPrime;
                }

                // Do the same for the bias, and then adjust
                _symbolicUpperBias[i] *= lambdaPrime;
                _symbolicUpperBias[i] += sourceUbSigmoid - lambdaPrime * sourceUb;
            }


            /*
              We now have the symbolic representation for the current
              layer. Next, we compute new lower and upper bounds for
              it. For each of these bounds, we compute an upper bound and
              a lower bound.
            */
            _symbolicLbOfLb[i] = _symbolicLowerBias[i];
            _symbolicUbOfLb[i] = _symbolicLowerBias[i];
            _symbolicLbOfUb[i] = _symbolicUpperBias[i];
            _symbolicUbOfUb[i] = _symbolicUpperBias[i];

            for ( unsigned j = 0; j < _inputLayerSize; ++j )
            {
                double inputLb = _layerOwner->getLayer( 0 )->getLb( j );
                double inputUb = _layerOwner->getLayer( 0 )->getUb( j );

                double entry = _symbolicLb[j * _size + i];

                if ( entry >= 0 )
                {
                    _symbolicLbOfLb[i] += ( entry * inputLb );
                    _symbolicUbOfLb[i] += ( entry * inputUb );
                }
                else
                {
                    _symbolicLbOfLb[i] += ( entry * inputUb );
                    _symbolicUbOfLb[i] += ( entry * inputLb );
                }

                entry = _symbolicUb[j * _size + i];

                if ( entry >= 0 )
                {
                    _symbolicLbOfUb[i] += ( entry * inputLb );
                    _symbolicUbOfUb[i] += ( entry * inputUb );
                }
                else
                {
                    _symbolicLbOfUb[i] += ( entry * inputUb );
                    _symbolicUbOfUb[i] += ( entry * inputLb );
                }
            }
        }

        if ( _symbolicLbOfLb[i] < -1 )
            _symbolicLbOfLb[i] = -1;
        if ( _symbolicUbOfUb[i] > 1 )
            _symbolicUbOfUb[i] = 1;

        /*
          We now have the tightest bounds we can for the relu
          variable. If they are tigheter than what was previously
          known, store them.
        */
        if ( _lb[i] < _symbolicLbOfLb[i] )
        {
            _lb[i] = _symbolicLbOfLb[i];
            _layerOwner->receiveTighterBound(
                Tightening( _neuronToVariable[i], _lb[i], Tightening::LB ) );
        }

        if ( _ub[i] > _symbolicUbOfUb[i] )
        {
            _ub[i] = _symbolicUbOfUb[i];
            _layerOwner->receiveTighterBound(
                Tightening( _neuronToVariable[i], _ub[i], Tightening::UB ) );
        }
    }
}

void Layer::computeSymbolicBoundsForRound()
{
    std::fill_n( _symbolicLb, _size * _inputLayerSize, 0 );
    std::fill_n( _symbolicUb, _size * _inputLayerSize, 0 );

    for ( unsigned i = 0; i < _size; ++i )
    {
        if ( _eliminatedNeurons.exists( i ) )
        {
            _symbolicLowerBias[i] = _eliminatedNeurons[i];
            _symbolicUpperBias[i] = _eliminatedNeurons[i];

            _symbolicLbOfLb[i] = _eliminatedNeurons[i];
            _symbolicUbOfLb[i] = _eliminatedNeurons[i];
            _symbolicLbOfUb[i] = _eliminatedNeurons[i];
            _symbolicUbOfUb[i] = _eliminatedNeurons[i];
        }
    }

    for ( unsigned i = 0; i < _size; ++i )
    {
        if ( _eliminatedNeurons.exists( i ) )
            continue;

        ASSERT( _neuronToActivationSources.exists( i ) );
        NeuronIndex sourceIndex = *_neuronToActivationSources[i].begin();
        const Layer *sourceLayer = _layerOwner->getLayer( sourceIndex._layer );

        /*
          A Round initially "inherits" the symbolic bounds computed
          for its input variable
        */
        unsigned sourceLayerSize = sourceLayer->getSize();
        const double *sourceSymbolicLb = sourceLayer->getSymbolicLb();
        const double *sourceSymbolicUb = sourceLayer->getSymbolicUb();

        for ( unsigned j = 0; j < _inputLayerSize; ++j )
        {
            _symbolicLb[j * _size + i] =
                sourceSymbolicLb[j * sourceLayerSize + sourceIndex._neuron];
            _symbolicUb[j * _size + i] =
                sourceSymbolicUb[j * sourceLayerSize + sourceIndex._neuron];
        }
        _symbolicLowerBias[i] = sourceLayer->getSymbolicLowerBias()[sourceIndex._neuron];
        _symbolicUpperBias[i] = sourceLayer->getSymbolicUpperBias()[sourceIndex._neuron];

        double sourceLb = sourceLayer->getLb( sourceIndex._neuron );
        double sourceUb = sourceLayer->getUb( sourceIndex._neuron );

        _symbolicLbOfLb[i] = sourceLayer->getSymbolicLbOfLb( sourceIndex._neuron );
        _symbolicUbOfLb[i] = sourceLayer->getSymbolicUbOfLb( sourceIndex._neuron );
        _symbolicLbOfUb[i] = sourceLayer->getSymbolicLbOfUb( sourceIndex._neuron );
        _symbolicUbOfUb[i] = sourceLayer->getSymbolicUbOfUb( sourceIndex._neuron );


        // Bounds of lb, ub are the rounded values of source lb, ub
        double sourceUbRound = FloatUtils::round( sourceUb );
        double sourceLbRound = FloatUtils::round( sourceLb );

        _symbolicLbOfUb[i] = sourceUbRound;
        _symbolicUbOfUb[i] = sourceUbRound;
        _symbolicLbOfLb[i] = sourceLbRound;
        _symbolicUbOfLb[i] = sourceLbRound;


        // Case when the Round constraint is fixed
        if ( FloatUtils::areEqual( FloatUtils::round( sourceUb ), FloatUtils::round( sourceLb ) ) )
        {
            _symbolicUb[i] = 0;
            _symbolicUpperBias[i] = sourceUbRound;

            _symbolicLb[i] = 0;
            _symbolicLowerBias[i] = sourceLbRound;
        }

        // Round not fixed
        else
        {
            // Symbolic upper bound: x_f <= x_b + 0.5
            // Concrete upper bound: x_f <= round(ub_b)

            _symbolicUpperBias[i] += 0.5;

            // Symbolic lower bound: x_f >= x_b - 0.5
            // Concrete lower bound: x_f >= round(lb_b)

            _symbolicLowerBias[i] -= 0.5;
        }

        /*
          We now have the tightest bounds we can for the relu
          variable. If they are tigheter than what was previously
          known, store them.
        */
        if ( _lb[i] < _symbolicLbOfLb[i] )
        {
            _lb[i] = _symbolicLbOfLb[i];
            _layerOwner->receiveTighterBound(
                Tightening( _neuronToVariable[i], _lb[i], Tightening::LB ) );
        }

        if ( _ub[i] > _symbolicUbOfUb[i] )
        {
            _ub[i] = _symbolicUbOfUb[i];
            _layerOwner->receiveTighterBound(
                Tightening( _neuronToVariable[i], _ub[i], Tightening::UB ) );
        }
    }
}

void Layer::computeSymbolicBoundsForMax()
{
    std::fill_n( _symbolicLb, _size * _inputLayerSize, 0 );
    std::fill_n( _symbolicUb, _size * _inputLayerSize, 0 );

    for ( unsigned i = 0; i < _size; ++i )
    {
        if ( _eliminatedNeurons.exists( i ) )
        {
            _symbolicLowerBias[i] = _eliminatedNeurons[i];
            _symbolicUpperBias[i] = _eliminatedNeurons[i];

            _symbolicLbOfLb[i] = _eliminatedNeurons[i];
            _symbolicUbOfLb[i] = _eliminatedNeurons[i];
            _symbolicLbOfUb[i] = _eliminatedNeurons[i];
            _symbolicUbOfUb[i] = _eliminatedNeurons[i];
        }
    }

    for ( unsigned i = 0; i < _size; ++i )
    {
        if ( _eliminatedNeurons.exists( i ) )
            continue;

        ASSERT( _neuronToActivationSources.exists( i ) );
        List<NeuronIndex> sources = getActivationSources( i );
        const Layer *sourceLayer = _layerOwner->getLayer( sources.begin()->_layer );

        unsigned sourceLayerSize = sourceLayer->getSize();
        const double *sourceSymbolicLb = sourceLayer->getSymbolicLb();
        const double *sourceSymbolicUb = sourceLayer->getSymbolicUb();

        NeuronIndex indexOfMaxLowerBound = *( sources.begin() );
        double maxLowerBound = FloatUtils::negativeInfinity();
        double maxUpperBound = FloatUtils::negativeInfinity();

        Map<NeuronIndex, double> sourceLbs;
        Map<NeuronIndex, double> sourceUbs;
        for ( const auto &sourceIndex : sources )
        {
            unsigned sourceNeuron = sourceIndex._neuron;
            double sourceLb = sourceLayer->getLb( sourceNeuron );
            double sourceUb = sourceLayer->getUb( sourceNeuron );

            sourceLbs[sourceIndex] = sourceLb;
            sourceUbs[sourceIndex] = sourceUb;

            if ( maxLowerBound < sourceLb )
            {
                indexOfMaxLowerBound = sourceIndex;
                maxLowerBound = sourceLb;
            }
            if ( maxUpperBound < sourceUb )
            {
                maxUpperBound = sourceUb;
            }
        }

        // The phase is fixed if the lower-bound of a source variable x_b is
        // larger than the upper-bounds of the other source variables.
        bool phaseFixed = true;
        for ( const auto &sourceIndex : sources )
        {
            if ( sourceIndex != indexOfMaxLowerBound &&
                 FloatUtils::gt( sourceUbs[sourceIndex], maxLowerBound ) )
            {
                phaseFixed = false;
                break;
            }
        }

        if ( phaseFixed )
        {
            // Phase fixed
            // Symbolic bound: x_b <= x_f <= x_b
            // Concrete bound: lb_b <= x_f <= ub_b
            for ( unsigned j = 0; j < _inputLayerSize; ++j )
            {
                _symbolicLb[j * _size + i] =
                    sourceSymbolicLb[j * sourceLayerSize + indexOfMaxLowerBound._neuron];
                _symbolicUb[j * _size + i] =
                    sourceSymbolicUb[j * sourceLayerSize + indexOfMaxLowerBound._neuron];
            }
            _symbolicLowerBias[i] =
                sourceLayer->getSymbolicLowerBias()[indexOfMaxLowerBound._neuron];
            _symbolicUpperBias[i] =
                sourceLayer->getSymbolicUpperBias()[indexOfMaxLowerBound._neuron];


            _symbolicLbOfLb[i] = maxLowerBound;
            _symbolicUbOfLb[i] = maxLowerBound;
            _symbolicLbOfUb[i] = sourceUbs[indexOfMaxLowerBound];
            _symbolicUbOfUb[i] = sourceUbs[indexOfMaxLowerBound];
        }
        else
        {
            // MaxPool not fixed
            // Symbolic bounds: x_b <= x_f <= maxUpperBound
            // Concrete bounds: lb_b <= x_f <= maxUpperBound
            for ( unsigned j = 0; j < _inputLayerSize; ++j )
            {
                _symbolicLb[j * _size + i] =
                    sourceSymbolicLb[j * sourceLayerSize + indexOfMaxLowerBound._neuron];
                _symbolicUb[j * _size + i] = 0;
            }
            _symbolicLowerBias[i] =
                sourceLayer->getSymbolicLowerBias()[indexOfMaxLowerBound._neuron];
            _symbolicUpperBias[i] = maxUpperBound;

            _symbolicLbOfLb[i] = maxLowerBound;
            _symbolicUbOfLb[i] = maxLowerBound;
            _symbolicLbOfUb[i] = maxUpperBound;
            _symbolicUbOfUb[i] = maxUpperBound;
        }

        /*
          We now have the tightest bounds we can for the relu
          variable. If they are tigheter than what was previously
          known, store them.
        */
        if ( _lb[i] < _symbolicLbOfLb[i] )
        {
            _lb[i] = _symbolicLbOfLb[i];
            _layerOwner->receiveTighterBound(
                Tightening( _neuronToVariable[i], _lb[i], Tightening::LB ) );
        }

        if ( _ub[i] > _symbolicUbOfUb[i] )
        {
            _ub[i] = _symbolicUbOfUb[i];
            _layerOwner->receiveTighterBound(
                Tightening( _neuronToVariable[i], _ub[i], Tightening::UB ) );
        }
    }
}

void Layer::computeSymbolicBoundsForSoftmax()
{
    std::fill_n( _symbolicLowerBias, _size, 0 );
    std::fill_n( _symbolicUpperBias, _size, 0 );

    double *symbolicLb = new double[_size * _size];
    double *symbolicUb = new double[_size * _size];
    std::fill_n( symbolicLb, _size * _size, 0 );
    std::fill_n( symbolicUb, _size * _size, 0 );

    double *_work = new double[_size * _size];
    std::fill_n( _work, _size * _size, 0 );

    Set<unsigned> handledInputNeurons;
    unsigned sourceLayerSize = _size;
    SoftmaxBoundType boundType = Options::get()->getSoftmaxBoundType();

    for ( unsigned i = 0; i < _size; ++i )
    {
        if ( _eliminatedNeurons.exists( i ) )
        {
            _symbolicLowerBias[i] = _eliminatedNeurons[i];
            _symbolicUpperBias[i] = _eliminatedNeurons[i];

            _symbolicLbOfLb[i] = _eliminatedNeurons[i];
            _symbolicUbOfLb[i] = _eliminatedNeurons[i];
            _symbolicLbOfUb[i] = _eliminatedNeurons[i];
            _symbolicUbOfUb[i] = _eliminatedNeurons[i];
        }
    }

    for ( unsigned i = 0; i < _size; ++i )
    {
        if ( _eliminatedNeurons.exists( i ) )
            continue;

        ASSERT( _neuronToActivationSources.exists( i ) );
        List<NeuronIndex> sources = getActivationSources( i );
        const Layer *sourceLayer = _layerOwner->getLayer( sources.begin()->_layer );

        sourceLayerSize = sourceLayer->getSize();
        ASSERT( sourceLayerSize == _size );

        Vector<double> sourceLbs;
        Vector<double> sourceUbs;
        Vector<double> sourceMids;
        Vector<double> targetLbs;
        Vector<double> targetUbs;
        unsigned len = 0;
        for ( const auto &sourceIndex : sources )
        {
            unsigned sourceNeuron = sourceIndex._neuron;
            double sourceLb = sourceLayer->getLb( sourceNeuron );
            double sourceUb = sourceLayer->getUb( sourceNeuron );

            sourceLbs.append( sourceLb - GlobalConfiguration::DEFAULT_EPSILON_FOR_COMPARISONS );
            sourceUbs.append( sourceUb + GlobalConfiguration::DEFAULT_EPSILON_FOR_COMPARISONS );
            sourceMids.append( ( sourceLb + sourceUb ) / 2 );
            targetLbs.append( _lb[i] );
            targetUbs.append( _ub[i] );

            ++len;
        }

        // Find the index of i in the softmax
        unsigned index = 0;
        for ( const auto &sourceIndex : sources )
        {
            if ( handledInputNeurons.exists( sourceIndex._neuron ) )
                ++index;
            else
            {
                handledInputNeurons.insert( sourceIndex._neuron );
                break;
            }
        }

        double lb = softmaxLinearLowerBound( sourceLbs, sourceUbs, index );
        double ub = softmaxLinearUpperBound( sourceLbs, sourceUbs, index );
        if ( _lb[i] < lb )
        {
            _lb[i] = lb;
            _layerOwner->receiveTighterBound(
                Tightening( _neuronToVariable[i], _lb[i], Tightening::LB ) );
        }
        if ( _ub[i] > ub )
        {
            _ub[i] = ub;
            _layerOwner->receiveTighterBound(
                Tightening( _neuronToVariable[i], _ub[i], Tightening::UB ) );
        }
        targetLbs[index] = _lb[i];
        targetUbs[index] = _ub[i];

        if ( FloatUtils::areEqual( _lb[i], _ub[i] ) )
        {
            _symbolicLowerBias[i] = _lb[i];
            _symbolicUpperBias[i] = _ub[i];
            for ( const auto &sourceIndex : sources )
            {
                symbolicLb[len * sourceIndex._neuron + i] = 0;
                symbolicUb[len * sourceIndex._neuron + i] = 0;
            }
        }
        else
        {
            // Compute symbolic bound
            if ( boundType == SoftmaxBoundType::LOG_SUM_EXP_DECOMPOSITION )
            {
                bool useLSE2 = false;
                for ( const auto &lb : targetLbs )
                {
                    if ( lb > GlobalConfiguration::SOFTMAX_LSE2_THRESHOLD )
                        useLSE2 = true;
                }
                unsigned inputIndex = 0;
                if ( !useLSE2 )
                {
                    _symbolicLowerBias[i] =
                        softmaxLSELowerBound( sourceMids, sourceLbs, sourceUbs, index );
                    for ( const auto &sourceIndex : sources )
                    {
                        double dldj = softmaxdLSELowerBound(
                            sourceMids, sourceLbs, sourceUbs, index, inputIndex );
                        symbolicLb[len * sourceIndex._neuron + i] = dldj;
                        _symbolicLowerBias[i] -= dldj * sourceMids[inputIndex];
                        ++inputIndex;
                    }
                }
                else
                {
                    _symbolicLowerBias[i] =
                        softmaxLSELowerBound2( sourceMids, sourceLbs, sourceUbs, index );
                    for ( const auto &sourceIndex : sources )
                    {
                        double dldj = softmaxdLSELowerBound2(
                            sourceMids, sourceLbs, sourceUbs, index, inputIndex );
                        symbolicLb[len * sourceIndex._neuron + i] = dldj;
                        _symbolicLowerBias[i] -= dldj * sourceMids[inputIndex];
                        ++inputIndex;
                    }
                }

                _symbolicUpperBias[i] =
                    softmaxLSEUpperBound( sourceMids, targetLbs, targetUbs, index );
                inputIndex = 0;
                for ( const auto &sourceIndex : sources )
                {
                    double dudj = softmaxdLSEUpperbound(
                        sourceMids, targetLbs, targetUbs, index, inputIndex );
                    symbolicUb[len * sourceIndex._neuron + i] = dudj;
                    _symbolicUpperBias[i] -= dudj * sourceMids[inputIndex];
                    ++inputIndex;
                }
            }
            else if ( boundType == SoftmaxBoundType::EXPONENTIAL_RECIPROCAL_DECOMPOSITION )
            {
                _symbolicLowerBias[i] =
                    softmaxERLowerBound( sourceMids, sourceLbs, sourceUbs, index );
                unsigned inputIndex = 0;
                for ( const auto &sourceIndex : sources )
                {
                    double dldj =
                        softmaxdERLowerBound( sourceMids, sourceLbs, sourceUbs, index, inputIndex );
                    symbolicLb[len * sourceIndex._neuron + i] = dldj;
                    _symbolicLowerBias[i] -= dldj * sourceMids[inputIndex];
                    ++inputIndex;
                }

                _symbolicUpperBias[i] =
                    softmaxERUpperBound( sourceMids, targetLbs, targetUbs, index );
                inputIndex = 0;
                for ( const auto &sourceIndex : sources )
                {
                    double dudj =
                        softmaxdERUpperBound( sourceMids, targetLbs, targetUbs, index, inputIndex );
                    symbolicUb[len * sourceIndex._neuron + i] = dudj;
                    _symbolicUpperBias[i] -= dudj * sourceMids[inputIndex];
                    ++inputIndex;
                }
            }
        }
    }

    for ( const auto &sourceLayerEntry : _sourceLayers )
    {
        const Layer *sourceLayer = _layerOwner->getLayer( sourceLayerEntry.first );

        /*
          Perform the multiplication

          newUB = oldUB * posWeights + oldLB * negWeights
          newLB = oldUB * negWeights + oldLB * posWeights
        */

        for ( unsigned i = 0; i < sourceLayerSize * _size; ++i )
        {
            if ( symbolicLb[i] > 0 )
                _work[i] = symbolicLb[i];
            else
                _work[i] = 0;
        }
        // _work is now positive weights in symbolicLb
        matrixMultiplication( sourceLayer->getSymbolicLb(),
                              _work,
                              _symbolicLb,
                              _inputLayerSize,
                              sourceLayerSize,
                              _size );
        if ( sourceLayer->getSymbolicLowerBias() )
            matrixMultiplication( sourceLayer->getSymbolicLowerBias(),
                                  _work,
                                  _symbolicLowerBias,
                                  1,
                                  sourceLayerSize,
                                  _size );

        for ( unsigned i = 0; i < sourceLayerSize * _size; ++i )
        {
            if ( symbolicLb[i] < 0 )
                _work[i] = symbolicLb[i];
            else
                _work[i] = 0;
        }
        // _work is now negative weights in symbolicLb
        matrixMultiplication( sourceLayer->getSymbolicUb(),
                              _work,
                              _symbolicLb,
                              _inputLayerSize,
                              sourceLayerSize,
                              _size );
        if ( sourceLayer->getSymbolicLowerBias() )
            matrixMultiplication( sourceLayer->getSymbolicUpperBias(),
                                  _work,
                                  _symbolicLowerBias,
                                  1,
                                  sourceLayerSize,
                                  _size );

        for ( unsigned i = 0; i < sourceLayerSize * _size; ++i )
        {
            if ( symbolicUb[i] > 0 )
                _work[i] = symbolicUb[i];
            else
                _work[i] = 0;
        }
        // _work is now positive weights in symbolicUb
        matrixMultiplication( sourceLayer->getSymbolicUb(),
                              _work,
                              _symbolicUb,
                              _inputLayerSize,
                              sourceLayerSize,
                              _size );
        if ( sourceLayer->getSymbolicUpperBias() )
            matrixMultiplication( sourceLayer->getSymbolicUpperBias(),
                                  _work,
                                  _symbolicUpperBias,
                                  1,
                                  sourceLayerSize,
                                  _size );

        for ( unsigned i = 0; i < sourceLayerSize * _size; ++i )
        {
            if ( symbolicUb[i] < 0 )
                _work[i] = symbolicUb[i];
            else
                _work[i] = 0;
        }
        // _work is now negative weights in symbolicUb
        matrixMultiplication( sourceLayer->getSymbolicLb(),
                              _work,
                              _symbolicUb,
                              _inputLayerSize,
                              sourceLayerSize,
                              _size );
        if ( sourceLayer->getSymbolicUpperBias() )
            matrixMultiplication( sourceLayer->getSymbolicLowerBias(),
                                  _work,
                                  _symbolicUpperBias,
                                  1,
                                  sourceLayerSize,
                                  _size );
    }

    /*
      We now have the symbolic representation for the current
      layer. Next, we compute new lower and upper bounds for
      it. For each of these bounds, we compute an upper bound and
      a lower bound.
    */
    for ( unsigned i = 0; i < _size; ++i )
    {
        if ( _eliminatedNeurons.exists( i ) )
            continue;

        _symbolicLbOfLb[i] = _symbolicLowerBias[i];
        _symbolicUbOfLb[i] = _symbolicLowerBias[i];
        _symbolicLbOfUb[i] = _symbolicUpperBias[i];
        _symbolicUbOfUb[i] = _symbolicUpperBias[i];

        for ( unsigned j = 0; j < _inputLayerSize; ++j )
        {
            double inputLb = _layerOwner->getLayer( 0 )->getLb( j );
            double inputUb = _layerOwner->getLayer( 0 )->getUb( j );

            double entry = _symbolicLb[j * _size + i];

            if ( entry >= 0 )
            {
                _symbolicLbOfLb[i] += ( entry * inputLb );
                _symbolicUbOfLb[i] += ( entry * inputUb );
            }
            else
            {
                _symbolicLbOfLb[i] += ( entry * inputUb );
                _symbolicUbOfLb[i] += ( entry * inputLb );
            }

            entry = _symbolicUb[j * _size + i];

            if ( entry >= 0 )
            {
                _symbolicLbOfUb[i] += ( entry * inputLb );
                _symbolicUbOfUb[i] += ( entry * inputUb );
            }
            else
            {
                _symbolicLbOfUb[i] += ( entry * inputUb );
                _symbolicUbOfUb[i] += ( entry * inputLb );
            }
        }
    }

    if ( symbolicLb )
    {
        delete[] symbolicLb;
        symbolicLb = NULL;
    }
    if ( symbolicUb )
    {
        delete[] symbolicUb;
        symbolicUb = NULL;
    }
    if ( _work )
    {
        delete[] _work;
        _work = NULL;
    }
}

void Layer::computeSymbolicBoundsForBilinear()
{
    std::fill_n( _symbolicLb, _size * _inputLayerSize, 0 );
    std::fill_n( _symbolicUb, _size * _inputLayerSize, 0 );

    for ( unsigned i = 0; i < _size; ++i )
    {
        if ( _eliminatedNeurons.exists( i ) )
        {
            _symbolicLowerBias[i] = _eliminatedNeurons[i];
            _symbolicUpperBias[i] = _eliminatedNeurons[i];

            _symbolicLbOfLb[i] = _eliminatedNeurons[i];
            _symbolicUbOfLb[i] = _eliminatedNeurons[i];
            _symbolicLbOfUb[i] = _eliminatedNeurons[i];
            _symbolicUbOfUb[i] = _eliminatedNeurons[i];
        }
    }

    for ( unsigned i = 0; i < _size; ++i )
    {
        if ( _eliminatedNeurons.exists( i ) )
            continue;

        ASSERT( _neuronToActivationSources.exists( i ) );
        List<NeuronIndex> sources = getActivationSources( i );
        ASSERT( sources.size() == 2 );

        const Layer *sourceLayer = _layerOwner->getLayer( sources.begin()->_layer );

        unsigned sourceLayerSize = sourceLayer->getSize();
        const double *sourceSymbolicLb = sourceLayer->getSymbolicLb();
        const double *sourceSymbolicUb = sourceLayer->getSymbolicUb();

        Vector<double> sourceLbs;
        Vector<double> sourceUbs;
        Vector<double> sourceValues;
        bool allConstant = true;
        unsigned indexA = 0;
        unsigned indexB = 0;
        unsigned counter = 0;
        for ( const auto &sourceIndex : sources )
        {
            unsigned sourceNeuron = sourceIndex._neuron;
            double sourceLb = sourceLayer->getLb( sourceNeuron );
            double sourceUb = sourceLayer->getUb( sourceNeuron );

            sourceLbs.append( sourceLb );
            sourceUbs.append( sourceUb );

            if ( !sourceLayer->neuronEliminated( sourceNeuron ) )
            {
                allConstant = false;
            }
            else
            {
                double sourceValue = sourceLayer->getEliminatedNeuronValue( sourceNeuron );
                sourceValues.append( sourceValue );
            }

            if ( counter == 0 )
            {
                indexA = sourceIndex._neuron;
            }
            else
            {
                indexB = sourceIndex._neuron;
            }
            ++counter;
        }

        if ( allConstant )
        {
            // If the both source neurons have been eliminated, this neuron is constant
            for ( unsigned j = 0; j < _inputLayerSize; ++j )
            {
                _symbolicUb[j * _size + i] = 0;
                _symbolicLb[j * _size + i] = 0;
            }

            _symbolicUpperBias[i] = sourceValues[0] * sourceValues[1];
            _symbolicLowerBias[i] = sourceValues[0] * sourceValues[1];
            continue;
        }

        for ( unsigned j = 0; j < _inputLayerSize; ++j )
        {
            _symbolicUb[j * _size + i] = 0;
            _symbolicLb[j * _size + i] = 0;
        }

        // Symbolic lower bound:
        // out >= alpha * x + beta * y + gamma
        // where alpha = lb_y, beta = lb_x, gamma = -lb_x * lb_y

        // Symbolic upper bound:
        // out <= alpha * x + beta * y + gamma
        // where alpha = ub_y, beta = lb_x, gamma = -lb_x * ub_y
        for ( unsigned j = 0; j < _inputLayerSize; ++j )
        {
            if ( sourceLbs[1] >= 0 )
            {
                _symbolicLb[j * _size + i] +=
                    sourceLbs[1] * sourceSymbolicLb[j * sourceLayerSize + indexA];
            }
            else
            {
                _symbolicLb[j * _size + i] +=
                    sourceLbs[1] * sourceSymbolicUb[j * sourceLayerSize + indexA];
            }

            if ( sourceUbs[1] >= 0 )
            {
                _symbolicUb[j * _size + i] +=
                    sourceUbs[1] * sourceSymbolicUb[j * sourceLayerSize + indexA];
            }
            else
            {
                _symbolicLb[j * _size + i] +=
                    sourceUbs[1] * sourceSymbolicLb[j * sourceLayerSize + indexA];
            }

            if ( sourceLbs[0] >= 0 )
            {
                _symbolicLb[j * _size + i] +=
                    sourceLbs[0] * sourceSymbolicLb[j * sourceLayerSize + indexB];
                _symbolicUb[j * _size + i] +=
                    sourceLbs[0] * sourceSymbolicUb[j * sourceLayerSize + indexB];
            }
            else
            {
                _symbolicLb[j * _size + i] +=
                    sourceLbs[0] * sourceSymbolicUb[j * sourceLayerSize + indexB];
                _symbolicUb[j * _size + i] +=
                    sourceLbs[0] * sourceSymbolicLb[j * sourceLayerSize + indexB];
            }
        }
        _symbolicLowerBias[i] = -sourceLbs[0] * sourceLbs[1];
        _symbolicUpperBias[i] = -sourceLbs[0] * sourceUbs[1];

        double lb = FloatUtils::infinity();
        double ub = FloatUtils::negativeInfinity();
        List<double> values = { sourceLbs[0] * sourceLbs[1],
                                sourceLbs[0] * sourceUbs[1],
                                sourceUbs[0] * sourceLbs[1],
                                sourceUbs[0] * sourceUbs[1] };
        for ( const auto &v : values )
        {
            if ( v < lb )
                lb = v;
            if ( v > ub )
                ub = v;
        }

        /*
          We now have the symbolic representation for the current
          layer. Next, we compute new lower and upper bounds for
          it. For each of these bounds, we compute an upper bound and
          a lower bound.
        */
        _symbolicLbOfLb[i] = _symbolicLowerBias[i];
        _symbolicUbOfLb[i] = _symbolicLowerBias[i];
        _symbolicLbOfUb[i] = _symbolicUpperBias[i];
        _symbolicUbOfUb[i] = _symbolicUpperBias[i];

        for ( unsigned j = 0; j < _inputLayerSize; ++j )
        {
            double inputLb = _layerOwner->getLayer( 0 )->getLb( j );
            double inputUb = _layerOwner->getLayer( 0 )->getUb( j );

            double entry = _symbolicLb[j * _size + i];

            if ( entry >= 0 )
            {
                _symbolicLbOfLb[i] += ( entry * inputLb );
                _symbolicUbOfLb[i] += ( entry * inputUb );
            }
            else
            {
                _symbolicLbOfLb[i] += ( entry * inputUb );
                _symbolicUbOfLb[i] += ( entry * inputLb );
            }

            entry = _symbolicUb[j * _size + i];

            if ( entry >= 0 )
            {
                _symbolicLbOfUb[i] += ( entry * inputLb );
                _symbolicUbOfUb[i] += ( entry * inputUb );
            }
            else
            {
                _symbolicLbOfUb[i] += ( entry * inputUb );
                _symbolicUbOfUb[i] += ( entry * inputLb );
            }
        }

        /*
          We now have the tightest bounds we can for the relu
          variable. If they are tigheter than what was previously
          known, store them.
        */
        if ( _lb[i] < _symbolicLbOfLb[i] )
        {
            _lb[i] = _symbolicLbOfLb[i];
            _layerOwner->receiveTighterBound(
                Tightening( _neuronToVariable[i], _lb[i], Tightening::LB ) );
        }

        if ( _ub[i] > _symbolicUbOfUb[i] )
        {
            _ub[i] = _symbolicUbOfUb[i];
            _layerOwner->receiveTighterBound(
                Tightening( _neuronToVariable[i], _ub[i], Tightening::UB ) );
        }
    }
}

void Layer::computeSymbolicBoundsForWeightedSum()
{
    std::fill_n( _symbolicLb, _size * _inputLayerSize, 0 );
    std::fill_n( _symbolicUb, _size * _inputLayerSize, 0 );

    for ( unsigned i = 0; i < _size; ++i )
    {
        if ( _eliminatedNeurons.exists( i ) )
        {
            _symbolicLowerBias[i] = _eliminatedNeurons[i];
            _symbolicUpperBias[i] = _eliminatedNeurons[i];

            _symbolicLbOfLb[i] = _eliminatedNeurons[i];
            _symbolicUbOfLb[i] = _eliminatedNeurons[i];
            _symbolicLbOfUb[i] = _eliminatedNeurons[i];
            _symbolicUbOfUb[i] = _eliminatedNeurons[i];
        }
        else
        {
            _symbolicLowerBias[i] = _bias[i];
            _symbolicUpperBias[i] = _bias[i];
        }
    }

    for ( const auto &sourceLayerEntry : _sourceLayers )
    {
        unsigned sourceLayerIndex = sourceLayerEntry.first;
        unsigned sourceLayerSize = sourceLayerEntry.second;
        const Layer *sourceLayer = _layerOwner->getLayer( sourceLayerEntry.first );

        /*
          Perform the multiplication

          newUB = oldUB * posWeights + oldLB * negWeights
          newLB = oldUB * negWeights + oldLB * posWeights
        */

        matrixMultiplication( sourceLayer->getSymbolicUb(),
                              _layerToPositiveWeights[sourceLayerIndex],
                              _symbolicUb,
                              _inputLayerSize,
                              sourceLayerSize,
                              _size );
        matrixMultiplication( sourceLayer->getSymbolicLb(),
                              _layerToNegativeWeights[sourceLayerIndex],
                              _symbolicUb,
                              _inputLayerSize,
                              sourceLayerSize,
                              _size );
        matrixMultiplication( sourceLayer->getSymbolicLb(),
                              _layerToPositiveWeights[sourceLayerIndex],
                              _symbolicLb,
                              _inputLayerSize,
                              sourceLayerSize,
                              _size );
        matrixMultiplication( sourceLayer->getSymbolicUb(),
                              _layerToNegativeWeights[sourceLayerIndex],
                              _symbolicLb,
                              _inputLayerSize,
                              sourceLayerSize,
                              _size );

        // Restore the zero bound on eliminated neurons
        unsigned index;
        for ( const auto &eliminated : _eliminatedNeurons )
        {
            for ( unsigned i = 0; i < _inputLayerSize; ++i )
            {
                index = i * _size + eliminated.first;
                _symbolicLb[index] = 0;
                _symbolicUb[index] = 0;
            }
        }

        /*
          Compute the biases for the new layer
        */
        for ( unsigned j = 0; j < _size; ++j )
        {
            if ( _eliminatedNeurons.exists( j ) )
                continue;

            // Add the weighted bias from the source layer
            for ( unsigned k = 0; k < sourceLayerSize; ++k )
            {
                double weight = _layerToWeights[sourceLayerIndex][k * _size + j];

                if ( weight > 0 )
                {
                    _symbolicLowerBias[j] += sourceLayer->getSymbolicLowerBias()[k] * weight;
                    _symbolicUpperBias[j] += sourceLayer->getSymbolicUpperBias()[k] * weight;
                }
                else
                {
                    _symbolicLowerBias[j] += sourceLayer->getSymbolicUpperBias()[k] * weight;
                    _symbolicUpperBias[j] += sourceLayer->getSymbolicLowerBias()[k] * weight;
                }
            }
        }
    }


    /*
      We now have the symbolic representation for the current
      layer. Next, we compute new lower and upper bounds for
      it. For each of these bounds, we compute an upper bound and
      a lower bound.
    */
    for ( unsigned i = 0; i < _size; ++i )
    {
        if ( _eliminatedNeurons.exists( i ) )
            continue;

        _symbolicLbOfLb[i] = _symbolicLowerBias[i];
        _symbolicUbOfLb[i] = _symbolicLowerBias[i];
        _symbolicLbOfUb[i] = _symbolicUpperBias[i];
        _symbolicUbOfUb[i] = _symbolicUpperBias[i];

        for ( unsigned j = 0; j < _inputLayerSize; ++j )
        {
            double inputLb = _layerOwner->getLayer( 0 )->getLb( j );
            double inputUb = _layerOwner->getLayer( 0 )->getUb( j );

            double entry = _symbolicLb[j * _size + i];

            if ( entry >= 0 )
            {
                _symbolicLbOfLb[i] += ( entry * inputLb );
                _symbolicUbOfLb[i] += ( entry * inputUb );
            }
            else
            {
                _symbolicLbOfLb[i] += ( entry * inputUb );
                _symbolicUbOfLb[i] += ( entry * inputLb );
            }

            entry = _symbolicUb[j * _size + i];

            if ( entry >= 0 )
            {
                _symbolicLbOfUb[i] += ( entry * inputLb );
                _symbolicUbOfUb[i] += ( entry * inputUb );
            }
            else
            {
                _symbolicLbOfUb[i] += ( entry * inputUb );
                _symbolicUbOfUb[i] += ( entry * inputLb );
            }
        }

        /*
          We now have the tightest bounds we can for the
          weighted sum variable. If they are tigheter than
          what was previously known, store them.
        */
        if ( _lb[i] < _symbolicLbOfLb[i] )
        {
            _lb[i] = _symbolicLbOfLb[i];
            _layerOwner->receiveTighterBound(
                Tightening( _neuronToVariable[i], _lb[i], Tightening::LB ) );
        }

        if ( _ub[i] > _symbolicUbOfUb[i] )
        {
            _ub[i] = _symbolicUbOfUb[i];
            _layerOwner->receiveTighterBound(
                Tightening( _neuronToVariable[i], _ub[i], Tightening::UB ) );
        }
    }
}

double Layer::softmaxLSELowerBound( const Vector<double> &inputs,
                                    const Vector<double> &inputLbs,
                                    const Vector<double> &inputUbs,
                                    unsigned i )
{
    double sum = 0;
    for ( unsigned j = 0; j < inputs.size(); ++j )
    {
        double lj = inputLbs[j];
        double uj = inputUbs[j];
        double xj = inputs[j];
        sum +=
            ( uj - xj ) / ( uj - lj ) * std::exp( lj ) + ( xj - lj ) / ( uj - lj ) * std::exp( uj );
    }

    return std::exp( inputs[i] ) / sum;
}

double Layer::softmaxdLSELowerBound( const Vector<double> &inputMids,
                                     const Vector<double> &inputLbs,
                                     const Vector<double> &inputUbs,
                                     unsigned i,
                                     unsigned di )
{
    double val = 0;
    if ( i == di )
        val += softmaxLSELowerBound( inputMids, inputLbs, inputUbs, i );

    double ldi = inputLbs[di];
    double udi = inputUbs[di];

    double sum = 0;
    for ( unsigned j = 0; j < inputMids.size(); ++j )
    {
        double lj = inputLbs[j];
        double uj = inputUbs[j];
        double xj = inputMids[j];

        sum +=
            ( uj - xj ) / ( uj - lj ) * std::exp( lj ) + ( xj - lj ) / ( uj - lj ) * std::exp( uj );
    }

    val -= std::exp( inputMids[i] ) / ( sum * sum ) * ( std::exp( udi ) - std::exp( ldi ) ) /
           ( udi - ldi );

    return val;
}

double Layer::softmaxLSELowerBound2( const Vector<double> &inputMids,
                                     const Vector<double> &inputLbs,
                                     const Vector<double> &inputUbs,
                                     unsigned i )
{
    double max = FloatUtils::negativeInfinity();
    unsigned maxInputIndex = 0;
    unsigned index = 0;
    for ( const auto &mid : inputMids )
    {
        if ( mid > max )
        {
            max = mid;
            maxInputIndex = index;
        }
        ++index;
    }

    if ( maxInputIndex == i )
        return softmaxERLowerBound( inputMids, inputLbs, inputUbs, i );
    else
    {
        double sum = 0;
        for ( unsigned j = 0; j < inputMids.size(); ++j )
        {
            if ( j == maxInputIndex )
                sum += 1;
            else
            {
                double ljjstar = inputLbs[j] - inputUbs[maxInputIndex];
                double ujjstar = inputUbs[j] - inputLbs[maxInputIndex];
                double xjjstar = inputMids[j] - inputMids[maxInputIndex];

                sum += ( ujjstar - xjjstar ) / ( ujjstar - ljjstar ) * std::exp( ljjstar ) +
                       ( xjjstar - ljjstar ) / ( ujjstar - ljjstar ) * std::exp( ujjstar );
            }
        }

        return std::exp( inputMids[i] - inputMids[maxInputIndex] ) / sum;
    }
}

double Layer::softmaxdLSELowerBound2( const Vector<double> &inputMids,
                                      const Vector<double> &inputLbs,
                                      const Vector<double> &inputUbs,
                                      unsigned i,
                                      unsigned di )
{
    double max = FloatUtils::negativeInfinity();
    unsigned maxInputIndex = 0;
    unsigned index = 0;
    for ( const auto &mid : inputMids )
    {
        if ( mid > max )
        {
            max = mid;
            maxInputIndex = index;
        }
        ++index;
    }

    if ( maxInputIndex == i )
        return softmaxdERLowerBound( inputMids, inputLbs, inputUbs, i, di );
    else
    {
        double val = softmaxLSELowerBound2( inputMids, inputLbs, inputUbs, i );

        double sum = 0;
        for ( unsigned j = 0; j < inputMids.size(); ++j )
        {
            if ( j == maxInputIndex )
                sum += 1;
            else
            {
                double ljjstar = inputLbs[j] - inputUbs[maxInputIndex];
                double ujjstar = inputUbs[j] - inputLbs[maxInputIndex];
                double xjjstar = inputMids[j] - inputMids[maxInputIndex];
                sum += ( ujjstar - xjjstar ) / ( ujjstar - ljjstar ) * std::exp( ljjstar ) +
                       ( xjjstar - ljjstar ) / ( ujjstar - ljjstar ) * std::exp( ujjstar );
            }
        }
        double val2 = std::exp( inputMids[i] - inputMids[maxInputIndex] ) / ( sum * sum );

        if ( i == di )
        {
            double ldijstar = inputLbs[i] - inputUbs[maxInputIndex];
            double udijstar = inputUbs[i] - inputLbs[maxInputIndex];
            return val -
                   val2 * ( std::exp( udijstar ) - std::exp( ldijstar ) ) / ( udijstar - ldijstar );
        }
        else if ( maxInputIndex == di )
        {
            double sum2 = 0;
            for ( unsigned j = 0; j < inputMids.size(); ++j )
            {
                if ( j == maxInputIndex )
                    continue;
                else
                {
                    double ljjstar = inputLbs[j] - inputUbs[maxInputIndex];
                    double ujjstar = inputUbs[j] - inputLbs[maxInputIndex];
                    sum2 += ( std::exp( ujjstar ) - std::exp( ljjstar ) ) / ( ujjstar - ljjstar );
                }
            }
            return -val + val2 * sum2;
        }
        else
        {
            double ldijstar = inputLbs[di] - inputUbs[maxInputIndex];
            double udijstar = inputUbs[di] - inputLbs[maxInputIndex];
            return -val2 * ( std::exp( udijstar ) - std::exp( ldijstar ) ) /
                   ( udijstar - ldijstar );
        }
    }
}

double Layer::softmaxLSEUpperBound( const Vector<double> &inputs,
                                    const Vector<double> &outputLb,
                                    const Vector<double> &outputUb,
                                    unsigned i )
{
    double li = outputLb[i];
    double ui = outputUb[i];

    Vector<double> inputTilda;
    SoftmaxConstraint::xTilda( inputs, inputs[i], inputTilda );

    return ( ( li * std::log( ui ) - ui * std::log( li ) ) / ( std::log( ui ) - std::log( li ) ) -
             ( ui - li ) / ( std::log( ui ) - std::log( li ) ) *
                 SoftmaxConstraint::logSumOfExponential( inputTilda ) );
}

double Layer::softmaxdLSEUpperbound( const Vector<double> &inputMids,
                                     const Vector<double> &outputLb,
                                     const Vector<double> &outputUb,
                                     unsigned i,
                                     unsigned di )
{
    double li = outputLb[i];
    double ui = outputUb[i];

    double val = -( ui - li ) / ( std::log( ui ) - std::log( li ) );

    double val2 = std::exp( inputMids[di] ) / SoftmaxConstraint::sumOfExponential( inputMids );
    if ( i == di )
        val2 -= 1;

    return val * val2;
}

double Layer::softmaxERLowerBound( const Vector<double> &inputs,
                                   const Vector<double> &inputLbs,
                                   const Vector<double> &inputUbs,
                                   unsigned i )
{
    Vector<double> inputTilda;
    SoftmaxConstraint::xTilda( inputs, inputs[i], inputTilda );

    double sum = 0;
    for ( unsigned j = 0; j < inputs.size(); ++j )
    {
        if ( i == j )
            sum += 1;
        else
        {
            double ljTilda = inputLbs[j] - inputUbs[i];
            double ujTilda = inputUbs[j] - inputLbs[i];
            double xjTilda = inputTilda[j];

            sum += ( ujTilda - xjTilda ) / ( ujTilda - ljTilda ) * std::exp( ljTilda ) +
                   ( xjTilda - ljTilda ) / ( ujTilda - ljTilda ) * std::exp( ujTilda );
        }
    }

    return 1 / sum;
}

double Layer::softmaxdERLowerBound( const Vector<double> &inputMids,
                                    const Vector<double> &inputLbs,
                                    const Vector<double> &inputUbs,
                                    unsigned i,
                                    unsigned di )
{
    double val = softmaxERLowerBound( inputMids, inputLbs, inputUbs, i );

    if ( i != di )
    {
        double ldiTilda = inputLbs[di] - inputUbs[i];
        double udiTilda = inputUbs[di] - inputLbs[i];
        return -val * val * ( std::exp( udiTilda ) - std::exp( ldiTilda ) ) /
               ( udiTilda - ldiTilda );
    }
    else
    {
        double val2 = 0;
        for ( unsigned j = 0; j < inputMids.size(); ++j )
        {
            if ( j != i )
            {
                double ljTilda = inputLbs[j] - inputUbs[i];
                double ujTilda = inputUbs[j] - inputLbs[i];
                val2 += ( std::exp( ujTilda ) - std::exp( ljTilda ) ) / ( ujTilda - ljTilda );
            }
        }
        return val * val * val2;
    }
}

double Layer::softmaxERUpperBound( const Vector<double> &inputs,
                                   const Vector<double> &outputLb,
                                   const Vector<double> &outputUb,
                                   unsigned i )
{
    double li = outputLb[i];
    double ui = outputUb[i];

    Vector<double> inputTilda;
    SoftmaxConstraint::xTilda( inputs, inputs[i], inputTilda );

    return ui + li - ui * li * SoftmaxConstraint::sumOfExponential( inputTilda );
}

double Layer::softmaxdERUpperBound( const Vector<double> &inputMids,
                                    const Vector<double> &outputLb,
                                    const Vector<double> &outputUb,
                                    unsigned i,
                                    unsigned di )
{
    double li = outputLb[i];
    double ui = outputUb[i];


    if ( i == di )
    {
        double val2 = -1;
        for ( unsigned j = 0; j < inputMids.size(); ++j )
            val2 += std::exp( inputMids[j] - inputMids[i] );
        return li * ui * val2;
    }
    else
        return -li * ui * std::exp( inputMids[di] - inputMids[i] );
}

double Layer::softmaxLinearLowerBound( const Vector<double> &inputLbs,
                                       const Vector<double> &inputUbs,
                                       unsigned i )
{
    Vector<double> uTilda;
    SoftmaxConstraint::xTilda( inputUbs, inputLbs[i], uTilda );
    uTilda[i] = 0;
    return 1 / SoftmaxConstraint::sumOfExponential( uTilda );
}

double Layer::softmaxLinearUpperBound( const Vector<double> &inputLbs,
                                       const Vector<double> &inputUbs,
                                       unsigned i )
{
    Vector<double> lTilda;
    SoftmaxConstraint::xTilda( inputLbs, inputUbs[i], lTilda );
    lTilda[i] = 0;
    return 1 / SoftmaxConstraint::sumOfExponential( lTilda );
}

void Layer::eliminateVariable( unsigned variable, double value )
{
    if ( !_variableToNeuron.exists( variable ) )
        return;

    ASSERT( _type != INPUT );

    unsigned neuron = _variableToNeuron[variable];
    _eliminatedNeurons[neuron] = value;
    _lb[neuron] = value;
    _ub[neuron] = value;
    _neuronToVariable.erase( _variableToNeuron[variable] );
    _variableToNeuron.erase( variable );
}

void Layer::updateVariableIndices( const Map<unsigned, unsigned> &oldIndexToNewIndex,
                                   const Map<unsigned, unsigned> &mergedVariables )
{
    // First, do a pass to handle any merged variables
    auto neuronIt = _neuronToVariable.begin();
    while ( neuronIt != _neuronToVariable.end() )
    {
        unsigned variable = neuronIt->second;
        while ( mergedVariables.exists( variable ) )
        {
            neuronIt->second = mergedVariables[variable];
            variable = neuronIt->second;
        }

        ++neuronIt;
    }

    // Now handle re-indexing
    neuronIt = _neuronToVariable.begin();
    while ( neuronIt != _neuronToVariable.end() )
    {
        unsigned variable = neuronIt->second;

        if ( !oldIndexToNewIndex.exists( variable ) )
        {
            // This variable has been eliminated, remove from map
            neuronIt = _neuronToVariable.erase( neuronIt );
        }
        else
        {
            if ( oldIndexToNewIndex[variable] == variable )
            {
                // Index hasn't changed, skip
            }
            else
            {
                // Index has changed
                neuronIt->second = oldIndexToNewIndex[variable];
            }

            ++neuronIt;
            continue;
        }
    }
}

unsigned Layer::getLayerIndex() const
{
    return _layerIndex;
}

Layer::Layer( const Layer *other )
    : _bias( NULL )
    , _assignment( NULL )
    , _lb( NULL )
    , _ub( NULL )
    , _inputLayerSize( 0 )
    , _symbolicLb( NULL )
    , _symbolicUb( NULL )
    , _symbolicLowerBias( NULL )
    , _symbolicUpperBias( NULL )
    , _symbolicLbOfLb( NULL )
    , _symbolicUbOfLb( NULL )
    , _symbolicLbOfUb( NULL )
    , _symbolicUbOfUb( NULL )
{
    _layerIndex = other->_layerIndex;
    _type = other->_type;
    _size = other->_size;
    _layerOwner = other->_layerOwner;
    _alpha = other->_alpha;

    allocateMemory();

    for ( auto &sourceLayerEntry : other->_sourceLayers )
    {
        addSourceLayer( sourceLayerEntry.first, sourceLayerEntry.second );

        if ( other->_layerToWeights.exists( sourceLayerEntry.first ) )
            memcpy( _layerToWeights[sourceLayerEntry.first],
                    other->_layerToWeights[sourceLayerEntry.first],
                    sizeof( double ) * sourceLayerEntry.second * _size );

        if ( other->_layerToPositiveWeights.exists( sourceLayerEntry.first ) )
            memcpy( _layerToPositiveWeights[sourceLayerEntry.first],
                    other->_layerToPositiveWeights[sourceLayerEntry.first],
                    sizeof( double ) * sourceLayerEntry.second * _size );

        if ( other->_layerToNegativeWeights.exists( sourceLayerEntry.first ) )
            memcpy( _layerToNegativeWeights[sourceLayerEntry.first],
                    other->_layerToNegativeWeights[sourceLayerEntry.first],
                    sizeof( double ) * sourceLayerEntry.second * _size );
    }

    _successorLayers = other->_successorLayers;

    if ( other->_bias )
        memcpy( _bias, other->_bias, sizeof( double ) * _size );

    _neuronToActivationSources = other->_neuronToActivationSources;

    _neuronToVariable = other->_neuronToVariable;
    _variableToNeuron = other->_variableToNeuron;
    _eliminatedNeurons = other->_eliminatedNeurons;

    _inputLayerSize = other->_inputLayerSize;
}

const double *Layer::getSymbolicLb() const
{
    return _symbolicLb;
}

const double *Layer::getSymbolicUb() const
{
    return _symbolicUb;
}

const double *Layer::getSymbolicLowerBias() const
{
    return _symbolicLowerBias;
}

const double *Layer::getSymbolicUpperBias() const
{
    return _symbolicUpperBias;
}

double Layer::getSymbolicLbOfLb( unsigned neuron ) const
{
    return _symbolicLbOfLb[neuron];
}

double Layer::getSymbolicUbOfLb( unsigned neuron ) const
{
    return _symbolicUbOfLb[neuron];
}

double Layer::getSymbolicLbOfUb( unsigned neuron ) const
{
    return _symbolicLbOfUb[neuron];
}

double Layer::getSymbolicUbOfUb( unsigned neuron ) const
{
    return _symbolicUbOfUb[neuron];
}

void Layer::freeMemoryIfNeeded()
{
    for ( const auto &weights : _layerToWeights )
        delete[] weights.second;
    _layerToWeights.clear();

    for ( const auto &weights : _layerToPositiveWeights )
        delete[] weights.second;
    _layerToPositiveWeights.clear();

    for ( const auto &weights : _layerToNegativeWeights )
        delete[] weights.second;
    _layerToNegativeWeights.clear();

    if ( _bias )
    {
        delete[] _bias;
        _bias = NULL;
    }

    if ( _assignment )
    {
        delete[] _assignment;
        _assignment = NULL;
    }

    if ( _lb )
    {
        delete[] _lb;
        _lb = NULL;
    }

    if ( _ub )
    {
        delete[] _ub;
        _ub = NULL;
    }

    if ( _symbolicLb )
    {
        delete[] _symbolicLb;
        _symbolicLb = NULL;
    }

    if ( _symbolicUb )
    {
        delete[] _symbolicUb;
        _symbolicUb = NULL;
    }

    if ( _symbolicLowerBias )
    {
        delete[] _symbolicLowerBias;
        _symbolicLowerBias = NULL;
    }

    if ( _symbolicUpperBias )
    {
        delete[] _symbolicUpperBias;
        _symbolicUpperBias = NULL;
    }

    if ( _symbolicLbOfLb )
    {
        delete[] _symbolicLbOfLb;
        _symbolicLbOfLb = NULL;
    }

    if ( _symbolicUbOfLb )
    {
        delete[] _symbolicUbOfLb;
        _symbolicUbOfLb = NULL;
    }

    if ( _symbolicLbOfUb )
    {
        delete[] _symbolicLbOfUb;
        _symbolicLbOfUb = NULL;
    }

    if ( _symbolicUbOfUb )
    {
        delete[] _symbolicUbOfUb;
        _symbolicUbOfUb = NULL;
    }
}

String Layer::typeToString( Type type )
{
    switch ( type )
    {
    case INPUT:
        return "INPUT";
        break;

    case WEIGHTED_SUM:
        return "WEIGHTED_SUM";
        break;

    case RELU:
        return "RELU";
        break;

    case LEAKY_RELU:
        return "LEAKY_RELU";
        break;

    case SIGMOID:
        return "SIGMOID";
        break;

    case ABSOLUTE_VALUE:
        return "ABSOLUTE_VALUE";
        break;

    case MAX:
        return "MAX";
        break;

    case SIGN:
        return "SIGN";
        break;

    case ROUND:
        return "ROUND";
        break;

    case SOFTMAX:
        return "SOFTMAX";
        break;

    case BILINEAR:
        return "BILINEAR";
        break;


    default:
        return "UNKNOWN TYPE";
        break;
    }
}

void Layer::dump() const
{
    printf( "\nDumping info for layer %u (%s):\n", _layerIndex, typeToString( _type ).ascii() );
    printf( "\tNeurons:\n" );

    switch ( _type )
    {
    case INPUT:
        printf( "\t\t" );
        for ( unsigned i = 0; i < _size; ++i )
            printf( "x%u ", _neuronToVariable[i] );

        printf( "\n\n" );
        break;

    case WEIGHTED_SUM:

        for ( unsigned i = 0; i < _size; ++i )
        {
            if ( _eliminatedNeurons.exists( i ) )
            {
                printf( "\t\tNeuron %u = %+.4lf\t[ELIMINATED]\n", i, _eliminatedNeurons[i] );
                continue;
            }

            printf( "\t\tx%u = %+.4lf\n\t\t\t", _neuronToVariable[i], _bias[i] );
            for ( const auto &sourceLayerEntry : _sourceLayers )
            {
                const Layer *sourceLayer = _layerOwner->getLayer( sourceLayerEntry.first );
                for ( unsigned j = 0; j < sourceLayer->getSize(); ++j )
                {
                    double weight = _layerToWeights[sourceLayerEntry.first][j * _size + i];
                    if ( !FloatUtils::isZero( weight ) )
                    {
                        if ( sourceLayer->_neuronToVariable.exists( j ) )
                            printf( "%+.5lfx%u ", weight, sourceLayer->_neuronToVariable[j] );
                        else
                            printf( "%+.5lf", weight * sourceLayer->_eliminatedNeurons[j] );
                    }
                }
            }
            printf( "\n" );
        }

        printf( "\n" );
        break;

    case RELU:
    case ROUND:
    case LEAKY_RELU:
    case ABSOLUTE_VALUE:
    case MAX:
    case SIGN:
    case SIGMOID:
    case BILINEAR:
    case SOFTMAX:
        for ( unsigned i = 0; i < _size; ++i )
        {
            if ( _eliminatedNeurons.exists( i ) )
            {
                printf( "\t\tNeuron %u = %+.4lf\t[ELIMINATED]\n", i, _eliminatedNeurons[i] );
                continue;
            }

            printf( "\t\tSources for x%u: ", _neuronToVariable[i] );
            for ( const auto &source : _neuronToActivationSources[i] )
            {
                const Layer *sourceLayer = _layerOwner->getLayer( source._layer );
                if ( sourceLayer->_neuronToVariable.exists( source._neuron ) )
                    printf( "x%u ", sourceLayer->_neuronToVariable[source._neuron] );
                else
                    printf( "Neuron_%u (eliminated)", source._neuron );
            }

            printf( "\n" );
        }

        printf( "\n" );
        break;
    }
}

bool Layer::neuronHasVariable( unsigned neuron ) const
{
    ASSERT( _neuronToVariable.exists( neuron ) || _eliminatedNeurons.exists( neuron ) );
    return _neuronToVariable.exists( neuron );
}

unsigned Layer::neuronToVariable( unsigned neuron ) const
{
    DEBUG( {
        if ( !_neuronToVariable.exists( neuron ) )
            ASSERT( _eliminatedNeurons.exists( neuron ) );
    } )

    ASSERT( _neuronToVariable.exists( neuron ) );
    return _neuronToVariable[neuron];
}

unsigned Layer::variableToNeuron( unsigned variable ) const
{
    ASSERT( _variableToNeuron.exists( variable ) );
    return _variableToNeuron[variable];
}

bool Layer::neuronEliminated( unsigned neuron ) const
{
    return _eliminatedNeurons.exists( neuron );
}

double Layer::getEliminatedNeuronValue( unsigned neuron ) const
{
    ASSERT( _eliminatedNeurons.exists( neuron ) );
    return _eliminatedNeurons[neuron];
}

void Layer::reduceIndexFromAllMaps( unsigned startIndex )
{
    // Adjust the source layers
    Map<unsigned, unsigned> copyOfSources = _sourceLayers;
    _sourceLayers.clear();
    for ( const auto &pair : copyOfSources )
        _sourceLayers[pair.first >= startIndex ? pair.first - 1 : pair.first] = pair.second;

    // Adjust all weight maps
    adjustWeightMapIndexing( _layerToWeights, startIndex );
    adjustWeightMapIndexing( _layerToPositiveWeights, startIndex );
    adjustWeightMapIndexing( _layerToNegativeWeights, startIndex );

    // Adjust the neuron activations
    for ( auto &neuronToSources : _neuronToActivationSources )
    {
        for ( auto &source : neuronToSources.second )
        {
            if ( source._layer >= startIndex )
                --source._layer;
        }
    }
}

void Layer::adjustWeightMapIndexing( Map<unsigned, double *> &map, unsigned startIndex )
{
    Map<unsigned, double *> copyOfWeights = map;
    map.clear();
    for ( const auto &pair : copyOfWeights )
        map[pair.first >= startIndex ? pair.first - 1 : pair.first] = pair.second;
}

void Layer::reduceIndexAfterMerge( unsigned startIndex )
{
    if ( _layerIndex >= startIndex )
        --_layerIndex;
}

bool Layer::operator==( const Layer &layer ) const
{
    if ( _layerIndex != layer._layerIndex )
        return false;

    if ( _type != layer._type )
        return false;

    if ( _size != layer._size )
        return false;

    if ( _inputLayerSize != layer._inputLayerSize )
        return false;

    if ( ( _bias && !layer._bias ) || ( !_bias && layer._bias ) )
        return false;

    if ( _bias && layer._bias )
    {
        if ( std::memcmp( _bias, layer._bias, _size * sizeof( double ) ) != 0 )
            return false;
    }

    if ( _sourceLayers != layer._sourceLayers )
        return false;

    if ( !compareWeights( _layerToWeights, layer._layerToWeights ) )
        return false;

    if ( !compareWeights( _layerToPositiveWeights, layer._layerToPositiveWeights ) )
        return false;

    if ( !compareWeights( _layerToNegativeWeights, layer._layerToNegativeWeights ) )
        return false;

    return true;
}

bool Layer::compareWeights( const Map<unsigned, double *> &map,
                            const Map<unsigned, double *> &mapOfOtherLayer ) const
{
    if ( map.size() != mapOfOtherLayer.size() )
        return false;

    for ( const auto &pair : map )
    {
        unsigned key = pair.first;
        double *value = pair.second;

        if ( !mapOfOtherLayer.exists( key ) )
            return false;

        if ( std::memcmp(
                 value, mapOfOtherLayer[key], _size * _sourceLayers[key] * sizeof( double ) ) != 0 )
        {
            return false;
        }
    }

    return true;
}

unsigned Layer::getMaxVariable() const
{
    unsigned result = 0;
    for ( const auto &pair : _neuronToVariable )
        if ( pair.second > result )
            result = pair.second;

    return result;
}

void Layer::dumpBounds() const
{
    printf( "Layer %u:\n", _layerIndex );
    for ( unsigned i = 0; i < _size; ++i )
    {
        printf( "\tNeuron%u\tLB: %.4f, UB: %.4f \n", i + 1, _lb[i], _ub[i] );
    }
    printf( "\n" );
}

} // namespace NLR
