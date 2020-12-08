/*********************                                                        */
/*! \file DeepPolyAnalysis.cpp
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

#include "Debug.h"
#include "DeepPolyAnalysis.h"
#include "FloatUtils.h"
#include "InfeasibleQueryException.h"
#include "Layer.h"
#include "MatrixMultiplication.h"
#include "MStringf.h"
#include "NLRError.h"
#include "TimeUtils.h"

#include <boost/thread.hpp>

namespace NLR {

DeepPolyAnalysis::DeepPolyAnalysis( LayerOwner *layerOwner )
    : _layerOwner( layerOwner )
    , _workSymbolicLb( NULL )
    , _workSymbolicUb( NULL )
    , _workSymbolicLowerBias( NULL )
    , _workSymbolicUpperBias( NULL )
    , _startLayerIndex( 0 )
{
    allocateMemory();

    _endLayerIndex = _layerOwner->getNumberOfLayers() - 1;
}

DeepPolyAnalysis::~DeepPolyAnalysis()
{
    freeMemoryIfNeeded();
}

void DeepPolyAnalysis::allocateMemory()
{
    unsigned maxLayerSize = _layerOwner->getMaxLayerSize();
    _workSymbolicLb = new double[maxLayerSize * maxLayerSize];
    _workSymbolicUb = new double[maxLayerSize * maxLayerSize];
    _workSymbolicLowerBias = new double[maxLayerSize];
    _workSymbolicUpperBias = new double[maxLayerSize];
}

void DeepPolyAnalysis::freeMemoryIfNeeded()
{
    if ( _workSymbolicLb )
    {
        delete[] _workSymbolicLb;
        _workSymbolicLb = NULL;
    }

    if ( _workSymbolicUb )
    {
        delete[] _workSymbolicUb;
        _workSymbolicUb = NULL;
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

void DeepPolyAnalysis::run( const Map<unsigned, Layer *> &layers )
{
    struct timespec deepPolyStart;
    (void) deepPolyStart;
    struct timespec deepPolyEnd;
    (void) deepPolyEnd;

    deepPolyStart = TimeUtils::sampleMicro();

    for ( unsigned i = _startLayerIndex; i <= _endLayerIndex; ++i )
    {
        /*
          Go over the layers, one by one. Each time construct the abstract element
          and tighten the bounds
        */
        ASSERT( layers.exists( i ) );
        Layer *layer = layers[i];

        DeepPolyElement *deepPolyElement = new DeepPolyElement( layer );
        executeDeepPolyElement( *deepPolyElement );
        _layerIndexTodeepPolyElement[i] = deepPolyElement;
    }
}

void DeepPolyAnalysis::executeDeepPolyElement( DeepPolyElement &deepPolyElement )
{
    switch ( deepPolyElement._layer->getLayerType() )
    {
    case Layer::INPUT:
        executeDeepPolyElementForInput( deepPolyElement );
        break;
    case Layer::WEIGHTED_SUM:
        executeDeepPolyElementForWeightedSum( deepPolyElement );
        break;
    case Layer::RELU:
        executeDeepPolyElementForReLU( deepPolyElement );
        break;
    default:
        throw NLRError( NLRError::LAYER_TYPE_NOT_SUPPORTED,
                        Stringf( "Layer %u not yet supported",
                                 deepPolyElement._layer->getLayerType() ).ascii() );
        break;
    }
}

void DeepPolyAnalysis::executeDeepPolyElementForInput( DeepPolyElement
                                                       &deepPolyElement )
{
    // Do not need to update symbolic bound, just update the concrete bounds
    ASSERT( deepPolyElement._layer->getLayerType() == Layer::INPUT );
    std::memcpy( deepPolyElement._lb, deepPolyElement._layer->getLbs(),
                 deepPolyElement._layer->getSize() );
    std::memcpy( deepPolyElement._ub, deepPolyElement._layer->getUbs(),
                 deepPolyElement._layer->getSize() );
}

void DeepPolyAnalysis::executeDeepPolyElementForWeightedSum( DeepPolyElement
                                                             &deepPolyElement )
{
    // Do not need to update symbolic bound, just update the concrete bounds
    ASSERT( deepPolyElement._layer->getLayerType() == Layer::WEIGHTED_SUM );
    computeBoundsWithBackSubstitution( deepPolyElement );
}

void DeepPolyAnalysis::computeBoundsWithBackSubstitution( DeepPolyElement
                                                          &deepPolyElement )
{
    auto type = deepPolyElement._type;
    std::fill( _workSymbolicLb, maxLayerSize * maxLayerSize, 0 );
    std::fill( _workSymbolicUb, maxLayerSize * maxLayerSize, 0 );
    std::fill( _workSymbolicLowerBias, maxLayerSize, 0 );
    std::fill( _workSymbolicUpperBias, maxLayerSize, 0 );

    std::memcpy( _workSymbolicLb, _deepPolyElements

    for ( unsigned index = deepPolyElement._layerIndex - 1;
          index > _startLayerIndex; --index )
    {
        if ( type == Layer::INPUT )
        {
           //

        }
        if ( type == Layer::ReLU )
        {
        }
        if ( type == Layer::WEIGHTED_SUM )
        {


        }


    }
}

void DeepPolyAnalysis::executeDeepPolyElementForReLU( DeepPolyElement
                                                      &deepPolyElement )
{
    ASSERT( deepPolyElement._layer->getLayerType() == Layer::RELU );
    DeepPolyElement &previousElement = *_deepPolyElements.end();
    ASSERT( previousElement._size == deepPolyElement._size );
    for ( unsigned i = 0; i < deepPolyElement._size; ++i )
    {
        double sourceLb = previousElement._lb[i];
        double sourceUb = previousElement._ub[i];

        if ( !FloatUtils::isNegative( sourceLb ) )
        {
            // Phase active
            deepPolyElement._symbolicUb[i] = 1;
            deepPolyElement._symbolicUpperBias[i] = 0;
            deepPolyElement._ub[i] = sourceUb;

            deepPolyElement._symbolicLb[i] = 1;
            deepPolyElement._symbolicLowerBias[i] = 0;
            deepPolyElement._lb[i] = sourceUb;
        }
        else if ( !FloatUtils::isPositive( sourceUb ) )
        {
            // Phase active
            deepPolyElement._symbolicUb[i] = 0;
            deepPolyElement._symbolicUpperBias[i] = 0;
            deepPolyElement._ub[i] = 0;

            deepPolyElement._symbolicLb[i] = 0;
            deepPolyElement._symbolicLowerBias[i] = 0;
            deepPolyElement._lb[i] = 0;
        }
        else
        {
            // ReLU not fixed

            // Set symbolic upper bound
            // x_f <= (x_b - l) * u / ( u - l)
            double coeff = sourceUb / ( sourceUb - sourceLb );
            deepPolyElement._symbolicUb[i] = coeff;
            deepPolyElement._symbolicUpperBias[i] = -sourceLb * coeff;
            deepPolyElement._ub[i] = sourceUb;

            // Set symbolic lower bound
            if ( sourceUb > -sourceLb )
            {
                // x_f >= x_b
                // l = sourceLb
                deepPolyElement._symbolicLb[i] = 1;
                deepPolyElement._symbolicLowerBias[i] = 0;
                deepPolyElement._lb[i] = sourceLb;
            }
            else
            {
                // x_f >= 0
                // l = 0
                deepPolyElement._symbolicLb[i] = 0;
                deepPolyElement._symbolicLowerBias[i] = 0;
                deepPolyElement._lb[i] = 0;

            }
        }
    }
}


} // namespace NLR
