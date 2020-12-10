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
                    _lb[i] = sourceUb;
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

} // namespace NLR
