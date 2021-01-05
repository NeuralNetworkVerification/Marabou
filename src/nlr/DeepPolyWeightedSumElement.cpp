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

#include <string.h>

namespace NLR {

    DeepPolyWeightedSumElement::DeepPolyWeightedSumElement( Layer *layer )
    {
        _layer = layer;
        _size = layer->getSize();
        _layerIndex = layer->getLayerIndex();
    }

    DeepPolyWeightedSumElement::~DeepPolyWeightedSumElement()
    {
        freeMemoryIfNeeded();
    }

    void DeepPolyWeightedSumElement::execute( const Map<unsigned, DeepPolyElement *>
                                   &deepPolyElementsBefore )
    {
        log( "Executing..." );
        allocateMemory();
        if ( deepPolyElementsBefore.empty() )
        {
            // If this is the first layer, just update the concrete bounds
            getConcreteBounds();
        } else
        {
            // Otherwise, compute bounds with back-substitution
            computeBoundWithBackSubstitution( deepPolyElementsBefore );
        }
        log( "Executing - done" );
    }

    void DeepPolyWeightedSumElement::computeBoundWithBackSubstitution
    ( const Map<unsigned, DeepPolyElement *> &deepPolyElementsBefore )
    {
        log( "Computing bounds with back substitution..." );

        // We start with the symbolic upper-/lower- bounds of this layer w.r.t.
        // its immediate predecessor, the goal is to compute the symbolic
        // upper-/lower- bounds of this layer w.r.t. the first layer.

        log( "Computing symbolic bounds w.r.t. the first element..." );

        Map<unsigned, unsigned> sourceLayerToSize = _layer->getSourceLayers();
        // Right now, assume that each layer has one source layer
        ASSERT( sourceLayerToSize.size() == 1 );
        unsigned sourceLayerIndex = _layer->getSourceLayers().begin()->first;
        unsigned previousSize = deepPolyElementsBefore[sourceLayerIndex]->getSize();
        unsigned size = getSize();


        const double *symbolicBound = _layer->getWeights( sourceLayerIndex );
        unsigned matrixSize = size * previousSize;
        memcpy(_work1SymbolicLb, symbolicBound, matrixSize * sizeof(double) );
        memcpy(_work1SymbolicUb, symbolicBound, matrixSize * sizeof(double) );

        double *bias = _layer->getBiases();
        memcpy( _workSymbolicLowerBias, bias, size * sizeof(double) );
        memcpy( _workSymbolicUpperBias, bias, size * sizeof(double) );

        if ( sourceLayerIndex == 0 )
        {
            log( "Concretizing bound..." );
            std::fill_n( _lb, size, 0 );
            std::fill_n( _ub, size, 0 );
            DeepPolyElement *firstElement = deepPolyElementsBefore[0];
            // Get concrete bounds from the first element
            for ( unsigned i = 0; i < size; ++i )
            {
                for ( unsigned j = 0; j < firstElement->getSize(); ++j )
                {
                    double firstLb = firstElement->getLowerBound( j );
                    double firstUb = firstElement->getUpperBound( j );
                    // Compute lower bound
                    double weight = symbolicBound[j * size + i];
                    if ( weight >= 0 )
                    {
                        _lb[i] += ( weight * firstLb );
                    } else
                    {
                        _lb[i] += ( weight * firstUb );
                    }

                    // Compute upper bound
                    weight = symbolicBound[j * size + i];
                    if ( weight >= 0 )
                    {
                        _ub[i] += ( weight * firstUb );
                    } else
                    {
                        _ub[i] += ( weight * firstLb );
                    }
                }
                _lb[i] += _workSymbolicLowerBias[i];
                _ub[i] += _workSymbolicUpperBias[i];
                log( Stringf( "Neuron%u LB: %f, UB: %f", i, _lb[i], _ub[i] ) );
            }
            log( "Concretizing bound - done" );
            log( "Computing bounds with back substitution - done" );
            return;
        }

        for ( unsigned i = getLayerIndex() - 1; i >= 1; --i )
        {
            DeepPolyElement *layer = deepPolyElementsBefore[i];
            DeepPolyElement *previousLayer = deepPolyElementsBefore[i - 1];
            layer->symbolicBoundInTermsOfPredecessor
                ( _work1SymbolicLb, _work1SymbolicUb, _workSymbolicLowerBias,
                  _workSymbolicUpperBias, _work2SymbolicLb, _work2SymbolicUb,
                  getSize(), previousLayer->getSize(), i - 1 );
            double* temp = _work1SymbolicLb;
            _work1SymbolicLb = _work2SymbolicLb;
            _work2SymbolicLb = temp;

            temp = _work1SymbolicUb;
            _work1SymbolicUb = _work2SymbolicUb;
            _work2SymbolicUb = temp;
        }
        log( "Computing symbolic bounds w.r.t. the first element - done" );

        log( "Concretizing bound..." );
        std::fill_n( _lb, size, 0 );
        std::fill_n( _ub, size, 0 );
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
            _lb[i] += _workSymbolicLowerBias[i];
            _ub[i] += _workSymbolicUpperBias[i];
            log( Stringf( "Neuron%u LB: %f, UB: %f", i, _lb[i], _ub[i] ) );
        }
        log( "Concretizing bound - done" );
        log( "Computing bounds with back substitution - done" );
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

    void DeepPolyWeightedSumElement::log( const String &message )
    {
        if ( GlobalConfiguration::NETWORK_LEVEL_REASONER_LOGGING )
            printf( "DeepPolyWeightedSumElement: %s\n", message.ascii() );
    }

} // namespace NLR
