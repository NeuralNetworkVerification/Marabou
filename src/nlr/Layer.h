/*********************                                                        */
/*! \file Layer.h
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

#ifndef __Layer_h__
#define __Layer_h__

#include "Debug.h"
#include "FloatUtils.h"
#include "MarabouError.h"
#include "ReluConstraint.h"

namespace NLR {

class Layer
{
public:
    class LayerOwner
    {
    public:
        virtual ~LayerOwner() {}
        virtual const Layer *getLayer( unsigned index ) const = 0;
        virtual const ITableau *getTableau() const = 0;
        virtual void receiveTighterBound( Tightening tightening ) = 0;
    };

    enum Type {
        // Linear layers
        INPUT = 0,
        OUTPUT,
        WEIGHTED_SUM,

        // Activation functions
        RELU,
        ABSOLUTE_VALUE,
        MAX,
    };

    ~Layer()
    {
        freeMemoryIfNeeded();
    }

    Layer( unsigned index, Type type, unsigned size, LayerOwner *layerOwner )
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
        if ( _type == WEIGHTED_SUM || _type == OUTPUT )
        {
            _bias = new double[size];
            std::fill_n( _bias, size, 0 );
        }

        _lb = new double[size];
        _ub = new double[size];

        std::fill_n( _lb, size, 0 );
        std::fill_n( _ub, size, 0 );

        _assignment = new double[size];

        _inputLayerSize = ( type == INPUT ) ? size : layerOwner->getLayer( 0 )->getSize();
        if ( GlobalConfiguration::USE_SYMBOLIC_BOUND_TIGHTENING )
        {
            _symbolicLb = new double[size * _inputLayerSize];
            _symbolicUb = new double[size * _inputLayerSize];

            std::fill_n( _symbolicLb, size * _inputLayerSize, 0 );
            std::fill_n( _symbolicUb, size * _inputLayerSize, 0 );

            _symbolicLowerBias = new double[size];
            _symbolicUpperBias = new double[size];

            std::fill_n( _symbolicLowerBias, size, 0 );
            std::fill_n( _symbolicUpperBias, size, 0 );

            _symbolicLbOfLb = new double[size];
            _symbolicUbOfLb = new double[size];
            _symbolicLbOfUb = new double[size];
            _symbolicUbOfUb = new double[size];

            std::fill_n( _symbolicLbOfLb, size, 0 );
            std::fill_n( _symbolicUbOfLb, size, 0 );
            std::fill_n( _symbolicLbOfUb, size, 0 );
            std::fill_n( _symbolicUbOfUb, size, 0 );
        }
    }

    void setAssignment( const double *values )
    {
        memcpy( _assignment, values, _size * sizeof(double) );
    }

    const double *getAssignment() const
    {
        return _assignment;
    }

    double getAssignment( unsigned neuron ) const
    {
        return _assignment[neuron];
    }

    void computeAssignment()
    {
        ASSERT( _type != INPUT );

        if ( _type == WEIGHTED_SUM || _type == OUTPUT )
        {
            // Initialize to bias
            memcpy( _assignment, _bias, sizeof(double) * _size );

            // Process each of the source layers
            for ( unsigned sourceLayerIndex : _sourceLayers )
            {
                const Layer *sourceLayer = _layerOwner->getLayer( sourceLayerIndex );
                const double *sourceAssignment = sourceLayer->getAssignment();
                unsigned sourceSize = sourceLayer->getSize();
                const double *weights = _layerToWeights[sourceLayerIndex];

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
    }

    void addSourceLayer( unsigned layerNumber, unsigned layerSize )
    {
        ASSERT( _type != INPUT );

        _sourceLayers.insert( layerNumber );

        if ( _type == WEIGHTED_SUM || _type == OUTPUT )
        {
            _layerToWeights[layerNumber] = new double[layerSize * _size];
            _layerToPositiveWeights[layerNumber] = new double[layerSize * _size];
            _layerToNegativeWeights[layerNumber] = new double[layerSize * _size];

            std::fill_n( _layerToWeights[layerNumber], layerSize * _size, 0 );
            std::fill_n( _layerToPositiveWeights[layerNumber], layerSize * _size, 0 );
            std::fill_n( _layerToPositiveWeights[layerNumber], layerSize * _size, 0 );
        }
    }

    void setWeight( unsigned sourceLayer, unsigned sourceNeuron, unsigned targetNeuron, double weight )
    {
        unsigned index = sourceNeuron * _size + targetNeuron;
        _layerToWeights[sourceLayer][index] = weight;

        if ( weight > 0 )
            _layerToPositiveWeights[sourceLayer][index] = weight;
        else
            _layerToNegativeWeights[sourceLayer][index] = weight;
    }

    void setBias( unsigned neuron, double bias )
    {
        _bias[neuron] = bias;
    }

    void addActivationSource( unsigned sourceLayer, unsigned sourceNeuron, unsigned targetNeuron )
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

    unsigned getSize() const
    {
        return _size;
    }

    void setNeuronVariable( unsigned neuron, unsigned variable )
    {
        _neuronToVariable[neuron] = variable;
    }

    void obtainCurrentBounds()
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
                // TODO: handle eliminated activation varibales
            }
        }
    }

    double getLb( unsigned neuron ) const
    {
        return _lb[neuron];
    }

    double getUb( unsigned neuron ) const
    {
        return _ub[neuron];
    }

    void computeSymbolicBounds()
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

            break;


        case MAX:

            break;

        default:
            printf( "Error! Actiation type %u unsupported\n", _type );
            throw MarabouError( MarabouError::NETWORK_LEVEL_REASONER_ACTIVATION_NOT_SUPPORTED );
            break;
        }
    }

    void comptueSymbolicBoundsForInput()
    {
        std::fill_n( _symbolicLb, _size * _inputLayerSize, 0 );
        std::fill_n( _symbolicUb, _size * _inputLayerSize, 0 );

        // For the input layer, the bounds are just the identity polynomials
        for ( unsigned i = 0; i < _size; ++i )
        {
            _symbolicLb[_size * i + i] = 1;
            _symbolicUb[_size * i + i] = 1;
        }
    }

    void computeSymbolicBoundsForRelu()
    {
        for ( unsigned i = 0; i < _size; ++i )
        {
            /*
              There are two ways we can determine that a ReLU has become fixed:

              1. If the ReLU's variables have been externally fixed
              2. lbLb >= 0 (ACTIVE) or ubUb <= 0 (INACTIVE)
            */
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
                _symbolicLowerBias[i] = sourceLayer->getSymbolicLowerBias()[sourceIndex._neuron];
                _symbolicUpperBias[i] = sourceLayer->getSymbolicUpperBias()[sourceIndex._neuron];
            }

            ReluConstraint::PhaseStatus reluPhase = ReluConstraint::PHASE_NOT_FIXED;

            // if ( _eliminatedWeightedSumVariables.exists( index ) )
            // {
            //     if ( _eliminatedWeightedSumVariables[index] <= 0 )
            //         reluPhase = ReluConstraint::PHASE_INACTIVE;
            //     else
            //         reluPhase = ReluConstraint::PHASE_ACTIVE;
            // }
            // else if ( _eliminatedActivationResultVariables.exists( index ) )
            // {
            //     if ( FloatUtils::isZero( _eliminatedWeightedSumVariables[index] ) )
            //         reluPhase = ReluConstraint::PHASE_INACTIVE;
            //     else
            //         reluPhase = ReluConstraint::PHASE_ACTIVE;
            // }

            double sourceLb = sourceLayer->getLb( sourceIndex._neuron );
            double sourceUb = sourceLayer->getUb( sourceIndex._neuron );

            if ( sourceLb >= 0 )
                reluPhase = ReluConstraint::PHASE_ACTIVE;
            else if ( sourceUb <= 0 )
                reluPhase = ReluConstraint::PHASE_INACTIVE;

            // TODO: copy symbolicUB and symblocLB from ws neuron, and also biases

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

                _symbolicLbOfLb[i] = sourceLayer->getSymbolicLbOfLb( sourceIndex._neuron );
                _symbolicUbOfLb[i] = sourceLayer->getSymbolicUbOfLb( sourceIndex._neuron );
                _symbolicLbOfUb[i] = sourceLayer->getSymbolicLbOfUb( sourceIndex._neuron );
                _symbolicUbOfUb[i] = sourceLayer->getSymbolicUbOfUb( sourceIndex._neuron );

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
                    _symbolicLowerBias[i] = 0;
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

    void computeSymbolicBoundsForWeightedSum()
    {
        std::fill_n( _symbolicLb, _size * _inputLayerSize, 0 );
        std::fill_n( _symbolicUb, _size * _inputLayerSize, 0 );

        for ( const auto &sourceLayerIndex : _sourceLayers )
        {
            const Layer *sourceLayer = _layerOwner->getLayer( sourceLayerIndex );
            unsigned sourceLayerSize = sourceLayer->getSize();

            /*
              Perform the multiplication.

              newUB = oldUB * posWeights + oldLB * negWeights
              newLB = oldUB * negWeights + oldLB * posWeights
            */

            for ( unsigned i = 0; i < _inputLayerSize; ++i )
            {
                for ( unsigned j = 0; j < _size; ++j )
                {
                    for ( unsigned k = 0; k < sourceLayerSize; ++k )
                    {
                        _symbolicLb[i * _size + j] +=
                            sourceLayer->getSymbolicUb()[i * sourceLayerSize + k] *
                            _layerToNegativeWeights[sourceLayerIndex][k * _size + j];

                        // _currentLayerLowerBounds[i * currentLayerSize + j] +=
                        //     _previousLayerUpperBounds[i * previousLayerSize + k] *
                        //     _negativeWeights[currentLayer - 1][k * currentLayerSize + j];

                        _symbolicLb[i * _size + j] +=
                            sourceLayer->getSymbolicLb()[i * sourceLayerSize + k] *
                            _layerToPositiveWeights[sourceLayerIndex][k * _size + j];

                        // _currentLayerLowerBounds[i * currentLayerSize + j] +=
                        //     _previousLayerLowerBounds[i * previousLayerSize + k] *
                        //     _positiveWeights[currentLayer - 1][k * currentLayerSize + j];

                        _symbolicUb[i * _size + j] +=
                            sourceLayer->getSymbolicUb()[i * sourceLayerSize + k] *
                            _layerToPositiveWeights[sourceLayerIndex][k * _size + j];

                        // _currentLayerUpperBounds[i * currentLayerSize + j] +=
                        //     _previousLayerUpperBounds[i * previousLayerSize + k] *
                        //     _positiveWeights[currentLayer - 1][k * currentLayerSize + j];

                        _symbolicUb[i * _size + j] +=
                            sourceLayer->getSymbolicLb()[i * sourceLayerSize + k] *
                            _layerToNegativeWeights[sourceLayerIndex][k * _size + j];

                        // _currentLayerUpperBounds[i * currentLayerSize + j] +=
                        //     _previousLayerLowerBounds[i * previousLayerSize + k] *
                        //     _negativeWeights[currentLayer - 1][k * currentLayerSize + j];

                    }
                }
            }

            /*
              Compute the biases for the new layer
            */
            for ( unsigned j = 0; j < _size; ++j )
            {
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
                        // _currentLayerLowerBias[j] += _previousLayerLowerBias[k] * weight;
                        // _currentLayerUpperBias[j] += _previousLayerUpperBias[k] * weight;
                    }
                    else
                    {
                        _symbolicLowerBias[j] += sourceLayer->getSymbolicUpperBias()[k] * weight;
                        _symbolicUpperBias[j] += sourceLayer->getSymbolicLowerBias()[k] * weight;

                        // _currentLayerLowerBias[j] += _previousLayerUpperBias[k] * weight;
                        // _currentLayerUpperBias[j] += _previousLayerLowerBias[k] * weight;
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

private:
    unsigned _layerIndex;
    Type _type;
    unsigned _size;
    LayerOwner *_layerOwner;

    Set<unsigned> _sourceLayers;

    Map<unsigned, double *> _layerToWeights;
    Map<unsigned, double *> _layerToPositiveWeights;
    Map<unsigned, double *> _layerToNegativeWeights;
    double *_bias;

    double *_assignment;

    double *_lb;
    double *_ub;

    Map<unsigned, List<NeuronIndex>> _neuronToActivationSources;

    Map<unsigned, unsigned> _neuronToVariable;

    unsigned _inputLayerSize;
    double *_symbolicLb;
    double *_symbolicUb;
    double *_symbolicLowerBias;
    double *_symbolicUpperBias;
    double *_symbolicLbOfLb;
    double *_symbolicUbOfLb;
    double *_symbolicLbOfUb;
    double *_symbolicUbOfUb;

    const double *getSymbolicLb() const
    {
        return _symbolicLb;
    }

    const double *getSymbolicUb() const
    {
        return _symbolicUb;
    }

    const double *getSymbolicLowerBias() const
    {
        return _symbolicLowerBias;
    }

    const double *getSymbolicUpperBias() const
    {
        return _symbolicUpperBias;
    }

    double getSymbolicLbOfLb( unsigned neuron ) const
    {
        return _symbolicLbOfLb[neuron];
    }

    double getSymbolicUbOfLb( unsigned neuron ) const
    {
        return _symbolicUbOfLb[neuron];
    }

    double getSymbolicLbOfUb( unsigned neuron ) const
    {
        return _symbolicLbOfUb[neuron];
    }

    double getSymbolicUbOfUb( unsigned neuron ) const
    {
        return _symbolicUbOfUb[neuron];
    }

    void freeMemoryIfNeeded()
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
};

} // namespace NLR

#endif // __Layer_h__
