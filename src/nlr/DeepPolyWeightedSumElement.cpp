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
        : _workSymbolicLbPositiveWeights( NULL )
        , _workSymbolicLbNegativeWeights( NULL )
        , _workSymbolicUbPositiveWeights( NULL )
        , _workSymbolicUbNegativeWeights( NULL )
        , _workSymbolicLb( NULL )
        , _workSymbolicUb( NULL )
        , _workSymbolicLowerBias( NULL )
        , _workSymbolicUpperBias( NULL )
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
        unsigned sourceLayerIndex = deepPolyElementsBefore[getLayerIndex() - 1]
            ->getLayerIndex();

        // We start with the symbolic upper-/lower- bounds of this layer w.r.t.
        // its immediate predecessor, the goal is to compute the symbolic
        // upper-/lower- bounds of this layer w.r.t. the first layer.

        double *symbolicLbPositiveWeights =
            _layer->getPositiveWeights( sourceLayerIndex );
        double *symbolicLbNegativeWeights =
            _layer->getNegativeWeights( sourceLayerIndex );
        double *symbolicUbPositiveWeights =
            _layer->getPositiveWeights( sourceLayerIndex );
        double *symbolicUbNegativeWeights =
            _layer->getNegativeWeights( sourceLayerIndex );
        double *symbolicLowerBias = _layer->getBiases();
        double *symbolicUpperBias = _layer->getBiases();

        for ( unsigned i = getLayerIndex() - 1; i >= 1; ++i )
        {
            // have sLb_k_i, sLBias_k_i, sUb_k_i, sUBias_k_i
            // sLb_i_i-1, sLBias_i_i-1, sUb_i_i-1, sUBias_i_i-1
            // Try to get sLb_k_i-1, sLBias_k_i-1, sUb_k_i-1, sUBias_k_i-1
            // sLb_k_i-1 = sLb_k_i_pos * sLB_i_i-1 + sLb_k_i_neg * sUB_i_i-1
            // sUb_k_i-1 = sUb_k_i_pos * sUB_i_i-1 + sUb_k_i_neg * sLB_i_i-1
            // sLBias_k_i-1 = sLBias_k_i-1 + sLb_k_i_pos * sLBias_i_i-1 + sLb_k_i_neg * sUBias_i_i-1
            // sUBias_k_i-1 = sUBias_k_i-1 + sUb_k_i_pos * sUBias_i_i-1 + sUb_k_i_neg * sLBias_i_i-1
            DeepPolyElement *lastLayer = deepPolyElementsBefore[i];
            lastLayer->symbolicBoundInTermsOfPredecessor
                ( symbolicLbPositiveWeights, symbolicLbNegativeWeights,
                  symbolicUbPositiveWeights, symbolicUbNegativeWeights,
                  symbolicLowerBias, symbolicUpperBias, _workSymbolicLb,
                  _workSymbolicUb );
        }
    }

    void DeepPolyWeightedSumElement::allocateMemory( const Map<unsigned,
                                                     DeepPolyElement *>
                                                     &deepPolyElementsBefore )
    {
        allocateMemoryForUpperAndLowerBounds();

        unsigned size = getSize();
        // Get the maximal layer size
        unsigned maxLayerSize = 0;
        for ( const auto &pair : deepPolyElementsBefore )
        {
            unsigned thisLayerSize = pair.second->getSize();
            if ( thisLayerSize > maxLayerSize )
                maxLayerSize = thisLayerSize;
        }

       _workSymbolicLbPositiveWeights= new double[size * maxLayerSize];
       _workSymbolicLbNegativeWeights= new double[size * maxLayerSize];
       _workSymbolicUbPositiveWeights= new double[size * maxLayerSize];
       _workSymbolicUbNegativeWeights= new double[size * maxLayerSize];
       _workSymbolicLb= new double[size * maxLayerSize];
       _workSymbolicUb= new double[size * maxLayerSize];
       _workSymbolicLowerBias = new double[size];
       _workSymbolicUpperBias = new double[size];

       std::fill_n( _workSymbolicLbPositiveWeights, size * maxLayerSize, 0 );
       std::fill_n( _workSymbolicLbNegativeWeights, size * maxLayerSize, 0 );
       std::fill_n( _workSymbolicUbPositiveWeights, size * maxLayerSize, 0 );
       std::fill_n( _workSymbolicUbNegativeWeights, size * maxLayerSize, 0 );
       std::fill_n( _workSymbolicLb, size * maxLayerSize, 0 );
       std::fill_n( _workSymbolicUb, size * maxLayerSize, 0 );

       std::fill_n( _workSymbolicLowerBias, size, 0 );
       std::fill_n( _workSymbolicUpperBias, size, 0 );
    }

    void DeepPolyWeightedSumElement::freeMemoryIfNeeded()
    {
        DeepPolyElement::freeMemoryIfNeeded();
    }


} // namespace NLR
