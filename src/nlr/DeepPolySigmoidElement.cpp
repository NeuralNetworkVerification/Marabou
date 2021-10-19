 /*********************                                                        */
/*! \file DeepPolySigmoidElement.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Teruhiro Tagomori
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

**/

#include "DeepPolySigmoidElement.h"
#include "FloatUtils.h"

namespace NLR {

DeepPolySigmoidElement::DeepPolySigmoidElement( Layer *layer )
{
    _layer = layer;
    _size = layer->getSize();
    _layerIndex = layer->getLayerIndex();
}

DeepPolySigmoidElement::~DeepPolySigmoidElement()
{
    freeMemoryIfNeeded();
}

void DeepPolySigmoidElement::execute( const Map<unsigned, DeepPolyElement *>
                               &deepPolyElementsBefore )
{
    log( "Executing..." );
    ASSERT( hasPredecessor() );
    allocateMemory();

    // Update the symbolic and concrete upper- and lower- bounds
    // of each neuron
    for ( unsigned i = 0; i < _size; ++i )
    {
        NeuronIndex sourceIndex = *( _layer->getActivationSources( i ).begin() );
        DeepPolyElement *predecessor =
            deepPolyElementsBefore[sourceIndex._layer];
        double sourceLb = predecessor->getLowerBound
            ( sourceIndex._neuron );
        double sourceUb = predecessor->getUpperBound
            ( sourceIndex._neuron );

        _ub[i] = sigmoid( sourceUb );
        _lb[i] = sigmoid( sourceLb );

        double lambda = ( _ub[i] - _lb[i] ) / ( sourceUb - sourceLb );
        double lambdaPrime = std::min( sigmoid_diff( sourceLb ), sigmoid_diff( sourceUb ) );

        if ( sourceUb == sourceLb )
        {
            _symbolicUb[i] = 0;
            _symbolicUpperBias[i] = _lb[i];
            _symbolicLb[i] = 0;
            _symbolicLowerBias[i] = _lb[i];
        }
        else
        {
            // update lower bound
            if ( FloatUtils::isPositive( sourceLb ) )
            {
                _symbolicLb[i] = lambda;
                _symbolicLowerBias[i] = sigmoid( sourceLb ) - lambda * sourceLb;   
            }
            else
            {
                _symbolicLb[i] = lambdaPrime;
                _symbolicLowerBias[i] = sigmoid( sourceLb ) - lambdaPrime * sourceLb;   
            }

            // update upper bound
            if ( !FloatUtils::isPositive( sourceUb ) )
            {
                _symbolicUb[i] = lambda;
                _symbolicUpperBias[i] = sigmoid( sourceUb ) - lambda * sourceUb;            
            }
            else
            {
                _symbolicUb[i] = lambdaPrime;
                _symbolicUpperBias[i] = sigmoid( sourceUb ) - lambdaPrime * sourceUb;               
            }
        }

        log( Stringf( "Neuron%u LB: %f b + %f, UB: %f b + %f",
                      i, _symbolicLb[i], _symbolicLowerBias[i],
                      _symbolicUb[i], _symbolicUpperBias[i] ) );
        log( Stringf( "Neuron%u LB: %f, UB: %f", i, _lb[i], _ub[i] ) );
    }
    log( "Executing - done" );
}

void DeepPolySigmoidElement::symbolicBoundInTermsOfPredecessor
( const double *symbolicLb, const double*symbolicUb, double
  *symbolicLowerBias, double *symbolicUpperBias, double
  *symbolicLbInTermsOfPredecessor, double *symbolicUbInTermsOfPredecessor,
  unsigned targetLayerSize, DeepPolyElement *predecessor )
{
    log( Stringf( "Computing symbolic bounds with respect to layer %u...",
                  predecessor->getLayerIndex() ) );

    /*
      We have the symbolic bound of the target layer in terms of the
      Sigmoid outputs, the goal is to compute the symbolic bound of the target
      layer in terms of the Sigmoid inputs.
    */
    for ( unsigned i = 0; i < _size; ++i )
    {
        NeuronIndex sourceIndex = *( _layer->
                                     getActivationSources( i ).begin() );
        unsigned sourceNeuronIndex = sourceIndex._neuron;
        DEBUG({
                ASSERT( predecessor->getLayerIndex() == sourceIndex._layer );
            });

        /*
          Take symbolic upper bound as an example.
          Suppose the symbolic upper bound of the j-th neuron in the
          target layer is ... + a_i * f_i + ...,
          and the symbolic bounds of f_i in terms of b_i is
          m * b_i + n <= f_i <= p * b_i + q.
          If a_i >= 0, replace f_i with p * b_i + q, otherwise,
          replace f_i with m * b_i + n
        */

        // Symbolic bounds of the Sigmoid output in terms of the Sigmoid input
        // coeffLb * b_i + lowerBias <= f_i <= coeffUb * b_i + upperBias
        double coeffLb = _symbolicLb[i];
        double coeffUb = _symbolicUb[i];
        double lowerBias = _symbolicLowerBias[i];
        double upperBias = _symbolicUpperBias[i];

        // Substitute the Sigmoid input for the Sigmoid output
        for ( unsigned j = 0; j < targetLayerSize; ++j )
        {
            // The symbolic lower- and upper- bounds of the j-th neuron in the
            // target layer are ... + weightLb * f_i + ...
            // and ... + weightUb * f_i + ..., respectively.
            unsigned newIndex = sourceNeuronIndex * targetLayerSize + j;
            unsigned oldIndex = i * targetLayerSize + j;

            // Update the symbolic lower bound
            double weightLb = symbolicLb[oldIndex];
            if ( weightLb >= 0 )
            {
                symbolicLbInTermsOfPredecessor[newIndex] += weightLb * coeffLb;
                symbolicLowerBias[j] += weightLb * lowerBias;
            } else
            {
                symbolicLbInTermsOfPredecessor[newIndex] += weightLb * coeffUb;
                symbolicLowerBias[j] += weightLb * upperBias;
            }

            // Update the symbolic upper bound
            double weightUb = symbolicUb[oldIndex];
            if ( weightUb >= 0 )
            {
                symbolicUbInTermsOfPredecessor[newIndex] += weightUb * coeffUb;
                symbolicUpperBias[j] += weightUb * upperBias;
            } else
            {
                symbolicUbInTermsOfPredecessor[newIndex] += weightUb * coeffLb;
                symbolicUpperBias[j] += weightUb * lowerBias;
            }
        }
    }
}

void DeepPolySigmoidElement::allocateMemory()
{
    freeMemoryIfNeeded();

    DeepPolyElement::allocateMemory();

    _symbolicLb = new double[_size];
    _symbolicUb = new double[_size];

    std::fill_n( _symbolicLb, _size, 0 );
    std::fill_n( _symbolicUb, _size, 0 );

    _symbolicLowerBias = new double[_size];
    _symbolicUpperBias = new double[_size];

    std::fill_n( _symbolicLowerBias, _size, 0 );
    std::fill_n( _symbolicUpperBias, _size, 0 );
}

void DeepPolySigmoidElement::freeMemoryIfNeeded()
{
    DeepPolyElement::freeMemoryIfNeeded();
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

void DeepPolySigmoidElement::log( const String &message )
{
    if ( GlobalConfiguration::NETWORK_LEVEL_REASONER_LOGGING )
        printf( "DeepPolySigmoidElement: %s\n", message.ascii() );
}

double DeepPolySigmoidElement::sigmoid( double x )
{
    return 1 / ( 1 + std::exp( -x ) );
}

double DeepPolySigmoidElement::sigmoid_diff( double x )
{
    return sigmoid( x ) * ( 1 - sigmoid( x ) );
}

} // namespace NLR
