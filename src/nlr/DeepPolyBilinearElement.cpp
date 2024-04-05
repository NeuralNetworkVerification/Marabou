/*********************                                                        */
/*! \file DeepPolyBilinearElement.cpp
** \verbatim
** Top contributors (to current version):
**   Andrew Wu
** This file is part of the Marabou project.
** Copyright (c) 2017-2024 by the authors listed in the file AUTHORS
** in the top-level source directory) and their institutional affiliations.
** All rights reserved. See the file COPYING in the top-level source
** directory for licensing information.\endverbatim
**
** [[ Add lengthier description here ]]
**/

#include "DeepPolyBilinearElement.h"

#include "FloatUtils.h"

namespace NLR {

DeepPolyBilinearElement::DeepPolyBilinearElement( Layer *layer )
    : _symbolicLbA( NULL )
    , _symbolicUbA( NULL )
    , _symbolicLbB( NULL )
    , _symbolicUbB( NULL )
{
    _layer = layer;
    _size = layer->getSize();
    _layerIndex = layer->getLayerIndex();
}

DeepPolyBilinearElement::~DeepPolyBilinearElement()
{
    freeMemoryIfNeeded();
}

void DeepPolyBilinearElement::execute(
    const Map<unsigned, DeepPolyElement *> &deepPolyElementsBefore )
{
    log( "Executing..." );
    ASSERT( hasPredecessor() );
    allocateMemory();
    getConcreteBounds();

    // Update the symbolic and concrete upper- and lower- bounds
    // of each neuron
    for ( unsigned i = 0; i < _size; ++i )
    {
        log( Stringf( "Handling Neuron %u_%u...", _layerIndex, i ) );
        List<NeuronIndex> sources = _layer->getActivationSources( i );

        ASSERT( sources.size() == 2 );
        double sourceLbs[2];
        double sourceUbs[2];
        unsigned counter = 0;
        for ( const auto &sourceIndex : sources )
        {
            DeepPolyElement *predecessor = deepPolyElementsBefore[sourceIndex._layer];
            double sourceLb = predecessor->getLowerBound( sourceIndex._neuron );
            sourceLbs[counter] = sourceLb;
            double sourceUb = predecessor->getUpperBound( sourceIndex._neuron );
            sourceUbs[counter] = sourceUb;

            if ( counter == 0 )
            {
                _indexA.append( sourceIndex );
            }
            else
            {
                _indexB.append( sourceIndex );
            }

            ++counter;
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
        _lb[i] = std::max( lb, _lb[i] );
        _ub[i] = std::min( ub, _ub[i] );

        // Symbolic lower bound:
        // out >= alpha * x + beta * y + gamma
        // where alpha = lb_y, beta = lb_x, gamma = -lb_x * lb_y
        _symbolicLbA[i] = sourceLbs[1];
        _symbolicLbB[i] = sourceLbs[0];
        _symbolicLowerBias[i] = -sourceLbs[0] * sourceLbs[1];

        // Symbolic upper bound:
        // out <= alpha * x + beta * y + gamma
        // where alpha = ub_y, beta = lb_x, gamma = -lb_x * ub_y
        _symbolicUbA[i] = sourceUbs[1];
        _symbolicUbB[i] = sourceLbs[0];
        _symbolicUpperBias[i] = -sourceLbs[0] * sourceUbs[1];
    }

    DEBUG( {
        for ( unsigned i = 0; i < _size; ++i )
        {
            ASSERT( _indexA[i]._layer == _indexA[0]._layer );
            ASSERT( _indexB[i]._layer == _indexB[0]._layer );
        }
    } );

    log( "Executing - done" );
}

void DeepPolyBilinearElement::symbolicBoundInTermsOfPredecessor(
    const double *symbolicLb,
    const double *symbolicUb,
    double *symbolicLowerBias,
    double *symbolicUpperBias,
    double *symbolicLbInTermsOfPredecessor,
    double *symbolicUbInTermsOfPredecessor,
    unsigned targetLayerSize,
    DeepPolyElement *predecessor )
{
    unsigned predecessorIndex = predecessor->getLayerIndex();

    log( Stringf( "Computing symbolic bounds with respect to layer %u...", predecessorIndex ) );

    ASSERT( predecessorIndex == _indexA[0]._layer || predecessorIndex == _indexB[0]._layer );

    /*
      We have the symbolic bound of the target layer in terms of the
      Bilinear outputs, the goal is to compute the symbolic bound of the target
      layer in terms of the Bilinear inputs.
    */
    for ( unsigned i = 0; i < _size; ++i )
    {
        Vector<NeuronIndex> sourceIndices;
        Vector<double> coeffLbs;
        Vector<double> coeffUbs;

        if ( predecessorIndex == _indexA[0]._layer )
        {
            sourceIndices.append( _indexA[i] );
            coeffLbs.append( _symbolicLbA[i] );
            coeffUbs.append( _symbolicUbA[i] );
        }

        if ( predecessorIndex == _indexB[0]._layer )
        {
            sourceIndices.append( _indexB[i] );
            coeffLbs.append( _symbolicLbB[i] );
            coeffUbs.append( _symbolicUbB[i] );
        }

        Vector<unsigned> sourceNeuronIndices;
        for ( const auto &sourceIndex : sourceIndices )
            sourceNeuronIndices.append( sourceIndex._neuron );

        double lowerBias = _symbolicLowerBias[i];
        double upperBias = _symbolicUpperBias[i];

        /*
          Take symbolic upper bound as an example.
          Suppose the symbolic upper bound of the j-th neuron in the
          target layer is ... + a_i * f_i + ...,
          and the symbolic bounds of f_i in terms of b_i is
          m * b_i + n <= f_i <= p * b_i + q.
          If a_i >= 0, replace f_i with p * b_i + q, otherwise,
          replace f_i with m * b_i + n
        */

        // Substitute the Bilinear input for the Bilinear output
        for ( unsigned j = 0; j < targetLayerSize; ++j )
        {
            // The symbolic lower- and upper- bounds of the j-th neuron in the
            // target layer are ... + weightLb * f_i + ...
            // and ... + weightUb * f_i + ..., respectively.
            Vector<unsigned> newIndices;
            for ( const auto &sourceNeuronIndex : sourceNeuronIndices )
                newIndices.append( sourceNeuronIndex * targetLayerSize + j );
            unsigned oldIndex = i * targetLayerSize + j;

            // Update the symbolic lower bound
            double weightLb = symbolicLb[oldIndex];
            if ( weightLb >= 0 )
            {
                for ( unsigned q = 0; q < newIndices.size(); ++q )
                    symbolicLbInTermsOfPredecessor[newIndices[q]] += weightLb * coeffLbs[q];
                if ( symbolicLowerBias )
                    symbolicLowerBias[j] += weightLb * lowerBias;
            }
            else
            {
                for ( unsigned q = 0; q < newIndices.size(); ++q )
                    symbolicLbInTermsOfPredecessor[newIndices[q]] += weightLb * coeffUbs[q];
                if ( symbolicLowerBias )
                    symbolicLowerBias[j] += weightLb * upperBias;
            }

            // Update the symbolic upper bound
            double weightUb = symbolicUb[oldIndex];
            if ( weightUb >= 0 )
            {
                for ( unsigned q = 0; q < newIndices.size(); ++q )
                    symbolicUbInTermsOfPredecessor[newIndices[q]] += weightUb * coeffUbs[q];
                if ( symbolicUpperBias )
                    symbolicUpperBias[j] += weightUb * upperBias;
            }
            else
            {
                for ( unsigned q = 0; q < newIndices.size(); ++q )
                    symbolicUbInTermsOfPredecessor[newIndices[q]] += weightUb * coeffLbs[q];
                if ( symbolicUpperBias )
                    symbolicUpperBias[j] += weightUb * lowerBias;
            }
        }
    }
}

void DeepPolyBilinearElement::allocateMemory()
{
    freeMemoryIfNeeded();
    DeepPolyElement::allocateMemory();

    _symbolicLbA = new double[_size];
    _symbolicUbA = new double[_size];

    std::fill_n( _symbolicLbA, _size, 0 );
    std::fill_n( _symbolicUbA, _size, 0 );

    _symbolicLbB = new double[_size];
    _symbolicUbB = new double[_size];

    std::fill_n( _symbolicLbB, _size, 0 );
    std::fill_n( _symbolicUbB, _size, 0 );

    _symbolicLowerBias = new double[_size];
    _symbolicUpperBias = new double[_size];

    std::fill_n( _symbolicLowerBias, _size, 0 );
    std::fill_n( _symbolicUpperBias, _size, 0 );
}

void DeepPolyBilinearElement::freeMemoryIfNeeded()
{
    DeepPolyElement::freeMemoryIfNeeded();
    if ( _symbolicLbA )
    {
        delete[] _symbolicLbA;
        _symbolicLbA = NULL;
    }
    if ( _symbolicUbA )
    {
        delete[] _symbolicUbA;
        _symbolicUbA = NULL;
    }
    if ( _symbolicLbB )
    {
        delete[] _symbolicLbB;
        _symbolicLbB = NULL;
    }
    if ( _symbolicUbB )
    {
        delete[] _symbolicUbB;
        _symbolicUbB = NULL;
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

void DeepPolyBilinearElement::log( const String &message )
{
    if ( GlobalConfiguration::NETWORK_LEVEL_REASONER_LOGGING )
        printf( "DeepPolyBilinearElement: %s\n", message.ascii() );
}

} // namespace NLR
