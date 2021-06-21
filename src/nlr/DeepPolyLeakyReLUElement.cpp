 /*********************                                                        */
/*! \file DeepPolyLeakyReLUElement.cpp
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

#include "DeepPolyLeakyReLUElement.h"
#include "FloatUtils.h"

namespace NLR {

DeepPolyLeakyReLUElement::DeepPolyLeakyReLUElement( Layer *layer )
{
    _layer = layer;
    _size = layer->getSize();
    _layerIndex = layer->getLayerIndex();
    _alpha = layer->getAlpha();
    ASSERT( _alpha > 0 );
    log( Stringf( "Slope is %f", _alpha ) );
}

DeepPolyLeakyReLUElement::~DeepPolyLeakyReLUElement()
{
    freeMemoryIfNeeded();
}

void DeepPolyLeakyReLUElement::execute( const Map<unsigned, DeepPolyElement *>
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

        if ( !FloatUtils::isNegative( sourceLb ) )
        {
            // Phase active
            // Symbolic bound: x_b <= x_f <= x_b
            // Concrete bound: lb_b <= x_f <= ub_b
            _symbolicUb[i] = 1;
            _symbolicUpperBias[i] = 0;
            _ub[i] = sourceUb;

            _symbolicLb[i] = 1;
            _symbolicLowerBias[i] = 0;
            _lb[i] = sourceLb;
        }
        else if ( !FloatUtils::isPositive( sourceUb ) )
        {
            // Phase inactive
            // Symbolic bound: slope * x_b <= x_f <= slope * x_b
            // Concrete bound: slope * lb_b <= x_f <= slope * ub_b
            _symbolicUb[i] = _alpha;
            _symbolicUpperBias[i] = 0;
            _ub[i] = _alpha * sourceUb;

            _symbolicLb[i] = _alpha;
            _symbolicLowerBias[i] = 0;
            _lb[i] = _alpha * sourceLb;
        }
        else
        {
            // LeakyReLU not fixed
            // Symbolic upper bound: x_f <= (x_b - l) * u / ( u - l)
            // Concrete upper bound: x_f <= ub_b
            double width = sourceUb - sourceLb;
            double coeff = (sourceUb - _alpha * sourceLb) / width;
            _symbolicUb[i] = coeff;
            _symbolicUpperBias[i] = ( ( _alpha - 1 ) * sourceUb * sourceLb ) / width;
            _ub[i] = sourceUb;

            // For the lower bound, in general, x_f >= lambda * x_b, where
            // 0 <= lambda <= 1, would be a sound lower bound. We
            // use the heuristic described in section 4.1 of
            // https://files.sri.inf.ethz.ch/website/papers/DeepPoly.pdf
            // to set the value of lambda (either 0 or 1 is considered).
            if ( sourceUb > (2 * _alpha - 1 ) * sourceLb )
            {
                // lambda = 1
                // Symbolic lower bound: x_f >= x_b
                // Concrete lower bound: x_f >= sourceLb
                _symbolicLb[i] = 1;
                _symbolicLowerBias[i] = 0;
                _lb[i] = sourceLb;
            }
            else
            {
                // lambda = 1
                // Symbolic lower bound: x_f >= 0
                // Concrete lower bound: x_f >= 0
                _symbolicLb[i] = _alpha;
                _symbolicLowerBias[i] = 0;
                _lb[i] = _alpha * sourceLb;
            }
        }
        log( Stringf( "Neuron%u LB: %f b + %f, UB: %f b + %f",
                      i, _symbolicLb[i], _symbolicLowerBias[i],
                      _symbolicUb[i], _symbolicUpperBias[i] ) );
        log( Stringf( "Neuron%u LB: %f, UB: %f", i, _lb[i], _ub[i] ) );
    }
    log( "Executing - done" );
}

void DeepPolyLeakyReLUElement::symbolicBoundInTermsOfPredecessor
( const double *symbolicLb, const double*symbolicUb, double
  *symbolicLowerBias, double *symbolicUpperBias, double
  *symbolicLbInTermsOfPredecessor, double *symbolicUbInTermsOfPredecessor,
  unsigned targetLayerSize, DeepPolyElement *predecessor )
{
    log( Stringf( "Computing symbolic bounds with respect to layer %u...",
                  predecessor->getLayerIndex() ) );

    /*
      We have the symbolic bound of the target layer in terms of the
      LeakyReLU outputs, the goal is to compute the symbolic bound of the target
      layer in terms of the LeakyReLU inputs.
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

        // Symbolic bounds of the LeakyReLU output in terms of the LeakyReLU input
        // coeffLb * b_i + lowerBias <= f_i <= coeffUb * b_i + upperBias
        double coeffLb = _symbolicLb[i];
        double coeffUb = _symbolicUb[i];
        double lowerBias = _symbolicLowerBias[i];
        double upperBias = _symbolicUpperBias[i];

        // Substitute the LeakyReLU input for the LeakyReLU output
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

void DeepPolyLeakyReLUElement::allocateMemory()
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

void DeepPolyLeakyReLUElement::freeMemoryIfNeeded()
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

void DeepPolyLeakyReLUElement::log( const String &message )
{
    if ( GlobalConfiguration::NETWORK_LEVEL_REASONER_LOGGING )
        printf( "DeepPolyLeakyReLUElement: %s\n", message.ascii() );
}

} // namespace NLR
