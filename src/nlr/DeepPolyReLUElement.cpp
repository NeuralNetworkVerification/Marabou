/*********************                                                        */
/*! \file DeepPolyReLUElement.cpp
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

#include "DeepPolyReLUElement.h"
#include "FloatUtils.h"

namespace NLR {

    DeepPolyReLUElement::DeepPolyReLUElement( Layer *layer )
    {
        _layer = layer;
        _size = layer->getSize();
        _layerIndex = layer->getLayerIndex();
    }

    DeepPolyReLUElement::~DeepPolyReLUElement()
    {
        freeMemoryIfNeeded();
    }

    void DeepPolyReLUElement::execute( const Map<unsigned, DeepPolyElement *>
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
                // Symbolic bound: 0 <= x_f <= 0
                // Concrete bound: 0 <= x_f <= 0
                _symbolicUb[i] = 0;
                _symbolicUpperBias[i] = 0;
                _ub[i] = 0;

                _symbolicLb[i] = 0;
                _symbolicLowerBias[i] = 0;
                _lb[i] = 0;
            }
            else
            {
                // ReLU not fixed
                // Symbolic upper bound: x_f <= (x_b - l) * u / ( u - l)
                // Concrete upper bound: x_f <= ub_b
                double coeff = sourceUb / ( sourceUb - sourceLb );
                _symbolicUb[i] = coeff;
                _symbolicUpperBias[i] = -sourceLb * coeff;
                _ub[i] = sourceUb;

                // For the lower bound, in general, x_f >= lambda * x_b, where
                // 0 <= lambda <= 1, would be a sound lower bound. We
                // use the heuristic described in section 4.1 of
                // https://files.sri.inf.ethz.ch/website/papers/DeepPoly.pdf
                // to set the value of lambda (either 0 or 1 is considered).
                if ( sourceUb > -sourceLb )
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
                    _symbolicLb[i] = 0;
                    _symbolicLowerBias[i] = 0;
                    _lb[i] = 0;

                }
            }
            log( Stringf( "Neuron%u LB: %f b + %f, UB: %f b + %f",
                          i, _symbolicLb[i], _symbolicLowerBias[i],
                          _symbolicUb[i], _symbolicUpperBias[i] ) );
            log( Stringf( "Neuron%u LB: %f, UB: %f", i, _lb[i], _ub[i] ) );
        }
        log( "Executing - done" );
    }

    void DeepPolyReLUElement::symbolicBoundInTermsOfPredecessor
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

        for ( unsigned i = 0; i < _size; ++i )
        {
            double weightLbPred = _symbolicLb[i];
            double weightUbPred = _symbolicUb[i];
            double lowerBiasPred = _symbolicLowerBias[i];
            double upperBiasPred = _symbolicUpperBias[i];

            // Update weights
            for ( unsigned j = 0; j < targetLayerSize; ++j )
            {
                unsigned index = i * targetLayerSize + j;
                double weightLb = symbolicLb[index];
                double weightUb = symbolicUb[index];

                if ( weightLb >= 0 )
                {
                    symbolicLbInTermsOfPredecessor[index] = weightLb * weightLbPred;
                    symbolicLowerBias[j] += weightLb * lowerBiasPred;
                } else
                {
                    symbolicLbInTermsOfPredecessor[index] = weightLb * weightUbPred;
                    symbolicLowerBias[j] += weightLb * upperBiasPred;
                }

                if ( weightUb >= 0 )
                {
                    symbolicUbInTermsOfPredecessor[index] = weightUb * weightUbPred;
                    symbolicUpperBias[j] += weightUb * upperBiasPred;
                } else
                {
                    symbolicUbInTermsOfPredecessor[index] = weightUb * weightLbPred;
                    symbolicUpperBias[j] += weightUb * lowerBiasPred;
                }
            }
        }
    }

    void DeepPolyReLUElement::allocateMemory()
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

    void DeepPolyReLUElement::freeMemoryIfNeeded()
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

    void DeepPolyReLUElement::log( const String &message )
    {
        if ( GlobalConfiguration::NETWORK_LEVEL_REASONER_LOGGING )
            printf( "DeepPolyReLUElement: %s\n", message.ascii() );
    }

} // namespace NLR
