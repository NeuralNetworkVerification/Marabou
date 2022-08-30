/*********************                                                        */
/*! \file DeepPolyQuadraticElement.cpp
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

#include "DeepPolyQuadraticElement.h"
#include "FloatUtils.h"

namespace NLR {

DeepPolyQuadraticElement::DeepPolyQuadraticElement( Layer *layer )
{
    _layer = layer;
    _size = layer->getSize();
    _layerIndex = layer->getLayerIndex();
}

DeepPolyQuadraticElement::~DeepPolyQuadraticElement()
{
    freeMemoryIfNeeded();
}

void DeepPolyQuadraticElement::execute( const Map<unsigned, DeepPolyElement *>
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

        ASSERT(sources.size() == 2);
        double sourceLbs[2];
        double sourceUbs[2];
        unsigned counter = 0;
        for (const auto &sourceIndex : sources)
        {
            DeepPolyElement *predecessor =
                deepPolyElementsBefore[sourceIndex._layer];
            double sourceLb = predecessor->getLowerBound
                ( sourceIndex._neuron );
            sourceLbs[counter] = sourceLb;
            double sourceUb = predecessor->getUpperBound
                ( sourceIndex._neuron );
            sourceUbs[counter] = sourceUb;
            ++counter;
        }


        double lb = FloatUtils::infinity();
        double ub = FloatUtils::negativeInfinity();
        List<double> values = {sourceLbs[0] * sourceLbs[1],
                               sourceLbs[0] * sourceUbs[1],
                               sourceUbs[0] * sourceLbs[1],
                               sourceUbs[0] * sourceUbs[1]};
        for (const auto &v : values)
        {
          if (v < lb)
            lb = v;
          if (v > ub)
            ub = v;
        }
        _lb[i] = lb;
        _ub[i] = ub;

        // Symbolic lower bound:
        // out >= alpha * x + beta * y + gamma
        // where alpha = lb_y, beta = lb_x, gamma = -lb_x * lb_y
        _symbolicLb[2*i] = sourceLbs[1];
        _symbolicLb[2*i + 1] = sourceLbs[0];
        _symbolicLowerBias[i] = -sourceLbs[0] * sourceLbs[1];

        // Symbolic upper bound:
        // out <= alpha * x + beta * y + gamma
        // where alpha = ub_y, beta = lb_x, gamma = -lb_x * ub_y
        _symbolicUb[2*i] = sourceUbs[1];
        _symbolicUb[2*i + 1] = sourceLbs[0];
        _symbolicUpperBias[i] = -sourceLbs[0] * sourceUbs[1];
    }
    log( "Executing - done" );
}

void DeepPolyQuadraticElement::symbolicBoundInTermsOfPredecessor
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
      Quadratic outputs, the goal is to compute the symbolic bound of the target
      layer in terms of the Quadratic inputs.
    */
    for ( unsigned i = 0; i < _size; ++i )
    {
      NeuronIndex sourceIndexX = *( _layer->
                                   getActivationSources( i ).begin() );
      NeuronIndex sourceIndexY = *( _layer->
                                    getActivationSources( i ).rbegin() );

      unsigned sourceNeuronIndexX = sourceIndexX._neuron;
      unsigned sourceNeuronIndexY = sourceIndexY._neuron;

        DEBUG({
                ASSERT( predecessor->getLayerIndex() == sourceIndexX._layer );
                ASSERT( predecessor->getLayerIndex() == sourceIndexY._layer );
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

        // Symbolic bounds of the ReLU output in terms of the ReLU input
        // coeffLb * b_i + lowerBias <= f_i <= coeffUb * b_i + upperBias
        double coeffLbX = _symbolicLb[2*i];
        double coeffLbY = _symbolicLb[2*i+1];
        double coeffUbX = _symbolicUb[2*i];
        double coeffUbY = _symbolicUb[2*i+1];
        double lowerBias = _symbolicLowerBias[i];
        double upperBias = _symbolicUpperBias[i];

        // Substitute the ReLU input for the ReLU output
        for ( unsigned j = 0; j < targetLayerSize; ++j )
        {
            // The symbolic lower- and upper- bounds of the j-th neuron in the
            // target layer are ... + weightLb * f_i + ...
            // and ... + weightUb * f_i + ..., respectively.
            unsigned newIndexX = sourceNeuronIndexX * targetLayerSize + j;
            unsigned newIndexY = sourceNeuronIndexY * targetLayerSize + j;
            unsigned oldIndex = i * targetLayerSize + j;

            // Update the symbolic lower bound
            double weightLb = symbolicLb[oldIndex];
            if ( weightLb >= 0 )
            {
                symbolicLbInTermsOfPredecessor[newIndexX] += weightLb * coeffLbX;
                symbolicLbInTermsOfPredecessor[newIndexY] += weightLb * coeffLbY;
                symbolicLowerBias[j] += weightLb * lowerBias;
            } else
            {
                symbolicLbInTermsOfPredecessor[newIndexX] += weightLb * coeffUbX;
                symbolicLbInTermsOfPredecessor[newIndexY] += weightLb * coeffUbY;
                symbolicLowerBias[j] += weightLb * upperBias;
            }

            // Update the symbolic upper bound
            double weightUb = symbolicUb[oldIndex];
            if ( weightUb >= 0 )
            {
                symbolicUbInTermsOfPredecessor[newIndexX] += weightUb * coeffUbX;
                symbolicUbInTermsOfPredecessor[newIndexY] += weightUb * coeffUbY;
                symbolicUpperBias[j] += weightUb * upperBias;
            } else
            {
                symbolicUbInTermsOfPredecessor[newIndexX] += weightUb * coeffLbX;
                symbolicUbInTermsOfPredecessor[newIndexY] += weightUb * coeffLbY;
                symbolicUpperBias[j] += weightUb * lowerBias;
            }
        }
    }
}

void DeepPolyQuadraticElement::allocateMemory()
{
    freeMemoryIfNeeded();
    DeepPolyElement::allocateMemory();

    // Arranged as a1, b1, a2, b2, ... where ai, bi are the coefficients in
    // of x and y respectively for neuron i.
    _symbolicLb = new double[_size*2];
    _symbolicUb = new double[_size*2];

    std::fill_n( _symbolicLb, _size*2, 0 );
    std::fill_n( _symbolicUb, _size*2, 0 );

    _symbolicLowerBias = new double[_size];
    _symbolicUpperBias = new double[_size];

    std::fill_n( _symbolicLowerBias, _size, 0 );
    std::fill_n( _symbolicUpperBias, _size, 0 );
}

void DeepPolyQuadraticElement::freeMemoryIfNeeded()
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

void DeepPolyQuadraticElement::log( const String &message )
{
    if ( GlobalConfiguration::NETWORK_LEVEL_REASONER_LOGGING )
        printf( "DeepPolyQuadraticElement: %s\n", message.ascii() );
}

} // namespace NLR
