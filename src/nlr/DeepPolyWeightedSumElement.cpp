/*********************                                                        */
/*! \file DeepPolyWeightedSumElement.cpp
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

#include "DeepPolyWeightedSumElement.h"
#include "FloatUtils.h"
#include "NLRError.h"

namespace NLR {

    DeepPolyWeightedSumElement::DeepPolyWeightedSumElement( Layer *layer )
        : _work1( NULL )
        , _work2( NULL )
    {
        _layer = layer;
    }

    DeepPolyWeightedSumElement::~DeepPolyWeightedSumElement()
    {
        freeMemoryIfNeeded();
    }

    void DeepPolyWeightedSumElement::execute( const Map<unsigned, DeepPolyElement *>
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
            // Otherwise, compute bounds with back-substitution
            allocateMemory( deepPolyElementsBefore );
            computeBoundWithBackSubstitution( deepPolyElementsBefore );
        }
    }

    void DeepPolyWeightedSumElement::computeBoundWithBackSubstitution
    ( const Map<unsigned, DeepPolyElement *> &deepPolyElementsBefore )
    {
        for ( unsigned i = getLayerIndex() - 1; i >= 1; ++i )
        {
            // have sLb_k_i, sLBias_k_i, sUb_k_i, sUBias_k_i
            // Try to get sLb_k_i-1, sLBias_k_i-1, sUb_k_i-1, sUBias_k_i-1
            // Need sLb_i_i-1, sLBias_i_i-1, sUb_i_i-1, sUBias_i_i-1
            // sLb_k_i-1 = sLb_k_i_pos * sLB_i_i-1 + sLb_k_i_neg * sUB_i_i-1
            // sUb_k_i-1 = sUb_k_i_pos * sUB_i_i-1 + sUb_k_i_neg * sLB_i_i-1
            // sLBias_k_i-1 = sLBias_k_i-1 + sLb_k_i_pos * sLBias_i_i-1 + sLb_k_i_neg * sUBias_i_i-1
            // sUBias_k_i-1 = sUBias_k_i-1 + sUb_k_i_pos * sUBias_i_i-1 + sUb_k_i_neg * sLBias_i_i-1
        }
    }

    void DeepPolyWeightedSumElement::allocateMemory( const Map<unsigned,
                                                     DeepPolyElement *>
                                                     &deepPolyElementsBefore )
    {
        allocateMemoryForUpperAndLowerBounds();

        // Get the maximal layer size
        unsigned maxLayerSize = 0;
        for ( const auto &pair : deepPolyElementsBefore )
        {
            unsigned thisLayerSize = pair.second->getSize();
            if ( thisLayerSize > maxLayerSize )
                maxLayerSize = thisLayerSize;
        }

        _work1 = new double[maxLayerSize * maxLayerSize];
        _work2 = new double[maxLayerSize * maxLayerSize];

        std::fill_n( _work1, maxLayerSize * maxLayerSize, 0 );
        std::fill_n( _work2, maxLayerSize * maxLayerSize, 0 );
    }

    void DeepPolyWeightedSumElement::freeMemoryIfNeeded()
    {
        DeepPolyElement::freeMemoryIfNeeded();

        if ( _work1 )
        {
            delete[] _work1;
            _work1 = NULL;
        }

        if ( _work2 )
        {
            delete[] _work2;
            _work2 = NULL;
        }
    }


} // namespace NLR
