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

        // Start with the symbolic upper-/lower- bounds of this layer with
        // respect to its immediate predecessor.
        unsigned predecessorIndex = getPredecessorIndex();
        log( Stringf( "Computing symbolic bounds with respect to layer %u...", predecessorIndex ) );
        DeepPolyElement *precedingElement = deepPolyElementsBefore[predecessorIndex];
        unsigned sourceLayerSize = precedingElement->getSize();

        const double *weights = _layer->getWeights( predecessorIndex );
        memcpy(_work1SymbolicLb, weights, _size * sourceLayerSize * sizeof(double) );
        memcpy(_work1SymbolicUb, weights, _size * sourceLayerSize * sizeof(double) );

        double *bias = _layer->getBiases();
        memcpy( _workSymbolicLowerBias, bias, _size * sizeof(double) );
        memcpy( _workSymbolicUpperBias, bias, _size * sizeof(double) );

        DeepPolyElement *currentElement = precedingElement;
        while ( currentElement->hasPredecessor() )
        {
            // We have the symbolic bounds in terms of the current abstract
            // element--currentElement, stored in _work1SymbolicLb,
            // _work1SymbolicUb, _workSymbolicLowerBias, _workSymbolicLowerBias,
            // now compute the symbolic bounds in terms of currentElement's
            // predecessor.

            precedingElement =
                deepPolyElementsBefore[precedingElement->getPredecessorIndex()];

            currentElement->symbolicBoundInTermsOfPredecessor
                ( _work1SymbolicLb, _work1SymbolicUb, _workSymbolicLowerBias,
                  _workSymbolicUpperBias, _work2SymbolicLb, _work2SymbolicUb,
                  _size, precedingElement );

            double* temp = _work1SymbolicLb;
            _work1SymbolicLb = _work2SymbolicLb;
            _work2SymbolicLb = temp;

            temp = _work1SymbolicUb;
            _work1SymbolicUb = _work2SymbolicUb;
            _work2SymbolicUb = temp;

            currentElement = precedingElement;
        }

        log( "Concretizing bound..." );
        std::fill_n( _lb, _size, 0 );
        std::fill_n( _ub, _size, 0 );
        // Get concrete bounds from the first element
        for ( unsigned i = 0; i < _size; ++i )
        {
            for ( unsigned j = 0; j < currentElement->getSize(); ++j )
            {
                double firstLb = currentElement->getLowerBound( j );
                double firstUb = currentElement->getUpperBound( j );
                // Compute lower bound
                double weight = _work1SymbolicLb[j * _size + i];
                if ( weight >= 0 )
                {
                    _lb[i] += ( weight * firstLb );
                } else
                {
                    _lb[i] += ( weight * firstUb );
                }

                // Compute upper bound
                weight = _work1SymbolicUb[j * _size + i];
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
      unsigned targetLayerSize, DeepPolyElement *predecessor )
    {
        unsigned predecessorIndex = predecessor->getLayerIndex();
        ASSERT( getPredecessorIndex() == predecessorIndex );
        log( Stringf( "Computing symbolic bounds with respect to layer %u...",
                      predecessorIndex ) );
        unsigned predecessorSize = predecessor->getSize();

        std::fill_n( symbolicLbInTermsOfPredecessor, targetLayerSize *
                     predecessorSize, 0 );
        std::fill_n( symbolicUbInTermsOfPredecessor, targetLayerSize *
                     predecessorSize, 0 );

        double *weights = _layer->getWeights( predecessorIndex );
        double *biases = _layer->getBiases();

        // newSymbolicLb = weights * symbolicLb
        // newSymbolicUb = weights * symbolicUb
        matrixMultiplication( weights, symbolicLb,
                              symbolicLbInTermsOfPredecessor, predecessorSize,
                              _size, targetLayerSize );
        matrixMultiplication( weights, symbolicUb,
                              symbolicUbInTermsOfPredecessor, predecessorSize,
                              _size, targetLayerSize );

        // symbolicLowerBias = biases * symbolicLb
        // symbolicUpperBias = biases * symbolicUb
        matrixMultiplication( biases, symbolicLb,
                              symbolicLowerBias, 1,
                              _size, targetLayerSize );
        matrixMultiplication( biases, symbolicUb,
                              symbolicUpperBias, 1,
                              _size, targetLayerSize );
    }

    void DeepPolyWeightedSumElement::log( const String &message )
    {
        if ( GlobalConfiguration::NETWORK_LEVEL_REASONER_LOGGING )
            printf( "DeepPolyWeightedSumElement: %s\n", message.ascii() );
    }

} // namespace NLR
