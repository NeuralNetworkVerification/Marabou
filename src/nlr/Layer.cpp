/*********************                                                        */
/*! \file Layer.cpp
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

#include "Layer.h"

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
    if ( _type == WEIGHTED_SUM || _type == OUTPUT )
    {
        _bias = new double[_size];
        std::fill_n( _bias, _size, 0 );
    }

    _lb = new double[_size];
    _ub = new double[_size];

    std::fill_n( _lb, _size, 0 );
    std::fill_n( _ub, _size, 0 );

    _assignment = new double[_size];

    _inputLayerSize = ( _type == INPUT ) ? _size : _layerOwner->getLayer( 0 )->getSize();
    if ( GlobalConfiguration::USE_SYMBOLIC_BOUND_TIGHTENING )
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
    memcpy( _assignment, values, _size * sizeof(double) );
}

const double *Layer::getAssignment() const
{
    return _assignment;
}

double Layer::getAssignment( unsigned neuron ) const
{
    return _assignment[neuron];
}

void Layer::computeAssignment()
{
    ASSERT( _type != INPUT );

    if ( _type == WEIGHTED_SUM || _type == OUTPUT )
    {
        // Initialize to bias
        memcpy( _assignment, _bias, sizeof(double) * _size );

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
            double inputValue = _layerOwner->getLayer( sourceIndex._layer )->getAssignment( sourceIndex._neuron );

            _assignment[i] = FloatUtils::max( inputValue, 0 );
        }
    }

    else if ( _type == ABSOLUTE_VALUE )
    {
        for ( unsigned i = 0; i < _size; ++i )
        {
            NeuronIndex sourceIndex = *_neuronToActivationSources[i].begin();
            double inputValue = _layerOwner->getLayer( sourceIndex._layer )->getAssignment( sourceIndex._neuron );

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
                double value = _layerOwner->getLayer( input._layer )->getAssignment( input._neuron );
                if ( value > _assignment[i] )
                    _assignment[i] = value;
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

void Layer::addSourceLayer( unsigned layerNumber, unsigned layerSize )
{
    ASSERT( _type != INPUT );

    if ( _sourceLayers.exists( layerNumber ) )
        return;

    _sourceLayers[layerNumber] = layerSize;

    if ( _type == WEIGHTED_SUM || _type == OUTPUT )
    {
        _layerToWeights[layerNumber] = new double[layerSize * _size];
        _layerToPositiveWeights[layerNumber] = new double[layerSize * _size];
        _layerToNegativeWeights[layerNumber] = new double[layerSize * _size];

        std::fill_n( _layerToWeights[layerNumber], layerSize * _size, 0 );
        std::fill_n( _layerToPositiveWeights[layerNumber], layerSize * _size, 0 );
        std::fill_n( _layerToNegativeWeights[layerNumber], layerSize * _size, 0 );
    }
}

void Layer::setWeight( unsigned sourceLayer, unsigned sourceNeuron, unsigned targetNeuron, double weight )
{
    unsigned index = sourceNeuron * _size + targetNeuron;
    _layerToWeights[sourceLayer][index] = weight;

    if ( weight > 0 )
        _layerToPositiveWeights[sourceLayer][index] = weight;
    else
        _layerToNegativeWeights[sourceLayer][index] = weight;
}

void Layer::setBias( unsigned neuron, double bias )
{
    _bias[neuron] = bias;
}

void Layer::addActivationSource( unsigned sourceLayer, unsigned sourceNeuron, unsigned targetNeuron )
{
    ASSERT( _type == RELU || _type == ABSOLUTE_VALUE || _type == MAX );

    if ( !_neuronToActivationSources.exists( targetNeuron ) )
        _neuronToActivationSources[targetNeuron] = List<NeuronIndex>();

    _neuronToActivationSources[targetNeuron].append( NeuronIndex( sourceLayer, sourceNeuron ) );

    DEBUG({
            if ( _type == RELU || _type == ABSOLUTE_VALUE )
                ASSERT( _neuronToActivationSources[targetNeuron].size() == 1 );
        });
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

void Layer::computeIntervalArithmeticBounds()
{
    ASSERT( _type != INPUT );

    switch ( _type )
    {
    case WEIGHTED_SUM:
    case OUTPUT:
        computeIntervalArithmeticBoundsForWeightedSum();
        break;

    case RELU:
        computeIntervalArithmeticBoundsForRelu();
        break;

    case ABSOLUTE_VALUE:
        computeIntervalArithmeticBoundsForAbs();
        break;

    case MAX:

    default:
        printf( "Error! Actiation type %u unsupported\n", _type );
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

        if ( newLb[i] > _lb[i] )
        {
            _lb[i] = newLb[i];
            _layerOwner->receiveTighterBound( Tightening( _neuronToVariable[i], _lb[i], Tightening::LB ) );
        }
        if ( newUb[i] < _ub[i] )
        {
            _ub[i] = newUb[i];
            _layerOwner->receiveTighterBound( Tightening( _neuronToVariable[i], _ub[i], Tightening::UB ) );
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

        if ( lb > _lb[i] )
        {
            _lb[i] = lb;
            _layerOwner->receiveTighterBound( Tightening( _neuronToVariable[i], _lb[i], Tightening::LB ) );
        }
        if ( ub < _ub[i] )
        {
            _ub[i] = ub;
            _layerOwner->receiveTighterBound( Tightening( _neuronToVariable[i], _ub[i], Tightening::UB ) );
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
            if ( lb > _lb[i] )
            {
                _lb[i] = lb;
                _layerOwner->receiveTighterBound( Tightening( _neuronToVariable[i], _lb[i], Tightening::LB ) );
            }
            if ( ub < _ub[i] )
            {
                _ub[i] = ub;
                _layerOwner->receiveTighterBound( Tightening( _neuronToVariable[i], _ub[i], Tightening::UB ) );
            }
        }
        else if ( ub < 0 )
        {
            if ( -ub > _lb[i] )
            {
                _lb[i] = -ub;
                _layerOwner->receiveTighterBound( Tightening( _neuronToVariable[i], _lb[i], Tightening::LB ) );
            }
            if ( -lb < _ub[i] )
            {
                _ub[i] = -lb;
                _layerOwner->receiveTighterBound( Tightening( _neuronToVariable[i], _ub[i], Tightening::UB ) );
            }
        }
        else
        {
            // lb < 0 < ub
            if ( _lb[i] < 0 )
            {
                _lb[i] = 0;
                _layerOwner->receiveTighterBound( Tightening( _neuronToVariable[i], _lb[i], Tightening::LB ) );
            }

            if ( FloatUtils::max( ub, -lb ) < _ub[i] )
            {
                _ub[i] = FloatUtils::max( ub, -lb );
                _layerOwner->receiveTighterBound( Tightening( _neuronToVariable[i], _ub[i], Tightening::UB ) );
            }
        }
    }
}

void Layer::computeSymbolicBounds()
{
    switch ( _type )
    {

    case INPUT:
        comptueSymbolicBoundsForInput();
        break;

    case WEIGHTED_SUM:
    case OUTPUT:
        computeSymbolicBoundsForWeightedSum();
        break;

    case RELU:
        computeSymbolicBoundsForRelu();
        break;

    case ABSOLUTE_VALUE:
        computeSymbolicBoundsForAbsoluteValue();
        break;

    case MAX:

    default:
        printf( "Error! Actiation type %u unsupported\n", _type );
        throw MarabouError( MarabouError::NETWORK_LEVEL_REASONER_ACTIVATION_NOT_SUPPORTED );
        break;
    }
}

void Layer::comptueSymbolicBoundsForInput()
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
        ReluConstraint::PhaseStatus reluPhase = ReluConstraint::PHASE_NOT_FIXED;

        // Has the f variable been eliminated or fixed?
        if ( FloatUtils::isPositive( _lb[i] ) )
            reluPhase = ReluConstraint::PHASE_ACTIVE;
        else if ( FloatUtils::isZero( _ub[i] ) )
            reluPhase = ReluConstraint::PHASE_INACTIVE;

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
            _symbolicLb[j * _size + i] = sourceSymbolicLb[j * sourceLayerSize + sourceIndex._neuron];
            _symbolicUb[j * _size + i] = sourceSymbolicUb[j * sourceLayerSize + sourceIndex._neuron];
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
            reluPhase = ReluConstraint::PHASE_ACTIVE;
        }
        else if ( !FloatUtils::isPositive( sourceUb ) )
        {
            reluPhase = ReluConstraint::PHASE_INACTIVE;
        }

        if ( reluPhase == ReluConstraint::PHASE_NOT_FIXED )
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
                    _symbolicUb[j * _size + i] =
                        _symbolicUb[j * _size + i] * _symbolicUbOfUb[i] / ( _symbolicUbOfUb[i] - _symbolicLbOfUb[i] );

                // Do the same for the bias, and then adjust
                _symbolicUpperBias[i] = _symbolicUpperBias[i] * _symbolicUbOfUb[i] / ( _symbolicUbOfUb[i] - _symbolicLbOfUb[i] );
                _symbolicUpperBias[i] -= _symbolicLbOfUb[i] * _symbolicUbOfUb[i] / ( _symbolicUbOfUb[i] - _symbolicLbOfUb[i] );
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
                    _symbolicLb[j * _size + i] =
                        _symbolicLb[j * _size + i] * _symbolicUbOfLb[i] / ( _symbolicUbOfLb[i] - _symbolicLbOfLb[i] );

                _symbolicLowerBias[i] = _symbolicLowerBias[i] * _symbolicUbOfLb[i] / ( _symbolicUbOfLb[i] - _symbolicLbOfLb[i] );
            }

            _symbolicLbOfLb[i] = 0;
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
            _layerOwner->receiveTighterBound( Tightening( _neuronToVariable[i], _lb[i], Tightening::LB ) );
        }

        if ( _ub[i] > _symbolicUbOfUb[i] )
        {
            _ub[i] = _symbolicUbOfUb[i];
            _layerOwner->receiveTighterBound( Tightening( _neuronToVariable[i], _ub[i], Tightening::UB ) );
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

        AbsoluteValueConstraint::PhaseStatus absPhase = AbsoluteValueConstraint::PHASE_NOT_FIXED;

        ASSERT( _neuronToActivationSources.exists( i ) );
        NeuronIndex sourceIndex = *_neuronToActivationSources[i].begin();
        const Layer *sourceLayer = _layerOwner->getLayer( sourceIndex._layer );

        unsigned sourceLayerSize = sourceLayer->getSize();
        const double *sourceSymbolicLb = sourceLayer->getSymbolicLb();
        const double *sourceSymbolicUb = sourceLayer->getSymbolicUb();

        for ( unsigned j = 0; j < _inputLayerSize; ++j )
        {
            _symbolicLb[j * _size + i] = sourceSymbolicLb[j * sourceLayerSize + sourceIndex._neuron];
            _symbolicUb[j * _size + i] = sourceSymbolicUb[j * sourceLayerSize + sourceIndex._neuron];
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
            absPhase = AbsoluteValueConstraint::PHASE_POSITIVE;
        else if ( sourceUb <= 0 )
            absPhase = AbsoluteValueConstraint::PHASE_NEGATIVE;

        if ( absPhase == AbsoluteValueConstraint::PHASE_NOT_FIXED )
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
            if ( absPhase == AbsoluteValueConstraint::PHASE_POSITIVE )
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
            _layerOwner->receiveTighterBound( Tightening( _neuronToVariable[i], _lb[i], Tightening::LB ) );
        }

        if ( _ub[i] > _symbolicUbOfUb[i] )
        {
            _ub[i] = _symbolicUbOfUb[i];
            _layerOwner->receiveTighterBound( Tightening( _neuronToVariable[i], _ub[i], Tightening::UB ) );
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
    }

    for ( const auto &sourceLayerEntry : _sourceLayers )
    {
        unsigned sourceLayerIndex = sourceLayerEntry.first;
        unsigned sourceLayerSize = sourceLayerEntry.second;
        const Layer *sourceLayer = _layerOwner->getLayer( sourceLayerEntry.first );

        /*
          Perform the multiplication, don't change pre calucalted bounds for
          eliminated variables

          newUB = oldUB * posWeights + oldLB * negWeights
          newLB = oldUB * negWeights + oldLB * posWeights
        */

        Map<unsigned, double> _eliminatedNeuronsLB;
        Map<unsigned, double> _eliminatedNeuronsUB;
        unsigned index;
        for ( const auto &eliminated : _eliminatedNeurons ) {
                for ( unsigned i = 0; i < _inputLayerSize; ++i ) {
                    index = i * _size + eliminated.first;
                    printf("_symbolicLB[%u] = %f\n", index, _symbolicLb[index]);
                     _eliminatedNeuronsLB[index] = _symbolicLb[index];
                     _eliminatedNeuronsUB[index] = _symbolicUb[index];
                }
        }

        matrixMultiplication( sourceLayer->getSymbolicUb(), _layerToPositiveWeights[sourceLayerIndex],
                              _symbolicUb, _inputLayerSize,
                              sourceLayerSize, _size);
        matrixMultiplication( sourceLayer->getSymbolicLb(), _layerToNegativeWeights[sourceLayerIndex],
                              _symbolicUb, _inputLayerSize,
                              sourceLayerSize, _size);
        matrixMultiplication( sourceLayer->getSymbolicLb(), _layerToPositiveWeights[sourceLayerIndex],
                              _symbolicLb, _inputLayerSize,
                              sourceLayerSize, _size);
        matrixMultiplication( sourceLayer->getSymbolicUb(), _layerToNegativeWeights[sourceLayerIndex],
                              _symbolicLb, _inputLayerSize, sourceLayerSize,
                              _size);

        for ( const auto &eliminatedLB : _eliminatedNeuronsLB) {
            _symbolicLb[eliminatedLB.first] = eliminatedLB.second;
        }
        for ( const auto &eliminatedUB : _eliminatedNeuronsUB) {
            _symbolicUb[eliminatedUB.first] = eliminatedUB.second;
        }

        /*
          Compute the biases for the new layer
        */
        for ( unsigned j = 0; j < _size; ++j )
        {
            if ( _eliminatedNeurons.exists( j ) )
                continue;

            _symbolicLowerBias[j] = _bias[j];
            _symbolicUpperBias[j] = _bias[j];

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
                _layerOwner->receiveTighterBound( Tightening( _neuronToVariable[i], _lb[i], Tightening::LB ) );
            }

            if ( _ub[i] > _symbolicUbOfUb[i] )
            {
                _ub[i] = _symbolicUbOfUb[i];
                _layerOwner->receiveTighterBound( Tightening( _neuronToVariable[i], _ub[i], Tightening::UB ) );
            }
        }
    }
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

    allocateMemory();

    for ( auto &sourceLayerEntry : other->_sourceLayers )
    {
        addSourceLayer( sourceLayerEntry.first, sourceLayerEntry.second );

        if ( other->_layerToWeights.exists( sourceLayerEntry.first ) )
            memcpy( _layerToWeights[sourceLayerEntry.first],
                    other->_layerToWeights[sourceLayerEntry.first],
                    sizeof(double) * sourceLayerEntry.second * _size );

        if ( other->_layerToPositiveWeights.exists( sourceLayerEntry.first ) )
            memcpy( _layerToPositiveWeights[sourceLayerEntry.first],
                    other->_layerToPositiveWeights[sourceLayerEntry.first],
                    sizeof(double) * sourceLayerEntry.second * _size );

        if ( other->_layerToNegativeWeights.exists( sourceLayerEntry.first ) )
            memcpy( _layerToNegativeWeights[sourceLayerEntry.first],
                    other->_layerToNegativeWeights[sourceLayerEntry.first],
                    sizeof(double) * sourceLayerEntry.second * _size );
    }

    if ( other->_bias )
        memcpy( _bias, other->_bias, sizeof(double) * _size );

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

    case OUTPUT:
        return "OUTPUT";
        break;

    case WEIGHTED_SUM:
        return "WEIGHTED_SUM";
        break;

    case RELU:
        return "RELU";
        break;

    case ABSOLUTE_VALUE:
        return "ABSOLUTE_VALUE";
        break;

    case MAX:
        return "MAX";
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

    case OUTPUT:
    case WEIGHTED_SUM:

        for ( unsigned i = 0; i < _size; ++i )
        {
            if ( _eliminatedNeurons.exists( i ) )
            {
                printf( "\t\tNeuron %u = %+.4lf\t[ELIMINATED]\n", i, _eliminatedNeurons[i] );
                continue;
            }

            printf( "\t\tx%u = %+.4lf\n", _neuronToVariable[i], _bias[i] );
            for ( const auto &sourceLayerEntry : _sourceLayers )
            {
                printf( "\t\t\t" );
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
    case ABSOLUTE_VALUE:
    case MAX:

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
    DEBUG({
            if ( !_neuronToVariable.exists( neuron ) )
                ASSERT( _eliminatedNeurons.exists( neuron ) );
        })

    ASSERT( _neuronToVariable.exists( neuron ) );
    return _neuronToVariable[neuron];
}

unsigned Layer::variableToNeuron( unsigned variable ) const
{
    ASSERT( _variableToNeuron.exists( variable ) );
    return _variableToNeuron[variable];
}

} // namespace NLR
