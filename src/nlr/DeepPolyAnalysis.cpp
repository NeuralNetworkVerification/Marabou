/*********************                                                        */
/*! \file DeepPolyAnalysis.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Haoze Andrew Wu
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2024 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

 **/

#include "DeepPolyAnalysis.h"

#include "Debug.h"
#include "DeepPolyAbsoluteValueElement.h"
#include "DeepPolyBilinearElement.h"
#include "DeepPolyInputElement.h"
#include "DeepPolyLeakyReLUElement.h"
#include "DeepPolyMaxPoolElement.h"
#include "DeepPolyReLUElement.h"
#include "DeepPolyRoundElement.h"
#include "DeepPolySigmoidElement.h"
#include "DeepPolySignElement.h"
#include "DeepPolySoftmaxElement.h"
#include "DeepPolyWeightedSumElement.h"
#include "FloatUtils.h"
#include "InfeasibleQueryException.h"
#include "Layer.h"
#include "MStringf.h"
#include "MatrixMultiplication.h"
#include "NLRError.h"
#include "TimeUtils.h"

#include <boost/thread.hpp>

namespace NLR {

DeepPolyAnalysis::DeepPolyAnalysis( LayerOwner *layerOwner )
    : _layerOwner( layerOwner )
    , _work1SymbolicLb( NULL )
    , _work1SymbolicUb( NULL )
    , _work2SymbolicLb( NULL )
    , _work2SymbolicUb( NULL )
    , _workSymbolicLowerBias( NULL )
    , _workSymbolicUpperBias( NULL )
{
    const Map<unsigned, Layer *> &layers = _layerOwner->getLayerIndexToLayer();
    // Get the maximal layer size
    unsigned maxLayerSize = 0;
    for ( const auto &pair : layers )
    {
        unsigned thisLayerSize = pair.second->getSize();
        if ( thisLayerSize > maxLayerSize )
            maxLayerSize = thisLayerSize;
    }
    _maxLayerSize = maxLayerSize;

    allocateMemory();
    for ( const auto &pair : layers )
    {
        /*
          Go over the layers, one by one. Each time construct and execute
          the abstract element.
        */
        unsigned index = pair.first;
        Layer *layer = pair.second;
        log( Stringf( "Creating deeppoly element for layer %u...", index ) );
        DeepPolyElement *deepPolyElement = createDeepPolyElement( layer );
        _deepPolyElements[index] = deepPolyElement;
        log( Stringf( "Creating deeppoly element for layer %u - done", index ) );
    }
}

DeepPolyAnalysis::~DeepPolyAnalysis()
{
    freeMemoryIfNeeded();
}

void DeepPolyAnalysis::freeMemoryIfNeeded()
{
    for ( const auto &pair : _deepPolyElements )
    {
        if ( pair.second )
            delete pair.second;
    }
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
    if ( _workSymbolicLowerBias )
    {
        delete[] _workSymbolicLowerBias;
        _workSymbolicLowerBias = NULL;
    }
    if ( _workSymbolicUpperBias )
    {
        delete[] _workSymbolicUpperBias;
        _workSymbolicUpperBias = NULL;
    }
}

void DeepPolyAnalysis::run()
{
    struct timespec deepPolyStart;
    (void)deepPolyStart;
    struct timespec deepPolyEnd;
    (void)deepPolyEnd;

    deepPolyStart = TimeUtils::sampleMicro();

    const Map<unsigned, Layer *> &layers = _layerOwner->getLayerIndexToLayer();
    for ( const auto &pair : layers )
    {
        /*
          Go over the layers, one by one. Each time construct and execute
          the abstract element.
        */
        unsigned index = pair.first;
        Layer *layer = pair.second;

        ASSERT( _deepPolyElements.exists( index ) );
        log( Stringf( "Running deeppoly analysis for layer %u...", index ) );
        DeepPolyElement *deepPolyElement = _deepPolyElements[index];
        deepPolyElement->execute( _deepPolyElements );

        // Extract updated bounds
        for ( unsigned j = 0; j < deepPolyElement->getSize(); ++j )
        {
            if ( layer->neuronEliminated( j ) )
                continue;
            double lb = deepPolyElement->getLowerBound( j );
            if ( layer->getLb( j ) < lb )
            {
                log( Stringf( "Neuron %u_%u lower-bound updated from  %f to %f",
                              index,
                              j,
                              layer->getLb( j ),
                              lb ) );
                layer->setLb( j, lb );
                _layerOwner->receiveTighterBound(
                    Tightening( layer->neuronToVariable( j ), lb, Tightening::LB ) );
            }
            double ub = deepPolyElement->getUpperBound( j );
            if ( layer->getUb( j ) > ub )
            {
                log( Stringf( "Neuron %u_%u upper-bound updated from  %f to %f",
                              index,
                              j,
                              layer->getUb( j ),
                              ub ) );
                layer->setUb( j, ub );
                _layerOwner->receiveTighterBound(
                    Tightening( layer->neuronToVariable( j ), ub, Tightening::UB ) );
            }
        }
        log( Stringf( "Running deeppoly analysis for layer %u - done", index ) );
    }
}

void DeepPolyAnalysis::allocateMemory()
{
    freeMemoryIfNeeded();
    _work1SymbolicLb = new double[_maxLayerSize * _maxLayerSize];
    _work1SymbolicUb = new double[_maxLayerSize * _maxLayerSize];
    _work2SymbolicLb = new double[_maxLayerSize * _maxLayerSize];
    _work2SymbolicUb = new double[_maxLayerSize * _maxLayerSize];

    _workSymbolicLowerBias = new double[_maxLayerSize];
    _workSymbolicUpperBias = new double[_maxLayerSize];

    std::fill_n( _work1SymbolicLb, _maxLayerSize * _maxLayerSize, 0 );
    std::fill_n( _work1SymbolicUb, _maxLayerSize * _maxLayerSize, 0 );
    std::fill_n( _work2SymbolicLb, _maxLayerSize * _maxLayerSize, 0 );
    std::fill_n( _work2SymbolicUb, _maxLayerSize * _maxLayerSize, 0 );

    std::fill_n( _workSymbolicLowerBias, _maxLayerSize, 0 );
    std::fill_n( _workSymbolicUpperBias, _maxLayerSize, 0 );
}

DeepPolyElement *DeepPolyAnalysis::createDeepPolyElement( Layer *layer )
{
    Layer::Type type = layer->getLayerType();
    DeepPolyElement *deepPolyElement;
    if ( type == Layer::INPUT )
        deepPolyElement = new DeepPolyInputElement( layer );
    else if ( type == Layer::WEIGHTED_SUM )
    {
        deepPolyElement = new DeepPolyWeightedSumElement( layer );
        // Weighted sum layers need working memory for back substitution
        deepPolyElement->setWorkingMemory( _work1SymbolicLb,
                                           _work1SymbolicUb,
                                           _work2SymbolicLb,
                                           _work2SymbolicUb,
                                           _workSymbolicLowerBias,
                                           _workSymbolicUpperBias );
    }
    else if ( type == Layer::RELU )
        deepPolyElement = new DeepPolyReLUElement( layer );
    else if ( type == Layer::ROUND )
        deepPolyElement = new DeepPolyRoundElement( layer );
    else if ( type == Layer::LEAKY_RELU )
        deepPolyElement = new DeepPolyLeakyReLUElement( layer );
    else if ( type == Layer::SIGN )
        deepPolyElement = new DeepPolySignElement( layer );
    else if ( type == Layer::ABSOLUTE_VALUE )
        deepPolyElement = new DeepPolyAbsoluteValueElement( layer );
    else if ( type == Layer::MAX )
        deepPolyElement = new DeepPolyMaxPoolElement( layer );
    else if ( type == Layer::SIGMOID )
        deepPolyElement = new DeepPolySigmoidElement( layer );
    else if ( type == Layer::SOFTMAX )
        deepPolyElement = new DeepPolySoftmaxElement( layer, _maxLayerSize );
    else if ( type == Layer::BILINEAR )
        deepPolyElement = new DeepPolyBilinearElement( layer );
    else
        throw NLRError( NLRError::LAYER_TYPE_NOT_SUPPORTED,
                        Stringf( "Layer %u not yet supported", layer->getLayerType() ).ascii() );
    return deepPolyElement;
}

void DeepPolyAnalysis::log( const String &message )
{
    if ( GlobalConfiguration::NETWORK_LEVEL_REASONER_LOGGING )
        printf( "DeepPolyAnalysis: %s\n", message.ascii() );
}

} // namespace NLR
