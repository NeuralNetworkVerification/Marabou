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
        : _work1SymbolicLb( NULL )
        , _work1SymbolicUb( NULL )
        , _work2SymbolicLb( NULL )
        , _work2SymbolicUb( NULL )
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
        unsigned sourceLayerIndex = getLayerIndex() - 1;

        // We start with the symbolic upper-/lower- bounds of this layer w.r.t.
        // its immediate predecessor, the goal is to compute the symbolic
        // upper-/lower- bounds of this layer w.r.t. the first layer.

        double *symbolicLb = _layer->getWeights( sourceLayerIndex );
        double *symbolicUb = _layer->getWeights( sourceLayerIndex );
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
            DeepPolyElement *layer = deepPolyElementsBefore[i];
            DeepPolyElement *previousLayer = deepPolyElementsBefore[i - 1];
            layer->symbolicBoundInTermsOfPredecessor
                ( symbolicLb, symbolicUb, symbolicLowerBias, symbolicUpperBias,
                  _work2SymbolicLb, _work2SymbolicUb, getSize(),
                  previousLayer->getSize(), sourceLayerIndex );
        }
    }

    void DeepPolyWeightedSumElement::symbolicBoundInTermsOfPredecessor
    ( const double *symbolicLb, const double*symbolicUb, double
      *symbolicLowerBias, double *symbolicUpperBias, double
      *symbolicLbInTermsOfPredecessor, double *symbolicUbInTermsOfPredecessor,
      unsigned targetLayerSize, unsigned previousLayerSize,
      unsigned previousLayerIndex )
    {
        // sLb_k_i-1 = sLb_k_i_pos * sLB_i_i-1 + sLb_k_i_neg * sUB_i_i-1
        // sUb_k_i-1 = sUb_k_i_pos * sUB_i_i-1 + sUb_k_i_neg * sLB_i_i-1
        // sLBias_k_i-1 = sLBias_k_i-1 + sLb_k_i_pos * sLBias_i_i-1 + sLb_k_i_neg * sUBias_i_i-1
        // sUBias_k_i-1 = sUBias_k_i-1 + sUb_k_i_pos * sUBias_i_i-1 + sUb_k_i_neg * sLBias_i_i-1
        std::fill_n( symbolicLbInTermsOfPredecessor, targetLayerSize *
                     previousLayerSize, 0 );
        std::fill_n( symbolicUbInTermsOfPredecessor, targetLayerSize *
                     previousLayerSize, 0 );

        unsigned size = getSize();
        double *weights = _layer->getWeights( previousLayerIndex );
        double *biases = _layer->getBiases();

        // newSymbolicLb = weights * symbolicLb
        // newSymbolicUb = weights * symbolicUb
        matrixMultiplication( weights, symbolicLb,
                              symbolicLbInTermsOfPredecessor, previousLayerSize,
                              size, targetLayerSize );
        matrixMultiplication( weights, symbolicUb,
                              symbolicUbInTermsOfPredecessor, previousLayerSize,
                              size, targetLayerSize );

        // symbolicLowerBias = biases * symbolicLb
        // symbolicUpperBias = biases * symbolicUb
        matrixMultiplication( biases, symbolicLb,
                              symbolicLowerBias, 1,
                              size, targetLayerSize );
        matrixMultiplication( biases, symbolicUb,
                              symbolicUpperBias, 1,
                              size, targetLayerSize );
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

       _work1SymbolicLb= new double[size * maxLayerSize];
       _work1SymbolicUb= new double[size * maxLayerSize];
       _work2SymbolicLb= new double[size * maxLayerSize];
       _work2SymbolicUb= new double[size * maxLayerSize];

       _workSymbolicLowerBias = new double[size];
       _workSymbolicUpperBias = new double[size];


       std::fill_n( _work1SymbolicLb, size * maxLayerSize, 0 );
       std::fill_n( _work1SymbolicUb, size * maxLayerSize, 0 );
       std::fill_n( _work2SymbolicLb, size * maxLayerSize, 0 );
       std::fill_n( _work2SymbolicUb, size * maxLayerSize, 0 );

       std::fill_n( _workSymbolicLowerBias, size, 0 );
       std::fill_n( _workSymbolicUpperBias, size, 0 );
    }

    void DeepPolyWeightedSumElement::freeMemoryIfNeeded()
    {
        DeepPolyElement::freeMemoryIfNeeded();
        if ( _work1SymbolicLb )
        {
            delete[] _work1SymbolicLb;
            _work1SymbolicLb = NULL;
        }
        if ( _work2SymbolicLb )
        {
            delete[] _work2SymbolicLb;
            _work2SymbolicLb = NULL;
        }
        if ( _work1SymbolicUb )
        {
            delete[] _work1SymbolicUb;
            _work1SymbolicUb = NULL;
        }
        if ( _work2SymbolicUb )
        {
            delete[] _work2SymbolicUb;
            _work2SymbolicUb = NULL;
        }
    }


} // namespace NLR
