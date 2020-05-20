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
        std::fill_n( _symbolicLb, _size * _inputLayerSize, 0 );
        std::fill_n( _symbolicUb, _size * _inputLayerSize, 0 );

        std::fill_n( _symbolicLowerBias, _size, 0 );
        std::fill_n( _symbolicUpperBias, _size, 0 );

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
        // For the input layer, the bounds are just the identity polynomials
        for ( unsigned i = 0; i < _size; ++i )
        {
            _symbolicLb[_size * i + i] = 1;
            _symbolicUb[_size * i + i] = 1;
        }
    }

    void computeSymbolicBoundsForWeightedSum()
    {
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
                double lbOfLb = _symbolicLowerBias[i];
                double ubOfLb = _symbolicLowerBias[i];
                double lbOfUb = _symbolicUpperBias[i];
                double ubOfUb = _symbolicUpperBias[i];

                for ( unsigned j = 0; j < _inputLayerSize; ++j )
                {
                    double inputLb = _layerOwner->getLayer( 0 )->getLb( j );
                    double inputUb = _layerOwner->getLayer( 0 )->getUb( j );

                    double entry = _symbolicLb[j * _size + i];

                    if ( entry >= 0 )
                    {
                        lbOfLb += ( entry * inputLb );
                        ubOfLb += ( entry * inputUb );
                    }
                    else
                    {
                        lbOfLb += ( entry * inputUb );
                        ubOfLb += ( entry * inputLb );
                    }

                    entry = _symbolicUb[j * _size + i];

                    if ( entry >= 0 )
                    {
                        lbOfUb += ( entry * inputLb );
                        ubOfUb += ( entry * inputUb );
                    }
                    else
                    {
                        lbOfUb += ( entry * inputUb );
                        ubOfUb += ( entry * inputLb );
                    }
                }

                /*
                  We now have the tightest bounds we can for the
                  weighted sum variable. If they are tigheter than
                  what was previously known, store them.
                */
                if ( _lb[i] < lbOfLb )
                {
                    _lb[i] = lbOfLb;
                    _layerOwner->receiveTighterBound( Tightening( _neuronToVariable[i], _lb[i], Tightening::LB ) );
                }
                if ( _ub[i] > ubOfUb )
                {
                    _ub[i] = ubOfUb;
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
    }
};

} // namespace NLR

#endif // __Layer_h__
