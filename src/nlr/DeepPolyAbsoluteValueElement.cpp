/*********************                                                        */
/*! \file DeepPolyAbsoluteValueElement.cpp
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

#include "DeepPolyAbsoluteValueElement.h"
#include "FloatUtils.h"

namespace NLR {

DeepPolyAbsoluteValueElement::DeepPolyAbsoluteValueElement( Layer *layer )
{
    _layer = layer;
    _size = layer->getSize();
    _layerIndex = layer->getLayerIndex();
}

DeepPolyAbsoluteValueElement::~DeepPolyAbsoluteValueElement()
{
    freeMemoryIfNeeded();
}

void DeepPolyAbsoluteValueElement::execute( const Map<unsigned, DeepPolyElement *>
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
            // Symbolic bound: -x_b <= x_f <= -x_b
            // Concrete bound: -ub_b <= x_f <= -lb_b
            _symbolicUb[i] = -1;
            _symbolicUpperBias[i] = 0;
            _ub[i] = -sourceLb;

            _symbolicLb[i] = -1;
            _symbolicLowerBias[i] = 0;
            _lb[i] = -sourceUb;
        }
        else
        {
            // AbsoluteValue not fixed
            // Naive concretization: 0 <= x_f <= max(-lb, ub)
            _symbolicUb[i] = 0;
            double upperBound = FloatUtils::max( -sourceLb, sourceUb );
            _symbolicUpperBias[i] = upperBound;
            _ub[i] = upperBound;

            _symbolicLb[i] = 0;
            _symbolicLowerBias[i] = 0;
            _lb[i] = 0;
        }
        log( Stringf( "Neuron%u LB: %f b + %f, UB: %f b + %f",
                      i, _symbolicLb[i], _symbolicLowerBias[i],
                      _symbolicUb[i], _symbolicUpperBias[i] ) );
        log( Stringf( "Neuron%u LB: %f, UB: %f", i, _lb[i], _ub[i] ) );
    }
    log( "Executing - done" );
}

void DeepPolyAbsoluteValueElement::symbolicBoundInTermsOfPredecessor
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
      AbsoluteValue outputs, the goal is to compute the symbolic bound of the target
      layer in terms of the AbsoluteValue inputs.
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
          m * b_i + n <= f_i <= p * b_i + q.
          If a_i >= 0, replace f_i with p * b_i + q, otherwise,
          replace f_i with m * b_i + n
        */

        // Symbolic bounds of the AbsoluteValue output in terms of the AbsoluteValue input
        // coeffLb * b_i + lowerBias <= f_i <= coeffUb * b_i + upperBias
        double coeffLb = _symbolicLb[i];
        double coeffUb = _symbolicUb[i];
        double lowerBias = _symbolicLowerBias[i];
        double upperBias = _symbolicUpperBias[i];

        // Substitute the AbsoluteValue input for the AbsoluteValue output
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

void DeepPolyAbsoluteValueElement::allocateMemory()
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

void DeepPolyAbsoluteValueElement::freeMemoryIfNeeded()
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

void DeepPolyAbsoluteValueElement::log( const String &message )
{
    if ( GlobalConfiguration::NETWORK_LEVEL_REASONER_LOGGING )
        printf( "DeepPolyAbsoluteValueElement: %s\n", message.ascii() );
}

} // namespace NLR
