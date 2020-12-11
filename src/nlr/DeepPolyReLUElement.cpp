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
    }

    DeepPolyReLUElement::~DeepPolyReLUElement()
    {
        freeMemoryIfNeeded();
    }

    void DeepPolyReLUElement::execute( const Map<unsigned, DeepPolyElement *>
                                   &deepPolyElementsBefore )
    {
        log( "Executing..." );
        freeMemoryIfNeeded();

        if ( deepPolyElementsBefore.empty() )
        {
            // If this is the first layer, just update the concrete bounds
            allocateMemoryForUpperAndLowerBounds();
            getConcreteBounds();
        } else
        {
            allocateMemory();
            ASSERT( deepPolyElementsBefore.exists( getLayerIndex() - 1 ) );
            DeepPolyElement *previousElement =
                deepPolyElementsBefore[ getLayerIndex() - 1];
            ASSERT( previousElement->getSize() == getSize() );
            for ( unsigned i = 0; i < getSize(); ++i )
            {
                double sourceLb = previousElement->getLowerBound( i );
                double sourceUb = previousElement->getUpperBound( i );

                if ( !FloatUtils::isNegative( sourceLb ) )
                {
                    // Phase active
                    _symbolicUb[i] = 1;
                    _symbolicUpperBias[i] = 0;
                    _ub[i] = sourceUb;

                    _symbolicLb[i] = 1;
                    _symbolicLowerBias[i] = 0;
                    _lb[i] = sourceLb;
                }
                else if ( !FloatUtils::isPositive( sourceUb ) )
                {
                    // Phase active
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

                    // Set symbolic upper bound
                    // x_f <= (x_b - l) * u / ( u - l)
                    double coeff = sourceUb / ( sourceUb - sourceLb );
                    _symbolicUb[i] = coeff;
                    _symbolicUpperBias[i] = -sourceLb * coeff;
                    _ub[i] = sourceUb;

                    // Set symbolic lower bound
                    if ( sourceUb > -sourceLb )
                    {
                        // x_f >= x_b
                        // l = sourceLb
                        _symbolicLb[i] = 1;
                        _symbolicLowerBias[i] = 0;
                        _lb[i] = sourceLb;
                    }
                    else
                    {
                        // x_f >= 0
                        // l = 0
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
        }
        log( "Executing - done" );
    }

    void DeepPolyReLUElement::symbolicBoundInTermsOfPredecessor
    ( const double *symbolicLb, const double*symbolicUb, double
      *symbolicLowerBias, double *symbolicUpperBias, double
      *symbolicLbInTermsOfPredecessor, double *symbolicUbInTermsOfPredecessor,
      unsigned targetLayerSize, unsigned previousLayerSize,
      unsigned previousLayerIndex )
    {
        std::cout << previousLayerSize << previousLayerIndex << std::endl;
        std::fill_n( symbolicLbInTermsOfPredecessor, targetLayerSize *
                     previousLayerSize, 0 );
        std::fill_n( symbolicUbInTermsOfPredecessor, targetLayerSize *
                     previousLayerSize, 0 );

        unsigned size = getSize();

        for ( unsigned i = 0; i < size; ++i )
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
        allocateMemoryForUpperAndLowerBounds();

        unsigned size = getSize();
        _symbolicLb = new double[size];
        _symbolicUb = new double[size];

        std::fill_n( _symbolicLb, size, 0 );
        std::fill_n( _symbolicUb, size, 0 );

        _symbolicLowerBias = new double[size];
        _symbolicUpperBias = new double[size];

        std::fill_n( _symbolicLowerBias, size, 0 );
        std::fill_n( _symbolicUpperBias, size, 0 );
    }

    void DeepPolyReLUElement::log( const String &message )
    {
        if ( GlobalConfiguration::NETWORK_LEVEL_REASONER_LOGGING )
            printf( "DeepPolyReLUElement: %s\n", message.ascii() );
    }

} // namespace NLR
