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
    Map<unsigned, unsigned> predecessorIndices = getPredecessorIndices();

    DEBUG({
            std::cout << "Source layers of layer "
                      << _layerIndex << ": ";
            for ( const auto &pair : predecessorIndices )
                std::cout << pair.first << " ";
            std::cout << std::endl;
        });

    unsigned counter = 0;
    unsigned numPredecessors = predecessorIndices.size();
    ASSERT( numPredecessors > 0 );
    unsigned predecessorIndex = 0;
    for ( const auto &pair : predecessorIndices )
    {
        predecessorIndex = pair.first;
        if ( counter < numPredecessors - 1 )
        {
            log( Stringf( "Adding residual from layer %u...",
                          predecessorIndex ) );
            allocateMemoryForResidualsIfNeeded( predecessorIndex, pair.second );
            const double *weights = _layer->getWeights( predecessorIndex );
            memcpy( _residualLb[predecessorIndex], weights,
                    _size * pair.second * sizeof(double) );
            memcpy( _residualUb[predecessorIndex], weights,
                    _size * pair.second * sizeof(double) );
            ++counter;
            log( Stringf( "Adding residual from layer %u - done", pair.first ) );
        }
    }

    log( Stringf( "Computing symbolic bounds with respect to layer %u...",
                  predecessorIndex ) );
    DeepPolyElement *precedingElement =
        deepPolyElementsBefore[predecessorIndex];
    unsigned sourceLayerSize = precedingElement->getSize();

    const double *weights = _layer->getWeights( predecessorIndex );
    memcpy( _work1SymbolicLb,
            weights, _size * sourceLayerSize * sizeof(double) );
    memcpy( _work1SymbolicUb,
            weights, _size * sourceLayerSize * sizeof(double) );

    double *bias = _layer->getBiases();
    memcpy( _workSymbolicLowerBias, bias, _size * sizeof(double) );
    memcpy( _workSymbolicUpperBias, bias, _size * sizeof(double) );

    DeepPolyElement *currentElement = precedingElement;
    concretizeSymbolicBound( _work1SymbolicLb, _work1SymbolicUb,
                             _workSymbolicLowerBias,
                             _workSymbolicUpperBias,
                             currentElement, deepPolyElementsBefore );
    log( Stringf( "Computing symbolic bounds with respect to layer %u - done",
                  predecessorIndex ) );

    while ( currentElement->hasPredecessor() )
    {
        // We have the symbolic bounds in terms of the current abstract
        // element--currentElement, stored in _work1SymbolicLb,
        // _work1SymbolicUb, _workSymbolicLowerBias, _workSymbolicLowerBias,
        // now compute the symbolic bounds in terms of currentElement's
        // predecessor.
        predecessorIndices = currentElement->getPredecessorIndices();
        counter = 0;
        numPredecessors = predecessorIndices.size();
        ASSERT( numPredecessors > 0 );
        for ( const auto &pair : predecessorIndices )
        {
            if ( counter < numPredecessors - 1 )
            {
                unsigned residualLayerIndex = pair.first;
                log( Stringf( "Adding residual from layer %u...", residualLayerIndex ) );
                allocateMemoryForResidualsIfNeeded( residualLayerIndex,
                                                    pair.second );
                DeepPolyElement *residualElement =
                    deepPolyElementsBefore[residualLayerIndex];
                // Do we need to add bias here?
                currentElement->symbolicBoundInTermsOfPredecessor
                    ( _work1SymbolicLb, _work1SymbolicUb, NULL, NULL,
                      _residualLb[residualLayerIndex],
                      _residualUb[residualLayerIndex],
                      _size, residualElement );
                ++counter;
                log( Stringf( "Adding residual from layer %u - done", pair.first ) );
            }
            else
            {
                predecessorIndex = pair.first;
            }
        }
        precedingElement = deepPolyElementsBefore[predecessorIndex];
        std::fill_n( _work2SymbolicLb, _size * precedingElement->getSize(), 0 );
        std::fill_n( _work2SymbolicUb, _size * precedingElement->getSize(), 0 );
        currentElement->symbolicBoundInTermsOfPredecessor
            ( _work1SymbolicLb, _work1SymbolicUb, _workSymbolicLowerBias,
              _workSymbolicUpperBias, _work2SymbolicLb, _work2SymbolicUb,
              _size, precedingElement );

        // The symbolic lower-bound is
        // _work2SymbolicLb * precedingElement + residualLb1 * residualElement1 +
        // residualLb2 * residualElement2 + ...
        // If the precedingElement is a residual source layer, we can merge
        // in the residualWeights, and remove it from the residual source layers.
        if ( _residualLayerIndices.exists( predecessorIndex ) )
        {
            log( Stringf( "merge residual from layer %u...", predecessorIndex ) );
            // Add weights of this residual layer
            for ( unsigned i = 0; i < _size * precedingElement->getSize(); ++i )
            {
                _work2SymbolicLb[i] += _residualLb[predecessorIndex][i];
                _work2SymbolicUb[i] += _residualUb[predecessorIndex][i];
            }
            _residualLayerIndices.erase( predecessorIndex );
            std::fill_n( _residualLb[predecessorIndex],
                         _size * precedingElement->getSize(), 0 );
            std::fill_n( _residualUb[predecessorIndex],
                         _size * precedingElement->getSize(), 0 );
            log( Stringf( "merge residual from layer %u - done", predecessorIndex ) );
        }

        DEBUG({
                // Residual layers topologically after precedingElement should
                // have been merged already.
                for ( const auto &residualLayerIndex : _residualLayerIndices )
                {
                    ASSERT( residualLayerIndex < predecessorIndex );
                }
            });

        double *temp = _work1SymbolicLb;
        _work1SymbolicLb = _work2SymbolicLb;
        _work2SymbolicLb = temp;

        temp = _work1SymbolicUb;
        _work1SymbolicUb = _work2SymbolicUb;
        _work2SymbolicUb = temp;

        currentElement = precedingElement;
        concretizeSymbolicBound( _work1SymbolicLb, _work1SymbolicUb,
                                 _workSymbolicLowerBias, _workSymbolicUpperBias,
                                 currentElement, deepPolyElementsBefore );
    }
    ASSERT( _residualLayerIndices.empty() );
    log( "Computing bounds with back substitution - done" );
}

void DeepPolyWeightedSumElement::concretizeSymbolicBound
( const double *symbolicLb, const double*symbolicUb, double const
  *symbolicLowerBias, const double *symbolicUpperBias, DeepPolyElement
  *sourceElement, const Map<unsigned, DeepPolyElement *>
  &deepPolyElementsBefore )
{
    log( "Concretizing bound..." );
    std::fill_n( _workLb, _size, 0 );
    std::fill_n( _workUb, _size, 0 );

    concretizeSymbolicBoundForSourceLayer( symbolicLb, symbolicUb,
                                           symbolicLowerBias, symbolicUpperBias,
                                           sourceElement );

    for ( const auto &residualLayerIndex : _residualLayerIndices )
    {
        ASSERT( residualLayerIndex < sourceElement->getLayerIndex() );
        DeepPolyElement *residualElement =
            deepPolyElementsBefore[residualLayerIndex];
        concretizeSymbolicBoundForSourceLayer( _residualLb[residualLayerIndex],
                                               _residualUb[residualLayerIndex],
                                               NULL,
                                               NULL,
                                               residualElement );
    }
    for ( unsigned i = 0; i <_size; ++i )
    {
        if ( _lb[i] < _workLb[i] )
            _lb[i] = _workLb[i];
        if ( _ub[i] > _workUb[i] )
            _ub[i] = _workUb[i];
        log( Stringf( "Neuron%u working LB: %f, UB: %f", i, _workLb[i], _workUb[i] ) );
        log( Stringf( "Neuron%u LB: %f, UB: %f", i, _lb[i], _ub[i] ) );
    }

    log( "Concretizing bound - done" );
}

void DeepPolyWeightedSumElement::concretizeSymbolicBoundForSourceLayer
( const double *symbolicLb, const double*symbolicUb, const double
  *symbolicLowerBias, const double *symbolicUpperBias, DeepPolyElement
  *sourceElement )
{
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
        if ( symbolicLowerBias )
            _workLb[i] += symbolicLowerBias[i];
        if ( symbolicUpperBias )
        _workUb[i] += symbolicUpperBias[i];
    }
}


void DeepPolyWeightedSumElement::symbolicBoundInTermsOfPredecessor
( const double *symbolicLb, const double*symbolicUb, double
  *symbolicLowerBias, double *symbolicUpperBias, double
  *symbolicLbInTermsOfPredecessor, double *symbolicUbInTermsOfPredecessor,
  unsigned targetLayerSize, DeepPolyElement *predecessor )
{
    unsigned predecessorIndex = predecessor->getLayerIndex();
    log( Stringf( "Computing symbolic bounds with respect to layer %u...",
                  predecessorIndex ) );
    unsigned predecessorSize = predecessor->getSize();

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
    if  ( symbolicLowerBias )
        matrixMultiplication( biases, symbolicLb, symbolicLowerBias, 1,
                              _size, targetLayerSize );
    if  ( symbolicUpperBias )
        matrixMultiplication( biases, symbolicUb, symbolicUpperBias, 1,
                              _size, targetLayerSize );
    log( Stringf( "Computing symbolic bounds with respect to layer %u - done",
                  predecessorIndex ) );
}

void DeepPolyWeightedSumElement::allocateMemoryForResidualsIfNeeded
( unsigned residualLayerIndex, unsigned residualLayerSize )
{
    _residualLayerIndices.insert( residualLayerIndex );
    unsigned matrixSize = residualLayerSize * _size;
    if ( !_residualLb.exists( residualLayerIndex ) )
    {
        double *residualLb = new double[matrixSize];
        std::fill_n( residualLb, matrixSize, 0 );
        _residualLb[residualLayerIndex] = residualLb;
    }
    if ( !_residualUb.exists( residualLayerIndex ) )
    {
        double *residualUb = new double[residualLayerSize * _size];
        std::fill_n( residualUb, matrixSize, 0 );
        _residualUb[residualLayerIndex] = residualUb;
    }
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
    for ( auto const &pair : _residualLb )
    {
        delete[] pair.second;
    }
    _residualLb.clear();
    for ( auto const &pair : _residualUb )
    {
        delete[] pair.second;
    }
    _residualUb.clear();
    _residualLayerIndices.clear();
}

void DeepPolyWeightedSumElement::log( const String &message )
{
    if ( GlobalConfiguration::NETWORK_LEVEL_REASONER_LOGGING )
        printf( "DeepPolyWeightedSumElement: %s\n", message.ascii() );
}

} // namespace NLR
