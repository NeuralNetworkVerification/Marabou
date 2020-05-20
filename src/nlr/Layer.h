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

    Layer( unsigned index, Type type, unsigned size, const LayerOwner *layerOwner )
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
            std::fill_n( _layerToWeights[layerNumber], layerSize * _size, 0 );
        }
    }

    void setWeight( unsigned sourceLayer, unsigned sourceNeuron, unsigned targetNeuron, double weight )
    {
        _layerToWeights[sourceLayer][sourceNeuron * _size + targetNeuron] = weight;
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

    void computeSymbolicBounds()
    {
        switch ( _type )
        {

        case INPUT:

            std::fill_n( _symbolicLb, _size * _inputLayerSize, 0 );
            std::fill_n( _symbolicUb, _size * _inputLayerSize, 0 );

            // For the input layer, the bounds are just the identity polynomials
            for ( unsigned i = 0; i < _size; ++i )
            {
                _symbolicLb[_size * i + i] = 1;
                _symbolicUb[_size * i + i] = 1;
            }

            break;

        case WEIGHTED_SUM:
        case OUTPUT:

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

private:
    unsigned _layerIndex;
    Type _type;
    unsigned _size;
    const LayerOwner *_layerOwner;

    Set<unsigned> _sourceLayers;

    Map<unsigned, double *> _layerToWeights;
    double *_bias;

    double *_assignment;

    double *_lb;
    double *_ub;

    Map<unsigned, List<NeuronIndex>> _neuronToActivationSources;

    Map<unsigned, unsigned> _neuronToVariable;

    unsigned _inputLayerSize;
    double *_symbolicLb;
    double *_symbolicUb;

    void freeMemoryIfNeeded()
    {
        for ( const auto &weights : _layerToWeights )
            delete[] weights.second;
        _layerToWeights.clear();

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
    }
};

} // namespace NLR

#endif // __Layer_h__
