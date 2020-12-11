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

        const double *symbolicLb = _layer->getWeights( sourceLayerIndex );
        const double *symbolicUb = _layer->getWeights( sourceLayerIndex );

        unsigned size = getSize();
        double *biases = _layer->getBiases();
        std::memcpy( _workSymbolicLowerBias, biases, size );
        std::memcpy( _workSymbolicUpperBias, biases, size );

        for ( unsigned i = getLayerIndex() - 1; i >= 1; ++i )
        {
            DeepPolyElement *layer = deepPolyElementsBefore[i];
            DeepPolyElement *previousLayer = deepPolyElementsBefore[i - 1];
            layer->symbolicBoundInTermsOfPredecessor
                ( symbolicLb, symbolicUb, _workSymbolicLowerBias,
                  _workSymbolicUpperBias, _work2SymbolicLb, _work2SymbolicUb,
                  getSize(), previousLayer->getSize(), sourceLayerIndex );
            double* temp = _work1SymbolicLb;
            _work1SymbolicLb = _work2SymbolicLb;
            _work2SymbolicLb = temp;

            temp = _work1SymbolicUb;
            _work1SymbolicUb = _work2SymbolicUb;
            _work2SymbolicUb = temp;
        }

        DeepPolyElement *firstElement = deepPolyElementsBefore[0];
        // Get concrete bounds from the first element
        for ( unsigned i = 0; i < size; ++i )
        {
            for ( unsigned j = 0; j < firstElement->getSize(); ++j )
            {
                double firstLb = firstElement->getLowerBound( j );
                double firstUb = firstElement->getUpperBound( j );

                // Compute lower bound
                double weight = _work1SymbolicLb[j * size + i];
                if ( weight >= 0 )
                {
                    _lb[i] += ( weight * firstLb );
                } else
                {
                    _lb[i] += ( weight * firstUb );
                }

                // Compute upper bound
                weight = _work1SymbolicUb[j * size + i];
                if ( weight >= 0 )
                {
                    _ub[i] += ( weight * firstUb );
                } else
                {
                    _ub[i] += ( weight * firstLb );
                }

            }
        }
    }

    void DeepPolyWeightedSumElement::symbolicBoundInTermsOfPredecessor
    ( const double *symbolicLb, const double*symbolicUb, double
      *symbolicLowerBias, double *symbolicUpperBias, double
      *symbolicLbInTermsOfPredecessor, double *symbolicUbInTermsOfPredecessor,
      unsigned targetLayerSize, unsigned previousLayerSize,
      unsigned previousLayerIndex )
    {
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
