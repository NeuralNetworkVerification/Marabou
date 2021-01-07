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

#include <string.h>

namespace NLR {

DeepPolyWeightedSumElement::DeepPolyWeightedSumElement( Layer *layer )
    : _workLb( NULL )
    , _workUb( NULL )
{
    _layer = layer;
    _size = layer->getSize();
    _layerIndex = layer->getLayerIndex();
}

DeepPolyWeightedSumElement::~DeepPolyWeightedSumElement()
{
    freeMemoryIfNeeded();
}

void DeepPolyWeightedSumElement::execute
( const Map<unsigned, DeepPolyElement *> &deepPolyElementsBefore )
{
    log( "Executing..." );
    ASSERT( hasPredecessor() );
    allocateMemory();
    // Otherwise, compute bounds with back-substitution
    computeBoundWithBackSubstitution( deepPolyElementsBefore );
    log( "Executing - done" );
}

void DeepPolyWeightedSumElement::computeBoundWithBackSubstitution
( const Map<unsigned, DeepPolyElement *> &deepPolyElementsBefore )
{
    log( "Computing bounds with back substitution..." );

    // Start with the symbolic upper-/lower- bounds of this layer with
    // respect to its immediate predecessor.
    const Map<unsigned, unsigned> &predecessorIndices = getPredecessorIndices();
    // Right now, assume that each layer has one predecessor.
    DEBUG({
            std::cout << "Source layers of layer "
                      << _layerIndex << ": ";
            for ( const auto &pair : predecessorIndices )
                std::cout << pair.first << " ";
            std::cout << std::endl;
        });

    // For now, assumes that each weighted sum layer has one source layer.
    ASSERT( predecessorIndices.size() == 1 );
    unsigned predecessorIndex = predecessorIndices.begin()->first;
    log( Stringf( "Computing symbolic bounds with respect to layer %u...",
                  predecessorIndex ) );
    DeepPolyElement *precedingElement =
        deepPolyElementsBefore[predecessorIndex];
    unsigned sourceLayerSize = precedingElement->getSize();

    const double *weights = _layer->getWeights( predecessorIndex );
    memcpy(_work1SymbolicLb,
           weights, _size * sourceLayerSize * sizeof(double) );
    memcpy(_work1SymbolicUb,
           weights, _size * sourceLayerSize * sizeof(double) );

    double *bias = _layer->getBiases();
    memcpy( _workSymbolicLowerBias, bias, _size * sizeof(double) );
    memcpy( _workSymbolicUpperBias, bias, _size * sizeof(double) );

    DeepPolyElement *currentElement = precedingElement;
    concretizeSymbolicBound( _work1SymbolicLb, _work1SymbolicUb,
                             _workSymbolicLowerBias,
                             _workSymbolicUpperBias,
                             currentElement );

    while ( currentElement->hasPredecessor() )
    {
        // We have the symbolic bounds in terms of the current abstract
        // element--currentElement, stored in _work1SymbolicLb,
        // _work1SymbolicUb, _workSymbolicLowerBias, _workSymbolicLowerBias,
        // now compute the symbolic bounds in terms of currentElement's
        // predecessor.

        precedingElement =
            deepPolyElementsBefore[precedingElement->
                                   getPredecessorIndices().begin()->first];

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
        concretizeSymbolicBound( _work1SymbolicLb, _work1SymbolicUb,
                                 _workSymbolicLowerBias,
                                 _workSymbolicUpperBias,
                                 currentElement );
    }
    log( "Computing bounds with back substitution - done" );
}

void DeepPolyWeightedSumElement::concretizeSymbolicBound
( const double *symbolicLb, const double*symbolicUb, double const
  *symbolicLowerBias, const double *symbolicUpperBias, DeepPolyElement
  *sourceElement )
{
    log( "Concretizing bound..." );
    std::fill_n( _workLb, _size, 0 );
    std::fill_n( _workUb, _size, 0 );

    // Get concrete bounds from the first element
    for ( unsigned i = 0; i < _size; ++i )
    {
        for ( unsigned j = 0; j < sourceElement->getSize(); ++j )
        {
            double firstLb = sourceElement->getLowerBound( j );
            double firstUb = sourceElement->getUpperBound( j );
            // Compute lower bound
            double weight = symbolicLb[j * _size + i];
            if ( weight >= 0 )
            {
                _workLb[i] += ( weight * firstLb );
            } else
            {
                _workLb[i] += ( weight * firstUb );
            }

            // Compute upper bound
            weight = symbolicUb[j * _size + i];
            if ( weight >= 0 )
            {
                _workUb[i] += ( weight * firstUb );
            } else
            {
                _workUb[i] += ( weight * firstLb );
            }
        }
        _workLb[i] += symbolicLowerBias[i];
        _workUb[i] += symbolicUpperBias[i];

        if ( _lb[i] < _workLb[i] )
            _lb[i] = _workLb[i];
        if ( _ub[i] > _workUb[i] )
            _ub[i] = _workUb[i];
        log( Stringf( "Neuron%u LB: %f, UB: %f", i, _lb[i], _ub[i] ) );
    }
    log( "Concretizing bound - done" );
}

void DeepPolyWeightedSumElement::symbolicBoundInTermsOfPredecessor
( const double *symbolicLb, const double*symbolicUb, double
  *symbolicLowerBias, double *symbolicUpperBias, double
  *symbolicLbInTermsOfPredecessor, double *symbolicUbInTermsOfPredecessor,
  unsigned targetLayerSize, DeepPolyElement *predecessor )
{
    unsigned predecessorIndex = predecessor->getLayerIndex();
    ASSERT( getPredecessorIndices().begin()->first == predecessorIndex );
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

void DeepPolyWeightedSumElement::allocateMemory()
{
    freeMemoryIfNeeded();

    DeepPolyElement::allocateMemory();

    _workLb = new double[_size];
    _workUb = new double[_size];

    std::fill_n( _workLb, _size, FloatUtils::negativeInfinity() );
    std::fill_n( _workUb, _size, FloatUtils::infinity() );
}

void DeepPolyWeightedSumElement::freeMemoryIfNeeded()
{
    DeepPolyElement::freeMemoryIfNeeded();
    if ( _workLb )
    {
        delete[] _workLb;
        _workLb = NULL;
    }
    if ( _workUb )
    {
        delete[] _workUb;
        _workUb = NULL;
    }
}

void DeepPolyWeightedSumElement::log( const String &message )
{
    if ( GlobalConfiguration::NETWORK_LEVEL_REASONER_LOGGING )
        printf( "DeepPolyWeightedSumElement: %s\n", message.ascii() );
}

} // namespace NLR
