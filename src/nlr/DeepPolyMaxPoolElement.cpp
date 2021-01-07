/*********************                                                        */
/*! \file DeepPolyMaxPoolElement.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Haoze Andrew Wu
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

**/

#include "DeepPolyMaxPoolElement.h"
#include "FloatUtils.h"

namespace NLR {

DeepPolyMaxPoolElement::DeepPolyMaxPoolElement( Layer *layer )
{
    _layer = layer;
    _size = layer->getSize();
    _layerIndex = layer->getLayerIndex();
}

DeepPolyMaxPoolElement::~DeepPolyMaxPoolElement()
{
    freeMemoryIfNeeded();
}

void DeepPolyMaxPoolElement::execute( const Map<unsigned, DeepPolyElement *>
                               &deepPolyElementsBefore )
{
    log( "Executing..." );
    ASSERT( hasPredecessor() );
    allocateMemory();

    // Update the symbolic and concrete upper- and lower- bounds
    // of each neuron
    for ( unsigned i = 0; i < _size; ++i )
    {
        log( Stringf( "Handling Neuron %u_%u...", _layerIndex, i ) );
        List<NeuronIndex> sources = _layer->getActivationSources( i );

        NeuronIndex indexOfMaxLowerBound = *( sources.begin() );
        double maxLowerBound = FloatUtils::negativeInfinity();
        double maxUpperBound = FloatUtils::negativeInfinity();

        Map<NeuronIndex, double> sourceLbs;
        Map<NeuronIndex, double> sourceUbs;
        for ( const auto &sourceIndex : sources )
        {
            DeepPolyElement *predecessor =
                deepPolyElementsBefore[sourceIndex._layer];
            double sourceLb = predecessor->getLowerBound
                ( sourceIndex._neuron );
            sourceLbs[sourceIndex] = sourceLb;
            double sourceUb = predecessor->getUpperBound
                ( sourceIndex._neuron );
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
                phaseFixed = false;
        }

        if ( phaseFixed )
        {
            log( Stringf( "Neuron %u_%u fixed to Neuron %u_%u",
                          _layerIndex, i, indexOfMaxLowerBound._layer,
                          indexOfMaxLowerBound._neuron ) );
            // Phase fixed
            // Symbolic bound: x_b <= x_f <= x_b
            // Concrete bound: lb_b <= x_f <= ub_b
            _sourceIndexForSymbolicBounds[i] = indexOfMaxLowerBound;
            _ub[i] = sourceUbs[indexOfMaxLowerBound];
            _lb[i] = maxLowerBound;
        }
        else
        {
            log( Stringf( "Neuron %u_%u not fixed",
                          _layerIndex, i ) );
            // MaxPool not fixed
            // Symbolic bounds: x_b <= x_f <= maxUpperBound
            // Concrete bounds: lb_b <= x_f <= maxUpperBound
            _sourceIndexForSymbolicBounds[i] = indexOfMaxLowerBound;
            _ub[i] = maxUpperBound;
            _lb[i] = maxLowerBound;
        }
        log( Stringf( "Neuron%u LB: %f, UB: %f", i, _lb[i], _ub[i] ) );
        log( Stringf( "Handling Neuron %u_%u - done", _layerIndex, i ) );
    }
    log( "Executing - done" );
}

void DeepPolyMaxPoolElement::symbolicBoundInTermsOfPredecessor
( const double *symbolicLb, const double*symbolicUb, double
  *symbolicLowerBias, double *symbolicUpperBias, double
  *symbolicLbInTermsOfPredecessor, double *symbolicUbInTermsOfPredecessor,
  unsigned targetLayerSize, DeepPolyElement *predecessor )
{
    log( Stringf( "Computing symbolic bounds with respect to layer %u...",
                  predecessor->getLayerIndex() ) );

    unsigned predecessorSize = predecessor->getSize();
    std::fill_n( symbolicLbInTermsOfPredecessor, targetLayerSize *
                 predecessorSize, 0 );
    std::fill_n( symbolicUbInTermsOfPredecessor, targetLayerSize *
                 predecessorSize, 0 );

    /*
      We have the symbolic bound of the target layer in terms of the
      MaxPool outputs, the goal is to compute the symbolic bound of the target
      layer in terms of the MaxPool inputs.
    */
    for ( unsigned i = 0; i < _size; ++i )
    {
        DEBUG({
                NeuronIndex sourceIndex = *( _layer->
                                             getActivationSources( i ).begin() );
                ASSERT( predecessor->getLayerIndex() == sourceIndex._layer );
            });

        /*
          Take symbolic upper bound as an example.
          Suppose the symbolic upper bound of the j-th neuron in the
          target layer is ... + a_i * f_i + ...,
          and the symbolic bounds of f_i in terms of b_i is
          k <= f_i <= maxUpperBound
          If a_i >= 0, replace f_i with k, otherwise,
          replace f_i with maxUpperBound
        */

        // Symbolic bounds of the MaxPool output in terms of the MaxPool input
        // If phase fixed:
        //    b_sourceIndex <= f_i <= b_sourceIndex
        // If phase not fixed:
        //    b_sourceIndex <= f_i <= maxUpperBound
        NeuronIndex sourceNeuronIndex = _sourceIndexForSymbolicBounds[i];
        ASSERT( sourceNeuronIndex._layer == predecessor->getLayerIndex() );
        unsigned sourceIndex =  sourceNeuronIndex._neuron;
        ASSERT( sourceIndex < predecessorSize );
        // coeffLb = 1
        // lowerBias = 0
        bool phaseFixed =  _phaseFixed[i];
        double coeffUb = phaseFixed ? 1 : 0;
        double upperBias = phaseFixed ? 0 : _ub[i];

        // Substitute the MaxPool input for the MaxPool output
        for ( unsigned j = 0; j < targetLayerSize; ++j )
        {
            // The symbolic lower- and upper- bounds of the j-th neuron in the
            // target layer are ... + weightLb * f_i + ...
            // and ... + weightUb * f_i + ..., respectively.
            unsigned indexInNewSymbolicBound = sourceIndex * targetLayerSize + j;
            unsigned indexInOldSymbolicBound = i * targetLayerSize + j;

            // Update the symbolic lower bound
            double weightLb = symbolicLb[indexInOldSymbolicBound];
            if ( weightLb >= 0 )
            {
                symbolicLbInTermsOfPredecessor[indexInNewSymbolicBound] +=
                    weightLb;
                // symbolicLowerBias[j] += weightLb * lowerBias;
            }
            else
            {
                symbolicLbInTermsOfPredecessor[indexInNewSymbolicBound] +=
                    weightLb * coeffUb;
                symbolicLowerBias[j] += weightLb * upperBias;
            }

            // Update the symbolic upper bound
            double weightUb = symbolicUb[indexInOldSymbolicBound];
            if ( weightUb >= 0 )
            {
                symbolicUbInTermsOfPredecessor[indexInNewSymbolicBound] +=
                    weightUb * coeffUb;
                symbolicUpperBias[j] += weightUb * upperBias;
            } else
            {
                symbolicUbInTermsOfPredecessor[indexInNewSymbolicBound] +=
                    weightUb;
                //symbolicUpperBias[j] += weightUb * lowerBias;
            }
        }
    }
}

void DeepPolyMaxPoolElement::allocateMemory()
{
    freeMemoryIfNeeded();
    DeepPolyElement::allocateMemory();
}

void DeepPolyMaxPoolElement::freeMemoryIfNeeded()
{
    DeepPolyElement::freeMemoryIfNeeded();
    _sourceIndexForSymbolicBounds.clear();
    _phaseFixed.clear();
}

void DeepPolyMaxPoolElement::log( const String &message )
{
    if ( GlobalConfiguration::NETWORK_LEVEL_REASONER_LOGGING )
        printf( "DeepPolyMaxPoolElement: %s\n", message.ascii() );
}

} // namespace NLR
