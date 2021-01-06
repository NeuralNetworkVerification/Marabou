/*********************                                                        */
/*! \file DeepPolySignElement.cpp
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

#include "DeepPolySignElement.h"
#include "FloatUtils.h"

namespace NLR {

DeepPolySignElement::DeepPolySignElement( Layer *layer )
{
    _layer = layer;
    _size = layer->getSize();
    _layerIndex = layer->getLayerIndex();
}

DeepPolySignElement::~DeepPolySignElement()
{
    freeMemoryIfNeeded();
}

void DeepPolySignElement::execute( const Map<unsigned, DeepPolyElement *>
                               &deepPolyElementsBefore )
{
    log( "Executing..." );
    ASSERT( hasPredecessor() );
    allocateMemory();
    DeepPolyElement *predecessor =
        deepPolyElementsBefore[getPredecessorIndex()];
    ASSERT( predecessor->getSize() == _size );

    // Update the symbolic and concrete upper- and lower- bounds
    // of each neuron
    for ( unsigned i = 0; i < _size; ++i )
    {
        double sourceLb = predecessor->getLowerBound( i );
        double sourceUb = predecessor->getUpperBound( i );

        if ( !FloatUtils::isNegative( sourceLb ) )
        {
            // Phase positive
            // Symbolic bound: 1 <= x_f <= 1
            // Concrete bound: 1 <= x_f <= 1
            _symbolicUb[i] = 0;
            _symbolicUpperBias[i] = 1;
            _ub[i] = 1;

            _symbolicLb[i] = 0;
            _symbolicLowerBias[i] = 1;
            _lb[i] = 1;
        }
        else if ( !FloatUtils::isPositive( sourceUb ) )
        {
            // Phase negative
            // Symbolic bound: -1 <= x_f <= -1
            // Concrete bound: -1 <= x_f <= -1
            _symbolicUb[i] = 0;
            _symbolicUpperBias[i] = -1;
            _ub[i] = -1;

            _symbolicLb[i] = 0;
            _symbolicLowerBias[i] = -1;
            _lb[i] = -1;
        }
        else
        {
            // Sign not fixed
            // Symbolic upper bound: x_f <= -2 / l * x_b + 1
            // Concrete upper bound: x_f <= 1
            _symbolicUb[i] = -2 / sourceLb;
            _symbolicUpperBias[i] = 1;
            _ub[i] = sourceUb;

            // For the lower bound, in general, x_f >= lambda * x_b - 1, where
            // 0 <= lambda <= 2 / u, would be a sound lower bound.
            if ( sourceUb > -sourceLb )
            {
                // lambda = 2 / u
                // Symbolic lower bound: x_f >= (2 / u) * x_b - 1
                // Concrete lower bound: x_f >= sourceLb
                _symbolicLb[i] = 2 / sourceUb;
                _symbolicLowerBias[i] = -1;
                _lb[i] = sourceLb;
            }
            else
            {
                // lambda = 0
                // Symbolic lower bound: x_f >= -1
                // Concrete lower bound: x_f >= -1
                _symbolicLb[i] = 0;
                _symbolicLowerBias[i] = -1;
                _lb[i] = -1;

            }
        }
        log( Stringf( "Neuron%u LB: %f b + %f, UB: %f b + %f",
                      i, _symbolicLb[i], _symbolicLowerBias[i],
                      _symbolicUb[i], _symbolicUpperBias[i] ) );
        log( Stringf( "Neuron%u LB: %f, UB: %f", i, _lb[i], _ub[i] ) );
    }
    log( "Executing - done" );
}

void DeepPolySignElement::symbolicBoundInTermsOfPredecessor
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
      Sign outputs, the goal is to compute the symbolic bound of the target
      layer in terms of the Sign inputs.
    */
    for ( unsigned i = 0; i < _size; ++i )
    {
        /*
          Take symbolic upper bound as an example.
          Suppose the symbolic upper bound of the j-th neuron in the
          target layer is ... + a_i * f_i + ...,
          and the symbolic bounds of f_i in terms of b_i is
          m * b_i + n <= f_i <= p * b_i + q.
          If a_i >= 0, replace f_i with p * b_i + q, otherwise,
          replace f_i with m * b_i + n
        */

        // Symbolic bounds of the Sign output in terms of the Sign input
        // coeffLb * b_i + lowerBias <= f_i <= coeffUb * b_i + upperBias
        double coeffLb = _symbolicLb[i];
        double coeffUb = _symbolicUb[i];
        double lowerBias = _symbolicLowerBias[i];
        double upperBias = _symbolicUpperBias[i];

        // Substitute the Sign input for the Sign output
        for ( unsigned j = 0; j < targetLayerSize; ++j )
        {
            // The symbolic lower- and upper- bounds of the j-th neuron in the
            // target layer are ... + weightLb * f_i + ...
            // and ... + weightUb * f_i + ..., respectively.
            unsigned index = i * targetLayerSize + j;

            // Update the symbolic lower bound
            double weightLb = symbolicLb[index];
            if ( weightLb >= 0 )
            {
                symbolicLbInTermsOfPredecessor[index] = weightLb * coeffLb;
                symbolicLowerBias[j] += weightLb * lowerBias;
            } else
            {
                symbolicLbInTermsOfPredecessor[index] = weightLb * coeffUb;
                symbolicLowerBias[j] += weightLb * upperBias;
            }

            // Update the symbolic upper bound
            double weightUb = symbolicUb[index];
            if ( weightUb >= 0 )
            {
                symbolicUbInTermsOfPredecessor[index] = weightUb * coeffUb;
                symbolicUpperBias[j] += weightUb * upperBias;
            } else
            {
                symbolicUbInTermsOfPredecessor[index] = weightUb * coeffLb;
                symbolicUpperBias[j] += weightUb * lowerBias;
            }
        }
    }
}

void DeepPolySignElement::allocateMemory()
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

void DeepPolySignElement::freeMemoryIfNeeded()
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

void DeepPolySignElement::log( const String &message )
{
    if ( GlobalConfiguration::NETWORK_LEVEL_REASONER_LOGGING )
        printf( "DeepPolySignElement: %s\n", message.ascii() );
}

} // namespace NLR
