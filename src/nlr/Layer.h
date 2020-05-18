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

namespace NLR {

class Layer
{
public:
    class LayerOwner
    {
    public:
        virtual ~LayerOwner() {}
        virtual const Layer *getLayer( unsigned index ) const = 0;
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
    {
        if ( _type == WEIGHTED_SUM || _type == OUTPUT )
        {
            _bias = new double[size];
            std::fill_n( _bias, size, 0 );
        }

        _assignment = new double[size];
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
        printf( "Compute assignment called for layer %u\n", _layerIndex );

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
                double weightedSum = _layerOwner->getLayer( sourceIndex._layer )->getAssignment( sourceIndex._neuron );

                _assignment[i] = FloatUtils::max( weightedSum, 0 );
            }
        }
        else
        {
            ASSERT( false );
        }

        printf( "\tComputed the following assignment:\n" );
        for ( unsigned i = 0; i < _size; ++i )
        {
            printf( "\t\tx%u = %lf\n", i, _assignment[i] );
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


private:
    unsigned _layerIndex;
    Type _type;
    unsigned _size;
    const LayerOwner *_layerOwner;

    Set<unsigned> _sourceLayers;

    Map<unsigned, double *> _layerToWeights;
    double *_bias;

    double *_assignment;

    Map<unsigned, List<NeuronIndex>> _neuronToActivationSources;

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
    }
};

} // namespace NLR

#endif // __Layer_h__
